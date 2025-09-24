#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/logging.h>  // Logging macros
// std headers
#include <algorithm>
#include <sstream>
#include <string>

namespace datacrumbs {

/**
 * @brief USDTGenerator generates USDT (User-level Statically Defined Tracing) probe code.
 */
class USDTGenerator {
 public:
  /**
   * @brief Constructor for USDTGenerator.
   * @param event_id Unique identifier for the event.
   * @param func_name Name of the function to instrument.
   * @param binary Name of the binary containing the function.
   * @param provider Name of the USDT provider (e.g., "python").
   */
  USDTGenerator(int event_id, const std::string& func_name, const std::string& binary,
                const std::string& provider)
      : event_id_(event_id), func_name_(func_name), binary_(binary), provider_(provider) {
    DC_LOG_TRACE("USDTGenerator constructor called for function: %s, event_id: %d",
                 func_name.c_str(), event_id);
  }

  /**
   * @brief Generates the USDT probe code as a stringstream.
   * @return std::stringstream containing the generated code.
   */
  std::stringstream generate() const {
    DC_LOG_TRACE("USDTGenerator::generate() start for function: %s", func_name_.c_str());

    std::string sanitized_func_name = func_name_.substr(0, 10);
    // Replace '.' and '@' with '_' to sanitize the function name for use in identifiers.
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');

    std::stringstream ss;
    if (provider_ == "python") {
      // Log info about probe generation for Python provider.
      DC_LOG_DEBUG("Generating USDT probe for Python function: %s in binary: %s",
                   func_name_.c_str(), binary_.c_str());
      ss << "SEC(\"usdt/" << binary_ << ":" << provider_ << ":function__entry\")\n";
      ss << "int BPF_USDT(python_function_entry, long cls, long function) {\n";
      ss << "  usdt_entry(ctx, " << event_id_ << ");\n";
      ss << "  return 0;\n";
      ss << "}\n";

      // Log info about probe generation for Python provider.
      DC_LOG_DEBUG("Generating USDT probe for Python function: %s in binary: %s",
                   func_name_.c_str(), binary_.c_str());
      ss << "SEC(\"usdt/" << binary_ << ":" << provider_ << ":function__return\")\n";
      ss << "int BPF_USDT(python_function_return, long cls, long function) {\n";
      ss << "  usdt_exit(ctx, " << event_id_ << ", cls, function);\n";
      ss << "  return 0;\n";
      ss << "}\n";
    } else {
      // Warn if provider is not supported.
      DC_LOG_WARN("USDTGenerator: Provider '%s' is not supported. No code generated.",
                  provider_.c_str());
    }

    DC_LOG_TRACE("USDTGenerator::generate() end for function: %s", func_name_.c_str());
    return ss;
  }

 private:
  int event_id_;           ///< Unique identifier for the event.
  std::string func_name_;  ///< Name of the function to instrument.
  std::string binary_;     ///< Name of the binary containing the function.
  std::string provider_;   ///< Name of the USDT provider.
};

}  // namespace datacrumbs