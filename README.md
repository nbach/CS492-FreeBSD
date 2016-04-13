# CS492-FreeBSD
FreeBSD Senior Project @ UIUC


1/27 Compilation Instructions

make

sudo kldload ./main.ko

clang++ daemon.cpp -lutil -o daemon

./daemon

clang memhog.c -o memhog

sudo ./memhog [daemon pid] [signal #]

4/3

clang++ -c -lutil -fPIC daemon.cpp

clang++ -shared -lutil -o libDaemon.so daemon.o

clang -o registration_test -I/usr/local/include -L/usr/local/lib registration_test.c -latf-c 

kyua test
