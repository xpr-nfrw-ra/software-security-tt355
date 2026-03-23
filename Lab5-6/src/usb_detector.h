#pragma once
#ifndef USB_DETECTOR_H
#define USB_DETECTOR_H

#include <string>
#include <vector>

class USBDetector {
public:
    // Detect connected USB (removable) drives
    static std::vector<std::string> getUSBDrives();

    // Detect connected DVD/CD-ROM drives
    static std::vector<std::string> getDVDDrives();

    // Check if given path is a USB drive
    static bool isUSBDrive(const std::string& path);

    // Check if given path is a DVD/CD-ROM drive
    static bool isDVDDrive(const std::string& path);

    // Get drive information (volume name, serial)
    static std::string getDriveInfo(const std::string& path);
};

#endif
