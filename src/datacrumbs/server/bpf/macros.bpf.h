#ifndef DATACRUMBS_SERVER_BPF_MACROS_BPF_H
#define DATACRUMBS_SERVER_BPF_MACROS_BPF_H

/**
 * Macros for defining BPF ring buffers
 */
#define DATACRUMBS_BPF_RING_BUF_1_ARGS(name) \
  struct {                                   \
    __uint(type, BPF_MAP_TYPE_RINGBUF);      \
    __uint(max_entries, 1024 * 1024);        \
  } name SEC(".maps");
#define DATACRUMBS_BPF_RING_BUF_2_ARGS(name, size) \
  struct {                                         \
    __uint(type, BPF_MAP_TYPE_RINGBUF);            \
    __uint(max_entries, size);                     \
  } name SEC(".maps");

#define GET_3TH_ARG(arg1, arg2, arg3, ...) arg3
#define DATACRUMBS_BPF_RING_BUF_MACRO_CHOOSER(...) \
  GET_3TH_ARG(__VA_ARGS__, DATACRUMBS_BPF_RING_BUF_2_ARGS, DATACRUMBS_BPF_RING_BUF_1_ARGS, )

#define DATACRUMBS_RINGBUF(...) DATACRUMBS_BPF_RING_BUF_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

/**
 * Macro for defining a BPF map
 */

#define DATACRUMBS_MAP_3_ARGS(name, map_key, map_value) \
  struct {                                              \
    __uint(type, BPF_MAP_TYPE_HASH);                    \
    __uint(max_entries, 10240);                         \
    __type(key, map_key);                               \
    __type(value, map_value);                           \
  } name SEC(".maps");

#define DATACRUMBS_MAP_4_ARGS(name, map_key, map_value, size) \
  struct {                                                    \
    __uint(type, BPF_MAP_TYPE_HASH);                          \
    __uint(max_entries, size);                                \
    __type(key, map_key);                                     \
    __type(value, map_value);                                 \
  } name SEC(".maps");

#define GET_5TH_ARG(arg1, arg2, arg3, arg4, arg5, ...) arg5
#define DATACRUMBS_MAP_MACRO_CHOOSER(...) \
  GET_5TH_ARG(__VA_ARGS__, DATACRUMBS_MAP_4_ARGS, DATACRUMBS_MAP_3_ARGS, )

#define DATACRUMBS_MAP(...) DATACRUMBS_MAP_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

/**
 * Helper Macros
 */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#endif  // DATACRUMBS_SERVER_BPF_MACROS_BPF_H