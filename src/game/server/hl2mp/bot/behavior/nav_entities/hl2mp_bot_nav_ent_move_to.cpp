//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "nav_mesh.h"
#include "hl2mp_player.h"
#include "bot/hl2mp_bot.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_move_to.h"

extern ConVar hl2mp_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
CHL2MPBotNavEntMoveTo::CHL2MPBotNavEntMoveTo( const CFuncNavPrerequisite *prereq )
{
	m_prereq = prereq;
	m_pGoalArea = NULL;
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotNavEntMoveTo::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed before we started" );
	}

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_waitTimer.Invalidate();

	CBaseEntity *target = m_prereq->GetTaskEntity();
	if ( target == NULL )
	{
		return Done( "Prerequisite target entity is NULL" );
	}

	Extent targetExtent;
	targetExtent.Init( target );

	// pick random ground position within target entity as move-to goal
	m_goalPosition = targetExtent.lo + Vector( RandomFloat( 0.0f, targetExtent.SizeX() ), RandomFloat( 0.0f, targetExtent.SizeY() ), targetExtent.SizeZ() );

	TheNavMesh->GetSimpleGroundHeight( m_goalPosition, &m_goalPosition.z );

	m_pGoalArea = (CNavArea*)TheNavMesh->GetNavArea( m_goalPosition );
	if ( !m_pGoalArea )
	{
		return Done( "There's no nav area for the goal position" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotNavEntMoveTo::Update( CHL2MPBot *me, float interval )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed" );
	}

	if ( !m_prereq->IsEnabled() )
	{
		return Done( "Prerequisite has been disabled" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_waitTimer.HasStarted() )
	{
		if ( m_waitTimer.IsElapsed() )
		{
			return Done( "Wait duration elapsed" );
		}
	}
	else
	{
		// move to the goal area
		if ( m_pGoalArea == me->GetLastKnownArea() )
		{
			// in area
			m_waitTimer.Start( m_prereq->GetTaskValue() );
		}
		else
		{
			if ( m_repathTimer.IsElapsed() )
			{
				m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

				CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
				m_path.Compute( me, m_goalPosition, cost );
			}

			// move into position
			m_path.Update( me );
		}
	}

	return Continue();
}


