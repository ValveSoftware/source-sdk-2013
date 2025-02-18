//========= Copyright Valve Corporation, All rights reserved. ============//
// crybaby_boss_escape.cpp
// Crybaby Boss runs to the pit in Mann Manor to escape
// Michael Booth, October 2011

#include "cbase.h"

#include "nav_mesh.h"
#include "tf_player.h"

#include "../headless_hatman.h"
#include "crybaby_boss_escape.h"
#include "crybaby_boss_attack.h"

extern ConVar tf_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CCryBabyBossEscape::OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction )
{
	m_path.SetMinLookAheadDistance( tf_bot_path_lookahead_range.GetFloat() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CCryBabyBossEscape::Update( CHeadlessHatman *me, float interval )
{
	// any nearby player that taunts me, enrages me and provokes me to attack them!
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		if ( playerVector[i]->m_Shared.InCond( TF_COND_TAUNTING ) )
		{
			const float noticeTauntRange = 750.0f;
			if ( me->IsRangeLessThan( playerVector[i], noticeTauntRange ) )
			{
				if ( playerVector[i]->IsLookingTowards( me ) )
				{
					if ( me->IsLineOfSightClear( playerVector[i] ) )
					{
						return SuspendFor( new CCryBabyBossAttack( playerVector[i] ), "DON'T LAUGH AT ME!!!" );
					}
				}
			}
		}
	}

	// push players away to avoid penetration issues
	const float pushRange = 250.0f;
	const float pushForce = 200.0f;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		Vector toPlayer = player->EyePosition() - me->GetAbsOrigin();
		float range = toPlayer.NormalizeInPlace();

		if ( range < pushRange )
		{
			// make sure we push players up and away
			toPlayer.z = 0.0f;
			toPlayer.NormalizeInPlace();
			toPlayer.z = 1.0f;

			Vector push = pushForce * toPlayer;

			player->ApplyAbsVelocityImpulse( push );
		}
	}

	// run to the pit and escape
	if ( m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CHeadlessHatmanPathCost cost( me );

		// hack for now
		CNavArea *goalArea = TheNavMesh->GetNavAreaByID( 27 );
		if ( !goalArea )
		{
			return Done( "No goal area!" );
		}

		m_path.Compute( me, goalArea->GetCenter(), cost );
	}

	m_path.Update( me );

	// animation state
	if ( me->GetLocomotionInterface()->IsAttemptingToMove() )
	{
		// play running animation
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
		}

		if ( m_footfallTimer.IsElapsed() )
		{
			m_footfallTimer.Start( 0.25f );

			// periodically shake nearby players' screens when we're moving
			UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
		}
	}
	else
	{
		// standing still
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_STAND_ITEM1 ) )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM1 );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHeadlessHatman > CCryBabyBossEscape::OnMoveToSuccess( CHeadlessHatman *me, const Path *path )
{
	UTIL_Remove( me );

	return TryDone( RESULT_CRITICAL, "Reached escape pit" );
}

