# Readme for Ares Cluster

## Manage Environments

```bash
export DATACRUMBS_DIR=/mnt/common/$USER/datacrumbs
export DATACRUMBS_INSTALL_DIR=/mnt/common/$USER/datacrumbs/install
```

## Dependencies

Use release version of `bcc` **0.35.0**

Install system dependencies:

```bash
sudo apt install libelf-dev libclang-14 libclang-14-dev zip
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-14 50
```

Ensure `llvm-config` is symlinked using `update-alternatives` like so:

```bash
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-14 50
```

## Installation

```bash
export DATACRUMBS_INSTALL_DIR=/mnt/common/$USER/datacrumbs/install
export DATACRUMBS_DIR=/mnt/common/$USER/datacrumbs
git clone git@github.com:hariharan-devarajan/datacrumbs.git
cd datacrumbs
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} ..
make install -j9
cd -
```

### Install Python Environment

#### Create a python environment

```bash
virtualenv --python=python3 ${DATACRUMBS_INSTALL_DIR}
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
pip install --upgrade pip
pip install -r ${DATACRUMBS_DIR}/requirements.txt
pip install -r ${DATACRUMBS_DIR}/bcc_requirements.txt
```

#### Install `bcc`s python bindings

```bash
cmake -DPYTHON_CMD=python -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} ..
make install -j9
```

You may have to copy the python bindings manually if the above does not work:

```bash
cp -rv bcc-python3/bcc $DATACRUMBS_INSTALL_DIR/lib/python3.10/site-packages/
```

Add this to your `activate` file.
This makes sure when you activate the
environment, the paths are set correctly for `bcc`:

```bash
BCC_INSTALL_DIR="/mnt/repo/software/bcc"
BCC_SITEPKG="$BCC_INSTALL_DIR/lib/python3.10/site-packages/"
export PATH=$PATH:$BCC_INSTALL_DIR/bin:$BCC_INSTALL_DIR/sbin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BCC_INSTALL_DIR/lib:$BCC_INSTALL_DIR/lib64
export PYTHONPATH=$PYTHONPATH:$BCC_SITEPKG
```

Or simply run the following commands to set the environment variables:

```bash
cat << EOF >> ${DATACRUMBS_INSTALL_DIR}/bin/activate
BCC_INSTALL_DIR="/mnt/repo/software/bcc"
BCC_SITEPKG="$BCC_INSTALL_DIR/lib/python3.10/site-packages/"

export PATH=$PATH:$BCC_INSTALL_DIR/bin:$BCC_INSTALL_DIR/sbin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BCC_INSTALL_DIR/lib:$BCC_INSTALL_DIR/lib64
export PYTHONPATH=$PYTHONPATH:$BCC_SITEPKG
EOF
```

## Running the tool

### Setup

```bash
export DATACRUMBS_DIR=/mnt/common/$USER/datacrumbs
source ${DATACRUMBS_DIR}/scripts/ares/server/setup.sh
source ${DATACRUMBS_DIR}/scripts/ares/server/init.sh
source ${DATACRUMBS_DIR}/scripts/ares/server/run.sh
```

### Running Simple Test

```bash
export DATACRUMBS_DIR=/mnt/common/$USER/datacrumbs
export DATACRUMBS_INSTALL_DIR=/mnt/common/$USER/datacrumbs/install
source ${DATACRUMBS_DIR}/scripts/ares/client/setup.sh
source ${DATACRUMBS_DIR}/scripts/ares/client/tests/test_posix.sh
```

## Slurm integration

Slurm job installation will be added here (need to be done by admin)
