#!/bin/bash
export SHM_ENV_VAR=666
export AFL_DEBUG=1
export AFL_DISABLE_TRIM=1
export AFL_CUSTOM_MUTATOR_ONLY=1
export AFL_SKIP_CPUFREQ=1
export AFL_CUSTOM_MUTATOR_LIBRARY=./libm.so
afl-fuzz -i input -o output -- ./test