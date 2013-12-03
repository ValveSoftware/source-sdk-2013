//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//=============================================================================//

#ifndef IHTMLCHROME_H
#define IHTMLCHROME_H

#ifdef _WIN32
#pragma once
#endif

#include "htmlmessages.h"
#include "tier1/utlbuffer.h"

// prototypes for all the protobuf messages, so we don't need to include all of protobuf
class CMsgBrowserReady;
class CMsgNeedsPaint;
class CMsgStartRequest;
class CMsgURLChanged;
class CMsgFinishedRequest;
class CMsgShowPopup;
class CMsgHidePopup;
class CMsgOpenNewTab;
class CMsgPopupHTMLWindow;
class CMsgSetHTMLTitle;
class CMsgLoadingResource;
class CMsgStatusText;
class CMsgSetCursor;
class CMsgFileLoadDialog;
class CMsgShowToolTip;
class CMsgUpdateToolTip;
class CMsgHideToolTip;
class CMsgSearchResults;
class CMsgClose;
class CMsgHorizontalScrollBarSizeResponse;
class CMsgVerticalScrollBarSizeResponse;
class CMsgGetZoomResponse;
class CMsgLinkAtPositionResponse;
class CMsgZoomToElementAtPositionResponse;
class CMsgJSAlert;
class CMsgJSConfirm;
class CMsgCanGoBackAndForward;
class CMsgOpenSteamURL;
class CMsgSizePopup;
class CMsgScalePageToValueResponse;
class CMsgRequestFullScreen;
class CMsgExitFullScreen;
class CMsgGetCookiesForURLResponse;
class CMsgNodeHasFocus;
class CMsgSavePageToJPEGResponse;
class CMsgFocusedNodeTextResponse;

//-----------------------------------------------------------------------------
// Purpose: a single IPC packet for the html thread (in and out)
//-----------------------------------------------------------------------------
struct HTMLCommandBuffer_t
{
	EHTMLCommands m_eCmd;
	int m_iBrowser;
	CUtlBuffer m_Buffer;
#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const tchar *pchName )
	{
		VALIDATE_SCOPE();
		ValidateObj( m_Buffer );
	}
#endif
};


//-----------------------------------------------------------------------------
// Purpose: callback interfaces for messages from the html thread
//-----------------------------------------------------------------------------
class IHTMLResponses
{
public:
	virtual ~IHTMLResponses() {} 

	virtual void BrowserSetIndex( int idx ) = 0;
	virtual int  BrowserGetIndex() = 0;
	virtual void BrowserReady( const CMsgBrowserReady *pCmd ) = 0;
	virtual void BrowserNeedsPaint( const CMsgNeedsPaint *pCmd ) = 0;
	virtual void BrowserStartRequest( const CMsgStartRequest *pCmd ) = 0;
	virtual void BrowserURLChanged( const CMsgURLChanged *pCmd ) = 0;
	virtual void BrowserFinishedRequest( const CMsgFinishedRequest *pCmd ) = 0;
	virtual void BrowserShowPopup( const CMsgShowPopup *pCmd ) = 0;
	virtual void BrowserHidePopup( const CMsgHidePopup *pCmd ) = 0;
	virtual void BrowserOpenNewTab( const CMsgOpenNewTab *pCmd ) = 0;
	virtual void BrowserPopupHTMLWindow( const CMsgPopupHTMLWindow *pCmd ) = 0;
	virtual void BrowserSetHTMLTitle( const CMsgSetHTMLTitle *pCmd ) = 0;
	virtual void BrowserLoadingResource( const CMsgLoadingResource *pCmd ) = 0;
	virtual void BrowserStatusText( const CMsgStatusText *pCmd ) = 0;
	virtual void BrowserSetCursor( const CMsgSetCursor *pCmd ) = 0;
	virtual void BrowserFileLoadDialog( const CMsgFileLoadDialog *pCmd ) = 0;
	virtual void BrowserShowToolTip( const CMsgShowToolTip *pCmd ) = 0;
	virtual void BrowserUpdateToolTip( const CMsgUpdateToolTip *pCmd ) = 0;
	virtual void BrowserHideToolTip( const CMsgHideToolTip *pCmd ) = 0;
	virtual void BrowserSearchResults( const CMsgSearchResults *pCmd ) = 0;
	virtual void BrowserClose( const CMsgClose *pCmd ) = 0;
	virtual void BrowserHorizontalScrollBarSizeResponse( const CMsgHorizontalScrollBarSizeResponse *pCmd ) = 0;
	virtual void BrowserVerticalScrollBarSizeResponse( const CMsgVerticalScrollBarSizeResponse *pCmd ) = 0;
	virtual void BrowserGetZoomResponse( const CMsgGetZoomResponse *pCmd ) = 0;
	virtual void BrowserLinkAtPositionResponse( const CMsgLinkAtPositionResponse *pCmd ) = 0;
	virtual void BrowserZoomToElementAtPositionResponse( const CMsgZoomToElementAtPositionResponse *pCmd ) = 0;
	virtual void BrowserJSAlert( const CMsgJSAlert *pCmd ) = 0;
	virtual void BrowserJSConfirm( const CMsgJSConfirm *pCmd ) = 0;
	virtual void BrowserCanGoBackandForward( const CMsgCanGoBackAndForward *pCmd ) = 0;
	virtual void BrowserOpenSteamURL( const CMsgOpenSteamURL *pCmd ) = 0;
	virtual void BrowserSizePopup( const CMsgSizePopup *pCmd ) = 0;
	virtual void BrowserScalePageToValueResponse( const CMsgScalePageToValueResponse *pCmd ) = 0;
	virtual void BrowserRequestFullScreen( const CMsgRequestFullScreen *pCmd ) = 0;
	virtual void BrowserExitFullScreen( const CMsgExitFullScreen *pCmd ) = 0;
	virtual void BrowserGetCookiesForURLResponse( const CMsgGetCookiesForURLResponse *pCmd ) = 0;
	virtual void BrowserNodeGotFocus( const CMsgNodeHasFocus *pCmd ) = 0;
	virtual void BrowserSavePageToJPEGResponse( const CMsgSavePageToJPEGResponse *pCmd ) = 0;
	virtual void BrowserFocusedNodeValueResponse( const CMsgFocusedNodeTextResponse *pCmd ) = 0;
};

#endif // IHTMLCHROME_H
