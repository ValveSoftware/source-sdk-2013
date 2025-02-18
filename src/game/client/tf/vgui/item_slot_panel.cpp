//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "item_slot_panel.h"
#include "tf_item_inventory.h"
#include "item_selection_panel.h"
#include "tf_gcmessages.h"
#include "gc_clientsystem.h"

#define NUM_MAX_SLOTS	1

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemSlotPanel::CItemSlotPanel( vgui::Panel *parent )
	: CBaseLoadoutPanel( parent, "item_slot_panel" )
{
	m_pItem = NULL;
	m_pSelectionPanel = NULL;
	m_iCurrentSlotIndex = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemSlotPanel::~CItemSlotPanel()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/ItemSlotPanel.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		if ( !m_itemSlots[i].m_bHasSlot )
		{
			m_pItemModelPanels[i]->SetVisible( false );
			continue;
		}

		int iCenter = GetWide() * 0.5;
		int iButtonX = (i % GetNumColumns());
		int iButtonY = (i / GetNumColumns());
		int iXPos = (iCenter + m_iItemBackpackOffcenterX) + (iButtonX * m_pItemModelPanels[i]->GetWide()) + (m_iItemBackpackXDelta * iButtonX);
		int iYPos = m_iItemYPos + (iButtonY * m_pItemModelPanels[i]->GetTall() ) + (m_iItemBackpackYDelta * iButtonY);

		m_pItemModelPanels[i]->SetPos( iXPos, iYPos );
		m_pItemModelPanels[i]->SetVisible( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
		{
			if ( m_pItemModelPanels[i] == pItemPanel  )
			{
				OnCommand( VarArgs("change%d", i) );
				return;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::OnSelectionReturned( KeyValues *data )
{
	if ( data )
	{
		uint64 ulIndex = data->GetUint64( "itemindex", INVALID_ITEM_ID );
		if ( ulIndex != INVALID_ITEM_ID )
		{
			CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByItemID( ulIndex );
			if ( pItemData )
			{
				m_pItemModelPanels[ m_iCurrentSlotIndex ]->SetItem( pItemData );

				itemid_t ulOriginalID = pItemData->GetSOCData()->GetOriginalID();

				m_itemSlots[ m_iCurrentSlotIndex ].m_ulOriginalID = ulOriginalID;

				// tell GC to update the slot attribute
				GCSDK::CProtoBufMsg<CMsgSetItemSlotAttribute> msg( k_EMsgGC_ClientSetItemSlotAttribute );

				msg.Body().set_item_id( m_pItem->GetItemID() );
				msg.Body().set_slot_item_original_id( ulOriginalID );
				msg.Body().set_slot_index( m_iCurrentSlotIndex + 1 );

				//EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_upgrade_card", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

				GCClientSystem()->BSendMessage( msg );
			}
		}
	}

	PostMessage( GetParent(), new KeyValues("SelectionEnded") );

	// It'll have deleted itself, so we don't need to clean it up
	m_pSelectionPanel = NULL;
	OnCancelSelection();

	// find the selected item and give it the focus
	CItemModelPanel *pSelection = GetFirstSelectedItemModelPanel( true );
	if( !pSelection )
	{
		m_pItemModelPanels[0]->SetSelected( true );
		pSelection = m_pItemModelPanels[0];
	}

	pSelection->RequestFocus();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::OnCancelSelection( void )
{
	if ( m_pSelectionPanel )
	{
		m_pSelectionPanel->SetVisible( false );
		m_pSelectionPanel->MarkForDeletion();
		m_pSelectionPanel = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::OnCommand( const char *command )
{
	if ( !V_stricmp( command, "ok" ) )
	{
		SetVisible( false );
		return;
	}
	else if ( V_stristr( command, "change" ) )
	{
		const char *pszNum = command+6;
		if ( pszNum && pszNum[0] )
		{
			int iSlot = atoi(pszNum);
			if ( iSlot >= 0 && iSlot < m_itemSlots.Count() )
			{
				if ( m_iCurrentSlotIndex != iSlot )
				{
					m_iCurrentSlotIndex = iSlot;
				}

				m_selectionCriteria = CItemSelectionCriteria();
				m_selectionCriteria.SetTags( m_itemSlots[m_iCurrentSlotIndex].m_slotCriteriaAttribute.tags().c_str() );
				m_selectionCriteria.SetIgnoreEnabledFlag( true );

				// Create the selection screen. It removes itself on close.
				m_pSelectionPanel = new CItemCriteriaSelectionPanel( this, &m_selectionCriteria );
				m_pSelectionPanel->InvalidateLayout( false, true ); // need to ApplySchemeSettings now so it doesn't override our SetDialogVariable below later
				m_pSelectionPanel->ShowPanel( 0, true );
				m_pSelectionPanel->SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( "#EditSlots_SelectItemPanel" ) );

				PostMessage( GetParent(), new KeyValues("SelectionStarted") );
			}
		}

		return;
	}

	BaseClass::OnCommand( command );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::UpdateModelPanels( void )
{
	// For now, fill them out with the local player's currently wielded items
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		CEconItemView *pItemData = TFInventoryManager()->GetLocalTFInventory()->GetInventoryItemByOriginalID( m_itemSlots[i].m_ulOriginalID );
		m_pItemModelPanels[i]->SetItem( pItemData );
		m_pItemModelPanels[i]->SetShowQuantity( true );
		m_pItemModelPanels[i]->SetSelected( false );
		SetBorderForItem( m_pItemModelPanels[i], false );
	}

	// Now layout again to position our item buttons 
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CItemSlotPanel::GetNumItemPanels( void )
{
	return NUM_MAX_SLOTS;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::OnShowPanel( bool bVisible, bool bReturningFromArmory )
{
	if ( bVisible )
	{
		if ( m_pSelectionPanel )
		{
			m_pSelectionPanel->SetVisible( false );
			m_pSelectionPanel->MarkForDeletion();
			m_pSelectionPanel = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::AddNewItemPanel( int iPanelIndex )
{
	BaseClass::AddNewItemPanel( iPanelIndex );

	m_itemSlots.AddToTail();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemSlotPanel::SetItem( CEconItem* pItem )
{
	if ( !pItem )
	{
		SetVisible( false );
		return;
	}

	OnCancelSelection();

	m_pItem = pItem;

	static CSchemaAttributeDefHandle s_itemSlotCriteriaAttributes[] =
	{
		CSchemaAttributeDefHandle( "item slot criteria 1" ),
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( s_itemSlotCriteriaAttributes ) == NUM_MAX_SLOTS );

	static CSchemaAttributeDefHandle s_itemInSlotAttributes[] =
	{
		CSchemaAttributeDefHandle( "item in slot 1" ),
	};
	COMPILE_TIME_ASSERT( ARRAYSIZE( s_itemInSlotAttributes ) == NUM_MAX_SLOTS );

	for ( int i=0; i<ARRAYSIZE( s_itemSlotCriteriaAttributes ); ++i )
	{
		m_itemSlots[i].m_bHasSlot = false;
		if ( m_pItem->FindAttribute( s_itemSlotCriteriaAttributes[i], &m_itemSlots[i].m_slotCriteriaAttribute ) )
		{
			m_itemSlots[i].m_bHasSlot = true;
			m_itemSlots[i].m_ulOriginalID = INVALID_ITEM_ID;
			m_pItem->FindAttribute( s_itemInSlotAttributes[i], &m_itemSlots[i].m_ulOriginalID );
		}
	}

	UpdateModelPanels();
}
