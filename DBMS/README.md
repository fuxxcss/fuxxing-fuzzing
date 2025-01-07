# No-SQL Fuzzer
* [What is it ?](#introduction)
* [Prepare DBMS](#prepare-targets)
   * [Redis](#redis)
   * [KeyDB](#keydb)
   * [Redis Stack](#redis-stack)
   * [ArangoDB](#arangodb)
* [How to Install ?](#install)
* [How to Use ?](#fuzz)

## introduction
This is a Project fuxxing No-SQL Database Management System.
Target No-SQL DBMS:
``` shell
1. Redis (key-value)
2. KeyDB (key-value)
3. Redis Stack (Multi-model)
4. ArangoDB (Graph)
```

## prepare targets

### redis
redis fuzz required:
- instrument redis (disable shared)
- hiredis
instrument redis-server,if you dont have afl-clang-lto,look up here [afl-clang-lto](#afl-clang-lto).
``` shell
> cd /usr/local/redis
> AFL_USE_ASAN=1 CC=afl-clang-lto make -j4
```
activate redis (maybe need to trash /root/dump.rdb first) : 
``` shell
> AFL_DEBUG=1 /usr/local/redis/src/redis-server # __afl_map_size ssss
> ^C
> ipcmk -M ssss -p 0666 #Shared memory id: xxxx
> AFL_MAP_SIZE=ssss __AFL_SHM_ID=xxxx /usr/local/redis/src/redis-server &
```
construct initial testcases from [redis commands](https://redis.io/docs/latest/commands/).

### keydb
keydb fuzz required:
- instrument keydb (disable shared)
- hiredis
instrument keydb-server
``` shell
> apt install libcurl4-openssl-dev
> cd /usr/local/keydb
> AFL_USE_ASAN=1 CC=afl-clang-lto CXX=afl-clang-lto++ make MALLOC=libc -j4
```
activate redis (maybe need to trash /root/dump.rdb first) : 
``` shell
> AFL_DEBUG=1 /usr/local/keydb/src/keydb-server -- port 6380 # __afl_map_size ssss
> ^C
> ipcmk -M ssss -p 0666 #Shared memory id: xxxx
> AFL_MAP_SIZE=ssss __AFL_SHM_ID=xxxx /usr/local/keydb/src/keydb-server --port 6380 &
```
keydb is a fork of redis,so we reuse input/redis.

### redis-stack
redis stack fuzz required:
- instrument redis (disable shared)
- hiredis
- redis-stack-server
download redis-stack-server from [redis stack server](https://redis.io/downloads/#redis-stack-downloads).
copy redis-stack-server 、etc and lib into /usr/local/redis/src/.
add
``` shell
REDIS_DATA_DIR=redis-stack
echo "Starting redis-stack-server, database path ${REDIS_DATA_DIR}"
CMD=./redis-server
CONFFILE=etc/redis-stack.conf
MODULEDIR=$(realpath .)/lib
```
before
``` shell
${CMD} \
${CONFFILE} \
--dir ${REDIS_DATA_DIR} \
...
```
activate redis stack :
``` shell
> mkdir redis-stack
> chmod +x ./redis-stack-server
> ./redis-stack-server
```

### arangodb
arangodb fuzz required:
- instrument arangodb (disable shared)
- official go-driver
instrument arangodb (maybe need to rm -f CMakeCache.txt first) :
``` shell
> cd /usr/local/
> git clone -b vx.y.z --depth=1 --recursive https://github.com/arangodb/arangodb.git
# get pubkey.gpg from https://dl.yarnpkg.com/debian/pubkey.gpg
# apt-key add pubkey.gpg
# echo "deb https://dl.yarnpkg.com/debian/ stable main" | tee /etc/apt/sources.list.d/yarn.list
# apt install yarn
> cmake .. -DCMAKE_C_COMPILER=afl-clang-fast -DCMAKE_CXX_COMPILER=afl-clang-fast++ -Wno-dev -DUSE_MAINTAINER_MODE=off -DUSE_GOOGLE_TESTS=off -DUSE_JEMALLOC=Off
> CC=afl-clang-fast CXX=afl-clang-fast++ AFL_USE_ASAN=1 make -j4

```

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

#### afl-clang-lto
in order to use afl-clang-lto, for example, your llvm version is 16 and lld-16 was installed.
``` shell
> export LLVM_CONFIG=llvm-config-16
> make && make install
```
#### afl-gxx-fast
in order to use afl-gxx-fast, for example, your gcc version is 12 and gcc-12-plugin-dev was installed.
``` shell
> make && make install
```

## fuzz
add
``` shell
export AFL_MAP_SIZE=ssss
export SHM_ID=xxxx # same as __AFL_SHM_ID
```
into run.sh
``` shell
> ./run.sh
select db from (redis,keydb,stack,arango)
db:
redis
```
