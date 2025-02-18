//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMETIMESELECTIONTIMES_H
#define DMETIMESELECTIONTIMES_H
#ifdef _WIN32
#pragma once
#endif

enum DmeTimeSelectionTimes_t
{
	TS_LEFT_FALLOFF = 0,
	TS_LEFT_HOLD,
	TS_RIGHT_HOLD,
	TS_RIGHT_FALLOFF,

	TS_TIME_COUNT,
};

// NOTE: _side == 0 means left, == 1 means right
#define TS_FALLOFF( _side ) ( ( TS_TIME_COUNT - (_side) ) & 0x3 )
#define TS_HOLD( _side ) ( TS_LEFT_HOLD + (_side) )


#endif // DMETIMESELECTIONTIMES_H
