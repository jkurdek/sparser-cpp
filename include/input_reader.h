#ifndef INPUT_READER_H_
#define INPUT_READER_H_

#include <string>
#include <string_view>
#include <vector>

class InputReader {
   public:
    [[nodiscard]] static std::string ReadFile(const std::string& filename);
    [[nodiscard]] static std::vector<std::string_view> ReadRecords(const std::string& input);
};

#endif  // INPUT_READER_H_
