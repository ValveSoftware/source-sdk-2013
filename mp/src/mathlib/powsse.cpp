//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "mathlib/ssemath.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


fltx4 Pow_FixedPoint_Exponent_SIMD( const fltx4 & x, int exponent)
{
	fltx4 rslt=Four_Ones;									// x^0=1.0
	int xp=abs(exponent);
	if (xp & 3)												// fraction present?
	{
		fltx4 sq_rt=SqrtEstSIMD(x);
		if (xp & 1)											// .25?
			rslt=SqrtEstSIMD(sq_rt);						// x^.25
		if (xp & 2)
			rslt=MulSIMD(rslt,sq_rt);
	}
	xp>>=2;													// strip fraction
	fltx4 curpower=x;										// curpower iterates through  x,x^2,x^4,x^8,x^16...

	while(1)
	{
		if (xp & 1)
			rslt=MulSIMD(rslt,curpower);
		xp>>=1;
		if (xp)
			curpower=MulSIMD(curpower,curpower);
		else
			break;
	}
	if (exponent<0)
		return ReciprocalEstSaturateSIMD(rslt);				// pow(x,-b)=1/pow(x,b)
	else
		return rslt;
}




/*
 * (c) Ian Stephenson
 *
 * ian@dctsystems.co.uk
 *
 * Fast pow() reference implementation
 */


static float shift23=(1<<23);
static float OOshift23=1.0/(1<<23);

float FastLog2(float i)
{
	float LogBodge=0.346607f;
	float x;
	float y;
	x=*(int *)&i;
	x*= OOshift23; //1/pow(2,23);
	x=x-127;

	y=x-floorf(x);
	y=(y-y*y)*LogBodge;
	return x+y;
}
float FastPow2(float i)
{
	float PowBodge=0.33971f;
	float x;
	float y=i-floorf(i);
	y=(y-y*y)*PowBodge;

	x=i+127-y;
	x*= shift23; //pow(2,23);
	*(int*)&x=(int)x;
	return x;
}
float FastPow(float a, float b)
{
	if (a <= OOshift23)
	{
		return 0.0f;
	}
	return FastPow2(b*FastLog2(a));
}
float FastPow10( float i )
{
	return FastPow2( i * 3.321928f );
}

