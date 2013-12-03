//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: provides an interface for plugins to query information about the gamerules in a simple
// and organized mannor.
//
//===============================================================================================//
#ifndef IGAMEINFO_H
#define IGAMEINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "pluginvariant.h"

//Tony; prefixing everything in this so that i can make IGameInfo an extension of CGameRules and not stomp on anything, since gamerules isn't an entity.
abstract_class IGameInfo
{
public:
	// returns an enumerated id for the current game type
	virtual const int GetInfo_GameType() = 0;
	// returns a name associated with the gametype, if defined.
	virtual const char *GetInfo_GameTypeName() = 0;
	// returns the team name associated with the number
	virtual const char *GetInfo_GetTeamName(int teamNumber) = 0;
	// returns how many teams the game has (typically always 4; 0 = unassigned, 1 = spectator, 2 = team1, 3 = team2)
	virtual const int GetInfo_GetTeamCount() = 0;
	// returns how many players are on a given team
	virtual const int GetInfo_NumPlayersOnTeam(int teamNumber) = 0;

	// accessor to hook mod specific information about the rules. for TF2, fields such as 
	virtual bool GetInfo_Custom(int valueType, pluginvariant &outValue, pluginvariant options) = 0;

};


//Interface is very simple, there's not much really needed for the manager, this stuff is just in it's own interface so it's not mixed up with the entity
//or player managers.
#define INTERFACEVERSION_GAMEINFOMANAGER			"GameInfoManager001"
abstract_class IGameInfoManager
{
public:
	virtual IGameInfo *GetGameInfo() = 0;
};
#endif // IGAMEINFO_H
