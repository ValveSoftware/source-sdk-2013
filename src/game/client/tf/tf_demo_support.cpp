//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "gcsdk/webapi_response.h"
#include "c_tf_player.h"
#include "tf_weapon_medigun.h"
#include "tf_demo_support.h"
#include "tf_gamerules.h"
#include "tf_hud_chat.h"
#include "vguicenterprint.h"

// Global singleton
static CTFDemoSupport g_DemoSupport;

extern ConVar mp_tournament;

ConVar ds_enable( "ds_enable", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - enable automatic .dem file recording and features. 0 - Manual, 1 - Auto-record competitive matches, 2 - Auto-record all matches, 3 - Auto-record tournament (mp_tournament) matches", true, 0, true, 3 ); 
ConVar ds_dir( "ds_dir", "demos", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - will put all files into this folder under the gamedir. 24 characters max." );
ConVar ds_prefix( "ds_prefix", "", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - will prefix files with this string. 24 characters max." );
ConVar ds_min_streak( "ds_min_streak", "4", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - minimum kill streak count before being recorded.", true, 2, false, 0 );
ConVar ds_kill_delay( "ds_kill_delay", "15", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - maximum time between kills for tracking kill streaks.", true, 5, false, 0 );
ConVar ds_log( "ds_log", "1", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - log kill streak and bookmark events to an associated .txt file.", true, 0, true, 1 );
ConVar ds_sound( "ds_sound", "1", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - play start/stop sound for demo recording.", true, 0, true, 1 ); 
ConVar ds_notify( "ds_notify", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - text output when recording start/stop/bookmark events : 0 - console, 1 - console and chat, 2 - console and HUD.", true, 0, true, 2 ); 
ConVar ds_screens( "ds_screens", "1", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - take screenshot of the scoreboard for non-competitive matches or the match summary stats for competitive matches. For competitive matches, it will not capture the screenshot if you disconnect from the server before the medal awards have completed.", true, 0, true, 1 );
ConVar ds_autodelete( "ds_autodelete", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Demo support - automatically delete .dem files with no associated bookmark or kill streak events.", true, 0, true, 1 ); 

CON_COMMAND_F( ds_mark, "Demo support - bookmark (with optional single-word description) the current tick count for the demo being recorded.", FCVAR_CLIENTDLL | FCVAR_DONTRECORD )
{
	g_DemoSupport.BookMarkCurrentTick( ( args.ArgC() > 1 ) ? args[1] : NULL );
}

CON_COMMAND_F( ds_record, "Demo support - start recording a demo.", FCVAR_CLIENTDLL | FCVAR_DONTRECORD )
{
	g_DemoSupport.StartRecording();
}

CON_COMMAND_F( ds_stop, "Demo support - stop recording a demo.", FCVAR_CLIENTDLL | FCVAR_DONTRECORD )
{
	g_DemoSupport.StopRecording();
}

CON_COMMAND_F( ds_status, "Demo support - show the current recording status.", FCVAR_CLIENTDLL | FCVAR_DONTRECORD )
{
	g_DemoSupport.Status();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static const char *g_aDemoEventNames[] =
{
	"Bookmark",
	"Killstreak",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aDemoEventNames ) == eDemoEvent_Last );

const char *GetDemoEventName( EDemoEventType eEventType )
{
	if ( ( eEventType >= ARRAYSIZE( g_aDemoEventNames ) ) || ( eEventType < 0 ) )
		return NULL;

	return g_aDemoEventNames[eEventType];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFDemoSupport::CTFDemoSupport() : CAutoGameSystemPerFrame( "CTFDemoSupport" )
{
	m_nKillCount = 0;
	m_flLastKill = -1.f;
	m_bRecording = false;
	m_hGlobalEventList = FILESYSTEM_INVALID_HANDLE;
	m_DemoSpecificEventList.Clear();
	m_DemoSpecificEventList.SetStatusCode( k_EHTTPStatusCode200OK );
	m_pRoot = NULL;
	m_pChildArray = NULL;
	m_flScreenshotTime = -1.f;
	m_bAlreadyAutoRecordedOnce = false;
	m_flNextRecordStartCheckTime = -1.f;
	m_bFirstEvent = false;
	m_nStartingTickCount = 0;
	m_bHasAtLeastOneEvent = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDemoSupport::Init()
{
	ListenForGameEvent( "localplayer_respawn" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "ds_screenshot" );
	ListenForGameEvent( "ds_stop" );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::LevelInitPostEntity()
{
	if ( engine->IsPlayingDemo() )
		return;

	m_bAlreadyAutoRecordedOnce = false;
	m_flNextRecordStartCheckTime = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::LevelShutdownPostEntity()
{
	if ( engine->IsPlayingDemo() )
		return;

	StopRecording();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::Update( float frametime )
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( ds_enable.GetInt() > 0 )
	{
		if ( !m_bRecording && !m_bAlreadyAutoRecordedOnce )
		{
			if ( ds_enable.GetInt() == 1 )
			{
				if ( TFGameRules() && !TFGameRules()->IsCompetitiveMode() )
					return;
			}
			else if ( ds_enable.GetInt() == 3 )
			{
				if ( !mp_tournament.GetBool() )
					return;
			}

			if ( ( m_flNextRecordStartCheckTime < 0 ) || ( m_flNextRecordStartCheckTime < gpGlobals->curtime ) )
			{
				CTFPlayer *pLocalPlayer = CTFPlayer::GetLocalTFPlayer();
				if ( pLocalPlayer )
				{
					// if the local player is on team spectator or is on a game team and has picked a player class
					if ( ( pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR ) || 
						 ( ( pLocalPlayer->GetTeamNumber() >= FIRST_GAME_TEAM ) && pLocalPlayer->GetPlayerClass() && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() >= TF_FIRST_NORMAL_CLASS ) && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() < TF_LAST_NORMAL_CLASS ) ) )
					{
						if ( !StartRecording() )
						{
							// we'll try again in 5 seconds
							m_flNextRecordStartCheckTime = gpGlobals->curtime + 5.f;
							return;
						}
					}
				}
			}
		}
	}

	if ( !m_bRecording )
		return;

	if ( ( m_flScreenshotTime > 0 ) && ( m_flScreenshotTime < gpGlobals->curtime ) )
	{
		m_flScreenshotTime = -1.f;

		if ( ds_screens.GetBool() )
		{
			engine->TakeScreenshot( m_szFilename, m_szFolder );
			Notify( "(Demo Support) Screenshot saved\n" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::Status( void )
{
	if ( engine->IsPlayingDemo() )
		return;

	char szStatus[64] = {0};

	if ( m_bRecording )
	{
		V_sprintf_safe( szStatus, "(Demo Support) Currently recording to %s\n", m_szFolderAndFilename );
	}
	else
	{
		V_strcpy_safe( szStatus, "(Demo Support) Not currently recording\n" );
	}

	Notify( szStatus );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::Notify( char *pszMessage )
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( pszMessage && pszMessage[0] )
	{
		// we'll always put the message in the console
		Msg( "%s", pszMessage );

		switch ( ds_notify.GetInt() )
		{
		default: // console
		case 0:
			break;
		case 1: // chat window
			{
				CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
				if ( pHUDChat )
				{
					pHUDChat->Printf( CHAT_FILTER_NONE, "%s", pszMessage );
				}
			}
			break;
		case 2: // hud center print
			internalCenterPrint->Print( pszMessage );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::LogEvent( EDemoEventType eType, int nValue /* = 0 */, const char *pszValue /* = NULL */ )
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( !m_bRecording )
		return;

	if ( !ds_log.GetBool() )
		return;

	char szArg[32] = {0};

	switch ( eType )
	{
	case eDemoEvent_Bookmark:
		{
			const char *pszTemp = "General";
			if ( pszValue && pszValue[0] )
			{
				pszTemp = pszValue;
			}

			V_strcpy_safe( szArg, pszTemp );
		}
		break;
	case eDemoEvent_Killstreak:
		V_sprintf_safe( szArg, "%d", nValue );
		break;
	default:
		// don't continue if this is an unknown type
		return;
	}

	time_t tTime = CRTime::RTime32TimeCur();
	struct tm tmStruct;
	struct tm *ptm = Plat_localtime( &tTime, &tmStruct );

	int nTickCount = gpGlobals->tickcount - m_nStartingTickCount;

	if ( m_hGlobalEventList != FILESYSTEM_INVALID_HANDLE )
	{
		if ( m_bFirstEvent )
		{
			m_bFirstEvent = false;
			g_pFullFileSystem->FPrintf( m_hGlobalEventList, ">\n" );
		}

		g_pFullFileSystem->FPrintf( m_hGlobalEventList, "[%04u/%02u/%02u %02u:%02u] %s %s (\"%s\" at %d)\n",
			ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, g_aDemoEventNames[eType], szArg, m_szFilename, nTickCount );
	}

	if ( m_pChildArray )
	{
		GCSDK::CWebAPIValues *pChildObject = m_pChildArray->AddChildObjectToArray();
		pChildObject->CreateChildObject( "name" )->SetStringValue( g_aDemoEventNames[eType] );
		pChildObject->CreateChildObject( "value" )->SetStringValue( szArg );
		pChildObject->CreateChildObject( "tick" )->SetInt32Value( nTickCount );
		m_bHasAtLeastOneEvent = true;
	}

	char szMessage[MAX_PATH] = {0};
	V_sprintf_safe( szMessage, "(Demo Support) Event recorded: %s %s\n", g_aDemoEventNames[eType], szArg );
	Notify( szMessage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::FireGameEvent( IGameEvent * event )
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( !m_bRecording )
		return;

	const char *pszEvent = event->GetName();

	if ( FStrEq( pszEvent, "localplayer_respawn" ) )
	{
		m_nKillCount = 0;
		m_flLastKill = -1.f;
	}
	else if ( FStrEq( pszEvent, "player_death" ) )
	{
		if ( m_bRecording )
		{
			CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
			if ( !pLocalPlayer )
				return;

			int nOldKillCount = m_nKillCount;
			int nLocalPlayerUserID = pLocalPlayer->GetUserID();

			if ( nLocalPlayerUserID == event->GetInt( "userid" ) )
			{
				// local player was the victim
				m_nKillCount = 0;
			}
			else if ( nLocalPlayerUserID == event->GetInt( "attacker" ) )
			{
				// local player was the killer
				if ( ( m_flLastKill < 0 ) || ( gpGlobals->curtime - m_flLastKill > ds_kill_delay.GetFloat() ) )
				{
					m_nKillCount = 1;
				}
				else
				{
					m_nKillCount++;
				}

				m_flLastKill = gpGlobals->curtime;
			}
			else if ( nLocalPlayerUserID == event->GetInt( "assister" ) )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( pLocalPlayer );
				if ( pTFPlayer )
				{
					if ( pTFPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pLocalPlayer->GetActiveWeapon() )
					{
						CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>( pLocalPlayer->GetActiveWeapon() );
						if ( pMedigun )
						{
							// local player was a medic healing the attacker so give them credit
							if ( ( m_flLastKill < 0 ) || ( gpGlobals->curtime - m_flLastKill > ds_kill_delay.GetFloat() ) )
							{
								m_nKillCount = 1;
							}
							else
							{
								m_nKillCount++;
							}

							m_flLastKill = gpGlobals->curtime;
						}
					}
				}
			}

			// if our kill-streak has increased, make an event entry
			if ( ( nOldKillCount != m_nKillCount ) && ( m_nKillCount > 0 ) && ( m_nKillCount >= ds_min_streak.GetInt() ) )
			{
				LogEvent( eDemoEvent_Killstreak, m_nKillCount );
			}
		}
	}
	else if ( FStrEq( pszEvent, "client_disconnect" ) )
	{
		StopRecording();
	}
	else if ( FStrEq( pszEvent, "ds_stop" ) )
	{
		StopRecording( true );
	}
	else if ( FStrEq( pszEvent, "ds_screenshot" ) )
	{
		if ( ds_screens.GetBool() )
		{
			float flDelay = event->GetFloat( "delay" );
			m_flScreenshotTime = gpGlobals->curtime + flDelay;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::BookMarkCurrentTick( const char *pszValue /* = NULL */ )
{
	if ( engine->IsPlayingDemo() )
		return;

	LogEvent( eDemoEvent_Bookmark, 0, pszValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDemoSupport::IsValidPath( const char *pszFolder )
{
	if ( !pszFolder )
		return false;

	if ( Q_strlen( pszFolder ) <= 0 ||
		Q_strstr( pszFolder, "\\\\" ) ||	// to protect network paths
		Q_strstr( pszFolder, ":" ) ||	// to protect absolute paths
		Q_strstr( pszFolder, ".." ) ||	// to protect relative paths
		Q_strstr( pszFolder, "\n" ) ||	// CFileSystem_Stdio::FS_fopen doesn't allow this
		Q_strstr( pszFolder, "\r" ) )	// CFileSystem_Stdio::FS_fopen doesn't allow this
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDemoSupport::StartRecording( void )
{
	if ( engine->IsPlayingDemo() )
		return false;

	// are we already recording?
	if ( m_bRecording )
	{
		Notify( "(Demo Support) Already recording\n" );
		return false;
	}

	// start recording the demo
	char szTime[k_RTimeRenderBufferSize] = {0};

	time_t tTime = CRTime::RTime32TimeCur();
	struct tm tmStruct;
	struct tm *ptm = Plat_localtime( &tTime, &tmStruct );

	V_sprintf_safe( szTime, "%04u-%02u-%02u_%02u-%02u-%02u",
		ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec );

	char szPrefix[24] = {0};
	V_sprintf_safe( szPrefix, "%s", ds_prefix.GetString() );
	V_sprintf_safe( m_szFilename, "%s%s", szPrefix, szTime );

	if ( Q_strlen( ds_dir.GetString() ) > 0 )
	{
		// check folder
		if ( !IsValidPath( ds_dir.GetString() ) )
		{
			Msg( "DemoSupport: invalid folder.\n" );
			return false;
		}

		V_sprintf_safe( m_szFolder, "%s", ds_dir.GetString() );

		// make sure the folder exists
		g_pFullFileSystem->CreateDirHierarchy( m_szFolder, "GAME" );

		V_sprintf_safe( m_szFolderAndFilename, "%s%c%s", m_szFolder, CORRECT_PATH_SEPARATOR, m_szFilename );
	}
	else
	{
		m_szFolder[0] = '\0';
		V_sprintf_safe( m_szFolderAndFilename, "%s", m_szFilename );
	}

	if ( !engine->StartDemoRecording( m_szFilename, m_szFolder ) )
	{
		Notify( "(Demo Support) Unable to start recording\n" );
		return false;
	}

	char szGobalFile[MAX_PATH] = { 0 };
	V_sprintf_safe( szGobalFile, "%s%c%s", m_szFolder, CORRECT_PATH_SEPARATOR, EVENTS_FILENAME );
	m_hGlobalEventList = g_pFullFileSystem->Open( szGobalFile, "at", "GAME" );
	m_bFirstEvent = true;

	m_DemoSpecificEventList.Clear();
	m_DemoSpecificEventList.SetStatusCode( k_EHTTPStatusCode200OK );
	m_pRoot = m_DemoSpecificEventList.CreateRootValue( "summary" );
	m_DemoSpecificEventList.SetJSONAnonymousRootNode( true );
	m_pChildArray = m_pRoot->CreateChildArray( "events", "event" );

	m_bRecording = true;
	m_bAlreadyAutoRecordedOnce = true;
	m_nStartingTickCount = gpGlobals->tickcount;
	m_bHasAtLeastOneEvent = false;

	if ( ds_sound.GetBool() )
	{
		CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "DemoSupport.StartRecording" );
		}
	}

	char szMessage[MAX_PATH] = { 0 };
	V_sprintf_safe( szMessage, "(Demo Support) Start recording %s\n", m_szFolderAndFilename );
	Notify( szMessage );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDemoSupport::StopRecording( bool bFromEngine /* = false */ )
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( !m_bRecording )
		return;

	m_bRecording = false;
	m_nStartingTickCount = 0;

	// stop recording the demo
	if ( !bFromEngine )
	{
		engine->StopDemoRecording();
	}

	if ( ds_sound.GetBool() )
	{
		CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "DemoSupport.EndRecording" );
		}
	}

	char szMessage[MAX_PATH] = { 0 };
	V_sprintf_safe( szMessage, "(Demo Support) End recording %s\n", m_szFolderAndFilename );
	Notify( szMessage );

	if ( m_hGlobalEventList != FILESYSTEM_INVALID_HANDLE )
	{
		g_pFullFileSystem->Close( m_hGlobalEventList );
		m_bFirstEvent = true;
	}

	if ( ds_autodelete.GetBool() && !m_bHasAtLeastOneEvent )
	{
		char szTemp[MAX_PATH] = { 0 };
		V_sprintf_safe( szTemp, "%s.dem", m_szFolderAndFilename );

		g_pFullFileSystem->RemoveFile( szTemp, "GAME" );
		V_sprintf_safe( szMessage, "(Demo Support) Auto-delete recording %s\n", m_szFolderAndFilename );
		Notify( szMessage );
	}
	else
	{
		if ( ds_log.GetBool() )
		{
			// write out the associated bookmark and kill-streak data file
			char szTempFilename[MAX_PATH] = {0};
			V_sprintf_safe( szTempFilename, "%s.json", m_szFolderAndFilename );

			CUtlBuffer buffer( 0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::EXTERNAL_GROWABLE );
			m_DemoSpecificEventList.BEmitFormattedOutput( GCSDK::k_EWebAPIOutputFormat_JSON, buffer, 0 );
			g_pFullFileSystem->WriteFile( szTempFilename, "GAME", buffer );
		}
	}

	m_DemoSpecificEventList.Clear();
	m_pRoot = NULL;
	m_pChildArray = NULL;
	m_bHasAtLeastOneEvent = false;
}


