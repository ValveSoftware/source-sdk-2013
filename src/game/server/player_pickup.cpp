//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "player_pickup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// player pickup utility routine
void Pickup_ForcePlayerToDropThisObject( CBaseEntity *pTarget )
{
	if ( pTarget == NULL )
		return;

	IPhysicsObject *pPhysics = pTarget->VPhysicsGetObject();
	
	if ( pPhysics == NULL )
		return;

	if ( pPhysics->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		pPlayer->ForceDropOfCarriedPhysObjects( pTarget );
	}
}


void Pickup_OnPhysGunDrop( CBaseEntity *pDroppedObject, CBasePlayer *pPlayer, PhysGunDrop_t Reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pDroppedObject);
	if ( pPickup )
	{
		pPickup->OnPhysGunDrop( pPlayer, Reason );
	}
}


void Pickup_OnPhysGunPickup( CBaseEntity *pPickedUpObject, CBasePlayer *pPlayer, PhysGunPickup_t reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pPickedUpObject);
	if ( pPickup )
	{
		pPickup->OnPhysGunPickup( pPlayer, reason );
	}

	// send phys gun pickup item event, but only in single player
	if ( !g_pGameRules->IsMultiplayer() )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "physgun_pickup" );
		if ( event )
		{
			event->SetInt( "entindex", pPickedUpObject->entindex() );
			gameeventmanager->FireEvent( event );
		}
	}
}

bool Pickup_OnAttemptPhysGunPickup( CBaseEntity *pPickedUpObject, CBasePlayer *pPlayer, PhysGunPickup_t reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pPickedUpObject);
	if ( pPickup )
	{
		return pPickup->OnAttemptPhysGunPickup( pPlayer, reason );
	}
	return true;
}

CBaseEntity	*Pickup_OnFailedPhysGunPickup( CBaseEntity *pPickedUpObject, Vector vPhysgunPos )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pPickedUpObject);
	if ( pPickup )
	{
		return pPickup->OnFailedPhysGunPickup( vPhysgunPos );
	}

	return NULL;
}

bool Pickup_GetPreferredCarryAngles( CBaseEntity *pObject, CBasePlayer *pPlayer, matrix3x4_t &localToWorld, QAngle &outputAnglesWorldSpace )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if ( pPickup )
	{
		if ( pPickup->HasPreferredCarryAnglesForPlayer( pPlayer ) )
		{
			outputAnglesWorldSpace = TransformAnglesToWorldSpace( pPickup->PreferredCarryAngles(), localToWorld );
			return true;
		}
	}
	return false;
}

bool Pickup_ForcePhysGunOpen( CBaseEntity *pObject, CBasePlayer *pPlayer )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if ( pPickup )
	{
		return pPickup->ForcePhysgunOpen( pPlayer );
	}
	return false;
}

AngularImpulse Pickup_PhysGunLaunchAngularImpulse( CBaseEntity *pObject, PhysGunForce_t reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if ( pPickup != NULL && pPickup->ShouldPuntUseLaunchForces( reason ) )
	{
		return pPickup->PhysGunLaunchAngularImpulse();
	}
	return RandomAngularImpulse( -600, 600 );
}

Vector Pickup_DefaultPhysGunLaunchVelocity( const Vector &vecForward, float flMass )
{
#ifdef HL2_DLL
	// Calculate the velocity based on physcannon rules
	float flForceMax = physcannon_maxforce.GetFloat();
	float flForce = flForceMax;

	float mass = flMass;
	if ( mass > 100 )
	{
		mass = MIN( mass, 1000 );
		float flForceMin = physcannon_minforce.GetFloat();
		flForce = SimpleSplineRemapValClamped( mass, 100, 600, flForceMax, flForceMin );
	}

	return ( vecForward * flForce );
#endif

	// Do the simple calculation
	return ( vecForward * flMass );
}

Vector Pickup_PhysGunLaunchVelocity( CBaseEntity *pObject, const Vector &vecForward, PhysGunForce_t reason )
{
	// The object must be valid
	if ( pObject == NULL )
	{
		Assert( 0 );
		return vec3_origin;
	}

	// Shouldn't ever get here with a non-vphysics object.
	IPhysicsObject *pPhysicsObject = pObject->VPhysicsGetObject();
	if ( pPhysicsObject == NULL )
	{
		Assert( 0 );
		return vec3_origin;
	}

	// Call the pickup entity's callback
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if ( pPickup != NULL && pPickup->ShouldPuntUseLaunchForces( reason ) )
		return pPickup->PhysGunLaunchVelocity( vecForward, pPhysicsObject->GetMass() );

	// Do our default behavior
	return Pickup_DefaultPhysGunLaunchVelocity(	vecForward, pPhysicsObject->GetMass() );
}

bool Pickup_ShouldPuntUseLaunchForces( CBaseEntity *pObject, PhysGunForce_t reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if ( pPickup )
	{
		return pPickup->ShouldPuntUseLaunchForces( reason );
	}
	return false;
}

