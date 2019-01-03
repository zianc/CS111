
// NAME: Haiying Huang
// EMAIL: hhaiying1998@outlook.com
// ID: 804757410

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include "SortedList.h"

SortedList_t* head; // a shared sorted list
SortedListElement_t* repo; // a shared repository of sorted list elements

int num_iters, num_threads;
int opt_yield = 0;
int yield_flag;

int opt_lock_m, opt_lock_s;
pthread_mutex_t lock_m;  // a mutex lock
int lock_s; // a spin lock using builtin memory function


// a helper function that reports an error message and exit 1
void report_error_and_exit(const char* msg)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

void sighandler(int signum)  // catch the SIGPIPE fault which might occur at the final poll
{
	if (signum == SIGSEGV)
	{	
		fprintf(stderr, "Find corrupted list!\n");
		exit(2);
	}
}

void* myThreadFunc(void* vargp)
{
	int n = *((int*) vargp);
	int i;

	for (i = n * num_iters ; i < (n + 1) * num_iters; ++i)
	{
		if (opt_lock_m)
			pthread_mutex_lock(&lock_m);
		else if (opt_lock_s)
		{
			while (__sync_lock_test_and_set(&lock_s, 1))
				while (lock_s);
		}

		SortedList_insert(head, &repo[i]);

		if (opt_lock_m)
			pthread_mutex_unlock(&lock_m);
		else if (opt_lock_s)
			__sync_lock_release(&lock_s);

	}

	if (opt_lock_m)
			pthread_mutex_lock(&lock_m);
	else if (opt_lock_s)
	{
		while (__sync_lock_test_and_set(&lock_s, 1))
			while (lock_s);
	}

	if (SortedList_length(head) < 0)
	{
		fprintf(stderr, "Find corrupted list!\n");
		exit(2);
	}

	if (opt_lock_m)
		pthread_mutex_unlock(&lock_m);
	else if (opt_lock_s)
		__sync_lock_release(&lock_s);


	for (i = n * num_iters ; i < (n + 1) * num_iters; ++i)
	{
		if (opt_lock_m)
			pthread_mutex_lock(&lock_m);
		else if (opt_lock_s)
		{
			while (__sync_lock_test_and_set(&lock_s, 1))
				while (lock_s);
		}

		SortedListElement_t* target_to_del = SortedList_lookup(head, repo[i].key);

		if (! target_to_del)
		{
			fprintf(stderr, "Find corrupted list!\n");
			exit(2);
		}

		if (SortedList_delete(target_to_del) == 1)
		{
			fprintf(stderr, "Find corrupted list!\n");
			exit(2);
		}

		if (opt_lock_m)
			pthread_mutex_unlock(&lock_m);
		else if (opt_lock_s)
			__sync_lock_release(&lock_s);


	}
	return NULL;
}
// the subroutine that each thread runs

void gen_random_str(char* buf, const int len) {

    static const char dict[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int i;
    for (i = 0; i < len; ++i) {
        buf[i] = dict[rand() % strlen(dict)];
    }
    buf[len] = '\0';
}

int opt_threads, opt_iters;

int main(int argc, char* argv[])
{
	num_threads = 1;
	num_iters = 1;

	opt_threads = 0;
	opt_iters = 0;
	opt_lock_m = 0;
	opt_lock_s = 0;
	yield_flag = 0;
	char* yield_arg;
	char* usage = "Correct usage: lab2_list [--threads][--iterations][--yield][--sync]\n";

	// get the optional arguments
	static const struct option long_options[] = 
	{
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
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
				break; // for --iterations flag
			case 'y':
				yield_flag = 1;
				yield_arg = optarg;
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
			default:    //case '?':
				fprintf(stderr, "%s\n", usage);
				exit(1);  // for unrecognized argument
		}

	}

	if (yield_flag)
	{
		int i;
		for (i = 0; i < (int)strlen(yield_arg); ++i)
			if (yield_arg[i] == 'i')
				opt_yield |= INSERT_YIELD;
			else if (yield_arg[i] == 'd')
				opt_yield |= DELETE_YIELD;
			else if (yield_arg[i] == 'l')
				opt_yield |= LOOKUP_YIELD;
	}

	if (opt_lock_m)
		pthread_mutex_init(&lock_m, NULL);
	// if the add function is protected by a mutex

	if (opt_lock_s)
		lock_s = 0;
	// initially no thread holds the spin lock

	const int num_elements = num_threads * num_iters;

	head = (SortedList_t *) malloc(sizeof(SortedList_t)); 
	// init an empty list
	head->key = NULL;
	head->prev = head; 
	head->next = head;

	// create and initialize the required number of elements
	// srand(time(NULL));
	repo = (SortedListElement_t*) malloc(num_elements * sizeof(SortedListElement_t));
	int i, len;
	for (i = 0; i < num_elements; ++i)
	{
		len = rand() % 10;
		char* key = (char*) malloc((len + 1) * sizeof(char));
		gen_random_str(key, len);
		repo[i].key = key;
	}

	long long elapse;
	long long billion = 1000000000;
	struct timespec start, end;
	pthread_t* threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
	int* nums = (int*) malloc(num_threads * sizeof(int));

	signal(SIGSEGV, sighandler);

	if (clock_gettime(CLOCK_REALTIME, &start) < 0)
		report_error_and_exit("Error getting clock time");

	for (i = 0; i < num_threads; ++i)
	{
		nums[i] = i;
		if (pthread_create(threads + i, NULL, myThreadFunc, (void*)(nums + i)) != 0)
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

	int final_len = SortedList_length(head);
	if (final_len != 0)
	{
		fprintf(stderr, "Find corrupted list!\n");
		exit(2);
	}

	for (i = 0; i < num_elements; ++i)
		free((void*)repo[i].key);

	free(repo);
	free(threads);
	free(nums);

	char name[20];
	strcpy(name, "list-");
	if (! opt_yield)
		strcat(name, "none");
	else
	{
		if (opt_yield & INSERT_YIELD)
			strcat(name, "i");
		if (opt_yield & DELETE_YIELD)
			strcat(name, "d");
		if (opt_yield & LOOKUP_YIELD)
			strcat(name, "l");
	}
	if (opt_lock_m)
		strcat(name, "-m");
	else if (opt_lock_s)
		strcat(name, "-s");
	else 
		strcat(name, "-none");

	long long num_op = 3 * (long long) num_threads * (long long) num_iters;
	long long time_per_op = elapse / num_op;

	printf("%s,%d,%d,1,%lld,%lld,%lld\n", name, num_threads, num_iters, num_op, elapse, time_per_op);
	exit(0);

}
