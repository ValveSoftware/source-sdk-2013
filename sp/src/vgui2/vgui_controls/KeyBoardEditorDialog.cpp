//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "vgui_controls/KeyBoardEditorDialog.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/IVGui.h"
#include "vgui/ILocalize.h"
#include "KeyValues.h"
#include "vgui/Cursor.h"
#include "tier1/utldict.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


using namespace vgui;

static char *CopyString( const char *in )
{
	if ( !in )
		return NULL;

	int len = strlen( in );
	char *n = new char[ len + 1 ];
	Q_strncpy( n, in, len  + 1 );
	return n;
}

CKeyBoardEditorPage::SaveMapping_t::SaveMapping_t() : map( 0 )
{
}

CKeyBoardEditorPage::SaveMapping_t::SaveMapping_t( const SaveMapping_t& src )
{
	map = src.map;
	current = src.current;
	original = src.original;
}

//-----------------------------------------------------------------------------
// Purpose: Special list subclass to handle drawing of trap mode prompt on top of
//			lists client area
//-----------------------------------------------------------------------------
class VControlsListPanel : public ListPanel
{
	DECLARE_CLASS_SIMPLE( VControlsListPanel, ListPanel );

public:
	// Construction
					VControlsListPanel( vgui::Panel *parent, const char *listName );
	virtual			~VControlsListPanel();

	// Start/end capturing
	virtual void	StartCaptureMode(vgui::HCursor hCursor = NULL);
	virtual void	EndCaptureMode(vgui::HCursor hCursor = NULL);
	virtual bool	IsCapturing();

	// Set which item should be associated with the prompt
	virtual void	SetItemOfInterest(int itemID);
	virtual int		GetItemOfInterest();

	virtual void	OnMousePressed(vgui::MouseCode code);
	virtual void	OnMouseDoublePressed(vgui::MouseCode code);
	
	KEYBINDING_FUNC( clearbinding, KEY_DELETE, 0, OnClearBinding, 0, 0 );

private:
	void ApplySchemeSettings(vgui::IScheme *pScheme );

	// Are we showing the prompt?
	bool			m_bCaptureMode;
	// If so, where?
	int				m_nClickRow;
	// Font to use for showing the prompt
	vgui::HFont		m_hFont;
	// panel used to edit
	class CInlineEditPanel *m_pInlineEditPanel;
	int m_iMouseX, m_iMouseY;
};

//-----------------------------------------------------------------------------
// Purpose: panel used for inline editing of key bindings
//-----------------------------------------------------------------------------
class CInlineEditPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CInlineEditPanel, vgui::Panel );

public:
	CInlineEditPanel() : vgui::Panel(NULL, "InlineEditPanel")
	{
	}

	virtual void Paint()
	{
		int wide, tall;
		GetSize(wide, tall);

		// Draw a white rectangle around that cell
		vgui::surface()->DrawSetColor( 63, 63, 63, 255 );
		vgui::surface()->DrawFilledRect( 0, 0, wide, tall );

		vgui::surface()->DrawSetColor( 0, 255, 0, 255 );
		vgui::surface()->DrawOutlinedRect( 0, 0, wide, tall );
	}

	virtual void OnKeyCodeTyped(KeyCode code)
	{
		// forward up
		if (GetParent())
		{
			GetParent()->OnKeyCodeTyped(code);
		}
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		Panel::ApplySchemeSettings(pScheme);
		SetBorder(pScheme->GetBorder("DepressedButtonBorder"));
	}

	void OnMousePressed(vgui::MouseCode code)
	{
		// forward up mouse pressed messages to be handled by the key options
		if (GetParent())
		{
			GetParent()->OnMousePressed(code);
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: Construction
//-----------------------------------------------------------------------------
VControlsListPanel::VControlsListPanel( vgui::Panel *parent, const char *listName )	: BaseClass( parent, listName )
{
	m_bCaptureMode	= false;
	m_nClickRow		= 0;
	m_pInlineEditPanel = new CInlineEditPanel();
	m_hFont = INVALID_FONT;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
VControlsListPanel::~VControlsListPanel()
{
	m_pInlineEditPanel->MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VControlsListPanel::ApplySchemeSettings(IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_hFont	= pScheme->GetFont("DefaultVerySmall", IsProportional() ); 
}

//-----------------------------------------------------------------------------
// Purpose: Start capture prompt display
//-----------------------------------------------------------------------------
void VControlsListPanel::StartCaptureMode( HCursor hCursor )
{
	m_bCaptureMode = true;
	EnterEditMode(m_nClickRow, 1, m_pInlineEditPanel);
	input()->SetMouseFocus(m_pInlineEditPanel->GetVPanel());
	input()->SetMouseCapture(m_pInlineEditPanel->GetVPanel());

	if (hCursor)
	{
		m_pInlineEditPanel->SetCursor(hCursor);

		// save off the cursor position so we can restore it
		vgui::input()->GetCursorPos( m_iMouseX, m_iMouseY );
	}
}

void VControlsListPanel::OnClearBinding()
{
	if ( m_bCaptureMode )
		return;

	if ( GetItemOfInterest() < 0 )
		return;

	PostMessage( GetParent()->GetVPanel(), new KeyValues( "ClearBinding", "item", GetItemOfInterest() ) );
}

//-----------------------------------------------------------------------------
// Purpose: Finish capture prompt display
//-----------------------------------------------------------------------------
void VControlsListPanel::EndCaptureMode( HCursor hCursor )
{
	m_bCaptureMode = false;
	input()->SetMouseCapture(NULL);
	LeaveEditMode();
	RequestFocus();
	input()->SetMouseFocus(GetVPanel());
	if (hCursor)
	{
		m_pInlineEditPanel->SetCursor(hCursor);
		surface()->SetCursor(hCursor);	
		if ( hCursor != dc_none )
		{
			vgui::input()->SetCursorPos ( m_iMouseX, m_iMouseY );	
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set active row column
//-----------------------------------------------------------------------------
void VControlsListPanel::SetItemOfInterest(int itemID)
{
	m_nClickRow	= itemID;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieve row, column of interest
//-----------------------------------------------------------------------------
int VControlsListPanel::GetItemOfInterest()
{
	return m_nClickRow;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're currently waiting to capture a key
//-----------------------------------------------------------------------------
bool VControlsListPanel::IsCapturing( void )
{
	return m_bCaptureMode;
}

//-----------------------------------------------------------------------------
// Purpose: Forwards mouse pressed message up to keyboard page when in capture
//-----------------------------------------------------------------------------
void VControlsListPanel::OnMousePressed(vgui::MouseCode code)
{
	if (IsCapturing())
	{
		// forward up mouse pressed messages to be handled by the key options
		if (GetParent())
		{
			GetParent()->OnMousePressed(code);
		}
	}
	else
	{
		BaseClass::OnMousePressed(code);
	}
}


//-----------------------------------------------------------------------------
// Purpose: input handler
//-----------------------------------------------------------------------------
void VControlsListPanel::OnMouseDoublePressed( vgui::MouseCode code )
{
	int c = GetSelectedItemsCount();
	if ( c > 0 )
	{
		// enter capture mode
		OnKeyCodeTyped(KEY_ENTER);
	}
	else
	{
		BaseClass::OnMouseDoublePressed(code);
	}
}

CKeyBoardEditorPage::CKeyBoardEditorPage( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle )
	: BaseClass( parent, "KeyBoardEditorPage" ),
	m_pPanel( panelToEdit ),
	m_Handle( handle )
{
	Assert( m_pPanel );

	m_pList = new VControlsListPanel( this, "KeyBindings" );
	m_pList->SetIgnoreDoubleClick( true );
	m_pList->AddColumnHeader(0, "Action", "#KBEditorBindingName", 175, 0);
	m_pList->AddColumnHeader(1, "Binding", "#KBEditorBinding", 175, 0);
	m_pList->AddColumnHeader(2, "Description", "#KBEditorDescription", 300, 0);

	LoadControlSettings( "resource/KeyBoardEditorPage.res" );

	SaveMappings();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CKeyBoardEditorPage::~CKeyBoardEditorPage()
{
	int c = m_Save.Count();
	for ( int i = 0 ; i  < c; ++i )
	{
		delete m_Save[ i ];
	}
	m_Save.RemoveAll();
}

void CKeyBoardEditorPage::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	PopulateList();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CKeyBoardEditorPage::SaveMappings()
{
	Assert( m_Save.Count() == 0 );

	CUtlVector< PanelKeyBindingMap * > maps;
	GetMappingList( m_pPanel, maps );

	// add header item
	int c = maps.Count();
	for ( int i = 0; i < c; ++i )
	{
		PanelKeyBindingMap *m = maps[ i ];
		SaveMapping_t	*sm = new SaveMapping_t;
		sm->map			= m;
		sm->current = m->boundkeys;
		sm->original = m->boundkeys;
		m_Save.AddToTail( sm );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CKeyBoardEditorPage::UpdateCurrentMappings()
{
	int c = m_Save.Count();
	for ( int i = 0 ; i < c; ++i )
	{
		PanelKeyBindingMap *m = m_Save[ i ]->map;
		Assert( m );
		m_Save[ i ]->current = m->boundkeys;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CKeyBoardEditorPage::RestoreMappings()
{
	int c = m_Save.Count();
	for ( int i = 0; i < c; ++i )
	{
		SaveMapping_t *sm = m_Save[ i ];
		sm->current = sm->original;
	}
}

void CKeyBoardEditorPage::ApplyMappings()
{
	int c = m_Save.Count();
	for ( int i = 0; i < c; ++i )
	{
		SaveMapping_t *sm = m_Save[ i ];
		sm->map->boundkeys = sm->current;
	}
}


//-----------------------------------------------------------------------------
// Purpose: User clicked on item: remember where last active row/column was
//-----------------------------------------------------------------------------
void CKeyBoardEditorPage::ItemSelected()
{
	int c = m_pList->GetSelectedItemsCount();
	if ( c > 0 )
	{
		m_pList->SetItemOfInterest( m_pList->GetSelectedItem( 0 ) );
	}
}

void CKeyBoardEditorPage::BindKey( KeyCode code )
{
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

	int r = m_pList->GetItemOfInterest();

	// Retrieve clicked row and column
	m_pList->EndCaptureMode(dc_arrow);

	// Find item for this row
	KeyValues *item = m_pList->GetItem(r);
	if ( item )
	{
		BoundKey_t *kbMap = reinterpret_cast< BoundKey_t * >( item->GetPtr( "Item", 0 ) );
		if ( kbMap )
		{
			KeyBindingMap_t *binding = m_pPanel->LookupBindingByKeyCode( code, modifiers );
			if ( binding && Q_stricmp( kbMap->bindingname, binding->bindingname ) )
			{
				// Key is already rebound!!!
				Warning( "Can't bind to '%S', key is already bound to '%s'\n",
					Panel::KeyCodeToDisplayString( code ), binding->bindingname );
				return;
			}

			kbMap->keycode		= code;
			kbMap->modifiers	= modifiers; 

			PopulateList();
		}

		KeyBindingMap_t *bindingMap = reinterpret_cast< KeyBindingMap_t * >( item->GetPtr( "Unbound", 0 ) );
		if ( bindingMap )
		{
			KeyBindingMap_t *binding = m_pPanel->LookupBindingByKeyCode( code, modifiers );
			if ( binding && Q_stricmp( bindingMap->bindingname, binding->bindingname ) )
			{
				// Key is already rebound!!!
				Warning( "Can't bind to '%S', key is already bound to '%s'\n",
					Panel::KeyCodeToDisplayString( code ), binding->bindingname );
				return;
			}

			// Need to add to current entries
			m_pPanel->AddKeyBinding( bindingMap->bindingname, code, modifiers );
			UpdateCurrentMappings();
			PopulateList();
		}
	}
}

void CKeyBoardEditorPage::OnPageHide()
{
	if ( m_pList->IsCapturing() )
	{
		// Cancel capturing
		m_pList->EndCaptureMode(dc_arrow);
	}
}

//-----------------------------------------------------------------------------
// Purpose: binds double-clicking or hitting enter in the keybind list to changing the key
//-----------------------------------------------------------------------------
void CKeyBoardEditorPage::OnKeyCodeTyped(vgui::KeyCode code)
{
	switch ( code )
	{
	case KEY_ENTER:
        {
			if ( !m_pList->IsCapturing() )
			{
                OnCommand( "ChangeKey" );
			}
			else
			{
				BindKey( code );
			}
		}
		break;
	case KEY_LSHIFT:
	case KEY_RSHIFT:
	case KEY_LALT:
	case KEY_RALT:
	case KEY_LCONTROL:
	case KEY_RCONTROL:
		{
			// Swallow these
			break;
		}
		break;
	default:
		{
			if ( m_pList->IsCapturing() )
			{
				BindKey( code );
			}
			else
			{
				BaseClass::OnKeyCodeTyped(code);
			}
		}
	}
}

void CKeyBoardEditorPage::OnCommand( char const *cmd )
{
	if ( !m_pList->IsCapturing() && !Q_stricmp( cmd, "ChangeKey" ) )
	{
		m_pList->StartCaptureMode(dc_blank);
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

void CKeyBoardEditorPage::OnSaveChanges()
{
	ApplyMappings();
}

void CKeyBoardEditorPage::OnRevert()
{
	RestoreMappings();
	PopulateList();
}

void CKeyBoardEditorPage::OnUseDefaults()
{
	m_pPanel->RevertKeyBindingsToDefault();
	UpdateCurrentMappings();
	PopulateList();
}

void CKeyBoardEditorPage::GetMappingList( Panel *panel, CUtlVector< PanelKeyBindingMap * >& maps )
{
	PanelKeyBindingMap *map = panel->GetKBMap();
	while ( map )
	{
		maps.AddToTail( map );
		map = map->baseMap;
	}
}

static bool BindingLessFunc( KeyValues * const & lhs, KeyValues * const &rhs )
{
	KeyValues *p1, *p2;

	p1 = const_cast< KeyValues * >( lhs );
	p2 = const_cast< KeyValues * >( rhs );
	return ( Q_stricmp( p1->GetString( "Action" ), p2->GetString( "Action" ) ) < 0 ) ? true : false;
}

void CKeyBoardEditorPage::AnsiText( char const *token, char *out, size_t buflen )
{
	out[ 0 ] = 0;

	wchar_t *str = g_pVGuiLocalize->Find( token );
	if ( !str )
	{
		Q_strncpy( out, token, buflen );
	}
	else
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI( str, out, buflen );
	}
}

void CKeyBoardEditorPage::PopulateList()
{
	m_pList->DeleteAllItems();

	int i, j;

	CUtlRBTree< KeyValues *, int >	sorted( 0, 0, BindingLessFunc );

	// add header item
	int c = m_Save.Count();
	for ( i = 0; i < c; ++i )
	{
		SaveMapping_t* sm = m_Save[ i ];

		PanelKeyBindingMap *m = sm->map;
		Assert( m );

		int bindings = sm->current.Count();
		for ( j = 0; j < bindings; ++j )
		{
			BoundKey_t *kbMap = &sm->current[ j ];
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
			KeyBindingMap_t *bindingMap = m_pPanel->LookupBinding( kbMap->bindingname );
			if ( bindingMap && 
				 bindingMap->helpstring )
			{
				AnsiText( bindingMap->helpstring, ansi, sizeof( ansi ) );
				item->SetString( "Description", ansi);
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
			m_pPanel->LookupBoundKeys( kbMap->bindingname, list );
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

void CKeyBoardEditorPage::OnClearBinding( int item )
{
	// Find item for this row
	KeyValues *kv = m_pList->GetItem(item );
	if ( !kv )
	{
		return;
	}

	BoundKey_t *kbMap = reinterpret_cast< BoundKey_t * >( kv->GetPtr( "Item", 0 ) );
	if ( !kbMap )
	{
		return;
	}

	kbMap->keycode		= KEY_NONE;
	kbMap->modifiers	= 0; 

	PopulateList();
}

CKeyBoardEditorSheet::CKeyBoardEditorSheet( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle )
	: BaseClass( parent, "KeyBoardEditorSheet" ),
	m_bSaveToExternalFile( false ),
	m_Handle( handle ),
	m_SaveFileName( UTL_INVAL_SYMBOL ),
	m_SaveFilePathID( UTL_INVAL_SYMBOL )
{
	m_hPanel = panelToEdit;

	SetSmallTabs( true );

	// Create this sheet and add the subcontrols
	CKeyBoardEditorPage *active = NULL;

	int subCount = Panel::GetPanelsWithKeyBindingsCount( handle );
	for ( int i = 0; i < subCount; ++i )
	{
		Panel *p = Panel::GetPanelWithKeyBindings( handle, i );
		if ( !p )
			continue;

		// Don't display panels with no keymappings
		if ( p->GetKeyMappingCount() == 0 )
			continue;

        CKeyBoardEditorPage *newPage = new CKeyBoardEditorPage( this, p, handle );
		AddPage( newPage, p->GetName() );
		if ( p == panelToEdit )
		{
			active = newPage;
		}
	}

	if ( active )
	{
		SetActivePage( active );
	}

	LoadControlSettings( "resource/KeyBoardEditorSheet.res" );
}

void CKeyBoardEditorSheet::SetKeybindingsSaveFile( char const *filename, char const *pathID /*= 0*/ )
{
	Assert( filename );
	m_bSaveToExternalFile = true;
	m_SaveFileName = filename;
	if ( pathID != NULL )
	{
		m_SaveFilePathID = pathID;
	}
	else
	{
		m_SaveFilePathID = UTL_INVAL_SYMBOL;
	}
}

void CKeyBoardEditorSheet::OnSaveChanges()
{
	int c = GetNumPages();
	for ( int i = 0 ; i < c; ++i )
	{
		CKeyBoardEditorPage *page = static_cast< CKeyBoardEditorPage * >( GetPage( i ) );
		page->OnSaveChanges();
	}

	if ( m_bSaveToExternalFile )
	{
		m_hPanel->SaveKeyBindingsToFile( m_Handle, m_SaveFileName.String(), m_SaveFilePathID.IsValid() ? m_SaveFilePathID.String() : NULL );
	}
	else
	{
		m_hPanel->SaveKeyBindings( m_Handle );
	}
}

void CKeyBoardEditorSheet::OnRevert()
{
	int c = GetNumPages();
	for ( int i = 0 ; i < c; ++i )
	{
		CKeyBoardEditorPage *page = static_cast< CKeyBoardEditorPage * >( GetPage( i ) );
		page->OnRevert();
	}
}

void CKeyBoardEditorSheet::OnUseDefaults()
{
	int c = GetNumPages();
	for ( int i = 0 ; i < c; ++i )
	{
		CKeyBoardEditorPage *page = static_cast< CKeyBoardEditorPage * >( GetPage( i ) );
		page->OnUseDefaults();
	}
}

CKeyBoardEditorDialog::CKeyBoardEditorDialog( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle )
	: BaseClass( parent, "KeyBoardEditorDialog" )
{
	m_pSave = new Button( this, "Save", "#KBEditorSave", this, "save" );
	m_pCancel = new Button( this, "Cancel", "#KBEditorCancel", this, "cancel" );
	m_pRevert = new Button( this, "Revert", "#KBEditorRevert", this, "revert" );
	m_pUseDefaults = new Button( this, "Defaults", "#KBEditorUseDefaults", this, "defaults" );

	m_pKBEditor = new CKeyBoardEditorSheet( this, panelToEdit, handle );

	LoadControlSettings( "resource/KeyBoardEditorDialog.res" );

	SetTitle( "#KBEditorTitle", true );

	SetSmallCaption( true );
	SetMinimumSize( 640, 200 );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetSizeable( true );
	SetMoveable( true );
	SetMenuButtonVisible( false );

	SetVisible( true );

	MoveToCenterOfScreen();
}

void CKeyBoardEditorDialog::OnCommand( char const *cmd )
{
	if ( !Q_stricmp( cmd, "save" ) )
	{
		m_pKBEditor->OnSaveChanges();
		MarkForDeletion();
	}
	else if ( !Q_stricmp( cmd, "cancel" ) ||
		      !Q_stricmp( cmd, "Close" ) )
	{
		m_pKBEditor->OnRevert();
		MarkForDeletion();
	}
	else if ( !Q_stricmp( cmd, "revert" ) )
	{
		m_pKBEditor->OnRevert();
	}
	else if ( !Q_stricmp( cmd, "defaults" ) )
	{
		m_pKBEditor->OnUseDefaults();
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}

void CKeyBoardEditorDialog::SetKeybindingsSaveFile( char const *filename, char const *pathID /*= 0*/ )
{
	m_pKBEditor->SetKeybindingsSaveFile( filename, pathID );
}
