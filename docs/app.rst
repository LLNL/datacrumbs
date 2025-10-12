Install IOR
===========

IOR is a parallel file system benchmarking tool. To install IOR, follow these steps:

1. **Clone the IOR repository:**

    .. code-block:: bash

        git clone https://github.com/hpc/ior.git
        cd ior

2. **Build IOR:**

    .. code-block:: bash

        ./bootstrap
        ./configure --prefix=$PREFIX
        make -j
        make install

3. **(Optional) Install system-wide:**

    .. code-block:: bash

        sudo make install

For more details, refer to the official IOR documentation: https://github.com/hpc/ior