#include "header_capture.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <header-file-path>" << std::endl;
    return 1;
  }

  HeaderFunctionExtractor extractor(argv[1]);
  std::vector<std::string> functionNames = extractor.extractFunctionNames();

  std::cout << "Functions found in header:" << std::endl;
  for (const auto& name : functionNames) {
    std::cout << name << std::endl;
  }

  return 0;
}