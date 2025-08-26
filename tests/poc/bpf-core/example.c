#include "example.h"

#include <bpf/libbpf.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "example.skel.h"
static int libbpf_print_fn(enum libbpf_print_level level, const char* format, va_list args) {
  if (level >= LIBBPF_DEBUG) return 0;
  return vfprintf(stderr, format, args);
}
int handle_event(void* ctx, void* data, size_t data_sz) {
  struct event_t* m = data;
  if (m->pid == 0 || m->event_id == 0) {
    return 0;
  }
  printf("%-6d  %-6u\n", m->pid, m->event_id);
  return 0;
}
void lost_event(void* ctx, int cpu, long long unsigned int data_sz) {
  printf("lost event\n");
}
int main() {
  struct merged_bpf* skel;
  int err;
  struct ring_buffer* rb = NULL;
  libbpf_set_print(libbpf_print_fn);
  skel = merged_bpf__open_and_load();
  if (!skel) {
    printf("Failed to open BPF object\n");
    return 1;
  }
  err = merged_bpf__attach(skel);
  if (err) {
    fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
    merged_bpf__destroy(skel);
    return 1;
  }
  rb = ring_buffer__new(bpf_map__fd(skel->maps.output), handle_event, NULL, NULL);
  if (!rb) {
    err = -1;
    fprintf(stderr, "Failed to create ring buffer\n");
    merged_bpf__destroy(skel);
    return 1;
  }
  while (true) {
    err = ring_buffer__poll(rb, 30000);
    // Ctrl-C gives -EINTR
    if (err == -EINTR) {
      err = 0;
      break;
    }
    if (err < 0) {
      printf("Error polling ring buffer: %d\n", err);
      break;
    }
  }
  ring_buffer__free(rb);
  merged_bpf__destroy(skel);
  return -err;
}