/*******************************************************************************
* Team     : 11-06
* Term     : Spring 2017
* Class    : CS444
* Names    : Alex Wood, Miles Van de Wetering, Arman Hastings
* File Name: main.c
* Purpose  : This file implements the solution to the producer consumer problem
* Citations: 
******************************************************************************/
#include "main.h"
#define true 1
#define false 0

void *deleter(void *);
void *inserter(void *);
void *searcher(void *);
int gen_rand(int, int);
void seed_rand();

pthread_mutex_t status_lock;

int n_searchers = 0;
int n_deleters = 0;
int n_inserters = 0;
int deleters_waiting = 0;
void Initialize(const int32_t seed);
static void Twist();
int32_t ExtracU32();

struct node *list;

int main(int argc, char *argv[])
{
	seed_rand();


	int i;
	int deleterCount = 1;
	int inserterCount = 1;
	int searcherCount = 1;
	list = malloc(sizeof(struct node));
	list->next = list;
	list->prev = list;
	//command line processing code suggested by http://courses.cms.caltech.edu/cs11/material/c/mike/misc/cmdline_args.html
	for (i = 1; i < argc; i++)  /* Skip argv[0] (program name). */
    	{
        	if (strcmp(argv[i], "-d") == 0)  /* Process optional arguments. */
        	{
			deleterCount = (int)strtol(argv[i+1], NULL, 10);
			i++;
        	} else if (strcmp(argv[i], "-i") == 0)
		{
			inserterCount = (int)strtol(argv[i+1], NULL, 10);
			i++;
		} else if (strcmp(argv[i], "-s") == 0)
		{
			searcherCount = (int)strtol(argv[i+1], NULL, 10);
			i++;
		} else if (strcmp(argv[0], argv[i]) != 0) {
			printf("usage is: %s [-d numDeleters] [-i numInserters] [-s numSearchers]", argv[0]);
			return; //abort due to incorrect usage
		}
    	}

	pthread_t *deleter_ids = malloc(sizeof(pthread_t)*deleterCount);
	pthread_t *inserter_ids = malloc(sizeof(pthread_t)*inserterCount);
	pthread_t *searcher_ids = malloc(sizeof(pthread_t)*searcherCount);

	//initialize the mutex and buffer
	if (pthread_mutex_init(&status_lock, NULL) != 0)
    	{
        	printf("\n mutex init failed\n");
     		return 1;
    	}

	for (i = 0; i < deleterCount; i++) {	
		//spin up the threads
		int create_deleter_err = pthread_create(&deleter_ids[i], NULL, &deleter, NULL);
		if (create_deleter_err != 0)
	        	printf("\ncan't create deleter thread :[%s]", strerror(create_deleter_err));
	}

	for (i = 0; i < inserterCount; i++) {
		int create_inserter_err = pthread_create(&inserter_ids[i], NULL, &inserter, NULL);
	        if (create_inserter_err != 0)
	        	printf("\ncan't create inserter thread :[%s]", strerror(create_inserter_err));
	}
        for (i = 0; i < searcherCount; i++) {
                int create_searcher_err = pthread_create(&searcher_ids[i], NULL, &searcher, NULL);
                if (create_searcher_err != 0)
                        printf("\ncan't create searcher thread :[%s]", strerror(create_searcher_err));
        }

	
	
	//collect threads and clear out memory
	for (i = 0; i < inserterCount; i ++)
		pthread_join(inserter_ids[i], NULL);
	for (i = 0; i < deleterCount; i ++)
		pthread_join(deleter_ids[i], NULL);
	for (i = 0; i < searcherCount; i ++)
		pthread_join(searcher_ids[i], NULL);
	pthread_mutex_destroy(&status_lock);
	free(deleter_ids);
	free(searcher_ids);
	free(inserter_ids);
	return 0;

}

void seed_rand() {
	
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;

	char vendor[13];
	
	eax = 0x01;

	__asm__ __volatile__(
	                     "cpuid;"
	                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
	                     : "a"(eax)
	                     );
	
	if(ecx & 0x40000000){
		//use rdrand
		srand(time(NULL));
	}
	else{
		//use mt19937
		uint32_t seed = time(NULL);
		Initialize(seed);
	}

}

//create a random number between floor and ceiling, inclusive
int gen_rand(int floor, int ceiling) {
	
	
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;

	char vendor[13];
	
	eax = 0x01;

	__asm__ __volatile__(
	                     "cpuid;"
	                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
	                     : "a"(eax)
	                     );
	
	if(ecx & 0x40000000){
		//use rdrand
			srand(time(NULL));
			int tmp = (rand() % ((ceiling-floor)+1)) + floor;
			return tmp;
	}
	else{
		//use mt19937
		int tmpInt;
		tmpInt = ExtracU32();
		return tmpInt = tmpInt % ((ceiling - floor) + 1) + floor; 
	}

}

void* inserter(void *ignore)
{
	while(true) {
		struct node *newNode = malloc(sizeof(struct node));
		newNode->value = gen_rand(0, 20);
		
		int insert_time = gen_rand(1, 4);
		//try to push the item to the list
		while(true) {
			pthread_mutex_lock(&status_lock);
			if(n_deleters > 0 || n_inserters > 0) {
				//bad time
				pthread_mutex_unlock(&status_lock);
				sleep(1);
			} else {
				n_inserters = 1;
				pthread_mutex_unlock(&status_lock);
				break;
			}
		}
		printf("\tinserting item with value %d, this will take %d seconds\n", newNode->value, insert_time);
		sleep(insert_time);
		newNode->prev = list->prev;
		newNode->next = list;
		list->prev->next = newNode;
		list->prev = newNode;
		printf("\tinserted item with value %d\n\n", newNode->value);
		pthread_mutex_lock(&status_lock);
		n_inserters = 0;
		pthread_mutex_unlock(&status_lock);
		
		int wait_time = gen_rand(3, 7);
		sleep(wait_time);
	}
}

void* searcher(void *ignore)
{
	while(true) {
		int searchVal = gen_rand(1, 20);
		
		while(true) {
			pthread_mutex_lock(&status_lock);
			if(n_deleters > 0 || deleters_waiting > 0) {
				//bad time
				pthread_mutex_unlock(&status_lock);
				sleep(1);
			} else {
				n_searchers = n_searchers + 1;
				pthread_mutex_unlock(&status_lock);
				break;
			}
		}
		int searchTime = gen_rand(1, 2);
		printf("\t\tsearching for an item with value %d, this will take %d seconds\n", searchVal, searchTime);
		sleep(searchTime);
		struct node *curNode = list;
		int found = false;
		while(curNode->next != list) {
			curNode = curNode->next;
			if(curNode->value == searchVal) {
				printf("\t\tfound value %d in list\n\n", curNode->value);
				found = true;
				break;
			}
		}
		if(!found) printf("\t\tunable to find value %d in list\n\n", searchVal);
		pthread_mutex_lock(&status_lock);
		n_searchers = n_searchers-1;
		pthread_mutex_unlock(&status_lock);
		sleep(gen_rand(1, 4));
	}
}

void* deleter(void *ignore)
{
	while(true) {
		int deleteVal = gen_rand(1, 20);
		while(true) {
			pthread_mutex_lock(&status_lock);
			if(n_deleters > 0 || n_inserters > 0 || n_searchers > 0) {
				deleters_waiting += 1;
				pthread_mutex_unlock(&status_lock);
				sleep(1);
			} else {
				n_deleters = 1;
				deleters_waiting = deleters_waiting -1;
				pthread_mutex_unlock(&status_lock);
				break;
			}
		}
		int deleteTime = gen_rand(1, 6);
		printf("deleting an item with value %d, this will take %d seconds\n", deleteVal, deleteTime);
		sleep(deleteTime);
		int found = false;
		struct node *curNode = list;
		while(curNode->next != list) {
			curNode = curNode->next;
			if(curNode->value == deleteVal) {
				printf("deleted value %d in list\n\n", deleteVal);
				found = true;
				break;
			}
		}
		if(!found) printf("unable to delete value %d in list\n\n", deleteVal);
		pthread_mutex_lock(&status_lock);
		n_deleters = 0;
		pthread_mutex_unlock(&status_lock);
		sleep(gen_rand(1, 20));
	}	
}

//Mersenne Twister MT19937 constants for 32-bit range
enum
{
	//Coefficiants for MT 19937-64
	N = 624,			// degree of recurrence
	M = 397,			// middle word, an offset used in 
					// recurrence relation 
					// defining series x, 1<=m<=n
	R = 31,				// Seperation point of one word, or 
					// number of bits of the lower bit mask,
					// 0<= r <= w - 1
	A = 0x9908b0df,			//Coefficients of the rational normal
       					//form twist matrix
	F = 1812433253,			//Value for MT19937 for 32-bit
	U = 11,				//Additional Mersenne Twister tempering
       					//bit shifts/masks
					//Twisted Generalised Feedback Shift
					// Register = TGFSR 
					//(R) = Rational form
	S = 7,				//TGFSR(R) tempering bit shifts
	B = 0x9d2c5680,			//TGSFR(R) tempering bitmasks
	T = 15,				//TGSFR(R) tempering bit shifts 
	C = 0xefc60000,			//TGSFR(R) tempering bitmasks
	L = 18,				//TGSFR(R) tempering bit shifts 

	MASK_LOWER = (1ull << R) - 1,
	MASK_UPPER = (1ull << R)
};

//Global variables for mt19937
static uint32_t mt[N];
static uint16_t index;

//Function to re-initialize with new seed
void Initialize(const int32_t seed)
{
	int32_t i;

	mt[0] = seed;

	for (i = 1; i < N; i++)
	{
		mt[i] = (F * (mt[i - 1] ^ (mt[i - 1] >> 30)) + i);
	}

	index = N;

}

//Function to initiate "Twist"
static void Twist()
{
	int32_t i, x, xA;

	for (i = 0; i < N; i++)
	{
		x = (mt[i] & MASK_UPPER) + (mt[(i + 1) % N] & MASK_LOWER);

		xA = x >> 1;

		if (x & 0x1)
		{
			xA ^= A;
		}

		mt[i] = mt[(i + M) % N] ^ xA;
	}

	index = 0;
}

//Get a psuedo random 32-bit unsigned integer
int32_t ExtracU32()
{
	int32_t y;
	int i = index;

	if (index >= N)
	{
		Twist();
		i = index;
	}

	y = mt[i];
	index = i + 1;

	y ^= (mt[i] >> U);
	y ^= (y << S) & B;
	y ^= (y << T) & C;
	y ^= (y >> L);

	return y;
}
