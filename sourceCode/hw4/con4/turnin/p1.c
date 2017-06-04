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

void *process(void *);
int gen_rand(int, int);
void seed_rand();

pthread_mutex_t status_lock;

int n_users = 0;
void *process_ids(void *);
int process_count;
int locked = 0;
void Initialize(const int32_t seed);
static void Twist();
int32_t ExtracU32();

int main(int argc, char *argv[])
{
	seed_rand();


	int i;
	//command line processing code suggested by http://courses.cms.caltech.edu/cs11/material/c/mike/misc/cmdline_args.html
	for (i = 1; i < argc; i++)  /* Skip argv[0] (program name). */
    	{
        	if (strcmp(argv[i], "-n") == 0)  /* Process optional arguments. */
        	{
			process_count = (int)strtol(argv[i+1], NULL, 10);
			i++;
		} else if (strcmp(argv[0], argv[i]) != 0) {
			printf("usage is: %s [-d numDeleters] [-i numInserters] [-s numSearchers]", argv[0]);
			return; //abort due to incorrect usage
		}
    	}

	pthread_t *process_ids = malloc(sizeof(pthread_t)*process_count);

	//initialize the mutex and buffer
	if (pthread_mutex_init(&status_lock, NULL) != 0)
    	{
        	printf("\n mutex init failed\n");
     		return 1;
    	}

	for (i = 0; i < process_count; i++) {	
		//spin up the threads
		int create_process_err = pthread_create(&process_ids[i], NULL, &process, (void *) i);
		if (create_process_err != 0)
	        	printf("\ncan't create process thread :[%s]", strerror(create_process_err));
	}

	for (i = 0; i < process_count; i ++)
		pthread_join(process_ids[i], NULL);
	pthread_mutex_destroy(&status_lock);
	free(process_ids);
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

void* process(void *arg)
{
	int id = (int) arg;
	while(true) {
		int wait_time = gen_rand(1, 10);
		int use_time = gen_rand(3, 5);
		//try to do stuff 
		pthread_mutex_lock(&status_lock);
		if(locked) {
			printf("process %d tried to log in but resource was locked. trying again in %d seconds\n", id, wait_time);
			pthread_mutex_unlock(&status_lock);
		}
		else {
			switch(n_users) {
				case 0:
				case 1:
					n_users = n_users + 1;
					printf("process %d logging in to shared resource for %d seconds\n", id, use_time);
					pthread_mutex_unlock(&status_lock);
					sleep(use_time);
					pthread_mutex_lock(&status_lock);
					n_users = n_users - 1;
					if(n_users == 0)  {
                                                if(locked) {
                                                        locked = false;
                                                        printf("LAST USER GONE, UNLOCKING RESOURCE\n");
                                                }
                                        }
					pthread_mutex_unlock(&status_lock);
					break;
				case 2:
					n_users = n_users + 1;
					printf("process %d logging in to shared resource for %d seconds\n", id, use_time);
					printf("MAXIMUM PROCESSES REACHED, LOCKING UNTIL EMPTY\n");
					locked = true;
                       		        pthread_mutex_unlock(&status_lock);
					sleep(use_time);
					pthread_mutex_lock(&status_lock);
					n_users = n_users - 1;
					if(n_users == 0) {
						if(locked) {
							locked = false;
							printf("LAST USER GONE, UNLOCKING RESOURCE\n");
						}
					}
					pthread_mutex_unlock(&status_lock);
					break;
				defaut:
					printf("World ending now. Explode.\n");
			}
		}
		
		sleep(wait_time);
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
