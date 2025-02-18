//========= Copyright Valve Corporation, All rights reserved. ============//
// ghost.cpp
// A spooky halloween ghost bot
// Michael Booth, October 2011

#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "tf_projectile_arrow.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "nav_mesh/tf_nav_area.h"
#include "ghost.h"

#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "CRagdollMagnet.h"
#include "NextBot/Behavior/BehaviorMoveTo.h"


void CC_GhostSpawn( const CCommand& args )
{
	MDLCACHE_CRITICAL_SECTION();

	CBaseEntity *entity = CreateEntityByName( "ghost" );
	if ( entity )
	{
		entity->Precache();
		DispatchSpawn( entity );

		// Now attempt to drop into the world
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
		{
			// Raise the end position a little up off the floor, place the npc and drop him down
			tr.endpos.z += 12;
			entity->Teleport( &tr.endpos, NULL, NULL );
		}
	}
}
static ConCommand ghost_spawn( "ghost_spawn", CC_GhostSpawn, "Spawns a Ghost where the player is looking.", FCVAR_GAMEDLL | FCVAR_CHEAT );


//-----------------------------------------------------------------------------------------------------
CGhost *SpawnGhost( const Vector &spot, const QAngle &angles, float lifetime )
{
	CGhost *ghost = (CGhost *)CreateEntityByName( "ghost" );
	if ( ghost )
	{
		DispatchSpawn( ghost );

		ghost->SetAbsOrigin( spot );
		ghost->SetLocalAngles( angles );
		ghost->SetLifetime( lifetime );

		return ghost;
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------------------
// The Ghost
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( ghost, CGhost );


//-----------------------------------------------------------------------------------------------------
CGhost::CGhost()
{
	ALLOCATE_INTENTION_INTERFACE( CGhost );

	m_locomotor = new CGhostLocomotion( this );

	m_eyeOffset = vec3_origin;
	m_lifetime = 10.0f;
}


//-----------------------------------------------------------------------------------------------------
CGhost::~CGhost()
{
	DEALLOCATE_INTENTION_INTERFACE;

	if ( m_locomotor )
		delete m_locomotor;
}

//-----------------------------------------------------------------------------------------------------
void CGhost::PrecacheGhost()
{
	PrecacheModel( "models/props_halloween/ghost_no_hat.mdl" );
	PrecacheParticleSystem( "ghost_appearation" );
	PrecacheScriptSound( "Halloween.GhostMoan" );
	PrecacheScriptSound( "Halloween.GhostBoo" );
	PrecacheScriptSound( "Halloween.Haunted" );
}

//-----------------------------------------------------------------------------------------------------
void CGhost::Precache()
{
	BaseClass::Precache();

	// always allow late precaching, so we don't pay the cost of the
	// Halloween Ghost for the entire year

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheGhost();

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}


//-----------------------------------------------------------------------------------------------------
void CGhost::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	SetCollisionGroup( COLLISION_GROUP_NONE );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetModel( "models/props_halloween/ghost_no_hat.mdl" );
}


//---------------------------------------------------------------------------------------------
bool CGhost::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	return false;
	//return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

	
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CGhostBehavior : public Action< CGhost >
{
public:
	//---------------------------------------------------------------------------------------------
	virtual ActionResult< CGhost > OnStart( CGhost *me, Action< CGhost > *priorAction )
	{
		m_lifeTimer.Start();
		m_stuckAnchor = me->GetAbsOrigin();
		m_stuckTimer.Start( 1.0f );

		me->GetVectors( &m_forward, NULL, NULL );

		DispatchParticleEffect( "ghost_appearation", me->WorldSpaceCenter(), me->GetAbsAngles() );

		return Continue();
	}


	//---------------------------------------------------------------------------------------------
	virtual ActionResult< CGhost > Update( CGhost *me, float interval )
	{
		if ( m_lifeTimer.IsGreaterThen( me->GetLifetime() ) || m_stuckTimer.IsElapsed() )
		{
			DispatchParticleEffect( "ghost_appearation", me->WorldSpaceCenter(), me->GetAbsAngles() );

			me->EmitSound( "Halloween.Haunted" );

			UTIL_Remove( me );
			return Done();
		}

		if ( m_moanTimer.IsElapsed() )
		{
			me->EmitSound( "Halloween.GhostMoan" );
			m_moanTimer.Start( RandomFloat( 5.0f, 7.0f ) );
		}

		DriftAroundAndAvoidObstacles( me );
		ScareNearbyPlayers( me );

		return Continue();
	}


	//---------------------------------------------------------------------------------------------
	void DriftAroundAndAvoidObstacles( CGhost *me )
	{
		const float feelerRange = 150.0f;

		Vector left( -m_forward.y, m_forward.x, 0.0f );
		Vector right( m_forward.y, -m_forward.x, 0.0f );

		CTraceFilterNoNPCsOrPlayer traceFilter( me, COLLISION_GROUP_NONE );
		trace_t resultLeft;
		UTIL_TraceLine( me->WorldSpaceCenter(), me->WorldSpaceCenter() + feelerRange * ( m_forward + left ), MASK_PLAYERSOLID, &traceFilter, &resultLeft );
		//NDebugOverlay::Line( me->WorldSpaceCenter(), me->WorldSpaceCenter() + feelerRange * ( m_forward + left ), 0, 0, 255, true, interval );

		trace_t resultRight;
		UTIL_TraceLine( me->WorldSpaceCenter(), me->WorldSpaceCenter() + feelerRange * ( m_forward + right ), MASK_PLAYERSOLID, &traceFilter, &resultRight );
		//NDebugOverlay::Line( me->WorldSpaceCenter(), me->WorldSpaceCenter() + feelerRange * ( m_forward + right ), 255, 0, 0, true, interval );

		const float turnRate = 0.2f;

		if ( resultLeft.DidHit() )
		{
			if ( resultRight.DidHit() )
			{
				// both sides hit
				if ( resultLeft.fraction < resultRight.fraction )
				{
					// left hit closer - turn right
					m_forward += turnRate * right;
				}
				else
				{
					// right hit closer - turn left
					m_forward += turnRate * left;
				}
			}
			else
			{
				// left hit - turn right
				m_forward += turnRate * right;
			}
		}
		else if ( resultRight.DidHit() )
		{
			// right hit - turn left
			m_forward += turnRate * left;
		}

		m_forward.NormalizeInPlace();

		Vector goal = 100.0f * m_forward + me->GetAbsOrigin();

		me->GetLocomotionInterface()->Approach( goal );
		me->GetLocomotionInterface()->FaceTowards( goal );
		me->GetLocomotionInterface()->Run();

		if ( me->IsRangeGreaterThan( m_stuckAnchor, 50.0f ) )
		{
			m_stuckAnchor = me->GetAbsOrigin();
			m_stuckTimer.Reset();
		}
	}


	//---------------------------------------------------------------------------------------------
	void ScareNearbyPlayers( CGhost *me )
	{
		if ( m_scareTimer.IsElapsed() )
		{
			m_scareTimer.Start( 1.0f );

			CUtlVector< CTFPlayer * > playerVector;
			CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
			CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

			for( int i=0; i<playerVector.Count(); ++i )
			{
				CTFPlayer *victim = playerVector[i];

				if ( victim && !victim->HasPurgatoryBuff() )
				{
					if ( me->IsRangeLessThan( victim, GHOST_SCARE_RADIUS ) )
					{
						if ( me->IsLineOfSightClear( victim ) )
						{
							// scare them!
							const float scareTime = 2.0f;
							const float speedReduction = 0.0f;

							// "stun by trigger" results in the Halloween "yikes" effects
							int stunFlags = TF_STUN_LOSER_STATE | TF_STUN_BY_TRIGGER;
							victim->m_Shared.StunPlayer( scareTime, speedReduction, stunFlags, NULL );
						}
					}
				}
			}
		}
	}

	virtual const char *GetName( void ) const	{ return "Behavior"; }		// return name of this action

private:
	IntervalTimer m_lifeTimer;
	CountdownTimer m_moanTimer;
	CountdownTimer m_scareTimer;
	Vector m_forward;
	Vector m_stuckAnchor;
	CountdownTimer m_stuckTimer;
};


IMPLEMENT_INTENTION_INTERFACE( CGhost, CGhostBehavior );


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// Get maximum running speed
float CGhostLocomotion::GetRunSpeed( void ) const
{
	return 90.0f;
}


//---------------------------------------------------------------------------------------------
// Return maximum acceleration of locomotor
float CGhostLocomotion::GetMaxAcceleration( void ) const
{
	return 500.0f;
}


//---------------------------------------------------------------------------------------------
// Return maximum deceleration of locomotor
float CGhostLocomotion::GetMaxDeceleration( void ) const
{
	return 500.0f;
}
