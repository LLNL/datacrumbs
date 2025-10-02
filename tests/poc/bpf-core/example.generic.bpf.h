#ifndef __EXAMPLE_GENERIC_H
#define __EXAMPLE_GENERIC_H

#include "vmlinux.h"
//
#include <bpf/bpf_core_read.h>
//
#include <bpf/bpf_helpers.h>
//
#include <bpf/bpf_tracing.h>
//
#include <bpf/usdt.bpf.h>
//
#include "example.h"

#define DATACRUMBS_BPF_RING_BUF(name)   \
  extern struct {                       \
    __uint(type, BPF_MAP_TYPE_RINGBUF); \
    __uint(max_entries, 1024 * 1024);   \
  } name SEC(".maps");

struct lpm_key {
  __u32 prefixlen;
  __u32 addr;
};

// Define the value structure for the LPM_TRIE.
// This can be any data type, like an integer.
typedef int lpm_value_t;

extern struct {
  __uint(type, BPF_MAP_TYPE_LPM_TRIE);
  __uint(max_entries, 256);
  __type(key, struct lpm_key);
  __type(value, lpm_value_t);
  __uint(map_flags, BPF_F_NO_PREALLOC);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} lpm_trie_map SEC(".maps");

// anonymous struct assigned to rb variable
DATACRUMBS_BPF_RING_BUF(output);

static inline __attribute__((always_inline)) int generic_call(int event_id) {
  struct event_t* evt;
  evt = bpf_ringbuf_reserve(&output, sizeof(*evt), 0);
  if (!evt) {
    bpf_printk("Failed to reserve space in ring buffer\n");
    return 0;
  }
  unsigned long id = bpf_get_current_pid_tgid();
  int pid = id;
  evt->pid = pid;
  evt->event_id = event_id;
  bpf_ringbuf_submit(evt, 0);
  bpf_printk("Tracing function %d PID %d", event_id, pid);
  return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
#endif  // __EXAMPLE_GENERIC_H