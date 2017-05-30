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

void *customer(void *);
void *barber(void *);
int gen_rand(int, int);
void seed_rand();

pthread_mutex_t status_lock;

int n_customers = 0;
int n_chairs = 0;
int *chairs;
int last_customer = 0;
int customer_to_service = -1;
int servicing_customer = 0;
void *customer_ids(void *);
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
			n_chairs = (int)strtol(argv[i+1], NULL, 10);
			i++;
		} else if (strcmp(argv[0], argv[i]) != 0) {
			printf("usage is: %s [-d numDeleters] [-i numInserters] [-s numSearchers]", argv[0]);
			return; //abort due to incorrect usage
		}
    	}

	pthread_t barber_t;
        int create_process_err = pthread_create(&barber_t, NULL, &barber, NULL);
        if (create_process_err != 0)
	        printf("\ncan't create barber thread :[%s]", strerror(create_process_err));
	

	chairs = malloc(sizeof(int)*n_chairs);
	for(i = 0; i < n_chairs; i++) chairs[i] = 0;
	pthread_t *process_ids = malloc(sizeof(pthread_t)*n_chairs);

	while(true) {
		pthread_mutex_lock(&status_lock);
		int found = false;
		for(i = 0; i < n_chairs; i ++) {
			//printf("checking chair %d\n", i);
			if (chairs[i] == -1) {
				int left = pthread_tryjoin_np(process_ids[i], NULL);
				if(left == 0) chairs[i] = 0;
			}
			if (chairs[i] == 0 && !found) {
				found = true;
				printf("New customer sitting in chair %d\n", i);
				chairs[i] = 1;
		                create_process_err = pthread_create(&process_ids[i], NULL, &customer, (void *) i);
		                if (create_process_err != 0)
        		                printf("\ncan't create customer thread :[%s]", strerror(create_process_err));
			}
		}
		if(!found) {
			printf("Customer came but saw no empty chairs, left in a huff!\n");
		}
		pthread_mutex_unlock(&status_lock);
		int time_between_arrivals = gen_rand(1, 3);
		sleep(time_between_arrivals);

	}
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

void* barber(void *arg)
{
	int i = 0;
	while(true) {
		pthread_mutex_lock(&status_lock);
		int found = false;
		int j = 0;
		i = 0;
		for(j = last_customer; j < last_customer + n_chairs; j ++) {
			i = j % n_chairs;
			if(chairs[i] == 1 && !found) { // this means waiting for haircut
				printf("Barber calling customer %d\n", i);
				last_customer = i;
				found = true; //service this customer
				customer_to_service = i;
				pthread_mutex_unlock(&status_lock);
		                printf("Barber starts giving haircut\n");
		                sleep(6);
		                printf("Barber finishes giving haircut\n");
				customer_to_service = -1;

			}
		}
		if(!found) {
			pthread_mutex_unlock(&status_lock);
			sleep(1);
			printf("Barber: zzz\n");
		}
	}
}

void* customer(void *arg) {
	int id = (int)arg;
	while(true) {
		pthread_mutex_lock(&status_lock);
		if(customer_to_service == id) {
			printf("Customer %d gets a haircut\n", id);
			chairs[id] = -1;
			pthread_mutex_unlock(&status_lock);
			sleep(6);
			printf("Customer %d leaves with great hair\n", id);
			return;
		} else pthread_mutex_unlock(&status_lock);

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
