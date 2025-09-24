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
 * @brief Generates BPF syscall probe code for a given function name and event ID.
 */
class SyscallGenerator {
 public:
  /**
   * @brief Constructor for SyscallGenerator.
   * @param event_id Unique identifier for the event.
   * @param func_name Name of the syscall function to generate probes for.
   */
  SyscallGenerator(int event_id, const std::string& func_name)
      : event_id_(event_id), func_name_(func_name) {
    DC_LOG_TRACE("SyscallGenerator::SyscallGenerator(event_id=%d, func_name=%s) - start", event_id,
                 func_name.c_str());
    // No additional initialization required.
    DC_LOG_TRACE("SyscallGenerator::SyscallGenerator - end");
  }

  /**
   * @brief Generates the BPF probe code as a stringstream.
   * @return std::stringstream containing the generated code.
   */
  std::stringstream generate() const {
    DC_LOG_TRACE("SyscallGenerator::generate() - start");
    std::string sanitized_func_name = func_name_.substr(0, 10);
    // Replace '.' and '@' with '_' to sanitize function name for use in identifiers.
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');

    std::stringstream ss;
    // Generate entry probe
    ss << "SEC(\"ksyscall/" << func_name_ << "\")\n";
    ss << "int BPF_KSYSCALL(" << sanitized_func_name << event_id_
       << "_entry, struct pt_regs* regs) {\n";
    ss << "  generic_entry(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";
    // Generate return probe
    bool is_fork = std::regex_search(func_name_, std::regex(".*fork.*"));
    std::string exit_func;
    if (is_fork) {
      exit_func = "generic_fork_exit";
    } else {
      exit_func = "generic_exit";
    }
    ss << "SEC(\"kretsyscall/" << func_name_ << "\")\n";
    ss << "int BPF_KRETPROBE(" << sanitized_func_name << event_id_
       << "_exit, struct pt_regs* regs) {\n";
    ss << "  " << exit_func << "(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";

    DC_LOG_DEBUG("Generated BPF probe code for function '%s' with event_id %d", func_name_.c_str(),
                 event_id_);
    DC_LOG_TRACE("SyscallGenerator::generate() - end");
    return ss;
  }

 private:
  int event_id_;           ///< Unique identifier for the event.
  std::string func_name_;  ///< Name of the syscall function.
};

}  // namespace datacrumbs