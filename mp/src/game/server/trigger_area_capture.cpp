//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "team_train_watcher.h"
#include "trigger_area_capture.h"
#include "player.h"
#include "teamplay_gamerules.h"
#include "team.h"
#include "team_objectiveresource.h"
#include "team_control_point_master.h"
#include "teamplayroundbased_gamerules.h"

extern ConVar mp_capstyle;
extern ConVar mp_blockstyle;
extern ConVar mp_capdeteriorate_time;

IMPLEMENT_AUTO_LIST( ITriggerAreaCaptureAutoList );

BEGIN_DATADESC(CTriggerAreaCapture)

	// Touch functions
	DEFINE_FUNCTION( CTriggerAreaCaptureShim::Touch ),

	// Think functions
	DEFINE_THINKFUNC( CaptureThink ),

	// Keyfields
	DEFINE_KEYFIELD( m_iszCapPointName,	FIELD_STRING,	"area_cap_point" ),
	DEFINE_KEYFIELD( m_flCapTime,		FIELD_FLOAT,	"area_time_to_cap" ),

//	DEFINE_FIELD( m_iCapMode, FIELD_INTEGER ),
//	DEFINE_FIELD( m_bCapturing, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_nCapturingTeam, FIELD_INTEGER ),
//	DEFINE_FIELD( m_nOwningTeam, FIELD_INTEGER ),
//	DEFINE_FIELD( m_nTeamInZone, FIELD_INTEGER ),
//	DEFINE_FIELD( m_fTimeRemaining, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flLastReductionTime, FIELD_FLOAT ),
//	DEFINE_FIELD( m_bBlocked, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_TeamData, CUtlVector < perteamdata_t > ),
//	DEFINE_FIELD( m_Blockers, CUtlVector < blockers_t > ),
//	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_hPoint, CHandle < CTeamControlPoint > ),
//	DEFINE_FIELD( m_bRequiresObject, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_iCapAttemptNumber, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTeamCanCap", InputSetTeamCanCap ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetControlPoint", InputSetControlPoint ),
	DEFINE_INPUTFUNC( FIELD_VOID, "CaptureCurrentCP", InputCaptureCurrentCP ),

	// Outputs
	DEFINE_OUTPUT( m_OnStartTeam1,	"OnStartTeam1" ),
	DEFINE_OUTPUT( m_OnStartTeam2,	"OnStartTeam2" ),
	DEFINE_OUTPUT( m_OnBreakTeam1,	"OnBreakTeam1" ),
	DEFINE_OUTPUT( m_OnBreakTeam2,	"OnBreakTeam2" ),
	DEFINE_OUTPUT( m_OnCapTeam1,	"OnCapTeam1" ),
	DEFINE_OUTPUT( m_OnCapTeam2,	"OnCapTeam2" ),

	DEFINE_OUTPUT( m_StartOutput,	"OnStartCap" ),
	DEFINE_OUTPUT( m_BreakOutput,	"OnBreakCap" ),
	DEFINE_OUTPUT( m_CapOutput,		"OnEndCap" ),

	DEFINE_OUTPUT( m_OnNumCappersChanged, "OnNumCappersChanged" ),
	DEFINE_OUTPUT( m_OnNumCappersChanged2, "OnNumCappersChanged2" ),

END_DATADESC();

LINK_ENTITY_TO_CLASS( trigger_capture_area, CTriggerAreaCapture );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTriggerAreaCapture::CTriggerAreaCapture()
{
	m_TeamData.SetSize( GetNumberOfTeams() );
	m_bStartTouch = false;
	m_hTrainWatcher = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::Spawn( void )
{
	BaseClass::Spawn();

	AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );

	InitTrigger();
	
	Precache();

	SetTouch ( &CTriggerAreaCaptureShim::Touch );		
	SetThink( &CTriggerAreaCapture::CaptureThink );
	SetNextThink( gpGlobals->curtime + AREA_THINK_TIME );

	for ( int i = 0; i < m_TeamData.Count(); i++ )
	{
		if ( m_TeamData[i].iNumRequiredToCap < 1 )
		{
			m_TeamData[i].iNumRequiredToCap = 1;
		}

		if ( m_TeamData[i].iNumRequiredToStartCap < 1 )
		{
			m_TeamData[i].iNumRequiredToStartCap = 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerAreaCapture::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( !Q_strncmp( szKeyName, "team_numcap_", 12 ) )
	{
		int iTeam = atoi(szKeyName+12);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iNumRequiredToCap = atoi(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_cancap_", 12 ) )
	{
		int iTeam = atoi(szKeyName+12);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].bCanCap = (atoi(szValue) != 0);
	}
	else if ( !Q_strncmp( szKeyName, "team_spawn_", 11 ) )
	{
		int iTeam = atoi(szKeyName+11);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iSpawnAdjust = atoi(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_startcap_", 14 ) )
	{
		int iTeam = atoi(szKeyName+14);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iNumRequiredToStartCap = atoi(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerAreaCapture::IsActive( void )
{
	return !m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( PassesTriggerFilters(pOther) && m_hPoint )
	{
		m_nOwningTeam = m_hPoint->GetOwner();

		IGameEvent *event = gameeventmanager->CreateEvent( "controlpoint_starttouch" );
		if ( event )
		{
			event->SetInt( "player", pOther->entindex() );
			event->SetInt( "area", m_hPoint->GetPointIndex() );
			gameeventmanager->FireEvent( event );
		}

		// Call capture think immediately to make it update our area's player counts.
		// If we don't do this, the player can receive the above event telling him he's
		// in a zone, but the objective resource still thinks he's not.
		m_bStartTouch = true;
		CaptureThink();
		m_bStartTouch = false;

		if ( m_bCapturing )
		{
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster )
			{
				float flRate = pMaster->GetPartialCapturePointRate();

				if ( flRate > 0.0f )
				{
					CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer(pOther);
					if ( pPlayer && pPlayer->GetTeamNumber() == m_nCapturingTeam )
					{
						pPlayer->StartScoringEscortPoints( flRate );
					}
				}		
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::EndTouch(CBaseEntity *pOther)
{
	if ( IsTouching( pOther ) && m_hPoint )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "controlpoint_endtouch" );
		if ( event )
		{
			event->SetInt( "player", pOther->entindex() );
			event->SetInt( "area", m_hPoint->GetPointIndex() );
			gameeventmanager->FireEvent( event );
		}

		// incase we leave but the area keeps capturing
		CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer(pOther);
		if ( pPlayer )
		{
			pPlayer->StopScoringEscortPoints();
		}
	}

	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerAreaCapture::CaptureModeScalesWithPlayers() const
{
	return mp_capstyle.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::AreaTouch( CBaseEntity *pOther )
{
	if ( !IsActive() )
		return;
	if ( !PassesTriggerFilters(pOther) )
		return;

	// Don't cap areas unless the round is running
	if ( !TeamplayGameRules()->PointsMayBeCaptured() )
		return;

	// dont touch for non-alive or non-players
	if( !pOther->IsPlayer() || !pOther->IsAlive() )
		return;

	// make sure this point is in the round being played (if we're playing one)
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && m_hPoint )
	{
		if ( !pMaster->IsInRound( m_hPoint ) )
		{
			return;
		}
	}

	if ( m_hPoint )
	{
		m_nOwningTeam = m_hPoint->GetOwner();
	}

	CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer(pOther);
	Assert( pPlayer );

	if ( pPlayer->GetTeamNumber() != m_nOwningTeam )
	{
		if ( m_TeamData[ pPlayer->GetTeamNumber() ].bCanCap )
		{
			DisplayCapHintTo( pPlayer );
		}
	}
}

ConVar mp_simulatemultiplecappers( "mp_simulatemultiplecappers", "1", FCVAR_CHEAT );

#define MAX_CAPTURE_TEAMS 8

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::CaptureThink( void )
{
	SetNextThink( gpGlobals->curtime + AREA_THINK_TIME );

	// make sure this point is in the round being played (if we're playing one)
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && m_hPoint )
	{
		if ( !pMaster->IsInRound( m_hPoint ) )
		{
			return;
		}
	}

	if ( !TeamplayGameRules()->PointsMayBeCaptured() )
	{
		// Points aren't allowed to be captured. If we were 
		// being captured, we need to clean up and reset.
		if ( m_bCapturing )
		{
			BreakCapture( false );
			UpdateNumPlayers();
		}
		return;
	}

	// go through our list of players
	Assert( GetNumberOfTeams() <= MAX_CAPTURE_TEAMS );
	int iNumPlayers[MAX_CAPTURE_TEAMS];
	int iNumBlockablePlayers[MAX_CAPTURE_TEAMS]; // Players in the zone who can't cap, but can block / pause caps
	CBaseMultiplayerPlayer *pFirstPlayerTouching[MAX_CAPTURE_TEAMS];
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		iNumPlayers[i] = 0;
		iNumBlockablePlayers[i] = 0;
		pFirstPlayerTouching[i] = NULL;
	}

	if ( m_hPoint )
	{
		// Loop through the entities we're touching, and find players
		for ( int i = 0; i < m_hTouchingEntities.Count(); i++ )
		{
			CBaseEntity *ent = m_hTouchingEntities[i];
			if ( ent && ent->IsPlayer() )
			{
				CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer(ent);
				if ( pPlayer->IsAlive() )
				{	
					int iTeam = pPlayer->GetTeamNumber();

					// If a team's not allowed to cap a point, don't count players in it at all
					if ( !TeamplayGameRules()->TeamMayCapturePoint( iTeam, m_hPoint->GetPointIndex() ) )
						continue;

					if ( !TeamplayGameRules()->PlayerMayCapturePoint( pPlayer, m_hPoint->GetPointIndex() ) )
					{
						if ( TeamplayGameRules()->PlayerMayBlockPoint( pPlayer, m_hPoint->GetPointIndex() ) )
						{
							if ( iNumPlayers[iTeam] == 0 && iNumBlockablePlayers[iTeam] == 0 )
							{
								pFirstPlayerTouching[iTeam] = pPlayer;
							}

							iNumBlockablePlayers[iTeam] += TeamplayGameRules()->GetCaptureValueForPlayer( pPlayer );
							pPlayer->SetLastObjectiveTime( gpGlobals->curtime );
						}
						continue;
					}

					if ( iTeam >= FIRST_GAME_TEAM )
					{
						if ( iNumPlayers[iTeam] == 0 && iNumBlockablePlayers[iTeam] == 0 )
						{
							pFirstPlayerTouching[iTeam] = pPlayer;
						}

						iNumPlayers[iTeam] += TeamplayGameRules()->GetCaptureValueForPlayer( pPlayer );
						pPlayer->SetLastObjectiveTime( gpGlobals->curtime );
					}
				}
			}
		}
	}

	int iTeamsInZone = 0;
	bool bUpdatePlayers = false;
	m_nTeamInZone = TEAM_UNASSIGNED;
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		iNumPlayers[i] *= mp_simulatemultiplecappers.GetInt();

		if ( m_TeamData[i].iNumTouching != iNumPlayers[i] )
		{
			m_TeamData[i].iNumTouching = iNumPlayers[i];
			bUpdatePlayers = true;
		}
		m_TeamData[i].iBlockedTouching = m_TeamData[i].iNumTouching;

		if ( m_TeamData[i].iNumTouching )
		{
			iTeamsInZone++;

			m_nTeamInZone = i;
		}
	}

	if ( iTeamsInZone > 1 )
	{
		m_nTeamInZone = TEAM_UNASSIGNED;
	}
	else
	{
		// If we've got non-cappable, yet blockable players here for the team that's defending, they 
		// need to block the cap. This catches cases like the TF invulnerability, which needs to block
		// caps, but isn't allowed to contribute to a cap.
		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			if ( !iNumBlockablePlayers[i] || m_nTeamInZone == i )
				continue;

			iTeamsInZone++;
		}
	}

	UpdateTeamInZone();

	bool bBlocked = false;

	// If the cap is being blocked, reset the number of players so the client
	// knows to stop the capture as well.
	if ( mp_blockstyle.GetInt() == 1 )
	{
		if ( m_bCapturing && iTeamsInZone > 1 )
		{
			bBlocked = true;

			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				iNumPlayers[i] = 0;
				if ( m_TeamData[i].iNumTouching != iNumPlayers[i] )
				{
					m_TeamData[i].iNumTouching = iNumPlayers[i];
					bUpdatePlayers = true;
				}
			}
		}
	}

	if ( bUpdatePlayers )
	{
		UpdateNumPlayers( bBlocked );
	}

	// When a player blocks, tell them the cap index and attempt number
	// only give successive blocks to them if the attempt number is different
	if ( m_bCapturing )
	{
		if ( m_hPoint )
		{
			m_hPoint->SetLastContestedAt( gpGlobals->curtime );
		}

		// Calculate the amount of modification to the cap time
		float flTimeDelta = gpGlobals->curtime - m_flLastReductionTime;

		float flReduction = flTimeDelta;
		if ( CaptureModeScalesWithPlayers() )
		{
			// Diminishing returns for successive players.
			for ( int i = 1; i < m_TeamData[m_nTeamInZone].iNumTouching; i++ )
			{
				flReduction += (flTimeDelta / (float)(i+1));
			}
		}
		m_flLastReductionTime = gpGlobals->curtime;

		//if more than one team is in the zone
		if( iTeamsInZone > 1 )
		{
			if ( !m_bBlocked )
			{
				m_bBlocked = true;
				UpdateBlocked();
			}

			// See if anyone gets credit for the block
			float flPercentToGo = m_fTimeRemaining / m_flCapTime;
			if ( CaptureModeScalesWithPlayers() )
			{
				flPercentToGo = m_fTimeRemaining / ((m_flCapTime * 2) * m_TeamData[m_nCapturingTeam].iNumRequiredToCap);
			}

			if ( ( flPercentToGo <= 0.5 || TeamplayGameRules()->PointsMayAlwaysBeBlocked() ) && m_hPoint )
			{
				// find the first player that is not on the capturing team
				// they have just broken a cap and should be rewarded		
				// tell the player the capture attempt number, for checking later
				CBaseMultiplayerPlayer *pBlockingPlayer = NULL;
				for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
				{
					if ( m_nCapturingTeam == i )
						continue;

					if ( pFirstPlayerTouching[i] )
					{
						pBlockingPlayer = pFirstPlayerTouching[i];
						break;
					}
				}
				Assert( pBlockingPlayer );

				if ( pBlockingPlayer )
				{
					bool bRepeatBlocker = false;
					for ( int i = m_Blockers.Count()-1; i >= 0; i-- )
					{
						if ( m_Blockers[i].hPlayer != pBlockingPlayer )
							continue;

						// If this guy's was a blocker, but not valid now, remove him from the list
						if ( m_Blockers[i].iCapAttemptNumber != m_iCapAttemptNumber || !IsTouching(m_Blockers[i].hPlayer) ||
							 ( TeamplayGameRules()->PointsMayAlwaysBeBlocked() && m_Blockers[i].flNextBlockTime < gpGlobals->curtime && m_bStartTouch ) )
						{
							m_Blockers.Remove(i);
							continue;
						}

						bRepeatBlocker = true;
						break;
					}

					if ( !bRepeatBlocker )
					{
                        m_hPoint->CaptureBlocked( pBlockingPlayer, NULL );

						// Add this guy to our blocker list
						int iNew = m_Blockers.AddToTail();
						m_Blockers[iNew].hPlayer = pBlockingPlayer;
						m_Blockers[iNew].iCapAttemptNumber = m_iCapAttemptNumber;
						m_Blockers[iNew].flNextBlockTime = gpGlobals->curtime + 10.0f;
					}
				}
			}

			if ( mp_blockstyle.GetInt() == 0 )
			{
				BreakCapture( false );
			}
			return;
		}

		if ( m_bBlocked )
		{
			m_bBlocked = false;
			UpdateBlocked();
		}

		float flTotalTimeToCap = m_flCapTime;
		if ( CaptureModeScalesWithPlayers() )
		{
			flTotalTimeToCap = ((m_flCapTime * 2) * m_TeamData[m_nCapturingTeam].iNumRequiredToCap);
		}

		// Now remove the reduction amount after we've determined there's only 1 team in the area
		if ( m_nCapturingTeam == m_nTeamInZone )
		{
			SetCapTimeRemaining( m_fTimeRemaining - flReduction );
		}
		else if ( m_nOwningTeam == TEAM_UNASSIGNED && m_nTeamInZone != TEAM_UNASSIGNED )
		{
			SetCapTimeRemaining( m_fTimeRemaining + flReduction );
		}
		else
		{
			// Caps deteriorate over time
			if ( TeamplayRoundBasedRules() && m_hPoint && TeamplayRoundBasedRules()->TeamMayCapturePoint(m_nCapturingTeam,m_hPoint->GetPointIndex()) )
			{
				float flDecreaseScale = CaptureModeScalesWithPlayers() ? mp_capdeteriorate_time.GetFloat() : flTotalTimeToCap;
				float flDecrease = (flTotalTimeToCap / flDecreaseScale) * flTimeDelta;
				if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->InOvertime() )
				{
					flDecrease *= 6;
				}
				SetCapTimeRemaining( m_fTimeRemaining + flDecrease );
			}
			else
			{
				SetCapTimeRemaining( flTotalTimeToCap );
			}
		}

		/*
		//if no-one is in the area
		if( iTeamsInZone == 0 )
		{
			BreakCapture( true );
			return;
		}
		
		//if they've lost the number of players needed to cap
		int iTeamMembersHere = m_TeamData[m_nCapturingTeam].iNumTouching + iNumBlockablePlayers[m_nCapturingTeam];
		if ( (iTeamMembersHere == 0 ) || (mp_capstyle.GetInt() == 0 && iTeamMembersHere < m_TeamData[m_nCapturingTeam].iNumRequiredToCap) )
		{
			BreakCapture( true );
			return;
		}
		*/

		// if the cap is done
		if ( m_fTimeRemaining <= 0 )
		{
			EndCapture( m_nCapturingTeam );
			return;		//we're done
		}
		else
		{
			// We may get several simultaneous CaptureThink calls from StartTouch if there are several players on the trigger 
			// when it is enabled (like in Raid mode). We haven't started reducing m_fTimeRemaining yet but the second call to CaptureThink
			// from StartTouch has m_bCapturing set to true and we hit this condition and call BreakCapture right away.
			// We put this check here to prevent calling BreakCapture from the StartTouch call to CaptureThink. If the capture should
			// really be broken it will happen the next time the trigger thinks on its own.
			if ( !m_bStartTouch )
			{
				if ( m_fTimeRemaining >= flTotalTimeToCap )
				{
					BreakCapture( false );
					return;
				}
			}
		}
	}
	else	
	{
		// If there are any teams in the zone that aren't the owner, try to start capping
		if ( iTeamsInZone > 0 )
		{
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				if ( !m_TeamData[i].bCanCap || m_nOwningTeam == i )
					continue;

				if ( m_TeamData[i].iNumTouching == 0 )
					continue;

				if ( m_TeamData[i].iNumTouching < m_TeamData[i].iNumRequiredToStartCap )
					continue;

				if ( !CaptureModeScalesWithPlayers() && m_TeamData[i].iNumTouching < m_TeamData[i].iNumRequiredToCap )
					continue;

				StartCapture( i, CAPTURE_NORMAL );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::SetCapTimeRemaining( float flTime )
{
	m_fTimeRemaining = flTime;

	float flCapPercentage = 0;
	if ( m_nCapturingTeam )
	{
		flCapPercentage = m_fTimeRemaining / m_flCapTime;
		if ( CaptureModeScalesWithPlayers() )
		{
			flCapPercentage = m_fTimeRemaining / ((m_flCapTime * 2) * m_TeamData[m_nCapturingTeam].iNumRequiredToCap);
		}
	}

	ObjectiveResource()->SetCPCapPercentage( m_hPoint->GetPointIndex(), flCapPercentage );

	if ( m_hPoint )
	{
		m_hPoint->UpdateCapPercentage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::SetOwner( int team )
{
	//break any current capturing
	BreakCapture( false );

	HandleRespawnTimeAdjustments( m_nOwningTeam, team );

	//set the owner to the passed value
	m_nOwningTeam = team;

	UpdateOwningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::ForceOwner( int team )
{
	SetOwner( team );

	if ( m_hPoint )
	{
		m_hPoint->ForceOwner( team );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::HandleRespawnTimeAdjustments( int oldTeam, int newTeam )
{
	if ( oldTeam > LAST_SHARED_TEAM )
	{
		// reverse the adjust made when the old team captured this point (if we made one)
		if ( m_TeamData[oldTeam].iSpawnAdjust != 0 )
		{
			TeamplayRoundBasedRules()->AddTeamRespawnWaveTime( oldTeam, -m_TeamData[oldTeam].iSpawnAdjust );
		}
	}

	if ( newTeam > LAST_SHARED_TEAM )
	{
		if ( m_TeamData[newTeam].iSpawnAdjust != 0 )
		{
			TeamplayRoundBasedRules()->AddTeamRespawnWaveTime( newTeam, m_TeamData[newTeam].iSpawnAdjust );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::StartCapture( int team, int capmode )
{
	// Remap team to get first game team = 1
	switch ( team - FIRST_GAME_TEAM+1 )
	{
	case 1: 
		m_OnStartTeam1.FireOutput( this, this );
		break;
	case 2: 
		m_OnStartTeam2.FireOutput( this, this );
		break;
	default:
		Assert(0);
		break;
	}

	m_StartOutput.FireOutput(this,this);
	
	m_nCapturingTeam = team;

	OnStartCapture( m_nCapturingTeam );

	UpdateNumPlayers();

	if ( CaptureModeScalesWithPlayers() )
	{
		SetCapTimeRemaining( ((m_flCapTime * 2) * m_TeamData[team].iNumRequiredToCap) );
	}
	else
	{
		SetCapTimeRemaining( m_flCapTime );
	}
	m_bCapturing = true;
	m_bBlocked = false;
	m_iCapMode = capmode;

	m_flLastReductionTime = gpGlobals->curtime;

	UpdateCappingTeam( m_nCapturingTeam );
	UpdateBlocked();

	if( m_hPoint )
	{
		int numcappers = 0;
		int cappingplayers[MAX_AREA_CAPPERS];

		GetNumCappingPlayers( m_nCapturingTeam, numcappers, cappingplayers );
		m_hPoint->CaptureStart( m_nCapturingTeam, numcappers, cappingplayers );
	}

	// tell all touching players to start racking up capture points
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		float flRate = pMaster->GetPartialCapturePointRate();

		if ( flRate > 0.0f )
		{
			// for each player touch
			CTeam *pTeam = GetGlobalTeam( m_nCapturingTeam );
			if ( pTeam )
			{
				for ( int i=0;i<pTeam->GetNumPlayers();i++ )
				{
					CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( pTeam->GetPlayer(i) );
					if ( pPlayer && IsTouching( pPlayer ) )
					{
						pPlayer->StartScoringEscortPoints( flRate );
					}
				}
			}
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::GetNumCappingPlayers( int team, int &numcappers, int *cappingplayers )
{
	numcappers = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *ent = UTIL_PlayerByIndex( i );
		if ( ent )
		{
			CBaseMultiplayerPlayer *player = ToBaseMultiplayerPlayer(ent);

			if ( IsTouching( player ) && ( player->GetTeamNumber() == team ) ) // need to make sure disguised spies aren't included in the list of capping players
			{
				if ( numcappers < MAX_AREA_CAPPERS-1 )
				{
					cappingplayers[numcappers] = i;
					numcappers++;
				}
			}
		}
	}

	if ( numcappers < MAX_AREA_CAPPERS )
	{
		cappingplayers[numcappers] = 0;	//null terminate :)
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::EndCapture( int team )
{
	IncrementCapAttemptNumber();

	// Remap team to get first game team = 1
	switch ( team - FIRST_GAME_TEAM+1 )
	{
	case 1: 
		m_OnCapTeam1.FireOutput( this, this );
		break;
	case 2: 
		m_OnCapTeam2.FireOutput( this, this );
		break;
	default:
		Assert(0);
		break;
	}

	m_CapOutput.FireOutput(this,this);

	int numcappers = 0;
	int cappingplayers[MAX_AREA_CAPPERS];

	GetNumCappingPlayers( team, numcappers, cappingplayers );

	// Handle this before we assign the new team as the owner of this area
	HandleRespawnTimeAdjustments( m_nOwningTeam, team );
		
	m_nOwningTeam = team;
	m_bCapturing = false;
	m_nCapturingTeam = TEAM_UNASSIGNED;
	SetCapTimeRemaining( 0 );

	// play any special cap sounds. need to do this before we update the owner of the point.
	if ( TeamplayRoundBasedRules() )
	{
		TeamplayRoundBasedRules()->PlaySpecialCapSounds( m_nOwningTeam, m_hPoint.Get() );
	}

	//there may have been more than one capper, but only report this one.
	//he hasn't gotten points yet, and his name will go in the cap string if its needed
	//first capper gets name sent and points given by flag.
	//other cappers get points manually above, no name in message

	//send the player in the cap string
	if( m_hPoint )
	{
		OnEndCapture( m_nOwningTeam );

		UpdateOwningTeam();
		m_hPoint->SetOwner( m_nOwningTeam, true, numcappers, cappingplayers );
		m_hPoint->CaptureEnd();
	}

	SetNumCappers( 0 );

	// tell all touching players to stop racking up capture points
	CTeam *pTeam = GetGlobalTeam( m_nCapturingTeam );
	if ( pTeam )
	{
		for ( int i=0;i<pTeam->GetNumPlayers();i++ )
		{
			CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( pTeam->GetPlayer(i) );
			if ( pPlayer && IsTouching( pPlayer ) )
			{	
				pPlayer->StopScoringEscortPoints();					
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::BreakCapture( bool bNotEnoughPlayers )
{
	if( m_bCapturing )
	{
		// Remap team to get first game team = 1
		switch ( m_nCapturingTeam - FIRST_GAME_TEAM+1 )
		{
		case 1: 
			m_OnBreakTeam1.FireOutput( this, this );
			break;
		case 2: 
			m_OnBreakTeam2.FireOutput( this, this );
			break;
		default:
			Assert(0);
			break;
		}

		m_BreakOutput.FireOutput(this,this);

		m_bCapturing = false;
		m_nCapturingTeam = TEAM_UNASSIGNED;

		UpdateCappingTeam( TEAM_UNASSIGNED );

		if ( bNotEnoughPlayers )
		{
			IncrementCapAttemptNumber();
		}

		SetCapTimeRemaining( 0 );

		if( m_hPoint )
		{
			m_hPoint->CaptureEnd();

			// The point reverted to it's previous owner.
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_capture_broken" );
			if ( event )
			{
				event->SetInt( "cp", m_hPoint->GetPointIndex() );
				event->SetString( "cpname", m_hPoint->GetName() );
				event->SetFloat( "time_remaining", m_fTimeRemaining );
				gameeventmanager->FireEvent( event );
			}
		}

		SetNumCappers( 0 );

		// tell all touching players to stop racking up capture points
		CTeam *pTeam = GetGlobalTeam( m_nCapturingTeam );
		if ( pTeam )
		{
			for ( int i=0;i<pTeam->GetNumPlayers();i++ )
			{
				CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( pTeam->GetPlayer(i) );
				if ( pPlayer && IsTouching( pPlayer ) )
				{
					pPlayer->StopScoringEscortPoints();					
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::IncrementCapAttemptNumber( void )
{
	m_iCapAttemptNumber++;

	m_Blockers.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::InputRoundSpawn( inputdata_t &inputdata )
{
	// find the flag we're linked to
	if( !m_hPoint )
	{
		m_hPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName(NULL, STRING(m_iszCapPointName) ) );

		if ( m_hPoint )
		{
			m_nOwningTeam = m_hPoint->GetOwner();

			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				m_hPoint->SetCappersRequiredForTeam( i, m_TeamData[i].iNumRequiredToCap );

				ObjectiveResource()->SetCPRequiredCappers( m_hPoint->GetPointIndex(), i, m_TeamData[i].iNumRequiredToCap );
				ObjectiveResource()->SetTeamCanCap( m_hPoint->GetPointIndex(), i, m_TeamData[i].bCanCap );

				if ( CaptureModeScalesWithPlayers() )
				{
					ObjectiveResource()->SetCPCapTime( m_hPoint->GetPointIndex(), i, (m_flCapTime * 2) * m_TeamData[i].iNumRequiredToCap );
				}
				else
				{
					ObjectiveResource()->SetCPCapTime( m_hPoint->GetPointIndex(), i, m_flCapTime );
				}

				ObjectiveResource()->SetCPCapTimeScalesWithPlayers( m_hPoint->GetPointIndex(), CaptureModeScalesWithPlayers() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::InputSetTeamCanCap( inputdata_t &inputdata )
{
	// Get the interaction name & target
	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	char *pszParam = strtok(parseString," ");
	if ( pszParam && pszParam[0] )
	{
		int iTeam = atoi( pszParam );
		pszParam = strtok(NULL," ");

		if ( pszParam && pszParam[0] )
		{
			bool bCanCap = (atoi(pszParam) != 0);

			if ( iTeam >= 0 && iTeam < GetNumberOfTeams() )
			{
				m_TeamData[iTeam].bCanCap = bCanCap;
				if ( m_hPoint )
				{
					ObjectiveResource()->SetTeamCanCap( m_hPoint->GetPointIndex(), iTeam, m_TeamData[iTeam].bCanCap );
				}
				return;
			}
		}
	}

	Warning("%s(%s) received SetTeamCanCap input with invalid format. Format should be: <team number> <can cap (0/1)>.\n", GetClassname(), GetDebugName() );
}

void CTriggerAreaCapture::InputCaptureCurrentCP( inputdata_t &inputdata )
{
	if ( m_bCapturing )
	{
		EndCapture( m_nCapturingTeam );
	}
}

void CTriggerAreaCapture::InputSetControlPoint( inputdata_t &inputdata )
{
	BreakCapture( false );	// clear the capping for the previous point, forces us to recalc on the new one

	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	m_iszCapPointName = MAKE_STRING( parseString );
	m_hPoint = NULL;	// force a reset of this
	InputRoundSpawn( inputdata );

	// force everyone touching to re-touch so the hud gets set up properly
	for ( int i = 0; i < m_hTouchingEntities.Count(); i++ )
	{
		CBaseEntity *ent = m_hTouchingEntities[i];
		if ( ent && ent->IsPlayer() )
		{
			EndTouch( ent );
			StartTouch( ent );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if this player's death causes a block
// return FALSE if the player is not in this area
// return TRUE otherwise ( eg player is in area, but his death does not cause break )
//-----------------------------------------------------------------------------
bool CTriggerAreaCapture::CheckIfDeathCausesBlock( CBaseMultiplayerPlayer *pVictim, CBaseMultiplayerPlayer *pKiller )
{
	if ( !pVictim || !pKiller )
		return false;

	// make sure this player is in this area
	if ( !IsTouching( pVictim ) )
		return false;

	// Teamkills shouldn't give a block reward
	if ( pVictim->GetTeamNumber() == pKiller->GetTeamNumber() )
		return true;

	// return if the area is not being capped
	if ( !m_bCapturing )
		return true;

	int iTeam = pVictim->GetTeamNumber();

	// return if this player's team is not capping the area
	if ( iTeam != m_nCapturingTeam )
		return true;

	// break early incase we kill multiple people in the same frame
	bool bBreakCap = false;
	if ( CaptureModeScalesWithPlayers() )
	{
		bBreakCap = ( m_TeamData[m_nCapturingTeam].iBlockedTouching - 1 ) <= 0;
	}
	else
	{
		bBreakCap = ( m_TeamData[m_nCapturingTeam].iBlockedTouching - 1 < m_TeamData[m_nCapturingTeam].iNumRequiredToCap );
	}

	if ( bBreakCap )
	{
		m_hPoint->CaptureBlocked( pKiller, pVictim );
		//BreakCapture( true );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::UpdateNumPlayers( bool bBlocked /*= false */ )
{
	if( !m_hPoint )
		return;

	int index = m_hPoint->GetPointIndex();
	for ( int i = 0; i < m_TeamData.Count(); i++ )
	{
		if ( i >= FIRST_GAME_TEAM && i == m_nCapturingTeam )
		{
			SetNumCappers( m_TeamData[i].iNumTouching, bBlocked );
		}

		ObjectiveResource()->SetNumPlayers( index, i, m_TeamData[i].iNumTouching );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::UpdateOwningTeam( void )
{
	if ( m_hPoint )
	{
		ObjectiveResource()->SetOwningTeam( m_hPoint->GetPointIndex(), m_nOwningTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::UpdateCappingTeam( int iTeam )
{
	if ( m_hPoint )
	{
		ObjectiveResource()->SetCappingTeam( m_hPoint->GetPointIndex(), iTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::UpdateTeamInZone( void )
{
	if ( m_hPoint )
	{
		ObjectiveResource()->SetTeamInZone( m_hPoint->GetPointIndex(), m_nTeamInZone );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::UpdateBlocked( void )
{
	if ( m_hPoint )
	{
		ObjectiveResource()->SetCapBlocked( m_hPoint->GetPointIndex(), m_bBlocked );
		m_hPoint->CaptureInterrupted( m_bBlocked );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerAreaCapture::SetNumCappers( int nNumCappers, bool bBlocked /* = false */ )
{
	m_OnNumCappersChanged.Set( nNumCappers, this, this );

	// m_OnNumCappersChanged2 sets -1 for a blocked cart (for movement decisions on hills)
	if ( bBlocked )
	{
		nNumCappers = -1;
	}

	m_OnNumCappersChanged2.Set( nNumCappers, this, this );

	if ( m_hTrainWatcher.Get() )
	{
		m_hTrainWatcher->SetNumTrainCappers( nNumCappers, this );
	}
}