#pragma once

#include <algorithm>
#include <sstream>
#include <string>
namespace datacrumbs {

class UProbeGenerator {
 public:
  UProbeGenerator(int event_id, const std::string& func_name, const std::string& offset,
                  const std::string& provider)
      : event_id_(event_id), func_name_(func_name), offset_(offset), provider_(provider) {}

  std::stringstream generate() const {
    std::string sanitized_func_name = func_name_;
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');
    std::stringstream ss;
    ss << "SEC(\"uprobe/" << provider_ << ":" << func_name_ << "\")\n";
    ss << "int BPF_UPROBE(" << sanitized_func_name << event_id_ << "_entry) {\n";
    ss << "  generic_entry(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";
    ss << "SEC(\"uretprobe/" << provider_ << ":" << func_name_ << "\")\n";
    ss << "int BPF_URETPROBE(" << sanitized_func_name << event_id_ << "_exit) {\n";
    ss << "  generic_exit(ctx, " << event_id_ << ");\n";
    ss << "  return 0;\n";
    ss << "}\n";
    return ss;
  }

 private:
  int event_id_;
  std::string func_name_;
  std::string provider_;
  std::string offset_;
};

}  // namespace datacrumbs