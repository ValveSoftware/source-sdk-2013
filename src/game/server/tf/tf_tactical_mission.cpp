//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_tactical_mission.cpp
// Team Fortress specific missions
// Michael Booth, June 2009

#include "cbase.h"
#include "nav_pathfind.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_player.h"
#include "tf_tactical_mission.h"

extern ConVar tf_bot_max_point_defend_range;


/*
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
class CCollectPointDefenseAreas : public ISearchSurroundingAreasFunctor
{
public:
	CCollectPointDefenseAreas( CTFNavArea *pointArea, bool isKingOfTheHill, CUtlVector< CNavArea * > *areaVector )
	{
		m_pointArea = pointArea;
		m_isKingOfTheHill = isKingOfTheHill;

		// don't select areas that are beyond the point
		m_incursionFlowLimit = pointArea->GetIncursionDistance( TF_TEAM_RED ) + 250.0f;

		m_areaVector = areaVector;
		m_areaVector->RemoveAll();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( !m_isKingOfTheHill )
		{
			// don't select areas that are beyond the point
			if ( area->GetIncursionDistance( TF_TEAM_RED ) > m_incursionFlowLimit )
				return true;
		}

		if ( area->IsPotentiallyVisible( m_pointArea ) )
		{
			// a bit of a hack here to avoid bots choosing to defend in bottom of ravine at stage 3 of dustbowl
			const float tooLow = 220.0f;
			if ( m_pointArea->GetCenter().z - area->GetCenter().z < tooLow )
			{
				// valid defense position
				area->SetAttributeTF( TF_NAV_DEFEND_POINT );
				m_areaVector->AddToTail( area );
			}
		}

		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
	{
		if ( adjArea->IsBlocked( m_isKingOfTheHill ? TEAM_ANY : TF_TEAM_RED ) )
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
	CUtlVector< CNavArea * > *m_areaVector;
	float m_incursionFlowLimit;
	bool m_isKingOfTheHill;
};


/ **
 * Create a zone of areas that are good defensive places for the given point
 * /
CTFDefendPointZone::CTFDefendPointZone( CTeamControlPoint *point )
{
	CTFNavArea *pointArea = static_cast< CTFNavArea * >( TheNavMesh->GetNearestNavArea( point->GetAbsOrigin() ) );
	if ( pointArea )
	{
		bool isKingOfTheHill = false;

		CTeamControlPointMaster *master = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( master )
		{
			isKingOfTheHill = ( master->GetNumPoints() == 1 );
		}

		// search outwards from the point along walkable areas (not drop downs) to make sure we can get back to the point quickly
		CCollectPointDefenseAreas collector( pointArea, isKingOfTheHill, &m_areaVector );
		SearchSurroundingAreas( pointArea, collector );
	}
}
*/


//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
/**
 * Create a zone of areas that are good sniper spots to defend the given point
 */
CTFDefendPointSniperZone::CTFDefendPointSniperZone( CTeamControlPoint *point )
{

}


//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
CTFDefendPointMission::CTFDefendPointMission( CTeamControlPoint *point )
{
//	m_defenseZone = new CTFDefendPointZone( point );
	m_sniperZone = new CTFDefendPointSniperZone( point );

	m_name.sprintf( "DefendPoint%d", point->GetPointIndex() );
}


//-----------------------------------------------------------------------------------------------------
CTFDefendPointMission::~CTFDefendPointMission( void )
{
//	delete m_defenseZone;
	delete m_sniperZone;
}


//-----------------------------------------------------------------------------------------------------
// where give player should be during this mission
const CTacticalMissionZone *CTFDefendPointMission::GetDeployZone( CBasePlayer *baseWho ) const
{
//	CTFPlayer *who = ToTFPlayer( baseWho );

// 	if ( who->IsPlayerClass( TF_CLASS_SNIPER ) )
// 		return m_sniperZone;

	return m_defenseZone;
}


//-----------------------------------------------------------------------------------------------------
// control points, setup gates, sections of cart path, etc.
const CTacticalMissionZone *CTFDefendPointMission::GetObjectiveZone( void ) const
{
	return NULL;
}


//-----------------------------------------------------------------------------------------------------
// where we expect enemies to be during this mission
const CTacticalMissionZone *CTFDefendPointMission::GetEnemyZone( void ) const
{
	return NULL;
}



//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
/**
 * Invoked when server loads a new map, after everything has been created/spawned
 */
void CTFTacticalMissionManager::OnServerActivate( void )
{
}


//-----------------------------------------------------------------------------------------------------
/**
 * Invoked when a game round restarts
 */
void CTFTacticalMissionManager::OnRoundRestart( void )
{
	// for now, delete and rebuild all the missions
	// @todo only delete/rebuild generated missions
	FOR_EACH_VEC( m_missionVector, it )
	{
		delete m_missionVector[ it ];
	}
	m_missionVector.RemoveAll();


	// generate a defensive mission for each control point
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = pMaster->GetControlPoint( i );
			if ( point && pMaster->IsInRound( point ) )
			{
				CTFNavArea *pointArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( point->WorldSpaceCenter() );
				if ( pointArea && pointArea->IsReachableByTeam( TF_TEAM_BLUE ) )
				{
					CTacticalMission *defendPointMission = new CTFDefendPointMission( point );
					TheTacticalMissions().Register( defendPointMission );
				}
			}
		}
	}
}




