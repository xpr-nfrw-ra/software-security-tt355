#include "cipher.h"
#include "compression.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <stdexcept>

#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt")

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

static const ULONG AES_BLOCK_SIZE = 16;
static const ULONG AES_KEY_SIZE = 32;
static const size_t CHUNK_SIZE = 64 * 1024; // 64 KB
static const char FILE_MAGIC[4] = { 'P', 'U', 'S', 'B' };

// RAII helper for BCrypt handles
struct BCryptContext {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    ~BCryptContext() {
        if (hKey) BCryptDestroyKey(hKey);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    }
};

// --- Key derivation and IV generation ---

std::vector<unsigned char> AESCipher::deriveKey(const std::string& password) {
    std::vector<unsigned char> hash(32);
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;

    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status))
        throw std::runtime_error("Cannot open SHA-256 provider");

    status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status)) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        throw std::runtime_error("Cannot create hash object");
    }

    BCryptHashData(hHash, (PUCHAR)password.data(), (ULONG)password.size(), 0);
    BCryptFinishHash(hHash, hash.data(), 32, 0);

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return hash;
}

std::vector<unsigned char> AESCipher::generateIV() {
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    NTSTATUS status = BCryptGenRandom(NULL, iv.data(), AES_BLOCK_SIZE,
        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!NT_SUCCESS(status))
        throw std::runtime_error("Cannot generate random IV");
    return iv;
}

AESCipher::AESCipher(const std::string& password) : key(deriveKey(password)) {}

// --- Helper: initialize BCrypt AES-256-CBC context ---

static bool initAES(BCryptContext& ctx, const std::vector<unsigned char>& key) {
    NTSTATUS status = BCryptOpenAlgorithmProvider(&ctx.hAlg,
        BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!NT_SUCCESS(status)) return false;

    status = BCryptSetProperty(ctx.hAlg, BCRYPT_CHAINING_MODE,
        (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    if (!NT_SUCCESS(status)) return false;

    status = BCryptGenerateSymmetricKey(ctx.hAlg, &ctx.hKey, NULL, 0,
        (PUCHAR)key.data(), AES_KEY_SIZE, 0);
    if (!NT_SUCCESS(status)) return false;

    return true;
}

// --- In-memory encrypt/decrypt ---

std::vector<char> AESCipher::encrypt(const std::vector<char>& data) {
    BCryptContext ctx;
    if (!initAES(ctx, key)) return {};

    std::vector<unsigned char> iv = generateIV();
    std::vector<unsigned char> ivCopy = iv;

    ULONG outSize = ((ULONG)data.size() / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
    std::vector<unsigned char> ciphertext(outSize);

    ULONG resultSize = 0;
    NTSTATUS status = BCryptEncrypt(ctx.hKey,
        (PUCHAR)data.data(), (ULONG)data.size(), NULL,
        ivCopy.data(), AES_BLOCK_SIZE,
        ciphertext.data(), outSize, &resultSize,
        BCRYPT_BLOCK_PADDING);

    if (!NT_SUCCESS(status)) return {};

    // Return IV + ciphertext
    std::vector<char> result(AES_BLOCK_SIZE + resultSize);
    memcpy(result.data(), iv.data(), AES_BLOCK_SIZE);
    memcpy(result.data() + AES_BLOCK_SIZE, ciphertext.data(), resultSize);
    return result;
}

std::vector<char> AESCipher::decrypt(const std::vector<char>& data) {
    if (data.size() < AES_BLOCK_SIZE * 2) return {};

    BCryptContext ctx;
    if (!initAES(ctx, key)) return {};

    // Extract IV from front
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    memcpy(iv.data(), data.data(), AES_BLOCK_SIZE);

    ULONG cipherLen = (ULONG)(data.size() - AES_BLOCK_SIZE);
    std::vector<unsigned char> plaintext(cipherLen);

    ULONG resultSize = 0;
    NTSTATUS status = BCryptDecrypt(ctx.hKey,
        (PUCHAR)(data.data() + AES_BLOCK_SIZE), cipherLen, NULL,
        iv.data(), AES_BLOCK_SIZE,
        plaintext.data(), cipherLen, &resultSize,
        BCRYPT_BLOCK_PADDING);

    if (!NT_SUCCESS(status)) return {};

    return std::vector<char>(plaintext.begin(), plaintext.begin() + resultSize);
}

// --- File encryption with v2 format ---

bool AESCipher::encryptFile(const std::string& inputPath, const std::string& outputPath,
    bool compress) {
    try {
        std::ifstream inFile(inputPath, std::ios::binary);
        if (!inFile) {
            std::cerr << "Error: Cannot open file: " << inputPath << std::endl;
            return false;
        }

        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            std::cerr << "Error: Cannot create output file: " << outputPath << std::endl;
            return false;
        }

        // Get input file size
        inFile.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        // Write v2 header: magic + flags
        outFile.write(FILE_MAGIC, 4);
        uint8_t flags = compress ? 0x01 : 0x00;
        outFile.write(reinterpret_cast<char*>(&flags), 1);

        // Generate IV and write it
        std::vector<unsigned char> iv = generateIV();
        outFile.write(reinterpret_cast<char*>(iv.data()), AES_BLOCK_SIZE);

        // Setup AES-256-CBC
        BCryptContext ctx;
        if (!initAES(ctx, key)) {
            std::cerr << "Error: Failed to initialize AES" << std::endl;
            return false;
        }

        std::vector<unsigned char> ivCopy = iv;

        if (compress) {
            // Compressed path: read entire file, compress, encrypt in one shot
            std::vector<unsigned char> fileData(fileSize);
            if (fileSize > 0) {
                inFile.read(reinterpret_cast<char*>(fileData.data()), fileSize);
            }
            inFile.close();

            // Build payload: [8-byte original size][compressed data]
            std::vector<unsigned char> payload;
            if (fileSize > 0) {
                auto compressed = Compression::compress(fileData.data(), fileSize);
                payload.resize(8 + compressed.size());
                uint64_t origSize = static_cast<uint64_t>(fileSize);
                memcpy(payload.data(), &origSize, 8);
                memcpy(payload.data() + 8, compressed.data(), compressed.size());
            }
            // Empty file: payload stays empty (produces one padding block)

            ULONG maxOut = ((ULONG)payload.size() / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
            std::vector<unsigned char> ciphertext(maxOut);
            ULONG resultSize = 0;

            NTSTATUS status = BCryptEncrypt(ctx.hKey,
                payload.data(), (ULONG)payload.size(), NULL,
                ivCopy.data(), AES_BLOCK_SIZE,
                ciphertext.data(), maxOut, &resultSize,
                BCRYPT_BLOCK_PADDING);

            if (!NT_SUCCESS(status)) {
                std::cerr << "Error: Encryption failed" << std::endl;
                return false;
            }

            outFile.write(reinterpret_cast<char*>(ciphertext.data()), resultSize);
        }
        else {
            // Non-compressed: chunked encrypt
            std::vector<unsigned char> readBuf(CHUNK_SIZE);
            std::vector<unsigned char> outBuf(CHUNK_SIZE + AES_BLOCK_SIZE);

            if (fileSize == 0) {
                // Empty file: produce one padding block
                ULONG resultSize = 0;
                NTSTATUS status = BCryptEncrypt(ctx.hKey,
                    readBuf.data(), 0, NULL,
                    ivCopy.data(), AES_BLOCK_SIZE,
                    outBuf.data(), AES_BLOCK_SIZE, &resultSize,
                    BCRYPT_BLOCK_PADDING);
                if (!NT_SUCCESS(status)) {
                    std::cerr << "Error: Encryption failed" << std::endl;
                    return false;
                }
                outFile.write(reinterpret_cast<char*>(outBuf.data()), resultSize);
            }
            else {
                size_t totalRead = 0;
                while (totalRead < fileSize) {
                    size_t toRead = std::min(CHUNK_SIZE, fileSize - totalRead);
                    inFile.read(reinterpret_cast<char*>(readBuf.data()), toRead);
                    size_t bytesRead = static_cast<size_t>(inFile.gcount());
                    totalRead += bytesRead;

                    bool isLastChunk = (totalRead >= fileSize);
                    ULONG encFlags = isLastChunk ? BCRYPT_BLOCK_PADDING : 0;
                    ULONG maxOut = (ULONG)bytesRead + AES_BLOCK_SIZE;

                    if (outBuf.size() < maxOut)
                        outBuf.resize(maxOut);

                    ULONG resultSize = 0;
                    NTSTATUS status = BCryptEncrypt(ctx.hKey,
                        readBuf.data(), (ULONG)bytesRead, NULL,
                        ivCopy.data(), AES_BLOCK_SIZE,
                        outBuf.data(), maxOut, &resultSize,
                        encFlags);

                    if (!NT_SUCCESS(status)) {
                        std::cerr << "Error: Encryption failed" << std::endl;
                        return false;
                    }

                    outFile.write(reinterpret_cast<char*>(outBuf.data()), resultSize);
                }
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error during encryption: " << e.what() << std::endl;
        return false;
    }
}

// --- File decryption with v1/v2 auto-detection ---

bool AESCipher::decryptFile(const std::string& inputPath, const std::string& outputPath) {
    try {
        std::ifstream inFile(inputPath, std::ios::binary);
        if (!inFile) {
            std::cerr << "Error: Cannot open file: " << inputPath << std::endl;
            return false;
        }

        // Get file size
        inFile.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        if (fileSize < AES_BLOCK_SIZE * 2) {
            std::cerr << "Error: File too small to be a valid encrypted file" << std::endl;
            return false;
        }

        // Detect format: read first 4 bytes
        char header[4];
        inFile.read(header, 4);

        bool isV2 = false;
        bool isCompressed = false;
        std::vector<unsigned char> iv(AES_BLOCK_SIZE);
        size_t dataSize;

        if (fileSize >= 37 && memcmp(header, FILE_MAGIC, 4) == 0) {
            // v2 format: [PUSB][flags][IV][data]
            isV2 = true;
            uint8_t flags;
            inFile.read(reinterpret_cast<char*>(&flags), 1);
            isCompressed = (flags & 0x01) != 0;

            inFile.read(reinterpret_cast<char*>(iv.data()), AES_BLOCK_SIZE);
            dataSize = fileSize - 4 - 1 - AES_BLOCK_SIZE; // 21-byte header
        }
        else {
            // v1 legacy format: first 4 bytes are start of IV
            memcpy(iv.data(), header, 4);
            inFile.read(reinterpret_cast<char*>(iv.data() + 4), AES_BLOCK_SIZE - 4);
            dataSize = fileSize - AES_BLOCK_SIZE;
        }

        if (dataSize < AES_BLOCK_SIZE || dataSize % AES_BLOCK_SIZE != 0) {
            std::cerr << "Error: Invalid encrypted data size" << std::endl;
            return false;
        }

        // Setup AES-256-CBC
        BCryptContext ctx;
        if (!initAES(ctx, key)) {
            std::cerr << "Error: Failed to initialize AES" << std::endl;
            return false;
        }

        std::vector<unsigned char> ivCopy = iv;

        if (isCompressed) {
            // Compressed: decrypt all to memory, then decompress
            std::vector<unsigned char> encData(dataSize);
            inFile.read(reinterpret_cast<char*>(encData.data()), dataSize);

            std::vector<unsigned char> plaintext(dataSize);
            ULONG resultSize = 0;

            NTSTATUS status = BCryptDecrypt(ctx.hKey,
                encData.data(), (ULONG)dataSize, NULL,
                ivCopy.data(), AES_BLOCK_SIZE,
                plaintext.data(), (ULONG)dataSize, &resultSize,
                BCRYPT_BLOCK_PADDING);

            if (!NT_SUCCESS(status)) {
                std::cerr << "Error: Decryption failed (wrong key or corrupted file)"
                    << std::endl;
                return false;
            }

            // Payload: [8-byte original size][compressed data]
            if (resultSize < 8) {
                std::cerr << "Error: Decrypted data too small for compression header"
                    << std::endl;
                return false;
            }

            uint64_t originalSize;
            memcpy(&originalSize, plaintext.data(), 8);

            auto decompressed = Compression::decompress(
                plaintext.data() + 8, resultSize - 8,
                static_cast<size_t>(originalSize));

            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile) {
                std::cerr << "Error: Cannot create output file: " << outputPath << std::endl;
                return false;
            }
            outFile.write(reinterpret_cast<char*>(decompressed.data()), decompressed.size());
        }
        else {
            // Non-compressed: chunked decrypt
            std::ofstream outFile(outputPath, std::ios::binary);
            if (!outFile) {
                std::cerr << "Error: Cannot create output file: " << outputPath << std::endl;
                return false;
            }

            std::vector<unsigned char> readBuf(CHUNK_SIZE);
            std::vector<unsigned char> outBuf(CHUNK_SIZE);
            size_t totalRead = 0;

            while (totalRead < dataSize) {
                size_t toRead = std::min(CHUNK_SIZE, dataSize - totalRead);
                inFile.read(reinterpret_cast<char*>(readBuf.data()), toRead);
                size_t bytesRead = static_cast<size_t>(inFile.gcount());
                totalRead += bytesRead;

                bool isLastChunk = (totalRead >= dataSize);
                ULONG decFlags = isLastChunk ? BCRYPT_BLOCK_PADDING : 0;

                if (outBuf.size() < bytesRead)
                    outBuf.resize(bytesRead);

                ULONG resultSize = 0;
                NTSTATUS status = BCryptDecrypt(ctx.hKey,
                    readBuf.data(), (ULONG)bytesRead, NULL,
                    ivCopy.data(), AES_BLOCK_SIZE,
                    outBuf.data(), (ULONG)outBuf.size(), &resultSize,
                    decFlags);

                if (!NT_SUCCESS(status)) {
                    std::cerr << "Error: Decryption failed (wrong key or corrupted file)"
                        << std::endl;
                    return false;
                }

                outFile.write(reinterpret_cast<char*>(outBuf.data()), resultSize);
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error during decryption: " << e.what() << std::endl;
        return false;
    }
}
