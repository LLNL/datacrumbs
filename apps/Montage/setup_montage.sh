#! /usr/bin/env bash

set -eu
CWD=$(pwd)

RUNMAKE=${1:-false}
CLONEROOT=${2:-"$CWD"}
GITTAG=${3:-"v6.1"}
GITOUT=${4:-"Montage-${GITTAG}"}

# NOTE: Change OPENMPIMODULE as needed or change USEMPI to false if you don't want to use MPI.
OPENMPIMODULE="openmpi"
USEMPIMODULE=true
USEMPI=true

GITOUTDIR="${CLONEROOT}/${GITOUT}"

if [[ ! -d "$GITOUTDIR" ]]; then
  git clone https://github.com/Caltech-IPAC/Montage.git -b "$GITTAG" "$GITOUTDIR"
fi
cd "${GITOUTDIR}" || {
  echo "Failed to cd into $GITOUTDIR"
  exit 1
}

if [[ ! -f "${GITOUTDIR}"/PATCHED ]]; then
  {
    git apply "$CWD"/gcc-11-montage-"${GITTAG}".patch
    touch "${GITOUTDIR}"/PATCHED
  } ||
    {
      echo 'Patch failed'
      exit 1
    }
else
  echo "Patch already applied, skipping."
fi
#
# changing the MONTAGE_DIR variable to the current directory
cp "${CWD}"/MontageExec.template.sh "${CWD}"/MontageExec
sed -i "s|MONTAGE_DIR=.*|MONTAGE_DIR=${GITOUTDIR}|" "$CWD"/MontageExec

if [[ $USEMPI == true ]]; then
  if [[ $USEMPIMODULE == true ]]; then
    if ! module load "$OPENMPIMODULE"; then
      echo 'MPI module name not found, please check the OPENMPIMODULE variable.'
      exit 1
    fi
  elif ! command -v mpicc &>/dev/null; then
    echo "mpicc not found in system PATH"
    echo "Please load the MPI module or set USEMPI to false."
    exit 1
  fi
else
  echo "Continuing without MPI support."
fi

if [[ $RUNMAKE == true ]]; then
  make clean
  make -j "$(nproc)"
else
  echo "Please change directory to ${GITOUTDIR} and run 'make' to compile Montage."
fi

cd "${CWD}" || exit 1
