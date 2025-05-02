# Internal Imports
from datacrumbs.dfbcc.dfbcc import BCCMain
from datacrumbs.common.status import ProfilerStatus
from datacrumbs.configs.configuration_manager import ConfigurationManager


class Datacrumbs:
    """
    Datacrumbs Class
    """

    def __init__(self) -> None:
        self.config = ConfigurationManager.get_instance().load_and_override()

    def initialize(self) -> None:
        self.bcc = BCCMain()
    
    def build(self) -> None:
        self.bcc.build()
    
    
    def load(self) -> None:
        self.bcc.load()

    def run(self) -> None:
        self.bcc.run()

    def finalize(self) -> None:
        self.config.tool_logger.info("Detaching...")



def build() -> int:
    """
    The main method to start the profiler runtime.
    """    
    profiler = Datacrumbs()
    profiler.initialize()
    profiler.build()
    return ProfilerStatus.SUCCESS

def run() -> int:
    """
    The main method to start the profiler runtime.
    """
    profiler = Datacrumbs()
    profiler.initialize()
    profiler.load()
    profiler.run()
    profiler.finalize()
    return ProfilerStatus.SUCCESS
