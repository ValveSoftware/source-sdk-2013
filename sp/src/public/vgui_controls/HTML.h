//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Creates a HTML control
//
// $NoKeywords: $
//=============================================================================//

#ifndef HTML_H
#define HTML_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/IImage.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/TextEntry.h>
#include <tier1/utlmap.h>
#ifndef VERSION_SAFE_STEAM_API_INTERFACES
#define VERSION_SAFE_STEAM_API_INTERFACES
#endif
#include "steam/steam_api.h"

class HTMLComboBoxHost;
namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Control to display HTML content
//			This control utilises a hidden IE window to render a HTML page for you.
//			It can load any valid URL (i.e local files or web pages), you cannot dynamically change the
//			content however (internally to the control that is).
//-----------------------------------------------------------------------------
class HTML: public Panel
{
	DECLARE_CLASS_SIMPLE( HTML, Panel );
	// TODO::STYLE
	//DECLARE_STYLE_BASE( "HTML" );
public:

	HTML(Panel *parent,const char *name, bool allowJavaScript = false, bool bPopupWindow = false);
	~HTML();

	// IHTML pass through functions
	virtual void OpenURL( const char *URL, const char *pchPostData, bool bForce = false );
	virtual bool StopLoading();
	virtual bool Refresh();
	virtual void OnMove();
	virtual void RunJavascript( const char *pchScript );
	virtual void GoBack();
	virtual void GoForward();
	virtual bool BCanGoBack();
	virtual bool BCanGoFoward();

	// event functions you can override and specialize behavior of
	virtual bool OnStartRequest( const char *url, const char *target, const char *pchPostData, bool bIsRedirect );
	virtual void OnFinishRequest(const char *url, const char *pageTitle, const CUtlMap < CUtlString, CUtlString > &headers ) {}
	virtual void OnSetHTMLTitle( const char *pchTitle ) {}
	virtual void OnLinkAtPosition( const char *pchURL ) {}
	virtual void OnURLChanged( const char *url, const char *pchPostData, bool bIsRedirect ) {}

	virtual bool OnOpenNewTab( const char *pchURL, bool bForeground ) { return false; }

	// configuration
	virtual void SetScrollbarsEnabled(bool state);
	virtual void SetContextMenuEnabled(bool state);
	virtual void SetViewSourceEnabled( bool state );
	virtual void NewWindowsOnly( bool state );

	bool IsScrolledToBottom();
	bool IsScrollbarVisible();

	// url handlers, lets you have web page links fire vgui events
	// use to have custom web page links, eg. "steam://open/subscriptionpage"
	// message contains "CustomURL", "url"
	virtual void AddCustomURLHandler(const char *customProtocolName, vgui::Panel *target);

	// overridden to paint our special web browser texture
	virtual void Paint();
	
	// pass messages to the texture component to tell it about resizes
	virtual void OnSizeChanged(int wide,int tall);

	// pass mouse clicks through
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnCursorMoved(int x,int y);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyCodeReleased(KeyCode code);
	virtual void PerformLayout();
	virtual void OnMouseWheeled(int delta);
	virtual void PostChildPaint();

	/* message posting:

		"HTMLSliderMoved"	- indicates the scrollbar has moved

		"OnURLChanged"		- indicates a new URL is being loaded
		    "url"
			"postdata"

		"OnFinishRequest"		- indicates all url loaded has completed

		"HTMLBackRequested"		- mouse4 has been pressed on the dialog
		"HTMLForwardRequested"  - mouse5 has been pressed on the dialog

		"SecurityStatus"		- indicates the SSL status of the page (disabled,good,bad)
			"url"
			"secure" - true if an ssl page
			"certerror" - true if there is a cert error loading the page
			"isevcert" - true if this is an EV style cert
			"certname" - name of the entity this cert is issued to
	*/

	MESSAGE_FUNC_INT( OnSetCursorVGUI, "SetCursor", cursor );

	virtual void OnCommand( const char *pchCommand );

	void AddHeader( const char *pchHeader, const char *pchValue );
	void OnKillFocus();
	void OnSetFocus();
	
	void Find( const char *pchSubStr );
	void StopFind();
	void FindNext();
	void FindPrevious();
	void ShowFindDialog();
	void HideFindDialog();
	bool FindDialogVisible();
	int HorizontalScrollMax() { return m_scrollHorizontal.m_nMax; }
	int VerticalScrollMax() { return m_scrollVertical.m_nMax; }

	void GetLinkAtPosition( int x, int y );

	void HidePopup();

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName )
	{
		ValidateObj( m_CustomURLHandlers );
		BaseClass::Validate( validator, pchName );
	}
#endif // DBGFLAG_VALIDATE

	void PaintComboBox();
	ISteamHTMLSurface *SteamHTMLSurface() { return m_SteamAPIContext.SteamHTMLSurface(); }

	void OnHTMLMouseMoved( int x, int y )
	{
		if ( m_SteamAPIContext.SteamHTMLSurface() )
			m_SteamAPIContext.SteamHTMLSurface()->MouseMove( m_unBrowserHandle, x, y );
	}

protected:
	virtual void ApplySchemeSettings( IScheme *pScheme );

	friend class HTMLComboBoxHost;
	vgui::Menu *m_pContextMenu;


private:
	STEAM_CALLBACK( HTML, BrowserNeedsPaint, HTML_NeedsPaint_t, m_NeedsPaint );
	STEAM_CALLBACK( HTML, BrowserComboNeedsPaint, HTML_ComboNeedsPaint_t, m_ComboNeedsPaint );
	STEAM_CALLBACK( HTML, BrowserStartRequest, HTML_StartRequest_t, m_StartRequest );
	STEAM_CALLBACK( HTML, BrowserURLChanged, HTML_URLChanged_t, m_URLChanged );
	STEAM_CALLBACK( HTML, BrowserFinishedRequest, HTML_FinishedRequest_t, m_FinishedRequest );
	STEAM_CALLBACK( HTML, BrowserShowPopup, HTML_ShowPopup_t, m_ShowPopup );
	STEAM_CALLBACK( HTML, BrowserHidePopup, HTML_HidePopup_t, m_HidePopup );
	STEAM_CALLBACK( HTML, BrowserSizePopup, HTML_SizePopup_t, m_SizePopup );

	STEAM_CALLBACK( HTML, BrowserOpenNewTab, HTML_OpenLinkInNewTab_t, m_LinkInNewTab );
	STEAM_CALLBACK( HTML, BrowserSetHTMLTitle, HTML_ChangedTitle_t, m_ChangeTitle );
	STEAM_CALLBACK( HTML, BrowserPopupHTMLWindow, HTML_NewWindow_t, m_NewWindow );
	STEAM_CALLBACK( HTML, BrowserFileLoadDialog, HTML_FileOpenDialog_t, m_FileLoadDialog );
	STEAM_CALLBACK( HTML, BrowserSearchResults, HTML_SearchResults_t, m_SearchResults );
	STEAM_CALLBACK( HTML, BrowserClose, HTML_CloseBrowser_t, m_CloseBrowser );
	STEAM_CALLBACK( HTML, BrowserHorizontalScrollBarSizeResponse, HTML_HorizontalScroll_t, m_HorizScroll );
	STEAM_CALLBACK( HTML, BrowserVerticalScrollBarSizeResponse, HTML_VerticalScroll_t, m_VertScroll );
	STEAM_CALLBACK( HTML, BrowserLinkAtPositionResponse, HTML_LinkAtPosition_t, m_LinkAtPosResp );
	STEAM_CALLBACK( HTML, BrowserJSAlert, HTML_JSAlert_t, m_JSAlert );
	STEAM_CALLBACK( HTML, BrowserJSConfirm, HTML_JSConfirm_t, m_JSConfirm );
	STEAM_CALLBACK( HTML, BrowserCanGoBackandForward, HTML_CanGoBackAndForward_t, m_CanGoBackForward );
	STEAM_CALLBACK( HTML, BrowserSetCursor, HTML_SetCursor_t, m_SetCursor );
	STEAM_CALLBACK( HTML, BrowserStatusText, HTML_StatusText_t, m_StatusText );
	STEAM_CALLBACK( HTML, BrowserShowToolTip, HTML_ShowToolTip_t, m_ShowTooltip );
	STEAM_CALLBACK( HTML, BrowserUpdateToolTip, HTML_UpdateToolTip_t, m_UpdateTooltip );
	STEAM_CALLBACK( HTML, BrowserHideToolTip, HTML_HideToolTip_t, m_HideTooltip );

	void OnBrowserReady( HTML_BrowserReady_t *pBrowserReady, bool bIOFailure );

	void PostURL(const char *URL, const char *pchPostData, bool force);
	virtual void BrowserResize();
	void UpdateSizeAndScrollBars();
	MESSAGE_FUNC( OnSliderMoved, "ScrollBarSliderMoved" );
	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );
	MESSAGE_FUNC( OnFileSelectionCancelled, "FileSelectionCancelled" );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PTR( OnEditNewLine, "TextNewLine", panel );
	MESSAGE_FUNC_INT( DismissJSDialog, "DismissJSDialog", result );

	vgui::Panel *m_pInteriorPanel;
	vgui::ScrollBar *_hbar,*_vbar;
	vgui::DHANDLE<vgui::FileOpenDialog> m_hFileOpenDialog;
	class CHTMLFindBar : public vgui::EditablePanel
	{
		DECLARE_CLASS_SIMPLE( CHTMLFindBar, EditablePanel );
	public:
		CHTMLFindBar( HTML *parent );
		void SetText( const char *pchText ) { m_pFindBar->SetText( pchText ); }
		void GetText( char *pText, int ccText ) { m_pFindBar->GetText( pText, ccText ); }
		void OnCommand( const char *pchCmd );
		void ShowCountLabel() { m_pFindCountLabel->SetVisible( true ); }
		void HideCountLabel() { m_pFindCountLabel->SetVisible( false ); }
		void SetHidden( bool bState  ) { m_bHidden = bState; }
		bool BIsHidden() { return m_bHidden; }

	private:
		vgui::TextEntry *m_pFindBar;
		vgui::HTML *m_pParent;
		vgui::Label *m_pFindCountLabel;
		bool m_bHidden;
	};

	CHTMLFindBar *m_pFindBar;
	HTMLComboBoxHost *m_pComboBoxHost;

	int m_iMouseX,m_iMouseY; // where the mouse is on the control

	int m_iScrollBorderX,m_iScrollBorderY;
	int m_iWideLastHTMLSize, m_iTalLastHTMLSize;
	int m_iCopyLinkMenuItemID;

	bool m_bScrollBarEnabled;
	bool m_bContextMenuEnabled;
	int	m_iScrollbarSize;
	bool m_bNewWindowsOnly;
	int m_nViewSourceAllowedIndex;
	CUtlString m_sDragURL;
	int m_iDragStartX, m_iDragStartY;

	struct CustomURLHandler_t
	{
		PHandle hPanel;
		char url[32];
	};
	CUtlVector<CustomURLHandler_t> m_CustomURLHandlers;

	int m_iHTMLTextureID; // vgui texture id
	// Track the texture width and height requested so we can tell
	// when the size has changed and reallocate the texture.
	int m_allocedTextureWidth;
	int m_allocedTextureHeight;
	int m_iComboBoxTextureID; // vgui texture id of the combo box
	bool m_bNeedsFullTextureUpload; 
	int m_allocedComboBoxWidth;
	int m_allocedComboBoxHeight;
	CUtlString m_sCurrentURL; // the url of our current page
	// find in page state
	bool m_bInFind;
	CUtlString m_sLastSearchString;

	bool m_bCanGoBack; // cache of forward and back state
	bool m_bCanGoForward;

	struct LinkAtPos_t
	{
		LinkAtPos_t() { m_nX = m_nY = 0; }
		uint32 m_nX;
		uint32 m_nY;
		CUtlString m_sURL;
	};
	LinkAtPos_t m_LinkAtPos; // cache for link at pos requests, because the request is async
	bool m_bRequestingDragURL; // true if we need a response for a drag url loc
	bool m_bRequestingCopyLink; // true if we wanted to copy the link under the cursor

	struct ScrollData_t
	{
		ScrollData_t() 
		{
			m_bVisible = false;
			m_nMax = m_nScroll = 0;
		}

		bool operator==( ScrollData_t const &src ) const
		{
			return m_bVisible == src.m_bVisible && 
				m_nMax == src.m_nMax &&
				m_nScroll == src.m_nScroll;
		}

		bool operator!=( ScrollData_t const &src ) const
		{	
			return !operator==(src);
		}


		bool m_bVisible; // is the scroll bar visible
		int m_nMax; // most amount of pixels we can scroll
		int m_nScroll; // currently scrolled amount of pixels
		float m_flZoom; // zoom level this scroll bar is for
	};

	ScrollData_t m_scrollHorizontal; // details of horizontal scroll bar
	ScrollData_t m_scrollVertical; // details of vertical scroll bar
	float m_flZoom; // current page zoom level

	CUtlString m_sPendingURLLoad; // cache of url to load if we get a PostURL before the cef object is mage
	CUtlString m_sPendingPostData; // cache of the post data for above

	struct CustomCursorCache_t
	{
		CustomCursorCache_t() {}
		CustomCursorCache_t( const void *pchData ) { m_pchData = pchData; }
		float m_CacheTime; // the time we cached the cursor
		CursorCode m_Cursor; // the vgui handle to it
		const void *m_pchData; // the pointer to the cursor char data so we can detect the same cursor being used
		bool operator==(const CustomCursorCache_t& rhs) const
		{
			return m_pchData == rhs.m_pchData ; 
		}
	};
	CUtlVector<CustomCursorCache_t> m_vecHCursor;

	CSteamAPIContext m_SteamAPIContext;
	HHTMLBrowser m_unBrowserHandle;
	CCallResult< HTML, HTML_BrowserReady_t > m_SteamCallResultBrowserReady;
};

} // namespace vgui

#endif // HTML_H
