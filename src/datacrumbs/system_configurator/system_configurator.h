#ifndef DATACRUMBS_SYSTEM_CONFIGURATOR_SYSTEM_CONFIGURATOR_H__
#define DATACRUMBS_SYSTEM_CONFIGURATOR_SYSTEM_CONFIGURATOR_H__

#include <string>

struct json_object;

class SystemConfigurator {
 public:
  int run();

 private:
  std::string current_hostname() const;
  void add_string(json_object* root, const char* key, const std::string& value) const;
  std::string optional_macro_string(const char* value) const;
  void enforce_secret_ownership_and_mode() const;
  void add_system_configuration(json_object* root) const;
};

#endif
