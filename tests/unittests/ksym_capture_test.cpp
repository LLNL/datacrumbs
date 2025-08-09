#include "ksym_capture.h"
int main() {
  KSymCapture ksym;
  std::string pattern;
  std::cout << "Enter regex pattern to search for kernel symbols: ";
  std::getline(std::cin, pattern);

  auto matches = ksym.getFunctionsByRegex(pattern);
  std::cout << "Matched functions:\n";
  for (const auto& func : matches) {
    std::cout << func << '\n';
  }
  return 0;
}