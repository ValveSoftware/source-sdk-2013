//=========== Copyright Valve Corporation, All rights reserved. ===============
//
// Purpose: Declares realm enum
//
//=============================================================================

#ifndef STEAMREALM_H
#define STEAMREALM_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Purpose: Is the client running on Steam Global or Steam China
//-----------------------------------------------------------------------------
enum ESteamRealm
{
	k_ESteamRealmUnknown = 0,	// Unknown / not set
	k_ESteamRealmGlobal = 1,	// Steam Global
	k_ESteamRealmChina = 2,		// Steam China

	k_ESteamRealmMax
};


#endif // STEAMREALM_H
