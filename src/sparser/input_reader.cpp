#include "input_reader.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

constexpr size_t PREALLOCATED_CAPACITY = 1000000;
constexpr double GIGABYTE = 1e9;

std::string InputReader::ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Error opening file: " + std::string(filename));
    }

    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(fileSize, '\0');

    if (!file.read(buffer.data(), fileSize)) {
        throw std::runtime_error("Error reading file: " + std::string(filename));
    }
    std::cout << "Read " << static_cast<double>(fileSize) / GIGABYTE << " GB from " << filename << "\n";

    return buffer;
}

std::vector<std::string_view> InputReader::ReadRecords(const std::string& input) {
    std::vector<std::string_view> records;
    records.reserve(PREALLOCATED_CAPACITY);  // Pre-allocated capacity

    const char* start = input.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* input_end = start + input.size();

    while (start < input_end) {
        // Use memchr to find the next newline in the remaining input.
        const void* pos = std::memchr(start, '\n', input_end - start);
        if (pos != nullptr) {
            const char* newline = static_cast<const char*>(pos);
            records.emplace_back(start, newline - start);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            start = newline + 1;  // Move past the newline
        } else {
#ifndef NDEBUG
            std::cout << "Adding last segment: " << std::string_view(start, input_end - start) << "\n";
#endif
            // No newline found, so the rest is the last record.
            records.emplace_back(start, input_end - start);
            break;
        }
    }

    return records;
}
