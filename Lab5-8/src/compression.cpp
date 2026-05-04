#include "compression.h"
#include <windows.h>
#include <compressapi.h>
#include <stdexcept>
#include <iostream>

#pragma comment(lib, "Cabinet")

std::vector<unsigned char> Compression::compress(const unsigned char* data, size_t size) {
    COMPRESSOR_HANDLE compressor = NULL;
    if (!CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &compressor)) {
        throw std::runtime_error("Cannot create compressor");
    }

    // Query compressed size
    SIZE_T compressedSize = 0;
    Compress(compressor, data, size, NULL, 0, &compressedSize);

    std::vector<unsigned char> compressed(compressedSize);
    SIZE_T actualSize = 0;

    if (!Compress(compressor, data, size, compressed.data(), compressedSize, &actualSize)) {
        CloseCompressor(compressor);
        throw std::runtime_error("Compression failed");
    }

    compressed.resize(actualSize);
    CloseCompressor(compressor);
    return compressed;
}

std::vector<unsigned char> Compression::decompress(const unsigned char* data,
    size_t compressedSize, size_t originalSize) {
    DECOMPRESSOR_HANDLE decompressor = NULL;
    if (!CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &decompressor)) {
        throw std::runtime_error("Cannot create decompressor");
    }

    std::vector<unsigned char> decompressed(originalSize);
    SIZE_T actualSize = 0;

    if (!Decompress(decompressor, data, compressedSize,
        decompressed.data(), originalSize, &actualSize)) {
        CloseDecompressor(decompressor);
        throw std::runtime_error("Decompression failed");
    }

    decompressed.resize(actualSize);
    CloseDecompressor(decompressor);
    return decompressed;
}
