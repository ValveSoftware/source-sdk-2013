//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>

#include "classmenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <filesystem.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>
#include "inputsystem/iinputsystem.h"

#include "cdll_util.h"
#include "IGameUIFuncs.h" // for key bindings
#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif
#include <game/client/iviewport.h>

#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#ifdef TF_CLIENT_DLL
#define HUD_CLASSAUTOKILL_FLAGS		( FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO )
#else
#define HUD_CLASSAUTOKILL_FLAGS		( FCVAR_CLIENTDLL | FCVAR_ARCHIVE )
#endif // !TF_CLIENT_DLL

ConVar hud_classautokill( "hud_classautokill", "1", HUD_CLASSAUTOKILL_FLAGS, "Automatically kill player after choosing a new playerclass." );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClassMenu::CClassMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_CLASS)
{
	m_pViewPort = pViewPort;
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iTeam = 0;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this class
	m_pPanel = new EditablePanel( this, "ClassInfo" );

	LoadControlSettings( "Resource/UI/ClassMenu.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CClassMenu::CClassMenu(IViewPort *pViewPort, const char *panelName) : Frame(NULL, panelName)
{
	m_pViewPort = pViewPort;
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iTeam = 0;

	// initialize dialog
	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this class
	m_pPanel = new EditablePanel( this, "ClassInfo" );

	// Inheriting classes are responsible for calling LoadControlSettings()!
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CClassMenu::~CClassMenu()
{
}

MouseOverPanelButton* CClassMenu::CreateNewMouseOverPanelButton(EditablePanel *panel)
{ 
	return new MouseOverPanelButton(this, "MouseOverPanelButton", panel);
}


Panel *CClassMenu::CreateControlByName(const char *controlName)
{
	if( !Q_stricmp( "MouseOverPanelButton", controlName ) )
	{
		MouseOverPanelButton *newButton = CreateNewMouseOverPanelButton( m_pPanel );

		m_mouseoverButtons.AddToTail( newButton );
		return newButton;
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassMenu::Reset()
{
	for ( int i = 0 ; i < GetChildCount() ; ++i )
	{
		// Hide the subpanel for the MouseOverPanelButtons
		MouseOverPanelButton *pPanel = dynamic_cast<MouseOverPanelButton *>( GetChild( i ) );

		if ( pPanel )
		{
			pPanel->HidePage();
		}
	}

	// Turn the first button back on again (so we have a default description shown)
	Assert( m_mouseoverButtons.Count() );
	for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
	{
		if ( i == 0 )
		{
			m_mouseoverButtons[i]->ShowPage();	// Show the first page
		}
		else
		{
			m_mouseoverButtons[i]->HidePage();	// Hide the rest
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CClassMenu::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( const_cast<char *>( command ) );

#if !defined( CSTRIKE_DLL ) && !defined( TF_CLIENT_DLL )
		// They entered a command to change their class, kill them so they spawn with 
		// the new class right away
		if ( hud_classautokill.GetBool() )
		{
            engine->ClientCmd( "kill" );
		}
#endif // !CSTRIKE_DLL && !TF_CLIENT_DLL
	}

	Close();

	gViewPortInterface->ShowBackGround( false );

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: shows the class menu
//-----------------------------------------------------------------------------
void CClassMenu::ShowPanel(bool bShow)
{
	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );

		// load a default class page
		for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
		{
			if ( i == 0 )
			{
				m_mouseoverButtons[i]->ShowPage();	// Show the first page
			}
			else
			{
				m_mouseoverButtons[i]->HidePage();	// Hide the rest
			}
		}
		
		if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
		}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
	
	m_pViewPort->ShowBackGround( bShow );
}


void CClassMenu::SetData(KeyValues *data)
{
	m_iTeam = data->GetInt( "team" );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CClassMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the visibility of a button by name
//-----------------------------------------------------------------------------
void CClassMenu::SetVisibleButton(const char *textEntryName, bool state)
{
	Button *entry = dynamic_cast<Button *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetVisible(state);
	}
}

void CClassMenu::OnKeyCodePressed(KeyCode code)
{
	int nDir = 0;

	switch ( code )
	{
	case KEY_XBUTTON_UP:
	case KEY_XSTICK1_UP:
	case KEY_XSTICK2_UP:
	case KEY_UP:
	case KEY_XBUTTON_LEFT:
	case KEY_XSTICK1_LEFT:
	case KEY_XSTICK2_LEFT:
	case KEY_LEFT:
	case STEAMCONTROLLER_DPAD_LEFT:
		nDir = -1;
		break;

	case KEY_XBUTTON_DOWN:
	case KEY_XSTICK1_DOWN:
	case KEY_XSTICK2_DOWN:
	case KEY_DOWN:
	case KEY_XBUTTON_RIGHT:
	case KEY_XSTICK1_RIGHT:
	case KEY_XSTICK2_RIGHT:
	case KEY_RIGHT:
	case STEAMCONTROLLER_DPAD_RIGHT:
		nDir = 1;
		break;
	}

	if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else if ( nDir != 0 )
	{
		CUtlSortVector< SortedPanel_t, CSortedPanelYLess > vecSortedButtons;
		VguiPanelGetSortedChildButtonList( this, (void*)&vecSortedButtons, "&", 0 );

		int nNewArmed = VguiPanelNavigateSortedChildButtonList( (void*)&vecSortedButtons, nDir );

		if ( nNewArmed != -1 )
		{
			// Handled!
			if ( nNewArmed < m_mouseoverButtons.Count() )
			{
				m_mouseoverButtons[ nNewArmed ]->OnCursorEntered();
			}
			return;
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}






