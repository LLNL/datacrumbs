#!/bin/bash
GEN_PROBES=false

while getopts "gh" opt; do
    case $opt in
        g)
            GEN_PROBES=true
            ;;
        h)
            echo "Usage: $0 [-g] [-h]"
            echo "  -g    Generate probes"
            echo "  -h    Show this help message"

            ${DATACRUMBS_DIR}/scripts/ares/server/initialize_tool -h
            exit 0
            ;;
        *)
            echo "Invalid option: -$OPTARG" >&2
            echo "Use -h for help."
            exit 1
            ;;
    esac
done

if [ "$GEN_PROBES" = true ]; then
    ${DATACRUMBS_DIR}/scripts/ares/server/initialize_tool ares --generate_probes --log_file "${DATACRUMBS_DIR}/logs/ares-init.log"
else
    ${DATACRUMBS_DIR}/scripts/ares/server/initialize_tool ares --log_file "${DATACRUMBS_DIR}/logs/ares-init.log"
fi
