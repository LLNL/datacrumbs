============
Introduction
============

Overview
========

DataCrumbs is a comprehensive I/O profiling and tracing tool designed for high-performance computing (HPC) environments. It leverages eBPF (extended Berkeley Packet Filter) technology to provide low-overhead, real-time monitoring and analysis of application I/O behavior at scale.

Key Features
============

Low-Overhead Profiling
----------------------

DataCrumbs uses eBPF to achieve minimal performance overhead while capturing detailed I/O traces. Unlike traditional profiling tools that may introduce significant overhead, DataCrumbs operates in kernel space, minimizing interference with application execution.

Scalable Architecture
---------------------

Designed for large-scale HPC systems, DataCrumbs can efficiently trace I/O operations across distributed applications running on thousands of nodes. The tool supports multiple job schedulers including:

* FLUX
* SLURM
* OpenMPI
* Standalone execution

Multi-Layer Tracing
-------------------

DataCrumbs captures I/O operations at multiple layers:

* **System Calls**: Low-level I/O operations (open, read, write, close, etc.)
* **POSIX I/O**: Standard C library I/O functions
* **MPI-IO**: Parallel I/O operations in MPI applications
* **HDF5**: High-level I/O library operations
* **Custom Functions**: User-defined function tracing through configuration

Flexible Deployment
-------------------

DataCrumbs supports multiple deployment modes:

1. **Server Mode**: Long-running server process for continuous monitoring
2. **Service Mode**: Systemd service integration for managed execution
3. **Wrapper Mode**: Direct application wrapping for single executions
4. **Composable Mode**: Custom probe configurations for specific use cases

Real-Time Analysis
------------------

The tool provides:

* DFTracer format output (``.pfw.gz``) for efficient trace storage and analysis
* Event aggregation and correlation across processes
* Trace compression for efficient storage
* Real-time event processing with ring buffers

Use Cases
=========

DataCrumbs is particularly useful for:

* **Performance Analysis**: Understanding I/O bottlenecks in HPC applications
* **I/O Pattern Discovery**: Identifying access patterns and optimization opportunities
* **Debugging**: Tracing file access patterns to diagnose issues
* **System Monitoring**: Continuous monitoring of I/O behavior in production environments
* **Research**: Studying I/O characteristics of scientific applications

Architecture
============

DataCrumbs consists of several key components:

1. **eBPF Probes**: Kernel-space programs that capture events with minimal overhead
2. **Explorer**: Discovers available probes and functions in libraries
3. **Generator**: Creates custom eBPF programs based on configuration
4. **Server**: User-space daemon that collects and processes events
5. **Client Library**: Injected into applications for tracking
6. **Writer**: Formats and outputs trace data in DFTracer format (``.pfw.gz``)

Target Applications
===================

DataCrumbs is designed for profiling:

* MPI applications
* Parallel I/O workloads
* Scientific computing applications
* Data-intensive applications
* Any application using POSIX, HDF5, MPI-IO, or custom I/O libraries

License
=======

DataCrumbs is developed at Lawrence Livermore National Laboratory and released under an open-source license. See the LICENSE file in the repository for details.

Citation
========

If you use DataCrumbs in your research, please cite the appropriate publication. See CITATION.cff in the repository for citation information.
