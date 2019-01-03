
// NAME: Haiying Huang
// EMAIL: hhaiying1998@outlook.com
// ID: 804757410
//
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

long long counter;
// a shared counter

int opt_lock_m, opt_lock_s, opt_lock_c; // specify what type of lock
pthread_mutex_t lock_m;  // a mutex lock
int lock_s; // a spin lock using builtin memory function

int opt_yield;

// a helper function that reports an error message and exit 1
void report_error_and_exit(const char* msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

void add(long long *pointer, long long value) {

	if (opt_lock_m)
	{
		pthread_mutex_lock(&lock_m);
		long long sum = *pointer + value;
    	if (opt_yield)
        	sched_yield();
    	*pointer = sum;      
    	// the critical section
		pthread_mutex_unlock(&lock_m);
	}


	else if (opt_lock_s)
	{
		while (__sync_lock_test_and_set(&lock_s, 1))
			while (lock_s);

		long long sum = *pointer + value;
    	if (opt_yield)
        	sched_yield();
    	*pointer = sum;      // the critical section

		__sync_lock_release(&lock_s);

	}

	else if (opt_lock_c)
	{
		long long oldVal, sum;
		do
		{
			oldVal = *pointer; // save the counter previous value
			sum = oldVal + value; // find the sum based on the previous value
			if (opt_yield)
				sched_yield();
		} while ( __sync_val_compare_and_swap(pointer, oldVal, sum) != oldVal); // update only when counter is still equal to previous value

	}

	else
	{
	    long long sum = *pointer + value;
	    if (opt_yield)
	        sched_yield();
	    *pointer = sum;

	}
}

void* myThreadFunc(void* vargp)
{
	int n = *((int*) vargp);
	int i;
	for (i = 0; i < n; ++i)
	{
		add(&counter, 1); // the critical section

	}
	// increment the shared counter n times
	for (i = 0; i < n; ++i)
	{
		add(&counter, -1);  // the critical section
	}
	// decrement the shared counter n times
	return NULL;
}
// the subroutine that each thread runs


int opt_threads, opt_iters; 

int main(int argc, char* argv[])
{
	int num_threads = 1;
	int num_iters = 1;
	counter = 0;

	opt_threads = 0;
	opt_iters = 0;
	opt_yield = 0;
	opt_lock_m = 0;
	opt_lock_s = 0;
	opt_lock_c = 0;

	char* usage = "Correct usage: lab2-add [--threads][--iterations][--yield][--sync]\n";

	// get the optional arguments
	static const struct option long_options[] = 
	{
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", no_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{0, 0, 0, 0}
	};

	char c;
	while (1)
	{
		int option_index = 0;  // store the index of the next option we find
		c = getopt_long(argc, argv, "", long_options, &option_index); // get the next option

		if (c == -1)
			break;

		switch(c)
		{
			case 't':      
				opt_threads = 1;
				num_threads = atoi(optarg);
				break;  // for --threads flag
			case 'i':
				opt_iters = 1;
				num_iters = atoi(optarg);
				break;
			case 'y':
				opt_yield = 1;
				break;  // for --yield flag
			case 's':
				if (optarg[0] == 'm') 
				{
					opt_lock_m = 1;
					break;
				}
				else if (optarg[0] == 's')
				{
					opt_lock_s = 1;
					break;
				}
				else if (optarg[0] == 'c')
				{
					opt_lock_c = 1;
					break;
				}
			default:    //case '?':
				fprintf(stderr, "%s\n", usage);
				exit(1);  // for unrecognized argument
		}

	}

	if (opt_lock_m)
		pthread_mutex_init(&lock_m, NULL);
	// if the add function is protected by a mutex

	if (opt_lock_s)
		lock_s = 0;
	// initially no thread holds the spin lock

	long long elapse;
	long long billion = 1000000000;
	struct timespec start, end;
	pthread_t* threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));

	if (clock_gettime(CLOCK_REALTIME, &start) < 0)
		report_error_and_exit("Error getting clock time");

	int i;
	for (i = 0; i < num_threads; ++i)
	{
		if (pthread_create(threads + i, NULL, myThreadFunc, (void*)&num_iters) != 0)
			report_error_and_exit("Error creating threads");
	}

	for (i = 0; i < num_threads; ++i)
	{
		if (pthread_join(threads[i], NULL) != 0)
			report_error_and_exit("Error joining threads");
	}

	if (clock_gettime(CLOCK_REALTIME, &end) < 0)
		report_error_and_exit("Error getting clock time");

	elapse = billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

	free(threads);

	char name[20];
	strcat(name, "add");
	if (opt_yield)
		strcat(name, "-yield");

	if (opt_lock_m)
		strcat(name, "-m");
	else if (opt_lock_s)
		strcat(name, "-s");
	else if (opt_lock_c)
		strcat(name, "-c");
	else
		strcat(name, "-none");

	long long num_op = 2 * (long long) num_threads * (long long) num_iters;
	long long time_per_op = elapse / num_op;
	// printf("Take %lld nanoseconds to run %d threads, and the counter is %d\n", elapse, num_threads, counter);
	printf("%s,%d,%d,%lld,%lld,%lld,%lld\n", name, num_threads, num_iters, num_op, elapse, time_per_op, counter);
	exit(0);

}
