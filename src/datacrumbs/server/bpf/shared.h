#ifndef DATACRUMBS_SERVER_BPF_SHARED_H
#define DATACRUMBS_SERVER_BPF_SHARED_H

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
  char str[MAX_STR_READ_LEN];
};

#endif  // DATACRUMBS_SERVER_BPF_SHARED_H