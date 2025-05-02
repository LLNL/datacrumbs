# Python Native Imports
from typing import *
import os
import pathlib
import logging

# Internal Imports
from datacrumbs.common.utils import convert_or_fail
from datacrumbs.common.enumerations import Mode, TraceType
import yaml
import argparse


class ConfigurationManager:
    # singleton instance
    __instance = None
    project_root: str
    # Configuration variables
    user_libraries: Dict[str, str] = {}
    interval_sec: float
    module: str = "default"
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
        

    def derive(self):
        self.function_file = f"{self.project_root}/datacrumbs/configs/function.json"
        self.io_probes_file = f"{self.project_root}/datacrumbs/data/io_probes_file.json"
        self.user_probes_file = f"{self.project_root}/datacrumbs/data/user_probes_file.json"
        self.category_fn_map = f"{self.project_root}/datacrumbs/data/category_fn_map.json"
    
    def load_from_yaml(self, yaml_file_name: str):
        """
        Load configuration values from a YAML file located in datacrumbs/configs/module/.

        Args:
            yaml_file_name (str): The name of the YAML file (without extension) to load.
        """

        yaml_file_path = os.path.join(self.project_root, "datacrumbs", "configs", "module", f"{yaml_file_name}.yaml")
        if not os.path.exists(yaml_file_path):
            raise FileNotFoundError(f"YAML file {yaml_file_path} not found.")

        with open(yaml_file_path, "r") as yaml_file:
            try:
                config_data = yaml.safe_load(yaml_file)
                if not isinstance(config_data, dict):
                    raise ValueError(f"Invalid YAML structure in {yaml_file_path}.")
                self.load(config_data)
            except yaml.YAMLError as e:
                raise RuntimeError(f"Error parsing YAML file {yaml_file_path}: {e}")
    
    def define_args(self):
        """
        Define command-line arguments for the script.
        """

        parser = argparse.ArgumentParser(description="Configuration Manager")
        parser.add_argument("--module", type=str, help="Module name.  That is picked from datacrumbs/configs/module")
        parser.add_argument("--install_dir", type=str, default=os.path.join(self.project_root, "build"), help="Installation directory (default: project_root/build)")
        parser.add_argument("--mode", type=str, choices=[e.value for e in Mode], default=Mode.TRACE.value, help="Mode of operation")
        parser.add_argument(
            "--file", 
            type=str, 
            default="datacrumbs.pfw", 
            help="Profile/Trace file"
        )
        parser.add_argument("--generate_probes", action="store_true", default=False, help="Generate probes (default: False)")
        parser.add_argument("--interval_sec", type=float, default=1.0, help="Interval in seconds for profiling")
        parser.add_argument("--trace_type", type=str, choices=[e.value for e in TraceType], default=TraceType.RING_BUFFER.value, help="Type of trace (default: ring_buffer)")
        parser.add_argument("--log_level", type=str, choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"], default="INFO", help="Logging level")
        parser.add_argument("--log_file", type=str, default="datacrumbs.log", help="Log file name (default: datacrumbs.log)")
        args = parser.parse_args()
        return args
    
    def override_with_args(self, args):
        """
        Override configuration values using command-line arguments.
        """
        if args.module:
            self.module = args.module
        if args.install_dir:
            self.install_dir = args.install_dir
            if not os.path.isabs(self.install_dir):
                self.install_dir = os.path.join(self.project_root, self.install_dir)
        if args.file:
            self.profile_file = args.file
        if args.generate_probes:
            self.generate_probes = True
        if args.mode:
            self.mode = Mode.get_enum(args.mode)
        if args.interval_sec:
            self.interval_sec = args.interval_sec
        if args.trace_type:
            self.trace_type = TraceType.get_enum(args.trace_type)
        
    
    def pretty_print_config(self):
        """
        Pretty print the final configuration values using info logging.
        """
        config_values = {
            "module": self.module,
            "install_dir": self.install_dir,
            "profile_file": self.profile_file,
            "generate_probes": self.generate_probes,
            "mode": self.mode.name if self.mode else None,
            "interval_sec": self.interval_sec,
            "trace_type": self.trace_type.name if self.trace_type else None,
            "user_libraries": self.user_libraries,
            "io_probes_file": self.io_probes_file,
            "function_file": getattr(self, "function_file", None),
            "user_probes_file": getattr(self, "user_probes_file", None),
            "category_fn_map": getattr(self, "category_fn_map", None),
        }
        self.tool_logger.info("Final Configuration Values:")
        for key, value in config_values.items():
            self.tool_logger.info(f"{key}: {value}")
    
    def load_and_override(self):
        """
        Load configuration from a YAML file and override it with command-line arguments.

        Args:
            yaml_file_name (str): The name of the YAML file (without extension) to load.
        """
        args = self.define_args()
        self.load_from_yaml(args.module)
        self.override_with_args(args)
        try:
            os.remove(args.log_file)
        except OSError:
            pass
        self.tool_logger = self.setup_logger("tool", args.log_file, "%(asctime)s [%(levelname)s]: %(message)s in %(pathname)s:%(lineno)d", level=args.log_level)
        self.tool_logger.info("Configuration loaded and overridden successfully.")
        self.pretty_print_config()
        def validate_config(self):
            """
            Validate that all required configuration variables are set and not None.
            """
            required_fields = [
                "module",
                "install_dir",
                "profile_file",
                "mode",
                "interval_sec",
                "trace_type",
                "io_probes_file",
                "function_file",
                "user_probes_file",
                "category_fn_map",
            ]
            for field in required_fields:
                if getattr(self, field, None) is None:
                    raise ValueError(f"Configuration validation failed: '{field}' is not set.")
            self.tool_logger.info("Configuration validation passed successfully.")
    
    def load(self, config):
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
