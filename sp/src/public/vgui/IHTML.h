//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IHTML_H
#define IHTML_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>
#include <vgui/Cursor.h>
#include <vgui/IImage.h>

namespace vgui
{

	//-----------------------------------------------------------------------------
	// Purpose: basic interface for a HTML window
	//-----------------------------------------------------------------------------
	class IHTML
	{
	public:
		// open a new page
		virtual void OpenURL(const char *)=0;

		// stops the existing page from loading
		virtual bool StopLoading()=0;

		// refreshes the current page
		virtual bool Refresh()=0;

		// display the control -- deprecated !! Use SetVisible() instead!
		virtual bool Show(bool shown)=0;

		// return the currently opened page
		virtual const char *GetOpenedPage()=0;

		// called when the browser needs to be resized
		virtual void Obsolete_OnSize(int x,int y, int w,int h)=0;

		// returns the width and height (in pixels) of the HTML full page (not just the displayed region)
		virtual void GetHTMLSize(int &wide,int &tall) = 0;


		// clear the text in an existing control
		virtual void Clear()=0;

		// add text to the browser control (as a HTML formated string)
		virtual void AddText(const char *text)=0;

		enum MOUSE_STATE { UP,DOWN,MOVE,DBLCLICK };
		// unused functions we keep around so the vtable layout is binary compatible
		virtual void Obsolete_OnMouse(MouseCode code,MOUSE_STATE s,int x,int y)=0;
		virtual void Obsolete_OnChar(wchar_t unichar)=0;
		virtual void Obsolete_OnKeyDown(KeyCode code)=0;

		virtual vgui::IImage *GetBitmap()=0;
		virtual void SetVisible( bool state ) = 0;


		virtual void SetSize( int wide,int tall )=0;

		virtual void OnMouse(MouseCode code,MOUSE_STATE s,int x,int y, bool bPopupMenuMenu )=0;
		virtual void OnChar(wchar_t unichar, bool bPopupMenu)=0;
		virtual void OnKeyDown(KeyCode code, bool bPopupMenu)=0;

		virtual void ScrollV( int nPixels ) = 0;
		virtual void ScrollH( int nPixels ) = 0;
		virtual void OnMouseWheeled( int delta, bool bPopupMenu )= 0;

		// called when the browser needs to be resized
		virtual void OnKeyUp(KeyCode code, bool bPopupMenu)=0;


		// open a URL with the provided POST data (which can be much larger than the max URL of 512 chars)
		// NOTE - You CANNOT have get params (i.e a "?" ) in pchURL if pchPostData is set (due to an IE bug)
		virtual void PostURL( const char *pchURL, const char *pchPostData ) = 0;

		// Run javascript within the browser control
		virtual void RunJavascript( const char *pchScript ) = 0;

		virtual void SetMousePosition( int x, int y, bool bPopupMenu ) = 0;

		virtual void SetUserAgentInfo( const wchar_t *pwchUserAgent ) = 0;

		// can't add custom headers to IE
		virtual void AddHeader( const char *pchHeader, const char *pchValue ) = 0;	

		virtual void SetFileDialogChoice( const char *pchFileName ) = 0;

		// we are hiding the popup, so make sure webkit knows
		virtual void HidePopup() = 0;
		virtual void SetHTMLFocus() = 0;
		virtual void KillHTMLFocus() = 0;
		// ask webkit about the size of any scrollbars it wants to render
		virtual void HorizontalScrollBarSize( int &x, int &y, int &wide, int &tall) = 0;
		virtual void VerticalScrollBarSize( int &x, int &y, int &wide, int &tall) = 0;
		virtual int HorizontalScroll() = 0;
		virtual int VerticalScroll() = 0;
		virtual int HorizontalScrollMax() =0;
		virtual int VerticalScrollMax() =0;
		virtual bool IsHorizontalScrollBarVisible() =0;
		virtual bool IsVeritcalScrollBarVisible() =0;
		virtual void SetHorizontalScroll( int scroll ) =0;
		virtual void SetVerticalScroll( int scroll ) =0;
		virtual void ViewSource() = 0;
		virtual void Copy() = 0;
		virtual void Paste() = 0;

		// IE specific calls
		virtual bool IsIERender() = 0;
		virtual void GetIDispatchPtr( void **pIDispatch ) = 0;
		virtual void GetHTMLScroll( int &top, int &left ) = 0;
	};


	//-----------------------------------------------------------------------------
	// Purpose: possible load errors when you open a url in the web browser
	//-----------------------------------------------------------------------------
	enum EWebPageLoadError
	{
		eLoadErrorNone = 0,
		eMimeTypeNotSupported, // probably trying to download an exe or something
		eCacheMiss, // Usually caused by navigating to a page with POST data via back or forward buttons
		eBadURL, // bad url passed in (invalid hostname, malformed)
		eConnectionProblem, // network connectivity problem, server offline or user not on internet
		eProxyConnectionProblem, // User is configured to use proxy, but we can't use it

		eLoadErrorUnknown, // not a load type we classify right now, check out cef_handler_errorcode_t for the full list we could translate
	};


	//-----------------------------------------------------------------------------
	// Purpose: basic callback interface for a HTML window
	//-----------------------------------------------------------------------------
	class IHTMLEvents
	{
	public:
		// unused functions we keep around so the vtable layout is binary compatible
		virtual bool Obsolete_OnStartURL(const char *url, const char *target, bool first)=0;
		virtual void Obsolete_OnFinishURL(const char *url)=0;
		virtual void Obsolete_OnProgressURL(long current, long maximum)=0;
		virtual void Obsolete_OnSetStatusText(const char *text) =0;
		virtual void Obsolete_OnUpdate() =0;
		virtual void Obsolete_OnLink()=0;
		virtual void Obsolete_OffLink()=0;
	
		// call backs for events
		// when the top level browser is changing the page they are looking at (not sub iframes or the like loading)
		virtual void OnURLChanged( const char *url, const char *pchPostData, bool bIsRedirect ) = 0;
		// the control has finished loading a request, could be a sub request in the page
		virtual void OnFinishRequest( const char *url, const char *pageTitle ) = 0;

		// the lower html control wants to load a url, do we allow it?
		virtual bool OnStartRequestInternal( const char *url, const char *target, const char *pchPostData, bool bIsRedirect ) = 0;

		// show a popup menu for this html control
		virtual void ShowPopup( int x, int y, int wide, int tall ) = 0;
		// hide any popup menu you are showing
		virtual void HidePopup() = 0;
		// show an external html window at this position and side
		virtual bool OnPopupHTMLWindow( const char *pchURL, int x, int y, int wide, int tall ) = 0;
		// the browser is telling us the title it would like us to show
		virtual void SetHTMLTitle( const char *pchTitle ) = 0;
		// the browser is loading a sub url for a page, usually an image or css
		virtual void OnLoadingResource( const char *pchURL ) = 0;
		// the browser is telling us the user is hovering a url or the like 
		virtual void OnSetStatusText(const char *text) =0;
		// the browser wants the cursor changed please
		virtual void OnSetCursor( vgui::CursorCode cursor ) = 0;
		// the browser wants to ask the user to select a local file and tell it about it
		virtual void OnFileLoadDialog( const char *pchTitle, const char *pchInitialFile ) = 0;
		// show and hide tooltip text
		virtual void OnShowToolTip( const char *pchText ) = 0;
		virtual void OnUpdateToolTip( const char *pchText ) = 0;
		virtual void OnHideToolTip() = 0;


		// IE only code
		virtual bool BOnCreateNewWindow( void **ppDispatch ) = 0;
		virtual void OnLink()=0;
		virtual void OffLink()=0;
		virtual void OnCloseWindow() = 0;
		virtual void OnUpdate() =0;
		virtual void OnProgressRequest(long current, long maximum)=0;

		// new Chrome calls
		virtual bool OnOpenNewTab( const char *pchURL, bool bForeground ) = 0;
	};


}

#endif // IHTML_H
