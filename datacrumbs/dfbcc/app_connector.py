from datacrumbs.configs.configuration_manager import ConfigurationManager


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
            return 0;
        }
        int trace_datacrumbs_stop(struct pt_regs *ctx) {
            u64 id = bpf_get_current_pid_tgid();
            u32 pid = id;
            bpf_trace_printk(\"Stop tracing PID \%d\",pid);
            pid_map.delete(&pid);
            return 0;
        }
        """

    def __str__(self) -> str:
        return self.functions

    def attach_probe(self, bpf) -> None:
        self.config.tool_logger.info("Attaching probe for App Connector")
        bpf.add_module(f"{self.config.install_dir}/libdatacrumbs.so")
        bpf.attach_uprobe(
            name=f"{self.config.install_dir}/libdatacrumbs.so",
            sym="datacrumbs_start",
            fn_name="trace_datacrumbs_start",
        )
        bpf.attach_uprobe(
            name=f"{self.config.install_dir}/libdatacrumbs.so",
            sym="datacrumbs_stop",
            fn_name="trace_datacrumbs_stop",
        )
