#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/logging.h>  // Logging macros
// std headers
#include <string>
#include <vector>

// USDTFunctionExtractor extracts USDT (User-level Statically Defined Tracing) function names
// for a given provider. Currently, only the "python" provider is supported.
class USDTFunctionExtractor {
 public:
  // Constructor: initializes the extractor with the given provider name.
  explicit USDTFunctionExtractor(const std::string& provider) : provider_(provider) {
    DC_LOG_TRACE("USDTFunctionExtractor constructed for provider: %s", provider.c_str());
  }

  // Extracts function names for the specified provider.
  // Returns a vector of function names if the provider is supported, otherwise returns an empty
  // vector.
  std::vector<std::string> extractFunctionNames() const {
    DC_LOG_TRACE("extractFunctionNames() called for provider: %s", provider_.c_str());
    if (provider_ == "python") {
      DC_LOG_DEBUG("Extracting USDT function names for Python provider");
      return {"function__entry"};
    } else {
      DC_LOG_WARN("Provider '%s' is not supported. Returning empty function list.",
                  provider_.c_str());
    }
    return {};
  }

 private:
  std::string provider_;  // Name of the provider (e.g., "python")
};