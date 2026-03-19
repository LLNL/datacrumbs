#ifndef DATACRUMBS_SERVER_BPF_SHARED_H
#define DATACRUMBS_SERVER_BPF_SHARED_H

#include <custom_probes_process.h>
#include <datacrumbs/datacrumbs_config.h>

static int DATACRUMBS_TS_KEY = 1;
static int DATACRUMBS_FAILED_EVENTS_KEY = 2;

#define DATACRUMBS_MAX_CAPTURE_ARGS 5
#define DATACRUMBS_MAX_CAPTURE_BYTES 64

enum datacrumbs_runtime_probe_kind_t {
  DATACRUMBS_RUNTIME_PROBE_KIND_KPROBE = 1,
  DATACRUMBS_RUNTIME_PROBE_KIND_UPROBE = 2,
  DATACRUMBS_RUNTIME_PROBE_KIND_SYSCALL = 3,
  DATACRUMBS_RUNTIME_PROBE_KIND_USDT = 4,
};

struct generic_event_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long ts;
  unsigned long long dur;
  unsigned int arg_count;
  unsigned long long args[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned int arg_data_len[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned int arg_data_status[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned char arg_data[DATACRUMBS_MAX_CAPTURE_ARGS][DATACRUMBS_MAX_CAPTURE_BYTES];
};
typedef struct generic_event_t general_event_t;
struct usdt_event_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long ts;
  unsigned long long dur;
};

#define MAX_STR_READ_LEN 256

struct fn_key_t {
  unsigned long long id;
  unsigned long long event_id;
};

struct fn_value_t {
  unsigned long long ts;
  unsigned int arg_count;
  unsigned long long args[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned int arg_data_len[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned int arg_data_status[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned char arg_data[DATACRUMBS_MAX_CAPTURE_ARGS][DATACRUMBS_MAX_CAPTURE_BYTES];
};

struct runtime_event_config_t {
  unsigned long long event_id;
  unsigned int probe_kind;
  unsigned int arg_count;
  unsigned int arg_index[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned int arg_num_bytes[DATACRUMBS_MAX_CAPTURE_ARGS];
  unsigned int arg_is_pointer[DATACRUMBS_MAX_CAPTURE_ARGS];
};

struct fn_t {
  struct fn_key_t key;
  struct fn_value_t value;
};

struct string_t {
  unsigned int len;
  char str[MAX_STR_READ_LEN];
};

struct profile_key_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long time_interval;
};

struct profile_value_t {
  unsigned long long duration;
  unsigned long long frequency;
};

struct usdt_profile_key_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long time_interval;
  unsigned int class_hash;
  unsigned int method_hash;
};

struct counter_event_t {
  struct profile_key_t* key;
  struct profile_value_t* value;
};

struct usdt_counter_event_t {
  struct usdt_profile_key_t* key;
  struct profile_value_t* value;
};

#endif  // DATACRUMBS_SERVER_BPF_SHARED_H
