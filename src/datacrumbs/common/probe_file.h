#ifndef DATACRUMBS_COMMON_PROBE_FILE_H__
#define DATACRUMBS_COMMON_PROBE_FILE_H__

#include <fcntl.h>
#include <json-c/json.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace datacrumbs::probe_file {

inline std::filesystem::path secret_path() {
  return DATACRUMBS_PROBE_SECRET_FILE;
}

inline std::string bytes_to_hex(const unsigned char* data, std::size_t size) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (std::size_t i = 0; i < size; ++i) {
    oss << std::setw(2) << static_cast<unsigned int>(data[i]);
  }
  return oss.str();
}

inline std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

inline std::string read_probe_payload(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    return "";
  }

  unsigned char magic[2] = {0, 0};
  input.read(reinterpret_cast<char*>(magic), sizeof(magic));
  input.close();

  const bool is_gzip = magic[0] == 0x1f && magic[1] == 0x8b;
  if (!is_gzip) {
    return read_text_file(path);
  }

  gzFile gz_file = gzopen(path.string().c_str(), "rb");
  if (gz_file == nullptr) {
    return "";
  }

  std::string payload;
  char buffer[4096];
  int read_bytes = 0;
  while ((read_bytes = gzread(gz_file, buffer, sizeof(buffer))) > 0) {
    payload.append(buffer, read_bytes);
  }
  gzclose(gz_file);
  return payload;
}

inline bool write_owner_only_file(const std::filesystem::path& path, const std::string& content) {
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    return false;
  }

  const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    return false;
  }

  const ssize_t written = write(fd, content.data(), content.size());
  const bool chown_ok = (geteuid() != 0) || (fchown(fd, 0, 0) == 0);
  const bool chmod_ok = (fchmod(fd, S_IRUSR) == 0);
  const bool close_ok = (close(fd) == 0);
  return written == static_cast<ssize_t>(content.size()) && chown_ok && chmod_ok && close_ok;
}

inline bool write_gzip_file(const std::filesystem::path& path, const std::string& payload) {
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    return false;
  }

  gzFile gz_file = gzopen(path.string().c_str(), "wb");
  if (gz_file == nullptr) {
    return false;
  }

  const int written = gzwrite(gz_file, payload.data(), static_cast<unsigned int>(payload.size()));
  const int close_status = gzclose(gz_file);
  chmod(path.c_str(), S_IRUSR | S_IWUSR);
  return written > 0 && close_status == Z_OK;
}

inline bool ensure_probe_secret(std::string* secret_out = nullptr) {
  const auto path = secret_path();
  std::string secret = read_text_file(path);
  if (!secret.empty()) {
    if (geteuid() == 0) {
      chown(path.c_str(), 0, 0);
    }
    chmod(path.c_str(), S_IRUSR);
    if (secret_out != nullptr) {
      *secret_out = secret;
    }
    return true;
  }

  std::error_code ec;
  if (std::filesystem::exists(path, ec) && geteuid() != 0) {
    return false;
  }
  if (geteuid() != 0) {
    return false;
  }

  unsigned char random_bytes[32];
  if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
    return false;
  }

  secret = bytes_to_hex(random_bytes, sizeof(random_bytes));
  if (!write_owner_only_file(path, secret)) {
    return false;
  }

  if (secret_out != nullptr) {
    *secret_out = secret;
  }
  return true;
}

inline std::string hmac_sha256_hex(const std::string& secret, const std::string& payload) {
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len = 0;
  if (HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
           reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), digest,
           &digest_len) == nullptr) {
    return "";
  }
  return bytes_to_hex(digest, digest_len);
}

inline std::string categories_payload(json_object* categories) {
  const char* payload = json_object_to_json_string_ext(categories, JSON_C_TO_STRING_PLAIN);
  return payload != nullptr ? payload : "";
}

inline std::string signed_document_payload(json_object* summary, json_object* categories) {
  json_object* root = json_object_new_object();
  json_object_object_add(root, "summary", json_object_get(summary));
  json_object_object_add(root, "categories", json_object_get(categories));
  json_object_object_add(root, "checksum_algorithm", json_object_new_string("hmac-sha256"));
  const char* payload = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
  const std::string result = payload != nullptr ? payload : "";
  json_object_put(root);
  return result;
}

inline json_object* build_signed_categories_document(
    json_object* categories, const std::unordered_map<std::string, std::string>& summary_fields,
    const std::string& secret) {
  json_object* root = json_object_new_object();
  json_object* summary = json_object_new_object();

  for (const auto& [key, value] : summary_fields) {
    json_object_object_add(summary, key.c_str(), json_object_new_string(value.c_str()));
  }

  json_object_object_add(root, "summary", summary);
  json_object_object_add(root, "categories", json_object_get(categories));
  json_object_object_add(root, "checksum_algorithm", json_object_new_string("hmac-sha256"));
  const std::string payload = signed_document_payload(summary, categories);
  json_object_object_add(root, "checksum",
                         json_object_new_string(hmac_sha256_hex(secret, payload).c_str()));
  return root;
}

inline json_object* verified_categories_from_root(json_object* root, const std::string& secret,
                                                  std::string* error = nullptr) {
  if (root == nullptr || json_object_get_type(root) != json_type_object) {
    if (error != nullptr) {
      *error = "probe file root must be a JSON object";
    }
    return nullptr;
  }

  json_object* categories = nullptr;
  json_object* summary = nullptr;
  json_object* checksum_obj = nullptr;
  json_object* algorithm_obj = nullptr;
  if (!json_object_object_get_ex(root, "summary", &summary) ||
      json_object_get_type(summary) != json_type_object) {
    if (error != nullptr) {
      *error = "probe file is missing summary object";
    }
    return nullptr;
  }
  if (!json_object_object_get_ex(root, "categories", &categories) ||
      json_object_get_type(categories) != json_type_array) {
    if (error != nullptr) {
      *error = "probe file is missing a categories array";
    }
    return nullptr;
  }

#ifdef DATACRUMBS_DISABLE_PROBE_SIGNING
  (void)secret;
  return json_object_get(categories);
#else
  if (!json_object_object_get_ex(root, "checksum", &checksum_obj) ||
      json_object_get_type(checksum_obj) != json_type_string) {
    if (error != nullptr) {
      *error = "probe file is missing checksum";
    }
    return nullptr;
  }
  if (!json_object_object_get_ex(root, "checksum_algorithm", &algorithm_obj) ||
      json_object_get_type(algorithm_obj) != json_type_string ||
      std::string(json_object_get_string(algorithm_obj)) != "hmac-sha256") {
    if (error != nullptr) {
      *error = "probe file checksum algorithm is unsupported";
    }
    return nullptr;
  }

  const std::string expected =
      hmac_sha256_hex(secret, signed_document_payload(summary, categories));
  const std::string actual = json_object_get_string(checksum_obj);
  if (expected.empty() || actual != expected) {
    if (error != nullptr) {
      *error = "probe file checksum verification failed";
    }
    return nullptr;
  }

  return json_object_get(categories);
#endif
}

inline json_object* load_verified_categories_from_file(const std::filesystem::path& path,
                                                       std::string* error = nullptr) {
#ifdef DATACRUMBS_DISABLE_PROBE_SIGNING
  const std::string payload = read_probe_payload(path);
  if (payload.empty()) {
    if (error != nullptr) {
      *error = "failed to read probe file";
    }
    return nullptr;
  }

  json_object* root = json_tokener_parse(payload.c_str());
  if (root == nullptr) {
    if (error != nullptr) {
      *error = "failed to parse probe file";
    }
    return nullptr;
  }

  json_object* categories = verified_categories_from_root(root, "", error);
  json_object_put(root);
  return categories;
#else
  std::string secret;
  if (!ensure_probe_secret(&secret)) {
    if (error != nullptr) {
      *error = "failed to read or create probe signing secret";
    }
    return nullptr;
  }

  const std::string payload = read_probe_payload(path);
  if (payload.empty()) {
    if (error != nullptr) {
      *error = "failed to read probe file";
    }
    return nullptr;
  }

  json_object* root = json_tokener_parse(payload.c_str());
  if (root == nullptr) {
    if (error != nullptr) {
      *error = "failed to parse probe file";
    }
    return nullptr;
  }

  json_object* categories = verified_categories_from_root(root, secret, error);
  json_object_put(root);
  return categories;
#endif
}

}  // namespace datacrumbs::probe_file

#endif
