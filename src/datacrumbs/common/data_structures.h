#pragma once

#include <datacrumbs/common/enumerations.h>
#include <json-c/json.h>

#include <string>
#include <vector>
namespace datacrumbs {
class Probe {
 public:
  Probe(ProbeType _type) : type(_type) {}
  ProbeType type;  // The type of probe
  std::string name;
  std::vector<std::string> functions;  // Arguments for the probe
  virtual bool validate() const {
    if (name.empty()) return false;
    if (functions.empty()) return false;
    return true;
  }
  virtual json_object* toJson() const {
    json_object* j = json_object_new_object();
    json_object_object_add(j, "type", json_object_new_int(static_cast<int>(type)));
    json_object_object_add(j, "name", json_object_new_string(name.c_str()));

    json_object* funcs = json_object_new_array();
    for (const auto& func : functions) {
      json_object_array_add(funcs, json_object_new_string(func.c_str()));
    }
    json_object_object_add(j, "functions", funcs);

    return j;
  }

  static Probe fromJson(const json_object* j) {
    Probe p(static_cast<ProbeType>(json_object_get_int(json_object_object_get(j, "type"))));
    json_object* name_obj = json_object_object_get(j, "name");
    if (name_obj) p.name = json_object_get_string(name_obj);

    json_object* funcs_obj = json_object_object_get(j, "functions");
    if (funcs_obj && json_object_get_type(funcs_obj) == json_type_array) {
      int len = json_object_array_length(funcs_obj);
      for (int i = 0; i < len; ++i) {
        json_object* func = json_object_array_get_idx(funcs_obj, i);
        if (func) p.functions.push_back(json_object_get_string(func));
      }
    }
    return p;
  }
};

struct SysCallProbe : public Probe {
 public:
  SysCallProbe() : Probe(ProbeType::SYSCALLS) {}
};

struct KProbe : public Probe {
 public:
  KProbe() : Probe(ProbeType::KPROBE) {}
};

struct UProbe : public Probe {
 public:
  UProbe() : Probe(ProbeType::UPROBE), binary_path() {}
  std::string binary_path;  // Path to the binary
  bool validate() const override {
    if (!Probe::validate()) return false;
    if (binary_path.empty()) return false;
    return true;
  }
  json_object* toJson() const override {
    json_object* j = Probe::toJson();
    json_object_object_add(j, "binary_path", json_object_new_string(binary_path.c_str()));
    return j;
  }

  static UProbe fromJson(const json_object* j) {
    UProbe p;
    Probe base = Probe::fromJson(j);
    p.type = base.type;
    p.name = base.name;
    p.functions = base.functions;

    json_object* bin_obj = json_object_object_get(j, "binary_path");
    if (bin_obj) p.binary_path = json_object_get_string(bin_obj);

    return p;
  }
};

struct USDTProbe : public Probe {
 public:
  USDTProbe() : Probe(ProbeType::USDT), binary_path(), provider() {}
  std::string binary_path;  // Path to the binary
  std::string provider;     // USDT provider name
  bool validate() const override {
    if (!Probe::validate()) return false;
    if (binary_path.empty()) return false;
    if (provider.empty()) return false;
    return true;
  }
  json_object* toJson() const override {
    json_object* j = Probe::toJson();
    json_object_object_add(j, "binary_path", json_object_new_string(binary_path.c_str()));
    json_object_object_add(j, "provider", json_object_new_string(provider.c_str()));
    return j;
  }

  static USDTProbe fromJson(const json_object* j) {
    USDTProbe p;
    Probe base = Probe::fromJson(j);
    p.type = base.type;
    p.name = base.name;
    p.functions = base.functions;

    json_object* bin_obj = json_object_object_get(j, "binary_path");
    if (bin_obj) p.binary_path = json_object_get_string(bin_obj);

    json_object* provider_obj = json_object_object_get(j, "provider");
    if (provider_obj) p.provider = json_object_get_string(provider_obj);

    return p;
  }
};

class CaptureProbe {
 public:
  CaptureProbe(CaptureType _type) : type(_type) {}
  CaptureType type;  // The type of probe
  std::string regex;
  std::string name;
  ProbeType probe_type;  // Type of probe (e.g., SYSCALLS, KPROBE, UPROBE, USDT)
};
class KernelCaptureProbe : public CaptureProbe {
 public:
  KernelCaptureProbe() : CaptureProbe(CaptureType::KSYM) {}
};
class HeaderCaptureProbe : public CaptureProbe {
 public:
  HeaderCaptureProbe() : CaptureProbe(CaptureType::HEADER), file() {}
  std::string file;  // Name of the header to capture
};
class BinaryCaptureProbe : public CaptureProbe {
 public:
  BinaryCaptureProbe() : CaptureProbe(CaptureType::BINARY), file() {}
  std::string file;  // Path to the binary
};

class USDTCaptureProbe : public CaptureProbe {
 public:
  USDTCaptureProbe() : CaptureProbe(CaptureType::USDT), binary_path(), provider() {}
  std::string binary_path;  // Path to the binary
  std::string provider;     // USDT provider name
};

}  // namespace datacrumbs