//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Creates a HTML control
//
// $NoKeywords: $
//=============================================================================//
#include "winlite.h"
#include "html_chrome.h"
#include "tier1/interface.h"
#include "reliabletimer.h"
#include "htmlmanager.h"

#include "html/htmlprotobuf.h"
#include <html/ichromehtmlwrapper.h>

//-----------------------------------------------------------------------------
// Purpose: owner object that gets responses from the CEF thread and dispatches them
//-----------------------------------------------------------------------------
class CHTMLController : public IHTMLChromeController
{
public:
	CHTMLController() { m_BrowserSerial = 0; m_nCefTargetFrameRate = 0; SetDefLessFunc( m_mapBrowserRequests ); SetDefLessFunc( m_mapBrowsers ); }
	~CHTMLController() {}

	bool Init( const char *pchHTMLCacheDir, const char *pchCookiePath );
	void Shutdown();

	void SetWebCookie( const char *pchHostname, const char *pchKey, const char *pchValue, const char *pchPath, RTime32 nExpires );
	void GetWebCookiesForURL( CUtlString *pstrValue, const char *pchURL, const char *pchName );
	void SetClientBuildID( uint64 ulBuildID );

	bool BHasPendingMessages();

	void CreateBrowser( IHTMLResponses *pBrowser, bool bPopupWindow, const char *pchUserAgentIdentifier );
	void RemoveBrowser( IHTMLResponses *pBrowser );
	bool RunFrame();

	void WakeThread() { AccessHTMLWrapper().WakeThread(); }
	HTMLCommandBuffer_t *GetFreeCommandBuffer( EHTMLCommands eCmd, int iBrowser ) { return AccessHTMLWrapper().GetFreeCommandBuffer( eCmd, iBrowser ); }
	void PushCommand( HTMLCommandBuffer_t *pCmd ) { AccessHTMLWrapper().PushCommand( pCmd ); }


	bool GetMainThreadCommand( HTMLCommandBuffer_t **pCmd ) { return AccessHTMLWrapper().GetMainThreadCommand( pCmd ); }
	void ReleaseCommandBuffer( HTMLCommandBuffer_t *pCmd ) { AccessHTMLWrapper().ReleaseCommandBuffer( pCmd ); }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName );
	bool ChromePrepareForValidate();
	bool ChromeResumeFromValidate();
#endif
	
	void SetCefThreadTargetFrameRate( uint32 nFPS );

private:
	// keeps track of outstanding browser create requests
	CUtlMap< uint32, IHTMLResponses *, int > m_mapBrowserRequests;
	// the next unique identifier to use when doing a browser create
	uint32 m_BrowserSerial;
	// the map of browser handles to our html panel objects, used for cef thread command dispatching
	CUtlMap< uint32, IHTMLResponses *, int > m_mapBrowsers;

	int m_nCefTargetFrameRate;
};


static CHTMLController s_HTMLController;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CHTMLController, IHTMLChromeController, CHROMEHTML_CONTROLLER_INTERFACE_VERSION, s_HTMLController );


//-----------------------------------------------------------------------------
// Purpose: request the cef thread to make a new browser
//-----------------------------------------------------------------------------
void CHTMLController::CreateBrowser( IHTMLResponses *pBrowser, bool bPopupWindow, const char *pchUserAgentIdentifier )
{
	m_BrowserSerial++;
	m_mapBrowserRequests.Insert( m_BrowserSerial, pBrowser );

	CHTMLProtoBufMsg<CMsgBrowserCreate> cmd( eHTMLCommands_BrowserCreate );
	cmd.Body().set_request_id( m_BrowserSerial );
	cmd.Body().set_popup( bPopupWindow );
	cmd.Body().set_useragent( pchUserAgentIdentifier );
	HTMLCommandBuffer_t *pBuf = AccessHTMLWrapper().GetFreeCommandBuffer( eHTMLCommands_BrowserCreate, -1 ); 
	cmd.SerializeCrossProc( &pBuf->m_Buffer ); 
	AccessHTMLWrapper().PushCommand( pBuf ); 
}


//-----------------------------------------------------------------------------
// Purpose: delete a browser we have
//-----------------------------------------------------------------------------
void CHTMLController::RemoveBrowser( IHTMLResponses *pBrowser )
{
	
	// pull ourselves from the browser handle list as we are doing away
	FOR_EACH_MAP_FAST( m_mapBrowsers, i)
	{
		if ( m_mapBrowsers[i] == pBrowser )
		{
			// tell the cef thread this browser is going away
			CHTMLProtoBufMsg<CMsgBrowserRemove> cmd( eHTMLCommands_BrowserRemove );
			cmd.Body().set_browser_handle( pBrowser->BrowserGetIndex() );
			HTMLCommandBuffer_t *pBuf = AccessHTMLWrapper().GetFreeCommandBuffer( eHTMLCommands_BrowserRemove, pBrowser->BrowserGetIndex() ); 
			cmd.SerializeCrossProc( &pBuf->m_Buffer ); 
			AccessHTMLWrapper().PushCommand( pBuf ); 

			// now kill it
			m_mapBrowsers.RemoveAt( i );
		}
	}

	// also remove us from pending list if in it
	FOR_EACH_MAP_FAST( m_mapBrowserRequests, i)
	{
		if ( m_mapBrowserRequests[i] == pBrowser )
			m_mapBrowserRequests.RemoveAt( i );
	}
}


//-----------------------------------------------------------------------------
// Purpose: turn on the cef engine
//-----------------------------------------------------------------------------
bool CHTMLController::Init( const char *pchHTMLCacheDir, const char *pchCookiePath )
{
#if !defined(WIN64) && !defined(STATIC_TIER0)
	ChromeInit( pchHTMLCacheDir, pchCookiePath );
#endif

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: shutdown chrome
//-----------------------------------------------------------------------------
void CHTMLController::Shutdown()
{
#if !defined(WIN64) && !defined(STATIC_TIER0)
	ChromeShutdown();
#endif
}



// helper macro to dispatch messages from the cef thread
#define HTML_MSG_FUNC( eHTMLCommand, bodyType, commandFunc ) \
	case eHTMLCommand: \
{ \
	CHTMLProtoBufMsg< bodyType > cmd( pCmd->m_eCmd ); \
	if ( !cmd.BDeserializeCrossProc( &pCmd->m_Buffer ) ) \
{ \
	bError = true; \
} \
	else \
{ \
	int idx = m_mapBrowsers.Find( cmd.BodyConst().browser_handle() ); \
	if ( idx != m_mapBrowsers.InvalidIndex() ) \
{ \
	if ( m_mapBrowsers[idx] ) \
	m_mapBrowsers[idx]->commandFunc( &cmd.BodyConst() ); \
} \
} \
} \
	break;  \


//-----------------------------------------------------------------------------
// Purpose: process any ipc responses we have pending
//-----------------------------------------------------------------------------
bool CHTMLController::RunFrame()
{
	VPROF_BUDGET( "CHTMLController::RunFrame", VPROF_BUDGETGROUP_TENFOOT );
	HTMLCommandBuffer_t *pCmd;
	bool bError = false;
	bool bDidwork = false;
    
    // Paint messages are dispatched last to avoid doing excessive work on
    // the main thread when two paint messages have stacked up in the queue.
    // This could be greatly optimized by doing atomic buffer swaps instead
    // of pushing the paint updates through a queue, but this helps for now.
    // -henryg
    CUtlVector< HTMLCommandBuffer_t * > vecDeferredPaint;
    
	while( GetMainThreadCommand( &pCmd ) )
	{
        bool bRelease = true;
		bDidwork = true;
		//Msg( "Got response %d\n", pCmd->m_eCmd );
		switch( pCmd->m_eCmd )
		{
		default:
			break;
		case eHTMLCommands_BrowserCreateResponse:
			{	
				CHTMLProtoBufMsg< CMsgBrowserCreateResponse > cmd( pCmd->m_eCmd );
				if ( !cmd.BDeserializeCrossProc( &pCmd->m_Buffer ) )
				{ 
					bError = true;
				}
				else
				{
					int idx = m_mapBrowserRequests.Find( cmd.BodyConst().request_id() );
					if ( idx != m_mapBrowserRequests.InvalidIndex() )
					{
						m_mapBrowsers.Insert( cmd.BodyConst().browser_handle(), m_mapBrowserRequests[idx] );
						m_mapBrowserRequests[idx]->BrowserSetIndex( cmd.BodyConst().browser_handle() );
						m_mapBrowserRequests.RemoveAt( idx );
					}
				}
			}
			break;
        case eHTMLCommands_NeedsPaint:
            {
                bRelease = false;
                vecDeferredPaint.AddToTail( pCmd );
            }
            break;

            HTML_MSG_FUNC( eHTMLCommands_BrowserReady, CMsgBrowserReady, BrowserReady );
			HTML_MSG_FUNC( eHTMLCommands_StartRequest, CMsgStartRequest, BrowserStartRequest );
			HTML_MSG_FUNC( eHTMLCommands_URLChanged, CMsgURLChanged, BrowserURLChanged );
			HTML_MSG_FUNC( eHTMLCommands_FinishedRequest, CMsgFinishedRequest, BrowserFinishedRequest );
			HTML_MSG_FUNC( eHTMLCommands_ShowPopup, CMsgShowPopup, BrowserShowPopup );
			HTML_MSG_FUNC( eHTMLCommands_HidePopup, CMsgHidePopup, BrowserHidePopup );
			HTML_MSG_FUNC( eHTMLCommands_OpenNewTab, CMsgOpenNewTab, BrowserOpenNewTab );
			HTML_MSG_FUNC( eHTMLCommands_PopupHTMLWindow, CMsgPopupHTMLWindow, BrowserPopupHTMLWindow );
			HTML_MSG_FUNC( eHTMLCommands_SetHTMLTitle, CMsgSetHTMLTitle, BrowserSetHTMLTitle );
			HTML_MSG_FUNC( eHTMLCommands_LoadingResource, CMsgLoadingResource, BrowserLoadingResource );
			HTML_MSG_FUNC( eHTMLCommands_StatusText, CMsgStatusText, BrowserStatusText );
			HTML_MSG_FUNC( eHTMLCommands_SetCursor, CMsgSetCursor, BrowserSetCursor );
			HTML_MSG_FUNC( eHTMLCommands_FileLoadDialog, CMsgFileLoadDialog, BrowserFileLoadDialog );
			HTML_MSG_FUNC( eHTMLCommands_ShowToolTip, CMsgShowToolTip, BrowserShowToolTip );
			HTML_MSG_FUNC( eHTMLCommands_UpdateToolTip, CMsgUpdateToolTip, BrowserUpdateToolTip );
			HTML_MSG_FUNC( eHTMLCommands_HideToolTip, CMsgHideToolTip, BrowserHideToolTip );
			HTML_MSG_FUNC( eHTMLCommands_SearchResults, CMsgSearchResults, BrowserSearchResults );
			HTML_MSG_FUNC( eHTMLCommands_Close, CMsgClose, BrowserClose );
			HTML_MSG_FUNC( eHTMLCommands_GetZoomResponse, CMsgGetZoomResponse, BrowserGetZoomResponse );
			HTML_MSG_FUNC( eHTMLCommands_HorizontalScrollBarSizeResponse, CMsgHorizontalScrollBarSizeResponse, BrowserHorizontalScrollBarSizeResponse );
			HTML_MSG_FUNC( eHTMLCommands_VerticalScrollBarSizeResponse, CMsgVerticalScrollBarSizeResponse, BrowserVerticalScrollBarSizeResponse );
			HTML_MSG_FUNC( eHTMLCommands_LinkAtPositionResponse, CMsgLinkAtPositionResponse, BrowserLinkAtPositionResponse );
			HTML_MSG_FUNC( eHTMLCommands_ZoomToElementAtPositionResponse, CMsgZoomToElementAtPositionResponse, BrowserZoomToElementAtPositionResponse );
			HTML_MSG_FUNC( eHTMLCommands_JSAlert, CMsgJSAlert, BrowserJSAlert );
			HTML_MSG_FUNC( eHTMLCommands_JSConfirm, CMsgJSConfirm, BrowserJSConfirm );
			HTML_MSG_FUNC( eHTMLCommands_OpenSteamURL, CMsgOpenSteamURL, BrowserOpenSteamURL );
			HTML_MSG_FUNC( eHTMLCommands_CanGoBackandForward, CMsgCanGoBackAndForward, BrowserCanGoBackandForward );
			HTML_MSG_FUNC( eHTMLCommands_SizePopup, CMsgSizePopup, BrowserSizePopup );
			HTML_MSG_FUNC( eHTMLCommands_ScaleToValueResponse, CMsgScalePageToValueResponse, BrowserScalePageToValueResponse );
			HTML_MSG_FUNC( eHTMLCommands_RequestFullScreen, CMsgRequestFullScreen, BrowserRequestFullScreen );
			HTML_MSG_FUNC( eHTMLCommands_ExitFullScreen, CMsgExitFullScreen, BrowserExitFullScreen );
			HTML_MSG_FUNC( eHTMLCommands_GetCookiesForURLResponse, CMsgGetCookiesForURLResponse, BrowserGetCookiesForURLResponse );
			HTML_MSG_FUNC( eHTMLCommands_NodeGotFocus, CMsgNodeHasFocus, BrowserNodeGotFocus );
			HTML_MSG_FUNC( eHTMLCommands_SavePageToJPEGResponse, CMsgSavePageToJPEGResponse, BrowserSavePageToJPEGResponse );
			HTML_MSG_FUNC( eHTMLCommands_GetFocusedNodeValueResponse, CMsgFocusedNodeTextResponse, BrowserFocusedNodeValueResponse );
		}
		if ( bError )
		{
			Warning( "Failed to parse command %d", pCmd->m_eCmd );
			Assert( !"Bad Command" );
		}
        if ( bRelease )
        {
            ReleaseCommandBuffer( pCmd );
        }
	}
    
    // Collapse deferred paints by browser ID and process them; the latest texture always
    // has fully updated bits, we simply union its dirty rect with the skipped updates.
    // Note: browser resizes always include a full dirty rect, we don't have to check here.
    while ( vecDeferredPaint.Count() )
    {
        // Pull the last paint off the queue
        pCmd = vecDeferredPaint[ vecDeferredPaint.Count() - 1 ];
        int iBrowser = pCmd->m_iBrowser;
        CHTMLProtoBufMsg<CMsgNeedsPaint> cmd( eHTMLCommands_NeedsPaint );
        DbgVerify( cmd.BDeserializeCrossProc( &pCmd->m_Buffer ) );
        ReleaseCommandBuffer( pCmd );
        vecDeferredPaint.Remove( vecDeferredPaint.Count() - 1 );

        
        CMsgNeedsPaint &body = cmd.Body();        
        CChromeUpdateRegion region;
        if ( body.updatewide() && body.updatetall() )
        {
            region.MarkDirtyRect( body.updatex(), body.updatey(), body.updatex() + body.updatewide(), body.updatey() + body.updatetall() );
        }
        else
        {
            region.MarkAllDirty();
        }

        // Remove earlier paints for the same browser from the queue
        for ( int i = vecDeferredPaint.Count() - 1; i >= 0; --i )
        {
            if ( vecDeferredPaint[i]->m_iBrowser == iBrowser )
            {
                // Decode
                CHTMLProtoBufMsg<CMsgNeedsPaint> cmdMerge( eHTMLCommands_NeedsPaint );
                DbgVerify( cmdMerge.BDeserializeCrossProc( &vecDeferredPaint[i]->m_Buffer ) );
                CMsgNeedsPaint &bodyMerge = cmdMerge.Body();
                
                if ( body.browser_handle() == bodyMerge.browser_handle() )
                {
                    ReleaseCommandBuffer( vecDeferredPaint[i] );
                    vecDeferredPaint.Remove( i );
                    
                    // Merge update region
                    if ( bodyMerge.updatewide() && bodyMerge.updatetall() )
                    {
                        region.MarkDirtyRect( bodyMerge.updatex(), bodyMerge.updatey(), bodyMerge.updatex() + bodyMerge.updatewide(), bodyMerge.updatey() + bodyMerge.updatetall() );
                    }
                    else
                    {
                        region.MarkAllDirty();
                    }
                    
                    // Send response to the skipped paint update to free up the texture slot
                    pCmd = GetFreeCommandBuffer( eHTMLCommands_NeedsPaintResponse, bodyMerge.browser_handle() );
                    CHTMLProtoBufMsg<CMsgNeedsPaintResponse> cmdResponse( eHTMLCommands_NeedsPaintResponse );
                    cmdResponse.Body().set_browser_handle( bodyMerge.browser_handle() );
                    cmdResponse.Body().set_textureid( bodyMerge.textureid() );
                    cmdResponse.SerializeCrossProc( &pCmd->m_Buffer );
                    PushCommand( pCmd );
                }
            }
        }

        // Dispatch the merged update
        int idxBrowser = m_mapBrowsers.Find( body.browser_handle() );
        if ( idxBrowser != m_mapBrowsers.InvalidIndex() )
        {
            if ( m_mapBrowsers[idxBrowser] )
            {
                int updateWide = region.GetUpdateWide( body.wide() );
                int updateTall = region.GetUpdateTall( body.tall() );
                if ( updateWide != body.wide() || updateTall != body.tall() )
                {
                    body.set_updatex( region.GetUpdateX( body.wide() ) );
                    body.set_updatey( region.GetUpdateY( body.tall() ) );
                    body.set_updatewide( updateWide );
                    body.set_updatetall( updateTall );
                }
                else
                {
                    body.clear_updatex();
                    body.clear_updatey();
                    body.clear_updatewide();
                    body.clear_updatetall();
                }
                m_mapBrowsers[idxBrowser]->BrowserNeedsPaint( &body );
            }
        }
    }

	return bDidwork;
}

//-----------------------------------------------------------------------------
// Purpose: set a cef cookie
//-----------------------------------------------------------------------------
void CHTMLController::SetWebCookie( const char *pchHostname, const char *pchKey, const char *pchValue, const char *pchPath, RTime32 nExpires )
{
#if !defined(WIN64) && !defined(STATIC_TIER0)
	ChromeSetWebCookie( pchHostname, pchKey, pchValue, pchPath, nExpires );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: set the buildid to report
//-----------------------------------------------------------------------------
void CHTMLController::SetClientBuildID( uint64 ulBuildID )
{
#if !defined(WIN64) && !defined(STATIC_TIER0)
	ChromeSetClientBuildID( ulBuildID );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: get the cef cookies for a url
//-----------------------------------------------------------------------------
void CHTMLController::GetWebCookiesForURL( CUtlString *pstrValue, const char *pchURL, const char *pchName )
{
#if !defined(WIN64) && !defined(STATIC_TIER0)
	ChromeGetWebCookiesForURL( pstrValue, pchURL, pchName );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: true if any pending html message in the queue
//-----------------------------------------------------------------------------
bool CHTMLController::BHasPendingMessages()
{
	return AccessHTMLWrapper().BHasPendingMessages();
}


//-----------------------------------------------------------------------------
// Purpose: tell the cef thread the frame rate to use if it changes
//-----------------------------------------------------------------------------
void CHTMLController::SetCefThreadTargetFrameRate( uint32 nFPS )
{
	if ( nFPS != m_nCefTargetFrameRate )
	{
		m_nCefTargetFrameRate = nFPS;
		CHTMLProtoBufMsg<CMsgSetTargetFrameRate> cmd( eHTMLCommands_SetTargetFrameRate );
		cmd.Body().set_ntargetframerate( nFPS );
		HTMLCommandBuffer_t *pBuf = AccessHTMLWrapper().GetFreeCommandBuffer( eHTMLCommands_SetTargetFrameRate, -1 ); 
		cmd.SerializeCrossProc( &pBuf->m_Buffer ); 
		AccessHTMLWrapper().PushCommand( pBuf ); 
	}
}

#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: validate mem
//-----------------------------------------------------------------------------
void CHTMLController::Validate( CValidator &validator, const char *pchName )
{
	ChromeValidate( validator, "ChromeValidate" );

	validator.Push( "CHTMLController::ValidateStatics", NULL, pchName );
	ValidateObj( m_mapBrowserRequests );
	ValidateObj( m_mapBrowsers );
	validator.Pop();
}

bool CHTMLController::ChromeResumeFromValidate()
{
	return ::ChromeResumeFromValidate();
}

bool CHTMLController::ChromePrepareForValidate()
{
	return ::ChromePrepareForValidate();
}
#endif	// DBGFLAG_VALIDATE



