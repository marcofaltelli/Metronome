

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/prctl.h>
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>


#ifdef _MSC_VER
#include <intrin.h> /* for rdtscp and clflush */
#pragma optimize("gt",on)
#else
#include <x86intrin.h> /* for rdtscp and clflush */
#endif


#define NUM_TRIES 1000
#define TRIES 1

float scale = 2400; //number of clock cycles per microsecond (2800 is for a 2.8GHz processor)

int fd = -1;
static inline void hr_sleep(long int x, long y){
    ioctl(fd, 0, y);
}


int main(int argc, char** argv){
	
	long int arg;	
	unsigned long timeout;
	int i;
	unsigned long cumulative, time1, time2;
	int junk = 0;


	if(argc < 3){
		printf("usage: prog sys_call-num sleep_timeout\n");
		return EXIT_FAILURE;
	}	
	
	
    fd = open("/dev/hrsleep", 0);
    if (fd < 0) {
        printf("Could not open /dev/hrsleep : %d\n", fd);
        return;
    }
	timeout = strtol(argv[2],NULL,10);
	arg = strtol(argv[1],NULL,10);
	
//	prctl(PR_SET_TIMERSLACK, 1);	
	printf("expected number of clock cycles per %lu nanoseconds timeout is %lu\n",timeout,(unsigned long)(scale*timeout)/1000);

	cumulative = 0;
	time1 = __rdtscp( & junk);
	for (i=0; i<NUM_TRIES; i++){
		hr_sleep(arg,timeout);//timeout is nanosecs
	}
	time2 = __rdtscp( & junk);
	cumulative += time2 - time1;
	printf("hr_sleep average sleep cycles:\t%lu\n",cumulative/(NUM_TRIES*TRIES));

	struct timespec time;
	time.tv_nsec = (long) timeout;
	time.tv_sec = 0;

	cumulative = 0;
	time1 = __rdtscp( & junk);
	for (i=0; i<NUM_TRIES; i++){
		nanosleep(&time, NULL);//timeout is nanosecs
	}
	time2 = __rdtscp( & junk);
	cumulative += time2 - time1;
	printf("nanosleep average sleep cycles:\t%lu\n",cumulative/(NUM_TRIES*TRIES));
	return 0;
}
