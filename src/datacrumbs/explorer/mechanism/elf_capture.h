#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/logging.h>
// std headers
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
  explicit ElfSymbolExtractor(const std::string& path, bool include_offsets = false);

  /**
   * @brief Destructor to clean up resources.
   */
  ~ElfSymbolExtractor();

  /**
   * @brief Extracts symbol and demangled symbol names from the ELF file.
   * @return Pair of vectors: <mangled_names, demangled_names>
   */
  std::vector<std::string> extract_symbols();

 private:
  /**
   * @brief Checks if the mapped file is a valid ELF file.
   * @return True if ELF, false otherwise.
   */
  bool is_elf() const;

  int fd_;                 ///< File descriptor for the ELF file.
  uint8_t* data_;          ///< Pointer to mapped ELF file data.
  size_t size_;            ///< Size of the mapped ELF file.
  bool include_offsets_;   ///< Whether to include offsets in the extraction.
  uint64_t base_address_;  ///< Base address for relative symbols (0 for ET_DYN, entry point for
                           ///< ET_EXEC).
  std::unordered_set<std::string>
      kExcludedFunctions;  ///< Set of functions to exclude from extraction.
};

}  // namespace datacrumbs
/**
 * g++ -std=c++14 elf_capture_test.cpp -o elf_capture_test -lelf
 */
