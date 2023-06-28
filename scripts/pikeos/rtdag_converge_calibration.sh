#!/bin/bash

function usage() {
    echo "usage: ${BASH_SOURCE[0]} <CPU> <DURATION_US>"
}

function average() {
    awk '{ total += $1; count++ } END { print total/count }'
}

function get_mask() {
    echo $((2 ** $1))
}

function main() {
     if [ $# -lt 2 ]; then
        usage >&2
        return 1
    fi

    local amidone
    local cpu="$1"
    local duration_us="$2"

    local previous=0.0
    local estimation_error=
    TICKS_PER_US="${TICKS_PER_US:-}"

    local SCRIPT_DIR
    SCRIPT_DIR="$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

    amidone=0
    while [ $amidone = 0 ]; do
        TICKS_PER_US=$("$SCRIPT_DIR"/rtdag_calibrate.sh "$cpu" "$duration_us" | tail -1 | cut -d\' -f2)
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
