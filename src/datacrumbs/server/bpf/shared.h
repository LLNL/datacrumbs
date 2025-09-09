#ifndef DATACRUMBS_SERVER_BPF_SHARED_H
#define DATACRUMBS_SERVER_BPF_SHARED_H

#include <datacrumbs/datacrumbs_config.h>

static int DATACRUMBS_TS_KEY = 1;
static int DATACRUMBS_FAILED_EVENTS_KEY = 2;

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

struct counter_event_t {
  struct profile_key_t* key;
  struct profile_value_t* value;
};

struct usdt_counter_event_t {
  struct usdt_profile_key_t* key;
  struct profile_value_t* value;
};

#endif  // DATACRUMBS_SERVER_BPF_SHARED_H