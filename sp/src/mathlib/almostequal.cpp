//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Fast ways to compare equality of two floats.  Assumes 
// sizeof(float) == sizeof(int) and we are using IEEE format.
//
// Source:  http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
//=====================================================================================//

#include <float.h>
#include <math.h>

#include "mathlib/mathlib.h"

static inline bool AE_IsInfinite(float a)
{
    const int kInfAsInt = 0x7F800000;

    // An infinity has an exponent of 255 (shift left 23 positions) and
    // a zero mantissa. There are two infinities - positive and negative.
    if ((*(int*)&a & 0x7FFFFFFF) == kInfAsInt)
        return true;
    return false;
}

static inline bool AE_IsNan(float a)
{
    // a NAN has an exponent of 255 (shifted left 23 positions) and
    // a non-zero mantissa.
    int exp = *(int*)&a & 0x7F800000;
    int mantissa = *(int*)&a & 0x007FFFFF;
    if (exp == 0x7F800000 && mantissa != 0)
        return true;
    return false;
}

static inline int AE_Sign(float a)
{
    // The sign bit of a number is the high bit.
    return (*(int*)&a) & 0x80000000;
}

// This is the 'final' version of the AlmostEqualUlps function.
// The optional checks are included for completeness, but in many
// cases they are not necessary, or even not desirable.
bool AlmostEqual(float a, float b, int maxUlps)
{
    // There are several optional checks that you can do, depending
    // on what behavior you want from your floating point comparisons.
    // These checks should not be necessary and they are included
    // mainly for completeness.

    // If a or b are infinity (positive or negative) then
    // only return true if they are exactly equal to each other -
    // that is, if they are both infinities of the same sign.
    // This check is only needed if you will be generating
    // infinities and you don't want them 'close' to numbers
    // near FLT_MAX.
    if (AE_IsInfinite(a) || AE_IsInfinite(b))
        return a == b;

    // If a or b are a NAN, return false. NANs are equal to nothing,
    // not even themselves.
    // This check is only needed if you will be generating NANs
    // and you use a maxUlps greater than 4 million or you want to
    // ensure that a NAN does not equal itself.
    if (AE_IsNan(a) || AE_IsNan(b))
        return false;

    // After adjusting floats so their representations are lexicographically
    // ordered as twos-complement integers a very small positive number
    // will compare as 'close' to a very small negative number. If this is
    // not desireable, and if you are on a platform that supports
    // subnormals (which is the only place the problem can show up) then
    // you need this check.
    // The check for a == b is because zero and negative zero have different
    // signs but are equal to each other.
    if (AE_Sign(a) != AE_Sign(b))
        return a == b;

    int aInt = *(int*)&a;
    // Make aInt lexicographically ordered as a twos-complement int
    if (aInt < 0)
        aInt = 0x80000000 - aInt;
    // Make bInt lexicographically ordered as a twos-complement int
    int bInt = *(int*)&b;
    if (bInt < 0)
        bInt = 0x80000000 - bInt;

    // Now we can compare aInt and bInt to find out how far apart a and b
    // are.
    int intDiff = abs(aInt - bInt);
    if (intDiff <= maxUlps)
        return true;
    return false;
}


