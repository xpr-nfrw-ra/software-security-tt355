#include <iostream>
#include <string>
#include "LicenseManager.h"

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "    LICENSE SYSTEM - LABORATORY WORK    " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;

    LicenseManager manager;

    int choice;
    std::string userName, programName, programPath;

    do {
        std::cout << "\n--- SELECT ACTION ---" << std::endl;
        std::cout << "1. Create perpetual license" << std::endl;
        std::cout << "2. Create temporary license" << std::endl;
        std::cout << "3. Verify license" << std::endl;
        std::cout << "4. Show all licenses" << std::endl;
        std::cout << "5. Save license to file" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "Your choice: ";
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
        case 1: {
            std::cout << "Enter licensee name: ";
            std::getline(std::cin, userName);
            std::cout << "Enter program name (e.g., Visual Studio): ";
            std::getline(std::cin, programName);
            std::cout << "Enter program executable path: ";
            std::getline(std::cin, programPath);

            LicenseKey license = manager.createPerpetualLicense(userName, programPath, programName);
            std::cout << "\nPERPETUAL LICENSE CREATED:" << std::endl;
            license.printInfo();
            break;
        }

        case 2: {
            std::cout << "Enter licensee name: ";
            std::getline(std::cin, userName);
            std::cout << "Enter program name (e.g., Visual Studio): ";
            std::getline(std::cin, programName);
            std::cout << "Enter program executable path: ";
            std::getline(std::cin, programPath);

            int days = 0;
            std::cout << "\nSelect license duration:" << std::endl;
            std::cout << "1.  1 day" << std::endl;
            std::cout << "2.  2 days" << std::endl;
            std::cout << "3.  7 days" << std::endl;
            std::cout << "4.  14 days" << std::endl;
            std::cout << "5.  30 days" << std::endl;
            std::cout << "6.  Custom" << std::endl;
            std::cout << "Your choice: ";

            int durationChoice;
            std::cin >> durationChoice;
            std::cin.ignore();

            switch (durationChoice) {
                case 1: days = 1; break;
                case 2: days = 2; break;
                case 3: days = 7; break;
                case 4: days = 14; break;
                case 5: days = 30; break;
                case 6: {
                    std::cout << "Enter number of days: ";
                    std::cin >> days;
                    std::cin.ignore();
                    break;
                }
                default:
                    std::cout << "Invalid choice, defaulting to 30 days." << std::endl;
                    days = 30;
                    break;
            }

            LicenseKey license = manager.createTemporaryLicense(userName, days, programPath, programName);
            std::cout << "\nTEMPORARY LICENSE CREATED (" << days << " days):" << std::endl;
            license.printInfo();
            break;
        }

        case 3: {
            std::string key;
            std::cout << "Enter license key: ";
            std::getline(std::cin, key);
            std::cout << "Enter program executable path: ";
            std::getline(std::cin, programPath);

            if (manager.verifyLicense(key, programPath)) {
                std::cout << "LICENSE IS VALID" << std::endl;
            }
            else {
                std::cout << "LICENSE IS INVALID OR NOT FOUND" << std::endl;
            }
            break;
        }

        case 4: {
            manager.listAllLicenses();
            break;
        }

        case 5: {
            std::string key, filename;
            std::cout << "Enter license key: ";
            std::getline(std::cin, key);
            std::cout << "Enter filename: ";
            std::getline(std::cin, filename);

            if (manager.saveLicenseByKey(key, filename)) {
                std::cout << "License saved to " << filename << std::endl;
            }
            else {
                std::cout << "Error saving to file" << std::endl;
            }
            break;
        }

        case 0: {
            std::cout << "Program finished." << std::endl;
            break;
        }

        default: {
            std::cout << "Invalid choice." << std::endl;
            break;
        }
        }

    } while (choice != 0);

    return 0;
}
