cmake_minimum_required(VERSION 3.4)
project(datacrumbs LANGUAGES C CXX)
string(TOUPPER "${PROJECT_NAME}" UPPER_PROJECT_NAME)
string(TOLOWER "${PROJECT_NAME}" LOWER_PROJECT_NAME)

if(CMAKE_INSTALL_LIBDIR)
    message(STATUS "CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}")
    set(DATACRUMBS_LIBDIR ${CMAKE_INSTALL_LIBDIR})
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
    set(DATACRUMBS_LIBDIR "lib")
    set(DATACRUMBS_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
    set(DATACRUMBS_INSTALL_DOCDIR "${CMAKE_INSTALL_PREFIX}/doc")
    set(DATACRUMBS_INSTALL_SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc/modulefiles")
    set(DATACRUMBS_INSTALL_BINARYDIR "${CMAKE_INSTALL_PREFIX}/bin")
    message(STATUS "DATACRUMBS_LIBDIR set to ${DATACRUMBS_LIBDIR}")
    set(DATACRUMBS_INSTALL_LIBEXEC
        ${CMAKE_INSTALL_PREFIX}/libexec/${PROJECT_NAME})
endif()

if(NOT DATACRUMBS_EXTERNALLY_CONFIGURED)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
    set(CMAKE_INCLUDE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/include CACHE PATH "Store the headers.")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all Libraries")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DATACRUMBS_LIBDIR} CACHE PATH "Single Directory for all static libraries.")
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
add_subdirectory(tests)