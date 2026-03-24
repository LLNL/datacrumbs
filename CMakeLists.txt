cmake_minimum_required(VERSION 3.20)
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
include_directories(${CMAKE_BINARY_DIR}/libexec)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(ExternalProject)

set(_DATACRUMBS_UTILS_GITHUB_REPOSITORY_DEFAULT
    "https://github.com/LLNL/datacrumbs-utils.git"
)

set(
  DATACRUMBS_UTILS_GIT_REPOSITORY
  "${_DATACRUMBS_UTILS_GITHUB_REPOSITORY_DEFAULT}"
  CACHE STRING "GitHub repository URL for datacrumbs-utils" FORCE
)
set(
  DATACRUMBS_UTILS_GIT_REF
  "main"
  CACHE STRING "Git branch, tag, or commit used for datacrumbs-utils bootstrap"
)
set(
  DATACRUMBS_UTILS_SOURCE_DIR
  "${CMAKE_BINARY_DIR}/_deps/datacrumbs-utils-src"
  CACHE PATH "Path to the datacrumbs-utils source tree" FORCE
)
set(
  DATACRUMBS_UTILS_BUILD_DIR
  "${CMAKE_BINARY_DIR}/_deps/datacrumbs-utils-build"
  CACHE PATH "Path to the datacrumbs-utils build tree" FORCE
)
set(
  DATACRUMBS_UTILS_INSTALL_PREFIX
  "${CMAKE_INSTALL_PREFIX}"
  CACHE PATH "Path to the datacrumbs-utils install prefix"
)

# ------------------------------------------------------------------------------
# Options for CMake
# ------------------------------------------------------------------------------
option(DATACRUMBS_BPFTIME_COMPATIBLE "Enable compatibility with bpftime" OFF)
option(DATACRUMBS_ENABLE_OPT "Enable tracing" ON)
option(DATACRUMBS_TRACE_ALL_PROCESSES_OPT "Trace all processes" OFF)
option(DATACRUMBS_BPF_PRINT_ENABLE "Enable BPF print" OFF)
option(DATACRUMBS_DISABLE_PROBE_SIGNING "Disable probe signing and signature verification" OFF)
set(DATACRUMBS_MODE_STR "TRACE" CACHE STRING "Mode for datacrumbs (TRACE or PROFILE)")
set_property(CACHE DATACRUMBS_MODE_STR PROPERTY STRINGS TRACE PROFILE)

# options for managing installation
set(BPFTOOL_EXECUTABLE "NONE" CACHE STRING "Path to bpftool executable")

set(DATACRUMBS_INCLUSION_PATH "NONE" CACHE STRING "Inclusion path for datacrumbs")
set(DATACRUMBS_CONFIGURED_TRACE_DIR "/tmp" CACHE STRING "Trace directory for datacrumbs")
set(DATACRUMBS_CONFIGURED_LOG_DIR "/tmp" CACHE STRING "Log directory for datacrumbs")
set(DATACRUMBS_CONFIGURED_RUN_DIR "NONE" CACHE STRING "Run directory for datacrumbs")
set(DATACRUMBS_KERNEL_HEADERS_PATH "" CACHE PATH "Override path to the kernel headers for datacrumbs")
set(DATACRUMBS_HOST "NONE" CACHE STRING "Host name for datacrumbs")
set(DATACRUMBS_USER "$ENV{USER}" CACHE STRING "User name for datacrumbs")
set(DATACRUMBS_INSTALL_USER "$ENV{USER}" CACHE STRING "Installation user name for datacrumbs")
set(DATACRUMBS_SCHEDULER_TYPE "FLUX" CACHE STRING "Scheduler type for datacrumbs")
set_property(CACHE DATACRUMBS_SCHEDULER_TYPE PROPERTY STRINGS FLUX SLURM OPENMPI NONE)
set(DATACRUMBS_SCHEDULER_JOBID_ENV_VAR "NONE" CACHE STRING "Scheduler job ID environment variable name")
set(DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS "" CACHE STRING "Scheduler run extra args")

# Configure Make steps
option(DATACRUMBS_SKIP_PROBE_EXPLORING "Skip building the probe explorer executable" OFF)
option(DATACRUMBS_SKIP_PROBE_GENERATION "Skip building the probe generator executable" OFF)

# Tracing Options
set(DATACRUMBS_TRACE_RINGBUF_SIZE_MB "16" CACHE STRING "Ring buffer size in MB for BPF tracing")
set(DATACRUMBS_MAX_RUNTIME_FUNCTIONS "16384" CACHE STRING "Maximum number of runtime functions supported in one probes file")
set(DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS "1000" CACHE STRING "Threshold in microseconds for skipping events")

# Profiling Options
set(DATACRUMBS_TIME_INTERVAL_NS "1000000" CACHE STRING "Profiling time interval in nanosecond")

# Other options
set(DATACRUMBS_LOG_LEVEL_STR "INFO" CACHE STRING "Log Level for compiling ${UPPER_PROJECT_NAME}")
set_property(CACHE DATACRUMBS_LOG_LEVEL_STR PROPERTY STRINGS ERROR WARN INFO DEBUG TRACE)
option(DATACRUMBS_BUILD_ONLY "Build only the datacrumbs library without installation" OFF)

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
set(DATACRUMBS_INSTALL_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}")
set(DATACRUMBS_INSTALL_HOST "${DATACRUMBS_HOST}")
set(DATACRUMBS_INSTALL_CONFIGS_DIR "${DATACRUMBS_INSTALL_ETC_CONFIGSDIR}")
set(DATACRUMBS_INSTALL_SHARED_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/datacrumbs/data")
set(DATACRUMBS_INSTALL_DATA_DIR "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}")
set(DATACRUMBS_SYSTEM_PROBE_FILE
    "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}/system-probe-${DATACRUMBS_INSTALL_USER}-${DATACRUMBS_HOST}.json.gz"
)
set(DATACRUMBS_PROBE_SECRET_FILE
    "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}/.datacrumbs-probe-secret"
)
set(DATACRUMBS_SIGN_PROBES_SOCKET_PATH
    "${DATACRUMBS_INSTALL_SHARED_DATA_DIR}/datacrumbs-sign-probes.sock"
)
set(DATACRUMBS_UTILS_SYSTEM_CONFIGURATION_FILE
    "${CMAKE_BINARY_DIR}/configs/datacrumbs-system-configuration.json.gz"
)
set(DATACRUMBS_TRACE_DIR_PATTERN "${DATACRUMBS_CONFIGURED_TRACE_DIR}/%YY%/%MM%/%DD%")
set(DATACRUMBS_CLIENT_LIB "${DATACRUMBS_UTILS_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libdatacrumbs_client.so")
set(DATACRUMBS_JOB_SCHEDULER "${DATACRUMBS_SCHEDULER_TYPE}")
set(DATACRUMBS_JOB_OTHER_FLAGS "${DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS}")
set(DATACRUMBS_JOB_ID_VAR "${DATACRUMBS_SCHEDULER_JOBID_ENV_VAR}")
if(DATACRUMBS_ENABLE_INCLUSION_PATH)
  set(DATACRUMBS_INCLUSION_PATHS "${DATACRUMBS_INCLUSION_PATH}")
else()
  set(DATACRUMBS_INCLUSION_PATHS "")
endif()
set(DATACRUMBS_SERVER_RUN_DIR "${DATACRUMBS_INSTALL_RUNSTATEDIR}/datacrumbs")
set(DATACRUMBS_SERVER_RUN_ID_FILE "${DATACRUMBS_SERVER_RUN_DIR}/datacrumbs.runid")

set(DATACRUMBS_SYSTEM_CONFIGURATION_DEFINE_KEYS
    DATACRUMBS_INSTALL_PREFIX
    DATACRUMBS_INSTALL_HOST
    DATACRUMBS_INSTALL_USER
    DATACRUMBS_INSTALL_BIN_DIR
    DATACRUMBS_INSTALL_SBIN_DIR
    DATACRUMBS_INSTALL_LIB_DIR
    DATACRUMBS_INSTALL_LIBEXEC_DIR
    DATACRUMBS_INSTALL_ETC_DIR
    DATACRUMBS_INSTALL_DATA_DIR
    DATACRUMBS_INSTALL_MODULES_DIR
    DATACRUMBS_INSTALL_PROBE_OBJECTS_DIR
    DATACRUMBS_CLIENT_LIB
    DATACRUMBS_JOB_SCHEDULER
    DATACRUMBS_JOB_OTHER_FLAGS
    DATACRUMBS_JOB_ID_VAR
    DATACRUMBS_SERVER_RUN_DIR
    DATACRUMBS_CONFIGURED_TRACE_DIR
    DATACRUMBS_USER
    DATACRUMBS_INCLUSION_PATHS
    DATACRUMBS_KERNEL_UNAME_R
    DATACRUMBS_KERNEL_HEADERS_PATH
    DATACRUMBS_LIBC_SO
)

set(DATACRUMBS_INSTALL_PREFIX "${DATACRUMBS_INSTALL_PREFIX_PATH}")
set(DATACRUMBS_INSTALL_BIN_DIR "${DATACRUMBS_INSTALL_BINARYDIR}")
set(DATACRUMBS_INSTALL_SBIN_DIR "${DATACRUMBS_INSTALL_SBINARYDIR}")
set(DATACRUMBS_INSTALL_LIBEXEC_DIR "${DATACRUMBS_INSTALL_LIBEXEC}")
set(DATACRUMBS_INSTALL_ETC_DIR "${DATACRUMBS_INSTALL_SYSCONFDIR}")
set(DATACRUMBS_INSTALL_MODULES_DIR "${DATACRUMBS_INSTALL_ETC_MODULESDIR}")
set(DATACRUMBS_INSTALL_PROBE_OBJECTS_DIR "${DATACRUMBS_INSTALL_LIBEXEC}/objects")

set(DATACRUMBS_SYSTEM_CONFIGURATION_DEFINES "")
foreach(_dc_key IN LISTS DATACRUMBS_SYSTEM_CONFIGURATION_DEFINE_KEYS)
  if(NOT DEFINED ${_dc_key})
    continue()
  endif()
  set(_dc_value "${${_dc_key}}")
  if("${_dc_value}" STREQUAL "")
    continue()
  endif()
  if(_dc_value MATCHES "^[+-]?[0-9]+$")
    string(APPEND DATACRUMBS_SYSTEM_CONFIGURATION_DEFINES "#define ${_dc_key} ${_dc_value}\n")
  else()
    string(REPLACE "\\" "\\\\" _dc_escaped "${_dc_value}")
    string(REPLACE "\"" "\\\"" _dc_escaped "${_dc_escaped}")
    string(APPEND DATACRUMBS_SYSTEM_CONFIGURATION_DEFINES "#define ${_dc_key} \"${_dc_escaped}\"\n")
  endif()
endforeach()

# ------------------------------------------------------------------------------
# Generate Header file for all sources with compile definitions
# ------------------------------------------------------------------------------
message(STATUS "[${UPPER_PROJECT_NAME}] Set Variables for CMake")
message(STATUS "             - DATACRUMBS_LOG_LEVEL: ${DATACRUMBS_LOG_LEVEL}")
message(STATUS "             - DATACRUMBS_BPFTIME_COMPATIBLE: ${DATACRUMBS_BPFTIME_COMPATIBLE}")
message(STATUS "             - DATACRUMBS_MODE: ${DATACRUMBS_MODE}")
message(STATUS "             - DATACRUMBS_ENABLE: ${DATACRUMBS_ENABLE}")
message(STATUS "             - DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS: ${DATACRUMBS_SKIP_SMALL_EVENTS_THRESHOLD_NS}")
message(STATUS "             - DATACRUMBS_MAX_RUNTIME_FUNCTIONS: ${DATACRUMBS_MAX_RUNTIME_FUNCTIONS}")
message(STATUS "             - DATACRUMBS_TRACE_ALL_PROCESSES: ${DATACRUMBS_TRACE_ALL_PROCESSES}")
message(STATUS "             - DATACRUMBS_TIME_INTERVAL_NS: ${DATACRUMBS_TIME_INTERVAL_NS}")
message(STATUS "             - DATACRUMBS_BPF_PRINT_ENABLE_FLAG: ${DATACRUMBS_BPF_PRINT_ENABLE_FLAG}")
message(STATUS "             - DATACRUMBS_SKIP_PROBE_EXPLORING: ${DATACRUMBS_SKIP_PROBE_EXPLORING}")
message(STATUS "             - DATACRUMBS_SKIP_PROBE_GENERATION: ${DATACRUMBS_SKIP_PROBE_GENERATION}")
message(STATUS "             - DATACRUMBS_TRACE_RINGBUF_SIZE_MB: ${DATACRUMBS_TRACE_RINGBUF_SIZE_MB}")

message(STATUS "             - DATACRUMBS_SRC_GEN_PATH: ${DATACRUMBS_SRC_GEN_PATH}")
message(STATUS "             - DATACRUMBS_LIBC_SO: ${DATACRUMBS_LIBC_SO}")

message(STATUS "             - DATACRUMBS_CONFIG_PATH: ${DATACRUMBS_CONFIG_PATH}")
message(STATUS "             - DATACRUMBS_LOG_DIR: ${DATACRUMBS_LOG_DIR}")
message(STATUS "             - DATACRUMBS_DATA_DIR: ${DATACRUMBS_DATA_DIR}")
message(STATUS "             - DATACRUMBS_ENABLE_INCLUSION_PATH: ${DATACRUMBS_ENABLE_INCLUSION_PATH}")
message(STATUS "             - Kernel Version: ${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_VERSION_PATCH}")
message(STATUS "             - DATACRUMBS_BUILD_ONLY: ${DATACRUMBS_BUILD_ONLY}")
message(STATUS "             - DATACRUMBS_INSTALL_USER: ${DATACRUMBS_INSTALL_USER}")
message(STATUS "             - DATACRUMBS_USER: ${DATACRUMBS_USER}")
message(STATUS "             - Found kernel headers at: ${DATACRUMBS_KERNEL_HEADERS_PATH}")
message(STATUS "             - DATACRUMBS_KERNEL_UNAME_R: ${DATACRUMBS_KERNEL_UNAME_R}")
message(STATUS "             - DATACRUMBS_CONFIGURED_TRACE_DIR: ${DATACRUMBS_CONFIGURED_TRACE_DIR}")
message(STATUS "             - DATACRUMBS_CONFIGURED_LOG_DIR: ${DATACRUMBS_CONFIGURED_LOG_DIR}")
message(STATUS "             - DATACRUMBS_SCHEDULER_TYPE: ${DATACRUMBS_SCHEDULER_TYPE}")
message(STATUS "             - DATACRUMBS_SCHEDULER_JOBID_ENV_VAR: ${DATACRUMBS_SCHEDULER_JOBID_ENV_VAR}")
message(STATUS "             - DATACRUMBS_SYSTEM_PROBE_FILE: ${DATACRUMBS_SYSTEM_PROBE_FILE}")
message(STATUS "             - DATACRUMBS_PROBE_SECRET_FILE: ${DATACRUMBS_PROBE_SECRET_FILE}")

# ------------------------------------------------------------------------------
# Configure output files for build
# ------------------------------------------------------------------------------
configure_file("${CMAKE_SOURCE_DIR}/cmake/configure_files/datacrumbs_config.h.in"
    "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/datacrumbs_config.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/plugins/custom_probes/custom_probes.h.in"
    "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/custom_probes.h" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/plugins/custom_probes/custom_probes_process.h.in"
    "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/custom_probes_process.h" @ONLY)

# ------------------------------------------------------------------------------
# Make output directories
# ------------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/process/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/bpf/objects/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/server/bpf/src/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/objects/)

# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Find all dependencies
# ------------------------------------------------------------------------------
include_dependencies()

ExternalProject_Add(
  datacrumbs_utils_external
  PREFIX "${CMAKE_BINARY_DIR}/_deps/datacrumbs-utils"
  GIT_REPOSITORY "${DATACRUMBS_UTILS_GIT_REPOSITORY}"
  GIT_TAG "${DATACRUMBS_UTILS_GIT_REF}"
  GIT_SHALLOW FALSE
  GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
  GIT_SUBMODULES ""
  GIT_SUBMODULES_RECURSE FALSE
  UPDATE_DISCONNECTED FALSE
  SOURCE_DIR "${DATACRUMBS_UTILS_SOURCE_DIR}"
  BINARY_DIR "${DATACRUMBS_UTILS_BUILD_DIR}"
  INSTALL_DIR "${DATACRUMBS_UTILS_INSTALL_PREFIX}"
  CMAKE_ARGS
    "-DCMAKE_INSTALL_PREFIX=${DATACRUMBS_UTILS_INSTALL_PREFIX}"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/bin"
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR}"
    "-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR}"
    "-DCMAKE_INCLUDE_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/include"
    "-DCMAKE_CONFIG_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/configs"
    "-DCMAKE_DATA_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/data"
    "-DCMAKE_LIBEXEC_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/libexec/${PROJECT_NAME}"
    "-DCMAKE_ETC_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/etc/${PROJECT_NAME}"
    "-DCMAKE_MODULES_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/etc/${PROJECT_NAME}/modulefiles"
    "-DCMAKE_FLUX_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/etc/${PROJECT_NAME}/flux"
    "-DCMAKE_SYSTEMD_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/etc/${PROJECT_NAME}/systemd"
    "-DDATACRUMBS_PROJECT_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DDATACRUMBS_UTILS_PROJECT_PATH=<SOURCE_DIR>"
    "-DDATACRUMBS_BUILD_SYSTEM_CONFIGURATION_FILE=${DATACRUMBS_UTILS_SYSTEM_CONFIGURATION_FILE}"
    "-DDATACRUMBS_INSTALL_DIR=${CMAKE_INSTALL_PREFIX}"
    "-DDATACRUMBS_HOST=${DATACRUMBS_HOST}"
    "-DDATACRUMBS_USER=${DATACRUMBS_USER}"
    "-DDATACRUMBS_INSTALL_USER=${DATACRUMBS_INSTALL_USER}"
    "-DDATACRUMBS_INCLUSION_PATH=${DATACRUMBS_INCLUSION_PATH}"
    "-DDATACRUMBS_CONFIGURED_TRACE_DIR=${DATACRUMBS_CONFIGURED_TRACE_DIR}"
    "-DDATACRUMBS_CONFIGURED_LOG_DIR=${DATACRUMBS_CONFIGURED_LOG_DIR}"
    "-DDATACRUMBS_CONFIGURED_RUN_DIR=${DATACRUMBS_CONFIGURED_RUN_DIR}"
    "-DDATACRUMBS_SCHEDULER_TYPE=${DATACRUMBS_SCHEDULER_TYPE}"
    "-DDATACRUMBS_SCHEDULER_JOBID_ENV_VAR=${DATACRUMBS_SCHEDULER_JOBID_ENV_VAR}"
    "-DDATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS=${DATACRUMBS_SCHEDULER_RUN_EXTRA_ARGS}"
    "-DDATACRUMBS_LOG_LEVEL_STR=${DATACRUMBS_LOG_LEVEL_STR}"
    "-DDATACRUMBS_MODE_STR=${DATACRUMBS_MODE_STR}"
    "-DDATACRUMBS_ENABLE_OPT=${DATACRUMBS_ENABLE_OPT}"
    "-DDATACRUMBS_TRACE_ALL_PROCESSES_OPT=${DATACRUMBS_TRACE_ALL_PROCESSES_OPT}"
    "-DDATACRUMBS_BPF_PRINT_ENABLE=${DATACRUMBS_BPF_PRINT_ENABLE}"
    "-DDATACRUMBS_DISABLE_PROBE_SIGNING=${DATACRUMBS_DISABLE_PROBE_SIGNING}"
    "-DDATACRUMBS_KERNEL_HEADERS_PATH=${DATACRUMBS_KERNEL_HEADERS_PATH}"
    "-DDATACRUMBS_KERNEL_VERSION=${DATACRUMBS_KERNEL_VERSION}"
  BUILD_COMMAND
    "${CMAKE_COMMAND}" --build "<BINARY_DIR>" --clean-first
  INSTALL_COMMAND
    "${CMAKE_COMMAND}" --build "<BINARY_DIR>" --target install
  USES_TERMINAL_DOWNLOAD TRUE
  USES_TERMINAL_UPDATE TRUE
  USES_TERMINAL_BUILD TRUE
)

add_custom_target(datacrumbs_utils_install DEPENDS datacrumbs_utils_external)

# print_all_variables()
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# Sources for Datacrumbs Components
# ------------------------------------------------------------------------------
set(DATACRUMBS_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/data_structures.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/enumerations.h
)

set(DATACRUMBS_SYSTEM_CONFIGURATOR_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/system_configurator/system_configurator_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/system_configurator/system_configurator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/system_configurator/system_configurator.h
)
set(DATACRUMBS_SIGN_PROBES_SERVICE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/signer/sign_probes_service_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/signer/sign_probes_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/signer/sign_probes_service.h
)

add_executable(datacrumbs_system_configurator ${DATACRUMBS_SYSTEM_CONFIGURATOR_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(datacrumbs_system_configurator ${DEPENDENCY_LIB})
install(TARGETS datacrumbs_system_configurator
    RUNTIME DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
)

add_custom_target(
    generate_utils_system_configuration
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/configs
    COMMAND
      ${CMAKE_COMMAND} -E env
      DATACRUMBS_SYSTEM_PROBE_FILE_OVERRIDE=${DATACRUMBS_UTILS_SYSTEM_CONFIGURATION_FILE}
      $<TARGET_FILE:datacrumbs_system_configurator>
    DEPENDS datacrumbs_system_configurator
    COMMENT "Generating build-tree system configuration for datacrumbs-utils"
)
add_dependencies(datacrumbs_utils_external generate_utils_system_configuration)

add_executable(datacrumbs_sign_probes ${DATACRUMBS_SIGN_PROBES_SERVICE_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(datacrumbs_sign_probes ${DEPENDENCY_LIB})
add_dependencies(datacrumbs_sign_probes datacrumbs_utils_install run_system_configurator)
install(TARGETS datacrumbs_sign_probes
    RUNTIME DESTINATION "${DATACRUMBS_INSTALL_BINARYDIR}"
)

add_custom_target(
    run_system_configurator ALL
    COMMAND
    ${CMAKE_COMMAND}
    -DDATACRUMBS_SYSTEM_PROBE_FILE=${DATACRUMBS_SYSTEM_PROBE_FILE}
    -DDATACRUMBS_PROBE_SECRET_FILE=${DATACRUMBS_PROBE_SECRET_FILE}
    -DDATACRUMBS_SYSTEM_CONFIGURATOR_BIN=$<TARGET_FILE:datacrumbs_system_configurator>
    -P
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_system_configurator_if_missing.cmake
    DEPENDS datacrumbs_system_configurator
    COMMENT "Generating install-time system configuration and probe secret"
)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/plugins/custom_probes
)

set(DATACRUMBS_MULTIARCH_INCLUDE_CANDIDATES
    /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}
    /usr/include/x86_64-linux-gnu
    /usr/include/aarch64-linux-gnu
)
foreach(path IN LISTS DATACRUMBS_MULTIARCH_INCLUDE_CANDIDATES)
    if(EXISTS "${path}/asm/errno.h")
        list(APPEND DATACRUMBS_BPF_INCLUDE_PATHS ${path})
        break()
    endif()
endforeach()

set(DATACRUMBS_BPF_INCLUDE_FLAGS "")

file(
    GLOB_RECURSE DATACRUMBS_CUSTOM_BPF_PLUGIN_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/plugins/custom_probes/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/plugins/custom_probes/*.c"
)

foreach(path IN LISTS DATACRUMBS_BPF_INCLUDE_PATHS)
    list(APPEND DATACRUMBS_BPF_INCLUDE_FLAGS -I${path})
endforeach()

list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/common.c)
list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/init.bpf.c)
list(APPEND DATACRUMBS_BPF_EXTRA_SOURCES ${BPF_SOURCE})

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
        DEPENDS ${BPF_SRC_FILE} ${DATACRUMBS_CUSTOM_BPF_PLUGIN_FILES} vmlinux_header
        COMMENT "Compiling BPF object: ${BPF_OBJ_FILE} (timed)"
        VERBATIM
    )
    install(FILES ${BPF_OBJ_FILE}
        DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/objects"
    )
    list(APPEND DATACRUMBS_BPF_OBJECTS ${BPF_OBJ_FILE})
endforeach()

string(REPLACE ";" " " DATACRUMBS_BPF_OBJECTS_STR "${DATACRUMBS_BPF_OBJECTS}")

add_custom_target(datacrumbs_bpf_force_clean
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATACRUMBS_BPF_OBJECTS} ${BPF_OUTPUT} ${BPF_SKEL_OUTPUT}
    COMMENT "Removing stale BPF objects and skeleton before rebuild"
)

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

add_dependencies(datacrumbs_bpf vmlinux_header)
add_dependencies(datacrumbs_bpf datacrumbs_bpf_force_clean)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/runtime_configuration_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/runtime_configuration_manager.h
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
add_dependencies(datacrumbs_obj datacrumbs_utils_install datacrumbs_bpf_skel run_system_configurator)

# Build the final executable by linking server.cpp and the object library
add_executable(${PROJECT_NAME} ${DATACRUMBS_SERVER_SRC})
target_link_libraries(${PROJECT_NAME} datacrumbs_obj ${DEPENDENCY_LIB})
add_dependencies(${PROJECT_NAME} datacrumbs_utils_install datacrumbs_bpf_skel datacrumbs_obj run_system_configurator)

# Install the main processing executable into DATACRUMBS_INSTALL_BINARYDIR
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${DATACRUMBS_INSTALL_SBINARYDIR}"
)

# Install the main processing datacrumbs object into DATACRUMBS_INSTALL_LIBEXEC
install(FILES ${BPF_OUTPUT}
    DESTINATION "${DATACRUMBS_INSTALL_LIBEXEC}/objects"
)

# ------------------------------------------------------------------------------
# Custom executables for Datacrumbs
# ------------------------------------------------------------------------------
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E rm -rf
    ${BPF_OUTPUT}
    ${BPF_SKEL_OUTPUT}
    ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs
    COMMENT "Cleaning BPF build artifacts (ignoring if cannot remove)"
)

# flux cli plugin file
configure_file("${CMAKE_SOURCE_DIR}/etc/datacrumbs/flux/cli/plugins/datacrumbs.py"
    "${CMAKE_FLUX_OUTPUT_DIRECTORY}/cli/plugins/datacrumbs.py" @ONLY)

install(FILES "${CMAKE_FLUX_OUTPUT_DIRECTORY}/cli/plugins/datacrumbs.py"
    DESTINATION "${DATACRUMBS_INSTALL_ETC_FLUX}/cli/plugins"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/systemd/datacrumbs@.service.in ${CMAKE_SYSTEMD_OUTPUT_DIRECTORY}/datacrumbs@.service @ONLY)
install(
    PROGRAMS
    ${CMAKE_SYSTEMD_OUTPUT_DIRECTORY}/datacrumbs@.service
    DESTINATION
    ${DATACRUMBS_INSTALL_ETC_SYSTEMD}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/systemd/datacrumbs_sign_probes.service.in ${CMAKE_SYSTEMD_OUTPUT_DIRECTORY}/datacrumbs_sign_probes.service @ONLY)
install(
    PROGRAMS
    ${CMAKE_SYSTEMD_OUTPUT_DIRECTORY}/datacrumbs_sign_probes.service
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

# Create an empty directory at install time
install(CODE "file(MAKE_DIRECTORY \"\${DATACRUMBS_INSTALL_LIBEXEC}/sbin\")")

add_test(NAME test_simple_dd
    COMMAND bash -c "dd if=/dev/zero of=/tmp/img_temp.bat bs=1M count=16"
)
