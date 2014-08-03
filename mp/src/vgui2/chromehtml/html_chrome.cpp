//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//=============================================================================//

#include "winlite.h"
#include "html_chrome.h"
#include "cef/include/cef_cookie.h"
#include "imageloadwrapper.h"
#include "html/ichromehtmlwrapper.h"
#include "input/mousecursors.h"
//#include "rtime.h"
#ifdef OSX
extern "C" void *CreateAutoReleasePool();
extern "C" void ReleaseAutoReleasePool( void *pool );
#endif
#ifdef WIN32
#include <TlHelp32.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static uint64 sm_ulBuildID = 0;	// will be appended to the user agent
static CCEFThread g_CEFThread; // main thread for CEF to do its thinking

// access for main thread to get to the html object
CCEFThread &AccessHTMLWrapper()
{
	return g_CEFThread;
}

// helper macro to send a command to the CEF thread
#define DISPATCH_MESSAGE( eCmd ) \
	cmd.Body().set_browser_handle( m_iBrowser ); \
	HTMLCommandBuffer_t *pBuf = g_CEFThread.GetFreeCommandBuffer( eCmd, m_iBrowser ); \
	cmd.SerializeCrossProc( &pBuf->m_Buffer ); \
	g_CEFThread.PushResponse( pBuf ); \

// wrappers for our handle to list index/serial, lower 16 bits are list index, top 16 bits are the serial number
#define BROWSER_HANDLE_FROM_INDEX_SERIAL( iList, nSerial ) iList + ((int)nSerial<<16)
#define BROWSER_SERIAL_FROM_HANDLE( nHandle ) (uint32)nHandle>>16

const int k_ScreenshotQuality = 95; // quality value used when making a jpeg image of a web page
const float k_flMaxScreenshotWaitTime = 5.0; // wait up to 5 sec before taking a screenshot

//-----------------------------------------------------------------------------
// Purpose: thread to manage pumping the CEF message loop
//-----------------------------------------------------------------------------
CCEFThread::CCEFThread() 
{
	m_bExit = false;
	m_nTargetFrameRate = 60; // 60 Hz by default
	m_nBrowserSerial = 0;
	m_bFullScreenFlashVisible = false;
	m_bSawUserInputThisFrame = false;
}


//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CCEFThread::~CCEFThread()
{
	// This can get called by the main thread before the actual thread exits,
	// if there is a forced process exit due to some error. In this case,
	// force the thread to exit before destroying the member variables it
	// is depending on.
	if ( !m_bExit )
	{
		TriggerShutdown();
		Join( 20 * k_nThousand );
	}
}


//-----------------------------------------------------------------------------
// Purpose: tell cef where to place its state dirs
//-----------------------------------------------------------------------------
void CCEFThread::SetCEFPaths( const char *pchHTMLCacheDir, const char *pchCookiePath )
{
	Assert( !IsAlive() );
	m_sHTMLCacheDir = pchHTMLCacheDir;
	m_sCookiePath = pchCookiePath;
}


//-----------------------------------------------------------------------------
// Purpose: set the exit event on the cef thread
//-----------------------------------------------------------------------------
void CCEFThread::TriggerShutdown() 
{
	m_bExit = true; 
	m_WakeEvent.Set();
	m_evWaitingForCommand.Set();
	m_eventDidExit.Wait( k_nThousand ); // wait 1 second at most for it to go away
}


//-----------------------------------------------------------------------------
// Purpose: tickle the run loop of the thread to run more commands
//-----------------------------------------------------------------------------
void CCEFThread::WakeThread() 
{ 
	m_WakeEvent.Set(); 
}


//-----------------------------------------------------------------------------
// Purpose: helper macro for message dispatch
//-----------------------------------------------------------------------------
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
			cmd.Body().set_browser_handle( pCmd->m_iBrowser ); \
	g_CEFThread.commandFunc( cmd ); \
} \
} \
	break; 


#define HTML_MSG_FUNC_NOHANDLE( eHTMLCommand, bodyType, commandFunc ) \
	case eHTMLCommand: \
		{ \
			CHTMLProtoBufMsg< bodyType > cmd( pCmd->m_eCmd ); \
			if ( !cmd.BDeserializeCrossProc( &pCmd->m_Buffer ) ) \
		{ \
			bError = true; \
		} \
				else \
		{ \
			g_CEFThread.commandFunc( cmd ); \
		} \
	} \
	break; 


//-----------------------------------------------------------------------------
// Purpose: clear any pending commands from the main thread
//-----------------------------------------------------------------------------
void CCEFThread::RunCurrentCommands()
{
	bool bError = false;
	HTMLCommandBuffer_t *pCmd =NULL;

	while( GetCEFThreadCommand( &pCmd ) )
	{
		//Msg( "Got command %d\n", pCmd->m_eCmd );
		switch( pCmd->m_eCmd )
		{
			HTML_MSG_FUNC_NOHANDLE( eHTMLCommands_BrowserCreate, CMsgBrowserCreate, ThreadCreateBrowser );
			HTML_MSG_FUNC( eHTMLCommands_BrowserRemove, CMsgBrowserRemove, ThreadRemoveBrowser );
			HTML_MSG_FUNC( eHTMLCommands_BrowserSize, CMsgBrowserSize, ThreadBrowserSize );
			HTML_MSG_FUNC( eHTMLCommands_BrowserPosition, CMsgBrowserPosition, ThreadBrowserPosition );
			HTML_MSG_FUNC( eHTMLCommands_PostURL, CMsgPostURL, ThreadBrowserPostURL );
			HTML_MSG_FUNC( eHTMLCommands_StopLoad, CMsgStopLoad, ThreadBrowserStopLoad );
			HTML_MSG_FUNC( eHTMLCommands_Reload, CMsgReload, ThreadBrowserReload );
			HTML_MSG_FUNC( eHTMLCommands_GoForward, CMsgGoForward, ThreadBrowserGoForward );
			HTML_MSG_FUNC( eHTMLCommands_GoBack, CMsgGoBack, ThreadBrowserGoBack );
			HTML_MSG_FUNC( eHTMLCommands_Copy, CMsgCopy, ThreadBrowserCopy );
			HTML_MSG_FUNC( eHTMLCommands_Paste, CMsgPaste, ThreadBrowserPaste );
			HTML_MSG_FUNC( eHTMLCommands_ExecuteJavaScript, CMsgExecuteJavaScript, ThreadBrowserExecuteJavascript );
			HTML_MSG_FUNC( eHTMLCommands_SetFocus, CMsgSetFocus, ThreadBrowserSetFocus );
			HTML_MSG_FUNC( eHTMLCommands_HorizontalScrollBarSize, CMsgHorizontalScrollBarSize, ThreadBrowserHorizontalScrollBarSize );
			HTML_MSG_FUNC( eHTMLCommands_VerticalScrollBarSize, CMsgVerticalScrollBarSize, ThreadBrowserVerticalScrollBarSize );
			HTML_MSG_FUNC( eHTMLCommands_Find, CMsgFind, ThreadBrowserFind );
			HTML_MSG_FUNC( eHTMLCommands_StopFind, CMsgStopFind, ThreadBrowserStopFind );
			HTML_MSG_FUNC( eHTMLCommands_SetHorizontalScroll, CMsgSetHorizontalScroll, ThreadBrowserSetHorizontalScroll );
			HTML_MSG_FUNC( eHTMLCommands_SetVerticalScroll, CMsgSetVerticalScroll, ThreadBrowserSetVerticalScroll );
			HTML_MSG_FUNC( eHTMLCommands_SetZoomLevel, CMsgSetZoomLevel, ThreadBrowserSetZoomLevel );
			HTML_MSG_FUNC( eHTMLCommands_ViewSource, CMsgViewSource, ThreadBrowserViewSource );
			HTML_MSG_FUNC( eHTMLCommands_NeedsPaintResponse, CMsgNeedsPaintResponse, ThreadNeedsPaintResponse );
			HTML_MSG_FUNC( eHTMLCommands_BrowserErrorStrings, CMsgBrowserErrorStrings, ThreadBrowserErrorStrings );
			HTML_MSG_FUNC( eHTMLCommands_AddHeader, CMsgAddHeader, ThreadAddHeader );
			HTML_MSG_FUNC( eHTMLCommands_GetZoom, CMsgGetZoom, ThreadGetZoom );
			HTML_MSG_FUNC_NOHANDLE( eHTMLCommands_SetCookie, CMsgSetCookie, ThreadSetCookie );
			HTML_MSG_FUNC_NOHANDLE( eHTMLCommands_SetTargetFrameRate, CMsgSetTargetFrameRate, ThreadSetTargetFrameRate );
			HTML_MSG_FUNC( eHTMLCommands_HidePopup, CMsgHidePopup, ThreadHidePopup );
			HTML_MSG_FUNC( eHTMLCommands_FullRepaint, CMsgFullRepaint, ThreadFullRepaint );
			HTML_MSG_FUNC( eHTMLCommands_GetCookiesForURL, CMsgGetCookiesForURL, ThreadGetCookiesForURL );
			HTML_MSG_FUNC( eHTMLCommands_ZoomToCurrentlyFocusedNode, CMsgZoomToFocusedElement, ThreadZoomToFocusedElement );
			HTML_MSG_FUNC( eHTMLCommands_GetFocusedNodeValue, CMsgFocusedNodeText, ThreadGetFocusedNodeText );
			
			HTML_MSG_FUNC( eHTMLCommands_MouseDown, CMsgMouseDown, ThreadMouseButtonDown );
			HTML_MSG_FUNC( eHTMLCommands_MouseUp, CMsgMouseUp, ThreadMouseButtonUp );
			HTML_MSG_FUNC( eHTMLCommands_MouseDblClick, CMsgMouseDblClick, ThreadMouseButtonDlbClick );
			HTML_MSG_FUNC( eHTMLCommands_MouseWheel, CMsgMouseWheel, ThreadMouseWheel );
			HTML_MSG_FUNC( eHTMLCommands_KeyDown, CMsgKeyDown, ThreadKeyDown );
			HTML_MSG_FUNC( eHTMLCommands_KeyUp, CMsgKeyUp, ThreadKeyUp );
			HTML_MSG_FUNC( eHTMLCommands_KeyChar, CMsgKeyChar, ThreadKeyTyped );
			HTML_MSG_FUNC( eHTMLCommands_MouseMove, CMsgMouseMove, ThreadMouseMove );
			HTML_MSG_FUNC( eHTMLCommands_MouseLeave, CMsgMouseLeave, ThreadMouseLeave );
			HTML_MSG_FUNC( eHTMLCommands_LinkAtPosition, CMsgLinkAtPosition, ThreadLinkAtPosition );
			HTML_MSG_FUNC( eHTMLCommands_ZoomToElementAtPosition, CMsgZoomToElementAtPosition, ThreadZoomToElementAtPosition );
			HTML_MSG_FUNC( eHTMLCommands_SavePageToJPEG, CMsgSavePageToJPEG, ThreadSavePageToJPEG );
			HTML_MSG_FUNC( eHTMLCommands_SetPageScale, CMsgScalePageToValue, ThreadSetPageScale );
			HTML_MSG_FUNC( eHTMLCommands_ExitFullScreen, CMsgExitFullScreen, ThreadExitFullScreen );
			HTML_MSG_FUNC( eHTMLCommands_CloseFullScreenFlashIfOpen, CMsgCloseFullScreenFlashIfOpen, ThreadCloseFullScreenFlashIfOpen );
			HTML_MSG_FUNC( eHTMLCommands_PauseFullScreenFlashMovieIfOpen, CMsgPauseFullScreenFlashMovieIfOpen, ThreadPauseFullScreenFlashMovieIfOpen );
			
		default:
			bError = true;
			AssertMsg1( false, "Invalid message in browser stream (%d)", pCmd->m_eCmd );
			break;
		}

		if (  pCmd->m_eCmd == eHTMLCommands_MouseDown ||  pCmd->m_eCmd == eHTMLCommands_MouseDblClick || pCmd->m_eCmd == eHTMLCommands_KeyDown  )
			m_bSawUserInputThisFrame = true;

		ReleaseCommandBuffer( pCmd );
	}
}


//-----------------------------------------------------------------------------
// Purpose: done with this buffer, put it in the free queue
//-----------------------------------------------------------------------------
void CCEFThread::ReleaseCommandBuffer( HTMLCommandBuffer_t *pBuf )
{
	pBuf->m_eCmd = eHTMLCommands_None;
	pBuf->m_iBrowser = -1;
	pBuf->m_Buffer.Clear();
	m_tslUnsedBuffers.PushItem( pBuf );
}


//-----------------------------------------------------------------------------
// Purpose: get the next free cef thread command buffer to write into
//-----------------------------------------------------------------------------
HTMLCommandBuffer_t *CCEFThread::GetFreeCommandBuffer( EHTMLCommands eCmd, int iBrowser )
{
	HTMLCommandBuffer_t *pBuf;
	if ( !m_tslUnsedBuffers.PopItem( &pBuf ) ) // if nothing in the free queue just make a new one
		pBuf =  new HTMLCommandBuffer_t;

	pBuf->m_eCmd = eCmd;
	pBuf->m_iBrowser = iBrowser;
	return pBuf;
}


//-----------------------------------------------------------------------------
// Purpose: wait for a command of this type on the cef thread
//-----------------------------------------------------------------------------
HTMLCommandBuffer_t *CCEFThread::BWaitForCommand( EHTMLCommands eCmd, int iBrowser )
{
	while ( !m_bExit )
	{
		if ( m_bSleepForValidate )
		{
			m_bSleepingForValidate = true;
			ThreadSleep( 100 );
			continue;
		}
		m_bSleepingForValidate = false;

		HTMLCommandBuffer_t *pBuf = NULL;
		while ( m_tslCommandBuffers.PopItem( &pBuf ) && pBuf )
		{
			if ( pBuf->m_iBrowser == iBrowser )
			{
				if ( pBuf->m_eCmd == eCmd ) // it is what we have been waiting for
					return pBuf;
				if ( pBuf->m_eCmd == eHTMLCommands_BrowserRemove  ) // check to see if this is being deleted while its launching
					return NULL;
			}

			m_vecQueueCommands.AddToTail( pBuf );
			pBuf = NULL;
		}
		Assert( pBuf == NULL );
		m_evWaitingForCommand.Wait();
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: wait for a command of this type on the cef thread
//-----------------------------------------------------------------------------
HTMLCommandBuffer_t *CCEFThread::BWaitForResponse( EHTMLCommands eCmd, int iBrowser )
{
	while ( !m_bExit )
	{
		if ( m_bSleepForValidate )
		{
			m_bSleepingForValidate = true;
			ThreadSleep( 100 );
			continue;
		}
		m_bSleepingForValidate = false;

		HTMLCommandBuffer_t *pBuf = NULL;
		while ( m_tslResponseBuffers.PopItem( &pBuf ) && pBuf )
		{
			if ( pBuf->m_iBrowser == iBrowser )
			{
				if ( pBuf->m_eCmd == eCmd ) // it is what we have been waiting for
					return pBuf;
				if ( pBuf->m_eCmd == eHTMLCommands_BrowserRemove  ) // check to see if this is being deleted while its launching
					return NULL;
			}

			m_vecQueueResponses.AddToTail( pBuf );
			pBuf = NULL;
		}
		Assert( pBuf == NULL );
		m_evWaitingForResponse.Wait();
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: add a command for the cef thread
//-----------------------------------------------------------------------------
void CCEFThread::PushCommand( HTMLCommandBuffer_t *pBuf )
{
	m_tslCommandBuffers.PushItem( pBuf );
	m_evWaitingForCommand.Set();
}


//-----------------------------------------------------------------------------
// Purpose: add a command for the main thread
//-----------------------------------------------------------------------------
void CCEFThread::PushResponse( HTMLCommandBuffer_t *pBuf )
{
	m_tslResponseBuffers.PushItem( pBuf );
	m_evWaitingForResponse.Set();
}



//-----------------------------------------------------------------------------
// Purpose: get any cef responses on the main thread, returns false if there are none
//-----------------------------------------------------------------------------
bool CCEFThread::GetMainThreadCommand( HTMLCommandBuffer_t **pBuf )
{
	if ( m_vecQueueResponses.Count() )
	{
		*pBuf = m_vecQueueResponses[0];
		m_vecQueueResponses.Remove(0);
		return true;
	}
	else
	return 	m_tslResponseBuffers.PopItem( pBuf );
}


//-----------------------------------------------------------------------------
// Purpose: get the next cef thread protobuf command to run, return false if there are none
//-----------------------------------------------------------------------------
bool CCEFThread::GetCEFThreadCommand( HTMLCommandBuffer_t **pBuf )
{
	if ( m_vecQueueCommands.Count() )
	{
		*pBuf = m_vecQueueCommands[0];
		m_vecQueueCommands.Remove(0);
		return true;
	}
	else
		return 	m_tslCommandBuffers.PopItem( pBuf );
}


//-----------------------------------------------------------------------------
// Purpose: the user agent to report when using this browser control
//-----------------------------------------------------------------------------
const char *CCEFThread::PchWebkitUserAgent()
{
	return  "Mozilla/5.0 (%s; U; %s; en-US; %s/%llu; %s) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166 Safari/535.19";
}


//-----------------------------------------------------------------------------
// Purpose: make a new browser object
//-----------------------------------------------------------------------------
void CCEFThread::ThreadCreateBrowser( const CHTMLProtoBufMsg<CMsgBrowserCreate> &htmlCommand )
{
	int idx = m_listClientHandlers.AddToTail();
	++m_nBrowserSerial;
	CClientHandler *pBrowser = new CClientHandler( BROWSER_HANDLE_FROM_INDEX_SERIAL( idx, m_nBrowserSerial ), PchWebkitUserAgent(), m_nBrowserSerial );
	pBrowser->SetUserAgentIdentifier( htmlCommand.BodyConst().useragent().c_str() );

	// send the handle info back to the main thread, needs to be before the CreateBrowser below
	// to stop a race condition of the browser being ready before the main thread knows its handle
	CHTMLProtoBufMsg<CMsgBrowserCreateResponse> cmd( eHTMLCommands_BrowserCreateResponse );
	cmd.Body().set_browser_handle( BROWSER_HANDLE_FROM_INDEX_SERIAL( idx, m_nBrowserSerial ) );
	cmd.Body().set_request_id( htmlCommand.BodyConst().request_id() );
	HTMLCommandBuffer_t *pBuf = GetFreeCommandBuffer( eHTMLCommands_BrowserCreateResponse, idx );
	cmd.SerializeCrossProc( &pBuf->m_Buffer );
	PushResponse( pBuf );

	m_listClientHandlers[idx] = pBrowser;
	pBrowser->AddRef();

	CefWindowInfo info;
	info.SetAsOffScreen( NULL );
	info.m_bPopupWindow = htmlCommand.BodyConst().popup();
	CefBrowserSettings settings;
	settings.fullscreen_enabled = true;
	settings.threaded_compositing_enabled = true;
	settings.java_disabled = true;
	//settings.accelerated_compositing_enabled = true; // not supported when going into fullscreen mode

    // Drag and drop is supposed to be disabled automatically for offscreen views, but
    // ports for Mac and Linux have bugs where it is not really disabled, causing havoc
    settings.drag_drop_disabled = true;

#ifdef LINUX
	// Turn off web features here that don't work on Linux
	settings.webgl_disabled = true;
#endif

	// CEF HTML local storage, databases, and offline application cache are all busted;
	// they live in a temp dir which gets cleaned up on shutdown. There's no point in
	// enabling them just to generate extra files to cleanup on shutdown when there's
	// no actual disk persistence. we need to upgrade CEF again before they will work.
	settings.local_storage_disabled = true;
	settings.databases_disabled = true;
	settings.application_cache_disabled = true;
	settings.java_disabled = true;

	// We don't provide a UI to connect to the WebKit developer tools API
	// so there is no point having it suck up CPU and listening on a port.
	settings.developer_tools_disabled = true;

    // Drag and drop is supposed to be disabled automatically for offscreen views, but
    // ports for Mac and Linux have bugs where it is not really disabled, causing havoc
    settings.drag_drop_disabled = true;

#ifdef LINUX
	// Turn off web features here that don't work on Linux
	settings.webgl_disabled = true;
#endif

	CefBrowser::CreateBrowserSync( info, pBrowser, "", settings );
	CefDoMessageLoopWork();
}


//-----------------------------------------------------------------------------
// Purpose: return true if this browser handle resolves into a currently valid browser handler object
//-----------------------------------------------------------------------------
bool CCEFThread::BIsValidBrowserHandle( uint32 nHandle, int &iClient )
{
	iClient = nHandle & 0xffff;
	if ( m_listClientHandlers.IsValidIndex( nHandle & 0xffff ) )
	{
		if ( m_listClientHandlers[ iClient ]->NSerial() == BROWSER_SERIAL_FROM_HANDLE( nHandle ) )
			return true;
	}
	iClient = -1;
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: delete this browser object, we are done with it
//-----------------------------------------------------------------------------
void CCEFThread::ThreadRemoveBrowser( const CHTMLProtoBufMsg<CMsgBrowserRemove> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		m_listClientHandlers[iClient]->CloseBrowser();
		m_listClientHandlers[iClient]->Release();
		m_listClientHandlers[iClient] = NULL;
		m_listClientHandlers.Remove( iClient );
	}
}


// helper macro to call browser functions if you have a valid handle
#define GET_BROSWER_FUNC( msg, cmd ) \
	int iClient; \
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) ) \
	{ \
		if ( m_listClientHandlers[iClient]->GetBrowser() ) \
			m_listClientHandlers[iClient]->GetBrowser()->cmd; \
	} \
	
	//else \
	//	Assert( false );


//-----------------------------------------------------------------------------
// Purpose: make the web page this many pixels wide&tall
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserSize( const CHTMLProtoBufMsg<CMsgBrowserSize> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, SetSize( PET_VIEW, htmlCommand.BodyConst().width(), htmlCommand.BodyConst().height() ) );
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
		m_listClientHandlers[iClient]->SetSize( htmlCommand.BodyConst().width(), htmlCommand.BodyConst().height() );
	}


//-----------------------------------------------------------------------------
// Purpose: set the global position of the browser to these co-ords, used by some activeX controls
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserPosition( const CHTMLProtoBufMsg<CMsgBrowserPosition> &htmlCommand )
{
	// no longer used - BUGBUG remove me
}


//-----------------------------------------------------------------------------
// Purpose: load this url in the browser with optional post data
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserPostURL( const CHTMLProtoBufMsg<CMsgPostURL> &htmlCommand )
{
	if ( htmlCommand.BodyConst().url().empty() )
		return; // they asked us to load nothing, ignore them

	const char *pchPostData = htmlCommand.BodyConst().post().c_str();
	if ( !pchPostData || !pchPostData[0] )
	{
		GET_BROSWER_FUNC( htmlCommand, GetMainFrame()->LoadURL( std::wstring( CStrAutoEncode( htmlCommand.BodyConst().url().c_str() ).ToWString() ) ) );
	}
	else
	{
		// Create a new request
		CefRefPtr<CefRequest> request(CefRequest::CreateRequest());

		// Set the request URL
		request->SetURL( CStrAutoEncode( htmlCommand.BodyConst().url().c_str() ).ToWString() );

		// Add post data to the request.  The correct method and content-
		// type headers will be set by CEF.
		CefRefPtr<CefPostDataElement> postDataElement( CefPostDataElement::CreatePostDataElement() );

		std::string data = pchPostData;

		postDataElement->SetToBytes(data.length(), data.c_str());

		CefRefPtr<CefPostData> postData(CefPostData::CreatePostData());

		postData->AddElement(postDataElement);
		request->SetPostData(postData);

		GET_BROSWER_FUNC( htmlCommand, GetMainFrame()->LoadRequest( request ) );
	}

	int iClient;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
		m_listClientHandlers[iClient]->SetPendingPageSerial( htmlCommand.BodyConst().pageserial() );

	CefDoMessageLoopWork(); // make sure CEF processes this load before we do anything else (like delete the browser control)
}


//-----------------------------------------------------------------------------
// Purpose: stop loading the page we are on
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserStopLoad( const CHTMLProtoBufMsg<CMsgStopLoad> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, StopLoad() );
}


//-----------------------------------------------------------------------------
// Purpose: reload the current page
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserReload( const CHTMLProtoBufMsg<CMsgReload> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, Reload() );
}


//-----------------------------------------------------------------------------
// Purpose: go forward one page in its history
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserGoForward( const CHTMLProtoBufMsg<CMsgGoForward> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, GoForward() );
}


//-----------------------------------------------------------------------------
// Purpose: go back one page in its history
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserGoBack( const CHTMLProtoBufMsg<CMsgGoBack> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, GoBack() );
}


//-----------------------------------------------------------------------------
// Purpose: copy selected text to the clipboard
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserCopy( const CHTMLProtoBufMsg<CMsgCopy> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand,  GetMainFrame()->Copy() );
}


//-----------------------------------------------------------------------------
// Purpose: paste from the clipboard into the web page
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserPaste( const CHTMLProtoBufMsg<CMsgPaste> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, GetMainFrame()->Paste() );
}


//-----------------------------------------------------------------------------
// Purpose: run this javascript on the current page
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserExecuteJavascript( const CHTMLProtoBufMsg<CMsgExecuteJavaScript> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, GetMainFrame()->ExecuteJavaScript( 
					CStrAutoEncode( htmlCommand.BodyConst().script().c_str() ).ToWString(), L"", 0 ) );
}


//-----------------------------------------------------------------------------
// Purpose: tell the browser it has key focus
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserSetFocus( const CHTMLProtoBufMsg<CMsgSetFocus> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, SetFocus( htmlCommand.BodyConst().focus() ) );
}


//-----------------------------------------------------------------------------
// Purpose: get the size of the horizontal scroll bar
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserHorizontalScrollBarSize( const CHTMLProtoBufMsg<CMsgHorizontalScrollBarSize> &htmlCommand )
{
	ThreadBrowserHorizontalScrollBarSizeHelper( htmlCommand.BodyConst().browser_handle(), true );
}


//-----------------------------------------------------------------------------
// Purpose: tell the main thread details of the horiztonal scrollbar
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserHorizontalScrollBarSizeHelper( int iBrowser, bool bForceSendUpdate )
{
	CClientHandler::CachedScrollBarState_t scroll = { };
	float flZoom = 0.0f;
	int iClient = 0;
	if ( BIsValidBrowserHandle( iBrowser, iClient ) )
	{ 
		CClientHandler *pHandler = m_listClientHandlers[iClient];

		if ( pHandler->GetBrowser() )
		{
			scroll.m_nMax = pHandler->GetBrowser()->HorizontalScrollMax();
			scroll.m_nScroll = pHandler->GetBrowser()->HorizontalScroll();
			scroll.m_bVisible = pHandler->GetBrowser()->IsHorizontalScrollBarVisible();
			pHandler->GetBrowser()->HorizontalScrollBarSize( scroll.m_nX, scroll.m_nY, scroll.m_nWide, scroll.m_nTall );
			flZoom = pHandler->GetBrowser()->GetZoomLevel();
			if ( scroll.m_nMax < 0 )
				scroll.m_nMax = 0;
			if ( flZoom == 0.0f )
				flZoom = 1.0f;
		}
		
		if ( bForceSendUpdate || 0 != memcmp( &pHandler->m_CachedHScroll, &scroll, sizeof( scroll ) ) )
		{
			pHandler->m_CachedHScroll = scroll;

	CHTMLProtoBufMsg<CMsgHorizontalScrollBarSizeResponse> cmd( eHTMLCommands_HorizontalScrollBarSizeResponse );
			cmd.Body().set_x( scroll.m_nX );
			cmd.Body().set_y( scroll.m_nY );
			cmd.Body().set_wide( scroll.m_nWide );
			cmd.Body().set_tall( scroll.m_nTall );
			cmd.Body().set_scroll_max( scroll.m_nMax );
			cmd.Body().set_scroll( scroll.m_nScroll );
			cmd.Body().set_visible( scroll.m_bVisible != 0 );
	cmd.Body().set_zoom( flZoom );
	int m_iBrowser = iBrowser;
	DISPATCH_MESSAGE( eHTMLCommands_HorizontalScrollBarSizeResponse );
}
	}
}


//-----------------------------------------------------------------------------
// Purpose: get the size of the verical scrollbar
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserVerticalScrollBarSize( const CHTMLProtoBufMsg<CMsgVerticalScrollBarSize> &htmlCommand )
{
	ThreadBrowserVerticalScrollBarSizeHelper( htmlCommand.BodyConst().browser_handle(), true );
}


//-----------------------------------------------------------------------------
// Purpose: tell the main thread details of the vertical scrollbar
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserVerticalScrollBarSizeHelper( int iBrowser, bool bForceSendUpdate )
{
	CClientHandler::CachedScrollBarState_t scroll = { };
	float flZoom = 0.0f;
	int iClient = 0;
	if ( BIsValidBrowserHandle( iBrowser, iClient ) )
	{ 
		CClientHandler *pHandler = m_listClientHandlers[iClient];

		if ( pHandler->GetBrowser() )
		{
			scroll.m_nMax = pHandler->GetBrowser()->VerticalScrollMax();
			scroll.m_nScroll = pHandler->GetBrowser()->VerticalScroll();
			scroll.m_bVisible = pHandler->GetBrowser()->IsVeritcalScrollBarVisible();
			pHandler->GetBrowser()->VerticalScrollBarSize( scroll.m_nX, scroll.m_nY, scroll.m_nWide, scroll.m_nTall );
			flZoom = pHandler->GetBrowser()->GetZoomLevel();
			if ( scroll.m_nMax < 0 )
				scroll.m_nMax = 0;
			if ( flZoom == 0.0f )
				flZoom = 1.0f;
		}

		if ( bForceSendUpdate || 0 != memcmp( &pHandler->m_CachedVScroll, &scroll, sizeof( scroll ) )  )
		{
			pHandler->m_CachedVScroll = scroll;

	CHTMLProtoBufMsg<CMsgVerticalScrollBarSizeResponse> cmd( eHTMLCommands_VerticalScrollBarSizeResponse );
			cmd.Body().set_x( scroll.m_nX );
			cmd.Body().set_y( scroll.m_nY );
			cmd.Body().set_wide( scroll.m_nWide );
			cmd.Body().set_tall( scroll.m_nTall );
			cmd.Body().set_scroll_max( scroll.m_nMax );
			cmd.Body().set_scroll( scroll.m_nScroll );
			cmd.Body().set_visible( scroll.m_bVisible != 0 );
	cmd.Body().set_zoom( flZoom );
	int m_iBrowser = iBrowser;
	DISPATCH_MESSAGE( eHTMLCommands_VerticalScrollBarSizeResponse );
}
	}
}


//-----------------------------------------------------------------------------
// Purpose: start a find in a web page
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserFind( const CHTMLProtoBufMsg<CMsgFind> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, Find( 1, htmlCommand.BodyConst().find().c_str(), !htmlCommand.BodyConst().reverse(), false, htmlCommand.BodyConst().infind() ) );
}


//-----------------------------------------------------------------------------
// Purpose: dismiss a current find
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserStopFind( const CHTMLProtoBufMsg<CMsgStopFind> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, StopFinding( 1 ) );
}


//-----------------------------------------------------------------------------
// Purpose: scroll the page horizontally to this pos
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserSetHorizontalScroll( const CHTMLProtoBufMsg<CMsgSetHorizontalScroll> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, SetHorizontalScroll( htmlCommand.BodyConst().scroll() ) );

	ThreadBrowserHorizontalScrollBarSizeHelper( htmlCommand.BodyConst().browser_handle(), true );
	}


//-----------------------------------------------------------------------------
// Purpose: scroll the page vertically to this pos
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserSetVerticalScroll( const CHTMLProtoBufMsg<CMsgSetVerticalScroll> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, SetVerticalScroll( htmlCommand.BodyConst().scroll() ) );

	ThreadBrowserVerticalScrollBarSizeHelper( htmlCommand.BodyConst().browser_handle(), true );
}


//-----------------------------------------------------------------------------
// Purpose: set the zoom to this level, 100% means normal size, 200% double size
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserSetZoomLevel( const CHTMLProtoBufMsg<CMsgSetZoomLevel> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, SetZoomLevel( htmlCommand.BodyConst().zoom() ) );

	CefDoMessageLoopWork(); // tell cef to think one frame, Zoom is in a work queue and we want it to apply right away so think now

	// now tell if about the scrollbars we now have
	ThreadBrowserHorizontalScrollBarSizeHelper( htmlCommand.BodyConst().browser_handle(), true );
	ThreadBrowserVerticalScrollBarSizeHelper( htmlCommand.BodyConst().browser_handle(), true );
}


//-----------------------------------------------------------------------------
// Purpose: show the page source in notepad
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserViewSource( const CHTMLProtoBufMsg<CMsgViewSource> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, GetMainFrame()->ViewSource() );
}


//-----------------------------------------------------------------------------
// Purpose: add a header to any requests we make
//-----------------------------------------------------------------------------
void CCEFThread::ThreadAddHeader( const CHTMLProtoBufMsg<CMsgAddHeader> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		m_listClientHandlers[iClient]->AddHeader( htmlCommand.BodyConst().key().c_str(), htmlCommand.BodyConst().value().c_str() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: main thread is done with a paint buffer, tell our handler
//-----------------------------------------------------------------------------
void CCEFThread::ThreadNeedsPaintResponse( const CHTMLProtoBufMsg<CMsgNeedsPaintResponse> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		m_listClientHandlers[iClient]->SetTextureUploaded( htmlCommand.BodyConst().textureid() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: set the error strings to display on page load
//-----------------------------------------------------------------------------
void CCEFThread::ThreadBrowserErrorStrings( const CHTMLProtoBufMsg<CMsgBrowserErrorStrings> &cmd )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( cmd.BodyConst().browser_handle(), iClient ) )
	{
		m_listClientHandlers[iClient]->SetErrorStrings( cmd.BodyConst().title().c_str(), cmd.BodyConst().header().c_str(),
																						cmd.BodyConst().cache_miss().c_str(), cmd.BodyConst().bad_url().c_str(),
																						cmd.BodyConst().connection_problem().c_str(),
																						cmd.BodyConst().proxy_problem().c_str(), cmd.BodyConst().unknown().c_str() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: return the zoom level for the current page, 100% means actual size, 200% double size
//-----------------------------------------------------------------------------
void CCEFThread::ThreadGetZoom( const CHTMLProtoBufMsg<CMsgGetZoom> &htmlCmd )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCmd.BodyConst().browser_handle(), iClient ) )
	{
		float flZoom = 0.0f;
		if ( m_listClientHandlers[iClient]->GetBrowser()  )
			flZoom = m_listClientHandlers[iClient]->GetBrowser()->GetZoomLevel();
		if ( flZoom == 0.0f )
			flZoom = 1.0f;
		if ( flZoom > 100.0f )
			flZoom /= 100.0f;
		CHTMLProtoBufMsg<CMsgGetZoomResponse> cmd( eHTMLCommands_GetZoomResponse );
		cmd.Body().set_zoom( flZoom );
		int m_iBrowser = htmlCmd.BodyConst().browser_handle();
		DISPATCH_MESSAGE( eHTMLCommands_GetZoomResponse );
	}
}


//-----------------------------------------------------------------------------
// Purpose: handler func to throw the cookie call to the cef IO thread
//-----------------------------------------------------------------------------
void IOT_SetCookie(const CefString& url, CefCookie* cookie, CThreadEvent *pEvent ) 
{
    CefCookieManager::GetGlobalManager()->SetCookie(url, *cookie);
	pEvent->Set();
}


//-----------------------------------------------------------------------------
// Purpose: set this cookie into the cef instance
//-----------------------------------------------------------------------------
void CCEFThread::ThreadSetCookie( const CHTMLProtoBufMsg<CMsgSetCookie> &htmlCommand )
{
	CefCookie cookie;
	cef_string_utf8_copy( htmlCommand.BodyConst().key().c_str(), htmlCommand.BodyConst().key().size(), &cookie.name );
	cef_string_utf8_copy( htmlCommand.BodyConst().value().c_str(), htmlCommand.BodyConst().value().size(), &cookie.value );
	cef_string_utf8_copy( htmlCommand.BodyConst().path().c_str(), htmlCommand.BodyConst().path().size(), &cookie.path );
	if ( htmlCommand.BodyConst().has_expires() )
	{
		AssertMsg( false, "Cookie expiration not implemented -- rtime.cpp needs to be ported." );
		/*
		cookie.has_expires = true;
		CRTime rtExpire( (RTime32)htmlCommand.BodyConst().expires() );
		cookie.expires.year = rtExpire.GetLocalYear();
		cookie.expires.month = rtExpire.GetLocalMonth();
#if !defined(OS_MACOSX)
		cookie.expires.day_of_week = rtExpire.GetLocalDayOfWeek();
#endif
		cookie.expires.day_of_month = rtExpire.GetLocalMonth();
		cookie.expires.hour = rtExpire.GetLocalHour();
		cookie.expires.minute = rtExpire.GetLocalMinute();
		cookie.expires.second = rtExpire.GetLocalSecond();
		*/
	}

	CThreadEvent event;
	CefPostTask(TID_IO, NewCefRunnableFunction( IOT_SetCookie, htmlCommand.BodyConst().host().c_str(), &cookie, &event));
	event.Wait();
}


struct CCefCookie
{
	CUtlString sValue;
	CUtlString sName;
	CUtlString sDomain;
	CUtlString sPath;
};
//-----------------------------------------------------------------------------
// Purpose: helper class to iterator cookies
//-----------------------------------------------------------------------------
class CookieVisitor : public CefCookieVisitor {
public:
	CookieVisitor( CUtlVector<CCefCookie> *pVec, CThreadEvent *pEvent )
	{
		m_pVecCookies = pVec;
		m_pEvent = pEvent;
	}
	~CookieVisitor()
	{
		m_pEvent->Set();
	}

	virtual bool Visit(const CefCookie& cookie, int count, int total,
		bool& deleteCookie)   {

			CCefCookie cookieCopy;
			cookieCopy.sValue = cookie.value.str;
			cookieCopy.sName = cookie.name.str;
			cookieCopy.sDomain = cookie.domain.str;
			cookieCopy.sPath = cookie.path.str;
			m_pVecCookies->AddToTail( cookieCopy );
			return true;
	}

private:
	CUtlVector<CCefCookie> *m_pVecCookies;
	CThreadEvent *m_pEvent;

	IMPLEMENT_REFCOUNTING(CookieVisitor);
};


//-----------------------------------------------------------------------------
// Purpose: handler func to throw the cookie call to the cef IO thread
//-----------------------------------------------------------------------------
void IOT_CookiesForURL(const CefString& url, CThreadEvent *pEvent, CUtlVector<CCefCookie> *pVecCookies ) 
{
	CefCookieManager::GetGlobalManager()->VisitUrlCookies( url, false, new CookieVisitor( pVecCookies, pEvent ) );
}


//-----------------------------------------------------------------------------
// Purpose: get all the cookies for this URL
//-----------------------------------------------------------------------------
void CCEFThread::ThreadGetCookiesForURL( const CHTMLProtoBufMsg<CMsgGetCookiesForURL> &htmlCommand )
{
	CUtlVector<CCefCookie> vecCookies;
	CThreadEvent event;
	CefPostTask(TID_IO, NewCefRunnableFunction( IOT_CookiesForURL, htmlCommand.BodyConst().url().c_str(), &event, &vecCookies ));
	event.Wait();

	CHTMLProtoBufMsg<CMsgGetCookiesForURLResponse> cmd( eHTMLCommands_GetCookiesForURLResponse );
	int m_iBrowser = htmlCommand.BodyConst().browser_handle();
	cmd.Body().set_url( htmlCommand.BodyConst().url() );

	FOR_EACH_VEC( vecCookies, i )
	{
		CCookie *pCookie = cmd.Body().add_cookies();
		pCookie->set_name( vecCookies[i].sName );
		pCookie->set_value( vecCookies[i].sValue );
		pCookie->set_domain( vecCookies[i].sDomain );
		pCookie->set_path( vecCookies[i].sPath );
	}

	DISPATCH_MESSAGE( eHTMLCommands_GetCookiesForURLResponse );
}


//-----------------------------------------------------------------------------
// Purpose: set the framerate to run CEF at
//-----------------------------------------------------------------------------
void CCEFThread::ThreadSetTargetFrameRate( const  CHTMLProtoBufMsg<CMsgSetTargetFrameRate> &htmlCommand )
{
	m_nTargetFrameRate = htmlCommand.BodyConst().ntargetframerate();
}


//-----------------------------------------------------------------------------
// Purpose: hide any showing popup for this browser
//-----------------------------------------------------------------------------
void CCEFThread::ThreadHidePopup( const  CHTMLProtoBufMsg<CMsgHidePopup> &htmlCommand )
{
	GET_BROSWER_FUNC( htmlCommand, HidePopup() );
}


//-----------------------------------------------------------------------------
// Purpose: request a full redraw of the client
//-----------------------------------------------------------------------------
void CCEFThread::ThreadFullRepaint( const  CHTMLProtoBufMsg<CMsgFullRepaint> &htmlCommand )
{
	int iClient;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		if ( m_listClientHandlers[iClient]->GetBrowser()->IsVisuallyNonEmpty() )
		{
			int wide, tall;
			m_listClientHandlers[iClient]->GetExpectedSize( wide, tall);
			CefRect rect;
			rect.x = rect.y = 0;
			rect.width = wide;
			rect.height = tall;
			m_listClientHandlers[iClient]->GetBrowser()->Invalidate( rect );
		}
		else
			m_listClientHandlers[iClient]->GetBrowser()->Reload();
	}
}


//-----------------------------------------------------------------------------
// Purpose: configure the options we want for cef
//-----------------------------------------------------------------------------
void CCEFThread::AppGetSettings(CefSettings& settings, CefRefPtr<CefApp>& app) 
{
	settings.multi_threaded_message_loop = false;
#if defined(OS_WIN)
	settings.auto_detect_proxy_settings_enabled = true;
#endif

	CefString(&settings.cache_path) = m_sHTMLCacheDir;
	CefString(&settings.product_version) = "Steam";
//	CefString(&settings.log_file) =
/*#ifdef WIN32
	settings.graphics_implementation = ANGLE_IN_PROCESS_COMMAND_BUFFER;
#else*/
	settings.graphics_implementation = DESKTOP_IN_PROCESS_COMMAND_BUFFER;
//#endif
}

//-----------------------------------------------------------------------------
// Purpose: clean up the temp folders cef can leave around on crash
//-----------------------------------------------------------------------------
void CCEFThread::CleanupTempFolders()
{
	/*
	// Temporarily commented out to avoid bringing in additional code.
#if defined( WIN32 )
	char rgchPath[MAX_PATH];
	if ( GetTempPathA( Q_ARRAYSIZE( rgchPath ), rgchPath ) == 0 )
		return;

	CUtlString strPath = rgchPath;
#elif defined( LINUX ) || defined( OSX )
	// TMPDIR first, T_tmpdir next, /tmp last.
	char *pszDir = getenv( "TMPDIR" );
	if ( pszDir == NULL )
	{
		pszDir = P_tmpdir;
		if ( pszDir == NULL )
			pszDir = "/tmp";
	}
	if ( pszDir == NULL )
		return;

	CUtlString strPath = pszDir;
#endif

	if ( strPath[strPath.Length()-1] != CORRECT_PATH_SEPARATOR )
		strPath += CORRECT_PATH_SEPARATOR_S;

	CUtlString strSearch = strPath;
	strSearch += "scoped_dir*";
	CDirIterator tempDirIterator( strSearch.String() );

	while ( tempDirIterator.BNextFile() )
	{
		if ( tempDirIterator.BCurrentIsDir() )
		{
			CUtlString sStrDir = strPath;
			sStrDir += tempDirIterator.CurrentFileName();
			BRemoveDirectoryRecursive( sStrDir );
		}
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: main thread for pumping CEF, get commands from the main thread and dispatches responses to it
//-----------------------------------------------------------------------------
int CCEFThread::Run()
{
	{ // scope to trigger destructors before setting the we have exited event
		CefSettings settings;
		CefRefPtr<CefApp> app;

		// blow away any temp folders CEF left lying around if we crashed last
		CleanupTempFolders();

		// Populate the settings based on command line arguments.
		AppGetSettings(settings, app);
		settings.pack_loading_disabled = true;

		// Initialize CEF.
		CefInitialize(settings, app, "");

	#if defined( VPROF_ENABLED )
//		CVProfile *pProfile = GetVProfProfileForCurrentThread();
	#endif
		
		CLimitTimer timer;
		CLimitTimer timerLastCefThink; // track when we think cef so we can do it at a minimum of 10hz
		timerLastCefThink.SetLimit( k_nMillion ); // 1Hz min think time
#ifdef WIN32
		CLimitTimer timerLastFlashFullscreenThink; // track when we think cef so we can do it at a minimum of 10hz
		timerLastFlashFullscreenThink.SetLimit( k_nMillion * k_nMillion ); // think again in the distance future
		bool bInitialThinkAfterInput = false;
#endif
		while( !m_bExit )
		{
	#ifdef OSX
			void *pool = CreateAutoReleasePool();
	#endif

			if ( m_bSleepForValidate )
			{
				m_bSleepingForValidate = true;
				ThreadSleep( 100 );
				continue;
			}
			m_bSleepingForValidate = false;

			m_bSawUserInputThisFrame = false;

	#if defined( VPROF_ENABLED )
//			if ( pProfile )
//				pProfile->MarkFrame( "UI CEF HTML Thread" );
	#endif
			// Limit animation frame rate
			timer.SetLimit( k_nMillion/m_nTargetFrameRate );
			
			// run any pending commands, get ack'd paint buffers
			{
				VPROF_BUDGET( "CCEFThread - RunCurrentCommands()", VPROF_BUDGETGROUP_VGUI );
				RunCurrentCommands();
			}

			// now let cef think
			if ( !m_bExit && ( m_listClientHandlers.Count() || timerLastCefThink.BLimitReached() ) && m_nTargetFrameRate > 0 )
			{
				VPROF_BUDGET( "CCEFThread - CefDoMessageLoopWork()", VPROF_BUDGETGROUP_TENFOOT );
				CefDoMessageLoopWork();
				timerLastCefThink.SetLimit( k_nMillion );  // 1Hz min think time
			}
	#ifdef OSX
			ReleaseAutoReleasePool( pool );
	#endif

			// push any changes to scrollbar data and refresh HTML element hover states
			{
				VPROF_BUDGET( "CCEFThread - Scroll", VPROF_BUDGETGROUP_VGUI );
				FOR_EACH_LL( m_listClientHandlers, i )
				{
					int nBrowser = BROWSER_HANDLE_FROM_INDEX_SERIAL( i, m_listClientHandlers[i]->NSerial() );
					ThreadBrowserHorizontalScrollBarSizeHelper( nBrowser, false );
					ThreadBrowserVerticalScrollBarSizeHelper( nBrowser, false );

					// workaround a CEF issue where mouse hover states don't update after scrolling
					m_listClientHandlers[i]->RefreshCEFHoverStatesAfterScroll();
				}
			}

			{
				VPROF_BUDGET( "CCEFThread - SendSizeChangeEvents", VPROF_BUDGETGROUP_VGUI );
				SendSizeChangeEvents();
			}

			//see if we need a paint
			{
				VPROF_BUDGET( "CCEFThread - Paint", VPROF_BUDGETGROUP_VGUI );
				FOR_EACH_LL( m_listClientHandlers, i )
				{
					if ( !m_listClientHandlers[i]->GetBrowser() )
						continue;

					m_listClientHandlers[i]->Paint();
					if ( m_listClientHandlers[i]->BPaintBufferReady() && m_listClientHandlers[i]->BNeedsPaint() )
					{
						uint32 textureID = m_listClientHandlers[i]->FlipTexture();
						const byte *pRGBA = m_listClientHandlers[i]->PComposedTextureData( textureID );
						if ( pRGBA )
						{
							CClientHandler *pHandler = m_listClientHandlers[i];
							pHandler->Lock();
							if ( pHandler->BPendingScreenShot() )
							{
								 pHandler->SavePageToJPEGIfNeeded( pHandler->GetBrowser(), pRGBA, pHandler->GetTextureWide(), pHandler->GetTextureTall() );
							}
							CHTMLProtoBufMsg<CMsgNeedsPaint> cmd( eHTMLCommands_NeedsPaint );
							cmd.Body().set_browser_handle( BROWSER_HANDLE_FROM_INDEX_SERIAL( i, pHandler->NSerial() ) );
							cmd.Body().set_wide( pHandler->GetTextureWide() );
							cmd.Body().set_tall( pHandler->GetTextureTall() );
							cmd.Body().set_rgba( (uint64)pRGBA );
							cmd.Body().set_pageserial( pHandler->GetPageSerial() );
							cmd.Body().set_textureid( textureID );
							cmd.Body().set_updatex( pHandler->GetUpdateX() );
							cmd.Body().set_updatey( pHandler->GetUpdateY() );
							cmd.Body().set_updatewide( pHandler->GetUpdateWide() );
							cmd.Body().set_updatetall( pHandler->GetUpdateTall() );
							cmd.Body().set_scrollx( pHandler->GetBrowser()->HorizontalScroll() );
							cmd.Body().set_scrolly( pHandler->GetBrowser()->VerticalScroll() );

							if ( pHandler->BPopupVisibleAndPainted() ) // add in the combo box's texture data if visible
							{
								int x,y,wide,tall; // popup sizes
								pHandler->PopupRect( x, y, wide, tall );
								cmd.Body().set_combobox_rgba( (uint64)pHandler->PPopupTextureDataCached() );
								cmd.Body().set_combobox_wide( wide );
								cmd.Body().set_combobox_tall( tall );
							}

							// Texture update rect has now been pushed to main thread
							pHandler->ClearUpdateRect();

							HTMLCommandBuffer_t *pBuf = g_CEFThread.GetFreeCommandBuffer( eHTMLCommands_NeedsPaint, i );
							cmd.SerializeCrossProc( &pBuf->m_Buffer );
							pHandler->Unlock();

							g_CEFThread.PushResponse( pBuf );
						}
						else
						{
							m_listClientHandlers[i]->SetTextureUploaded( textureID );
						}
					}
				}
			}		

#ifdef WIN32
			if ( m_bSawUserInputThisFrame )
			{
				if ( timerLastFlashFullscreenThink.CMicroSecLeft() > k_nMillion/10 || timerLastFlashFullscreenThink.BLimitReached() )
				{
					timerLastFlashFullscreenThink.SetLimit( k_nMillion/10 ); // check in 100msec
				}
				bInitialThinkAfterInput = true;
			}

			if ( m_listClientHandlers.Count() && timerLastFlashFullscreenThink.BLimitReached() )
			{
				CheckForFullScreenFlashControl();
				if ( ( !m_bFullScreenFlashVisible && bInitialThinkAfterInput ) || m_bFullScreenFlashVisible )
				{
					timerLastFlashFullscreenThink.SetLimit( k_nMillion );  // could be a slow machine, check again in 1 sec
				}
				else
				{
					timerLastFlashFullscreenThink.SetLimit( k_nMillion * k_nMillion ); // think again in the distance future
				}

				bInitialThinkAfterInput= false;
			}
#endif

			{
				VPROF_BUDGET( "Sleep - FPS Limiting", VPROF_BUDGETGROUP_TENFOOT );
				if ( timer.BLimitReached() )
					ThreadSleep( 1 );
				else
					m_WakeEvent.Wait( timer.CMicroSecLeft() / 1000 );
			}
		}

		FOR_EACH_LL( m_listClientHandlers, i )
		{
			if ( m_listClientHandlers[i] )
				m_listClientHandlers[i]->CloseBrowser();
			m_listClientHandlers[i] = NULL;;
		}
		m_listClientHandlers.RemoveAll();

		CefDoMessageLoopWork(); // pump the message loop to clear the close browser calls from above

		CefShutdown();
	}
	m_eventDidExit.Set();
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: special code to sniff for flash doing its usual naughty things
//-----------------------------------------------------------------------------
void CCEFThread::CheckForFullScreenFlashControl()
{
#ifdef WIN32
	VPROF_BUDGET( "Searching for Flash fullscreen - FindWindowEx", VPROF_BUDGETGROUP_TENFOOT );

	// see if we need to drag the flash fullscreen window to front
	HWND flashfullscreenHWND = ::FindWindowEx( NULL, NULL, "ShockwaveFlashFullScreen", NULL );
	if ( flashfullscreenHWND )
	{
		DWORD proccess_id;
		GetWindowThreadProcessId( flashfullscreenHWND, &proccess_id);
		TCHAR exe_path[MAX_PATH];
		GetModuleFileName( GetModuleHandle(NULL), exe_path, MAX_PATH);
		HANDLE hmodule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, proccess_id);
		MODULEENTRY32 mod = { sizeof(MODULEENTRY32) };
		if ( Module32First( hmodule, &mod) ) 
		{
			if ( Q_stricmp(mod.szExePath, exe_path) == 0 )  
			{
				if ( !m_bFullScreenFlashVisible )
				{
					m_bFullScreenFlashVisible = true;
					m_flashfullscreenHWND = flashfullscreenHWND;

					FOR_EACH_LL( m_listClientHandlers, i )
					{
						if ( m_listClientHandlers[i] )
							m_listClientHandlers[i]->GetRenderHandler()->OnEnterFullScreen( m_listClientHandlers[i]->GetBrowser() );
					}

					SetForegroundWindow( m_flashfullscreenHWND );
	}
			}
			else 
			{
				if ( m_bFullScreenFlashVisible )
				{
					m_bFullScreenFlashVisible = false;
					m_flashfullscreenHWND = NULL;
					FOR_EACH_LL( m_listClientHandlers, i )
					{
						if ( m_listClientHandlers[i] )
							m_listClientHandlers[i]->GetRenderHandler()->OnExitFullScreen( m_listClientHandlers[i]->GetBrowser() );
					}
				}
			}
		}
		CloseHandle( hmodule );
	}
	else
	{
		if ( m_bFullScreenFlashVisible )
		{
			m_bFullScreenFlashVisible = false;
			m_flashfullscreenHWND = NULL;
			FOR_EACH_LL( m_listClientHandlers, i )
			{
				if ( m_listClientHandlers[i] )
					m_listClientHandlers[i]->GetRenderHandler()->OnExitFullScreen( m_listClientHandlers[i]->GetBrowser() );
			}
		}
	}
#else
#warning "Do we need to sniff for fullscreen flash and it breaking us?"
#endif
}


#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: validate mem
//-----------------------------------------------------------------------------
void CCEFThread::Validate( CValidator &validator, const tchar *pchName )
{
	// hacky but reliable way to avoid both vgui and panorama validating all this stuff twice
	if ( !validator.IsClaimed( m_sHTMLCacheDir.Access() ) )
	{
	VALIDATE_SCOPE();
	ValidateObj( m_sHTMLCacheDir );
	ValidateObj( m_sCookiePath );
	ValidateObj( m_listClientHandlers );
	FOR_EACH_LL( m_listClientHandlers, i )
	{
		ValidatePtr( m_listClientHandlers[i] );
	}
	ValidateObj( m_vecQueueCommands );
	FOR_EACH_VEC( m_vecQueueCommands, i )
	{
		ValidatePtr( m_vecQueueCommands[i] );
	}
		ValidateObj( m_vecQueueResponses );
		FOR_EACH_VEC( m_vecQueueResponses, i )
		{
			ValidatePtr( m_vecQueueResponses[i] );
		}

	ValidateObj( m_tslUnsedBuffers );
	{
		CTSList<HTMLCommandBuffer_t*>::Node_t *pNode = m_tslUnsedBuffers.Detach();
		while ( pNode  )
		{
			CTSList<HTMLCommandBuffer_t*>::Node_t *pNext = (CTSList<HTMLCommandBuffer_t*>::Node_t *)pNode->Next;
			ValidatePtr( pNode->elem );
			m_tslUnsedBuffers.Push( pNode );
			pNode = pNext;
		}
	}

	ValidateObj( m_tslCommandBuffers );
	ValidateObj( m_tslResponseBuffers );
}
}
#endif


//-----------------------------------------------------------------------------
// Purpose: turn on CEF and its supporting thread
//-----------------------------------------------------------------------------
void ChromeInit( const char *pchHTMLCacheDir, const char *pchCookiePath )
{
	Assert( !g_CEFThread.IsAlive() );
	g_CEFThread.SetCEFPaths( pchHTMLCacheDir, pchCookiePath );
	g_CEFThread.SetName( "UICEFThread" );
	g_CEFThread.Start();
}


//-----------------------------------------------------------------------------
// Purpose: turn off CEF
//-----------------------------------------------------------------------------
void ChromeShutdown()
{
	g_CEFThread.TriggerShutdown();
	g_CEFThread.Join( 20 *k_nThousand );
}


#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: suspend the cef thread so we can validate mem
//-----------------------------------------------------------------------------
bool ChromePrepareForValidate()
{
	g_CEFThread.SleepForValidate();
	while ( !g_CEFThread.BSleepingForValidate() )
		ThreadSleep( 100 );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: wake the cef thread back up
//-----------------------------------------------------------------------------
bool ChromeResumeFromValidate()
{
	g_CEFThread.WakeFromValidate();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ChromeValidate( CValidator &validator, const char *pchName )
{
	g_CEFThread.Validate( validator, "g_CEFThread" );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: set this cookie to be used by cef
//-----------------------------------------------------------------------------
bool ChromeSetWebCookie( const char *pchHostname, const char *pchName, const char *pchValue, const char *pchPath, RTime32 nExpires )
{
	CHTMLProtoBufMsg< CMsgSetCookie > cmd( eHTMLCommands_SetCookie ); 
	cmd.Body().set_value( pchValue );
	cmd.Body().set_key( pchName );
	cmd.Body().set_path( pchPath );
	cmd.Body().set_host( pchHostname );
	if ( nExpires )
		cmd.Body().set_expires( nExpires );

	HTMLCommandBuffer_t *pBuf = g_CEFThread.GetFreeCommandBuffer( eHTMLCommands_SetCookie, -1 );
	cmd.SerializeCrossProc( &pBuf->m_Buffer );
	g_CEFThread.PushCommand( pBuf );
	g_CEFThread.WakeThread();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: set this cookie to be used by cef
//-----------------------------------------------------------------------------
bool ChromeGetWebCookiesForURL( CUtlString *pstrValue, const char *pchURL, const char *pchName )
{
	pstrValue->Clear();

	{
		CHTMLProtoBufMsg<CMsgGetCookiesForURL> cmd( eHTMLCommands_GetCookiesForURL );
		cmd.Body().set_url( pchURL );

		HTMLCommandBuffer_t *pCmd = g_CEFThread.GetFreeCommandBuffer( eHTMLCommands_GetCookiesForURL, -1 );
		cmd.SerializeCrossProc( &pCmd->m_Buffer );
		g_CEFThread.PushCommand( pCmd );
	}

	HTMLCommandBuffer_t *pBuf = g_CEFThread.BWaitForResponse( eHTMLCommands_GetCookiesForURLResponse, -1 );
	if ( pBuf )
	{
		CHTMLProtoBufMsg< CMsgGetCookiesForURLResponse > cmdResponse( eHTMLCommands_GetCookiesForURLResponse ); 
		if ( cmdResponse.BDeserializeCrossProc( &pBuf->m_Buffer ) )
		{
			for ( int i = 0; i < cmdResponse.BodyConst().cookies_size(); i++ )
			{
				const CCookie &cookie = cmdResponse.BodyConst().cookies(i);
				if ( cookie.name() == pchName )
					pstrValue->Set( cookie.value().c_str() );
			}
		}
		g_CEFThread.ReleaseCommandBuffer( pBuf );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: set the build number to report in our user agent
//-----------------------------------------------------------------------------
void ChromeSetClientBuildID( uint64 ulBuildID )
{
	sm_ulBuildID = ulBuildID;
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CChromePainter::CChromePainter( CClientHandler *pParent ) 
{
	m_iNextTexture = 0;
	m_iTexturesInFlightBits = 0;
	m_pParent = pParent;
	m_bUpdated = false;
	m_bPopupVisible = false;
}


//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CChromePainter::~CChromePainter()
{
}

//-----------------------------------------------------------------------------
// Purpose: cef is calling us back and saying it updated the html texture
//-----------------------------------------------------------------------------
void CChromePainter::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer)
{
	VPROF_BUDGET( "CChromePainter::DrawSubTextureRGBA", VPROF_BUDGETGROUP_VGUI );

	int wide, tall;
	browser->GetSize( type, wide, tall );

	if ( wide <= 0 || tall <= 0 )
		return;

	if ( type == PET_POPUP )
	{
		m_nPopupWide = wide;
		m_nPopupTall = tall;

		m_PopupTexture.EnsureCount( tall*wide*4 );
		Q_memcpy( m_PopupTexture.Base(), buffer, tall*wide*4 );

		// force a recomposition + display whenever painting a popup
		m_bUpdated = true;
	}
	else
	{
		// main browser painting

	if ( !m_pParent->IsVisuallyNonEmpty() )
	{
		return;
	}

		// If there were no dirty regions (unlikely), perhaps due to a bug, be conservative and paint all
		if ( dirtyRects.empty() )
	{
			m_MainTexture.MarkAllDirty();
	}
	else
	{
			for ( RectList::const_iterator iter = dirtyRects.begin(); iter != dirtyRects.end(); ++iter )
		{
				m_MainTexture.MarkDirtyRect( iter->x, iter->y, iter->x + iter->width, iter->y + iter->height );
				}
				}

		// Refresh all dirty main texture pixels from the chromium rendering buffer
		if ( m_MainTexture.BUpdatePixels( (byte*)buffer, wide, tall ) )
		{
			// at least one pixel in the main texture has changed
			m_bUpdated = true;

			// Notify the main thread that this newly painted region is dirty
			m_UpdateRect.MarkDirtyRect( m_MainTexture );

			// Merge update region into all buffer textures so that at composition time,
			// they know to copy the union of all updates since the last composition
			for ( size_t i = 0; i < Q_ARRAYSIZE(m_Texture); ++i )
			{
				m_Texture[i].MarkDirtyRect( m_MainTexture );
			}
		}

		// The main texture is now a clean copy of chromium's canvas backing
		m_MainTexture.MarkAllClean();
	}
}

//-----------------------------------------------------------------------------
// Purpose: true if we have had a paint call from cef
//-----------------------------------------------------------------------------
bool CChromePainter::BUpdated() 
{ 
	return m_bUpdated;
}


//-----------------------------------------------------------------------------
// Purpose: force the updated state
//-----------------------------------------------------------------------------
void CChromePainter::SetUpdated( bool state )
{
	m_bUpdated = state;
}


//-----------------------------------------------------------------------------
// Purpose: move to the next html texture to render into
//-----------------------------------------------------------------------------
uint32 CChromePainter::FlipTexture() 
{ 
	int iTex = m_iNextTexture;
	m_iTexturesInFlightBits |= ( 1<<iTex );
	m_iNextTexture = (m_iNextTexture+1)%Q_ARRAYSIZE(m_Texture);
	return iTex; 
}


//-----------------------------------------------------------------------------
// Purpose: we are done with this texture on the main (and render) threads
//-----------------------------------------------------------------------------
void CChromePainter::SetTextureUploaded( uint32 id ) 
{ 
	// Don't need to check for id < 0 because it is unsigned.
	Assert( id < Q_ARRAYSIZE(m_Texture) );
	if ( id < Q_ARRAYSIZE(m_Texture) )
	{
		m_iTexturesInFlightBits &= ~(1<<id);
	}
}


//-----------------------------------------------------------------------------
// Purpose: get the texure data for this textureid
//-----------------------------------------------------------------------------
byte *CChromePainter::PComposedTextureData( uint32 iTexture ) 
{ 
	Assert( iTexture < Q_ARRAYSIZE(m_Texture) );
	// Update pixels and then clean the dirty list
	m_Texture[ iTexture ].BUpdatePixels( m_MainTexture.GetPixels(), m_MainTexture.GetWide(), m_MainTexture.GetTall() );
	m_Texture[ iTexture ].MarkAllClean();
	byte *pTexture = m_Texture[ iTexture ].GetPixels();

	// overlay a popup if we have one
	if ( BPopupVisibleAndPainted() )
	{
		// composite in the popup texture, clipped to the browser width/height, and mark those pixels dirty

		int nCopyWide = MIN( GetPopupWide(), m_Texture[ iTexture ].GetWide() - GetPopupX() );
		int nCopyTall = MIN( GetPopupTall(), m_Texture[ iTexture ].GetTall() - GetPopupY() );

		// Notify the main thread that the composited popup region is dirty
		m_UpdateRect.MarkDirtyRect( GetPopupX(), GetPopupY(), GetPopupX() + nCopyWide, GetPopupY() + nCopyTall );

		byte *pCurTextureByte = pTexture;
		pCurTextureByte += ( GetPopupY() * m_Texture[ iTexture ].GetWide() * 4 ); // move ahead to the right row
		pCurTextureByte += ( GetPopupX() * 4 ); // now offset into the edge as needed

		const byte *pPopupTexture = PPopupTextureData();
		for ( int iRow = 0; iRow < nCopyTall; iRow++ )
{
			Q_memcpy( pCurTextureByte, pPopupTexture, nCopyWide*4 );
			pCurTextureByte += ( m_Texture[ iTexture ].GetWide() * 4 ); // move ahead one row in both images
			pPopupTexture += ( GetPopupWide() * 4 );
		}
}

	return pTexture;
}


//-----------------------------------------------------------------------------
// Purpose: true if we have a free slot in our paint buffers
//-----------------------------------------------------------------------------
bool CChromePainter::BPaintBufferAvailable()
{
	return (m_MainTexture.GetWide() > 0 && m_MainTexture.GetTall() > 0) && !( m_iTexturesInFlightBits & (1<<m_iNextTexture) );
}


//-----------------------------------------------------------------------------
// Purpose: copy off the current popup texture so we can hand it off to the main thread
//-----------------------------------------------------------------------------
const byte *CChromePainter::PPopupTextureDataCached()
{
	m_PopupTextureCache.EnsureCount( m_PopupTexture.Count() );
	Q_memcpy( m_PopupTextureCache.Base(), m_PopupTexture.Base(), m_PopupTexture.Count() );
	return m_PopupTextureCache.Base();
}


//-----------------------------------------------------------------------------
// Purpose: Client implementation of the browser handler class
//-----------------------------------------------------------------------------
#pragma warning( push )
#pragma warning( disable : 4355 ) // 'this' : used in base member initializer list
CClientHandler::CClientHandler( int iBrowser, const char *pchUserAgent, uint16 nSerial ) : m_Painter( this )
{
	m_nSerial = nSerial;
	m_iBrowser = iBrowser;
	m_nExpectedWide = m_nExpectedTall = 0;
	m_szUserAgentExtras[0] = 0;
	m_Browser = NULL;
	m_bShowingToolTip = false;
	m_strUserAgent = pchUserAgent;
	m_bPendingPaint = false;
	m_nMouseX = 0;
	m_nMouseY = 0;
	m_nMouseScrolledX = 0;
	m_nMouseScrolledY = 0;
	m_bMouseFocus = false;
	m_bBrowserClosing = false;
	m_nPageSerial = 0;
	m_nPendingPageSerial = 0;
	m_Painter.AddRef();
	memset( &m_CachedHScroll, 0, sizeof( m_CachedHScroll ) );
	memset( &m_CachedVScroll, 0, sizeof( m_CachedVScroll ) );
}
#pragma warning( pop )


//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CClientHandler::~CClientHandler()
{
	Assert( m_Browser == NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Event called to request a new tab is created. The |parentBrowser| parameter
// will point to the parent browser window, if any. The |foreground| parameter
// will be true if the new tab should take the foreground. If you create the window
// yourself you should populate the window handle member of |createInfo| and
// return RV_HANDLED.  Otherwise, return RV_CONTINUE and the framework may
// create a tab.  By default, a newly created tab will receive the
// same handler as the parent window.  To change the handler for the new
// window modify the object that |handler| points to.
//-----------------------------------------------------------------------------
bool CClientHandler::OnNewTab(CefRefPtr<CefBrowser> parentBrowser,
											const CefPopupFeatures& popupFeatures,
											CefWindowInfo& windowInfo,
											const CefString& url,
											bool bForeground,
											CefRefPtr<CefClient>& client,
											CefBrowserSettings& settings )
{
	CHTMLProtoBufMsg<CMsgOpenNewTab> cmd( eHTMLCommands_OpenNewTab );
	cmd.Body().set_url( url.c_str() );
	cmd.Body().set_bforeground( bForeground );
	
	DISPATCH_MESSAGE( eHTMLCommands_OpenNewTab );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Called before a new popup window is created. The |parentBrowser| parameter
// will point to the parent browser window. The |popupFeatures| parameter will
// contain information about the style of popup window requested. Return false
// to have the framework create the new popup window based on the parameters
// in |windowInfo|. Return true to cancel creation of the popup window. By
// default, a newly created popup window will have the same client and
// settings as the parent window. To change the client for the new window
// modify the object that |client| points to. To change the settings for the
// new window modify the |settings| structure.
//-----------------------------------------------------------------------------
bool CClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser,
				   const CefPopupFeatures& popupFeatures,
				   CefWindowInfo& windowInfo,
				   const CefString& url,
				   CefRefPtr<CefClient>& client,
				   CefBrowserSettings& settings)
{
	// If it's a steam:// url we already have a scheme handler installed, however that's too late to block the frame navigating
	// and we'll end up loading a blank new window.  So preempt that happening here and handle early returning that we've handled so
	// chromium won't actually change URLs or navigate at all.
	if ( url.size() > 0 && ( Q_stristr( url.c_str(), "steam://" ) || Q_stristr( url.c_str(), "steambeta://" ) ) && Q_stristr( url.c_str(), "/close" ) == NULL )
	{
		CStrAutoEncode urlString( url.c_str() );
		CHTMLProtoBufMsg<CMsgOpenSteamURL> cmd( eHTMLCommands_OpenSteamURL );
		cmd.Body().set_url( urlString.ToString() );
		DISPATCH_MESSAGE( eHTMLCommands_OpenSteamURL );
	
		return true;
	}

	CStrAutoEncode urlString( url.c_str() );
	static bool bInPopup = false;
	// if we get an empty url string when loading a new popup page just don't load it, we don't support the make a
	// new popup and .write() into the buffer to make the contents because we want to make a whole new VGUI HTML object
	// that contains the webkit widget for that popup, not allow this inline one
	if ( urlString.ToString() && Q_strlen( urlString.ToString() ) > 0 )
	{
		if ( !bInPopup  )
		{
			bInPopup = true;
			CHTMLProtoBufMsg<CMsgPopupHTMLWindow> cmd( eHTMLCommands_PopupHTMLWindow );
			cmd.Body().set_url( urlString.ToString() );
			if ( popupFeatures.xSet )
				cmd.Body().set_x( popupFeatures.x );
			if ( popupFeatures.ySet )
				cmd.Body().set_y( popupFeatures.y );
			if ( popupFeatures.widthSet )
				cmd.Body().set_wide( popupFeatures.width );
			if ( popupFeatures.heightSet )
				cmd.Body().set_tall( popupFeatures.height );

			DISPATCH_MESSAGE( eHTMLCommands_PopupHTMLWindow );
			bInPopup = false;
			return true;
		}
	}
	if ( !bInPopup )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Event called after a new window is created. The return value is currently
// ignored.
//-----------------------------------------------------------------------------
void CClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	Lock();
	if ( !m_Browser )
	{
		// We need to keep the main child window, but not popup windows
		m_Browser = browser;
		browser->SetShowScrollBars( false );
		SetBrowserAgent( browser );
		if ( m_nExpectedWide > 0 && m_nExpectedTall > 0 )
			browser->SetSize( PET_VIEW, m_nExpectedWide, m_nExpectedTall );

		CHTMLProtoBufMsg<CMsgBrowserReady> cmd( eHTMLCommands_BrowserReady );
		DISPATCH_MESSAGE( eHTMLCommands_BrowserReady );
	}
	Unlock();
}



//-----------------------------------------------------------------------------
// Purpose: 
// Event called when the page title changes. The return value is currently
// ignored.
//-----------------------------------------------------------------------------
void CClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
	if ( !title.empty() )
	{
		CHTMLProtoBufMsg<CMsgSetHTMLTitle> cmd( eHTMLCommands_SetHTMLTitle );
		cmd.Body().set_title( title );

		DISPATCH_MESSAGE( eHTMLCommands_SetHTMLTitle );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Event called before browser navigation. The client has an opportunity to
// modify the |request| object if desired.  Return RV_HANDLED to cancel
// navigation.
//-----------------------------------------------------------------------------
bool CClientHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
												  CefRefPtr<CefFrame> frame,
												  CefRefPtr<CefRequest> request,
												  NavType navType,
												  bool isRedirect,
												  bool isNewTabRequest 
												  )
{
	std::string sURL = request->GetURL();

	// If it's a steam:// url we already have a scheme handler installed, however that's too late to block the frame navigating
	// and we'll end up loading a blank page.  So preempt that happening here and handle early returning that we've handled so
	// chromium won't actually change URLs or navigate at all.	
	if ( sURL.size() > 0 && (sURL.find( "steam://" ) == 0 || sURL.find( "steambeta://" ) == 0)  && sURL.find( "/close" ) == std::wstring::npos )
	{
		CHTMLProtoBufMsg<CMsgOpenSteamURL> cmd( eHTMLCommands_OpenSteamURL );
		cmd.Body().set_url( CStrAutoEncode( request->GetURL().c_str() ).ToString() );
		DISPATCH_MESSAGE( eHTMLCommands_OpenSteamURL );

		return true ; 
	}

	if ( isNewTabRequest )
		return false;

	// We only care about these on the main frame
	if( !frame.get() || !frame->IsMain() )
		return false;

	if ( request->GetPostData() )
	{
		CefPostData::ElementVector elements;
		request->GetPostData()->GetElements( elements );
		CefPostData::ElementVector::const_iterator it = elements.begin();
		m_strPostData = "";
		CUtlVector<char> vecPostBytes;
		while ( it != elements.end() )
		{
			if ( it->get()->GetType() == PDE_TYPE_BYTES )
			{
				size_t nBytes = it->get()->GetBytesCount();
				int curInsertPos = vecPostBytes.Count();
				vecPostBytes.EnsureCount( curInsertPos + nBytes + 1 );
				it->get()->GetBytes( nBytes, vecPostBytes.Base() + curInsertPos );
				vecPostBytes[ curInsertPos + nBytes ] = 0;
			}
			it++;
		}

		m_strPostData = vecPostBytes.Base();
	}
	else
		m_strPostData = "";

	CStrAutoEncode strURL( sURL.c_str() );

	if ( isRedirect )
		m_strLastRedirectURL = strURL.ToString();

	bool rv = false;

	{
		// scope this so the wait below doesn't keep this allocation on the stack
		CHTMLProtoBufMsg<CMsgStartRequest> cmd( eHTMLCommands_StartRequest );
		cmd.Body().set_url( strURL.ToString() );
		CefString frameName = frame->GetName();
		if ( !frameName.empty() )
			cmd.Body().set_target( frameName.c_str() );
		cmd.Body().set_postdata( m_strPostData );
		cmd.Body().set_bisredirect( isRedirect );

		DISPATCH_MESSAGE( eHTMLCommands_StartRequest );
	}

	HTMLCommandBuffer_t *pBuf = g_CEFThread.BWaitForCommand( eHTMLCommands_StartRequestResponse, m_iBrowser );
	if ( pBuf )
	{
		CHTMLProtoBufMsg< CMsgStartRequestResponse > cmd( eHTMLCommands_StartRequestResponse ); 
		if ( cmd.BDeserializeCrossProc( &pBuf->m_Buffer ) ) 
		{
			if ( !cmd.BodyConst().ballow() )
				rv = true ; 
		}
		g_CEFThread.ReleaseCommandBuffer( pBuf );
	}

	if ( m_Snapshot.m_sURLSnapshot.IsValid() )
		m_Snapshot.m_flRequestTimeout = Plat_FloatTime(); // before we change URL lets queue up a snapshot for the next paint

	return rv;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Event called when the browser begins loading a page.  The |frame| pointer
// will be empty if the event represents the overall load status and not the
// load status for a particular frame.  The return value is currently ignored.
//-----------------------------------------------------------------------------
void CClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame, bool bIsNewNavigation )
{
	if ( !frame.get() )
		return;

	{
		if ( !frame->IsMain() )
			return;

	std::wstring sURL = frame->GetURL();
	if ( sURL.empty() )
		return;

	CStrAutoEncode url( sURL.c_str() );
	m_strCurrentUrl = url.ToString();

	if ( m_strCurrentUrl.IsEmpty() )
		return;

	bool bIsRedirect = false;
	if ( m_strCurrentUrl == m_strLastRedirectURL )
		bIsRedirect = true;

	CHTMLProtoBufMsg<CMsgURLChanged> cmd( eHTMLCommands_URLChanged );
	cmd.Body().set_url( url.ToString() );
		cmd.Body().set_bnewnavigation( bIsNewNavigation );
	
	if ( !m_strPostData.IsEmpty() )
			cmd.Body().set_postdata( m_strPostData.String() );

	cmd.Body().set_bisredirect( bIsRedirect );
	CefString frameName = frame->GetName();
	if ( !frameName.empty() )
		cmd.Body().set_pagetitle( frameName.c_str() );

	DISPATCH_MESSAGE( eHTMLCommands_URLChanged );
}

	{
		CHTMLProtoBufMsg<CMsgCanGoBackAndForward> cmd( eHTMLCommands_CanGoBackandForward );
		cmd.Body().set_bgoback( browser->CanGoBack() );
		cmd.Body().set_bgoforward( browser->CanGoForward() );
		DISPATCH_MESSAGE( eHTMLCommands_CanGoBackandForward );
	}

	m_nPageSerial = m_nPendingPageSerial;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Event called when the browser is done loading a page. The |frame| pointer
// will be empty if the event represents the overall load status and not the
// load status for a particular frame. This event will be generated
// irrespective of whether the request completes successfully. The return
// value is currently ignored.
//-----------------------------------------------------------------------------
void CClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
											 CefRefPtr<CefFrame> frame,
											 int httpStatusCode, CefRefPtr<CefRequest> request )
{
	// We only care about these on the main frame
	if( !frame.get() || !frame->IsMain() )
		return;

	{
		CHTMLProtoBufMsg<CMsgFinishedRequest> cmd( eHTMLCommands_FinishedRequest );

		if ( browser->GetMainFrame()->GetURL().size() > 0 )
			cmd.Body().set_url( CStrAutoEncode( browser->GetMainFrame()->GetURL().c_str() ).ToString() );
		else
			cmd.Body().set_url( "" );

		CefString frameName = browser->GetMainFrame()->GetName();
		if ( !frameName.empty() )
			cmd.Body().set_pagetitle( frameName.c_str() );

		if ( request.get() )
		{
			CefRequest::HeaderMap headerMap;
			request->GetHeaderMap( headerMap );

			CefRequest::HeaderMap::const_iterator it;
			for(it = headerMap.begin(); it != headerMap.end(); ++it) 
			{
				CHTMLHeader *pHeader = cmd.Body().add_headers();
				if ( !it->first.empty() )
					pHeader->set_key(  it->first.c_str() );
				if ( !it->second.empty() )
					pHeader->set_value( it->second.c_str() );
			}

			CefRefPtr<CefSecurityDetails> pRefSecurityDetails = request->SecurityDetails();
			CHTMLPageSecurityInfo *pSecurityInfo = cmd.Body().mutable_security_info();
			if ( pRefSecurityDetails.get() )
			{
				pSecurityInfo->set_bissecure( pRefSecurityDetails->BIsSecure() );
				pSecurityInfo->set_bhascerterror( pRefSecurityDetails->BHasCertError() );
				pSecurityInfo->set_issuername( pRefSecurityDetails->PchCertIssuer() );
				pSecurityInfo->set_certname( pRefSecurityDetails->PchCertCommonName() );
				pSecurityInfo->set_certexpiry( pRefSecurityDetails->TCertExpiry() );
				pSecurityInfo->set_bisevcert( pRefSecurityDetails->BIsEVCert() );
				pSecurityInfo->set_ncertbits( pRefSecurityDetails->NCertBits() );
			}
			else
			{
				pSecurityInfo->set_bissecure( false );
			}
		}

		DISPATCH_MESSAGE( eHTMLCommands_FinishedRequest );
	}

	{
		CHTMLProtoBufMsg<CMsgCanGoBackAndForward> cmd( eHTMLCommands_CanGoBackandForward );
		cmd.Body().set_bgoback( browser->CanGoBack() );
		cmd.Body().set_bgoforward( browser->CanGoForward() );
		DISPATCH_MESSAGE( eHTMLCommands_CanGoBackandForward );
	}

	if ( m_Snapshot.m_sURLSnapshot.IsValid() )
		m_Snapshot.m_flRequestTimeout = Plat_FloatTime(); // finished page load, lets queue up a snapshot for the next paint
}


//-----------------------------------------------------------------------------
// Purpose: 
// Called when the browser fails to load a resource.  |errorCode| is the
// error code number and |failedUrl| is the URL that failed to load.  To
// provide custom error text assign the text to |errorText| and return
// RV_HANDLED.  Otherwise, return RV_CONTINUE for the default error text.
//-----------------------------------------------------------------------------
bool CClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
											   CefRefPtr<CefFrame> frame,
											   ErrorCode errorCode,
											   const CefString& failedUrl,
											   CefString& errorText)
{
	// If it's a steam:// url we always get an error, but we handle it ok internally already, just ignore
	if ( failedUrl.size() > 0 &&  ( Q_stristr( failedUrl.c_str(), "steam://" )  || Q_stristr( failedUrl.c_str(), "steambeta://" ) ) )
		return false; 

	const char *pchDetail = NULL;
	switch ( errorCode )
	{
		case ERR_ABORTED:
			// We'll get this in cases where we just start another URL before finishing a previous and such, don't show it.
			return false;
			break;
		case ERR_CACHE_MISS:
			pchDetail = m_sErrorCacheMiss;
			break;
		case ERR_UNKNOWN_URL_SCHEME:
		case ERR_INVALID_URL:
			pchDetail =  m_sErrorBadURL;
			break;
		case ERR_CONNECTION_CLOSED:
		case ERR_CONNECTION_RESET:
		case ERR_CONNECTION_REFUSED:
		case ERR_CONNECTION_ABORTED:
		case ERR_CONNECTION_FAILED:
		case ERR_NAME_NOT_RESOLVED:
		case ERR_INTERNET_DISCONNECTED:
		case ERR_CONNECTION_TIMED_OUT:
			pchDetail =  m_sErrorConnectionProblem;
			break;
		case ERR_UNEXPECTED_PROXY_AUTH:
		case ERR_EMPTY_PROXY_LIST:
			pchDetail =  m_sErrorProxyProblem;
			break;
		default:
			pchDetail =  m_sErrorUnknown;
			break;
	}

	char rgchError[4096];
	Q_snprintf( rgchError, Q_ARRAYSIZE( rgchError ),
	"<html>"
		"<head>"
			"<title>%s</title>"
		"</head>"
		"<body style=\"background-color:#2D2D2B; color=#939393; font-family: Arial,Helvetica; margin:20px;\">"
		"<h1 style=\"color: #E1E1E1; font-size: 26px; font-weight: normal\">%s%d</h1>"
		"<p style=\"line-height: 17px; margin-top:10px; color: #939393;\">"
			"%s"
		"</p>"
		"</body>"
	"</html>",
		m_sErrorTitle.String(), 
		m_sErrorHeader.String(), 
		errorCode,
		pchDetail );

	errorText = rgchError;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Event called before a resource is loaded.  To allow the resource to load
// normally return RV_CONTINUE. To redirect the resource to a new url
// populate the |redirectUrl| value and return RV_CONTINUE.  To specify
// data for the resource return a CefStream object in |resourceStream|, set
// 'mimeType| to the resource stream's mime type, and return RV_CONTINUE.
// To cancel loading of the resource return RV_HANDLED.
//-----------------------------------------------------------------------------
bool CClientHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser,
										  CefRefPtr<CefRequest> request,
										  CefString& redirectUrl,
										  CefRefPtr<CefStreamReader>& resourceStream,
										  CefRefPtr<CefResponse> response,
										  int loadFlags)
{
	if ( request->GetURL().size() == 0 )
		return false;

	CHTMLProtoBufMsg<CMsgLoadingResource> cmd( eHTMLCommands_LoadingResource );
	cmd.Body().set_url( CStrAutoEncode( request->GetURL().c_str() ).ToString() );
	DISPATCH_MESSAGE( eHTMLCommands_LoadingResource );

	// insert custom headers
	CefRequest::HeaderMap headerMap;
	request->GetHeaderMap( headerMap );
	FOR_EACH_VEC( m_vecHeaders, i )
	{
		headerMap.insert( m_vecHeaders[i] );
	}

	request->SetHeaderMap( headerMap );

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Run a JS alert message.  Return RV_CONTINUE to display the default alert
// or RV_HANDLED if you displayed a custom alert.
//-----------------------------------------------------------------------------
bool CClientHandler::OnJSAlert(CefRefPtr<CefBrowser> browser,
											 CefRefPtr<CefFrame> frame,
											 const CefString& message)
{
	{
		// scope this so the wait below doesn't keep this allocation on the stack
		CHTMLProtoBufMsg<CMsgJSAlert> cmd( eHTMLCommands_JSAlert );
		cmd.Body().set_message( message.c_str() );
		DISPATCH_MESSAGE( eHTMLCommands_JSAlert );
	}

	HTMLCommandBuffer_t *pBuf = g_CEFThread.BWaitForCommand( eHTMLCommands_JSDialogResponse, m_iBrowser );
	if ( pBuf )
	{
		CHTMLProtoBufMsg< CMsgJSDialogResponse > cmd( eHTMLCommands_JSDialogResponse ); 
		g_CEFThread.ReleaseCommandBuffer( pBuf );
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Run a JS confirm request.  Return RV_CONTINUE to display the default alert
// or RV_HANDLED if you displayed a custom alert.  If you handled the alert
// set |CefHandler::RetVal| to true if the user accepted the confirmation.
//-----------------------------------------------------------------------------
bool CClientHandler::OnJSConfirm(CefRefPtr<CefBrowser> browser,
											   CefRefPtr<CefFrame> frame,
											   const CefString& message,
											   bool& retval)
{
	{
		// scope this so the wait below doesn't keep this allocation on the stack
		CHTMLProtoBufMsg<CMsgJSConfirm> cmd( eHTMLCommands_JSConfirm );
		cmd.Body().set_message( message.c_str() );
		DISPATCH_MESSAGE( eHTMLCommands_JSConfirm );
	}

	retval = false;

	HTMLCommandBuffer_t *pBuf = g_CEFThread.BWaitForCommand( eHTMLCommands_JSDialogResponse, m_iBrowser );
	if ( pBuf )
	{
		CHTMLProtoBufMsg< CMsgJSDialogResponse > cmd( eHTMLCommands_JSDialogResponse ); 
		if ( cmd.BDeserializeCrossProc( &pBuf->m_Buffer ) ) 
		{
			if ( cmd.BodyConst().result() )
				retval = true ; 
		}

		g_CEFThread.ReleaseCommandBuffer( pBuf );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Run a JS prompt request.  Return RV_CONTINUE to display the default prompt
// or RV_HANDLED if you displayed a custom prompt.  If you handled the prompt
// set |CefHandler::RetVal| to true if the user accepted the prompt and request and
// |result| to the resulting value.
//-----------------------------------------------------------------------------
bool CClientHandler::OnJSPrompt(CefRefPtr<CefBrowser> browser,
											  CefRefPtr<CefFrame> frame,
											  const CefString& message,
											  const CefString& defaultValue,
											  bool& retval,
											  CefString& result)
{
	retval = false; // just don't pop JS prompts for now
	result = defaultValue;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Called just before a window is closed.
//-----------------------------------------------------------------------------
void CClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	Lock();
	if ( m_Browser )
	{
		// Free the browser pointer so that the browser can be destroyed
		m_Browser = NULL;

		if ( !m_bBrowserClosing )
		{
			CHTMLProtoBufMsg<CMsgClose> cmd( eHTMLCommands_Close );
			DISPATCH_MESSAGE( eHTMLCommands_Close );
		}
	}
	Unlock();
}


//-----------------------------------------------------------------------------
// Purpose: show a html popup, a pulldown menu or the like
//-----------------------------------------------------------------------------
void CChromePainter::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
	m_bPopupVisible = show;
	int m_iBrowser = m_pParent->m_iBrowser;
	if ( show )
	{
		CHTMLProtoBufMsg<CMsgShowPopup> cmd( eHTMLCommands_ShowPopup );
		DISPATCH_MESSAGE( eHTMLCommands_ShowPopup );
	}
	else
	{
		CHTMLProtoBufMsg<CMsgHidePopup> cmd( eHTMLCommands_HidePopup );
		DISPATCH_MESSAGE( eHTMLCommands_HidePopup );

		// redraw the buffered texture behind the rectangle that was previously composited
		for ( size_t i = 0; i < Q_ARRAYSIZE( m_Texture ); ++i )
		{
			m_Texture[i].MarkDirtyRect( m_nPopupX, m_nPopupY, m_nPopupX + m_nPopupWide, m_nPopupY + m_nPopupTall );
		}

		// and notify the main thread to redraw the previously composited area as well
		m_UpdateRect.MarkDirtyRect( m_nPopupX, m_nPopupY, m_nPopupX + m_nPopupWide, m_nPopupY + m_nPopupTall );
	}
}


//-----------------------------------------------------------------------------
// Purpose: make the popup this big
//-----------------------------------------------------------------------------
void CChromePainter::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect )
{
	if ( m_bPopupVisible )
	{
		// redraw the buffered texture behind the rectangle that was previously composited
		for ( size_t i = 0; i < Q_ARRAYSIZE( m_Texture ); ++i )
		{
			m_Texture[i].MarkDirtyRect( m_nPopupX, m_nPopupY, m_nPopupX + m_nPopupWide, m_nPopupY + m_nPopupTall );
		}
		
		// and notify the main thread to redraw the previously composited area as well
		m_UpdateRect.MarkDirtyRect( m_nPopupX, m_nPopupY, m_nPopupX + m_nPopupWide, m_nPopupY + m_nPopupTall );
	}

	m_bPopupVisible = true;
	m_nPopupX = rect.x;
	m_nPopupWide = rect.width;
	m_nPopupY = rect.y;
	m_nPopupTall = rect.height;

	int m_iBrowser = m_pParent->m_iBrowser;
	CHTMLProtoBufMsg<CMsgSizePopup> cmd( eHTMLCommands_SizePopup );
	cmd.Body().set_x( m_nPopupX );
	cmd.Body().set_y( m_nPopupY );
	cmd.Body().set_wide( m_nPopupWide );
	cmd.Body().set_tall( m_nPopupTall );
	DISPATCH_MESSAGE( eHTMLCommands_SizePopup );
}


//-----------------------------------------------------------------------------
// Purpose: cef has status text for us
//-----------------------------------------------------------------------------
void CClientHandler::OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value, StatusType type)
{
	if ( !value.empty() )
	{
		CHTMLProtoBufMsg<CMsgStatusText> cmd( eHTMLCommands_StatusText );
		cmd.Body().set_text( value.c_str() );
		DISPATCH_MESSAGE( eHTMLCommands_StatusText );
	}
}


//-----------------------------------------------------------------------------
// Purpose: show  tooltip please
//-----------------------------------------------------------------------------
bool CClientHandler::OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text)
{
	if ( !m_bShowingToolTip && !text.empty() )
	{
		m_bShowingToolTip = true;
        m_strToolTip = text.c_str();

		CHTMLProtoBufMsg<CMsgShowToolTip> cmd( eHTMLCommands_ShowToolTip );
		cmd.Body().set_text( m_strToolTip );
		DISPATCH_MESSAGE( eHTMLCommands_ShowToolTip );
	}
	else if ( m_bShowingToolTip && !text.empty() )
	{
        if ( m_strToolTip != text.c_str() )
        {
            m_strToolTip = text.c_str();

		CHTMLProtoBufMsg<CMsgUpdateToolTip> cmd( eHTMLCommands_UpdateToolTip );
            cmd.Body().set_text( m_strToolTip );
		DISPATCH_MESSAGE( eHTMLCommands_UpdateToolTip );
	}
	}
	else if ( m_bShowingToolTip )
	{
		CHTMLProtoBufMsg<CMsgHideToolTip> cmd( eHTMLCommands_HideToolTip );
		DISPATCH_MESSAGE( eHTMLCommands_HideToolTip );

		m_bShowingToolTip = false;
        m_strToolTip.Clear();
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: set the mouse cursor to this image
//-----------------------------------------------------------------------------
bool CChromePainter::OnSetCursor( CefRefPtr<CefBrowser> browser, const CursorType type, const void *pchIconData, int iWide, int iTall, int xHotSpot, int yHotSpot )
{
	int m_iBrowser = m_pParent->m_iBrowser;
	CHTMLProtoBufMsg<CMsgSetCursor> cmd( eHTMLCommands_SetCursor );

	EMouseCursor cursor;
	switch( type )
	{
	case TypeCustom:
		cursor = dc_last;
		break;
	case TypeCross:
		cursor = dc_crosshair;
		break;
	case TypeHand:
		cursor = dc_hand;
		break;
	case TypeIBeam:
		cursor = dc_ibeam;
		break;
	case TypeWait:
		cursor = dc_hourglass;
		break;
	case TypeHelp:
		cursor = dc_help;
		break;
	case TypeEastResize:
		cursor = dc_sizee;
		break;
	case TypeNorthResize:
		cursor = dc_sizen;
		break;
	case TypeNorthEastResize:
		cursor = dc_sizene;
		break;
	case TypeNorthWestResize:
		cursor = dc_sizenw;
		break;
	case TypeSouthResize:
		cursor = dc_sizes;
		break;
	case TypeSouthEastResize:
		cursor = dc_sizese;
		break;
	case TypeSouthWestResize:
		cursor = dc_sizesw;
		break;
	case TypeNorthSouthResize:
		cursor = dc_sizes;
		break;
	case TypeEastWestResize:
		cursor = dc_sizew;
		break;
	case TypeNorthEastSouthWestResize:
		cursor = dc_sizeall;
		break;
	case TypeColumnResize:
		cursor = dc_colresize;
		break;
	case TypeRowResize:
		cursor = dc_rowresize;
		break;
	case TypeMiddlePanning:
		cursor = dc_middle_pan;
		break;
	case TypeEastPanning:
		cursor = dc_east_pan;
		break;
	case TypeNorthPanning:
		cursor = dc_north_pan;
		break;
	case TypeNorthEastPanning:
		cursor = dc_north_east_pan;
		break;
	case TypeNorthWestPanning:
		cursor = dc_north_west_pan;
		break;
	case TypeSouthPanning:
		cursor = dc_south_pan;
		break;
	case TypeSouthEastPanning:
		cursor = dc_south_east_pan;
		break;
	case TypeSouthWestPanning:
		cursor = dc_south_west_pan;
		break;
	case TypeWestPanning:
		cursor = dc_west_pan;
		break;
	case TypeMove:
		cursor = dc_sizeall;
		break;
	case TypeVerticalText:
		cursor = dc_verticaltext;
		break;
	case TypeCell:
		cursor = dc_cell;
		break;
	case TypeContextMenu:
		cursor = dc_none;
		break;
	case TypeAlias:
		cursor = dc_alias;
		break;
	case TypeProgress:
		cursor = dc_waitarrow;
		break;
	case TypeNoDrop:
		cursor = dc_no;
		break;
	case TypeCopy:
		cursor = dc_copycur;
		break;
	case TypeNone:
		cursor = dc_none;
		break;
	case TypeNotAllowed:
		cursor = dc_no;
		break;
	case TypeZoomIn:
		cursor = dc_zoomin;
		break;
	case TypeZoomOut:
		cursor = dc_zoomout;
		break;
	case TypePointer:
	default:
		cursor = dc_arrow;
	}
	cmd.Body().set_cursor( cursor );
	cmd.Body().set_data( (uint32)pchIconData ); // we are relying on chrome keeping around the cursor data after this call completes, it does right now.
	cmd.Body().set_wide( iWide );
	cmd.Body().set_tall( iTall );
	cmd.Body().set_xhotspot( xHotSpot );
	cmd.Body().set_yhotspot( yHotSpot );
	DISPATCH_MESSAGE( eHTMLCommands_SetCursor );
	
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: file open dialog to be shown
//-----------------------------------------------------------------------------
bool CChromePainter::OnFileOpenDialog( CefRefPtr<CefBrowser> browser, bool bMultiSelect, const CefString &default_title, const CefString &default_file, CefWebFileChooserCallback *pCallback  )
{
	if ( !pCallback )
		return true;

	int m_iBrowser = m_pParent->m_iBrowser;
	{
		// scope this so this allocation doesn't stay on the stack during validate
		CHTMLProtoBufMsg<CMsgFileLoadDialog> cmd( eHTMLCommands_FileLoadDialog );
		if ( !default_title.empty() )
			cmd.Body().set_title( default_title );
		if ( !default_file.empty() )
			cmd.Body().set_initialfile( default_file );
		DISPATCH_MESSAGE( eHTMLCommands_FileLoadDialog );
	}

	HTMLCommandBuffer_t *pBuf = g_CEFThread.BWaitForCommand( eHTMLCommands_FileLoadDialogResponse, m_iBrowser );
	if ( pBuf )
	{
		CHTMLProtoBufMsg< CMsgFileLoadDialogResponse > cmd( eHTMLCommands_FileLoadDialogResponse ); 
		if ( cmd.BDeserializeCrossProc( &pBuf->m_Buffer ) ) 
		{
			std::vector<std::wstring> files;
			for ( int i = 0; i < cmd.BodyConst().files_size(); i++ )
			{
				if ( !cmd.BodyConst().files(i).empty() )
				{
				CPathString path( cmd.BodyConst().files(i).c_str() );
				files.push_back( path.GetWCharPathPrePended() );
			}
			}

			// if you have a DEBUG build and are crashing here it is because
			// Chrome is a release library and the std::vector iterator isn't crossing
			// the interface happyily. Build release and you will run fine. 
#if defined(DEBUG) && defined(WIN32)
			Assert( !"File select dialog not available in debug due to STL debug/release issues\n" );
#else
			pCallback->OnFileChoose( files );
#endif			
		}
		g_CEFThread.ReleaseCommandBuffer( pBuf );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: CEF is asking if it can show itself fullscreen
//-----------------------------------------------------------------------------
bool CChromePainter::OnEnterFullScreen( CefRefPtr<CefBrowser> browser  )
{
	int m_iBrowser = m_pParent->m_iBrowser;
	{
		// scope this so this allocation doesn't stay on the stack during validate
		CHTMLProtoBufMsg<CMsgRequestFullScreen> cmd( eHTMLCommands_RequestFullScreen );
		DISPATCH_MESSAGE( eHTMLCommands_RequestFullScreen );
	}

	HTMLCommandBuffer_t *pBuf = g_CEFThread.BWaitForCommand( eHTMLCommands_RequestFullScreenResponse, m_iBrowser );
	if ( pBuf )
	{
		CHTMLProtoBufMsg< CMsgRequestFullScreenResponse > cmd( eHTMLCommands_RequestFullScreenResponse ); 
		if ( cmd.BDeserializeCrossProc( &pBuf->m_Buffer ) ) 
		{
			return cmd.BodyConst().ballow();
		}
		g_CEFThread.ReleaseCommandBuffer( pBuf );
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: cef is spewing to its console, print it
//-----------------------------------------------------------------------------
bool CChromePainter::OnExitFullScreen( CefRefPtr<CefBrowser> browser  )
{
	int m_iBrowser = m_pParent->m_iBrowser;
	{
		// scope this so this allocation doesn't stay on the stack during validate
		// tell the main thread we are exiting fullscreen
		CHTMLProtoBufMsg<CMsgExitFullScreen> cmd( eHTMLCommands_ExitFullScreen );
		DISPATCH_MESSAGE( eHTMLCommands_ExitFullScreen );
	}

	// BUGUBG - add a request/response here so you can disallow leaving fullscreen??
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: cef is spewing to its console, print it
//-----------------------------------------------------------------------------
bool CClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser,
											  const CefString& message,
											  const CefString& source,
											  int line)
{
	// the console is very chatty and doesn't provide useful information for us app developers, just for html/css editors, so lets ignore this for now
	//Msg( "Browser Message: %s - %s:%d\n", message.c_str(), source.c_str(), line );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: helper function to recurvisely search down the DOM for any input nodes and an input button name
//-----------------------------------------------------------------------------
void SearchForInputButtonAndOtherInputs_R( CefRefPtr<CefDOMNode> root, CefRefPtr<CefDOMNode> focusNode, bool &bHasMultipleTextInputNodes, CUtlString &sSearchButtonName, int nMaxRecurse )
{
	CefRefPtr<CefDOMNode> nodeChildren = root->GetFirstChild();
	while ( nodeChildren.get() )
	{
		if ( !nodeChildren->IsSame( focusNode ) && nodeChildren->IsElement() )
		{
			CUtlString sElementType = nodeChildren->GetElementTagName().c_str();
			if ( !Q_stricmp( "input", sElementType  ) )
			{
				CUtlString sChildControlType = nodeChildren->GetFormControlElementType().c_str();
				if ( sSearchButtonName.IsEmpty() && !Q_stricmp( sChildControlType, "submit" ) )
				{
					if ( nodeChildren->HasElementAttribute( "value" ) )
						sSearchButtonName = nodeChildren->GetElementAttribute( "value" ).c_str();
				}
				else if ( !Q_stricmp( "text", sChildControlType  ) || !Q_stricmp( "password", sChildControlType  ) || !Q_stricmp( "email", sChildControlType  ) )
				{
					//CefDOMNode::AttributeMap attrMap;
					//nodeChildren->GetElementAttributes( attrMap );
					if ( !nodeChildren->HasElementAttribute( "disabled" ) )
						bHasMultipleTextInputNodes = true;
				}
			}
			else if ( !Q_stricmp( "textarea", sElementType  ) )
			{
				if ( !nodeChildren->HasElementAttribute( "disabled" ) )
					bHasMultipleTextInputNodes = true;
			}
			else if ( nMaxRecurse > 0 /*&&  !Q_stricmp( "div", sElementType  ) || !Q_stricmp( "tr", sElementType  ) || !Q_stricmp( "td", sElementType  ) || !Q_stricmp( "table", sElementType  ) || !Q_stricmp( "tbody", sElementType  )*/ )
			{
				SearchForInputButtonAndOtherInputs_R( nodeChildren, focusNode, bHasMultipleTextInputNodes, sSearchButtonName, nMaxRecurse - 1 );
			}
		}
		nodeChildren = nodeChildren->GetNextSibling();

		if ( bHasMultipleTextInputNodes && sSearchButtonName.IsValid() )
			break; // if we found both multiple nodes and a search button name we can bail early
	}
}


//-----------------------------------------------------------------------------
// Purpose: a new node in the DOM has focus now
//-----------------------------------------------------------------------------
void CClientHandler::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node)
{
	VPROF_BUDGET( "CCEFThread - CClientHandler::OnFocusedNodeChanged()", VPROF_BUDGETGROUP_VGUI );

	bool bIsInputNode = false;
	CUtlString sElementType;
	if ( node.get() )
		sElementType = node->GetElementTagName().c_str();

	CUtlString sName;
	if ( node.get() )
		sName = node->GetName().c_str();

	CUtlString sSearchButtonName;
	CUtlString sControlType;
	bool bInputNode = !Q_stricmp( "input", sElementType  );
	bool bHasMultipleInputNodes = false;
	if ( sElementType.IsValid() && ( bInputNode || !Q_stricmp( "textarea", sElementType ) ) )
	{
		sControlType = node->GetFormControlElementType().c_str();
		
		// lets go searching for the submit button and grab the text it shows
		bIsInputNode = true;
		CefRefPtr<CefDOMNode> nodeParent = node->GetParent();
		while( nodeParent.get() )
		{
			CUtlString sParentElementType = nodeParent->GetElementTagName().c_str();
			if ( !Q_stricmp( "form", sParentElementType ) )
				break;
			nodeParent = nodeParent->GetParent();
		}

		if ( nodeParent.get() )
			SearchForInputButtonAndOtherInputs_R( nodeParent, node, bHasMultipleInputNodes, sSearchButtonName, 32 );
	}

	if ( sElementType.IsValid() && !Q_stricmp( "div", sElementType ) )
	{
		if ( node->HasElementAttribute( "contenteditable" ) )
			bIsInputNode = true;
	}

	if ( sSearchButtonName.IsEmpty() )
		sSearchButtonName = "#Web_FormSubmit";

	CHTMLProtoBufMsg<CMsgNodeHasFocus> cmd( eHTMLCommands_NodeGotFocus );
	cmd.Body().set_binput( bIsInputNode );
	if ( sName.IsValid() )
		cmd.Body().set_name( sName );
	if ( sElementType.IsValid() )
		cmd.Body().set_elementtagname( sElementType );
	if ( sSearchButtonName.IsValid() )
		cmd.Body().set_searchbuttontext( sSearchButtonName );
	cmd.Body().set_bhasmultipleinputs( bHasMultipleInputNodes );
	if ( sControlType.IsValid() )
		cmd.Body().set_input_type( sControlType );
	
	DISPATCH_MESSAGE( eHTMLCommands_NodeGotFocus );
}


//-----------------------------------------------------------------------------
// Purpose: a find has found matches on the page, feed back that info
//-----------------------------------------------------------------------------
void CClientHandler::OnFindResult(CefRefPtr<CefBrowser> browser,
												int identifier,
												int count,
												const CefRect& selectionRect,
												int activeMatchOrdinal,
												bool finalUpdate)
{
	CHTMLProtoBufMsg<CMsgSearchResults> cmd( eHTMLCommands_SearchResults );
	cmd.Body().set_activematch( activeMatchOrdinal );
	cmd.Body().set_results( count );
	DISPATCH_MESSAGE( eHTMLCommands_SearchResults );
}


//-----------------------------------------------------------------------------
// Purpose: return a pointer to the CEF browser object
//-----------------------------------------------------------------------------
CefRefPtr<CefBrowser> CClientHandler::GetBrowser()
{
	return m_Browser;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientHandler::CloseBrowser()
{
	if ( m_Browser )
	{
		m_bBrowserClosing = true;
		m_Browser->CloseBrowser();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClientHandler::SetMouseLocation( int nMouseX, int nMouseY )
{
	m_nMouseX = nMouseX; 
	m_nMouseY = nMouseY;
	if ( m_Browser.get() )
	{
		m_nMouseScrolledX = nMouseX + m_Browser->VerticalScroll();
		m_nMouseScrolledY = nMouseY + m_Browser->HorizontalScroll();
		m_bMouseFocus = true;
		m_Browser->SendMouseMoveEvent( m_nMouseX, m_nMouseY, false );
}
}

//-----------------------------------------------------------------------------
// Purpose: check if window has scrolled and generate a fake mouse move event
// to force CEF to check for hover state changes (seems like a CEF bug...)
//-----------------------------------------------------------------------------
void CClientHandler::RefreshCEFHoverStatesAfterScroll()
{
	if ( m_Browser.get() && m_bMouseFocus )
	{
		int nScrolledX = m_nMouseX + m_Browser->VerticalScroll();
		int nScrolledY = m_nMouseY + m_Browser->HorizontalScroll();
		if ( nScrolledX != m_nMouseScrolledX || nScrolledY != m_nMouseScrolledY )
		{
			m_nMouseScrolledX = nScrolledX;
			m_nMouseScrolledY = nScrolledY;
			m_Browser->SendMouseMoveEvent( m_nMouseX, m_nMouseY, false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: make the user agent for this browser
//-----------------------------------------------------------------------------
void CClientHandler::SetBrowserAgent( 	CefRefPtr<CefBrowser> browser )
{
	static bool bGotIEVersion = false;
	static char rgchWindowsVersion[64] = "Windows NT 5.1"; // XP SP1
	static char *rgchOS = "";

	if ( !bGotIEVersion )
	{
#ifdef WIN32
		rgchOS = "Windows";
		// First get windows version
		OSVERSIONINFO verInfo;
		memset( &verInfo, 0, sizeof(OSVERSIONINFO) );
		verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		if ( ::GetVersionEx( &verInfo ) )
		{
			// We only run on "Windows NT" os's, so we just need to append the major/minor version dynamically
			Q_snprintf( rgchWindowsVersion, sizeof(rgchWindowsVersion), "Windows NT %u.%u", verInfo.dwMajorVersion, verInfo.dwMinorVersion );
			//Log( "Windows Version is: %u.%u\n", verInfo.dwMajorVersion, verInfo.dwMinorVersion );
		}

#elif defined(OSX)
		Q_snprintf( rgchWindowsVersion, sizeof(rgchWindowsVersion), "Macintosh" );
		rgchOS = "Macintosh";
#elif defined(LINUX)
		Q_snprintf( rgchWindowsVersion, sizeof(rgchWindowsVersion), "X11" );// strange, but that's what Firefox uses
		rgchOS = "Linux";
#endif
	}

	// user agent is process wide for Chrome so you can only set it once and it appplies to everything you open
	{
		char szAgent[ 2048 ];
		Q_snprintf( szAgent, sizeof(szAgent), m_strUserAgent.String(), rgchOS, rgchWindowsVersion, m_strUserAgentIdentifier.String(), sm_ulBuildID, m_szUserAgentExtras );
		browser->SetUserAgent( szAgent );
	}
}


//-----------------------------------------------------------------------------
// Purpose: paint the cef control
//-----------------------------------------------------------------------------
void CClientHandler::Paint()
{
	Lock();
	if ( m_Browser )
	{
		m_bPendingPaint |= m_Painter.BUpdated();
		m_Painter.SetUpdated( false );
	}
	Unlock();
}


//-----------------------------------------------------------------------------
// Purpose: did we have a paint that updated the texture buffer?
//-----------------------------------------------------------------------------
bool CClientHandler::BNeedsPaint()
{
	bool bVal = m_bPendingPaint;
	m_bPendingPaint = false;
	return bVal;
}


//-----------------------------------------------------------------------------
// Purpose: get the texture data for the control
//-----------------------------------------------------------------------------
const byte *CClientHandler::PComposedTextureData( uint32 iTexture )
{ 
	VPROF_BUDGET( "CClientHandler::PTextureData", VPROF_BUDGETGROUP_VGUI );

	return m_Painter.PComposedTextureData( iTexture );
	}


//-----------------------------------------------------------------------------
// Purpose: true if something valid has rendered (i.e not the blank page)
//-----------------------------------------------------------------------------
bool CClientHandler::IsVisuallyNonEmpty()
{
	if ( m_Browser )
		return m_Browser->IsVisuallyNonEmpty();

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: set the loc strings to display on error
//-----------------------------------------------------------------------------
void CClientHandler::SetErrorStrings( const char *pchTitle, const char *pchHeader, const char *pchCacheMiss, const char *pchBadURL, const char *pchConnectionProblem,
					 const char *pchProxyProblem, const char *pchUnknown )
{
	m_sErrorTitle = pchTitle;
	m_sErrorHeader = pchHeader;
	m_sErrorCacheMiss = pchCacheMiss;
	m_sErrorBadURL = pchBadURL;
	m_sErrorConnectionProblem = pchConnectionProblem;
	m_sErrorProxyProblem = pchProxyProblem;
	m_sErrorUnknown = pchUnknown;
}


//-----------------------------------------------------------------------------
// Purpose: the user wants us to take a screenshot of a page
//-----------------------------------------------------------------------------
void CClientHandler::RequestScreenShot( const CHTMLProtoBufMsg<CMsgSavePageToJPEG> &cmd )
{
	m_Snapshot.m_sURLSnapshot = cmd.BodyConst().url().c_str();
	m_Snapshot.m_sFileNameSnapshot = cmd.BodyConst().filename().c_str();
	m_Snapshot.m_nWide = cmd.BodyConst().width();
	m_Snapshot.m_nTall = cmd.BodyConst().height();
	m_Snapshot.m_flRequestTimeout = Plat_FloatTime() + k_flMaxScreenshotWaitTime;
}


//-----------------------------------------------------------------------------
// Purpose: save a screenshot of the current html page to this file with this size
//-----------------------------------------------------------------------------
void CClientHandler::SavePageToJPEGIfNeeded( CefRefPtr<CefBrowser> browser, const byte *pRGBA, int wide, int tall )
{
	if ( m_Snapshot.m_sURLSnapshot.IsValid() && wide && tall && ( browser->GetMainFrame()->GetURL().size() > 0 ) )
	{
		if ( m_Snapshot.m_sURLSnapshot == CStrAutoEncode( browser->GetMainFrame()->GetURL().c_str() ).ToString() )
		{
			VPROF_BUDGET( "CClientHandler::SavePageToJPEGIfNeeded", VPROF_BUDGETGROUP_TENFOOT );

			CUtlBuffer bufRGB;

			bufRGB.Put( pRGBA, wide * tall *4 );
			if ( !BConvertRGBAToRGB( bufRGB, wide, tall ) )
				return;

			BResizeImageRGB( bufRGB, wide, tall, m_Snapshot.m_nWide, m_Snapshot.m_nTall );
			// input format is actually BGRA so now swizzle to rgb
			byte *pBGR = (byte *)bufRGB.Base();
			for ( int i = 0; i < m_Snapshot.m_nTall; i++ )
			{
				for ( int j = 0; j < m_Snapshot.m_nWide; j++ )
				{
					char cR = pBGR[0];
					pBGR[0] = pBGR[2];
					pBGR[2] = cR;
					pBGR += 3;
				}
			}

			if ( !ConvertRGBToJpeg( m_Snapshot.m_sFileNameSnapshot, k_ScreenshotQuality, m_Snapshot.m_nWide, m_Snapshot.m_nTall, bufRGB ) )
				return;
		
			CHTMLProtoBufMsg<CMsgSavePageToJPEGResponse> cmd( eHTMLCommands_SavePageToJPEGResponse );
			cmd.Body().set_url( m_Snapshot.m_sURLSnapshot );
			cmd.Body().set_filename( m_Snapshot.m_sFileNameSnapshot );
			DISPATCH_MESSAGE( eHTMLCommands_SavePageToJPEGResponse );		

			m_Snapshot.m_sURLSnapshot.Clear();
			m_Snapshot.m_flRequestTimeout = 0.0f;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: return the url located at this position if there is one
//-----------------------------------------------------------------------------
void CCEFThread::ThreadLinkAtPosition( const CHTMLProtoBufMsg<CMsgLinkAtPosition> &htmlCommand )
{
	CefString pchURL;
	int iClient = 0;
    int bLiveLink = false;
    int bInput = false;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{ 
		if ( m_listClientHandlers[iClient]->GetBrowser() )
			 m_listClientHandlers[iClient]->GetBrowser()->GetLinkAtPosition( htmlCommand.BodyConst().x(), htmlCommand.BodyConst().y(),
                                                                                    pchURL, bLiveLink, bInput);
	}

	CHTMLProtoBufMsg<CMsgLinkAtPositionResponse> cmd( eHTMLCommands_LinkAtPositionResponse );
	cmd.Body().set_x( htmlCommand.BodyConst().x() );
	cmd.Body().set_y( htmlCommand.BodyConst().y() );
	cmd.Body().set_blivelink( bLiveLink>0 ? true: false );
	cmd.Body().set_binput( bInput>0 ? true: false );
	if ( !pchURL.empty() )
		cmd.Body().set_url( pchURL );
	int m_iBrowser = htmlCommand.BodyConst().browser_handle();
	DISPATCH_MESSAGE( eHTMLCommands_LinkAtPositionResponse );
}


//-----------------------------------------------------------------------------
// Purpose: zoom the screen to the element at this position
//-----------------------------------------------------------------------------
void CCEFThread::ThreadZoomToElementAtPosition( const CHTMLProtoBufMsg<CMsgZoomToElementAtPosition> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{ 
		if ( m_listClientHandlers[iClient]->GetBrowser() )
		{
			CefRect initialRect, finalRect;
			float zoomLevel = m_listClientHandlers[iClient]->GetBrowser()->scalePageToFitElementAt(
																				htmlCommand.BodyConst().x(), htmlCommand.BodyConst().y(),
                                                                                initialRect, finalRect );
			int m_iBrowser = htmlCommand.BodyConst().browser_handle();
			ThreadBrowserVerticalScrollBarSizeHelper( m_iBrowser, true );
			ThreadBrowserHorizontalScrollBarSizeHelper( m_iBrowser, true );
			{
				CHTMLProtoBufMsg<CMsgZoomToElementAtPositionResponse> cmd( eHTMLCommands_ZoomToElementAtPositionResponse );
				cmd.Body().set_zoom( zoomLevel );
				cmd.Body().set_initial_x( initialRect.x );
				cmd.Body().set_initial_y( initialRect.y );
				cmd.Body().set_initial_width( initialRect.width );
				cmd.Body().set_initial_height( initialRect.height );
				cmd.Body().set_final_x( finalRect.x );
				cmd.Body().set_final_y( finalRect.y );
				cmd.Body().set_final_width( finalRect.width );
				cmd.Body().set_final_height( finalRect.height );
				DISPATCH_MESSAGE( eHTMLCommands_ZoomToElementAtPositionResponse );
			}
		}
	}
			}


//-----------------------------------------------------------------------------
// Purpose: zoom the screen to the element at this position
//-----------------------------------------------------------------------------
void CCEFThread::ThreadZoomToFocusedElement( const CHTMLProtoBufMsg<CMsgZoomToFocusedElement> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{ 
		if ( m_listClientHandlers[iClient]->GetBrowser() )
		{
			CefRect initialRect, finalRect;
			float zoomLevel = m_listClientHandlers[iClient]->GetBrowser()->scalePageToFocusedElement( htmlCommand.BodyConst().leftoffset(), htmlCommand.BodyConst().topoffset(), initialRect, finalRect );
			int m_iBrowser = htmlCommand.BodyConst().browser_handle();
			ThreadBrowserVerticalScrollBarSizeHelper( m_iBrowser, true );
			ThreadBrowserHorizontalScrollBarSizeHelper( m_iBrowser, true );
			{
				CHTMLProtoBufMsg<CMsgZoomToElementAtPositionResponse> cmd( eHTMLCommands_ZoomToElementAtPositionResponse );
				cmd.Body().set_zoom( zoomLevel );
				cmd.Body().set_initial_x( initialRect.x );
				cmd.Body().set_initial_y( initialRect.y );
				cmd.Body().set_initial_width( initialRect.width );
				cmd.Body().set_initial_height( initialRect.height );
				cmd.Body().set_final_x( finalRect.x );
				cmd.Body().set_final_y( finalRect.y );
				cmd.Body().set_final_width( finalRect.width );
				cmd.Body().set_final_height( finalRect.height );
				DISPATCH_MESSAGE( eHTMLCommands_ZoomToElementAtPositionResponse );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: increment the scale factor on the page by an increment
//-----------------------------------------------------------------------------
void CCEFThread::ThreadSetPageScale( const  CHTMLProtoBufMsg<CMsgScalePageToValue> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{ 
		CClientHandler *pHandler = m_listClientHandlers[iClient];
		if ( pHandler->GetBrowser() )
		{
			int nPageHeightBefore = pHandler->GetBrowser()->VerticalScrollMax();
			int nPageWidthBefore = pHandler->GetBrowser()->HorizontalScrollMax();

			float zoomLevel = pHandler->GetBrowser()->setPageScaleFactor( htmlCommand.BodyConst().scale(), htmlCommand.BodyConst().x(), htmlCommand.BodyConst().y() );

			int idx = m_mapSizeChangesPending.Find( htmlCommand.BodyConst().browser_handle() );
			if ( idx == m_mapSizeChangesPending.InvalidIndex() )
			{
				SizeChange_t &sizeChange = m_mapSizeChangesPending [ m_mapSizeChangesPending.Insert( htmlCommand.BodyConst().browser_handle() ) ];
				sizeChange.iBrowser = htmlCommand.BodyConst().browser_handle();
				sizeChange.nBeforeHeight = nPageHeightBefore;
				sizeChange.nBeforeWidth = nPageWidthBefore;
				sizeChange.flNewZoom = zoomLevel;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: fire off the details of any page scale changes we had pending from last frame
//-----------------------------------------------------------------------------
void CCEFThread::SendSizeChangeEvents()
{
	FOR_EACH_MAP_FAST( m_mapSizeChangesPending, i )
	{
		int iClient = 0;
		if ( BIsValidBrowserHandle( m_mapSizeChangesPending[i].iBrowser, iClient ) )
		{ 
			CClientHandler *pHandler = m_listClientHandlers[iClient];
			if ( pHandler->GetBrowser() )
			{
				int nPageHeightAfter = pHandler->GetBrowser()->VerticalScrollMax();
				int nPageWidthAfter = pHandler->GetBrowser()->HorizontalScrollMax();

				int m_iBrowser = m_mapSizeChangesPending[i].iBrowser;

				ThreadBrowserVerticalScrollBarSizeHelper( m_iBrowser, true );
				ThreadBrowserHorizontalScrollBarSizeHelper( m_iBrowser, true );
				{
					CHTMLProtoBufMsg<CMsgScalePageToValueResponse> cmd( eHTMLCommands_ScaleToValueResponse );
					cmd.Body().set_zoom( m_mapSizeChangesPending[i].flNewZoom );
					cmd.Body().set_width_delta( nPageWidthAfter - m_mapSizeChangesPending[i].nBeforeWidth );
					cmd.Body().set_height_delta( nPageHeightAfter - m_mapSizeChangesPending[i].nBeforeHeight );
					DISPATCH_MESSAGE( eHTMLCommands_ScaleToValueResponse );
				}
			}
		}
	}
	m_mapSizeChangesPending.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: exit from fullscreen if in it
//-----------------------------------------------------------------------------
void CCEFThread::ThreadExitFullScreen( const  CHTMLProtoBufMsg<CMsgExitFullScreen> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		m_listClientHandlers[ iClient ]->GetBrowser()->ExitFullScreen();
	}

}


//-----------------------------------------------------------------------------
// Purpose: the user has requested we save this url to a file on local disk
//-----------------------------------------------------------------------------
void CCEFThread::ThreadSavePageToJPEG( const CHTMLProtoBufMsg<CMsgSavePageToJPEG> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		m_listClientHandlers[ iClient ]->RequestScreenShot( htmlCommand );
	}
}


//-----------------------------------------------------------------------------
// Purpose: mouse moved to this x,y on the page
//-----------------------------------------------------------------------------
void CCEFThread::ThreadMouseMove( const CHTMLProtoBufMsg<CMsgMouseMove> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		m_listClientHandlers[ iClient ]->SetMouseLocation( htmlCommand.BodyConst().x(), htmlCommand.BodyConst().y() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: mouse left the control, tell cef
//-----------------------------------------------------------------------------
void CCEFThread::ThreadMouseLeave( const CHTMLProtoBufMsg<CMsgMouseLeave> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;

		m_listClientHandlers[ iClient ]->SetMouseFocus( false );

		int mx, my;
		m_listClientHandlers[ iClient ]->GetMouseLocation( mx, my );
		browser->SendMouseMoveEvent( mx, my, true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: helper to convert UI mouse codes to CEF ones
//-----------------------------------------------------------------------------
CefBrowser::MouseButtonType ConvertMouseCodeToCEFCode( int code )
{
	// BUGBUG
	switch( code )
	{
	case 0:
		return MBT_LEFT;
		break;
	case 1:
		return MBT_RIGHT;
		break;
	case 2:
		return MBT_MIDDLE;
		break;
	default:
		return MBT_LEFT;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: mouse button pressed
//-----------------------------------------------------------------------------
void CCEFThread::ThreadMouseButtonDown( const CHTMLProtoBufMsg<CMsgMouseDown> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{

		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;
		int nMouseX, nMouseY;
		m_listClientHandlers[ iClient ]->GetMouseLocation( nMouseX, nMouseY );

		browser->SendMouseClickEvent( nMouseX, nMouseY, ConvertMouseCodeToCEFCode( htmlCommand.BodyConst().mouse_button() ), false, 1 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: mouse button released
//-----------------------------------------------------------------------------
void CCEFThread::ThreadMouseButtonUp( const CHTMLProtoBufMsg<CMsgMouseUp> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;
		int nMouseX, nMouseY;
		m_listClientHandlers[ iClient ]->GetMouseLocation( nMouseX, nMouseY );

		browser->SendMouseClickEvent( nMouseX, nMouseY, ConvertMouseCodeToCEFCode( htmlCommand.BodyConst().mouse_button() ), true, 1 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: mouse button double pressed
//-----------------------------------------------------------------------------
void CCEFThread::ThreadMouseButtonDlbClick( const CHTMLProtoBufMsg<CMsgMouseDblClick> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;
		int nMouseX, nMouseY;
		m_listClientHandlers[ iClient ]->GetMouseLocation( nMouseX, nMouseY );

		browser->SendMouseClickEvent( nMouseX, nMouseY, ConvertMouseCodeToCEFCode( htmlCommand.BodyConst().mouse_button() ), false, 2 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: mouse was wheeled
//-----------------------------------------------------------------------------
void CCEFThread::ThreadMouseWheel( const CHTMLProtoBufMsg<CMsgMouseWheel> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;
		int nMouseX, nMouseY;
		m_listClientHandlers[ iClient ]->GetMouseLocation( nMouseX, nMouseY );

		browser->SendMouseWheelEvent( nMouseX, nMouseY, 0, htmlCommand.BodyConst().delta() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: unicode character was typed
//-----------------------------------------------------------------------------
void CCEFThread::ThreadKeyTyped( const CHTMLProtoBufMsg<CMsgKeyChar> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;

		CefKeyInfo keyInfo;
#ifdef OSX
		keyInfo.character = htmlCommand.BodyConst().unichar();
#else
		keyInfo.key = htmlCommand.BodyConst().unichar();
#endif
		browser->SendKeyEvent( KT_CHAR, keyInfo, 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: raw key was pressed
//-----------------------------------------------------------------------------
void CCEFThread::ThreadKeyDown( const CHTMLProtoBufMsg<CMsgKeyDown> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;

		CefKeyInfo keyInfo;
#ifdef OSX
		keyInfo.keyCode = htmlCommand.BodyConst().keycode();
#else
		keyInfo.key = htmlCommand.BodyConst().keycode();
#endif
		browser->SendKeyEvent( KT_KEYDOWN, keyInfo, htmlCommand.BodyConst().modifiers() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: raw key was released
//-----------------------------------------------------------------------------
void CCEFThread::ThreadKeyUp( const CHTMLProtoBufMsg<CMsgKeyUp> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;

		CefKeyInfo keyInfo;
#ifdef OSX
		keyInfo.keyCode = htmlCommand.BodyConst().keycode();
#else
		keyInfo.key = htmlCommand.BodyConst().keycode();
#endif
		browser->SendKeyEvent( KT_KEYUP, keyInfo, htmlCommand.BodyConst().modifiers() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: please close any fullscreen flash controls you see
//-----------------------------------------------------------------------------
void CCEFThread::ThreadCloseFullScreenFlashIfOpen( const CHTMLProtoBufMsg<CMsgCloseFullScreenFlashIfOpen> &htmlCommand )
{
	CheckForFullScreenFlashControl();
	if ( m_bFullScreenFlashVisible )
	{
#ifdef WIN32
		::PostMessageA( m_flashfullscreenHWND, WM_KEYDOWN, VK_ESCAPE, 0 );
		::PostMessageA( m_flashfullscreenHWND, WM_KEYUP, VK_ESCAPE, 0 );
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: please close any fullscreen flash controls you see
//-----------------------------------------------------------------------------
void CCEFThread::ThreadPauseFullScreenFlashMovieIfOpen( const CHTMLProtoBufMsg<CMsgPauseFullScreenFlashMovieIfOpen> &htmlCommand )
{
	CheckForFullScreenFlashControl();
	if ( m_bFullScreenFlashVisible )
	{
#ifdef WIN32
		::PostMessageA( m_flashfullscreenHWND, WM_KEYDOWN, VK_SPACE, 0 );
		::PostMessageA( m_flashfullscreenHWND, WM_KEYUP, VK_SPACE, 0 );
#endif
	}
}



//-----------------------------------------------------------------------------
// Purpose: helper class to get the focused node in the dom
//-----------------------------------------------------------------------------
class CVisitor :  public CefDOMVisitor 
{
public:
	CVisitor( CThreadEvent *pEvent, CUtlString *psValue ) 
	{
		m_pEvent = pEvent;
		m_psValue = psValue;
	}

	~CVisitor()
	{
		m_pEvent->Set();
	}

	virtual void Visit(CefRefPtr<CefDOMDocument> document) {
		CefRefPtr<CefDOMNode> focusedNode = document->GetFocusedNode();
		*m_psValue = focusedNode->GetValue().c_str();
	}


private:
	CThreadEvent *m_pEvent;
	CUtlString *m_psValue;

	IMPLEMENT_REFCOUNTING(CVisitor);
};


//-----------------------------------------------------------------------------
// Purpose: get the text out of the current dom field that has focus
//-----------------------------------------------------------------------------
void CCEFThread::ThreadGetFocusedNodeText( const CHTMLProtoBufMsg<CMsgFocusedNodeText> &htmlCommand )
{
	int iClient = 0;
	if ( BIsValidBrowserHandle( htmlCommand.BodyConst().browser_handle(), iClient ) )
	{
		CefRefPtr<CefBrowser> browser = m_listClientHandlers[ iClient ]->GetBrowser();
		if ( !browser.get() )
			return;

		CThreadEvent event;
		CUtlString sValue;
		browser->GetFocusedFrame()->VisitDOM( new CVisitor( &event, &sValue ) );
		do 
		{
			CefDoMessageLoopWork();
		}
		while ( !event.Wait( 100 ) ); // keep pumping CEF until it has done our walk

		{
			int m_iBrowser = htmlCommand.BodyConst().browser_handle();
			CHTMLProtoBufMsg<CMsgFocusedNodeTextResponse> cmd( eHTMLCommands_GetFocusedNodeValueResponse );
			cmd.Body().set_value( sValue );
			DISPATCH_MESSAGE( eHTMLCommands_GetFocusedNodeValueResponse );
		}
	}
}
