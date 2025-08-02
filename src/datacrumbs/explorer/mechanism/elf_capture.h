#pragma once

#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace datacrumbs {
class ElfSymbolExtractor {
 public:
  explicit ElfSymbolExtractor(const std::string& path);

  ~ElfSymbolExtractor();

  std::pair<std::vector<std::string>, std::vector<std::string>> extract_symbols();

 private:
  bool is_elf() const;

  int fd_;
  uint8_t* data_;
  size_t size_;
};
}  // namespace datacrumbs
/**
 * g++ -std=c++14 elf_capture_test.cpp -o elf_capture_test -lelf
 */
