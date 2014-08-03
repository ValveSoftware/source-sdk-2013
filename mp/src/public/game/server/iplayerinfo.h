//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: provides an interface for dlls to query information about players from the game dll
//
//=============================================================================//
#ifndef IPLAYERINFO_H
#define IPLAYERINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

// helper class for user commands
class CBotCmd
{
public:
	CBotCmd()
	{
		Reset();
	}

	virtual ~CBotCmd() { };

	void Reset()
	{
		command_number = 0;
		tick_count = 0;
		viewangles.Init();
		forwardmove = 0.0f;
		sidemove = 0.0f;
		upmove = 0.0f;
		buttons = 0;
		impulse = 0;
		weaponselect = 0;
		weaponsubtype = 0;
		random_seed = 0;
		mousedx = 0;
		mousedy = 0;

		hasbeenpredicted = false;
	}

	CBotCmd& operator =( const CBotCmd& src )
	{
		if ( this == &src )
			return *this;

		command_number		= src.command_number;
		tick_count			= src.tick_count;
		viewangles			= src.viewangles;
		forwardmove			= src.forwardmove;
		sidemove			= src.sidemove;
		upmove				= src.upmove;
		buttons				= src.buttons;
		impulse				= src.impulse;
		weaponselect		= src.weaponselect;
		weaponsubtype		= src.weaponsubtype;
		random_seed			= src.random_seed;
		mousedx				= src.mousedx;
		mousedy				= src.mousedy;
		hasbeenpredicted	= src.hasbeenpredicted;
		return *this;
	}

	// For matching server and client commands for debugging
	int		command_number;
	
	// the tick the client created this command
	int		tick_count;
	
	// Player instantaneous view angles.
	QAngle	viewangles;     
	// Intended velocities
	//	forward velocity.
	float	forwardmove;   
	//  sideways velocity.
	float	sidemove;      
	//  upward velocity.
	float	upmove;         
	// Attack button states
	int		buttons;		
	// Impulse command issued.
	byte    impulse;        
	// Current weapon id
	int		weaponselect;	
	int		weaponsubtype;

	int		random_seed;	// For shared random functions

	short	mousedx;		// mouse accum in x from create move
	short	mousedy;		// mouse accum in y from create move

	// Client only, tracks whether we've predicted this command at least once
	bool	hasbeenpredicted;
};




abstract_class IPlayerInfo
{
public:
	// returns the players name (UTF-8 encoded)
	virtual const char *GetName() = 0;
	// returns the userid (slot number)
	virtual int		GetUserID() = 0;
	// returns the string of their network (i.e Steam) ID
	virtual const char *GetNetworkIDString() = 0;
	// returns the team the player is on
	virtual int GetTeamIndex() = 0;
	// changes the player to a new team (if the game dll logic allows it)
	virtual void ChangeTeam( int iTeamNum ) = 0;
	// returns the number of kills this player has (exact meaning is mod dependent)
	virtual int	GetFragCount() = 0;
	// returns the number of deaths this player has (exact meaning is mod dependent)
	virtual int	GetDeathCount() = 0;
	// returns if this player slot is actually valid
	virtual bool IsConnected() = 0;
	// returns the armor/health of the player (exact meaning is mod dependent)
	virtual int	GetArmorValue() = 0;

	// extensions added to V2

	// various player flags
	virtual bool IsHLTV() = 0;
	virtual bool IsPlayer() = 0;
	virtual bool IsFakeClient() = 0;
	virtual bool IsDead() = 0;
	virtual bool IsInAVehicle() = 0;
	virtual bool IsObserver() = 0;

	// player position and size
	virtual const Vector GetAbsOrigin() = 0;
	virtual const QAngle GetAbsAngles() = 0;
	virtual const Vector GetPlayerMins() = 0;
	virtual const Vector GetPlayerMaxs() = 0;
	// the name of the weapon currently being carried
	virtual const char *GetWeaponName() = 0;
	// the name of the player model in use
	virtual const char *GetModelName() = 0;
	// current player health
	virtual const int GetHealth() = 0;
	// max health value
	virtual const int GetMaxHealth() = 0;
	// the last user input from this player
	virtual CBotCmd GetLastUserCommand() = 0;

	virtual bool IsReplay() = 0;
};


#define INTERFACEVERSION_PLAYERINFOMANAGER			"PlayerInfoManager002"
abstract_class IPlayerInfoManager
{
public:
	virtual IPlayerInfo *GetPlayerInfo( edict_t *pEdict ) = 0;
	virtual CGlobalVars *GetGlobalVars() = 0;
};




abstract_class IBotController
{
public:
	// change the bots position
	virtual void SetAbsOrigin( Vector & vec ) = 0;
	virtual void SetAbsAngles( QAngle & ang ) = 0;
	virtual void SetLocalOrigin( const Vector& origin ) = 0;
	virtual const Vector GetLocalOrigin( void ) = 0;
	virtual void SetLocalAngles( const QAngle& angles ) = 0;
	virtual const QAngle GetLocalAngles( void ) = 0;

	// strip them of weapons, etc
	virtual void RemoveAllItems( bool removeSuit ) = 0;
	// give them a weapon
	virtual void SetActiveWeapon( const char *WeaponName ) = 0;
	// check various effect flags
	virtual bool IsEFlagSet( int nEFlagMask ) = 0;
	// fire a virtual move command to the bot
	virtual void RunPlayerMove( CBotCmd *ucmd ) = 0;
};


#define INTERFACEVERSION_PLAYERBOTMANAGER			"BotManager001"
abstract_class IBotManager
{
public:
	virtual IBotController *GetBotController( edict_t *pEdict ) = 0;
	// create a new bot and spawn it into the server
	virtual edict_t *CreateBot( const char *botname ) = 0;
};

#endif // IPLAYERINFO_H
