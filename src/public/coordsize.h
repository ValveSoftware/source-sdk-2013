//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef COORDSIZE_H
#define COORDSIZE_H
#pragma once

#include "worldsize.h"

// OVERALL Coordinate Size Limits used in COMMON.C MSG_*BitCoord() Routines (and someday the HUD)
#define	COORD_INTEGER_BITS			14
#define COORD_FRACTIONAL_BITS		5
#define COORD_DENOMINATOR			(1<<(COORD_FRACTIONAL_BITS))
#define COORD_RESOLUTION			(1.0/(COORD_DENOMINATOR))

// Special threshold for networking multiplayer origins
#define COORD_INTEGER_BITS_MP		11
#define COORD_FRACTIONAL_BITS_MP_LOWPRECISION 3
#define COORD_DENOMINATOR_LOWPRECISION			(1<<(COORD_FRACTIONAL_BITS_MP_LOWPRECISION))
#define COORD_RESOLUTION_LOWPRECISION			(1.0/(COORD_DENOMINATOR_LOWPRECISION))

#define NORMAL_FRACTIONAL_BITS		11
#define NORMAL_DENOMINATOR			( (1<<(NORMAL_FRACTIONAL_BITS)) - 1 )
#define NORMAL_RESOLUTION			(1.0/(NORMAL_DENOMINATOR))

// this is limited by the network fractional bits used for coords
// because net coords will be only be accurate to 5 bits fractional
// Standard collision test epsilon
// 1/32nd inch collision epsilon
#define DIST_EPSILON (0.03125)

// Verify that coordsize.h and worldsize.h are consistently defined
#if (MAX_COORD_INTEGER != (1<<COORD_INTEGER_BITS) )
#error MAX_COORD_INTEGER does not match COORD_INTEGER_BITS
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// The following are Bit Packing Diagrams for client/server Coordinate BitField Messages
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//							"Coordinates" = +/-16384.9375	(21 Bits Max)
//
// | IntegerFlagBit:1 | FractionFlagBit:1 | SignBit:1 | IntegerField:14 | FractionPart:4 |		Total		
// --------------------------------------------------------------------------------------------------
//			0					0				-				-				-				2
//			0					1				x				-				xxxx			7
//			1					0				x		xxxxxxxxxxxxx			-				17
//			1					1				x		xxxxxxxxxxxxx			xxxx			21
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//							"Vec3Coordinate" = Up to 3 Coordinates (66 Bits Max)
//
// | NonZeroX:1 | NonZeroY:1 | NonZeroZ:1 |  0..3 "Coordinates"	|	BitField Total
// -------------------------------------------------------------------------------
//			0			0				0			-					3
//			0			0				1			7..21 Bits			10..24
//			0			1				0			7..21 Bits			10..24
//			1			0				0			7..21 Bits			10..24
//			0			1				1			14..42 Bits			17..45
//			1			1				0			14..42 Bits			17..45
//			1			0				1			14..42 Bits			17..45
//			1			1				1			21..63 Bits			24..66
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////
// The following are Bit Packing Diagrams for client/server Normal BitField Messages
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//							"Normals" = +/-1.0	(12 Bits Total)
//
// The only gotcha here is that normalization occurs so that 
// 011111111111 = +1.0 and 1111111111111 = -1.0
//
// | SignBit:1 | FractionPart:11 |		Total		
// --------------------------------------------------------------------------------------------------
//			1		xxxxxxxxxxx			12
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//							"Vec3Normal" = Up to 3 Coordinates (27 Bits Max)
//
// | NonZeroX:1 | NonZeroY:1 |  0..2 "Coordinates"	| Z Sign Bit |	BitField Total
// -------------------------------------------------------------------------------
//			0			0				-					x			3
//			0			1			12 Bits					x			14
//			1			0			12 Bits					x			14
//			1			1			24 Bits					x			27
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // COORDSIZE_H
