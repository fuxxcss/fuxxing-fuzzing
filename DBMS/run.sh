#!/bin/bash
echo -n "select db from (redis,keydb,mongodb,agensgraph)"
echo -e "\n> "
read db

if [ $db == "keydb" ];then
    path="redis"
else
    path=$db
if

export AFL_MAP_SIZE=2097152
export SHM_ID=54

#export AFL_DEBUG=1
export AFL_CUSTOM_MUTATOR_ONLY=1
export AFL_CUSTOM_MUTATOR_LIBRARY=build/libcustom.so
export AFL_DISABLE_TRIM=1
export AFL_SKIP_CPUFREQ=1
export AFL_SKIP_BIN_CHECK=1
export AFL_USE_ASAN=1
export AFL_FAST_CAL=1


INPUT=input
INPUT_PATH=$INPUT/$path
OUTPUT=output
OUTPUT_PATH=$OUTPUT/$db

afl-fuzz -i $INPUT_PATH -o $OUTPUT_PATH -- build/dbms-fuzz