#!/bin/bash

function usage() {
    echo "usage: ${BASH_SOURCE[0]} CPU DURATION_US"
}

function average() {
    awk '{s+=$1}END{print s/NR}' RS=" "
}

function max_all_freqs() {
    local max_freq
    local cpu
    for cpu in $(seq 0 $(($(nproc) - 1))); do
        max_freq=$(cpufreq-info -c $cpu --hwlimits | cut -d' ' -f2)
        cpufreq-set -c $cpu -g userspace
        cpufreq-set -c $cpu -f "$max_freq"
    done
}

(
    set -e

    cpu="$1"
    duration_us="$2"

    if [ -z "$cpu" ]; then
        echo "Missing cpu argument" >&2
        usage >&2
        false
    fi

    if [ -z "$duration_us" ] ; then
        echo "Missing duration_us argument" >&2
        usage >&2
        false
    fi

    max_all_freqs

    echo "DEBUG: TICKS_PER_US=$TICKS_PER_US" >&2

    for i in $(seq 1 1000); do
        taskset -c 4 chrt -f 99 ./build/rt_dag -c "$duration_us"
    done > /tmp/rt-dag.calib

    TICKS_PER_US=$(grep export /tmp/rt-dag.calib | cut -d '=' -f2 | cut -d "'" -f1 | average)
    echo "AVERAGE: $TICKS_PER_US"
    TICKS_PER_US=$(echo "$TICKS_PER_US * 0.9" | bc -l)

    echo "However, for safety reasons, use instead:"
    echo "export TICKS_PER_US='${TICKS_PER_US}'"
    export TICKS_PER_US

    max_all_freqs
)
