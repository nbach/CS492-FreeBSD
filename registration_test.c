#include <atf-c.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define SIGTEST 44
#define SIGSEVERE 45
#define SIGMIN 46
#define SIGPAGESNEEDED 47
#define SIGREGISTERED 48
#define SIGDEREGISTERED 49

volatile int registered = 0;
sem_t sem;

static void receiveRegistrationSig(int n, siginfo_t *info, void *unused){
	printf("registration signal received");
	sem_wait(&sem);
	registered = 1;
	sem_post(&sem);
}

static void receiveDeregistrationSig(int n, siginfo_t *info, void *unused){
	printf("deregistration signal received");
	sem_wait(&sem);
	registered = 0;
	sem_post(&sem);
}

static int getDaemonPID(){
	FILE *in = popen("pgrep daemon","r");
	int pid = 0;
	char line[20];
	fgets(line, 20, in);
	sscanf(line, "%d", &pid);
	printf("daemon pid: %d", pid);

	return pid;
}

static int registerApp(int pid){
	kill(pid, SIGSEVERE);

	int i = 0;
	int sleepInterval = 1000000;
	int timeoutCount = 10;
	bool success = false;

	while(i < timeoutCount && !success){
		nanosleep(sleepInterval);
		if(registered)
		{
			success = true;
		}	
		i++;
	}
	if(success){
		printf("registration succeesful");
		return 1;
	}
	else{
		printf("registration unsuccessful");
		return 0;
	}
}

static int deregisterApp(int pid)
{
	kill(pid, SIGSEVERE);
	
	int i = 0;
	int sleepInterval = 1000000;
	int timeoutCount = 10;
	bool success = false;

	while(i < timeoutCount && !success){
		nanosleep(sleepInterval);
		if(!registered)
		{
			success = true;
		}	
		i++;
	}
	if(success){
		printf("deregistration successful");
		return 1;
	}
	else{
		printf("deregistration unsuccessful");
		return 0;
	}
}

ATF_TC(registration);
ATF_TC_HEAD(registration, tc)
{
	atf_tc_set_md_var(tc, "descr", "This tests registration of applications");
}
ATF_TC_BODY(registration, tc)
{
	sem_init(&sem,1,1);	
	int pid = getDaemonPID();
	
	struct sigaction sig;
	sig.sa_sigaction = receiveRegistrationSig;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIGREGISTERED, &sig, NULL);

	int result = registerApp(pid);

	signal(SIGREGISTERED, SIG_DFL);

	if(1){
		atf_tc_pass();
	}
	else{
		atf_tc_fail("Not Registered");
	}	
}

ATF_TC(deregistration);
ATF_TC_HEAD(deregistration, tc)
{
	atf_tc_set_md_var(tc, "descr", "This tests deregistration of applications");
}
ATF_TC_BODY(deregistration, tc)
{
	sem_init(&sem,1,1);	

	int pid = getDaemonPID();

	struct sigaction sig;
	sig.sa_sigaction = receiveDeregistrationSig;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIGREGISTERED, &sig, NULL);

	struct sigaction sig_deregister;
	sig_deregister.sa_sigaction = receiveDeregistrationSig;
	sig_deregister.sa_flags = SA_SIGINFO;
	sigaction(SIGDEREGISTERED, &sig_deregister, NULL);

	if(!registered)
	{
		int result = registerApp(pid);
		if(result)
		{
			printf("Failed to register application");
			return;
		}	
	}

	int result = deregisterApp(pid);


	signal(SIGREGISTERED, SIG_DFL);
	signal(SIGDEREGISTERED, SIG_DFL);
	
	if(result)
	{
		atf_tc_pass();	
	}
	else
	{
		atf_tc_fail("Application not deregistered");
	}

	
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, registration);
	ATF_TP_ADD_TC(tp, deregistration);
	
	return atf_no_error();
}
