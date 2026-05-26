#ifndef DATACRUMBS_MANAGER_PROBE_MANAGER_SERVICE_H__
#define DATACRUMBS_MANAGER_PROBE_MANAGER_SERVICE_H__

#include <string>
#include <vector>

class ProbeManagerService {
 public:
  ProbeManagerService();
  ~ProbeManagerService();

  int run();

 private:
  bool initialize_socket();
  void handle_client(int client_fd) const;
  std::string build_response(const std::string& request_id, bool ok, const std::string& payload,
                             const std::string& error, int error_code) const;
  bool read_all_from_fd(int fd, std::string* payload) const;
  bool write_all_to_fd(int fd, const std::string& payload) const;
  std::string sign_signing_payload(const std::string& signing_payload, std::string* error) const;
  bool report_runtime_probe_state(const std::string& state_payload, const std::string& state_hmac,
                                  std::string* error) const;
  bool validate_signing_payload(const std::string& signing_payload,
                                std::vector<std::string>* errors) const;
  bool persist_runtime_probe_state_payload(const std::string& state_payload,
                                           std::string* error) const;

  std::string host_;
  int port_;
  int server_fd_ = -1;
};

#endif
