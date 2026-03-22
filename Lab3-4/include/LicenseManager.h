#pragma once
#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include <vector>
#include <string>
#include "LicenseKey.h"

class LicenseManager {
private:
    std::vector<LicenseKey> licenses;
    std::string currentMachineID;

    bool verifyFromFile(const std::string& filename, const std::string& programID);

public:
    LicenseManager();

    // Compute FNV-1a hash of a program executable file
    std::string computeProgramHash(const std::string& filePath);

    LicenseKey createPerpetualLicense(const std::string& userName,
                                      const std::string& programPath,
                                      const std::string& programName);

    LicenseKey createTemporaryLicense(const std::string& userName, int days,
                                      const std::string& programPath,
                                      const std::string& programName);

    bool verifyLicense(const std::string& licenseKey, const std::string& programPath);

    bool saveToFile(const LicenseKey& license, const std::string& filename);

    bool saveLicenseByKey(const std::string& licenseKey, const std::string& filename);

    void listAllLicenses();

    std::string getCurrentMachineID() { return currentMachineID; }
};

#endif
