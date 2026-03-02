#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

static const int MAX_RUN     = 129;   // 127 + 2
static const int MAX_LITERAL = 128;   // 127 + 1

uint8_t makeControlByte(int flag, int count)
{
    return static_cast<uint8_t>((flag << 7) | (count & 0x7F));
}

std::vector<uint8_t> rleEncode(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> out;
    if (data.empty()) return out;

    size_t i = 0;
    while (i < data.size()) {

        // Count the run of identical bytes starting at i
        size_t runLen = 1;
        while (i + runLen < data.size()
               && data[i + runLen] == data[i]
               && runLen < MAX_RUN)
            runLen++;

        if (runLen >= 2) {
            // Emit a run token: [1 | (runLen-2)] followed by the value
            out.push_back(makeControlByte(1, (int)runLen - 2));
            out.push_back(data[i]);
            i += runLen;
        } else {
            // Gather a literal run: collect bytes until a repeat starts or
            // we hit the max literal block size
            size_t litLen = 1;
            while (i + litLen < data.size() && litLen < MAX_LITERAL) {
                // Peek ahead: if the next two bytes are the same, stop here
                // so the encoder can pick up that run on the next iteration
                size_t ahead = 1;
                while (i + litLen + ahead < data.size()
                       && data[i + litLen + ahead] == data[i + litLen]
                       && ahead < MAX_RUN)
                    ahead++;
                if (ahead >= 2) break;
                litLen++;
            }

            // Emit a literal token: [0 | (litLen-1)] followed by the bytes
            out.push_back(makeControlByte(0, (int)litLen - 1));
            for (size_t k = 0; k < litLen; k++)
                out.push_back(data[i + k]);
            i += litLen;
        }
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

void printAnnotated(const std::vector<uint8_t>& encoded)
{
    std::cout << "Annotated output:\n";
    size_t i = 0;
    while (i < encoded.size()) {
        uint8_t ctrl = encoded[i++];
        int flag  = (ctrl >> 7) & 1;
        int count =  ctrl & 0x7F;

        if (flag == 1) {
            int reps = count + 2;
            uint8_t val = encoded[i++];
            std::cout << "  [1|" << std::setw(2) << count << "] "
                      << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(val) << std::dec
                      << "  →  " << reps << "x 0x"
                      << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(val) << std::dec << "\n";
        } else {
            int bytes = count + 1;
            std::cout << "  [0|" << std::setw(2) << count << "] ";
            for (int k = 0; k < bytes && i < encoded.size(); k++, i++) {
                std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(encoded[i]) << std::dec << " ";
            }
            std::cout << " →  " << bytes << " literal(s)\n";
        }
    }
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
    std::cout << "=== RLE Encoder ([flag|count] variant) ===\n";
    std::cout << "Control byte: high bit = flag (1=run, 0=literals), low 7 bits = count\n";
    std::cout << "  Run:     [1|count] value     →  repeat value (count+2) times\n";
    std::cout << "  Literal: [0|count] b0 b1...  →  (count+1) literal bytes\n\n";
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
        printAnnotated(encoded);

        float ratio = 100.0f * encoded.size() / input.size();
        std::cout << "Compression: " << input.size() << " → " << encoded.size()
                  << " bytes (" << std::fixed << std::setprecision(1) << ratio << "% of original)\n\n";
    }

    return 0;
}