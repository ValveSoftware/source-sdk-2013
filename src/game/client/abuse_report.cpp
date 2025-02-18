//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "abuse_report.h"
#include "abuse_report_ui.h"
#include "filesystem.h"
#include "imageutils.h"
#include "econ/confirm_dialog.h"
#include "econ/econ_notifications.h"

inline bool IsLoggedOnToSteam()
{
	return steamapicontext != NULL && steamapicontext->SteamUser() != NULL && steamapicontext->SteamUser()->BLoggedOn();
}

const char CAbuseReportManager::k_rchScreenShotFilenameBase[] = "abuse_report";
const char CAbuseReportManager::k_rchScreenShotFilename[] = "screenshots\\abuse_report.jpg";

//-----------------------------------------------------------------------------
class CEconNotification_AbuseReportReady : public CEconNotification
{
public:
	CEconNotification_AbuseReportReady() : CEconNotification()
	{
		m_bHasTriggered = false;
		m_bShowInGame = false;
	}

	~CEconNotification_AbuseReportReady()
	{
		//if ( !m_bHasTriggered )
		//{
		//	ReallyTrigger();
		//}
	}

	virtual void MarkForDeletion()
	{
		m_bHasTriggered = true;

		CEconNotification::MarkForDeletion();
	}

	virtual bool BShowInGameElements() const { return m_bShowInGame; }
	virtual EType NotificationType() { return eType_Trigger; }
	virtual void Trigger()
	{
		ReallyTrigger();
		MarkForDeletion();
	}

	virtual const char *GetUnlocalizedHelpText()
	{
		return "#AbuseReport_Notification_Help";
	}

	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast< CEconNotification_AbuseReportReady *>( pNotification ) != NULL; }
	static bool IsInGameNotificationType( CEconNotification *pNotification )
	{
		CEconNotification_AbuseReportReady *n = dynamic_cast< CEconNotification_AbuseReportReady *>( pNotification );
		return n != NULL && n->BShowInGameElements();
	}

	bool m_bShowInGame;

private:

	void ReallyTrigger()
	{
		Assert( !m_bHasTriggered );
		m_bHasTriggered = true;
		engine->ClientCmd_Unrestricted( "abuse_report_submit" );
	}

	bool m_bHasTriggered;
};

AbuseIncidentData_t::AbuseIncidentData_t()
{
	m_nScreenShotWaitFrames = 5;
}

AbuseIncidentData_t::~AbuseIncidentData_t()
{
}

bool AbuseIncidentData_t::Poll()
{
	bool bReady = true;

	// Poll player data
	for ( int i = 0 ; i < m_vecPlayers.Count() ; ++i )
	{

		// Make sure sure Steam knows we want the Avatar
		PlayerData_t *p = &m_vecPlayers[i];
		if ( p->m_iSteamAvatarIndex < 0 )
		{
			if ( steamapicontext && steamapicontext->SteamUser() )
			{

				p->m_iSteamAvatarIndex = steamapicontext->SteamFriends()->GetLargeFriendAvatar( p->m_steamID );
				if ( p->m_iSteamAvatarIndex < 0 )
				{
					bReady = false;
				}
			}
			else
			{
				p->m_iSteamAvatarIndex = 0;
			}
		}
	}

	// Screenshot ready?
	if ( !m_bitmapScreenshot.IsValid() && m_nScreenShotWaitFrames > 0 )
	{
		--m_nScreenShotWaitFrames;

		// Just load the whole file into a memory buffer
		char szFullPath[ MAX_PATH ] = "";
		if ( !g_pFullFileSystem->RelativePathToFullPath( CAbuseReportManager::k_rchScreenShotFilename, NULL, szFullPath, ARRAYSIZE(szFullPath) ) )
		{
			Assert( false ); // ???
		}

		// Load it

		if ( g_pFullFileSystem->FileExists( szFullPath ) )
		{

			// Load the screenshot into a local buffer
			if ( !g_pFullFileSystem->ReadFile( CAbuseReportManager::k_rchScreenShotFilename, NULL, m_bufScreenshotFileData ) )
			{
				Warning( "Failed to read back %s\n", CAbuseReportManager::k_rchScreenShotFilename );
				m_nScreenShotWaitFrames = 0;
			}
			else
			{
				ConversionErrorType nErrorCode = ImgUtl_LoadBitmap( szFullPath, m_bitmapScreenshot );
				if ( nErrorCode != CE_SUCCESS )
				{
					Warning( "Abuse report screenshot %s failed to load with error code %d\n", CAbuseReportManager::k_rchScreenShotFilename, nErrorCode );
					Assert( nErrorCode == CE_SUCCESS );
					m_nScreenShotWaitFrames = 0;
				}
				else
				{
					// !KLUDGE! Resize to power of two dimensions, since VGUI doesn't like odd sizes
					ImgUtl_ResizeBitmap( m_bitmapScreenshot, 1024, 1024, &m_bitmapScreenshot );
				}
			}
			g_pFullFileSystem->RemoveFile( CAbuseReportManager::k_rchScreenShotFilename );
		}
	}

	return bReady;
}

CAbuseReportManager *g_AbuseReportMgr;

CAbuseReportManager::CAbuseReportManager()
{
	m_pIncidentData = NULL;
	m_bTestReport = false;
	m_eIncidentDataStatus = k_EIncidentDataStatus_None;
	m_bReportUIPending = false;

	// We're the singleton --- set global pointer
	Assert( g_AbuseReportMgr == NULL );
	g_AbuseReportMgr = this;
	m_timeLastReportReadyNotification = 0.0;
	m_adrCurrentServer.Clear();
}

CAbuseReportManager::~CAbuseReportManager()
{
	Assert( m_pIncidentData == NULL );
}

char const *CAbuseReportManager::Name()
{
	return "AbuseRepotManager";
}

bool CAbuseReportManager::Init()
{
	// Clean out any temporary files
	Assert( m_pIncidentData == NULL );
	DestroyIncidentData();

	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "tf_game_over" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "server_spawn" );

	return true;
}

void CAbuseReportManager::LevelShutdownPreEntity()
{

	// Don't keep the dialog open across a level transition.  Don't discard their
	// report data, but let's kill the dialog
	if ( g_AbuseReportDlg.Get() != NULL )
	{
		Warning( "Abuse report dialog open during level shutdown.  Closing it.\n" );
		g_AbuseReportDlg.Get()->Close();
	}

	// And clear the 'pending' flag
	m_bReportUIPending = false;
}

void CAbuseReportManager::FireGameEvent( IGameEvent *event )
{
	//C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	const char *eventname = event->GetName();

	if ( !eventname || !eventname[0] )
		return;

	if (
		!Q_strcmp( "teamplay_round_win", eventname )
		|| !Q_strcmp( "tf_game_over", eventname )
	) {
		// Periodically remind them that they have a report ready to file
		CheckCreateReportReadyNotification( 60.0 * 5.0, true, 10.0f );
	}
	else if ( !Q_strcmp( "player_death", eventname ) )
	{
		// In some maps, the round just never ends.
		// So make sure we do remind them every now and then about this.
		// Just not too often
		CheckCreateReportReadyNotification( 60.0 * 20.0, true, 5.0f );
	}
	else if ( !Q_strcmp( "server_spawn", eventname ) )
	{
		m_adrCurrentServer.Clear();
		m_adrCurrentServer.SetFromString( event->GetString( "address", "" ), false );
		m_adrCurrentServer.SetPort( event->GetInt( "port", 0 ) );

		m_steamIDCurrentServer = CSteamID();
		if ( steamapicontext && steamapicontext->SteamUser() && GetUniverse() != k_EUniverseInvalid )
		{
			m_steamIDCurrentServer.SetFromString( event->GetString( "steamid", "" ), GetUniverse() );
		}
	}
}

void CAbuseReportManager::Shutdown()
{
	// Close the dialog, if any
	LevelShutdownPreEntity();

	DestroyIncidentData();

	// Clear global pointer
	Assert( g_AbuseReportMgr == this );
	if ( g_AbuseReportMgr == this )
	{
		g_AbuseReportMgr = NULL;
	}
}

void CAbuseReportManager::Update( float frametime )
{

	// if a dialog is already displayed, make sure we don't try to activate another
	if ( g_AbuseReportDlg.Get() != NULL )
	{
		m_bReportUIPending = false;
	}

	// Poll report data, if any
	if ( m_pIncidentData != NULL )
	{
		if ( m_eIncidentDataStatus == k_EIncidentDataStatus_Preparing )
		{
			if ( m_pIncidentData->Poll() )
			{
				m_eIncidentDataStatus = k_EIncidentDataStatus_Ready;
				CheckCreateReportReadyNotification( 1.0f, true, 7.0f );
			}
		}
		else
		{
			Assert( m_eIncidentDataStatus == k_EIncidentDataStatus_Ready );
		}

		if ( m_eIncidentDataStatus == k_EIncidentDataStatus_Ready && m_bReportUIPending )
		{
			m_bReportUIPending = false;
			ActivateSubmitReportUI();
		}
	}
	else
	{
		m_bReportUIPending = false;
	}

	// Re-create notification constantly in the menu.
	// While in game, we will only popup notifications
	// periodically at round end or player death
	CheckCreateReportReadyNotification( 10.0, false, 999.0f );
}

void CAbuseReportManager::SubmitReportUIRequested()
{
	if ( g_AbuseReportDlg.Get() != NULL )
	{
		Assert( g_AbuseReportDlg.Get() == NULL );
		return;
	}

	// If no report data already, then create some
	if ( m_pIncidentData == NULL )
	{
		QueueReport();
		if ( m_pIncidentData == NULL )
		{
			// Failed
			return;
		}
	}

	// Set flag to bring up the reporting UI at earliest opportunity,
	// once all data has been fetched asynchronously
	m_bReportUIPending = true;
}

bool CAbuseReportManager::CreateAndPopulateIncident()
{
	Assert( m_pIncidentData == NULL );

	// by default, just create the base class version
	m_pIncidentData = new AbuseIncidentData_t;

	// And populate it
	return PopulateIncident();
}

bool CAbuseReportManager::PopulateIncident()
{
	if ( m_pIncidentData == NULL )
	{
		Assert( m_pIncidentData );
		return false;
	}

	// Queue a screenshot
	CUtlString cmd;
	cmd.Format( "__screenshot_internal \"%s\"", k_rchScreenShotFilenameBase );
	engine->ClientCmd_Unrestricted( cmd );

	// Set status as preparing
	m_eIncidentDataStatus = k_EIncidentDataStatus_Preparing;

	m_pIncidentData->m_bCanReportGameServer = false;

	m_pIncidentData->m_adrGameServer.Clear();
	if (
		m_adrCurrentServer.IsValid()
		&& !m_adrCurrentServer.IsLocalhost()
		&& m_steamIDCurrentServer.IsValid()
		&& ( !m_adrCurrentServer.IsReservedAdr() || m_steamIDCurrentServer.GetEUniverse() != k_EUniversePublic )
	)
	{
		m_pIncidentData->m_adrGameServer = m_adrCurrentServer;
		m_pIncidentData->m_steamIDGameServer = m_steamIDCurrentServer;
		m_pIncidentData->m_bCanReportGameServer = true;
	}

	m_pIncidentData->m_matWorldToClip = engine->WorldToScreenMatrix();

	// Add in players
	for (int i = 1 ; i <= gpGlobals->maxClients ; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );

		#ifndef _DEBUG
			// Skip local players
			if ( player != NULL && player->IsLocalPlayer() )
			{
				continue;
			}
		#endif

		// Get player info from the engine.  This works even if they haven't spawned yet.
		player_info_t pi;
		if ( !engine->GetPlayerInfo( i, &pi ) )
		{
			continue;
		}

		if ( pi.fakeplayer )
		{
			continue;
		}
		if ( pi.friendsID == 0 )
		{
			continue;
		}
		CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
		if ( !steamID.IsValid() )
		{
			Assert( steamID.IsValid() );
			continue;
		}

		int arrayIndex = m_pIncidentData->m_vecPlayers.AddToTail();
		AbuseIncidentData_t::PlayerData_t *p = &m_pIncidentData->m_vecPlayers[ arrayIndex ];

		p->m_iClientIndex = i;
		p->m_steamID = steamID;
		p->m_sPersona = pi.name;
		p->m_bHasEntity = false;
		p->m_bRenderBoundsValid = false;
		p->m_screenBoundsMin.x = p->m_screenBoundsMin.y = 1.0f;
		p->m_screenBoundsMax.x = p->m_screenBoundsMax.y = 0.0f;

		if ( player==NULL )
		{
			continue;
		}

		p->m_bHasEntity = true;
		player->GetRenderBounds( p->m_vecRenderBoundsMin, p->m_vecRenderBoundsMax );
		p->m_matModelToWorld.CopyFrom3x4( player->RenderableToWorldTransform() );
		MatrixMultiply( m_pIncidentData->m_matWorldToClip, p->m_matModelToWorld, p->m_matModelToClip );

		// Gather up screen extents
		p->m_bRenderBoundsValid = false;
		for ( int j = 0 ; j < 8 ; ++j )
		{

			// Get corner point in model space
			Vector4D modelCorner(
				( j & 1 ) ? p->m_vecRenderBoundsMax.x : p->m_vecRenderBoundsMin.x,
				( j & 2 ) ? p->m_vecRenderBoundsMax.y : p->m_vecRenderBoundsMin.y,
				( j & 4 ) ? p->m_vecRenderBoundsMax.z : p->m_vecRenderBoundsMin.z,
				1.0f
			);

			// Transform to clip space
			Vector4D clipCorner;
			Vector4DMultiply( p->m_matModelToClip, modelCorner, clipCorner );

			//Msg( "%6.3f, %6.3f, %6.3f, %6.3f\n", clipCorner[0], clipCorner[1], clipCorner[2], clipCorner[3] );

			// If all points behind near clip plane, don't try to
			// figure out screen space bounds
			if ( clipCorner[3] > .1f )
			{
				p->m_bRenderBoundsValid = true;
			}

			// Push w forward to "near clip plane"
			float w = MAX( clipCorner[3], .1f );

			// Divide by w to project, and convert normalized device coordinates
			// where the view volume is (-1...1), to normalized screen coords, where
			// they are from 0...1
			float x = ( clipCorner[0] / w + 1.0f ) / 2.0f;
			float y = ( -clipCorner[1] / w + 1.0f ) / 2.0f;
			p->m_screenBoundsMin.x = MIN( p->m_screenBoundsMin.x, x );
			p->m_screenBoundsMax.x = MAX( p->m_screenBoundsMax.x, x );
			p->m_screenBoundsMin.y = MIN( p->m_screenBoundsMin.y, y );
			p->m_screenBoundsMax.y = MAX( p->m_screenBoundsMax.y, y );
		}

		// Clip projected rect to the screen
		if ( p->m_bRenderBoundsValid )
		{
			p->m_screenBoundsMin.x = MAX( p->m_screenBoundsMin.x, 0.0f );
			p->m_screenBoundsMax.x = MIN( p->m_screenBoundsMax.x, 1.0f );
			p->m_screenBoundsMin.y = MAX( p->m_screenBoundsMin.y, 0.0f );
			p->m_screenBoundsMax.y = MIN( p->m_screenBoundsMax.y, 1.0f );

			p->m_bRenderBoundsValid =
				p->m_screenBoundsMin.x + .01f < p->m_screenBoundsMax.x
				&& p->m_screenBoundsMin.y + .01f < p->m_screenBoundsMax.y;
		}

		// Sanity check that we agree on what their steam ID is!
		if ( player->GetSteamID( &steamID ) )
		{
			Assert( p->m_steamID == steamID );
		}
	}

	// Test harness: add in a handful of fake players
	#ifdef _DEBUG
		if ( m_bTestReport )
		{
			int arrayIndex = m_pIncidentData->m_vecPlayers.AddToTail();
			AbuseIncidentData_t::PlayerData_t *p = &m_pIncidentData->m_vecPlayers[ arrayIndex ];

			p->m_iClientIndex = -1;
			p->m_sPersona = "Lippencott";
			p->m_steamID.SetFromUint64( 148618791998333672 );

			arrayIndex = m_pIncidentData->m_vecPlayers.AddToTail();
			p = &m_pIncidentData->m_vecPlayers[ arrayIndex ];

			p->m_iClientIndex = -1;
			p->m_sPersona = "EricS";
			p->m_steamID.SetFromUint64( 148618791998195668 );

			arrayIndex = m_pIncidentData->m_vecPlayers.AddToTail();
			p = &m_pIncidentData->m_vecPlayers[ arrayIndex ];

			p->m_iClientIndex = -1;
			p->m_sPersona = "Sarenya";
			p->m_steamID.SetFromUint64( 148618791998429832 );

			arrayIndex = m_pIncidentData->m_vecPlayers.AddToTail();
			p = &m_pIncidentData->m_vecPlayers[ arrayIndex ];
			

			p->m_iClientIndex = -1;
			p->m_sPersona = "fletch";
			p->m_steamID.SetFromUint64( 148618791998436114 );
			{
				AbuseIncidentData_t::PlayerImage_t img;
				img.m_eType = AbuseIncidentData_t::k_PlayerImageType_UGC;
				img.m_hUGCHandle = 6978249415967519;
				p->m_vecImages.AddToTail( img );
			}

			if ( !m_pIncidentData->m_bCanReportGameServer)
			{
				m_pIncidentData->m_adrGameServer.SetFromString( "123.45.67.89:27015", false );
				m_pIncidentData->m_steamIDGameServer = CSteamID( 12345, 0, GetUniverse(), k_EAccountTypeAnonGameServer );
				m_pIncidentData->m_bCanReportGameServer = true;
			}
		}
	#endif

	// Make sure there is at least one other person we could file a report against!
	if ( m_pIncidentData->m_vecPlayers.Count() < 1 )
	{
		Warning( "No players to accuse of abuse, cannot file report\n" );
		return false;
	}

	return true;
}

void CAbuseReportManager::DestroyIncidentData()
{
	if ( m_pIncidentData != NULL )
	{
		delete m_pIncidentData;
		m_pIncidentData = NULL;
	}
	m_eIncidentDataStatus = k_EIncidentDataStatus_None;

	// Get rid of any existing screenshot file, both locally
	// and in the cloud.  We don't want this to count against
	// our quota
	if ( steamapicontext && steamapicontext->SteamRemoteStorage() && steamapicontext->SteamRemoteStorage()->FileExists( k_rchScreenShotFilename ) )
	{
		steamapicontext->SteamRemoteStorage()->FileDelete( k_rchScreenShotFilename );
	}

	if ( g_pFullFileSystem->FileExists( k_rchScreenShotFilename ) ) // !KLUDGE! To prevent warning if the file doesn't exist!
	{
		g_pFullFileSystem->RemoveFile( k_rchScreenShotFilename );
	}

	m_timeLastReportReadyNotification = 0.0;

	// Make sure we don't have any notifications queued
	NotificationQueue_Remove( &CEconNotification_AbuseReportReady::IsNotificationType );
}

void CAbuseReportManager::QueueReport()
{
	// Dialog is already active?
	if ( g_AbuseReportDlg.Get() != NULL )
	{
		Warning( "Cannot capture another incident report.  Submission dialog is active.\n" );
		return;
	}

	// Destroy any existing data
	DestroyIncidentData();

	// Make sure we're logged on to Steam
	if ( !IsLoggedOnToSteam() )
	{
		g_AbuseReportMgr->ShowNoSteamErrorMessage();
		return;
	}

	if ( CreateAndPopulateIncident() )
	{
		Msg( "Captured data for abuse report.\n");
	}
	else
	{
		Warning( "Failed to captured data for abuse report.\n");
		DestroyIncidentData();
	}
}

void CAbuseReportManager::ShowNoSteamErrorMessage()
{
	ShowMessageBox( "#AbuseReport_NoSteamTitle", "#AbuseReport_NoSteamMessage", "#GameUI_OK" );
}

void CAbuseReportManager::CheckCreateReportReadyNotification( float flMinSecondsSinceLastNotification, bool bInGame, float flLifetime )
{
	// We have to have some data ready
	if ( m_pIncidentData == NULL || m_eIncidentDataStatus != k_EIncidentDataStatus_Ready )
	{
		return;
	}

	// Don't pester them if they are already trying to do something about it
	if ( g_AbuseReportDlg.Get() != NULL || m_bReportUIPending )
	{
		return;
	}

	// Already notified them too recently?
	if ( m_timeLastReportReadyNotification != 0.0 && Plat_FloatTime() < m_timeLastReportReadyNotification + flMinSecondsSinceLastNotification )
	{
		return;
	}

	// Already a notification in the queue?
	if ( bInGame )
	{
		if ( NotificationQueue_Count( &CEconNotification_AbuseReportReady::IsInGameNotificationType ) > 0 )
		{
			return;
		}
	}
	else
	{
		if ( NotificationQueue_Count( &CEconNotification_AbuseReportReady::IsNotificationType ) > 0 )
		{
			return;
		}
	}

	CreateReportReadyNotification( bInGame, flLifetime );
}

void CAbuseReportManager::CreateReportReadyNotification( bool bInGame, float flLifetime )
{
	NotificationQueue_Remove( &CEconNotification_AbuseReportReady::IsNotificationType );
	CEconNotification_AbuseReportReady *pNotification = new CEconNotification_AbuseReportReady();
	pNotification->SetText( "AbuseReport_Notification" );
	pNotification->SetLifetime( flLifetime );
	pNotification->m_bShowInGame = bInGame;
	NotificationQueue_Add( pNotification );

	m_timeLastReportReadyNotification = Plat_FloatTime();
}

CON_COMMAND_F( abuse_report_queue, "Capture data for abuse report and queue for submission.  Use abose_report_submit to activate UI to submit the report", FCVAR_DONTRECORD )
{
	if ( !g_AbuseReportMgr )
	{
		Warning( "abuse_report_queue: No abuse report manager, cannot create report.\n" );
		return;
	}

	g_AbuseReportMgr->QueueReport();
}

CON_COMMAND_F( abuse_report_submit, "Activate UI to submit queued report.  Use abuse_report_queue to capture data for the report the report", FCVAR_DONTRECORD )
{
	if ( !g_AbuseReportMgr )
	{
		Warning( "abuse_report_submit: No abuse report manager, cannot submit report.\n" );
		return;
	}

	// Make sure we're logged on to Steam
	if ( !IsLoggedOnToSteam() )
	{
		g_AbuseReportMgr->ShowNoSteamErrorMessage();
		return;
	}

	if ( g_AbuseReportDlg.Get() != NULL )
	{
		// Dialog is already active
		return;
	}
	g_AbuseReportMgr->SubmitReportUIRequested();
}

// Test harness
#ifdef _DEBUG

CON_COMMAND_F( abuse_report_test, "Make a test abuse incident and activate UI", FCVAR_DONTRECORD )
{
	if ( !g_AbuseReportMgr )
	{
		Assert( g_AbuseReportMgr );
		return;
	}
	g_AbuseReportMgr->m_bTestReport = true;
	g_AbuseReportMgr->QueueReport();
	g_AbuseReportMgr->m_bTestReport = false;
	engine->ClientCmd_Unrestricted( "abuse_report_submit" );
}

#endif
