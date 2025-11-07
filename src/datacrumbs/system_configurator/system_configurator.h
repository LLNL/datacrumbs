#ifndef DATACRUMBS_SYSTEM_CONFIGURATOR_SYSTEM_CONFIGURATOR_H__
#define DATACRUMBS_SYSTEM_CONFIGURATOR_SYSTEM_CONFIGURATOR_H__

#include <string>
#include <unordered_map>

class SystemConfigurator {
 public:
  int run();

 private:
  std::string current_hostname() const;
  std::string system_configuration_path() const;
  std::string optional_macro_string(const char* value) const;
  void enforce_secret_ownership_and_mode() const;
  std::unordered_map<std::string, std::string> system_configuration_values() const;
};

#endif
