#include "file_operations.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#define RMDIR(dir) _rmdir(dir)
#else
#include <unistd.h>
#include <dirent.h>
#define MKDIR(dir) mkdir(dir, 0755)
#endif

void FileOperations::listFiles(const std::string& path, bool showHidden) {
    std::cout << "\nContents of " << path << ":\n";
    std::cout << "--------------------------------\n";

#ifdef _WIN32
    std::string searchPath = path + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Բաց թողնել . և ..
            if (strcmp(findData.cFileName, ".") == 0 ||
                strcmp(findData.cFileName, "..") == 0) {
                continue;
            }

            // Թաքնված ֆայլերի ստուգում
            if (!showHidden && findData.cFileName[0] == '.') {
                continue;
            }

            const char* type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                ? "[DIR]" : "[FILE]";

            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;

            std::cout << std::left << std::setw(8) << type
                << std::setw(30) << findData.cFileName
                << std::right << std::setw(12) << fileSize.QuadPart << " bytes\n";

        } while (FindNextFileA(hFind, &findData) != 0);

        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            if (!showHidden && entry->d_name[0] == '.') {
                continue;
            }

            const char* type = (entry->d_type == DT_DIR) ? "[DIR]" : "[FILE]";
            std::string fullPath = path + "/" + entry->d_name;
            struct stat st;
            long size = 0;

            if (stat(fullPath.c_str(), &st) == 0) {
                size = st.st_size;
            }

            std::cout << std::left << std::setw(8) << type
                << std::setw(30) << entry->d_name
                << std::right << std::setw(12) << size << " bytes\n";
        }
        closedir(dir);
    }
#endif
}

long FileOperations::getFileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return -1;
}

bool FileOperations::fileExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

bool FileOperations::createDirectory(const std::string& path) {
    if (MKDIR(path.c_str()) == 0) {
        return true;
    }
    return false;
}

bool FileOperations::deleteFile(const std::string& path) {
    if (remove(path.c_str()) == 0) {
        return true;
    }
    return false;
}

bool FileOperations::renameFile(const std::string& oldPath, const std::string& newPath) {
    if (rename(oldPath.c_str(), newPath.c_str()) == 0) {
        return true;
    }
    return false;
}

std::vector<std::string> FileOperations::collectFiles(const std::string& directory) {
    std::vector<std::string> files;

#ifdef _WIN32
    std::string searchPath = directory + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") == 0 ||
                strcmp(findData.cFileName, "..") == 0) {
                continue;
            }

            std::string fullPath = directory + "\\" + findData.cFileName;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // Ռեկուրսիվ կերպով հավաքել ենթաթղթապանակների ֆայլերը
                std::vector<std::string> subFiles = collectFiles(fullPath);
                files.insert(files.end(), subFiles.begin(), subFiles.end());
            }
            else {
                files.push_back(fullPath);
            }
        } while (FindNextFileA(hFind, &findData) != 0);
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(directory.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            std::string fullPath = directory + "/" + entry->d_name;

            if (entry->d_type == DT_DIR) {
                std::vector<std::string> subFiles = collectFiles(fullPath);
                files.insert(files.end(), subFiles.begin(), subFiles.end());
            }
            else {
                files.push_back(fullPath);
            }
        }
        closedir(dir);
    }
#endif

    return files;
}

std::vector<std::string> FileOperations::findFilesByExtension(const std::string& directory,
    const std::string& extension) {
    std::vector<std::string> allFiles = collectFiles(directory);
    std::vector<std::string> result;

    for (size_t i = 0; i < allFiles.size(); i++) {
        if (allFiles[i].length() >= extension.length()) {
            std::string ext = allFiles[i].substr(allFiles[i].length() - extension.length());
            if (ext == extension) {
                result.push_back(allFiles[i]);
            }
        }
    }

    return result;
}
