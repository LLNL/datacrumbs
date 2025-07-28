#!/bin/bash

# Pass all script arguments to the next command
ARGS="$@"

${DATACRUMBS_DIR}/scripts/chameleon/server/run_tool chameleon $ARGS
