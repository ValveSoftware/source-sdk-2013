//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_capture_point.cpp
// Move to and try to capture the next point
// Michael Booth, February 2009

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "trigger_area_capture.h"
#include "bot/tf_bot.h"
#include "bot/behavior/scenario/capture_point/tf_bot_capture_point.h"
#include "bot/behavior/scenario/capture_point/tf_bot_defend_point.h"
#include "bot/behavior/tf_bot_seek_and_destroy.h"


extern ConVar tf_bot_path_lookahead_range;
ConVar tf_bot_offense_must_push_time( "tf_bot_offense_must_push_time", "120", FCVAR_CHEAT, "If timer is less than this, bots will push hard to cap" );

ConVar tf_bot_capture_seek_and_destroy_min_duration( "tf_bot_capture_seek_and_destroy_min_duration", "15", FCVAR_CHEAT, "If a capturing bot decides to go hunting, this is the min duration he will hunt for before reconsidering" );
ConVar tf_bot_capture_seek_and_destroy_max_duration( "tf_bot_capture_seek_and_destroy_max_duration", "30", FCVAR_CHEAT, "If a capturing bot decides to go hunting, this is the max duration he will hunt for before reconsidering" );



//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotCapturePoint::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	VPROF_BUDGET( "CTFBotCapturePoint::OnStart", "NextBot" );

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_path.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotCapturePoint::Update( CTFBot *me, float interval )
{
	if ( TFGameRules()->InSetup() )
	{
		// wait until the gates open, then path
		m_path.Invalidate();
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		return Continue();
	}

	CTeamControlPoint *point = me->GetMyControlPoint();

	if ( point == NULL )
	{
		const float roamTime = 10.0f;
		return SuspendFor( new CTFBotSeekAndDestroy( roamTime ), "Seek and destroy until a point becomes available" );
	}

	if ( point->GetTeamNumber() == me->GetTeamNumber() )
	{
		return ChangeTo( new CTFBotDefendPoint, "We need to defend our point(s)" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	bool isPushingToCapture = ( me->IsPointBeingCaptured( point ) && !me->IsInCombat() ) ||			// a friend is capturing
							   me->IsCapturingPoint() ||												// we're capturing
							   // me->m_Shared.InCond( TF_COND_INVULNERABLE ) ||						// we're ubered
							   TFGameRules()->InOvertime() ||											// the game is in overtime
							   me->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat() ||	// nearly out of tim
							   TFGameRules()->IsInTraining() ||											// teach newbies to capture
							   me->IsNearPoint( point );


	// if we see an enemy at a good combat range, stop and engage them unless we're running out of time
	if ( !isPushingToCapture )
	{
		if ( threat && threat->IsVisibleRecently() )
		{
			return SuspendFor( new CTFBotSeekAndDestroy( RandomFloat( tf_bot_capture_seek_and_destroy_min_duration.GetFloat(), tf_bot_capture_seek_and_destroy_max_duration.GetFloat() ) ), "Too early to capture - hunting" );
		}
	}


	if ( me->IsCapturingPoint() )
	{
		// move around on the point while we capture
		const CUtlVector< CTFNavArea * > *controlPointAreas = TheTFNavMesh()->GetControlPointAreas( point->GetPointIndex() );
		if ( controlPointAreas )
		{
			if ( controlPointAreas->Count() == 0 )
			{
				Assert( controlPointAreas->Count() );
				Continue(); // this control point has no nav areas for bot to move around
			}

			// move to a random spot on this control point
			if ( m_repathTimer.IsElapsed() )
			{
				m_repathTimer.Start( RandomFloat( 0.5f, 1.0f ) );

				int which = RandomInt( 0, controlPointAreas->Count() - 1 );
				CTFNavArea *goalArea = controlPointAreas->Element( which );
				if ( goalArea )
				{
					CTFBotPathCost cost( me, DEFAULT_ROUTE );
					m_path.Compute( me, goalArea->GetRandomPoint(), cost );
				}
			}

			m_path.Update( me );
		}
	}
	else
	{
		// move toward the point, periodically repathing to account for changing situation
		if ( m_repathTimer.IsElapsed() )
		{
			VPROF_BUDGET( "CTFBotCapturePoint::Update( repath )", "NextBot" );

			CTFBotPathCost cost( me, SAFEST_ROUTE );
			m_path.Compute( me, point->GetAbsOrigin(), cost );
			m_repathTimer.Start( RandomFloat( 2.0f, 3.0f ) ); 
		}

		if ( TFGameRules()->IsInTraining() && !me->IsAnyPointBeingCaptured() )
		{
			// stop short of capturing until the human trainee starts it
			if ( m_path.GetLength() < 1000.0f )
			{
				// hold here and yell at player to get on the point
				me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_GO );

				return Continue();
			}
		}

		// move towards next capture point
		m_path.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotCapturePoint::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_repathTimer.Invalidate();
	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCapturePoint::OnStuck( CTFBot *me )
{
	m_repathTimer.Invalidate();
	me->GetLocomotionInterface()->ClearStuckStatus();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCapturePoint::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCapturePoint::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	m_repathTimer.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCapturePoint::OnTerritoryContested( CTFBot *me, int territoryID )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCapturePoint::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	// we got it, move on
	m_repathTimer.Invalidate();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCapturePoint::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotCapturePoint::ShouldRetreat( const INextBot *bot ) const
{
	CTFBot *me = (CTFBot *)bot->GetEntity();

	// if we're running out of time, we have to go for it
	if ( me->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat() )
		return ANSWER_NO;

	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotCapturePoint::ShouldHurry( const INextBot *bot ) const
{
	CTFBot *me = (CTFBot *)bot->GetEntity();

	// if we're running out of time, we have to go for it
	if ( me->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat() )
		return ANSWER_YES;

	return ANSWER_UNDEFINED;
}

