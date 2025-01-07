gcc -L/usr/local/hiredis-1.2.0/ test.c -o test -lhiredis 
LD_LIBRARY_PATH=/usr/local/hiredis-1.2.0/ ./test