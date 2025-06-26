from abc import ABC, abstractmethod
from datacrumbs.configs.configuration_manager import ConfigurationManager
from datacrumbs.common.enumerations import TraceType

class BCCHeader(ABC):
    def __init__(self):
        self.config = ConfigurationManager.get_instance()
        self.includes = """
        #include <linux/sched.h>
        #include <uapi/linux/limits.h>
        #include <uapi/linux/ptrace.h>
        """

        self.data_structures = """
        struct fn_key_t {
            u64 ip;
            s64 pid;
        };
        struct fn_t {
            u64 ts;
        };
        struct file_t {
          u64 id;
          int fd;  
        };
        struct filename_t {
            char fname[256];
        };
        """
        
        self.events_ds = """
        BPF_HASH(pid_map, u32, u64); // map for apps to collect data
        BPF_HASH(fn_pid_map, struct fn_key_t, struct fn_t); // collect start time and ip for apps
        BPF_HASH(file_hash, u64, struct filename_t, 10240);
        BPF_HASH(latest_hash, struct fn_key_t, u64);
        BPF_HASH(latest_fd, u64, int);
        BPF_HASH(fd_hash, struct file_t, u64);
        BPF_HASH(pid_hash, u64, u64);
        """
        self.util = """
        static u64 get_hash(unsigned char *str, u64 len) {
            u64 hash = 5381;
            int c = *str;
            int count = 0;
            while (count < len && c) {
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
                c = *str++;
                count++;
            }
            return hash;
        }
        /*static u64 get_hash(u64 id) {
            u64 first_hash = 1;
            u64* hash_value = pid_hash.lookup_or_init(&id, &first_hash);
            (*hash_value)++;
            return *hash_value;
        }*/
        """
        
        self.usdt_data_structures = """        
        #define MAX_STRING_LENGTH 80
        """
        
        self.usdt_events_ds = """
        BPF_HASH(pid_map, u32, u64); // map for apps to collect data
        BPF_HASH(fn_pid_map, struct fn_key_t, struct fn_t); // collect start time and ip for apps
        """

    def __str__(self) -> str:
        return self.includes + self.data_structures + self.events_ds + self.util
