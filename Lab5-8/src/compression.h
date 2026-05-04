#pragma once
#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <vector>
#include <cstdint>

// Windows Compression API wrapper (XPRESS with Huffman)
class Compression {
public:
    // Compress data, returns compressed bytes
    static std::vector<unsigned char> compress(const unsigned char* data, size_t size);

    // Decompress data given the original uncompressed size
    static std::vector<unsigned char> decompress(const unsigned char* data,
        size_t compressedSize, size_t originalSize);
};

#endif
