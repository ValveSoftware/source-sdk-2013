//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "buymenu.h"

#include "buysubmenu.h"
using namespace vgui;

#include "mouseoverpanelbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuyMenu::CBuyMenu(IViewPort *pViewPort) : WizardPanel( NULL, PANEL_BUY )
{
	SetScheme("ClientScheme");
	SetTitle( "#Cstrike_Buy_Menu", true);

	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

	// hide the system buttons
	SetTitleBarVisible( false );

	SetAutoDelete( false ); // we reuse this panel, don't let WizardPanel delete us
	
	LoadControlSettings( "Resource/UI/BuyMenu.res" );
	ShowButtons( false );

	m_pViewPort = pViewPort;

	m_pMainMenu = new CBuySubMenu( this, "mainmenu" );
	m_pMainMenu->LoadControlSettings( "Resource/UI/MainBuyMenu.res" );
	m_pMainMenu->SetVisible( false );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuyMenu::~CBuyMenu()
{
	if ( m_pMainMenu )
		m_pMainMenu->DeleteSubPanels();	//?
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CBuyMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Update();

		Run( m_pMainMenu );

		SetMouseInputEnabled( true );

		engine->ClientCmd_Unrestricted( "gameui_preventescapetoshow\n" );
	}
	else
	{
		engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );

		SetVisible( false );
		SetMouseInputEnabled( false );
	}

	m_pViewPort->ShowBackGround( bShow );
}


void CBuyMenu::Update()
{
	//Don't need to do anything, but do need to implement this function as base is pure virtual
}
void CBuyMenu::OnClose()
{
	engine->ClientCmd_Unrestricted( "gameui_allowescapetoshow\n" );

	BaseClass::OnClose();
	ResetHistory();
}

void CBuyMenu::OnKeyCodePressed( vgui::KeyCode code )
{
	int nDir = 0;

	switch ( code )
	{
	case KEY_XBUTTON_UP:
	case KEY_XSTICK1_UP:
	case KEY_XSTICK2_UP:
	case KEY_UP:
	case STEAMCONTROLLER_DPAD_UP:
		nDir = -1;
		break;

	case KEY_XBUTTON_DOWN:
	case KEY_XSTICK1_DOWN:
	case KEY_XSTICK2_DOWN:
	case KEY_DOWN:
	case STEAMCONTROLLER_DPAD_DOWN:
		nDir = 1;
		break;
	}

	if ( nDir != 0 )
	{
		Panel *pSubPanel = ( GetCurrentSubPanel() ? GetCurrentSubPanel() : m_pMainMenu );

		CUtlSortVector< SortedPanel_t, CSortedPanelYLess > vecSortedButtons;
		VguiPanelGetSortedChildButtonList( pSubPanel, (void*)&vecSortedButtons, "&", 0 );

		if ( VguiPanelNavigateSortedChildButtonList( (void*)&vecSortedButtons, nDir ) != -1 )
		{
			// Handled!
			return;
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

void CBuyMenu::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_ESCAPE	)
	{
		OnClose();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

