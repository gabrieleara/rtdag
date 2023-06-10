# RTDAG, a real-time DAG emulator

RTDAG is an application that given a real-time DAG description launches a
set of Linux threads (or processes) that emulate the behavior of the DAG.

The application can be used to test the execution of applications with
end-to-end (DAG) deadline requirements periodically. Multiple instances of
RTDAG can be executed in parallel, to test even multi-DAG scenarios.

## QUICK LAUNCH :rocket:

### Building

Quickest way you can get it up and running:

```bash
cmake -S . -B ./build # Configuration step
cmake --build ./build # Build step
```

That's it, you now can find in `./build/bin/rtdag` the final binary.

### Running

> TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
> TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
> TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
> TODO

## Customizing RTDAG behavior and selecting features

There are several options that you can customize in the configuration step.
Default values should be fine for running the application in "release" mode
using CPU-only tasks. During the configuration phase, CMake will print the
configuration options for you, like this:

```txt
$ cmake -S . -B ./build
-- [...]
-- ---------- CONFIGURATION OPTIONS ----------
-- CMAKE_BUILD_TYPE            Release
-- RTDAG_LOG_LEVEL             none (0)
-- RTDAG_TASK_IMPL             thread (0)
-- RTDAG_INPUT_TYPE            yaml (0)
-- RTDAG_COMPILER_BARRIER      ON
-- RTDAG_MEM_ACCESS            OFF
-- RTDAG_COUNT_TICK            ON
-- RTDAG_OPENCL_SUPPORT        OFF
-- RTDAG_FRED_SUPPORT          OFF
-- -------------------------------------------
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /home/xxx/rtdag/build
```

Any of the displayed options can be changed by passing it as argument to
`cmake`, like so:

```txt
$ cmake -S . -B ./build -DRTDAG_MEM_ACCESS=ON
-- [...]
-- ---------- CONFIGURATION OPTIONS ----------
-- [...]
-- RTDAG_MEM_ACCESS            ON
-- [...]
-- -------------------------------------------
-- Configuring done (2.6s)
-- Generating done (0.0s)
-- Build files have been written to: /home/xxx/rtdag/build
```

> **NOTE**: If you are re-configuring, either delete the original build
> directory or run `cmake` with `--fresh` (if supported on your platform,
> CMake version >= 3.24), otherwise it will NOT force a re-configuration
> and it may keep the old values as they are.

> **NOTE**: All these options are technically compatible with cross
> compilation, except with OpenCL, which is not tested yet.

## Authors

 - Tommaso Cucinotta (June 2022 - November 2022)
 - Alexandre Amory (June 2022 - November 2022)
 - Gabriele Ara (September 2022 - June 2023)

 [Real-Time Systems Laboratory (ReTiS Lab)][retis], [Scuola Superiore
 Sant'Anna (SSSA)][sssup], Pisa, Italy.

## Funding

This software package has been developed in the context of the [AMPERE
project](https://ampere-euproject.eu/). This project has received funding
from the European Union’s Horizon 2020 research and innovation programme
under grant agreement No 871669.


<!-- Links -->

[retis]: https://retis.santannapisa.it/
[sssup]: https://www.santannapisa.it/
