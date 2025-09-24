// map_def.c
#include "vmlinux.h"
//
#include <bpf/bpf_core_read.h>
//
#include <bpf/bpf_helpers.h>
//
#include <bpf/bpf_tracing.h>
//
#include <bpf/usdt.bpf.h>

#define DATACRUMBS_BPF_RING_BUF(name)   \
  struct {                              \
    __uint(type, BPF_MAP_TYPE_RINGBUF); \
    __uint(max_entries, 1024 * 1024);   \
  } name SEC(".maps");

// anonymous struct assigned to rb variable
DATACRUMBS_BPF_RING_BUF(output);
