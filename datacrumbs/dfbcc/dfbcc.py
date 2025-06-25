from time import sleep
import os
import ctypes
from ctypes import *
from typing import *
import threading
import psutil
import math
import concurrent.futures
from tqdm import tqdm

# Internal Imports
from datacrumbs.dfbcc.app_connector import BCCApplicationConnector
from datacrumbs.dfbcc.profile_collector import BCCProfileCollector
from datacrumbs.dfbcc.trace_collector import BCCTraceCollector
from datacrumbs.dfbcc.profile_header import BCCProfileHeader
from datacrumbs.dfbcc.trace_header import BCCTraceHeader
from datacrumbs.dfbcc.io_probes import IOProbes
from datacrumbs.dfbcc.user_probes import UserProbes
from datacrumbs.dfbcc.probes import BCCFunctions, BCCProbes
from datacrumbs.configs.configuration_manager import ConfigurationManager
from datacrumbs.common.data_structure import DFEvent, Filename, DFTraceEvent
from datacrumbs.common.enumerations import Mode, TraceType
from datacrumbs.common.utils import *
from datacrumbs.common.constants import *
from datacrumbs.writer.perfetto import PerfettoWriter
import json

def copy(dst, src):
    """Copies the contents of src to dst"""
    pointer(dst)[0] = src

class BCCMain:
    config: ConfigurationManager

    def __init__(self) -> None:
        self.config = ConfigurationManager.get_instance()
        self.category_fn_map = {}
        self.writer = PerfettoWriter()
        self.run_thread_counter = True
        self.has_events = False
        self.filename_map = {0: None}
        self.filehash_map = {0: None}
        self.executor = concurrent.futures.ThreadPoolExecutor(max_workers=16)
        self.futures = []
        self.index = 0
        self.filename = f"{self.config.mode.value}.c"
        self.category_fn_map_file = self.config.category_fn_map
        pass

    def build(self) -> any:
        app_connector = BCCApplicationConnector()
        bpf_text = ""
        if self.config.mode == Mode.PROFILE:
            collector = BCCProfileCollector()
            bpf_text += str(BCCProfileHeader())
        elif self.config.mode == Mode.TRACE:
            collector = BCCTraceCollector()
            bpf_text += str(BCCTraceHeader())
        bpf_text += str(app_connector)
        bpf_text += str(collector)
        io_probes = IOProbes(generate_probes=self.config.generate_probes)
        count = 0
        probe_text, self.category_fn_map, count = io_probes.collector_fn(
            collector, self.category_fn_map, count
        )
        bpf_text += probe_text
        user_probes = UserProbes(generate_probes=self.config.generate_probes)
        probe_text, self.category_fn_map, count = user_probes.collector_fn(
            collector, self.category_fn_map, count
        )
        
        # Store self.category_fn_map into a JSON file
        with open(self.category_fn_map_file, "w") as json_file:
            json.dump({key: (value[0], value[1].to_dict()) for key, value in self.category_fn_map.items()}, json_file, separators=(",", ":"))
        os.chmod(self.category_fn_map_file, 0o777)
        self.config.tool_logger.info(f"Total functions probed: {len(self.category_fn_map)}")
        bpf_text += probe_text
        # bpf_text += str(collector)
        bpf_text = bpf_text.replace(
            "INTERVAL_RANGE", str(int(self.config.interval_sec * 1e9))
        )
        self.config.tool_logger.debug(f"Compiled Program is \n{bpf_text}")
        
        f = open(f"{self.filename}", "w")
        f.write(bpf_text)
        f.close()
        # Format the C code using clang-format
        formatted_filename = f"{self.filename}.formatted"
        os.system(f"clang-format -i {self.filename}")
        os.rename(self.filename, formatted_filename)
        os.rename(formatted_filename, self.filename)
        self.config.tool_logger.info(f"Wrote program into {self.filename}")
        os.chmod(self.filename, 0o777)
        return self
    
    def load(self):
        # External Imports
        from bcc import BPF
        from bcc.utils import printb
        if os.path.exists(self.category_fn_map_file):
            with open(self.category_fn_map_file, "r") as json_file:
                self.category_fn_map = {key: (value[0], BCCFunctions.from_dict(value[1])) for key, value in json.load(json_file).items()}
            self.config.tool_logger.info(f"Loaded category function map from {self.category_fn_map_file}")
        else:
            raise FileNotFoundError(f"The file {self.category_fn_map_file} does not exist.")
        if not os.path.exists(self.filename):
            raise FileNotFoundError(f"The file {self.filename} does not exist.")
        with open(self.filename, "r") as file:
            bpf_text = file.read()
        app_connector = BCCApplicationConnector()
        io_probes = IOProbes()
        user_probes = UserProbes()
        self.config.tool_logger.info(f"Read program from {self.filename}")
        self.bpf = BPF(text=bpf_text, debug=0)
        self.config.tool_logger.info(f"Loaded program into BCC")
        app_connector.attach_probe(self.bpf)
        io_probes.attach_probes(self.bpf)
        user_probes.attach_probes(self.bpf)
        matched = self.bpf.num_open_kprobes()
        self.config.tool_logger.info(f"{matched} functions matched")
        return self

    def run_network_usage(self):
        start_counter = 0
        self.config.tool_logger.info("Running Network thread")
        while self.run_thread_counter:
            network_counts = psutil.net_io_counters(pernic=True)
            # self.config.tool_logger.debug(f"network_counts {network_counts}")
            event = DFEvent()
            event.pid = 0
            event.tid = 0
            event.cat = "Network"
            event.name = "snetio"
            event.ts = start_counter * self.config.interval_sec
            # self.config.tool_logger.debug(f"Logging event {event}")
            for interface, counter in network_counts.items():
                event.args = {
                    "nic": interface,
                    "bytes_sent": counter.bytes_sent,
                    "bytes_recv": counter.bytes_recv,
                    "packets_sent": counter.packets_sent,
                    "packets_recv": counter.packets_recv,
                    "errin": counter.errin,
                    "errout": counter.errout,
                    "dropin": counter.dropin,
                    "dropout": counter.dropout,
                }
                self.writer.write(event)
            sleep(self.config.interval_sec)
            start_counter += 1
        self.config.tool_logger.debug("Exiting Network thread")

    def run_disk_usage(self):
        self.config.tool_logger.info("Running Disk thread")
        start_counter = 0
        while self.run_thread_counter:
            disk_counters = psutil.disk_io_counters(perdisk=True)
            event = DFEvent()
            event.pid = 0
            event.tid = 0
            event.cat = "Disk"
            event.name = "io_stat"
            event.ts = start_counter * self.config.interval_sec

            for mount, counter in disk_counters.items():
                event.args = {
                    "mount": mount,
                    "read_count": counter.read_count,
                    "write_count": counter.write_count,
                    "read_bytes": counter.read_bytes,
                    "write_bytes": counter.write_bytes,
                    "read_time": counter.read_time,
                    "write_time": counter.write_time,
                }
                self.writer.write(event)
            sleep(self.config.interval_sec)
            start_counter += 1
        self.config.tool_logger.debug("Exiting Disk thread")

    def run_cpu_loop(self):
        self.config.tool_logger.info("Running CPU thread")
        start_counter = 0
        while self.run_thread_counter:
            cpu_utilization = psutil.cpu_percent(interval=1, percpu=True)
            event = DFEvent()
            event.pid = 0
            event.tid = 0
            event.cat = "CPU"
            event.name = "utilization"
            event.ts = start_counter * self.config.interval_sec
            event.args = {}
            for i, util in enumerate(cpu_utilization):
                event.args[f"CPU_{i}"] = util
            self.writer.write(event)
            sleep(self.config.interval_sec)
            start_counter += 1
        self.config.tool_logger.debug("Exiting CPU thread")

    def run_memory_loop(self):
        self.config.tool_logger.info("Running Memory thread")
        start_counter = 0
        while self.run_thread_counter:
            memory = psutil.virtual_memory()
            event = DFEvent()
            event.pid = 0
            event.tid = 0
            event.cat = "Memory"
            event.name = "virtual_memory"
            event.ts = start_counter * self.config.interval_sec
            event.args = {
                "total": memory.total,
                "available": memory.available,
                "percent": memory.percent,
                "used": memory.used,
                "free": memory.free,
                "active": memory.active,
                "inactive": memory.inactive,
                "buffers": memory.buffers,
                "cached": memory.cached,
                "shared": memory.shared,
                "slab": memory.slab,
            }
            self.writer.write(event)
            sleep(self.config.interval_sec)
            start_counter += 1
        self.config.tool_logger.debug("Exiting memory thread")

    def stop(self):
        self.run_thread_counter = False
        self.config.tool_logger.info("Stopping all threads")
        '''self.memory_loop.join()
        self.cpu_loop.join()
        self.disk_loop.join()
        self.network_loop.join()
        '''
        self.writer.finalize()
    def run(self) -> None:
        '''self.memory_loop = threading.Thread(target=self.run_memory_loop)
        self.memory_loop.start()
        self.cpu_loop = threading.Thread(target=self.run_cpu_loop)
        self.cpu_loop.start()
        self.disk_loop = threading.Thread(target=self.run_disk_usage)
        self.disk_loop.start()
        self.network_loop = threading.Thread(target=self.run_network_usage)
        self.network_loop.start()
        '''
        if self.config.mode == Mode.PROFILE:
            self.profile_run()
        elif self.config.mode == Mode.TRACE:
            self.trace_run()
            
            
    def profile_run(self) -> None:
        self.no_event_count = 0
        self.has_events = False
        self.last_processed_ts = -1
        sleep_sec = self.config.interval_sec * 5
        wait_for = 30 / (sleep_sec)
        self.config.tool_logger.info("Ready to run code")
        try:
            while True:
                counts = self.bpf.get_table("fn_map")
                filenames = self.bpf.get_table("file_hash")
                try:
                    self.config.tool_logger.debug(
                        f"sleeping for {sleep_sec} secs with last ts {self.last_processed_ts}"
                    )
                    sleep(sleep_sec)
                    if self.has_events and self.no_event_count > wait_for:
                        self.config.tool_logger.info(
                            f"No events for {self.no_event_count * sleep_sec} seconds. Quiting Profiler Now."
                        )
                        filenames.clear()
                        self.stop()
                        break
                except KeyboardInterrupt:
                    break
                for k, v in filenames.items():
                    if k.value not in self.filename_map:
                        self.filename_map[k.value] = v.fname.decode()
                map_values = sorted(
                    counts.items(),
                    key=lambda counts: counts[0].trange,
                )
                num_entries = len(map_values)
                big_ts = -1
                if num_entries > 0:
                    big_ts = map_values[num_entries - 1][0].trange
                processed = 0
                for k, v in map_values:
                    event = DFEvent()
                    event.pid = ctypes.c_uint32(k.id).value
                    self.has_events = True
                    processed += 1
                    if big_ts == k.trange and big_ts > self.last_processed_ts + 1:
                        self.config.tool_logger.debug(
                            f"Previous loop had {self.last_processed_ts} ts and now is {big_ts} ts"
                        )
                        continue
                    event.tid = ctypes.c_uint32(k.id >> 32).value
                    event_tuple = self.category_fn_map[k.event_id]
                    event.cat = event_tuple[0]
                    function_probe = event_tuple[1]
                    if function_probe.regex:
                        event.name = self.bpf.sym(k.ip, event.pid, show_module=True).decode()
                        if "unknown" in event.name:
                            event.name = self.bpf.ksym(k.ip, show_module=True).decode()
                    else:
                        event.name = function_probe.name
                    event.ts = int(k.trange * self.config.interval_sec * 1e6)
                    event.ph = 'C'
                    event.dur = -1
                    event.args = {}
                    event.args["fname"] = (
                        self.filename_map[k.file_hash]
                        if k.file_hash in self.filename_map
                        else None
                    )
                    event.args["freq"] = v.freq
                    event.args["time"] = v.time / 1e9
                    event.args["size_sum"] = v.size_sum if v.size_sum > 0 else None
                    self.last_processed_ts = k.trange
                    self.config.tool_logger.info(f"{self.last_processed_ts} timestamp processed")
                    self.writer.write(event)
                    keys = (counts.Key * 1)()
                    keys[0] = k
                    counts.items_delete_batch(keys)
                    self.no_event_count = 0
                if self.has_events:
                    self.no_event_count += 1
        except KeyboardInterrupt:
            pass
    
    def async_handle_trace(self, data):
        event = DFEvent()
        c_event = ctypes.cast(data, ctypes.POINTER(DFTraceEvent)).contents
        event_tuple = self.category_fn_map[c_event.event_id]
        event.cat = event_tuple[0]
        function_probe = event_tuple[1]
        # self.config.tool_logger.info(f"{c_event.ip} IP processed for {function_probe.name}")
        event.args = {}
        event.pid = ctypes.c_uint32(c_event.id).value
        event.tid = ctypes.c_uint32(c_event.id >> 32).value
        if not function_probe.regex:
            class_type = function_probe.get_class()
            if class_type:
                c_event = ctypes.cast(data, ctypes.POINTER(class_type)).contents
                event.args = function_probe.get_args(c_event)
                if "file_hash" in event.args:
                    fname = self.filename_map[event.args["file_hash"]]
                    file_hash = get_hash(fname)
                    if file_hash not in self.filehash_map:
                        self.filehash_map[file_hash] = fname
                        self.writer.write_metadata_event(event.pid, event.tid, "FH", fname, file_hash)
                    event.args['fhash'] = file_hash
                    del event.args['file_hash']
        event.ts = int(c_event.ts)
        event.ph = 'X'
        event.dur = c_event.dur
        if function_probe.regex:
            event.name = self.bpf.sym(c_event.ip, event.pid, show_module=True).decode()
            if "unknown" in event.name:
                event.name = self.bpf.ksym(c_event.ip, show_module=True).decode()
        else:
            event.name = function_probe.name
        self.last_processed_ts = c_event.ts
        self.config.tool_logger.debug(f"{self.last_processed_ts} timestamp processed")
        self.writer.write(event)
        return 0
    
    def get_hash_value(self, key):
        filenames = self.bpf.get_table("file_hash")
        for k, v in filenames.items():
            if k.value not in self.filename_map:
                self.filename_map[k.value] = v.fname.decode()
        if key in self.filename_map:
            return self.filename_map[key]
        else:
            return None
    
    def async_handle_event(self, index, data):
        event = DFEvent()
        event.id = index
        c_event = ctypes.cast(data, ctypes.POINTER(DFTraceEvent)).contents
        event_tuple = self.category_fn_map[str(c_event.event_id)]
        event.cat = event_tuple[0]
        function_probe = event_tuple[1]
        # self.config.tool_logger.info(f"{c_event.ip} IP processed for {function_probe.name}")
        event.args = {}
        event.pid = ctypes.c_uint32(c_event.id).value
        event.tid = ctypes.c_uint32(c_event.id >> 32).value
        event.ts = int(c_event.ts)
        event.ph = 'X'
        event.dur = c_event.dur
        if function_probe.regex:
            event.name = self.bpf.sym(c_event.ip, event.pid, show_module=True).decode()
            if "unknown" in event.name:
                event.name = self.bpf.ksym(c_event.ip, show_module=True).decode()
        else:
            event.name = function_probe.name
        if not function_probe.regex:
            class_type = function_probe.get_class()
            if class_type:
                c_event = ctypes.cast(data, ctypes.POINTER(class_type)).contents
                event.args = function_probe.get_args(c_event)
                if "file_hash" in event.args: # and event.args["file_hash"] in self.filename_map and self.filename_map[event.args["file_hash"]] is not None:
                    event.args["fhash"] = event.args.pop("file_hash")
        # self.last_processed_ts = c_event.ts
        # self.config.tool_logger.debug(f"{self.last_processed_ts} timestamp processed")
        self.writer.write(event)
        self.no_event_count = 0
        return
    
    def handle_trace_event(self, ctx, data, size):
        self.has_events = True
        self.index += 1
        self.async_handle_event(self.index, data)
        # future = self.executor.submit(self.async_handle_trace, self.index, data)
        # self.futures.append(future)
        self.no_event_count = 0
        self.pbar.update(1)
        return 
    
    def open_buffer(self, callback):
        if self.config.trace_type == TraceType.PERF:
            self.bpf["events"].open_perf_buffer(self.handle_trace_event, page_cnt=DEFAULT_PERF_BUFFER_PAGES)
        elif self.config.trace_type == TraceType.RING_BUFFER:
            self.bpf["events"].open_ring_buffer(self.handle_trace_event)
            
    def poll_buffer(self):
        if self.config.trace_type == TraceType.PERF:
            self.bpf.perf_buffer_poll()
        elif self.config.trace_type == TraceType.RING_BUFFER:
            self.bpf.ring_buffer_consume()
    
    def trace_run(self) -> None:
        self.open_buffer(self.handle_trace_event)
        sleep_sec = self.config.interval_sec * 5
        self.last_processed_ts = -1
        wait_for = (30.0 / (sleep_sec) - 1)
        self.no_event_count = 0
        self.config.tool_logger.info("Ready to run code")
        self.pbar = tqdm()
        try:
            while True:                
                try:
                    self.config.tool_logger.debug(
                        f"sleeping for {sleep_sec} secs with last ts {self.last_processed_ts}"
                    )
                    sleep(sleep_sec)                    
                    if self.has_events:
                        self.no_event_count += 1
                    if self.has_events and self.no_event_count > wait_for:
                        # for future in concurrent.futures.as_completed(self.futures):
                        #     try:
                        #         future.result()
                        #     except Exception as exc:
                        #         print('generated an exception: %s' % (exc))
                        self.config.tool_logger.info(
                            f"No events for {self.no_event_count * sleep_sec} seconds. Quiting Profiler Now."
                        )
                        for k, v in self.bpf["file_hash"].items_lookup_and_delete_batch():
                            self.writer.write_process_independent_metadata("FH", v.fname.decode(), k.value)
                        self.stop()
                        break
                except KeyboardInterrupt:
                    break
                for k, v in self.bpf["file_hash"].items_lookup_and_delete_batch():
                    self.writer.write_process_independent_metadata("FH", v.fname.decode(), k.value)
                #filenames.items_delete_batch(keys)
                self.poll_buffer()
        except KeyboardInterrupt:
            pass
