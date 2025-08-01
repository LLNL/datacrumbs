#pragma once
#include <algorithm>
#include <sstream>
#include <string>
namespace datacrumbs {
class USDTGenerator {
 public:
  USDTGenerator(int event_id, const std::string& func_name, const std::string& binary,
                const std::string& provider)
      : event_id_(event_id), func_name_(func_name), binary_(binary), provider_(provider) {}

  std::stringstream generate() const {
    std::string sanitized_func_name = func_name_;
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '.', '_');
    std::replace(sanitized_func_name.begin(), sanitized_func_name.end(), '@', '_');
    std::stringstream ss;
    if (provider_ == "python") {
      ss << "SEC(\"usdt/" << binary_ << ":" << provider_ << ":" << func_name_ << "\")\n";
      ss << "int BPF_USDT(" << sanitized_func_name << event_id_
         << "_entry, void* cls, void* function) {\n";
      ss << "  generic_entry(ctx, " << event_id_ << ");\n";
      ss << "  return 0;\n";
      ss << "}\n";
    }
    return ss;
  }

 private:
  int event_id_;
  std::string func_name_;
  std::string binary_;
  std::string provider_;
};
}  // namespace datacrumbs