
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
#include <time.h>
#include "SortedList.h"


SortedListElement_t* repo; // a shared repository of sorted list elements

int num_iters, num_threads;
int opt_yield = 0;
int yield_flag;

int opt_lock_m, opt_lock_s;

// declare a sublist with its own head and sync object 
int opt_lists, num_lists;
typedef struct Sublist {
	SortedList_t* head;
	int lock_s;
	pthread_mutex_t lock_m;
} Sublist_t;

// a shared sorted list divided into multiple sublists
Sublist_t* list_arr;
const long long billion = 1000000000;

// a simple hash function that hashes string into correspondent sublist
unsigned hash(const char* str, int num_lists)
{
	unsigned sum = 0;
	while (*str != '\0')
	{
		sum += *str;
		++str;
	}
	return sum % num_lists;
}
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

// collect the return value from working threads
typedef struct lock_stat
{
	long long nsec;
	int n;
} lock_stat_t;

void* myThreadFunc(void* vargp)
{
	int n = *((int*) vargp);
	int i, index;
	Sublist_t* list_curr;

	int num_locks = 0;
	long long lock_nsec = 0;
	struct timespec start, end;

	for (i = n * num_iters ; i < (n + 1) * num_iters; ++i)
	{
		index = (opt_lists) ? hash(repo[i].key, num_lists) : 0;
		list_curr = &list_arr[index];
		if (opt_lock_m)
		{	
			if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
				report_error_and_exit("Error getting clock time");
			pthread_mutex_lock(&list_curr->lock_m);
			if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
				report_error_and_exit("Error getting clock time");

			lock_nsec += billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
			num_locks += 1;

		}
		else if (opt_lock_s)
		{
			if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
				report_error_and_exit("Error getting clock time");
			while (__sync_lock_test_and_set(&list_curr->lock_s, 1))
				while (list_curr->lock_s);
			if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
				report_error_and_exit("Error getting clock time");

			lock_nsec += billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
			num_locks += 1;
		}

		SortedList_insert(list_curr->head, &repo[i]);

		if (opt_lock_m)
			pthread_mutex_unlock(&list_curr->lock_m);
		else if (opt_lock_s)
			__sync_lock_release(&list_curr->lock_s);

	}

	int length, sum;
	for (i = 0; i < num_lists; ++i)
	{
		list_curr = &list_arr[i];
		if (opt_lock_m)
		{
			if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
				report_error_and_exit("Error getting clock time");
			pthread_mutex_lock(&list_curr->lock_m);
			if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
				report_error_and_exit("Error getting clock time");

			lock_nsec += billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
			num_locks += 1;

		}
		else if (opt_lock_s)
		{
			if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
				report_error_and_exit("Error getting clock time");
			while (__sync_lock_test_and_set(&list_curr->lock_s, 1))
				while (list_curr->lock_s);
			if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
				report_error_and_exit("Error getting clock time");

			lock_nsec += billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
			num_locks += 1;
		}

		length = SortedList_length(list_curr->head);
		if (length < 0)
		{
			fprintf(stderr, "Find corrupted list!\n");
			exit(2);
		}

		if (opt_lock_m)
			pthread_mutex_unlock(&list_curr->lock_m);
		else if (opt_lock_s)
			__sync_lock_release(&list_curr->lock_s);
		sum += length;
	}

	for (i = n * num_iters ; i < (n + 1) * num_iters; ++i)
	{
		index = (opt_lists) ? hash(repo[i].key, num_lists) : 0;
		list_curr = &list_arr[index];

		if (opt_lock_m)
		{
			if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
				report_error_and_exit("Error getting clock time");
			pthread_mutex_lock(&list_curr->lock_m);
			if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
				report_error_and_exit("Error getting clock time");

			lock_nsec += billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
			num_locks += 1;

			//pthread_mutex_lock(&list_curr->lock_m);
		}
		else if (opt_lock_s)
		{
			if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
				report_error_and_exit("Error getting clock time");
			while (__sync_lock_test_and_set(&list_curr->lock_s, 1))
				while (list_curr->lock_s);
			if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
				report_error_and_exit("Error getting clock time");

			lock_nsec += billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);
			num_locks += 1;
			// while (__sync_lock_test_and_set(&list_curr->lock_s, 1))
				// while (list_curr->lock_s);
		}

		SortedListElement_t* target_to_del = SortedList_lookup(list_curr->head, repo[i].key);

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
			pthread_mutex_unlock(&list_curr->lock_m);
		else if (opt_lock_s)
			__sync_lock_release(&list_curr->lock_s);


	}

	lock_stat_t* temp = (lock_stat_t*) malloc(sizeof(lock_stat_t));
	temp->nsec = lock_nsec;
	temp->n = num_locks;
	return temp;
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
	opt_lists = 0;
	num_lists = 1;
	yield_flag = 0;
	char* yield_arg;
	char* usage = "Correct usage: lab2b_list [--threads][--iterations][--yield][--sync][--lists]\n";

	// get the optional arguments
	static const struct option long_options[] = 
	{
		{"threads", required_argument, NULL, 't'},
		{"iterations", required_argument, NULL, 'i'},
		{"yield", required_argument, NULL, 'y'},
		{"sync", required_argument, NULL, 's'},
		{"lists", required_argument, NULL, 'l'},
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
				}
				else if (optarg[0] == 's')
				{
					opt_lock_s = 1;
				}
				break;
			case 'l':
				opt_lists = 1;
				num_lists = atoi(optarg);
				break;
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


	const int num_elements = num_threads * num_iters;

	/*
	head = (SortedList_t *) malloc(sizeof(SortedList_t)); 
	// init an empty list
	head->key = NULL;
	head->prev = head; 
	head->next = head;
	*/
	list_arr = (Sublist_t*) malloc(num_lists * sizeof(Sublist_t));

	int i;
	for (i = 0; i < num_lists; ++i)
	{
		Sublist_t* temp = &list_arr[i];
		temp->head = (SortedList_t*) malloc(sizeof(SortedList_t));
		temp->head->key = NULL;
		temp->head->prev = temp->head;
		temp->head->next = temp->head;
		// initialize an empty list

		if (opt_lock_s)
			temp->lock_s = 0;
		// initialize a spin lock for the temp sub list
		if (opt_lock_m)
			pthread_mutex_init(&temp->lock_m, NULL);
		// initialzie a mutex lock for the temp sub list

	}

	// create and initialize the required number of elements
	// srand(time(NULL));
	repo = (SortedListElement_t*) malloc(num_elements * sizeof(SortedListElement_t));
	int len;
	for (i = 0; i < num_elements; ++i)
	{
		len = rand() % 10 + 10;
		char* key = (char*) malloc((len + 1) * sizeof(char));
		gen_random_str(key, len);
		repo[i].key = key;
	}

	long long elapse;
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

	long long total_lock_nsec = 0;
	int total_num_locks = 0;
	lock_stat_t* ret;

	for (i = 0; i < num_threads; ++i)
	{
		if (pthread_join(threads[i], (void**)&ret) != 0)
			report_error_and_exit("Error joining threads");
		total_lock_nsec += ret->nsec;
		total_num_locks += ret->n;
		free(ret);
	}

	if (clock_gettime(CLOCK_REALTIME, &end) < 0)
		report_error_and_exit("Error getting clock time");

	elapse = billion * (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec);

	int length;
	for (i = 0; i < num_lists; ++i)
	{
		length = SortedList_length(list_arr[i].head);
		if (length != 0)
		{
			fprintf(stderr, "Find corrupted list!\n");
			exit(2);
		}
	}

	for (i = 0; i < num_elements; ++i)
		free((void*)repo[i].key);

	free(repo);
	free(threads);
	free(nums);

	for (i = 0; i < num_lists; ++i)
		free(list_arr[i].head);
	free(list_arr);

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

	long long wait_for_lock = 0;
	if (opt_lock_s || opt_lock_m)
		wait_for_lock = total_lock_nsec / total_num_locks;

	printf("%s,%d,%d,%d,%lld,%lld,%lld,%lld\n", name, num_threads, num_iters, num_lists, num_op, elapse, time_per_op, wait_for_lock);
	exit(0);

}
