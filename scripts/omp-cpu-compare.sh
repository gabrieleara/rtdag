#!/bin/bash

function rsudo() {
    sudo PATH="$PATH" HOME="$HOME" LD_LIBRARY_PATH="$LD_LIBRARY_PATH" "$@"
}

function run_rtdag() {
    local type="$1"
    local msize="$2"
    rsudo -E chrt -f 99 taskset -c 1 build/rtdag -t 1 -M "$msize" -C "$type" | grep micros | cut -d' ' -f3
}

function avg() {
    awk '{ sum += $1 } END { if (NR > 0) print sum / NR ; else print sum }'
}

function run_rtdag_many_times() {
    local type="$1"
    local msize="$2"

    for i in $(seq 1 10); do
        echo -n "." >&2
        run_rtdag "$type" "$msize"
        sleep .1s
    done
}

function main() {
    SCRIPT_PATH="$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
    export TICKS_PER_US=1
    export LD_LIBRARY_PATH="$SCRIPT_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

    echo "matrix_size duration_cpu_us duration_omp_us"
    for msize in 64 100 110 127 128; do
        echo "" >&2
        echo -n "CPU $msize: " >&2
        t_cpu=$(run_rtdag_many_times cpu $msize | avg)
        echo "" >&2
        echo -n "OMP $msize: " >&2
        t_omp=$(run_rtdag_many_times omp $msize | avg)

        echo "$msize $t_cpu $t_omp"
    done
}

(
    set -e
    main "$@"
)
