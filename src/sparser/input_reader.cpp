#include "input_reader.h"

#include <fstream>
#include <iostream>

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

    std::cout << "Read " << fileSize / 1e9 << " GB from " << filename << "\n";

    return buffer;
}

std::vector<std::string_view> InputReader::ReadRecords(const std::string& input) {
    std::vector<std::string_view> records;
    records.reserve(1000000); // Pre-allocated capacity

    const char* start = input.data();
    const char* input_end = start + input.size();

    while (start < input_end) {
        // Use memchr to find the next newline in the remaining input.
        const void* pos = std::memchr(start, '\n', input_end - start);
        if (pos) {
            const char* newline = static_cast<const char*>(pos);
            records.emplace_back(start, newline - start);
            start = newline + 1; // Move past the newline
        } else {
#ifndef NDEBUG
            std::cout << "Adding last segment: " 
                      << std::string_view(start, input_end - start) << "\n";
#endif
            // No newline found, so the rest is the last record.
            records.emplace_back(start, input_end - start);
            break;
        }
    }

    return records;
}
