from typing import *
import os
import re
import json
from tqdm import tqdm
from datacrumbs.dfbcc.collector import BCCCollector
from datacrumbs.dfbcc.probes import BCCFunctions, BCCProbes
from datacrumbs.common.enumerations import ProbeType
from datacrumbs.configs.configuration_manager import ConfigurationManager
from datacrumbs.elf.elf import CorpusReader
from datacrumbs.common.constants import *

class UserProbes:
    config: ConfigurationManager
    probes: List[BCCProbes]

    def __init__(self, generate_probes=False) -> None:
        self.config = ConfigurationManager.get_instance()
        if generate_probes:
            self.probes = []
            num_symbols = 0
            for key, obj in self.config.user_libraries.items():
                if "link" in obj:
                    probe = BCCProbes(ProbeType.USER, key, [])
                    if "regex" not in obj:
                        pattern = re.compile(".*")
                    else:
                        pattern = re.compile(obj["regex"])
                    link = obj["link"]
                    reader = CorpusReader(link)
                    symbols_dict = reader.get_symbols()
                    symbols_dict = {k: v for k, v in symbols_dict.items() if v.get("type") == "FUNC" and v.get("defined") != "UND" and v.get("binding") != "WEAK"}
                    symbols = symbols_dict.keys()
                    for symbol in tqdm(symbols, desc=f"User symbols for {key}"):
                        if (symbol or symbol != "") and pattern.match(symbol):
                            probe.functions.append(BCCFunctions(symbol, address=symbols_dict[symbol]["address"]))
                            num_symbols += 1
                            self.config.tool_logger.debug(f"Adding Probe function {symbol} from {key}")
                    self.probes.append(probe)
            with open(self.config.user_probes_file, "w") as f:
                json.dump([probe.to_dict() for probe in self.probes], f, separators=(",", ":"))
            os.chmod(self.config.user_probes_file, 0o777)
            self.config.tool_logger.info(f"Probes generated and saved to {self.config.user_probes_file}")
        else:
            try:
                with open(self.config.user_probes_file, "r") as f:
                    loaded_probes = json.load(f)
                    self.probes = [BCCProbes.from_dict(probe) for probe in loaded_probes]
                self.config.tool_logger.info(f"Probes loaded from {self.config.user_probes_file}")
            except FileNotFoundError:
                self.config.tool_logger.error(f"Probes file {self.config.user_probes_file} not found")

    def collector_fn(self, collector: BCCCollector, category_fn_map, count: int):
        bpf_text = ""
        for probe in self.probes:
            for fn in probe.functions:
                count = count + 1
                if ProbeType.SYSTEM == probe.type:
                    if fn.custom:
                        text = collector.custom_sys_functions
                    else:
                        text = collector.generic_sys_functions
                else:
                    if fn.custom:
                        text = collector.custom_functions
                    else:
                        text = collector.generic_functions
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

    def attach_probes(self, bpf) -> None:
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
                        is_regex = False
                        if fn.regex:
                            is_regex = True
                            fname = fn.regex
                        if fn.address and fn.address != 0 and not is_regex:
                            bpf.attach_uprobe(
                                name=library,
                                addr=fn.address,
                                fn_name=f"trace_{probe.category}_{fn.name}_entry".replace(".", "_"),
                            )
                            bpf.attach_uretprobe(
                                name=library,
                                addr=fn.address,
                                fn_name=f"trace_{probe.category}_{fn.name}_exit".replace(".", "_"),
                            )
                        else:                        
                            if is_regex:
                                bpf.attach_uprobe(
                                    name=library,
                                    sym_re=fname,
                                    fn_name=f"trace_{probe.category}_{fn.name}_entry".replace(".", "_"),
                                )
                                bpf.attach_uretprobe(
                                    name=library,
                                    sym_re=fname,
                                    fn_name=f"trace_{probe.category}_{fn.name}_exit".replace(".", "_"),
                                )
                            else:
                                bpf.attach_uprobe(
                                    name=library,
                                    sym=fname,
                                    fn_name=f"trace_{probe.category}_{fn.name}_entry".replace(".", "_"),
                                )
                                bpf.attach_uretprobe(
                                    name=library,
                                    sym=fname,
                                    fn_name=f"trace_{probe.category}_{fn.name}_exit".replace(".", "_"),
                                )
                except Exception as e:
                    self.config.tool_logger.warn(
                        f"Unable attach probe {probe.category} to user function {fn.name} due to {e}"
                    )


