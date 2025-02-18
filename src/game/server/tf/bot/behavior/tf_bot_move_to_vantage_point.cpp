//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_move_to_vantage_point.h
// Move to a position where at least one enemy is visible
// Michael Booth, November 2009

#include "cbase.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_move_to_vantage_point.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
CTFBotMoveToVantagePoint::CTFBotMoveToVantagePoint( float maxTravelDistance )
{
	m_maxTravelDistance = maxTravelDistance;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMoveToVantagePoint::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_vantageArea = me->FindVantagePoint( m_maxTravelDistance );
	if ( !m_vantageArea )
	{
		return Done( "No vantage point found" );
	}

	m_path.Invalidate();
	m_repathTimer.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMoveToVantagePoint::Update( CTFBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleInFOVNow() )
	{
		return Done( "Enemy is visible" );
	}

	if ( !m_path.IsValid() && m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( 1.0f );

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		if ( !m_path.Compute( me, m_vantageArea->GetCenter(), cost ) )
		{
			return Done( "No path to vantage point exists" );
		}
	}

	// move along path to vantage point
	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMoveToVantagePoint::OnStuck( CTFBot *me )
{
	m_path.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMoveToVantagePoint::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryDone( RESULT_CRITICAL, "Vantage point reached" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMoveToVantagePoint::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue();
}


