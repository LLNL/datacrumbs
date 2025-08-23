# Montage

## Automatic Montage Setup

Run the `setup_montage.sh`

This script will download the appropriate version of Montage **v6.1**,
patch it for newer compilers (gcc 11+), updates the location for Montage in the
`MontageExec` script, load an MPI module, and then build Montage if `true` was passed.

The script may take 3 parameters:

1. `RUNMAKE`: This will determine whether or not to build Montage.
   Values include `true` or `false` (default: `false`)
2. `CLONEROOT`: Where should the script clone Montage to. Default is current working directory.
3. `GITTAG`: The Git tag, defaults to `v6.1`
4. `GITOUT`: The output directory, defaults to `Montage-$GITTAG`.
   If you change `$GITTAG` then `$GITOUT` will also change accordingly.

Montage will hence be cloned to `${CLONEROOT}/${GITOUT}`
