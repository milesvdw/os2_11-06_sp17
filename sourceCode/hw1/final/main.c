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

void* producer(void*);
void* consumer(void*);
int gen_rand(int, int);
void seed_rand();
myBuff *buffer;
int buffCount = 0;
pthread_mutex_t lock;

void Initialize(const int32_t seed);
static void Twist();
int32_t ExtracU32();

int main(int argc, char *argv)
{
	seed_rand();


	pthread_t consumer_id;
	pthread_t producer_id;

	//initialize the mutex and buffer
	if (pthread_mutex_init(&lock, NULL) != 0)
    	{
        	printf("\n mutex init failed\n");
     		return 1;
    	}
	buffer = malloc(sizeof(struct myBuff) * 32);

	//spin up the threads
	int create_consumer_err = pthread_create(&consumer_id, NULL, &consumer, NULL);
	if (create_consumer_err != 0)
            printf("\ncan't create consumer thread :[%s]", strerror(create_consumer_err));

	int create_producer_err = pthread_create(&producer_id, NULL, &producer, NULL);
        if (create_producer_err != 0)
            printf("\ncan't create producer thread :[%s]", strerror(create_producer_err));

	//collect threads and clear out memory
	pthread_join(consumer_id, NULL);
	pthread_join(producer_id, NULL);
	pthread_mutex_destroy(&lock);
	free(buffer);
	
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

/******************************************************************************
* Function: producer()
* Purpose: This function produces random numbers in the shared memory space of 
*			the pthreads in this program. When the producer is accessing shared 
*			memory, it locks the shared memory space using mutexes, then writes
*			the random number to the memory space. 
* Dependancies: Depends on the gen_rand function that uses rdrand or mersenne
*				twister.
* parameters: A void* pointer
* Output: A random number in the shared memory space for the consumer to get,
*			and consume.
******************************************************************************/
void* producer(void* thread1)
{
	while(true) {
		myBuff item;
		item.value = gen_rand(0, 10);
		item.waitTime = gen_rand(2, 9);
		
		//try to push the item to the buffer
		while(true) {
			pthread_mutex_lock(&lock);
			//if the buffer is full, unlock to prevent deadlocking
			//and wait for a bit before trying again
			if(buffCount == 32) {
				pthread_mutex_unlock(&lock);
				sleep(1);
			}
			//if there is room, push, unlock, move on
			else {
				printf("producing item with value %d\n", item.value);
				buffer[buffCount] = item;
				buffCount = buffCount + 1;
				pthread_mutex_unlock(&lock);
				break;
			}

		}
		int wait_time = gen_rand(3, 7);
		sleep(wait_time);
	}
}

/******************************************************************************
* Function: consumer()
* Purpose: The consumer function checks the shared memory space to see if there
*			is a value to consum, if there is, the consumer locks the shared mem
*			space, then reads teh character, it then generates a random number,
*			which is the time it will take the consumer to consume the value.
*			After that random wait time, it should consome the value in question
* Dependancies: gen_rand function
* parameters: void * 
* Output: Yum! when a value is consumed, and the ammount of time it will take to 
*			consume the value.
******************************************************************************/
void* consumer(void* thread2)
{
	while(true) {
		//try to pull an item from the buffer
		while(true) {
			pthread_mutex_lock(&lock);
			if(buffCount > 0) {
				myBuff item = buffer[buffCount-1];
				buffCount = buffCount - 1;
				pthread_mutex_unlock(&lock);
				printf("consuming... this will take %d seconds\n", item.waitTime);
				sleep(item.waitTime);
				printf("yum! the value was %d\n", item.value);
				pthread_mutex_unlock(&lock);
				break;
			} else {
				pthread_mutex_unlock(&lock);
				sleep(1);
			}
		}		
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
