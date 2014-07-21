//========= Copyright Valve Corporation, All rights reserved. ============//
// $Id$

// halton.h - classes, etc for generating numbers using the Halton pseudo-random sequence.  See
// http://halton-sequences.wikiverse.org/.
//
// what this function is useful for is any sort of sampling/integration problem where
// you want to solve it by random sampling. Each call the NextValue() generates
// a random number between 0 and 1, in an unclumped manner, so that the space can be more
// or less evenly sampled with a minimum number of samples.
//
// It is NOT useful for generating random numbers dynamically, since the outputs aren't
// particularly random.
//
// To generate multidimensional sample values (points in a plane, etc), use two
// HaltonSequenceGenerator_t's, with different (primes) bases.

#ifndef HALTON_H
#define HALTON_H

#include <tier0/platform.h>
#include <mathlib/vector.h>

class HaltonSequenceGenerator_t
{
	int seed;
	int base;
	float fbase;											//< base as a float

public:
	HaltonSequenceGenerator_t(int base);					//< base MUST be prime, >=2

	float GetElement(int element);

	inline float NextValue(void)
	{
		return GetElement(seed++);
	}

};


class DirectionalSampler_t									//< pseudo-random sphere sampling
{
	HaltonSequenceGenerator_t zdot;
	HaltonSequenceGenerator_t vrot;
public:
	DirectionalSampler_t(void)
		: zdot(2),vrot(3)
	{
	}

	Vector NextValue(void)
	{
		float zvalue=zdot.NextValue();
		zvalue=2*zvalue-1.0;								// map from 0..1 to -1..1
		float phi=acos(zvalue);
		// now, generate a random rotation angle for x/y
		float theta=2.0*M_PI*vrot.NextValue();
		float sin_p=sin(phi);
		return Vector(cos(theta)*sin_p,
					  sin(theta)*sin_p,
					  zvalue);

	}
};




#endif // halton_h
