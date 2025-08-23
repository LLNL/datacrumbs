# Readme for Quokka

```bash
export DATACRUMBS_DIR=/proj/csc671/proj_shared/datacrumbs
export DATACRUMBS_INSTALL_DIR=/proj/csc671/proj_shared/dc-install
export SPACK_DIR=/proj/csc671/proj_shared/spack
```

## Installation

### Setup spack

```bash
git clone --depth=2 https://github.com/spack/spack.git ${SPACK_DIR}
source ${SPACK_DIR}/share/spack/setup-env.sh
spack repo update builtin
```

### Setup packages.yaml

/ccsopen/home/dhari/.spack/packages.yaml

```yaml
packages:
  gcc:
    externals:
    - spec: gcc@11.5.0 languages:='c,c++,fortran'
      prefix: /usr
      extra_attributes:
        compilers:
          c: /usr/bin/gcc
          cxx: /usr/bin/g++
          fortran: /usr/bin/gfortran
  python:
    externals:
      - spec: python@3.9.21
        prefix: /usr
    buildable: False
  autoconf:
    externals:
      - spec: autoconf@2.69
        prefix: /usr
    buildable: False
  automake:
    externals:
      - spec: automake@1.16.2
        prefix: /usr
    buildable: False
  libtool:
    externals:
      - spec: libtool@2.4.6
        prefix: /usr
    buildable: False
  openssh:
    externals:
      - spec: openssh@8.7p1
        prefix: /usr
    buildable: False
  perl:
    externals:
      - spec: perl@5.32.1
        prefix: /usr
    buildable: False
  bzip2:
    externals:
      - spec: bzip2@1.0.8
        prefix: /usr
    buildable: False
  zlib-ng:
    externals:
      - spec: zlib-ng@1.2.11
        prefix: /usr
    buildable: False
  openssl:
    externals:
      - spec: openssl@3.2.2
        prefix: /usr
    buildable: False
```

### Install Packages

```bash
spack install gcc@11.5.0
spack install python@3.9.21
spack install openmpi@5.0.8
spack install llvm@19.1.7
spack install cmake@3.26.5
spack install hdf5@1.14.5 ior@4.0.0
spack view --verbose symlink ${DATACRUMBS_INSTALL_DIR} gcc@11.5.0 python@3.9.21 openmpi@5.0.8 llvm@19.1.7 cmake@3.26.5 hdf5@1.14.5 ior@4.0.0
```

### Load Packages

```bash
spack load  gcc@11.5.0 python@3.9.21 openmpi@5.0.8 llvm@19.1.7 cmake@3.26.5 hdf5@1.14.5 ior@4.0.0
```

### Create install directory

```bash
python3 -m venv ${DATACRUMBS_INSTALL_DIR}
```

### Install BCC

```bash
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
BCC_HOME=/proj/csc671/proj_shared/bcc
git clone https://github.com/iovisor/bcc.git ${BCC_HOME}
pushd ${BCC_HOME}
mkdir build
pushd build
cmake -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} -DCMAKE_C_COMPILER=`which gcc`  -DCMAKE_CXX_COMPILER=`which g++`  ..
make -j
make install -j
cmake -DPYTHON_CMD=python -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR}  -DCMAKE_C_COMPILER=`which gcc`  -DCMAKE_CXX_COMPILER=`which g++` .. # build python3 binding
pushd src/python/
make
make install
popd
popd
popd
cp -r ${BCC_HOME}/build/src/python/bcc-python3/bcc/* ${DATACRUMBS_INSTALL_DIR}/lib/python3.12/site-packages/bcc/
```

### Install DataCrumbs

```bash
git clone git@github.com:hariharan-devarajan/datacrumbs.git ${DATACRUMBS_DIR}
pushd ${DATACRUMBS_DIR}
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} -DCMAKE_C_COMPILER=`which gcc`  -DCMAKE_CXX_COMPILER=`which g++` ..
make install -j
chmod +x ${DATACRUMBS_INSTALL_DIR}/libexec/datacrumbs/bin/*
```

### Install Python Environment

```bash
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
pip install --upgrade pip
pip install -r ${DATACRUMBS_DIR}/requirements/requirement.quokka.txt
```


## Running the tool

### Starting Server

```bash
export DATACRUMBS_DIR=/proj/csc671/proj_shared/datacrumbs
cd ${DATACRUMBS_DIR}/scripts/quokka/server
source setup.sh
./init.sh -g
./run.sh
```

### Running Simple Test

```bash
export DATACRUMBS_DIR=/proj/csc671/proj_shared/datacrumbs
cd ${DATACRUMBS_DIR}/scripts/quokka/client
source setup.sh
./tests/test_posix.sh
```