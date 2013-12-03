//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SDK_PLAYERANIMSTATE_H
#define SDK_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "iplayeranimstate.h"
#include "base_playeranimstate.h"


#ifdef CLIENT_DLL
	class C_BaseAnimatingOverlay;
	class C_WeaponSDKBase;
	#define CBaseAnimatingOverlay C_BaseAnimatingOverlay
	#define CWeaponSDKBase C_WeaponSDKBase
	#define CSDKPlayer C_SDKPlayer
#else
	class CBaseAnimatingOverlay;
	class CWeaponSDKBase; 
	class CSDKPlayer;
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
	
	PLAYERANIMEVENT_COUNT
};


class ISDKPlayerAnimState : virtual public IPlayerAnimState
{
public:
	// This is called by both the client and the server in the same way to trigger events for
	// players firing, jumping, throwing grenades, etc.
	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 ) = 0;
	
	// Returns true if we're playing the grenade prime or throw animation.
	virtual bool IsThrowingGrenade() = 0;
};


// This abstracts the differences between SDK players and hostages.
class ISDKPlayerAnimStateHelpers
{
public:
	virtual CWeaponSDKBase* SDKAnim_GetActiveWeapon() = 0;
	virtual bool SDKAnim_CanMove() = 0;
};


ISDKPlayerAnimState* CreatePlayerAnimState( CBaseAnimatingOverlay *pEntity, ISDKPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences );

// If this is set, then the game code needs to make sure to send player animation events
// to the local player if he's the one being watched.
extern ConVar cl_showanimstate;


#endif // SDK_PLAYERANIMSTATE_H
