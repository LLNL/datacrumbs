cmake_minimum_required(VERSION 3.4)
project(datacrumbs LANGUAGES C CXX)
string(TOUPPER "${PROJECT_NAME}" UPPER_PROJECT_NAME)
string(TOLOWER "${PROJECT_NAME}" LOWER_PROJECT_NAME)

if(CMAKE_INSTALL_LIBDIR)
    message(STATUS "CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}")
    set(DATACRUMBS_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
    set(DATACRUMBS_INSTALL_INCLUDE_DIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
    set(DATACRUMBS_INSTALL_DOCDIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR})
    set(DATACRUMBS_INSTALL_SYSCONFDIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}/modulefiles)
    set(DATACRUMBS_INSTALL_BINFDIR
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
    set(DATACRUMBS_INSTALL_LIBEXEC
        ${CMAKE_INSTALL_PREFIX}/libexec/${PROJECT_NAME})
else()
    set(DATACRUMBS_LIBDIR lib)
    set(DATACRUMBS_INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    set(DATACRUMBS_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
    set(DATACRUMBS_INSTALL_DOCDIR "${CMAKE_INSTALL_PREFIX}/doc")
    set(DATACRUMBS_INSTALL_SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc/modulefiles")
    set(DATACRUMBS_INSTALL_BINARYDIR "${CMAKE_INSTALL_PREFIX}/bin")
    set(DATACRUMBS_INSTALL_LIBEXEC ${CMAKE_INSTALL_PREFIX}/libexec)
    message(STATUS "[${UPPER_PROJECT_NAME}] DATACRUMBS_LIBDIR set to ${DATACRUMBS_LIBDIR}")
endif()

if(NOT DATACRUMBS_EXTERNALLY_CONFIGURED)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
    set(CMAKE_INCLUDE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/include CACHE PATH "Store the headers.")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all Libraries")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all static libraries.")
    set(CMAKE_LIBEXEC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libexec)
endif()

function(print_all_variables)
    message(STATUS "CMake Variables:")
    get_cmake_property(_variableNames VARIABLES)
    list(SORT _variableNames)

    foreach(_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

option(DATACRUMBS_SKIP_PROBE_EXPLORING "Skip building the probe explorer executable" OFF)
option(DATACRUMBS_SKIP_PROBE_GENERATION "Skip building the probe generator executable" ON)
option(BPFTOOL_EXECUTABLE "Path to bpftool executable" "")

if(BPFTOOL_EXECUTABLE STREQUAL "OFF")
    set(BPFTOOL_EXECUTABLE "")
endif()

option(DATACRUMBS_LOG_LEVEL "Set log level (0=PRINT, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE)" 3)

if(DATACRUMBS_LOG_LEVEL STREQUAL "OFF")
    set(DATACRUMBS_LOG_LEVEL 3)
endif()

message(STATUS "[${UPPER_PROJECT_NAME}] DATACRUMBS_LOG_LEVEL is ${DATACRUMBS_LOG_LEVEL}")

# Optionally allow user to specify host name via CMake option
option(DATACRUMBS_HOST "Host name for datacrumbs (numbers will be stripped if not set)" "")

if(NOT DATACRUMBS_SKIP_PROBE_EXPLORING)
    set(ENABLE_PROBE_EXPLORER ON)
else()
    set(ENABLE_PROBE_EXPLORER OFF)
endif()

if(NOT DATACRUMBS_SKIP_PROBE_GENERATION)
    set(ENABLE_PROBE_GENERATOR ON)
else()
    set(ENABLE_PROBE_GENERATOR OFF)
endif()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR}/include)

find_library(LIBC_SO NAMES libc.so.6 libc.so PATHS /lib /usr/lib /lib64 /usr/lib64 /usr/local/lib /usr/local/lib64 /usr/lib/x86_64-linux-gnu NO_DEFAULT_PATH)

if(LIBC_SO)
    message(STATUS "[${UPPER_PROJECT_NAME}] Found libc: ${LIBC_SO}")
else()
    message(WARNING "[${UPPER_PROJECT_NAME}] libc.so not found!")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBPF libbpf)

if(LIBBPF_VERSION VERSION_LESS "1.0.0")
    message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] libbpf version 1.0.0 or newer is required, but found ${LIBBPF_VERSION}")
endif()

# print_all_variables()
if(LIBBPF_FOUND)
    set(LIBBPF_VERSION "${LIBBPF_VERSION}")
    message(STATUS "[${UPPER_PROJECT_NAME}] Found libbpf version: ${LIBBPF_VERSION}")
    include_directories(${LIBBPF_INCLUDEDIR})
    link_directories(${LIBBPF_LIBDIR})
else()
    message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] libbpf not found!")
endif()


# Ensure the process output directory exists
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs/server/process/)
file(MAKE_DIRECTORY ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs/server/bpf/)

set(configuration_variables --user $ENV{USER} --config_path ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/configs --data_dir ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/data --trace_log_dir ${CMAKE_CURRENT_SOURCE_DIR}/etc/datacrumbs/logs)

set(DATACRUMBS_CLIENT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/client/library.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/client/library.h)

add_library(${PROJECT_NAME}_client SHARED ${DATACRUMBS_CLIENT_SRC})
add_dependencies(${PROJECT_NAME}_client clean_bpf)
install(
    TARGETS ${PROJECT_NAME}_client
    EXPORT ${DATACRUMBS_EXPORTED_TARGETS}
    LIBRARY DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${DATACRUMBS_INSTALL_BIN_DIR}
)

set(DATACRUMBS_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/configuration_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/data_structures.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/common/enumerations.h
)

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
set(DEPENDENCY_LIB ${CMAKE_EXE_LINKER_FLAGS} $ENV{LDFLAGS})
find_package(yaml-cpp REQUIRED)

if(${yaml-cpp_FOUND})
    message(STATUS "[${UPPER_PROJECT_NAME}] found yaml-cpp at ${YAML_CPP_INCLUDE_DIR}")
    include_directories(${YAML_CPP_INCLUDE_DIR})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} yaml-cpp)
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] yaml-cpp is needed for ${PROJECT_NAME} build")
endif()

find_package(LLVM REQUIRED CONFIG COMPONENTS Clang)

if(LLVM_FOUND)
    message(STATUS "[${UPPER_PROJECT_NAME}] found LLVM at ${LLVM_INCLUDE_DIRS}")
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
        )
        message(STATUS "[${UPPER_PROJECT_NAME}] Found clang executable: ${CLANG_EXECUTABLE}")
        message(STATUS "[${UPPER_PROJECT_NAME}] clang version: ${CLANG_VERSION_OUTPUT}")
    endif()

else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] LLVM is needed for ${PROJECT_NAME} build")
endif()

find_package(json-c REQUIRED)

if(json-c_FOUND)
    message(STATUS "[${UPPER_PROJECT_NAME}] found json-c at ${json-c_INCLUDE_DIRS}")
    include_directories(json-c::json-c)
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} json-c::json-c)
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] json-c is needed for ${PROJECT_NAME} build")
endif()

find_package(ZLIB REQUIRED)

if(ZLIB_FOUND)
    message(STATUS "[${UPPER_PROJECT_NAME}] found zlib at ${ZLIB_INCLUDE_DIRS}")
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ZLIB::ZLIB)
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] zlib is needed for ${PROJECT_NAME} build")
endif()

# print_all_variables()
add_executable(${PROJECT_NAME}_explorer ${DATACRUMBS_EXPLORER_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(${PROJECT_NAME}_explorer ${DEPENDENCY_LIB})
target_compile_definitions(${PROJECT_NAME}_explorer PRIVATE -DDATACRUMBS_LOG_LEVEL=${DATACRUMBS_LOG_LEVEL})
install(
    TARGETS ${PROJECT_NAME}_explorer
    EXPORT ${DATACRUMBS_EXPORTED_TARGETS}
    LIBRARY DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${DATACRUMBS_INSTALL_BIN_DIR}
)

if(DATACRUMBS_HOST STREQUAL "" OR DATACRUMBS_HOST STREQUAL "OFF")
    # Get the system hostname
    execute_process(
        COMMAND hostname
        OUTPUT_VARIABLE RAW_HOSTNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Remove all numbers from the hostname
    string(REGEX REPLACE "[0-9]" "" DATACRUMBS_HOST "${RAW_HOSTNAME}")
endif()

message(STATUS "[${UPPER_PROJECT_NAME}] DATACRUMBS_HOST is ${DATACRUMBS_HOST}")

add_custom_target(run_explorer
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}_explorer ${DATACRUMBS_HOST} ${configuration_variables}
    DEPENDS ${PROJECT_NAME}_explorer
    COMMENT "Running ${PROJECT_NAME}_explorer with argument '${DATACRUMBS_HOST}'"
)

add_dependencies(run_explorer ${PROJECT_NAME}_client)

set(DATACRUMBS_GENERATOR_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/generator/generator.cpp
)
add_executable(${PROJECT_NAME}_generator ${DATACRUMBS_GENERATOR_SRC} ${DATACRUMBS_COMMON_SRC})
target_compile_definitions(${PROJECT_NAME}_generator PRIVATE -DDATACRUMBS_SRC_GEN_PATH="${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}")
target_compile_definitions(${PROJECT_NAME}_generator PRIVATE -DDATACRUMBS_LOG_LEVEL=${DATACRUMBS_LOG_LEVEL})
target_link_libraries(${PROJECT_NAME}_generator ${DEPENDENCY_LIB})
install(
    TARGETS ${PROJECT_NAME}_generator
    EXPORT ${DATACRUMBS_EXPORTED_TARGETS}
    LIBRARY DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${DATACRUMBS_INSTALL_BIN_DIR}
)

add_custom_target(run_generator
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}_generator ${DATACRUMBS_HOST} ${configuration_variables}
    DEPENDS ${PROJECT_NAME}_generator
    COMMENT "Running ${PROJECT_NAME}_generator with argument '${DATACRUMBS_HOST}'"
)

add_dependencies(${PROJECT_NAME}_generator ${PROJECT_NAME}_explorer)
add_dependencies(run_generator run_explorer)

set(DATACRUMBS_BPF_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/init.bpf.c
)
include_directories(${DATACRUMBS_INSTALL_LIBEXEC})
include_directories(${CMAKE_LIBEXEC_OUTPUT_DIRECTORY})

# Generate vmlinux.h using bpftool before building datacrumbs_bpf
if(BPFTOOL_EXECUTABLE AND NOT BPFTOOL_EXECUTABLE STREQUAL "OFF")
    message(STATUS "[${UPPER_PROJECT_NAME}] Using user-specified bpftool executable: ${BPFTOOL_EXECUTABLE}")
else()
    find_program(BPFTOOL_EXECUTABLE_SEARCH bpftool PATHS /usr/sbin/ /usr/bin/ /usr/local/bin/ /usr/local/sbin/)
    message(STATUS "[${UPPER_PROJECT_NAME}] $ENV{PATH} ${BPFTOOL_EXECUTABLE_SEARCH}")
    set(BPFTOOL_EXECUTABLE ${BPFTOOL_EXECUTABLE_SEARCH})

    if(NOT BPFTOOL_EXECUTABLE)
        message(FATAL_ERROR "[${UPPER_PROJECT_NAME}] bpftool executable not found! Please install bpftool or specify its path via BPFTOOL_EXECUTABLE.")
    endif()

    message(STATUS "[${UPPER_PROJECT_NAME}] Found bpftool executable: ${BPFTOOL_EXECUTABLE}")
endif()

set(VMLINUX_BTF_PATH "/sys/kernel/btf/vmlinux")
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

set(BPF_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/main.bpf.c)
set(BPF_OUTPUT ${CMAKE_BINARY_DIR}/bpf/datacrumbs.bpf.o)

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

message(STATUS "[${UPPER_PROJECT_NAME}] DATACRUMBS_ARCH is ${DATACRUMBS_ARCH}")

add_custom_command(
    OUTPUT ${BPF_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bpf
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Compiling BPF program: ${BPF_SOURCE} (timing start)"
    COMMAND ${CMAKE_COMMAND} -E time ${CLANG_EXECUTABLE} -target bpf -DDATACRUMBS_LOG_LEVEL=${DATACRUMBS_LOG_LEVEL} -DDATACRUMBS_LIBC_SO=${LIBC_SO} -DDATACRUMBS_BUILD_CLIENT_SO=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdatacrumbs_client.so -I${CMAKE_BINARY_DIR}/include -I${LIBBPF_INCLUDEDIR} -DDATACRUMBS_INSTALL_CLIENT_SO=${DATACRUMBS_LIBDIR}/libdatacrumbs_client.so -D__TARGET_ARCH_${DATACRUMBS_ARCH} -I${CMAKE_INCLUDE_OUTPUT_DIRECTORY} -I${CMAKE_LIBEXEC_OUTPUT_DIRECTORY} -I${DATACRUMBS_INSTALL_LIBEXEC} -I${CMAKE_CURRENT_SOURCE_DIR}/include -I${CMAKE_CURRENT_SOURCE_DIR}/src -Wall -g -O2 -c ${BPF_SOURCE} -o ${BPF_OUTPUT}
    DEPENDS ${BPF_SOURCE}
    COMMENT "Compiling BPF program: ${BPF_SOURCE} (timed)"
    VERBATIM
)

add_custom_target(datacrumbs_bpf ALL DEPENDS ${BPF_OUTPUT})

add_dependencies(datacrumbs_bpf vmlinux_header run_generator)

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

set(DATACRUMBS_PROCESS_EVENTS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/event_processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/process/writer/chrome_writer.cpp
    ${BPF_SKEL_OUTPUT}
)
include_directories(${CMAKE_INCLUDE_OUTPUT_DIRECTORY})
add_executable(${PROJECT_NAME} ${DATACRUMBS_PROCESS_EVENTS_SRC} ${DATACRUMBS_COMMON_SRC})
target_compile_definitions(${PROJECT_NAME} PRIVATE -DDATACRUMBS_LOG_LEVEL=${DATACRUMBS_LOG_LEVEL})
target_link_libraries(${PROJECT_NAME} ${DEPENDENCY_LIB} -l:libbpf.so -lelf -lz -lpthread)
add_dependencies(${PROJECT_NAME} datacrumbs_bpf_skel)

add_custom_target(clean_bpf
    COMMAND ${CMAKE_COMMAND} -E rm -rf
    ${BPF_OUTPUT}
    ${BPF_SKEL_OUTPUT}
    ${CMAKE_BINARY_DIR}/bpf
    ${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}/datacrumbs/server/bpf
    COMMENT "Cleaning BPF build artifacts (ignoring if cannot remove)"
)

add_subdirectory(tools)
add_subdirectory(tests)
# add_executable(callgraph_creator
#     tools/callgraph.cpp
#     tools/cJSON.c
# )
# install(
#     TARGETS callgraph_creator
#     RUNTIME DESTINATION ${DATACRUMBS_INSTALL_BINARYDIR}/tools
# )


enable_testing()

add_test(NAME datacrumbs COMMAND ${CMAKE_BINARY_DIR}/bin/datacrumbs ${DATACRUMBS_HOST} ${configuration_variables})
add_test(NAME datacrumbs_pretest COMMAND bash -c "ulimit -n 4096")
set_tests_properties(datacrumbs PROPERTIES DEPENDS datacrumbs_pretest)

add_test(NAME test_simple_dd
    COMMAND bash -c "dd if=/dev/zero of=/tmp/img_temp.bat bs=1M count=16"
)
set_tests_properties(test_simple_dd PROPERTIES ENVIRONMENT "LD_PRELOAD=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdatacrumbs_client.so")