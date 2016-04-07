#include <atf-c++.hpp>

#include "daemon.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

#define SIGTEST 44
#define SIGSEVERE 45
#define SIGMIN 46

volatile int registered = 0;

void receiveSig(int signal_number, siginfo_t *info, void *unused){
	std:: cout << "received";
}

static int getDaemonPID(){
	FILE *in = popen("pgrep daemon","r");
	int pid = 0;
	fscanf(in, "%d", &pid);
	std::cout << "daemon pid" << pid;

	return pid;
}

static int registerApp(int pid){
	kill(pid, SIGSEVERE);

	int i = 0;
	struct timespec sleepInterval;
	sleepInterval.tv_sec = 1;
	sleepInterval.tv_nsec = 0;

	int timeoutCount = 10;
	bool success = false;

	while(i < timeoutCount && !success){
		nanosleep(&sleepInterval, 0);
		if(isRegistered(pid))
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

static int deregisterApp(int pid){
	kill(pid, SIGSEVERE);

	int i = 0;
	struct timespec sleepInterval;
	sleepInterval.tv_sec = 1;
	sleepInterval.tv_nsec = 0;

	int timeoutCount = 10;
	bool success = false;

	while(i < timeoutCount && !success){
		nanosleep(&sleepInterval, 0);
		if(!isRegistered(pid))
		{
			success = true;
		}	
		i++;
	}
	if(success){
		printf("deregistration succeesful");
		return 1;
	}
	else{
		printf("deregistration unsuccessful");
		return 0;
	}
}

ATF_TEST_CASE(registration);
ATF_TEST_CASE_HEAD(registration)
{
	set_md_var("descr", "This tests registration of applications");
}
ATF_TEST_CASE_BODY(registration)
{
	std::fstream outFile("outFile");	

	outFile << "here" << "/n";	
	
	int pid = getDaemonPID();

	outFile << "pid" << pid << "/n";	
	struct sigaction sig;
	sig.sa_sigaction = receiveSig;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIGSEVERE, &sig, NULL);

	int result = registerApp(pid);

	outFile << "result" << result << "/n";	

	//signal(SIGREGISTERED, SIG_DFL);

	ATF_REQUIRE(result == 1);

	if(result){
		ATF_PASS();
	}
	else{
		ATF_FAIL("Not Registered");
	}	
}

ATF_TEST_CASE(deregistration);
ATF_TEST_CASE_HEAD(deregistration)
{
	set_md_var("descr", "This tests deregistration of applications");
}
ATF_TEST_CASE_BODY(deregistration)
{
	int pid = getDaemonPID();

	struct sigaction sig;
	sig.sa_sigaction = receiveSig;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIGSEVERE, &sig, NULL);

	int result = registerApp(pid);

	if(result == 0){
		ATF_FAIL("Registration Failed");	
	}	

	result = deregisterApp(pid);

	if(result){
		ATF_PASS();
	}
	else{
		ATF_FAIL("Not Deregistered");
	}	
}

ATF_INIT_TEST_CASES(tcs)
{
	ATF_ADD_TEST_CASE(tcs, registration);
	ATF_ADD_TEST_CASE(tcs, deregistration);
}
