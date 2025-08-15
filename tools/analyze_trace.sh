#!/bin/bash

# Function to echo with timestamp
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"
}

# Check if file is provided as argument
if [ $# -eq 0 ]; then
    log "Usage: $0 <json_file>"
    exit 1
fi

INPUT_FILE="$1"

# Check if file exists
if [ ! -f "$INPUT_FILE" ]; then
    log "Error: File '$INPUT_FILE' not found"
    exit 1
fi

# Create temporary file without brackets
TEMP_FILE=$(mktemp)

log "Removing brackets from input file..."
sed '1d;$d' "$INPUT_FILE" | sed 's/,$//' > "$TEMP_FILE"

# Declare associative arrays for calculations
declare -A total_dur
declare -A count
declare -A category

log "Processing JSON records..."

# Read each line and process with jq
# Count total lines for progress bar
total_lines=$(wc -l < "$TEMP_FILE")
current_line=0

# Use GNU parallel and jq to process lines in parallel and aggregate results
process_line() {
    line="$1"
    name=$(echo "$line" | jq -r '.name // empty')
    cat=$(echo "$line" | jq -r '.cat // empty')
    dur=$(echo "$line" | jq -r '.dur // 0')
    if [[ -n "$name" && "$name" != "null" ]]; then
        echo "$name|$cat|$dur"
    fi
}

export -f process_line

log "Processing JSON records in parallel with progress bar..."

# Function to update progress bar
update_progress() {
    local progress=$1
    local total=$2
    local start_time=$3
    local width=40
    local percent=$((progress * 100 / total))
    local filled=$((progress * width / total))
    local empty=$((width - filled))
    local now elapsed eta

    now=$(date +%s)
    elapsed=$((now - start_time))
    if ((progress > 0)); then
        eta=$(( (elapsed * (total - progress)) / progress ))
    else
        eta=0
    fi

    printf "\r["
    printf "%0.s#" $(seq 1 $filled)
    printf "%0.s-" $(seq 1 $empty)
    printf "] %3d%% (%d/%d) Elapsed: %ds ETA: %ds" "$percent" "$progress" "$total" "$elapsed" "$eta"
}

# Process lines in parallel and show progress bar
rm -f "$TEMP_FILE.results"
processed=0

export -f process_line

# Use a named pipe to avoid race conditions
PIPE=$(mktemp -u)
mkfifo "$PIPE"

# Start a background job to process lines and write to results
(
    cat "$TEMP_FILE" | parallel --pipe --block 1M --jobs "$(nproc)" --line-buffer \
        'while read -r line; do process_line "$line"; done' > "$PIPE"
) &

# In the awk aggregation, add sumsq
awk -F'|' -v total="$total_lines" '
{
    name=$1
    cat=$2
    dur=$3
    total_dur[name] += dur
    count[name] += 1
    sumsq[name] += (dur * dur)
    category[name] = cat
    processed++
    if (processed == 1) {
        start_time = systime()
    }
    if (processed % 100 == 0 || processed == total) {
        now = systime()
        elapsed = now - start_time
        if (processed > 0) {
            eta = (elapsed * (total - processed)) / processed
        } else {
            eta = 0
        }
        printf "\r[" > "/dev/stderr"
        width=40
        filled=int(processed * width / total)
        empty=width - filled
        for (i=0; i<filled; i++) printf "#" > "/dev/stderr"
        for (i=0; i<empty; i++) printf "-" > "/dev/stderr"
        percent=int(processed * 100 / total)
        printf "] %3d%% (%d/%d) Elapsed: %ds ETA: %ds", percent, processed, total, elapsed, eta > "/dev/stderr"
    }
}
END {
    printf "\n" > "/dev/stderr"
    for (name in total_dur) {
        printf "%s|%s|%d|%d|%d\n", name, category[name], count[name], total_dur[name], sumsq[name]
    }
}' "$PIPE" > "$TEMP_FILE.results"

# Read results into associative arrays (add sumsq)
declare -A sumsq
while IFS='|' read -r name cat cnt tdur ssq; do
    total_dur["$name"]=$tdur
    count["$name"]=$cnt
    category["$name"]="$cat"
    sumsq["$name"]=$ssq
done < "$TEMP_FILE.results"

# Prepare output directory and file
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$SCRIPT_DIR/output"
mkdir -p "$OUTPUT_DIR"
INPUT_BASENAME="$(basename "$INPUT_FILE")"
OUTPUT_FILE="$OUTPUT_DIR/${INPUT_BASENAME}.log"

# Output results to both stdout and file
{
    echo
    log "Results:"
    echo "=============================================================================================="
    printf "%-30s %-15s %-10s %-15s %-15s %-15s\n" "Function Name" "Category" "Count" "Total Duration" "Avg Duration" "Std Duration"
    echo "=============================================================================================="

    for name in $(for n in "${!total_dur[@]}"; do
        printf "%s|%d\n" "$n" "${total_dur[$n]}"
    done | sort -t'|' -k2,2n | cut -d'|' -f1); do
        avg_dur=0
        std_dur=0
        if [[ ${count[$name]} -gt 0 ]]; then
            avg_dur=$((total_dur[$name] / count[$name]))
            # stddev = sqrt((sumsq / count) - (avg^2))
            sumsq_val=${sumsq[$name]}
            cnt=${count[$name]}
            avg_float=$(awk "BEGIN {print ${total_dur[$name]}/$cnt}")
            mean_sq=$(awk "BEGIN {print $avg_float * $avg_float}")
            var=$(awk "BEGIN {print ($sumsq_val/$cnt) - $mean_sq}")
            std_dur=$(awk "BEGIN {if ($var<0) print 0; else print sqrt($var)}")
        fi

        printf "%-30s %-15s %-10d %-15d %-15.2f %-15.2f\n" \
            "$name" \
            "${category[$name]}" \
            "${count[$name]}" \
            "${total_dur[$name]}" \
            "$avg_dur" \
            "$std_dur"
    done

    echo "=============================================================================================="
    log "Total unique functions: ${#total_dur[@]}"
} | tee "$OUTPUT_FILE"
