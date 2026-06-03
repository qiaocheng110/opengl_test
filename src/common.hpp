#pragma once

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "log.hpp"

constexpr int kSrcWidth = 1280;
constexpr int kSrcHeight = 960;
constexpr int kDstWidth = 640;
constexpr int kDstHeight = 640;
constexpr int kIterations = 50;
constexpr size_t kNv12Size = kSrcWidth * kSrcHeight * 3 / 2;
constexpr size_t kRgbSize = kDstWidth * kDstHeight * 3;

inline std::vector<unsigned char> readNv12File(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        LOGE("Failed to open input file: %s", path);
        std::exit(EXIT_FAILURE);
    }

    const std::streamsize size = file.tellg();
    if (size != static_cast<std::streamsize>(kNv12Size)) {
        LOGE("Invalid NV12 file size: %lld, expected %zu for %dx%d NV12",
             static_cast<long long>(size), kNv12Size, kSrcWidth, kSrcHeight);
        std::exit(EXIT_FAILURE);
    }

    std::vector<unsigned char> data(kNv12Size);
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        LOGE("Failed to read input file: %s", path);
        std::exit(EXIT_FAILURE);
    }
    return data;
}

inline const char* inputPath(int argc, char** argv) {
    return argc > 1 ? argv[1] : "data";
}
