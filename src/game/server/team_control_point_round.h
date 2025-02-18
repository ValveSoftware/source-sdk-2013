//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TEAM_CONTROL_POINT_ROUND_H
#define TEAM_CONTROL_POINT_ROUND_H
#ifdef _WIN32
#pragma once
#endif

#include "utlmap.h"
#include "team.h"
#include "teamplay_gamerules.h"
#include "team_control_point.h"
#include "trigger_area_capture.h"
#include "team_objectiveresource.h"


class CTeamControlPointRound : public CBaseEntity
{
	DECLARE_CLASS( CTeamControlPointRound, CBaseEntity );

public:
	virtual void Spawn( void );	
	virtual void Activate( void );	

	bool IsDisabled( void ){ return m_bDisabled; }

	int GetPointOwner( int point ); 
//	int CountAdvantageFlags( int team );
	bool WouldNewCPOwnerWinGame( CTeamControlPoint *pPoint, int iNewOwner );

	void FireTeamWinOutput( int iWinningTeam );
	
	void SelectedToPlay( void );

	int CheckWinConditions( void ); // returns the team number of the team that's won, or returns -1 if no winner

	int GetPriorityValue( void ) const { return m_nPriority; }

	bool IsPlayable( void );
	bool MakePlayable( void );
	bool IsControlPointInRound( CTeamControlPoint *pPoint );

	void FireOnStartOutput( void );
	void FireOnEndOutput( void );

	inline const char *GetName( void ) { return STRING(m_iszPrintName); }

	CHandle<CTeamControlPoint> GetPointOwnedBy( int iTeam );

	bool RoundOwnedByTeam( int iTeam ) const { return ( TeamOwnsAllPoints() == iTeam ); }
	int GetInvalidCapWinner() { return m_iInvalidCapWinner; }

	CUtlVector< CHandle<CTeamControlPoint> > m_ControlPoints;

private:
	void FindControlPoints( void );	//look in the map to find the control points for this round
	void SetupSpawnPoints( void );
	int TeamOwnsAllPoints( CTeamControlPoint *pOverridePoint = NULL, int iOverrideNewTeam = TEAM_UNASSIGNED ) const;

	DECLARE_DATADESC();

	bool m_bDisabled;
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void InputRoundSpawn( inputdata_t &inputdata );

	string_t	m_iszCPNames;
	int			m_nPriority;
	int			m_iInvalidCapWinner;
	string_t	m_iszPrintName;

	COutputEvent m_OnStart;
	COutputEvent m_OnEnd;
	COutputEvent m_OnWonByTeam1;
	COutputEvent m_OnWonByTeam2;
};

#endif // TEAM_CONTROL_POINT_ROUND_H
