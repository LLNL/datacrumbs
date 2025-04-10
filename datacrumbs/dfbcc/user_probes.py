from typing import *
import os
import re
import json
from bcc import BPF
from tqdm import tqdm
from datacrumbs.dfbcc.collector import BCCCollector
from datacrumbs.dfbcc.probes import BCCFunctions, BCCProbes
from datacrumbs.common.enumerations import ProbeType
from datacrumbs.configs.configuration_manager import ConfigurationManager
from datacrumbs.dfbcc.elf import CorpusReader

class UserProbes:
    config: ConfigurationManager
    probes: List[BCCProbes]

    def __init__(self, generate_probes=False) -> None:
        self.config = ConfigurationManager.get_instance()
        if generate_probes:
            self.probes = []
            num_symbols = 0
            for key, obj in self.config.user_libraries.items():
                probe = BCCProbes(ProbeType.USER, key, [])
                if "regex" not in obj:
                    pattern = re.compile(".*")
                else:
                    pattern = re.compile(obj["regex"])
                link = obj["link"]
                reader = CorpusReader(link)
                symbols_dict = reader.get_symbols()
                symbols_dict = {k: v for k, v in symbols_dict.items() if v.get("type") == "FUNC"}
                symbols = reader.get_symbols().keys()
                for symbol in tqdm(symbols, desc=f"User symbols for {key}"):
                    if (symbol or symbol != "") and pattern.match(symbol):
                        probe.functions.append(BCCFunctions(symbol))
                        num_symbols += 1
                        self.config.tool_logger.debug(f"Adding Probe function {symbol} from {key}")
                self.probes.append(probe)
            probes_file = self.config.user_probes_file
            with open(probes_file, "w") as f:
                json.dump([probe.to_dict() for probe in self.probes], f)
            self.config.tool_logger.info(f"Probes generated and saved to {probes_file}")
        else:
            try:
                probes_file = self.config.user_probes_file
                with open(probes_file, "r") as f:
                    loaded_probes = json.load(f)
                    self.probes = [BCCProbes.from_dict(probe) for probe in loaded_probes]
                self.config.tool_logger.info(f"Probes loaded from {probes_file}")
            except FileNotFoundError:
                self.config.tool_logger.error(f"Probes file {probes_file} not found")

    def collector_fn(self, collector: BCCCollector, category_fn_map, count: int):
        bpf_text = ""
        for probe in self.probes:
            for fn in probe.functions:
                count = count + 1
                if ProbeType.SYSTEM == probe.type:
                    text = collector.sys_functions
                else:
                    text = collector.functions
                text = text.replace("DFCAT", probe.category)
                text = text.replace("DFFUNCTION", fn.name)
                text = text.replace("DFEVENTID", str(count))
                text = text.replace("DFENTRYCMD", fn.entry_cmd)
                text = text.replace("DFEXITCMDSTATS", fn.exit_cmd_stats)
                text = text.replace("DFEXITCMDKEY", fn.exit_cmd_key)
                text = text.replace("DFENTRYARGS", fn.entry_args)
                text = text.replace("DFENTRY_STRUCT", fn.entry_struct_str)
                text = text.replace("DFEXIT_STRUCT", fn.exit_struct_str)
                category_fn_map[count] = (probe.category, fn)
                bpf_text += text

        return (bpf_text, category_fn_map, count)

    def attach_probes(self, bpf: BPF) -> None:
        self.config.tool_logger.info("Attaching probe for User Probes")
        for probe in tqdm(self.probes, "attach User probes"):
            for fn in tqdm(probe.functions, "attach User functions"):
                try:
                    self.config.tool_logger.debug(
                        f"Adding Probe function {fn.name} from {probe.category}"
                    )
                    if ProbeType.USER == probe.type:
                        library = probe.category
                        fname = fn.name
                        if probe.category in self.config.user_libraries:
                            library = self.config.user_libraries[probe.category]["link"]
                            bpf.add_module(library)
                        bpf.attach_uprobe(
                            name=library,
                            sym=fname,
                            fn_name=f"trace_{probe.category}_{fn.name}_entry",
                        )
                        bpf.attach_uretprobe(
                            name=library,
                            sym=fname,
                            fn_name=f"trace_{probe.category}_{fn.name}_exit",
                        )
                except Exception as e:
                    self.config.tool_logger.warn(
                        f"Unable attach probe {probe.category} to user function {fn.name} due to {e}"
                    )
