#!/bin/bash

function usage() {
    echo "usage: ${BASH_SOURCE[0]} <CPU> <DURATION_US>"
}

function average() {
    awk '{ total += $1; count++ } END { print total/count }'
}

function get_mask() {
    echo $((2 ** $1))
}

function main() {
    # Use exported value or empty, export value such as ./usr/bin to
    # override executables instead of those in PATH
    PREFIX="${PREFIX:-}"

    if [ $# -lt 2 ]; then
        usage >&2
        return 1
    fi

    RUNSCHED=runsched
    RTDAG=rtdag

    local cpu="$1"
    local duration_us="$2"
    local cpu_mask
    local i
    local j

    cpu_mask="$(get_mask "$cpu")"

    echo "DEBUG: TICKS_PER_US=$TICKS_PER_US" >&2
    j=50
    for i in $(seq 1 1000); do
        if [ "$i" = "$j" ]; then
            echo "DEBUG: Calib [$i/1000]" >&2
            j=$((j+50))
        fi
        "${PREFIX}${RUNSCHED}" SCHED_FIFO 99 taskset "$cpu_mask" "${PREFIX}${RTDAG}" -c "$duration_us"
    done > /tmp/rt-dag.calib
    TICKS_PER_US=$(grep "export" /tmp/rt-dag.calib | cut -d '=' -f2 | cut -d "'" -f1 | average)
    echo "export TICKS_PER_US='$TICKS_PER_US'"
    echo "Calibration step done" >&2
}

(
    set -e
    main "$@"
)
