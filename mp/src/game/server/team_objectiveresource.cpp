//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that networks the state of the game's objectives.
//			May contain data for objectives that aren't used by your mod, but
//			the extra data will never be networked as long as it's zeroed out.
//
//=============================================================================
#include "cbase.h"
#include "team_objectiveresource.h"
#include "shareddefs.h"
#include <coordsize.h>
#include "team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAPHUD_PARITY_BITS		6
#define CAPHUD_PARITY_MASK		((1<<CAPHUD_PARITY_BITS)-1)

#define LAZY_UPDATE_TIME		3

// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CBaseTeamObjectiveResource, DT_BaseTeamObjectiveResource)

	SendPropInt( SENDINFO(m_iTimerToShowInHUD), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_iStopWatchTimer), MAX_EDICT_BITS, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO(m_iNumControlPoints), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO(m_bPlayingMiniRounds) ),
	SendPropBool( SENDINFO(m_bControlPointsReset) ),
	SendPropInt( SENDINFO(m_iUpdateCapHudParity), CAPHUD_PARITY_BITS, SPROP_UNSIGNED ),

	// data variables
	SendPropArray( SendPropVector( SENDINFO_ARRAY(m_vCPPositions), -1, SPROP_COORD), m_vCPPositions ),
	SendPropArray3( SENDINFO_ARRAY3(m_bCPIsVisible), SendPropInt( SENDINFO_ARRAY(m_bCPIsVisible), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_flLazyCapPerc), SendPropFloat( SENDINFO_ARRAY(m_flLazyCapPerc) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeamIcons), SendPropInt( SENDINFO_ARRAY(m_iTeamIcons), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeamOverlays), SendPropInt( SENDINFO_ARRAY(m_iTeamOverlays), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeamReqCappers), SendPropInt( SENDINFO_ARRAY(m_iTeamReqCappers), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_flTeamCapTime), SendPropTime( SENDINFO_ARRAY(m_flTeamCapTime) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iPreviousPoints), SendPropInt( SENDINFO_ARRAY(m_iPreviousPoints), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bTeamCanCap), SendPropBool( SENDINFO_ARRAY(m_bTeamCanCap) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeamBaseIcons), SendPropInt( SENDINFO_ARRAY(m_iTeamBaseIcons), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iBaseControlPoints), SendPropInt( SENDINFO_ARRAY(m_iBaseControlPoints), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bInMiniRound), SendPropBool( SENDINFO_ARRAY(m_bInMiniRound) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iWarnOnCap), SendPropInt( SENDINFO_ARRAY(m_iWarnOnCap), 4, SPROP_UNSIGNED ) ),
	SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_iszWarnSound ) ), m_iszWarnSound ),
	SendPropArray3( SENDINFO_ARRAY3(m_flPathDistance), SendPropFloat( SENDINFO_ARRAY(m_flPathDistance), 8, 0, 0.0f, 1.0f ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iCPGroup), SendPropInt( SENDINFO_ARRAY(m_iCPGroup), 5 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bCPLocked), SendPropBool( SENDINFO_ARRAY(m_bCPLocked) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_nNumNodeHillData), SendPropInt( SENDINFO_ARRAY(m_nNumNodeHillData), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_flNodeHillData), SendPropFloat( SENDINFO_ARRAY(m_flNodeHillData), 8, 0, 0.0f, 1.0f ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bTrackAlarm), SendPropBool( SENDINFO_ARRAY(m_bTrackAlarm) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_flUnlockTimes), SendPropFloat( SENDINFO_ARRAY(m_flUnlockTimes) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bHillIsDownhill), SendPropBool( SENDINFO_ARRAY(m_bHillIsDownhill) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_flCPTimerTimes), SendPropFloat( SENDINFO_ARRAY(m_flCPTimerTimes) ) ),
	
	// state variables
	SendPropArray3( SENDINFO_ARRAY3(m_iNumTeamMembers), SendPropInt( SENDINFO_ARRAY(m_iNumTeamMembers), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iCappingTeam), SendPropInt( SENDINFO_ARRAY(m_iCappingTeam), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iTeamInZone), SendPropInt( SENDINFO_ARRAY(m_iTeamInZone), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bBlocked), SendPropInt( SENDINFO_ARRAY(m_bBlocked), 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iOwner), SendPropInt( SENDINFO_ARRAY(m_iOwner), 4, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bCPCapRateScalesWithPlayers), SendPropBool( SENDINFO_ARRAY(m_bCPCapRateScalesWithPlayers) ) ),
	SendPropString( SENDINFO(m_pszCapLayoutInHUD) ),
	SendPropFloat( SENDINFO( m_flCustomPositionX ) ),
	SendPropFloat( SENDINFO( m_flCustomPositionY ) ),

END_SEND_TABLE()

BEGIN_DATADESC( CBaseTeamObjectiveResource )
	DEFINE_FIELD( m_iTimerToShowInHUD, FIELD_INTEGER ),
	DEFINE_FIELD( m_iStopWatchTimer, FIELD_INTEGER ),
	DEFINE_FIELD( m_iNumControlPoints, FIELD_INTEGER ),
	DEFINE_FIELD( m_bPlayingMiniRounds, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bControlPointsReset, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iUpdateCapHudParity, FIELD_INTEGER ),
	DEFINE_FIELD( m_flCustomPositionX, FIELD_FLOAT ),
	DEFINE_FIELD( m_flCustomPositionY, FIELD_FLOAT ),
	DEFINE_ARRAY( m_vCPPositions, FIELD_VECTOR, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_bCPIsVisible, FIELD_INTEGER, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_flLazyCapPerc, FIELD_FLOAT, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_iTeamIcons, FIELD_INTEGER, MAX_CONTROL_POINTS*MAX_CONTROL_POINT_TEAMS ),
	DEFINE_ARRAY( m_iTeamOverlays, FIELD_INTEGER, MAX_CONTROL_POINTS*MAX_CONTROL_POINT_TEAMS ),
	DEFINE_ARRAY( m_iTeamReqCappers, FIELD_INTEGER, MAX_CONTROL_POINTS*MAX_CONTROL_POINT_TEAMS ),
	DEFINE_ARRAY( m_flTeamCapTime, FIELD_FLOAT, MAX_CONTROL_POINTS*MAX_CONTROL_POINT_TEAMS ),
	DEFINE_ARRAY( m_iPreviousPoints, FIELD_INTEGER, MAX_CONTROL_POINTS*MAX_CONTROL_POINT_TEAMS*MAX_PREVIOUS_POINTS ),
	DEFINE_ARRAY( m_bTeamCanCap, FIELD_BOOLEAN, MAX_CONTROL_POINTS*MAX_CONTROL_POINT_TEAMS ),
	DEFINE_ARRAY( m_iTeamBaseIcons, FIELD_INTEGER, MAX_TEAMS ),
	DEFINE_ARRAY( m_iBaseControlPoints, FIELD_INTEGER, MAX_TEAMS ),
	DEFINE_ARRAY( m_bInMiniRound, FIELD_BOOLEAN, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_iWarnOnCap, FIELD_INTEGER, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_iszWarnSound, FIELD_STRING, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_iNumTeamMembers, FIELD_INTEGER, MAX_CONTROL_POINTS*MAX_CONTROL_POINT_TEAMS ),
	DEFINE_ARRAY( m_iCappingTeam, FIELD_INTEGER, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_iTeamInZone, FIELD_INTEGER, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_bBlocked, FIELD_BOOLEAN, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_iOwner, FIELD_INTEGER, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_bCPCapRateScalesWithPlayers, FIELD_BOOLEAN, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_pszCapLayoutInHUD, FIELD_CHARACTER, MAX_CAPLAYOUT_LENGTH ),
	DEFINE_ARRAY( m_flCapPercentages, FIELD_FLOAT,  MAX_CONTROL_POINTS  ),
	DEFINE_ARRAY( m_iCPGroup, FIELD_INTEGER, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_bCPLocked, FIELD_BOOLEAN, MAX_CONTROL_POINTS ),
	DEFINE_ARRAY( m_nNumNodeHillData, FIELD_INTEGER, TEAM_TRAIN_MAX_TEAMS ),
	DEFINE_ARRAY( m_flNodeHillData, FIELD_FLOAT, TEAM_TRAIN_HILLS_ARRAY_SIZE ),
	DEFINE_ARRAY( m_bTrackAlarm, FIELD_BOOLEAN, TEAM_TRAIN_MAX_TEAMS ),
	DEFINE_ARRAY( m_flUnlockTimes, FIELD_FLOAT,  MAX_CONTROL_POINTS  ),
	DEFINE_ARRAY( m_flCPTimerTimes, FIELD_FLOAT,  MAX_CONTROL_POINTS  ),
	DEFINE_THINKFUNC( ObjectiveThink ),
END_DATADESC()

CBaseTeamObjectiveResource *g_pObjectiveResource = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseTeamObjectiveResource::CBaseTeamObjectiveResource()
{
	g_pObjectiveResource = this;
	m_bPlayingMiniRounds = false;
	m_iUpdateCapHudParity = 0;
	m_bControlPointsReset = false;
	m_flCustomPositionX = -1.f;
	m_flCustomPositionY = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseTeamObjectiveResource::~CBaseTeamObjectiveResource()
{
	Assert( g_pObjectiveResource == this );
	g_pObjectiveResource = NULL;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::Spawn( void )
{
	m_iNumControlPoints = 0;

	// If you hit this, you've got too many teams for the control point system to handle.
	Assert( GetNumberOfTeams() < MAX_CONTROL_POINT_TEAMS );

	for ( int i=0; i < MAX_CONTROL_POINTS; i++ )
	{
		// data variables
		m_vCPPositions.Set( i, vec3_origin );
		m_bCPIsVisible.Set( i, true );
		m_bBlocked.Set( i, false );

		// state variables
		m_iOwner.Set( i, TEAM_UNASSIGNED );
		m_iCappingTeam.Set( i, TEAM_UNASSIGNED );
		m_iTeamInZone.Set( i, TEAM_UNASSIGNED );
		m_bInMiniRound.Set( i, true );
		m_iWarnOnCap.Set( i, CP_WARN_NORMAL );
		m_iCPGroup.Set( i, TEAM_INVALID );
		m_flLazyCapPerc.Set( i, 0.0 );
		m_bCPLocked.Set( i, false );
		m_flUnlockTimes.Set( i, 0.0 );
		m_flCPTimerTimes.Set( i, -1.0 );
		m_bCPCapRateScalesWithPlayers.Set( i, true );

		for ( int team = 0; team < MAX_CONTROL_POINT_TEAMS; team++ )
		{
			int iTeamIndex = TEAM_ARRAY( i, team );

			m_iTeamIcons.Set( iTeamIndex, 0 );
			m_iTeamOverlays.Set( iTeamIndex, 0 );
			m_iTeamReqCappers.Set( iTeamIndex, 0 );
			m_flTeamCapTime.Set( iTeamIndex, 0.0f );
			m_iNumTeamMembers.Set( TEAM_ARRAY( i, team ), 0 );
			for ( int ipoint = 0; ipoint < MAX_PREVIOUS_POINTS; ipoint++ )
			{
				int iIntIndex = ipoint + (i * MAX_PREVIOUS_POINTS) + (team * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS);
				m_iPreviousPoints.Set( iIntIndex, -1 );
			}
			m_bTeamCanCap.Set( iTeamIndex, false );
		}
	}

	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_iBaseControlPoints.Set( i, -1 );
	}

	int nNumEntriesPerTeam = TEAM_TRAIN_MAX_HILLS * TEAM_TRAIN_FLOATS_PER_HILL;
	for ( int i = 0; i < TEAM_TRAIN_MAX_TEAMS; i++ )
	{
		m_nNumNodeHillData.Set( i, 0 );
		m_bTrackAlarm.Set( i, false );

		int iStartingIndex = i * nNumEntriesPerTeam;
		for ( int j = 0 ; j < nNumEntriesPerTeam ; j++ )
		{
			m_flNodeHillData.Set( iStartingIndex + j, 0 );
		}

		iStartingIndex = i * TEAM_TRAIN_MAX_HILLS;
		for ( int j = 0; j < TEAM_TRAIN_MAX_HILLS; j++ )
		{
			m_bHillIsDownhill.Set( iStartingIndex + j, 0 );
		}
	}

	SetThink( &CBaseTeamObjectiveResource::ObjectiveThink );
	SetNextThink( gpGlobals->curtime + LAZY_UPDATE_TIME );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::ObjectiveThink( void )
{
	SetNextThink( gpGlobals->curtime + LAZY_UPDATE_TIME );

	for ( int i = 0; i < m_iNumControlPoints; i++ )
	{
		if ( m_iCappingTeam[i] )
		{
			m_flLazyCapPerc.Set( i, m_flCapPercentages[i] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: The objective resource is always transmitted to clients
//-----------------------------------------------------------------------------
int CBaseTeamObjectiveResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Round is starting, reset state
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::ResetControlPoints( void )
{
	for ( int i=0; i < MAX_CONTROL_POINTS; i++ )
	{
		m_iCappingTeam.Set( i, TEAM_UNASSIGNED );
		m_iTeamInZone.Set( i, TEAM_UNASSIGNED );
		m_bInMiniRound.Set( i, true );

		for ( int team = 0; team < MAX_CONTROL_POINT_TEAMS; team++ )
		{
			m_iNumTeamMembers.Set( TEAM_ARRAY( i, team ), 0.0f );
		}
	}

	UpdateCapHudElement();
	m_bControlPointsReset = !m_bControlPointsReset;
}

//-----------------------------------------------------------------------------
// Purpose: Data setting functions
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetNumControlPoints( int num )
{
	Assert( num <= MAX_CONTROL_POINTS );
	m_iNumControlPoints = num;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPIcons( int index, int iTeam, int iIcon )
{
	AssertValidIndex(index);
	m_iTeamIcons.Set( TEAM_ARRAY( index, iTeam ), iIcon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPOverlays( int index, int iTeam, int iIcon )
{
	AssertValidIndex(index);
	m_iTeamOverlays.Set( TEAM_ARRAY( index, iTeam ), iIcon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetTeamBaseIcons( int iTeam, int iBaseIcon )
{
	Assert( iTeam >= 0 && iTeam < MAX_TEAMS );
	m_iTeamBaseIcons.Set( iTeam, iBaseIcon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPPosition( int index, const Vector& vPosition )
{
	AssertValidIndex(index);
	m_vCPPositions.Set( index, vPosition );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPVisible( int index, bool bVisible )
{
	AssertValidIndex(index);
	m_bCPIsVisible.Set( index, bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetWarnOnCap( int index, int iWarnLevel )
{
	AssertValidIndex(index);
	m_iWarnOnCap.Set( index, iWarnLevel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetWarnSound( int index, string_t iszSound )
{
	AssertValidIndex(index);
	m_iszWarnSound.Set( index, iszSound );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPGroup( int index, int iCPGroup )
{
	AssertValidIndex(index);
	m_iCPGroup.Set( index, iCPGroup );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPRequiredCappers( int index, int iTeam, int iReqPlayers )
{
	AssertValidIndex(index);
	m_iTeamReqCappers.Set( TEAM_ARRAY( index, iTeam ), iReqPlayers );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPCapTime( int index, int iTeam, float flTime )
{
	AssertValidIndex(index);
	m_flTeamCapTime.Set( TEAM_ARRAY( index, iTeam ), flTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPCapPercentage( int index, float flTime )
{
	AssertValidIndex(index);
	m_flCapPercentages[index] = flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseTeamObjectiveResource::GetCPCapPercentage( int index )
{
	AssertValidIndex(index);
	return m_flCapPercentages[index];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPUnlockTime( int index, float flTime )
{
	AssertValidIndex(index);
	m_flUnlockTimes.Set( index, flTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPTimerTime( int index, float flTime )
{
	AssertValidIndex(index);
	m_flCPTimerTimes.Set( index, flTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPCapTimeScalesWithPlayers( int index, bool bScales )
{
	AssertValidIndex(index);
	m_bCPCapRateScalesWithPlayers.Set( index, bScales );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetTeamCanCap( int index, int iTeam, bool bCanCap )
{
	AssertValidIndex(index);
	m_bTeamCanCap.Set( TEAM_ARRAY( index, iTeam ), bCanCap );
	UpdateCapHudElement();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetBaseCP( int index, int iTeam )
{
	Assert( iTeam < MAX_TEAMS );
	m_iBaseControlPoints.Set( iTeam, index );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetPreviousPoint( int index, int iTeam, int iPrevIndex, int iPrevPoint )
{
	AssertValidIndex(index);
	Assert( iPrevIndex >= 0 && iPrevIndex < MAX_PREVIOUS_POINTS );
	int iIntIndex = iPrevIndex + (index * MAX_PREVIOUS_POINTS) + (iTeam * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS);
	m_iPreviousPoints.Set( iIntIndex, iPrevPoint );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseTeamObjectiveResource::GetPreviousPointForPoint( int index, int team, int iPrevIndex )
{
	AssertValidIndex(index);
	Assert( iPrevIndex >= 0 && iPrevIndex < MAX_PREVIOUS_POINTS );
	int iIntIndex = iPrevIndex + (index * MAX_PREVIOUS_POINTS) + (team * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS);
	return m_iPreviousPoints[ iIntIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseTeamObjectiveResource::TeamCanCapPoint( int index, int team )
{
	AssertValidIndex(index);
	return m_bTeamCanCap[ TEAM_ARRAY( index, team ) ];
}

//-----------------------------------------------------------------------------
// Purpose: Data setting functions
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetNumPlayers( int index, int team, int iNumPlayers )
{
	AssertValidIndex(index);
	m_iNumTeamMembers.Set( TEAM_ARRAY( index, team ), iNumPlayers );
	UpdateCapHudElement();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::StartCap( int index, int team )
{
	AssertValidIndex(index);
	if ( m_iCappingTeam.Get( index ) != team )
	{
		m_iCappingTeam.Set( index, team );
		UpdateCapHudElement();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetOwningTeam( int index, int team )
{
	AssertValidIndex(index);
	m_iOwner.Set( index, team );

	// clear the capper
	m_iCappingTeam.Set( index, TEAM_UNASSIGNED );
	UpdateCapHudElement();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCappingTeam( int index, int team )
{
	AssertValidIndex(index);
	if ( m_iCappingTeam.Get( index ) != team )
	{
		m_iCappingTeam.Set( index, team );
		UpdateCapHudElement();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetTeamInZone( int index, int team )
{
	AssertValidIndex(index);
	if ( m_iTeamInZone.Get( index ) != team )
	{
		m_iTeamInZone.Set( index, team );
		UpdateCapHudElement();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCapBlocked( int index, bool bBlocked )
{
	AssertValidIndex(index);
	if ( m_bBlocked.Get( index ) != bBlocked )
	{
		m_bBlocked.Set( index, bBlocked );
		UpdateCapHudElement();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseTeamObjectiveResource::GetOwningTeam( int index )
{
	if ( index >= m_iNumControlPoints )
		return TEAM_UNASSIGNED;

	return m_iOwner[index];
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::UpdateCapHudElement( void )
{
	m_iUpdateCapHudParity = (m_iUpdateCapHudParity + 1) & CAPHUD_PARITY_MASK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetTrainPathDistance( int index, float flDistance )
{
	AssertValidIndex(index);

	m_flPathDistance.Set( index, flDistance );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetCPLocked( int index, bool bLocked )
{
	// This assert always fires on map load and interferes with daily development
	//AssertValidIndex(index);
	m_bCPLocked.Set( index, bLocked );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTeamObjectiveResource::SetTrackAlarm( int index, bool bAlarm )
{
	Assert( index < TEAM_TRAIN_MAX_TEAMS );
	m_bTrackAlarm.Set( index, bAlarm );
}
