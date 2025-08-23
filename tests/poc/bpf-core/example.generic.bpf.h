#ifndef EXAMPLE_GENERIC_H
#define EXAMPLE_GENERIC_H

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
  struct {                              \
    __uint(type, BPF_MAP_TYPE_RINGBUF); \
    __uint(max_entries, 1024 * 1024);   \
  } name SEC(".maps");

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

#define DATACRUMBS_BPF_RING_BUF(...) DATACRUMBS_BPF_RING_BUF_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

char message[12] = "Hello World";
// anonymous struct assigned to rb variable
DATACRUMBS_BPF_RING_BUF(output);
DATACRUMBS_BPF_RING_BUF(event, 1024 * 1024);

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
#endif  // EXAMPLE_GENERIC_H