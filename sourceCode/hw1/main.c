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

void producer();
void consumer();


int main(int argc, char *argv)
{

	

	return 0;

}

/******************************************************************************
* Function: producer()
* Purpose: 
* Dependancies: 
* parameters: 
* Output: 
******************************************************************************/
void producer()
{

}

/******************************************************************************
* Function: consumer()
* Purpose:
* Dependancies:
* parameters:
* Output:
******************************************************************************/
void consumer()
{

}

/*******************************************************************************
* Team     : 11-06
* Term     : Spring 2017
* Class    : CS444
* Names    : Alex Wood, Miles Van de Wetering, Arman Hastings
* File Name: main.c
* Purpose  : This file implements the solution to the producer consumer problem
* Citations: Mersenne Twister: https://en.wikipedia.org/wiki/Mersenne_Twister
******************************************************************************/

#include "main.h"

//Mersenne Twister MT19937 constants for 32-bit range
enum
{
	//Coefficiants for MT 19937-64
	N = 624,			//degree of recurrence
	M = 397,			//middle word, an offset used in recurrence relation 
						//defining series x, 1<=m<=n
	R = 31,				//Seperation point of one word, or number of bits of 
						//the lower bit mask, 0<= r <= w - 1
	A = 0x9908b0df,		//Coefficients of the rational normal form twist matrix
	F = 1812433253,		//Value for MT19937 for 32-bit
	U = 11,				//Additional Mersenne Twister tempering bit shifts/masks
						//Twisted Generalised Feedback Shift Register = TGFSR 
						//(R) = Rational form
	S = 7,				//TGFSR(R) tempering bit shifts
	B = 0x9d2c5680,		//TGSFR(R) tempering bitmasks
	T = 15,				//TGSFR(R) tempering bit shifts 
	C = 0xefc60000,		//TGSFR(R) tempering bitmasks
	L = 18,				//TGSFR(R) tempering bit shifts 

	MASK_LOWER = (1ull << R) - 1,
	MASK_UPPER = (1ull << R)
};

//Global variables for mt19937
static uint32_t mt[N];
static uint16_t index;

//Function to re-initialize with new seed
void Initialize(const uint32_t seed)
{
	uint32_t i;

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
	uint32_t i, x, xA;

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
uint32_t ExtracU32()
{
	uint32_t y;
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