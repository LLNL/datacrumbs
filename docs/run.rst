Run Datacrumbs
==============

To run Datacrumbs tests, follow these instructions:

1. **Navigate to the build directory:**

    .. code-block:: bash

        cd $DATACRUMBS_DIR/build

2. **List all available tests:**

    .. code-block:: bash

        ctest -N

3. **View arguments for the Datacrumbs start test which is Daemonize script:**

    .. code-block:: bash

        ctest -R datacrumbs_start -VV

    This will show verbose output and the arguments used for the `datacrumbs_start` test.

4. **View arguments for the Datacrumbs run test which is sync script:**

    .. code-block:: bash

        ctest -R datacrumbs_run -VV

    This will show verbose output and the arguments used for the `datacrumbs_run` test.

Refer to your project's `ctest` configuration for additional test options.

5. **Run the `test_simple_dd` test:**

    .. code-block:: bash

        ctest -R test_simple_dd -VV

    This will execute the `test_simple_dd` test with verbose output from the build directory.