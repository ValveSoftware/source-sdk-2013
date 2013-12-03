//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// worldsize.h  -- extent of world and resolution/size of coordinate messages used in engine

#ifndef WORLDSIZE_H
#define WORLDSIZE_H
#pragma once


// These definitions must match the coordinate message sizes in coordsize.h

// Following values should be +16384, -16384, +15/16, -15/16
// NOTE THAT IF THIS GOES ANY BIGGER THEN DISK NODES/LEAVES CANNOT USE SHORTS TO STORE THE BOUNDS
#define MAX_COORD_INTEGER			(16384)
#define MIN_COORD_INTEGER			(-MAX_COORD_INTEGER)
#define MAX_COORD_FRACTION			(1.0-(1.0/16.0))
#define MIN_COORD_FRACTION			(-1.0+(1.0/16.0))

#define MAX_COORD_FLOAT				(16384.0f)
#define MIN_COORD_FLOAT				(-MAX_COORD_FLOAT)

// Width of the coord system, which is TOO BIG to send as a client/server coordinate value
#define COORD_EXTENT				(2*MAX_COORD_INTEGER)

// Maximum traceable distance ( assumes cubic world and trace from one corner to opposite )
// COORD_EXTENT * sqrt(3)
#define MAX_TRACE_LENGTH			( 1.732050807569 * COORD_EXTENT )		

// This value is the LONGEST possible range (limited by max valid coordinate number, not 2x)
#define MAX_COORD_RANGE				(MAX_COORD_INTEGER)

#define ASSERT_COORD( v ) Assert( (v.x>=MIN_COORD_INTEGER*2) && (v.x<=MAX_COORD_INTEGER*2) && \
								  (v.y>=MIN_COORD_INTEGER*2) && (v.y<=MAX_COORD_INTEGER*2) && \
								  (v.z>=MIN_COORD_INTEGER*2) && (v.z<=MAX_COORD_INTEGER*2) ); \


#endif // WORLDSIZE_H
