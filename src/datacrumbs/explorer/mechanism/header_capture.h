#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/logging.h>
// dependency headers
#include <clang-c/Index.h>  // Use custom logging macros
// std headers
#include <string>
#include <vector>

namespace datacrumbs {

/**
 * @brief Extracts function names from a given C/C++ header file using libclang.
 */
class HeaderFunctionExtractor {
 public:
  /**
   * @brief Constructs the extractor with the path to the header file.
   * @param headerPath Path to the header file to analyze.
   */
  HeaderFunctionExtractor(const std::string& headerPath);

  /**
   * @brief Destructor to clean up libclang resources.
   */
  ~HeaderFunctionExtractor();

  /**
   * @brief Extracts all function names from the header file.
   * @return Vector of function names as strings.
   */
  std::vector<std::string> extractFunctionNames();

 private:
  std::string headerPath_;  ///< Path to the header file.
  CXIndex index_;           ///< libclang index object.
  CXTranslationUnit tu_;    ///< libclang translation unit.
};

}  // namespace datacrumbs

/**
 * Example compilation command:
 * g++ -o extract_functions header_capture_test.cpp `llvm-config --cxxflags --ldflags --system-libs
 * --libs core` -lclang
 */
