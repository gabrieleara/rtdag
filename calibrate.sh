#!/bin/bash

function usage() {
    echo "usage: ${BASH_SOURCE[0]} CPU FREQ DURATION_US"
}

function average() {
    awk '{ total += $1; count++ } END { print total/count }'
}

function freq_max_all() {
    local cpu
    local ncpu

    ncpu=$(nproc)
    for ((cpu = 0; cpu < ncpu; cpu++)); do
        cpufreq-set -c "$cpu" -g userspace

        # Set the maximum freq on each CPU
        cpufreq-set -c "$cpu" -f "$(cpufreq-info -c "$cpu" -l | cut -d ' ' -f 2)"
    done
}

function main() {
    local cpu="$1"
    local freq="$2"
    local duration_us="$3"

    if [ -z "$cpu" ]; then
        echo "Missing CPU argument" >&2
        usage >&2
        return 1
    fi

    if [ -z "$freq" ]; then
        echo "Missing FREQ argument" >&2
        usage >&2
        return 1
    fi

    if [ -z "$duration_us" ]; then
        echo "Missing DURATION_US argument" >&2
        usage >&2
        return 1
    fi

    # FIXME: restore original frequencies

    freq_max_all
    cpufreq-set -c "$cpu" -f "$freq"
    echo "DEBUG: TICKS_PER_US=$TICKS_PER_US" >&2
    for i in $(seq 1 1000); do
        taskset -c "$cpu" chrt -f 99 ./build/rt_dag -c "$duration_us"
    done > /tmp/rt-dag.calib
    TICKS_PER_US=$(grep "export" /tmp/rt-dag.calib | cut -d '=' -f2 | cut -d "'" -f1 | average)
    echo "export TICKS_PER_US='$TICKS_PER_US'"
    freq_max_all
    echo "Calibration step done" >&2
}

(
    set -e

    main "$@"
)
