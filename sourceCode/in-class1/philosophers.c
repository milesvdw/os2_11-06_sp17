/*******************************************************************************
* Team     : 11-06
* Term     : Spring 2017
* Class    : CS444
* Names    : Alex Wood, Miles Van de Wetering, Arman Hastings
* File Name: main.c
* Purpose  : This file implements the solution to the producer consumer problem
* Citations: 
******************************************************************************/
#include "philosophers.h"
#define true 1
#define false 0

void* philosopher(void*);
int gen_rand(int, int);
void seed_rand();

pthread_mutex_t f1;
pthread_mutex_t f2;
pthread_mutex_t f3;
pthread_mutex_t f4;
pthread_mutex_t f5;

pthread_mutex_t forks[5];

void Initialize(const int32_t seed);
static void Twist();
int32_t ExtracU32();

int main(int argc, char *argv)
{
	seed_rand();

        if (pthread_mutex_init(&f1, NULL) != 0)
        {
                printf("\n mutex init failed\n");
                return 1;
        }

        if (pthread_mutex_init(&f2, NULL) != 0)
        {
                printf("\n mutex init failed\n");
                return 1;
        }

        if (pthread_mutex_init(&f3, NULL) != 0)
        {
                printf("\n mutex init failed\n");
                return 1;
        }
        if (pthread_mutex_init(&f4, NULL) != 0)
        {
                printf("\n mutex init failed\n");
                return 1;
        }
        if (pthread_mutex_init(&f5, NULL) != 0)
        {
                printf("\n mutex init failed\n");
                return 1;
	}        

	forks[0] = f1;
	forks[1] = f2;
	forks[2] = f3;
	forks[3] = f4;
	forks[4] = f5;

	pthread_t degas;
	pthread_t plato;
	pthread_t aristotle;
	pthread_t kant;
	pthread_t socrates;

	//spin up the threads
	int create_err = pthread_create(&degas, NULL, &philosopher, (void*)1);
	if (create_err != 0)
            printf("\ncan't create degas thread :[%s]", strerror(create_err));

	create_err = pthread_create(&plato, NULL, &philosopher, (void*)2);
        if (create_err != 0)
            printf("\ncan't create plato thread :[%s]", strerror(create_err));
        create_err = pthread_create(&aristotle, NULL, &philosopher, (void*)3);
        if (create_err != 0)
            printf("\ncan't create aristotle thread :[%s]", strerror(create_err));
        create_err = pthread_create(&kant, NULL, &philosopher, (void*)4);
        if (create_err != 0)
            printf("\ncan't create kant :[%s]", strerror(create_err));
        create_err = pthread_create(&socrates, NULL, &philosopher, (void*)5);
        if (create_err != 0)
            printf("\ncan't create socrates thread :[%s]", strerror(create_err));



	//collect threads and clear out memory
	pthread_join(plato, NULL);
	pthread_join(degas, NULL);
        pthread_join(aristotle, NULL);
        pthread_join(kant, NULL);
        pthread_join(socrates, NULL);

	
	return 0;

}

void seed_rand() {
	#ifdef __i386__
	srand(time(NULL));
	#else
	//Mersenne twister alternative
	uint32_t seed = time(NULL);
	 Initialize(seed);
	#endif
}

//create a random number between floor and ceiling, inclusive
int gen_rand(int floor, int ceiling) {
	#ifdef __i386__
	return int(rand() % ((ceiling-floor)+1)) + floor;
	#else
	//Mersenne twister alternative
	//return 5;
	int tmpInt;
	tmpInt = ExtracU32();

	tmpInt = tmpInt % ((ceiling - floor) + 1) + floor; 
	#endif
}

/******************************************************************************
* Function: consumer()
* Purpose:
* Dependancies:
* parameters:
* Output:
******************************************************************************/
char* phils[5] = {"Aristotle", "Socrates", "Kant", "Degas", "Plato"};
void* philosopher(void* input)
{
	int pos = (int) input;

	printf("%s started thinking\n", phils[pos-1]);
	while(true) {
	//thinking
	int thinkTime = gen_rand(1, 26);
	sleep(thinkTime);
	printf("%s finished thinking\n", phils[pos-1]);
	//eating
		while(true) {
			//try to pull an item from the buffer
			pthread_mutex_lock(&forks[pos]);
			if (pthread_mutex_trylock(&forks[(pos+1)%6]) == 0) {
				//do eating stuff
				printf("%s started eating\n", phils[pos-1]);
				int eatTime = gen_rand(2, 9);
				sleep(eatTime);
				printf("%s finished eating, started thinking\n", phils[pos-1]);
				pthread_mutex_unlock(&forks[pos]);
				pthread_mutex_unlock(&forks[(pos+1)%6]);
				break;
			} else {
				pthread_mutex_unlock(&forks[pos]);
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
					// q
					// q
					// q
					// q
					// q
					// q
					// q
					// q
					// q
					// q
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
