===================
eBPF Background
===================

What is eBPF?
=============

eBPF (extended Berkeley Packet Filter) is a revolutionary technology that allows running sandboxed programs in the Linux kernel without changing kernel source code or loading kernel modules. Originally designed for network packet filtering, eBPF has evolved into a general-purpose execution engine that can be used for a wide variety of use cases.

How eBPF Works
==============

eBPF programs are written in a restricted C subset, compiled to eBPF bytecode, and loaded into the kernel. The kernel verifies the program for safety before executing it, ensuring:

* **Memory Safety**: Programs cannot access arbitrary memory locations
* **Termination**: Programs must terminate (no infinite loops)
* **Bounded Execution**: Programs have limited instruction count and stack size

Key Components
--------------

1. **eBPF Programs**: Small programs that execute in kernel space
2. **eBPF Maps**: Key-value data structures for sharing data between kernel and user space
3. **eBPF Verifier**: Ensures program safety before execution
4. **BPF Type Format (BTF)**: Provides type information for eBPF programs
5. **Helper Functions**: Kernel-provided functions that eBPF programs can call

eBPF in DataCrumbs
==================

DataCrumbs leverages eBPF technology to provide low-overhead I/O tracing capabilities. The tool uses several eBPF features:

Kprobes and Uprobes
-------------------

* **Kprobes**: Attach eBPF programs to kernel functions for tracing system calls and kernel-level I/O operations
* **Uprobes**: Attach eBPF programs to user-space functions in libraries (libc, libhdf5, etc.)

These probes allow DataCrumbs to intercept function calls without modifying the application or kernel code.

Ring Buffers
------------

DataCrumbs uses eBPF ring buffers (introduced in Linux 5.8) for efficient data transfer from kernel to user space. Ring buffers provide:

* High-throughput event delivery
* Low latency
* Memory efficiency
* Multi-producer, single-consumer semantics

Maps for State Management
--------------------------

eBPF maps store:

* Process tracking information
* File descriptor mappings
* Thread-local data
* Aggregated statistics

Advantages of eBPF for I/O Tracing
===================================

Minimal Overhead
----------------

eBPF programs execute directly in the kernel with JIT compilation, resulting in near-native performance. DataCrumbs typically adds less than 5% overhead to application execution.

Safety and Stability
--------------------

The eBPF verifier ensures that programs cannot crash the kernel or compromise system security. This makes DataCrumbs safe to use in production environments.

Dynamic Instrumentation
------------------------

eBPF programs can be loaded and unloaded dynamically without rebooting the system or restarting applications. This allows DataCrumbs to:

* Start and stop tracing on demand
* Update probe configurations at runtime
* Trace running applications without interruption

No Code Modification
--------------------

Applications do not need to be recompiled or modified to be traced by DataCrumbs. The tool can trace:

* Binary-only applications
* Third-party libraries
* System calls
* Custom functions

eBPF Limitations
================

Kernel Version Requirements
---------------------------

eBPF features have evolved over time, with different capabilities available in different kernel versions:

* **Linux 4.18**: Basic eBPF support with compatibility layers
* **Linux 5.1+**: Modern eBPF features
* **Linux 5.8+**: Full modern eBPF features with BPF ring buffers (recommended)

Stack Size Limits
-----------------

eBPF programs have a limited stack size (512 bytes). DataCrumbs works around this by:

* Using per-CPU maps for temporary storage
* Minimizing stack variable usage
* Splitting complex operations across multiple helper functions

Verifier Restrictions
---------------------

The eBPF verifier imposes restrictions on:

* Loop complexity (bounded loops only in newer kernels)
* Function calls (limited call depth)
* Memory access patterns (must be verified safe)

DataCrumbs handles these restrictions through careful program design and code generation.

eBPF Tools and Ecosystem
=========================

libbpf
------

DataCrumbs uses **libbpf** (version 1.5.0+) as the primary library for:

* Loading eBPF programs into the kernel
* Managing eBPF maps
* Attaching probes to functions
* Handling BTF information

bpftool
-------

**bpftool** (version 7.5.0+) is used during the build process for:

* Generating vmlinux.h (kernel type definitions)
* Creating BPF object files
* Generating skeleton headers for C programs
* Inspecting loaded eBPF programs (debugging)

BCC vs libbpf
-------------

DataCrumbs uses the **libbpf** approach rather than BCC (BPF Compiler Collection) because:

* **Portability**: libbpf-based programs are compiled once and run anywhere
* **Performance**: No runtime compilation overhead
* **Dependencies**: Smaller dependency footprint
* **Distribution**: Easier to package and deploy

Further Reading
===============

For more information about eBPF:

- `eBPF.io <https://ebpf.io/>`_ - Official eBPF documentation
- `libbpf Documentation <https://libbpf.readthedocs.io/>`_ - libbpf API reference
- `Kernel Documentation <https://www.kernel.org/doc/html/latest/bpf/>`_ - Linux kernel eBPF docs
- `BPF Performance Tools <http://www.brendangregg.com/bpf-performance-tools-book.html>`_ - Book by Brendan Gregg
