# DBMS
fuxxing database management system.
## go build
please do thease after dbms init.
``` shell
> go install -buildmode=shared -linkshared std
> ./install.sh
```
## afl build
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
in order to use afl-clang-lto, for example, your llvm version is 16 and lld-16 was installed.
``` shell
export LLVM_CONFIG=llvm-config-16
make && make install
```
## dbms init
### postresql fuzz
postgresql fuzz required: 
we need two version : 
- instrument postresql (dir pgsql-inst)
- no-instrument (dir pgsql)<br>
instrument postresql :<br>add 
``` shell
export AFL_USE_ASAN=1
export CC=afl-cc
export CXX=afl-c++
```
into postgresql-xx.x/configure, then
``` shell
> ./configure --prefix=/usr/local/pgsql-inst
> make -j4 && make install
```
create dir /usr/local/pgsql-inst/data<br>
``` shell
> chown -R postgres /usr/local/pgsql-inst
> su postgres
> AFL_IGNORE_PROBLEMS=1 /usr/local/pgsql-inst/bin/initdb -D /usr/local/pgsql-inst/data
```
no-instrument postgresql:<br>
normal install, maybe need make clean
create new user and new database:<br>
``` shell
> su postgres
> /usr/local/pgsql-inst/bin/psql
postgres=# create user fuzzer superuser password 'goodluck';
postgres=# create database fuzzdb owner fuzzer;
postgres=# grant all privileges on database fuzzdb to fuzzer;
```
activate postgresql : 
``` shell
> su postgres
> AFL_DEBUG=1 /usr/local/pgsql-inst/bin/postgres -D /usr/local/pgsql-inst/data # __afl_map_size 2097152
> ^C
> ipcmk -M 2097152 -p 0666 #Shared memory id: xxxx
> AFL_MAP_SIZE=2097152 __AFL_SHM_ID=xxxx /usr/local/pgsql-inst/bin/postgres -D /usr/local/pgsql-inst/data &
```
### redis fuzz
redis fuzz required:
- instrument redis
- hiredis
instrument redis-server:
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
## dbms fuzz
add
``` shell
export AFL_MAP_SIZE=ssss
export SHM_ID=xxxx # different from __AFL_SHM_ID
```
into run.sh

``` shell
DBMS=xxx ./run.sh
```
