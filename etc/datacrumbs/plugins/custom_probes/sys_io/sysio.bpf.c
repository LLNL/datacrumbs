#include "sysio.bpf.h"

#include <datacrumbs/server/bpf/common.h>

struct filename_t {
  char fname[256];
};
struct file_t {
  u64 id;
  int fd;
};
DATACRUMBS_MAP(file_hash, u64, struct filename_t, 10240);
DATACRUMBS_MAP(latest_hash, struct fn_key_t, u64);
DATACRUMBS_MAP(fd_hash, struct file_t, u64);
DATACRUMBS_MAP(latest_fd, u64, int);
DATACRUMBS_MAP(pid_hash, u64, u64);
#define USER_EVENT_ID_START 100000

static inline __attribute__((always_inline)) int sysio_fd_init(u64 event_id, int fd) {
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    bpf_printk("Skipping entry for pid:%d, event_id:%llu\n", (u32)key.id, key.event_id);
    return 0;  // not tracing this pid
  }
  struct fn_value_t fn = {};
  fn.ts = bpf_ktime_get_ns();
  bpf_map_update_elem(&fn_pid_map, &key, &fn, BPF_ANY);
  bpf_map_update_elem(&latest_fd, &key.id, &fd, BPF_ANY);
  bpf_printk("Pushed pid:%d, event_id:%llu to map\n", (u32)key.id, key.event_id);
  return 0;
}

static inline __attribute__((always_inline)) int sysio_data_exit(struct pt_regs* ctx,
                                                                 u64 event_id) {
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    bpf_printk("Skipping entry for pid:%d, event_id:%llu\n", (u32)key.id, key.event_id);
    return 0;  // not tracing this pid
  }
  struct fn_value_t* fn = bpf_map_lookup_elem(&fn_pid_map, &key);
  if (fn == 0) return 0;  // missed entry
  struct sysio_event_t* event;
  event = bpf_ringbuf_reserve(&output, sizeof(struct sysio_event_t), 0);
  if (!event) {
    bpf_printk("Failed to reserve space in ring buffer for stats for pid:%d, event_id:%llu\n",
               (u32)key.id, key.event_id);
    return 0;  // failed to reserve space
  }
  event->type = 2;
  event->id = key.id;
  event->event_id = key.event_id;
  event->ts = (fn->ts - start_ts);
  event->dur = bpf_ktime_get_ns() - fn->ts;
  event->size += PT_REGS_RC(ctx);
  int* fd_ptr = bpf_map_lookup_elem(&latest_fd, &key.id);
  if (fd_ptr != 0) {
    struct file_t file_key = {};
    file_key.id = key.id;
    file_key.fd = *fd_ptr;
    u64* hash_ptr = bpf_map_lookup_elem(&fd_hash, &file_key);
    if (hash_ptr != 0) {
      event->file_hash = *hash_ptr;
    }
  }
  bpf_ringbuf_submit(event, 0);
  bpf_printk("Pushed pid:%d, event_id:%llu to output\n", (u32)key.id, key.event_id);
  return 0;
}

static inline __attribute__((always_inline)) int sysio_metadata_exit(struct pt_regs* ctx,
                                                                     u64 event_id) {
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    bpf_printk("Skipping entry for pid:%d, event_id:%llu\n", (u32)key.id, key.event_id);
    return 0;  // not tracing this pid
  }
  struct fn_value_t* fn = bpf_map_lookup_elem(&fn_pid_map, &key);
  if (fn == 0) return 0;  // missed entry
  struct sysio_event_t* event;
  event = bpf_ringbuf_reserve(&output, sizeof(struct sysio_event_t), 0);
  if (!event) {
    bpf_printk("Failed to reserve space in ring buffer for stats for pid:%d, event_id:%llu\n",
               (u32)key.id, key.event_id);
    return 0;  // failed to reserve space
  }
  event->type = 2;
  event->id = key.id;
  event->event_id = key.event_id;
  event->ts = (fn->ts - start_ts);
  event->dur = bpf_ktime_get_ns() - fn->ts;
  int* fd_ptr = bpf_map_lookup_elem(&latest_fd, &key.id);
  if (fd_ptr != 0) {
    struct file_t file_key = {};
    file_key.id = key.id;
    file_key.fd = *fd_ptr;
    u64* hash_ptr = bpf_map_lookup_elem(&fd_hash, &file_key);
    if (hash_ptr != 0) {
      event->file_hash = *hash_ptr;
    }
  }
  bpf_ringbuf_submit(event, 0);
  bpf_printk("Pushed pid:%d, event_id:%llu to output\n", (u32)key.id, key.event_id);
  return 0;
}

SEC("ksyscall/openat")
int BPF_KPROBE(openat_entry, int dfd, const char* filename, int flags) {
  struct fn_key_t key = {};
  key.event_id = USER_EVENT_ID_START + 0;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    bpf_printk("Skipping entry for pid:%d, event_id:%llu\n", (u32)key.id, key.event_id);
    return 0;  // not tracing this pid
  }
  struct fn_value_t fn = {};
  fn.ts = bpf_ktime_get_ns();
  bpf_map_update_elem(&fn_pid_map, &key, &fn, BPF_ANY);
  bpf_printk("Pushed pid:%d, event_id:%llu to map\n", (u32)key.id, key.event_id);

  struct filename_t fname_i;
  u64 filename_len = sizeof(fname_i.fname);
  int len = bpf_probe_read_user_str(&fname_i.fname, filename_len, filename);
  (void)len;
  u64 filehash = get_hash(fname_i.fname, filename_len);
  bpf_map_update_elem(&file_hash, &filehash, &fname_i, BPF_ANY);
  bpf_map_update_elem(&latest_hash, &key, &filehash, BPF_ANY);
  bpf_printk("Pushed filehash:%llu for pid:%d, event_id:%llu to map\n", filehash, (u32)key.id,
             key.event_id);
  return 0;
}

SEC("kretsyscall/openat")
int BPF_KRETPROBE(openat_exit) {
  struct fn_key_t key = {};
  key.event_id = USER_EVENT_ID_START + 0;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    bpf_printk("Skipping entry for pid:%d, event_id:%llu\n", (u32)key.id, key.event_id);
    return 0;  // not tracing this pid
  }
  struct fn_value_t* fn = bpf_map_lookup_elem(&fn_pid_map, &key);
  if (fn == 0) return 0;  // missed entry
  struct sysio_event_t* event;
  event = bpf_ringbuf_reserve(&output, sizeof(struct sysio_event_t), 0);
  if (!event) {
    bpf_printk("Failed to reserve space in ring buffer for stats for pid:%d, event_id:%llu\n",
               (u32)key.id, key.event_id);
    return 0;  // failed to reserve space
  }

  event->type = 2;
  event->id = key.id;
  event->event_id = key.event_id;
  event->ts = (fn->ts - start_ts);
  event->dur = bpf_ktime_get_ns() - fn->ts;
  u64* hash_ptr = bpf_map_lookup_elem(&latest_hash, &key);
  if (hash_ptr != 0) {
    event->file_hash = *hash_ptr;
    int fd = PT_REGS_RC(ctx);
    struct file_t file_key = {};
    file_key.id = key.id;
    file_key.fd = fd;
    bpf_map_update_elem(&fd_hash, &file_key, &hash_ptr, BPF_ANY);
  }
  bpf_ringbuf_submit(event, 0);
  bpf_printk("Pushed pid:%d, event_id:%llu to output\n", (u32)key.id, key.event_id);
  return 0;
}

SEC("ksyscall/read")
int BPF_KPROBE(read_entry, int fd, void* data, u64 count) {
  return sysio_fd_init(USER_EVENT_ID_START + 1, fd);
}
SEC("kretsyscall/read")
int BPF_KRETPROBE(read_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 1);
}
SEC("ksyscall/write")
int BPF_KPROBE(write_entry, int fd, const void* data, u64 count) {
  return sysio_fd_init(USER_EVENT_ID_START + 2, fd);
}
SEC("kretsyscall/write")
int BPF_KRETPROBE(write_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 2);
}
SEC("ksyscall/close")
int BPF_KPROBE(close_entry, int fd) {
  return sysio_fd_init(USER_EVENT_ID_START + 3, fd);
}
SEC("kretsyscall/close")
int BPF_KRETPROBE(close_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 3);
}
SEC("ksyscall/fallocate")
int BPF_KPROBE(fallocate_entry, int fd, int mode, int offset, int len) {
  return sysio_fd_init(USER_EVENT_ID_START + 4, fd);
}
SEC("kretsyscall/fallocate")
int BPF_KRETPROBE(fallocate_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 4);
}
SEC("ksyscall/fdatasync")
int BPF_KPROBE(fdatasync_entry, int fd) {
  return sysio_fd_init(USER_EVENT_ID_START + 5, fd);
}
SEC("kretsyscall/fdatasync")
int BPF_KRETPROBE(fdatasync_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 5);
}
SEC("ksyscall/flock")
int BPF_KPROBE(flock_entry, int fd, int cmd) {
  return sysio_fd_init(USER_EVENT_ID_START + 6, fd);
}
SEC("kretsyscall/flock")
int BPF_KRETPROBE(flock_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 6);
}
SEC("ksyscall/fsync")
int BPF_KPROBE(fsync_entry, int fd) {
  return sysio_fd_init(USER_EVENT_ID_START + 7, fd);
}
SEC("kretsyscall/fsync")
int BPF_KRETPROBE(fsync_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 7);
}
SEC("ksyscall/ftruncate")
int BPF_KPROBE(ftruncate_entry, int fd, int length) {
  return sysio_fd_init(USER_EVENT_ID_START + 8, fd);
}
SEC("kretsyscall/ftruncate")
int BPF_KRETPROBE(ftruncate_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 8);
}
SEC("ksyscall/lseek")
int BPF_KPROBE(lseek_entry, int fd, int offset, int whence) {
  return sysio_fd_init(USER_EVENT_ID_START + 9, fd);
}
SEC("kretsyscall/lseek")
int BPF_KRETPROBE(lseek_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 9);
}
SEC("ksyscall/pread64")
int BPF_KPROBE(pread64_entry, int fd, void* buf, u64 count, u64 pos) {
  return sysio_fd_init(USER_EVENT_ID_START + 10, fd);
}
SEC("kretsyscall/pread64")
int BPF_KRETPROBE(pread64_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 10);
}

SEC("ksyscall/preadv")
int BPF_KPROBE(preadv_entry, int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h) {
  return sysio_fd_init(USER_EVENT_ID_START + 11, fd);
}
SEC("kretsyscall/preadv")
int BPF_KRETPROBE(preadv_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 11);
}
SEC("ksyscall/preadv2")
int BPF_KPROBE(preadv2_entry, int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h, u64 flags) {
  return sysio_fd_init(USER_EVENT_ID_START + 12, fd);
}
SEC("kretsyscall/preadv2")
int BPF_KRETPROBE(preadv2_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 12);
}
SEC("ksyscall/pwrite64")
int BPF_KPROBE(pwrite64_entry, int fd, const void* data, u64 count, u64 pos) {
  return sysio_fd_init(USER_EVENT_ID_START + 13, fd);
}
SEC("kretsyscall/pwrite64")
int BPF_KRETPROBE(pwrite64_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 13);
}
SEC("ksyscall/pwritev")
int BPF_KPROBE(pwritev_entry, int fd, u64 vec, u64 vlen) {
  return sysio_fd_init(USER_EVENT_ID_START + 14, fd);
}
SEC("kretsyscall/pwritev")
int BPF_KRETPROBE(pwritev_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 14);
}
SEC("ksyscall/pwritev2")
int BPF_KPROBE(pwritev2_entry, int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h, u64 flags) {
  return sysio_fd_init(USER_EVENT_ID_START + 15, fd);
}
SEC("kretsyscall/pwritev2")
int BPF_KRETPROBE(pwritev2_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 15);
}
SEC("ksyscall/readahead")
int BPF_KPROBE(readahead_entry, int fd, u64 offset, u64 count) {
  return sysio_fd_init(USER_EVENT_ID_START + 16, fd);
}
SEC("kretsyscall/readahead")
int BPF_KRETPROBE(readahead_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 16);
}
SEC("ksyscall/readv")
int BPF_KPROBE(readv_entry, int fd, u64 vec, u64 vlen) {
  return sysio_fd_init(USER_EVENT_ID_START + 17, fd);
}
SEC("kretsyscall/readv")
int BPF_KRETPROBE(readv_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 17);
}
SEC("ksyscall/writev")
int BPF_KPROBE(writev_entry, int fd, u64 vec, u64 vlen) {
  return sysio_fd_init(USER_EVENT_ID_START + 18, fd);
}
SEC("kretsyscall/writev")
int BPF_KRETPROBE(writev_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 18);
}