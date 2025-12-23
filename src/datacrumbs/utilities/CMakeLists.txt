find_package(yaml-cpp REQUIRED)

set(DATACRUMBS_UTILITIES_CONFIG_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/config/yaml_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config/yaml_query.cpp
)

add_executable(datacrumbs_config_parser ${DATACRUMBS_UTILITIES_CONFIG_SRCS})
target_include_directories(datacrumbs_config_parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_link_libraries(datacrumbs_config_parser PRIVATE yaml-cpp)

install(
    PROGRAMS
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/datacrumbs_config_parser
    DESTINATION
    ${DATACRUMBS_INSTALL_BINARYDIR}
)