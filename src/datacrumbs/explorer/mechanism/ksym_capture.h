#pragma once

#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace datacrumbs {
class KSymCapture {
 public:
  KSymCapture(const std::string& kallsyms_path = "/proc/kallsyms") { loadFunctions(kallsyms_path); }

  std::vector<std::string> getFunctionsByRegex(const std::string& pattern) const {
    std::vector<std::string> result;
    std::regex re(pattern);
    for (auto func : functions_) {
      if (std::regex_search(func, re)) {
        result.push_back(func);
      }
    }
    return result;
  }

 private:
  std::vector<std::string> functions_;

  void loadFunctions(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
      std::istringstream iss(line);
      std::string addr, type, name;
      if (!(iss >> addr >> type >> name)) continue;
      // Only add functions (type 'T' or 't')
      if (type == "T" || type == "t") {
        functions_.push_back(name);
      }
    }
  }
};

}  // namespace datacrumbs
