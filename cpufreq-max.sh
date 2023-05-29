#!/bin/bash

(
    set -e
    ncpu=$(nproc)

    for ((i = 0; i < "$ncpu"; i++)); do
        cpufreq-set -c "$i" -g userspace
        # Get maximum frequency it can run at and set it manually
        cpufreq-set -c "$i" -f "$(cpufreq-info -c "$i" -l | cut -d ' ' -f 2)"
    done
)
