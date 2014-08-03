//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replay/cdll_replay.h"
#include "replay/replaycamera.h"
#include "replay/replay_ragdoll.h"
#include "replay/iclientreplay.h"
#include "replay/ireplayscreenshotsystem.h"
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/vgui/replayconfirmquitdlg.h"
#include "replay/vgui/replaybrowsermainpanel.h"
#include "replay/vgui/replayinputpanel.h"
#include "replay/vgui/replayperformanceeditor.h"
#include "replayperformanceplaybackhandler.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "clientmode.h"
#include "iviewrender.h"
#include "igameevents.h"
#include "replaycamera.h"
#if defined( TF_CLIENT_DLL )
#include "c_tf_gamestats.h"
#endif
#include "steamworks_gamestats.h"
#include "replay_gamestats_shared.h"

//----------------------------------------------------------------------------------------

class IReplayScreenshotSystem;

//----------------------------------------------------------------------------------------

extern void ReplayUI_OpenReplayRenderOverlay();
extern void ReplayUI_HideRenderOverlay();

//----------------------------------------------------------------------------------------

extern IReplayMovieManager *g_pReplayMovieManager;

//----------------------------------------------------------------------------------------

class CClientReplayImp : public IClientReplay
{
public:
	virtual uint64 GetServerSessionId()
	{
		return GetSteamWorksSGameStatsUploader().GetServerSessionID();
	}
	
	virtual IReplayScreenshotSystem *GetReplayScreenshotSystem()
	{
		if ( g_pEngineReplay->IsSupportedModAndPlatform() )
			return view->GetReplayScreenshotSystem();
		return NULL;
	}

	virtual IReplayPerformancePlaybackHandler *GetPerformancePlaybackHandler()
	{
		return g_pReplayPerformancePlaybackHandler;
	}

	virtual bool CacheReplayRagdolls( const char* pFilename, int nStartTick )
	{
		return Replay_CacheRagdolls( pFilename, nStartTick );
	}

	virtual void OnSaveReplay( ReplayHandle_t hNewReplay, bool bShowInputDlg )
	{
		if ( bShowInputDlg )
		{
			// Get a name for the replay, saves to disk, add thumbnail to replay browser
			ShowReplayInputPanel( hNewReplay );
		}
		else
		{
			// Just add the thumbnail if the replay browser exists
			CReplayBrowserPanel* pReplayBrowser = ReplayUI_GetBrowserPanel();
			if ( pReplayBrowser )
			{
				pReplayBrowser->OnSaveReplay( hNewReplay );
			}
		}

		// Fire a message the game DLL can intercept (for achievements, etc).
		IGameEvent *event = gameeventmanager->CreateEvent( "replay_saved" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	virtual void OnDeleteReplay( ReplayHandle_t hReplay )
	{
		CReplayBrowserPanel* pReplayBrowser = ReplayUI_GetBrowserPanel();
		if ( pReplayBrowser )
		{
			pReplayBrowser->OnDeleteReplay( hReplay );
		}
	}

	virtual void DisplayReplayMessage( const char *pLocalizeStr, bool bUrgent, bool bDlg, const char *pSound )
	{
		// Display a message?
		if ( !pLocalizeStr || !pLocalizeStr[0] )
			return;

		g_pClientMode->DisplayReplayMessage( pLocalizeStr, -1.0f, bUrgent, pSound, bDlg );
	}

	virtual void DisplayReplayMessage( const wchar_t *pText, bool bUrgent, bool bDlg, const char *pSound )
	{
		if ( !pText || !pText[0] )
			return;
	
		const int nLen = wcslen( pText ) + 1;
		char *pAnsi = new char[ nLen ];
		g_pVGuiLocalize->ConvertUnicodeToANSI( pText, pAnsi, nLen );

		g_pClientMode->DisplayReplayMessage( pAnsi, -1.0f, bUrgent, pSound, bDlg );
	}

	virtual bool OnConfirmQuit()
	{
		return ReplayUI_ShowConfirmQuitDlg();
	}

	virtual void OnRenderStart()
	{
		ReplayUI_OpenReplayRenderOverlay();
	}

	virtual void OnRenderComplete( const RenderMovieParams_t &RenderParams, bool bCancelled, bool bSuccess, bool bShowBrowser )
	{
		ReplayUI_HideRenderOverlay();

		if ( bShowBrowser )
		{
			ReplayUI_ReloadBrowser();
		}

		// Upload a row to the OGS now that rendering has completed
		GetReplayGameStatsHelper().SW_ReplayStats_WriteRenderDataEnd( RenderParams, bCancelled ? "cancelled" : bSuccess ? "success" : "failed" );
	}

	virtual void InitPerformanceEditor( ReplayHandle_t hReplay )
	{
		ReplayUI_InitPerformanceEditor( hReplay );
	}

	virtual void HidePerformanceEditor()
	{
		ReplayUI_ClosePerformanceEditor();
	}

	virtual bool ShouldRender()
	{
		extern ConVar replay_enablerenderpreview;
		return !g_pReplayMovieManager->IsRendering() || replay_enablerenderpreview.GetBool();
	}

	virtual void PlaySound( const char *pSound )
	{
		if ( g_pVGuiSurface )
		{
			g_pVGuiSurface->PlaySound( pSound );
		}
	}

	virtual void UploadOgsData( KeyValues *pData, bool bIncludeTimeField )
	{
		GetReplayGameStatsHelper().UploadError( pData, bIncludeTimeField );
	}

	virtual bool ShouldCompletePendingReplay( IGameEvent *pEvent )
	{
#if defined( TF_CLIENT_DLL )
		return !( pEvent->GetInt( "death_flags" ) & TF_DEATH_FEIGN_DEATH );
#else
		return true;
#endif
	}

	virtual void OnPlaybackComplete( ReplayHandle_t hReplay, int iPerformance )
	{
		ReplayUI_ReloadBrowser( hReplay, iPerformance );
	}

	virtual IReplayCamera *GetReplayCamera()
	{
		return ReplayCamera();
	}

	virtual bool OnEndOfReplayReached()
	{
		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( !pEditor )
			return false;

		return pEditor->OnEndOfReplayReached();
	}
};

static CClientReplayImp s_ClientReplayImp;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CClientReplayImp, IClientReplay, CLIENT_REPLAY_INTERFACE_VERSION, s_ClientReplayImp );

#endif	// #if defined( REPLAY_ENABLED )