# Python Native Imports
from typing import *
import os
import pathlib
import logging

# External Imports
from omegaconf import DictConfig

# Internal Imports
from datacrumbs.common.utils import convert_or_fail
from datacrumbs.common.enumerations import Mode, TraceType


class ConfigurationManager:
    # singleton instance
    __instance = None
    project_root: str
    # Configuration variables
    user_libraries: Dict[str, str] = {}
    interval_sec: float
    module: str
    install_dir: str
    profile_file: str
    mode: Mode = Mode.PROFILE
    trace_type: TraceType = TraceType.PERF
    io_probes_file: str
    generate_probes: bool = False

    @staticmethod
    def get_instance():
        """Static access method."""
        if ConfigurationManager.__instance is None:
            ConfigurationManager.__instance = ConfigurationManager()
        return ConfigurationManager.__instance

    def setup_logger(self, name, log_file, formatter, level=logging.INFO):
        """To setup as many loggers as you want"""
        handler = logging.FileHandler(log_file)        
        handler.setFormatter(logging.Formatter(formatter))
        logger = logging.getLogger(name)
        logger.setLevel(level)
        logger.addHandler(handler)
        return logger

    def __init__(self):
        self.project_root = pathlib.Path(__file__).parent.parent.parent.resolve()
        log_file = "datacrumbs.log"
        try:
            os.remove(log_file)
        except OSError:
            pass
        self.tool_logger = self.setup_logger("tool", log_file, "%(asctime)s [%(levelname)s]: %(message)s in %(pathname)s:%(lineno)d", level=logging.INFO)

    def derive(self):
        self.function_file = f"{self.project_root}/datacrumbs/configs/function.json"
        self.io_probes_file = f"{self.project_root}/datacrumbs/data/io_probes_file.json"
        self.user_probes_file = f"{self.project_root}/datacrumbs/data/user_probes_file.json"
        self.category_fn_map = f"{self.project_root}/datacrumbs/data/category_fn_map.json"
        
    def load(self, config: DictConfig):
        if "name" in config:
            self.module = config["name"]
        if "install_dir" in config:
            self.install_dir = config["install_dir"]
            if not os.path.isabs(self.install_dir):
                self.install_dir = os.path.join(self.project_root, self.install_dir)
        if "file" in config:
            self.profile_file = config["file"]
        if "generate_probes" in config:
            self.generate_probes = config["generate_probes"]
        if "mode" in config:
            self.mode = Mode.get_enum(config["mode"])
            self.tool_logger.debug(f'yaml mode {config["mode"]} set conf value {self.mode}')
        if "user" in config:
            for obj in config["user"]:
                self.user_libraries[obj["name"]] = obj
        if "profile" in config:
            if "interval_sec" in config["profile"]:
                status, self.interval_sec = convert_or_fail(
                    float, config["profile"]["interval_sec"]
                )
                if status.failed():
                    exit(status)
        if "trace" in config:
            if "type" in config["trace"]:
                self.trace_type = TraceType.get_enum(config["trace"]["type"])
        self.derive()
        return self
