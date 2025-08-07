
from datacrumbs.dfbcc.header import BCCHeader
from datacrumbs.common.enumerations import TraceType

class BCCTraceHeader(BCCHeader):
    def __init__(self):
        super().__init__()
        self.include_general_struct = """
            struct general_event_t {                                                       
                u64 id;
                u64 event_id;
                u64 ip;
                u64 ts;                                                                   
                u64 dur;
            };
        """
        if self.config.trace_type == TraceType.PERF:
            self.events_ds += """
            BPF_PERF_OUTPUT(events); // emit events to python
            """
        elif self.config.trace_type == TraceType.RING_BUFFER:
            self.events_ds += """
            BPF_RINGBUF_OUTPUT(events, 1 << 16); // emit events to python
            """
            

    def __str__(self) -> str:
        return self.includes + self.data_structures  + self.include_general_struct + self.events_ds + self.util
