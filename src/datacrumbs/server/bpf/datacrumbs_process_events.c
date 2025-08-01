#include <bpf/libbpf.h>
#include <datacrumbs/bpf/datacrumbs.skel.h>
#include <datacrumbs/server/bpf/shared.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
static int libbpf_print_fn(enum libbpf_print_level level, const char* format, va_list args) {
  if (level >= LIBBPF_DEBUG) return 0;
  return vfprintf(stderr, format, args);
}
int handle_event(void* ctx, void* data, size_t data_sz) {
  struct general_event_t* m = data;
  unsigned int pid = m->id;
  if (pid == 0) {
    return 0;
  }
  printf("%-6u  %-6llu %-6llu %-6llu \n", pid, m->event_id, m->ts, m->dur);
  return 0;
}
void lost_event(void* ctx, int cpu, long long unsigned int data_sz) {
  printf("lost event\n");
}
int main() {
  struct datacrumbs_bpf* skel;
  int err;
  struct ring_buffer* rb = NULL;
  libbpf_set_print(libbpf_print_fn);
  skel = datacrumbs_bpf__open_and_load();
  if (!skel) {
    printf("Failed to open BPF object\n");
    return 1;
  }
  err = datacrumbs_bpf__attach(skel);
  if (err) {
    fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
    datacrumbs_bpf__destroy(skel);
    return 1;
  }

  rb = ring_buffer__new(bpf_map__fd(skel->maps.output), handle_event, NULL, NULL);
  if (!rb) {
    err = -1;
    fprintf(stderr, "Failed to create ring buffer\n");
    datacrumbs_bpf__destroy(skel);
    return 1;
  }
  printf("%-6s  %-6s %-6s %-6s\n", "PID", "EID", "TS", "DUR");
  printf("Ready to run the code.\n");
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
  datacrumbs_bpf__destroy(skel);
  return -err;
}