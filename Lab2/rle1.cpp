#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

static void encodeRun(std::vector<uint8_t>& out, uint8_t value, int count)
{
    while (count > 0) {
        if (value == 0xFF) {
            if (count >= 2) {
                int batch = std::min(count, 254);
                out.push_back(0xFF);
                out.push_back(static_cast<uint8_t>(batch));
                out.push_back(0xFF);
                count -= batch;
            } else {
                // single 0xFF → escape it
                out.push_back(0xFF);
                out.push_back(0xFF);
                count--;
            }
        } else {
            if (count >= 3) {
                // emit run token
                int batch = std::min(count, 254);
                out.push_back(0xFF);
                out.push_back(static_cast<uint8_t>(batch));
                out.push_back(value);
                count -= batch;
            } else {
                // emit 1 or 2 raw literals
                out.push_back(value);
                count--;
            }
        }
    }
}

std::vector<uint8_t> rleEncode(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> out;
    if (data.empty()) return out;

    size_t i = 0;
    while (i < data.size()) {
        uint8_t val = data[i];
        int runLen = 1;

        // Count how many consecutive identical bytes follow
        while (i + runLen < data.size() && data[i + runLen] == val && runLen < 254)
            runLen++;

        encodeRun(out, val, runLen);
        i += runLen;
    }
    return out;
}

void printHex(const std::vector<uint8_t>& v, const std::string& label)
{
    std::cout << label << " (" << v.size() << " bytes):\n  ";
    for (size_t i = 0; i < v.size(); i++) {
        std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(v[i]);
        if (i + 1 < v.size()) std::cout << " ";
        if ((i + 1) % 16 == 0 && i + 1 < v.size()) std::cout << "\n  ";
    }
    std::cout << std::dec << "\n";
}

std::vector<uint8_t> parseInput(const std::string& line)
{
    std::vector<uint8_t> data;
    std::istringstream ss(line);
    int val;
    while (ss >> val) {
        if (val < 0 || val > 255) {
            std::cerr << "Warning: value " << val << " out of byte range, clamped.\n";
            val = std::max(0, std::min(255, val));
        }
        data.push_back(static_cast<uint8_t>(val));
        // skip commas or other separators
        char c;
        while (ss.peek() == ',' || ss.peek() == ' ') ss.get(c);
    }
    return data;
}


int main()
{
    std::cout << "=== RLE Encoder (FF escape-byte variant) ===\n";
    std::cout << "Enter byte values separated by spaces or commas (decimal 0-255).\n";
    std::cout << "Type 'quit' to exit.\n\n";

    std::string line;
    while (true) {
        std::cout << "Input: ";
        if (!std::getline(std::cin, line)) break;
        if (line == "quit" || line == "exit") break;
        if (line.empty()) continue;

        std::vector<uint8_t> input = parseInput(line);
        if (input.empty()) {
            std::cout << "No valid bytes found.\n\n";
            continue;
        }

        std::vector<uint8_t> encoded = rleEncode(input);

        printHex(input,   "Original ");
        printHex(encoded, "Encoded  ");

        float ratio = 100.0f * encoded.size() / input.size();
        std::cout << "Compression: " << input.size() << " → " << encoded.size()
                  << " bytes (" << std::fixed << std::setprecision(1) << ratio << "% of original)\n\n";
    }

    return 0;
}