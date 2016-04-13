#include <iostream>
#include <vector>

#include <fcntl.h> //Some C libraries are required
#include <kvm.h>
#include <libutil.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/event.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <vm/vm_param.h>
using namespace std; 

//These signals will eventually come from a .h file
//44 is a test number, the number used in deployment will require review by an architect
#define SIGTEST 44

#define SIGSEVERE 45
#define SIGMIN 46
#define SIGPAGESNEEDED 47 
#define SIGREGISTERED 48 
#define SIGDEREGISTERED 49 

#define CONVERT(v)	((int64_t)(v) * pagesize / blocksize)
#define CONVERT_BLOCKS(v) 	((int64_t)(v) * pagesize)
static struct kvm_swap swtot;
static int nswdev;
static SLIST_HEAD(slisthead, managed_application) head = SLIST_HEAD_INITIALIZER(head);
static struct slisthead *headp;

//Track all the markers we want to observe
struct memStatus
{
	bool target, min, needed, severe, swap_low;
};

struct managed_application
{
	int pid, condition;
	SLIST_ENTRY(managed_application) next_application;
};

int isRegistered(int pid){

	struct managed_application *current_application = (managed_application*)malloc(sizeof(struct managed_application));
	struct managed_application *np_temp = (managed_application*)malloc(sizeof(struct managed_application));
	
	if (SLIST_FIRST(&head) != NULL){
		SLIST_FOREACH_SAFE(current_application, &head, next_application, np_temp){
			if (current_application->pid == pid){
				return true;				
			}
		}
	}
	return false;	
}

//Query the device for updates statuses. 
memStatus queryDev()
{
	memStatus status = {false,false,false,false,false};
	// Read the file, C++ libraries are no good for reading from a device
	int devfile = open("/dev/lowmem", O_RDWR | O_NONBLOCK);
	if(devfile >= 0){
		char buf[5+sizeof(int)];
		int bytesRead=0;
		int swap_pages=0;
		int swap_space=0;
		//If the transfer worked
		
		if ((bytesRead = read(devfile,&buf,100))) {
		    if(buf[0] & 0b1)
		    	status.target=true;
		    if(buf[0] & 0b10)
		    	status.min=true;
		    if(buf[0] & 0b100)
		    	status.needed=true;
		    if(buf[0] & 0b1000)
		    	status.severe=true;
		    memcpy(&swap_pages, &buf[1], sizeof(int));
		    swap_space = swap_pages * getpagesize();
		    printf("swap_space: %d\n", swap_space);
		    if(swap_space<250000000){
			status.swap_low=true;
			printf("LOW SWAP!!\n");
			}
		}
	}
	return status;
}

static void print_swap_stats(const char *swdevname, intmax_t nblks, intmax_t bused, intmax_t bavail, float bpercent)
{
	char usedbuf[5];
	char availbuf[5];
	int hlen, pagesize;
	long blocksize;
	const char *header;	

	pagesize = getpagesize();
	getbsize(&hlen, &blocksize);
	header = getbsize(&hlen, &blocksize);
		

	(void)printf("%-15s %*s %8s %8s %8s\n", "Device", hlen, header, "Used", "Avail", "Capacity");
	
	printf("%-15s %*jd ", swdevname, hlen, CONVERT(nblks));
	humanize_number(usedbuf, sizeof(usedbuf), CONVERT_BLOCKS(bused), "",
			HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
	humanize_number(availbuf, sizeof(availbuf), CONVERT_BLOCKS(bavail), "",
			HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
	printf("%8s %8s %5.0f%%\n", usedbuf, availbuf, bpercent);
}

static void swapmode_sysctl(void)
{
	struct kvm_swap kswap;
	struct xswdev xsw;
	size_t mibsize, size;
	int mib[16], n;
	swtot.ksw_total = 0;
	swtot.ksw_used = 0;
	mibsize = sizeof mib / sizeof mib[0];
	sysctlnametomib("vm.swap_info", mib, &mibsize);
	for (n=0; ; ++n){
		mib[mibsize] = n;
		size = sizeof xsw;
		if (sysctl(mib, mibsize + 1, &xsw, &size, NULL, 0) == -1)
			break;
		kswap.ksw_used = xsw.xsw_used;
		kswap.ksw_total = xsw.xsw_nblks;
		swtot.ksw_total += kswap.ksw_total;
		swtot.ksw_used += kswap.ksw_used;
		++nswdev;	
	}
	print_swap_stats("Swap Total", swtot.ksw_total, swtot.ksw_used,
			swtot.ksw_total - swtot.ksw_used,
			(swtot.ksw_used * 100.0) / swtot.ksw_total);	
}

static void physmem_sysctl(void)
{
	int mib[2], usermem;
	size_t len;
	mib[0] = CTL_HW;
	mib[1] = HW_USERMEM;
	len = sizeof(usermem);
	sysctl(mib, 2, &usermem, &len, NULL, 0);
//	cout << "Free memory: " << usermem << endl; //change to printf
}

void monitor_application(int signal_number, siginfo_t *info, void *unused){
	
	struct managed_application *current_application = (managed_application*)malloc(sizeof(struct managed_application));
	struct managed_application *np_temp = (managed_application*)malloc(sizeof(struct managed_application));
	struct managed_application *application = (managed_application*)malloc(sizeof(struct managed_application));
	
	if (SLIST_FIRST(&head) != NULL){
		SLIST_FOREACH_SAFE(current_application, &head, next_application, np_temp){
			if (current_application->pid == info->si_pid){
				SLIST_REMOVE(&head, current_application, managed_application, next_application);
	
				kill(current_application->pid, SIGDEREGISTERED);
				free(current_application);
				printf("DEREGISTERED\n");
				return;
			}
			if (kill(current_application->pid,0)==-1){
				SLIST_REMOVE(&head, current_application, managed_application, next_application);
				free(current_application);
				printf("TIMED OUT\n");
			}
		}
	}
	application->pid = info->si_pid;	
	application->condition = signal_number;
	SLIST_INSERT_HEAD(&head, application, next_application);
	kill(application->pid, SIGREGISTERED);
	printf("REGISTERED\n");

}

void random_millisecond_sleep(int min, int max)
{
	struct timespec sleepFor;
	int randomMilliseconds = ((rand() % max)+min) * 1000 * 1000;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = randomMilliseconds;
	nanosleep(&sleepFor, 0);
}

void suspend_applications()
{
	struct managed_application *current_application = (managed_application*)malloc(sizeof(struct managed_application));
	SLIST_FOREACH(current_application, &head, next_application){
		int pid = current_application->pid;
		kill(pid, SIGSTOP);
		printf("SUSPENDED: %d\n", pid);
	}
}

void resume_applications()
{
	struct managed_application *current_application = (managed_application*)malloc(sizeof(struct managed_application));
	SLIST_FOREACH(current_application, &head, next_application){
		int pid = current_application->pid;
		kill(pid, SIGCONT);
		printf("CONTINUED: %d\n", pid);
		random_millisecond_sleep(0,1000);
	}
}

/*
* Memory Conditions:
* 0 = Severe Low Memory
* 1 = Under Minimum Free Pages Threshold
* 2 = Not Enough Free Pages
*/

int main(int argc, char ** argv)
{
	if (argc != 1){
		printf("Args: %d\n", argc);
		return -1;
	}

//	daemon(0,0);
	
	struct sigaction sig;
	sig.sa_sigaction = monitor_application;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIGSEVERE, &sig, NULL);
	sigaction(SIGMIN, &sig, NULL);
	sigaction(SIGPAGESNEEDED, &sig, NULL);
		
	SLIST_INIT(&head);
	struct managed_application *current_application = (managed_application*)malloc(sizeof(struct managed_application));
	
	for(;;){
		swapmode_sysctl();
		physmem_sysctl();
		memStatus status = queryDev();
		if (status.severe || status.min || status.needed){
			SLIST_FOREACH(current_application, &head, next_application){
				int pid = current_application->pid;
				if(status.severe && current_application->condition == SIGSEVERE){
					kill(pid,SIGTEST);
					printf("KILLED SEVERE: %d\n", pid);
				}
				if(status.min && current_application->condition == SIGMIN){
					kill(pid,SIGTEST);
					printf("KILLED MIN: %d\n", pid);
				}
				if(status.needed && current_application->condition == SIGPAGESNEEDED){
					kill(pid,SIGTEST);
					printf("KILLED PAGES NEEDED: %d\n", pid);
				}
				random_millisecond_sleep(0,1000);
			}
			status = queryDev();
			if (status.severe || status.swap_low){
				suspend_applications();
				resume_applications();
			}
			
		}
		sleep(2);
	}
	return 0;
}
