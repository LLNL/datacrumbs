#ifndef DATACRUMBS_SERVER_BPF_SHARED_H
#define DATACRUMBS_SERVER_BPF_SHARED_H


#ifndef DATACRUMBS_MODE
// 0 - no tracing, 1 - tracing, 2 - profiling
#define DATACRUMBS_MODE 2 
#endif

#define DATACRUMBS_TIME_MS 1000000

#ifndef DATACRUMBS_TIME_INTERVAL_MS
#define DATACRUMBS_TIME_INTERVAL_MS 1000
#endif

#ifndef DATACRUMBS_TRACE_ALL
#define DATACRUMBS_TRACE_ALL 0
#endif

struct general_event_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long ts;
  unsigned long long dur;
};

#define MAX_STR_READ_LEN 256
struct usdt_event_t {
  unsigned int type;
  unsigned long long id;
  unsigned long long event_id;
  unsigned long long ts;
  unsigned long long dur;
  unsigned int class_hash;
  unsigned int method_hash;
};

struct fn_key_t {
  unsigned long long id;
  unsigned long long event_id;
};

struct fn_value_t {
  unsigned long long ts;
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

#endif  // DATACRUMBS_SERVER_BPF_SHARED_H