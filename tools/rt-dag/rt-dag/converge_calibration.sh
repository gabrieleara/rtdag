#!/bin/bash

function usage() {
    echo "usage: ${BASH_SOURCE[0]} CPU FREQ DURATION_US"
}

function main() {
    local amidone
    local cpu
    local freq
    local duration_us

    cpu="$1"
    freq="$2"
    duration_us="$3"

    if [ -z "$cpu" ]; then
        echo "Missing CPU argument" >&2
        usage >&2
        false
    fi

    if [ -z "$freq" ]; then
        echo "Missing FREQ argument" >&2
        usage >&2
        false
    fi

    if [ -z "$duration_us" ]; then
        echo "Missing DURATION_US argument" >&2
        usage >&2
        false
    fi

    local previous=0.0
    local estimation_error=
    TICKS_PER_US="${TICKS_PER_US:-}"

    amidone=0
    while [ $amidone = 0 ]; do
        TICKS_PER_US=$(./calibrate.sh "$cpu" "$freq" "$duration_us" | tail -1 | cut -d\' -f2)
        estimation_error=$(echo "($previous - $TICKS_PER_US) / $TICKS_PER_US" | bc -l)

        # Convert to positive using string manipulation
        estimation_error=${estimation_error#-}

        # Convert value printed by bc to true or false
        if (($(echo "$estimation_error < 0.05" | bc -l))); then
            amidone=1
        fi

        previous="$TICKS_PER_US"
        export TICKS_PER_US
    done

    echo "Final answer:"
    echo "export TICKS_PER_US=$TICKS_PER_US"
}

(
    set -e
    main "$@"
)
