# CS492-FreeBSD
FreeBSD Senior Project @ UIUC

Made for FreeBSD 10.2 and compiled with clang 3.4.1


daemon.cpp is the daemon that handles registrating/deregistration of applications and blocks waiting for events from our char device. On event it sends notifications to all registered applications

main.c is our character device. This polls kernel memory variables and upon certain thresholds it will alert our daemon

memhog.c is used for testing. It registers with our daemon and continuously allocates memory until it receives a signal from our daemon. It then cuts memory usage and then keeps allocating new memory



Compilation Instructions

make

sudo kldload ./main.ko

clang++ daemon.cpp -lutil -pthread -o daemon

./daemon

clang memhog.c -o memhog

sudo ./memhog [signal #]

clang -o registration_test -I/usr/local/include -L/usr/local/lib registration_test.c -latf-c 

kyua test
