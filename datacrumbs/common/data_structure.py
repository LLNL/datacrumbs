import ctypes
from typing import *
from datacrumbs.common.constants import *


class DFEvent:
    id: int
    pid: int
    tid: int
    name: str
    cat: str
    ts: int
    dur: int
    ph: str
    args: Dict


class Filename(ctypes.Structure):
    _fields_ = [
        ("fname", ctypes.c_char * 256),
    ]

class DFTraceEvent(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint64),
        ("event_id", ctypes.c_uint64),
        ("ip", ctypes.c_uint64),
        ("ts", ctypes.c_uint64),
        ("dur", ctypes.c_uint64),
    ]
