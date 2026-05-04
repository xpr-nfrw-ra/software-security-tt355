#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <conio.h>  // Windows (getch)

#ifdef _WIN32
#include <windows.h>
#endif

void Utils::printColor(const std::string& text, const std::string& color) {
    std::cout << text;
}

std::string Utils::getCurrentTime() {
    time_t now = time(0);
    struct tm* tstruct = localtime(&now);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", tstruct);
    return std::string(buf);
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

void Utils::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void Utils::pressAnyKey() {
    std::cout << "\nPress any key to continue...";
    _getch();
    std::cout << std::endl;
}

std::string Utils::center(const std::string& text, int width) {
    size_t len = text.length();
    if (width <= len) {
        return text;
    }

    size_t left = (width - len) / 2;
    size_t right = width - len - left;

    return std::string(left, ' ') + text + std::string(right, ' ');
}

std::string Utils::getUserInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

std::string Utils::getPassword(const std::string& prompt) {
    std::cout << prompt;
    std::string password;
    char ch;

    while ((ch = _getch()) != '\r') {  // Enter
        if (ch == '\b') {  // Backspace
            if (!password.empty()) {
                std::cout << "\b \b";
                password.pop_back();
            }
        }
        else {
            password.push_back(ch);
            std::cout << '*';
        }
    }
    std::cout << std::endl;

    return password;
}

std::string Utils::readKeyFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Cannot open key file: " << path << std::endl;
        return "";
    }

    std::string key;
    std::getline(file, key);

    // Trim trailing whitespace / carriage return
    while (!key.empty() && (key.back() == '\r' || key.back() == '\n' ||
        key.back() == ' ' || key.back() == '\t')) {
        key.pop_back();
    }

    return key;
}
