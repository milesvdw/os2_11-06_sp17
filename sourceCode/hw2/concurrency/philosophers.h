/*******************************************************************************
* Team     : 11-06
* Term     : Spring 2017
* Class    : CS444
* Names    : Alex Wood, Miles Van de Wetering, Arman Hastings
* File Name: main.h
* Purpose  : Header file for solution to the producer consumer problem
* Citations:
******************************************************************************/
#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>			//For Mersenne Twister
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#ifdef __i386__
#include <rdrand.h>
#endif


typedef struct myBuff
{
	int value;
	int waitTime;
}myBuff;

#endif
