#!/bin/bash
export DBMS=postgresql
export AFL_CUSTOM_MUTATOR_ONLY=1
export AFL_CUSTOM_MUTATOR_LIBRARY=build/libcustom.so
export AFL_DISABLE_TRIM=1
export AFL_SKIP_CPUFREQ=1
export AFL_SKIP_BIN_CHECK=1
export AFL_USE_ASAN=1
export AFL_FAST_CAL=1
#export AFL_DEBUG=1

export AFL_MAP_SIZE=2097152
export SHM_ID=32787

INPUT=input
INPUT_PATH=$INPUT/$DBMS
OUTPUT=output
OUTPUT_PATH=$OUTPUT/$DBMS

afl-fuzz -i $INPUT_PATH -o $OUTPUT_PATH -- build/dbms-fuzz