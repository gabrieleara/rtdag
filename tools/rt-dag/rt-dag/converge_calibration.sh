#!/bin/bash

(
    set -e

    amidone=0

    duration_us="$1"

    if [ -z "$duration_us" ]; then
        echo "Missing duration_us argument" >&2
        false
    fi

    PREV=0.0

    while [ $amidone = 0 ]; do
        TICKS_PER_US=$(./calibrate.sh "$duration_us" | tail -1 | cut -d\' -f2)
        ERROR=$(echo "($PREV - $TICKS_PER_US) / $TICKS_PER_US" | bc -l)

        # Convert to positive using string manipulation
        ERROR=${ERROR#-}

        # Convert value printed by bc to true or false
        if (($(echo "$ERROR < 0.05" | bc -l))); then
            amidone=1
        fi

        PREV="$TICKS_PER_US"
        export TICKS_PER_US
    done

    echo "Final answer:"
    echo "export TICKS_PER_US=$TICKS_PER_US"
)
