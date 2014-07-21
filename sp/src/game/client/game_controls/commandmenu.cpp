//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
#include <cdll_util.h>
#include <globalvars_base.h>
#include <icvar.h>
#include <filesystem.h>

#include "commandmenu.h"
#include "vgui_controls/MenuItem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CommandMenu::CommandMenu( Panel *parent, const char *panelName, IViewPort * viewport) : Menu( parent, panelName )
{ 
	if ( !viewport )
		return;

	m_ViewPort = viewport; 
	SetVisible( false );
	m_CurrentMenu = this;

	m_MenuKeys = NULL;
}

bool CommandMenu::LoadFromFile( const char * fileName)	// load menu from KeyValues
{
	KeyValues * kv = new KeyValues(fileName);

	if  ( !kv->LoadFromFile( g_pFullFileSystem, fileName, "GAME" ) )
		return false;

	bool ret = LoadFromKeyValues( kv );

	kv->deleteThis();
	return ret;
}

CommandMenu::~CommandMenu()
{
	ClearMenu();
}

void CommandMenu::OnMessage(const KeyValues *params, VPANEL fromPanel)
{
	char text[255];
	bool bHandled = false;

	KeyValues *param1 = const_cast<KeyValues *>(params);

	// toggle attached cvar, if any
	Q_strncpy( text, param1->GetString("toggle"), sizeof( text ) );

	if ( text[0] )
	{
		ConVarRef convar( text );
		if ( convar.IsValid() )
		{
			// toggle cvar 

			if ( convar.GetInt() )	
			{
				convar.SetValue( 0 );
			}
			else
			{
				convar.SetValue( 1 );
			}

			UpdateMenu();
		}
		else
		{
			Msg("CommandComboBox::OnMessage: cvar %s not found.\n", param1->GetString("typedata") );
		}

		bHandled = true;
	}

	// execute attached command, if any
	Q_strncpy( text, param1->GetString("command"), sizeof( text ) );
	if ( text[0] )
	{
		engine->ClientCmd( text );
		bHandled = true;
	}

	// fire custom message, if any
	Q_strncpy( text, param1->GetString("custom"), sizeof( text ) );
	if ( text[0] )
	{
		OnCustomItem( param1 ); // let derived class decide what to do
		bHandled = true;
	}

	if ( bHandled )
	{
		PostMessage( GetParent(), new KeyValues("CommandMenuClosed") );
	}
		
	BaseClass::OnMessage( params, fromPanel );
}

void CommandMenu::StartNewSubMenu(KeyValues * params)
{
	CommandMenuItem menuitem;
	menuitem.menu = m_CurrentMenu;

	Menu * menu = new Menu( this, params->GetString("name") );	// create new menu

	menuitem.itemnr = m_CurrentMenu->AddCascadingMenuItem( params->GetString("label"), this, menu, params ); // add to current menu as item

	m_MenuItems.AddToTail( menuitem ); // add to global list

	m_pMenuStack.Push( m_CurrentMenu ); // remember current menu
	
	m_CurrentMenu = menu; // continue adding items in new menu
}

void CommandMenu::FinishSubMenu()
{
	m_CurrentMenu = m_pMenuStack.Top(); // get menu one level above
	m_pMenuStack.Pop(); // remove it from stack
}

void CommandMenu::AddMenuCommandItem(KeyValues * params)
{
	CommandMenuItem menuitem;	// create new menuItem
	menuitem.menu = m_CurrentMenu;	// save the current menu context
	menuitem.itemnr = m_CurrentMenu->AddMenuItem( params->GetString("label"), params->MakeCopy(), this, params ); // add it
	m_MenuItems.AddToTail( menuitem ); // add to global list
}

void CommandMenu::AddMenuToggleItem(KeyValues * params)
{
	CommandMenuItem menuitem;	// create new menuItem
	menuitem.menu = m_CurrentMenu;	// save the current menu context
	menuitem.itemnr = m_CurrentMenu->AddCheckableMenuItem( params->GetString("label"), params->MakeCopy(), this, params  ); // add it
	m_MenuItems.AddToTail( menuitem ); // add to global list
}

void CommandMenu::AddMenuCustomItem(KeyValues * params)
{
	CommandMenuItem menuitem;	// create new menuItem
	menuitem.menu = m_CurrentMenu;	// save the current menu context
	menuitem.itemnr = AddCustomItem( params, m_CurrentMenu );
	m_MenuItems.AddToTail( menuitem ); // add to global list
}

void CommandMenu::ClearMenu()
{
	SetVisible( false );
	m_pMenuStack.Clear();
	m_MenuItems.RemoveAll(); 
	// DeleteAllItems();
	MarkForDeletion();

	if ( m_MenuKeys )
	{
		m_MenuKeys->deleteThis();
		m_MenuKeys = NULL;
	}

}

void CommandMenu::RebuildMenu()
{
	if ( !m_MenuKeys )
		return;	

	m_pMenuStack.Clear();
	m_MenuItems.RemoveAll(); 
	DeleteAllItems();
	
	LoadFromKeyValues( m_MenuKeys ); // and reload respecting new team, mapname etc.
}

void CommandMenu::UpdateMenu()
{
	char text[255];

	int num = m_MenuItems.Count();

	for (int i=0; i < num; i++)
	{
		CommandMenuItem menuitem = m_MenuItems.Element(i);
		KeyValues * keys = menuitem.menu->GetItemUserData( menuitem.itemnr );

		if ( !keys )
			continue;

		// let custom menu items update themself
		Q_strncpy( text,  keys->GetString("custom"), sizeof(text) );
				
		if ( text[0] )
		{
			// let derived class modify the menu item
			UpdateCustomItem( keys,  menuitem.menu->GetMenuItem(menuitem.itemnr) ); 
			continue; 
		}

		// update toggle buttons
		Q_strncpy( text,  keys->GetString("toggle"), sizeof(text) );

		if ( text[0] )
		{
			// set toggle state equal to cvar state
			ConVarRef convar( text );

			if ( convar.IsValid() )
			{
				menuitem.menu->SetMenuItemChecked( menuitem.itemnr, convar.GetBool() );
			}
		}
	}
}

void CommandMenu::SetVisible(bool state)
{
	if ( state && !IsVisible() )
	{
		UpdateMenu();
	}

	BaseClass::SetVisible( state );
}

bool CommandMenu::CheckRules(const char *rule, const char *ruledata)
{
	if ( !rule  || !ruledata )
	{
		return true; // no rule defined, show item
	}

	if ( Q_strcmp( rule, "team") == 0 )
	{
		// if team is same as specified in rule, show item
		return ( Q_strcmp( m_CurrentTeam, ruledata ) == 0 );
	}
	else if ( Q_strcmp( rule, "map") == 0 )
	{
		// if team is same as specified in rule, show item
		return ( Q_strcmp( m_CurrentMap, ruledata ) == 0 );
	}

	return true;
}

KeyValues *	CommandMenu::GetKeyValues()
{
	return m_MenuKeys;
}

bool CommandMenu::LoadFromKeyValues( KeyValues * params )
{
	if ( !params )
		return false;

	Q_snprintf( m_CurrentTeam, 4, "%i", GetLocalPlayerTeam() );

	Q_FileBase( engine->GetLevelName(), m_CurrentMap, sizeof(m_CurrentMap) );
	
	if ( params != m_MenuKeys )
	{
		if ( m_MenuKeys )
			m_MenuKeys->deleteThis();

		m_MenuKeys = params->MakeCopy(); // save keyvalues
	}

	// iterate through all menu items

	KeyValues * subkey = m_MenuKeys->GetFirstSubKey();

	while ( subkey )
	{
		if ( subkey->GetDataType() == KeyValues::TYPE_NONE )
		{
			if ( !LoadFromKeyValuesInternal( subkey, 0 ) ) // recursive call
				return false;
		}

		subkey = subkey->GetNextKey();
	}
	
	UpdateMenu();

	return true;
}

bool CommandMenu::LoadFromKeyValuesInternal(KeyValues * key, int depth)
{
	char text[255];
	KeyValues * subkey = NULL;

	if ( depth > 100 )
	{
		Msg("CommandMenu::LoadFromKeyValueInternal: depth > 100.\n");
		return false;
	}

	Q_strncpy( text,  key->GetString("custom"), sizeof(text) );	 // get type

	if  ( text[0] ) 
	{
		AddMenuCustomItem( key ); // do whatever custom item wants to
		return true;
	}
	
	if ( !CheckRules( key->GetString("rule"), key->GetString("ruledata") ) )
	{	
		return true;
	}

	// rules OK add subkey
	Q_strncpy( text,  key->GetString("toggle"), sizeof(text) );	 // get type

	if ( text[0] )
	{
		AddMenuToggleItem( key );
		return true;
	}

	Q_strncpy( text,  key->GetString("command"), sizeof(text) );	 // get type

	if ( text[0] )
	{
		AddMenuCommandItem( key );
		return true;
	}

	// not a command, nor a toggle. Must be a submenu:
		
	StartNewSubMenu( key );	// create submenu

	// iterate through all subkeys

	subkey = key->GetFirstSubKey();

	while ( subkey )
	{
		if ( subkey->GetDataType() == KeyValues::TYPE_NONE )
		{
			LoadFromKeyValuesInternal( subkey, depth+1 ); // recursive call
		}

		subkey = subkey->GetNextKey();
	}

	FinishSubMenu(); // go one level back
	
	return true;
}