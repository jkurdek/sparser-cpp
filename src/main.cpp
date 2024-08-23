#include <exception>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

constexpr double GIGABYTE = 1e9;

/**
 * Reads the contents of a file into a dynamically allocated buffer.
 *
 * @param filename The name of the file to be read.
 * @return A string containing the contents of the file.
 */
std::string readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Error opening file: " + filename);
    }

    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(fileSize, '\0');

    if (!file.read(buffer.data(), fileSize)) {
        throw std::runtime_error("Error reading file: " + filename);
    }

    return buffer;
}

int main(int argc, char* argv[]) {
    try {
        auto args = std::span(argv, argc);

        if (argc < 2) {
            std::cerr << "Usage: " << args[0] << " <filename>" << "\n";
            return 1;
        }

        const std::string filename = args[1];

        std::cout << "Reading file: " << filename << "\n";
        auto buffer = readFile(filename);
        std::cout << "Done reading! File size: " << static_cast<double>(buffer.size()) / GIGABYTE << " GB" << "\n";

        if (buffer.empty()) {
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << "\n";
        return 1;
    }
    return 0;
}
