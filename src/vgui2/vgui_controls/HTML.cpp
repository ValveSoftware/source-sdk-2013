//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// This class is a message box that has two buttons, ok and cancel instead of
// just the ok button of a message box. We use a message box class for the ok button
// and implement another button here.
//
// $NoKeywords: $
//=============================================================================//

#include "vgui_controls/pch_vgui_controls.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MessageBox.h>

#include "filesystem.h"
#include "../vgui2/src/vgui_key_translation.h"

#undef PostMessage
#undef MessageBox

#include "OfflineMode.h"

// memdbgon must be the last include file in a .cpp file
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: A simple passthrough panel to render the border onto the HTML widget
//-----------------------------------------------------------------------------
class HTMLInterior : public Panel
{
	DECLARE_CLASS_SIMPLE( HTMLInterior, Panel );
public:
	HTMLInterior( HTML *parent ) : BaseClass( parent, "HTMLInterior" ) 
	{ 	
		m_pHTML = parent; 
		SetPaintBackgroundEnabled( false );
		SetKeyBoardInputEnabled( false );
		SetMouseInputEnabled( false );
	}

private:
	HTML *m_pHTML;
};

//-----------------------------------------------------------------------------
// Purpose: container class for any external popup windows the browser requests
//-----------------------------------------------------------------------------
class HTMLPopup : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( HTMLPopup, vgui::Frame );
	class PopupHTML : public vgui::HTML
	{
		DECLARE_CLASS_SIMPLE( PopupHTML, vgui::HTML );
	public:
		PopupHTML( Frame *parent, const char *pchName, bool allowJavaScript , bool bPopupWindow  ) : HTML( parent, pchName, allowJavaScript, bPopupWindow ) { m_pParent = parent; }

		virtual void OnSetHTMLTitle( const char *pchTitle )
		{
			BaseClass::OnSetHTMLTitle( pchTitle );
			m_pParent->SetTitle( pchTitle, true );
		}

	private:
		Frame *m_pParent;
	};
public:
	HTMLPopup( Panel *parent, const char *pchURL, const char *pchTitle ) : Frame( NULL, "HtmlPopup", true )
	{
		m_pHTML = new PopupHTML( this, "htmlpopupchild", true, true );
		m_pHTML->OpenURL( pchURL, NULL, false );
		SetTitle( pchTitle, true );
	}

	~HTMLPopup()
	{
	}

	enum
	{
		vert_inset = 40,
		horiz_inset = 6
	};

	void PerformLayout()
	{
		BaseClass::PerformLayout();
		int wide, tall;
		GetSize( wide, tall );
		m_pHTML->SetPos( horiz_inset, vert_inset );
		m_pHTML->SetSize( wide - horiz_inset*2, tall - vert_inset*2 );
	}

	void SetBounds( int x, int y, int wide, int tall )
	{
		BaseClass::SetBounds( x, y, wide + horiz_inset*2, tall + vert_inset*2 );
	}

	MESSAGE_FUNC( OnCloseWindow, "OnCloseWindow" )
	{
		Close();
	}
private:
	PopupHTML *m_pHTML;
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
#ifdef _WIN32
// Old code, stricter compiler
#pragma warning(disable : 4355) // warning C4355: 'this': used in base member initializer list
#endif
HTML::HTML(Panel *parent, const char *name, bool allowJavaScript, bool bPopupWindow) : Panel(parent, name), 
m_NeedsPaint( this, &HTML::BrowserNeedsPaint ),
m_StartRequest( this, &HTML::BrowserStartRequest ),
m_URLChanged( this, &HTML::BrowserURLChanged ),
m_FinishedRequest( this, &HTML::BrowserFinishedRequest ),
m_LinkInNewTab( this, &HTML::BrowserOpenNewTab ),
m_ChangeTitle( this, &HTML::BrowserSetHTMLTitle ),
m_NewWindow( this, &HTML::BrowserPopupHTMLWindow ),
m_FileLoadDialog( this, &HTML::BrowserFileLoadDialog ),
m_SearchResults( this, &HTML::BrowserSearchResults ),
m_CloseBrowser( this, &HTML::BrowserClose ),
m_HorizScroll( this, &HTML::BrowserHorizontalScrollBarSizeResponse ),
m_VertScroll( this, &HTML::BrowserVerticalScrollBarSizeResponse ),
m_LinkAtPosResp( this, &HTML::BrowserLinkAtPositionResponse ),
m_JSAlert( this, &HTML::BrowserJSAlert ),
m_JSConfirm( this, &HTML::BrowserJSConfirm ),
m_CanGoBackForward( this, &HTML::BrowserCanGoBackandForward ),
m_SetCursor( this, &HTML::BrowserSetCursor ),
m_StatusText( this, &HTML::BrowserStatusText ),
m_ShowTooltip( this, &HTML::BrowserShowToolTip ),
m_UpdateTooltip( this, &HTML::BrowserUpdateToolTip ),
m_HideTooltip( this, &HTML::BrowserHideToolTip )
{
	m_iHTMLTextureID = 0;
	m_bCanGoBack = false;
	m_bCanGoForward = false;
	m_bInFind = false;
	m_bRequestingDragURL = false;
	m_bRequestingCopyLink = false;
	m_flZoom = 100.0f;
	m_bNeedsFullTextureUpload = false;

	m_pInteriorPanel = new HTMLInterior( this );
	SetPostChildPaintEnabled( true );

	m_unBrowserHandle = INVALID_HTMLBROWSER;
	m_SteamAPIContext.Init();
	if ( m_SteamAPIContext.SteamHTMLSurface() )
	{
		m_SteamAPIContext.SteamHTMLSurface()->Init();
		SteamAPICall_t hSteamAPICall = m_SteamAPIContext.SteamHTMLSurface()->CreateBrowser( surface()->GetWebkitHTMLUserAgentString(), NULL );
		m_SteamCallResultBrowserReady.Set( hSteamAPICall, this, &HTML::OnBrowserReady );
	}
	else
	{
		Warning("Unable to access SteamHTMLSurface");
	}
	m_iScrollBorderX=m_iScrollBorderY=0;
	m_bScrollBarEnabled = true;
	m_bContextMenuEnabled = true; 
	m_bNewWindowsOnly = false;
	m_iMouseX = m_iMouseY = 0;
	m_iDragStartX = m_iDragStartY = 0;
	m_nViewSourceAllowedIndex = -1;
	m_iWideLastHTMLSize = m_iTalLastHTMLSize = 0;

	_hbar = new ScrollBar(this, "HorizScrollBar", false);
	_hbar->SetVisible(false);
	_hbar->AddActionSignalTarget(this);

	_vbar = new ScrollBar(this, "VertScrollBar", true);
	_vbar->SetVisible(false);
	_vbar->AddActionSignalTarget(this);

	m_pFindBar = new HTML::CHTMLFindBar( this );
	m_pFindBar->SetZPos( 2 );
	m_pFindBar->SetVisible( false );

	m_pContextMenu = new Menu( this, "contextmenu" );
	m_pContextMenu->AddMenuItem( "#vgui_HTMLBack", new KeyValues( "Command", "command", "back" ), this );
	m_pContextMenu->AddMenuItem( "#vgui_HTMLForward", new KeyValues( "Command", "command", "forward" ), this );
	m_pContextMenu->AddMenuItem( "#vgui_HTMLReload", new KeyValues( "Command", "command", "reload" ), this );
	m_pContextMenu->AddMenuItem( "#vgui_HTMLStop", new KeyValues( "Command", "command", "stop" ), this );
	m_pContextMenu->AddSeparator();
	m_pContextMenu->AddMenuItem( "#vgui_HTMLCopyUrl", new KeyValues( "Command", "command", "copyurl" ), this );
	m_iCopyLinkMenuItemID = m_pContextMenu->AddMenuItem( "#vgui_HTMLCopyLink", new KeyValues( "Command", "command", "copylink" ), this );
	m_pContextMenu->AddMenuItem( "#TextEntry_Copy", new KeyValues( "Command", "command", "copy" ), this );
	m_pContextMenu->AddMenuItem( "#TextEntry_Paste", new KeyValues( "Command", "command", "paste" ), this );
	m_pContextMenu->AddSeparator();
	m_nViewSourceAllowedIndex = m_pContextMenu->AddMenuItem( "#vgui_HTMLViewSource", new KeyValues( "Command", "command", "viewsource" ), this );
}


//-----------------------------------------------------------------------------
// Purpose: browser is ready to show pages
//-----------------------------------------------------------------------------
void HTML::OnBrowserReady( HTML_BrowserReady_t *pBrowserReady, bool bIOFailure )
{
	m_unBrowserHandle = pBrowserReady->unBrowserHandle;
	BrowserResize();

	if (!m_sPendingURLLoad.IsEmpty())
	{
		PostURL( m_sPendingURLLoad, m_sPendingPostData, false );
		m_sPendingURLLoad.Clear();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
HTML::~HTML()
{
	m_pContextMenu->MarkForDeletion();

	if ( m_SteamAPIContext.SteamHTMLSurface() )
	{
		m_SteamAPIContext.SteamHTMLSurface()->RemoveBrowser( m_unBrowserHandle );
	}
	
	FOR_EACH_VEC( m_vecHCursor, i )
	{
		// BR FIXME!
//		surface()->DeleteCursor( m_vecHCursor[i].m_Cursor );
	}
	m_vecHCursor.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Handle message to change our cursor
//-----------------------------------------------------------------------------
void HTML::OnSetCursorVGUI( int cursor )
{
	SetCursor( (HCursor)cursor );
}

//-----------------------------------------------------------------------------
// Purpose: sets up colors/fonts/borders
//-----------------------------------------------------------------------------
void HTML::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
	BrowserResize();
}


//-----------------------------------------------------------------------------
// Purpose: overrides panel class, paints a texture of the HTML window as a background
//-----------------------------------------------------------------------------
void HTML::Paint()
{
	//VPROF_BUDGET( "HTML::Paint()", VPROF_BUDGETGROUP_OTHER_VGUI );
	BaseClass::Paint();

	if ( m_iHTMLTextureID != 0 )
	{
		surface()->DrawSetTexture( m_iHTMLTextureID );
		int tw = 0, tt = 0;
		surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
		GetSize( tw, tt );
		surface()->DrawTexturedRect( 0, 0, tw, tt );
	}

	// If we have scrollbars, we need to draw the bg color under them, since the browser
	// bitmap is a checkerboard under them, and they are transparent in the in-game client
	if ( m_iScrollBorderX > 0 || m_iScrollBorderY > 0 )
	{
		int w, h;
		GetSize( w, h );
		IBorder *border = GetBorder();
		int left = 0, top = 0, right = 0, bottom = 0;
		if ( border )
		{
			border->GetInset( left, top, right, bottom );
		}
		surface()->DrawSetColor( GetBgColor() );
		if ( m_iScrollBorderX )
		{
			surface()->DrawFilledRect( w-m_iScrollBorderX - right, top, w, h - bottom );
		}
		if ( m_iScrollBorderY )
		{
			surface()->DrawFilledRect( left, h-m_iScrollBorderY - bottom, w-m_iScrollBorderX - right, h );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: causes a repaint when the layout changes
//-----------------------------------------------------------------------------
void HTML::PerformLayout()
{
	BaseClass::PerformLayout();
	Repaint();
	int vbarInset = _vbar->IsVisible() ? _vbar->GetWide() : 0;
	int maxw = GetWide() - vbarInset;
	m_pInteriorPanel->SetBounds( 0, 0, maxw, GetTall() );

	IScheme *pClientScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "ClientScheme" ) );

	int iSearchInsetY = 5;
	int iSearchInsetX = 5;
	int iSearchTall = 24;
	int iSearchWide = 150;
	const char *resourceString = pClientScheme->GetResourceString( "HTML.SearchInsetY");
	if ( resourceString )
	{
		iSearchInsetY = atoi(resourceString);
	}
	resourceString = pClientScheme->GetResourceString( "HTML.SearchInsetX");
	if ( resourceString )
	{
		iSearchInsetX = atoi(resourceString);
	}
	resourceString = pClientScheme->GetResourceString( "HTML.SearchTall");
	if ( resourceString )
	{
		iSearchTall = atoi(resourceString);
	}
	resourceString = pClientScheme->GetResourceString( "HTML.SearchWide");
	if ( resourceString )
	{
		iSearchWide = atoi(resourceString);
	}

	m_pFindBar->SetBounds( GetWide() - iSearchWide - iSearchInsetX - vbarInset, m_pFindBar->BIsHidden() ? -1*iSearchTall-5: iSearchInsetY, iSearchWide, iSearchTall );
}


//-----------------------------------------------------------------------------
// Purpose: updates the underlying HTML surface widgets position
//-----------------------------------------------------------------------------
void HTML::OnMove()
{
	BaseClass::OnMove();

	// tell cef where we are on the screen so plugins can correctly render
	int nPanelAbsX, nPanelAbsY;
	ipanel()->GetAbsPos( GetVPanel(), nPanelAbsX, nPanelAbsY );
}


//-----------------------------------------------------------------------------
// Purpose: opens the URL, will accept any URL that IE accepts
//-----------------------------------------------------------------------------
void HTML::OpenURL(const char *URL, const char *postData, bool force)
{
	PostURL( URL, postData, force );
}

//-----------------------------------------------------------------------------
// Purpose: opens the URL, will accept any URL that IE accepts
//-----------------------------------------------------------------------------
void HTML::PostURL(const char *URL, const char *pchPostData, bool force)
{
	if ( m_unBrowserHandle == INVALID_HTMLBROWSER )
	{
		m_sPendingURLLoad = URL;
		m_sPendingPostData = pchPostData;
		return;
	}

	if ( IsSteamInOfflineMode() && !force )
	{
		const char *baseDir = getenv("HTML_OFFLINE_DIR");
		if ( baseDir )
		{
			// get the app we need to run
			char htmlLocation[_MAX_PATH];
			char otherName[128];
			char fileLocation[_MAX_PATH];

			if ( ! g_pFullFileSystem->FileExists( baseDir ) ) 
			{
				Q_snprintf( otherName, sizeof(otherName), "%senglish.html", OFFLINE_FILE );
				baseDir = otherName;
			}
			g_pFullFileSystem->GetLocalCopy( baseDir ); // put this file on disk for IE to load

			g_pFullFileSystem->GetLocalPath( baseDir, fileLocation, sizeof(fileLocation) );
			Q_snprintf(htmlLocation, sizeof(htmlLocation), "file://%s", fileLocation);
	
			if (m_SteamAPIContext.SteamHTMLSurface())
				m_SteamAPIContext.SteamHTMLSurface()->LoadURL( m_unBrowserHandle, htmlLocation, NULL );
		}
		else
		{
			if (m_SteamAPIContext.SteamHTMLSurface())
				m_SteamAPIContext.SteamHTMLSurface()->LoadURL( m_unBrowserHandle, URL, NULL );
		}
	}
	else
	{
		if ( pchPostData && Q_strlen(pchPostData) > 0 )
		{
			if (m_SteamAPIContext.SteamHTMLSurface())
				m_SteamAPIContext.SteamHTMLSurface()->LoadURL( m_unBrowserHandle, URL, pchPostData );

		}
		else
		{			
			if (m_SteamAPIContext.SteamHTMLSurface())
				m_SteamAPIContext.SteamHTMLSurface()->LoadURL( m_unBrowserHandle, URL, NULL );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: opens the URL, will accept any URL that IE accepts
//-----------------------------------------------------------------------------
bool HTML::StopLoading()
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->StopLoad( m_unBrowserHandle );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: refreshes the current page
//-----------------------------------------------------------------------------
bool HTML::Refresh()
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->Reload( m_unBrowserHandle );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Tells the browser control to go back
//-----------------------------------------------------------------------------
void HTML::GoBack()
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->GoBack( m_unBrowserHandle );
}


//-----------------------------------------------------------------------------
// Purpose: Tells the browser control to go forward
//-----------------------------------------------------------------------------
void HTML::GoForward()
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->GoForward( m_unBrowserHandle );
}


//-----------------------------------------------------------------------------
// Purpose: Checks if the browser can go back further
//-----------------------------------------------------------------------------
bool HTML::BCanGoBack()
{
	return m_bCanGoBack;
}


//-----------------------------------------------------------------------------
// Purpose: Checks if the browser can go forward further
//-----------------------------------------------------------------------------
bool HTML::BCanGoFoward()
{
	return m_bCanGoForward;
}


//-----------------------------------------------------------------------------
// Purpose: handle resizing
//-----------------------------------------------------------------------------
void HTML::OnSizeChanged(int wide,int tall)
{
	BaseClass::OnSizeChanged(wide,tall);
	UpdateSizeAndScrollBars();
}


//-----------------------------------------------------------------------------
// Purpose: Run javascript in the page
//-----------------------------------------------------------------------------
void HTML::RunJavascript( const char *pchScript )
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->ExecuteJavascript( m_unBrowserHandle, pchScript );
}




//-----------------------------------------------------------------------------
// Purpose: helper to convert UI mouse codes to CEF ones
//-----------------------------------------------------------------------------
ISteamHTMLSurface::EHTMLMouseButton ConvertMouseCodeToCEFCode( MouseCode code )
{
	switch( code )
	{
	default:
	case MOUSE_LEFT:
		return ISteamHTMLSurface::eHTMLMouseButton_Left;
		break;
	case MOUSE_RIGHT:
		return ISteamHTMLSurface::eHTMLMouseButton_Right;
		break;
	case MOUSE_MIDDLE:
		return ISteamHTMLSurface::eHTMLMouseButton_Middle;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: passes mouse clicks to the control
//-----------------------------------------------------------------------------
void HTML::OnMousePressed(MouseCode code)
{
	m_sDragURL = NULL;

	// mouse4 = back button
	if ( code == MOUSE_4 )
	{
        PostActionSignal( new KeyValues( "HTMLBackRequested" ) );
		return;
	}
	if ( code == MOUSE_5 )
	{
        PostActionSignal( new KeyValues( "HTMLForwardRequested" ) );
		return;
	}


	if ( code == MOUSE_RIGHT && m_bContextMenuEnabled )
	{
		GetLinkAtPosition( m_iMouseX, m_iMouseY );
		Menu::PlaceContextMenu( this, m_pContextMenu );
		return;
	}

	// ask for the focus to come to this window
	RequestFocus();

	// now tell the browser about the click
	// ignore right clicks if context menu has been disabled
	if ( code != MOUSE_RIGHT )
	{
		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->MouseDown( m_unBrowserHandle, ConvertMouseCodeToCEFCode( code ) );
	}

	if ( code == MOUSE_LEFT )
	{
		input()->GetCursorPos( m_iDragStartX, m_iDragStartY );
		int htmlx, htmly;
		ipanel()->GetAbsPos( GetVPanel(), htmlx, htmly );

		GetLinkAtPosition( m_iDragStartX - htmlx, m_iDragStartY - htmly );

		m_bRequestingDragURL = true;
		// make sure we get notified when the mouse gets released
		if ( !m_sDragURL.IsEmpty() )
		{
			input()->SetMouseCapture( GetVPanel() );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: passes mouse up events
//-----------------------------------------------------------------------------
void HTML::OnMouseReleased(MouseCode code)
{
	if ( code == MOUSE_LEFT )
	{
		input()->SetMouseCapture( NULL );
		input()->SetCursorOveride( 0 );

		if ( !m_sDragURL.IsEmpty() && input()->GetMouseOver() != GetVPanel() && input()->GetMouseOver() )
		{
			// post the text as a drag drop to the target panel
			KeyValuesAD kv( "DragDrop" );
			if ( ipanel()->RequestInfo( input()->GetMouseOver(), kv )
				&& kv->GetPtr( "AcceptPanel" ) != NULL )
			{
				VPANEL vpanel = (VPANEL)kv->GetPtr( "AcceptPanel" );
				ivgui()->PostMessage( vpanel, new KeyValues( "DragDrop", "text", m_sDragURL.Get() ), GetVPanel() );
			}
		}
		m_sDragURL = NULL;
	}

	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->MouseUp( m_unBrowserHandle, ConvertMouseCodeToCEFCode( code ) );
}


//-----------------------------------------------------------------------------
// Purpose: keeps track of where the cursor is
//-----------------------------------------------------------------------------
void HTML::OnCursorMoved(int x,int y)
{
	// Only do this when we are over the current panel
	if ( vgui::input()->GetMouseOver() == GetVPanel() )
	{
		m_iMouseX = x;
		m_iMouseY = y;

		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->MouseMove( m_unBrowserHandle, m_iMouseX, m_iMouseY );
	}
	else if ( !m_sDragURL.IsEmpty() )
	{
		if ( !input()->GetMouseOver() )
		{
			// we're not over any vgui window, switch to the OS implementation of drag/drop
			// BR FIXME
//			surface()->StartDragDropText( m_sDragURL );
			m_sDragURL = NULL;
		}
	}

	if ( !m_sDragURL.IsEmpty() && !input()->GetCursorOveride() )
	{
		// if we've dragged far enough (in global coordinates), set to use the drag cursor
		int gx, gy;
		input()->GetCursorPos( gx, gy );
		if ( abs(m_iDragStartX-gx) + abs(m_iDragStartY-gy) > 3 )
		{
//			input()->SetCursorOveride( dc_alias );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: passes double click events to the browser
//-----------------------------------------------------------------------------
void HTML::OnMouseDoublePressed(MouseCode code)
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->MouseDoubleClick( m_unBrowserHandle, ConvertMouseCodeToCEFCode( code ) );
}


//-----------------------------------------------------------------------------
// Purpose: return the bitmask of any modifier keys that are currently down
//-----------------------------------------------------------------------------
int GetKeyModifiers()
{
	// Any time a key is pressed reset modifier list as well
	int nModifierCodes = 0;
	if (vgui::input()->IsKeyDown( KEY_LCONTROL ) || vgui::input()->IsKeyDown( KEY_RCONTROL ))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_CtrlDown;

	if (vgui::input()->IsKeyDown( KEY_LALT ) || vgui::input()->IsKeyDown( KEY_RALT ))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_AltDown;

	if (vgui::input()->IsKeyDown( KEY_LSHIFT ) || vgui::input()->IsKeyDown( KEY_RSHIFT ))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_ShiftDown;

#ifdef OSX
	// for now pipe through the cmd-key to be like the control key so we get copy/paste
	if (vgui::input()->IsKeyDown( KEY_LWIN ) || vgui::input()->IsKeyDown( KEY_RWIN ))
		nModifierCodes |= ISteamHTMLSurface::k_eHTMLKeyModifier_CtrlDown;
#endif

	return nModifierCodes;
}


//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser (we don't current do this)
//-----------------------------------------------------------------------------
void HTML::OnKeyTyped(wchar_t unichar)
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->KeyChar( m_unBrowserHandle, unichar, (ISteamHTMLSurface::EHTMLKeyModifiers)GetKeyModifiers() );
}


//-----------------------------------------------------------------------------
// Purpose: pop up the find dialog
//-----------------------------------------------------------------------------
void HTML::ShowFindDialog()
{
	IScheme *pClientScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "ClientScheme" ) );
	if ( !pClientScheme )
		return;

	m_pFindBar->SetVisible( true );
	m_pFindBar->RequestFocus();
	m_pFindBar->SetText( "" );
	m_pFindBar->HideCountLabel();
	m_pFindBar->SetHidden( false );
	int x = 0, y = 0, h = 0, w = 0;
	m_pFindBar->GetBounds( x, y, w, h );
	m_pFindBar->SetPos( x, -1*h );
	int iSearchInsetY = 0;
	const char *resourceString = pClientScheme->GetResourceString( "HTML.SearchInsetY");
	if ( resourceString )
	{
		iSearchInsetY = atoi(resourceString);
	}
	float flAnimationTime = 0.0f;
	resourceString = pClientScheme->GetResourceString( "HTML.SearchAnimationTime");
	if ( resourceString )
	{
		flAnimationTime = atof(resourceString);
	}

	GetAnimationController()->RunAnimationCommand( m_pFindBar, "ypos", iSearchInsetY, 0.0f, flAnimationTime, AnimationController::INTERPOLATOR_LINEAR );
}


//-----------------------------------------------------------------------------
// Purpose: hide the find dialog
//-----------------------------------------------------------------------------
void HTML::HideFindDialog()
{
	IScheme *pClientScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "ClientScheme" ) );
	if ( !pClientScheme )
		return;

	int x = 0, y = 0, h = 0, w = 0;
	m_pFindBar->GetBounds( x, y, w, h );
	float flAnimationTime = 0.0f;
	const char *resourceString = pClientScheme->GetResourceString( "HTML.SearchAnimationTime");
	if ( resourceString )
	{
		flAnimationTime = atof(resourceString);
	}

	GetAnimationController()->RunAnimationCommand( m_pFindBar, "ypos", -1*h-5, 0.0f, flAnimationTime, AnimationController::INTERPOLATOR_LINEAR );
	m_pFindBar->SetHidden( true );
	StopFind();
}


//-----------------------------------------------------------------------------
// Purpose: is the find dialog visible?
//-----------------------------------------------------------------------------
bool HTML::FindDialogVisible()
{
	return m_pFindBar->IsVisible() && !m_pFindBar->BIsHidden();
}


//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser 
//-----------------------------------------------------------------------------
void HTML::OnKeyCodeTyped(KeyCode code)
{
	switch( code )
	{
	case KEY_PAGEDOWN:
		{
		int val = _vbar->GetValue();
		val += 200;
		_vbar->SetValue(val);
		break;
		}
	case KEY_PAGEUP:
		{
		int val = _vbar->GetValue();
		val -= 200;
		_vbar->SetValue(val);
		break;	
		}
	case KEY_F5:
		{
		Refresh();
		break;
		}
	case KEY_F:
		{
			if ( (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL) )
				|| ( IsOSX() && ( input()->IsKeyDown(KEY_LWIN) || input()->IsKeyDown(KEY_RWIN) ) ) )
			{
				if ( !FindDialogVisible() )
				{
					ShowFindDialog();
				}
				else
				{
					HideFindDialog();
				}
				break;
			}
		}
	case KEY_ESCAPE:
		{
			if ( FindDialogVisible() )
			{
				HideFindDialog();
				break;
			}
		}
	case KEY_TAB:
		{
			if ( input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL) )
			{
				// pass control-tab to parent (through baseclass)
				BaseClass::OnKeyTyped( code );
				return;
			}
			break;
		}
	}

	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->KeyDown( m_unBrowserHandle, KeyCode_VGUIToVirtualKey(code), (ISteamHTMLSurface::EHTMLKeyModifiers)GetKeyModifiers() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTML::OnKeyCodeReleased(KeyCode code)
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->KeyUp( m_unBrowserHandle, KeyCode_VGUIToVirtualKey( code ), (ISteamHTMLSurface::EHTMLKeyModifiers)GetKeyModifiers() );
}


//-----------------------------------------------------------------------------
// Purpose: scrolls the vertical scroll bar on a web page
//-----------------------------------------------------------------------------
void HTML::OnMouseWheeled(int delta)
{
	if (_vbar )
	{
		int val = _vbar->GetValue();
		val -= (delta * 100.0/3.0 ); // 100 for every 3 lines matches chromes code
		_vbar->SetValue(val);
	}

	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->MouseWheel( m_unBrowserHandle, delta* 100.0/3.0 );
}


//-----------------------------------------------------------------------------
// Purpose: Inserts a custom URL handler
//-----------------------------------------------------------------------------
void HTML::AddCustomURLHandler(const char *customProtocolName, vgui::Panel *target)
{
	int index = m_CustomURLHandlers.AddToTail();
	m_CustomURLHandlers[index].hPanel = target;
	Q_strncpy(m_CustomURLHandlers[index].url, customProtocolName, sizeof(m_CustomURLHandlers[index].url));
}


//-----------------------------------------------------------------------------
// Purpose: shared code for sizing the HTML surface window
//-----------------------------------------------------------------------------
void HTML::BrowserResize()
{
	if (m_unBrowserHandle == INVALID_HTMLBROWSER)
		return;

	int w,h;
	GetSize( w, h );
	int right = 0, bottom = 0;
	// TODO::STYLE
	/*
	IAppearance *pAppearance = GetAppearance();
	int left = 0, top = 0;
	if ( pAppearance )
	{
		pAppearance->GetInset( left, top, right, bottom );
	}
	*/


	if ( m_iWideLastHTMLSize != (  w - m_iScrollBorderX - right ) || m_iTalLastHTMLSize != ( h - m_iScrollBorderY - bottom ) )
	{
		m_iWideLastHTMLSize = w - m_iScrollBorderX - right;
		m_iTalLastHTMLSize = h - m_iScrollBorderY - bottom;
		if ( m_iTalLastHTMLSize <= 0 )
		{
			SetTall( 64 );
			m_iTalLastHTMLSize = 64 - bottom;
		}

		{
			if (m_SteamAPIContext.SteamHTMLSurface())
				m_SteamAPIContext.SteamHTMLSurface()->SetSize( m_unBrowserHandle, m_iWideLastHTMLSize, m_iTalLastHTMLSize );
		}

	
		// webkit forgets the scroll offset when you resize (it saves the scroll in a DC and a resize throws away the DC)
		// so just tell it after the resize
		int scrollV = _vbar->GetValue();
		int scrollH = _hbar->GetValue();

		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->SetHorizontalScroll( m_unBrowserHandle, scrollH );
		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->SetVerticalScroll( m_unBrowserHandle, scrollV );
	}

}


//-----------------------------------------------------------------------------
// Purpose: when a slider moves causes the IE images to re-render itself
//-----------------------------------------------------------------------------
void HTML::OnSliderMoved()
{
	if(_hbar->IsVisible())
	{
		int scrollX = _hbar->GetValue();
		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->SetHorizontalScroll( m_unBrowserHandle, scrollX );
	}

	if(_vbar->IsVisible())
	{
		int scrollY=_vbar->GetValue();
		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->SetVerticalScroll( m_unBrowserHandle, scrollY );
	}
	
	// post a message that the slider has moved
	PostActionSignal( new KeyValues( "HTMLSliderMoved" ) );
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
bool HTML::IsScrolledToBottom()
{
	if ( !_vbar->IsVisible() )
		return true;

	return m_scrollVertical.m_nScroll >= m_scrollVertical.m_nMax;
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
bool HTML::IsScrollbarVisible()
{
	return _vbar->IsVisible();
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::SetScrollbarsEnabled(bool state)
{
	m_bScrollBarEnabled = state;
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::SetContextMenuEnabled(bool state)
{
	m_bContextMenuEnabled = state;
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::SetViewSourceEnabled(bool state)
{
	m_pContextMenu->SetItemVisible( m_nViewSourceAllowedIndex, state );
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::NewWindowsOnly( bool state )
{
	m_bNewWindowsOnly = state;
}


//-----------------------------------------------------------------------------
// Purpose: called when our children have finished painting
//-----------------------------------------------------------------------------
void HTML::PostChildPaint()
{
	BaseClass::PostChildPaint();
	// TODO::STYLE
	//m_pInteriorPanel->SetPaintAppearanceEnabled( true ); // turn painting back on so the IE hwnd can render this border
}


//-----------------------------------------------------------------------------
// Purpose: Adds a custom header to all requests
//-----------------------------------------------------------------------------
void HTML::AddHeader( const char *pchHeader, const char *pchValue )
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->AddHeader( m_unBrowserHandle, pchHeader, pchValue );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTML::OnSetFocus()
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->SetKeyFocus( m_unBrowserHandle, true );

	BaseClass::OnSetFocus();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTML::OnKillFocus()
{
	BaseClass::OnKillFocus();

	// Don't clear the actual html focus if a context menu is what took focus
	if ( m_pContextMenu->HasFocus() )
		return;

	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->SetKeyFocus( m_unBrowserHandle, false );
}


//-----------------------------------------------------------------------------
// Purpose: webkit is telling us to use this cursor type
//-----------------------------------------------------------------------------
void HTML::OnCommand( const char *pchCommand )
{
	if ( !Q_stricmp( pchCommand, "back" ) )
	{
		PostActionSignal( new KeyValues( "HTMLBackRequested" ) );
	}
	else if ( !Q_stricmp( pchCommand, "forward" ) )
	{
		PostActionSignal( new KeyValues( "HTMLForwardRequested" ) );
	}
	else if ( !Q_stricmp( pchCommand, "reload" ) )
	{
		Refresh();
	}
	else if ( !Q_stricmp( pchCommand, "stop" ) )
	{
		StopLoading();
	}
	else if ( !Q_stricmp( pchCommand, "viewsource" ) )
	{
		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->ViewSource( m_unBrowserHandle );
	}
	else if ( !Q_stricmp( pchCommand, "copy" ) )
	{
		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->CopyToClipboard( m_unBrowserHandle );
	}
	else if ( !Q_stricmp( pchCommand, "paste" ) )
	{
		if (m_SteamAPIContext.SteamHTMLSurface())
			m_SteamAPIContext.SteamHTMLSurface()->PasteFromClipboard( m_unBrowserHandle );
	}
	else if ( !Q_stricmp( pchCommand, "copyurl" ) )
	{
		system()->SetClipboardText( m_sCurrentURL, m_sCurrentURL.Length() );
	}
	else if ( !Q_stricmp( pchCommand, "copylink" ) )
	{
		int x, y;
		m_pContextMenu->GetPos( x, y );
		int htmlx, htmly;
		ipanel()->GetAbsPos( GetVPanel(), htmlx, htmly );

		m_bRequestingCopyLink = true;
		GetLinkAtPosition( x - htmlx, y - htmly );
	}
	else
		BaseClass::OnCommand( pchCommand );

}


//-----------------------------------------------------------------------------
// Purpose: the control wants us to ask the user what file to load
//-----------------------------------------------------------------------------
void HTML::OnFileSelected( const char *pchSelectedFile )
{
	const char *ppchSelectedFiles[] = { pchSelectedFile, NULL };
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->FileLoadDialogResponse( m_unBrowserHandle , ppchSelectedFiles );

	m_hFileOpenDialog->Close();
}

//-----------------------------------------------------------------------------
// Purpose: called when the user dismissed the file dialog with no selection
//-----------------------------------------------------------------------------
void HTML::OnFileSelectionCancelled()
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->FileLoadDialogResponse( m_unBrowserHandle, NULL );

	m_hFileOpenDialog->Close();
}

//-----------------------------------------------------------------------------
// Purpose: find any text on the html page with this sub string
//-----------------------------------------------------------------------------
void HTML::Find( const char *pchSubStr )
{
	m_bInFind = false;
	if ( m_sLastSearchString == pchSubStr ) // same string as last time, lets fine next
		m_bInFind = true;

	m_sLastSearchString = pchSubStr;

	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->Find( m_unBrowserHandle, pchSubStr, m_bInFind, false );
}


//-----------------------------------------------------------------------------
// Purpose: find any text on the html page with this sub string
//-----------------------------------------------------------------------------
void HTML::FindPrevious()
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->Find( m_unBrowserHandle, m_sLastSearchString, m_bInFind, true );
}


//-----------------------------------------------------------------------------
// Purpose: find any text on the html page with this sub string
//-----------------------------------------------------------------------------
void HTML::FindNext()
{
	Find( m_sLastSearchString );
}


//-----------------------------------------------------------------------------
// Purpose: stop an outstanding find request
//-----------------------------------------------------------------------------
void HTML::StopFind( )
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->StopFind( m_unBrowserHandle );
	m_bInFind = false;
}


//-----------------------------------------------------------------------------
// Purpose: input handler
//-----------------------------------------------------------------------------
void HTML::OnEditNewLine( Panel *pPanel )
{
	OnTextChanged( pPanel );
}


//-----------------------h------------------------------------------------------
// Purpose: input handler
//-----------------------------------------------------------------------------
void HTML::OnTextChanged( Panel *pPanel )
{
	char rgchText[2048];
	m_pFindBar->GetText( rgchText, sizeof( rgchText ) );
	Find( rgchText );
}

//-----------------------------------------------------------------------------
// Purpose: helper class for the find bar
//-----------------------------------------------------------------------------
HTML::CHTMLFindBar::CHTMLFindBar( HTML *parent ) : EditablePanel( parent, "FindBar" )
{
	m_pParent = parent;
	m_bHidden = false;
	m_pFindBar = new TextEntry( this, "FindEntry" );
	m_pFindBar->AddActionSignalTarget( parent );
	m_pFindBar->SendNewLine( true );
	m_pFindCountLabel = new Label( this, "FindCount", "" );
	m_pFindCountLabel->SetVisible( false );

	if ( g_pFullFileSystem->FileExists( "resource/layout/htmlfindbar.layout" ) )
	{
		LoadControlSettings( "resource/layout/htmlfindbar.layout" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: button input into the find bar
//-----------------------------------------------------------------------------
void HTML::CHTMLFindBar::OnCommand( const char *pchCmd )
{
	if ( !Q_stricmp( pchCmd, "close" ) )
	{
		m_pParent->HideFindDialog();
	}
	else if ( !Q_stricmp( pchCmd, "previous" ) )
	{
		m_pParent->FindPrevious();
	}
	else if ( !Q_stricmp( pchCmd, "next" ) )
	{
		m_pParent->FindNext();
	}
	else
		BaseClass::OnCommand( pchCmd );

}


//-----------------------------------------------------------------------------
// Purpose: we have a new texture to update
//-----------------------------------------------------------------------------
void HTML::BrowserNeedsPaint( HTML_NeedsPaint_t *pCallback )
{
	int tw = 0, tt = 0;
	if ( m_iHTMLTextureID != 0 )
	{
		tw = m_allocedTextureWidth;
		tt = m_allocedTextureHeight;
	}

	if ( m_iHTMLTextureID != 0 && ( ( _vbar->IsVisible() && pCallback->unScrollY > 0 && abs( (int)pCallback->unScrollY - m_scrollVertical.m_nScroll) > 5 ) || ( _hbar->IsVisible() && pCallback->unScrollX > 0 && abs( (int)pCallback->unScrollX - m_scrollHorizontal.m_nScroll ) > 5 ) ) )
	{
		m_bNeedsFullTextureUpload = true;
		return;
	}

	// update the vgui texture
	if ( m_bNeedsFullTextureUpload || m_iHTMLTextureID == 0  || tw != (int)pCallback->unWide || tt != (int)pCallback->unTall )
	{
		m_bNeedsFullTextureUpload = false;
		if ( m_iHTMLTextureID != 0 )
			surface()->DeleteTextureByID( m_iHTMLTextureID );

		// if the dimensions changed we also need to re-create the texture ID to support the overlay properly (it won't resize a texture on the fly, this is the only control that needs
		//   to so lets have a tiny bit more code here to support that)
		m_iHTMLTextureID = surface()->CreateNewTextureID( true );
		surface()->DrawSetTextureRGBAEx( m_iHTMLTextureID, (const unsigned char *)pCallback->pBGRA, pCallback->unWide, pCallback->unTall, IMAGE_FORMAT_BGRA8888 );// BR FIXME - this call seems to shift by some number of pixels?
		m_allocedTextureWidth = pCallback->unWide;
		m_allocedTextureHeight = pCallback->unTall;
	}
	else if ( (int)pCallback->unUpdateWide > 0 && (int)pCallback->unUpdateTall > 0 )
	{
		// same size texture, just bits changing in it, lets twiddle
		surface()->DrawUpdateRegionTextureRGBA( m_iHTMLTextureID, pCallback->unUpdateX, pCallback->unUpdateY, (const unsigned char *)pCallback->pBGRA, pCallback->unUpdateWide, pCallback->unUpdateTall, IMAGE_FORMAT_BGRA8888 );
	}
	else
	{
		surface()->DrawSetTextureRGBAEx( m_iHTMLTextureID, (const unsigned char *)pCallback->pBGRA,pCallback->unWide, pCallback->unTall, IMAGE_FORMAT_BGRA8888 );
	}

	// need a paint next time
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: browser wants to start loading this url, do we let it?
//-----------------------------------------------------------------------------
bool HTML::OnStartRequest( const char *url, const char *target, const char *pchPostData, bool bIsRedirect )
{
	if ( !url || !Q_stricmp( url, "about:blank") )
		return true ; // this is just webkit loading a new frames contents inside an existing page

	HideFindDialog();
	// see if we have a custom handler for this
	bool bURLHandled = false;
	for (int i = 0; i < m_CustomURLHandlers.Count(); i++)
	{
		if (!Q_strnicmp(m_CustomURLHandlers[i].url,url, Q_strlen(m_CustomURLHandlers[i].url)))
		{
			// we have a custom handler
			Panel *targetPanel = m_CustomURLHandlers[i].hPanel;
			if (targetPanel)
			{
				PostMessage(targetPanel, new KeyValues("CustomURL", "url", m_CustomURLHandlers[i].url ) );
			}

			bURLHandled = true;
		}
	}

	if (bURLHandled)
		return false;

	if ( m_bNewWindowsOnly && bIsRedirect )
	{
		if ( target && ( !Q_stricmp( target, "_blank" ) || !Q_stricmp( target, "_new" ) )  ) // only allow NEW windows (_blank ones)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	if ( target && !Q_strlen( target ) )
	{
		m_sCurrentURL = url;

		KeyValues *pMessage = new KeyValues( "OnURLChanged" );
		pMessage->SetString( "url", url );
		pMessage->SetString( "postdata", pchPostData );
		pMessage->SetInt( "isredirect", bIsRedirect ? 1 : 0 );

		PostActionSignal( pMessage );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: callback from cef thread, load a url please
//-----------------------------------------------------------------------------
void HTML::BrowserStartRequest( HTML_StartRequest_t *pCmd )
{
	bool bRes = OnStartRequest( pCmd->pchURL, pCmd->pchTarget, pCmd->pchPostData, pCmd->bIsRedirect );

	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->AllowStartRequest( m_unBrowserHandle, bRes );
}


//-----------------------------------------------------------------------------
// Purpose: browser went to a new url
//-----------------------------------------------------------------------------
void HTML::BrowserURLChanged( HTML_URLChanged_t *pCmd )
{
	m_sCurrentURL = pCmd->pchURL;

	KeyValues *pMessage = new KeyValues( "OnURLChanged" );
	pMessage->SetString( "url", pCmd->pchURL );
	pMessage->SetString( "postdata", pCmd->pchPostData );
	pMessage->SetInt( "isredirect", pCmd->bIsRedirect ? 1 : 0 );

	PostActionSignal( pMessage );

	OnURLChanged( m_sCurrentURL, pCmd->pchPostData, pCmd->bIsRedirect );
}


//-----------------------------------------------------------------------------
// Purpose: finished loading this page
//-----------------------------------------------------------------------------
void HTML::BrowserFinishedRequest( HTML_FinishedRequest_t *pCmd )
{
	PostActionSignal( new KeyValues( "OnFinishRequest", "url", pCmd->pchURL ) );
	if (  pCmd->pchPageTitle && pCmd->pchPageTitle[0] )
		PostActionSignal( new KeyValues( "PageTitleChange", "title", pCmd->pchPageTitle ) );

	CUtlMap < CUtlString, CUtlString > mapHeaders;
	mapHeaders.SetLessFunc( UtlStringLessFunc );
	// headers are no longer reported on loads

	OnFinishRequest( pCmd->pchURL, pCmd->pchPageTitle, mapHeaders );
}

//-----------------------------------------------------------------------------
// Purpose: browser wants to open a new tab
//-----------------------------------------------------------------------------
void HTML::BrowserOpenNewTab( HTML_OpenLinkInNewTab_t *pCmd )
{
	(pCmd);
	// Not supported by default, if a child class overrides us and knows how to handle tabs, then it can do this.
}

//-----------------------------------------------------------------------------
// Purpose: display a new html window
//-----------------------------------------------------------------------------
void HTML::BrowserPopupHTMLWindow( HTML_NewWindow_t *pCmd )
{
	HTMLPopup *p = new HTMLPopup( this, pCmd->pchURL, "" );
	int wide = pCmd->unWide;
	int tall = pCmd->unTall;
	if ( wide == 0 || tall == 0 )
	{
		wide = MAX( 640, GetWide() );
		tall = MAX( 480, GetTall() );
	}

	p->SetBounds( pCmd->unX, pCmd->unY, wide, tall  );
	p->SetDeleteSelfOnClose( true );
	if ( pCmd->unX == 0 || pCmd->unY == 0 )
		p->MoveToCenterOfScreen();
	p->Activate();

}


//-----------------------------------------------------------------------------
// Purpose: browser telling us the page title
//-----------------------------------------------------------------------------
void HTML::BrowserSetHTMLTitle( HTML_ChangedTitle_t *pCmd )
{
	PostMessage( GetParent(), new KeyValues( "OnSetHTMLTitle", "title", pCmd->pchTitle ) );
	OnSetHTMLTitle( pCmd->pchTitle );
}


//-----------------------------------------------------------------------------
// Purpose: status bar details
//-----------------------------------------------------------------------------
void HTML::BrowserStatusText( HTML_StatusText_t *pCmd )
{
	PostActionSignal( new KeyValues( "OnSetStatusText", "status", pCmd->pchMsg ) );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us to use this cursor
//-----------------------------------------------------------------------------
void HTML::BrowserSetCursor( HTML_SetCursor_t *pCmd )
{
	vgui::CursorCode cursor = dc_last;

	switch ( pCmd->eMouseCursor )
	{
	case ISteamHTMLSurface::dc_user:
		cursor = dc_user; 
		break;
	case ISteamHTMLSurface::dc_none:
		cursor = dc_none;
		break;
	default:
	case ISteamHTMLSurface::dc_arrow:
		cursor = dc_arrow;
		break;
	case ISteamHTMLSurface::dc_ibeam:
		cursor = dc_ibeam;
		break;
	case ISteamHTMLSurface::dc_hourglass:
		cursor = dc_hourglass;
		break;
	case ISteamHTMLSurface::dc_waitarrow:
		cursor = dc_waitarrow;
		break;
	case ISteamHTMLSurface::dc_crosshair:
		cursor = dc_crosshair;
		break;
	case ISteamHTMLSurface::dc_up:
		cursor = dc_up;
		break;
	/*case ISteamHTMLSurface::dc_sizenw:
		cursor = dc_sizenw;
		break;
	case ISteamHTMLSurface::dc_sizese:
		cursor = dc_sizese;
		break;
	case ISteamHTMLSurface::dc_sizene:
		cursor = dc_sizene;
		break;
	case ISteamHTMLSurface::dc_sizesw:
		cursor = dc_sizesw;
		break;
	case ISteamHTMLSurface::dc_sizew:
		cursor = dc_sizew;
		break;
	case ISteamHTMLSurface::dc_sizee:
		cursor = dc_sizee;
		break;
	case ISteamHTMLSurface::dc_sizen:
		cursor = dc_sizen;
		break;
	case ISteamHTMLSurface::dc_sizes:
		cursor = dc_sizes;
		break;*/
	case ISteamHTMLSurface::dc_sizewe:
		cursor = dc_sizewe;
		break;
	case ISteamHTMLSurface::dc_sizens:
		cursor = dc_sizens;
		break;
	case ISteamHTMLSurface::dc_sizeall:
		cursor = dc_sizeall;
		break;
	case ISteamHTMLSurface::dc_no:
		cursor = dc_no;
		break;
	case ISteamHTMLSurface::dc_hand:
		cursor = dc_hand;
		break;
	case ISteamHTMLSurface::dc_blank:
		cursor = dc_blank;
		break;
/*	case ISteamHTMLSurface::dc_middle_pan:
		cursor = dc_middle_pan;
		break;
	case ISteamHTMLSurface::dc_north_pan:
		cursor = dc_north_pan;
		break;
	case ISteamHTMLSurface::dc_north_east_pan:
		cursor = dc_north_east_pan;
		break;
	case ISteamHTMLSurface::dc_east_pan:
		cursor = dc_east_pan;
		break;
	case ISteamHTMLSurface::dc_south_east_pan:
		cursor = dc_south_east_pan;
		break;
	case ISteamHTMLSurface::dc_south_pan:
		cursor = dc_south_pan;
		break;
	case ISteamHTMLSurface::dc_south_west_pan:
		cursor = dc_south_west_pan;
		break;
	case ISteamHTMLSurface::dc_west_pan:
		cursor = dc_west_pan;
		break;
	case ISteamHTMLSurface::dc_north_west_pan:
		cursor = dc_north_west_pan;
		break;
	case ISteamHTMLSurface::dc_alias:
		cursor = dc_alias;
		break;
	case ISteamHTMLSurface::dc_cell:
		cursor = dc_cell;
		break;
	case ISteamHTMLSurface::dc_colresize:
		cursor = dc_colresize;
		break;
	case ISteamHTMLSurface::dc_copycur:
		cursor = dc_copycur;
		break;
	case ISteamHTMLSurface::dc_verticaltext:
		cursor = dc_verticaltext;
		break;
	case ISteamHTMLSurface::dc_rowresize:
		cursor = dc_rowresize;
		break;
	case ISteamHTMLSurface::dc_zoomin:
		cursor = dc_zoomin;
		break;
	case ISteamHTMLSurface::dc_zoomout:
		cursor = dc_zoomout;
		break;
	case ISteamHTMLSurface::dc_custom:
		cursor = dc_custom;
		break;
	case ISteamHTMLSurface::dc_help:
		cursor = dc_help;
		break;*/

	}

	if ( cursor >= dc_last )
	{
		cursor = dc_arrow;
	}
	
	SetCursor( cursor );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling to show the file loading dialog
//-----------------------------------------------------------------------------
void HTML::BrowserFileLoadDialog( HTML_FileOpenDialog_t *pCmd )
{
	// couldn't access an OS-specific dialog, use the internal one
	if ( m_hFileOpenDialog.Get() )
	{
		delete m_hFileOpenDialog.Get();
		m_hFileOpenDialog = NULL;
	}
	m_hFileOpenDialog = new FileOpenDialog( this, pCmd->pchTitle, true );
	m_hFileOpenDialog->SetStartDirectory( pCmd->pchInitialFile );
	m_hFileOpenDialog->AddActionSignalTarget( this );
	m_hFileOpenDialog->SetAutoDelete( true );
	m_hFileOpenDialog->DoModal(false);
}


//-----------------------------------------------------------------------------
// Purpose: browser asking to show a tooltip
//-----------------------------------------------------------------------------
void HTML::BrowserShowToolTip( HTML_ShowToolTip_t *pCmd )
{
/*	
	BR FIXME
	Tooltip *tip = GetTooltip();
	tip->SetText( pCmd->text().c_str() );
	tip->SetTooltipFormatToMultiLine();
	tip->SetTooltipDelayMS( 250 );
	tip->SetMaxToolTipWidth( MAX( 200, GetWide()/2 ) );
	tip->ShowTooltip( this );
	*/
	
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us to update tool tip text
//-----------------------------------------------------------------------------
void HTML::BrowserUpdateToolTip( HTML_UpdateToolTip_t *pCmd )
{
//	GetTooltip()->SetText( pCmd->text().c_str() );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling that it is done with the tip
//-----------------------------------------------------------------------------
void HTML::BrowserHideToolTip( HTML_HideToolTip_t *pCmd )
{
//	GetTooltip()->HideTooltip();
//	DeleteToolTip();
}


//-----------------------------------------------------------------------------
// Purpose: callback when performing a search
//-----------------------------------------------------------------------------
void HTML::BrowserSearchResults( HTML_SearchResults_t *pCmd )
{
	if ( pCmd->unResults == 0 )
		m_pFindBar->HideCountLabel();
	else
		m_pFindBar->ShowCountLabel();

	if ( pCmd->unResults > 0 )
		m_pFindBar->SetDialogVariable( "findcount", (int)pCmd->unResults );
	if ( pCmd->unCurrentMatch > 0 )
		m_pFindBar->SetDialogVariable( "findactive", (int)pCmd->unCurrentMatch );
	m_pFindBar->InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us it had a close requested
//-----------------------------------------------------------------------------
void HTML::BrowserClose( HTML_CloseBrowser_t *pCmd )
{
	PostActionSignal( new KeyValues( "OnCloseWindow" ) );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us the size of the horizontal scrollbars
//-----------------------------------------------------------------------------
void HTML::BrowserHorizontalScrollBarSizeResponse( HTML_HorizontalScroll_t *pCmd )
{
	ScrollData_t scrollHorizontal;
	scrollHorizontal.m_nScroll = pCmd->unScrollCurrent;
	scrollHorizontal.m_nMax = pCmd->unScrollMax;
	scrollHorizontal.m_bVisible = pCmd->bVisible;
	scrollHorizontal.m_flZoom = pCmd->flPageScale;

	if ( scrollHorizontal != m_scrollHorizontal )
	{
		m_scrollHorizontal = scrollHorizontal;
		UpdateSizeAndScrollBars();
		m_bNeedsFullTextureUpload = true;
	}
	else
		m_scrollHorizontal = scrollHorizontal;
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us the size of the vertical scrollbars
//-----------------------------------------------------------------------------
void HTML::BrowserVerticalScrollBarSizeResponse( HTML_VerticalScroll_t *pCmd )
{
	ScrollData_t scrollVertical;
	scrollVertical.m_nScroll = pCmd->unScrollCurrent;
	scrollVertical.m_nMax = pCmd->unScrollMax;
	scrollVertical.m_bVisible = pCmd->bVisible;
	scrollVertical.m_flZoom = pCmd->flPageScale;

	if ( scrollVertical != m_scrollVertical )
	{
		m_scrollVertical = scrollVertical;
		UpdateSizeAndScrollBars();
		m_bNeedsFullTextureUpload = true;
	}
	else
		m_scrollVertical = scrollVertical;
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us what is at this location on the page
//-----------------------------------------------------------------------------
void HTML::BrowserLinkAtPositionResponse( HTML_LinkAtPosition_t *pCmd )
{
	m_LinkAtPos.m_sURL = pCmd->pchURL;
	m_LinkAtPos.m_nX = pCmd->x;
	m_LinkAtPos.m_nY = pCmd->y;

	m_pContextMenu->SetItemVisible( m_iCopyLinkMenuItemID, !m_LinkAtPos.m_sURL.IsEmpty() ? true : false );
	if ( m_bRequestingDragURL )
	{
		m_bRequestingDragURL = false;
		m_sDragURL = m_LinkAtPos.m_sURL;
		// make sure we get notified when the mouse gets released
		if ( !m_sDragURL.IsEmpty() )
		{
			input()->SetMouseCapture( GetVPanel() );
		}
	}

	if ( m_bRequestingCopyLink )
	{
		m_bRequestingCopyLink = false;
		if ( !m_LinkAtPos.m_sURL.IsEmpty() )
			system()->SetClipboardText( m_LinkAtPos.m_sURL, m_LinkAtPos.m_sURL.Length() );
		else
			system()->SetClipboardText( "", 1 );
	}

	OnLinkAtPosition( m_LinkAtPos.m_sURL );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us to pop a javascript alert dialog
//-----------------------------------------------------------------------------
void HTML::BrowserJSAlert( HTML_JSAlert_t *pCmd )
{
	MessageBox *pDlg = new MessageBox( m_sCurrentURL, (const char *)pCmd->pchMessage, this );
	pDlg->AddActionSignalTarget( this );
	pDlg->SetCommand( new KeyValues( "DismissJSDialog", "result", false ) );
	pDlg->DoModal();
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us to pop a js confirm dialog
//-----------------------------------------------------------------------------
void HTML::BrowserJSConfirm( HTML_JSConfirm_t *pCmd )
{
	QueryBox *pDlg = new QueryBox( m_sCurrentURL, (const char *)pCmd->pchMessage, this );
	pDlg->AddActionSignalTarget( this );
	pDlg->SetOKCommand( new KeyValues( "DismissJSDialog", "result", true ) );
	pDlg->SetCancelCommand( new KeyValues( "DismissJSDialog", "result", false ) );
	pDlg->DoModal();
}


//-----------------------------------------------------------------------------
// Purpose: got an answer from the dialog, tell cef
//-----------------------------------------------------------------------------
void HTML::DismissJSDialog( int bResult )
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->JSDialogResponse( m_unBrowserHandle, bResult );
};


//-----------------------------------------------------------------------------
// Purpose: browser telling us the state of back and forward buttons
//-----------------------------------------------------------------------------
void HTML::BrowserCanGoBackandForward( HTML_CanGoBackAndForward_t *pCmd )
{
	m_bCanGoBack = pCmd->bCanGoBack;
	m_bCanGoForward = pCmd->bCanGoForward;
}


//-----------------------------------------------------------------------------
// Purpose: ask the browser for what is at this x,y
//-----------------------------------------------------------------------------
void HTML::GetLinkAtPosition( int x, int y )
{
	if (m_SteamAPIContext.SteamHTMLSurface())
		m_SteamAPIContext.SteamHTMLSurface()->GetLinkAtPosition( m_unBrowserHandle, x, y );
}


//-----------------------------------------------------------------------------
// Purpose: update the size of the browser itself and scrollbars it shows
//-----------------------------------------------------------------------------
void HTML::UpdateSizeAndScrollBars()
{
	BrowserResize();
	InvalidateLayout();
}


