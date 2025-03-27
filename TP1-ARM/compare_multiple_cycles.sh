#!/bin/bash

# TO RUN:
# chmod +x compare_multiple_cycles.sh
# ./compare_multiple_cycles.sh <program_file1> [program_file2 ...] [--cycles_start=N] [--cycles_end=M] [--cycles_step=P]

set -e

# Get the absolute path of the script
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Check if program files are provided
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <program_file1> [program_file2 ...] [--cycles_start=N] [--cycles_end=M] [--cycles_step=P]"
    exit 1
fi

# Default values for cycles
CYCLES_START=1
CYCLES_END=20
CYCLES_STEP=1
ARGS=()

# Parse arguments
for arg in "$@"; do
    if [[ "$arg" == --cycles_start=* ]]; then
        CYCLES_START="${arg#--cycles_start=}"
    elif [[ "$arg" == --cycles_end=* ]]; then
        CYCLES_END="${arg#--cycles_end=}"
    elif [[ "$arg" == --cycles_step=* ]]; then
        CYCLES_STEP="${arg#--cycles_step=}"
    else
        ARGS+=("$arg")
    fi
done

# Function to compare dumpsim for a given cycle
compare_cycle() {
    local cycles=$1
    echo "Running comparison with cycles=$cycles..."
    ./compare_rdump.sh "${ARGS[@]}" --cycles="$cycles"
    if [ $? -ne 0 ]; then
        echo "❌ Difference found for cycles=$cycles."
        return 1
    fi
    return 0    
}

# Run comparisons for the specified range of cycles
all_same=true
for ((cycles=$CYCLES_START; cycles<=$CYCLES_END; cycles+=$CYCLES_STEP)); do
    compare_cycle "$cycles" || all_same=false
done

# Final result
if $all_same; then
    echo "✅ All simulations returned the same memory dump for the given cycles."
else
    echo "❌ Some simulations returned different memory dumps. Please check the diff files for details."
fi
