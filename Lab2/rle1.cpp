#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

std::vector<uint8_t> rleEncode(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> out;
    if (data.empty()) return out;

    size_t i = 0;
    while (i < data.size()) {
        uint8_t val = data[i];

        // Count consecutive identical bytes (capped at 254)
        int runLen = 1;
        while (i + runLen < data.size() && data[i + runLen] == val && runLen < 254)
            runLen++;

        // Decide how to encode this run
        int remaining = runLen;
        while (remaining > 0) {
            if (val == 0xFF) {
                if (remaining >= 2) {
                    // Run token for 0xFF: FF <count> FF
                    int batch = std::min(remaining, 254);
                    out.push_back(0xFF);
                    out.push_back(static_cast<uint8_t>(batch));
                    out.push_back(0xFF);
                    remaining -= batch;
                } else {
                    // Single 0xFF: escape as FF FF
                    out.push_back(0xFF);
                    out.push_back(0xFF);
                    remaining--;
                }
            } else {
                if (remaining >= 3) {
                    // Run token: FF <count> <value>
                    int batch = std::min(remaining, 254);
                    out.push_back(0xFF);
                    out.push_back(static_cast<uint8_t>(batch));
                    out.push_back(val);
                    remaining -= batch;
                } else {
                    // Raw literal (1 or 2 bytes not worth encoding)
                    out.push_back(val);
                    remaining--;
                }
            }
        }

        i += runLen;
    }
    return out;
}

std::vector<uint8_t> rleDecode(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> out;
    size_t i = 0;
    while (i < data.size()) {
        if (data[i] == 0xFF) {
            // Escape sequence - need at least one more byte
            if (i + 1 >= data.size()) {
                std::cerr << "Warning: incomplete escape sequence at end of input.\n";
                break;
            }
            uint8_t next = data[i + 1];
            if (next == 0xFF) {
                // FF FF → single literal 0xFF
                out.push_back(0xFF);
                i += 2;
            } else {
                // FF <count> <value> → repeat value <count> times
                if (i + 2 >= data.size()) {
                    std::cerr << "Warning: incomplete run token at end of input.\n";
                    break;
                }
                uint8_t value = data[i + 2];
                for (int k = 0; k < next; k++)
                    out.push_back(value);
                i += 3;
            }
        } else {
            // Plain literal byte
            out.push_back(data[i]);
            i++;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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
        char c;
        while (ss.peek() == ',' || ss.peek() == ' ') ss.get(c);
    }
    return data;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== RLE Encoder/Decoder (FF escape-byte variant) ===\n";
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
        std::vector<uint8_t> decoded = rleDecode(encoded);

        printHex(input,   "Original ");
        printHex(encoded, "Encoded  ");
        printHex(decoded, "Decoded  ");

        if (decoded == input)
            std::cout << "Round-trip check: OK\n";
        else
            std::cout << "Round-trip check: MISMATCH\n";

        float ratio = 100.0f * encoded.size() / input.size();
        std::cout << "Compression: " << input.size() << " → " << encoded.size()
                  << " bytes (" << std::fixed << std::setprecision(1) << ratio << "% of original)\n\n";
    }

    return 0;
}
