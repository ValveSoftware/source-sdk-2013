//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "vgui_controls/KeyBindingHelpDialog.h"
#include "vgui_controls/ListPanel.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui/ILocalize.h"
#include "vgui/IInput.h"
#include "vgui/ISystem.h"
#include "KeyValues.h"
#include "vgui/Cursor.h"
#include "tier1/utldict.h"
#include "vgui_controls/KeyBoardEditorDialog.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


using namespace vgui;

// If the user holds the key bound to help down for this long, then the dialog will stay on automatically
#define KB_HELP_CONTINUE_SHOWING_TIME		1.0

static bool BindingLessFunc( KeyValues * const & lhs, KeyValues * const &rhs )
{
	KeyValues *p1, *p2;

	p1 = const_cast< KeyValues * >( lhs );
	p2 = const_cast< KeyValues * >( rhs );
	return ( Q_stricmp( p1->GetString( "Action" ), p2->GetString( "Action" ) ) < 0 ) ? true : false;
}

CKeyBindingHelpDialog::CKeyBindingHelpDialog( Panel *parent, Panel *panelToView, KeyBindingContextHandle_t handle, KeyCode code, int modifiers )
	: BaseClass( parent, "KeyBindingHelpDialog" ),
	m_Handle( handle ),
	m_KeyCode( code ),
	m_Modifiers( modifiers ),
	m_bPermanent( false )
{
	Assert( panelToView );
	m_hPanel = panelToView;

	m_pList = new ListPanel( this, "KeyBindings" );
	m_pList->SetIgnoreDoubleClick( true );
	m_pList->AddColumnHeader(0, "Action", "#KBEditorBindingName", 175, 0);
	m_pList->AddColumnHeader(1, "Binding", "#KBEditorBinding", 175, 0);
	m_pList->AddColumnHeader(2, "Description", "#KBEditorDescription", 300, 0);

	LoadControlSettings( "resource/KeyBindingHelpDialog.res" );

	if ( panelToView && panelToView->GetName() && panelToView->GetName()[0] )
	{
		SetTitle( panelToView->GetName(), true );
	}
	else
	{
		SetTitle( "#KBHelpDialogTitle", true );
	}

	SetSmallCaption( true );
	SetMinimumSize( 400, 400 );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetSizeable( true );
	SetMoveable( true );
	SetMenuButtonVisible( false );

	SetVisible( true );

	MoveToCenterOfScreen();

	PopulateList();

	m_flShowTime = system()->GetCurrentTime();
	ivgui()->AddTickSignal( GetVPanel(), 0 );

	input()->SetAppModalSurface( GetVPanel() );
}

CKeyBindingHelpDialog::~CKeyBindingHelpDialog()
{
	if ( input()->GetAppModalSurface() == GetVPanel() )
	{
		input()->SetAppModalSurface( 0 );
	}
}

void CKeyBindingHelpDialog::OnTick()
{
	BaseClass::OnTick();

	bool keyStillDown = IsHelpKeyStillBeingHeld();

	double curtime = system()->GetCurrentTime();
	double elapsed = curtime - m_flShowTime;
	// After a second of holding the key, releasing the key will close the dialog
	if ( elapsed > KB_HELP_CONTINUE_SHOWING_TIME )
	{
		if ( !keyStillDown )
		{
			MarkForDeletion();
			return;
		}
	}
	// Otherwise, if they tapped the key within a second and now have released...
	else if ( !keyStillDown )
	{
		// Continue showing dialog indefinitely
		ivgui()->RemoveTickSignal( GetVPanel() );
		m_bPermanent = true;
	}
}

// The key originally bound to help was pressed
void CKeyBindingHelpDialog::HelpKeyPressed()
{
	// Don't kill while editor is being shown...
	if ( m_hKeyBindingsEditor.Get() )
		return;

	if ( m_bPermanent )
	{
		MarkForDeletion();
	}
}

bool CKeyBindingHelpDialog::IsHelpKeyStillBeingHeld()
{
	bool keyDown = input()->IsKeyDown( m_KeyCode );
	if ( !keyDown )
		return false;

	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));

	int modifiers = 0;
	if ( shift )
	{
		modifiers |= MODIFIER_SHIFT;
	}
	if ( ctrl )
	{
		modifiers |= MODIFIER_CONTROL;
	}
	if ( alt )
	{
		modifiers |= MODIFIER_ALT;
	}

	if ( modifiers != m_Modifiers )
	{
		return false;
	}

	return true;
}

void CKeyBindingHelpDialog::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "OK" ) ||
		 !Q_stricmp( cmd, "cancel" ) ||
		 !Q_stricmp( cmd, "Close" ) )
	{
		MarkForDeletion();
	}
	else if ( !Q_stricmp( cmd, "edit" ) )
	{
		// Show the keybindings edit dialog
		if ( m_hKeyBindingsEditor.Get() )
		{
			delete m_hKeyBindingsEditor.Get();
		}

		// Don't delete panel if H key is released...

		m_hKeyBindingsEditor = new CKeyBoardEditorDialog( this, m_hPanel, m_Handle );
		m_hKeyBindingsEditor->DoModal();

		ivgui()->RemoveTickSignal( GetVPanel() );
		m_bPermanent = true;
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

void CKeyBindingHelpDialog::OnKeyCodeTyped(vgui::KeyCode code)
{
	BaseClass::OnKeyCodeTyped( code );
}

void CKeyBindingHelpDialog::GetMappingList( Panel *panel, CUtlVector< PanelKeyBindingMap * >& maps )
{
	PanelKeyBindingMap *map = panel->GetKBMap();
	while ( map )
	{
		maps.AddToTail( map );
		map = map->baseMap;
	}
}


void CKeyBindingHelpDialog::AnsiText( char const *token, char *out, int nBuflen )
{
	out[ 0 ] = 0;

	wchar_t *str = g_pVGuiLocalize->Find( token );
	if ( !str )
	{
		Q_strncpy( out, token, nBuflen );
	}
	else
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI( str, out, nBuflen );
	}
}

struct ListInfo_t
{
	PanelKeyBindingMap *m_pMap;
	Panel *m_pPanel;
};

void CKeyBindingHelpDialog::PopulateList()
{
	m_pList->DeleteAllItems();

	int i, j;

	CUtlVector< ListInfo_t > maps;
	vgui::Panel *pPanel = m_hPanel;
	while ( pPanel->IsKeyBindingChainToParentAllowed() )
	{
		PanelKeyBindingMap *map = pPanel->GetKBMap();
		while ( map )
		{
			int k;
			int c = maps.Count();
			for ( k = 0; k < c; ++k )
			{
				if ( maps[k].m_pMap == map )
					break;
			}
			if ( k == c )
			{
				int iMap = maps.AddToTail( );
				maps[iMap].m_pMap = map;
				maps[iMap].m_pPanel = pPanel;
			}
			map = map->baseMap;
		}

		pPanel = pPanel->GetParent();
		if ( !pPanel )
			break;
	}

	CUtlRBTree< KeyValues *, int >	sorted( 0, 0, BindingLessFunc );

	// add header item
	int c = maps.Count();
	for ( i = 0; i < c; ++i )
	{
		PanelKeyBindingMap *m = maps[ i ].m_pMap;
		pPanel = maps[i].m_pPanel;
		Assert( m );

		int bindings = m->boundkeys.Count();
		for ( j = 0; j < bindings; ++j )
		{
			BoundKey_t *kbMap = &m->boundkeys[ j ];
			Assert( kbMap );

			// Create a new: blank item
			KeyValues *item = new KeyValues( "Item" );
			
			// Fill in data
			char loc[ 128 ];
			Q_snprintf( loc, sizeof( loc ), "#%s", kbMap->bindingname );

			char ansi[ 256 ];
			AnsiText( loc, ansi, sizeof( ansi ) );

			item->SetString( "Action", ansi );
			item->SetWString( "Binding", Panel::KeyCodeModifiersToDisplayString( (KeyCode)kbMap->keycode, kbMap->modifiers ) );

			// Find the binding
			KeyBindingMap_t *bindingMap = pPanel->LookupBinding( kbMap->bindingname );
			if ( bindingMap && 
				 bindingMap->helpstring )
			{
				AnsiText( bindingMap->helpstring, ansi, sizeof( ansi ) );
				item->SetString( "Description", ansi );
			}
			
			item->SetPtr( "Item", kbMap );			

			sorted.Insert( item );
		}

		// Now try and find any "unbound" keys...
		int mappings = m->entries.Count();
		for ( j = 0; j < mappings; ++j )
		{
			KeyBindingMap_t *kbMap = &m->entries[ j ];

			// See if it's bound
			CUtlVector< BoundKey_t * > list;
			pPanel->LookupBoundKeys( kbMap->bindingname, list );
			if ( list.Count() > 0 )
				continue;

			// Not bound, add a placeholder entry
			// Create a new: blank item
			KeyValues *item = new KeyValues( "Item" );
			
			// fill in data
			char loc[ 128 ];
			Q_snprintf( loc, sizeof( loc ), "#%s", kbMap->bindingname );

			char ansi[ 256 ];
			AnsiText( loc, ansi, sizeof( ansi ) );

			item->SetString( "Action", ansi );
			item->SetWString( "Binding", L"" );
			if ( kbMap->helpstring )
			{
				AnsiText( kbMap->helpstring, ansi, sizeof( ansi ) );
				item->SetString( "Description", ansi );
			}

			item->SetPtr( "Unbound", kbMap );						

			sorted.Insert( item );
		}
	}

	for ( j = sorted.FirstInorder() ; j != sorted.InvalidIndex(); j = sorted.NextInorder( j ) )
	{
		KeyValues *item = sorted[ j ];

		// Add to list
		m_pList->AddItem( item, 0, false, false );

		item->deleteThis();
	}

	sorted.RemoveAll();
}
