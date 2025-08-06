#include "sysio.bpf.h"

#include <datacrumbs/server/bpf/common.h>

struct file_t {
  u64 id;
  int fd;
};

DATACRUMBS_MAP(latest_fname, struct fn_key_t, u32);
DATACRUMBS_MAP(fd_fname, struct file_t, u32);
DATACRUMBS_MAP(latest_fd, struct fn_key_t, int);
#define USER_EVENT_ID_START 100000

static inline __attribute__((always_inline)) int sysio_fd_init(u64 event_id, int fd) {
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    return 0;  // not tracing this pid
  }
  bpf_map_update_elem(&latest_fd, &key, &fd, BPF_ANY);
  struct fn_value_t fn = {};
  fn.ts = bpf_ktime_get_ns();
  bpf_map_update_elem(&fn_pid_map, &key, &fn, BPF_ANY);
  DBG_PRINTK("Pushed pid:%d, event_id:%llu to map\n", (u32)key.id, key.event_id);
  return 0;
}

static inline __attribute__((always_inline)) int sysio_data_exit(struct pt_regs* ctx,
                                                                 u64 event_id) {
  u64 te = bpf_ktime_get_ns();
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    return 0;  // not tracing this pid
  }
  struct fn_value_t* fn = bpf_map_lookup_elem(&fn_pid_map, &key);
  if (fn == 0) return 0;  // missed entry
  DATACRUMBS_SKIP_SMALL_EVENTS(fn, te);
  struct sysio_event_t* event;
  DATACRUMBS_RB_RESERVE(output, struct sysio_event_t, event);
  event->type = 2;
  event->id = key.id;
  event->event_id = key.event_id;
  DATACRUMBS_COLLECT_TIME(event);
  event->size = 0;
  event->fhash = 0;  // Initialize fhash to empty
  event->size += PT_REGS_RC(ctx);
  int* fd_ptr = bpf_map_lookup_elem(&latest_fd, &key);
  if (fd_ptr != 0) {
    DBG_PRINTK("Found fd:%d, event_id:%llu\n", *fd_ptr, key.event_id);
    struct file_t file_key = {};
    file_key.id = key.id;
    file_key.fd = *fd_ptr;
    u32* fhash = bpf_map_lookup_elem(&fd_fname, &file_key);
    if (fhash != 0) {
      event->fhash = *fhash;
      DBG_PRINTK("Found fd:%d, file:%u, event_id:%llu\n", *fd_ptr, *fhash, key.event_id);
    } else {
      DBG_PRINTK("Not Found fd:%d, file:%u, event_id:%llu\n", *fd_ptr, *fhash, key.event_id);
    }
  } else {
    DBG_PRINTK("Not Found fd:%d, event_id:%llu\n", *fd_ptr, key.event_id);
  }
  DATACRUMBS_EVENT_SUBMIT(event);
  return 0;
}

static inline __attribute__((always_inline)) int sysio_metadata_exit(struct pt_regs* ctx,
                                                                     u64 event_id) {
  u64 te = bpf_ktime_get_ns();
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    return 0;  // not tracing this pid
  }
  struct fn_value_t* fn = bpf_map_lookup_elem(&fn_pid_map, &key);
  if (fn == 0) return 0;  // missed entry
  DATACRUMBS_SKIP_SMALL_EVENTS(fn, te);
  struct sysio_event_t* event;
  DATACRUMBS_RB_RESERVE(output, struct sysio_event_t, event);
  event->type = 2;
  event->id = key.id;
  event->event_id = key.event_id;
  DATACRUMBS_COLLECT_TIME(event);
  event->size = 0;
  event->fhash = 0;  // Initialize fhash to empty
  int* fd_ptr = bpf_map_lookup_elem(&latest_fd, &key);
  if (fd_ptr != 0) {
    DBG_PRINTK("Found fd:%d, event_id:%llu\n", *fd_ptr, key.event_id);
    struct file_t file_key = {};
    file_key.id = key.id;
    file_key.fd = *fd_ptr;
    u32* fhash = bpf_map_lookup_elem(&fd_fname, &file_key);
    if (fhash != 0) {
      event->fhash = *fhash;
      DBG_PRINTK("Found fd:%d, file:%u, event_id:%llu\n", *fd_ptr, *fhash, key.event_id);
    } else {
      DBG_PRINTK("Not Found fd:%d, file:%u, event_id:%llu\n", *fd_ptr, *fhash, key.event_id);
    }
  } else {
    DBG_PRINTK("Not Found fd:%d, event_id:%llu\n", *fd_ptr, key.event_id);
  }
  DATACRUMBS_EVENT_SUBMIT(event);
  return 0;
}

static inline __attribute__((always_inline)) int sysio_open_entry(struct pt_regs* ctx, u64 event_id,
                                                                  const char* filename) {
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    return 0;  // not tracing this pid
  }
  DBG_PRINTK("Pushed pid:%d, event_id:%llu to map\n", (u32)key.id, key.event_id);
  struct string_t fname_i;
  int len = bpf_probe_read_user_str(&fname_i.str, MAX_STR_READ_LEN, filename);
  u32 fhash = hash_and_store(&fname_i, len);
  bpf_map_update_elem(&latest_fname, &key, &fhash, BPF_ANY);
  struct fn_value_t fn = {};
  fn.ts = bpf_ktime_get_ns();
  bpf_map_update_elem(&fn_pid_map, &key, &fn, BPF_ANY);
  DBG_PRINTK("Pushed filename:%s with len: %dfor pid:%d, event_id:%llu to map\n", fname_i.str, len,
             (u32)key.id, key.event_id);
  return 0;
}

static inline __attribute__((always_inline)) int sysio_open_exit(struct pt_regs* ctx,
                                                                 u64 event_id) {
  u64 te = bpf_ktime_get_ns();
  struct fn_key_t key = {};
  key.event_id = event_id;
  u64 start_ts;
  if (!need_tracing(&key, &start_ts)) {
    return 0;  // not tracing this pid
  }
  struct fn_value_t* fn = bpf_map_lookup_elem(&fn_pid_map, &key);
  if (fn == 0) return 0;  // missed entry
  DATACRUMBS_SKIP_SMALL_EVENTS(fn, te);
  struct sysio_event_t* event;
  DATACRUMBS_RB_RESERVE(output, struct sysio_event_t, event);

  event->type = 2;
  event->id = key.id;
  event->event_id = key.event_id;
  DATACRUMBS_COLLECT_TIME(event);
  event->size = 0;
  event->fhash = 0;  // Initialize fhash to empty
  u32* fhash = bpf_map_lookup_elem(&latest_fname, &key);
  if (fhash != 0) {
    event->fhash = *fhash;
    struct file_t file_key = {};
    int fd = PT_REGS_RC(ctx);
    file_key.id = key.id;
    file_key.fd = fd;
    DBG_PRINTK("Adding Found fd:%d, file:%u event_id:%llu\n", fd, *fhash, key.event_id);
    bpf_map_update_elem(&fd_fname, &file_key, fhash, BPF_ANY);
  }
  DATACRUMBS_EVENT_SUBMIT(event);
  return 0;
}

SEC("ksyscall/openat")
int BPF_KSYSCALL(openat_entry, int dfd, const char* filename, int flags) {
  return sysio_open_entry(ctx, USER_EVENT_ID_START + 0, filename);
}

SEC("kretsyscall/openat")
int BPF_KRETPROBE(openat_exit) {
  return sysio_open_exit(ctx, USER_EVENT_ID_START + 0);
}

SEC("ksyscall/read")
int BPF_KSYSCALL(read_entry, int fd, void* data, u64 count) {
  return sysio_fd_init(USER_EVENT_ID_START + 1, fd);
}
SEC("kretsyscall/read")
int BPF_KRETPROBE(read_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 1);
}
SEC("ksyscall/write")
int BPF_KSYSCALL(write_entry, int fd, const void* data, u64 count) {
  return sysio_fd_init(USER_EVENT_ID_START + 2, fd);
}
SEC("kretsyscall/write")
int BPF_KRETPROBE(write_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 2);
}
SEC("ksyscall/close")
int BPF_KSYSCALL(close_entry, int fd) {
  return sysio_fd_init(USER_EVENT_ID_START + 3, fd);
}
SEC("kretsyscall/close")
int BPF_KRETPROBE(close_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 3);
}
SEC("ksyscall/fallocate")
int BPF_KSYSCALL(fallocate_entry, int fd, int mode, int offset, int len) {
  return sysio_fd_init(USER_EVENT_ID_START + 4, fd);
}
SEC("kretsyscall/fallocate")
int BPF_KRETPROBE(fallocate_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 4);
}
SEC("ksyscall/fdatasync")
int BPF_KSYSCALL(fdatasync_entry, int fd) {
  return sysio_fd_init(USER_EVENT_ID_START + 5, fd);
}
SEC("kretsyscall/fdatasync")
int BPF_KRETPROBE(fdatasync_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 5);
}
SEC("ksyscall/flock")
int BPF_KSYSCALL(flock_entry, int fd, int cmd) {
  return sysio_fd_init(USER_EVENT_ID_START + 6, fd);
}
SEC("kretsyscall/flock")
int BPF_KRETPROBE(flock_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 6);
}
SEC("ksyscall/fsync")
int BPF_KSYSCALL(fsync_entry, int fd) {
  return sysio_fd_init(USER_EVENT_ID_START + 7, fd);
}
SEC("kretsyscall/fsync")
int BPF_KRETPROBE(fsync_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 7);
}
SEC("ksyscall/ftruncate")
int BPF_KSYSCALL(ftruncate_entry, int fd, int length) {
  return sysio_fd_init(USER_EVENT_ID_START + 8, fd);
}
SEC("kretsyscall/ftruncate")
int BPF_KRETPROBE(ftruncate_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 8);
}
SEC("ksyscall/lseek")
int BPF_KSYSCALL(lseek_entry, int fd, int offset, int whence) {
  return sysio_fd_init(USER_EVENT_ID_START + 9, fd);
}
SEC("kretsyscall/lseek")
int BPF_KRETPROBE(lseek_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 9);
}
SEC("ksyscall/pread64")
int BPF_KSYSCALL(pread64_entry, int fd, void* buf, u64 count, u64 pos) {
  return sysio_fd_init(USER_EVENT_ID_START + 10, fd);
}
SEC("kretsyscall/pread64")
int BPF_KRETPROBE(pread64_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 10);
}

SEC("ksyscall/preadv")
int BPF_KSYSCALL(preadv_entry, int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h) {
  return sysio_fd_init(USER_EVENT_ID_START + 11, fd);
}
SEC("kretsyscall/preadv")
int BPF_KRETPROBE(preadv_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 11);
}
SEC("ksyscall/preadv2")
int BPF_KSYSCALL(preadv2_entry, int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h, u64 flags) {
  return sysio_fd_init(USER_EVENT_ID_START + 12, fd);
}
SEC("kretsyscall/preadv2")
int BPF_KRETPROBE(preadv2_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 12);
}
SEC("ksyscall/pwrite64")
int BPF_KSYSCALL(pwrite64_entry, int fd, const void* data, u64 count, u64 pos) {
  return sysio_fd_init(USER_EVENT_ID_START + 13, fd);
}
SEC("kretsyscall/pwrite64")
int BPF_KRETPROBE(pwrite64_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 13);
}
SEC("ksyscall/pwritev")
int BPF_KSYSCALL(pwritev_entry, int fd, u64 vec, u64 vlen) {
  return sysio_fd_init(USER_EVENT_ID_START + 14, fd);
}
SEC("kretsyscall/pwritev")
int BPF_KRETPROBE(pwritev_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 14);
}
SEC("ksyscall/pwritev2")
int BPF_KSYSCALL(pwritev2_entry, int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h, u64 flags) {
  return sysio_fd_init(USER_EVENT_ID_START + 15, fd);
}
SEC("kretsyscall/pwritev2")
int BPF_KRETPROBE(pwritev2_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 15);
}
SEC("ksyscall/readahead")
int BPF_KSYSCALL(readahead_entry, int fd, u64 offset, u64 count) {
  return sysio_fd_init(USER_EVENT_ID_START + 16, fd);
}
SEC("kretsyscall/readahead")
int BPF_KRETPROBE(readahead_exit) {
  return sysio_metadata_exit(ctx, USER_EVENT_ID_START + 16);
}
SEC("ksyscall/readv")
int BPF_KSYSCALL(readv_entry, int fd, u64 vec, u64 vlen) {
  return sysio_fd_init(USER_EVENT_ID_START + 17, fd);
}
SEC("kretsyscall/readv")
int BPF_KRETPROBE(readv_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 17);
}
SEC("ksyscall/writev")
int BPF_KSYSCALL(writev_entry, int fd, u64 vec, u64 vlen) {
  return sysio_fd_init(USER_EVENT_ID_START + 18, fd);
}
SEC("kretsyscall/writev")
int BPF_KRETPROBE(writev_exit) {
  return sysio_data_exit(ctx, USER_EVENT_ID_START + 18);
}

SEC("ksyscall/open")
int BPF_KSYSCALL(open_entry, const char* filename, int flags, int mode) {
  sysio_open_entry(ctx, USER_EVENT_ID_START + 19, filename);
  return 0;
}
SEC("kretsyscall/open")
int BPF_KRETPROBE(open_exit, struct pt_regs* regs) {
  sysio_open_exit(ctx, USER_EVENT_ID_START + 19);
  return 0;
}

SEC("ksyscall/openat2")
int BPF_KSYSCALL(openat2_entry, int dfd, const char* filename, int flags) {
  sysio_open_entry(ctx, USER_EVENT_ID_START + 20, filename);
  return 0;
}
SEC("kretsyscall/openat2")
int BPF_KRETPROBE(openat2_exit) {
  sysio_open_exit(ctx, USER_EVENT_ID_START + 20);
  return 0;
}