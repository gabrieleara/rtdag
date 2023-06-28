#!/bin/bash

function rsudo() {
    sudo PATH="$PATH" HOME="$HOME" LD_LIBRARY_PATH="$LD_LIBRARY_PATH" "$@"
}

function run_rtdag() {
    local infile="$1"
    rsudo -E build/rtdag "$infile"
}

function main() {
    SCRIPT_PATH="$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
    export TICKS_PER_US=6.26861
    export LD_LIBRARY_PATH="$SCRIPT_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us

    if [ $# -lt 1 ]; then
        echo "Expecting argument file!!" >&2
        return 1
    fi

    run_rtdag "$1"
}

(
    set -e
    main "$@"
)
