#!/bin/bash

function average() {
    awk '{s+=$1}END{print s/NR}' RS=" "
}

(
    set -e

    duration_us="$1"

    if [ -z "$duration_us" ] ; then
        echo "Missing duration_us argument" >&2
        false
    fi

    for i in $(seq 1 1000); do
        taskset -c 4 chrt -f 99 ./build/rt_dag -c "$duration_us"
    done > /tmp/rt-dag.calib

    TICKS_PER_US=$(grep export /tmp/rt-dag.calib | cut -d '=' -f2 | cut -d "'" -f1 | average)
    echo "AVERAGE: $TICKS_PER_US"
    TICKS_PER_US=$(echo "($TICKS_PER_US * 9) / 10" | bc)

    echo "However, for safety reasons, use instead:"
    echo "export TICKS_PER_US='${TICKS_PER_US}'"
    export TICKS_PER_US
)
