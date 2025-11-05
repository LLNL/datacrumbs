#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "malloc.skel.h"

int main(int argc, char* argv[]) {
  struct malloc_bpf* skel;
  int err;

  skel = malloc_bpf__open_and_load();
  if (!skel) {
    fprintf(stderr, "Failed to open and load BPF skeleton\n");
    return 1;
  }

  err = malloc_bpf__attach(skel);
  if (err) {
    fprintf(stderr, "Failed to attach BPF program\n");
    goto cleanup;
  }

  printf("Successfully started! Watching malloc calls...\n");

  while (1) {
    sleep(2);
    __u32 key = 0;
    __u64 count;
    bpf_map_lookup_elem(bpf_map__fd(skel->maps.counts), &key, &count);
    printf("malloc calls: %llu\n", count);
  }

cleanup:
  malloc_bpf__destroy(skel);
  return -err;
}
