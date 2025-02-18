//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that networks the state of the game's objectives.
//
//=============================================================================

#include "cbase.h"
#include "c_team_objectiveresource.h"
#include "igameevents.h"
#include "teamplayroundbased_gamerules.h"
#include "c_baseplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define RESOURCE_THINK_TIME		0.1

extern ConVar mp_capstyle;
extern ConVar mp_capdeteriorate_time;

//-----------------------------------------------------------------------------
// Purpose: Owner recv proxy
//-----------------------------------------------------------------------------
void RecvProxy_Owner( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// hacks? Not sure how else to get the index of the integer that is 
	// being transmitted.
	int index = pData->m_pRecvProp->GetOffset() / sizeof(int);

	ObjectiveResource()->SetOwningTeam( index, pData->m_Value.m_Int );
}

//-----------------------------------------------------------------------------
// Purpose: capper recv proxy
//-----------------------------------------------------------------------------
void RecvProxy_CappingTeam( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int index = pData->m_pRecvProp->GetOffset() / sizeof(int);

	ObjectiveResource()->SetCappingTeam( index, pData->m_Value.m_Int );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RecvProxy_CapLayout( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	ObjectiveResource()->SetCapLayout( pData->m_Value.m_pString );
}

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_BaseTeamObjectiveResource, DT_BaseTeamObjectiveResource, CBaseTeamObjectiveResource)
	RecvPropInt( RECVINFO(m_iTimerToShowInHUD) ),
	RecvPropInt( RECVINFO(m_iStopWatchTimer) ),

	RecvPropInt( RECVINFO(m_iNumControlPoints) ),
	RecvPropBool( RECVINFO(m_bPlayingMiniRounds) ),
	RecvPropBool( RECVINFO(m_bControlPointsReset) ),
	RecvPropInt( RECVINFO(m_iUpdateCapHudParity) ),

	RecvPropArray( RecvPropVector(RECVINFO(m_vCPPositions[0])), m_vCPPositions),
	RecvPropArray3( RECVINFO_ARRAY(m_bCPIsVisible),		RecvPropInt( RECVINFO(m_bCPIsVisible[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flLazyCapPerc),	RecvPropFloat( RECVINFO(m_flLazyCapPerc[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeamIcons),		RecvPropInt( RECVINFO(m_iTeamIcons[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeamOverlays),	RecvPropInt( RECVINFO(m_iTeamOverlays[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeamReqCappers),	RecvPropInt( RECVINFO(m_iTeamReqCappers[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flTeamCapTime),	RecvPropTime( RECVINFO(m_flTeamCapTime[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iPreviousPoints),	RecvPropInt( RECVINFO(m_iPreviousPoints[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bTeamCanCap),		RecvPropBool( RECVINFO(m_bTeamCanCap[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeamBaseIcons),	RecvPropInt( RECVINFO(m_iTeamBaseIcons[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iBaseControlPoints), RecvPropInt( RECVINFO(m_iBaseControlPoints[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bInMiniRound),		RecvPropBool( RECVINFO(m_bInMiniRound[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iWarnOnCap),		RecvPropInt( RECVINFO(m_iWarnOnCap[0]) ) ),
	RecvPropArray( RecvPropString( RECVINFO( m_iszWarnSound[0]) ), m_iszWarnSound ),
	RecvPropArray3( RECVINFO_ARRAY(m_flPathDistance),	RecvPropFloat( RECVINFO(m_flPathDistance[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iCPGroup),			RecvPropInt( RECVINFO(m_iCPGroup[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bCPLocked),		RecvPropBool( RECVINFO(m_bCPLocked[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_nNumNodeHillData),	RecvPropInt( RECVINFO(m_nNumNodeHillData[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flNodeHillData),	RecvPropFloat( RECVINFO(m_flNodeHillData[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bTrackAlarm),		RecvPropBool( RECVINFO(m_bTrackAlarm[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flUnlockTimes),	RecvPropFloat( RECVINFO(m_flUnlockTimes[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bHillIsDownhill),	RecvPropBool( RECVINFO(m_bHillIsDownhill[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flCPTimerTimes),	RecvPropFloat( RECVINFO(m_flCPTimerTimes[0]) ) ),

	// state variables
	RecvPropArray3( RECVINFO_ARRAY(m_iNumTeamMembers),	RecvPropInt( RECVINFO(m_iNumTeamMembers[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iCappingTeam),		RecvPropInt( RECVINFO(m_iCappingTeam[0]), 0, RecvProxy_CappingTeam ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeamInZone),		RecvPropInt( RECVINFO(m_iTeamInZone[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bBlocked),		RecvPropInt( RECVINFO(m_bBlocked[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iOwner),			RecvPropInt( RECVINFO(m_iOwner[0]), 0, RecvProxy_Owner ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bCPCapRateScalesWithPlayers), RecvPropBool( RECVINFO(m_bCPCapRateScalesWithPlayers[0]) ) ),
	RecvPropString( RECVINFO(m_pszCapLayoutInHUD), 0, RecvProxy_CapLayout ),
	RecvPropFloat( RECVINFO(m_flCustomPositionX) ),
	RecvPropFloat( RECVINFO(m_flCustomPositionY) ),
END_RECV_TABLE()

C_BaseTeamObjectiveResource *g_pObjectiveResource = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseTeamObjectiveResource::C_BaseTeamObjectiveResource()
{
	m_iNumControlPoints = 0;
	m_iPrevNumControlPoints = 0;
	m_pszCapLayoutInHUD[0] = 0;
	m_iUpdateCapHudParity = 0;
	m_bControlPointsReset = false;

	int i;

	for ( i=0; i < MAX_CONTROL_POINTS; i++ )
	{
		m_flCapTimeLeft[i] = 0;
		m_flCapLastThinkTime[i] = 0;
		m_flLastCapWarningTime[i] = 0;
		m_bWarnedOnFinalCap[i] = false; // have we warned
		m_iWarnOnCap[i] = CP_WARN_NORMAL; // should we warn
		m_iCPGroup[i] = -1;
		m_iszWarnSound[i][0] = 0; // what sound should be played
		m_flLazyCapPerc[i] = 0.0;
		m_flUnlockTimes[i] = 0.0;
		m_flCPTimerTimes[i] = -1.0;

		for ( int team = 0; team < MAX_CONTROL_POINT_TEAMS; team++ )
		{
			int iTeamIndex = TEAM_ARRAY( i, team );

			m_iTeamIcons[ iTeamIndex ] = 0;
			m_iTeamOverlays[ iTeamIndex ] = 0;
			m_iTeamReqCappers[ iTeamIndex ] = 0;
			m_flTeamCapTime[ iTeamIndex ] = 0.0f;
			m_iNumTeamMembers[ iTeamIndex ] = 0;
			for ( int ipoint = 0; ipoint < MAX_PREVIOUS_POINTS; ipoint++ )
			{
				int iIntIndex = ipoint + (i * MAX_PREVIOUS_POINTS) + (team * MAX_CONTROL_POINTS * MAX_PREVIOUS_POINTS);
				m_iPreviousPoints[ iIntIndex ] = -1;
			}
		}
	}

	for ( int team = 0; team < MAX_CONTROL_POINT_TEAMS; team++ )
	{
		m_iTeamBaseIcons[team] = 0;
	}

	for ( i=0; i < TEAM_TRAIN_MAX_TEAMS; i++ )
	{
		m_nNumNodeHillData[i] = 0;
		m_bTrainOnHill[i] = false;
	}

	for ( i=0; i < TEAM_TRAIN_HILLS_ARRAY_SIZE; i++ )
	{
		m_flNodeHillData[i] = 0;
	}

	m_flCustomPositionX = -1.f;
	m_flCustomPositionY = -1.f;

	g_pObjectiveResource = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseTeamObjectiveResource::~C_BaseTeamObjectiveResource()
{
	g_pObjectiveResource = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTeamObjectiveResource::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iPrevNumControlPoints = m_iNumControlPoints;
	m_iOldUpdateCapHudParity = m_iUpdateCapHudParity;
	m_bOldControlPointsReset = m_bControlPointsReset;

	m_flOldCustomPositionX = m_flCustomPositionX;
	m_flOldCustomPositionY = m_flCustomPositionY;

	memcpy( m_flOldLazyCapPerc, m_flLazyCapPerc, sizeof(float)*m_iNumControlPoints );
	memcpy( m_flOldUnlockTimes, m_flUnlockTimes, sizeof(float)*m_iNumControlPoints );
	memcpy( m_flOldCPTimerTimes, m_flCPTimerTimes, sizeof(float)*m_iNumControlPoints );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTeamObjectiveResource::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bOldControlPointsReset != m_bControlPointsReset || m_iNumControlPoints != m_iPrevNumControlPoints )
	{
		// Tell everyone we know how many control points we have
		IGameEvent *event = gameeventmanager->CreateEvent( "controlpoint_initialized" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	if ( m_iUpdateCapHudParity != m_iOldUpdateCapHudParity )
	{
		UpdateControlPoint( "controlpoint_updateimages" );
	}

	for ( int i = 0; i < m_iNumControlPoints; i++ )
	{
		if ( m_flOldLazyCapPerc[i] != m_flLazyCapPerc[i] )
		{
			m_flCapTimeLeft[i] = m_flLazyCapPerc[i] * m_flTeamCapTime[ TEAM_ARRAY(i,m_iCappingTeam[i]) ];
		}

		if ( m_flOldUnlockTimes[i] != m_flUnlockTimes[i] )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "controlpoint_unlock_updated" );
			if ( event )
			{
				event->SetInt( "index", i );
				event->SetFloat( "time", m_flUnlockTimes[i] );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		if ( m_flOldCPTimerTimes[i] != m_flCPTimerTimes[i] )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "controlpoint_timer_updated" );
			if ( event )
			{
				event->SetInt( "index", i );
				event->SetFloat( "time", m_flCPTimerTimes[i] );
				gameeventmanager->FireEventClientSide( event );
			}
		}
	}

	if ( m_flOldCustomPositionX != m_flCustomPositionX || m_flOldCustomPositionY != m_flCustomPositionY )
	{
		UpdateControlPoint( "controlpoint_updatelayout" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTeamObjectiveResource::UpdateControlPoint( const char *pszEvent, int index_ )
{
	IGameEvent *event = gameeventmanager->CreateEvent( pszEvent );
	if ( event )
	{
		event->SetInt( "index", index_ );
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float C_BaseTeamObjectiveResource::GetCPCapPercentage( int index_ )
{
	Assert( 0 <= index_ && index_ <= m_iNumControlPoints );

	float flCapLength = m_flTeamCapTime[ TEAM_ARRAY(index_,m_iCappingTeam[index_]) ];

	if( flCapLength <= 0 )
		return 0.0f;

	float flElapsedTime = flCapLength - m_flCapTimeLeft[index_];

	if( flElapsedTime > flCapLength )
		return 1.0f;

	return ( flElapsedTime / flCapLength );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BaseTeamObjectiveResource::GetNumControlPointsOwned( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return 0;

	int iTeam = pPlayer->GetTeamNumber();
	int nOwned = 0;
	for ( int i = 0; i < GetNumControlPoints(); ++i )
	{
		if ( GetOwningTeam( i ) == iTeam )
		{
			++nOwned;
		}
	}
	return nOwned;
}

//-----------------------------------------------------------------------------
// Purpose: 
//			team - 
//-----------------------------------------------------------------------------
void C_BaseTeamObjectiveResource::SetOwningTeam( int index_, int team )
{
	if ( team == m_iCappingTeam[index_] )
	{
		// successful cap, reset things
		m_iCappingTeam[index_] = TEAM_UNASSIGNED;
		m_flCapTimeLeft[index_] = 0.0f;
		m_flCapLastThinkTime[index_] = 0;
	}

	m_iOwner[index_] = team;

	UpdateControlPoint( "controlpoint_updateowner", index_ );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTeamObjectiveResource::SetCappingTeam( int index_, int team )
{
	if ( team != GetOwningTeam( index_ ) && ( team > LAST_SHARED_TEAM ) )
	{
		m_flCapTimeLeft[index_] = m_flTeamCapTime[ TEAM_ARRAY( index_,team) ];
	}
	else
	{
		m_flCapTimeLeft[index_] = 0.0;
	}

	m_iCappingTeam[index_] = team;
	m_bWarnedOnFinalCap[index_] = false;

	m_flCapLastThinkTime[index_] = gpGlobals->curtime;
	SetNextClientThink( gpGlobals->curtime + RESOURCE_THINK_TIME );
	UpdateControlPoint( "controlpoint_updatecapping", index_ );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTeamObjectiveResource::SetCapLayout( const char *pszLayout )
{
	Q_strncpy( m_pszCapLayoutInHUD, pszLayout, MAX_CAPLAYOUT_LENGTH );

	UpdateControlPoint( "controlpoint_updatelayout" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseTeamObjectiveResource::CapIsBlocked( int index_ )
{
	Assert( 0 <= index_ && index_ <= m_iNumControlPoints );

	if ( m_flCapTimeLeft[index_] )
	{
		// Blocked caps have capping teams & cap times, but no players on the point
		if ( GetNumPlayersInArea( index_, m_iCappingTeam[index_] ) == 0 )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseTeamObjectiveResource::ClientThink()
{
	BaseClass::ClientThink();

	for ( int i = 0; i < MAX_CONTROL_POINTS; i++ )
	{
		if ( m_flCapTimeLeft[i] )
		{
			if ( !IsCPBlocked(i) )
			{
				bool bDeteriorateNormally = true;

				// Make sure there is only 1 team on the cap
				int iPlayersCapping = GetNumPlayersInArea( i, GetTeamInZone(i) );
				if ( iPlayersCapping > 0 )
				{
					float flReduction = gpGlobals->curtime - m_flCapLastThinkTime[i];
					if ( mp_capstyle.GetInt() == 1 && m_bCPCapRateScalesWithPlayers[i] )
					{
						// Diminishing returns for successive players.
						for ( int iPlayer = 1; iPlayer < iPlayersCapping; iPlayer++ )
						{
							flReduction += ((gpGlobals->curtime - m_flCapLastThinkTime[i]) / (float)(iPlayer+1));
						}
					}

					if ( GetTeamInZone(i) == m_iCappingTeam[i] )
					{
						bDeteriorateNormally = false;
						m_flCapTimeLeft[i] -= flReduction;

						if ( !m_bWarnedOnFinalCap[i] )
						{
							// If this the local player's team, warn him
							C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
							if ( pPlayer )
							{
								if ( m_iCappingTeam[i] != TEAM_UNASSIGNED && 
									pPlayer->GetTeamNumber() != m_iCappingTeam[i] && 
									GetCapWarningLevel( i ) == CP_WARN_FINALCAP )
								{
									// Prevent spam
									if ( gpGlobals->curtime > ( m_flLastCapWarningTime[i] + 5 ) )
									{
										pPlayer->EmitSound( GetWarnSound( i ) );

										m_bWarnedOnFinalCap[i] = true;
										m_flLastCapWarningTime[i] = gpGlobals->curtime;
									}
								}
							}
						}
					}
					else if ( GetOwningTeam(i) == TEAM_UNASSIGNED && GetTeamInZone(i) != TEAM_UNASSIGNED )
					{
						bDeteriorateNormally = false;
						m_flCapTimeLeft[i] += flReduction;
					}
				}

				if ( bDeteriorateNormally )
				{
					// Caps deteriorate over time
					// If we're not cappable at all right now, wipe all progress
					if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->TeamMayCapturePoint(m_iCappingTeam[i],i) )
					{
						float flCapLength = m_flTeamCapTime[ TEAM_ARRAY(i,m_iCappingTeam[i]) ];
						float flDecreaseScale = m_bCPCapRateScalesWithPlayers[i] ? mp_capdeteriorate_time.GetFloat() : flCapLength;
						float flDecrease = (flCapLength / flDecreaseScale) * (gpGlobals->curtime - m_flCapLastThinkTime[i]);
						if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->InOvertime() )
						{
							flDecrease *= 6;
						}
						m_flCapTimeLeft[i] += flDecrease;
					}
					else
					{
						m_flCapTimeLeft[i] = 0.0;
					}

					m_bWarnedOnFinalCap[i] = false;
				}
			}
			
			UpdateControlPoint( "controlpoint_updatelayout", i );
			m_flCapLastThinkTime[i] = gpGlobals->curtime;
		}
	}


	SetNextClientThink( gpGlobals->curtime + RESOURCE_THINK_TIME );
}
