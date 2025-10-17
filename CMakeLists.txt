cmake_minimum_required(VERSION 3.4)
cmake_policy(SET CMP0075 NEW)
project(datacrumbs LANGUAGES C CXX)

# ------------------------------------------------------------------------------
# Version information
# ------------------------------------------------------------------------------
set(DATACRUMBS_VERSION_MAJOR "1")
set(DATACRUMBS_VERSION_MINOR "0")
set(DATACRUMBS_VERSION_PATCH "0")
set(DATACRUMBS_PACKAGE ${PROJECT_NAME})
set(DATACRUMBS_PACKAGE_NAME ${PROJECT_NAME})
set(DATACRUMBS_PACKAGE_VERSION "${DATACRUMBS_VERSION_MAJOR}.${DATACRUMBS_VERSION_MINOR}.${DATACRUMBS_VERSION_PATCH}")
set(DATACRUMBS_PACKAGE_VERSION_MAJOR "${DATACRUMBS_VERSION_MAJOR}.${DATACRUMBS_VERSION_MINOR}")
set(DATACRUMBS_PACKAGE_VERSION_MINOR "${DATACRUMBS_VERSION_PATCH}")
set(DATACRUMBS_PACKAGE_STRING "${DATACRUMBS_PACKAGE_NAME} ${DATACRUMBS_PACKAGE_VERSION}")
set(DATACRUMBS_PACKAGE_TARNAME "${DATACRUMBS_PACKAGE}")

set(DATACRUMBS_VERSION "(0, 0, 4)")

# Convenience defines
string(TOUPPER "${PROJECT_NAME}" UPPER_PROJECT_NAME)
string(TOLOWER "${PROJECT_NAME}" LOWER_PROJECT_NAME)

# Get installation directories -- these get used in various places;
# best to just make them available
option(DATACRUMBS_LIBDIR_AS_LIB OFF)

if(NOT DATACRUMBS_LIBDIR_AS_LIB)
    include(GNUInstallDirs)
endif()

if(CMAKE_INSTALL_LIBDIR AND NOT DATACRUMBS_LIBDIR_AS_LIB)
    set(DATACRUMBS_LIBDIR ${CMAKE_INSTALL_LIBDIR})
    set(DATACRUMBS_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
    set(DATACRUMBS_INSTALL_INCLUDE_DIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    set(DATACRUMBS_INSTALL_DOCDIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR})
    set(DATACRUMBS_INSTALL_SYSCONFDIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME})
    set(DATACRUMBS_INSTALL_BINARYDIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
    set(DATACRUMBS_INSTALL_LIBEXEC
        ${CMAKE_INSTALL_PREFIX}/libexec/${PROJECT_NAME})    
else()
    set(DATACRUMBS_LIBDIR lib)
    set(DATACRUMBS_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    set(DATACRUMBS_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
    set(DATACRUMBS_INSTALL_DOCDIR "${CMAKE_INSTALL_PREFIX}/doc")
    set(DATACRUMBS_INSTALL_SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc/${PROJECT_NAME}")
    set(DATACRUMBS_INSTALL_BINARYDIR "${CMAKE_INSTALL_PREFIX}/bin")
    set(DATACRUMBS_INSTALL_LIBEXEC ${CMAKE_INSTALL_PREFIX}/libexec)
endif()

message(STATUS "[${UPPER_PROJECT_NAME}] DATACRUMBS_LIBDIR set to ${DATACRUMBS_LIBDIR}")

# ------------------------------------------------------------------------------
# Internal Paths for cmake libraries and Setup install and output Directories
# ------------------------------------------------------------------------------
# This sets where to look for dependent libraries
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_BINARY_DIR} ${CMAKE_INSTALL_PREFIX})

# This sets where to look for dependent library's cmake files
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/share/cmake)
include(datacrumbs-utils)

# ------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
set(CMAKE_INCLUDE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/include CACHE PATH "Store the headers.")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all Libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all static libraries.")
set(CMAKE_LIBEXEC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libexec)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${DATACRUMBS_INSTALL_LIBEXEC})
include_directories(${CMAKE_LIBEXEC_OUTPUT_DIRECTORY})

# ------------------------------------------------------------------------------
# Options for CMake
# ------------------------------------------------------------------------------
option(DATACRUMBS_BPFTIME_COMPATIBLE "Enable compatibility with bpftime" OFF)
option(DATACRUMBS_ENABLE_OPT "Enable tracing" ON)
option(DATACRUMBS_TRACE_ALL_PROCESSES_OPT "Trace all processes" OFF)
set(DATACRUMBS_INCLUSION_PATH "NONE" CACHE STRING "Inclusion path for datacrumbs")
option(DATACRUMBS_BPF_PRINT_ENABLE "Enable BPF print" OFF)
set(DATACRUMBS_MODE_STR "TRACE" CACHE STRING "Mode for datacrumbs (TRACE or PROFILE)")
set_property(CACHE DATACRUMBS_MODE_STR PROPERTY STRINGS TRACE PROFILE)

# options for managing installation
set(BPFTOOL_EXECUTABLE "NONE" CACHE STRING "Path to bpftool executable")
set(DATACRUMBS_HOST "NONE" CACHE STRING "Host name for datacrumbs (numbers will be stripped if not set)")
set(DATACRUMBS_USER "$ENV{USER}" CACHE STRING "User name for datacrumbs (default: current environment user)")

# Configure Make steps
option(DATACRUMBS_SKIP_PROBE_EXPLORING "Skip building the probe explorer executable" OFF)
option(DATACRUMBS_SKIP_PROBE_GENERATION "Skip building the probe generator executable" OFF)

# Tracing Options
set(DATACRUMBS_TRACE_RINGBUF_SIZE_MB "16" CACHE STRING "Ring buffer size in MB for BPF tracing")
set(DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS "1000" CACHE STRING "Threshold in microseconds for skipping events")

# Profiling Options
set(DATACRUMBS_TIME_INTERVAL_NS "1000000" CACHE STRING "Profiling time interval in nanosecond")

# Other options
set(DATACRUMBS_LOG_LEVEL_STR "INFO" CACHE STRING "Log Level for compiling ${UPPER_PROJECT_NAME}")
set_property(CACHE DATACRUMBS_LOG_LEVEL_STR PROPERTY STRINGS ERROR WARN INFO DEBUG TRACE)

# ------------------------------------------------------------------------------
# Derive CMake variables based on options
# ------------------------------------------------------------------------------
if(DATACRUMBS_BPFTIME_COMPATIBLE)
    set(DATACRUMBS_BPFTIME_COMPATIBLE_FLAG 1)
else()
    set(DATACRUMBS_BPFTIME_COMPATIBLE_FLAG 0)
endif()
if(DATACRUMBS_LOG_LEVEL_STR STREQUAL "ERROR")
    set(DATACRUMBS_LOG_LEVEL 1)
elseif(DATACRUMBS_LOG_LEVEL_STR STREQUAL "WARN")
    set(DATACRUMBS_LOG_LEVEL 2)
elseif(DATACRUMBS_LOG_LEVEL_STR STREQUAL "INFO")
    set(DATACRUMBS_LOG_LEVEL 3)
elseif(DATACRUMBS_LOG_LEVEL_STR STREQUAL "DEBUG")
    set(DATACRUMBS_LOG_LEVEL 4)
elseif(DATACRUMBS_LOG_LEVEL_STR STREQUAL "TRACE")
    set(DATACRUMBS_LOG_LEVEL 5)
endif()

if(DATACRUMBS_MODE_STR AND DATACRUMBS_MODE_STR STREQUAL "TRACE")
    set(DATACRUMBS_MODE 1)
else()
    set(DATACRUMBS_MODE 2)
endif()

if(DATACRUMBS_TRACE_ALL_PROCESSES_OPT AND DATACRUMBS_TRACE_ALL_PROCESSES_OPT STREQUAL "ON")
    set(DATACRUMBS_TRACE_ALL_PROCESSES 1)
else()
    set(DATACRUMBS_TRACE_ALL_PROCESSES 0)
endif()

option(BPFTOOL_EXECUTABLE "Path to bpftool executable" "")

if(BPFTOOL_EXECUTABLE STREQUAL "NONE")
    set(BPFTOOL_EXECUTABLE "")
endif()

if(NOT DATACRUMBS_SKIP_PROBE_EXPLORING)
    set(ENABLE_PROBE_EXPLORER 1)
else()
    set(ENABLE_PROBE_EXPLORER 0)
endif()

if(NOT DATACRUMBS_SKIP_PROBE_GENERATION)
    set(ENABLE_PROBE_GENERATOR 1)
else()
    set(ENABLE_PROBE_GENERATOR 0)
endif()

if(DATACRUMBS_INCLUSION_PATH STREQUAL "NONE")
    set(DATACRUMBS_ENABLE_INCLUSION_PATH 0)
else()
    set(DATACRUMBS_ENABLE_INCLUSION_PATH 1)
endif()

if(DATACRUMBS_BPF_PRINT_ENABLE)
    set(DATACRUMBS_BPF_PRINT_ENABLE_FLAG 1)
else()
    set(DATACRUMBS_BPF_PRINT_ENABLE_FLAG 0)
endif()

set(DATACRUMBS_SRC_GEN_PATH ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY})
set(DATACRUMBS_VARS --user ${DATACRUMBS_USER} --config_path ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/configs --data_dir ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/data --trace_log_dir ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/logs)

if(NOT DATACRUMBS_INCLUSION_PATH STREQUAL "NONE")
    set(DATACRUMBS_VARS ${DATACRUMBS_VARS} --inclusion_path ${DATACRUMBS_INCLUSION_PATH})
endif()

set(DATACRUMBS_BUILD_CLIENT_SO ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR}/libdatacrumbs_client.so)

set(DATACRUMBS_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/configs)
set(DATACRUMBS_LOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/logs)
set(DATACRUMBS_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/data)

if(DATACRUMBS_ENABLE_OPT AND DATACRUMBS_ENABLE_OPT STREQUAL "ON")
    set(DATACRUMBS_ENABLE 1)
else()
    set(DATACRUMBS_ENABLE 0)
endif()

# Detect system kernel version: major, minor, patch
execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_VERSION_STR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Split kernel version string (e.g., "6.5.0-101-generic") into major, minor, patch
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" KERNEL_VERSION_MATCH "${KERNEL_VERSION_STR}")

if(KERNEL_VERSION_MATCH)
    string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" KERNEL_VERSION_MAJOR "${KERNEL_VERSION_STR}")
    string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\..*" "\\1" KERNEL_VERSION_MINOR "${KERNEL_VERSION_STR}")
    string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" KERNEL_VERSION_PATCH "${KERNEL_VERSION_STR}")
else()
    set(KERNEL_VERSION_MAJOR "0")
    set(KERNEL_VERSION_MINOR "0")
    set(KERNEL_VERSION_PATCH "0")
endif()

set(KERNEL_VERSION "(${KERNEL_VERSION_MAJOR}, ${KERNEL_VERSION_MINOR}, ${KERNEL_VERSION_PATCH})")

# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Find system details
# ------------------------------------------------------------------------------
message(STATUS "[${UPPER_PROJECT_NAME}] Detecting system details")

find_library(LIBC_SO NAMES libc.so.6 libc.so PATHS /lib /usr/lib /lib64 /usr/lib64 /usr/local/lib /usr/local/lib64 /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)

if(LIBC_SO)
    message(STATUS "             - Found libc: ${LIBC_SO}")
    set(DATACRUMBS_LIBC_SO ${LIBC_SO})
else()
    message(FATAL_ERROR "             - libc.so not found!")
endif()

if(DATACRUMBS_HOST STREQUAL "NONE")
    # Get the system hostname
    execute_process(
        COMMAND hostname
        OUTPUT_VARIABLE RAW_HOSTNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Remove all numbers from the hostname
    string(REGEX REPLACE "[0-9]" "" DATACRUMBS_HOST "${RAW_HOSTNAME}")
    message(STATUS "             - Derived hostname: ${DATACRUMBS_HOST}")
else()
    message(STATUS "             - Using provided hostname: ${DATACRUMBS_HOST}")
endif()

# Generate vmlinux.h using bpftool before building datacrumbs_bpf
if(BPFTOOL_EXECUTABLE AND NOT BPFTOOL_EXECUTABLE STREQUAL "NONE")
    message(STATUS "             - Using provided bpftool executable: ${BPFTOOL_EXECUTABLE}")
else()
    find_program(BPFTOOL_EXECUTABLE_SEARCH bpftool PATHS /usr/sbin/ /usr/bin/ /usr/local/bin/ /usr/local/sbin/)
    set(BPFTOOL_EXECUTABLE ${BPFTOOL_EXECUTABLE_SEARCH})

    if(NOT BPFTOOL_EXECUTABLE)
        message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] bpftool executable not found! Please install bpftool or specify its path via BPFTOOL_EXECUTABLE.")
    endif()

    message(STATUS "             - Found bpftool executable: ${BPFTOOL_EXECUTABLE}")
endif()

find_file(VMLINUX_BTF_PATH vmlinux PATHS /sys/kernel/btf NO_DEFAULT_PATH)

if(NOT VMLINUX_BTF_PATH)
    message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] vmlinux BTF file not found in /sys/kernel/btf. Please ensure your kernel provides BTF information at /sys/kernel/btf/vmlinux.")
else()
    message(STATUS "             - Found vmlinux BTF file at: ${VMLINUX_BTF_PATH}")
endif()

if(CMAKE_SYSTEM_PROCESSOR)
    set(DATACRUMBS_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
elseif(DEFINED ENV{ARCH})
    set(DATACRUMBS_ARCH "$ENV{ARCH}")
else()
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE DATACRUMBS_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

# Normalize architecture names
if(DATACRUMBS_ARCH STREQUAL "x86_64")
    set(DATACRUMBS_ARCH "x86")
elseif(DATACRUMBS_ARCH STREQUAL "aarch64")
    set(DATACRUMBS_ARCH "arm64")
endif()

message(STATUS "             - Detected architecture: ${DATACRUMBS_ARCH}")

# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Generate Header file for all sources with compile definitions
# ------------------------------------------------------------------------------
message(STATUS "[${UPPER_PROJECT_NAME}] Set Variables for CMake")
message(STATUS "             - DATACRUMBS_LOG_LEVEL: ${DATACRUMBS_LOG_LEVEL}")
message(STATUS "             - DATACRUMBS_BPFTIME_COMPATIBLE: ${DATACRUMBS_BPFTIME_COMPATIBLE}")
message(STATUS "             - DATACRUMBS_MODE: ${DATACRUMBS_MODE}")
message(STATUS "             - DATACRUMBS_ENABLE: ${DATACRUMBS_ENABLE}")
message(STATUS "             - DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS: ${DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS}")
message(STATUS "             - DATACRUMBS_TRACE_ALL_PROCESSES: ${DATACRUMBS_TRACE_ALL_PROCESSES}")
message(STATUS "             - DATACRUMBS_TIME_INTERVAL_NS: ${DATACRUMBS_TIME_INTERVAL_NS}")
message(STATUS "             - DATACRUMBS_BPF_PRINT_ENABLE_FLAG: ${DATACRUMBS_BPF_PRINT_ENABLE_FLAG}")
message(STATUS "             - DATACRUMBS_SKIP_PROBE_EXPLORING: ${DATACRUMBS_SKIP_PROBE_EXPLORING}")
message(STATUS "             - DATACRUMBS_SKIP_PROBE_GENERATION: ${DATACRUMBS_SKIP_PROBE_GENERATION}")
message(STATUS "             - DATACRUMBS_TRACE_RINGBUF_SIZE_MB: ${DATACRUMBS_TRACE_RINGBUF_SIZE_MB}")

message(STATUS "             - DATACRUMBS_SRC_GEN_PATH: ${DATACRUMBS_SRC_GEN_PATH}")
message(STATUS "             - DATACRUMBS_LIBC_SO: ${DATACRUMBS_LIBC_SO}")
message(STATUS "             - DATACRUMBS_BUILD_CLIENT_SO: ${DATACRUMBS_BUILD_CLIENT_SO}")

message(STATUS "             - DATACRUMBS_CONFIG_PATH: ${DATACRUMBS_CONFIG_PATH}")
message(STATUS "             - DATACRUMBS_LOG_DIR: ${DATACRUMBS_LOG_DIR}")
message(STATUS "             - DATACRUMBS_DATA_DIR: ${DATACRUMBS_DATA_DIR}")
message(STATUS "             - DATACRUMBS_ENABLE_INCLUSION_PATH: ${DATACRUMBS_ENABLE_INCLUSION_PATH}")
message(STATUS "             - Kernel Version: ${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_VERSION_PATCH}")
configure_file("${CMAKE_SOURCE_DIR}/cmake/configure_files/datacrumbs_config.h.in"
    "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/datacrumbs_config.h" @ONLY)

# ------------------------------------------------------------------------------
# Make output directories
# ------------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs/server/process/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs/server/bpf/)

# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Find all dependencies
# ------------------------------------------------------------------------------
set(DEPENDENCY_LIBRARY_DIRS "")
set(DEPENDENCY_LIB ${CMAKE_EXE_LINKER_FLAGS} $ENV{LDFLAGS} -lpthread)

message(STATUS "[${UPPER_PROJECT_NAME}] Detecting dependencies")

# find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBPF libbpf)
find_package(yaml-cpp REQUIRED)
find_package(LLVM REQUIRED CONFIG COMPONENTS Clang)
find_package(json-c REQUIRED)
find_package(ZLIB REQUIRED)

# all validation
if(LIBBPF_VERSION VERSION_LESS "1.0.0")
    message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] libbpf version 1.0.0 or newer is required, but found ${LIBBPF_VERSION}")
endif()

# include all links
if(LIBBPF_FOUND)
    include_directories(${LIBBPF_INCLUDEDIR})
    link_directories(${LIBBPF_LIBRARY_DIRS})

    # If LIBBPF_LIBRARY_DIRS is not set, try LIBBPF_LIBDIR, else get parent dir of LIBBPF_LINK_LIBRARIES
    if(NOT LIBBPF_LIBRARY_DIRS)
        if(LIBBPF_LIBDIR)
            set(LIBBPF_LIBRARY_DIRS "${LIBBPF_LIBDIR}")
        elseif(LIBBPF_LINK_LIBRARIES)
            get_filename_component(_LIBBPF_LIB_PARENT "${LIBBPF_LINK_LIBRARIES}" DIRECTORY)
            set(LIBBPF_LIBRARY_DIRS "${_LIBBPF_LIB_PARENT}")
        endif()
    endif()

    list(APPEND DEPENDENCY_LIBRARY_DIRS ${LIBBPF_LIBRARY_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} -L${LIBBPF_LIBRARY_DIRS} -lbpf -lelf)
else()
    message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] libbpf not found!")
endif()

if(${yaml-cpp_FOUND})
    get_filename_component(YAML_CPP_INCLUDE_DIR "${YAML_CPP_INCLUDE_DIR}" ABSOLUTE)
    include_directories(${YAML_CPP_INCLUDE_DIR})

    if(NOT DEFINED YAML_CPP_LIBRARY_DIR)
        get_filename_component(YAML_CPP_LIBRARY_DIR "${YAML_CPP_CMAKE_DIR}/../../" ABSOLUTE)
        get_filename_component(YAML_CPP_LIBRARY_DIR "${YAML_CPP_LIBRARY_DIR}" ABSOLUTE)
    endif()

    list(APPEND DEPENDENCY_LIBRARY_DIRS ${YAML_CPP_LIBRARY_DIR})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} -L${YAML_CPP_LIBRARY_DIR} yaml-cpp)
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] yaml-cpp is needed for ${PROJECT_NAME} build")
endif()

if(LLVM_FOUND)
    include_directories(${LLVM_INCLUDE_DIRS})
    link_directories(${LLVM_LIBRARY_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ${LLVM_LIBRARIES} -lclang)
    set(CLANG_EXECUTABLE ${LLVM_TOOLS_BINARY_DIR}/clang)

    if(NOT EXISTS ${CLANG_EXECUTABLE})
        message(FATAL_ERROR "clang executable not found at ${CLANG_EXECUTABLE}. Please check your LLVM installation.")
    else()
        execute_process(
            COMMAND ${CLANG_EXECUTABLE} --version
            OUTPUT_VARIABLE CLANG_VERSION_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            COMMAND_ECHO NONE
        )

        # message(STATUS "[${UPPER_PROJECT_NAME}] Found clang executable: ${CLANG_EXECUTABLE}")
        # message(STATUS "[${UPPER_PROJECT_NAME}] clang version: ${CLANG_VERSION_OUTPUT}")
    endif()
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] LLVM is needed for ${PROJECT_NAME} build")
endif()

if(json-c_FOUND)
    get_filename_component(json-c_INCLUDE_DIR "${json-c_DIR}/../../../include" ABSOLUTE)
    get_filename_component(json-c_LIBRARY_DIR "${json-c_DIR}/../../" ABSOLUTE)
    include_directories(json-c::json-c)
    list(APPEND DEPENDENCY_LIBRARY_DIRS ${json-c_LIBRARY_DIR})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} json-c::json-c)
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] json-c is needed for ${PROJECT_NAME} build")
endif()

if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    get_filename_component(ZLIB_LIBRARY_DIRS "${ZLIB_LIBRARIES}/../" ABSOLUTE)
    list(APPEND DEPENDENCY_LIBRARY_DIRS ${ZLIB_LIBRARY_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ZLIB::ZLIB)
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] zlib is needed for ${PROJECT_NAME} build")
endif()

list(REMOVE_DUPLICATES DEPENDENCY_LIBRARY_DIRS)

# print found packages
message(STATUS "             - Found libbpf:${LIBBPF_VERSION} at include:${LIBBPF_INCLUDEDIR} lib:${LIBBPF_LIBRARY_DIRS}")
message(STATUS "             - Found yaml-cpp:${yaml-cpp_VERSION} at include:${YAML_CPP_INCLUDE_DIR} lib:${YAML_CPP_LIBRARY_DIR}")
message(STATUS "             - Found llvm:${LLVM_VERSION} at include:${LLVM_INCLUDE_DIRS} lib:${LLVM_LIBRARY_DIRS} clang:${CLANG_EXECUTABLE}")
message(STATUS "             - Found json-c:${json-c_CONSIDERED_VERSIONS} at include:${json-c_INCLUDE_DIR} lib:${json-c_LIBRARY_DIR}")
message(STATUS "             - Found zlib:${ZLIB_VERSION} at include:${ZLIB_INCLUDE_DIRS} lib:${ZLIB_LIBRARY_DIRS}")
message(STATUS "             - DEPENDENCY_LIBRARY_DIRS for RPATH:${DEPENDENCY_LIBRARY_DIRS}")
message(STATUS "             - DEPENDENCY_LIB for linking :${DEPENDENCY_LIB}")

set(CMAKE_INSTALL_RPATH "${DEPENDENCY_LIBRARY_DIRS}")
set(CMAKE_BUILD_RPATH "${DEPENDENCY_LIBRARY_DIRS}")

# print_all_variables()
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Sources for Datacrumbs Components
# ------------------------------------------------------------------------------
set(DATACRUMBS_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/data_structures.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/enumerations.h
)

# Datacrumbs client
set(DATACRUMBS_CLIENT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/client/library.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/client/library.h)
add_library(${PROJECT_NAME}_client SHARED ${DATACRUMBS_CLIENT_SRC})
message(STATUS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set_target_properties(${PROJECT_NAME}_client PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)

# Datacrumbs explorer
set(DATACRUMBS_EXPLORER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/probe_explorer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/probe_explorer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/mechanism/elf_capture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/mechanism/elf_capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/mechanism/header_capture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/mechanism/header_capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/mechanism/ksym_capture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/mechanism/ksym_capture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/explorer/mechanism/usdt_functions.h
)
add_executable(${PROJECT_NAME}_explorer ${DATACRUMBS_EXPLORER_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(${PROJECT_NAME}_explorer ${DEPENDENCY_LIB})
add_dependencies(${PROJECT_NAME}_explorer ${PROJECT_NAME}_client clean_all)

if(NOT DEFINED DATACRUMBS_SKIP_PROBE_EXPLORING OR NOT DATACRUMBS_SKIP_PROBE_EXPLORING)
    add_custom_target(run_explorer
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}_explorer ${DATACRUMBS_HOST} ${DATACRUMBS_VARS}
        DEPENDS ${PROJECT_NAME}_explorer
        COMMENT "Running ${PROJECT_NAME}_explorer with argument '${DATACRUMBS_HOST}'"
    )
else()
    add_custom_target(run_explorer
        COMMAND ${CMAKE_COMMAND} -E echo "Dummy run_explorer target skipped probe exploration"
        COMMENT "Dummy run_explorer target: probe exploration is skipped"
    )
endif()

add_dependencies(run_explorer ${PROJECT_NAME}_client)

# Datacrumbs generator
set(DATACRUMBS_GENERATOR_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/generator/generator.cpp
)
add_executable(${PROJECT_NAME}_generator ${DATACRUMBS_GENERATOR_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(${PROJECT_NAME}_generator ${DEPENDENCY_LIB})
add_dependencies(${PROJECT_NAME}_generator ${PROJECT_NAME}_explorer)

if(NOT DEFINED DATACRUMBS_SKIP_PROBE_GENERATION OR NOT DATACRUMBS_SKIP_PROBE_GENERATION)
    add_custom_target(run_generator
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}_generator ${DATACRUMBS_HOST} ${DATACRUMBS_VARS}
        DEPENDS ${PROJECT_NAME}_generator
        COMMENT "Running ${PROJECT_NAME}_generator with argument '${DATACRUMBS_HOST}'"
    )
    add_dependencies(${PROJECT_NAME}_generator ${PROJECT_NAME}_explorer)
    add_dependencies(run_generator run_explorer)
else()
    add_custom_target(run_generator
        COMMAND ${CMAKE_COMMAND} -E echo "Dummy run_generator target skipped probe generation"
        COMMENT "Dummy run_generator target: probe generation is skipped"
    )
endif()

add_dependencies(${PROJECT_NAME}_generator ${PROJECT_NAME}_explorer)
add_dependencies(run_generator run_explorer)

# Generate vmlinux.h from vmlinux BTF
set(VMLINUX_HEADER "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/server/bpf/vmlinux.h")

add_custom_command(
    OUTPUT ${VMLINUX_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/server/bpf
    COMMAND ${BPFTOOL_EXECUTABLE} btf dump file ${VMLINUX_BTF_PATH} format c > ${VMLINUX_HEADER}
    DEPENDS ${VMLINUX_BTF_PATH}
    COMMENT "[${UPPER_PROJECT_NAME}] Generating vmlinux.h from ${VMLINUX_BTF_PATH} (timed)"
    VERBATIM
)
add_custom_target(vmlinux_header ALL DEPENDS ${VMLINUX_HEADER})

# Compile DataCrumbs BPF Code
set(DATACRUMBS_BPF_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/init.bpf.c
)
set(BPF_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/main.bpf.c)
set(BPF_OUTPUT ${CMAKE_BINARY_DIR}/bpf/datacrumbs.bpf.o)

set(DATACRUMBS_BPF_INCLUDE_PATHS
    ${CMAKE_BINARY_DIR}/include
    ${LIBBPF_INCLUDEDIR}
    ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}
    ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}
    ${DATACRUMBS_INSTALL_LIBEXEC}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set(DATACRUMBS_BPF_INCLUDE_FLAGS "")

foreach(path IN LISTS DATACRUMBS_BPF_INCLUDE_PATHS)
    list(APPEND DATACRUMBS_BPF_INCLUDE_FLAGS -I${path})
endforeach()

# Read the YAML config file and extract the list of probe names under capture_probes
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/configs/${DATACRUMBS_HOST}.yaml" DATACRUMBS_CONFIG_YAML)

# Extract the capture_probes block and get all names under it
set(DATACRUMBS_CAPTURE_PROBES_LIST "")

# Find the capture_probes block
string(REGEX MATCH "capture_probes:[^\n]*\n([^\n]*\n)*" _CAPTURE_PROBES_BLOCK "${DATACRUMBS_CONFIG_YAML}")

# Split the _CAPTURE_PROBES_BLOCK into lines
string(REPLACE "\n" ";" _CAPTURE_PROBES_LINES "${_CAPTURE_PROBES_BLOCK}")

# If any line in _CAPTURE_PROBES_LINES contains 'name:', add it to _CAPTURE_PROBES_NAME_LINES
set(_CAPTURE_PROBES_NAME_LINES "")

foreach(_line ${_CAPTURE_PROBES_LINES})
    if(_line MATCHES "^[ ]*-[ ]*name:[ ]*")
        list(APPEND _CAPTURE_PROBES_NAME_LINES "${_line}")
    endif()
endforeach()

foreach(_line ${_CAPTURE_PROBES_NAME_LINES})
    string(REGEX REPLACE "^[ ]*-[ ]*name:[ ]*" "" _probe_name "${_line}")

    # message(STATUS "[${UPPER_PROJECT_NAME}]  Capture probe name: ${_probe_name}")
    list(APPEND DATACRUMBS_CAPTURE_PROBES_LIST "${_probe_name}")
endforeach()

message(STATUS "[${UPPER_PROJECT_NAME}]  Capture probes: ${DATACRUMBS_CAPTURE_PROBES_LIST}")

list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/common.c)
list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/init.bpf.c)

# For each probe in DATACRUMBS_CAPTURE_PROBES_LIST, add probename.bpf.c to DATACRUMBS_BPF_EXTRA_SOURCES
foreach(_probe_name ${DATACRUMBS_CAPTURE_PROBES_LIST})
    set(_probe_bpf_c "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs/server/bpf/${_probe_name}.bpf.c")
    list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES "${_probe_bpf_c}")
endforeach()

# Find all .c source files in ${CMAKE_BINARY_DIR}/libexec/datacrumbs/server/bpf
set(DATACRUMBS_BPF_OBJECTS "")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bpf)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bpf/server)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bpf/validator)

foreach(BPF_SRC_FILE ${DATACRUMBS_BPF_EXTRA_SOURCES})
    get_filename_component(BPF_SRC_NAME ${BPF_SRC_FILE} NAME_WE)
    set(BPF_OBJ_FILE "${CMAKE_BINARY_DIR}/bpf/server/${BPF_SRC_NAME}.o")

    add_custom_command(
        OUTPUT ${BPF_OBJ_FILE}
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Compiling BPF object: ${BPF_OBJ_FILE} (timed)"
        COMMAND /usr/bin/time -f "Elapsed time for ${BPF_OBJ_FILE}: %E" ${CLANG_EXECUTABLE}
        -target bpf
        -D__TARGET_ARCH_${DATACRUMBS_ARCH}
        -Wall -g -O2
        ${DATACRUMBS_BPF_INCLUDE_FLAGS}
        -c ${BPF_SRC_FILE}
        -o ${BPF_OBJ_FILE}
        DEPENDS ${BPF_SRC_FILE} vmlinux_header run_generator
        COMMENT "Compiling BPF object: ${BPF_OBJ_FILE} (timed)"
        VERBATIM
    )
    list(APPEND DATACRUMBS_BPF_OBJECTS ${BPF_OBJ_FILE})
endforeach()

string(REPLACE ";" " " DATACRUMBS_BPF_OBJECTS_STR "${DATACRUMBS_BPF_OBJECTS}")

# Link all object files into a single object file
add_custom_command(
    OUTPUT ${BPF_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Linking all BPF objects into ${BPF_OUTPUT} (timed)"
    COMMAND /usr/bin/time -f "Elapsed time for linking: %E" ${BPFTOOL_EXECUTABLE} gen object ${BPF_OUTPUT} ${DATACRUMBS_BPF_OBJECTS}
    DEPENDS ${DATACRUMBS_BPF_OBJECTS}
    COMMENT "Linking all BPF objects into ${BPF_OUTPUT} (timed)"
    VERBATIM
)

add_custom_target(datacrumbs_bpf ALL DEPENDS ${BPF_OUTPUT})

if(NOT DEFINED DATACRUMBS_SKIP_PROBE_GENERATION OR NOT DATACRUMBS_SKIP_PROBE_GENERATION)
    add_dependencies(datacrumbs_bpf vmlinux_header run_generator)
endif()

# Generate Skeleton header for Datacrumbs
set(BPF_SKEL_OUTPUT ${CMAKE_BINARY_DIR}/include/datacrumbs/bpf/datacrumbs.skel.h)
add_custom_command(
    OUTPUT ${BPF_SKEL_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/datacrumbs/bpf
    COMMAND ${BPFTOOL_EXECUTABLE} gen skeleton ${BPF_OUTPUT} > ${BPF_SKEL_OUTPUT}
    DEPENDS ${BPF_OUTPUT}
    COMMENT "Generating BPF skeleton: ${BPF_SKEL_OUTPUT}"
    VERBATIM
)
add_custom_target(datacrumbs_bpf_skel ALL DEPENDS ${BPF_SKEL_OUTPUT})
add_dependencies(datacrumbs_bpf_skel datacrumbs_bpf)

# Processing Executable
set(DATACRUMBS_PROCESS_EVENTS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/event_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/writer/chrome_writer.cpp
    ${BPF_SKEL_OUTPUT}
)
include_directories(${CMAKE_INCLUDE_OUTPUT_DIRECTORY})
add_executable(${PROJECT_NAME} ${DATACRUMBS_PROCESS_EVENTS_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(${PROJECT_NAME} ${DEPENDENCY_LIB})
add_dependencies(${PROJECT_NAME} datacrumbs_bpf_skel)

# Install the main processing executable into DATACRUMBS_INSTALL_BINARYDIR
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
)

set(DATACRUMBS_VALIDATE_BPF_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/validator/bpf/probe.bpf.c
)
set(DATACRUMBS_VALIDATE_BPF_OBJ_FILE ${CMAKE_BINARY_DIR}/bpf/validator/probe.bpf.o)
add_custom_command(
    OUTPUT ${DATACRUMBS_VALIDATE_BPF_OBJ_FILE}
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Compiling BPF object: ${DATACRUMBS_VALIDATE_BPF_OBJ_FILE} (timed)"
    COMMAND /usr/bin/time -f "Elapsed time for ${DATACRUMBS_VALIDATE_BPF_OBJ_FILE}: %E" ${CLANG_EXECUTABLE}
    -target bpf
    -D__TARGET_ARCH_${DATACRUMBS_ARCH}
    -Wall -g -O2
    ${DATACRUMBS_BPF_INCLUDE_FLAGS}
    -c ${DATACRUMBS_VALIDATE_BPF_SRC}
    -o ${DATACRUMBS_VALIDATE_BPF_OBJ_FILE}
    DEPENDS ${DATACRUMBS_VALIDATE_BPF_SRC} vmlinux_header
    COMMENT "Compiling BPF object: ${DATACRUMBS_VALIDATE_BPF_OBJ_FILE} (timed)"
    VERBATIM
)

set(DATACRUMBS_VALIDATOR_BPF_OUTPUT ${CMAKE_BINARY_DIR}/bpf/validator/validator.o)

add_custom_command(
    OUTPUT ${DATACRUMBS_VALIDATOR_BPF_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Linking all BPF objects into ${DATACRUMBS_VALIDATOR_BPF_OUTPUT} (timed)"
    COMMAND /usr/bin/time -f "Elapsed time for linking: %E" ${BPFTOOL_EXECUTABLE} gen object ${DATACRUMBS_VALIDATOR_BPF_OUTPUT} ${DATACRUMBS_VALIDATE_BPF_OBJ_FILE}
    DEPENDS ${DATACRUMBS_VALIDATE_BPF_OBJ_FILE}
    COMMENT "Linking all BPF objects into ${DATACRUMBS_VALIDATOR_BPF_OUTPUT} (timed)"
    VERBATIM
)
add_custom_target(datacrumbs_validator_bpf ALL DEPENDS ${DATACRUMBS_VALIDATOR_BPF_OUTPUT})
add_dependencies(datacrumbs_validator_bpf vmlinux_header)

# Generate Skeleton header for Datacrumbs
set(DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT ${CMAKE_BINARY_DIR}/include/datacrumbs/bpf/datacrumbs_validator.skel.h)
add_custom_command(
    OUTPUT ${DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/datacrumbs/bpf
    COMMAND ${BPFTOOL_EXECUTABLE} gen skeleton ${DATACRUMBS_VALIDATOR_BPF_OUTPUT} > ${DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT}
    DEPENDS ${DATACRUMBS_VALIDATOR_BPF_OUTPUT}
    COMMENT "Generating BPF skeleton: ${DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT}"
    VERBATIM
)
add_custom_target(datacrumbs_validator_bpf_skel ALL DEPENDS ${DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT})
add_dependencies(datacrumbs_validator_bpf_skel datacrumbs_validator_bpf)

# Processing Executable
set(DATACRUMBS_VALIDATOR_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/validator/probe_validator.cpp
    ${DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT}
)
add_executable(${PROJECT_NAME}_validator ${DATACRUMBS_VALIDATOR_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(${PROJECT_NAME}_validator ${DEPENDENCY_LIB})
add_dependencies(${PROJECT_NAME}_validator datacrumbs_validator_bpf_skel)

# Install the main processing executable into DATACRUMBS_INSTALL_BINARYDIR
install(TARGETS ${PROJECT_NAME}_validator
    RUNTIME DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
)

# Install host-specific config YAML
set(_DATACRUMBS_HOST_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/configs/${DATACRUMBS_HOST}.yaml")
if(EXISTS "${_DATACRUMBS_HOST_CONFIG}")
    install(FILES "${_DATACRUMBS_HOST_CONFIG}"
        DESTINATION "${DATACRUMBS_INSTALL_SYSCONFDIR}/configs"
    )
else()
    message(WARNING "[${UPPER_PROJECT_NAME}] Host config not found: ${_DATACRUMBS_HOST_CONFIG}; skipping install")
endif()

# Install host/user specific data JSON files
file(GLOB _DATACRUMBS_DATA_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/data/*${DATACRUMBS_USER}-${DATACRUMBS_HOST}*.json"
)

if(_DATACRUMBS_DATA_FILES)
    install(FILES ${_DATACRUMBS_DATA_FILES}
        DESTINATION "${DATACRUMBS_INSTALL_SYSCONFDIR}/data"
    )
else()
    message(WARNING "[${UPPER_PROJECT_NAME}] No data files matching pattern for user '${DATACRUMBS_USER}' and host '${DATACRUMBS_HOST}' found; skipping install.")
endif()


# ------------------------------------------------------------------------------
# Custom executables for Datacrumbs
# ------------------------------------------------------------------------------
if(NOT DEFINED DATACRUMBS_SKIP_PROBE_GENERATION OR NOT DATACRUMBS_SKIP_PROBE_GENERATION)
    add_custom_target(clean_all
        COMMAND ${CMAKE_COMMAND} -E rm -rf
        ${BPF_OUTPUT}
        ${BPF_SKEL_OUTPUT}
        ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs
        COMMENT "Cleaning BPF build artifacts (ignoring if cannot remove)"
    )
else()
    add_custom_target(clean_all
        COMMAND ${CMAKE_COMMAND} -E echo "Dummy clean_all target skipped cleaning BPF build artifacts"
        COMMENT "Dummy clean_all target: skipping cleaning BPF build artifacts"
    )
endif()

# ------------------------------------------------------------------------------
# Run configurations for DataCrumbs executables.
# ------------------------------------------------------------------------------
enable_testing()
add_test(NAME datacrumbs_pretest COMMAND bash -c "ulimit -n 4096")

add_test(NAME datacrumbs_start COMMAND ${CMAKE_BINARY_DIR}/bin/datacrumbs start ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_start PROPERTIES DEPENDS datacrumbs_pretest)
set_tests_properties(datacrumbs_start PROPERTIES DEPENDS ${PROJECT_NAME})

add_test(NAME datacrumbs_run COMMAND ${CMAKE_BINARY_DIR}/bin/datacrumbs run ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_run PROPERTIES DEPENDS datacrumbs_pretest)
set_tests_properties(datacrumbs_run PROPERTIES DEPENDS ${PROJECT_NAME})

add_test(NAME datacrumbs_stop COMMAND ${CMAKE_BINARY_DIR}/bin/datacrumbs stop ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_stop PROPERTIES DEPENDS datacrumbs_pretest)
set_tests_properties(datacrumbs_stop PROPERTIES DEPENDS ${PROJECT_NAME})

add_test(NAME datacrumbs_validator COMMAND ${CMAKE_BINARY_DIR}/bin/datacrumbs_validator ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_validator PROPERTIES DEPENDS ${PROJECT_NAME}_validator)

add_subdirectory(tools)
add_subdirectory(tests)

add_test(NAME test_simple_dd
    COMMAND bash -c "dd if=/dev/zero of=/tmp/img_temp.bat bs=1M count=16"
)
set_tests_properties(test_simple_dd PROPERTIES ENVIRONMENT "LD_PRELOAD=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdatacrumbs_client.so")