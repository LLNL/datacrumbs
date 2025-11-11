#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/logging.h>  // Include logging header
// std headers
#include <string>
#include <vector>

namespace datacrumbs {
namespace utils {

// Base64 encoding table (URL-safe, no special characters)
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

// Check if a character is base64 (URL-safe)
inline bool is_base64(unsigned char c) {
  DC_LOG_TRACE("Entering is_base64 with char: %c", c);
  bool result = (isalnum(c) || (c == '-') || (c == '_'));
  DC_LOG_DEBUG("is_base64(%c) = %d", c, result);
  DC_LOG_TRACE("Exiting is_base64");
  return result;
}

// Encode a byte vector to base64 string (URL-safe, no special chars)
inline std::string base64_encode(const std::vector<unsigned char>& bytes_to_encode) {
  DC_LOG_TRACE("Start base64_encode, input size: %zu", bytes_to_encode.size());
  std::string ret;
  int i = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];
  size_t in_len = bytes_to_encode.size();
  size_t pos = 0;

  while (in_len--) {
    char_array_3[i++] = bytes_to_encode[pos++];
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (int j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (int j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];

    // No padding for URL-safe base64
    // while ((i++ < 3)) ret += '=';
  }

  DC_LOG_DEBUG("base64_encode completed, output size: %zu", ret.size());
  DC_LOG_TRACE("End base64_encode");
  return ret;
}

// Decode a base64 string to byte vector (URL-safe, no special chars)
inline std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
  DC_LOG_TRACE("Start base64_decode, input size: %zu", encoded_string.size());
  int in_len = encoded_string.size();
  int i = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::vector<unsigned char> ret;

  while (in_len-- && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++) char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; i < 3; i++) ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (int j = i; j < 4; j++) char_array_4[j] = 0;

    for (int j = 0; j < 4; j++) char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (int j = 0; j < i - 1; j++) ret.push_back(char_array_3[j]);
  }

  if (ret.empty()) {
    DC_LOG_WARN("base64_decode: Decoded output is empty for input of size %zu",
                encoded_string.size());
  }
  DC_LOG_INFO("base64_decode completed, output size: %zu", ret.size());
  DC_LOG_TRACE("End base64_decode");
  return ret;
}

// Timer class for measuring elapsed time between code segments
class Timer {
 public:
  Timer() : elapsed_time(0) {
    // Trace constructor entry
    DC_LOG_TRACE("Timer constructed, elapsed_time initialized to 0");
  }

  // Resume or start the timer
  void resumeTime() {
    DC_LOG_TRACE("Timer::resumeTime called");
    t1 = std::chrono::high_resolution_clock::now();
    DC_LOG_DEBUG("Timer resumed at current time point");
  }

  // Pause the timer and accumulate elapsed time
  double pauseTime() {
    DC_LOG_TRACE("Timer::pauseTime called");
    auto t2 = std::chrono::high_resolution_clock::now();
    double segment = std::chrono::duration<double>(t2 - t1).count();
    elapsed_time += segment;
    DC_LOG_DEBUG("Timer paused, segment duration: %f seconds, total elapsed: %f seconds", segment,
                 elapsed_time);
    return elapsed_time;
  }

  // Get the total elapsed time
  double getElapsedTime() {
    DC_LOG_TRACE("Timer::getElapsedTime called");
    DC_LOG_DEBUG("Returning elapsed_time: %f seconds", elapsed_time);
    return elapsed_time;
  }

 private:
  std::chrono::high_resolution_clock::time_point t1;  // Last start/resume time
  double elapsed_time;                                // Accumulated elapsed time in seconds
};

// Function to remove non-UTF8 characters from a string
inline std::string remove_non_utf8(const std::string& input) {
  DC_LOG_TRACE("Start remove_non_utf8, input size: %zu", input.size());
  std::string result;
  result.reserve(input.size());

  for (size_t i = 0; i < input.size();) {
    unsigned char byte = static_cast<unsigned char>(input[i]);

    // Single-byte UTF-8 character (0xxxxxxx)
    if (byte <= 0x7F) {
      // Only keep characters valid for filenames/paths
      if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
          (byte >= '0' && byte <= '9') || byte == '_' || byte == '-' || byte == '.' ||
          byte == '/') {
        result += input[i];
      } else {
        DC_LOG_DEBUG("Skipping invalid filename character 0x%02X at position %zu", byte, i);
      }
      i++;
    }
    // Multi-byte UTF-8 character
    else if ((byte & 0xE0) == 0xC0) {  // 2-byte (110xxxxx)
      if (i + 1 < input.size() && (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80) {
        result += input.substr(i, 2);
        i += 2;
      } else {
        DC_LOG_DEBUG("Invalid 2-byte UTF-8 sequence at position %zu", i);
        i++;
      }
    } else if ((byte & 0xF0) == 0xE0) {  // 3-byte (1110xxxx)
      if (i + 2 < input.size() && (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
          (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80) {
        result += input.substr(i, 3);
        i += 3;
      } else {
        DC_LOG_DEBUG("Invalid 3-byte UTF-8 sequence at position %zu", i);
        i++;
      }
    } else if ((byte & 0xF8) == 0xF0) {  // 4-byte (11110xxx)
      if (i + 3 < input.size() && (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
          (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80 &&
          (static_cast<unsigned char>(input[i + 3]) & 0xC0) == 0x80) {
        result += input.substr(i, 4);
        i += 4;
      } else {
        DC_LOG_DEBUG("Invalid 4-byte UTF-8 sequence at position %zu", i);
        i++;
      }
    } else {
      // Invalid UTF-8 start byte
      DC_LOG_DEBUG("Invalid UTF-8 start byte 0x%02X at position %zu", byte, i);
      i++;
    }
  }

  DC_LOG_DEBUG("remove_non_utf8 completed, output size: %zu", result.size());
  DC_LOG_TRACE("End remove_non_utf8");
  return result;
}

}  // namespace utils
}  // namespace datacrumbs