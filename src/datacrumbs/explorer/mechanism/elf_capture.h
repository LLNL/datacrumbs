#pragma once

#include <datacrumbs/common/logging.h>  // For logging macros
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace datacrumbs {

/**
 * @class ElfSymbolExtractor
 * @brief Extracts symbol names from ELF files.
 */
class ElfSymbolExtractor {
 public:
  /**
   * @brief Constructs the extractor for a given ELF file path.
   * @param path Path to the ELF file.
   */
  explicit ElfSymbolExtractor(const std::string& path);

  /**
   * @brief Destructor to clean up resources.
   */
  ~ElfSymbolExtractor();

  /**
   * @brief Extracts symbol and demangled symbol names from the ELF file.
   * @return Pair of vectors: <mangled_names, demangled_names>
   */
  std::pair<std::vector<std::string>, std::vector<std::string>> extract_symbols();

 private:
  /**
   * @brief Checks if the mapped file is a valid ELF file.
   * @return True if ELF, false otherwise.
   */
  bool is_elf() const;

  int fd_;         ///< File descriptor for the ELF file.
  uint8_t* data_;  ///< Pointer to mapped ELF file data.
  size_t size_;    ///< Size of the mapped ELF file.
};

}  // namespace datacrumbs
/**
 * g++ -std=c++14 elf_capture_test.cpp -o elf_capture_test -lelf
 */
