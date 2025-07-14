#! /usr/bin/env bash

set -eu
CWD=$(pwd)

RUNMAKE=${1:-false}
CLONEROOT=${2:-"$CWD"}
GITTAG=${3:-"v6.1"}
GITOUT=${4:-"Montage-${GITTAG}"}

# NOTE: Change OPENMPIMODULE as needed
OPENMPIMODULE="openmpi"

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

module load $OPENMPIMODULE || {
  echo 'MPI module not found'
  exit 1
}

if [[ $RUNMAKE == true ]]; then
  make clean
  make -j "$(nproc)"
else
  echo "Please change directory to ${GITOUTDIR} and run 'make' to compile Montage."
fi

cd "${CWD}" || exit 1
