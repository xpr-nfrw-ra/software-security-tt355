#include "LicenseManager.h"
#include "HardwareID.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

LicenseManager::LicenseManager() {
    HardwareID hwid;
    currentMachineID = hwid.getMachineID();

    std::cout << "Current Device ID: " << currentMachineID << std::endl;
    std::cout << "CPU ID: " << hwid.getCpuId() << std::endl;
    std::cout << "MAC address: " << hwid.getMacAddress() << std::endl;
    std::cout << std::endl;
}

std::string LicenseManager::computeProgramHash(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Warning: Could not open file: " << filePath << std::endl;
        return "FILE_NOT_FOUND";
    }

    // FNV-1a hash of file contents
    unsigned long long hash = 14695981039346656037ULL;
    char buffer[4096];

    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        std::streamsize bytesRead = file.gcount();
        for (std::streamsize i = 0; i < bytesRead; i++) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= 1099511628211ULL;
        }
    }

    file.close();

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

LicenseKey LicenseManager::createPerpetualLicense(const std::string& userName,
                                                   const std::string& programPath,
                                                   const std::string& programName) {
    std::string progHash = computeProgramHash(programPath);
    LicenseKey license(currentMachineID, userName, progHash, programName);
    licenses.push_back(license);
    return license;
}

LicenseKey LicenseManager::createTemporaryLicense(const std::string& userName, int days,
                                                   const std::string& programPath,
                                                   const std::string& programName) {
    std::string progHash = computeProgramHash(programPath);
    LicenseKey license(currentMachineID, userName, days, progHash, programName);
    licenses.push_back(license);
    return license;
}

bool LicenseManager::verifyLicense(const std::string& licenseKey, const std::string& programPath) {
    std::string progHash = computeProgramHash(programPath);

    // Check in-memory licenses first
    for (auto& license : licenses) {
        if (license.getKey() == licenseKey) {
            return license.isValid(currentMachineID, progHash);
        }
    }

    // Fall back to file-based verification
    return verifyFromFile(licenseKey, progHash);
}

bool LicenseManager::saveToFile(const LicenseKey& license, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << "KEY=" << license.getKey() << std::endl;
    file << "MACHINE=" << license.getMachineID() << std::endl;
    file << "TYPE=" << (license.isTemporaryLicense() ? "TEMPORARY" : "PERPETUAL") << std::endl;
    file << "PROGRAM_NAME=" << license.getProgramName() << std::endl;
    file << "PROGRAM_ID=" << license.getProgramID() << std::endl;
    file << "LICENSEE=" << license.getLicensee() << std::endl;

    file.close();
    return true;
}

bool LicenseManager::verifyFromFile(const std::string& filename, const std::string& programID) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    std::string savedKey, savedMachine, savedType, savedProgramID, savedProgramName;

    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (name == "KEY") savedKey = value;
            else if (name == "MACHINE") savedMachine = value;
            else if (name == "TYPE") savedType = value;
            else if (name == "PROGRAM_ID") savedProgramID = value;
            else if (name == "PROGRAM_NAME") savedProgramName = value;
        }
    }

    file.close();

    // Check machine ID
    if (savedMachine != currentMachineID) {
        std::cout << "ERROR: License is not for this machine!" << std::endl;
        std::cout << "  Expected: " << savedMachine << std::endl;
        std::cout << "  Current:  " << currentMachineID << std::endl;
        return false;
    }

    // Check program ID
    if (savedProgramID != programID) {
        std::cout << "ERROR: License is not for this program!" << std::endl;
        std::cout << "  Licensed program: " << savedProgramName << std::endl;
        std::cout << "  Expected hash: " << savedProgramID << std::endl;
        std::cout << "  Current hash:  " << programID << std::endl;
        return false;
    }

    return true;
}

bool LicenseManager::saveLicenseByKey(const std::string& licenseKey, const std::string& filename) {
    for (auto& license : licenses) {
        if (license.getKey() == licenseKey) {
            return saveToFile(license, filename);
        }
    }
    std::cout << "License key not found in memory." << std::endl;
    return false;
}

void LicenseManager::listAllLicenses() {
    if (licenses.empty()) {
        std::cout << "No licenses found." << std::endl;
        return;
    }

    std::cout << "\nALL LICENSES:" << std::endl;
    for (auto& license : licenses) {
        license.printInfo();
    }
}
