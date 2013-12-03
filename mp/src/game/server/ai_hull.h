//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================//

#ifndef AI_HULL_H
#define AI_HULL_H
#pragma once

class Vector;
//=========================================================
// Link Properties. These hulls must correspond to the hulls
// in AI_Hull.cpp!
//=========================================================
enum Hull_t
{
	HULL_HUMAN,				// Combine, Stalker, Zombie...
	HULL_SMALL_CENTERED,	// Scanner
	HULL_WIDE_HUMAN,		// Vortigaunt
	HULL_TINY,				// Headcrab
	HULL_WIDE_SHORT,		// Bullsquid
	HULL_MEDIUM,			// Cremator
	HULL_TINY_CENTERED,		// Manhack 
	HULL_LARGE,				// Antlion Guard
	HULL_LARGE_CENTERED,	// Mortar Synth
	HULL_MEDIUM_TALL,		// Hunter
//--------------------------------------------
	NUM_HULLS,
	HULL_NONE				// No Hull (appears after num hulls as we don't want to count it)
};

enum Hull_Bits_t
{
	bits_HUMAN_HULL				=	0x00000001,
	bits_SMALL_CENTERED_HULL	=	0x00000002,
	bits_WIDE_HUMAN_HULL		=	0x00000004,
	bits_TINY_HULL				=	0x00000008,
	bits_WIDE_SHORT_HULL		=	0x00000010,
	bits_MEDIUM_HULL			=	0x00000020,
	bits_TINY_CENTERED_HULL		=	0x00000040,
	bits_LARGE_HULL				=	0x00000080,
	bits_LARGE_CENTERED_HULL	=	0x00000100,
	bits_MEDIUM_TALL_HULL		=	0x00000200,
	bits_HULL_BITS_MASK			=	0x000002ff,
};

inline int HullToBit( Hull_t hull )
{
	return ( 1 << hull );
}



//=============================================================================
//	>> CAI_Hull
//=============================================================================
namespace NAI_Hull
{
	const Vector &Mins(int id);
	const Vector &Maxs(int id);

	const Vector &SmallMins(int id);
	const Vector &SmallMaxs(int id);

	float		Length(int id);
	float		Width(int id);
	float		Height(int id);

	int			Bits(int id);
 
	const char*	Name(int id);

	Hull_t		LookupId(const char *szName);
};

#endif // AI_HULL_H
