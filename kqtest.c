#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <kvm.h> 
#include <libutil.h>
#include <signal.h>
#include <unistd.h>

#include <sys/event.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <vm/vm_param.h>

struct kevent ev;
struct timespec nullts = {0,0};
int fd=0;
int main(int argc, char **argv){
	fd = open("/dev/lowmem", O_RDWR | O_NONBLOCK);
	int kq=kqueue();
	EV_SET(&ev,fd,EVFILT_READ, EV_ADD,0,0,NULL);
	kevent(kq,&ev,1,NULL,0,&nullts);
	for(;;){
		printf("Starting\n");
		int n=kevent(kq,NULL,0,&ev,1,NULL);
		printf("After\n");
		if(n>0){
			printf("Something happened ev.fflags=%i\n",(int)ev.fflags);
		} else {
			printf("Cycle");
		}
	}
	return 0;
}