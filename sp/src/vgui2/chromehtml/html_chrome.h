//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//=============================================================================//

#ifndef HTML_CHROME_H
#define HTML_CHROME_H

#ifdef _WIN32
#pragma once
#endif

#include "cef/include/cef_render_handler.h"
#include "cef/include/cef_client.h"
#include "cef/include/cef_app.h"
#include "cef/include/cef_browser.h"
#include "cef/include/cef_command_line.h"
#include "cef/include/cef_frame.h"
#include "cef/include/cef_runnable.h"
#include "cef/include/cef_web_urlrequest.h"
#include "cef/include/cef_request_handler.h"
#include "cef/include/cef_load_handler.h"
#include "cef/include/cef_display_handler.h"
#include "cef/include/cef_life_span_handler.h"
#include "cef/include/cef_render_handler.h"

#include "tier0/platform.h"
#include "tier0/vprof.h"
#include "tier1/utlarray.h"
#include "tier1/utlbuffer.h"
#include "fileio.h"
//#include "constants.h"
#include "tier0/validator.h"
#include "tier1/utlmap.h"
#include "tier1/utlstring.h"
#include "tier0/tslist.h"
#include "html/ihtmlchrome.h"
#include "html/htmlprotobuf.h"
// These must be undefed so that the MacOS build will work -- their STL
// fails if min and max are defined. I'm undefing them in all builds to
// avoid having a Windows dependency on min/max creep in.
#undef min
#undef max
#include "htmlmessages.pb.h"

class CClientHandler;
class CChromePainter;
class ICookieCallback;

bool ChromeSetWebCookie( const char *pchHostname, const char *pchName, const char *pchValue, const char *pchPath, RTime32 nExpires );
bool ChromeGetWebCookiesForURL( CUtlString *pstrValue, const char *pchURL, const char *pchName );

//-----------------------------------------------------------------------------
// Purpose: track dirty rects, shuttle bits between renderer and main thread
//-----------------------------------------------------------------------------
class CChromeUpdateRegion
{
public:
	CChromeUpdateRegion() { MarkAllClean(); }
	int GetUpdateX( int clampWide ) const { return clamp( m_nUpdateX0, 0, clampWide ); }
	int GetUpdateY( int clampTall ) const { return clamp( m_nUpdateY0, 0, clampTall ); }
	int GetUpdateWide( int clampWide ) const { return clamp( m_nUpdateX1, 0, clampWide ) - GetUpdateX( clampWide ); }
	int GetUpdateTall( int clampTall ) const { return clamp( m_nUpdateY1, 0, clampTall ) - GetUpdateY( clampTall ); }

	void MarkAllClean() { m_nUpdateX0 = m_nUpdateY0 = INT_MAX; m_nUpdateX1 = m_nUpdateY1 = 0; }
	void MarkAllDirty() { m_nUpdateX0 = m_nUpdateY0 = 0; m_nUpdateX1 = m_nUpdateY1 = INT_MAX; }
	void MarkDirtyRect( int x0, int y0, int x1, int y1 )
	{
		if ( x0 >= x1 || y0 >= y1 ) return;
		if ( m_nUpdateX0 > x0 ) m_nUpdateX0 = x0;
		if ( m_nUpdateY0 > y0 ) m_nUpdateY0 = y0;
		if ( m_nUpdateX1 < x1 ) m_nUpdateX1 = x1;
		if ( m_nUpdateY1 < y1 ) m_nUpdateY1 = y1;
	}
	void MarkDirtyRect( const CChromeUpdateRegion& other )
	{
		MarkDirtyRect( other.m_nUpdateX0, other.m_nUpdateY0, other.m_nUpdateX1, other.m_nUpdateY1 );
	}

protected:
	int m_nUpdateX0;
	int m_nUpdateY0;
	int m_nUpdateX1;
	int m_nUpdateY1;
};

class CChromeRenderBuffer : public CChromeUpdateRegion
{
public:
	CChromeRenderBuffer() { m_nWide = m_nTall = 0; }
	void SetSize( int wide, int tall )
	{
		if ( wide != m_nWide || tall != m_nTall )
		{
			m_nWide = wide;
			m_nTall = tall;
			MarkAllDirty();
			m_Texture.EnsureCapacity( wide * tall * 4 );
		}
	}
	int GetWide() const { return m_nWide; }
	int GetTall() const { return m_nTall; }
	byte *GetPixels() { return m_Texture.Base(); }

	bool BUpdatePixels( byte *pOther, int wide, int tall )
	{
		SetSize( wide, tall );
		int x0 = clamp( m_nUpdateX0, 0, wide );
		int y0 = clamp( m_nUpdateY0, 0, tall );
		int x1 = clamp( m_nUpdateX1, 0, wide );
		int y1 = clamp( m_nUpdateY1, 0, tall );
		if ( x0 >= x1 || y0 >= y1 )
			return false;

		if ( x0 == 0 && x1 == wide )
		{
			byte *pDst = GetPixels() + y0*wide*4;
			byte *pSrc = pOther + y0*wide*4;
			Q_memcpy( pDst, pSrc, wide * ( y1 - y0 ) * 4 );
		}
		else
		{
			byte *pDst = GetPixels() + y0*wide*4 + x0*4;
			byte *pSrc = pOther + y0*wide*4 + x0*4;
			int nCopyBytesPerRow = (x1 - x0)*4;
			for ( int rows = y1 - y0; rows > 0; --rows )
			{
				Q_memcpy( pDst, pSrc, nCopyBytesPerRow );
				pSrc += wide * 4;
				pDst += wide * 4;
			}
		}
		return true;
	}

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		ValidateObj( m_Texture );
	}
#endif

protected:
	int m_nWide; // dimensions of the buffer
	int m_nTall;
	CUtlVector<byte> m_Texture; // rgba data
};

//-----------------------------------------------------------------------------
// Purpose: interface Chrome uses to send up paint messages
//-----------------------------------------------------------------------------
class CChromePainter : public CefRenderHandler
{
public:
	CChromePainter( CClientHandler *pParent );
	~CChromePainter();

	virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) OVERRIDE { return false; }
	virtual bool GetScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) OVERRIDE { return false; }
	virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) OVERRIDE { return false; }

	virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) OVERRIDE; 
	virtual void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) OVERRIDE;

	virtual void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer) OVERRIDE;
	virtual void OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor) OVERRIDE {}
	virtual bool OnSetCursor( CefRefPtr<CefBrowser> browser, const CursorType type, const void *pchIconData, int iWide, int iTall, int xHotSpot, int yHotSpot ) OVERRIDE;
	virtual bool OnFileOpenDialog( CefRefPtr<CefBrowser> browser, bool bMultiSelect, const CefString &default_title, const CefString &default_file, CefWebFileChooserCallback *pCallback  ) OVERRIDE;
	virtual bool OnEnterFullScreen( CefRefPtr<CefBrowser> browser  ) OVERRIDE;
	virtual bool OnExitFullScreen( CefRefPtr<CefBrowser> browser  ) OVERRIDE;
	
	bool BUpdated();
	void SetUpdated( bool state );

	int GetPopupTall() const { return m_nPopupTall; }
	int GetPopupWide() const { return m_nPopupWide; }
	int GetPopupX() const { return m_nPopupX; }
	int GetPopupY() const { return m_nPopupY; }
	const byte *PPopupTextureData() 
	{
		return m_PopupTexture.Base();
	}

	uint32 FlipTexture();
	void SetTextureUploaded( uint32 id );
	bool BPaintBufferAvailable();

	byte *PComposedTextureData( uint32 iTexture );
	int GetTall() const { return m_MainTexture.GetTall(); }
	int GetWide() const { return m_MainTexture.GetWide(); }
	int GetUpdateX() const { return m_UpdateRect.GetUpdateX( GetWide() ); }
	int GetUpdateY() const { return m_UpdateRect.GetUpdateY( GetTall() ); }
	int GetUpdateWide() const { return m_UpdateRect.GetUpdateWide( GetWide() ); }
	int GetUpdateTall() const { return m_UpdateRect.GetUpdateTall( GetTall() ); }
	void ClearUpdateRect() { m_UpdateRect.MarkAllClean(); }

	// XXX not shuttled safely between threads, should use real buffering
	const byte *PPopupTextureDataCached(); 

	bool BPopupVisible() const { return m_bPopupVisible; }
    // WebKit will show popups before they have been given a size and painted and we may do
    // a drawing pass during that time, so we want to make sure to only do something
    // with the popup when it has an actual size and content.
    bool BPopupVisibleAndPainted() const
    {
        return m_bPopupVisible &&
            m_nPopupWide != 0 &&
            m_nPopupTall != 0 &&
            m_PopupTexture.Count() >= m_nPopupWide * m_nPopupTall * 4;
    }
	void PopupRect( int &x, int &y, int &wide, int &tall ) const { x = m_nPopupX; y = m_nPopupY; wide = m_nPopupWide; tall = m_nPopupTall; }

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		for ( int i = 0; i < Q_ARRAYSIZE(m_Texture); i++ )
			ValidateObj( m_Texture[i] );

		ValidateObj( m_PopupTextureCache );
		ValidateObj( m_PopupTexture );
		ValidateObj( m_MainTexture );
	}
#endif
	IMPLEMENT_REFCOUNTING(CChromePainter);

private:
	uint32 m_iNextTexture;
	uint32 m_iTexturesInFlightBits;
	int m_nPopupWide;
	int m_nPopupTall;
	CChromeRenderBuffer m_MainTexture;
	CChromeRenderBuffer m_Texture[2]; // buffering -- XXX should use atomic swap, not push multiple buffers to command buffer
	CChromeUpdateRegion m_UpdateRect;
	CUtlVector<byte> m_PopupTextureCache;
	CUtlVector<byte> m_PopupTexture;
	bool m_bUpdated;
	CClientHandler *m_pParent;
	bool m_bPopupVisible;
	int m_nPopupX, m_nPopupY;
};



//-----------------------------------------------------------------------------
// Purpose: CEF callback object
//-----------------------------------------------------------------------------
class CClientHandler : public CefClient,
						public CefLifeSpanHandler,
						public CefLoadHandler,
						public CefRequestHandler,
						public CefDisplayHandler,
						
						public CefJSDialogHandler,
						public CefFindHandler,
						public CefMenuHandler,
						public CefFocusHandler,
						public CefPermissionHandler
{
public:
	CClientHandler( int iBrowser, const char *pchUserAgent, uint16 nSerial );
	~CClientHandler();

	// CefClient methods
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE {
		return &m_Painter;
	}

	virtual CefRefPtr<CefFocusHandler> GetFocusHandler() OVERRIDE {
		return this;
	}

	virtual CefRefPtr<CefMenuHandler> GetMenuHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefPermissionHandler> GetPermissionHandler() OVERRIDE {
		return this;
	}

	virtual CefRefPtr<CefFindHandler> GetFindHandler() OVERRIDE {
		return this;
	}
	virtual CefRefPtr<CefJSDialogHandler> GetJSDialogHandler() OVERRIDE {
		return this;
	}


	// CefLifeSpanHandler methods

	virtual bool OnBeforePopup(CefRefPtr<CefBrowser> parentBrowser, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, const CefString& url, CefRefPtr<CefClient>& client, CefBrowserSettings& settings) OVERRIDE;
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
	virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser)  OVERRIDE;
	virtual bool DoClose(CefRefPtr<CefBrowser> browser) { return false; }
	virtual bool OnNewTab(CefRefPtr<CefBrowser> parentBrowser, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, const CefString& url, bool bForeground, CefRefPtr<CefClient>& client, CefBrowserSettings& settings );

	// CefLoadHandler methods
	virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, bool bIsNewNavigation ) OVERRIDE;

	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode, CefRefPtr<CefRequest> request ) OVERRIDE;
	virtual bool OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& failedUrl, CefString& errorText);

	// CefRequestHandler methods
	virtual bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, NavType navType, bool isRedirect, bool isNewTabRequest  );
	virtual bool OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefRequest> request, CefString& redirectUrl, CefRefPtr<CefStreamReader>& resourceStream, CefRefPtr<CefResponse> response, int loadFlags);

	// CefDisplayHandler methods
	virtual void OnNavStateChange(CefRefPtr<CefBrowser> browser, bool canGoBack, bool canGoForward) OVERRIDE {}
	virtual void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) OVERRIDE { } 
	virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) OVERRIDE;

	virtual bool OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text);
	virtual void OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value, StatusType type);
	virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString& message, const CefString& source, int line);

	virtual void OnTakeFocus(CefRefPtr<CefBrowser> browser, bool next) {}
	virtual bool OnSetFocus(CefRefPtr<CefBrowser> browser, FocusSource source) { return true; }
	virtual void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node);

	virtual bool OnBeforeMenu(CefRefPtr<CefBrowser> browser, const CefMenuInfo& menuInfo) {	return true; }
	virtual void GetMenuLabel(CefRefPtr<CefBrowser> browser, MenuId menuId, CefString& label) {}
	virtual bool OnMenuAction(CefRefPtr<CefBrowser> browser, MenuId menuId) { return true; }
	virtual bool OnBeforeScriptExtensionLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& extensionName) { return false;	}

	virtual void OnFindResult(CefRefPtr<CefBrowser> browser, int identifier, int count, const CefRect& selectionRect, int activeMatchOrdinal, bool finalUpdate);
	virtual bool OnJSAlert(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& message);
	virtual bool OnJSConfirm(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& message, bool& retval);
	virtual bool OnJSPrompt(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& message, const CefString& defaultValue, bool& retval, CefString& result);

	void FileOpenDialogResult( const char *pchFileName );


	// paint helpers
	void Paint();
	bool BNeedsPaint();

	int GetTextureTall() const { return m_Painter.GetTall(); }
	int GetTextureWide() const { return m_Painter.GetWide(); }
	int GetUpdateX() const { return m_Painter.GetUpdateX(); }
	int GetUpdateY() const { return m_Painter.GetUpdateY(); }
	int GetUpdateWide() const { return m_Painter.GetUpdateWide(); }
	int GetUpdateTall() const { return m_Painter.GetUpdateTall(); }

	const byte *PPopupTextureDataCached() { return m_Painter.PPopupTextureDataCached(); }
	bool BPopupVisible() const { return m_Painter.BPopupVisible(); }
    bool BPopupVisibleAndPainted() const { return m_Painter.BPopupVisibleAndPainted(); }
	void PopupRect( int &x, int &y, int &wide, int &tall ) const { m_Painter.PopupRect( x, y, wide, tall ); }

	const byte *PComposedTextureData( uint32 iTexture );
	uint32 FlipTexture() { return m_Painter.FlipTexture(); }
	void SetTextureUploaded( uint32 iTexture ) { m_Painter.SetTextureUploaded( iTexture ); }
	void ClearUpdateRect() { m_Painter.ClearUpdateRect(); }
	bool BPaintBufferReady() { return m_Painter.BPaintBufferAvailable(); }

	// Helpers
	CefRefPtr<CefBrowser> GetBrowser();
	void CloseBrowser();
	uint32 GetPageSerial() { return m_nPageSerial; }
	void SetPendingPageSerial( uint32 nPageSerial ) 
	{ 
		m_nPendingPageSerial = nPageSerial; 
	}


	void SetUserAgent( const char *pchAgent ) { Q_strncpy( m_szUserAgentExtras, pchAgent, sizeof(m_szUserAgentExtras) ); }
	const char *PchCurrentURL() { return m_strCurrentUrl.String(); }
	bool IsVisuallyNonEmpty();
	void SetErrorStrings( const char *pchTitle, const char *pchHeader, const char *pchCacheMiss, const char *pchBadURL, const char *pchConnectionProblem,
							const char *pchProxyProblem, const char *pchUnknown );
	void SetMouseLocation( int x, int y );
	void GetMouseLocation( int &x, int &y ) { x = m_nMouseX; y = m_nMouseY; }
	void SetMouseFocus( bool bFocus ) { m_bMouseFocus = bFocus; }
	void SetSize( int wide, int tall ) { m_nExpectedWide = wide; m_nExpectedTall = tall; }
	void GetExpectedSize( int &wide, int &tall ) { wide = m_nExpectedWide; tall =m_nExpectedTall; }

	//-----------------------------------------------------------------------------
	// Purpose: Adds a custom header to all requests
	//-----------------------------------------------------------------------------
	void AddHeader( const char *pchHeader, const char *pchValue )
	{
		m_vecHeaders.AddToTail( std::make_pair( CStrAutoEncode( pchHeader ).ToWString(), CStrAutoEncode( pchValue ).ToWString() ) );
	}

	void RequestScreenShot( const CHTMLProtoBufMsg<CMsgSavePageToJPEG> &cmd );
	void SavePageToJPEGIfNeeded( CefRefPtr<CefBrowser> browser, const byte *pRGBA, int wide, int tall );
	bool BPendingScreenShot() 
	{
		return m_Snapshot.m_flRequestTimeout > 0.0f && m_Snapshot.m_sURLSnapshot.IsValid() && m_Snapshot.m_flRequestTimeout < Plat_FloatTime(); 
	}

	void SetUserAgentIdentifier( const char *pchIdent ) 
	{ 
		m_strUserAgentIdentifier = pchIdent; 
	}

	uint16 NSerial() { return m_nSerial; }
	
	void RefreshCEFHoverStatesAfterScroll();

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
		ValidateObj( m_strCurrentUrl );
		ValidateObj( m_strPostData );
		ValidateObj( m_strLastRedirectURL );
		ValidateObj( m_vecHeaders );
		ValidateObj( m_strUserAgent );
		ValidateObj( m_Painter );
		ValidateObj( m_sErrorTitle );
		ValidateObj( m_sErrorHeader );
		ValidateObj( m_sErrorCacheMiss );
		ValidateObj( m_sErrorBadURL );
		ValidateObj( m_sErrorConnectionProblem );
		ValidateObj( m_sErrorProxyProblem );
		ValidateObj( m_sErrorUnknown );
		ValidateObj( m_strUserAgentIdentifier );
		//		ValidateObj( m_vecHCursor );
	}
#endif

	IMPLEMENT_REFCOUNTING(CClientHandler);
	IMPLEMENT_LOCKING(CClientHandler);
private:
	friend class CChromePainter;
	friend class CCEFThread;

	void SetBrowserAgent( 	CefRefPtr<CefBrowser> browser );

	// The child browser window
	CefRefPtr<CefBrowser> m_Browser;

	CefRenderHandler::CefWebFileChooserCallback *m_pOpenFileCallback;

	uint16 m_nSerial;
	char m_szUserAgentExtras[ 256 ];
	CUtlString m_strCurrentUrl;
	CUtlString m_strPostData;
	CUtlString m_strLastRedirectURL;
	CUtlString m_strUserAgent;
	CUtlString m_strUserAgentIdentifier;
    CUtlString m_strToolTip;
	bool m_bShowingToolTip;
	bool m_bPendingPaint;
	bool m_bBrowserClosing;
	bool m_bMouseFocus;
	CChromePainter m_Painter;
	CUtlVector< std::pair< std::wstring, std::wstring > > m_vecHeaders;
	int m_iBrowser;
	int m_nExpectedWide;
	int m_nExpectedTall;
	uint32 m_nPageSerial;
	uint32 m_nPendingPageSerial;

	CUtlString m_sErrorTitle;
	CUtlString m_sErrorHeader;
	CUtlString m_sErrorCacheMiss;
	CUtlString m_sErrorBadURL;
	CUtlString m_sErrorConnectionProblem;
	CUtlString m_sErrorProxyProblem;
	CUtlString m_sErrorUnknown;
	int m_nMouseX;
	int m_nMouseY;
	int m_nMouseScrolledX;
	int m_nMouseScrolledY;

	struct SnapshotData_t
	{
		CUtlString m_sURLSnapshot;
		CUtlString m_sFileNameSnapshot;
		int m_nWide, m_nTall;
		float m_flRequestTimeout;
	};

	SnapshotData_t m_Snapshot;

	// Scroll bar state is cached and compared to CEF's current values every frame;
	// any changes are passed on to the client handler as serialized update messages.
	struct CachedScrollBarState_t
	{
		int m_nX;
		int m_nY;
		int m_nWide;
		int m_nTall;
		int m_nMax;
		int m_nScroll;
		int m_bVisible; // using 'int' to avoid padding bytes so memcmp can be used
	};
	CachedScrollBarState_t m_CachedVScroll;
	CachedScrollBarState_t m_CachedHScroll;
};



//-----------------------------------------------------------------------------
// Purpose: CEF thinking thread
//-----------------------------------------------------------------------------
class CCEFThread : public CValidatableThread
{
public:
	CCEFThread();
	~CCEFThread();
	void SetCEFPaths( const char *pchHTMLCacheDir, const char *pchCookiePath );
	void TriggerShutdown();
	void WakeThread();
	virtual int Run();

	bool BHasPendingMessages() { return m_tslResponseBuffers.Count() > 0; }

	HTMLCommandBuffer_t *GetFreeCommandBuffer( EHTMLCommands eCmd, int iBrowser );
	void ReleaseCommandBuffer( HTMLCommandBuffer_t *pBuf );
	void PushCommand( HTMLCommandBuffer_t * );
	void PushResponse( HTMLCommandBuffer_t * );
	bool GetMainThreadCommand( HTMLCommandBuffer_t ** );
	HTMLCommandBuffer_t *BWaitForCommand( EHTMLCommands eCmd, int iBrowser );
	HTMLCommandBuffer_t *BWaitForResponse( EHTMLCommands eCmd, int iBrowser );
	bool GetCEFThreadCommand( HTMLCommandBuffer_t **pBuf );

#ifdef DBGFLAG_VALIDATE
	virtual void SleepForValidate() { CValidatableThread::SleepForValidate(); m_evWaitingForCommand.Set(); WakeThread(); }
	virtual void Validate( CValidator &validator, const tchar *pchName );
#endif


private:
	void RunCurrentCommands();
	const char *PchWebkitUserAgent();
	void AppGetSettings(CefSettings& settings, CefRefPtr<CefApp>& app);
	void CleanupTempFolders();

	void ThreadCreateBrowser( const CHTMLProtoBufMsg<CMsgBrowserCreate> &htmlCommand );
	void ThreadRemoveBrowser( const CHTMLProtoBufMsg<CMsgBrowserRemove> &htmlCommand );
	void ThreadBrowserSize( const CHTMLProtoBufMsg<CMsgBrowserSize> &htmlCommand );
	void ThreadBrowserPosition( const CHTMLProtoBufMsg<CMsgBrowserPosition> &htmlCommand );
	void ThreadBrowserPostURL( const CHTMLProtoBufMsg<CMsgPostURL> &htmlCommand );
	void ThreadBrowserStopLoad( const CHTMLProtoBufMsg<CMsgStopLoad> &htmlCommand );
	void ThreadBrowserReload( const CHTMLProtoBufMsg<CMsgReload> &htmlCommand );
	void ThreadBrowserGoForward( const CHTMLProtoBufMsg<CMsgGoForward> &htmlCommand );
	void ThreadBrowserGoBack( const CHTMLProtoBufMsg<CMsgGoBack> &htmlCommand );
	void ThreadBrowserCopy( const CHTMLProtoBufMsg<CMsgCopy> &htmlCommand );
	void ThreadBrowserPaste( const CHTMLProtoBufMsg<CMsgPaste> &htmlCommand );
	void ThreadBrowserExecuteJavascript( const CHTMLProtoBufMsg<CMsgExecuteJavaScript> &htmlCommand );
	void ThreadBrowserSetFocus( const CHTMLProtoBufMsg<CMsgSetFocus> &htmlCommand );
	void ThreadBrowserHorizontalScrollBarSize( const CHTMLProtoBufMsg<CMsgHorizontalScrollBarSize> &htmlCommand );
	void ThreadBrowserVerticalScrollBarSize( const CHTMLProtoBufMsg<CMsgVerticalScrollBarSize> &htmlCommand );
	void ThreadBrowserFind( const CHTMLProtoBufMsg<CMsgFind> &htmlCommand );
	void ThreadBrowserStopFind( const CHTMLProtoBufMsg<CMsgStopFind> &htmlCommand );
	void ThreadBrowserSetHorizontalScroll( const CHTMLProtoBufMsg<CMsgSetHorizontalScroll> &htmlCommand );
	void ThreadBrowserSetVerticalScroll( const CHTMLProtoBufMsg<CMsgSetVerticalScroll> &htmlCommand );
	void ThreadBrowserSetZoomLevel( const CHTMLProtoBufMsg<CMsgSetZoomLevel> &htmlCommand );
	void ThreadBrowserViewSource( const CHTMLProtoBufMsg<CMsgViewSource> &htmlCommand );
	void ThreadNeedsPaintResponse( const CHTMLProtoBufMsg<CMsgNeedsPaintResponse> &htmlCommand );
	void ThreadBrowserErrorStrings( const CHTMLProtoBufMsg<CMsgBrowserErrorStrings> &htmlCommand );
	void ThreadAddHeader( const CHTMLProtoBufMsg<CMsgAddHeader> &htmlCommand );
	void ThreadGetZoom( const CHTMLProtoBufMsg<CMsgGetZoom> &htmlCommand );
	void ThreadHidePopup( const CHTMLProtoBufMsg<CMsgHidePopup> &htmlCommand );
	void ThreadGetCookiesForURL( const CHTMLProtoBufMsg<CMsgGetCookiesForURL> &htmlCommand );

	void ThreadKeyDown( const CHTMLProtoBufMsg<CMsgKeyDown> &htmlCommand );
	void ThreadKeyUp( const CHTMLProtoBufMsg<CMsgKeyUp> &htmlCommand );
	void ThreadKeyTyped( const CHTMLProtoBufMsg<CMsgKeyChar> &htmlCommand );
	void ThreadMouseButtonDown( const CHTMLProtoBufMsg<CMsgMouseDown> &htmlCommand );
	void ThreadMouseButtonUp( const CHTMLProtoBufMsg<CMsgMouseUp> &htmlCommand );
	void ThreadMouseButtonDlbClick( const CHTMLProtoBufMsg<CMsgMouseDblClick> &htmlCommand );
	void ThreadMouseWheel( const CHTMLProtoBufMsg<CMsgMouseWheel> &htmlCommand );
	void ThreadMouseMove( const CHTMLProtoBufMsg<CMsgMouseMove> &htmlCommand );
	void ThreadMouseLeave( const CHTMLProtoBufMsg<CMsgMouseLeave> &htmlCommand );
	void ThreadLinkAtPosition( const CHTMLProtoBufMsg<CMsgLinkAtPosition> &htmlCommand );
	void ThreadZoomToElementAtPosition( const CHTMLProtoBufMsg<CMsgZoomToElementAtPosition> &htmlCommand );
	void ThreadZoomToFocusedElement( const CHTMLProtoBufMsg<CMsgZoomToFocusedElement> &htmlCommand );
	void ThreadSavePageToJPEG( const CHTMLProtoBufMsg<CMsgSavePageToJPEG> &htmlCommand );
	void ThreadSetCookie( const CHTMLProtoBufMsg<CMsgSetCookie> &htmlCommand );
	void ThreadSetTargetFrameRate( const  CHTMLProtoBufMsg<CMsgSetTargetFrameRate> &htmlCommand );
	void ThreadFullRepaint( const  CHTMLProtoBufMsg<CMsgFullRepaint> &htmlCommand );
	void ThreadSetPageScale( const  CHTMLProtoBufMsg<CMsgScalePageToValue> &htmlCommand );
	void ThreadExitFullScreen( const  CHTMLProtoBufMsg<CMsgExitFullScreen> &htmlCommand );
	void ThreadCloseFullScreenFlashIfOpen( const  CHTMLProtoBufMsg<CMsgCloseFullScreenFlashIfOpen> &htmlCommand );
	void ThreadPauseFullScreenFlashMovieIfOpen( const  CHTMLProtoBufMsg<CMsgPauseFullScreenFlashMovieIfOpen> &htmlCommand );
	void ThreadGetFocusedNodeText( const CHTMLProtoBufMsg<CMsgFocusedNodeText> &htmlCommand );

	void ThreadBrowserVerticalScrollBarSizeHelper( int iBrowser, bool bForceSendUpdate );
	void ThreadBrowserHorizontalScrollBarSizeHelper( int iBrowser, bool bForceSendUpdate );

	bool BIsValidBrowserHandle( uint32 nHandle, int &iClient );
	void SendSizeChangeEvents();
	void CheckForFullScreenFlashControl();

	CThreadEvent m_WakeEvent;
	CUtlString m_sHTMLCacheDir;
	CUtlString m_sCookiePath;
	bool m_bExit;
	CThreadEvent m_eventDidExit;

	CUtlLinkedList<CClientHandler *> m_listClientHandlers;

	CTSQueue<HTMLCommandBuffer_t*> m_tslCommandBuffers;
	CUtlVector<HTMLCommandBuffer_t*> m_vecQueueCommands;
	CTSQueue<HTMLCommandBuffer_t*> m_tslResponseBuffers;
	CUtlVector<HTMLCommandBuffer_t*> m_vecQueueResponses;

	CTSList<HTMLCommandBuffer_t*> m_tslUnsedBuffers;

	CThreadEvent m_evWaitingForCommand;
	CThreadEvent m_evWaitingForResponse;
	int m_nTargetFrameRate;
	uint16 m_nBrowserSerial;
	bool m_bFullScreenFlashVisible;
#ifdef WIN32
	HWND m_flashfullscreenHWND;
#endif
	bool m_bSawUserInputThisFrame;

	struct SizeChange_t
	{
		int iBrowser;
		int nBeforeWidth;
		int nBeforeHeight;
		float flNewZoom;
	};
	CUtlMap< int, SizeChange_t, int > m_mapSizeChangesPending;
};

CCEFThread &AccessHTMLWrapper();

#endif // HTML_CHROME_H
