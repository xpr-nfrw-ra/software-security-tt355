#pragma once

#ifndef LICENSEKEY_H
#define LICENSEKEY_H

#include <string>
#include <ctime>

class LicenseKey {
private:
    std::string key;
    std::string machineID;
    std::string programID;
    std::string programName;
    time_t issueDate;
    time_t expiryDate;
    std::string licensee;
    bool isTemporary;

    void generateKey();

public:
    // Perpetual license
    LicenseKey(const std::string& machine, const std::string& user,
               const std::string& progID, const std::string& progName);

    // Temporary license (daysValid days)
    LicenseKey(const std::string& machine, const std::string& user, int daysValid,
               const std::string& progID, const std::string& progName);

    bool isValid(const std::string& currentMachineID, const std::string& currentProgramID);

    void printInfo();

    std::string getKey() const { return key; }
    std::string getMachineID() const { return machineID; }
    std::string getProgramID() const { return programID; }
    std::string getProgramName() const { return programName; }
    std::string getLicensee() const { return licensee; }
    bool isTemporaryLicense() const { return isTemporary; }
};

#endif
