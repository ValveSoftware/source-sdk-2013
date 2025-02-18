//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_seek_and_destroy.h
// Roam the environment, attacking victims
// Michael Booth, January 2010

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/tf_bot_seek_and_destroy.h"
#include "bot/behavior/sniper/tf_bot_sniper_attack.h"
#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;
extern ConVar tf_bot_offense_must_push_time;
extern ConVar tf_bot_defense_must_defend_time;

ConVar tf_bot_debug_seek_and_destroy( "tf_bot_debug_seek_and_destroy", "0", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CTFBotSeekAndDestroy::CTFBotSeekAndDestroy( float duration )
{
	if ( duration > 0.0f )
	{
		m_giveUpTimer.Start( duration );
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSeekAndDestroy::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	RecomputeSeekPath( me );

	CTeamControlPoint *point = me->GetMyControlPoint();
	m_isPointLocked = ( point && point->IsLocked() );

	// restart the timer if we have one
	if ( m_giveUpTimer.HasStarted() )
	{
		m_giveUpTimer.Reset();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSeekAndDestroy::Update( CTFBot *me, float interval )
{
	if ( m_giveUpTimer.HasStarted() && m_giveUpTimer.IsElapsed() )
	{
		return Done( "Behavior duration elapsed" );
	}

	if ( TFGameRules()->IsInTraining() )
	{
		// if the trainee has started capturing the point, assist them
		if ( me->IsAnyPointBeingCaptured() )
		{
			return Done( "Assist trainee in capturing the point" );
		}
	}
	else
	{
		if ( me->IsCapturingPoint() )
		{
			return Done( "Keep capturing point I happened to stumble upon" );
		}

		if ( m_isPointLocked )
		{
			CTeamControlPoint *point = me->GetMyControlPoint();

			if ( point && !point->IsLocked() )
			{
				return Done( "The point just unlocked" );
			}
		}
		
		if ( !TFGameRules()->RoundHasBeenWon() && me->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat() )
		{
			return Done( "Time to push for the objective" );
		}
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat )
	{
		if ( TFGameRules()->RoundHasBeenWon() )
		{
			// hunt down the losers
			return SuspendFor( new CTFBotAttack, "Chasing down the losers" );
		}

		const float engageRange = 1000.0f;
		if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), engageRange ) )
		{
			return SuspendFor( new CTFBotAttack, "Going after an enemy" );
		}
	}

	// move towards our seek goal
	m_path.Update( me );

	if ( !m_path.IsValid() && m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( 1.0f );

		RecomputeSeekPath( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotSeekAndDestroy::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	RecomputeSeekPath( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSeekAndDestroy::OnStuck( CTFBot *me )
{
	RecomputeSeekPath( me );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSeekAndDestroy::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	RecomputeSeekPath( me );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSeekAndDestroy::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	RecomputeSeekPath( me );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotSeekAndDestroy::ShouldRetreat( const INextBot *meBot ) const
{
	CTFBot *me = (CTFBot *)meBot->GetEntity();

	if ( me->IsPlayerClass( TF_CLASS_PYRO ) )
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSeekAndDestroy::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
CTFNavArea *CTFBotSeekAndDestroy::ChooseGoalArea( CTFBot *me )
{
	CUtlVector< CTFNavArea * > goalVector;

	TheTFNavMesh()->CollectSpawnRoomThresholdAreas( &goalVector, GetEnemyTeam( me->GetTeamNumber() ) );

	CTeamControlPoint *point = me->GetMyControlPoint();
	if ( point && !point->IsLocked() )
	{
		// add current control point as a seek goal
		const CUtlVector< CTFNavArea * > *controlPointAreas = TheTFNavMesh()->GetControlPointAreas( point->GetPointIndex() );
		if ( controlPointAreas && controlPointAreas->Count() > 0 )
		{
			goalVector.AddToTail( controlPointAreas->Element( RandomInt( 0, controlPointAreas->Count()-1 ) ) );
		}
	}

	if ( tf_bot_debug_seek_and_destroy.GetBool() )
	{
		for( int i=0; i<goalVector.Count(); ++i )
		{
			TheNavMesh->AddToSelectedSet( goalVector[i] );
		}
	}

	// pick a new goal
	if ( goalVector.Count() > 0 )
	{
		return goalVector[ RandomInt( 0, goalVector.Count()-1 ) ];
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
void CTFBotSeekAndDestroy::RecomputeSeekPath( CTFBot *me )
{
	m_goalArea = ChooseGoalArea( me );
	if ( m_goalArea )
	{
		CTFBotPathCost cost( me, SAFEST_ROUTE );
		m_path.Compute( me, m_goalArea->GetCenter(), cost );
	}
	else
	{
		m_path.Invalidate();
	}
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSeekAndDestroy::OnTerritoryContested( CTFBot *me, int territoryID )
{
	return TryDone( RESULT_IMPORTANT, "Defending the point" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSeekAndDestroy::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return TryDone( RESULT_IMPORTANT, "Giving up due to point capture" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSeekAndDestroy::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return TryDone( RESULT_IMPORTANT, "Giving up due to point lost" );
}

