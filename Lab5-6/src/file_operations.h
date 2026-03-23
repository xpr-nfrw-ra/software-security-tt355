#pragma once
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <string>
#include <vector>

class FileOperations {
public:
    // Ֆայլերի ցուցակում
    static void listFiles(const std::string& path, bool showHidden = false);

    // Ֆայլի չափ
    static long getFileSize(const std::string& path);

    // Ստուգել արդյոք ֆայլը գոյություն ունի
    static bool fileExists(const std::string& path);

    // Ստեղծել թղթապանակ
    static bool createDirectory(const std::string& path);

    // Ջնջել ֆայլը
    static bool deleteFile(const std::string& path);

    // Վերանվանել/տեղափոխել ֆայլը
    static bool renameFile(const std::string& oldPath, const std::string& newPath);

    // Հավաքել բոլոր ֆայլերը դիրեկտորիայում (ռեկուրսիվ)
    static std::vector<std::string> collectFiles(const std::string& directory);

    // Գտնել ֆայլերն ըստ ընդլայնման
    static std::vector<std::string> findFilesByExtension(const std::string& directory,
        const std::string& extension);
};

#endif
