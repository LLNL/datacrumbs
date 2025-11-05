.. _build_instructions:

Build Instructions
==================

This guide describes how to build and install all dependencies required for `datacrumbs`, including `bpftool` (tag v7.5.0), with all components installed under a custom prefix directory.

Prerequisites
-------------

- Git
- CMake
- GCC or Clang
- Python (for Sphinx documentation)
- Spack (recommended for dependency management)

Set the installation prefix:

.. code-block:: bash

    export PREFIX=/your/custom/prefix

1. Build and Install Dependencies for datacrumbs
-----------------------------------------------

The following dependencies are required for building `datacrumbs`:

- bpftool (v7.5.0) and libbpf (v1.5.0)
- json-c
- yaml-cpp
- llvm
- (Optional) bpftime for user-space

**bpftool (v7.5.0) and libbpf (v1.5.0):**

.. code-block:: bash

    git clone https://github.com/libbpf/bpftool.git
    pushd bpftool
    git checkout tags/v7.5.0 -b v7.5.0
    git submodule update --init --recursive
    pushd libbpf
    git checkout tags/v1.5.0 -b v1.5.0
    cd src
    DESTDIR=$PREFIX make install -j
    popd
    cd src
    DESTDIR=$PREFIX make install -j
    popd
    pushd $PREFIX
    bpf_header=$(find . -name bpf.h | head -n 1)
    bpf_header=$(readlink -f $bpf_header)
    bpf_install_dir=$(dirname $(dirname $(dirname $bpf_header)))
    if [ "$bpf_install_dir" != "$PREFIX" ]; then
        mv $bpf_install_dir/include $PREFIX
        mv $bpf_install_dir/lib* $PREFIX
    fi

    bpftool=$(find . -name bpftool | head -n 1)
    bpftool=$(readlink -f $bpftool)
    bpftool_install_dir=$(dirname $(dirname $bpftool))
    if [ "$bpftool_install_dir" != "$PREFIX" ]; then  
        mv $bpftool_install_dir/* $PREFIX
    fi
    popd
    echo "Checking installed files under \$PREFIX:"
    echo "bpf.h:"
    find $PREFIX -name bpf.h

    echo "libbpf.so:"
    find $PREFIX -name libbpf.so

    echo "libbpf.pc:"
    find $PREFIX -name libbpf.pc

    echo "bpftool:"
    find $PREFIX -name bpftool

Expected Output (with PREFIX=/home/haridev/temp/install)
--------------------------------------------------------

After running the above commands, you should see output similar to:

bpf.h:
    /home/haridev/temp/install/include/bpf/bpf.h

libbpf.so:
    /home/haridev/temp/install/lib/libbpf.so

libbpf.pc:
    /home/haridev/temp/install/lib/pkgconfig/libbpf.pc

bpftool:
    /home/haridev/temp/install/sbin/bpftool

    This confirms that bpftool and libbpf have been installed under your custom prefix directory.

**Clone datacrumbs:**  

.. code-block:: bash

    git clone https://github.com/eunomia-bpf/datacrumbs.git
    export DATACRUMBS_DIR=$(realpath datacrumbs)

**(Optional) bpftime for User-space:**

.. code-block:: bash

    git clone https://github.com/eunomia-bpf/bpftime.git
    pushd bpftime
    git checkout tags/v0.2.0 -b v0.2.0
    git apply $DATACRUMBS_DIR/docs/patch/bpftime-v0.2.0.patch
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
    make
    make install
    popd



1. Install Remaining Dependencies: json-c, yaml-cpp, llvm
---------------------------------------------------------

**Recommended: Use Spack**

.. code-block:: bash

    git clone https://github.com/spack/spack.git
    . spack/share/spack/setup-env.sh
    spack install json-c cppyaml llvm

**If Spack is not available:**

- **LLVM:** Install via your package manager

  - Fedora/RHEL:
     .. code-block:: bash

         sudo dnf install llvm-devel

  - Ubuntu/Debian:
     .. code-block:: bash

         sudo apt-get install llvm-dev

- **json-c:** Build from source

  .. code-block:: bash

      git clone https://github.com/json-c/json-c.git
      pushd json-c
      git checkout tags/json-c-0.18-20240915 -b json-c-0.18-20240915    
      mkdir build && cd build
      cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
      make -j
      make install -j
      popd

- **yaml-cpp:** Build from source

  .. code-block:: bash

      git clone https://github.com/jbeder/yaml-cpp.git
      pushd yaml-cpp
      git checkout tags/yaml-cpp-0.7.0   -b yaml-cpp-0.7.0
      mkdir build && cd build
      cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ..
      make -j
      make install -j
      popd

Set all paths

.. code-block:: bash

    export PATH=$PREFIX/bin:$PREFIX/sbin:$PATH
    export LD_LIBRARY_PATH=$PREFIX/lib:$PREFIX/lib64:$LD_LIBRARY_PATH

2. Build and Install datacrumbs
---------------------------------
With all dependencies installed under your custom prefix directory, you can now build and install `datacrumbs`:

Create a probe YAML for the system:

.. code-block:: bash

    cp $DATACRUMBS_DIR/docs/example/example.yaml $DATACRUMBS_DIR/etc/datacrumbs/configs/probe.yaml
    # Edit the probe.yaml as needed

Set CMake arguments:

.. code-block:: bash

    cmake_args=(
        -DCMAKE_PREFIX_PATH=$PREFIX
        -DCMAKE_INSTALL_PREFIX=$PREFIX
        -DBPFTOOL_EXECUTABLE=$PREFIX/sbin/bpftool
        -DDATACRUMBS_HOST=$(hostname)
        -DDATACRUMBS_USER=${USER}
    )

If you want to use a custom host name or user, set them explicitly:

.. code-block:: bash

    # cmake_args+=(-DDATACRUMBS_HOST=<YOUR_HOST_NAME>)
    # cmake_args+=(-DDATACRUMBS_USER=<TARGET_USER>)

Build and install datacrumbs:

.. code-block:: bash

    export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:$PREFIX/lib64/pkgconfig:$PKG_CONFIG_PATH
    pushd $DATACRUMBS_DIR
    mkdir -p build && cd build
    cmake "${cmake_args[@]}" ..
    make -j
    popd
