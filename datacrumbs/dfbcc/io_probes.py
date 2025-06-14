from typing import *
import re
import json
from tqdm import tqdm
from datacrumbs.dfbcc.collector import BCCCollector
from datacrumbs.dfbcc.probes import BCCFunctions, BCCProbes
from datacrumbs.common.enumerations import ProbeType
from datacrumbs.configs.configuration_manager import ConfigurationManager
from datacrumbs.elf.elf import CorpusReader
from datacrumbs.llvm.functions import Functions

class IOProbes:
    config: ConfigurationManager
    probes: List[BCCProbes]
    regex_functions: Set[str]

    def __init__(self, generate_probes=False) -> None:
        self.config = ConfigurationManager.get_instance()

        if generate_probes:
            self.regex_functions = set()
            self.probes = []
            exit_cmd_key_file_check = """
                            int* fd_ptr = latest_fd.lookup(&id);
                            if (fd_ptr != 0 ) {
                                struct file_t file_key = {};
                                file_key.id = id;
                                file_key.fd = *fd_ptr;
                                u64* hash_ptr = fd_hash.lookup(&file_key);
                                if (hash_ptr != 0) {
                                    stats_key->file_hash = *hash_ptr; 
                                }
                            }
                            """
            functions_added = set()
            
            self.probes.append(
                BCCProbes(
                    ProbeType.SYSTEM,
                    "sys",
                    list(filter(None, [
                        self.get_bcc_function(
                            "openat",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args=", int dfd, const char *filename, int flags",
                            entry_cmd=r"""
                            struct filename_t fname_i;
                            u64 filename_len = sizeof(fname_i.fname);
                            int len = bpf_probe_read_user_str(&fname_i.fname, filename_len, filename);
                            //fname_i.fname[len-1] = '\0';
                            u64 filehash = get_hash(fname_i.fname, filename_len);
                            bpf_trace_printk("Hash value is %d for filename %s",filehash,filename);
                            file_hash.update(&filehash, &fname_i);
                            latest_hash.update(&key, &filehash);
                            """,
                            exit_cmd_key="""
                            u64* hash_ptr = latest_hash.lookup(&key);
                            if (hash_ptr != 0) {
                                stats_key->file_hash = *hash_ptr; 
                            }
                            """,
                            exit_cmd_stats="""
                            if (hash_ptr != 0) {
                                int fd = PT_REGS_RC(ctx);
                                struct file_t file_key = {};
                                file_key.id = id;
                                file_key.fd = fd;
                                fd_hash.update(&file_key, hash_ptr);
                            }
                            """,
                        ),
                        self.get_bcc_function(
                            "read",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, void *data, u64 count
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "write",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, const void *data, u64 count
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "close",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args="""
                            , int fd
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "fallocate",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args="""
                            , int fd, int mode, int offset, int len
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "fdatasync",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args="""
                            , int fd
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "flock",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args="""
                            , int fd, int cmd
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "fsync",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args="""
                            , int fd
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "ftruncate",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args="""
                            , int fd, int length
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "lseek",
                            entry_struct=[("uint64", "file_hash")],
                            entry_args="""
                            , int fd, int offset, int whence
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "pread64",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, void *buf, u64 count, u64 pos
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "preadv",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "preadv2",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h, u64 flags
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "pwrite64",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, const void *data, u64 count, u64 pos
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "pwritev",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "pwritev2",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, u64 buf, u64 vlen, u64 pos_l, u64 pos_h, u64 flags
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "readahead",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, u64 offset, u64 count
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "readlinkat"
                        ),
                        self.get_bcc_function(
                            "readv",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, u64 vec, u64 vlen
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                        self.get_bcc_function(
                            "writev",                        
                            entry_struct=[("uint64", "file_hash")],                        
                            exit_struct=[("uint64", "size_sum")],
                            entry_args="""
                            , int fd, u64 vec, u64 vlen
                            """,
                            entry_cmd="""
                            latest_fd.update(&id,&fd);
                            """,
                            exit_cmd_stats="""
                                    stats->size_sum += PT_REGS_RC(ctx);
                                    """,
                            exit_cmd_key=exit_cmd_key_file_check,
                        ),
                    ])),
                )
            )
            for fn in self.probes[-1].functions:
                if fn.name not in functions_added:
                    functions_added.add(fn.name)
            for name, value in self.config.system_io_headers.items():
                probe = BCCProbes(ProbeType.KERNEL, name, [])
                functions = Functions(value["header"], value["regex"])
                function_names = functions.get_function_names()
                for fname in tqdm(function_names, desc=f"System I/O headers for {name}"):
                    if fname not in functions_added:
                        probe.functions.append(BCCFunctions(fname))
                        functions_added.add(fname)
                self.probes.append(probe)
                self.config.tool_logger.info(f"Added {len(probe.functions)} for system I/O probes: {name}")
                
            for name, value in self.config.io_libraries.items():
                probe = BCCProbes(ProbeType.USER, name, [])
                if "regex" not in value:
                    pattern = re.compile(".*")
                else:
                    pattern = re.compile(value["regex"])
                link = value["link"]
                reader = CorpusReader(link)
                symbols_dict = reader.get_symbols()
                symbols_dict = {k: v for k, v in symbols_dict.items() if v.get("type") == "FUNC" and v.get("defined") != "UND" and v.get("binding") != "WEAK"}
                symbols = symbols_dict.keys()
                for symbol in tqdm(symbols, desc=f"User symbols for {name}"):
                    if (symbol or symbol != "") and pattern.match(symbol):
                        if fname not in functions_added:
                            probe.functions.append(BCCFunctions(symbol))
                            functions_added.add(fname)
                            self.config.tool_logger.debug(f"Adding Probe function {symbol} from {name}")
                self.probes.append(probe)
                self.config.tool_logger.info(f"Added {len(probe.functions)} for I/O probes: {name}")
                    
            self.config.tool_logger.info(f"Added {len(functions_added)} I/O probes")
            io_probes_file = self.config.io_probes_file
            with open(io_probes_file, "w") as f:
                json.dump([probe.to_dict() for probe in self.probes], f, separators=(",", ":"))
            self.config.tool_logger.info(f"Probes generated and saved to {io_probes_file}")
        else:
            try:
                io_probes_file = self.config.io_probes_file
                with open(io_probes_file, "r") as f:
                    loaded_probes = json.load(f)
                    self.probes = [BCCProbes.from_dict(probe) for probe in loaded_probes]
                self.config.tool_logger.info(f"Probes loaded from {io_probes_file}")
            except FileNotFoundError:
                self.config.tool_logger.error(f"Probes file {io_probes_file} not found")
        

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
    def is_function_valid(self, function_name):
        return  "." not in function_name and "$" not in function_name

    def get_bcc_functions(self, regex):
        
        from bcc import BPF
        matches = BPF.get_kprobe_functions(regex)
        probes = []
        bcc_list = {}
        for line in tqdm(matches, desc=f"Matching for {regex}"):
            if line.decode() not in self.regex_functions and self.is_function_valid(line.decode()):
                self.config.tool_logger.debug(f"Adding {line.decode()} to probe")
                self.regex_functions.add(line.decode())
                value = BPF.ksym(BPF.ksymname(line), show_module=True).decode()
                value = list(filter(None, re.split(r'\]|\[| ', value)))
                function_name = value[0]
                module = value[1]
                if self.is_function_valid(function_name):
                    if module not in bcc_list:
                        bcc_list[module] = []
                    bcc_list[module].append(BCCFunctions(function_name))
            else:
                self.config.tool_logger.debug(f"Skipping {line.decode()} to probe")
        for key, value in bcc_list.items():
            probes.append(BCCProbes(ProbeType.KERNEL, key, value))
        return probes

    def get_bcc_function(self, function_name,
        entry_struct: List[Tuple] = [],
        exit_struct: List[Tuple] = [],
        entry_args: str = "",
        entry_cmd: str = "",
        exit_cmd_stats: str = "",
        exit_cmd_key: str = "",):
        if function_name not in self.regex_functions:
            self.regex_functions.add(function_name)            
            return BCCFunctions(function_name, 
                                entry_struct=entry_struct, 
                                exit_struct=exit_struct,
                                entry_args=entry_args,
                                entry_cmd=entry_cmd,
                                exit_cmd_stats=exit_cmd_stats,
                                exit_cmd_key=exit_cmd_key)
        else:
            return None
            

    def attach_probes(self, bpf) -> None:
        self.config.tool_logger.info("Attaching I/O Probes")
        for probe in tqdm(self.probes, "attach I/O probes"):
            for fn in tqdm(probe.functions, "attach I/O functions"):
                try:
                    if ProbeType.SYSTEM == probe.type:
                        fnname = bpf.get_syscall_prefix().decode() + fn.name
                        # self.config.tool_logger.debug(
                        #     f"attaching name {fnname} with {fn.name} for cat {probe.category}"
                        # )
                        bpf.attach_kprobe(
                            event=fnname,
                            fn_name=f"syscall__trace_entry_{fn.name}",
                        )
                        bpf.attach_kretprobe(
                            event=fnname,
                            fn_name=f"sys__trace_exit_{fn.name}",
                        )
                    elif ProbeType.KERNEL == probe.type:
                        fname = fn.name
                        if fn.regex:
                            fname = fn.regex
                            bpf.attach_kprobe(
                                event_re=fname,
                                fn_name=f"trace_{probe.category}_{fn.name}_entry",
                            )
                            bpf.attach_kretprobe(
                                event_re=fname,
                                fn_name=f"trace_{probe.category}_{fn.name}_exit",
                            )
                        else:
                            bpf.attach_kprobe(
                                event=fname,
                                fn_name=f"trace_{probe.category}_{fn.name}_entry",
                            )
                            bpf.attach_kretprobe(
                                event=fname,
                                fn_name=f"trace_{probe.category}_{fn.name}_exit",
                            )
                    elif ProbeType.USER == probe.type:
                        library = probe.category
                        fname = fn.name
                        is_regex = False
                        if fn.regex:
                            is_regex = True
                            fname = fn.regex
                        if probe.category in self.config.user_libraries:
                            library = self.config.user_libraries[probe.category]["link"]
                            bpf.add_module(library)

                        if is_regex:
                            bpf.attach_uprobe(
                                name=library,
                                sym_re=fname,
                                fn_name=f"trace_{probe.category}_{fn.name}_entry",
                            )
                            bpf.attach_uretprobe(
                                name=library,
                                sym_re=fname,
                                fn_name=f"trace_{probe.category}_{fn.name}_exit",
                            )
                        else:
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
                        f"Unable attach probe  {probe.category} to io function {fn.name} due to {e}"
                    )
