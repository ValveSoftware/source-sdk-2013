//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: APIs for player pickup of physics objects
//
//=============================================================================//

#ifndef PLAYER_PICKUP_H
#define PLAYER_PICKUP_H
#ifdef _WIN32
#pragma once
#endif

#ifdef HL2_DLL
// Needed for launch velocity
extern ConVar physcannon_minforce;
extern ConVar physcannon_maxforce;
#endif

// Reasons behind a pickup
enum PhysGunPickup_t
{
	PICKED_UP_BY_CANNON,
	PUNTED_BY_CANNON,
	PICKED_UP_BY_PLAYER, // Picked up by +USE, not physgun.
};

// Reasons behind a drop
enum PhysGunDrop_t
{
	DROPPED_BY_PLAYER,
	THROWN_BY_PLAYER,
	DROPPED_BY_CANNON,
	LAUNCHED_BY_CANNON,
};

enum PhysGunForce_t
{
	PHYSGUN_FORCE_DROPPED,	// Dropped by +USE
	PHYSGUN_FORCE_THROWN,	// Thrown from +USE
	PHYSGUN_FORCE_PUNTED,	// Punted by cannon
	PHYSGUN_FORCE_LAUNCHED,	// Launched by cannon
};

void PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject );
void Pickup_ForcePlayerToDropThisObject( CBaseEntity *pTarget );

void Pickup_OnPhysGunDrop( CBaseEntity *pDroppedObject, CBasePlayer *pPlayer, PhysGunDrop_t reason );
void Pickup_OnPhysGunPickup( CBaseEntity *pPickedUpObject, CBasePlayer *pPlayer, PhysGunPickup_t reason = PICKED_UP_BY_CANNON );
bool Pickup_OnAttemptPhysGunPickup( CBaseEntity *pPickedUpObject, CBasePlayer *pPlayer, PhysGunPickup_t reason = PICKED_UP_BY_CANNON );
bool Pickup_GetPreferredCarryAngles( CBaseEntity *pObject, CBasePlayer *pPlayer, matrix3x4_t &localToWorld, QAngle &outputAnglesWorldSpace );
bool Pickup_ForcePhysGunOpen( CBaseEntity *pObject, CBasePlayer *pPlayer );
bool Pickup_ShouldPuntUseLaunchForces( CBaseEntity *pObject, PhysGunForce_t reason );
AngularImpulse Pickup_PhysGunLaunchAngularImpulse( CBaseEntity *pObject, PhysGunForce_t reason );
Vector Pickup_DefaultPhysGunLaunchVelocity( const Vector &vecForward, float flMass );
Vector Pickup_PhysGunLaunchVelocity( CBaseEntity *pObject, const Vector &vecForward, PhysGunForce_t reason );

CBaseEntity	*Pickup_OnFailedPhysGunPickup( CBaseEntity *pPickedUpObject, Vector vPhysgunPos );

abstract_class IPlayerPickupVPhysics
{
public:
	// Callbacks for the physgun/cannon picking up an entity
	virtual bool			OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) = 0;
	virtual CBaseEntity		*OnFailedPhysGunPickup( Vector vPhysgunPos ) = 0;
	virtual void			OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) = 0;
	virtual void			OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason ) = 0;
	virtual bool			HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer = NULL ) = 0;
	virtual QAngle			PreferredCarryAngles( void )  = 0;
	virtual bool			ForcePhysgunOpen( CBasePlayer *pPlayer ) = 0;
	virtual AngularImpulse	PhysGunLaunchAngularImpulse() = 0;
	virtual bool			ShouldPuntUseLaunchForces( PhysGunForce_t reason ) = 0;
	virtual Vector			PhysGunLaunchVelocity( const Vector &vecForward, float flMass ) = 0;
};

class CDefaultPlayerPickupVPhysics : public IPlayerPickupVPhysics
{
public:
	virtual bool			OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) { return true; }
	virtual CBaseEntity		*OnFailedPhysGunPickup( Vector vPhysgunPos ) { return NULL; }
	virtual void			OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) {}
	virtual void			OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason ) {}
	virtual bool			HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer ) { return false; }
	virtual QAngle			PreferredCarryAngles( void ) { return vec3_angle; }
	virtual bool			ForcePhysgunOpen( CBasePlayer *pPlayer ) { return false; }
	virtual AngularImpulse	PhysGunLaunchAngularImpulse() { return RandomAngularImpulse( -600, 600 ); }
	virtual bool			ShouldPuntUseLaunchForces( PhysGunForce_t reason ) { return false; }
	virtual Vector			PhysGunLaunchVelocity( const Vector &vecForward, float flMass )
	{
		return Pickup_DefaultPhysGunLaunchVelocity( vecForward, flMass );
	}
};

#endif // PLAYER_PICKUP_H
