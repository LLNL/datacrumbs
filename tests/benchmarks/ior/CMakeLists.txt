find_program(IOR_EXECUTABLE NAMES ior PATHS ENV PATH)

if(IOR_EXECUTABLE)
    message(STATUS "Found IOR: ${IOR_EXECUTABLE}")
    set(ENABLE_IOR_TESTS ON)
else()
    message(STATUS "IOR binary not found. IOR tests will be disabled.")
    set(ENABLE_IOR_TESTS OFF)
endif()

set(DATACRUMBS_TEST_DIR "/tmp" CACHE STRING "Directory for Datacrumbs test files")

if(ENABLE_IOR_TESTS AND ENABLE_MPIRUN_TEST)
    add_test(
        NAME ior_posix_file_per_process
        COMMAND ${MPIRUN_EXECUTABLE} -np 16
        -x LD_PRELOAD=${DATACRUMBS_BUILD_CLIENT_SO}
        ${IOR_EXECUTABLE}
        -a POSIX
        -F
        -t 1m
        -b 32m
        -o ${DATACRUMBS_TEST_DIR}/ior_testfile
    )
    add_test(
        NAME ior_mpiio_collective_shared
        COMMAND ${MPIRUN_EXECUTABLE} -np 16
        -x LD_PRELOAD=${DATACRUMBS_BUILD_CLIENT_SO}
        ${IOR_EXECUTABLE}
        -a MPIIO
        -c
        -t 1m
        -b 32m
        -w -r
        -o ${DATACRUMBS_TEST_DIR}/ior_mpiio_testfile
    )
    add_test(
        NAME ior_hdf5_collective_shared
        COMMAND ${MPIRUN_EXECUTABLE} -np 16
        -x LD_PRELOAD=${DATACRUMBS_BUILD_CLIENT_SO}
        ${IOR_EXECUTABLE}
        -a HDF5
        -C
        -t 1m
        -b 32m
        -w -r
        -o ${DATACRUMBS_TEST_DIR}/ior_hdf5_collective_testfile
    )

    add_test(
        NAME ior_hdf5_file_per_process
        COMMAND ${MPIRUN_EXECUTABLE} -np 16
        -x LD_PRELOAD=${DATACRUMBS_BUILD_CLIENT_SO}
        ${IOR_EXECUTABLE}
        -a HDF5
        -F
        -t 1m
        -b 32m
        -w -r
        -o ${DATACRUMBS_TEST_DIR}/ior_hdf5_fpp_testfile
    )
endif()
