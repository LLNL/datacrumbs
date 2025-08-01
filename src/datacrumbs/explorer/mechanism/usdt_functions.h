#pragma once
#include <string>
#include <vector>

class USDTFunctionExtractor {
 public:
  USDTFunctionExtractor(const std::string& provider) : provider_(provider) {}

  std::vector<std::string> extractFunctionNames() const {
    if (provider_ == "python") {
      return {"function__entry", "function__return"};
    }
    return {};
  }

 private:
  std::string provider_;
};