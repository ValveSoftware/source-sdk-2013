//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "obstacle_pushaway.h"
#include "props_shared.h"

#if defined( CSTRIKE_DLL )
#define SV_PUSH_CONVAR_FLAGS  (FCVAR_REPLICATED)
#else
#define SV_PUSH_CONVAR_FLAGS  (FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY)
#endif // CSTRIKE_DLL

//-----------------------------------------------------------------------------------------------------
ConVar sv_pushaway_force( "sv_pushaway_force", "30000", SV_PUSH_CONVAR_FLAGS, "How hard physics objects are pushed away from the players on the server." );
ConVar sv_pushaway_min_player_speed( "sv_pushaway_min_player_speed", "75", SV_PUSH_CONVAR_FLAGS, "If a player is moving slower than this, don't push away physics objects (enables ducking behind things)." );
ConVar sv_pushaway_max_force( "sv_pushaway_max_force", "1000", SV_PUSH_CONVAR_FLAGS, "Maximum amount of force applied to physics objects by players." );
ConVar sv_pushaway_clientside( "sv_pushaway_clientside", "0", SV_PUSH_CONVAR_FLAGS, "Clientside physics push away (0=off, 1=only localplayer, 1=all players)" );

ConVar sv_pushaway_player_force( "sv_pushaway_player_force", "200000", SV_PUSH_CONVAR_FLAGS | FCVAR_CHEAT, "How hard the player is pushed away from physics objects (falls off with inverse square of distance)." );
ConVar sv_pushaway_max_player_force( "sv_pushaway_max_player_force", "10000", SV_PUSH_CONVAR_FLAGS | FCVAR_CHEAT, "Maximum of how hard the player is pushed away from physics objects." );

#ifdef CLIENT_DLL
ConVar sv_turbophysics( "sv_turbophysics", "0", FCVAR_REPLICATED, "Turns on turbo physics" );
#else
extern ConVar sv_turbophysics;
#endif

//-----------------------------------------------------------------------------------------------------
bool IsPushAwayEntity( CBaseEntity *pEnt )
{
	if ( pEnt == NULL )
		return false;

	if ( pEnt->GetCollisionGroup() != COLLISION_GROUP_PUSHAWAY )
	{
		// Try backing away from doors that are currently rotating, to prevent blocking them
#ifndef CLIENT_DLL
		if ( FClassnameIs( pEnt, "func_door_rotating" ) )
		{
			CBaseDoor *door = dynamic_cast<CBaseDoor *>(pEnt);
			if ( !door )
			{
				return false;
			}

			if ( door->m_toggle_state != TS_GOING_UP && door->m_toggle_state != TS_GOING_DOWN )
			{
				return false;
			}
		}
		else if ( FClassnameIs( pEnt, "prop_door_rotating" ) )
		{
			CBasePropDoor *door = dynamic_cast<CBasePropDoor *>(pEnt);
			if ( !door )
			{
				return false;
			}

			if ( !door->IsDoorOpening() && !door->IsDoorClosing() )
			{
				return false;
			}
		}
		else
#endif // !CLIENT_DLL
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------
bool IsPushableEntity( CBaseEntity *pEnt )
{
	if ( pEnt == NULL )
		return false;

	if ( sv_turbophysics.GetBool() )
	{
		if ( pEnt->GetCollisionGroup() == COLLISION_GROUP_NONE )
		{
#ifdef CLIENT_DLL
			if ( FClassnameIs( pEnt, "class CPhysicsPropMultiplayer" ) )
#else
			if ( FClassnameIs( pEnt, "prop_physics_multiplayer" ) )
#endif // CLIENT_DLL
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------
#ifndef CLIENT_DLL
bool IsBreakableEntity( CBaseEntity *pEnt )
{
	if ( pEnt == NULL )
		return false;

	// If we won't be able to break it, don't try
	if ( pEnt->m_takedamage != DAMAGE_YES )
		return false;

	if ( pEnt->GetCollisionGroup() != COLLISION_GROUP_PUSHAWAY && pEnt->GetCollisionGroup() != COLLISION_GROUP_BREAKABLE_GLASS && pEnt->GetCollisionGroup() != COLLISION_GROUP_NONE )
		return false;

	if ( pEnt->m_iHealth > 200 )
		return false;

	IMultiplayerPhysics *pPhysicsInterface = dynamic_cast< IMultiplayerPhysics * >( pEnt );
	if ( pPhysicsInterface )
	{
		if ( pPhysicsInterface->GetMultiplayerPhysicsMode() != PHYSICS_MULTIPLAYER_SOLID )
			return false;
	}
	else
	{
		if ((FClassnameIs( pEnt, "func_breakable" ) || FClassnameIs( pEnt, "func_breakable_surf" )))
		{
			if (FClassnameIs( pEnt, "func_breakable_surf" ))
			{
				// don't try to break it if it has already been broken
				CBreakableSurface *surf = static_cast< CBreakableSurface * >( pEnt );

				if ( surf->m_bIsBroken )
					return false;
			}
		}
		else if ( pEnt->PhysicsSolidMaskForEntity() & CONTENTS_PLAYERCLIP )
		{
			// hostages and players use CONTENTS_PLAYERCLIP, so we can use it to ignore them
			return false;
		}
	}

	IBreakableWithPropData *pBreakableInterface = dynamic_cast< IBreakableWithPropData * >( pEnt );
	if ( pBreakableInterface )
	{
		// Bullets don't damage it - ignore
		if ( pBreakableInterface->GetDmgModBullet() <= 0.0f )
		{
			return false;
		}
	}

	CBreakableProp *pProp = dynamic_cast< CBreakableProp * >( pEnt );
	if ( pProp )
	{
		// It takes a large amount of damage to even scratch it - ignore
		if ( pProp->m_iMinHealthDmg >= 50 )
		{
			return false;
		}
	}

	return true;
}
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------------------------------
int GetPushawayEnts( CBaseCombatCharacter *pPushingEntity, CBaseEntity **ents, int nMaxEnts, float flPlayerExpand, int PartitionMask, CPushAwayEnumerator *enumerator )
{
	
	Vector vExpand( flPlayerExpand, flPlayerExpand, flPlayerExpand );

	Ray_t ray;
	ray.Init( pPushingEntity->GetAbsOrigin(), pPushingEntity->GetAbsOrigin(), pPushingEntity->GetCollideable()->OBBMins() - vExpand, pPushingEntity->GetCollideable()->OBBMaxs() + vExpand );

	CPushAwayEnumerator *physPropEnum = NULL;
	if  ( !enumerator )
	{
		physPropEnum = new CPushAwayEnumerator( ents, nMaxEnts );
		enumerator = physPropEnum;
	}

	::partition->EnumerateElementsAlongRay( PartitionMask, ray, false, enumerator );

	int numHit = enumerator->m_nAlreadyHit;

	if ( physPropEnum )
		delete physPropEnum;

	return numHit;
}

void AvoidPushawayProps( CBaseCombatCharacter *pPlayer, CUserCmd *pCmd )
{
	// Figure out what direction we're moving and the extents of the box we're going to sweep 
	// against physics objects.
	Vector currentdir;
	Vector rightdir;
	AngleVectors( pCmd->viewangles, &currentdir, &rightdir, NULL );

	CBaseEntity *props[512];
#ifdef CLIENT_DLL
	int nEnts = GetPushawayEnts( pPlayer, props, ARRAYSIZE( props ), 0.0f, PARTITION_CLIENT_SOLID_EDICTS, NULL );
#else
	int nEnts = GetPushawayEnts( pPlayer, props, ARRAYSIZE( props ), 0.0f, PARTITION_ENGINE_SOLID_EDICTS, NULL );
#endif

	const Vector & ourCenter = pPlayer->WorldSpaceCenter();
	Vector nearestPropPoint;
	Vector nearestPlayerPoint;

	for ( int i=0; i < nEnts; i++ )
	{
		// Don't respond to this entity on the client unless it has PHYSICS_MULTIPLAYER_FULL set.
		IMultiplayerPhysics *pInterface = dynamic_cast<IMultiplayerPhysics*>( props[i] );
		if ( pInterface && pInterface->GetMultiplayerPhysicsMode() != PHYSICS_MULTIPLAYER_SOLID )
			continue;

		const float minMass = 10.0f; // minimum mass that can push a player back
		const float maxMass = 30.0f; // cap at a decently large value
		float mass = maxMass;
		if ( pInterface )
		{
			mass = pInterface->GetMass();
		}
		mass = clamp( mass, minMass, maxMass );
		
		mass = MAX( mass, 0 );
		mass /= maxMass; // bring into a 0..1 range

		// Push away from the collision point. The closer our center is to the collision point,
		// the harder we push away.
		props[i]->CollisionProp()->CalcNearestPoint( ourCenter, &nearestPropPoint );
		pPlayer->CollisionProp()->CalcNearestPoint( nearestPropPoint, &nearestPlayerPoint );
		Vector vPushAway = (nearestPlayerPoint - nearestPropPoint);
		float flDist = VectorNormalize( vPushAway );

		const float MaxPushawayDistance = 5.0f;
		if ( flDist > MaxPushawayDistance && !pPlayer->CollisionProp()->IsPointInBounds( nearestPropPoint ) )
		{
			continue;
		}

		// If we're not pushing, try from our center to the nearest edge of the prop
		if ( vPushAway.IsZero() )
		{
			vPushAway = (ourCenter - nearestPropPoint);
			flDist = VectorNormalize( vPushAway );
		}

		// If we're still not pushing, try from our center to the center of the prop
		if ( vPushAway.IsZero() )
		{
			vPushAway = (ourCenter - props[i]->WorldSpaceCenter());
			flDist = VectorNormalize( vPushAway );
		}

		flDist = MAX( flDist, 1 );

		float flForce = sv_pushaway_player_force.GetFloat() / flDist * mass;
		flForce = MIN( flForce, sv_pushaway_max_player_force.GetFloat() );

#ifndef CLIENT_DLL
		pPlayer->PushawayTouch( props[i] );

		// We can get right up next to rotating doors before they start to move, so scale back our force so we don't go flying
		if ( FClassnameIs( props[i], "func_door_rotating" ) || FClassnameIs( props[i], "prop_door_rotating" ) )
#endif
		{
			flForce *= 0.25f;
		}

		vPushAway *= flForce;

		pCmd->forwardmove += vPushAway.Dot( currentdir );
		pCmd->sidemove    += vPushAway.Dot( rightdir );
	}
}

//-----------------------------------------------------------------------------------------------------
void PerformObstaclePushaway( CBaseCombatCharacter *pPushingEntity )
{
	if (  pPushingEntity->m_lifeState != LIFE_ALIVE )
		return;

	// Give a push to any barrels that we're touching.
	// The client handles adjusting our usercmd to push us away.
	CBaseEntity *props[256];

#ifdef CLIENT_DLL
	// if sv_pushaway_clientside is disabled, clientside phys objects don't bounce away
	if ( sv_pushaway_clientside.GetInt() == 0 )
		return;

	// if sv_pushaway_clientside is 1, only local player can push them
	CBasePlayer *pPlayer = pPushingEntity->IsPlayer() ? (dynamic_cast< CBasePlayer * >(pPushingEntity)) : NULL;
	if ( (sv_pushaway_clientside.GetInt() == 1) && (!pPlayer || !pPlayer->IsLocalPlayer()) )
		return;

	int nEnts = GetPushawayEnts( pPushingEntity, props, ARRAYSIZE( props ), 3.0f, PARTITION_CLIENT_RESPONSIVE_EDICTS, NULL );
#else
	int nEnts = GetPushawayEnts( pPushingEntity, props, ARRAYSIZE( props ), 3.0f, PARTITION_ENGINE_SOLID_EDICTS, NULL );
#endif
	
	for ( int i=0; i < nEnts; i++ )
	{
		// If this entity uas PHYSICS_MULTIPLAYER_FULL set (ie: it's not just debris), and we're moving too slow, don't push it away.
		// Instead, let the client bounce off it. This allows players to get close to and duck behind things without knocking them over.
		IMultiplayerPhysics *pInterface = dynamic_cast<IMultiplayerPhysics*>( props[i] );

		if ( pInterface && pInterface->GetMultiplayerPhysicsMode() == PHYSICS_MULTIPLAYER_SOLID )
		{
			if ( pPushingEntity->GetAbsVelocity().Length2D() < sv_pushaway_min_player_speed.GetFloat() )
				continue;
		}

		IPhysicsObject *pObj = props[i]->VPhysicsGetObject();

		if ( pObj )
		{		
			Vector vPushAway = (props[i]->WorldSpaceCenter() - pPushingEntity->WorldSpaceCenter());
			vPushAway.z = 0;
			
			float flDist = VectorNormalize( vPushAway );
			flDist = MAX( flDist, 1 );
			
			float flForce = sv_pushaway_force.GetFloat() / flDist;
			flForce = MIN( flForce, sv_pushaway_max_force.GetFloat() );

			pObj->ApplyForceOffset( vPushAway * flForce, pPushingEntity->WorldSpaceCenter() );
		}
	}
}
