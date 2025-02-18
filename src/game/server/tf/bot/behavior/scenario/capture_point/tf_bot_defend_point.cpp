//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_defend_point.h
// Move to and defend current point from capture
// Michael Booth, February 2009

#include "cbase.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "trigger_area_capture.h"
#include "bot/tf_bot.h"
#include "bot/behavior/scenario/capture_point/tf_bot_defend_point.h"
#include "bot/behavior/scenario/capture_point/tf_bot_capture_point.h"
#include "bot/behavior/medic/tf_bot_medic_heal.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/tf_bot_seek_and_destroy.h"
#include "bot/behavior/engineer/tf_bot_engineer_build.h"
#include "bot/behavior/scenario/capture_point/tf_bot_defend_point_block_capture.h"
#include "bot/behavior/sniper/tf_bot_sniper_attack.h"
#include "bot/behavior/demoman/tf_bot_prepare_stickybomb_trap.h"


extern ConVar tf_bot_path_lookahead_range;
extern ConVar tf_bot_min_setup_gate_defend_range;
extern ConVar tf_bot_max_setup_gate_defend_range;
extern ConVar tf_bot_min_setup_gate_sniper_defend_range;
extern ConVar tf_bot_offense_must_push_time;

ConVar tf_bot_defense_must_defend_time( "tf_bot_defense_must_defend_time", "300", FCVAR_CHEAT, "If timer is less than this, bots will stay near point and guard" );
ConVar tf_bot_max_point_defend_range( "tf_bot_max_point_defend_range", "1250", FCVAR_CHEAT, "How far (in travel distance) from the point defending bots will take up positions" );
ConVar tf_bot_defense_debug( "tf_bot_defense_debug", "0", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDefendPoint::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_defenseArea = NULL;

	// higher skilled bots prefer to seek and destroy until the time is almost up
	static float roamChance[ CTFBot::NUM_DIFFICULTY_LEVELS ] = { 10.0f, 50.0f, 75.0f, 90.0f };
	m_isAllowedToRoam = ( RandomFloat( 0.0f, 100.0f ) < roamChance[ (int)clamp( me->GetDifficulty(), CTFBot::EASY, CTFBot::EXPERT ) ] );

	return Continue();
}


//---------------------------------------------------------------------------------------------
/**
 * Return true if we're in immediate danger of losing the point
 */
bool CTFBotDefendPoint::IsPointThreatened( CTFBot *me )
{
	CTeamControlPoint *point = me->GetMyControlPoint();

	if ( point == NULL )
		return false;

	if ( point->LastContestedAt() > 0.0f && ( gpGlobals->curtime - point->LastContestedAt() ) < 5.0f )
	{
		// the point is, or was very recently, contested
		return true;
	}

	// if we just lost a point, we should fall back and stand on the next point to defend against a rush
	if ( me->WasPointJustLost() )
	{
		return true;
	}

/*
	// if an enemy is closer to the point than we are, head them off
	// TODO: Compare time to reach, not distance
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat )
	{
		const float tolerance = 100.0f;

		float themRange = ( threat->GetLastKnownPosition() - point->GetAbsOrigin() ).Length();
		float myRange = ( me->GetAbsOrigin() - point->GetAbsOrigin() ).Length();
		if ( myRange + tolerance > themRange )
			return true;
	}
*/

	return false;
}


//---------------------------------------------------------------------------------------------
// Are we smart enough to get on the point to block the cap
bool CTFBotDefendPoint::WillBlockCapture( CTFBot *me ) const
{
	if ( TFGameRules()->IsInTraining() )
		return false;
	
	if ( me->IsDifficulty( CTFBot::EASY ) )
		return false;

	if ( me->IsDifficulty( CTFBot::NORMAL ) )
	{
		// 50% chance of blocking cap
		return me->TransientlyConsistentRandomValue() > 0.5f;
	}

	return true;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDefendPoint::Update( CTFBot *me, float interval )
{
	// King of the Hill logic
	CTeamControlPointMaster *master = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( master && master->GetNumPoints() == 1 )
	{
		// if we don't own the only point, switch to capture behavior
		CTeamControlPoint *point = master->GetControlPoint( 0 );
		if ( point && point->GetOwner() != me->GetTeamNumber() )
		{
			return ChangeTo( new CTFBotCapturePoint, "We need to capture the point!" );
		}
	}

	CTeamControlPoint *point = me->GetMyControlPoint();

	if ( point == NULL )
	{
		const float roamTime = 10.0f;
		return SuspendFor( new CTFBotSeekAndDestroy( roamTime ), "Seek and destroy until a point becomes available" );
	}

	if ( point->GetTeamNumber() != me->GetTeamNumber() )
	{
		return ChangeTo( new CTFBotCapturePoint, "We need to capture our point(s)" );
	}

	// if point in is danger - get ON the point!
	// Don't do this in training to keep things easy for the new trainee
	if ( IsPointThreatened( me ) && WillBlockCapture( me ) )
	{
		// point is being captured - get on it!
		return SuspendFor( new CTFBotDefendPointBlockCapture, "Moving to block point capture!" );
	}

	// point is safe for the moment

	// if I'm uber'd, go get 'em!
	if ( me->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		const float uberChargeTime = 6.0;
		return SuspendFor( new CTFBotSeekAndDestroy( uberChargeTime ), "Attacking because I'm uber'd!" );
	}

	if ( point && point->IsLocked() )
	{
		return SuspendFor( new CTFBotSeekAndDestroy, "Seek and destroy until the point unlocks" );
	}

	if ( m_isAllowedToRoam && me->GetTimeLeftToCapture() > tf_bot_defense_must_defend_time.GetFloat() )
	{
		return SuspendFor( new CTFBotSeekAndDestroy( 15.0f ), "Seek and destroy - we have lots of time" );
	}

	if ( TFGameRules()->InSetup() )
	{
		// don't lose patience during setup time
		m_idleTimer.Reset();
	}

	// if we see an enemy as we have a melee weapon equipped, chase them down
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	me->EquipBestWeaponForThreat( threat );

	if ( threat && threat->IsVisibleRecently() )
	{
		// we're aware of an enemy
		m_idleTimer.Reset();

		if ( me->IsPlayerClass( TF_CLASS_PYRO ) )
		{
			// go get 'em
			return SuspendFor( new CTFBotSeekAndDestroy( 15.0f ), "Going after an enemy" );
		}

		CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
		if ( myWeapon && ( myWeapon->IsMeleeWeapon() || myWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) ) )
		{
			// TODO: Check if threat is visible and if not, move to last known position
			CTFBotPathCost cost( me, me->IsPlayerClass( TF_CLASS_PYRO ) ? SAFEST_ROUTE : FASTEST_ROUTE );
			m_chasePath.Update( me, threat->GetEntity(), cost );

			return Continue();
		}
	}

	// choose where we'll defend from
	if ( m_defenseArea == NULL || m_idleTimer.IsElapsed() )
	{
		m_defenseArea = SelectAreaToDefendFrom( me );
	}

	if ( m_defenseArea )
	{
		if ( me->GetLastKnownArea() == m_defenseArea )
		{
			// at our defense position
			if ( CTFBotPrepareStickybombTrap::IsPossible( me ) )
			{
				return SuspendFor( new CTFBotPrepareStickybombTrap, "Laying sticky bombs!" );
			}
		}
		else
		{
			// move to our desired defense position, repathing periodically to account for changing situation
			VPROF_BUDGET( "CTFBotDefendPoint::Update( repath )", "NextBot" );

			if ( m_repathTimer.IsElapsed() )
			{
				m_repathTimer.Start( RandomFloat( 2.0f, 3.0f ) ); 

				CTFBotPathCost cost( me, DEFAULT_ROUTE );
				m_path.Compute( me, m_defenseArea->GetCenter(), cost );
			}

			m_path.Update( me );

			// we're not idle while we're moving to our defend position
			m_idleTimer.Reset();
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotDefendPoint::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	// may have lost point - recheck
	me->ClearMyControlPoint();
	m_repathTimer.Invalidate();
	m_path.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPoint::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result  )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPoint::OnStuck( CTFBot *me )
{
	m_path.Invalidate();
	m_defenseArea = SelectAreaToDefendFrom( me );
	me->GetLocomotionInterface()->ClearStuckStatus();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPoint::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPoint::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	m_defenseArea = SelectAreaToDefendFrom( me );
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPoint::OnTerritoryContested( CTFBot *me, int territoryID )
{
	// handled in the Update() loop
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPoint::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDefendPoint::OnTerritoryLost( CTFBot *me, int territoryID )
{
	// we lost it, fall back to next point
	me->ClearMyControlPoint();
	m_defenseArea = SelectAreaToDefendFrom( me );
	m_repathTimer.Invalidate();
	m_path.Invalidate();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
class CSelectDefenseAreaForPoint : public ISearchSurroundingAreasFunctor
{
public:
	CSelectDefenseAreaForPoint( CTFNavArea *pointArea, int myTeam, CUtlVector< CTFNavArea * > *areaVector )
	{
		m_pointArea = pointArea;
		m_myTeam = myTeam;

		// don't select areas that are beyond the point
		m_incursionFlowLimit = pointArea->GetIncursionDistance( m_myTeam ) + 250.0f;

		m_areaVector = areaVector;
		m_areaVector->RemoveAll();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( !TFGameRules()->IsInKothMode() )
		{
			// don't select areas that are beyond the point
			if ( area->GetIncursionDistance( m_myTeam ) > m_incursionFlowLimit )
				return true;
		}

		if ( area->IsPotentiallyVisible( m_pointArea ) )
		{
			// a bit of a hack here to avoid bots choosing to defend in bottom of ravine at stage 3 of dustbowl
			const float tooLow = 220.0f;
			if ( m_pointArea->GetCenter().z - area->GetCenter().z < tooLow )
			{
				// valid defense position
				m_areaVector->AddToTail( area );
			}
		}

		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
	{
		if ( adjArea->IsBlocked( TFGameRules()->IsInKothMode() ? TEAM_ANY : m_myTeam ) )
		{
			return false;
		}

		if ( travelDistanceSoFar > tf_bot_max_point_defend_range.GetFloat() )
		{
			// too far away
			return false;
		}

		const float maxHeightChange = 65.0f;
		float deltaZ = currentArea->ComputeAdjacentConnectionHeightChange( adjArea );
		return ( fabs( deltaZ ) < maxHeightChange );
	}

	CTFNavArea *m_pointArea;
	CUtlVector< CTFNavArea * > *m_areaVector;
	float m_incursionFlowLimit;
	int m_myTeam;
};


//---------------------------------------------------------------------------------------------
/**
 * Select the area where we will guard the point from
 */
CTFNavArea *CTFBotDefendPoint::SelectAreaToDefendFrom( CTFBot *me )
{
	VPROF_BUDGET( "CTFBotDefendPoint::SelectAreaToDefendFrom", "NextBot" );

	CTeamControlPoint *point = me->GetMyControlPoint();
	if ( !point )
	{
		return NULL;
	}

	// decide where we will defend from
	CUtlVector< CTFNavArea * > defenseAreas;

/*
	if ( !TFGameRules()->IsInKothMode() &&
		 point->GetTeamCapPercentage( me->GetTeamNumber() ) <= 0.0f &&	// point is currently safe
		 ( ObjectiveResource()->GetPreviousPointForPoint( point->GetPointIndex(), me->GetTeamNumber(), 0 ) < 0 ||		 // this is the first cap point
			me->IsPlayerClass( TF_CLASS_PYRO ) ) )						// pyros are skirmishers
	{
		if ( TheTFNavMesh()->GetSetupGateDefenseAreas() )
		{
			defenseAreas = *TheTFNavMesh()->GetSetupGateDefenseAreas();
		}
	}
*/

	if ( defenseAreas.Count() == 0 )
	{
		CTFNavArea *pointArea = TheTFNavMesh()->GetControlPointCenterArea( point->GetPointIndex() );
		if ( pointArea )
		{
			// search outwards from the point along walkable areas (not drop downs) to make sure we can get back to the point quickly
			CSelectDefenseAreaForPoint defenseScan( pointArea, me->GetTeamNumber(), &defenseAreas );
			SearchSurroundingAreas( pointArea, defenseScan );
		}
	}

	// select a specific area from the potential defense set
	if ( defenseAreas.Count() == 0 )
	{
		return NULL;
	}

	// how long will we wait if we don't see any action
	m_idleTimer.Start( RandomFloat( 10.0f, 20.0f ) );

	if ( tf_bot_defense_debug.GetBool() )
	{
		for( int i=0; i<defenseAreas.Count(); ++i )
		{
			defenseAreas[i]->DrawFilled( 0, 200, 200, 999.9f );
		}
	}

	// select one of the defense areas
	int which = RandomInt( 0, defenseAreas.Count()-1 );
	return defenseAreas[ which ];
}

