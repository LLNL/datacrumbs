from typing import *
import ctypes
import json
from datacrumbs.common.enumerations import ProbeType
from datacrumbs.common.data_structure import DFEvent, Filename, DFTraceEvent

class BCCFunctions:
    name: str
    regex: str
    entry_struct: List[Tuple]
    exit_struct: List[Tuple]
    entry_cmd: str
    exit_cmd_stats: str
    exit_cmd_key: str
    entry_args: str
    entry_struct_str: str
    exit_struct_str: str
    custom: bool

    def __init__(
        self,
        name: str,
        regex: str = None,
        entry_struct: List[Tuple] = [],
        exit_struct: List[Tuple] = [],
        entry_args: str = "",
        entry_cmd: str = "",
        exit_cmd_stats: str = "",
        exit_cmd_key: str = "",
    ) -> None:
        self.name = name
        self.regex = regex
        self.entry_struct = entry_struct
        self.exit_struct = exit_struct
        self.entry_cmd = entry_cmd
        self.exit_cmd_stats = exit_cmd_stats
        self.exit_cmd_key = exit_cmd_key
        self.entry_args = entry_args
        self.entry_struct_str = self.get_str_from_args(entry_struct)
        self.exit_struct_str = self.get_str_from_args(exit_struct)
        if self.entry_struct_str == "" and self.exit_struct_str == "":
            self.custom = False
        else:
            self.custom = True
        
    def get_str_from_args(self, args):
        args_str = ""
        for entry in args:
            var_type_str = entry[0]
            var_name_str = entry[1]
            var_c_type = ""
            if var_type_str == "uint64":
                var_c_type = "u64"
            elif var_type_str == "uint32":
                var_c_type = "u32"
            if var_c_type != "":
                args_str += f"{var_c_type} {var_name_str};"
        return args_str

    def get_args(self, obj):
        args = {}
        for entry in self.entry_struct:
            var_name = entry[1]
            args[var_name] = eval(f"obj.{var_name}")
        
        for entry in self.exit_struct:
            var_name = entry[1]
            args[var_name] = eval(f"obj.{var_name}")
        
        return args
    def __str__(self):
        return f"{self.name}"
    def __repr__(self):
        return f"{self.name}"

    def to_dict(self) -> Dict:
        return {
            "name": self.name,
            "regex": self.regex,
            "entry_struct": self.entry_struct,
            "exit_struct": self.exit_struct,
            "entry_cmd": self.entry_cmd,
            "exit_cmd_stats": self.exit_cmd_stats,
            "exit_cmd_key": self.exit_cmd_key,
            "entry_args": self.entry_args,
            "custom": self.custom,
        }

    @staticmethod
    def from_dict(data: Dict) -> "BCCFunctions":
        return BCCFunctions(
            name=data["name"],
            regex=data.get("regex"),
            entry_struct=data.get("entry_struct", []),
            exit_struct=data.get("exit_struct", []),
            entry_cmd=data.get("entry_cmd", ""),
            exit_cmd_stats=data.get("exit_cmd_stats", ""),
            exit_cmd_key=data.get("exit_cmd_key", ""),
            entry_args=data.get("entry_args", ""),
        )
    
    def get_class(self):
        array = []
        for entry in self.entry_struct:
            var_type_str = entry[0]
            var_name_str = entry[1]
            var_c_type = None
            if var_type_str == "uint64":
                var_c_type = ctypes.c_uint64
            elif var_type_str == "uint32":
                var_c_type = ctypes.c_uint32
            if var_c_type:
                array.append((var_name_str, var_c_type))
        
        
        for entry in self.exit_struct:
            var_type_str = entry[0]
            var_name_str = entry[1]
            var_c_type = None
            if var_type_str == "uint64":
                var_c_type = ctypes.c_uint64
            elif var_type_str == "uint32":
                var_c_type = ctypes.c_uint32
            if var_c_type:
                array.append((var_name_str, var_c_type))
        
        if len(array) == 0:
            return None
        
        class ProbeEventype(DFTraceEvent):
            _fields_ = array
        return ProbeEventype



class BCCProbes:
    type: ProbeType
    category: str
    functions: List[BCCFunctions]

    def __init__(
        self, type: ProbeType, category: str, functions: List[BCCFunctions]
    ) -> None:
        self.type = type
        self.category = category
        self.functions = functions
    
    def to_dict(self) -> Dict:
        return {
            "type": self.type.name,
            "category": self.category,
            "functions": [func.to_dict() for func in self.functions],
        }

    @staticmethod
    def from_dict(data: Dict) -> "BCCProbes":
        functions = [
            BCCFunctions.from_dict(func) for func in data.get("functions", [])
        ]
        return BCCProbes(
            type=ProbeType[data["type"]],
            category=data["category"],
            functions=functions,
        )