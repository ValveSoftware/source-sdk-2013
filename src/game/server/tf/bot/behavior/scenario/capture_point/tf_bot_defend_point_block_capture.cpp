//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_defend_point_block_capture.h
// Move to and defend current point from capture
// Michael Booth, February 2009

#include "cbase.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "trigger_area_capture.h"
#include "bot/tf_bot.h"
#include "bot/behavior/scenario/capture_point/tf_bot_defend_point_block_capture.h"
#include "bot/behavior/medic/tf_bot_medic_heal.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/demoman/tf_bot_prepare_stickybomb_trap.h"


extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_defend_owned_point_percent( "tf_bot_defend_owned_point_percent", "0.5", FCVAR_CHEAT, "Stay on the contested point we own until enemy cap percent falls below this" );


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDefendPointBlockCapture::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_point = me->GetMyControlPoint();
	if ( m_point == NULL )
	{
		return Done( "Point is NULL" );
	}

	m_defenseArea = static_cast< CTFNavArea * >( TheTFNavMesh()->GetNearestNavArea( m_point->GetAbsOrigin() ) );
	if ( m_defenseArea == NULL )
	{
		return Done( "Can't find nav area on point" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
bool CTFBotDefendPointBlockCapture::IsPointSafe( CTFBot *me )
{
	// if a point was just captured, defend this point for awhile
	if ( me->WasPointJustLost() )
	{
		return false;
	}

	if ( m_point == NULL )
	{
		return true;
	}

	if ( m_point->GetTeamCapPercentage( me->GetTeamNumber() ) < tf_bot_defend_owned_point_percent.GetFloat() )
	{
		// we're not in complete control of this point yet
		return false;
	}

	// is point is being contested, or was just being contested, its not safe
	if ( m_point->HasBeenContested() && ( gpGlobals->curtime - m_point->LastContestedAt() ) < 5.0f )
	{
		return false;
	}

	// if we still see a near threat, stay put
	const CKnownEntity *knownThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( knownThreat )
	{
		const float dangerRange = 500.0f;
		if ( ( knownThreat->GetLastKnownPosition() - m_point->GetAbsOrigin() ).IsLengthLessThan( dangerRange ) )
			return false;
	}

	return true;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDefendPointBlockCapture::Update( CTFBot *me, float interval )
{
	// if point is safe, we can move back to our defense positions
	if ( IsPointSafe( me ) )
	{
		return Done( "Point is safe again" );
	}

	if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		// medics look ridiculous rushing to the point - they need to heal
		return SuspendFor( new CTFBotMedicHeal );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	Extent pointExtent;
	pointExtent.Init( m_point );

	bool isStandingOnThePoint = pointExtent.Contains( me->GetAbsOrigin() );

	const CUtlVector< CTFNavArea * > *controlPointAreas = TheTFNavMesh()->GetControlPointAreas( m_point->GetPointIndex() );
	if ( controlPointAreas )
	{
		for( int i=0; i<controlPointAreas->Count(); ++i )
		{
			if ( me->GetLastKnownArea() && me->GetLastKnownArea()->GetID() == controlPointAreas->Element(i)->GetID() )
			{
				isStandingOnThePoint = true;
			}
		}
	}

	if ( isStandingOnThePoint && CTFBotPrepareStickybombTrap::IsPossible( me ) )
	{
		return SuspendFor( new CTFBotPrepareStickybombTrap, "Placing stickies for defense" );
	}

	if ( controlPointAreas )
	{
		// move to a random spot on this control point
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 0.5f, 1.0f ) );

			float totalArea = 0.0f;
			int i;
			for( i=0; i<controlPointAreas->Count(); ++i )
			{
				CTFNavArea *area = controlPointAreas->Element(i);
				totalArea += area->GetSizeX() * area->GetSizeY();
			}

			float which = RandomFloat( 0.0f, totalArea - 1.0f );
			CTFNavArea *goalArea = NULL;
			for( i=0; i<controlPointAreas->Count(); ++i )
			{
				CTFNavArea *area = controlPointAreas->Element(i);
				which -= area->GetSizeX() * area->GetSizeY();
				if ( which <= 0.0f )
				{
					goalArea = area;
					break;
				}
			}

			if ( goalArea )
			{
				CTFBotPathCost cost( me, DEFAULT_ROUTE );
				m_path.Compute( me, goalArea->GetRandomPoint(), cost );
			}
		}

		m_path.Update( me );
	}
	else if ( !isStandingOnThePoint )
	{
		// get on the point!
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 0.5f, 1.0f ) ); 

			CTFBotPathCost cost( me, DEFAULT_ROUTE );
			m_path.Compute( me, ( pointExtent.lo + pointExtent.hi )/2.0f, cost );
		}

		m_path.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotDefendPointBlockCapture::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_path.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPointBlockCapture::OnStuck( CTFBot *me )
{
	m_path.Invalidate();
	me->GetLocomotionInterface()->ClearStuckStatus();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPointBlockCapture::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPointBlockCapture::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPointBlockCapture::OnTerritoryContested( CTFBot *me, int territoryID )
{
	return TryToSustain();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPointBlockCapture::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPointBlockCapture::OnTerritoryLost( CTFBot *me, int territoryID )
{
	// we lost it, fall back
	return TryDone( RESULT_CRITICAL, "Lost the point" );
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotDefendPointBlockCapture::ShouldHurry( const INextBot *me ) const
{
	// hurry up and get on the point!
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotDefendPointBlockCapture::ShouldRetreat( const INextBot *me ) const
{
	// get on the point!
	return ANSWER_NO;
}
