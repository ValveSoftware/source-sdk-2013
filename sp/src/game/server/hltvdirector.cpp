//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// hltvdirector.cpp: implementation of the CHLTVDirector class.
//
//////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hltvdirector.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


static ConVar tv_delay( "tv_delay", "30", 0, "SourceTV broadcast delay in seconds", true, 0, true, HLTV_MAX_DELAY );
static ConVar tv_allow_static_shots( "tv_allow_static_shots", "1", 0, "Auto director uses fixed level cameras for shots" );
static ConVar tv_allow_camera_man( "tv_allow_camera_man", "1", 0, "Auto director allows spectators to become camera man" );

static bool GameEventLessFunc( CHLTVGameEvent const &e1, CHLTVGameEvent const &e2 )
{
	return e1.m_Tick < e2.m_Tick;
}

#define RANDOM_MAX_ELEMENTS		256
static int s_RndOrder[RANDOM_MAX_ELEMENTS];
static void InitRandomOrder(int nFields)
{
	if ( nFields > RANDOM_MAX_ELEMENTS )
	{
		Assert( nFields > RANDOM_MAX_ELEMENTS );
		nFields = RANDOM_MAX_ELEMENTS;
	}

	for ( int i=0; i<nFields; i++ )
	{
		s_RndOrder[i]=i;
	}

	for ( int i=0; i<(nFields/2); i++ )
	{
		int pos1 = RandomInt( 0, nFields-1 );
		int pos2 = RandomInt( 0, nFields-1 );
		int temp = s_RndOrder[pos1];
		s_RndOrder[pos1] = s_RndOrder[pos2];
		s_RndOrder[pos2] = temp;
	}
};


static float WeightedAngle( Vector vec1, Vector vec2)
{
	VectorNormalize( vec1 );
	VectorNormalize( vec2 );

	float a = DotProduct( vec1, vec2 ); // a = [-1,1]

	a = (a + 1.0f) / 2.0f;

	Assert ( a <= 1 && a >= 0 );
		
	return a*a;	// vectors are facing opposite direction
}

#if !defined( CSTRIKE_DLL ) && !defined( DOD_DLL ) && !defined( TF_DLL )// add your mod here if you use your own director

static CHLTVDirector s_HLTVDirector;	// singleton

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CHLTVDirector, IHLTVDirector, INTERFACEVERSION_HLTVDIRECTOR, s_HLTVDirector );

CHLTVDirector* HLTVDirector()
{
	return &s_HLTVDirector;
}

IGameSystem* HLTVDirectorSystem()
{
	return &s_HLTVDirector;
}

#endif // MODs



CHLTVDirector::CHLTVDirector()
{
	m_iPVSEntity = 0;
	m_fDelay = 30.0;
	m_iLastPlayer = 1;
	m_pHLTVServer = NULL;
	m_pHLTVClient = NULL;
	m_iCameraMan = 0;
	m_nNumFixedCameras = 0;
	m_EventHistory.SetLessFunc( GameEventLessFunc );
	m_nNextAnalyzeTick = 0;
	m_iCameraManIndex = 0;
}

CHLTVDirector::~CHLTVDirector()
{

}

bool CHLTVDirector::Init()
{
	return gameeventmanager->LoadEventsFromFile( "resource/hltvevents.res" ) > 0;
}

void CHLTVDirector::Shutdown()
{
	RemoveEventsFromHistory(-1); // all
}

void CHLTVDirector::FireGameEvent( IGameEvent * event )
{
	if ( !m_pHLTVServer )
		return;	// don't do anything

	CHLTVGameEvent gameevent;

	gameevent.m_Event = gameeventmanager->DuplicateEvent( event );
	gameevent.m_Priority = event->GetInt( "priority", -1 ); // priorities are leveled between 0..10, -1 means ignore
	gameevent.m_Tick = gpGlobals->tickcount;
	
	m_EventHistory.Insert( gameevent );
}

IHLTVServer* CHLTVDirector::GetHLTVServer( void )
{
	return m_pHLTVServer;
}

void CHLTVDirector::SetHLTVServer( IHLTVServer *hltv )
{
	RemoveEventsFromHistory(-1); // all

	if ( hltv ) 
	{
		m_pHLTVClient = UTIL_PlayerByIndex( hltv->GetHLTVSlot() + 1 );

		if ( m_pHLTVClient && m_pHLTVClient->IsHLTV() )
		{
			m_pHLTVServer = hltv;
		}
		else
		{
			m_pHLTVServer  = NULL;
			Error( "Couldn't find HLTV client player." );
		}

		// register for events the director needs to know
		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "round_end" );
		ListenForGameEvent( "round_start" );
		ListenForGameEvent( "hltv_cameraman" );
		ListenForGameEvent( "hltv_rank_entity" );
		ListenForGameEvent( "hltv_rank_camera" );
	}
	else
	{
		// deactivate HLTV director
		m_pHLTVServer = NULL;
	}
}

bool CHLTVDirector::IsActive( void )
{
	return (m_pHLTVServer != NULL );
}

float CHLTVDirector::GetDelay( void )
{
	return m_fDelay;
}

int	CHLTVDirector::GetDirectorTick( void )
{
	// just simple delay it
	return m_nBroadcastTick;
}

int	CHLTVDirector::GetPVSEntity( void )
{
	return m_iPVSEntity;
}

Vector CHLTVDirector::GetPVSOrigin( void )
{
	return m_vPVSOrigin;
}

void CHLTVDirector::UpdateSettings()
{
	// set delay
	m_fDelay = tv_delay.GetFloat();

	int newBroadcastTick = gpGlobals->tickcount;
	
	if ( m_fDelay < HLTV_MIN_DIRECTOR_DELAY )
	{
		// instant broadcast, no delay
		m_fDelay = 0.0;
	}
	else
	{
		// broadcast time is current time - delay time
		newBroadcastTick -= TIME_TO_TICKS( m_fDelay );
	}

	if( (m_nBroadcastTick == 0) && (newBroadcastTick > 0) )
	{
		// we start broadcasting right now, reset NextShotTimer
		m_nNextShotTick = 0;
	}

	// check if camera man is still valid 
	if ( m_iCameraManIndex > 0 )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( m_iCameraManIndex );
		if ( !pPlayer || pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			SetCameraMan( 0 );
		}
	}

   	m_nBroadcastTick = MAX( 0, newBroadcastTick );
}

const char** CHLTVDirector::GetModEvents()
{
	static const char *s_modevents[] =
	{
		"hltv_status",
		"hltv_chat",
		"player_connect",
		"player_disconnect",
		"player_team",
		"player_info",
		"server_cvar",
		"player_death",
		"player_chat",
		"round_start",
		"round_end",
		NULL
	};

	return s_modevents;
}


void CHLTVDirector::BuildCameraList( void )
{
	m_nNumFixedCameras = 0;
	memset( m_pFixedCameras, 0, sizeof ( m_pFixedCameras ) );

	CBaseEntity *pCamera = gEntList.FindEntityByClassname( NULL, GetFixedCameraEntityName() );

	while ( pCamera && m_nNumFixedCameras < MAX_NUM_CAMERAS)
	{
		CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, STRING(pCamera->m_target) );

		if ( pTarget )
		{
			// look at target if any given
			QAngle angles;
			VectorAngles( pTarget->GetAbsOrigin() - pCamera->GetAbsOrigin(), angles );
			pCamera->SetAbsAngles( angles );
		}

		m_pFixedCameras[m_nNumFixedCameras] = pCamera;

		m_nNumFixedCameras++;
		pCamera = gEntList.FindEntityByClassname( pCamera, GetFixedCameraEntityName() );
	}
}

// this is called with every new map 
void CHLTVDirector::LevelInitPostEntity( void )
{
	BuildCameraList();

	m_vPVSOrigin.Init();
	m_iPVSEntity = 0;
	m_nNextShotTick = 0;
	m_nNextAnalyzeTick = 0;
	m_iCameraManIndex = 0;

	RemoveEventsFromHistory(-1); // all

	// DevMsg("HLTV Director: found %i fixed cameras.\n", m_nNumFixedCameras );
}

void CHLTVDirector::FrameUpdatePostEntityThink( void )
{
	if ( !m_pHLTVServer )
		return;	// don't do anything

	// This function is called each tick
	UpdateSettings();	// update settings from cvars

	if ( (m_nNextAnalyzeTick < gpGlobals->tickcount) && 
		 (m_fDelay >= HLTV_MIN_DIRECTOR_DELAY) )
	{
		m_nNextAnalyzeTick = gpGlobals->tickcount + TIME_TO_TICKS( 0.5f );

		AnalyzePlayers();

		AnalyzeCameras();
	}

	if ( m_nBroadcastTick <= 0 )
	{
		// game start is still in delay loop
		StartDelayMessage();
	}
	else if ( m_nNextShotTick <= m_nBroadcastTick )
	{
		// game is being broadcasted, generate camera shots
		StartNewShot();		
	}
}

void CHLTVDirector::StartDelayMessage()
{
	if ( m_nNextShotTick > gpGlobals->tickcount )
		return;

	// check the next 8 seconds for interrupts/important events
	m_nNextShotTick = gpGlobals->tickcount + TIME_TO_TICKS( DEF_SHOT_LENGTH );

	// game hasn't started yet, we are still in the broadcast delay hole
	IGameEvent *msg = gameeventmanager->CreateEvent( "hltv_message", true );

	if ( msg )
	{
		msg->SetString("text", "Please wait for broadcast to start ..." );

		// send spectators the HLTV director command as a game event
		m_pHLTVServer->BroadcastEvent( msg );
		gameeventmanager->FreeEvent( msg );
	}

	StartBestFixedCameraShot( true );
}

void CHLTVDirector::StartBestPlayerCameraShot()
{
	float flPlayerRanking[MAX_PLAYERS];

	memset( flPlayerRanking, 0, sizeof(flPlayerRanking) );

	int firstIndex = FindFirstEvent( m_nBroadcastTick );

	int index = firstIndex;

	float flBestRank = -1.0f;
	int iBestCamera = -1;
	int iBestTarget = -1;

	// sum all ranking values for the cameras

	while( index != m_EventHistory.InvalidIndex() )
	{
		CHLTVGameEvent &dc = m_EventHistory[index];

		if ( dc.m_Tick >= m_nNextShotTick )
			break; 

		// search for camera ranking events
		if ( Q_strcmp( dc.m_Event->GetName(), "hltv_rank_entity") == 0 )
		{
			int index = dc.m_Event->GetInt("index"); 

			if ( index < MAX_PLAYERS )
			{
				flPlayerRanking[index] += dc.m_Event->GetFloat("rank" );

				// find best camera
				if ( flPlayerRanking[index] > flBestRank )
				{
					iBestCamera = index;
					flBestRank = flPlayerRanking[index];
					iBestTarget = dc.m_Event->GetInt("target"); 
				}
			}
		}

		index = m_EventHistory.NextInorder( index );
	}

	if ( iBestCamera != -1 )
	{
		// view over shoulder, randomly left or right
		StartChaseCameraShot( iBestCamera, iBestTarget, 112.0f, 20, (RandomFloat()>0.5)?20:-20, false );
	}
	else
	{
		StartBestFixedCameraShot( true );
	}
}

void CHLTVDirector::StartFixedCameraShot(int iCamera, int iTarget)
{
	CBaseEntity *pCamera = m_pFixedCameras[iCamera];
	

	Vector vCamPos = pCamera->GetAbsOrigin();
	QAngle aViewAngle = pCamera->GetAbsAngles();

	m_iPVSEntity = 0;	// don't use camera entity, since it may not been transmitted
	m_vPVSOrigin = vCamPos;

	IGameEvent *shot = gameeventmanager->CreateEvent( "hltv_fixed", true );

	if ( shot )
	{
		shot->SetInt("posx", vCamPos.x );
		shot->SetInt("posy", vCamPos.y );
		shot->SetInt("posz", vCamPos.z );
		shot->SetInt("theta", aViewAngle.x );
		shot->SetInt("phi", aViewAngle.y );
		shot->SetInt("target", iTarget );
		shot->SetFloat("fov", RandomFloat(50,110) );
	
		// send spectators the HLTV director command as a game event
		m_pHLTVServer->BroadcastEvent( shot );
		gameeventmanager->FreeEvent( shot );
	}
}

void CHLTVDirector::StartChaseCameraShot(int iTarget1, int iTarget2, int distance, int phi, int theta, bool bInEye)
{
	IGameEvent *shot = gameeventmanager->CreateEvent( "hltv_chase", true );

	if ( !shot )
		return;
	
	shot->SetInt("target1", iTarget1 );
	shot->SetInt("target2", iTarget2 );
	shot->SetInt("distance", distance );
	shot->SetInt("phi", phi ); // hi/low
	shot->SetInt( "theta", theta ); // left/right
	shot->SetInt( "ineye", bInEye?1:0 );
		
	m_iPVSEntity = iTarget1;

	// send spectators the HLTV director command as a game event
	m_pHLTVServer->BroadcastEvent( shot );
	gameeventmanager->FreeEvent( shot );
}

void CHLTVDirector::StartBestFixedCameraShot( bool bForce )
{
	float	flCameraRanking[MAX_NUM_CAMERAS];

	if ( m_nNumFixedCameras <= 0 )
		return;

	memset( flCameraRanking, 0, sizeof(flCameraRanking) );

	int firstIndex = FindFirstEvent( m_nBroadcastTick );

	int index = firstIndex;

	float flBestRank = -1.0f;
	int iBestCamera = -1;
	int iBestTarget = -1;

	// sum all ranking values for the cameras

	while( index != m_EventHistory.InvalidIndex() )
	{
		CHLTVGameEvent &dc = m_EventHistory[index];

		if ( dc.m_Tick >= m_nNextShotTick )
			break; 

		// search for camera ranking events
		if ( Q_strcmp( dc.m_Event->GetName(), "hltv_rank_camera") == 0 )
		{
			int index = dc.m_Event->GetInt("index"); 
			flCameraRanking[index] += dc.m_Event->GetFloat("rank" );

			// find best camera
			if ( flCameraRanking[index] > flBestRank )
			{
				iBestCamera = index;
				flBestRank = flCameraRanking[index];
				iBestTarget = dc.m_Event->GetInt("target"); 
			}
		}

		index = m_EventHistory.NextInorder( index );
	}

	if ( !bForce && flBestRank == 0 )
	{
		// if we are not forcing a fixed camera shot, switch to player chase came
		// if no camera shows any players
		StartBestPlayerCameraShot();
	}
	else if ( iBestCamera != -1 )
	{
		StartFixedCameraShot( iBestCamera, iBestTarget );
	}
}

void CHLTVDirector::StartRandomShot() 
{
	int toTick = m_nBroadcastTick + TIME_TO_TICKS ( DEF_SHOT_LENGTH );
	m_nNextShotTick = MIN( m_nNextShotTick, toTick );

	if ( RandomFloat(0,1) < 0.25 && tv_allow_static_shots.GetBool() )
	{
		// create a static shot from a level camera
		StartBestFixedCameraShot( false );
	}
	else
	{
		// follow a player
		StartBestPlayerCameraShot();
	}
}

void CHLTVDirector::CreateShotFromEvent( CHLTVGameEvent *event )
{
	// show event at least for 2 more seconds after it occured
	const char *name = event->m_Event->GetName();
	
	bool bPlayerHurt = Q_strcmp( "player_hurt", name ) == 0;
	bool bPlayerKilled = Q_strcmp( "player_death", name ) == 0;
	bool bRoundStart = Q_strcmp( "round_start", name ) == 0;
	bool bRoundEnd = Q_strcmp( "round_end", name ) == 0;

	if ( bPlayerHurt || bPlayerKilled )
	{
		CBaseEntity *victim = UTIL_PlayerByUserId( event->m_Event->GetInt("userid") );
		CBaseEntity *attacker = UTIL_PlayerByUserId( event->m_Event->GetInt("attacker") );

		if ( !victim )
			return;

		if ( attacker == victim || attacker == NULL )
		{
			// player killed self or by WORLD
			StartChaseCameraShot( victim->entindex(), 0, 96, 20, 0, false );
		}
		else // attacker != NULL
		{
			// check if we would show it from ineye view
			bool bInEye = (bPlayerKilled && RandomFloat(0,1) > 0.33) || (bPlayerHurt && RandomFloat(0,1) > 0.66); 

			// if we show ineye view, show it more likely from killer
			if ( RandomFloat(0,1) > (bInEye?0.3f:0.7f)  )
			{
				::V_swap( attacker, victim );
			}
						
			// hurting a victim is shown as chase more often
			// view from behind over head
			// lower view point, dramatic
			// view over shoulder, randomly left or right
			StartChaseCameraShot( victim->entindex(), attacker->entindex(), 96, -20, (RandomFloat()>0.5)?30:-30, bInEye );
		}
				
		// shot 2 seconds after death/hurt
		m_nNextShotTick = MIN( m_nNextShotTick, (event->m_Tick+TIME_TO_TICKS(2.0)) );
	}
	else if ( bRoundStart || bRoundEnd )
	{
		StartBestFixedCameraShot( false );
	}
	else
	{
		DevMsg( "No known TV shot for event %s\n", name );
	}
}

void CHLTVDirector::CheckHistory()
{
	int index = m_EventHistory.FirstInorder();
	int lastTick = -1;

	while ( index != m_EventHistory.InvalidIndex() )
	{
		CHLTVGameEvent &dc = m_EventHistory[index];

		Assert( lastTick <= dc.m_Tick );
		lastTick = dc.m_Tick;

		index = m_EventHistory.NextInorder( index );
	}
}

void CHLTVDirector::RemoveEventsFromHistory(int tick)
{
	int index = m_EventHistory.FirstInorder();

	while ( index != m_EventHistory.InvalidIndex() )
	{
		CHLTVGameEvent &dc = m_EventHistory[index];

		if ( (dc.m_Tick < tick) || (tick == -1) )
		{
			gameeventmanager->FreeEvent( dc.m_Event );
			dc.m_Event = NULL;
			m_EventHistory.RemoveAt( index );
			index = m_EventHistory.FirstInorder();	// start again
		}
		else
		{
			index = m_EventHistory.NextInorder( index );
		}
	}

#ifdef _DEBUG
	CheckHistory();
#endif
}

int CHLTVDirector::FindFirstEvent( int tick )
{
	// TODO cache last queried ticks

	int index = m_EventHistory.FirstInorder();

	if ( index == m_EventHistory.InvalidIndex() )
		return index; // no commands in list

	CHLTVGameEvent *event = &m_EventHistory[index];

	while ( event->m_Tick < tick )
	{
		index = m_EventHistory.NextInorder( index );
		
		if ( index == m_EventHistory.InvalidIndex() )
			break;

		event = &m_EventHistory[index];
	}

	return index;
}

bool CHLTVDirector::SetCameraMan( int iPlayerIndex )
{
	if ( !tv_allow_camera_man.GetBool() )
		return false;

	if ( m_iCameraManIndex == iPlayerIndex )
		return true;

	// check if somebody else is already the camera man
	if ( m_iCameraManIndex != 0 && iPlayerIndex != 0 )
		return false;

	CBasePlayer *pPlayer = NULL;

	if ( iPlayerIndex > 0 )
	{
		pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
		if ( !pPlayer || pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
			return false;
	}

	m_iCameraManIndex = iPlayerIndex;

	// create event for director event history
	IGameEvent *event = gameeventmanager->CreateEvent( "hltv_cameraman" );
	if ( event )
	{
		event->SetInt("index", iPlayerIndex );
		gameeventmanager->FireEvent( event );
	}

	CRecipientFilter filter;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && pPlayer->GetTeamNumber() == TEAM_SPECTATOR && !pPlayer->IsFakeClient() )
		{
			filter.AddRecipient( pPlayer );
		}
	}

	filter.MakeReliable();
	
	if ( iPlayerIndex > 0 )
	{
		// tell all spectators that the camera is in use.
		char szText[200];
		Q_snprintf( szText, sizeof(szText), "SourceTV camera is now controlled by %s.", pPlayer->GetPlayerName() );
		UTIL_ClientPrintFilter( filter, HUD_PRINTTALK, szText );
	}
	else
	{
		// tell all spectators that the camera is available again.
		UTIL_ClientPrintFilter( filter, HUD_PRINTTALK, "SourceTV camera switched to auto-director mode." );
	}
	
	return true;
}

void CHLTVDirector::FinishCameraManShot()
{
	Assert( m_iCameraMan == m_iPVSEntity );

	int index = FindFirstEvent( m_nBroadcastTick );

	if ( index == m_EventHistory.InvalidIndex() )
	{
		// check next frame again if event history is empty
		m_nNextShotTick = m_nBroadcastTick+1;
		return;
	}

	m_nNextShotTick = m_nBroadcastTick + TIME_TO_TICKS( MIN_SHOT_LENGTH );

	//check if camera turns camera off within broadcast time and game time
	while( index != m_EventHistory.InvalidIndex() )
	{
		CHLTVGameEvent &dc = m_EventHistory[index];

		if ( dc.m_Tick >= m_nNextShotTick )
			break;

		if ( Q_strcmp( dc.m_Event->GetName(), "hltv_cameraman") == 0 )
		{
			int iNewCameraMan = dc.m_Event->GetInt("index");

			if ( iNewCameraMan == 0 )
			{
				// camera man switched camera off
				m_nNextShotTick = dc.m_Tick+1;
				m_iCameraMan = 0;
				return;
			}
		}

		index = m_EventHistory.NextInorder( index );
	}

	// camera man is still recording and live, resend camera man message
	IGameEvent *msg = gameeventmanager->CreateEvent( "hltv_cameraman", true );
	if ( msg )
	{
		msg->SetInt("index", m_iCameraMan );
		m_pHLTVServer->BroadcastEvent( msg );
		gameeventmanager->FreeEvent( msg );
	}

}

bool CHLTVDirector::StartCameraManShot()
{
	Assert( m_nNextShotTick <= m_nBroadcastTick );

	int index = FindFirstEvent( m_nNextShotTick );

	// check for cameraman mode
	while( index != m_EventHistory.InvalidIndex() )
	{
		CHLTVGameEvent &dc = m_EventHistory[index];

		// only check if this is the current tick
		if ( dc.m_Tick > m_nBroadcastTick )
			break; 

		if ( Q_strcmp( dc.m_Event->GetName(), "hltv_cameraman") == 0 )
		{
			if ( dc.m_Event->GetInt("index") > 0 )
			{
				// ok, this guy is now the active camera man
				m_iCameraMan = dc.m_Event->GetInt("index");

				m_iPVSEntity = m_iCameraMan;
				m_nNextShotTick = m_nBroadcastTick+1; // check setting right on next frame

				// send camera man command to client
				m_pHLTVServer->BroadcastEvent( dc.m_Event );
				return true;
			}
		}

		index = m_EventHistory.NextInorder( index );
	}

	return false;	// no camera man found
}

void CHLTVDirector::StartInstantBroadcastShot()
{
	m_nNextShotTick = m_nBroadcastTick + TIME_TO_TICKS( MAX_SHOT_LENGTH );

	if ( m_iCameraManIndex > 0 )
	{
		// camera man is still recording and live, resend camera man message
		IGameEvent *msg = gameeventmanager->CreateEvent( "hltv_cameraman", true );
		if ( msg )
		{
			msg->SetInt("index", m_iCameraManIndex );
			m_pHLTVServer->BroadcastEvent( msg );
			gameeventmanager->FreeEvent( msg );

			m_iPVSEntity = m_iCameraManIndex;
			m_nNextShotTick = m_nBroadcastTick+TIME_TO_TICKS( MIN_SHOT_LENGTH ); 
		}
	}
	else
	{
		RemoveEventsFromHistory(-1); // all

		AnalyzePlayers();

		AnalyzeCameras();

		StartRandomShot();
	}
}

void CHLTVDirector::StartNewShot()
{
	// we can remove all events the
	int smallestTick = MAX(0, gpGlobals->tickcount - TIME_TO_TICKS(HLTV_MAX_DELAY) );
    RemoveEventsFromHistory( smallestTick );

	// if the delay time is to short for autodirector, just show next best thing
	if ( m_fDelay < HLTV_MIN_DIRECTOR_DELAY )
	{
		StartInstantBroadcastShot();
		return;
	}

	if ( m_iCameraMan > 0 )
	{
		// we already have an active camera man,
		// wait until he releases the "record" lock
		FinishCameraManShot();
		return;	
	}

	if ( StartCameraManShot() )
	{
		// now we have an active camera man
		return;
	} 

	   // ok, no camera man active, now check how much time
	// we have for the next shot, if the time diff to the next
	// important event we have to switch to is too short (<2sec)
	// just extent the current shot and don't start a new one

	// check the next 8 seconds for interrupts/important events
	m_nNextShotTick = m_nBroadcastTick + TIME_TO_TICKS( MAX_SHOT_LENGTH );

	if ( m_nBroadcastTick <= 0 )
	{
		// game hasn't started yet, we are still in the broadcast delay hole
		IGameEvent *msg = gameeventmanager->CreateEvent( "hltv_message", true );

		if ( msg )
		{
			msg->SetString("text", "Please wait for broadcast to start ..." );
			
			// send spectators the HLTV director command as a game event
			m_pHLTVServer->BroadcastEvent( msg );
			gameeventmanager->FreeEvent( msg );
		}

		StartBestFixedCameraShot( true );
		return;
	}
	
	int index = FindFirstEvent( m_nBroadcastTick );

	while( index != m_EventHistory.InvalidIndex() )
	{
		CHLTVGameEvent &dc = m_EventHistory[index];

		if ( dc.m_Tick >= m_nNextShotTick )
			break; // we have searched enough

		// a camera man is always interrupting auto director
		if ( Q_strcmp( dc.m_Event->GetName(), "hltv_cameraman") == 0 )
		{
			if ( dc.m_Event->GetInt("index") > 0 )
			{
				// stop the next cut when this cameraman starts recording
				m_nNextShotTick = dc.m_Tick;
				break;
			}
		}

		index = m_EventHistory.NextInorder( index );
	} 

	float flDuration = TICKS_TO_TIME(m_nNextShotTick - m_nBroadcastTick);

	if ( flDuration < MIN_SHOT_LENGTH )
		return;	// not enough time for a new shot

	// find the most interesting game event for next shot
	CHLTVGameEvent *dc = FindBestGameEvent();

	if ( dc )
	{
		// show the game event
		CreateShotFromEvent( dc );
	}
	else
	{
		// no interesting events found, start random shot
		StartRandomShot();
	}
}

CHLTVGameEvent *CHLTVDirector::FindBestGameEvent()
{
	int	bestEvent[4];
	int	bestEventPrio[4];

	Q_memset( bestEvent, 0, sizeof(bestEvent) );
	Q_memset( bestEventPrio, 0, sizeof(bestEventPrio) );
	
	int index = FindFirstEvent( m_nBroadcastTick );
		
	// search for next 4 best events within next 8 seconds
	for (int i = 0; i<4; i ++)
	{
		bestEventPrio[i] = 0;
		bestEvent[i] = 0;
	
		int tillTick = m_nBroadcastTick + TIME_TO_TICKS( 2.0f*(1.0f+i) );

		if ( tillTick > m_nNextShotTick )
			break;

		// sum all action for the next time
		while ( index != m_EventHistory.InvalidIndex()  )
		{
			CHLTVGameEvent &event = m_EventHistory[index];

			if ( event.m_Tick > tillTick )
				break;

			int priority = event.m_Priority;

			if ( priority > bestEventPrio[i] )
			{
				bestEvent[i] = index;
				bestEventPrio[i] = priority;
			}
			
			index = m_EventHistory.NextInorder( index );
		}
	}

	if ( !( bestEventPrio[0] || bestEventPrio[1] || bestEventPrio[2] ) )
		return NULL; // no event found at all, give generic algorithm a chance

	// camera cut rules :

	if ( bestEventPrio[1] >= bestEventPrio[0] &&
		 bestEventPrio[1] >= bestEventPrio[2] &&
		 bestEventPrio[1] >= bestEventPrio[3] )
	{
		return &m_EventHistory[ bestEvent[1] ];	// best case
	}
	else if ( bestEventPrio[0] > bestEventPrio[1] &&
			  bestEventPrio[0] > bestEventPrio[2] )
	{
		return &m_EventHistory[ bestEvent[0] ];	// event 0 is very important
	}
	else if (	bestEventPrio[2] > bestEventPrio[3] ) 
	{
		return &m_EventHistory[ bestEvent[2] ];	
	}
	else
	{
		// event 4 is the best but too far away, so show event 1 
		if ( bestEvent[0] )
			return &m_EventHistory[ bestEvent[0] ];
		else
			return NULL;
	}
}

void CHLTVDirector::AnalyzeCameras()
{
	InitRandomOrder( m_nNumFixedCameras );

	for ( int i = 0; i<m_nNumFixedCameras; i++ )
	{
		int iCameraIndex = s_RndOrder[i];
		CBaseEntity *pCamera = m_pFixedCameras[ iCameraIndex ];

		float	flRank = 0.0f;
		int		iClosestPlayer = 0;
		float	flClosestPlayerDist = 100000.0f;
		int		nCount = 0; // Number of visible targets
		Vector	vDistribution; vDistribution.Init(); // distribution of targets

		Vector vCamPos = pCamera->GetAbsOrigin();

		for ( int j=0; j<m_nNumActivePlayers; j++ )
		{
			CBasePlayer *pPlayer = m_pActivePlayers[j];

			Vector vPlayerPos = pPlayer->GetAbsOrigin();

			float dist = VectorLength( vPlayerPos - vCamPos );

			if ( dist > 1024.0f || dist < 4.0f )
				continue;	// too colse or far away

			// check visibility
			trace_t tr;
			UTIL_TraceLine( vCamPos, pPlayer->GetAbsOrigin(), MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr  );

			if ( tr.fraction < 1.0 )
				continue;	// not visible for camera

			nCount++;

			// remember closest player
			if ( dist <  flClosestPlayerDist )
			{
				iClosestPlayer = pPlayer->entindex();
				flClosestPlayerDist = dist;
			}

			Vector v1; AngleVectors( pPlayer->EyeAngles(), &v1 );

			// check players orientation towards camera
			Vector v2 = vCamPos - vPlayerPos;
			VectorNormalize( v2 );

			// player/camera cost function:
			flRank += ( 1.0f/sqrt(dist) ) * WeightedAngle( v1, v2 );

			vDistribution += v2;
		}

		if ( nCount > 0 )
		{
			// normalize distribution
			flRank *= VectorLength( vDistribution ) / nCount; 
		}
		
		IGameEvent *event = gameeventmanager->CreateEvent("hltv_rank_camera");

		if ( event )
		{
			event->SetFloat("rank", flRank );
			event->SetInt("index",  iCameraIndex ); // index in m_pFixedCameras
			event->SetInt("target",  iClosestPlayer ); // ent index
			gameeventmanager->FireEvent( event );
		}
	}
}

void CHLTVDirector::BuildActivePlayerList()
{
	// first build list of all active players

	m_nNumActivePlayers = 0;

	for ( int i =1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		if ( !pPlayer->IsAlive() )
			continue;

		if ( pPlayer->IsObserver() )
			continue;

		if ( pPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
			continue;
        
		m_pActivePlayers[m_nNumActivePlayers] = pPlayer;
		m_nNumActivePlayers++;
	}
}

void CHLTVDirector::AnalyzePlayers()
{
	// build list of current active players
	BuildActivePlayerList();

	// analyzes every active player

	InitRandomOrder( m_nNumActivePlayers );
	
	for ( int i = 0; i<m_nNumActivePlayers; i++ )
	{
		int iPlayerIndex = s_RndOrder[i];

		CBasePlayer *pPlayer = m_pActivePlayers[ iPlayerIndex ];

		float	flRank = 0.0f;
		int		iBestFacingPlayer = 0;
		float	flBestFacingPlayer = 0.0f;
		int		nCount = 0; // Number of visible targets
		Vector	vDistribution; vDistribution.Init(); // distribution of targets
		
		Vector vCamPos = pPlayer->GetAbsOrigin();

		Vector v1; AngleVectors( pPlayer->EyeAngles(), &v1 );

		v1 *= -1; // inverted

		for ( int j=0; j<m_nNumActivePlayers; j++ )
		{
			if ( iPlayerIndex == j )
				continue;  // don't check against itself
			
			CBasePlayer *pOtherPlayer = m_pActivePlayers[j];

			Vector vPlayerPos = pOtherPlayer->GetAbsOrigin();

			float dist = VectorLength( vPlayerPos - vCamPos );

			if ( dist > 1024.0f || dist < 4.0f )
				continue;	// too close or far away

			// check visibility
			trace_t tr;
			UTIL_TraceLine( vCamPos, pOtherPlayer->GetAbsOrigin(), MASK_SOLID, pOtherPlayer, COLLISION_GROUP_NONE, &tr  );

			if ( tr.fraction < 1.0 )
				continue;	// not visible for camera

			nCount++;

			// check players orientation towards camera
			Vector v2; AngleVectors( pOtherPlayer->EyeAngles(), &v2 );

			float facing = WeightedAngle( v1, v2 );

			// remember closest player
			if ( facing > flBestFacingPlayer )
			{
				iBestFacingPlayer = pOtherPlayer->entindex();
				flBestFacingPlayer = facing;
			}

			// player/camera cost function:
			flRank += ( 1.0f/sqrt(dist) ) * facing;

			vDistribution += v2;
		}

		if ( nCount > 0 )
		{
			float flDistribution =  VectorLength( vDistribution ) / nCount; // normalize distribution
			flRank *= flDistribution;
		}

		IGameEvent *event = gameeventmanager->CreateEvent("hltv_rank_entity");
		if ( event )
		{
			event->SetInt("index",  pPlayer->entindex() );
			event->SetFloat("rank", flRank );
			event->SetInt("target",  iBestFacingPlayer ); // ent index
			gameeventmanager->FireEvent( event );
		}
	}
}

