#include "usb_detector.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

std::vector<std::string> USBDetector::getUSBDrives() {
    std::vector<std::string> usbDrives;

#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            char rootPath[4] = { char('A' + i), ':', '\\', '\0' };
            UINT driveType = GetDriveTypeA(rootPath);
            if (driveType == DRIVE_REMOVABLE) {
                std::string drive(1, 'A' + i);
                drive += ":\\";
                usbDrives.push_back(drive);
            }
        }
    }
#else
    usbDrives.push_back("/media/usb");
    usbDrives.push_back("/mnt/usb");
#endif

    return usbDrives;
}

std::vector<std::string> USBDetector::getDVDDrives() {
    std::vector<std::string> dvdDrives;

#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            char rootPath[4] = { char('A' + i), ':', '\\', '\0' };
            UINT driveType = GetDriveTypeA(rootPath);
            if (driveType == DRIVE_CDROM) {
                std::string drive(1, 'A' + i);
                drive += ":\\";
                dvdDrives.push_back(drive);
            }
        }
    }
#else
    dvdDrives.push_back("/dev/cdrom");
    dvdDrives.push_back("/media/cdrom");
#endif

    return dvdDrives;
}

bool USBDetector::isUSBDrive(const std::string& path) {
#ifdef _WIN32
    if (path.length() >= 2 && path[1] == ':') {
        std::string root = path.substr(0, 3);
        UINT driveType = GetDriveTypeA(root.c_str());
        return (driveType == DRIVE_REMOVABLE);
    }
#endif
    return false;
}

bool USBDetector::isDVDDrive(const std::string& path) {
#ifdef _WIN32
    if (path.length() >= 2 && path[1] == ':') {
        std::string root = path.substr(0, 3);
        UINT driveType = GetDriveTypeA(root.c_str());
        return (driveType == DRIVE_CDROM);
    }
#endif
    return false;
}

std::string USBDetector::getDriveInfo(const std::string& path) {
    char buffer[256] = { 0 };

#ifdef _WIN32
    if (path.length() >= 2 && path[1] == ':') {
        std::string root = path.substr(0, 3);

        DWORD serialNumber;
        if (GetVolumeInformationA(root.c_str(), buffer, sizeof(buffer),
            &serialNumber, NULL, NULL, NULL, 0)) {
            char info[512];
            sprintf(info, "Drive: %s\nVolume Name: %s\nSerial: %08X",
                path.c_str(), buffer, serialNumber);
            return std::string(info);
        }
    }
#endif

    sprintf(buffer, "Drive: %s", path.c_str());
    return std::string(buffer);
}
