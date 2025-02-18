//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "halloween_offering_panel.h"
#include "cdll_client_int.h"
#include "ienginevgui.h"
#include "econ_item_tools.h"
#include "econ_ui.h"
#include <vgui_controls/AnimationController.h>
#include "clientmode_tf.h"
#include "softline.h"
#include "drawing_panel.h"
#include "tf_item_inventory.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHalloweenOfferingPanel::CHalloweenOfferingPanel( vgui::Panel *parent, CItemModelPanelToolTip* pTooltip ) 
	: BaseClass( parent, pTooltip )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHalloweenOfferingPanel::~CHalloweenOfferingPanel( void )
{

}

//-----------------------------------------------------------------------------
void CHalloweenOfferingPanel::CreateSelectionPanel()
{
	CHalloweenOfferingSelectionPanel *pSelectionPanel = new CHalloweenOfferingSelectionPanel( this );
	m_hSelectionPanel = (CCollectionCraftingSelectionPanel*)pSelectionPanel;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHalloweenOfferingPanel::OnCommand( const char *command )
{
	if ( FStrEq( "envelopesend", command ) )
	{
		GCSDK::CProtoBufMsg<CMsgCraftHalloweenOffering> msg( k_EMsgGCCraftHalloweenOffering );

		// Find the Garygoyle 'tool' item for this
		static CSchemaItemDefHandle pItemDef_Gargoyle( "Activated Halloween Pass" );
		Assert( pItemDef_Gargoyle );
		if ( !pItemDef_Gargoyle )
			return;
		// Find out if the user owns this item or not and place in the proper bucket
		CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
		if ( !pLocalInv )
			return;

		const CEconItemView *pRefItem = pLocalInv->FindFirstItembyItemDef( pItemDef_Gargoyle->GetDefinitionIndex() );
		if ( !pRefItem )
			return;

		msg.Body().set_tool_id( pRefItem->GetItemID() );

		FOR_EACH_VEC( m_vecItemPanels, i )
		{
			 if ( m_vecItemPanels[ i ]->GetItem() == NULL )
				 return;

			msg.Body().add_item_id( m_vecItemPanels[ i ]->GetItem()->GetItemID() );
		}
		 // Send if off
		GCClientSystem()->BSendMessage( msg );

		m_bWaitingForGCResponse = true;
		m_nFoundItemID.Purge();
		m_timerResponse.Start( 5.f );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "CollectionCrafting_LetterSend" );	
		return;
	}

	BaseClass::OnCommand( command );
}