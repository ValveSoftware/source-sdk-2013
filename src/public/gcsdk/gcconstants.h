//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: declares a variety of constants
//
// $NoKeywords: $
//=============================================================================

#ifndef GCCONSTANTS_H
#define GCCONSTANTS_H
#ifdef _WIN32
#pragma once
#endif

#include "steam/steamtypes.h"
#include "tier0/t0constants.h"

namespace GCSDK
{

//-----------------------------------------------------------------------------
// Timing constants
//-----------------------------------------------------------------------------


// Default Max time to allow a job to be blocked on I/O
const int k_cMicroSecJobPausedTimeout = 20 * k_nMillion;

// How much time a job should run before heartbeating
const int k_cMicroSecJobHeartbeat = k_cMicroSecJobPausedTimeout / 4;

// Default Max number of job heartbeat intervals to allow a job to be blocked on I/O
const int k_cJobHeartbeatsBeforeTimeoutDefault = k_cMicroSecJobPausedTimeout / k_cMicroSecJobHeartbeat;


//-----------------------------------------------------------------------------
// Server types
//-----------------------------------------------------------------------------
// EServerType
// Specifies the type of a specific server
// MUST BE KEPT IN SYNC with g_rgchServerTypeName !!! 
enum EServerType
{
	k_EServerTypeInvalid = -1,

	k_EServerTypeShell = 0,
	k_EServerTypeGC = 1,
	k_EServerTypeGCClient = 2,

	// Must be last!!!
	k_EServerTypeMax = 2,
};
const EServerType k_EServerTypeFirst = k_EServerTypeShell;



//-----------------------------------------------------------------------------
// Spew / EmitEvent constants
//-----------------------------------------------------------------------------

#define SPEW_ALWAYS 1
#define SPEW_NEVER 5

#define LOG_ALWAYS 1
#define LOG_NEVER 5


} // namespace GCSDK

#endif // GCCONSTANTS_H
