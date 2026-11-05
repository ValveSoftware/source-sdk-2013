//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// menu.cpp
//
// generic menu handler
//
#include "cbase.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "weapon_selection.h"

#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>

#if defined( HL2MP )
#include "ienginevgui.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/TextImage.h>
#endif

#define MAX_MENU_STRING	512
wchar_t g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];

#include "menu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
//-----------------------------------------------------
//

DECLARE_HUDELEMENT( CHudMenu );
DECLARE_HUD_MESSAGE( CHudMenu, ShowMenu );

#if defined( HL2MP )
ConVar cl_radio_menus( "cl_radio_menus", "1", FCVAR_ARCHIVE, "Display server menus on the HUD (1) or in a dialog when ESC is pressed (0).", true, 0, true, 1 );
ConVar cl_menu_dialog_x( "cl_menu_dialog_x", "-1", FCVAR_ARCHIVE, "Saved ESC menu horizontal position as a fraction of screen width (-1 centers it).", true, -1, true, 1 );
ConVar cl_menu_dialog_y( "cl_menu_dialog_y", "-1", FCVAR_ARCHIVE, "Saved ESC menu vertical position as a fraction of screen height (-1 centers it).", true, -1, true, 1 );

class CHudMenuDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CHudMenuDialog, vgui::Frame );
public:
	CHudMenuDialog( CHudMenu *pMenu ) : BaseClass( NULL, "HudMenuDialog" ),
		m_nMenuSerial( ~0u ),
		m_nLastX( 0 ), m_nLastY( 0 ), m_bPositionInitialized( false )
	{
		m_hMenu = pMenu;
		SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
		SetAutoDelete( false );
		SetProportional( false );
		SetTitle( "Menu", true );
		SetSizeable( false );
		SetClipToParent( true );
		SetFadeEffectDisableOverride( true );
		SetMinimizeButtonVisible( false );
		SetMaximizeButtonVisible( false );
		SetMenuButtonVisible( false );
		SetCloseButtonVisible( false );
		m_pItems = new vgui::PanelListPanel( this, "MenuItems" );
		m_pItems->SetFirstColumnWidth( 0 );
		KeyValues *pSettings = new KeyValues( "MenuItems" );
		pSettings->SetInt( "autohide_scrollbar", 1 );
		m_pItems->ApplySettings( pSettings );
		pSettings->deleteThis();
		MakePopup();
		SetVisible( false );
	}

	virtual ~CHudMenuDialog()
	{
		SavePosition();
	}

	void UpdateMenu()
	{
		CHudMenu *pMenu = m_hMenu.Get();
		if ( !pMenu || m_nMenuSerial == pMenu->m_nMenuSerial )
			return;

		m_nMenuSerial = pMenu->m_nMenuSerial;
		m_pItems->DeleteAllItems();
		SetTitle( "Menu", true );
		for ( int i = 0; i < pMenu->m_Processed.Count(); ++i )
		{
			const CHudMenu::ProcessedLine &line = pMenu->m_Processed[i];
			wchar_t text[MAX_MENU_STRING];
			V_wcsncpy( text, &g_szMenuString[line.startchar], ( line.length + 1 ) * sizeof( wchar_t ) );
			int slot = line.menuitem;
			if ( !slot && line.length >= 2 && text[0] >= L'0' && text[0] <= L'9' && text[1] == L'.' )
				slot = text[0] == L'0' ? 10 : text[0] - L'0';

			if ( i == 0 && !slot && line.length <= 64 )
			{
				if ( text[line.length - 1] == L':' )
					text[line.length - 1] = 0;
				SetTitle( text, true );
				continue;
			}

			vgui::Label *pItem;
			if ( slot >= 1 && slot <= 10 )
			{
				vgui::Button *pButton = new vgui::Button( m_pItems, "MenuItem", text );
				pButton->SetCommand( new KeyValues( "MenuItemSelected", "slot", slot, "serial", (int)m_nMenuSerial ) );
				pButton->AddActionSignalTarget( this );
				pButton->SetEnabled( ( pMenu->m_bitsValidSlots & ( 1 << ( slot - 1 ) ) ) != 0 );
				pButton->SetTabPosition( i + 1 );
				pItem = pButton;
			}
			else
			{
				pItem = new vgui::Label( m_pItems, "MenuText", text );
			}
			pItem->SetContentAlignment( vgui::Label::a_west );
			pItem->SetWrap( true );
			m_pItems->AddItem( NULL, pItem );
		}
		m_pItems->MoveScrollBarToTop();
		InvalidateLayout( true );
	}

	void SavePosition()
	{
		if ( !m_bPositionInitialized )
			return;
		int x, y;
		GetPos( x, y );
		if ( x == m_nLastX && y == m_nLastY )
			return;

		int screenWide, screenTall;
		vgui::surface()->GetScreenSize( screenWide, screenTall );
		if ( screenWide > 0 && screenTall > 0 )
		{
			m_nLastX = x;
			m_nLastY = y;
			cl_menu_dialog_x.SetValue( (float)x / screenWide );
			cl_menu_dialog_y.SetValue( (float)y / screenTall );
		}
	}

private:
	MESSAGE_FUNC_INT_INT( OnMenuItemSelected, "MenuItemSelected", slot, serial )
	{
		CHudMenu *pMenu = m_hMenu.Get();
		if ( pMenu && IsVisible() && (unsigned int)serial == pMenu->m_nMenuSerial &&
			!pMenu->UseRadioMenus() && engine->IsInGame() && enginevgui->IsGameUIVisible() )
		{
			pMenu->SelectMenuItem( slot );
			if ( !pMenu->IsMenuOpen() )
				SetVisible( false );
		}
	}

	virtual void PerformLayout()
	{
		SavePosition();
		int screenWide, screenTall;
		vgui::surface()->GetScreenSize( screenWide, screenTall );
		if ( screenWide <= 0 || screenTall <= 0 )
			return;

		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		vgui::HFont font = pScheme->GetFont( "Default", IsProportional() );
		int fontTall = vgui::surface()->GetFontTall( font );
		int padding = 8;
		int gap = 4;
		SetSize( MIN( screenWide - 2 * padding, 360 ), MIN( screenTall - 2 * padding, 480 ) );
		int x, y, wide, tall;
		GetClientArea( x, y, wide, tall );
		int frameTall = GetTall() - tall;

		m_pItems->InvalidateLayout( true, true );
		m_pItems->SetVerticalBufferPixels( gap );
		m_pItems->SetPaintBackgroundEnabled( false );
		m_pItems->SetPaintBorderEnabled( false );
		int itemWide = MAX( 1, wide - m_pItems->GetScrollbar()->GetWide() - gap - 12 );
		for ( int i = 0; i < m_pItems->GetItemCount(); ++i )
		{
			vgui::Label *pItem = static_cast<vgui::Label *>( m_pItems->GetItemPanel( m_pItems->GetItemIDFromRow( i ) ) );
			pItem->SetSize( itemWide, 1 );
			pItem->SetTextInset( padding, 0 );
			pItem->InvalidateLayout( true, true );
			pItem->SetFont( font );
			pItem->GetTextImage()->SetDrawWidth( MAX( 1, itemWide - 2 * padding ) );
			int textWide, textTall;
			pItem->GetTextImage()->GetContentSize( textWide, textTall );
			pItem->SetTall( MAX( textTall, fontTall ) + padding );
		}
		int listTall = MIN( m_pItems->ComputeVPixelsNeeded(), MAX( 1, tall ) );
		SetTall( frameTall + listTall );
		BaseClass::PerformLayout();
		m_pItems->SetBounds( x, y, wide, listTall );
		m_pItems->InvalidateLayout( true );
		RestorePosition();
	}

	void RestorePosition()
	{
		int screenWide, screenTall;
		vgui::surface()->GetScreenSize( screenWide, screenTall );
		if ( screenWide <= 0 || screenTall <= 0 )
			return;

		int x = cl_menu_dialog_x.GetFloat() < 0 ? ( screenWide - GetWide() ) / 2 : RoundFloatToInt( cl_menu_dialog_x.GetFloat() * screenWide );
		int y = cl_menu_dialog_y.GetFloat() < 0 ? ( screenTall - GetTall() ) / 2 : RoundFloatToInt( cl_menu_dialog_y.GetFloat() * screenTall );
		m_nLastX = clamp( x, 0, MAX( 0, screenWide - GetWide() ) );
		m_nLastY = clamp( y, 0, MAX( 0, screenTall - GetTall() ) );
		m_bPositionInitialized = true;
		SetPos( m_nLastX, m_nLastY );
		if ( cl_menu_dialog_x.GetFloat() < 0 )
			cl_menu_dialog_x.SetValue( (float)m_nLastX / screenWide );
		if ( cl_menu_dialog_y.GetFloat() < 0 )
			cl_menu_dialog_y.SetValue( (float)m_nLastY / screenTall );
	}

	virtual void OnScreenSizeChanged( int oldwide, int oldtall )
	{
		m_bPositionInitialized = false;
		BaseClass::OnScreenSizeChanged( oldwide, oldtall );
		InvalidateLayout( true );
	}

	vgui::DHANDLE< CHudMenu > m_hMenu;
	vgui::PanelListPanel *m_pItems;
	unsigned int m_nMenuSerial;
	int m_nLastX;
	int m_nLastY;
	bool m_bPositionInitialized;
};
#endif

//
//-----------------------------------------------------
//

static char* ConvertCRtoNL( char *str )
{
	for ( char *ch = str; *ch != 0; ch++ )
		if ( *ch == '\r' )
			*ch = '\n';
	return str;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenu::CHudMenu( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass(NULL, "HudMenu")
#if defined( HL2MP )
	, m_nMenuSerial( 0 )
#endif
{
	Reset();

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	
	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

CHudMenu::~CHudMenu()
{
#if defined( HL2MP )
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	if ( m_hMenuDialog.Get() )
		m_hMenuDialog->MarkForDeletion();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::Init( void )
{
	HOOK_HUD_MESSAGE( CHudMenu, ShowMenu );

#if defined( HL2MP )
	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
#endif
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::Reset( void )
{
#if defined( HL2MP )
	++m_nMenuSerial;
	m_bNetworkMenu = false;
	if ( m_hMenuDialog.Get() )
		m_hMenuDialog->SetVisible( false );
#endif
	m_bMenuTakesInput = false;
	m_bMenuDisplayed = false;
	m_bitsValidSlots = 0;
	m_flShutoffTime = -1;
	m_nSelectedItem = -1;
	m_Processed.RemoveAll();
	m_nMaxPixels = 0;
	m_nHeight = 0;
	g_szMenuString[0] = 0;
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudMenu::IsMenuOpen( void )
{
	return m_bMenuDisplayed && m_bMenuTakesInput &&
		( m_flShutoffTime < 0 || m_flShutoffTime > gpGlobals->realtime );
}

bool CHudMenu::UseRadioMenus( void )
{
#if defined( HL2MP )
	return !m_bNetworkMenu || cl_radio_menus.GetBool();
#else
	return true;
#endif
}

bool CHudMenu::IsTakingInput( void )
{
	return UseRadioMenus() && IsMenuOpen();
}

#if defined( HL2MP )
void CHudMenu::OnTick()
{
	if ( m_hMenuDialog.Get() )
		m_hMenuDialog->SavePosition();

	bool visible = !UseRadioMenus() && IsMenuOpen() && engine->IsInGame() && enginevgui->IsGameUIVisible();
	if ( !visible )
	{
		if ( m_hMenuDialog.Get() )
			m_hMenuDialog->SetVisible( false );
		return;
	}

	if ( !m_hMenuDialog.Get() )
		m_hMenuDialog = new CHudMenuDialog( this );
	m_hMenuDialog->UpdateMenu();
	if ( !m_hMenuDialog->IsVisible() )
		m_hMenuDialog->Activate();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::VidInit( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::OnThink()
{
	if ( m_bMenuDisplayed && m_flShutoffTime >= 0 && m_flShutoffTime <= gpGlobals->realtime )
	{
		m_bMenuDisplayed = false;
		m_bMenuTakesInput = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenu::ShouldDraw( void )
{
#if defined( HL2MP )
	if ( !UseRadioMenus() )
		return CHudElement::ShouldDraw() && IsMenuOpen() && engine->IsInGame() && !enginevgui->IsGameUIVisible();
#endif

	bool draw = CHudElement::ShouldDraw() && m_bMenuDisplayed;
	if ( !draw )
		return false;

	// check for if menu is set to disappear
	if ( m_flShutoffTime >= 0 && m_flShutoffTime <= gpGlobals->realtime )
	{  
		// times up, shutoff
		m_bMenuDisplayed = false;
		m_bMenuTakesInput = false;
		return false;
	}

	return draw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
//			textlen - 
//			font - 
//			x - 
//			y - 
//-----------------------------------------------------------------------------
void CHudMenu::PaintString( const wchar_t *text, int textlen, vgui::HFont& font, int x, int y )
{
	vgui::surface()->DrawSetTextFont( font );
	vgui::surface()->DrawSetTextPos( x, y );

	for ( int ch = 0; ch < textlen; ch++ )
	{
		vgui::surface()->DrawUnicodeChar( text[ch] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::Paint()
{
#if defined( HL2MP )
	if ( !UseRadioMenus() )
	{
		if ( !ShouldDraw() )
			return;

		const wchar_t *text = L"Press ESC to open menu";
		int wide, tall;
		vgui::surface()->GetTextSize( m_hTextFont, text, wide, tall );
		int inset = YRES( 8 );
		int padding = YRES( 4 );
		DrawBox( inset, inset, wide + 2 * padding, tall + 2 * padding, m_BoxColor, 1.0f );
		vgui::surface()->DrawSetTextColor( m_MenuColor );
		PaintString( text, V_wcslen( text ), m_hTextFont, inset + padding, inset + padding );
		return;
	}
#endif

	if ( !m_bMenuDisplayed )
		return;

	// center it
	int x = 20;

	Color	menuColor = m_MenuColor;
	Color itemColor = m_ItemColor;

	int c = m_Processed.Count();

	int border = 20;

	int wide = m_nMaxPixels + border;
	int tall = m_nHeight + border;

	int y = ( ScreenHeight() - tall ) * 0.5f;

	DrawBox( x - border/2, y - border/2, wide, tall, m_BoxColor, m_flSelectionAlphaOverride / 255.0f );

	//DrawTexturedBox( x - border/2, y - border/2, wide, tall, m_BoxColor, m_flSelectionAlphaOverride / 255.0f );

	menuColor[3] = menuColor[3] * ( m_flSelectionAlphaOverride / 255.0f );
	itemColor[3] = itemColor[3] * ( m_flSelectionAlphaOverride / 255.0f );

	for ( int i = 0; i < c; i++ )
	{
		ProcessedLine *line = &m_Processed[ i ];
		Assert( line );

		Color clr = line->menuitem != 0 ? itemColor : menuColor;

		bool canblur = false;
		if ( line->menuitem != 0 &&
			m_nSelectedItem >= 0 && 
			( line->menuitem == m_nSelectedItem ) )
		{
			canblur = true;
		}
		
		vgui::surface()->DrawSetTextColor( clr );

		int drawLen = line->length;
		if ( line->menuitem != 0 )
		{
			drawLen *= m_flTextScan;
		}

		vgui::surface()->DrawSetTextFont( line->menuitem != 0 ? m_hItemFont : m_hTextFont );

		PaintString( &g_szMenuString[ line->startchar ], drawLen, 
			line->menuitem != 0 ? m_hItemFont : m_hTextFont, x, y );

		if ( canblur )
		{
			// draw the overbright blur
			for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
			{
				if (fl >= 1.0f)
				{
					PaintString( &g_szMenuString[ line->startchar ], drawLen, m_hItemFontPulsing, x, y );
				}
				else
				{
					// draw a percentage of the last one
					Color col = clr;
					col[3] *= fl;
					vgui::surface()->DrawSetTextColor(col);
					PaintString( &g_szMenuString[ line->startchar ], drawLen, m_hItemFontPulsing, x, y );
				}
			}
		}

		y += line->height;
	}
}

//-----------------------------------------------------------------------------
// Purpose: selects an item from the menu
//-----------------------------------------------------------------------------
void CHudMenu::SelectMenuItem( int menu_item )
{
	if ( menu_item == 0 )
		menu_item = 10;

	// if menu_item is in a valid slot,  send a menuselect command to the server
	if ( IsMenuOpen() && menu_item > 0 && menu_item <= 10 && ( m_bitsValidSlots & ( 1 << ( menu_item - 1 ) ) ) )
	{
		char szbuf[32];
		Q_snprintf( szbuf, sizeof( szbuf ), "menuselect %d\n", menu_item );
		engine->ClientCmd( szbuf );

		m_nSelectedItem = menu_item;
		// Pulse the selection
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuPulse");

		// remove the menu quickly
		m_bMenuTakesInput = false;
		m_flShutoffTime = gpGlobals->realtime + m_flOpenCloseTime;
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuClose");
	}
}

void CHudMenu::ProcessText( void )
{
#if defined( HL2MP )
	++m_nMenuSerial;
#endif
	m_Processed.RemoveAll();
	m_nMaxPixels = 0;
	m_nHeight = 0;

	int i = 0;
	int startpos = i;
	int menuitem = 0;
	while ( i < MAX_MENU_STRING  )
	{
		wchar_t ch = g_szMenuString[ i ];
		if ( ch == 0 )
			break;

		if ( i == startpos && 
			( ch == L'-' && g_szMenuString[ i + 1 ] == L'>' ) )
		{
			// Special handling for menu item specifiers
			swscanf( &g_szMenuString[ i + 2 ], L"%d", &menuitem );
			if ( menuitem == 0 )
				menuitem = 10;
			i += 2;
			startpos += 2;

			continue;
		}

		// Skip to end of line
		while ( i < MAX_MENU_STRING && g_szMenuString[i] != 0 && g_szMenuString[i] != L'\n' )
		{
			i++;
		}

		// Store off line
		if ( ( i - startpos ) >= 1 )
		{
			ProcessedLine line;
			line.menuitem = menuitem;
			line.startchar = startpos;
			line.length = i - startpos;
			line.pixels = 0;
			line.height = 0;

			m_Processed.AddToTail( line );
		}

		menuitem = 0;

		// Skip delimiter
		if ( g_szMenuString[i] == '\n' )
		{
			i++;
		}
		startpos = i;
	}

	// Add final block
	if ( i - startpos >= 1 )
	{
		ProcessedLine line;
		line.menuitem = menuitem;
		line.startchar = startpos;
		line.length = i - startpos;
		line.pixels = 0;
		line.height = 0;

		m_Processed.AddToTail( line );
	}

	// Now compute pixels needed
	int c = m_Processed.Count();
	for ( i = 0; i < c; i++ )
	{
		ProcessedLine *l = &m_Processed[ i ];
		Assert( l );

		int pixels = 0;
		vgui::HFont font = l->menuitem != 0 ? m_hItemFont : m_hTextFont;

		for ( int ch = 0; ch < l->length; ch++ )
		{
			pixels += vgui::surface()->GetCharacterWidth( font, g_szMenuString[ ch + l->startchar ] );
		}

		l->pixels = pixels;
		l->height = vgui::surface()->GetFontTall( font );
		if ( pixels > m_nMaxPixels )
		{
			m_nMaxPixels = pixels;
		}
		m_nHeight += l->height;
	}
}
//-----------------------------------------------------------------------------
// Purpose: Local method to hide a menu, mirroring code found in
//          MsgFunc_ShowMenu.
//-----------------------------------------------------------------------------
void CHudMenu::HideMenu( void )
{
	m_fWaitingForMore = false;
	m_bMenuTakesInput = false;
	m_flShutoffTime = gpGlobals->realtime + m_flOpenCloseTime;
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuClose");
}

//-----------------------------------------------------------------------------
// Purpose: Local method to bring up a menu, mirroring code found in
//          MsgFunc_ShowMenu.
//
//   takes two values:
//		menuName  : menu name string 
//		validSlots: a bitfield describing the valid keys
//-----------------------------------------------------------------------------
void CHudMenu::ShowMenu( const char * menuName, int validSlots )
{
#if defined( HL2MP )
	m_bNetworkMenu = false;
#endif
	m_flShutoffTime = gpGlobals->realtime + MENU_SELECTION_TIMEOUT;
	m_bitsValidSlots = validSlots;
	m_fWaitingForMore = 0;

	Q_strncpy( g_szPrelocalisedMenuString, menuName, sizeof( g_szPrelocalisedMenuString ) );

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuOpen");
	m_nSelectedItem = -1;

	// we have the whole string, so we can localise it now
	char szMenuString[MAX_MENU_STRING];
	Q_strncpy( szMenuString, ConvertCRtoNL( hudtextmessage->BufferedLocaliseTextString( g_szPrelocalisedMenuString ) ), sizeof( szMenuString ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( szMenuString, g_szMenuString, sizeof( g_szMenuString ) );
	
	ProcessText();

	m_bMenuDisplayed = true;
	m_bMenuTakesInput = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenu::ShowMenu_KeyValueItems( KeyValues *pKV )
{
#if defined( HL2MP )
	m_bNetworkMenu = false;
#endif
	m_flShutoffTime = gpGlobals->realtime + MENU_SELECTION_TIMEOUT;
	m_fWaitingForMore = 0;
	m_bitsValidSlots = 0;

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuOpen");
	m_nSelectedItem = -1;
	
	g_szMenuString[0] = '\0';
	wchar_t *pWritePosition = g_szMenuString;
	int		nRemaining = sizeof( g_szMenuString ) / sizeof( wchar_t );
	int		nCount;

	int i = 0;
	for ( KeyValues *item = pKV->GetFirstSubKey(); item != NULL; item = item->GetNextKey() )
	{
		// Set this slot valid
		m_bitsValidSlots |= (1<<i);

		const char *pszItem = item->GetName();
		const wchar_t *wLocalizedItem = g_pVGuiLocalize->Find( pszItem );

		nCount = _snwprintf( pWritePosition, nRemaining, L"%d. %ls\n", i+1, wLocalizedItem );
		nRemaining -= nCount;
		pWritePosition += nCount;

		i++;
	}

	// put a cancel on the end
	m_bitsValidSlots |= (1<<9);

	nCount = _snwprintf( pWritePosition, nRemaining, L"0. %ls\n", g_pVGuiLocalize->Find( "#Cancel" ) );
	nRemaining -= nCount;
	pWritePosition += nCount;

	ProcessText();

	m_bMenuDisplayed = true;
	m_bMenuTakesInput = true;
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for ShowMenu message
//   takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, false if it's the last string
//		string: menu string to display
//  if this message is never received, then scores will simply be the combined totals of the players.
//-----------------------------------------------------------------------------
void CHudMenu::MsgFunc_ShowMenu( bf_read &msg)
{
#if defined( HL2MP )
	m_bNetworkMenu = true;
#endif
	m_bitsValidSlots = (short)msg.ReadWord();
	int DisplayTime = msg.ReadChar();
	int NeedMore = msg.ReadByte();

	if ( DisplayTime > 0 )
	{
		m_flShutoffTime = DisplayTime + gpGlobals->realtime;

	}
	else
	{
		m_flShutoffTime = -1;
	}

	if ( m_bitsValidSlots )
	{
		char szString[2048];
		msg.ReadString( szString, sizeof(szString) );

		if ( !m_fWaitingForMore ) // this is the start of a new menu
		{
			m_bMenuDisplayed = false;
			m_bMenuTakesInput = false;
			Q_strncpy( g_szPrelocalisedMenuString, szString, sizeof( g_szPrelocalisedMenuString ) );
		}
		else
		{  // append to the current menu string
			Q_strncat( g_szPrelocalisedMenuString, szString, sizeof( g_szPrelocalisedMenuString ), COPY_ALL_CHARACTERS );
		}

		if ( !NeedMore )
		{  
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("MenuOpen");
			m_nSelectedItem = -1;
			
			// we have the whole string, so we can localise it now
			char szMenuString[MAX_MENU_STRING];
			Q_strncpy( szMenuString, ConvertCRtoNL( hudtextmessage->BufferedLocaliseTextString( g_szPrelocalisedMenuString ) ), sizeof( szMenuString ) );
			g_pVGuiLocalize->ConvertANSIToUnicode( szMenuString, g_szMenuString, sizeof( g_szMenuString ) );
			
			ProcessText();

			m_bMenuDisplayed = true;
			m_bMenuTakesInput = true;
		}
	}
	else
	{
		HideMenu();
		return;
	}

	m_fWaitingForMore = NeedMore;
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetPaintBackgroundEnabled( false );

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);
	SetBounds(0, y, screenWide, screenTall - y);

	ProcessText();
}
