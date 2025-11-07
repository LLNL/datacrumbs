#ifndef DATACRUMBS_COMMON_RUNTIME_CONFIGURATION_MANAGER_H__
#define DATACRUMBS_COMMON_RUNTIME_CONFIGURATION_MANAGER_H__

#include <datacrumbs/common/data_structures.h>
#include <datacrumbs/datacrumbs_config.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace datacrumbs {

using RuntimeProbeScopes = std::unordered_map<std::string, std::unordered_set<std::string>>;

class RuntimeConfigurationManager {
 public:
  RuntimeConfigurationManager();
  explicit RuntimeConfigurationManager(const std::filesystem::path& runtime_probe_file,
                                       const std::string& explicit_run_id,
                                       const std::string& explicit_user, bool print = true);

  void print_configurations() const;

  std::optional<uint64_t> get_runtime_event_id(const std::string& probe_name,
                                               const std::string& function_name) const;
  const RuntimeEventMetadata* get_runtime_event_metadata(uint64_t event_id) const;

  std::filesystem::path data_dir;
  std::filesystem::path trace_log_dir;
  std::filesystem::path trace_file_path;
  std::filesystem::path probe_file_path;
  std::filesystem::path system_probe_path;
  std::filesystem::path runtime_probe_state_db_path;
  std::filesystem::path server_run_dir;
  std::filesystem::path server_run_id_file;
  std::filesystem::path server_ready_file;

  std::string user;
  std::string log_dir;
  std::string hostname;
  std::string run_id;
  std::string trace_dir_pattern;
  std::string inclusion_paths;

  std::vector<std::shared_ptr<Probe>> runtime_probes;
  std::unordered_map<uint64_t, std::pair<std::string, std::string>> category_map;
  std::unordered_map<std::string, uint64_t> runtime_event_ids;
  std::unordered_map<uint64_t, RuntimeEventMetadata> runtime_event_metadata;
  std::unordered_map<std::string, RuntimeProbeScopes> invalid_runtime_probes;
  std::unordered_map<std::string, RuntimeProbeScopes> successful_runtime_probes;

  bool is_known_invalid_runtime_probe(const std::shared_ptr<Probe>& probe,
                                      const std::string& function_name) const;
  void record_invalid_runtime_probe(const std::shared_ptr<Probe>& probe,
                                    const std::string& function_name);
  void record_successful_runtime_probe(const std::shared_ptr<Probe>& probe,
                                       const std::string& function_name);
  void persist_runtime_probe_state() const;

 private:
  void derive_configurations();
  void load_runtime_system_configuration();
  void load_runtime_probe_file();
  void load_runtime_probe_state();
  void validate_configurations() const;
};

}  // namespace datacrumbs

#endif
