#ifndef DATACRUMBS_SIGNER_SIGN_PROBES_SERVICE_H__
#define DATACRUMBS_SIGNER_SIGN_PROBES_SERVICE_H__

#include <filesystem>
#include <string>

class SignProbesService {
 public:
  SignProbesService();
  ~SignProbesService();

  int run();

 private:
  bool initialize_socket();
  void handle_client(int client_fd) const;
  std::string build_response(bool ok, const std::string& payload, const std::string& error) const;
  bool read_all_from_fd(int fd, std::string* payload) const;
  bool write_all_to_fd(int fd, const std::string& payload) const;
  std::string sign_signing_payload(const std::string& request_payload, std::string* error) const;
  bool is_authorized_client(int client_fd, std::string* error) const;

  std::filesystem::path socket_path_;
  std::string allowed_client_executable_name_;
  int server_fd_ = -1;
};

#endif
