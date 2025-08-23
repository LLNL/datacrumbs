#!/usr/bin/bash
# NOTE: Leave this as is to stop for errors and unset variables
set -eu

# NOTE: This dataset name should be the name of the folder in the ${INPUT_DATA_DIR}, change $OUTPUT_DATA_DIR as needed
export DATASET=M16_6x6
export OUTPUT_DATA_DIR=/mnt/nvme/$USER/Montage/output

export MONTAGE_DIR=$HOME/sources/Montage
export INPUT_DATA_DIR=$HOME/data/Montage/
export PATH=${MONTAGE_DIR}/bin:$PATH
export LD_LIBRARY_PATH=${MONTAGE_DIR}/lib:$LD_LIBRARY_PATH

mkdir -p "$OUTPUT_DATA_DIR"

export OMP_NUM_THREADS=1

#NONMPIRUN="srun -n1 "
#NONMPIRUN="srun --ntasks-per-node=1 -N 1 -w ares-comp-31  "
NONMPIRUN="mpirun -n 1 "

# Needed to run MPI version programs
MPIRUN="mpirun -n 16 "
#MPIRUN="srun --ntasks-per-node=16 -N 1 -w ares-comp-31  "

# NOTE: This is to get a log of the commands being run
set -x

function run_montage_workflow {
  rm -f "$OUTPUT_DATA_DIR"/*.fits "$OUTPUT_DATA_DIR"/*.hdr "$OUTPUT_DATA_DIR"/*.tbl "$OUTPUT_DATA_DIR"/*.png
  rm -rf "$OUTPUT_DATA_DIR"/M16_*dir

  mkdir "$OUTPUT_DATA_DIR"/M16_projdir "$OUTPUT_DATA_DIR"/M16_diffdir "$OUTPUT_DATA_DIR"/M16_corrdir
  echo "Create a metadata table of the input images, Kimages.tbl"
  $NONMPIRUN mImgtbl "$INPUT_DATA_DIR"/$DATASET "$OUTPUT_DATA_DIR"/m16.tbl

  echo "Create a FITS header describing the footprint of the mosaic"
  $NONMPIRUN mMakeHdr "$OUTPUT_DATA_DIR"/m16.tbl "$OUTPUT_DATA_DIR"/m16.hdr

  echo "Reproject the input images"
  $MPIRUN mProjExecMPI -p "$INPUT_DATA_DIR"/$DATASET "$OUTPUT_DATA_DIR"/m16.tbl "$OUTPUT_DATA_DIR"/m16.hdr "$OUTPUT_DATA_DIR"/M16_projdir "$OUTPUT_DATA_DIR"/m16_stats.tbl

  echo "Create a metadata table of the reprojected images"
  $NONMPIRUN mImgtbl "$OUTPUT_DATA_DIR"/M16_projdir "$OUTPUT_DATA_DIR"/m16_proj.tbl

  echo "Analyze the overlaps between images"
  $NONMPIRUN mOverlaps "$OUTPUT_DATA_DIR"/m16_proj.tbl "$OUTPUT_DATA_DIR"/m16_diffs.tbl

  $MPIRUN mDiffExecMPI -p "$OUTPUT_DATA_DIR"/M16_projdir "$OUTPUT_DATA_DIR"/m16_diffs.tbl "$OUTPUT_DATA_DIR"/m16.hdr "$OUTPUT_DATA_DIR"/M16_diffdir
  $MPIRUN mFitExecMPI "$OUTPUT_DATA_DIR"/m16_diffs.tbl "$OUTPUT_DATA_DIR"/m16_fits.tbl "$OUTPUT_DATA_DIR"/M16_diffdir

  echo "Perform background modeling and compute corrections for each image"
  $NONMPIRUN mBgModel "$OUTPUT_DATA_DIR"/m16_proj.tbl "$OUTPUT_DATA_DIR"/m16_fits.tbl "$OUTPUT_DATA_DIR"/m16_corrections.tbl

  echo "Apply corrections to each image"
  # This is a time-comsuming step and is not parallelized
  $NONMPIRUN mBgExec -p "$OUTPUT_DATA_DIR"/M16_projdir/ "$OUTPUT_DATA_DIR"/m16_proj.tbl "$OUTPUT_DATA_DIR"/m16_corrections.tbl "$OUTPUT_DATA_DIR"/M16_corrdir

  echo "Coadd the images to create a mosaic with background corrections"
  $MPIRUN mAddMPI -p "$OUTPUT_DATA_DIR"/M16_corrdir/ "$OUTPUT_DATA_DIR"/m16_proj.tbl "$OUTPUT_DATA_DIR"/m16.hdr "$OUTPUT_DATA_DIR"/m16.fits

  echo "Make a PNG of the corrected mosaic for visualization"
  $NONMPIRUN mViewer -ct 1 -gray "$OUTPUT_DATA_DIR"/m16.fits -1s max gaussian-log -out "$OUTPUT_DATA_DIR"/m16.png
}

run_montage_workflow
