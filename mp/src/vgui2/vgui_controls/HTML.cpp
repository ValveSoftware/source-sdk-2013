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

#include <tier0/memdbgoff.h>
#include <tier0/valve_minmax_off.h>
#include "htmlmessages.pb.h"
#include <tier0/valve_minmax_on.h>
#include <tier0/memdbgon.h>

#include "html/ipainthtml.h"
#include "html/ihtmlchrome.h"
#include "html/ichromehtmlwrapper.h"
#include "html/htmlprotobuf.h"

#include "filesystem.h"
#include "../vgui2/src/vgui_key_translation.h"

#undef PostMessage
#undef MessageBox

#include "OfflineMode.h"

// memdbgon must be the last include file in a .cpp file
#include "tier0/memdbgon.h"

using namespace vgui;

// helper to send IPC messages to the CEF thread
#define DISPATCH_MESSAGE( eCmd ) \
	if (surface()->AccessChromeHTMLController()) \
	{ \
		cmd.Body().set_browser_handle( m_iBrowser );\
		HTMLCommandBuffer_t *pBuf = surface()->AccessChromeHTMLController()->GetFreeCommandBuffer( eCmd, m_iBrowser ); \
		cmd.SerializeCrossProc( &pBuf->m_Buffer ); \
		if ( m_iBrowser == -1 ) { m_vecPendingMessages.AddToTail( pBuf ); } \
		else \
		{ \
		surface()->AccessChromeHTMLController()->PushCommand( pBuf ); \
		surface()->AccessChromeHTMLController()->WakeThread(); \
		}\
	} \

const int k_nMaxCustomCursors = 2; // the max number of custom cursors we keep cached PER html control

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
// Purpose: a vgui container for popup menus displayed by a control, only 1 menu for any control can be visible at a time
//-----------------------------------------------------------------------------
class HTMLComboBoxHost : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( HTMLComboBoxHost, EditablePanel );
public:
	HTMLComboBoxHost( HTML *parent, const char *panelName ) : EditablePanel( parent, panelName ) 
	{ 
		m_pParent = parent; 
		MakePopup(false);
	}
	~HTMLComboBoxHost() {} 

	virtual void PaintBackground();

	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnCursorMoved(int x,int y);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyCodeReleased(KeyCode code);
	virtual void OnMouseWheeled(int delta);

	virtual void OnKillFocus()
	{
		if ( vgui::input()->GetFocus() != m_pParent->GetVPanel() ) // if its not our parent trying to steal focus
		{
			BaseClass::OnKillFocus();
			if ( m_pParent )
				m_pParent->HidePopup();
		}
	}

	virtual void PerformLayout()
	{
	// no op the perform layout as we just render the html controls popup texture into it
	// we don't want the menu logic trying to play with its size
	}


private:
	HTML *m_pParent;
	CUtlVector<HTMLCommandBuffer_t *> m_vecPendingMessages;
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
HTML::HTML(Panel *parent, const char *name, bool allowJavaScript, bool bPopupWindow) : Panel(parent, name)
{
	m_iHTMLTextureID = 0;
	m_iComboBoxTextureID = 0;
	m_bCanGoBack = false;
	m_bCanGoForward = false;
	m_bInFind = false;
	m_bRequestingDragURL = false;
	m_bRequestingCopyLink = false;
	m_flZoom = 100.0f;
	m_iBrowser = -1;
	m_bNeedsFullTextureUpload = false;

	m_pInteriorPanel = new HTMLInterior( this );
	SetPostChildPaintEnabled( true );
	if (surface() && surface()->AccessChromeHTMLController())
	{
		surface()->AccessChromeHTMLController()->CreateBrowser( this, bPopupWindow, surface()->GetWebkitHTMLUserAgentString() );
	}
	else
	{
		Warning("Unable to access ChromeHTMLController");
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
	
	m_pComboBoxHost = new HTMLComboBoxHost( this, "ComboBoxHost" );
	m_pComboBoxHost->SetPaintBackgroundEnabled( true );
	m_pComboBoxHost->SetVisible( false );

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
// Purpose: Destructor
//-----------------------------------------------------------------------------
HTML::~HTML()
{
	m_pContextMenu->MarkForDeletion();

	if (surface()->AccessChromeHTMLController())
	{
		surface()->AccessChromeHTMLController()->RemoveBrowser( this );
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
// Purpose: paint the combo box texture if we have one
//-----------------------------------------------------------------------------
void HTML::PaintComboBox()
{
	BaseClass::Paint();
	if ( m_iComboBoxTextureID != 0 )
	{
		surface()->DrawSetTexture( m_iComboBoxTextureID );
		surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
		int tw = m_allocedComboBoxWidth;
		int tt = m_allocedComboBoxHeight;
		surface()->DrawTexturedRect( 0, 0, tw, tt );
	}

}


//-----------------------------------------------------------------------------
// Purpose: overrides panel class, paints a texture of the HTML window as a background
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::PaintBackground()
{
	BaseClass::PaintBackground();

	m_pParent->PaintComboBox();
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
	CHTMLProtoBufMsg<CMsgBrowserPosition> cmd( eHTMLCommands_BrowserPosition );
	cmd.Body().set_x( nPanelAbsX );
	cmd.Body().set_y( nPanelAbsY );
	DISPATCH_MESSAGE( eHTMLCommands_BrowserPosition );

	if ( m_pComboBoxHost && m_pComboBoxHost->IsVisible() )
	{
		m_pComboBoxHost->SetVisible( false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: calculates the need for and position of both horizontal and vertical scroll bars
//-----------------------------------------------------------------------------
void HTML::CalcScrollBars(int w, int h)
{
	bool bScrollbarVisible = _vbar->IsVisible();

	if ( m_bScrollBarEnabled )
	{
		for ( int i = 0; i < 2; i++ )
		{
			int scrollx, scrolly, scrollwide, scrolltall;
			bool bVisible = false;
			if ( i==0 )
			{
				scrollx = m_scrollHorizontal.m_nX;
				scrolly = m_scrollHorizontal.m_nY;
				scrollwide = m_scrollHorizontal.m_nWide;
				scrolltall = m_scrollHorizontal.m_nTall;
				bVisible = m_scrollHorizontal.m_bVisible;

				// scrollbar positioning tweaks - should be moved into a resource file
				scrollwide += 14;
				scrolltall += 5;
			}
			else
			{
				scrollx = m_scrollVertical.m_nX;
				scrolly = m_scrollVertical.m_nY;
				scrollwide = m_scrollVertical.m_nWide;
				scrolltall = m_scrollVertical.m_nTall;
				bVisible = m_scrollVertical.m_bVisible;

				// scrollbar positioning tweaks - should be moved into a resource file
				//scrollx -= 3;
				if ( m_scrollHorizontal.m_bVisible )
					scrolltall += 16;
				else
					scrolltall -= 2;

				scrollwide += 5;
			}
			
			if ( bVisible && scrollwide && scrolltall )
			{
				int panelWide, panelTall;
				GetSize( panelWide, panelTall );

				ScrollBar *bar = _vbar; 
				if ( i == 0 )
					bar = _hbar;
				
				if (!bar->IsVisible())
				{
					bar->SetVisible(true);
					// displayable area has changed, need to force an update
					PostMessage(this, new KeyValues("OnSliderMoved"), 0.02f);
				}

				int rangeWindow = panelTall - scrollwide;
				if ( i==0 )
					rangeWindow = panelWide - scrolltall;
				int range = m_scrollVertical.m_nMax + m_scrollVertical.m_nTall;
				if ( i == 0 )
					range = m_scrollHorizontal.m_nMax + m_scrollVertical.m_nWide;
				int curValue = m_scrollVertical.m_nScroll;
				if ( i == 0 )
					curValue = m_scrollHorizontal.m_nScroll;

				bar->SetEnabled(false);
				bar->SetRangeWindow( rangeWindow );
				bar->SetRange( 0, range ); // we want the range [0.. (img_h - h)], but the scrollbar actually returns [0..(range-rangeWindow)] so make sure -h gets deducted from the max range value	
				bar->SetButtonPressedScrollValue( 5 );
				if ( curValue > ( bar->GetValue() + 5 ) || curValue < (bar->GetValue() - 5 ) )
					bar->SetValue( curValue );

				if ( i == 0 )
				{
					bar->SetPos( 0, h - scrolltall - 1 );
					bar->SetWide( scrollwide );
					bar->SetTall( scrolltall );
				}
				else
				{
					bar->SetPos( w - scrollwide, 0 );
					bar->SetTall( scrolltall );
					bar->SetWide( scrollwide );
				}

				if ( i == 0 )
					m_iScrollBorderY=scrolltall;
				else
					m_iScrollBorderX=scrollwide;
			}
			else
			{
				if ( i == 0 )
				{
					m_iScrollBorderY=0;
					_hbar->SetVisible( false );
				}
				else
				{
					m_iScrollBorderX=0;
					_vbar->SetVisible( false );

				}
			}
		}
	}
	else
	{
		m_iScrollBorderX = 0;
		m_iScrollBorderY=0;
		_vbar->SetVisible(false);
		_hbar->SetVisible(false);
	}

	if ( bScrollbarVisible != _vbar->IsVisible() )
		InvalidateLayout();
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
	if ( m_iBrowser < 0 )
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
	
			CHTMLProtoBufMsg<CMsgPostURL> cmd( eHTMLCommands_PostURL );
			cmd.Body().set_url( htmlLocation );
			DISPATCH_MESSAGE( eHTMLCommands_PostURL );

		}
		else
		{
			CHTMLProtoBufMsg<CMsgPostURL> cmd( eHTMLCommands_PostURL );
			cmd.Body().set_url( URL );
			DISPATCH_MESSAGE( eHTMLCommands_PostURL );
		}
	}
	else
	{
		if ( pchPostData && Q_strlen(pchPostData) > 0 )
		{
			CHTMLProtoBufMsg<CMsgPostURL> cmd( eHTMLCommands_PostURL );
			cmd.Body().set_url( URL );
			cmd.Body().set_post( pchPostData );
			DISPATCH_MESSAGE( eHTMLCommands_PostURL );

		}
		else
		{			
			CHTMLProtoBufMsg<CMsgPostURL> cmd( eHTMLCommands_PostURL );
			cmd.Body().set_url( URL );
			DISPATCH_MESSAGE( eHTMLCommands_PostURL );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: opens the URL, will accept any URL that IE accepts
//-----------------------------------------------------------------------------
bool HTML::StopLoading()
{
	CHTMLProtoBufMsg<CMsgStopLoad> cmd( eHTMLCommands_StopLoad );
	DISPATCH_MESSAGE( eHTMLCommands_StopLoad );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: refreshes the current page
//-----------------------------------------------------------------------------
bool HTML::Refresh()
{
	CHTMLProtoBufMsg<CMsgReload> cmd( eHTMLCommands_Reload );
	DISPATCH_MESSAGE( eHTMLCommands_Reload );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Tells the browser control to go back
//-----------------------------------------------------------------------------
void HTML::GoBack()
{
	CHTMLProtoBufMsg<CMsgGoBack> cmd( eHTMLCommands_GoBack );
	DISPATCH_MESSAGE( eHTMLCommands_GoBack );
}


//-----------------------------------------------------------------------------
// Purpose: Tells the browser control to go forward
//-----------------------------------------------------------------------------
void HTML::GoForward()
{
	CHTMLProtoBufMsg<CMsgGoForward> cmd( eHTMLCommands_GoForward );
	DISPATCH_MESSAGE( eHTMLCommands_GoForward );
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
	UpdateCachedHTMLValues();
#ifdef WIN32
	// under windows we get stuck in the windows message loop pushing out WM_WINDOWPOSCHANGED without returning in the windproc loop
	// so we need to manually pump the html dispatching of messages here
	if ( surface() && surface()->AccessChromeHTMLController() )
	{
		surface()->AccessChromeHTMLController()->RunFrame();
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Run javascript in the page
//-----------------------------------------------------------------------------
void HTML::RunJavascript( const char *pchScript )
{
	CHTMLProtoBufMsg<CMsgExecuteJavaScript> cmd( eHTMLCommands_ExecuteJavaScript );
	cmd.Body().set_script( pchScript );
	DISPATCH_MESSAGE( eHTMLCommands_ExecuteJavaScript );
}




//-----------------------------------------------------------------------------
// Purpose: helper to convert UI mouse codes to CEF ones
//-----------------------------------------------------------------------------
int ConvertMouseCodeToCEFCode( MouseCode code )
{
	switch( code )
	{
	case MOUSE_LEFT:
		return IInputEventHTML::eButtonLeft;
		break;
	case MOUSE_RIGHT:
		return IInputEventHTML::eButtonRight;
		break;
	case MOUSE_MIDDLE:
		return IInputEventHTML::eButtonMiddle;
		break;
	default:
		return IInputEventHTML::eButtonLeft;
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
		CHTMLProtoBufMsg<CMsgMouseDown> cmd( eHTMLCommands_MouseDown );
		cmd.Body().set_mouse_button( ConvertMouseCodeToCEFCode( code ) );
		DISPATCH_MESSAGE( eHTMLCommands_MouseDown );
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

		if ( !m_sDragURL.IsEmpty() && input()->GetMouseOver() != GetVPanel() && input()->GetMouseOver() != NULL )
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

	CHTMLProtoBufMsg<CMsgMouseUp> cmd( eHTMLCommands_MouseUp );
	cmd.Body().set_mouse_button( ConvertMouseCodeToCEFCode( code ) );
	DISPATCH_MESSAGE( eHTMLCommands_MouseUp );
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

		CHTMLProtoBufMsg<CMsgMouseMove> cmd( eHTMLCommands_MouseMove );
		cmd.Body().set_x( m_iMouseX ); 
		cmd.Body().set_y( m_iMouseY );
		DISPATCH_MESSAGE( eHTMLCommands_MouseMove );
	}
	else if ( !m_sDragURL.IsEmpty() )
	{
		if ( input()->GetMouseOver() == NULL )
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
	CHTMLProtoBufMsg<CMsgMouseDblClick> cmd( eHTMLCommands_MouseDblClick );
	cmd.Body().set_mouse_button( ConvertMouseCodeToCEFCode( code ) );
	DISPATCH_MESSAGE( eHTMLCommands_MouseDblClick );
}


//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser (we don't current do this)
//-----------------------------------------------------------------------------
void HTML::OnKeyTyped(wchar_t unichar)
{
	CHTMLProtoBufMsg<CMsgKeyChar> cmd( eHTMLCommands_KeyChar );
	cmd.Body().set_unichar( unichar );
	DISPATCH_MESSAGE( eHTMLCommands_KeyChar );
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
// Purpose: return the bitmask of any modifier keys that are currently down
//-----------------------------------------------------------------------------
int GetKeyModifiers()
{
	// Any time a key is pressed reset modifier list as well
	int nModifierCodes = 0;
	if( vgui::input()->IsKeyDown( KEY_LCONTROL ) || vgui::input()->IsKeyDown( KEY_RCONTROL ) )
		nModifierCodes |= IInputEventHTML::CrtlDown;

	if( vgui::input()->IsKeyDown( KEY_LALT ) || vgui::input()->IsKeyDown( KEY_RALT ) )
		nModifierCodes |= IInputEventHTML::AltDown;

	if( vgui::input()->IsKeyDown( KEY_LSHIFT ) || vgui::input()->IsKeyDown( KEY_RSHIFT ) )
		nModifierCodes |= IInputEventHTML::ShiftDown;

#ifdef OSX
	// for now pipe through the cmd-key to be like the control key so we get copy/paste
	if( vgui::input()->IsKeyDown( KEY_LWIN ) || vgui::input()->IsKeyDown( KEY_RWIN ) )
		nModifierCodes |= IInputEventHTML::CrtlDown;
#endif

	return nModifierCodes;
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

	CHTMLProtoBufMsg<CMsgKeyDown> cmd( eHTMLCommands_KeyDown );
	cmd.Body().set_keycode( KeyCode_VGUIToVirtualKey(code) );
	cmd.Body().set_modifiers( GetKeyModifiers() );
	DISPATCH_MESSAGE( eHTMLCommands_KeyDown );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTML::OnKeyCodeReleased(KeyCode code)
{
	CHTMLProtoBufMsg<CMsgKeyDown> cmd( eHTMLCommands_KeyUp );
	cmd.Body().set_keycode( KeyCode_VGUIToVirtualKey(code) );
	cmd.Body().set_modifiers( GetKeyModifiers() );
	DISPATCH_MESSAGE( eHTMLCommands_KeyUp );
}


//-----------------------------------------------------------------------------
// Purpose: scrolls the vertical scroll bar on a web page
//-----------------------------------------------------------------------------
void HTML::OnMouseWheeled(int delta)
{	
	if (_vbar && ( ( m_pComboBoxHost && !m_pComboBoxHost->IsVisible() ) ) )
	{
		int val = _vbar->GetValue();
		val -= (delta * 100.0/3.0 ); // 100 for every 3 lines matches chromes code
		_vbar->SetValue(val);
	}

	CHTMLProtoBufMsg<CMsgMouseWheel> cmd( eHTMLCommands_MouseWheel );
	cmd.Body().set_delta( delta );
	DISPATCH_MESSAGE( eHTMLCommands_MouseWheel );
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
			CHTMLProtoBufMsg<CMsgBrowserSize> cmd( eHTMLCommands_BrowserSize );
			cmd.Body().set_width( m_iWideLastHTMLSize );
			cmd.Body().set_height( m_iTalLastHTMLSize );
			DISPATCH_MESSAGE( eHTMLCommands_BrowserSize );
		}

	
		// webkit forgets the scroll offset when you resize (it saves the scroll in a DC and a resize throws away the DC)
		// so just tell it after the resize
		int scrollV = _vbar->GetValue();
		int scrollH = _hbar->GetValue();

		{
			CHTMLProtoBufMsg<CMsgSetHorizontalScroll> cmd( eHTMLCommands_SetHorizontalScroll );
			cmd.Body().set_scroll( scrollH );
			DISPATCH_MESSAGE( eHTMLCommands_SetHorizontalScroll );
		}
		{
			CHTMLProtoBufMsg<CMsgSetVerticalScroll> cmd( eHTMLCommands_SetVerticalScroll );
			cmd.Body().set_scroll( scrollV );
			DISPATCH_MESSAGE( eHTMLCommands_SetVerticalScroll );
		}
	}

}


//-----------------------------------------------------------------------------
// Purpose: when a slider moves causes the IE images to re-render itself
//-----------------------------------------------------------------------------
void HTML::OnSliderMoved()
{
	if(_hbar->IsVisible())
	{
		int scrollX =_hbar->GetValue();
		CHTMLProtoBufMsg<CMsgSetHorizontalScroll> cmd( eHTMLCommands_SetHorizontalScroll );
		cmd.Body().set_scroll( scrollX );
		DISPATCH_MESSAGE( eHTMLCommands_SetHorizontalScroll );
	}

	if(_vbar->IsVisible())
	{
		int scrollY=_vbar->GetValue();
		CHTMLProtoBufMsg<CMsgSetVerticalScroll> cmd( eHTMLCommands_SetVerticalScroll );
		cmd.Body().set_scroll( scrollY );
		DISPATCH_MESSAGE( eHTMLCommands_SetVerticalScroll );
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
	CHTMLProtoBufMsg<CMsgAddHeader> cmd( eHTMLCommands_AddHeader );
	cmd.Body().set_key( pchHeader );
	cmd.Body().set_value( pchValue );
	DISPATCH_MESSAGE( eHTMLCommands_AddHeader );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTML::OnSetFocus()
{
	BaseClass::OnSetFocus();
	CHTMLProtoBufMsg<CMsgSetFocus> cmd( eHTMLCommands_SetFocus );
	cmd.Body().set_focus( true );
	DISPATCH_MESSAGE( eHTMLCommands_SetFocus );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTML::OnKillFocus()
{
	if ( vgui::input()->GetFocus() != m_pComboBoxHost->GetVPanel() ) // if its not the menu stealing our focus
		BaseClass::OnKillFocus();

	// Don't clear the actual html focus if a context menu is what took focus
	if ( m_pContextMenu->HasFocus() )
		return;

	if ( m_pComboBoxHost->HasFocus() )
		return;

	CHTMLProtoBufMsg<CMsgSetFocus> cmd( eHTMLCommands_SetFocus );
	cmd.Body().set_focus( false );
	DISPATCH_MESSAGE( eHTMLCommands_SetFocus );
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
		CHTMLProtoBufMsg<CMsgViewSource> cmd( eHTMLCommands_ViewSource );
		DISPATCH_MESSAGE( eHTMLCommands_ViewSource );
	}
	else if ( !Q_stricmp( pchCommand, "copy" ) )
	{
		CHTMLProtoBufMsg<CMsgCopy> cmd( eHTMLCommands_Copy );
		DISPATCH_MESSAGE( eHTMLCommands_Copy );
	}
	else if ( !Q_stricmp( pchCommand, "paste" ) )
	{
		CHTMLProtoBufMsg<CMsgPaste> cmd( eHTMLCommands_Paste );
		DISPATCH_MESSAGE( eHTMLCommands_Paste );
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
	CHTMLProtoBufMsg<CMsgFileLoadDialogResponse> cmd( eHTMLCommands_FileLoadDialogResponse );
	cmd.Body().add_files( pchSelectedFile );
	DISPATCH_MESSAGE( eHTMLCommands_FileLoadDialogResponse );
	m_hFileOpenDialog->Close();
}

//-----------------------------------------------------------------------------
// Purpose: called when the user dismissed the file dialog with no selection
//-----------------------------------------------------------------------------
void HTML::OnFileSelectionCancelled()
{
	CHTMLProtoBufMsg<CMsgFileLoadDialogResponse> cmd( eHTMLCommands_FileLoadDialogResponse );
	DISPATCH_MESSAGE( eHTMLCommands_FileLoadDialogResponse );
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

	CHTMLProtoBufMsg<CMsgFind> cmd( eHTMLCommands_Find );
	cmd.Body().set_find( pchSubStr );
	cmd.Body().set_infind( m_bInFind );
	DISPATCH_MESSAGE( eHTMLCommands_Find );
}


//-----------------------------------------------------------------------------
// Purpose: find any text on the html page with this sub string
//-----------------------------------------------------------------------------
void HTML::FindPrevious()
{
	CHTMLProtoBufMsg<CMsgFind> cmd( eHTMLCommands_Find );
	cmd.Body().set_find( m_sLastSearchString );
	cmd.Body().set_infind( m_bInFind );
	cmd.Body().set_reverse( true );
	DISPATCH_MESSAGE( eHTMLCommands_Find );
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
	CHTMLProtoBufMsg<CMsgStopFind> cmd( eHTMLCommands_StopFind );
	DISPATCH_MESSAGE( eHTMLCommands_StopFind );

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
// Purpose: passes mouse clicks to the control
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnMousePressed(MouseCode code)
{
	m_pParent->OnMousePressed(code);
}


//-----------------------------------------------------------------------------
// Purpose: passes mouse up events
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnMouseReleased(MouseCode code)
{
	m_pParent->OnMouseReleased(code);
}


//-----------------------------------------------------------------------------
// Purpose: keeps track of where the cursor is
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnCursorMoved(int x,int y)
{
	// Only do this when we are over the current panel
	if ( vgui::input()->GetMouseOver() == GetVPanel() )
	{
		int m_iBrowser = m_pParent->BrowserGetIndex();
		CHTMLProtoBufMsg<CMsgMouseMove> cmd( eHTMLCommands_MouseMove );
		cmd.Body().set_x( x ); 
		cmd.Body().set_y( y );
		DISPATCH_MESSAGE( eHTMLCommands_MouseMove );
	}
}


//-----------------------------------------------------------------------------
// Purpose: passes double click events to the browser
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnMouseDoublePressed(MouseCode code)
{
	m_pParent->OnMouseDoublePressed(code);
}


//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser (we don't current do this)
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnKeyTyped(wchar_t unichar)
{
	m_pParent->OnKeyTyped(unichar);
}


//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser 
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnKeyCodeTyped(KeyCode code)
{
	m_pParent->OnKeyCodeTyped(code);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnKeyCodeReleased(KeyCode code)
{
	m_pParent->OnKeyCodeReleased(code);
}


//-----------------------------------------------------------------------------
// Purpose: scrolls the vertical scroll bar on a web page
//-----------------------------------------------------------------------------
void HTMLComboBoxHost::OnMouseWheeled(int delta)
{	
	 m_pParent->OnMouseWheeled( delta );
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
	LoadControlSettings( "resource/layout/htmlfindbar.layout" );
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
// Purpose: TEMPORARY WORKAROUND FOR VS2005 INTERFACE ISSUES
//-----------------------------------------------------------------------------
#define TMP_HTML_MSG_FUNC( eHTMLCommand, bodyType, commandFunc ) \
	case eHTMLCommand: \
	{ \
		CHTMLProtoBufMsg< bodyType > cmd( pCmd->m_eCmd ); \
		if ( cmd.BDeserializeCrossProc( &pCmd->m_Buffer ) ) \
			commandFunc( &cmd.BodyConst() ); \
	} \
	break;  \

void HTML::_DeserializeAndDispatch( HTMLCommandBuffer_t *pCmd )
{
	switch ( pCmd->m_eCmd )
	{
	default:
		break;
	TMP_HTML_MSG_FUNC( eHTMLCommands_BrowserReady, CMsgBrowserReady, BrowserReady );
	TMP_HTML_MSG_FUNC( eHTMLCommands_NeedsPaint, CMsgNeedsPaint, BrowserNeedsPaint );
	TMP_HTML_MSG_FUNC( eHTMLCommands_StartRequest, CMsgStartRequest, BrowserStartRequest );
	TMP_HTML_MSG_FUNC( eHTMLCommands_URLChanged, CMsgURLChanged, BrowserURLChanged );
	TMP_HTML_MSG_FUNC( eHTMLCommands_FinishedRequest, CMsgFinishedRequest, BrowserFinishedRequest );
	TMP_HTML_MSG_FUNC( eHTMLCommands_ShowPopup, CMsgShowPopup, BrowserShowPopup );
	TMP_HTML_MSG_FUNC( eHTMLCommands_HidePopup, CMsgHidePopup, BrowserHidePopup );
	TMP_HTML_MSG_FUNC( eHTMLCommands_OpenNewTab, CMsgOpenNewTab, BrowserOpenNewTab );
	TMP_HTML_MSG_FUNC( eHTMLCommands_PopupHTMLWindow, CMsgPopupHTMLWindow, BrowserPopupHTMLWindow );
	TMP_HTML_MSG_FUNC( eHTMLCommands_SetHTMLTitle, CMsgSetHTMLTitle, BrowserSetHTMLTitle );
	TMP_HTML_MSG_FUNC( eHTMLCommands_LoadingResource, CMsgLoadingResource, BrowserLoadingResource );
	TMP_HTML_MSG_FUNC( eHTMLCommands_StatusText, CMsgStatusText, BrowserStatusText );
	TMP_HTML_MSG_FUNC( eHTMLCommands_SetCursor, CMsgSetCursor, BrowserSetCursor );
	TMP_HTML_MSG_FUNC( eHTMLCommands_FileLoadDialog, CMsgFileLoadDialog, BrowserFileLoadDialog );
	TMP_HTML_MSG_FUNC( eHTMLCommands_ShowToolTip, CMsgShowToolTip, BrowserShowToolTip );
	TMP_HTML_MSG_FUNC( eHTMLCommands_UpdateToolTip, CMsgUpdateToolTip, BrowserUpdateToolTip );
	TMP_HTML_MSG_FUNC( eHTMLCommands_HideToolTip, CMsgHideToolTip, BrowserHideToolTip );
	TMP_HTML_MSG_FUNC( eHTMLCommands_SearchResults, CMsgSearchResults, BrowserSearchResults );
	TMP_HTML_MSG_FUNC( eHTMLCommands_Close, CMsgClose, BrowserClose );
	TMP_HTML_MSG_FUNC( eHTMLCommands_GetZoomResponse, CMsgGetZoomResponse, BrowserGetZoomResponse );
	TMP_HTML_MSG_FUNC( eHTMLCommands_HorizontalScrollBarSizeResponse, CMsgHorizontalScrollBarSizeResponse, BrowserHorizontalScrollBarSizeResponse );
	TMP_HTML_MSG_FUNC( eHTMLCommands_VerticalScrollBarSizeResponse, CMsgVerticalScrollBarSizeResponse, BrowserVerticalScrollBarSizeResponse );
	TMP_HTML_MSG_FUNC( eHTMLCommands_LinkAtPositionResponse, CMsgLinkAtPositionResponse, BrowserLinkAtPositionResponse );
	TMP_HTML_MSG_FUNC( eHTMLCommands_ZoomToElementAtPositionResponse, CMsgZoomToElementAtPositionResponse, BrowserZoomToElementAtPositionResponse );
	TMP_HTML_MSG_FUNC( eHTMLCommands_JSAlert, CMsgJSAlert, BrowserJSAlert );
	TMP_HTML_MSG_FUNC( eHTMLCommands_JSConfirm, CMsgJSConfirm, BrowserJSConfirm );
	TMP_HTML_MSG_FUNC( eHTMLCommands_OpenSteamURL, CMsgOpenSteamURL, BrowserOpenSteamURL );
	TMP_HTML_MSG_FUNC( eHTMLCommands_CanGoBackandForward, CMsgCanGoBackAndForward, BrowserCanGoBackandForward );
	TMP_HTML_MSG_FUNC( eHTMLCommands_SizePopup, CMsgSizePopup, BrowserSizePopup );
	}
}

//-----------------------------------------------------------------------------
// Purpose: browser has been constructed on the cef thread, lets use it
//-----------------------------------------------------------------------------
void HTML::BrowserReady( const CMsgBrowserReady *pCmd )
{
	const char *pchTitle = g_pVGuiLocalize->FindAsUTF8( "#cef_error_title" );
	const char *pchHeader = g_pVGuiLocalize->FindAsUTF8( "#cef_error_header" );
	const char *pchDetailCacheMiss = g_pVGuiLocalize->FindAsUTF8( "#cef_cachemiss" );
	const char *pchDetailBadUURL = g_pVGuiLocalize->FindAsUTF8( "#cef_badurl" );
	const char *pchDetailConnectionProblem = g_pVGuiLocalize->FindAsUTF8( "#cef_connectionproblem" );
	const char *pchDetailProxyProblem = g_pVGuiLocalize->FindAsUTF8( "#cef_proxyconnectionproblem" );
	const char *pchDetailUnknown = g_pVGuiLocalize->FindAsUTF8( "#cef_unknown" );

	// tell it utf8 loc strings to use
	CHTMLProtoBufMsg<CMsgBrowserErrorStrings> cmd( eHTMLCommands_BrowserErrorStrings );
	cmd.Body().set_title( pchTitle );
	cmd.Body().set_header( pchHeader );
	cmd.Body().set_cache_miss( pchDetailCacheMiss );
	cmd.Body().set_bad_url( pchDetailBadUURL );
	cmd.Body().set_connection_problem( pchDetailConnectionProblem );
	cmd.Body().set_proxy_problem( pchDetailProxyProblem );
	cmd.Body().set_unknown( pchDetailUnknown );
	DISPATCH_MESSAGE( eHTMLCommands_BrowserErrorStrings );

	if ( !m_sPendingURLLoad.IsEmpty() )
	{
		PostURL( m_sPendingURLLoad, m_sPendingPostData, false );
		m_sPendingURLLoad.Clear();
	}
}


//-----------------------------------------------------------------------------
// Purpose: we have a new texture to update
//-----------------------------------------------------------------------------
void HTML::BrowserNeedsPaint( const CMsgNeedsPaint *pCmd )
{
	int tw = 0, tt = 0;
	if ( m_iHTMLTextureID != 0 )
	{
		tw = m_allocedTextureWidth;
		tt = m_allocedTextureHeight;
	}

	if ( m_iHTMLTextureID != 0 && ( ( _vbar->IsVisible() && pCmd->scrolly() > 0 && abs( (int)pCmd->scrolly() - m_scrollVertical.m_nScroll) > 5 ) || ( _hbar->IsVisible() && pCmd->scrollx() > 0 && abs( (int)pCmd->scrollx() - m_scrollHorizontal.m_nScroll ) > 5 ) ) )
	{
		// this isn't an update from a scroll position we expect, ignore it and ask for a refresh of our update pos2
		CHTMLProtoBufMsg<CMsgNeedsPaintResponse> cmd( eHTMLCommands_NeedsPaintResponse );
		cmd.Body().set_textureid( pCmd->textureid() );
		DISPATCH_MESSAGE( eHTMLCommands_NeedsPaintResponse );
		m_bNeedsFullTextureUpload = true;
		return;
	}

	// update the vgui texture
	if ( m_bNeedsFullTextureUpload || m_iHTMLTextureID == 0  || tw != (int)pCmd->wide() || tt != (int)pCmd->tall() )
	{
		m_bNeedsFullTextureUpload = false;
		if ( m_iHTMLTextureID != 0 )
			surface()->DeleteTextureByID( m_iHTMLTextureID );

		// if the dimensions changed we also need to re-create the texture ID to support the overlay properly (it won't resize a texture on the fly, this is the only control that needs
		//   to so lets have a tiny bit more code here to support that)
		m_iHTMLTextureID = surface()->CreateNewTextureID( true );
		surface()->DrawSetTextureRGBAEx( m_iHTMLTextureID, (const unsigned char *)pCmd->rgba(), pCmd->wide(), pCmd->tall(), IMAGE_FORMAT_BGRA8888 );// BR FIXME - this call seems to shift by some number of pixels?
		m_allocedTextureWidth = pCmd->wide();
		m_allocedTextureHeight = pCmd->tall();
	}
	else if ( (int)pCmd->updatewide() > 0 && (int)pCmd->updatetall() > 0 )
	{
		// same size texture, just bits changing in it, lets twiddle
		surface()->DrawUpdateRegionTextureRGBA( m_iHTMLTextureID, pCmd->updatex(), pCmd->updatey(), (const unsigned char *)pCmd->rgba(), pCmd->updatewide(), pCmd->updatetall(), IMAGE_FORMAT_BGRA8888 );
	}
	else
	{
		surface()->DrawSetTextureRGBAEx( m_iHTMLTextureID, (const unsigned char *)pCmd->rgba(), pCmd->wide(), pCmd->tall(), IMAGE_FORMAT_BGRA8888 );
	}

	if ( m_pComboBoxHost->IsVisible() )
	{
		// update the combo box texture also
		if ( m_iComboBoxTextureID != 0 )
		{
			tw = m_allocedComboBoxWidth;
			tt = m_allocedComboBoxHeight;
		}

		if ( m_iComboBoxTextureID == 0  || tw != (int)pCmd->combobox_wide() || tt != (int)pCmd->combobox_tall() )
		{
			if ( m_iComboBoxTextureID != 0 )
				surface()->DeleteTextureByID( m_iComboBoxTextureID );

			// if the dimensions changed we also need to re-create the texture ID to support the overlay properly (it won't resize a texture on the fly, this is the only control that needs
			//   to so lets have a tiny bit more code here to support that)
			m_iComboBoxTextureID = surface()->CreateNewTextureID( true );
			surface()->DrawSetTextureRGBAEx( m_iComboBoxTextureID, (const unsigned char *)pCmd->combobox_rgba(), pCmd->combobox_wide(), pCmd->combobox_tall(), IMAGE_FORMAT_BGRA8888 );
			m_allocedComboBoxWidth = (int)pCmd->combobox_wide();
			m_allocedComboBoxHeight = (int)pCmd->combobox_tall();
		}
		else
		{
			// same size texture, just bits changing in it, lets twiddle
			surface()->DrawUpdateRegionTextureRGBA( m_iComboBoxTextureID, 0, 0, (const unsigned char *)pCmd->combobox_rgba(), pCmd->combobox_wide(), pCmd->combobox_tall(), IMAGE_FORMAT_BGRA8888 );
		}
	}

	// need a paint next time
	Repaint();

	CHTMLProtoBufMsg<CMsgNeedsPaintResponse> cmd( eHTMLCommands_NeedsPaintResponse );
	cmd.Body().set_textureid( pCmd->textureid() );
	DISPATCH_MESSAGE( eHTMLCommands_NeedsPaintResponse );
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
void HTML::BrowserStartRequest( const CMsgStartRequest *pCmd )
{
	bool bRes = OnStartRequest( pCmd->url().c_str(), pCmd->target().c_str(), pCmd->postdata().c_str(), pCmd->bisredirect() );

	CHTMLProtoBufMsg<CMsgStartRequestResponse> cmd( eHTMLCommands_StartRequestResponse );
	cmd.Body().set_ballow( bRes );
	DISPATCH_MESSAGE( eHTMLCommands_StartRequestResponse );

	UpdateCachedHTMLValues();
}


//-----------------------------------------------------------------------------
// Purpose: browser went to a new url
//-----------------------------------------------------------------------------
void HTML::BrowserURLChanged( const CMsgURLChanged *pCmd )
{
	m_sCurrentURL = pCmd->url().c_str();

	KeyValues *pMessage = new KeyValues( "OnURLChanged" );
	pMessage->SetString( "url", pCmd->url().c_str() );
	pMessage->SetString( "postdata", pCmd->postdata().c_str() );
	pMessage->SetInt( "isredirect", pCmd->bisredirect() ? 1 : 0 );

	PostActionSignal( pMessage );

	OnURLChanged( m_sCurrentURL, pCmd->postdata().c_str(), pCmd->bisredirect() );
}


//-----------------------------------------------------------------------------
// Purpose: finished loading this page
//-----------------------------------------------------------------------------
void HTML::BrowserFinishedRequest( const CMsgFinishedRequest *pCmd )
{
	PostActionSignal( new KeyValues( "OnFinishRequest", "url", pCmd->url().c_str() ) );
	if (  pCmd->pagetitle().length()  )
		PostActionSignal( new KeyValues( "PageTitleChange", "title", pCmd->pagetitle().c_str() ) );
	KeyValues *pKVSecure = new KeyValues( "SecurityStatus" );
	pKVSecure->SetString( "url", pCmd->url().c_str() );
	pKVSecure->SetInt( "secure", pCmd->security_info().bissecure() );
	pKVSecure->SetInt( "certerror", pCmd->security_info().bhascerterror() );
	pKVSecure->SetInt( "isevcert", pCmd->security_info().bisevcert() );
	pKVSecure->SetString( "certname", pCmd->security_info().certname().c_str() );
	PostActionSignal( pKVSecure );

	CUtlMap < CUtlString, CUtlString > mapHeaders;
	SetDefLessFunc( mapHeaders );
	for ( int i = 0; i < pCmd->headers_size(); i++ )
	{
		const CHTMLHeader &header = pCmd->headers(i);
		mapHeaders.Insert( header.key().c_str(), header.value().c_str() );
	}

	OnFinishRequest( pCmd->url().c_str(), pCmd->pagetitle().c_str(), mapHeaders );

	UpdateCachedHTMLValues();
}


//-----------------------------------------------------------------------------
// Purpose: show a popup dialog
//-----------------------------------------------------------------------------
void HTML::BrowserShowPopup( const CMsgShowPopup *pCmd )
{
	m_pComboBoxHost->SetVisible( true );
}


//-----------------------------------------------------------------------------
// Purpose: hide the popup
//-----------------------------------------------------------------------------
void HTML::HidePopup()
{ 
	m_pComboBoxHost->SetVisible( false ); 
}


//-----------------------------------------------------------------------------
// Purpose: browser wants us to hide a popup
//-----------------------------------------------------------------------------
void HTML::BrowserHidePopup( const CMsgHidePopup *pCmd )
{
	HidePopup();
}


//-----------------------------------------------------------------------------
// Purpose: browser wants us to position a popup
//-----------------------------------------------------------------------------
void HTML::BrowserSizePopup( const CMsgSizePopup *pCmd )
{
	int nAbsX, nAbsY;
	ipanel()->GetAbsPos( GetVPanel(), nAbsX, nAbsY );
	m_pComboBoxHost->SetBounds( pCmd->x() + 1 + nAbsX, pCmd->y() + nAbsY, pCmd->wide(), pCmd->tall() );
}


//-----------------------------------------------------------------------------
// Purpose: browser wants to open a new tab
//-----------------------------------------------------------------------------
void HTML::BrowserOpenNewTab( const CMsgOpenNewTab *pCmd )
{
	(pCmd);
	// Not suppored by default, if a child class overrides us and knows how to handle tabs, then it can do this.
}


//-----------------------------------------------------------------------------
// Purpose: display a new html window 
//-----------------------------------------------------------------------------
void HTML::BrowserPopupHTMLWindow( const CMsgPopupHTMLWindow *pCmd )
{
	HTMLPopup *p = new HTMLPopup( this, pCmd->url().c_str(), "" );
	int wide = pCmd->wide();
	int tall = pCmd->tall();
	if ( wide == 0 || tall == 0 )
	{
		wide = MAX( 640, GetWide() );
		tall = MAX( 480, GetTall() );
	}

	p->SetBounds( pCmd->x(), pCmd->y(), wide, tall  );
	p->SetDeleteSelfOnClose( true );
	if ( pCmd->x() == 0 || pCmd->y() == 0 )
		p->MoveToCenterOfScreen();
	p->Activate();

}


//-----------------------------------------------------------------------------
// Purpose: browser telling us the page title
//-----------------------------------------------------------------------------
void HTML::BrowserSetHTMLTitle( const CMsgSetHTMLTitle *pCmd )
{
	PostMessage( GetParent(), new KeyValues( "OnSetHTMLTitle", "title", pCmd->title().c_str() ) );
	OnSetHTMLTitle( pCmd->title().c_str() );
}


//-----------------------------------------------------------------------------
// Purpose: still loading stuff for this page
//-----------------------------------------------------------------------------
void HTML::BrowserLoadingResource( const CMsgLoadingResource *pCmd )
{
	UpdateCachedHTMLValues();
}


//-----------------------------------------------------------------------------
// Purpose: status bar details
//-----------------------------------------------------------------------------
void HTML::BrowserStatusText( const CMsgStatusText *pCmd )
{
	PostActionSignal( new KeyValues( "OnSetStatusText", "status", pCmd->text().c_str() ) );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us to use this cursor
//-----------------------------------------------------------------------------
void HTML::BrowserSetCursor( const CMsgSetCursor *pCmd )
{
	// Mouse cursor value in CMsgSetCursor is set to one of EMouseCursor,
	// by CChromePainter::OnSetCursor in html_chrome.cpp
	// Code below relies on start of EMouseCursor being exactly same as vgui::CursorCode  
	
	vgui::CursorCode cursor;
	uint32 msgCursor = pCmd->cursor();

	if ( msgCursor >= (uint32)(dc_last) )
	{
		cursor = dc_arrow;
	}
	else
	{
		cursor = (CursorCode)msgCursor;
	}
	
	SetCursor( cursor );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling to show the file loading dialog
//-----------------------------------------------------------------------------
void HTML::BrowserFileLoadDialog( const CMsgFileLoadDialog *pCmd )
{
	/*
	// try and use the OS-specific file dialog first
	char rgchFileName[MAX_UNICODE_PATH_IN_UTF8];
	if ( surface()->OpenOSFileOpenDialog( pCmd->title().c_str(), pCmd->initialfile().c_str(), NULL, rgchFileName, sizeof(rgchFileName) ) )
	{
		CHTMLProtoBufMsg<CMsgFileLoadDialogResponse> cmd( eHTMLCommands_FileLoadDialogResponse );
		cmd.Body().add_files( rgchFileName );
		DISPATCH_MESSAGE( eHTMLCommands_FileLoadDialogResponse );
	}
	else*/
	{
		// couldn't access an OS-specific dialog, use the internal one
		if ( m_hFileOpenDialog.Get() )
		{
			delete m_hFileOpenDialog.Get();
			m_hFileOpenDialog = NULL;
		}
		m_hFileOpenDialog = new FileOpenDialog( this, pCmd->title().c_str(), true );
		m_hFileOpenDialog->SetStartDirectory( pCmd->initialfile().c_str() );
		m_hFileOpenDialog->AddActionSignalTarget( this );
		m_hFileOpenDialog->SetAutoDelete( true );
		m_hFileOpenDialog->DoModal(false);
	}
}


//-----------------------------------------------------------------------------
// Purpose: browser asking to show a tooltip
//-----------------------------------------------------------------------------
void HTML::BrowserShowToolTip( const CMsgShowToolTip *pCmd )
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
void HTML::BrowserUpdateToolTip( const CMsgUpdateToolTip *pCmd )
{
//	GetTooltip()->SetText( pCmd->text().c_str() );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling that it is done with the tip
//-----------------------------------------------------------------------------
void HTML::BrowserHideToolTip( const CMsgHideToolTip *pCmd )
{
//	GetTooltip()->HideTooltip();
//	DeleteToolTip();
}


//-----------------------------------------------------------------------------
// Purpose: callback when performing a search
//-----------------------------------------------------------------------------
void HTML::BrowserSearchResults( const CMsgSearchResults *pCmd )
{
	if ( pCmd->results() == 0 )
		m_pFindBar->HideCountLabel();
	else
		m_pFindBar->ShowCountLabel();

	if ( pCmd->results() > 0 )
		m_pFindBar->SetDialogVariable( "findcount", (int)pCmd->results() );
	if ( pCmd->activematch() > 0 )
		m_pFindBar->SetDialogVariable( "findactive", (int)pCmd->activematch() );
	m_pFindBar->InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us it had a close requested
//-----------------------------------------------------------------------------
void HTML::BrowserClose( const CMsgClose *pCmd )
{
	PostActionSignal( new KeyValues( "OnCloseWindow" ) );
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us the size of the horizontal scrollbars
//-----------------------------------------------------------------------------
void HTML::BrowserHorizontalScrollBarSizeResponse( const CMsgHorizontalScrollBarSizeResponse *pCmd )
{
	ScrollData_t scrollHorizontal;
	scrollHorizontal.m_nX = pCmd->x();
	scrollHorizontal.m_nY = pCmd->y();
	scrollHorizontal.m_nWide = pCmd->wide();
	scrollHorizontal.m_nTall = pCmd->tall();
	scrollHorizontal.m_nScroll = pCmd->scroll();
	scrollHorizontal.m_nMax = pCmd->scroll_max();
	scrollHorizontal.m_bVisible = ( m_scrollHorizontal.m_nTall > 0 ); 
	scrollHorizontal.m_flZoom = pCmd->zoom();

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
void HTML::BrowserVerticalScrollBarSizeResponse( const CMsgVerticalScrollBarSizeResponse *pCmd )
{
	ScrollData_t scrollVertical;
	scrollVertical.m_nX = pCmd->x();
	scrollVertical.m_nY = pCmd->y();
	scrollVertical.m_nWide = pCmd->wide();
	scrollVertical.m_nTall = pCmd->tall();
	scrollVertical.m_nScroll = pCmd->scroll();
	scrollVertical.m_nMax = pCmd->scroll_max();
	scrollVertical.m_bVisible = ( m_scrollVertical.m_nTall > 0 ); 
	scrollVertical.m_flZoom = pCmd->zoom();

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
// Purpose: browser telling us the current page zoom
//-----------------------------------------------------------------------------
void HTML::BrowserGetZoomResponse( const CMsgGetZoomResponse *pCmd )
{
	m_flZoom = pCmd->zoom();
	if ( m_flZoom == 0.0f )
		m_flZoom = 100.0f;
	m_flZoom /= 100; // scale zoom to 1.0 being 100%, webkit gives us 100 for normal scale

}


//-----------------------------------------------------------------------------
// Purpose: browser telling us what is at this location on the page
//-----------------------------------------------------------------------------
void HTML::BrowserLinkAtPositionResponse( const CMsgLinkAtPositionResponse *pCmd )
{
	m_LinkAtPos.m_sURL = pCmd->url().c_str();
	m_LinkAtPos.m_nX = pCmd->x();
	m_LinkAtPos.m_nY = pCmd->y();

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
// Purpose: browser telling us that a zoom to element is done
//-----------------------------------------------------------------------------
void HTML::BrowserZoomToElementAtPositionResponse( const CMsgZoomToElementAtPositionResponse *pCmd )
{

}


//-----------------------------------------------------------------------------
// Purpose: browser telling us to pop a javascript alert dialog
//-----------------------------------------------------------------------------
void HTML::BrowserJSAlert( const CMsgJSAlert *pCmd )
{
	MessageBox *pDlg = new MessageBox( m_sCurrentURL, (const char *)pCmd->message().c_str(), this );
	pDlg->AddActionSignalTarget( this );
	pDlg->SetCommand( new KeyValues( "DismissJSDialog", "result", false ) );
	pDlg->DoModal();
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us to pop a js confirm dialog
//-----------------------------------------------------------------------------
void HTML::BrowserJSConfirm( const CMsgJSConfirm *pCmd )
{
	QueryBox *pDlg = new QueryBox( m_sCurrentURL, (const char *)pCmd->message().c_str(), this );
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
	CHTMLProtoBufMsg<CMsgJSDialogResponse> cmd( eHTMLCommands_JSDialogResponse );
	cmd.Body().set_result( bResult==1 );
	DISPATCH_MESSAGE( eHTMLCommands_JSDialogResponse );
};


//-----------------------------------------------------------------------------
// Purpose: browser telling us the state of back and forward buttons
//-----------------------------------------------------------------------------
void HTML::BrowserCanGoBackandForward( const CMsgCanGoBackAndForward *pCmd )
{
	m_bCanGoBack = pCmd->bgoback();
	m_bCanGoForward = pCmd->bgoforward();
}


//-----------------------------------------------------------------------------
// Purpose: browser telling us a steam url was asked to be loaded
//-----------------------------------------------------------------------------
void HTML::BrowserOpenSteamURL( const CMsgOpenSteamURL *pCmd )
{
	vgui::ivgui()->PostMessage( surface()->GetEmbeddedPanel(), 
		new KeyValues( "OpenSteamDialog", "cmd", pCmd->url().c_str() ), NULL, 0.3f );
}


//-----------------------------------------------------------------------------
// Purpose: update the value of the cached variables we keep
//-----------------------------------------------------------------------------
void HTML::UpdateCachedHTMLValues()
{
	// request scroll bar sizes
	{
		CHTMLProtoBufMsg<CMsgVerticalScrollBarSize> cmd( eHTMLCommands_VerticalScrollBarSize );
		DISPATCH_MESSAGE( eHTMLCommands_VerticalScrollBarSize );
	}
	{
		CHTMLProtoBufMsg<CMsgHorizontalScrollBarSize> cmd( eHTMLCommands_HorizontalScrollBarSize );
		DISPATCH_MESSAGE( eHTMLCommands_HorizontalScrollBarSize );
	}
	{
		CHTMLProtoBufMsg<CMsgGetZoom> cmd( eHTMLCommands_GetZoom );
		DISPATCH_MESSAGE( eHTMLCommands_GetZoom );
	}
}


//-----------------------------------------------------------------------------
// Purpose: ask the browser for what is at this x,y
//-----------------------------------------------------------------------------
void HTML::GetLinkAtPosition( int x, int y )
{
	CHTMLProtoBufMsg<CMsgLinkAtPosition> cmd( eHTMLCommands_LinkAtPosition );
	cmd.Body().set_x( x );
	cmd.Body().set_y( y );
	DISPATCH_MESSAGE( eHTMLCommands_LinkAtPosition );
}

//-----------------------------------------------------------------------------
// Purpose: send any queued html messages we have
//-----------------------------------------------------------------------------
void HTML::SendPendingHTMLMessages()
{
	FOR_EACH_VEC( m_vecPendingMessages, i )
	{
		m_vecPendingMessages[i]->m_iBrowser = m_iBrowser;
		surface()->AccessChromeHTMLController()->PushCommand( m_vecPendingMessages[i] );
		surface()->AccessChromeHTMLController()->WakeThread();
	}
	m_vecPendingMessages.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: update the size of the browser itself and scrollbars it shows
//-----------------------------------------------------------------------------
void HTML::UpdateSizeAndScrollBars()
{
	// Tell IE
	BrowserResize();

	// Do this after we tell IE!
	int w,h;
	GetSize( w, h );
	CalcScrollBars(w,h);

	InvalidateLayout();
}


