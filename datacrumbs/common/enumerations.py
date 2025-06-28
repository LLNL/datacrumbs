from enum import Enum


class ProbeType(Enum):
    SYSTEM = "SYSTEM"
    KERNEL = "KERNEL"
    USER = "USER"
    USDT = "USDT"

    def __str__(self):
        return self.value
    
    @staticmethod
    def get_enum(value):
        if ProbeType.SYSTEM.value == value:
            return ProbeType.SYSTEM
        elif ProbeType.KERNEL.value == value:
            return ProbeType.KERNEL
        elif ProbeType.USER.value == value:
            return ProbeType.USER
        elif ProbeType.USDT.value == value:
            return ProbeType.USDT
        return None

class Mode(Enum):
    PROFILE = 'profile'
    TRACE = 'trace'

    def __str__(self):
        return self.value

    @staticmethod
    def get_enum(value):
        if Mode.PROFILE.value == value:
            return Mode.PROFILE
        elif Mode.TRACE.value == value:
            return Mode.TRACE
        return None
    
class TraceType(Enum):
    PERF = 'perf'
    RING_BUFFER = 'ring_buffer'

    def __str__(self):
        return self.value

    @staticmethod
    def get_enum(value):
        if TraceType.PERF.value == value:
            return TraceType.PERF
        elif TraceType.RING_BUFFER.value == value:
            return TraceType.RING_BUFFER
        return None