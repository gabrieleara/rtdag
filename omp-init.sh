#!/bin/bash

# Mount LLVM remote host if requested
USER_HOST="$1"
if [ -n "$USER_HOST" ]; then
    export LD_LIBRARY_PATH=/usr/local/llvm-17.0.0-bsc/lib
    sudo sshfs -o allow_other $USER_HOST:llvm /llvm
else
    export LD_LIBRARY_PATH=./lib
fi

# Enable the fan (NOTICE: this does NOT make the fan run at
# full speed, it just turns it up!)
# sudo jetson_clocks --fan

# Enable RT tasks to run
echo -1 | sudo tee /proc/sys/kernel/sched_rt_runtime_us

# Force maximum frequency
sudo cpufreq-set -c 0-7 -g performance
sudo cpufreq-set -c 0-7 -d 2.27GHz

# Turn off graphic interface
sudo systemctl isolate multi-user.target

# Sudo with LD
alias sudo='sudo PATH="$PATH" HOME="$HOME" LD_LIBRARY_PATH="$LD_LIBRARY_PATH"'

export TICKS_PER_US=12
