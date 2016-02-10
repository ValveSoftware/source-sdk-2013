//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TF_PLAYERANIMSTATE_H
#define TF_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "iplayeranimstate.h"
#include "base_playeranimstate.h"
#include "mom_player_shared.h"

#ifdef CLIENT_DLL
	class C_BaseAnimatingOverlay;
	class C_WeaponCSBase;
	#define CBaseAnimatingOverlay C_BaseAnimatingOverlay
	#define CWeaponCSBase C_WeaponCSBase
#else
	class CBaseAnimatingOverlay;
	class CWeaponCSBase; 
#endif


// When moving this fast, he plays run anim.
#define ARBITRARY_RUN_SPEED		175.0f


enum PlayerAnimEvent_t
{
	PLAYERANIMEVENT_FIRE_GUN_PRIMARY=0,
	PLAYERANIMEVENT_FIRE_GUN_SECONDARY,
	PLAYERANIMEVENT_THROW_GRENADE,
	PLAYERANIMEVENT_JUMP,
	PLAYERANIMEVENT_RELOAD,
	PLAYERANIMEVENT_RELOAD_START,	///< w_model partial reload for shotguns
	PLAYERANIMEVENT_RELOAD_LOOP,	///< w_model partial reload for shotguns
	PLAYERANIMEVENT_RELOAD_END,		///< w_model partial reload for shotguns
	
	PLAYERANIMEVENT_COUNT
};


class ICSPlayerAnimState : virtual public IPlayerAnimState
{
public:
	// This is called by both the client and the server in the same way to trigger events for
	// players firing, jumping, throwing grenades, etc.
	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 ) = 0;
	
	// Returns true if we're playing the grenade prime or throw animation.
	virtual bool IsThrowingGrenade() = 0;
};


// This abstracts the differences between CS players and hostages.
class ICSPlayerAnimStateHelpers
{
public:
	virtual CWeaponCSBase* CSAnim_GetActiveWeapon() = 0;
	virtual bool CSAnim_CanMove() = 0;
};


ICSPlayerAnimState* CreatePlayerAnimState( CBaseAnimatingOverlay *pEntity, ICSPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences );
ICSPlayerAnimState* CreateHostageAnimState( CBaseAnimatingOverlay *pEntity, ICSPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences );

// If this is set, then the game code needs to make sure to send player animation events
// to the local player if he's the one being watched.
extern ConVar cl_showanimstate;


#endif // TF_PLAYERANIMSTATE_H
