//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "vgui//ILocalize.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "econ_item_tools.h"
#include "tool_items.h"
#include "rename_tool_ui.h"
#include "econ_ui.h"
#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

CNameToolUsageDialog::CNameToolUsageDialog( vgui::Panel *pParent, const char* pszName, CEconItemView *pTool, CEconItemView *pToolSubject, bool bDescription )
: 	CBaseToolUsageDialog( pParent, pszName, pTool, pToolSubject )
{
	m_bDescription = bDescription;
}

int CNameToolUsageDialog::GetMaxLength()
{
	if ( m_bDescription )
		return MAX_ITEM_CUSTOM_DESC_LENGTH;
	else
		return MAX_ITEM_CUSTOM_NAME_LENGTH;
}

int CNameToolUsageDialog::GetMaxDBSize()
{
	if ( m_bDescription )
		return MAX_ITEM_CUSTOM_DESC_DATABASE_SIZE;
	else
		return MAX_ITEM_CUSTOM_NAME_DATABASE_SIZE;
}

void CEconTool_NameTag::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CRequestNameDialog *dialog = vgui::SETUP_PANEL( new CRequestNameDialog( pParent, "ItemRenameDialog", pTool, pSubject, false ) );
	MakeModalAndBringToFront( dialog );
}

//-----------------------------------------------------------------------------
CRequestNameDialog::CRequestNameDialog( vgui::Panel *parent, const char* pszName, CEconItemView *pTool, CEconItemView *pToolSubject, bool bDescription ) : 
	CNameToolUsageDialog( parent, pszName, pTool, pToolSubject, bDescription )
{
	m_pCustomNameEntry = new vgui::TextEntry( this, "CustomNameEntry" );
	m_bDescription = bDescription;
}

//-----------------------------------------------------------------------------
void CRequestNameDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/ItemRenameDialog.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pOldNameLabel = dynamic_cast<vgui::Label *>( FindChildByName( "OldItemNameDescLabel" ) );
	if ( m_pOldNameLabel )
	{
		if ( m_bDescription )
			m_pOldNameLabel->SetText( g_pVGuiLocalize->Find( "#ToolItemRenameOldItemDesc" ) );
		else
			m_pOldNameLabel->SetText( g_pVGuiLocalize->Find( "#ToolItemRenameOldItemName" ) );
	}

	m_pNewNameLabel = dynamic_cast<vgui::Label *>( FindChildByName( "NewItemNameDescLabel" ) );
	if ( m_pNewNameLabel )
	{
		if ( m_bDescription )
			m_pNewNameLabel->SetText( g_pVGuiLocalize->Find( "#ToolItemRenameNewItemDesc" ) );
		else
			m_pNewNameLabel->SetText( g_pVGuiLocalize->Find( "#ToolItemRenameNewItemName" ) );
	}

	m_pOldName = dynamic_cast<vgui::Label *>( FindChildByName( "OldItemNameLabel" ) );
	if ( m_pOldName )
	{
		if ( m_bDescription )
		{
			CEconItem *pSOCData = m_pSubjectModelPanel->GetItem()->GetSOCData();
			if ( pSOCData && pSOCData->GetCustomDesc() )
			{
				m_pOldName->SetText( pSOCData->GetCustomDesc() );
			}
			else
			{
				const char *pszItemDesc = m_pSubjectModelPanel->GetItem()->GetStaticData()->GetItemDesc();
				if ( pszItemDesc && ( pszItemDesc[0] == '#' ) )
				{
					// don't show localization strings for item descriptions
					pszItemDesc = "";
				}
				m_pOldName->SetText( pszItemDesc );
			}
		}
		else
		{
			CEconItem *pSOCData = m_pSubjectModelPanel->GetItem()->GetSOCData();
			if ( pSOCData && pSOCData->GetCustomName() )
				m_pOldName->SetText( pSOCData->GetCustomName() );
			else
				m_pOldName->SetText( m_pSubjectModelPanel->GetItem()->GetStaticData()->GetItemBaseName() );
		}
	}

	CExButton *pOKButton = dynamic_cast< CExButton* >( FindChildByName( "OkButton" ) );
	if ( pOKButton )
	{
		if ( m_bDescription )
			pOKButton->SetText( "#CraftDescribeOk" );
	}

	m_pCustomNameEntry->SetMaximumCharCount( GetMaxLength() );
	m_pCustomNameEntry->SetAllowNonAsciiCharacters( true );
}


//-----------------------------------------------------------------------------
void	CRequestNameDialog::MoveToFront()
{
	BaseClass::MoveToFront();

	// do this after MoveToFront so we can force the text box to have focus instead 
	// of the dialog itself
	m_pCustomNameEntry->RequestFocus();
}


//-----------------------------------------------------------------------------
void CRequestNameDialog::Apply( void )
{
	const int maxNameLength = MAX_ITEM_CUSTOM_DESC_LENGTH + 1;
	wchar_t inputName[ maxNameLength ];

	m_pCustomNameEntry->GetText( inputName, sizeof(inputName) );

	// pop up modal confirmation dialog
	CConfirmNameDialog *dialog = vgui::SETUP_PANEL( new CConfirmNameDialog( GetParent(), "ItemRenameConfirmationDialog", m_pToolModelPanel->GetItem(), m_pSubjectModelPanel->GetItem(), inputName, m_bDescription ) );
	MakeModalAndBringToFront( dialog );
}


//-----------------------------------------------------------------------------
// Purpose: Gives focus back to the name entry field after the mouse enters a
//			item model panel
//-----------------------------------------------------------------------------
void CRequestNameDialog::OnItemPanelEntered( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		// The item panel is going to try and steal our focus.  Steal it back!
		m_pCustomNameEntry->RequestFocus();
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CConfirmNameDialog::CConfirmNameDialog( vgui::Panel *parent, const char* pszName, CEconItemView *pTool, CEconItemView *pToolSubject, const wchar_t *name, bool bDescription ) : 
	CNameToolUsageDialog( parent, pszName, pTool, pToolSubject, bDescription )
{
	Q_wcsncpy( m_name, name, sizeof(m_name) ); 
	m_bDescription = bDescription;
}


//-----------------------------------------------------------------------------
//
// We're going to want to flesh this out to trim off leading/training spaces, etc
//
bool CConfirmNameDialog::IsNameValid( void ) const
{
	// legal names are 1 or more alphanumeric values (only)
	const wchar_t *c = m_name;
	int length = 0;
	while( *c )
	{
		// no leading spaces
		if ( length == 0 && *c == ' ' )
			return false;

		++c;
		++length;
	}

	// no trailing spaces
	if ( length > 0 && m_name[length-1] == ' ' )
		return false;

	return (length > 0);
}


//-----------------------------------------------------------------------------
void CConfirmNameDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	if ( !IsNameValid() )
	{
		// pop up bad name dialog
		LoadControlSettings( "resource/UI/ItemRenameInvalidDialog.res" );
	}
	else
	{
		// pop up "are you sure" dialog
		LoadControlSettings( "resource/UI/ItemRenameConfirmationDialog.res" );
	}

	// Set our dialog name, but pre & post pend it with quotes
	wchar_t tmpname[ MAX_ITEM_CUSTOM_DESC_LENGTH+3 ];
	V_wcscpy_safe( tmpname, L"\"" );
	V_wcscat_safe( tmpname, m_name );
	V_wcscat_safe( tmpname, L"\"" );
	SetDialogVariable( "name", tmpname );

	BaseClass::ApplySchemeSettings( pScheme );
}


//-----------------------------------------------------------------------------
void CConfirmNameDialog::Apply( void )
{
	// the GC stores 8-bit chars, so convert Unicode name to UTF8
	char* utf8Name = new char[ GetMaxDBSize() ];
	int count = V_UnicodeToUTF8( m_name, utf8Name, GetMaxDBSize() );

	if ( count > GetMaxDBSize() )
	{
		// the encoded name exceeds the GC's storage limit
		return;
	}

	if ( m_pSubjectModelPanel->GetItem()->GetItemID() != INVALID_ITEM_ID )
	{
		// Name has been confirmed - send message to GC to apply name to item
		GCSDK::CGCMsg< MsgGCNameItem_t > msg( k_EMsgGCNameItem );

		msg.Body().m_unToolItemID = m_pToolModelPanel->GetItem()->GetItemID();
		msg.Body().m_unSubjectItemID = m_pSubjectModelPanel->GetItem()->GetItemID();
		msg.AddStrData( utf8Name );
		GCClientSystem()->BSendMessage( msg );
	}
	else
	{
		// Name has been confirmed - send message to GC to apply name to item
		GCSDK::CGCMsg< MsgGCNameBaseItem_t > msg( k_EMsgGCNameBaseItem );

		msg.Body().m_unToolItemID = m_pToolModelPanel->GetItem()->GetItemID();
		msg.Body().m_unBaseItemDefinitionID = m_pSubjectModelPanel->GetItem()->GetStaticData()->GetDefinitionIndex();
		msg.AddStrData( utf8Name );
		GCClientSystem()->BSendMessage( msg );
	}

	if ( m_bDescription )
		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "redescription_item" );
	else
		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "renamed_item" );

	delete []utf8Name;
}

//-----------------------------------------------------------------------------
void CConfirmNameDialog::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );

	if ( !Q_stricmp( command, "backfrominvalid" ) )
	{
		// Re-open the name dialog
		CRequestNameDialog *dialog = vgui::SETUP_PANEL( new CRequestNameDialog( GetParent(), "ItemRenameDialog", m_pToolModelPanel->GetItem(), m_pSubjectModelPanel->GetItem(), m_bDescription ) );
		MakeModalAndBringToFront( dialog );
	}
}


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the name base item response
//-----------------------------------------------------------------------------
class CGCNameBaseItemResponse : public GCSDK::CGCClientJob
{
public:
	CGCNameBaseItemResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );
		InventoryManager()->ShowItemsPickedUp( true );
		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCNameBaseItemResponse, "CGCNameBaseItemResponse", k_EMsgGCNameBaseItemResponse, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
// Purpose: UI Hook for applying a new description to items.
//-----------------------------------------------------------------------------
void CEconTool_DescTag::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CRequestNameDialog *dialog = vgui::SETUP_PANEL( new CRequestNameDialog( pParent, "ItemRenameDialog", pTool, pSubject, true ) );
	MakeModalAndBringToFront( dialog );
}