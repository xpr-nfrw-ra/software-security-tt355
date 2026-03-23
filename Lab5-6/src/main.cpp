#include "usb_detector.h"
#include "file_operations.h"
#include "cipher.h"
#include "utils.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <ctime>
#include <future>
#include <mutex>
#include <atomic>

#include <filesystem>
namespace fs = std::filesystem;

// --- CLI option parsing ---

struct Options {
    std::string keyFile;   // --key-file <path>
    bool compress = false; // --compress
    bool parallel = false; // --parallel
};

Options parseOptions(int argc, char* argv[], int startIdx) {
    Options opts;
    for (int i = startIdx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--key-file" && i + 1 < argc) {
            opts.keyFile = argv[++i];
        }
        else if (arg == "--compress") {
            opts.compress = true;
        }
        else if (arg == "--parallel") {
            opts.parallel = true;
        }
    }
    return opts;
}

std::string getKey(const Options& opts, const std::string& prompt) {
    if (!opts.keyFile.empty()) {
        std::string key = Utils::readKeyFromFile(opts.keyFile);
        if (!key.empty()) {
            std::cout << "Key loaded from file: " << opts.keyFile << std::endl;
        }
        return key;
    }
    return Utils::getPassword(prompt);
}

// --- Help ---

void showHelp() {
    std::cout << "\n";
    std::cout << "ProtectUSB - USB Drive Content Protection\n";
    std::cout << "==========================================\n\n";
    std::cout << "Usage:\n";
    std::cout << "  protectusb [command] [arguments] [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  scan              - Detect connected USB and DVD drives\n";
    std::cout << "  list <path>       - List files in specified path\n";
    std::cout << "  protect <path>    - Protect USB drive (encrypt all files)\n";
    std::cout << "  unprotect <path>  - Restore USB drive (decrypt all files)\n";
    std::cout << "  encrypt <file>    - Encrypt a single file\n";
    std::cout << "  decrypt <file>    - Decrypt a single file\n";
    std::cout << "  info <path>       - Show drive information\n";
    std::cout << "  help              - Show this help\n\n";
    std::cout << "Options:\n";
    std::cout << "  --key-file <path> - Read encryption key from file\n";
    std::cout << "  --compress        - Compress files before encryption\n";
    std::cout << "  --parallel        - Encrypt/decrypt files in parallel\n\n";
    std::cout << "Examples:\n";
    std::cout << "  protectusb scan\n";
    std::cout << "  protectusb list D:\\\n";
    std::cout << "  protectusb protect D:\\ --compress --parallel\n";
    std::cout << "  protectusb protect D:\\ --key-file key.txt\n";
    std::cout << "  protectusb encrypt secret.doc --compress\n";
}

// --- Config file for protected state ---

void writeProtectionConfig(const std::string& drivePath, int fileCount, bool compressed) {
    std::string configPath = drivePath + "\\config.dat";
    std::ofstream config(configPath);
    if (config) {
        time_t now = time(NULL);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));

        config << "[ProtectUSB]\n";
        config << "Status=Protected\n";
        config << "Timestamp=" << timeStr << "\n";
        config << "FileCount=" << fileCount << "\n";
        config << "Algorithm=AES-256-CBC\n";
        config << "Compression=" << (compressed ? "XPRESS_HUFF" : "None") << "\n";
    }
}

void removeProtectionConfig(const std::string& drivePath) {
    std::string configPath = drivePath + "\\config.dat";
    FileOperations::deleteFile(configPath);
}

// --- Protect / Unprotect ---

bool protectUSBDrive(const std::string& drivePath, const std::string& key,
    bool compress, bool parallel) {
    std::cout << "\nProtecting USB drive: " << drivePath << std::endl;
    std::cout << "--------------------------------\n";

    if (compress)  std::cout << "Compression: enabled\n";
    if (parallel)  std::cout << "Parallel mode: enabled\n";

    // Check if already protected
    std::string configPath = drivePath + "\\config.dat";
    if (FileOperations::fileExists(configPath)) {
        std::cout << "Warning: Drive appears to already be protected.\n";
        std::cout << "Use 'unprotect' first to decrypt existing files.\n";
        return false;
    }

    // Create Protected folder
    std::string protectedDir = drivePath + "\\Protected";
    if (!FileOperations::fileExists(protectedDir)) {
        FileOperations::createDirectory(protectedDir);
    }

    // Collect all files
    std::vector<std::string> files = FileOperations::collectFiles(drivePath);
    std::vector<std::string> filesToProtect;

    // Filter: skip already encrypted, config.dat, and Protected folder
    for (size_t i = 0; i < files.size(); i++) {
        if (files[i].find(".encrypted") == std::string::npos &&
            files[i].find("config.dat") == std::string::npos &&
            files[i].find("Protected") == std::string::npos) {
            filesToProtect.push_back(files[i]);
        }
    }

    if (filesToProtect.empty()) {
        std::cout << "No files to protect found.\n";
        return true;
    }

    std::cout << "Found " << filesToProtect.size() << " files to encrypt.\n";

    int successCount = 0;

    if (parallel && filesToProtect.size() > 1) {
        // Parallel encryption using std::async
        std::mutex coutMutex;
        std::vector<std::future<bool>> futures;

        for (size_t i = 0; i < filesToProtect.size(); i++) {
            futures.push_back(std::async(std::launch::async,
                [&, i]() -> bool {
                    std::string encryptedFile = filesToProtect[i] + ".encrypted";

                    // Each thread gets its own AESCipher instance
                    AESCipher cipher(key);
                    bool ok = cipher.encryptFile(filesToProtect[i], encryptedFile, compress);

                    if (ok) {
                        if (FileOperations::deleteFile(filesToProtect[i])) {
                            std::lock_guard<std::mutex> lock(coutMutex);
                            std::cout << "Encrypted: " << filesToProtect[i] << " ... OK\n";
                            return true;
                        }
                        else {
                            std::lock_guard<std::mutex> lock(coutMutex);
                            std::cout << "Encrypted: " << filesToProtect[i]
                                << " ... Failed to delete original\n";
                        }
                    }
                    else {
                        std::lock_guard<std::mutex> lock(coutMutex);
                        std::cout << "Encrypting: " << filesToProtect[i]
                            << " ... Encryption failed\n";
                    }
                    return false;
                }));
        }

        // Collect results
        for (auto& f : futures) {
            if (f.get()) successCount++;
        }
    }
    else {
        // Sequential encryption
        AESCipher cipher(key);

        for (size_t i = 0; i < filesToProtect.size(); i++) {
            std::string encryptedFile = filesToProtect[i] + ".encrypted";

            std::cout << "Encrypting: " << filesToProtect[i] << " ... ";

            if (cipher.encryptFile(filesToProtect[i], encryptedFile, compress)) {
                if (FileOperations::deleteFile(filesToProtect[i])) {
                    std::cout << "OK\n";
                    successCount++;
                }
                else {
                    std::cout << "Failed to delete original\n";
                }
            }
            else {
                std::cout << "Encryption failed\n";
            }
        }
    }

    // Write config.dat to mark drive as protected
    writeProtectionConfig(drivePath, successCount, compress);

    std::cout << "\nProtection completed.\n";
    std::cout << "Encrypted " << successCount << " of " << filesToProtect.size() << " files.\n";

    return true;
}

bool unprotectUSBDrive(const std::string& drivePath, const std::string& key, bool parallel) {
    std::cout << "\nUnprotecting USB drive: " << drivePath << std::endl;
    std::cout << "--------------------------------\n";

    if (parallel) std::cout << "Parallel mode: enabled\n";

    // Find all .encrypted files
    std::vector<std::string> encryptedFiles =
        FileOperations::findFilesByExtension(drivePath, ".encrypted");

    if (encryptedFiles.empty()) {
        std::cout << "No encrypted files found.\n";
        return true;
    }

    std::cout << "Found " << encryptedFiles.size() << " encrypted files.\n";

    int successCount = 0;

    if (parallel && encryptedFiles.size() > 1) {
        // Parallel decryption
        std::mutex coutMutex;
        std::vector<std::future<bool>> futures;

        for (size_t i = 0; i < encryptedFiles.size(); i++) {
            futures.push_back(std::async(std::launch::async,
                [&, i]() -> bool {
                    size_t pos = encryptedFiles[i].rfind(".encrypted");
                    std::string originalFile = (pos != std::string::npos)
                        ? encryptedFiles[i].substr(0, pos)
                        : encryptedFiles[i] + ".decrypted";

                    AESCipher cipher(key);
                    bool ok = cipher.decryptFile(encryptedFiles[i], originalFile);

                    if (ok) {
                        if (FileOperations::deleteFile(encryptedFiles[i])) {
                            std::lock_guard<std::mutex> lock(coutMutex);
                            std::cout << "Decrypted: " << encryptedFiles[i] << " ... OK\n";
                            return true;
                        }
                        else {
                            std::lock_guard<std::mutex> lock(coutMutex);
                            std::cout << "Decrypted: " << encryptedFiles[i]
                                << " ... Failed to delete encrypted file\n";
                        }
                    }
                    else {
                        std::lock_guard<std::mutex> lock(coutMutex);
                        std::cout << "Decrypting: " << encryptedFiles[i]
                            << " ... Decryption failed\n";
                    }
                    return false;
                }));
        }

        for (auto& f : futures) {
            if (f.get()) successCount++;
        }
    }
    else {
        // Sequential decryption
        AESCipher cipher(key);

        for (size_t i = 0; i < encryptedFiles.size(); i++) {
            size_t pos = encryptedFiles[i].rfind(".encrypted");
            std::string originalFile = (pos != std::string::npos)
                ? encryptedFiles[i].substr(0, pos)
                : encryptedFiles[i] + ".decrypted";

            std::cout << "Decrypting: " << encryptedFiles[i] << " ... ";

            if (cipher.decryptFile(encryptedFiles[i], originalFile)) {
                if (FileOperations::deleteFile(encryptedFiles[i])) {
                    std::cout << "OK\n";
                    successCount++;
                }
                else {
                    std::cout << "Failed to delete encrypted file\n";
                }
            }
            else {
                std::cout << "Decryption failed\n";
            }
        }
    }

    // Remove Protected folder if empty
    std::string protectedDir = drivePath + "\\Protected";
    if (fs::exists(protectedDir) && fs::is_empty(protectedDir)) {
        fs::remove(protectedDir);
    }

    // Remove config.dat
    removeProtectionConfig(drivePath);

    std::cout << "\nUnprotection completed.\n";
    std::cout << "Decrypted " << successCount << " of " << encryptedFiles.size() << " files.\n";

    return true;
}

// --- Main ---

int main(int argc, char* argv[]) {
    if (argc < 2) {
        showHelp();
        return 1;
    }

    std::string command = argv[1];

    if (command == "help") {
        showHelp();
    }
    else if (command == "scan") {
        std::cout << "Detected USB drives:\n";
        std::cout << "--------------------\n";

        std::vector<std::string> usbDrives = USBDetector::getUSBDrives();

        if (usbDrives.empty()) {
            std::cout << "No USB drives detected.\n";
        }
        else {
            for (size_t i = 0; i < usbDrives.size(); i++) {
                std::cout << "  " << i + 1 << ". " << usbDrives[i] << std::endl;
            }
        }

        std::cout << "\nDetected DVD/CD-ROM drives:\n";
        std::cout << "---------------------------\n";

        std::vector<std::string> dvdDrives = USBDetector::getDVDDrives();

        if (dvdDrives.empty()) {
            std::cout << "No DVD/CD-ROM drives detected.\n";
        }
        else {
            for (size_t i = 0; i < dvdDrives.size(); i++) {
                std::cout << "  " << i + 1 << ". " << dvdDrives[i] << std::endl;
            }
        }
    }
    else if (command == "list") {
        if (argc < 3) {
            std::cerr << "Error: Please specify path\n";
            return 1;
        }
        FileOperations::listFiles(argv[2], false);
    }
    else if (command == "info") {
        if (argc < 3) {
            std::cerr << "Error: Please specify path\n";
            return 1;
        }
        std::string info = USBDetector::getDriveInfo(argv[2]);
        std::cout << info << std::endl;
    }
    else if (command == "protect") {
        if (argc < 3) {
            std::cerr << "Error: Please specify USB drive path\n";
            return 1;
        }

        Options opts = parseOptions(argc, argv, 3);
        std::string key = getKey(opts, "Enter encryption key: ");
        if (key.empty()) {
            std::cerr << "Key cannot be empty.\n";
            return 1;
        }

        protectUSBDrive(argv[2], key, opts.compress, opts.parallel);
    }
    else if (command == "unprotect") {
        if (argc < 3) {
            std::cerr << "Error: Please specify USB drive path\n";
            return 1;
        }

        Options opts = parseOptions(argc, argv, 3);
        std::string key = getKey(opts, "Enter decryption key: ");
        if (key.empty()) {
            std::cerr << "Key cannot be empty.\n";
            return 1;
        }

        unprotectUSBDrive(argv[2], key, opts.parallel);
    }
    else if (command == "encrypt") {
        if (argc < 3) {
            std::cerr << "Error: Please specify file path\n";
            return 1;
        }

        Options opts = parseOptions(argc, argv, 3);
        std::string key = getKey(opts, "Enter encryption key: ");
        if (key.empty()) {
            std::cerr << "Key cannot be empty.\n";
            return 1;
        }

        std::string outputFile = std::string(argv[2]) + ".encrypted";
        AESCipher cipher(key);
        if (cipher.encryptFile(argv[2], outputFile, opts.compress)) {
            std::cout << "File encrypted successfully: " << outputFile << std::endl;
        }
    }
    else if (command == "decrypt") {
        if (argc < 3) {
            std::cerr << "Error: Please specify file path\n";
            return 1;
        }

        Options opts = parseOptions(argc, argv, 3);
        std::string key = getKey(opts, "Enter decryption key: ");
        if (key.empty()) {
            std::cerr << "Key cannot be empty.\n";
            return 1;
        }

        std::string inputFile = argv[2];
        std::string outputFile;

        size_t pos = inputFile.rfind(".encrypted");
        if (pos != std::string::npos) {
            outputFile = inputFile.substr(0, pos);
        }
        else {
            outputFile = inputFile + ".decrypted";
        }

        AESCipher cipher(key);
        if (cipher.decryptFile(inputFile, outputFile)) {
            std::cout << "File decrypted successfully: " << outputFile << std::endl;
        }
    }
    else {
        std::cerr << "Unknown command: " << command << std::endl;
        showHelp();
        return 1;
    }

    return 0;
}
