# External Imports
import hydra
from omegaconf import DictConfig

# Internal Imports
from datacrumbs.dfbcc.dfbcc import BCCMain
from datacrumbs.common.status import ProfilerStatus
from datacrumbs.configs.configuration_manager import ConfigurationManager


class Datacrumbs:
    """
    Datacrumbs Class
    """

    def __init__(self, cfg: DictConfig) -> None:
        self.config = ConfigurationManager.get_instance().load(cfg)

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


@hydra.main(version_base=None, config_path="configs", config_name="config")
def build(cfg: DictConfig) -> int:
    """
    The main method to start the profiler runtime.
    """
    profiler = Datacrumbs(cfg["module"])
    profiler.initialize()
    profiler.build()
    return ProfilerStatus.SUCCESS

@hydra.main(version_base=None, config_path="configs", config_name="config")
def run(cfg: DictConfig) -> int:
    """
    The main method to start the profiler runtime.
    """
    profiler = Datacrumbs(cfg["module"])
    profiler.initialize()
    profiler.load()
    profiler.run()
    profiler.finalize()
    return ProfilerStatus.SUCCESS
