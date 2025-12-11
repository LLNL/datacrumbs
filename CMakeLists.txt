cmake_minimum_required(VERSION 3.5)
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

set(DATACRUMBS_RELEASE_VERSION_MAJOR "0")
set(DATACRUMBS_RELEASE_VERSION_MINOR "0")
set(DATACRUMBS_RELEASE_VERSION_PATCH "4")
set(DATACRUMBS_VERSION "(${DATACRUMBS_RELEASE_VERSION_MAJOR}, ${DATACRUMBS_RELEASE_VERSION_MINOR}, ${DATACRUMBS_RELEASE_VERSION_PATCH})")
set(DATACRUMBS_RELEASE_VERSION_STRING "${DATACRUMBS_RELEASE_VERSION_MAJOR}.${DATACRUMBS_RELEASE_VERSION_MINOR}.${DATACRUMBS_RELEASE_VERSION_PATCH}")

# Convenience defines
string(TOUPPER "${PROJECT_NAME}" UPPER_PROJECT_NAME)
string(TOLOWER "${PROJECT_NAME}" LOWER_PROJECT_NAME)

# This sets where to look for dependent library's cmake files
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/share/cmake)
include(datacrumbs-utils)

# ------------------------------------------------------------------------------
# For building only the library without installation
# ------------------------------------------------------------------------------
option(DATACRUMBS_BUILD_ONLY "Build only the datacrumbs library without installation" OFF)

load_build_variables()

message(STATUS "[${UPPER_PROJECT_NAME}] DATACRUMBS_LIBDIR set to ${DATACRUMBS_LIBDIR}")

# ------------------------------------------------------------------------------
# Internal Paths for cmake libraries and Setup install and output Directories
# ------------------------------------------------------------------------------
# This sets where to look for dependent libraries
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_BINARY_DIR} ${CMAKE_INSTALL_PREFIX})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${DATACRUMBS_INSTALL_LIBEXEC})
include_directories(${CMAKE_BINARY_DIR}/libexec)

# ------------------------------------------------------------------------------
# Options for CMake
# ------------------------------------------------------------------------------
option(DATACRUMBS_BPFTIME_COMPATIBLE "Enable compatibility with bpftime" OFF)
option(DATACRUMBS_ENABLE_OPT "Enable tracing" ON)
option(DATACRUMBS_TRACE_ALL_PROCESSES_OPT "Trace all processes" OFF)
set(DATACRUMBS_INCLUSION_PATH "NONE" CACHE STRING "Inclusion path for datacrumbs")
set(DATACRUMBS_CONFIGURED_TRACE_DIR "/tmp" CACHE STRING "Trace directory for datacrumbs")
set(DATACRUMBS_CONFIGURED_LOG_DIR "/tmp" CACHE STRING "Log directory for datacrumbs")
set(DATACRUMBS_CONFIGURED_RUN_DIR "NONE" CACHE STRING "Run directory for datacrumbs")

option(DATACRUMBS_BPF_PRINT_ENABLE "Enable BPF print" OFF)
set(DATACRUMBS_MODE_STR "TRACE" CACHE STRING "Mode for datacrumbs (TRACE or PROFILE)")
set_property(CACHE DATACRUMBS_MODE_STR PROPERTY STRINGS TRACE PROFILE)

# options for managing installation
set(BPFTOOL_EXECUTABLE "NONE" CACHE STRING "Path to bpftool executable")
set(DATACRUMBS_HOST "NONE" CACHE STRING "Host name for datacrumbs (numbers will be stripped if not set)")
set(DATACRUMBS_USER "$ENV{USER}" CACHE STRING "User name for datacrumbs (default: current environment user)")
set(DATACRUMBS_INSTALL_USER "$ENV{USER}" CACHE STRING "Installation user name for datacrumbs (used in file naming)")

# Configure Make steps
option(DATACRUMBS_SKIP_PROBE_EXPLORING "Skip building the probe explorer executable" OFF)
option(DATACRUMBS_SKIP_PROBE_GENERATION "Skip building the probe generator executable" OFF)

# Tracing Options
set(DATACRUMBS_TRACE_RINGBUF_SIZE_MB "16" CACHE STRING "Ring buffer size in MB for BPF tracing")
set(DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS "1000" CACHE STRING "Threshold in microseconds for skipping events")

# Profiling Options
set(DATACRUMBS_TIME_INTERVAL_NS "1000000" CACHE STRING "Profiling time interval in nanosecond")

# Scheduling Options
set(DATACRUMBS_SCHEDULER_TYPE "FLUX" CACHE STRING "Scheduler type for datacrumbs (FLUX, SLURM, OPENMPI, or NONE)")
set_property(CACHE DATACRUMBS_SCHEDULER_TYPE PROPERTY STRINGS FLUX SLURM OPENMPI NONE)
set(DATACRUMBS_SCHEDULER_JOBID_ENV_VAR "NONE" CACHE STRING "Scheduler job ID environment variable name")
set(DATACRUMBS_SCHEDULER_NODES_CMD_OPT "NONE" CACHE STRING "Scheduler option for setting nodes")
set(DATACRUMBS_SCHEDULER_PPN_CMD_OPT "NONE" CACHE STRING "Scheduler option for setting processes per node")
set(DATACRUMBS_SCHEDULER_RUN_CMD "NONE" CACHE STRING "Scheduler run command")
set(DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS "" CACHE STRING "Scheduler run extra args")

# Other options
set(DATACRUMBS_LOG_LEVEL_STR "INFO" CACHE STRING "Log Level for compiling ${UPPER_PROJECT_NAME}")
set_property(CACHE DATACRUMBS_LOG_LEVEL_STR PROPERTY STRINGS ERROR WARN INFO DEBUG TRACE)
set(DATACRUMBS_KERNEL_VERSION "" CACHE STRING "Kernel version")
set(DATACRUMBS_KERNEL_HEADERS_PATH "" CACHE STRING "Path to the kernel headers directory")
option(DATACRUMBS_BUILD_ONLY "Build only the datacrumbs library without installation" OFF)
set(DATACRUMBS_KERNEL_PATH "" CACHE STRING "Path to the kernel source directory")
option(DATACRUMBS_ENABLE_MPI_SUPPORT "Enable MPI support in datacrumbs" OFF)

# ------------------------------------------------------------------------------
# Derive CMake variables based on options
# ------------------------------------------------------------------------------
derive_configurations()

# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Find system details
# ------------------------------------------------------------------------------
find_system_details()

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
message(STATUS "             - DATACRUMBS_BUILD_ONLY: ${DATACRUMBS_BUILD_ONLY}")
message(STATUS "             - DATACRUMBS_INSTALL_USER: ${DATACRUMBS_INSTALL_USER}")
message(STATUS "             - DATACRUMBS_USER: ${DATACRUMBS_USER}")
message(STATUS "             - DATACRUMBS_KERNEL_VERSION: ${DATACRUMBS_KERNEL_VERSION}")
message(STATUS "             - Found kernel headers at: ${DATACRUMBS_KERNEL_HEADERS_PATH}")
message(STATUS "             - DATACRUMBS_KERNEL_UNAME_R: ${DATACRUMBS_KERNEL_UNAME_R}")
message(STATUS "             - DATACRUMBS_CONFIGURED_TRACE_DIR: ${DATACRUMBS_CONFIGURED_TRACE_DIR}")
message(STATUS "             - DATACRUMBS_CONFIGURED_LOG_DIR: ${DATACRUMBS_CONFIGURED_LOG_DIR}")
message(STATUS "             - DATACRUMBS_SCHEDULER_TYPE: ${DATACRUMBS_SCHEDULER_TYPE}")
message(STATUS "             - DATACRUMBS_SCHEDULER_JOBID_ENV_VAR: ${DATACRUMBS_SCHEDULER_JOBID_ENV_VAR}")
message(STATUS "             - DATACRUMBS_SCHEDULER_NODES_CMD_OPT: ${DATACRUMBS_SCHEDULER_NODES_CMD_OPT}")
message(STATUS "             - DATACRUMBS_SCHEDULER_PPN_CMD_OPT: ${DATACRUMBS_SCHEDULER_PPN_CMD_OPT}")
message(STATUS "             - DATACRUMBS_SCHEDULER_RUN_CMD: ${DATACRUMBS_SCHEDULER_RUN_CMD}")

if(NOT EXISTS "${DATACRUMBS_CONFIGURED_TRACE_DIR}")
    message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] DATACRUMBS_CONFIGURED_TRACE_DIR does not exist: ${DATACRUMBS_CONFIGURED_TRACE_DIR}")
endif()

if(NOT EXISTS "${DATACRUMBS_CONFIGURED_LOG_DIR}")
    message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] DATACRUMBS_CONFIGURED_LOG_DIR does not exist: ${DATACRUMBS_CONFIGURED_LOG_DIR}")
endif()

# ------------------------------------------------------------------------------
# Configure output files for build
# ------------------------------------------------------------------------------
configure_file("${CMAKE_SOURCE_DIR}/cmake/configure_files/datacrumbs_config.h.in"
    "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/datacrumbs_config.h" @ONLY)
configure_file("${CMAKE_SOURCE_DIR}/cmake/configure_files/datacrumbs_config.h.in"
    "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/composable/include/datacrumbs/datacrumbs_config.h" @ONLY)
install(FILES "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/datacrumbs_config.h"
    DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/composable/include/datacrumbs/")

configure_file("${CMAKE_SOURCE_DIR}/etc/datacrumbs/configs/${DATACRUMBS_HOST}.yaml"
    "${CMAKE_CONFIG_OUTPUT_DIRECTORY}/${DATACRUMBS_HOST}.yaml" @ONLY)

file(GLOB _DATACRUMBS_DATA_FILES_COPY
    "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/data/*${DATACRUMBS_USER}-${DATACRUMBS_HOST}*.json"
)

if(_DATACRUMBS_DATA_FILES_COPY)
    file(COPY ${_DATACRUMBS_DATA_FILES_COPY}
        DESTINATION "${CMAKE_DATA_OUTPUT_DIRECTORY}"
    )
else()
    message(WARNING "[${UPPER_PROJECT_NAME}] No data files to copy for user '${DATACRUMBS_USER}' and host '${DATACRUMBS_HOST}'")
endif()

# ------------------------------------------------------------------------------
# Make output directories
# ------------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/process/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/bpf/objects/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/bpf/src/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/validator/bpf/objects/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/validator/bpf/src/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/objects/)

# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Find all dependencies
# ------------------------------------------------------------------------------
include_dependencies()

# print_all_variables()
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Sources for Datacrumbs Components
# ------------------------------------------------------------------------------
set(DATACRUMBS_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.h
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
install(TARGETS ${PROJECT_NAME}_client
    LIBRARY DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
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
set(BPF_OUTPUT ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/objects/datacrumbs.bpf.o)

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
file(READ "${DATACRUMBS_CONFIG_PATH}/${DATACRUMBS_HOST}.yaml" DATACRUMBS_CONFIG_YAML)

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

message(STATUS "[${UPPER_PROJECT_NAME}] Capture probes: ${DATACRUMBS_CAPTURE_PROBES_LIST}")

list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/common.c)
list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/init.bpf.c)

# For each probe in DATACRUMBS_CAPTURE_PROBES_LIST, add probename.bpf.c to DATACRUMBS_BPF_EXTRA_SOURCES
foreach(_probe_name ${DATACRUMBS_CAPTURE_PROBES_LIST})
    set(_probe_bpf_c "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/bpf/src/${_probe_name}.bpf.c")
    list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES "${_probe_bpf_c}")
endforeach()

# Find all .c source files in ${CMAKE_BINARY_DIR}/libexec/datacrumbs/server/bpf
set(DATACRUMBS_BPF_OBJECTS "")

foreach(BPF_SRC_FILE ${DATACRUMBS_BPF_EXTRA_SOURCES})
    get_filename_component(BPF_SRC_NAME ${BPF_SRC_FILE} NAME_WE)
    set(BPF_OBJ_FILE "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/bpf/objects/${BPF_SRC_NAME}.o")

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
    install(FILES ${BPF_OBJ_FILE}
        DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/objects"
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
set(BPF_SKEL_OUTPUT ${CMAKE_BINARY_DIR}/include/datacrumbs/server/bpf/datacrumbs.skel.h)
add_custom_command(
    OUTPUT ${BPF_SKEL_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/datacrumbs/server/bpf
    COMMAND ${BPFTOOL_EXECUTABLE} gen skeleton ${BPF_OUTPUT} > ${BPF_SKEL_OUTPUT}
    DEPENDS ${BPF_OUTPUT}
    COMMENT "Generating BPF skeleton: ${BPF_SKEL_OUTPUT}"
    VERBATIM
)
add_custom_target(datacrumbs_bpf_skel ALL DEPENDS ${BPF_SKEL_OUTPUT})
add_dependencies(datacrumbs_bpf_skel datacrumbs_bpf)

# Build object file with everything except server.cpp
set(DATACRUMBS_PROCESS_EVENTS_OBJ_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/writer/chrome_writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/event_processor.cpp
    ${DATACRUMBS_COMMON_SRC}
)
set(DATACRUMBS_SERVER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/server.cpp
)

include_directories(${CMAKE_INCLUDE_OUTPUT_DIRECTORY})

add_library(datacrumbs_obj SHARED ${DATACRUMBS_PROCESS_EVENTS_OBJ_SRC})
target_link_libraries(datacrumbs_obj ${DEPENDENCY_LIB})
install(TARGETS datacrumbs_obj
    LIBRARY DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
)
add_dependencies(datacrumbs_obj datacrumbs_bpf_skel)

# Build the final executable by linking server.cpp and the object library
add_executable(${PROJECT_NAME} ${DATACRUMBS_SERVER_SRC})
target_link_libraries(${PROJECT_NAME} datacrumbs_obj ${DEPENDENCY_LIB})
add_dependencies(${PROJECT_NAME} datacrumbs_bpf_skel datacrumbs_obj)

# Install the main processing executable into DATACRUMBS_INSTALL_BINARYDIR
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${DATACRUMBS_INSTALL_SBINARYDIR}"
)

# Install the main processing datacrumbs object into DATACRUMBS_INSTALL_LIBEXEC
install(FILES ${BPF_OUTPUT}
    DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/objects"
)

set(DATACRUMBS_VALIDATE_BPF_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/validator/bpf/probe.bpf.c
)
set(DATACRUMBS_VALIDATE_BPF_OBJ_FILE ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/validator/bpf/objects/probe.bpf.o)
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

set(DATACRUMBS_VALIDATOR_BPF_OUTPUT ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/objects/validator.o)

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
set(DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT ${CMAKE_BINARY_DIR}/include/datacrumbs/validator/bpf/datacrumbs_validator.skel.h)
add_custom_command(
    OUTPUT ${DATACRUMBS_VALIDATOR_BPF_SKEL_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/datacrumbs/validator/bpf
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
add_dependencies(${PROJECT_NAME}_validator datacrumbs_bpf_skel)

# Install the main processing executable into DATACRUMBS_INSTALL_BINARYDIR
install(TARGETS ${PROJECT_NAME}_validator
    RUNTIME DESTINATION "${DATACRUMBS_INSTALL_SBINARYDIR}"
)

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
# Install Files
# ------------------------------------------------------------------------------

# Compose cmake file
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/composable)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/datacrumbs-utils.cmake"
    "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/composable/cmake/modules/datacrumbs-utils.cmake" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/composable/CMakeLists.txt.in"
    "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/composable/CMakeLists.txt" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/composable/process/server.cpp"
    "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/composable/src/datacrumbs/composable/process/server.cpp" @ONLY)
install(FILES "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/composable/cmake/modules/datacrumbs-utils.cmake"
    DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/composable/cmake/modules"
)
install(FILES "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/composable/CMakeLists.txt"
    DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/composable"
)
install(FILES "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/composable/src/datacrumbs/composable/process/server.cpp"
    DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/composable/src/datacrumbs/composable/process"
)
install(FILES "${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/process/generated_process.h"
    DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/composable/include/datacrumbs/server/process"
)

# Install src and header files for composable components
set(DATACRUMBS_PUBLIC_HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/event_processor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/writer/chrome_writer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/compress/zlib_compressor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/data_structures.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/enumerations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/logging.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/constants.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/typedefs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/shared.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/compat/map.h
)
set(DATACRUMBS_PUBLIC_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/server.impl.cpp
)

datacrumbs_composable_install_headers("${DATACRUMBS_PUBLIC_HEADER_FILES}")
datacrumbs_composable_install_src("${DATACRUMBS_PUBLIC_SRC_FILES}")

# Install project configuration file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/configs/project.yaml.in ${CMAKE_CONFIG_OUTPUT_DIRECTORY}/project.yaml @ONLY)
install(
    FILES
    ${CMAKE_CONFIG_OUTPUT_DIRECTORY}/project.yaml
    DESTINATION
    ${DATACRUMBS_INSTALL_ETC_CONFIGSDIR}
)

# Install host-specific config YAML
set(_DATACRUMBS_HOST_CONFIG "${CMAKE_CONFIG_OUTPUT_DIRECTORY}/${DATACRUMBS_HOST}.yaml")

if(EXISTS "${_DATACRUMBS_HOST_CONFIG}")
    install(FILES "${_DATACRUMBS_HOST_CONFIG}"
        DESTINATION "${DATACRUMBS_INSTALL_ETC_CONFIGSDIR}"
    )
else()
    message(WARNING "[${UPPER_PROJECT_NAME}] Host config not found: ${_DATACRUMBS_HOST_CONFIG}; skipping install")
endif()

# Install host/user specific data JSON files
set(_DATACRUMBS_DATA_FILES
    "${CMAKE_DATA_OUTPUT_DIRECTORY}/categories-${DATACRUMBS_USER}-${DATACRUMBS_HOST}.json"
    "${CMAKE_DATA_OUTPUT_DIRECTORY}/probes-${DATACRUMBS_USER}-${DATACRUMBS_HOST}.json"
)

# Ensure the data output directory exists
file(MAKE_DIRECTORY ${CMAKE_DATA_OUTPUT_DIRECTORY})

if(_DATACRUMBS_DATA_FILES)
    install(FILES ${_DATACRUMBS_DATA_FILES}
        DESTINATION "${DATACRUMBS_INSTALL_DATADIR}"
    )
else()
    message(WARNING "[${UPPER_PROJECT_NAME}] No data files matching pattern for user '${DATACRUMBS_USER}' and host '${DATACRUMBS_HOST}' found; skipping install.")
endif()

# Module file
configure_file("${CMAKE_SOURCE_DIR}/etc/datacrumbs/modulefiles/module.lua.in"
    "${CMAKE_MODULES_OUTPUT_DIRECTORY}/${PROJECT_NAME}/${DATACRUMBS_RELEASE_VERSION_STRING}.lua" @ONLY)

install(FILES "${CMAKE_MODULES_OUTPUT_DIRECTORY}/${PROJECT_NAME}/${DATACRUMBS_RELEASE_VERSION_STRING}.lua"
    DESTINATION "${DATACRUMBS_INSTALL_ETC_MODULESDIR}/${PROJECT_NAME}"
)

# flux cli plugin file
configure_file("${CMAKE_SOURCE_DIR}/etc/datacrumbs/flux/cli/plugins/datacrumbs.py"
    "${CMAKE_FLUX_OUTPUT_DIRECTORY}/cli/plugins/datacrumbs.py" @ONLY)

install(FILES "${CMAKE_FLUX_OUTPUT_DIRECTORY}/cli/plugins/datacrumbs.py"
    DESTINATION "${DATACRUMBS_INSTALL_ETC_FLUX}/cli/plugins"
)

# ------------------------------------------------------------------------------
# Install Datacrumbs scripts
# ------------------------------------------------------------------------------
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_setup.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_setup @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_setup
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_utility.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_utility @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_utility
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_args.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_args @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_args
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_logging.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_logging @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_logging
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_run.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_run @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_run
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_run_validator.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_run_validator @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_run_validator
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_track.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_track @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_track
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_untrack.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_untrack @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_untrack
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_wrap.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_wrap @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_wrap
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_compose.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_compose @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_compose
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_compose_run.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_compose_run @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_compose_run
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_stop.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_stop @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_stop
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_compose_wrapper.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_compose_wrapper @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_compose_wrapper
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_create_log_dir.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_create_log_dir @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_create_log_dir
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_server_run.sh.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_server_run.sh @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_server_run.sh
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_server_stop.sh.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_server_stop.sh @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_server_stop.sh
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/systemd/datacrumbs@.service.in ${CMAKE_SYSTEMD_OUTPUT_DIRECTORY}/datacrumbs@.service @ONLY)
install(
    PROGRAMS
    ${CMAKE_SYSTEMD_OUTPUT_DIRECTORY}/datacrumbs@.service
    DESTINATION
    ${DATACRUMBS_INSTALL_ETC_SYSTEMD}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_service_common.sh.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_service_common.sh @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_service_common.sh
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_service_run.sh.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_service_run.sh @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_service_run.sh
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_service_stop.sh.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_service_stop.sh @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_service_stop.sh
    DESTINATION
    ${DATACRUMBS_INSTALL_SBINARYDIR}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/datacrumbs/datacrumbs_salloc.in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_salloc @ONLY)
install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_salloc
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)

# Create an empty directory at install time
install(CODE "file(MAKE_DIRECTORY \"\${DATACRUMBS_INSTALL_LIBEXEC}/sbin\")")

# ------------------------------------------------------------------------------
# Run configurations for DataCrumbs executables.
# ------------------------------------------------------------------------------
enable_testing()

# set(DATACRUMBS_VARS --user ${DATACRUMBS_USER} --config_path ${CMAKE_CONFIG_OUTPUT_DIRECTORY} --data_dir ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/data --trace_log_dir $ENV{HOME}/logs)
add_test(NAME datacrumbs_validator COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_validator ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_validator PROPERTIES DEPENDS ${PROJECT_NAME}_validator)

add_test(NAME datacrumbs_pretest COMMAND bash -c "ulimit -n 4096")

set(DATACRUMBS_VARS --user ${DATACRUMBS_USER} --config_path ${DATACRUMBS_INSTALL_SYSCONFDIR}/configs --data_dir ${DATACRUMBS_INSTALL_SYSCONFDIR}/data --trace_log_dir $ENV{HOME}/logs)

if(NOT DATACRUMBS_INCLUSION_PATH STREQUAL "NONE")
    set(DATACRUMBS_VARS ${DATACRUMBS_VARS} --inclusion_path ${DATACRUMBS_INCLUSION_PATH})
endif()

add_test(NAME datacrumbs_start COMMAND ${DATACRUMBCMAKE_RUNTIME_OUTPUT_DIRECTORYS_PATH}/datacrumbs start ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_start PROPERTIES DEPENDS datacrumbs_pretest)
set_tests_properties(datacrumbs_start PROPERTIES DEPENDS ${PROJECT_NAME})

add_test(NAME datacrumbs_run COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs run ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_run PROPERTIES DEPENDS datacrumbs_pretest)
set_tests_properties(datacrumbs_run PROPERTIES DEPENDS ${PROJECT_NAME})

add_test(NAME datacrumbs_stop COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs stop ${DATACRUMBS_HOST} ${DATACRUMBS_VARS})
set_tests_properties(datacrumbs_stop PROPERTIES DEPENDS datacrumbs_pretest)
set_tests_properties(datacrumbs_stop PROPERTIES DEPENDS ${PROJECT_NAME})

add_subdirectory(tools)
add_subdirectory(tests)

add_test(NAME test_simple_dd
    COMMAND bash -c "dd if=/dev/zero of=/tmp/img_temp.bat bs=1M count=16"
)
set_tests_properties(test_simple_dd PROPERTIES ENVIRONMENT "LD_PRELOAD=${DATACRUMBS_BUILD_CLIENT_SO}")