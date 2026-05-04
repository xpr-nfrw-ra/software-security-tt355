#pragma once
#ifndef CIPHER_H
#define CIPHER_H

#include <vector>
#include <string>

// AES-256-CBC encryption with PKCS7 padding
// Key derived from password via SHA-256
// Random IV prepended to ciphertext
//
// File format v2: [4:'PUSB'][1:flags][16:IV][encrypted data]
//   flags bit 0 = compressed (XPRESS_HUFF before encryption)
// File format v1 (legacy): [16:IV][encrypted data]
class AESCipher {
private:
    std::vector<unsigned char> key; // 32 bytes (AES-256)

    static std::vector<unsigned char> deriveKey(const std::string& password);
    static std::vector<unsigned char> generateIV();

public:
    AESCipher(const std::string& password);

    // In-memory encrypt/decrypt
    std::vector<char> encrypt(const std::vector<char>& data);
    std::vector<char> decrypt(const std::vector<char>& data);

    // File encrypt/decrypt with chunked I/O
    // compress: if true, compress data before encryption (XPRESS_HUFF)
    bool encryptFile(const std::string& inputPath, const std::string& outputPath,
        bool compress = false);

    // Auto-detects v1/v2 format and compression
    bool decryptFile(const std::string& inputPath, const std::string& outputPath);
};

#endif
