#include "datacrumbs/signer/sign_probes_service.h"

#include <datacrumbs/common/logging.h>
#include <datacrumbs/common/probe_file.h>
#include <datacrumbs/datacrumbs_config.h>
#include <json-c/json.h>
#include <linux/limits.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <string>

SignProbesService::SignProbesService()
    : socket_path_(DATACRUMBS_SIGN_PROBES_SOCKET_PATH),
      allowed_client_executable_name_("datacrumbs_probe_configurator_exec") {}

SignProbesService::~SignProbesService() {
  if (server_fd_ >= 0) {
    close(server_fd_);
  }
  if (!socket_path_.empty()) {
    unlink(socket_path_.c_str());
  }
}

int SignProbesService::run() {
  signal(SIGPIPE, SIG_IGN);

  std::string secret;
  if (!datacrumbs::probe_file::ensure_probe_secret(&secret)) {
    DC_LOG_ERROR("Failed to create or read probe signing secret");
    return 1;
  }

  if (!initialize_socket()) {
    return 1;
  }

  DC_LOG_INFO("Datacrumbs probe signing service listening on %s", socket_path_.c_str());

  for (;;) {
    const int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
      if (errno == EINTR) {
        continue;
      }
      DC_LOG_ERROR("Failed to accept signer connection");
      continue;
    }
    try {
      handle_client(client_fd);
    } catch (const std::exception& ex) {
      const std::string response_payload = build_response(false, "", ex.what());
      write_all_to_fd(client_fd, response_payload);
      DC_LOG_ERROR("Signer request handling failed: %s", ex.what());
    } catch (...) {
      const std::string response_payload =
          build_response(false, "", "unexpected signer service error");
      write_all_to_fd(client_fd, response_payload);
      DC_LOG_ERROR("Signer request handling failed: unknown exception");
    }
    close(client_fd);
  }

  return 0;
}

bool SignProbesService::initialize_socket() {
  std::error_code ec;
  std::filesystem::create_directories(socket_path_.parent_path(), ec);
  if (ec) {
    DC_LOG_ERROR("Failed to create signer socket directory: %s",
                 socket_path_.parent_path().string().c_str());
    return false;
  }

  unlink(socket_path_.c_str());

  server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd_ < 0) {
    DC_LOG_ERROR("Failed to create signer socket");
    return false;
  }

  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  const std::string path_string = socket_path_.string();
  if (path_string.size() >= sizeof(address.sun_path)) {
    DC_LOG_ERROR("Signer socket path is too long: %s", path_string.c_str());
    return false;
  }
  std::strncpy(address.sun_path, path_string.c_str(), sizeof(address.sun_path) - 1);

  if (bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    DC_LOG_ERROR("Failed to bind signer socket at %s: %s", path_string.c_str(),
                 std::strerror(errno));
    return false;
  }

  chmod(socket_path_.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  if (listen(server_fd_, 16) != 0) {
    DC_LOG_ERROR("Failed to listen on signer socket");
    return false;
  }

  return true;
}

void SignProbesService::handle_client(int client_fd) const {
  std::string request_payload;
  std::string response_payload;
  std::string authorization_error;

  if (!is_authorized_client(client_fd, &authorization_error)) {
    response_payload = build_response(false, "", authorization_error);
    write_all_to_fd(client_fd, response_payload);
    return;
  }
  DC_LOG_INFO("Accepted signer request");

  if (!read_all_from_fd(client_fd, &request_payload)) {
    response_payload = build_response(false, "", "failed to read signing request");
    write_all_to_fd(client_fd, response_payload);
    return;
  }

  std::string error;
  std::string checksum = sign_signing_payload(request_payload, &error);
  if (checksum.empty()) {
    response_payload = build_response(false, "", error);
  } else {
    DC_LOG_INFO("Successfully signed probe payload");
    response_payload = build_response(true, checksum, "");
  }
  write_all_to_fd(client_fd, response_payload);
}

std::string SignProbesService::build_response(bool ok, const std::string& payload,
                                              const std::string& error) const {
  json_object* root = json_object_new_object();
  json_object_object_add(root, "ok", json_object_new_boolean(ok ? 1 : 0));
  if (!payload.empty()) {
    json_object_object_add(root, "payload", json_object_new_string(payload.c_str()));
  }
  if (!error.empty()) {
    json_object_object_add(root, "error", json_object_new_string(error.c_str()));
  }
  const char* response = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
  std::string output = response != nullptr ? response : "";
  json_object_put(root);
  return output;
}

bool SignProbesService::read_all_from_fd(int fd, std::string* payload) const {
  char buffer[4096];
  payload->clear();
  ssize_t read_bytes = 0;
  for (;;) {
    read_bytes = read(fd, buffer, sizeof(buffer));
    if (read_bytes > 0) {
      payload->append(buffer, static_cast<std::size_t>(read_bytes));
      continue;
    }
    if (read_bytes < 0 && errno == EINTR) {
      continue;
    }
    break;
  }
  return read_bytes == 0;
}

bool SignProbesService::write_all_to_fd(int fd, const std::string& payload) const {
  std::size_t total_written = 0;
  while (total_written < payload.size()) {
    const ssize_t written =
        write(fd, payload.data() + total_written, payload.size() - total_written);
    if (written < 0 && errno == EINTR) {
      continue;
    }
    if (written <= 0) {
      return false;
    }
    total_written += static_cast<std::size_t>(written);
  }
  return true;
}

std::string SignProbesService::sign_signing_payload(const std::string& request_payload,
                                                    std::string* error) const {
  json_object* root = json_tokener_parse(request_payload.c_str());
  if (root == nullptr || json_object_get_type(root) != json_type_object) {
    if (error != nullptr) {
      *error = "failed to parse signing request";
    }
    if (root != nullptr) {
      json_object_put(root);
    }
    return "";
  }

  json_object* signing_payload = nullptr;
  if (!json_object_object_get_ex(root, "signing_payload", &signing_payload) ||
      json_object_get_type(signing_payload) != json_type_string) {
    if (error != nullptr) {
      *error = "signing request missing signing payload";
    }
    json_object_put(root);
    return "";
  }

  std::string secret;
  if (!datacrumbs::probe_file::ensure_probe_secret(&secret)) {
    if (error != nullptr) {
      *error = "failed to read signing secret";
    }
    json_object_put(root);
    return "";
  }

  const std::string result =
      datacrumbs::probe_file::hmac_sha256_hex(secret, json_object_get_string(signing_payload));
  json_object_put(root);
  if (result.empty() && error != nullptr) {
    *error = "failed to compute probe signature";
  }
  return result;
}

bool SignProbesService::is_authorized_client(int client_fd, std::string* error) const {
  struct ucred credentials{};
  socklen_t credentials_size = sizeof(credentials);
  if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &credentials, &credentials_size) != 0) {
    if (error != nullptr) {
      *error = "failed to get peer credentials";
    }
    return false;
  }

  char exe_path_buffer[PATH_MAX] = {0};
  const std::string proc_exe = "/proc/" + std::to_string(credentials.pid) + "/exe";
  const ssize_t exe_len = readlink(proc_exe.c_str(), exe_path_buffer, sizeof(exe_path_buffer) - 1);
  if (exe_len <= 0) {
    if (error != nullptr) {
      *error = "failed to resolve peer executable path";
    }
    return false;
  }
  exe_path_buffer[exe_len] = '\0';

  std::error_code ec;
  const auto peer_executable = std::filesystem::weakly_canonical(exe_path_buffer, ec);
  if (ec) {
    if (error != nullptr) {
      *error = "failed to canonicalize peer executable path";
    }
    return false;
  }

  const std::string peer_executable_name = peer_executable.filename().string();
  if (peer_executable_name != allowed_client_executable_name_) {
    if (error != nullptr) {
      *error = "probe signing requests are only accepted from datacrumbs_probe_configurator";
    }
    DC_LOG_WARN("Rejected signer request from pid=%d exe=%s expected-name=%s", credentials.pid,
                peer_executable.string().c_str(), allowed_client_executable_name_.c_str());
    return false;
  }

  return true;
}
