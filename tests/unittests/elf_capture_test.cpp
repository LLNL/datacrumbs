
#include "elf_capture.h"

#include <iostream>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <elf_file_path>\n";
    return 1;
  }

  try {
    ElfSymbolExtractor extractor(argv[1]);
    auto symbols = extractor.extract_symbols();
    for (const auto& sym : symbols) {
      std::cout << sym << '\n';
    }
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << '\n';
    return 1;
  }

  return 0;
}