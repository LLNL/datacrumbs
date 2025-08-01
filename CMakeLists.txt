cmake_minimum_required(VERSION 3.4)
project(datacrumbs LANGUAGES C CXX)
string(TOUPPER "${PROJECT_NAME}" UPPER_PROJECT_NAME)
string(TOLOWER "${PROJECT_NAME}" LOWER_PROJECT_NAME)

if(CMAKE_INSTALL_LIBDIR)
    message(STATUS "CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}")
    set(DATACRUMBS_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
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
    set(DATACRUMBS_LIBDIR ${CMAKE_INSTALL_PREFIX}/"lib")
    set(DATACRUMBS_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
    set(DATACRUMBS_INSTALL_DOCDIR "${CMAKE_INSTALL_PREFIX}/doc")
    set(DATACRUMBS_INSTALL_SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc/modulefiles")
    set(DATACRUMBS_INSTALL_BINARYDIR "${CMAKE_INSTALL_PREFIX}/bin")
    set(DATACRUMBS_INSTALL_LIBEXEC ${CMAKE_INSTALL_PREFIX}/libexec)
    message(STATUS "DATACRUMBS_LIBDIR set to ${DATACRUMBS_LIBDIR}")
endif()

if(NOT DATACRUMBS_EXTERNALLY_CONFIGURED)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
    set(CMAKE_INCLUDE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/include CACHE PATH "Store the headers.")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all Libraries")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all static libraries.")
    set(CMAKE_LIBEXEC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libexec)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(DATACRUMBS_CLIENT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/client/library.c ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/client/library.h)

add_library(${PROJECT_NAME}_client SHARED ${DATACRUMBS_CLIENT_SRC})
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
    set(YAML_CPP_LIBRARY_DIR "${YAML_CPP_CMAKE_DIR}/../../")
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} -L${YAML_CPP_LIBRARY_DIR} ${YAML_CPP_LIBRARIES})
else()
    message(FATAL_ERROR "-- [${UPPER_PROJECT_NAME}] yaml-cpp is needed for ${PROJECT_NAME} build")
endif()

find_package(LLVM REQUIRED CONFIG)

if(LLVM_FOUND)
    message(STATUS "[${UPPER_PROJECT_NAME}] found LLVM at ${LLVM_INCLUDE_DIRS}")
    include_directories(${LLVM_INCLUDE_DIRS})
    link_directories(${LLVM_LIBRARY_DIRS})
    set(DEPENDENCY_LIB ${DEPENDENCY_LIB} ${LLVM_LIBRARIES} -lclang)
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

add_executable(${PROJECT_NAME}_explorer ${DATACRUMBS_EXPLORER_SRC} ${DATACRUMBS_COMMON_SRC})
target_link_libraries(${PROJECT_NAME}_explorer ${DEPENDENCY_LIB})
install(
    TARGETS ${PROJECT_NAME}_explorer
    EXPORT ${DATACRUMBS_EXPORTED_TARGETS}
    LIBRARY DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${DATACRUMBS_INSTALL_BIN_DIR}
)


set(DATACRUMBS_BPF_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/init.bpf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/datacrumbs_process_events.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/datacrumbs_process_events.c

)
include_directories(${DATACRUMBS_INSTALL_LIBEXEC})
include_directories(${CMAKE_LIBEXEC_OUTPUT_DIRECTORY})

# Generate vmlinux.h using bpftool before building datacrumbs_bpf
find_program(BPFTool_EXECUTABLE NAMES bpftool REQUIRED)

set(VMLINUX_BTF_PATH "/sys/kernel/btf/vmlinux")
set(VMLINUX_HEADER "${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/bpf/vmlinux.h")

add_custom_command(
    OUTPUT ${VMLINUX_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/bpf
    COMMAND ${BPFTool_EXECUTABLE} btf dump file ${VMLINUX_BTF_PATH} format c > ${VMLINUX_HEADER}
    DEPENDS ${VMLINUX_BTF_PATH}
    COMMENT "Generating vmlinux.h from ${VMLINUX_BTF_PATH}"
    VERBATIM
)

add_custom_target(vmlinux_header ALL DEPENDS ${VMLINUX_HEADER})

find_program(CLANG_EXECUTABLE NAMES clang REQUIRED)

if(NOT CLANG_EXECUTABLE)
    message(FATAL_ERROR "clang compiler not found!")
endif()

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
message(STATUS "DATACRUMBS_ARCH is ${DATACRUMBS_ARCH}")

add_custom_command(
    OUTPUT ${BPF_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bpf
    COMMAND ${CLANG_EXECUTABLE} -target bpf -DDATACRUMBS_BUILD_CLIENT_SO=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdatacrumbs_client.so -DDATACRUMBS_INSTALL_CLIENT_SO=${DATACRUMBS_LIBDIR}/libdatacrumbs_client.so -D__TARGET_ARCH_${DATACRUMBS_ARCH} -I${CMAKE_INCLUDE_OUTPUT_DIRECTORY} -I${CMAKE_LIBEXEC_OUTPUT_DIRECTORY} -I${DATACRUMBS_INSTALL_LIBEXEC} -I${CMAKE_CURRENT_SOURCE_DIR}/include -I${CMAKE_CURRENT_SOURCE_DIR}/src -Wall -g -O2 -c ${BPF_SOURCE} -o ${BPF_OUTPUT}
    DEPENDS ${BPF_SOURCE}
    COMMENT "Compiling BPF program: ${BPF_SOURCE}"
    VERBATIM
)

add_custom_target(datacrumbs_bpf ALL DEPENDS ${BPF_OUTPUT})

add_dependencies(datacrumbs_bpf vmlinux_header)

set(BPF_SKEL_OUTPUT ${CMAKE_BINARY_DIR}/include/datacrumbs/bpf/datacrumbs.skel.h)

add_custom_command(
    OUTPUT ${BPF_SKEL_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/datacrumbs/bpf
    COMMAND ${BPFTool_EXECUTABLE} gen skeleton ${BPF_OUTPUT} > ${BPF_SKEL_OUTPUT}
    DEPENDS ${BPF_OUTPUT}
    COMMENT "Generating BPF skeleton: ${BPF_SKEL_OUTPUT}"
    VERBATIM
)

add_custom_target(datacrumbs_bpf_skel ALL DEPENDS ${BPF_SKEL_OUTPUT})

add_dependencies(datacrumbs_bpf_skel datacrumbs_bpf)


set(DATACRUMBS_PROCESS_EVENTS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/server/bpf/datacrumbs_process_events.c
    ${BPF_SKEL_OUTPUT}
)
include_directories(${CMAKE_INCLUDE_OUTPUT_DIRECTORY})
add_executable(${PROJECT_NAME} ${DATACRUMBS_PROCESS_EVENTS_SRC})

target_link_libraries(${PROJECT_NAME}
    -l:libbpf.so
    -lelf
    -lz
)
add_dependencies(${PROJECT_NAME} datacrumbs_bpf_skel)

add_custom_target(clean_bpf
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        ${BPF_OUTPUT}
        ${BPF_SKEL_OUTPUT}
        ${VMLINUX_HEADER}
        ${CMAKE_BINARY_DIR}/bpf
        ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/datacrumbs/bpf
    COMMENT "Cleaning BPF build artifacts"
)

add_custom_target(extra_clean
    DEPENDS clean_bpf
    COMMENT "Extra clean: cleans BPF and related generated files"
)

set(DATACRUMBS_GENERATOR_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/datacrumbs/generator/generator.cpp
)
add_executable(${PROJECT_NAME}_generator ${DATACRUMBS_GENERATOR_SRC} ${DATACRUMBS_COMMON_SRC})
target_compile_definitions(${PROJECT_NAME}_generator PRIVATE -DDATACRUMBS_SRC_GEN_PATH="${CMAKE_LIBEXEC_OUTPUT_DIRECTORY}")
target_link_libraries(${PROJECT_NAME}_generator ${DEPENDENCY_LIB})
install(
    TARGETS ${PROJECT_NAME}_generator
    EXPORT ${DATACRUMBS_EXPORTED_TARGETS}
    LIBRARY DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${DATACRUMBS_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${DATACRUMBS_INSTALL_BIN_DIR}
)

add_subdirectory(tests)