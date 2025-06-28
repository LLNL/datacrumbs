from datacrumbs.configs.configuration_manager import ConfigurationManager
from datacrumbs.common.constants import USDT_PROBE_EVENT_ID

class BCCApplicationConnector:
    config: ConfigurationManager

    def __init__(self) -> None:
        self.config = ConfigurationManager.get_instance()
        self.functions = """
        int trace_datacrumbs_start(struct pt_regs *ctx) {
            u64 id = bpf_get_current_pid_tgid();
            u32 pid = 0;
            u64* start_ts = pid_map.lookup(&pid);
            u64 tsp = bpf_ktime_get_ns();
            if (start_ts != 0)                                      
                tsp = *start_ts;
            else
                pid_map.update(&pid, &tsp);
            pid = id;
            bpf_trace_printk(\"Tracing PID \%d\",pid);
            pid_map.update(&pid, &tsp);
            struct general_event_t stats_key_v = {};
            struct general_event_t *stats_key = &stats_key_v;
            stats_key->id = id;
            stats_key->ts= tsp;
            stats_key->event_id = DFEVENTID;
            events.ringbuf_output(&stats_key_v, sizeof(struct general_event_t), 0);
            return 0;
        }
        int trace_datacrumbs_stop(struct pt_regs *ctx) {
            u64 id = bpf_get_current_pid_tgid();
            u32 pid = id;
            bpf_trace_printk(\"Stop tracing PID \%d\",pid);
            pid_map.delete(&pid);
            return 0;
        }
        int fork_datacrums_exit(struct pt_regs *ctx) {
            u64 id = bpf_get_current_pid_tgid();
            u32 pid = id;
            u64* start_ts = pid_map.lookup(&pid);
            if (start_ts == 0 || pid == 0)                                      
                return 0;
            pid = id;
            bpf_trace_printk(\"Tracing PID \%d\",pid);
            pid = PT_REGS_RC(ctx);
            u64 tsp = bpf_ktime_get_ns();
            pid_map.update(&pid, &tsp);
            struct general_event_t stats_key_v = {};
            struct general_event_t *stats_key = &stats_key_v;
            stats_key->id = pid;
            stats_key->ts= tsp;
            stats_key->event_id = DFEVENTID;
            events.ringbuf_output(&stats_key_v, sizeof(struct general_event_t), 0);
            return 0;
        }
        """.replace("DFEVENTID", str(USDT_PROBE_EVENT_ID))

    def __str__(self) -> str:
        return self.functions

    def attach_probe(self, bpf) -> None:
        self.config.tool_logger.info("Attaching probe for App Connector")
        bpf.add_module(f"{self.config.install_dir}/lib/libdatacrumbs.so")
        bpf.attach_uprobe(
            name=f"{self.config.install_dir}/lib/libdatacrumbs.so",
            sym="datacrumbs_start",
            fn_name="trace_datacrumbs_start",
        )
        bpf.attach_uprobe(
            name=f"{self.config.install_dir}/lib/libdatacrumbs.so",
            sym="datacrumbs_stop",
            fn_name="trace_datacrumbs_stop",
        )
        bpf.attach_uretprobe(
            name="c",
            sym="fork",
            fn_name=f"fork_datacrums_exit",
        )
