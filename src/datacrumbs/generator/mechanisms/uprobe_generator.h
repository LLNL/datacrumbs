#pragma once
// include first
#include <datacrumbs/datacrumbs_config.h>
// other headers
#include <datacrumbs/common/logging.h>  // Logging macros
// std headers
#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
namespace datacrumbs {

/**
 * @brief Generates eBPF uprobe and uretprobe function code for a given function.
 *
 * This class is responsible for generating the C code for attaching uprobes and uretprobes
 * to a specified function, using the provided event id, function name, offset, and provider.
 */
class UProbeGenerator {
 public:
  /**
   * @brief Constructor for UProbeGenerator.
   * @param event_id Unique identifier for the event.
   * @param func_name Name of the function to probe.
   * @param offset Offset within the function (currently unused).
   * @param provider Name of the provider (e.g., binary or library).
   */
  UProbeGenerator(int event_id, const std::string& func_name, const std::string& offset,
                  const std::string& provider, bool is_manual = false)
      : event_id_(event_id),
        func_name_(func_name),
        offset_(offset),
        provider_(provider),
        is_manual_(is_manual) {
    DC_LOG_TRACE(
        "UProbeGenerator::UProbeGenerator - event_id=%d, func_name=%s, offset=%s, provider=%s, "
        "is_manual=%d",
        event_id, func_name.c_str(), offset.c_str(), provider.c_str(), is_manual);
  }

  /**
   * @brief Generates the uprobe and uretprobe C code as a stringstream.
   * @return std::stringstream containing the generated code.
   */
  std::stringstream generate() const {
    DC_LOG_TRACE("UProbeGenerator::generate - start");
    // Sanitize function name for use in symbol names
    std::string sanitized_func_name = func_name_.substr(0, 10);

    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');
    auto load_func = func_name_;
    if (offset_ != "") {
      load_func = offset_;
    }
    std::stringstream ss;
    // Generate uprobe section and function
    if (is_manual_) {
      ss << "SEC(\"uprobe\")\n";
    } else {
      ss << "SEC(\"uprobe/" << provider_ << ":" << load_func << "\")\n";
    }
    ss << "int BPF_UPROBE(" << sanitized_func_name << event_id_ << "_entry) {\n";
    ss << "  generic_entry(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";
    bool is_fork = std::regex_search(func_name_, std::regex(".*fork.*"));
    std::string exit_func;
    if (is_fork) {
      exit_func = "generic_fork_exit";
    } else {
      exit_func = "generic_exit";
    }
    // Generate uretprobe section and function
    if (is_manual_) {
      ss << "SEC(\"uretprobe\")\n";
    } else {
      ss << "SEC(\"uretprobe/" << provider_ << ":" << func_name_ << "\")\n";
    }
    ss << "int BPF_URETPROBE(" << sanitized_func_name << event_id_ << "_exit) {\n";
    ss << "  " << exit_func << "(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";

    DC_LOG_TRACE("UProbeGenerator::generate - end");
    return ss;
  }

 private:
  int event_id_;           ///< Unique identifier for the event
  std::string func_name_;  ///< Name of the function to probe
  std::string provider_;   ///< Provider (binary/library)
  std::string offset_;     ///< Offset within the function (currently unused)
  bool is_manual_;         ///< Whether the probe is manual
};

}  // namespace datacrumbs