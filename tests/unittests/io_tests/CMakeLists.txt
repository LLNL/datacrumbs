find_package(MPI)

if(MPI_FOUND)
        add_executable(df_tracer_test test.cpp)
        target_link_libraries(df_tracer_test MPI::MPI_CXX)
        add_executable(df_tracer_test_stdio test_stdio.cpp)
        target_link_libraries(df_tracer_test_stdio MPI::MPI_CXX)
endif()