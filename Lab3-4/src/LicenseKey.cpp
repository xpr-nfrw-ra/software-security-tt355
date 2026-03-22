#include "LicenseKey.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// Perpetual license
LicenseKey::LicenseKey(const std::string& machine, const std::string& user,
                       const std::string& progID, const std::string& progName)
    : machineID(machine), licensee(user), programID(progID), programName(progName),
      isTemporary(false) {
    issueDate = time(nullptr);
    expiryDate = 0;
    generateKey();
}

// Temporary license
LicenseKey::LicenseKey(const std::string& machine, const std::string& user, int daysValid,
                       const std::string& progID, const std::string& progName)
    : machineID(machine), licensee(user), programID(progID), programName(progName),
      isTemporary(true) {
    issueDate = time(nullptr);
    expiryDate = issueDate + (daysValid * 24 * 60 * 60);
    generateKey();
}

void LicenseKey::generateKey() {
    std::stringstream ss;

    // Combine machine ID + program ID + licensee + timestamp
    std::string base = machineID + "|" + programID + "|" + licensee + "|" + std::to_string(issueDate);

    // FNV-1a hash
    unsigned long hash = 2166136261u;
    for (char c : base) {
        hash = (hash * 16777619) ^ c;
    }

    if (isTemporary) {
        hash = (hash * 16777619) ^ expiryDate;
    }

    // Format as XXXXX-XXXXX-XXXXX
    ss << std::hex << std::uppercase;
    for (int i = 0; i < 15; i++) {
        if (i > 0 && i % 5 == 0) ss << "-";
        ss << ((hash >> (i * 4)) & 0xF);
    }

    key = ss.str();
}

bool LicenseKey::isValid(const std::string& currentMachineID, const std::string& currentProgramID) {
    // Check machine ID match
    if (currentMachineID != machineID) {
        return false;
    }

    // Check program ID match
    if (currentProgramID != programID) {
        return false;
    }

    // Check expiry for temporary licenses
    if (isTemporary) {
        time_t now = time(nullptr);
        if (now > expiryDate) {
            return false;
        }
    }

    return true;
}

void LicenseKey::printInfo() {
    std::cout << "======================================" << std::endl;
    std::cout << "License Key:  " << key << std::endl;
    std::cout << "Licensee:     " << licensee << std::endl;
    std::cout << "Program:      " << programName << std::endl;
    std::cout << "Program Hash: " << programID << std::endl;
    std::cout << "Issue Date:   " << ctime(&issueDate);

    if (isTemporary) {
        std::cout << "Valid until:  " << ctime(&expiryDate);
        std::cout << "Type:         TEMPORARY" << std::endl;
    }
    else {
        std::cout << "Type:         PERPETUAL" << std::endl;
    }
    std::cout << "======================================" << std::endl;
}
