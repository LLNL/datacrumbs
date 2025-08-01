#pragma once

#include <algorithm>
#include <sstream>
#include <string>

namespace datacrumbs {

class KProbeGenerator {
 public:
  KProbeGenerator(int event_id, const std::string& func_name)
      : event_id_(event_id), func_name_(func_name) {}

  std::stringstream generate() const {
    std::string sanitized_func_name = func_name_;
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');
    std::stringstream ss;
    ss << "SEC(\"kprobe/" << func_name_ << "\")\n";
    ss << "int BPF_KPROBE(" << sanitized_func_name << event_id_ << "_entry) {\n";
    ss << "  generic_entry(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";
    ss << "SEC(\"kretprobe/" << func_name_ << "\")\n";
    ss << "int BPF_KRETPROBE(" << sanitized_func_name << event_id_ << "_exit) {\n";
    ss << "  generic_exit(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";
    return ss;
  }

 private:
  int event_id_;
  std::string func_name_;
};

}  // namespace datacrumbs