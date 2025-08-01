#pragma once
#include <clang-c/Index.h>

#include <iostream>
#include <string>
#include <vector>

namespace datacrumbs {
class HeaderFunctionExtractor {
 public:
  HeaderFunctionExtractor(const std::string& headerPath);

  ~HeaderFunctionExtractor();
  std::vector<std::string> extractFunctionNames();

 private:
  std::string headerPath_;
  CXIndex index_;
  CXTranslationUnit tu_;
};
}  // namespace datacrumbs
/**
 * g++ -o extract_functions  header_capture_test.cpp `llvm-config --cxxflags --ldflags --system-libs
 * --libs core` -lclang
 */
