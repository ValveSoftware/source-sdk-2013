//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_DEBUG_H
#define AI_DEBUG_H

#include "fmtstr.h"
#include "ai_debug_shared.h"

#if defined( _WIN32 )
#pragma once
#endif

// This dumps a summary result on exit
//#define PROFILE_AI 1

#define AI_PROFILE_SCOPE_BEGIN( tag )	if (0) ; else { AI_PROFILE_SCOPE( tag )
#define AI_PROFILE_SCOPE_BEGIN_( pszName )	if (0) ; else { AI_PROFILE_SCOPE_( pszName )
#define AI_PROFILE_SCOPE_END()			} do {} while (0)

#if defined(VPROF_AI)
#define VProfAI() true
#else
#define VProfAI() false
#endif
#if defined(VPROF_AI)
#include "tier0/vprof.h"
#define AI_PROFILE_SCOPE( tag )			VPROF( #tag )
#define AI_PROFILE_SCOPE_( pszName )	VPROF( pszName )
#define AI_PROFILE_MEASURE_SCOPE( tag )	VPROF( #tag )
#elif defined(PROFILE_AI)
#include "tier0/fasttimer.h"
#define AI_PROFILE_SCOPE( tag )			PROFILE_SCOPE( tag )
#define AI_PROFILE_MEASURE_SCOPE( tag )	PROFILE_SCOPE( tag )
#else
#define AI_PROFILE_MEASURE_SCOPE( tag )	((void)0)
#define AI_PROFILE_SCOPE( tag )			((void)0)
#endif

#ifndef AI_PROFILE_SCOPE_
#define AI_PROFILE_SCOPE_( pszName ) 	((void)0)
#endif


enum AIMsgFlags
{
	AIMF_IGNORE_SELECTED = 0x01
};

void DevMsg( CAI_BaseNPC *pAI, unsigned flags, PRINTF_FORMAT_STRING const char *pszFormat, ... );
void DevMsg( CAI_BaseNPC *pAI, PRINTF_FORMAT_STRING const char *pszFormat, ... );


//-----------------------------------------------------------------------------
// Purpose: Use this to perform AI tracelines that are trying to determine LOS between points.
//			LOS checks between entities should use FVisible.
//-----------------------------------------------------------------------------
void AI_TraceLOS( const Vector& vecAbsStart, const Vector& vecAbsEnd, CBaseEntity *pLooker, trace_t *ptr, ITraceFilter *pFilter = NULL );

//-----------------------------------------------------------------------------

#ifdef DEBUG
extern bool g_fTestSteering;
#define TestingSteering() g_fTestSteering
#else
#define TestingSteering() false
#endif

//-----------------------------------------------------------------------------


#ifdef _DEBUG
extern ConVar ai_debug_doors;
#define AIIsDebuggingDoors( pNPC ) ( ai_debug_doors.GetBool() && pNPC->m_bSelected )
#define AIDoorDebugMsg( pNPC, msg )	if ( !AIIsDebuggingDoors( pNPC ) ) ; else Msg( msg )
#else
#define AIIsDebuggingDoors( pNPC ) (false)
#define AIDoorDebugMsg( pNPC, msg )	((void)(0)) 
#endif


//-----------------------------------------------------------------------------

#endif // AI_DEBUG_H
