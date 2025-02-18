//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "hl2mp_player.h"
#include "bot/hl2mp_bot.h"
#include "bot/behavior/hl2mp_bot_move_to_vantage_point.h"

#include "nav_mesh.h"

extern ConVar hl2mp_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
CHL2MPBotMoveToVantagePoint::CHL2MPBotMoveToVantagePoint( float maxTravelDistance )
{
	m_maxTravelDistance = maxTravelDistance;
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotMoveToVantagePoint::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
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
ActionResult< CHL2MPBot >	CHL2MPBotMoveToVantagePoint::Update( CHL2MPBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleInFOVNow() )
	{
		return Done( "Enemy is visible" );
	}

	if ( !m_path.IsValid() && m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( 1.0f );

		CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
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
EventDesiredResult< CHL2MPBot > CHL2MPBotMoveToVantagePoint::OnStuck( CHL2MPBot *me )
{
	m_path.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotMoveToVantagePoint::OnMoveToSuccess( CHL2MPBot *me, const Path *path )
{
	return TryDone( RESULT_CRITICAL, "Vantage point reached" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotMoveToVantagePoint::OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue();
}


