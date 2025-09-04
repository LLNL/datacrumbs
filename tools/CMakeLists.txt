cmake_minimum_required(VERSION 3.10)
project(aggregate_trace_tool)

find_package(json-c REQUIRED)

add_executable(aggregate_trace aggregate_trace.cpp)

target_link_libraries(aggregate_trace PRIVATE json-c::json-c ZLIB::ZLIB)

add_executable(callgraph_creator callgraph_vis.cpp)
target_link_libraries(callgraph_creator PRIVATE json-c::json-c ZLIB::ZLIB)