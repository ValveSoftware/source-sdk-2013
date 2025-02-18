//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic random number generator
//
// $NoKeywords: $
//===========================================================================//


#include <time.h>
#include "Random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)

static long idum = 0;

void SeedRandomNumberGenerator(long lSeed)
{
	if (lSeed)
	{
		idum = lSeed;
	}
	else
	{
		idum = -time(NULL);
	}
	if (1000 < idum)
	{
		idum = -idum;
	}
	else if (-1000 < idum)
	{
		idum -= 22261048;
	}
}

long ran1(void)
{
	int j;
	long k;
	static long iy=0;
	static long iv[NTAB];
	
	if (idum <= 0 || !iy)
	{
		if (-(idum) < 1) idum=1;
		else idum = -(idum);
		for (j=NTAB+7;j>=0;j--)
		{
			k=(idum)/IQ;
			idum=IA*(idum-k*IQ)-IR*k;
			if (idum < 0) idum += IM;
			if (j < NTAB) iv[j] = idum;
		}
		iy=iv[0];
	}
	k=(idum)/IQ;
	idum=IA*(idum-k*IQ)-IR*k;
	if (idum < 0) idum += IM;
	j=iy/NDIV;
	iy=iv[j];
	iv[j] = idum;

	return iy;
}

// fran1 -- return a random floating-point number on the interval [0,1)
//
#define AM (1.0/IM)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)
float fran1(void)
{
	float temp = (float)AM*ran1();
	if (temp > RNMX) return (float)RNMX;
	else return temp;
}

#ifndef _XBOX
float RandomFloat( float flLow, float flHigh )
{
	if (idum == 0)
	{
		SeedRandomNumberGenerator(0);
	}

	float fl = fran1(); // float in [0,1)
	return (fl * (flHigh-flLow)) + flLow; // float in [low,high)
}
#endif

long RandomLong( long lLow, long lHigh )
{
	if (idum == 0)
	{
		SeedRandomNumberGenerator(0);
	}

	unsigned long maxAcceptable;
	unsigned long x = lHigh-lLow+1;
	unsigned long n;
	if (x <= 0 || MAX_RANDOM_RANGE < x-1)
	{
		return lLow;
	}

	// The following maps a uniform distribution on the interval [0,MAX_RANDOM_RANGE]
	// to a smaller, client-specified range of [0,x-1] in a way that doesn't bias
	// the uniform distribution unfavorably. Even for a worst case x, the loop is
	// guaranteed to be taken no more than half the time, so for that worst case x,
	// the average number of times through the loop is 2. For cases where x is
	// much smaller than MAX_RANDOM_RANGE, the average number of times through the
	// loop is very close to 1.
	//
	maxAcceptable = MAX_RANDOM_RANGE - ((MAX_RANDOM_RANGE+1) % x );
	do
	{
		n = ran1();
	} while (n > maxAcceptable);

	return lLow + (n % x);
}


