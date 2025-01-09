#!/bin/bash
echo -e "select db from (redis,keydb,memcached,stack)\ndb:"
read db

if [ $db == "keydb" ];then
    path="redis"
else
    path=$db
fi

export AFL_MAP_SIZE=2097152
export SHM_ID=32777

#export AFL_DEBUG=1
export DBMS=$db
export LD_LIBRARY_PATH=/usr/local/hiredis-1.2.0/
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

afl-fuzz -t 5000 -i $INPUT_PATH -o $OUTPUT_PATH -- build/dbms-fuzz