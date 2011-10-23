/*
 *
 *  rand & srand implementation for chameleon by Cadet-petit Armel <armelcadetpetit@gmail.com>
 */

#include "libsa.h"

static long holdrand = 1L;
#define	RAND_MAX	0x7fffffff

void srand (unsigned int seed)
{
	holdrand = (long)seed;
}

int rand (void)
{	
	holdrand = holdrand * 214013L + 2531011L;
	return ((holdrand >> 16) & RAND_MAX);
}