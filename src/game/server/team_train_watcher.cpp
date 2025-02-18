//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "team_train_watcher.h"
#include "team_control_point.h"
#include "trains.h"
#include "team_objectiveresource.h"
#include "teamplayroundbased_gamerules.h"
#include "team_control_point.h"
#include "team_control_point_master.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "mp_shareddefs.h"
#include "props.h"
#include "physconstraint.h"

#ifdef TF_DLL
#include "tf_shareddefs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
/*
#define TWM_FIRSTSTAGEOUTCOME01	"Announcer.PLR_FirstStageOutcome01"
#define TWM_FIRSTSTAGEOUTCOME02	"Announcer.PLR_FirstStageOutcome02"
#define TWM_RACEGENERAL01	"Announcer.PLR_RaceGeneral01"
#define TWM_RACEGENERAL02	"Announcer.PLR_RaceGeneral02"
#define TWM_RACEGENERAL03	"Announcer.PLR_RaceGeneral03"
#define TWM_RACEGENERAL04	"Announcer.PLR_RaceGeneral04"
#define TWM_RACEGENERAL05	"Announcer.PLR_RaceGeneral05"
#define TWM_RACEGENERAL08	"Announcer.PLR_RaceGeneral08"
#define TWM_RACEGENERAL06	"Announcer.PLR_RaceGeneral06"
#define TWM_RACEGENERAL07	"Announcer.PLR_RaceGeneral07"
#define TWM_RACEGENERAL09	"Announcer.PLR_RaceGeneral09"
#define TWM_RACEGENERAL12	"Announcer.PLR_RaceGeneral12"
#define TWM_RACEGENERAL13	"Announcer.PLR_RaceGeneral13"
#define TWM_RACEGENERAL14	"Announcer.PLR_RaceGeneral14"
#define TWM_RACEGENERAL15	"Announcer.PLR_RaceGeneral15"
#define TWM_RACEGENERAL10	"Announcer.PLR_RaceGeneral10"
#define TWM_RACEGENERAL11	"Announcer.PLR_RaceGeneral11"
#define TWM_SECONDSTAGEOUTCOME01	"Announcer.PLR_SecondStageOutcome01"
#define TWM_SECONDSTAGEOUTCOME04	"Announcer.PLR_SecondStageOutcome04"
#define TWM_SECONDSTAGEOUTCOME02	"Announcer.PLR_SecondStageOutcome02"
#define TWM_SECONDSTAGEOUTCOME03	"Announcer.PLR_SecondStageOutcome03"
#define TWM_FINALSTAGEOUTCOME01		"Announcer.PLR_FinalStageOutcome01"
#define TWM_FINALSTAGEOUTCOME02		"Announcer.PLR_FinalStageOutcome02"
#define TWM_FINALSTAGESTART01	"Announcer.PLR_FinalStageStart01"
#define TWM_FINALSTAGESTART04	"Announcer.PLR_FinalStageStart04"
#define TWM_FINALSTAGESTART08	"Announcer.PLR_FinalStageStart08"
#define TWM_FINALSTAGESTART09	"Announcer.PLR_FinalStageStart09"
#define TWM_FINALSTAGESTART07	"Announcer.PLR_FinalStageStart07"
#define TWM_FINALSTAGESTART02	"Announcer.PLR_FinalStageStart02"
#define TWM_FINALSTAGESTART03	"Announcer.PLR_FinalStageStart03"
#define TWM_FINALSTAGESTART05	"Announcer.PLR_FinalStageStart05"
#define TWM_FINALSTAGESTART06	"Announcer.PLR_FinalStageStart06"

EHANDLE g_hTeamTrainWatcherMaster = NULL;
*/
#define MAX_ALARM_TIME_NO_RECEDE 18 // max amount of time to play the alarm if the train isn't going to recede

BEGIN_DATADESC( CTeamTrainWatcher )

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetNumTrainCappers", InputSetNumTrainCappers ),
	DEFINE_INPUTFUNC( FIELD_VOID, "OnStartOvertime", InputOnStartOvertime ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeedForwardModifier", InputSetSpeedForwardModifier ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTrainRecedeTime", InputSetTrainRecedeTime ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetTrainCanRecede", InputSetTrainCanRecede ),

	// Outputs
	DEFINE_OUTPUT( m_OnTrainStartRecede, "OnTrainStartRecede" ),

	// key
	DEFINE_KEYFIELD( m_iszTrain, FIELD_STRING, "train" ),
	DEFINE_KEYFIELD( m_iszStartNode, FIELD_STRING, "start_node" ),
	DEFINE_KEYFIELD( m_iszGoalNode, FIELD_STRING, "goal_node" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[0], FIELD_STRING, "linked_pathtrack_1" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[0], FIELD_STRING, "linked_cp_1" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[1], FIELD_STRING, "linked_pathtrack_2" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[1], FIELD_STRING, "linked_cp_2" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[2], FIELD_STRING, "linked_pathtrack_3" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[2], FIELD_STRING, "linked_cp_3" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[3], FIELD_STRING, "linked_pathtrack_4" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[3], FIELD_STRING, "linked_cp_4" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[4], FIELD_STRING, "linked_pathtrack_5" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[4], FIELD_STRING, "linked_cp_5" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[5], FIELD_STRING, "linked_pathtrack_6" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[5], FIELD_STRING, "linked_cp_6" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[6], FIELD_STRING, "linked_pathtrack_7" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[6], FIELD_STRING, "linked_cp_7" ),

	DEFINE_KEYFIELD( m_iszLinkedPathTracks[7], FIELD_STRING, "linked_pathtrack_8" ),
	DEFINE_KEYFIELD( m_iszLinkedCPs[7], FIELD_STRING, "linked_cp_8" ),

	DEFINE_KEYFIELD( m_bTrainCanRecede, FIELD_BOOLEAN, "train_can_recede" ),

	DEFINE_KEYFIELD( m_bHandleTrainMovement, FIELD_BOOLEAN, "handle_train_movement" ),

	// can be up to 8 links

	// min speed for train hud speed levels
	DEFINE_KEYFIELD( m_flSpeedLevels[0], FIELD_FLOAT, "hud_min_speed_level_1" ),
	DEFINE_KEYFIELD( m_flSpeedLevels[1], FIELD_FLOAT, "hud_min_speed_level_2" ),
	DEFINE_KEYFIELD( m_flSpeedLevels[2], FIELD_FLOAT, "hud_min_speed_level_3" ),

	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_KEYFIELD( m_iszSparkName, FIELD_STRING, "env_spark_name" ),

	DEFINE_KEYFIELD( m_flSpeedForwardModifier, FIELD_FLOAT, "speed_forward_modifier" ),

	DEFINE_KEYFIELD( m_nTrainRecedeTime, FIELD_INTEGER, "train_recede_time" ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CTeamTrainWatcher, DT_TeamTrainWatcher)

	SendPropFloat( SENDINFO( m_flTotalProgress ), 11, 0, 0.0f, 1.0f ),
	SendPropInt( SENDINFO( m_iTrainSpeedLevel ), 4 ),
	SendPropTime( SENDINFO( m_flRecedeTime ) ),
	SendPropInt( SENDINFO( m_nNumCappers ) ),
#ifdef GLOWS_ENABLE
	SendPropEHandle( SENDINFO( m_hGlowEnt ) ),
#endif // GLOWS_ENABLE

END_SEND_TABLE()


LINK_ENTITY_TO_CLASS( team_train_watcher, CTeamTrainWatcher );

IMPLEMENT_AUTO_LIST( ITFTeamTrainWatcher );

/*
LINK_ENTITY_TO_CLASS( team_train_watcher_master, CTeamTrainWatcherMaster );
PRECACHE_REGISTER( team_train_watcher_master );

CTeamTrainWatcherMaster::CTeamTrainWatcherMaster()
{
	m_pBlueWatcher = NULL;
	m_pRedWatcher = NULL;

	m_flBlueProgress = 0.0f;
	m_flRedProgress = 0.0f;

	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_round_win" );
}

CTeamTrainWatcherMaster::~CTeamTrainWatcherMaster()
{
	if ( g_hTeamTrainWatcherMaster.Get() == this )
	{
		g_hTeamTrainWatcherMaster = NULL;
	}
}

void CTeamTrainWatcherMaster::Precache( void )
{
	PrecacheScriptSound( TWM_FIRSTSTAGEOUTCOME01 );
	PrecacheScriptSound( TWM_FIRSTSTAGEOUTCOME02 );
	PrecacheScriptSound( TWM_RACEGENERAL01 );
	PrecacheScriptSound( TWM_RACEGENERAL02 );
	PrecacheScriptSound( TWM_RACEGENERAL03 );
	PrecacheScriptSound( TWM_RACEGENERAL04 );
	PrecacheScriptSound( TWM_RACEGENERAL05 );
	PrecacheScriptSound( TWM_RACEGENERAL08 );
	PrecacheScriptSound( TWM_RACEGENERAL06 );
	PrecacheScriptSound( TWM_RACEGENERAL07 );
	PrecacheScriptSound( TWM_RACEGENERAL09 );
	PrecacheScriptSound( TWM_RACEGENERAL12 );
	PrecacheScriptSound( TWM_RACEGENERAL13 );
	PrecacheScriptSound( TWM_RACEGENERAL14 );
	PrecacheScriptSound( TWM_RACEGENERAL15 );
	PrecacheScriptSound( TWM_RACEGENERAL10 );
	PrecacheScriptSound( TWM_RACEGENERAL11 );
	PrecacheScriptSound( TWM_SECONDSTAGEOUTCOME01 );
	PrecacheScriptSound( TWM_SECONDSTAGEOUTCOME04 );
	PrecacheScriptSound( TWM_SECONDSTAGEOUTCOME02 );
	PrecacheScriptSound( TWM_SECONDSTAGEOUTCOME03 );
	PrecacheScriptSound( TWM_FINALSTAGEOUTCOME01 );
	PrecacheScriptSound( TWM_FINALSTAGEOUTCOME02 );
	PrecacheScriptSound( TWM_FINALSTAGESTART01 );
	PrecacheScriptSound( TWM_FINALSTAGESTART04 );
	PrecacheScriptSound( TWM_FINALSTAGESTART08 );
	PrecacheScriptSound( TWM_FINALSTAGESTART09 );
	PrecacheScriptSound( TWM_FINALSTAGESTART07 );
	PrecacheScriptSound( TWM_FINALSTAGESTART02 );
	PrecacheScriptSound( TWM_FINALSTAGESTART03 );
	PrecacheScriptSound( TWM_FINALSTAGESTART05 );
	PrecacheScriptSound( TWM_FINALSTAGESTART06 );

	BaseClass::Precache();
}

bool CTeamTrainWatcherMaster::FindTrainWatchers( void )
{
	m_pBlueWatcher = NULL;
	m_pRedWatcher = NULL;

	// find the train_watchers for this round
	CTeamTrainWatcher *pTrainWatcher = (CTeamTrainWatcher *)gEntList.FindEntityByClassname( NULL, "team_train_watcher" );
	while ( pTrainWatcher )
	{
		if ( pTrainWatcher->IsDisabled() == false )
		{
			if ( pTrainWatcher->GetTeamNumber() == TF_TEAM_BLUE )
			{
				m_pBlueWatcher = pTrainWatcher;
			}
			else if ( pTrainWatcher->GetTeamNumber() == TF_TEAM_RED )
			{
				m_pRedWatcher = pTrainWatcher;
			}
		}

		pTrainWatcher = (CTeamTrainWatcher *)gEntList.FindEntityByClassname( pTrainWatcher, "team_train_watcher" );
	}

	return ( m_pBlueWatcher && m_pRedWatcher );
}

void CTeamTrainWatcherMaster::TWMThink( void )
{
	if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING )
	{
		// the next time we 'think'
		SetContextThink( &CTeamTrainWatcherMaster::TWMThink, gpGlobals->curtime + 0.2, TWMASTER_THINK );
		return;
	}



	// the next time we 'think'
	SetContextThink( &CTeamTrainWatcherMaster::TWMThink, gpGlobals->curtime + 0.2, TWMASTER_THINK );
}

void CTeamTrainWatcherMaster::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();`

	if ( FStrEq( "teamplay_round_start", eventname ) )
	{
		if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->HasMultipleTrains() )
		{
			if ( FindTrainWatchers() )
			{
				// we found train watchers so start thinking
				SetContextThink( &CTeamTrainWatcherMaster::TWMThink, gpGlobals->curtime + 0.2, TWMASTER_THINK );
			}
		}
	}
	else if ( FStrEq( "teamplay_round_win", eventname ) )
	{
		if ( TeamplayRoundBasedRules() )
		{
			int iWinningTeam = event->GetInt( "team" );
			int iLosingTeam = ( iWinningTeam == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
			bool bFullRound = event->GetBool( "full_round" );

			CTeamRecipientFilter filterWinner( iWinningTeam, true );
			CTeamRecipientFilter filterLoser( iLosingTeam, true );

			if ( bFullRound )
			{
				EmitSound( filterWinner, entindex(), TWM_FINALSTAGEOUTCOME01 );
				EmitSound( filterLoser, entindex(), TWM_FINALSTAGEOUTCOME02 );
			}
			else
			{
				EmitSound( filterWinner, entindex(), TWM_FIRSTSTAGEOUTCOME01 );
				EmitSound( filterLoser, entindex(), TWM_FIRSTSTAGEOUTCOME02 );
			}
		}
	}
}
*/
CTeamTrainWatcher::CTeamTrainWatcher()
{
	m_bDisabled = false;
	m_flRecedeTime = 0;
	m_bWaitingToRecede = false;
	m_bCapBlocked = false;

	m_flNextSpeakForwardConceptTime = 0;
	m_hAreaCap = NULL;

	m_bTrainCanRecede = true;
	m_bAlarmPlayed = false;
	m_pAlarm = NULL;
	m_flAlarmEndTime = -1;

	m_bHandleTrainMovement = false;
	m_flSpeedForwardModifier = 1.0f;
	m_iCurrentHillType = HILL_TYPE_NONE;
	m_flCurrentSpeed = 0.0f;
	m_bReceding = false;

	m_flTrainDistanceFromStart = 0.0f;

	m_nTrainRecedeTime = 0;

#ifdef GLOWS_ENABLE
	m_hGlowEnt.Set( NULL );
#endif // GLOWS_ENABLE

#ifdef TF_DLL
	ChangeTeam( TF_TEAM_BLUE );
#else
	ChangeTeam( TEAM_UNASSIGNED );
#endif
/*
	// create a CTeamTrainWatcherMaster entity
	if ( g_hTeamTrainWatcherMaster.Get() == NULL )
	{
		g_hTeamTrainWatcherMaster = CreateEntityByName( "team_train_watcher_master" );
	}
*/
	ListenForGameEvent( "path_track_passed" );
}

CTeamTrainWatcher::~CTeamTrainWatcher()
{
	m_Sparks.Purge();
}

void CTeamTrainWatcher::UpdateOnRemove( void )
{
	StopCaptureAlarm();

	BaseClass::UpdateOnRemove();
}

int CTeamTrainWatcher::UpdateTransmitState()
{
	if ( m_bDisabled )
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}

	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CTeamTrainWatcher::InputRoundActivate( inputdata_t &inputdata )
{
	StopCaptureAlarm();

	if ( !m_bDisabled )
	{
		WatcherActivate();
	}
}

void CTeamTrainWatcher::InputEnable( inputdata_t &inputdata )
{
	StopCaptureAlarm();

	m_bDisabled = false;

	WatcherActivate();

	UpdateTransmitState();
}

void CTeamTrainWatcher::InputDisable( inputdata_t &inputdata )
{
	StopCaptureAlarm();

	m_bDisabled = true;
	SetContextThink( NULL, 0, TW_THINK );

	m_bWaitingToRecede = false;

	m_Sparks.Purge();

#ifdef GLOWS_ENABLE
	m_hGlowEnt.Set( NULL );
#endif // GLOWS_ENABLE

	// if we're moving the train, let's shut it down
	if ( m_bHandleTrainMovement )
	{
		m_flCurrentSpeed = 0.0f;

		if ( m_hTrain )
		{
			m_hTrain->SetSpeedDirAccel( m_flCurrentSpeed );
		}

		// handle the sparks under the train
		HandleSparks( false );
	}

	UpdateTransmitState();
}

ConVar tf_escort_recede_time( "tf_escort_recede_time", "30", 0, "", true, 0, false, 0 );
ConVar tf_escort_recede_time_overtime( "tf_escort_recede_time_overtime", "5", 0, "", true, 0, false, 0 );

void CTeamTrainWatcher::FireGameEvent( IGameEvent *event )
{
	if ( IsDisabled() || !m_bHandleTrainMovement )
		return;

	const char *pszEventName = event->GetName();
	if ( FStrEq( pszEventName, "path_track_passed" ) )
	{
		int iIndex = event->GetInt( "index" );
		CPathTrack *pNode = dynamic_cast< CPathTrack* >( UTIL_EntityByIndex( iIndex ) );

		if ( pNode )
		{
			bool bHandleEvent = false;
			CPathTrack *pTempNode = m_hStartNode.Get();

			// is this a node in the track we're watching?
			while ( pTempNode )
			{
				if ( pTempNode == pNode )
				{
					bHandleEvent = true;
					break;
				}

				pTempNode = pTempNode->GetNext();
			}

			if ( bHandleEvent )
			{
				// If we're receding and we've hit a node but the next node (going backwards) is disabled 
				// the train is going to stop (like at the base of a downhill section) when we start forward 
				// again we won't pass this node again so don't change our hill state based on this node.
				if ( m_bReceding )
				{
					if ( pNode->GetPrevious() && pNode->GetPrevious()->IsDisabled() )
					{
						return;
					}
				}

				int iHillType = pNode->GetHillType();
				bool bUpdate = ( m_iCurrentHillType != iHillType );

				if ( !bUpdate )
				{
					// the hill settings are the same, but are we leaving an uphill or downhill segment?
					if ( m_iCurrentHillType != HILL_TYPE_NONE )
					{
						// let's peek at the next node
						CPathTrack *pNextNode = pNode->GetNext();
						if ( m_flCurrentSpeed < 0 )
						{
							// we're going backwards
							pNextNode = pNode->GetPrevious();
						}

						if ( pNextNode )
						{
							int iNextHillType = pNextNode->GetHillType();
							if ( m_iCurrentHillType != iNextHillType )
							{
								// we're leaving an uphill or downhill segment...so reset our state until we pass the next node
								bUpdate = true;
								iHillType = HILL_TYPE_NONE;
							}
						}
					}
				}

				if ( bUpdate )
				{
					m_iCurrentHillType = iHillType;
					HandleTrainMovement();
				}
			}
		}
	}
}

void CTeamTrainWatcher::HandleSparks( bool bSparks )
{
	if ( IsDisabled() || !m_bHandleTrainMovement )
		return;

	for ( int i = 0 ; i < m_Sparks.Count() ; i++ )
	{
		CEnvSpark* pSpark = m_Sparks[i].Get();
		if ( pSpark && ( pSpark->IsSparking() != bSparks ) )
		{
			if ( bSparks )
			{
				pSpark->StartSpark();
			}
			else
			{
				pSpark->StopSpark();
			}
		}
	}
}

void CTeamTrainWatcher::HandleTrainMovement( bool bStartReceding /* = false */ )
{
	if ( IsDisabled() || !m_bHandleTrainMovement )
		return;

	if ( m_hTrain )
	{
		float flSpeed = 0.0f;

		if ( bStartReceding )
		{
			flSpeed = -0.1f;
			m_bReceding = true;
		}
		else
		{
			// do we have cappers on the train?
			if ( m_nNumCappers > 0 )
			{
				m_bReceding = false;

				if ( m_iCurrentHillType == HILL_TYPE_DOWNHILL )
				{
					flSpeed = 1.0f;
				}
				else
				{
					switch( m_nNumCappers )
					{
					case 1:
						flSpeed = 0.55f;
						break;
					case 2:
						flSpeed = 0.77f;
						break;
					case 3:
					default:
						flSpeed = 1.0f;
						break;
					}
				}
			}
			else if ( m_nNumCappers == -1 )
			{
				// we'll get a -1 for a blocked cart (speed should be 0 for that unless we're on a hill)
				if ( m_iCurrentHillType == HILL_TYPE_DOWNHILL )
				{
					flSpeed = 1.0f;
				}
			}
			else
			{
				// there's nobody on the train, what should it be doing?
				if ( m_flCurrentSpeed > 0 )
				{
					if ( m_iCurrentHillType == HILL_TYPE_DOWNHILL )
					{
						flSpeed = 1.0f;
					}
				}
				else
				{
					// we're rolling backwards
					if ( m_iCurrentHillType == HILL_TYPE_UPHILL )
					{
						flSpeed = -1.0f;
					}
					else
					{
						if ( m_bReceding )
						{
							// resume our previous backup speed
							flSpeed = -0.1f;
						}
					}
				}
			}
		}

		// only need to update the train if our speed has changed
		if ( m_flCurrentSpeed != flSpeed )
		{
			if ( flSpeed >= 0.0f )
			{
				m_bReceding = false;
			}

			m_flCurrentSpeed = flSpeed;
			m_hTrain->SetSpeedDirAccel( m_flCurrentSpeed );

			// handle the sparks under the train
			bool bSparks = false;
			if ( m_flCurrentSpeed < 0 )
			{
				bSparks = true;
			}

			HandleSparks( bSparks );
		}
	}
}

void CTeamTrainWatcher::InputSetSpeedForwardModifier( inputdata_t &inputdata )
{
	InternalSetSpeedForwardModifier( inputdata.value.Float() );
}

void CTeamTrainWatcher::InternalSetSpeedForwardModifier( float flModifier )
{
	if ( IsDisabled() || !m_bHandleTrainMovement )
		return;

	// store the passed value
	float flSpeedForwardModifier = flModifier;
	flSpeedForwardModifier = fabs( flSpeedForwardModifier );

	m_flSpeedForwardModifier = clamp( flSpeedForwardModifier, 0.f, 1.f );

	if ( m_hTrain )
	{
		m_hTrain->SetSpeedForwardModifier( m_flSpeedForwardModifier );
	}
}

void CTeamTrainWatcher::InternalSetNumTrainCappers( int iNumCappers, CBaseEntity *pTrigger )
{
	if ( IsDisabled() )
		return;

	m_nNumCappers = iNumCappers;

	// inputdata.pCaller is hopefully an area capture
	// lets see if its blocked, and not start receding if it is
	CTriggerAreaCapture *pAreaCap = dynamic_cast<CTriggerAreaCapture *>( pTrigger );
	if ( pAreaCap )
	{
		m_bCapBlocked = pAreaCap->IsBlocked();
		m_hAreaCap = pAreaCap;
	}

	if ( iNumCappers <= 0 && !m_bCapBlocked && m_bTrainCanRecede )
	{
		if ( !m_bWaitingToRecede )
		{
			// start receding in [tf_escort_cart_recede_time] seconds
			m_bWaitingToRecede = true;

			if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->InOvertime() )
			{
				m_flRecedeTotalTime = tf_escort_recede_time_overtime.GetFloat();
			}
			else
			{
				m_flRecedeTotalTime = tf_escort_recede_time.GetFloat();
				if ( m_nTrainRecedeTime > 0 )
				{
					m_flRecedeTotalTime = m_nTrainRecedeTime;
				}
			}

			m_flRecedeStartTime = gpGlobals->curtime;
			m_flRecedeTime = m_flRecedeStartTime + m_flRecedeTotalTime;
		}		
	}
	else
	{
		// cancel receding
		m_bWaitingToRecede = false;
		m_flRecedeTime = 0;
	}

	HandleTrainMovement();
}

// only used for train watchers that control the train movement
void CTeamTrainWatcher::SetNumTrainCappers( int iNumCappers, CBaseEntity *pTrigger )
{
	if ( IsDisabled() || !m_bHandleTrainMovement )
		return;

	InternalSetNumTrainCappers( iNumCappers, pTrigger );
}

void CTeamTrainWatcher::InputSetNumTrainCappers( inputdata_t &inputdata )
{
	InternalSetNumTrainCappers( inputdata.value.Int(), inputdata.pCaller );
}

void CTeamTrainWatcher::InputSetTrainRecedeTime( inputdata_t &inputdata )
{
	int nSeconds = inputdata.value.Int();
	if ( nSeconds >= 0 )
	{
		m_nTrainRecedeTime = nSeconds;
	}
	else
	{
		m_nTrainRecedeTime = 0;
	}
}

void CTeamTrainWatcher::InputSetTrainCanRecede( inputdata_t &inputdata )
{
	m_bTrainCanRecede = inputdata.value.Bool();
}

void CTeamTrainWatcher::InputOnStartOvertime( inputdata_t &inputdata )
{
	// recalculate the recede time
	if ( m_bWaitingToRecede )
	{
		float flRecedeTimeRemaining = m_flRecedeTime - gpGlobals->curtime;
		float flOvertimeRecedeLen = tf_escort_recede_time_overtime.GetFloat();

		// drop to overtime recede time if it's more than that
		if ( flRecedeTimeRemaining > flOvertimeRecedeLen )
		{
			m_flRecedeTotalTime = flOvertimeRecedeLen;
			m_flRecedeStartTime = gpGlobals->curtime;
			m_flRecedeTime = m_flRecedeStartTime + m_flRecedeTotalTime;
		}
	}
}

#ifdef GLOWS_ENABLE
void CTeamTrainWatcher::FindGlowEntity( void )
{
	if ( m_hTrain && ( m_hTrain->GetEntityName() != NULL_STRING ) )
	{
		string_t iszTrainName = m_hTrain->GetEntityName();
		CBaseEntity *pGlowEnt = NULL;

		// first try to find a phys_constraint relationship with the train
		CPhysFixed *pPhysConstraint = dynamic_cast<CPhysFixed*>( gEntList.FindEntityByClassname( NULL, "phys_constraint" ) );
		while ( pPhysConstraint )
		{
			string_t iszName1 = pPhysConstraint->GetNameAttach1();
			string_t iszName2 = pPhysConstraint->GetNameAttach2();

			if ( iszTrainName == iszName1 )
			{
				pGlowEnt = gEntList.FindEntityByName( NULL, STRING( iszName2 ) );
				break;
			}
			else if ( iszTrainName == iszName2 )
			{
				pGlowEnt = gEntList.FindEntityByName( NULL, STRING( iszName1 ) );
				break;
			}
			
			pPhysConstraint = dynamic_cast<CPhysFixed*>( gEntList.FindEntityByClassname( pPhysConstraint, "phys_constraint" ) );
		}

		if ( !pGlowEnt )
		{
			// if we're here, we haven't found the glow entity yet...try all of the prop_dynamic entities
			CDynamicProp *pPropDynamic = dynamic_cast<CDynamicProp*>( gEntList.FindEntityByClassname( NULL, "prop_dynamic" ) );
			while ( pPropDynamic )
			{
				if ( pPropDynamic->GetParent() == m_hTrain )
				{
					pGlowEnt = pPropDynamic;
					break;
				}

				pPropDynamic = dynamic_cast<CDynamicProp*>( gEntList.FindEntityByClassname( pPropDynamic, "prop_dynamic" ) );
			}
		}

		// if we still haven't found a glow entity, just have the CFuncTrackTrain glow
		if ( !pGlowEnt )
		{
			pGlowEnt = m_hTrain.Get();
		}

		if ( pGlowEnt )
		{
			pGlowEnt->SetTransmitState( FL_EDICT_ALWAYS );
			m_hGlowEnt.Set( pGlowEnt );
		}
	}
}
#endif // GLOWS_ENABLE

// ==========================================================
// given a start node and a list of goal nodes
// calculate the distance between each
// ==========================================================
void CTeamTrainWatcher::WatcherActivate( void )
{		
	m_flRecedeTime = 0;
	m_bWaitingToRecede = false;
	m_bCapBlocked = false;
	m_flNextSpeakForwardConceptTime = 0;
	m_hAreaCap = NULL;
	m_flTrainDistanceFromStart = 0.0f;

	m_bAlarmPlayed = false;

	m_Sparks.Purge();

	StopCaptureAlarm();

	// init our train
	m_hTrain = dynamic_cast<CFuncTrackTrain*>( gEntList.FindEntityByName( NULL, m_iszTrain ) );
	if ( !m_hTrain )
	{
		Warning("%s failed to find train named '%s'\n", GetClassname(), STRING( m_iszTrain ) );
	}

	// find the trigger area that will give us movement updates and find the sparks (if we're going to handle the train movement)
	if ( m_bHandleTrainMovement )
	{
		if ( m_hTrain )
		{
			for ( int i=0; i<ITriggerAreaCaptureAutoList::AutoList().Count(); ++i )
			{
				CTriggerAreaCapture *pArea = static_cast< CTriggerAreaCapture * >( ITriggerAreaCaptureAutoList::AutoList()[i] );
				if ( pArea->GetParent() == m_hTrain.Get() )
				{
					// this is the capture area we care about, so let it know that we want updates on the capture numbers
					pArea->SetTrainWatcher( this );
					break;
				}
			}
		}

		// init the sprites (if any)
		CEnvSpark *pSpark = dynamic_cast<CEnvSpark*>( gEntList.FindEntityByName( NULL, m_iszSparkName ) );
		while ( pSpark )
		{
			m_Sparks.AddToTail( pSpark );
			pSpark = dynamic_cast<CEnvSpark*>( gEntList.FindEntityByName( pSpark, m_iszSparkName ) );
		}
	}

	// init our array of path_tracks linked to control points
	m_iNumCPLinks = 0;

	int i;
	for ( i = 0 ; i < MAX_CONTROL_POINTS ; i++ )
	{
		CPathTrack *pPathTrack = dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( NULL, m_iszLinkedPathTracks[i] ) );
		CTeamControlPoint *pCP = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszLinkedCPs[i] ) );
		if ( pPathTrack && pCP )
		{
			m_CPLinks[m_iNumCPLinks].hPathTrack = pPathTrack;
			m_CPLinks[m_iNumCPLinks].hCP = pCP;
			m_CPLinks[m_iNumCPLinks].flDistanceFromStart = 0;	// filled in when we parse the nodes
			m_CPLinks[m_iNumCPLinks].bAlertPlayed = false;
			m_iNumCPLinks++;
		}
	}

	// init our start and goal nodes
	m_hStartNode = dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( NULL, m_iszStartNode ) );
	if ( !m_hStartNode )
	{
		Warning("%s failed to find path_track named '%s'\n", GetClassname(), STRING(m_iszStartNode) );
	}

	m_hGoalNode = dynamic_cast<CPathTrack*>( gEntList.FindEntityByName( NULL, m_iszGoalNode ) );
	if ( !m_hGoalNode )
	{
		Warning("%s failed to find path_track named '%s'\n", GetClassname(), STRING(m_iszGoalNode) );
	}

	m_flTotalPathDistance = 0.0f;

	CUtlVector< float > hillData;
	bool bOnHill = false;

	bool bDownHillData[TEAM_TRAIN_MAX_HILLS];
	Q_memset( bDownHillData, 0, sizeof( bDownHillData ) );
	int iHillCount = 0;

	if( m_hStartNode.Get() && m_hGoalNode.Get() )
	{
		CPathTrack *pNode = m_hStartNode;
		CPathTrack *pPrev = pNode;
		CPathTrack *pHillStart = NULL;
		pNode = pNode->GetNext();
		int iHillType = HILL_TYPE_NONE;

		// don't check the start node for links. If it's linked, it will have 0 distance anyway
		while ( pNode )
		{
			Vector dir = pNode->GetLocalOrigin() - pPrev->GetLocalOrigin();
			float length = dir.Length();

			m_flTotalPathDistance += length;

			// gather our hill data for the HUD
			if ( pNode->GetHillType() != iHillType )
			{
				if ( !bOnHill ) // we're at the start of a hill
				{
					hillData.AddToTail( m_flTotalPathDistance );
					bOnHill = true;
					pHillStart = pNode;

					if ( iHillCount < TEAM_TRAIN_MAX_HILLS )
					{
						bDownHillData[iHillCount] = pNode->IsDownHill() ? true : false;
						iHillCount++;
					}
				}
				else // we're at the end of a hill
				{
					float flDistance = m_flTotalPathDistance - length; // subtract length because the prev node was the end of the hill (not this one)

					if ( pHillStart && ( pHillStart == pPrev ) )
					{
						flDistance = m_flTotalPathDistance; // we had a single node marked as a hill, so we'll use the current distance as the next marker
					}

					hillData.AddToTail( flDistance ); 

					// is our current node the start of another hill?
					if ( pNode->GetHillType() != HILL_TYPE_NONE )
					{
						hillData.AddToTail( m_flTotalPathDistance );
						bOnHill = true;
						pHillStart = pNode;

						if ( iHillCount < TEAM_TRAIN_MAX_HILLS )
						{
							bDownHillData[iHillCount] = pNode->IsDownHill() ? true : false;
							iHillCount++;
						}
					}
					else
					{
						bOnHill = false;
						pHillStart = NULL;
					}
				}

				iHillType = pNode->GetHillType();
			}

			// if pNode is one of our cp nodes, store its distance from m_hStartNode
			for ( i = 0 ; i < m_iNumCPLinks ; i++ )
			{
				if ( m_CPLinks[i].hPathTrack == pNode )
				{
					m_CPLinks[i].flDistanceFromStart = m_flTotalPathDistance;
					break;
				}
			}

			if ( pNode == m_hGoalNode )
				break;

			pPrev = pNode;
			pNode = pNode->GetNext();
		}
	}

	// if we don't have an even number of entries in our hill data (beginning/end) add the final distance
	if ( ( hillData.Count() % 2 ) != 0 )
	{
		hillData.AddToTail( m_flTotalPathDistance );
	}

	if ( ObjectiveResource() )
	{
		ObjectiveResource()->ResetHillData( GetTeamNumber() );

		// convert our hill data into 0-1 percentages for networking
		if ( m_flTotalPathDistance > 0 && hillData.Count() > 0 )
		{
			i = 0;
	 		while ( i < hillData.Count() )
			{
				if ( i < TEAM_TRAIN_HILLS_ARRAY_SIZE - 1 ) // - 1 because we want to use 2 entries
				{
					// add/subtract to the hill start/end to fix rounding errors in the HUD when the train
					// stops at the bottom/top of a hill but the HUD thinks the train is still on the hill
					ObjectiveResource()->SetHillData( GetTeamNumber(), (hillData[i] / m_flTotalPathDistance) + 0.005f, (hillData[i+1] / m_flTotalPathDistance) - 0.005f, bDownHillData[i/2] );
				}
				i = i + 2;
			}
		}
	}
 
	// We have total distance and increments in our links array
	for ( i=0;i<m_iNumCPLinks;i++ )
	{
		int iCPIndex = m_CPLinks[i].hCP.Get()->GetPointIndex();
// This can be pulled once DoD includes team_objectiveresource.* and c_team_objectiveresource.*
#ifndef DOD_DLL 
		ObjectiveResource()->SetTrainPathDistance( iCPIndex, m_CPLinks[i].flDistanceFromStart / m_flTotalPathDistance );
#endif
	}

#ifdef GLOWS_ENABLE
	FindGlowEntity();
#endif // GLOWS_ENABLE

	InternalSetSpeedForwardModifier( m_flSpeedForwardModifier );

	SetContextThink( &CTeamTrainWatcher::WatcherThink, gpGlobals->curtime + 0.1, TW_THINK );
}

void CTeamTrainWatcher::StopCaptureAlarm( void )
{
	if ( m_pAlarm )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pAlarm );
		m_pAlarm = NULL;
		m_flAlarmEndTime = -1.0f;
	}

	SetContextThink( NULL, 0, TW_ALARM_THINK );
}

void CTeamTrainWatcher::StartCaptureAlarm( CTeamControlPoint *pPoint )
{
	StopCaptureAlarm();

	if ( pPoint )
	{
		CReliableBroadcastRecipientFilter filter;
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pAlarm = controller.SoundCreate( filter, pPoint->entindex(), CHAN_STATIC, TEAM_TRAIN_ALARM, ATTN_NORM );
		controller.Play( m_pAlarm, 1.0, PITCH_NORM );

		m_flAlarmEndTime = gpGlobals->curtime + MAX_ALARM_TIME_NO_RECEDE;
	}
}

void CTeamTrainWatcher::PlayCaptureAlert( CTeamControlPoint *pPoint, bool bFinalPointInMap )
{
	if ( !pPoint )
		return;

	if ( TeamplayRoundBasedRules() )
	{
		TeamplayRoundBasedRules()->PlayTrainCaptureAlert( pPoint, bFinalPointInMap );
	}
}


ConVar tf_show_train_path( "tf_show_train_path", "0", FCVAR_CHEAT );

void CTeamTrainWatcher::WatcherThink( void )
{
	if ( m_bWaitingToRecede )
	{
		if ( m_flRecedeTime < gpGlobals->curtime )
		{
			m_bWaitingToRecede = false;

			// don't actually recede in overtime
			if ( TeamplayRoundBasedRules() && !TeamplayRoundBasedRules()->InOvertime() )
			{
				// fire recede output
				m_OnTrainStartRecede.FireOutput( this, this );
				HandleTrainMovement( true );
			}
		}
	}

	bool bDisableAlarm = (TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->State_Get() != GR_STATE_RND_RUNNING);
	if ( bDisableAlarm )
	{
		StopCaptureAlarm();
	}

	// given its next node, we can walk the nodes and find the linear
	// distance to the next cp node, or to the goal node

	CFuncTrackTrain *pTrain = m_hTrain;
	if ( pTrain )
	{
		int iOldTrainSpeedLevel = m_iTrainSpeedLevel;

		// how fast is the train moving?
		float flSpeed = pTrain->GetDesiredSpeed();

		// divide speed into regions
		// anything negative is -1

		if ( flSpeed < 0 )
		{
			m_iTrainSpeedLevel = -1;

			// even though our desired speed might be negative,
			// our actual speed might be zero if we're at a dead end...
			// this will turn off the < image when the train is done moving backwards
			if ( pTrain->GetCurrentSpeed() == 0 )
			{
				m_iTrainSpeedLevel = 0;
			}
		}
		else if ( flSpeed > m_flSpeedLevels[2] )
		{
			m_iTrainSpeedLevel = 3;
		}
		else if ( flSpeed > m_flSpeedLevels[1] )
		{
			m_iTrainSpeedLevel = 2;
		}
		else if ( flSpeed > m_flSpeedLevels[0] )
		{
			m_iTrainSpeedLevel = 1;
		}
		else
		{
			m_iTrainSpeedLevel = 0;
		}

		if ( m_iTrainSpeedLevel != iOldTrainSpeedLevel )
		{
			// make sure the sparks are off if we're not moving backwards anymore
			if ( m_bHandleTrainMovement )
			{
				if ( m_iTrainSpeedLevel == 0 && iOldTrainSpeedLevel != 0 )
				{
					HandleSparks( false );
				}
			}

			// play any concepts that we might need to play		
			if ( TeamplayRoundBasedRules() )
			{
				if ( m_iTrainSpeedLevel == 0 && iOldTrainSpeedLevel != 0 )
				{
					TeamplayRoundBasedRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_CART_STOP );
					m_flNextSpeakForwardConceptTime = 0;
				}
				else if ( m_iTrainSpeedLevel < 0 && iOldTrainSpeedLevel == 0 )
				{
					TeamplayRoundBasedRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_CART_MOVING_BACKWARD );
					m_flNextSpeakForwardConceptTime = 0;
				}
			}
		}

		if ( m_iTrainSpeedLevel > 0 && m_flNextSpeakForwardConceptTime < gpGlobals->curtime )
		{
			if ( m_hAreaCap.Get() )
			{
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );
					if ( pPlayer )
					{
						if ( m_hAreaCap->IsTouching( pPlayer ) )
						{
							pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_CART_MOVING_FORWARD );
						}
					}
				}
			}

			m_flNextSpeakForwardConceptTime = gpGlobals->curtime + 3.0;
		}

		// what percent progress are we at?
		CPathTrack *pNode = ( pTrain->m_ppath ) ? pTrain->m_ppath->GetNext() : NULL;

		// if we're moving backwards, GetNext is going to be wrong
		if ( flSpeed < 0 )
		{
			pNode = pTrain->m_ppath;
		}

		if ( pNode )
		{
			float flDistanceToGoal = 0;

			// distance to next node
			Vector vecDir = pNode->GetLocalOrigin() - pTrain->GetLocalOrigin();
			flDistanceToGoal = vecDir.Length();

			// distance of next node to goal node
			if ( pNode && pNode != m_hGoalNode )
			{
				// walk this until we get to goal node, or a dead end
				CPathTrack *pPrev = pNode;
				pNode = pNode->GetNext();
				while ( pNode )
				{
					vecDir = pNode->GetLocalOrigin() - pPrev->GetLocalOrigin();
					flDistanceToGoal += vecDir.Length();

					if ( pNode == m_hGoalNode )
						break;

					pPrev = pNode;
					pNode = pNode->GetNext();
				}
			}

			if ( m_flTotalPathDistance <= 0 )
			{
				Assert( !"No path distance in team_train_watcher\n" );
				m_flTotalPathDistance = 1;
			}

			m_flTotalProgress = clamp( 1.0 - ( flDistanceToGoal / m_flTotalPathDistance ), 0.0, 1.0 );

			m_flTrainDistanceFromStart = m_flTotalPathDistance - flDistanceToGoal;

			// play alert sounds if necessary
			for ( int iCount = 0 ; iCount < m_iNumCPLinks ; iCount++ )
			{
				if ( m_flTrainDistanceFromStart < m_CPLinks[iCount].flDistanceFromStart - TEAM_TRAIN_ALERT_DISTANCE )
				{
					// back up twice the alert distance before resetting our flag to play the warning again
					if ( ( m_flTrainDistanceFromStart < m_CPLinks[iCount].flDistanceFromStart - ( TEAM_TRAIN_ALERT_DISTANCE * 2 ) ) || // has receded back twice the alert distance or...
						 ( !m_bTrainCanRecede ) ) // used to catch the case where the train doesn't normally recede but has rolled back down a hill away from the CP
					{
						// reset our alert flag
						m_CPLinks[iCount].bAlertPlayed = false;
					}
				}
				else
				{
					if ( m_flTrainDistanceFromStart < m_CPLinks[iCount].flDistanceFromStart && !m_CPLinks[iCount].bAlertPlayed )
					{
						m_CPLinks[iCount].bAlertPlayed = true;
						bool bFinalPointInMap = false;

						CTeamControlPoint *pCurrentPoint = m_CPLinks[iCount].hCP.Get();
						CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
						if ( pMaster )
						{
							// if we're not playing mini-rounds 
							if ( !pMaster->PlayingMiniRounds() )  
							{
								for ( int i = FIRST_GAME_TEAM ; i < MAX_CONTROL_POINT_TEAMS ; i++ )
								{
									if ( ObjectiveResource() && ObjectiveResource()->TeamCanCapPoint( pCurrentPoint->GetPointIndex(), i ) )
									{
										if ( pMaster->WouldNewCPOwnerWinGame( pCurrentPoint, i ) )
										{
											bFinalPointInMap = true;
										}
									}
								}
							}
							else 
							{
								// or this is the last round
								if ( pMaster->NumPlayableControlPointRounds() == 1 )
								{
									CTeamControlPointRound *pRound = pMaster->GetCurrentRound();
									if ( pRound )
									{
										for ( int i = FIRST_GAME_TEAM ; i < MAX_CONTROL_POINT_TEAMS ; i++ )
										{
											if ( ObjectiveResource() && ObjectiveResource()->TeamCanCapPoint( pCurrentPoint->GetPointIndex(), i ) )
											{
												if ( pRound->WouldNewCPOwnerWinGame( pCurrentPoint, i ) )
												{
													bFinalPointInMap = true;
												}
											}
										}
									}
								}
							}
						}

						PlayCaptureAlert( pCurrentPoint, bFinalPointInMap );
					}
				}
			}

			// check to see if we need to start or stop the alarm
			if ( flDistanceToGoal <= TEAM_TRAIN_ALARM_DISTANCE )
			{
				if ( ObjectiveResource() )
				{
					ObjectiveResource()->SetTrackAlarm( GetTeamNumber(), true );
				}

				if ( !bDisableAlarm )
				{
					if ( !m_pAlarm )
					{
						if ( m_iNumCPLinks > 0 && !m_bAlarmPlayed )
						{
							// start the alarm at the final point
							StartCaptureAlarm( m_CPLinks[m_iNumCPLinks-1].hCP.Get() );
							m_bAlarmPlayed = true; // used to prevent the alarm from starting again on maps where the train doesn't recede (alarm loops for short time then only plays singles)
						}
					}
					else
					{
						if ( !m_bTrainCanRecede ) // if the train won't recede, we only want to play the alarm for a short time
						{
							if ( m_flAlarmEndTime > 0 && m_flAlarmEndTime < gpGlobals->curtime )
							{
								StopCaptureAlarm();
								SetContextThink( &CTeamTrainWatcher::WatcherAlarmThink, gpGlobals->curtime + TW_ALARM_THINK_INTERVAL, TW_ALARM_THINK );
							}
						}
					}
				}
			}
			else
			{
				if ( ObjectiveResource() )
				{
					ObjectiveResource()->SetTrackAlarm( GetTeamNumber(), false );
				}

				StopCaptureAlarm();
				m_bAlarmPlayed = false;
			}
		}

		if ( tf_show_train_path.GetBool() )
		{
			CPathTrack *nextNode = NULL;
			CPathTrack *node = m_hStartNode;

			CPathTrack::BeginIteration();
			while( node )
			{
				node->Visit();
				nextNode = node->GetNext();

				if ( !nextNode || nextNode->HasBeenVisited() )
					break;

				NDebugOverlay::Line( node->GetAbsOrigin(), nextNode->GetAbsOrigin(), 255, 255, 0, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );

				node = nextNode;
			}
			CPathTrack::EndIteration();

			// show segment of path train is actually on
			node = pTrain->m_ppath;
			if ( node && node->GetNext() )
			{
				NDebugOverlay::HorzArrow( node->GetAbsOrigin(), node->GetNext()->GetAbsOrigin(), 5.0f, 255, 0, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}
	}

	SetContextThink( &CTeamTrainWatcher::WatcherThink, gpGlobals->curtime + 0.1, TW_THINK );
}

void CTeamTrainWatcher::WatcherAlarmThink( void )
{
	CTeamControlPoint *pPoint = m_CPLinks[m_iNumCPLinks-1].hCP.Get();
	if ( pPoint )
	{
		pPoint->EmitSound( TEAM_TRAIN_ALARM_SINGLE );
	}

	SetContextThink( &CTeamTrainWatcher::WatcherAlarmThink, gpGlobals->curtime + TW_ALARM_THINK_INTERVAL, TW_ALARM_THINK );
}

CBaseEntity *CTeamTrainWatcher::GetTrainEntity( void )
{
	return m_hTrain.Get();
}

bool CTeamTrainWatcher::TimerMayExpire( void )
{
	if ( IsDisabled() )
	{
		return true;
	}

	// Still in overtime if we're waiting to recede
	if ( m_bWaitingToRecede )
		return false;

	// capture blocked so we're not receding, but game shouldn't end
	if ( m_bCapBlocked )
		return false;

	// not waiting, so we're capping, in which case the area capture
	// will not let us expire
	return true;
}


// Project the given position onto the track and return the point and how far along that projected position is
void CTeamTrainWatcher::ProjectPointOntoPath( const Vector &pos, Vector *posOnPathResult, float *distanceAlongPathResult ) const
{
	CPathTrack *nextNode = NULL;
	CPathTrack *node = m_hStartNode;

	Vector toPos;
	Vector alongPath;
	float distanceAlong = 0.0f;

	Vector closestPointOnPath = vec3_origin;
	float closestPerpendicularDistanceSq = FLT_MAX;
	float closestDistanceAlongPath = FLT_MAX;

	CPathTrack::BeginIteration();
	while( node )
	{
		node->Visit();
		nextNode = node->GetNext();

		if ( !nextNode || nextNode->HasBeenVisited() )
			break;

		alongPath = nextNode->GetAbsOrigin() - node->GetAbsOrigin();
		float segmentLength = alongPath.NormalizeInPlace();

		toPos = pos - node->GetAbsOrigin();
		float segmentOverlap = DotProduct( toPos, alongPath );

		if ( segmentOverlap >= 0.0f && segmentOverlap < segmentLength )
		{
			// projection is within segment bounds
			Vector onPath = node->GetAbsOrigin() + alongPath * segmentOverlap;

			float perpendicularDistanceSq = ( onPath - pos ).LengthSqr();
			if ( perpendicularDistanceSq < closestPerpendicularDistanceSq )
			{
				closestPointOnPath = onPath;
				closestPerpendicularDistanceSq = perpendicularDistanceSq;
				closestDistanceAlongPath = distanceAlong + segmentOverlap;
			}
		}

		distanceAlong += segmentLength;
		node = nextNode;
	}
	CPathTrack::EndIteration();

	if ( posOnPathResult )
	{
		*posOnPathResult = closestPointOnPath;
	}

	if ( distanceAlongPathResult )
	{
		*distanceAlongPathResult = closestDistanceAlongPath;
	}
}


// Return true if the given position is farther down the track than the train is
bool CTeamTrainWatcher::IsAheadOfTrain( const Vector &pos ) const
{
	float distanceAlongPath;
	ProjectPointOntoPath( pos, NULL, &distanceAlongPath );

	return ( distanceAlongPath > m_flTrainDistanceFromStart );
}


// return true if the train is almost at the next checkpoint
bool CTeamTrainWatcher::IsTrainNearCheckpoint( void ) const
{
	for( int i = 0; i < m_iNumCPLinks ; ++i )
	{
		if ( m_flTrainDistanceFromStart > m_CPLinks[i].flDistanceFromStart - TEAM_TRAIN_ALERT_DISTANCE &&
			m_flTrainDistanceFromStart < m_CPLinks[i].flDistanceFromStart )
		{
			return true;
		}
	}

	return false;
}


// return true if the train hasn't left its starting position yet
bool CTeamTrainWatcher::IsTrainAtStart( void ) const
{
	return ( m_flTrainDistanceFromStart < TEAM_TRAIN_ALARM_DISTANCE );
}


// return world space location of next checkpoint along the path
Vector CTeamTrainWatcher::GetNextCheckpointPosition( void ) const
{
	for( int i = 0; i < m_iNumCPLinks ; ++i )
	{
		if ( m_flTrainDistanceFromStart < m_CPLinks[i].flDistanceFromStart )
		{
			return m_CPLinks[i].hPathTrack->GetAbsOrigin();
		}
	}

	Assert( !"No checkpoint found in team train watcher\n" );
	return vec3_origin;
}

#if defined( STAGING_ONLY ) && defined( TF_DLL )
CON_COMMAND_F( tf_dumptrainstats, "Dump the stats for the current train watcher to the console", FCVAR_GAMEDLL )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CTeamTrainWatcher *pWatcher = NULL;
	while( ( pWatcher = dynamic_cast< CTeamTrainWatcher * >( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) ) ) != NULL )
	{
		pWatcher->DumpStats();
	}
}

void CTeamTrainWatcher::DumpStats( void )
{
	float flLastPosition = 0.0f;
	float flTotalDistance = 0.0f;
 	char szOutput[2048];
	char szTemp[256];

	V_strcpy_safe( szOutput, "\n\nTrain Watcher stats for team " );
	V_strcat_safe( szOutput, ( GetTeamNumber() == TF_TEAM_RED ) ? "Red\n" : "Blue\n" );

	for( int i = 0; i < m_iNumCPLinks ; ++i )
	{
		float flDistance = m_CPLinks[i].flDistanceFromStart - flLastPosition;
		if ( i == 0 )
		{
			V_sprintf_safe( szTemp, "\tControl Point: %d\tDistance from start: %0.2f\n", i + 1, flDistance );
		}
		else
		{
			V_sprintf_safe( szTemp, "\tControl Point: %d\tDistance from previous point: %0.2f\n", i + 1, flDistance );
		}
		V_strcat_safe( szOutput, szTemp );
		flTotalDistance += flDistance;
		flLastPosition = m_CPLinks[i].flDistanceFromStart;
	}

	V_sprintf_safe( szTemp, "\tTotal Distance: %0.2f\n\n", flTotalDistance ); 
	V_strcat_safe( szOutput, szTemp );
	Msg( "%s", szOutput );
}
#endif // STAGING_ONLY && TF_DLL


