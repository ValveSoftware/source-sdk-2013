//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Add a specially formatted string to each debug DLL of the form 
//          "%DLLNAME%.dll is built debug!". We can search for this string via
//			a Perforce trigger to ensure that debug LIBs are not checked in.
//
//=============================================================================//

#if defined(DEBUG) || defined(_DEBUG)
#include "tier0/platform.h"

#define _DEBUGONLYSTRING(x) #x
#define DEBUGONLYSTRING(x) _DEBUGONLYSTRING(x) 
DLL_GLOBAL_EXPORT char const *pDebugString = DEBUGONLYSTRING(DLLNAME) ".dll is built debug!";

#endif