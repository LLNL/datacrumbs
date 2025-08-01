#!/bin/bash

# Pass all script arguments to the next command
ARGS="$@"

${DATACRUMBS_DIR}/scripts/arta_lima/server/run_tool arta_lima $ARGS
