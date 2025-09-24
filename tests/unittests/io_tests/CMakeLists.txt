find_package(MPI REQUIRED)
add_executable(df_tracer_test test.cpp)
target_link_libraries(df_tracer_test MPI::MPI_CXX)

install(
        FILES
        ${EXECUTABLE_OUTPUT_PATH}/df_tracer_test
        DESTINATION
        ${DATACRUMBS_INSTALL_LIBEXEC}/bin
)

add_executable(df_tracer_test_stdio test_stdio.cpp)
target_link_libraries(df_tracer_test_stdio MPI::MPI_CXX)

install(
        FILES
        ${EXECUTABLE_OUTPUT_PATH}/df_tracer_test_stdio
        DESTINATION
        ${DATACRUMBS_INSTALL_LIBEXEC}/bin
)