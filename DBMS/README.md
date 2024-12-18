# No-SQL Fuzzer
* [What is it ?](#introduction)
* [Prepare DBMS](#prepare-targets)
   * [Redis](#redis)
   * [KeyDB](#keydb)
   * [MongoDB](#mongodb)
   * [AgensGraph](#agensgraph)
* [How to Install ?](#install)
* [How to Use ?](#fuzz)

## introduction
This is a Project fuxxing No-SQL Database Management System.<br>
Target No-SQL DBMS:
``` shell
1. Redis (key-value)
2. KeyDB (key-value)
3. MongoDB (JSON Doc)
4. AgensGraph (Cyper)
```

## prepare targets
### redis
redis fuzz required:
- instrument redis
- hiredis
instrument redis-server,if you dont have afl-clang-lto,look up here[afl-clang-lto](#afl-clang-lto)
``` shell
cd /usr/local/redis
AFL_USE_ASAN=1 CC=afl-clang-lto make
```
activate redis: 
``` shell
> AFL_DEBUG=1 /usr/local/redis/src/redis-server # __afl_map_size ssss
> ^C
> ipcmk -M ssss -p 0666 #Shared memory id: xxxx
> AFL_MAP_SIZE=ssss __AFL_SHM_ID=xxxx src/redis-server &
```
### keydb
``` shell
export AFL_USE_ASAN=1
export CC=afl-cc
export CXX=afl-c++
```
### mongodb
### agensgraph

## install
### go build
please do thease after dbms init.
``` shell
> go install -buildmode=shared -linkshared std
> ./install.sh
```
### afl build
in order to use shmem for afl-fuzz, dbms server.<br>
add
``` shell
char *id_str = getenv("SHM_ID");
if (id_str) {
    shm->shm_id = atoi(id_str);
}
```
into afl-sharedmem.c, between 
``` shell
shm->shm_id =
    shmget(IPC_PRIVATE, map_size == MAP_SIZE ? map_size + 8 : map_size,
        IPC_CREAT | IPC_EXCL | DEFAULT_PERMISSION);
<here>
if (shm->cmplog_mode) {
    ...
}
```
### afl-clang-lto
in order to use afl-clang-lto, for example, your llvm version is 16 and lld-16 was installed.
``` shell
export LLVM_CONFIG=llvm-config-16
make && make install
```
## fuzz
add
``` shell
export AFL_MAP_SIZE=ssss
export SHM_ID=xxxx # different from __AFL_SHM_ID
```
into run.sh
``` shell
#select xxx from (redis,keydb,mongodb,agensgraph)
DBMS=xxx ./run.sh 
```
