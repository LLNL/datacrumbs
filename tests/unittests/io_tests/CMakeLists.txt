find_package(MPI)

if(MPI_FOUND)
        find_program(MPIRUN_EXECUTABLE mpirun)

        if(MPIRUN_EXECUTABLE)
                message(STATUS "Found mpirun: ${MPIRUN_EXECUTABLE}")
                set(ENABLE_MPIRUN_TEST ON)
        else()
                message(STATUS "mpirun binary not found. mpirun tests will be disabled.")
                set(ENABLE_MPIRUN_TEST OFF)
        endif()

        add_executable(df_tracer_test test.cpp)
        target_link_libraries(df_tracer_test MPI::MPI_CXX)
        add_executable(df_tracer_test_stdio test_stdio.cpp)
        target_link_libraries(df_tracer_test_stdio MPI::MPI_CXX)
endif()