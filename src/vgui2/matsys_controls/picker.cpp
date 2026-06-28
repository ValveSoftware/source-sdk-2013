//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "filesystem.h"
#include "matsys_controls/picker.h"
#include "tier1/KeyValues.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/Button.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"


using namespace vgui;


//-----------------------------------------------------------------------------
//
// Base asset Picker
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Sort by asset name
//-----------------------------------------------------------------------------
static int __cdecl PickerBrowserSortFunc( vgui::ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("choice");
	const char *string2 = item2.kv->GetString("choice");
	return stricmp( string1, string2 );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPicker::CPicker( vgui::Panel *pParent, const char *pColumnHeader, const char *pTextType ) : 
	BaseClass( pParent, "Picker" )
{
	m_pPickerType = pColumnHeader;
	m_pPickerTextType = pTextType;

	// FIXME: Make this an image browser
	m_pPickerBrowser = new vgui::ListPanel( this, "Browser" );
 	m_pPickerBrowser->AddColumnHeader( 0, "choice", m_pPickerType, 52, 0 );
    m_pPickerBrowser->SetSelectIndividualCells( true );
	m_pPickerBrowser->SetEmptyListText( "Nothing to pick" );
 	m_pPickerBrowser->SetDragEnabled( true );
 	m_pPickerBrowser->AddActionSignalTarget( this );
	m_pPickerBrowser->SetSortFunc( 0, PickerBrowserSortFunc );
	m_pPickerBrowser->SetSortColumn( 0 );
						 
	// filter selection
	m_pFilterList = new TextEntry( this, "FilterList" );
	m_pFilterList->AddActionSignalTarget( this );
	m_pFilterList->RequestFocus();

	LoadControlSettingsAndUserConfig( "resource/picker.res" );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CPicker::~CPicker()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPicker::OnKeyCodePressed( KeyCode code )
{
	if (( code == KEY_UP ) || ( code == KEY_DOWN ) || ( code == KEY_PAGEUP ) || ( code == KEY_PAGEDOWN ))
	{
		KeyValues *pMsg = new KeyValues("KeyCodePressed", "code", code);
		vgui::ipanel()->SendMessage( m_pPickerBrowser->GetVPanel(), pMsg, GetVPanel());
		pMsg->deleteThis();
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: refreshes the asset list
//-----------------------------------------------------------------------------
void CPicker::SetStringList( const PickerList_t &list ) 
{
	m_Type = list.m_Type;
	m_pPickerBrowser->RemoveAll();	

	int nCount = list.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		const char *pPickerName = list[i].m_pChoiceString;
		KeyValues *kv = new KeyValues( "node", "choice", pPickerName );
		if ( m_Type == PICKER_CHOICE_STRING )
		{
			kv->SetString( "value", list[i].m_pChoiceValue ); 
		}
		else
		{
			kv->SetPtr( "value", list[i].m_pChoiceValuePtr ); 
		}
		int nItemID = m_pPickerBrowser->AddItem( kv, 0, false, false );

		if ( m_Type == PICKER_CHOICE_STRING )
		{
			KeyValues *pDrag = new KeyValues( "drag", "text", list[i].m_pChoiceValue );
			if ( m_pPickerTextType )
			{
				pDrag->SetString( "texttype", m_pPickerTextType );
			}
			m_pPickerBrowser->SetItemDragData( nItemID, pDrag );
		}
	}
	RefreshChoiceList();
}


//-----------------------------------------------------------------------------
// Purpose: refreshes the choice list
//-----------------------------------------------------------------------------
void CPicker::RefreshChoiceList( ) 
{
	// Check the filter matches
	int nMatchingCount = 0;
	int nTotalCount = 0;
	for ( int nItemID = m_pPickerBrowser->FirstItem(); nItemID != m_pPickerBrowser->InvalidItemID(); nItemID = m_pPickerBrowser->NextItem( nItemID ) )
	{
		KeyValues *kv = m_pPickerBrowser->GetItem( nItemID );
		const char *pPickerName = kv->GetString( "choice" );
		bool bVisible = !m_Filter.Length() || Q_stristr( pPickerName, m_Filter.Get() );
		m_pPickerBrowser->SetItemVisible( nItemID, bVisible );
		if ( bVisible )
		{
			++nMatchingCount;
		}
		++nTotalCount;
	}

	char pColumnTitle[512];
	Q_snprintf( pColumnTitle, sizeof(pColumnTitle), "%s (%d/%d)",
		m_pPickerType, nMatchingCount, nTotalCount );
	m_pPickerBrowser->SetColumnHeaderText( 0, pColumnTitle );

	m_pPickerBrowser->SortList();
	if ( ( m_pPickerBrowser->GetSelectedItemsCount() == 0 ) && ( m_pPickerBrowser->GetItemCount() > 0 ) )
	{
		int nItemID = m_pPickerBrowser->GetItemIDFromRow( 0 );
		m_pPickerBrowser->SetSelectedCell( nItemID, 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: refreshes dialog on text changing
//-----------------------------------------------------------------------------
void CPicker::OnTextChanged( )
{
	int nLength = m_pFilterList->GetTextLength();
	m_Filter.SetLength( nLength );
	if ( nLength > 0 )
	{
		m_pFilterList->GetText( m_Filter.GetForModify(), nLength+1 );
	}
	RefreshChoiceList();
}


//-----------------------------------------------------------------------------
// Returns the selected string
//-----------------------------------------------------------------------------
PickerChoiceType_t CPicker::GetSelectionType() const
{
	return m_Type;
}

const char *CPicker::GetSelectedString( ) const
{
	if ( m_pPickerBrowser->GetSelectedItemsCount() == 0 )
		return NULL;

	if ( m_Type != PICKER_CHOICE_STRING )
		return NULL;

	int nIndex = m_pPickerBrowser->GetSelectedItem( 0 );
	KeyValues *pItemKeyValues = m_pPickerBrowser->GetItem( nIndex );
	return pItemKeyValues->GetString( "value" );
}

void *CPicker::GetSelectedPtr( ) const
{
	if ( m_pPickerBrowser->GetSelectedItemsCount() == 0 )
		return NULL;

	if ( m_Type != PICKER_CHOICE_PTR )
		return NULL;

	int nIndex = m_pPickerBrowser->GetSelectedItem( 0 );
	KeyValues *pItemKeyValues = m_pPickerBrowser->GetItem( nIndex );
	return pItemKeyValues->GetPtr( "value" );
}


//-----------------------------------------------------------------------------
// Returns the index of the selected string
//-----------------------------------------------------------------------------
int CPicker::GetSelectedIndex()
{
	if ( m_pPickerBrowser->GetSelectedItemsCount() == 0 )
		return -1;

	return m_pPickerBrowser->GetSelectedItem( 0 );
}

	
//-----------------------------------------------------------------------------
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CPickerFrame::CPickerFrame( vgui::Panel *pParent, const char *pTitle, const char *pPickerType, const char *pTextType ) : 
	BaseClass( pParent, "PickerFrame" )
{
	m_pContextKeyValues = NULL;
	SetDeleteSelfOnClose( true );
	m_pPicker = new CPicker( this, pPickerType, pTextType );
	m_pPicker->AddActionSignalTarget( this );
	m_pOpenButton = new Button( this, "OpenButton", "#FileOpenDialog_Open", this, "Open" );
	m_pCancelButton = new Button( this, "CancelButton", "#FileOpenDialog_Cancel", this, "Cancel" );
	SetBlockDragChaining( true );

	LoadControlSettingsAndUserConfig( "resource/pickerframe.res" );

	SetTitle( pTitle, false );
}

CPickerFrame::~CPickerFrame()
{
	CleanUpMessage();
}


//-----------------------------------------------------------------------------
// Deletes the message
//-----------------------------------------------------------------------------
void CPickerFrame::CleanUpMessage()
{
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Activate the dialog
//-----------------------------------------------------------------------------
void CPickerFrame::DoModal( const PickerList_t &list, KeyValues *pContextKeyValues )
{
	CleanUpMessage();
	m_pContextKeyValues = pContextKeyValues;
 	m_pPicker->SetStringList( list );
	BaseClass::DoModal();
}


//-----------------------------------------------------------------------------
// On command
//-----------------------------------------------------------------------------
void CPickerFrame::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "Open" ) )
	{
		KeyValues *pActionKeys = new KeyValues( "Picked" );
		pActionKeys->SetInt( "choiceIndex", m_pPicker->GetSelectedIndex( ) );

		if ( m_pPicker->GetSelectionType() == PICKER_CHOICE_STRING )
		{
			const char *pPickerName = m_pPicker->GetSelectedString( );
			pActionKeys->SetString( "choice", pPickerName );
		}
		else
		{
			void *pPickerPtr = m_pPicker->GetSelectedPtr( );
			pActionKeys->SetPtr( "choice", pPickerPtr );
		}

		if ( m_pContextKeyValues )
		{
			pActionKeys->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		PostActionSignal( pActionKeys );
		CloseModal();
		return;
	}

	if ( !Q_stricmp( pCommand, "Cancel" ) )
	{
		CloseModal();
		return;
	}

	BaseClass::OnCommand( pCommand );
}

	
