#!/bin/bash
GEN_PROBES=true

while getopts "gh" opt; do
    case $opt in
        g)
            GEN_PROBES=true
            ;;
        h)
            echo "Usage: $0 [-g] [-h]"
            echo "  -g    Generate probes"
            echo "  -h    Show this help message"
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
    ${DATACRUMBS_DIR}/scripts/lima/server/initialize_tool lima --generate_probes --log_file "${DATACRUMBS_DIR}/logs/lima-init.log"
else
    ${DATACRUMBS_DIR}/scripts/lima/server/initialize_tool lima --log_file "${DATACRUMBS_DIR}/logs/lima-init.log"
fi
