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
 * @brief Generates kprobe and kretprobe BPF program code for a given function.
 *
 * This class is responsible for generating the C code for attaching kprobes and
 * kretprobes to kernel functions. The generated code uses sanitized function names
 * to ensure valid identifiers.
 */
class KProbeGenerator {
 public:
  /**
   * @brief Constructor for KProbeGenerator.
   * @param event_id Unique identifier for the event.
   * @param func_name Name of the kernel function to probe.
   */
  KProbeGenerator(int event_id, const std::string& func_name)
      : event_id_(event_id), func_name_(func_name) {
    DC_LOG_TRACE("KProbeGenerator::KProbeGenerator(event_id=%d, func_name=%s) - start", event_id,
                 func_name.c_str());
    DC_LOG_DEBUG("Initialized KProbeGenerator with event_id=%d and func_name=%s", event_id,
                 func_name.c_str());
    DC_LOG_TRACE("KProbeGenerator::KProbeGenerator - end");
  }

  /**
   * @brief Generates the kprobe and kretprobe code as a stringstream.
   * @return std::stringstream containing the generated code.
   */
  std::stringstream generate() const {
    DC_LOG_TRACE("KProbeGenerator::generate() - start");
    // Sanitize function name for use in identifiers
    std::string sanitized_func_name = func_name_.substr(0, 10);
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');
    DC_LOG_DEBUG("Sanitized function name: %s", sanitized_func_name.c_str());

    std::stringstream ss;
    // Generate kprobe entry code
    ss << "SEC(\"kprobe/" << func_name_ << "\")\n";
    ss << "int BPF_KPROBE(" << sanitized_func_name << event_id_ << "_entry) {\n";
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
    // Generate kretprobe exit code
    ss << "SEC(\"kretprobe/" << func_name_ << "\")\n";
    ss << "int BPF_KRETPROBE(" << sanitized_func_name << event_id_ << "_exit) {\n";
    ss << "  " << exit_func << "(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";

    DC_LOG_DEBUG("Generated kprobe and kretprobe code for function: %s (event_id=%d)",
                 func_name_.c_str(), event_id_);
    DC_LOG_TRACE("KProbeGenerator::generate() - end");
    return ss;
  }

 private:
  int event_id_;           ///< Unique identifier for the event
  std::string func_name_;  ///< Name of the kernel function to probe
};

}  // namespace datacrumbs