#!/bin/bash
cd src
go build -o ../build/libcustom.so -buildmode=c-shared DBMS/afl-custom
go build -o ../build/dbms-fuzz
cd ..

