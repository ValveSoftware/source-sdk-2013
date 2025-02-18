//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "strange_count_transfer_panel.h"
#include "cdll_client_int.h"
#include "ienginevgui.h"
#include "econ_item_tools.h"
#include "econ_ui.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStatModuleItemSelectionPanel : public CItemCriteriaSelectionPanel
{
	DECLARE_CLASS_SIMPLE( CStatModuleItemSelectionPanel, CItemCriteriaSelectionPanel );
public:
	CStatModuleItemSelectionPanel( Panel *pParent, const CEconItemView* pCorrespondingItem ) 
		: BaseClass( pParent, NULL )
		, m_pCorrespondingItem( pCorrespondingItem )
		, m_mapXifierClassCount( CaselessStringLessThan )
	{
		int nCount = InventoryManager()->GetLocalInventory()->GetItemCount();
		for( int i=0; i<nCount; ++i )
		{
			if ( !BIsItemStrange( InventoryManager()->GetLocalInventory()->GetItem( i ) ) )
				continue;

			const char *pItemXifier = InventoryManager()->GetLocalInventory()->GetItem( i )->GetItemDefinition()->GetXifierRemapClass();
			auto idx = m_mapXifierClassCount.Find( pItemXifier );
			if ( idx == m_mapXifierClassCount.InvalidIndex() )
			{
				idx = m_mapXifierClassCount.Insert( pItemXifier, 0 );
			}

			m_mapXifierClassCount[ idx ] = m_mapXifierClassCount[ idx ] + 1;
		}
	}

	void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		vgui::Label* pWeaponLabel = dynamic_cast<vgui::Label*>( FindChildByName("ItemSlotLabel") );
		if ( pWeaponLabel )
		{
			pWeaponLabel->SetVisible( false );
		}

	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	const char *GetItemNotSelectableReason( const CEconItemView *pItem ) const
	{
		if ( !pItem )
			return NULL;

		if ( !BIsItemStrange( pItem ) )
			return "#TF_StrangeCount_Transfer_NotStrange";

		if ( m_pCorrespondingItem )
		{
			if ( pItem->GetItemID() == m_pCorrespondingItem->GetItemID() )
				return "#TF_StrangeCount_Transfer_Self";

			if ( !CEconTool_StrangeCountTransfer::AreItemsEligibleForStrangeCountTransfer( m_pCorrespondingItem, pItem ) )
				return "#TF_StrangeCount_Transfer_TypeMismatch";
		}

		const char *pItemXifier = pItem->GetItemDefinition()->GetXifierRemapClass();
		auto idx = m_mapXifierClassCount.Find( pItemXifier );
		int nCount = 0;
		if ( !pItemXifier )
		{
			// if no xifier, find atleast 1 other matching item
			CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
			if ( pInventory )
			{
				for ( int i = 0; i < pInventory->GetItemCount(); i++ )
				{
					CEconItemView *pIterItem = pInventory->GetItem( i );
					if ( pIterItem->GetItemDefIndex() == pItem->GetItemDefIndex() && BIsItemStrange(pIterItem) )
					{
						// find 2 or more, yourself and another
						if ( ++nCount >= 2 )
							return NULL;
					}
				}
			}
		}
		else if ( idx != m_mapXifierClassCount.InvalidIndex() )
		{
			nCount = m_mapXifierClassCount[ idx ];
		}

		if ( nCount < 2 )
		{
			return "#TF_StrangeCount_Transfer_NotEnoughMatches";
		}

		return NULL;
	}

protected:
	const char * m_pszTitleToken;
	const CEconItemView* m_pCorrespondingItem;
	CUtlMap< const char*, int > m_mapXifierClassCount;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStrangeCountTransferPanel::CStrangeCountTransferPanel( vgui::Panel *parent, CEconItemView* pToolItem ) 
	: BaseClass( parent, "StrangeCountTrasnferDialog" )
	, m_pToolItem( pToolItem )
{
	Assert( pToolItem );

	ListenForGameEvent( "gameui_hidden" );

	m_hSelectionPanel = 0;
	m_pSelectingItemModelPanel = NULL;

	EditablePanel* pBG = new EditablePanel( this, "BG" );

	m_pSourceStrangeModelPanel = new CItemModelPanel( pBG, "SourceItem" );
	m_pSourceStrangeModelPanel->SetActAsButton( true, true );
	m_pTargetStrangeModelPanel = new CItemModelPanel( pBG, "TargetItem" );
	m_pTargetStrangeModelPanel->SetActAsButton( true, true );
	m_pOKButton = new CExButton( pBG, "OkButton", "" );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( scheme );
	SetProportional( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStrangeCountTransferPanel::~CStrangeCountTransferPanel( void )
{
	if ( m_hSelectionPanel )
	{
		m_hSelectionPanel->MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStrangeCountTransferPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFile() );
}

void CStrangeCountTransferPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	UpdateOKButton();

	m_pSourceStrangeModelPanel->SetTooltip( EconUI()->GetBackpackPanel()->GetMouseOverToolTipPanel(), "" );
	m_pTargetStrangeModelPanel->SetTooltip( EconUI()->GetBackpackPanel()->GetMouseOverToolTipPanel(), "" );
}

void CStrangeCountTransferPanel::OnCommand( const char *command )
{
	if( FStrEq( "apply", command ) )
	{
		GCSDK::CProtoBufMsg<CMsgApplyStrangeCountTransfer> msg( k_EMsgGCApplyStrangeCountTransfer );

		if ( !m_pToolItem || !m_pSourceStrangeModelPanel->GetItem() || !m_pTargetStrangeModelPanel->GetItem() )
			return;

		msg.Body().set_tool_item_id( m_pToolItem->GetItemID() );
		msg.Body().set_item_src_item_id( m_pSourceStrangeModelPanel->GetItem()->GetItemID() );
		msg.Body().set_item_dest_item_id( m_pTargetStrangeModelPanel->GetItem()->GetItemID() );
		GCClientSystem()->BSendMessage( msg );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolItem, "applied_strangecounttransfer", m_pToolItem->GetItemDefIndex() );

		GCClientSystem()->BSendMessage( msg );

		SetVisible( false );
		MarkForDeletion();

		return;
	}
	else if ( FStrEq( "cancel", command ) )
	{
		MarkForDeletion();
		return;
	}

	BaseClass::OnCommand( command );
}

void CStrangeCountTransferPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "gameui_hidden" ) )
	{
		SetVisible( false );
		MarkForDeletion();
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStrangeCountTransferPanel::OnItemPanelMousePressed( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() && !pItemPanel->IsGreyedOut() )
	{
		m_pSelectingItemModelPanel = pItemPanel;

		CEconItemView* pOtherItem = pItemPanel == m_pSourceStrangeModelPanel ? m_pTargetStrangeModelPanel->GetItem()
																		 : m_pSourceStrangeModelPanel->GetItem();

		m_hSelectionPanel = new CStatModuleItemSelectionPanel( GetParent(), pOtherItem );

		// Clicked on an item in the crafting area. Open up the selection panel.
		m_hSelectionPanel->ShowDuplicateCounts( false );
		m_hSelectionPanel->ShowPanel( 0, true );
		m_hSelectionPanel->SetCaller( this );
		m_hSelectionPanel->SetZPos( GetZPos() + 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStrangeCountTransferPanel::OnSelectionReturned( KeyValues *data )
{
	Assert( m_pSelectingItemModelPanel );

	if ( data && m_pSelectingItemModelPanel )
	{
		uint64 ulIndex = data->GetUint64( "itemindex", INVALID_ITEM_ID );

		CEconItemView* pSelectedItem = InventoryManager()->GetLocalInventory()->GetInventoryItemByItemID( ulIndex );
		m_pSelectingItemModelPanel->SetItem( pSelectedItem );
	}

	UpdateOKButton();

	m_pSelectingItemModelPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStrangeCountTransferPanel::UpdateOKButton()
{
	bool bOKEnabled = m_pSourceStrangeModelPanel->GetItem() && m_pTargetStrangeModelPanel->GetItem();
	m_pOKButton->SetEnabled( bOKEnabled );
}