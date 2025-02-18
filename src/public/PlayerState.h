//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "edict.h"
#include "networkvar.h"
// Only care about this stuff in game/client .dlls
#if defined( CLIENT_DLL )
#include "predictable_entity.h"
#endif

class CPlayerState
{
public:
	DECLARE_CLASS_NOBASE( CPlayerState );
	DECLARE_EMBEDDED_NETWORKVAR();
	
	// This virtual method is necessary to generate a vtable in all cases
	// (DECLARE_PREDICTABLE will generate a vtable also)!
	virtual ~CPlayerState() {}

	// true if the player is dead
	CNetworkVar( bool, deadflag );	
	// Viewing angle (player only)
	QAngle		v_angle;		
	
// The client .dll only cares about deadflag
//  the game and engine .dlls need to worry about the rest of this data
#if !defined( CLIENT_DLL )
	// Player's network name
	string_t	netname;
	// 0:nothing, 1:force view angles, 2:add avelocity
	int			fixangle;
	// delta angle for fixangle == FIXANGLE_RELATIVE
	QAngle		anglechange;
	// flag to single the HLTV/Replay fake client, not transmitted
	bool		hltv;
	bool		replay;
	int			frags;
	int			deaths;
#endif

// NOTE:  Only care about this stuff in game/client dlls
// Put at end in case it has any effect on size of structure
#if defined( GAME_DLL )
	DECLARE_SIMPLE_DATADESC();
#endif

#if defined( CLIENT_DLL )
	DECLARE_PREDICTABLE();
#endif
};

#endif // PLAYERSTATE_H
