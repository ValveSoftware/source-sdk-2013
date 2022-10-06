//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEMS_H
#define ITEMS_H

#ifdef _WIN32
#pragma once
#endif

#include "entityoutput.h"
#include "player_pickup.h"
#include "vphysics/constraints.h"


// Armor given by a battery
#define MAX_NORMAL_BATTERY	100

// Ammo counts given by ammo items
#define SIZE_AMMO_PISTOL			20
#define SIZE_AMMO_PISTOL_LARGE		100
#define SIZE_AMMO_SMG1				45
#define SIZE_AMMO_SMG1_LARGE		225
#define SIZE_AMMO_AR2				20
#define SIZE_AMMO_AR2_LARGE			100
#define SIZE_AMMO_RPG_ROUND			1
#define SIZE_AMMO_SMG1_GRENADE		1
#define SIZE_AMMO_BUCKSHOT			20
#define SIZE_AMMO_357				6
#define SIZE_AMMO_357_LARGE			20
#define SIZE_AMMO_CROSSBOW			6
#define	SIZE_AMMO_AR2_ALTFIRE		1

#define SF_ITEM_START_CONSTRAINED	0x00000001
#ifdef MAPBASE
// Copied from CBaseCombatWeapon's flags, including any additions we made to those.
// I really, REALLY hope no item uses their own spawnflags either.
#define SF_ITEM_NO_PLAYER_PICKUP	(1<<1)
#define SF_ITEM_NO_PHYSCANNON_PUNT (1<<2)
#define SF_ITEM_NO_NPC_PICKUP	(1<<3)

#define SF_ITEM_ALWAYS_TOUCHABLE	(1<<6) // This needs to stay synced with the weapon spawnflag
#endif


class CItem : public CBaseAnimating, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS( CItem, CBaseAnimating );

	CItem();

	virtual void Spawn( void );
	virtual void Precache();

	unsigned int PhysicsSolidMaskForEntity( void ) const;

	virtual CBaseEntity* Respawn( void );
	virtual void ItemTouch( CBaseEntity *pOther );
	virtual void Materialize( void );
	virtual bool MyTouch( CBasePlayer *pPlayer ) { return false; };

	// Become touchable when we are at rest
	virtual void OnEntityEvent( EntityEvent_t event, void *pEventData );

	// Activate when at rest, but don't allow pickup until then
	void ActivateWhenAtRest( float flTime = 0.5f );

	// IPlayerPickupVPhysics
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason );

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_WCEDIT_POSITION; };
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	Vector	GetOriginalSpawnOrigin( void ) { return m_vOriginalSpawnOrigin;	}
	QAngle	GetOriginalSpawnAngles( void ) { return m_vOriginalSpawnAngles;	}
	void	SetOriginalSpawnOrigin( const Vector& origin ) { m_vOriginalSpawnOrigin = origin; }
	void	SetOriginalSpawnAngles( const QAngle& angles ) { m_vOriginalSpawnAngles = angles; }
	bool	CreateItemVPhysicsObject( void );
	virtual bool	ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer );

#if defined( HL2MP ) || defined( TF_DLL )
	void	FallThink( void );
	float  m_flNextResetCheckTime;
#endif

#ifdef MAPBASE
	// This appeared to have no prior use in Source SDK 2013.
	// It may have been originally intended for TF2 or some other game-specific item class.
	virtual bool IsCombatItem() const { return true; }

	// Used to access item_healthkit values, etc. from outside of the class
	virtual float GetItemAmount() { return 1.0f; }

	void	InputEnablePlayerPickup( inputdata_t &inputdata );
	void	InputDisablePlayerPickup( inputdata_t &inputdata );
	void	InputEnableNPCPickup( inputdata_t &inputdata );
	void	InputDisableNPCPickup( inputdata_t &inputdata );
	void	InputBreakConstraint( inputdata_t &inputdata );
#endif

	DECLARE_DATADESC();
protected:
	virtual void ComeToRest( void );

private:
	bool		m_bActivateWhenAtRest;
	COutputEvent m_OnPlayerTouch;
	COutputEvent m_OnCacheInteraction;
	
	Vector		m_vOriginalSpawnOrigin;
	QAngle		m_vOriginalSpawnAngles;

	IPhysicsConstraint		*m_pConstraint;
};

#endif // ITEMS_H
