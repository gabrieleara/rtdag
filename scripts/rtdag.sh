#!/bin/bash

function rsudo() {
    sudo PATH="$PATH" HOME="$HOME" LD_LIBRARY_PATH="$LD_LIBRARY_PATH" "$@"
}

function run_rtdag() {
    local mdir="$PWD"
    local tmpdir=$(mktemp -d)

    flist=()
    for f in "$@" ;do
        flist+=("$(realpath "$f")")
    done

    cd "$tmpdir"
    for f in "${flist[@]}"; do
        rsudo -E "$RTDAG_CMD" "$f" &
    done

    wait

    for f in $(find -type f); do
        cp "$f" "$mdir"
    done

    cd - >/dev/null
    rsudo rm -rf "$tmpdir"
}

function main() {
    SCRIPT_PATH="$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
    RTDAG_HOME=$(realpath "$SCRIPT_PATH/..")
    LIB_PATH="$RTDAG_HOME/lib"
    RTDAG_CMD="$RTDAG_HOME/build/rtdag"

    export LD_LIBRARY_PATH="$LIB_PATH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export TICKS_PER_US=6.26861
    echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us

    if [ $# -lt 1 ]; then
        echo "Expecting at least one argument file!!" >&2
        return 1
    fi

    run_rtdag "$@"
}

(
    set -e
    main "$@"
)
