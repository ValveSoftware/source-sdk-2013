//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "particle_parse.h"
#include "team_control_point_master.h"

#include "../merasmus.h"
#include "../merasmus_trick_or_treat_prop.h"
#include "merasmus_attack.h"
#include "merasmus_staff_attack.h"
#include "merasmus_stunned.h"
#include "merasmus_throwing_grenade.h"
#include "merasmus_zap.h"

//----------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusAttack::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	// smooth out the bot's path following by moving toward a point farther down the path
	m_path.SetMinLookAheadDistance( 100.0f );

	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM1 );

	m_attackTimer.Invalidate();

	m_attackTarget = NULL;
	m_attackTargetFocusTimer.Start( 0.f );

	RandomGrenadeTimer();
	RandomZapTimer();

	m_homePos = me->GetAbsOrigin();
	m_homePosRecalcTimer.Start( 3.0f );

	m_bombHeadTimer.Start( 10.f );

	return Continue();
}


//----------------------------------------------------------------------------------
bool CMerasmusAttack::IsPotentiallyChaseable( CMerasmus *me, CTFPlayer *victim )
{
	if ( !victim )
	{
		return false;
	}

	if ( !victim->IsAlive() )
	{
		// victim is dead - pick a new one
		return false;
	}

	CTFNavArea *victimArea = (CTFNavArea *)victim->GetLastKnownArea();
	if ( !victimArea || victimArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
	{
		// unreachable - pick a new victim
		return false;
	}

	if ( victim->GetGroundEntity() != NULL )
	{
		Vector victimAreaPos;
		victimArea->GetClosestPointOnArea( victim->GetAbsOrigin(), &victimAreaPos );
		if ( ( victim->GetAbsOrigin() - victimAreaPos ).AsVector2D().IsLengthGreaterThan( 50.0f ) )
		{
			// off the mesh and unreachable - pick a new victim
			return false;
		}
	}

	if ( victim->m_Shared.IsInvulnerable() )
	{
		// invulnerable - pick a new victim
		return false;
	}

	Vector toHome = m_homePos - victim->GetAbsOrigin();
	if ( toHome.IsLengthGreaterThan( tf_merasmus_chase_range.GetFloat() ) )
	{
		// too far from home - pick a new victim
		return false;
	}

	return true;
}


//----------------------------------------------------------------------------------
void CMerasmusAttack::SelectVictim( CMerasmus *me )
{
	if ( IsPotentiallyChaseable( me, m_attackTarget ) && !m_attackTargetFocusTimer.IsElapsed() )
	{
		return;
	}

	// pick a new victim to chase
	CTFPlayer *newVictim = NULL;
	CUtlVector< CTFPlayer * > playerVector;

	// collect everyone
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	float victimRangeSq = FLT_MAX;
	for( int i=0; i<playerVector.Count(); ++i )
	{
		if ( !IsPotentiallyChaseable( me, playerVector[i] ) )
		{
			continue;
		}

		float rangeSq = me->GetRangeSquaredTo( playerVector[i] );
		if ( rangeSq < victimRangeSq )
		{
			newVictim = playerVector[i];
			victimRangeSq = rangeSq;
		}
	}

	if ( newVictim )
	{
		// we have a new victim
		m_attackTargetFocusTimer.Start( tf_merasmus_chase_duration.GetFloat() );
	}

	m_attackTarget = newVictim;
}


//----------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusAttack::Update( CMerasmus *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done();
	}

	if ( me->HasStunTimer() )
	{
		return SuspendFor( new CMerasmusStunned, "Stunned!" );
	}

	SelectVictim( me );
	RecomputeHomePosition();

	if ( m_attackTarget == NULL )
	{
		// go home
		const float atHomeRange = 50.0f;
		if ( me->IsRangeGreaterThan( m_homePos, atHomeRange ) )
		{
			if ( m_path.GetAge() > 3.0f )
			{
				CMerasmusPathCost cost( me );
				m_path.Compute( me, m_homePos, cost );
			}

			m_path.Update( me );
		}
// 		else
// 		{
// 			// at home with nothing to do - taunt!
// 			if ( !me->IsMoving() && m_tauntTimer.IsElapsed() )
// 			{
// 				m_tauntTimer.Start( RandomFloat( 3.0f, 5.0f ) );
// 
// 				return SuspendFor( new CMerasmusTaunt, "Taunting because I have nothing to do." );
// 			}
// 		}
	}
	else
	{
		// chase after our chase victim
		const float standAndSwingRange = 100.0f;
		CTFPlayer *chaseVictim = m_attackTarget;

		if ( me->IsRangeGreaterThan( chaseVictim, standAndSwingRange ) || !me->IsLineOfSightClear( chaseVictim ) )
		{
			if ( m_path.GetAge() > 1.0f )
			{
				CMerasmusPathCost cost( me );
				m_path.Compute( me, chaseVictim, cost );
			}

			m_path.Update( me );
		}
	}

	if ( m_bombHeadTimer.IsElapsed() )
	{
		// bomb heads last 15 seconds - make sure we don't add more while existing ones are out
		m_bombHeadTimer.Start( 16.0f );
		me->BombHeadMode();
	}

	// swing our axe at our attack target if they are in range
	if ( m_attackTarget != NULL && m_attackTarget->IsAlive() )
	{
		if ( m_zapTimer.IsElapsed() )
		{
			RandomZapTimer();
			return SuspendFor( new CMerasmusZap, "Zap!" );
		}

		if ( me->IsRangeLessThan( m_attackTarget, tf_merasmus_attack_range.GetFloat() ) )
		{
			if ( m_attackTimer.IsElapsed() )
			{
				m_attackTimer.Start( 1.f );
				return SuspendFor( new CMerasmusStaffAttack( m_attackTarget ), "Whack!" );
			}

			me->GetLocomotionInterface()->FaceTowards( m_attackTarget->WorldSpaceCenter() );
		}

		if ( m_grenadeTimer.IsElapsed() )
		{
			RandomGrenadeTimer();
			return SuspendFor( new CMerasmusThrowingGrenade( m_attackTarget ), "Fire in the hole!" );
		}
	}

	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
	{
		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
	}


	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CMerasmus > CMerasmusAttack::OnStuck( CMerasmus *me )
{
	// we're stuck - just warp to the our next path goal
	if ( m_path.GetCurrentGoal() )
	{
		me->SetAbsOrigin( m_path.GetCurrentGoal()->pos + Vector( 0, 0, 10.0f ) );
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CMerasmus > CMerasmusAttack::OnContact( CMerasmus *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( other );
		if ( pTFPlayer )
		{
			if ( pTFPlayer->IsAlive() )
			{
				// force attack the thing we bumped into
				// this prevents us from being stuck on dispensers, for example
				m_attackTarget = pTFPlayer;
				m_attackTargetFocusTimer.Start( tf_merasmus_chase_duration.GetFloat() );
			}
		}
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
void CMerasmusAttack::RecomputeHomePosition( void )
{
	if ( !m_homePosRecalcTimer.IsElapsed() )
	{
		return;
	}

	m_homePosRecalcTimer.Reset();

	CTeamControlPoint *contestedPoint = NULL;
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			contestedPoint = pMaster->GetControlPoint( i );
			if ( contestedPoint && pMaster->IsInRound( contestedPoint ) )
			{
				if ( ObjectiveResource()->GetOwningTeam( contestedPoint->GetPointIndex() ) == TF_TEAM_BLUE )
					continue;

				// blue are the invaders
				if ( !TeamplayGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, contestedPoint->GetPointIndex() ) )
					continue;

				break;
			}
		}
	}

	if ( contestedPoint )
	{
		m_homePos = contestedPoint->GetAbsOrigin();
	}
}


void CMerasmusAttack::RandomGrenadeTimer()
{
	m_grenadeTimer.Start( RandomFloat( 2.f, 3.f ) );
}


void CMerasmusAttack::RandomZapTimer()
{
	m_zapTimer.Start( RandomFloat( 3.f, 4.f ) );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus > CMerasmusTaunt::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	m_timer.Start( 3.0f );

	const char *taunts[] =
	{
		"gesture_melee_cheer",
		"gesture_melee_go",
		"taunt01",		// wave
		"taunt06",		// thriller
		"taunt_laugh",
		NULL
	};

	// count the available taunts
	int count = 0;
	while( true )
	{
		if ( taunts[ count ] == NULL )
			break;

		++count;
	}

	// pick one and play it
	int which = RandomInt( 0, count-1 );
	me->AddGestureSequence( me->LookupSequence( taunts[ which ] ) );

	int staffBodyGroup = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( staffBodyGroup, 1 );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus > CMerasmusTaunt::Update( CMerasmus *me, float interval )
{
	if ( m_timer.IsElapsed() )
	{
		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CMerasmusTaunt::OnEnd( CMerasmus *me, Action< CMerasmus > *nextAction )
{
	// turn the staff back on
	int staffBodyGroup = me->FindBodygroupByName( "staff" );
	me->SetBodygroup( staffBodyGroup, 0 );
}



