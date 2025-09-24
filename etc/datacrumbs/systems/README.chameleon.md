# Readme for Chameleon

## Account

Go to [chameleon cloud website](https://chameleoncloud.org) and create an account. 
Once the account is created, get added to project.

## Creating an instance

- Navigate to [chameleon cloud website](https://chameleoncloud.org)
- Go to Experiment -> CHI@TACC
- Go to Compute -> Key Pairs from the left hand side bar and create a key pair.
- Then go to reservation, click on create-reservation, fill details, and allocate compute_cascadelake nodes from the Hosts section.
- Then go to compute instances and spawn your instance using Launch Instance. Fill the details and make sure,
  - to select the HARI-UBUNTU-22.04.04-BCC image as the base image
  - select your key pair.


## Setting up dependencies for DataCrumbs

### Clone the repository

```bash
export SPACK_DIR=/opt/spack
export DATACRUMBS_DIR=/home/cc/datacrumbs
export DATACRUMBS_INSTALL_DIR=/home/cc/datacrumbs/install

git clone git@github.com:hariharan-devarajan/datacrumbs.git ${DATACRUMBS_DIR}
```

### Setup spack

This step will clone latest spack into `$SPACK_DIR` and then install hdf5 dependency

```bash
cd ${DATACRUMBS_DIR}/scripts/chameleon
./install_env.sh
cd -
```

### Setting up DataCrumbs Python Dependencies

```bash
python3 -m venv --system-site-packages ${DATACRUMBS_INSTALL_DIR}
source ${DATACRUMBS_INSTALL_DIR}/bin/activate
pip install -r ${DATACRUMBS_DIR}/requirements/requirement.chameleon.txt
```

### Setting up DataCrumbs

```bash
pushd ${DATACRUMBS_DIR}
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${DATACRUMBS_INSTALL_DIR} ..
make install -j
popd
```
## Running the tool


### Generating Probes

check your `${DATACRUMBS_DIR}/datacrumbs/configs/module/chameleon.yaml` and make neccessary changes.

```bash
sudo su
export DATACRUMBS_DIR=/home/cc/datacrumbs
source ${DATACRUMBS_DIR}/scripts/chameleon/server/setup.sh
. ${DATACRUMBS_DIR}/scripts/chameleon/server/init.sh -g
```

### Starting Server

```bash
export DATACRUMBS_DIR=/home/cc/datacrumbs
source ${DATACRUMBS_DIR}/scripts/chameleon/server/setup.sh
. ${DATACRUMBS_DIR}/scripts/chameleon/server/run.sh
```

### Running Simple Test

```bash
export DATACRUMBS_DIR=/home/cc/datacrumbs
source ${DATACRUMBS_DIR}/scripts/chameleon/client/setup.sh
. ${DATACRUMBS_DIR}/scripts/chameleon/client/tests/test_posix.sh
```