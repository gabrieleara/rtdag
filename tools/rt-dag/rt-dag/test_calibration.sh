#!/bin/bash

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

    if [ -z "$cpu" ] ; then
        echo "Missing cpu argument" >&2
        false
    fi

    if [ -z "$duration_us" ] ; then
        echo "Missing duration_us argument" >&2
        false
    fi

    max_all_freqs

    for freq in $(cat "/sys/devices/system/cpu/cpufreq/policy${cpu}"/scaling_available_frequencies) ; do
        echo -n "$freq "
        cpufreq-set -c "$cpu" -f "$freq"
        for i in $(seq 1 100); do
            taskset -c "$cpu" chrt -f 99 ./build/rt_dag -c "$duration_us"
        done > /tmp/rt-dag.calib
        grep "Test duration" /tmp/rt-dag.calib | cut -d ' ' -f3 | average
    done

    max_all_freqs
)
