//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "eternalcity_hud.h"
#include "c_basecombatweapon.h"
#include "c_baseplayer.h"
#include "clientmode_shared.h"
#include "hud_chat.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace
{
	const int kHudMargin = 20;
	const int kPanelWidth = 520;
	const int kPanelHeight = 380;
	const int kTabHeight = 38;
}

DECLARE_HUDELEMENT( CEternalCityHud );

static void ExecuteEternalCityCommand( const char *pCommand )
{
	if ( !engine->IsInGame() || !C_BasePlayer::GetLocalPlayer() )
	{
		Warning( "Eternal City UI requires a loaded game.\n" );
		return;
	}

	CEternalCityHud *pHud = GET_HUDELEMENT( CEternalCityHud );
	if ( !pHud )
	{
		Warning( "Eternal City HUD is unavailable.\n" );
		return;
	}
	pHud->HandleCommand( pCommand );
}

CON_COMMAND( ec_character, "Toggle the mock Eternal City character panel." )
{
	ExecuteEternalCityCommand( "ec_character" );
}

CON_COMMAND( ec_equipment, "Toggle the mock Eternal City equipment panel." )
{
	ExecuteEternalCityCommand( "ec_equipment" );
}

CON_COMMAND( ec_inventory, "Toggle the mock Eternal City inventory panel." )
{
	ExecuteEternalCityCommand( "ec_inventory" );
}

CON_COMMAND( ec_close, "Close the Eternal City modal panel." )
{
	ExecuteEternalCityCommand( "ec_close" );
}

CEternalCityHud::CEternalCityHud( const char *pElementName )
	: BaseClass( NULL, "EternalCityHud" ),
	  CHudElement( pElementName ),
	  m_PanelType( PANEL_NONE ),
	  m_bPrototypeVestEquipped( false ),
	  m_iMockMedkits( 2 ),
	  m_hFont( 0 )
{
	SetParent( g_pClientMode->GetViewport() );
	SetPaintBackgroundEnabled( false );
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetVisible( true );
	SetActive( true );
}

void CEternalCityHud::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_hFont = pScheme->GetFont( "Default", true );
}

void CEternalCityHud::LevelShutdown()
{
	CloseModal();
}

void CEternalCityHud::ToggleCharacter()
{
	TogglePanel( PANEL_CHARACTER );
}

void CEternalCityHud::ToggleEquipment()
{
	TogglePanel( PANEL_EQUIPMENT );
}

void CEternalCityHud::ToggleInventory()
{
	TogglePanel( PANEL_INVENTORY );
}

bool CEternalCityHud::IsModalOpen() const
{
	return m_PanelType != PANEL_NONE;
}

bool CEternalCityHud::HandleCommand( const char *pCommand )
{
	if ( !pCommand || !pCommand[0] )
		return false;

	if ( Q_stricmp( pCommand, "ec_character" ) == 0 )
		ToggleCharacter();
	else if ( Q_stricmp( pCommand, "ec_equipment" ) == 0 )
		ToggleEquipment();
	else if ( Q_stricmp( pCommand, "ec_inventory" ) == 0 )
		ToggleInventory();
	else if ( Q_stricmp( pCommand, "ec_close" ) == 0 )
		CloseModal();
	else
		return false;

	return true;
}

bool CEternalCityHud::HandleShortcut( vgui::KeyCode keynum, const char *pBinding )
{
	if ( HandleCommand( pBinding ) )
		return true;

	const bool bUnbound = !pBinding || !pBinding[0];
	if ( keynum == KEY_E && ( bUnbound || Q_stricmp( pBinding, "+use" ) == 0 ) &&
		( vgui::input()->IsKeyDown( KEY_LCONTROL ) || vgui::input()->IsKeyDown( KEY_RCONTROL ) ) )
		ToggleCharacter();
	else if ( keynum == KEY_I && bUnbound )
		ToggleEquipment();
	else if ( keynum == KEY_J && bUnbound )
		ToggleInventory();
	else
		return false;

	return true;
}

void CEternalCityHud::TogglePanel( PanelType_t panel )
{
	CHudChat *pChat = GET_HUDELEMENT( CHudChat );
	const vgui::VPANEL modalSurface = vgui::input()->GetAppModalSurface();
	if ( !engine->IsInGame() || !C_BasePlayer::GetLocalPlayer() || engine->IsPlayingDemo() ||
		enginevgui->IsGameUIVisible() || ( pChat && pChat->GetMessageMode() != MM_NONE ) ||
		( modalSurface && modalSurface != GetVPanel() ) )
	{
		Warning( "Eternal City UI cannot open while another UI owns focus or no live game is available.\n" );
		return;
	}

	if ( m_PanelType == panel )
	{
		CloseModal();
		return;
	}

	m_PanelType = panel;
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	MakePopup();
	MoveToFront();
	RequestFocus();
	vgui::input()->SetAppModalSurface( GetVPanel() );
	InvalidateLayout( true, true );
}

void CEternalCityHud::CloseModal()
{
	if ( !IsModalOpen() )
		return;

	if ( vgui::input()->GetAppModalSurface() == GetVPanel() )
		vgui::input()->ReleaseAppModalSurface();

	m_PanelType = PANEL_NONE;
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetVisible( true );
}

void CEternalCityHud::PerformLayout()
{
	int width, height;
	vgui::surface()->GetScreenSize( width, height );
	SetBounds( 0, 0, width, height );
	BaseClass::PerformLayout();
}

void CEternalCityHud::DrawRect( int x, int y, int width, int height, const Color &color )
{
	vgui::surface()->DrawSetColor( color );
	vgui::surface()->DrawFilledRect( x, y, x + width, y + height );
}

void CEternalCityHud::DrawText( int x, int y, const char *text, const Color &color )
{
	if ( !text || !m_hFont || !g_pVGuiLocalize )
		return;

	wchar_t unicode[256];
	g_pVGuiLocalize->ConvertANSIToUnicode( text, unicode, sizeof( unicode ) );

	vgui::surface()->DrawSetTextFont( m_hFont );
	vgui::surface()->DrawSetTextColor( color );
	vgui::surface()->DrawSetTextPos( x, y );
	vgui::surface()->DrawPrintText( unicode, wcslen( unicode ) );
}

void CEternalCityHud::DrawVitals()
{
	const int width = GetWide();
	const int height = GetTall();
	const int cardY = height - 104;
	const Color panelColor( 10, 18, 26, 220 );
	const Color accentColor( 78, 192, 183, 255 );
	const Color textColor( 235, 241, 244, 255 );
	const Color mutedColor( 163, 180, 190, 255 );

	DrawRect( kHudMargin, cardY, 304, 84, panelColor );
	DrawRect( kHudMargin, cardY, 4, 84, accentColor );
	DrawText( kHudMargin + 16, cardY + 9, "ETERNAL CITY  /  PROTOTYPE", mutedColor );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	char status[128];
	if ( pPlayer )
		Q_snprintf( status, sizeof( status ), "HEALTH  %d", pPlayer->GetHealth() );
	else
		Q_strncpy( status, "HEALTH  --", sizeof( status ) );
	DrawText( kHudMargin + 16, cardY + 31, status, textColor );

	if ( pPlayer )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
		if ( pWeapon && pWeapon->GetPrimaryAmmoType() >= 0 )
		{
			const int clip = pWeapon->Clip1();
			const int reserve = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			if ( clip >= 0 )
				Q_snprintf( status, sizeof( status ), "AMMO  %d  /  %d", clip, reserve );
			else
				Q_snprintf( status, sizeof( status ), "AMMO  %d", reserve );
		}
		else
		{
			Q_strncpy( status, "AMMO  --", sizeof( status ) );
		}
	}
	else
	{
		Q_strncpy( status, "AMMO  --", sizeof( status ) );
	}
	DrawText( kHudMargin + 16, cardY + 55, status, accentColor );

	const int recordWidth = 280;
	DrawRect( width - kHudMargin - recordWidth, cardY, recordWidth, 84, panelColor );
	DrawText( width - kHudMargin - recordWidth + 14, cardY + 9, "FIELD RECORD", mutedColor );
	DrawText( width - kHudMargin - recordWidth + 14, cardY + 31, "LEVEL  3  |  XP  240 / 400", textColor );
	DrawText( width - kHudMargin - recordWidth + 14, cardY + 55, "CREDITS  125", accentColor );
}

void CEternalCityHud::DrawActionButton( int x, int y, int width, const char *label, bool enabled )
{
	const Color buttonColor = enabled ? Color( 30, 70, 78, 255 ) : Color( 35, 43, 48, 255 );
	DrawRect( x, y, width, 38, buttonColor );
	DrawText( x + 12, y + 10, label, enabled ? Color( 235, 241, 244, 255 ) : Color( 145, 155, 160, 255 ) );
}

void CEternalCityHud::DrawModal()
{
	const int width = GetWide();
	const int height = GetTall();
	const int panelWidth = MIN( kPanelWidth, width - 32 );
	const int panelHeight = MIN( kPanelHeight, height - 48 );
	const int x = ( width - panelWidth ) / 2;
	const int y = ( height - panelHeight ) / 2;
	const Color textColor( 235, 241, 244, 255 );
	const Color mutedColor( 163, 180, 190, 255 );
	const Color accentColor( 78, 192, 183, 255 );

	DrawRect( 0, 0, width, height, Color( 0, 0, 0, 125 ) );
	DrawRect( x, y, panelWidth, panelHeight, Color( 12, 22, 30, 248 ) );
	DrawRect( x, y, panelWidth, 3, accentColor );
	DrawText( x + 24, y + 18, "CHARACTER  /  EQUIPMENT  /  INVENTORY", textColor );

	const int tabY = y + 60;
	const int tabWidth = ( panelWidth - 48 ) / 3;
	const char *tabs[] = { "Character", "Equipment", "Inventory" };
	const PanelType_t tabTypes[] = { PANEL_CHARACTER, PANEL_EQUIPMENT, PANEL_INVENTORY };
	for ( int i = 0; i < 3; ++i )
	{
		const int tabX = x + 24 + i * tabWidth;
		DrawRect( tabX, tabY, tabWidth - 6, kTabHeight, m_PanelType == tabTypes[i] ? Color( 30, 70, 78, 255 ) : Color( 24, 38, 47, 255 ) );
		DrawText( tabX + 10, tabY + 10, tabs[i], m_PanelType == tabTypes[i] ? accentColor : textColor );
	}

	const int contentX = x + 28;
	const int contentY = tabY + kTabHeight + 26;
	if ( m_PanelType == PANEL_CHARACTER )
	{
		DrawText( contentX, contentY, "FIELD OPERATIVE  /  LEVEL 3", textColor );
		DrawText( contentX, contentY + 30, "Experience: 240 / 400", mutedColor );
		DrawText( contentX, contentY + 56, "Credits: 125  (session only)", mutedColor );
		DrawText( contentX, contentY + 82, "Health and ammunition remain live HL2 values.", mutedColor );
	}
	else if ( m_PanelType == PANEL_EQUIPMENT )
	{
		DrawText( contentX, contentY, "EQUIPPED GEAR", textColor );
		DrawText( contentX, contentY + 30, m_bPrototypeVestEquipped ? "Prototype vest  /  Defense +10" : "No prototype vest equipped", mutedColor );
		DrawActionButton( contentX, contentY + 76, MIN( 250, panelWidth - 56 ), m_bPrototypeVestEquipped ? "Unequip prototype vest" : "Equip prototype vest", true );
	}
	else
	{
		char inventory[96];
		Q_snprintf( inventory, sizeof( inventory ), "Medkits: %d  (mock inventory)", m_iMockMedkits );
		DrawText( contentX, contentY, inventory, textColor );
		DrawText( contentX, contentY + 30, "Using an item changes mock inventory only.", mutedColor );
		DrawActionButton( contentX, contentY + 76, MIN( 250, panelWidth - 56 ), "Use one medkit", m_iMockMedkits > 0 );
	}

	DrawText( x + 24, y + panelHeight - 32, "ESC closes  |  Click a tab to switch", mutedColor );
}

void CEternalCityHud::Paint()
{
	if ( !ShouldDraw() )
		return;

	if ( GetWide() < 100 || GetTall() < 100 )
		return;

	DrawVitals();

	const int slotSize = 40;
	const int gap = 6;
	const int slotCount = 6;
	const int barWidth = slotCount * slotSize + ( slotCount - 1 ) * gap;
	const int startX = ( GetWide() - barWidth ) / 2;
	const int startY = GetTall() - 58;
	for ( int slot = 0; slot < slotCount; ++slot )
	{
		const int slotX = startX + slot * ( slotSize + gap );
		DrawRect( slotX, startY, slotSize, slotSize, Color( 10, 18, 26, 220 ) );
		char number[8];
		Q_snprintf( number, sizeof( number ), "%d", slot + 1 );
		DrawText( slotX + 14, startY + 11, number, Color( 163, 180, 190, 255 ) );
	}

	if ( IsModalOpen() )
		DrawModal();
}

bool CEternalCityHud::ShouldDraw()
{
	return CHudElement::ShouldDraw();
}

void CEternalCityHud::OnMousePressed( vgui::MouseCode code )
{
	if ( code != MOUSE_LEFT || !IsModalOpen() )
		return;

	int cursorX, cursorY;
	vgui::input()->GetCursorPosition( cursorX, cursorY );
	ScreenToLocal( cursorX, cursorY );
	const int panelWidth = MIN( kPanelWidth, GetWide() - 32 );
	const int panelHeight = MIN( kPanelHeight, GetTall() - 48 );
	const int x = ( GetWide() - panelWidth ) / 2;
	const int y = ( GetTall() - panelHeight ) / 2;
	const int tabY = y + 60;
	const int tabWidth = ( panelWidth - 48 ) / 3;

	if ( cursorY >= tabY && cursorY < tabY + kTabHeight && cursorX >= x + 24 && cursorX < x + panelWidth - 24 )
	{
		const int tab = MIN( 2, ( cursorX - x - 24 ) / tabWidth );
		TogglePanel( static_cast<PanelType_t>( PANEL_CHARACTER + tab ) );
		return;
	}

	const int contentY = tabY + kTabHeight + 26;
	const int contentX = x + 28;
	const int buttonWidth = MIN( 250, panelWidth - 56 );
	const int buttonY = contentY + 76;
	if ( cursorX < contentX || cursorX >= contentX + buttonWidth || cursorY < buttonY || cursorY >= buttonY + 38 )
		return;

	if ( m_PanelType == PANEL_EQUIPMENT )
		m_bPrototypeVestEquipped = !m_bPrototypeVestEquipped;
	else if ( m_PanelType == PANEL_INVENTORY && m_iMockMedkits > 0 )
		--m_iMockMedkits;

	Repaint();
}

void CEternalCityHud::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		CloseModal();
		return;
	}

	if ( HandleShortcut( code, engine->Key_BindingForKey( code ) ) )
		return;

	BaseClass::OnKeyCodePressed( code );
}
