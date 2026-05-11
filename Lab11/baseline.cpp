// baseline.cpp — unobfuscated license-key check.
// Key format: XXXX-XXXX-XXXX  (3 groups of 4 hex digits, joined by '-')
// Valid iff:
//   - length is 14
//   - positions 4 and 9 are '-'
//   - other positions are hex digits [0-9A-Fa-f]
//   - sum of all 12 nibbles, mod 16, equals 0xA
//   - g1 XOR g2 XOR g3 (each group as a 16-bit value) equals 0xBEEF
//
// Usage: baseline <key>

#include <cstdio>
#include <cstring>
#include <cctype>

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int check_key(const char* k) {
    if (std::strlen(k) != 14)            return 0;
    if (k[4] != '-' || k[9] != '-')      return 0;

    int nibbles[12];
    int idx = 0;
    for (int i = 0; i < 14; ++i) {
        if (i == 4 || i == 9) continue;
        int v = hex_val(k[i]);
        if (v < 0) return 0;
        nibbles[idx++] = v;
    }

    int sum = 0;
    for (int i = 0; i < 12; ++i) sum += nibbles[i];
    if ((sum & 0xF) != 0xA) return 0;

    unsigned g1 = (nibbles[0] << 12) | (nibbles[1] << 8) | (nibbles[2] << 4) | nibbles[3];
    unsigned g2 = (nibbles[4] << 12) | (nibbles[5] << 8) | (nibbles[6] << 4) | nibbles[7];
    unsigned g3 = (nibbles[8] << 12) | (nibbles[9] << 8) | (nibbles[10]<< 4) | nibbles[11];
    if ((g1 ^ g2 ^ g3) != 0xBEEF) return 0;

    return 1;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::printf("usage: %s <key>\n", argv[0]);
        return 2;
    }
    std::printf("%s\n", check_key(argv[1]) ? "OK" : "BAD");
    return 0;
}
