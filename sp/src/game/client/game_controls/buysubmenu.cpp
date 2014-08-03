//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "buysubmenu.h"

#include <KeyValues.h>
#include <vgui_controls/WizardPanel.h>
#include <filesystem.h>
#include <game/client/iviewport.h>
#include <cdll_client_int.h>

#include "mouseoverpanelbutton.h"
// #include "cs_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CBuySubMenu::CBuySubMenu(vgui::Panel *parent, const char *name) : WizardSubPanel(parent, name)
{
	m_NextPanel = NULL;
	m_pFirstButton = NULL;
	SetProportional(true);
	
	m_pPanel = new EditablePanel( this, "ItemInfo" );// info window about these items
	m_pPanel->SetProportional( true );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBuySubMenu::~CBuySubMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: magic override to allow vgui to create mouse over buttons for us
//-----------------------------------------------------------------------------
Panel *CBuySubMenu::CreateControlByName( const char *controlName )
{
	if( !Q_stricmp( "MouseOverPanelButton", controlName ) )
	{
		MouseOverPanelButton *newButton = CreateNewMouseOverPanelButton( m_pPanel );
		
		if( !m_pFirstButton )
		{
			m_pFirstButton = newButton;
		}
		return newButton;
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make the first buttons page get displayed when the menu becomes visible
//-----------------------------------------------------------------------------
void CBuySubMenu::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	for( int i = 0; i< GetChildCount(); i++ ) // get all the buy buttons to performlayout
	{
		MouseOverPanelButton *buyButton = dynamic_cast<MouseOverPanelButton *>(GetChild(i));
		if ( buyButton )
		{
			if( buyButton == m_pFirstButton && state == true )
				buyButton->ShowPage();
			else
				buyButton->HidePage();

			buyButton->InvalidateLayout();
		}
	}
}

CBuySubMenu* CBuySubMenu::CreateNewSubMenu()
{
	return new CBuySubMenu( this );
}

MouseOverPanelButton* CBuySubMenu::CreateNewMouseOverPanelButton(EditablePanel *panel)
{ 
	return new MouseOverPanelButton(this, NULL, panel);
}


//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CBuySubMenu::OnCommand( const char *command)
{
	if ( Q_strstr( command, ".res" ) ) // if its a .res file then its a new menu
	{ 
		int i;
		// check the cache
		for ( i = 0; i < m_SubMenus.Count(); i++ )
		{
			if ( !Q_stricmp( m_SubMenus[i].filename, command ) )
			{
				m_NextPanel = m_SubMenus[i].panel;
				Assert( m_NextPanel );
				m_NextPanel->InvalidateLayout(); // force it to reset it prices
				break;
			}
		}	

		if ( i == m_SubMenus.Count() )
		{
			// not there, add a new entry
			SubMenuEntry_t newEntry;
			memset( &newEntry, 0x0, sizeof( newEntry ) );

			CBuySubMenu *newMenu = CreateNewSubMenu();
			newMenu->LoadControlSettings( command );
			m_NextPanel = newMenu;
			Q_strncpy( newEntry.filename, command, sizeof( newEntry.filename ) );
			newEntry.panel = newMenu;
			m_SubMenus.AddToTail( newEntry );
		}

		GetWizardPanel()->OnNextButton();
	}
	else 
	{
		GetWizardPanel()->Close();
		gViewPortInterface->ShowBackGround( false );
	
		if ( Q_stricmp( command, "vguicancel" ) != 0 )
			engine->ClientCmd( command );

		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Causes the panel to delete itself when it closes
//-----------------------------------------------------------------------------
void CBuySubMenu::DeleteSubPanels()
{
	if ( m_NextPanel )
	{
		m_NextPanel->SetVisible( false );
		m_NextPanel = NULL;
	}

	m_pFirstButton = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: return the next panel to show
//-----------------------------------------------------------------------------
vgui::WizardSubPanel *CBuySubMenu::GetNextSubPanel()
{ 
	return m_NextPanel;
}




