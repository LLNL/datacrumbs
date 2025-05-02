from abc import ABC, abstractmethod
from datacrumbs.configs.configuration_manager import ConfigurationManager
class BCCCollector(ABC):
    entry_fn: str
    exit_fn: str

    def __init__(self) -> None:
        self.config = ConfigurationManager.get_instance()
        self.filter_pid = """
            u64 id = bpf_get_current_pid_tgid();
            u32 pid = id;
            u64* start_ts = pid_map.lookup(&pid);
            if (start_ts == 0 || pid == 0)                                      
                return 0;
        """
        self.capture_entry_fn = """
            struct fn_key_t key = {};
            key.pid = pid;
            key.ip = DFEVENTID;
            struct fn_t fn = {};
            fn.ts = bpf_ktime_get_ns();
            fn_pid_map.update(&key, &fn);
        """
        self.lookup_fn = """
            struct fn_key_t key = {};
            key.pid = pid;
            key.ip = DFEVENTID;
            struct fn_t *fn = fn_pid_map.lookup(&key);
            if (fn == 0) return 0; // missed entry
        """
        
        self.common_generic_functions = """
            static int generic_entry(struct pt_regs *ctx, u64 event_id) {
                DFFILTERPID
                DFFNENTRY
                return 0;
            }
            static int generic_exit(struct pt_regs *ctx, u64 event_id) {
                DFFILTERPID
                DFFNLOOKUP            
                DFCAPTUREEVENTKEY
                DFCAPTUREEVENTVALUE
                DFSUBMITEVENT
                DFEXITSTATSCLEAN
                return 0;
            }
        """.replace(
            "DFFILTERPID", self.filter_pid
        ).replace(
            "DFFNENTRY", self.capture_entry_fn
        ).replace(
            "DFFNLOOKUP", self.lookup_fn
        ).replace(
            "DFEVENTID", "event_id"
        )
        
        self.generic_sys_functions = """
            int syscall__trace_entry_DFFUNCTION(struct pt_regs *ctx) {
                return generic_entry(ctx, DFEVENTID);
            }
            int sys__trace_exit_DFFUNCTION(struct pt_regs *ctx) {
                return generic_exit(ctx, DFEVENTID);
            }
        """.replace(
            "DFFILTERPID", self.filter_pid
        ).replace(
            "DFFNENTRY", self.capture_entry_fn
        ).replace(
            "DFFNLOOKUP", self.lookup_fn
        )
        
        
        self.custom_sys_functions = """
        DFEVENTSTRUCT
        int syscall__trace_entry_DFFUNCTION(struct pt_regs *ctx DFENTRYARGS) {
            DFFILTERPID
            DFFNENTRY
            DFENTRYCMD   
            return 0;
        }

        int sys__trace_exit_DFFUNCTION(struct pt_regs *ctx) {
            DFFILTERPID
            DFFNLOOKUP            
            DFCAPTUREEVENTKEY
            DFEXITCMDKEY
            DFCAPTUREEVENTVALUE
            DFEXITCMDSTATS
            DFSUBMITEVENT
            DFEXITSTATSCLEAN
            return 0;
        }
        """.replace(
            "DFFILTERPID", self.filter_pid
        ).replace(
            "DFFNENTRY", self.capture_entry_fn
        ).replace(
            "DFFNLOOKUP", self.lookup_fn
        )
        
        self.generic_functions = """
            int trace_DFCAT_DFFUNCTION_entry(struct pt_regs *ctx) {
                return generic_entry(ctx, DFEVENTID);
            }
            int trace_DFCAT_DFFUNCTION_exit(struct pt_regs *ctx) {
                return generic_exit(ctx, DFEVENTID);
            }
        """.replace(
            "DFFILTERPID", self.filter_pid
        ).replace(
            "DFFNENTRY", self.capture_entry_fn
        ).replace(
            "DFFNLOOKUP", self.lookup_fn
        )
        
        self.custom_functions = """
        
        DFEVENTSTRUCT
        
        int trace_DFCAT_DFFUNCTION_entry(struct pt_regs *ctx DFENTRYARGS) {
            DFFILTERPID
            
            DFFNENTRY
            
            DFENTRYCMD
            
            return 0;
        }

        int trace_DFCAT_DFFUNCTION_exit(struct pt_regs *ctx) {
            DFFILTERPID
            
            DFFNLOOKUP
            
            DFCAPTUREEVENTKEY
            
            DFEXITCMDKEY
            
            DFCAPTUREEVENTVALUE
            
            DFEXITCMDSTATS
            
            DFSUBMITEVENT
            return 0;
        }
        """.replace(
            "DFFILTERPID", self.filter_pid
        ).replace(
            "DFFNENTRY", self.capture_entry_fn
        ).replace(
            "DFFNLOOKUP", self.lookup_fn
        )
