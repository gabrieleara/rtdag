#!/bin/bash

function main() {
    if [ -z "$1" ]; then
        echo "Argument!" >&2
        return 1
    fi

    cpufreq-set -c 0 -g userspace
    cpufreq-set -c 4 -g userspace
    cpufreq-set -c 4 -f 1.4GHz
    cpufreq-set -c 0 -f 1.4GHz

    for i in $(seq 1 10000); do
        rt_dag -t 100000
    done > /tmp/rt-dag.calib

    grep "Test duration" /tmp/rt-dag.calib | cut -d ' ' -f3 > "$1"
}

(
    set -e
    main "$@"
)
