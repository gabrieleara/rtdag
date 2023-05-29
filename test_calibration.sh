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

function freq_format() {
    printf "%.1f" "$(echo "$1 / 1000000.0" | bc -l)"
}

function main() {
    local cpu
    local duration_us
    local freq
    local i

    cpu="$1"
    duration_us="$2"

    if [ -z "$cpu" ] ; then
        echo "Missing cpu argument" >&2
        usage >&2
        return 1
    fi

    if [ -z "$duration_us" ] ; then
        echo "Missing duration_us argument" >&2
        usage >&2
        return 1
    fi

    freq_max_all

    for freq in $(cat "/sys/devices/system/cpu/cpufreq/policy${cpu}"/scaling_available_frequencies); do
        echo -n "$cpu $(freq_format "$freq") "
        cpufreq-set -c "$cpu" -f "$freq"
        for ((i=0; i < 100; i++)); do
            taskset -c "$cpu" chrt -f 99 ./build/rt_dag -c "$duration_us"
        done > /tmp/rt-dag.calib
        grep "Test duration" /tmp/rt-dag.calib | cut -d ' ' -f3 | average
    done

    freq_max_all
}

(
    set -e
    main "$@"
)
