//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TEAM_CONTROL_POINT_MASTER_H
#define TEAM_CONTROL_POINT_MASTER_H
#ifdef _WIN32
#pragma once
#endif

#include "utlmap.h"
#include "team.h"
#include "teamplay_gamerules.h"
#include "team_control_point.h"
#include "trigger_area_capture.h"
#include "team_objectiveresource.h"
#include "team_control_point_round.h"

#define CPM_THINK			"CTeamControlPointMasterCPMThink"
#define CPM_POSTINITTHINK	"CTeamControlPointMasterCPMPostInitThink"

//-----------------------------------------------------------------------------
// Purpose: One ControlPointMaster is spawned per level. Shortly after spawning it detects all the Control
// points in the map and puts them into the m_ControlPoints. From there it detects the state 
// where all points are captured and resets them if necessary It gives points every time interval to 
// the owners of the points
//-----------------------------------------------------------------------------
class CTeamControlPointMaster : public CBaseEntity
{
	DECLARE_CLASS( CTeamControlPointMaster, CBaseEntity );

	// Derived, game-specific control point masters must override these functions
public:
	CTeamControlPointMaster();

	// Used to find game specific entities
	virtual const char *GetControlPointName( void ) { return "team_control_point"; }
	virtual const char *GetControlPointRoundName( void ) { return "team_control_point_round"; }

public:
	virtual void Spawn( void );	
	virtual void UpdateOnRemove( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void Precache( void );	
	virtual void Activate( void );	

	void RoundRespawn( void );
	void Reset( void );

	int GetNumPoints( void ){ return m_ControlPoints.Count(); }
	int GetNumPointsOwnedByTeam( int iTeam );
	int CalcNumRoundsRemaining( int iTeam );

	bool IsActive( void ) { return ( m_bDisabled == false ); }

	void FireTeamWinOutput( int iWinningTeam );

	bool IsInRound( CTeamControlPoint *pPoint );
	void CheckWinConditions( void );

	bool WouldNewCPOwnerWinGame( CTeamControlPoint *pPoint, int iNewOwner );

	int	GetBaseControlPoint( int iTeam );
	bool IsBaseControlPoint( int iPointIndex );

	bool PlayingMiniRounds( void ){	return ( m_ControlPointRounds.Count() > 0 ); }

	float PointLastContestedAt( int point );
	CTeamControlPoint *GetControlPoint( int point )
	{
		Assert( point >= 0 );
		Assert( point < MAX_CONTROL_POINTS );

		for ( unsigned int i = 0; i < m_ControlPoints.Count(); i++ )
		{
			CTeamControlPoint *pPoint = m_ControlPoints[i];
			if ( pPoint && pPoint->GetPointIndex() == point )
				return pPoint;
		}

		return NULL;
	}
	
	CTeamControlPointRound *GetCurrentRound( void )
	{
		if ( !PlayingMiniRounds() || m_iCurrentRoundIndex == -1 )
		{
			return NULL;
		}

		return m_ControlPointRounds[m_iCurrentRoundIndex];
	}

	string_t GetRoundToUseAfterRestart( void )
	{
		int nCurrentPriority = -1;
		int nHighestPriority = -1;

		string_t nRetVal = NULL_STRING;

		if ( PlayingMiniRounds() && GetCurrentRound() )
		{
			nCurrentPriority = GetCurrentRound()->GetPriorityValue();
			nHighestPriority = GetHighestRoundPriorityValue();

			// if the current round has the highest priority, then use it again
			if ( nCurrentPriority == nHighestPriority )
			{
				nRetVal = GetCurrentRound()->GetEntityName();
			}
		}

		return nRetVal;
	}

	void FireRoundStartOutput( void );
	void FireRoundEndOutput( void );

	bool ShouldScorePerCapture( void ){ return m_bScorePerCapture; }
	bool ShouldPlayAllControlPointRounds( void ){ return m_bPlayAllRounds; }
	int NumPlayableControlPointRounds( void ); // checks to see if there are any more rounds to play (but doesn't actually "get" one to play)
	
//	void ListRounds( void );

	float GetPartialCapturePointRate( void );

	void SetLastOwnershipChangeTime( float m_flTime ) { m_flLastOwnershipChangeTime = m_flTime; }
	float GetLastOwnershipChangeTime( void ) { return m_flLastOwnershipChangeTime; }

private:
	void EXPORT CPMThink( void );

    void SetBaseControlPoints( void );
	int TeamOwnsAllPoints( CTeamControlPoint *pOverridePoint = NULL, int iOverrideNewTeam = TEAM_UNASSIGNED );

	bool FindControlPoints( void );	// look in the map to find active control points
	bool FindControlPointRounds( void );	// look in the map to find active control point rounds
	bool GetControlPointRoundToPlay( void ); // gets the next round we should play
	bool SelectSpecificRound( void ); // selects a specific round to play

	int GetHighestRoundPriorityValue( void )
	{
		int nRetVal = -1;

		// rounds are sorted with the higher priority rounds first
		for ( int i = 0 ; i < m_ControlPointRounds.Count() ; ++i )
		{
			CTeamControlPointRound *pRound = m_ControlPointRounds[i];

			if ( pRound )
			{
				if ( pRound->GetPriorityValue() > nRetVal )
				{
					nRetVal = pRound->GetPriorityValue();
				}
			}
		}

		return nRetVal;
	}

	void RegisterRoundBeingPlayed( void );

	CUtlMap<int, CTeamControlPoint *> m_ControlPoints;

	bool m_bFoundPoints;		// true when the control points have been found and the array is initialized
	
	CUtlVector<CTeamControlPointRound *> m_ControlPointRounds;
	int m_iCurrentRoundIndex;
	
	DECLARE_DATADESC();

	bool m_bDisabled;
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void InputRoundSpawn( inputdata_t &inputdata );
	void InputRoundActivate( inputdata_t &inputdata );
	void InputSetWinner( inputdata_t &inputdata );
	void InputSetWinnerAndForceCaps( inputdata_t &inputdata );
	void InputSetCapLayout( inputdata_t &inputdata );
	void InputSetCapLayoutCustomPositionX( inputdata_t &inputdata );
	void InputSetCapLayoutCustomPositionY( inputdata_t &inputdata );

	void InternalSetWinner( int iTeam );

	void HandleRandomOwnerControlPoints( void );

	string_t m_iszTeamBaseIcons[MAX_TEAMS];
	int m_iTeamBaseIcons[MAX_TEAMS];
	string_t m_iszCapLayoutInHUD;

	float m_flCustomPositionX;
	float m_flCustomPositionY;

	int m_iInvalidCapWinner;
	bool m_bSwitchTeamsOnWin;
	bool m_bScorePerCapture;
	bool m_bPlayAllRounds;

	bool m_bFirstRoundAfterRestart;

	COutputEvent m_OnWonByTeam1;
	COutputEvent m_OnWonByTeam2;

	float m_flPartialCapturePointsRate;
	float m_flLastOwnershipChangeTime;
};

extern CUtlVector< CHandle<CTeamControlPointMaster> >		g_hControlPointMasters;

#endif // TEAM_CONTROL_POINT_MASTER_H
