import json
import os
import gzip
import shutil
import socket
import logging
from datacrumbs.common.data_structure import DFEvent
from datacrumbs.common.utils import *
from datacrumbs.configs.configuration_manager import ConfigurationManager
import hashlib

class PerfettoWriter:
    config: ConfigurationManager
    host_hash: int
    def __init__(self) -> None:
        self.config = ConfigurationManager.get_instance()
        try:
            os.remove(self.config.profile_file)
        except OSError:
            pass
        try:
            os.remove(f"{self.config.profile_file}.gz")
        except OSError:
            pass
        host = socket.gethostname()
        self.host_hash = get_hash(host)
        self.trace_log = self.config.setup_logger("datacrumbs.trace", self.config.profile_file, "%(message)s", level=logging.INFO)
        self.trace_log.info("[")
        self.write_process_independent_metadata("HH", host, self.host_hash)

    def finalize(self):
        self.config.tool_logger.info(f"Finalizing Writer")
        self.trace_log.info("]")
        with open(self.config.profile_file, "rb") as f_in:
            with gzip.open(f"{self.config.profile_file}.gz", "wb") as f_out:
                shutil.copyfileobj(f_in, f_out)
            os.chmod(f"{self.config.profile_file}.gz", 0o777)
        try:
            os.remove(self.config.profile_file)
        except OSError:
            pass

    def write(self, event: DFEvent):
        obj = {
            "id": event.id,
            "pid": event.pid,
            "tid": event.tid,
            "name": event.name,
            "cat": event.cat,
            "ph": event.ph,
            "ts": event.ts,  # Convert to us
        }
        if event.dur and event.dur >= 0:
            obj["dur"] = event.dur
        obj["args"] = {}
        obj["args"]["hhash"] = self.host_hash
        for key, value in event.args.items():
            obj["args"][key] = value
        self.trace_log.info(json.dumps(obj))
    
    def write_process_independent_metadata(self, metadata_name, name, value):
        obj = {
            "name": metadata_name,
            "cat": "dftracer",
            "ph": "M",
            "args": {
                "name": name,
                "value": value
            },            
        }
        self.trace_log.info(json.dumps(obj))
    
    def write_metadata_event(self, pid, tid, metadata_name, name, value):
        obj = {
            "pid": pid,
            "tid": tid,
            "name": metadata_name,
            "cat": "dftracer",
            "ph": "M",
            "args": {
                "name": name,
                "value": value
            },            
        }
        self.trace_log.info(json.dumps(obj))
