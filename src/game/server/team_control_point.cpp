//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "team_control_point.h"
#include "player.h"
#include "teamplay_gamerules.h"
#include "teamplayroundbased_gamerules.h"
#include "team.h"
#include "team_control_point_master.h"
#include "mp_shareddefs.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"

#ifdef TF_DLL
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#endif

#define CONTROL_POINT_UNLOCK_THINK			"UnlockThink"

BEGIN_DATADESC(CTeamControlPoint)
	DEFINE_KEYFIELD( m_iszPrintName,			FIELD_STRING,	"point_printname" ),
	DEFINE_KEYFIELD( m_iCPGroup,				FIELD_INTEGER,	"point_group" ),
	DEFINE_KEYFIELD( m_iDefaultOwner,			FIELD_INTEGER,	"point_default_owner" ),
	DEFINE_KEYFIELD( m_iPointIndex,				FIELD_INTEGER,	"point_index" ),
	DEFINE_KEYFIELD( m_iWarnOnCap,				FIELD_INTEGER,	"point_warn_on_cap" ),
	DEFINE_KEYFIELD( m_iszWarnSound,			FIELD_STRING,	"point_warn_sound" ),

	DEFINE_KEYFIELD( m_iszCaptureStartSound,	FIELD_STRING,	"point_capture_start_sound" ),
	DEFINE_KEYFIELD( m_iszCaptureEndSound,		FIELD_STRING,	"point_capture_end_sound" ),
	DEFINE_KEYFIELD( m_iszCaptureInProgress,	FIELD_STRING,	"point_capture_progress_sound" ),
	DEFINE_KEYFIELD( m_iszCaptureInterrupted,	FIELD_STRING,	"point_capture_interrupted_sound" ),
	DEFINE_KEYFIELD( m_bRandomOwnerOnRestart,	FIELD_BOOLEAN,	"random_owner_on_restart" ),
	DEFINE_KEYFIELD( m_bLocked,					FIELD_BOOLEAN,	"point_start_locked" ),

	DEFINE_FUNCTION( UnlockThink ),

//	DEFINE_FIELD( m_iTeam, FIELD_INTEGER ),
//	DEFINE_FIELD( m_iIndex, FIELD_INTEGER ),
//	DEFINE_FIELD( m_TeamData, CUtlVector < perteamdata_t > ),
//	DEFINE_FIELD( m_bPointVisible, FIELD_INTEGER ),
//	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_iszName, FIELD_STRING ),
//	DEFINE_FIELD( m_bStartDisabled, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_flLastContestedAt, FIELD_FLOAT ),
//	DEFINE_FIELD( m_pCaptureInProgressSound, CSoundPatch ),

	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetOwner",			InputSetOwner ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"ShowModel",		InputShowModel ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"HideModel",		InputHideModel ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"RoundActivate",	InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetLocked",		InputSetLocked ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetUnlockTime",	InputSetUnlockTime ),

	DEFINE_OUTPUT(	m_OnCapTeam1,		"OnCapTeam1" ),	// these are fired whenever the point changes modes
	DEFINE_OUTPUT(	m_OnCapTeam2,		"OnCapTeam2" ),
	DEFINE_OUTPUT(	m_OnCapReset,		"OnCapReset" ),

	DEFINE_OUTPUT(	m_OnOwnerChangedToTeam1,	"OnOwnerChangedToTeam1" ),	// these are fired when a team does the work to change the owner
	DEFINE_OUTPUT(	m_OnOwnerChangedToTeam2,	"OnOwnerChangedToTeam2" ),

	DEFINE_OUTPUT(	m_OnRoundStartOwnedByTeam1,	"OnRoundStartOwnedByTeam1" ),	// these are fired when a round is starting
	DEFINE_OUTPUT(	m_OnRoundStartOwnedByTeam2,	"OnRoundStartOwnedByTeam2" ),

	DEFINE_OUTPUT(	m_OnUnlocked, "OnUnlocked" ),

	DEFINE_THINKFUNC( AnimThink ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( team_control_point, CTeamControlPoint );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamControlPoint::CTeamControlPoint()
{
	m_TeamData.SetSize( GetNumberOfTeams() );
	m_pCaptureInProgressSound = NULL;

	m_bLocked = false;
	m_flUnlockTime = -1;
	m_bBotsIgnore = false;

#ifdef  TF_DLL
	UseClientSideAnimation();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::Spawn( void )
{
	// Validate our default team
	if ( m_iDefaultOwner < 0 || m_iDefaultOwner >= GetNumberOfTeams() )
	{
		Warning( "team_control_point '%s' has bad point_default_owner.\n", GetDebugName() );
		m_iDefaultOwner = TEAM_UNASSIGNED;
	}

#ifdef TF_DLL
	if ( m_iszCaptureStartSound == NULL_STRING )
	{
		m_iszCaptureStartSound = AllocPooledString( "Hologram.Start" );
	}
	if ( m_iszCaptureEndSound == NULL_STRING )
	{
		m_iszCaptureEndSound = AllocPooledString( "Hologram.Stop" );
	}
	if ( m_iszCaptureInProgress == NULL_STRING )
	{
		m_iszCaptureInProgress = AllocPooledString( "Hologram.Move" );
	}
	if ( m_iszCaptureInterrupted == NULL_STRING )
	{
		m_iszCaptureInterrupted = AllocPooledString( "Hologram.Interrupted" );
	}
#endif

	Precache();

	InternalSetOwner( m_iDefaultOwner, false );	//init the owner of this point
	TeamplayRoundBasedRules()->RecalculateControlPointState();

	SetActive( !m_bStartDisabled );

	BaseClass::Spawn();

	SetPlaybackRate( 1.0 );
	SetThink( &CTeamControlPoint::AnimThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( FBitSet( m_spawnflags, SF_CAP_POINT_HIDE_MODEL ) )
	{
		AddEffects( EF_NODRAW );
	}

	if ( FBitSet( m_spawnflags, SF_CAP_POINT_HIDE_SHADOW ) )
	{
		AddEffects( EF_NOSHADOW );
	}

	m_bBotsIgnore = FBitSet( m_spawnflags, SF_CAP_POINT_BOTS_IGNORE ) > 0;

	m_flLastContestedAt = -1;

	m_pCaptureInProgressSound = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPoint::KeyValue( const char *szKeyName, const char *szValue )
{	
	if ( !Q_strncmp( szKeyName, "team_capsound_", 14 ) )
	{
		int iTeam = atoi(szKeyName+14);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iszCapSound = AllocPooledString(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_model_", 11 ) )
	{
		int iTeam = atoi(szKeyName+11);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iszModel = AllocPooledString(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_timedpoints_", 17 ) )
	{
		int iTeam = atoi(szKeyName+17);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iTimedPoints = atoi(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_bodygroup_", 15 ) )
	{
		int iTeam = atoi(szKeyName+15);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iModelBodygroup = atoi(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_icon_", 10 ) )
	{
		int iTeam = atoi(szKeyName+10);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iszIcon = AllocPooledString(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_overlay_", 13 ) )
	{
		int iTeam = atoi(szKeyName+13);
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );

		m_TeamData[iTeam].iszOverlay = AllocPooledString(szValue);
	}
	else if ( !Q_strncmp( szKeyName, "team_previouspoint_", 19 ) )
	{
		int iTeam;
		int iPoint = 0;
		sscanf( szKeyName+19, "%d_%d", &iTeam, &iPoint );
		Assert( iTeam >= 0 && iTeam < m_TeamData.Count() );
		Assert( iPoint >= 0 && iPoint < MAX_PREVIOUS_POINTS );
		m_TeamData[iTeam].iszPreviousPoint[iPoint] = AllocPooledString(szValue);
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
void CTeamControlPoint::Precache( void )
{
	for ( int i = 0; i < m_TeamData.Count(); i++ )
	{
		// Skip over spectator
		if ( i == TEAM_SPECTATOR )
			continue;

		if ( m_TeamData[i].iszCapSound != NULL_STRING )
		{
			PrecacheScriptSound( STRING(m_TeamData[i].iszCapSound) );
		}

		if ( m_TeamData[i].iszModel != NULL_STRING )
		{
			PrecacheModel( STRING(m_TeamData[i].iszModel) );
		}

		if ( m_TeamData[i].iszIcon != NULL_STRING )
		{
			PrecacheMaterial( STRING( m_TeamData[i].iszIcon ) );
			m_TeamData[i].iIcon = GetMaterialIndex( STRING( m_TeamData[i].iszIcon ) );
			Assert( m_TeamData[i].iIcon != 0 );
		}

		if ( !m_TeamData[i].iIcon )
		{
			Warning( "Invalid hud icon material for team %d in control point '%s' ( point index %d )\n", i, GetDebugName(), GetPointIndex() );
		}

		if ( m_TeamData[i].iszOverlay != NULL_STRING )
		{
			PrecacheMaterial( STRING( m_TeamData[i].iszOverlay ) );
			m_TeamData[i].iOverlay = GetMaterialIndex( STRING( m_TeamData[i].iszOverlay ) );
			Assert( m_TeamData[i].iOverlay != 0 );

			if ( !m_TeamData[i].iOverlay )
			{
				Warning( "Invalid hud overlay material for team %d in control point '%s' ( point index %d )\n", i, GetDebugName(), GetPointIndex() );
			}
		}
	}

	PrecacheScriptSound( STRING( m_iszCaptureStartSound ) );
	PrecacheScriptSound( STRING( m_iszCaptureEndSound ) );
	PrecacheScriptSound( STRING( m_iszCaptureInProgress ) );
	PrecacheScriptSound( STRING( m_iszCaptureInterrupted ) );

	if ( m_iszWarnSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszWarnSound ) );
	}

#ifdef TF_DLL
	PrecacheScriptSound( "Announcer.ControlPointContested" );
	PrecacheScriptSound( "Announcer.ControlPointContested_Neutral" );
#endif
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTeamControlPoint::AnimThink( void )
{
	StudioFrameAdvance();
	DispatchAnimEvents(this);
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Used by ControlMaster to this point to its default owner
//-----------------------------------------------------------------------------
void CTeamControlPoint::InputReset( inputdata_t &input )
{
	m_flLastContestedAt = -1;
	InternalSetOwner( m_iDefaultOwner, false );
	ObjectiveResource()->SetOwningTeam( GetPointIndex(), m_iTeam );
	TeamplayRoundBasedRules()->RecalculateControlPointState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::HandleScoring( int iTeam )
{
	if ( TeamplayRoundBasedRules() && !TeamplayRoundBasedRules()->ShouldScorePerRound() )
	{
		GetGlobalTeam( iTeam )->AddScore( 1 );
		TeamplayRoundBasedRules()->HandleTeamScoreModify( iTeam, 1 );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster && !pMaster->WouldNewCPOwnerWinGame( this, iTeam ) )
		{
#ifdef TF_DLL
			if ( TeamplayRoundBasedRules()->GetGameType() == TF_GAMETYPE_ESCORT )
			{
				CBroadcastRecipientFilter filter;
				EmitSound( filter, entindex(), "Hud.EndRoundScored" );
			}
			else
#endif
			{
				CTeamRecipientFilter filter( iTeam );
				EmitSound( filter, entindex(), "Hud.EndRoundScored" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used by Area caps to set the owner
//-----------------------------------------------------------------------------
void CTeamControlPoint::InputSetOwner( inputdata_t &input )
{
	int iCapTeam = input.value.Int();

	Assert( iCapTeam >= 0 && iCapTeam < GetNumberOfTeams() );

	if ( GetOwner() == iCapTeam )
		return;

	if ( TeamplayGameRules()->PointsMayBeCaptured() )
	{
		// must be done before setting the owner
		HandleScoring( iCapTeam );

		if ( input.pCaller && input.pCaller->IsPlayer() )
		{
			int iCappingPlayer = input.pCaller->entindex();
			InternalSetOwner( iCapTeam, true, 1, &iCappingPlayer );
		}
		else
		{
			InternalSetOwner( iCapTeam, false );
		}

		ObjectiveResource()->SetOwningTeam( GetPointIndex(), m_iTeam );
		TeamplayRoundBasedRules()->RecalculateControlPointState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::InputShowModel( inputdata_t &input )
{
	RemoveEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::InputHideModel( inputdata_t &input )
{
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPoint::GetCurrentHudIconIndex( void )
{
	return m_TeamData[GetOwner()].iIcon;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPoint::GetHudIconIndexForTeam( int iGameTeam )
{
	return m_TeamData[iGameTeam].iIcon;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPoint::GetHudOverlayIndexForTeam( int iGameTeam )
{
	return m_TeamData[iGameTeam].iOverlay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTeamControlPoint::GetPreviousPointForTeam( int iGameTeam, int iPrevPoint )
{
	Assert( iPrevPoint >= 0 && iPrevPoint < MAX_PREVIOUS_POINTS );

	int iRetVal = -1;
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, STRING(m_TeamData[iGameTeam].iszPreviousPoint[iPrevPoint]) );

	if ( pEntity )
	{
		CTeamControlPoint *pPoint = dynamic_cast<CTeamControlPoint*>( pEntity );

		if ( pPoint )
		{
			iRetVal = pPoint->GetPointIndex();
		}
	}

	return iRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::ForceOwner( int iTeam )
{
	InternalSetOwner( iTeam, false, 0, 0 );
	ObjectiveResource()->SetOwningTeam( GetPointIndex(), m_iTeam );
	TeamplayRoundBasedRules()->RecalculateControlPointState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::SetOwner( int iCapTeam, bool bMakeSound, int iNumCappers, int *pCappingPlayers )
{
	if ( TeamplayGameRules()->PointsMayBeCaptured() )
	{
		// must be done before setting the owner
		HandleScoring( iCapTeam );

		InternalSetOwner( iCapTeam, bMakeSound, iNumCappers, pCappingPlayers );
		ObjectiveResource()->SetOwningTeam( GetPointIndex(), m_iTeam );
		TeamplayRoundBasedRules()->RecalculateControlPointState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::CaptureStart( int iCapTeam, int iNumCappingPlayers, int *pCappingPlayers )
{
	int iNumCappers = iNumCappingPlayers;

	float flLastOwnershipChangeTime = -1.f;
	CBaseEntity *pEnt =	gEntList.FindEntityByClassname( NULL, GetControlPointMasterName() );
	while( pEnt )
	{
		CTeamControlPointMaster *pMaster = dynamic_cast<CTeamControlPointMaster *>( pEnt );
		if ( pMaster && pMaster->IsActive() )
		{
			flLastOwnershipChangeTime = pMaster->GetLastOwnershipChangeTime();
		}
		pEnt = gEntList.FindEntityByClassname( pEnt, GetControlPointMasterName() );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_point_startcapture" );
	if ( event )
	{
		event->SetInt( "cp", m_iPointIndex );
		event->SetString( "cpname", STRING( m_iszPrintName ) );
		event->SetInt( "team", m_iTeam );
		event->SetInt( "capteam", iCapTeam );
		event->SetFloat( "captime", gpGlobals->curtime - flLastOwnershipChangeTime );

		// safety check
		if ( iNumCappers > 8 )
		{
			iNumCappers = 8;
		}

		char cappers[9];	// pCappingPlayers should be max length 8
		int i;
		for( i = 0 ; i < iNumCappers ; i++ )
		{
			cappers[i] = (char)pCappingPlayers[i];
		}

		cappers[i] = '\0';

		// pCappingPlayers is a null terminated list of player indices
		event->SetString( "cappers", cappers );
		event->SetInt( "priority", 7 );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::CaptureEnd( void )
{
	StopLoopingSounds();

	if ( !FBitSet( m_spawnflags, SF_CAP_POINT_NO_CAP_SOUNDS ) )
	{
		EmitSound( STRING( m_iszCaptureEndSound ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::CaptureInterrupted( bool bBlocked )
{
	StopLoopingSounds();

	if ( FBitSet( m_spawnflags, SF_CAP_POINT_NO_CAP_SOUNDS ) )
	{
		return;
	}

	const char *pSoundName = NULL;

	if ( bBlocked == true )
	{
		pSoundName = STRING( m_iszCaptureInterrupted );
	}
	else
	{
		pSoundName = STRING( m_iszCaptureInProgress );
		EmitSound( STRING( m_iszCaptureStartSound ) );
	}

	if ( m_pCaptureInProgressSound == NULL && pSoundName != NULL )
	{
		CPASFilter filter( GetAbsOrigin() );

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pCaptureInProgressSound = controller.SoundCreate( filter, entindex(), pSoundName );

		controller.Play( m_pCaptureInProgressSound, 1.0, 100 );
	}

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::StopLoopingSounds( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pCaptureInProgressSound )
	{
		controller.SoundDestroy( m_pCaptureInProgressSound );
		m_pCaptureInProgressSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the new owner of the point, plays the appropriate sound and shows the right model
//-----------------------------------------------------------------------------
void CTeamControlPoint::InternalSetOwner( int iCapTeam, bool bMakeSound, int iNumCappers, int *pCappingPlayers )
{
	Assert( iCapTeam >= 0 && iCapTeam < GetNumberOfTeams() );

	int iOldTeam = m_iTeam;

	m_iTeam = iCapTeam;
	ChangeTeam( iCapTeam );

	if ( bMakeSound )
	{
		CBroadcastRecipientFilter filter;
		EmitSound( filter, entindex(), STRING( m_TeamData[m_iTeam].iszCapSound ) );
	}

	// Update visuals
	SetModel( STRING(m_TeamData[m_iTeam].iszModel) );
	SetBodygroup( 0, m_iTeam );
	m_nSkin = ( m_iTeam == TEAM_UNASSIGNED ) ? 2 : (m_iTeam - 2);
	ResetSequence( LookupSequence("idle") );

	// We add 1 to the index because we consider the default "no points capped" as 0.
	TeamplayGameRules()->SetLastCapPointChanged( m_iPointIndex+1 );

	// Determine the pose parameters for each team
	for ( int i = 0; i < m_TeamData.Count(); i++ )
	{
		// Skip spectator
		if ( i == TEAM_SPECTATOR )
			continue;

		if ( GetModelPtr() && GetModelPtr()->SequencesAvailable() )
		{
			m_TeamData[i].iTeamPoseParam = LookupPoseParameter( UTIL_VarArgs( "cappoint_%d_percentage", i ) );
		}
		else
		{
			m_TeamData[i].iTeamPoseParam = -1;
		}
	}
	UpdateCapPercentage();

	if ( m_iTeam == TEAM_UNASSIGNED )
	{
		m_OnCapReset.FireOutput( this, this );
	}
	else
	{
		// Remap team to get first game team = 1
		switch ( m_iTeam - FIRST_GAME_TEAM+1 )
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
	}

	// If we're playing a sound, this is a true cap by players.
	if ( bMakeSound )	
	{
		if ( iOldTeam > LAST_SHARED_TEAM && iOldTeam != m_iTeam )
		{
			// Make the members of our old team say something
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );
				if ( !pPlayer )
					continue;
				if ( pPlayer->GetTeamNumber() == iOldTeam )
				{
					pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_LOST_CONTROL_POINT );
				}
			}
		}

		for( int i = 0; i < iNumCappers; i++ )
		{
			int playerIndex = pCappingPlayers[i];

			Assert( playerIndex > 0 && playerIndex <= gpGlobals->maxClients );

			CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( playerIndex ) );
			PlayerCapped( pPlayer );

#ifdef TF_DLL
			if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_EOTL ) )
			{
				TFGameRules()->DropBonusDuck( pPlayer->GetAbsOrigin(), ToTFPlayer( pPlayer ), NULL, NULL, false, true );
			}
#endif
		}

		// Remap team to get first game team = 1
		switch ( m_iTeam - FIRST_GAME_TEAM+1 )
		{
		case 1: 
			m_OnOwnerChangedToTeam1.FireOutput( this, this );
			break;
		case 2: 
			m_OnOwnerChangedToTeam2.FireOutput( this, this );
			break;
		}

		if ( m_iTeam != TEAM_UNASSIGNED && iNumCappers )
		{
			SendCapString( m_iTeam, iNumCappers, pCappingPlayers );
		}

#ifdef TF_DLL
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			TFGameRules()->DropHalloweenSoulPackToTeam( 5, GetAbsOrigin(), m_iTeam, TEAM_SPECTATOR );
		}
#endif
	}

	// Have control point master check the win conditions now!
	CBaseEntity *pEnt =	gEntList.FindEntityByClassname( NULL, GetControlPointMasterName() );

	while( pEnt )
	{
		CTeamControlPointMaster *pMaster = dynamic_cast<CTeamControlPointMaster *>( pEnt );

		if ( pMaster->IsActive() )
		{
			pMaster->CheckWinConditions();
			pMaster->SetLastOwnershipChangeTime( gpGlobals->curtime );
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, GetControlPointMasterName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::SendCapString( int iCapTeam, int iNumCappingPlayers, int *pCappingPlayers )
{
	if ( strlen( STRING(m_iszPrintName) ) <= 0 )
		return;

	int iNumCappers = iNumCappingPlayers;

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_point_captured" );
	if ( event )
	{
		event->SetInt( "cp", m_iPointIndex );
		event->SetString( "cpname", STRING( m_iszPrintName ) );
		event->SetInt( "team", iCapTeam );

		// safety check
		if ( iNumCappers > 8 )
		{
			iNumCappers = 8;
		}

		char cappers[9];	// pCappingPlayers should be max length 8
		int i;
		for( i = 0 ; i < iNumCappers ; i++ )
		{
			cappers[i] = (char)pCappingPlayers[i];
		}

		cappers[i] = '\0';

		// pCappingPlayers is a null terminated list of player indices
		event->SetString( "cappers", cappers );
		event->SetInt( "priority", 9 );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::CaptureBlocked( CBaseMultiplayerPlayer *pPlayer, CBaseMultiplayerPlayer *pVictim )
{
	if( strlen( STRING(m_iszPrintName) ) <= 0 )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_capture_blocked" );

	if ( event )
	{
		event->SetInt( "cp", m_iPointIndex );
		event->SetString( "cpname", STRING(m_iszPrintName) );
		event->SetInt( "blocker", pPlayer->entindex() );
		event->SetInt( "priority", 9 );
		if ( pVictim )
		{
			event->SetInt( "victim", pVictim->entindex() );
		}

		gameeventmanager->FireEvent( event );
	}

	PlayerBlocked( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPoint::GetOwner( void ) const
{ 
	return m_iTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPoint::GetDefaultOwner( void ) const
{
	return m_iDefaultOwner;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPoint::GetCPGroup( void )
{
	return m_iCPGroup;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the time-based point value of this control point
//-----------------------------------------------------------------------------
int CTeamControlPoint::PointValue( void )
{
	if ( GetOwner() != m_iDefaultOwner )
		return m_TeamData[ GetOwner() ].iTimedPoints;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::SetActive( bool active )
{
	m_bActive = active;
	
	if( active )
	{
		RemoveEffects( EF_NODRAW );
	}
	else
	{
		AddEffects( EF_NODRAW );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::SetCappersRequiredForTeam( int iGameTeam, int iCappers )
{
	m_TeamData[iGameTeam].iPlayersRequired = iCappers;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if this point has ever been contested, false if the enemy has never contested this point yet
//-----------------------------------------------------------------------------
bool CTeamControlPoint::HasBeenContested( void ) const
{
	return m_flLastContestedAt > 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTeamControlPoint::LastContestedAt( void )
{
	return m_flLastContestedAt;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::SetLastContestedAt( float flTime )
{
	m_flLastContestedAt = flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::UpdateCapPercentage( void )
{
	for ( int i = LAST_SHARED_TEAM+1; i < m_TeamData.Count(); i++ )
	{
		// Skip spectator
		if ( i == TEAM_SPECTATOR )
			continue;

		float flPerc = GetTeamCapPercentage(i);

		if ( m_TeamData[i].iTeamPoseParam != -1 )
		{
			SetPoseParameter( m_TeamData[i].iTeamPoseParam, flPerc );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTeamControlPoint::GetTeamCapPercentage( int iTeam )
{
	int iCappingTeam = ObjectiveResource()->GetCappingTeam( GetPointIndex() );
	if ( iCappingTeam == TEAM_UNASSIGNED )
	{
		// No-one's capping this point.
		if ( iTeam == m_iTeam )
			return 1.0;

		return 0.0;
	}

	float flCapPerc = ObjectiveResource()->GetCPCapPercentage( GetPointIndex() );
	if ( iTeam == iCappingTeam )
		return (1.0 - flCapPerc);
	if ( iTeam == m_iTeam )
		return flCapPerc;

	return 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPoint::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[1024];
		Q_snprintf(tempstr, sizeof(tempstr), "INDEX: (%d)", GetPointIndex() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf( tempstr, sizeof(tempstr), "Red Previous Points: ");
		for ( int i = 0; i < MAX_PREVIOUS_POINTS; i++ )
		{
			if ( m_TeamData[2].iszPreviousPoint[i] != NULL_STRING )
			{
				Q_strncat( tempstr, STRING(m_TeamData[2].iszPreviousPoint[i]), 1024, COPY_ALL_CHARACTERS );
				Q_strncat( tempstr, ", ", 1024, COPY_ALL_CHARACTERS );
			}
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf( tempstr, sizeof(tempstr), "Blue Previous Points: " );
		for ( int i = 0; i < MAX_PREVIOUS_POINTS; i++ )
		{
			if ( m_TeamData[3].iszPreviousPoint[i] != NULL_STRING )
			{
				Q_strncat( tempstr, STRING(m_TeamData[3].iszPreviousPoint[i]), 1024, COPY_ALL_CHARACTERS );
				Q_strncat( tempstr, ", ", 1024, COPY_ALL_CHARACTERS );
			}
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

		for ( int i = 0; i < MAX_CONTROL_POINT_TEAMS; i++ )
		{
			if ( ObjectiveResource()->GetBaseControlPointForTeam(i) == GetPointIndex() )
			{
				Q_snprintf(tempstr, sizeof(tempstr), "Base Control Point for Team %d", i );
				EntityText(text_offset,tempstr,0);
				text_offset++;
			}
		}
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: The specified player took part in capping this point.
//-----------------------------------------------------------------------------
void CTeamControlPoint::PlayerCapped( CBaseMultiplayerPlayer *pPlayer )
{
	if ( pPlayer )
	{
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_CAPTURED_POINT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The specified player blocked the enemy team from capping this point.
//-----------------------------------------------------------------------------
void CTeamControlPoint::PlayerBlocked( CBaseMultiplayerPlayer *pPlayer )
{
	if ( pPlayer )
	{
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_CAPTURE_BLOCKED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::InputRoundActivate( inputdata_t &inputdata )
{
	switch ( m_iTeam - FIRST_GAME_TEAM+1 )
	{
	case 1: 
		m_OnRoundStartOwnedByTeam1.FireOutput( this, this );
		break;
	case 2: 
		m_OnRoundStartOwnedByTeam2.FireOutput( this, this );
		break;
	}

	InternalSetLocked( m_bLocked );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::InputSetLocked( inputdata_t &inputdata )
{
	// never lock/unlock the point if we're in waiting for players
	if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		return;

	bool bLocked = inputdata.value.Int() > 0;
	InternalSetLocked( bLocked );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::InternalSetLocked( bool bLocked )
{
	if ( !bLocked && m_bLocked )
	{
		// unlocked this point
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_point_unlocked" );
		if ( event )
		{
			event->SetInt( "cp", m_iPointIndex );
			event->SetString( "cpname", STRING( m_iszPrintName ) );
			event->SetInt( "team", m_iTeam );
			gameeventmanager->FireEvent( event );
		}
	}
	else if ( bLocked && !m_bLocked )
	{
		// locked this point
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_point_locked" );
		if ( event )
		{
			event->SetInt( "cp", m_iPointIndex );
			event->SetString( "cpname", STRING( m_iszPrintName ) );
			event->SetInt( "team", m_iTeam );
			gameeventmanager->FireEvent( event );
		}
	}

	m_bLocked = bLocked;

	if ( ObjectiveResource() && GetPointIndex() < ObjectiveResource()->GetNumControlPoints() )
	{
		ObjectiveResource()->SetCPLocked( GetPointIndex(), m_bLocked );
		ObjectiveResource()->SetCPUnlockTime( GetPointIndex(), 0.0f );
	}

	if ( !m_bLocked )
	{
		m_flUnlockTime = -1;
		m_OnUnlocked.FireOutput( this, this );
		SetContextThink( NULL, 0, CONTROL_POINT_UNLOCK_THINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::InputSetUnlockTime( inputdata_t &inputdata )
{
	// never lock/unlock the point if we're in waiting for players
	if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		return;

	int nTime = inputdata.value.Int();

	if ( nTime <= 0 )
	{
		InternalSetLocked( false );
		return;
	}

	m_flUnlockTime = gpGlobals->curtime + nTime;

	if ( ObjectiveResource() )
	{
		ObjectiveResource()->SetCPUnlockTime( GetPointIndex(), m_flUnlockTime );
	}

	SetContextThink( &CTeamControlPoint::UnlockThink, gpGlobals->curtime + 0.1, CONTROL_POINT_UNLOCK_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPoint::UnlockThink( void )
{
	if ( m_flUnlockTime > 0 && 
		 m_flUnlockTime < gpGlobals->curtime && 
		 ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() == GR_STATE_RND_RUNNING ) )
	{
		InternalSetLocked( false );
		return;
	}

	SetContextThink( &CTeamControlPoint::UnlockThink, gpGlobals->curtime + 0.1, CONTROL_POINT_UNLOCK_THINK );
}
