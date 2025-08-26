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