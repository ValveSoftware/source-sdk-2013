//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_DEBUG_SHARED_H
#define AI_DEBUG_SHARED_H
#ifdef _WIN32
#pragma once
#endif


#include "tier0/vprof.h"


// This uses VPROF to profile
//#define VPROF_AI 1


#ifdef VPROF_AI
inline void AI_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
						 const IHandleEntity *ignore, int collisionGroup, trace_t *ptr )
{
	VPROF( "AI_TraceLine" );
	UTIL_TraceLine( vecAbsStart, vecAbsEnd, mask, ignore, collisionGroup, ptr );
}

inline void AI_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
						 ITraceFilter *pFilter, trace_t *ptr )
{
	VPROF( "AI_TraceLine" );
	UTIL_TraceLine( vecAbsStart, vecAbsEnd, mask, pFilter, ptr );
}

inline void AI_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
						 const Vector &hullMax,	unsigned int mask, const IHandleEntity *ignore, 
						 int collisionGroup, trace_t *ptr )
{
	VPROF( "AI_TraceHull" );
	UTIL_TraceHull( vecAbsStart, vecAbsEnd, hullMin, hullMax, mask, ignore, collisionGroup, ptr );
}

inline void AI_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
						 const Vector &hullMax,	unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
	VPROF( "AI_TraceHull" );
	UTIL_TraceHull( vecAbsStart, vecAbsEnd, hullMin, hullMax, mask, pFilter, ptr );
}

inline void AI_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr )
{
	VPROF( "AI_TraceEntity" );
	UTIL_TraceEntity( pEntity, vecAbsStart, vecAbsEnd, mask, ptr );
}

#else
#define AI_TraceLine UTIL_TraceLine
#define AI_TraceHull UTIL_TraceHull
#define AI_TraceEntity UTIL_TraceEntity
#endif


#endif // AI_DEBUG_SHARED_H
