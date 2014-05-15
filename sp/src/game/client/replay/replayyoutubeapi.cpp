//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayyoutubeapi.h"

#include "tier1/KeyValues.h"

#include "replay/ireplaymovie.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/genericclassbased_replay.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/ScrollBar.h"

#include "confirm_dialog.h"
#include "replay/vgui/replaybrowserdetailspanel.h"

#include "base_gcmessages.pb.h"

#include "youtubeapi.h"
#include "steamworks_gamestats.h"
#include "replayvideo.h"

#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

static ConVar youtube_username( "youtube_username", "", FCVAR_ARCHIVE, "Username for YouTube." );

//-----------------------------------------------------------------------------

extern IReplayMovieManager *g_pReplayMovieManager;
extern const char *COM_GetModDirectory();
extern void GetYouTubeAPIKey( const char *pGameDir, bool bOnSteamPublic, const char **ppSource, const char **ppDeveloperTag, const char **ppDeveloperKey );



//-----------------------------------------------------------------------------

static bool HasInvalidCharacters( const char *pString )
{
	while ( *pString != 0 )
	{
		switch ( *pString )
		{
		case '<': return true;
		case '>': return true;
		case '&': return true;
		}
		++pString;
	}
	return false;
}

//-----------------------------------------------------------------------------

void UploadOgsData( IReplayMovie *pMovie, bool bEnded = false, const char *pEndReason = NULL )
{
	static int s_nUploadCounter = 0;

	KeyValues* pKVData = new KeyValues( "TF2ReplayUploads" );

	pKVData->SetInt( "UploadCounter", s_nUploadCounter++ );
	pKVData->SetInt( "ReplayHandle", (int)pMovie->GetReplayHandle() );

	const ReplayRenderSettings_t &RenderSettings = pMovie->GetRenderSettings();
	CFmtStr fmtResolution( "%ix%i", RenderSettings.m_nWidth, RenderSettings.m_nHeight );
	pKVData->SetString( "ResolutionID", fmtResolution.Access() );

	pKVData->SetString( "CodecID", ReplayVideo_GetCodec( ReplayVideo_FindCodecPresetFromCodec( RenderSettings.m_Codec ) ).m_pName );
	pKVData->SetInt( "MotionBlurQuality", RenderSettings.m_nMotionBlurQuality );
	pKVData->SetInt( "RenderQuality", RenderSettings.m_nEncodingQuality );
	pKVData->SetInt( "FPSUPF", RenderSettings.m_FPS.GetUnitsPerFrame() );
	pKVData->SetInt( "FPSUPS", RenderSettings.m_FPS.GetUnitsPerSecond() );

	if ( bEnded )
	{
		pKVData->SetInt( "EndUploadTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
		pKVData->SetString( "EndUploadReasonID", pEndReason );
	}
	else
	{
		pKVData->SetInt( "StartUploadTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );
	}

	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
}

//-----------------------------------------------------------------------------

class CYouTubeLoginWaitDialog : public CGenericWaitingDialog
{
	DECLARE_CLASS_SIMPLE( CYouTubeLoginWaitDialog, CGenericWaitingDialog );
public:
	CYouTubeLoginWaitDialog( IReplayMovie *pMovie, CConfirmDialog *pLoginDialog ) 
		: CGenericWaitingDialog( pLoginDialog->GetParent() )
		, m_pMovie( pMovie )
		, m_pLoginDialog( pLoginDialog )
	{
	}

	virtual void OnUserClose()
	{
		BaseClass::OnUserClose();
		YouTube_LoginCancel();
		ShowMessageBox( "#YouTube_LoginResults_Title", "#YouTube_LoginResults_Cancel", "#GameUI_OK" );
	}

	virtual void OnTick()
	{
		BaseClass::OnTick();

		eYouTubeLoginStatus loginStatus = YouTube_GetLoginStatus();
		switch ( loginStatus )
		{
		case kYouTubeLogin_NotLoggedIn:
			break;
		case kYouTubeLogin_LoggedIn:
			Close();
			PostMessage( m_pLoginDialog, new KeyValues("Command", "command", "cancel" ), NULL);
			YouTube_ShowUploadDialog( m_pMovie, m_pLoginDialog->GetParent() );
			break;
		case kYouTubeLogin_CouldNotConnect:
			Close();
			ShowMessageBox( "#YouTube_LoginResults_Title", "#YouTube_LoginResults_CouldNotConnect", "#GameUI_OK" );
			break;
		case kYouTubeLogin_Forbidden:
			Close();
			ShowMessageBox( "#YouTube_LoginResults_Title", "#YouTube_LoginResults_Forbidden", "#GameUI_OK" );
			break;
		case kYouTubeLogin_GenericFailure:
		default:
			Close();
			ShowMessageBox( "#YouTube_LoginResults_Title", "#YouTube_LoginResults_Failure", "#GameUI_OK" );
			break;
		}
	}

private:
	IReplayMovie *m_pMovie;
	CConfirmDialog *m_pLoginDialog;
};

class CYouTubeUploadWaitDialog : public CGenericWaitingDialog
{
	DECLARE_CLASS_SIMPLE( CYouTubeUploadWaitDialog, CGenericWaitingDialog );
public:
	CYouTubeUploadWaitDialog( IReplayMovie *pMovie, const char *pTitle, const char *pDescription, YouTubeUploadHandle_t handle, vgui::Panel *pParent )  
		: CGenericWaitingDialog( pParent )
		, m_pMovie( pMovie )
		, m_strTitle( pTitle )
		, m_strDescription( pDescription )
		, m_uploadHandle( handle )
		, m_iTick( 0 )
	{
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		m_pProgressBar = dynamic_cast< ProgressBar * >( FindChildByName( "Progress" ) );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		if ( m_pProgressBar )
		{
			m_pProgressBar->SetVisible( true );
			m_pProgressBar->SetSegmentInfo( XRES(1), XRES(5) );
		}
	}

	virtual const char* GetResFile() const { return "resource/UI/YouTubeUploadWaitingDialog.res"; }
	virtual const char *GetResFilePathId() const { return "GAME"; }

	virtual void OnUserClose()
	{
		BaseClass::OnUserClose();
		YouTube_CancelUpload( m_uploadHandle );
		UploadOgsData( m_pMovie, true, "cancelled" );
	}

	virtual void OnTick()
	{
		BaseClass::OnTick();

		double ultotal = 0.0;
		double ulnow = 0.0;
		if ( YouTube_GetUploadProgress( m_uploadHandle, ultotal, ulnow ) == false )
		{
			Close();
			ShowMessageBox( "#YouTube_Upload_Title", "#YouTube_Upload_Failure", "#GameUI_OK" );
		}
		else if ( YouTube_IsUploadFinished( m_uploadHandle ) )
		{
			bool bSuccess = true;
			CUtlString strURLToVideo;
			CUtlString strURLToVideoStats;
			if ( YouTube_GetUploadResults( m_uploadHandle, bSuccess, strURLToVideo, strURLToVideoStats ) && bSuccess )
			{
				// mark movie uploaded
				m_pMovie->SetUploaded( true );
				m_pMovie->SetUploadURL( strURLToVideoStats.Get() );
				g_pReplayMovieManager->FlagMovieForFlush( m_pMovie, true );
				// update the UI
				CReplayDetailsPanel *pDetailsPanel = dynamic_cast< CReplayDetailsPanel *>( GetParent() );
				if ( pDetailsPanel )
				{
					pDetailsPanel->InvalidateLayout( true, false );
				}		
				ShowMessageBox( "#YouTube_Upload_Title", "#YouTube_Upload_Success", "#GameUI_OK", NULL, GetParent() );

				// notify the GC
				uint64 uSessionId = g_pClientReplayContext->GetServerSessionId( m_pMovie->GetReplayHandle() );
				if ( uSessionId != 0 )
				{
					GCSDK::CProtoBufMsg< CMsgReplayUploadedToYouTube > msg( k_EMsgGCReplay_UploadedToYouTube );
					msg.Body().set_youtube_url( strURLToVideoStats.Get() );
					msg.Body().set_youtube_account_name( YouTube_GetLoginName() );
					msg.Body().set_session_id( uSessionId );
					GCClientSystem()->BSendMessage( msg );
				}

				surface()->PlaySound( "replay\\youtube_uploadfinished.wav" );

				UploadOgsData( m_pMovie, true, "completed" );

				// share on Steam Community
				if ( steamapicontext && steamapicontext->SteamRemoteStorage() )
				{					
					CUtlString strPreviewFileName;
					AppId_t nConsumerAppId = steamapicontext->SteamUtils()->GetAppID();
					ERemoteStoragePublishedFileVisibility eVisibility = k_ERemoteStoragePublishedFileVisibilityPublic;
					SteamParamStringArray_t tags;
					tags.m_ppStrings = NULL;
					tags.m_nNumStrings = 0;

					// don't bother waiting for result
					SteamAPICall_t hSteamAPICall = steamapicontext->SteamRemoteStorage()->PublishVideo(
						k_EWorkshopVideoProviderNone, "", 
						strURLToVideo.Get(),
						strPreviewFileName.Get(), 
						nConsumerAppId, 
						m_strTitle.Get(), 
						m_strDescription.Get(), 
						eVisibility, 
						&tags
						);
					hSteamAPICall;
				}
			}
			else
			{
				ShowMessageBox( "#YouTube_Upload_Title", "#YouTube_Upload_Failure", "#GameUI_OK" );

				surface()->PlaySound( "replay\\youtube_uploadfailed.wav" );

				UploadOgsData( m_pMovie, true, "failed" );
			}
			// close wait dialog
			YouTube_ClearUploadResults( m_uploadHandle );
			Close();
		}
		else
		{
			float flProgress = MIN( ulnow / MAX( ultotal, 1.0 ), 1.0f );
			int iProgress = int( 100.0 * flProgress );
			int ulnow_kb = uint32( ulnow / 1024 );
			int ultotal_kb = uint32( ultotal / 1024 );

			if ( ulnow_kb == ultotal_kb )
			{
				// we tick at 500 ms, so this should be ok
				m_iTick = ( m_iTick + 1 ) % 4;
				switch ( m_iTick )
				{
				case 0:	SetDialogVariable( "duration", g_pVGuiLocalize->Find( "YouTube_UploadFinishing1" ) ); break;
				case 1: SetDialogVariable( "duration", g_pVGuiLocalize->Find( "YouTube_UploadFinishing2" ) ); break;
				case 2: SetDialogVariable( "duration", g_pVGuiLocalize->Find( "YouTube_UploadFinishing3" ) ); break;
				case 3: SetDialogVariable( "duration", g_pVGuiLocalize->Find( "YouTube_UploadFinishing4" ) ); break;
				}
			}
			else
			{
				wchar_t wszProgress[1024];
				wchar_t wszPercentage[32];
				wchar_t wszNow[32];
				wchar_t wszTotal[32];
				_snwprintf( wszPercentage, ARRAYSIZE( wszPercentage ), L"%u", iProgress );
				_snwprintf( wszNow, ARRAYSIZE( wszNow ), L"%u", ulnow_kb );
				_snwprintf( wszTotal, ARRAYSIZE( wszTotal ), L"%u", ultotal_kb );
				g_pVGuiLocalize->ConstructString( wszProgress,sizeof( wszProgress ), g_pVGuiLocalize->Find( "#YouTube_UploadProgress" ), 3, 
												  wszPercentage,
												  wszNow,
												  wszTotal );

				SetDialogVariable( "duration", wszProgress );
			}

			if ( m_pProgressBar )
			{
				m_pProgressBar->SetProgress( flProgress );
			}
		}
	}

private:
	IReplayMovie *m_pMovie;
	YouTubeUploadHandle_t m_uploadHandle;
	CUtlString m_strTitle;
	CUtlString m_strDescription;
	ProgressBar *m_pProgressBar;
	int m_iTick;
};

//-----------------------------------------------------------------------------
// Purpose: Dialog for logging into YouTube
//-----------------------------------------------------------------------------
class CYouTubeLoginDialog : public CConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CYouTubeLoginDialog, CConfirmDialog );
public:
	CYouTubeLoginDialog( IReplayMovie *pMovie, Panel *pParent ) : BaseClass( pParent ), m_pMovie( pMovie ) {}

	const wchar_t *GetText() { return NULL; }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		TextEntry *pTextEntryUserName = dynamic_cast< TextEntry * >( FindChildByName( "UserNameTextEntry" ) );
		if ( pTextEntryUserName )
		{
			pTextEntryUserName->SetText( "" );
			pTextEntryUserName->InsertString( youtube_username.GetString() );
		}
	}

	virtual void OnCommand( const char *command )
	{
		if ( !Q_strnicmp( command, "register", 8 ) )
		{
			if ( steamapicontext && steamapicontext->SteamFriends() )
			{
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.youtube.com/create_account?next=/" );
			}
		}		
		else if ( !Q_strnicmp( command, "confirm", 7 ) )
		{
			TextEntry *pTextEntryUserName = dynamic_cast< TextEntry * >( FindChildByName( "UserNameTextEntry" ) );
			TextEntry *pTextEntryPassword = dynamic_cast< TextEntry * >( FindChildByName( "PasswordTextEntry" ) );
			if ( pTextEntryUserName && pTextEntryPassword )
			{
				char szUserName[256];
				pTextEntryUserName->GetText( szUserName, sizeof( szUserName ) );
				char szPassword[256];
				pTextEntryPassword->GetText( szPassword, sizeof( szPassword ) );
				youtube_username.SetValue( szUserName );
				Login( szUserName, szPassword );
			}

			return;
		}
		BaseClass::OnCommand( command );
	}

protected:
	virtual const char *GetResFile() { return "Resource/UI/YouTubeLoginDialog.res"; }
	virtual const char *GetResFilePathId() { return "GAME"; }

	void Login( const char* pUserName, const char *pPassword )
	{
		const bool bOnSteamPublic = steamapicontext && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->GetConnectedUniverse() == k_EUniversePublic;
		const char *pGameDir = COM_GetModDirectory();
		const char *pSource = NULL;
		const char *pDeveloperTag = NULL;
		const char *pDeveloperKey = NULL;

		GetYouTubeAPIKey( pGameDir, bOnSteamPublic, &pSource, &pDeveloperTag, &pDeveloperKey );

		Assert( pSource );
		Assert( pDeveloperTag );
		Assert( pDeveloperKey );
		
		YouTube_SetDeveloperSettings( pDeveloperKey, pDeveloperTag );

		// try to log in
		YouTube_Login( pUserName, pPassword, pSource );
		
		CYouTubeLoginWaitDialog *pDialog = new CYouTubeLoginWaitDialog( m_pMovie, this );
		ShowWaitingDialog( pDialog, "#YouTube_LoggingIn", true, true, -1 );
	}

private:
	IReplayMovie *m_pMovie;
};

//-----------------------------------------------------------------------------
// Purpose: Dialog for uploading a file to YouTube
//-----------------------------------------------------------------------------
class CYouTubeUploadDialog : public CConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CYouTubeUploadDialog, CConfirmDialog );
public:
	CYouTubeUploadDialog( IReplayMovie *pMovie, Panel *pParent ) : BaseClass( pParent ), m_pMovie( pMovie ) {}

	const wchar_t *GetText() { return NULL; }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		m_pTextEntryMovieTitle = dynamic_cast< TextEntry * >( FindChildByName( "MovieTitleTextEntry" ) );
		m_pTextEntryMovieDesc = dynamic_cast< TextEntry * >( FindChildByName( "MovieDescTextEntry" ) );

		// use insert, so that max characters is obeyed
		m_pTextEntryMovieTitle->InsertString( m_pMovie->GetItemTitle() );
		// @todo add steam profile to description
		m_pTextEntryMovieDesc->SetText( "" );
		m_pTextEntryMovieDesc->SetMultiline( true );
		m_pTextEntryMovieDesc->SetCatchEnterKey( true );
		m_pTextEntryMovieDesc->SetVerticalScrollbar( true );
		ScrollBar *pScrollbar = dynamic_cast< ScrollBar* >( m_pTextEntryMovieDesc->FindChildByName( "Scrollbar" ) );
		if ( pScrollbar )
		{
			pScrollbar->SetAutohideButtons( false );
			pScrollbar->SetScrollbarButtonsVisible( true );
		}

		m_pUnlistedCheckbox = dynamic_cast< CheckButton * >( FindChildByName( "UnlistedCheckbox" ) );
		if ( m_pUnlistedCheckbox )
		{
			m_pUnlistedCheckbox->SetMouseInputEnabled( true );
		}
		
	}

	void GetGameNameStrings( const char **ppShortGameName, const char **ppFullGameName )
	{
		const char *pGameDir = COM_GetModDirectory();

		// Team Fortress 2?
		if ( FStrEq( pGameDir, "tf" ) )
		{
			*ppShortGameName = "TF2";
			*ppFullGameName = "Team Fortress 2";
		}
		// Team Fortress 2 Beta?
		else if ( FStrEq( pGameDir, "tf_beta" ) )
		{
			*ppShortGameName = "TF2";
			*ppFullGameName = "Team Fortress 2 Beta";
		}
		// Counter-Strike: Source?
		else if ( FStrEq( pGameDir, "cstrike" ) )
		{
			*ppShortGameName = "CSS";
			*ppFullGameName = "Counter-Strike: Source";
		}
		// Counter-Strike: Source Beta?
		else if ( FStrEq( pGameDir, "cstrike_beta" ) )
		{
			*ppShortGameName = "CSS";
			*ppFullGameName = "Counter-Strike: Source Beta";
		}
		else
		{
			AssertMsg( 0, "Unknown game" );
			*ppShortGameName = "";
			*ppFullGameName = "";
		}
	}

	virtual void OnCommand( const char *command )
	{
		if ( !Q_strnicmp( command, "termsofservice", 14 ) )
		{
			if ( steamapicontext && steamapicontext->SteamFriends() )
			{
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.youtube.com/t/terms" );
			}
		}		
		else if ( !Q_strnicmp( command, "confirm", 7 ) )
		{
			char szTitle[256];
			m_pTextEntryMovieTitle->GetText( szTitle, sizeof( szTitle ) );
			char szDesc[2048];
			m_pTextEntryMovieDesc->GetText( szDesc, sizeof( szDesc ) );

			if ( HasInvalidCharacters( szTitle ) )
			{
				ShowMessageBox( "#YouTube_Upload_Title", "#YouTube_Upload_InvalidChars_Title", "#GameUI_OK" );
				return;
			}
			if ( HasInvalidCharacters( szDesc ) )
			{
				ShowMessageBox( "#YouTube_Upload_Title", "#YouTube_Upload_InvalidChars_Desc", "#GameUI_OK" );
				return;
			}

			CGenericClassBasedReplay *pReplay = ToGenericClassBasedReplay( m_pMovie->GetItemReplay() );

			CFmtStr fmtMovieFullFilename( "%s%s", g_pReplayMovieManager->GetRenderDir(), m_pMovie->GetMovieFilename() );
			const char *pMimeType = "video/mp4";
			const char *pTitle = szTitle;
			const char *pCategory = "Games";

			// add steam profile to the description for verification if necessary
			EUniverse eSteamUniverse = steamapicontext && steamapicontext->SteamUtils() ? steamapicontext->SteamUtils()->GetConnectedUniverse() : k_EUniverseDev;
			CUtlString description( szDesc );
			if ( steamapicontext && steamapicontext->SteamUser() )
			{
				const char *pchCommunityURL = "http://steamcommunity.com/";
				switch ( eSteamUniverse )
				{
				case k_EUniverseDev:
					pchCommunityURL = "http://localhost/community/";
					break;
				case k_EUniverseBeta:
					pchCommunityURL = "http://beta.steamcommunity.com/";
					break;
				case k_EUniversePublic:
				default:
					pchCommunityURL = "http://steamcommunity.com/";
				}
				description.Format( "%s\n\n%sprofiles/%llu", szDesc, pchCommunityURL, steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );
			}

			const char *pShortGameName = NULL;
			const char *pFullGameName = NULL;

			GetGameNameStrings( &pShortGameName, &pFullGameName );

			CFmtStr1024 keywords( "%s Replay, %s, %s, Replay, Valve, %s", pShortGameName, pShortGameName, pFullGameName, pReplay->GetPlayerClass() );
			bool bUnlisted = m_pUnlistedCheckbox->IsSelected();

			uint64 uSessionId = g_pClientReplayContext->GetServerSessionId( pReplay->GetHandle() );
			if ( uSessionId != 0 )
			{
				char szSessionId[32];

				// Write session ID as hex (this code taken from KeyValues.cpp, modified).
#ifdef WIN32
				Q_snprintf( szSessionId, sizeof( szSessionId ), "%I64X", uSessionId );
#else
				Q_snprintf( szSessionId, sizeof( szSessionId ), "%llX", uSessionId );
#endif

				// Add the match tag to the list of keywords.
				keywords.AppendFormat( ", match_%s", szSessionId );
			}

			UploadOgsData( m_pMovie );

			YouTubeUploadHandle_t handle = YouTube_Upload( fmtMovieFullFilename.Access(), pMimeType, pTitle, description.Get(), pCategory, keywords.Access(), bUnlisted ? kYouTubeAccessControl_Unlisted : kYouTubeAccessControl_Public );
			if ( handle != NULL )
			{
				// Play a sound
				surface()->PlaySound( "replay\\youtube_startingupload.wav" );

				CYouTubeUploadWaitDialog *pDialog = new CYouTubeUploadWaitDialog( m_pMovie, pTitle, description.Get(), handle, GetParent() );
				ShowWaitingDialog( pDialog, "#YouTube_Uploading", true, true, -1 );

				// get rid of this dialog
				FinishUp();
			}
			else
			{
				ShowMessageBox( "#YouTube_Upload_Title", "#YouTube_Upload_Failure", "#GameUI_OK" );
			}

			return;
		}
		BaseClass::OnCommand( command );
	}

protected:
	virtual const char *GetResFile() { return "Resource/UI/YouTubeUploadDialog.res"; }
	virtual const char *GetResFilePathId() { return "GAME"; }

private:
	IReplayMovie *m_pMovie;
	TextEntry *m_pTextEntryMovieTitle;
	TextEntry *m_pTextEntryMovieDesc;
	CheckButton *m_pUnlistedCheckbox;
};

//-----------------------------------------------------------------------------

void YouTube_ShowLoginDialog( IReplayMovie *pMovie, vgui::Panel *pParent )
{
	CYouTubeLoginDialog *pDialog = vgui::SETUP_PANEL( new CYouTubeLoginDialog( pMovie, pParent ) );
	pDialog->Show( false );
}

//-----------------------------------------------------------------------------

void YouTube_ShowUploadDialog( IReplayMovie *pMovie, vgui::Panel *pParent )
{
	CYouTubeUploadDialog *pDialog = vgui::SETUP_PANEL( new CYouTubeUploadDialog( pMovie, pParent ) );
	pDialog->Show( false );
}

//-----------------------------------------------------------------------------

#endif // REPLAY_ENABLED
