//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "econ_item_tools.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "tool_items.h"
#include "gift_wrap_tool.h"
#include "econ_ui.h"
#include "vgui/ISurface.h"
#include "econ_controls.h"
#include "confirm_dialog.h"
#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Confirm / abort tool application
//-----------------------------------------------------------------------------
class CConfirmGiftWrapDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmGiftWrapDialog, CBaseToolUsageDialog );

public:
	CConfirmGiftWrapDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Apply( void );
};

//-----------------------------------------------------------------------------
// Purpose: Completed wrapping dialog
//-----------------------------------------------------------------------------
class CWaitForGiftWrapDialog : public CGenericWaitingDialog
{
public:
	CWaitForGiftWrapDialog( vgui::Panel *pParent ) : CGenericWaitingDialog( pParent )
	{
	}

protected:
	virtual void OnTimeout()
	{
		// Play an exciting sound!
		vgui::surface()->PlaySound( "misc/achievement_earned.wav" );

		// Show them the result item.
		InventoryManager()->ShowItemsPickedUp( true );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmGiftWrapDialog::CConfirmGiftWrapDialog( vgui::Panel *parent, CEconItemView *pTool, CEconItemView *pToolSubject ) : CBaseToolUsageDialog( parent, "ConfirmApplyGiftWrapDialog", pTool, pToolSubject )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmGiftWrapDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/econ/ConfirmApplyGiftWrapDialog.res" );

	// We might want to change our label text to explicitly call out that we'll reset strange scores
	// on gift wrap, but only if we're trying to use this gift wrap on a strange item that has scores
	// that would be affected by it.
	CEconItemView *pSubjectItemView = GetSubjectItem();
	CExLabel *pTextLabel = dynamic_cast<CExLabel *>( FindChildByName( "ConfirmLabel" ) );
	CExLabel *pTextLabelStrange = dynamic_cast<CExLabel *>( FindChildByName( "ConfirmLabelStrange" ) );

	if ( pSubjectItemView && pTextLabel && pTextLabelStrange )
	{
		bool bHasNonZeroScore = false;

		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			uint32 unScore;
			if ( pSubjectItemView->FindAttribute( GetKillEaterAttr_Score( i ), &unScore ) && unScore > 0 )
			{
				bHasNonZeroScore = true;
				break;
			}
		}

		if ( bHasNonZeroScore )
		{
			pTextLabel->SetVisible( false );
			pTextLabelStrange->SetVisible( true );
		}
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmGiftWrapDialog::Apply( void )
{
	// Tell the GC to wrap the subject item.
	GCSDK::CGCMsg< MsgGCGiftWrapItem_t > msg( k_EMsgGCGiftWrapItem );

	msg.Body().m_unToolItemID = m_pToolModelPanel->GetItem()->GetItemID();
	msg.Body().m_unSubjectItemID = m_pSubjectModelPanel->GetItem()->GetItemID();

	EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pSubjectModelPanel->GetItem(), "gift_wrap_item" );

	GCClientSystem()->BSendMessage( msg );

	vgui::surface()->PlaySound( "ui/item_gift_wrap_use.wav" );
	ShowWaitingDialog( new CWaitForGiftWrapDialog( NULL ), "#ToolGiftWrapInProgress", true, false, 5.0f );
}

// Entry point from the UI.
void CEconTool_GiftWrap::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmGiftWrapDialog *dialog = vgui::SETUP_PANEL( new CConfirmGiftWrapDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( dialog );
}


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the server response that we've given an item.
//-----------------------------------------------------------------------------
class CGCGiftGivenResponse : public GCSDK::CGCClientJob
{
public:
	CGCGiftGivenResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgDeliverGiftResponseGiver> msg( pNetPacket );

		// Pop up a notification to confirm that the gift has been sent.
		switch ( msg.Body().response_code() )
		{
			case k_EGCMsgResponseOK:
				if ( msg.Body().has_receiver_account_name() )
				{
					KeyValues *pkv = new KeyValues( "GiftReceiverParams" );
					KeyValuesAD kvad( pkv );

					pkv->SetString( "receiver_account_name", msg.Body().receiver_account_name().c_str() ); 
					ShowMessageBox( "#TF_DeliverGiftResultDialog_Title", "#TF_DeliverGiftResultDialog_Success_WithAccount", pkv, "#GameUI_OK" );
				}
				else
				{
					ShowMessageBox( "#TF_DeliverGiftResultDialog_Title", "#TF_DeliverGiftResultDialog_Success", "#GameUI_OK" );
				}
				break;
			case k_EGCMsgResponseDenied:
				ShowMessageBox( "#TF_DeliverGiftResultDialog_Title", "#TF_DeliverGiftResultDialog_VAC", "#GameUI_OK" );
				break;
			default:
				ShowMessageBox( "#TF_DeliverGiftResultDialog_Title", "#TF_DeliverGiftResultDialog_Fail", "#GameUI_OK" );
				break;
		} // switch

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCGiftGivenResponse, "CGCGiftGivenResponse", k_EMsgGCDeliverGiftResponseGiver, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the server response that we've received an item.
//-----------------------------------------------------------------------------
class CGCGiftReceivedResponse : public GCSDK::CGCClientJob
{
public:
	CGCGiftReceivedResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );

		// If the receiver is online when the gift is sent, they will get this response.
		InventoryManager()->GetLocalInventory()->NotifyHasNewItems();

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCGiftReceivedResponse, "CGCGiftReceivedResponse", k_EMsgGCDeliverGiftResponseReceiver, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: Completed unwrapping...
//-----------------------------------------------------------------------------
class CWaitForGiftUnwrapDialog : public CGenericWaitingDialog
{
public:
	CWaitForGiftUnwrapDialog( vgui::Panel *pParent ) : CGenericWaitingDialog( pParent )
	{
	}

protected:
	virtual void OnTimeout()
	{
		// Play an exciting sound!
		vgui::surface()->PlaySound( "misc/achievement_earned.wav" );

		// Show them the result item.
		InventoryManager()->ShowItemsPickedUp( true );
	}
};

static void UnwrapGiftConfirm( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		vgui::surface()->PlaySound( "ui/item_gift_wrap_unwrap.wav" );
		ShowWaitingDialog( new CWaitForGiftWrapDialog( NULL ), "#ToolGiftUnwrapInProgress", true, false, 5.0f );

		CEconItemView *pItem = (CEconItemView*) pContext;
		GCSDK::CGCMsg< MsgGCUnwrapGiftRequest_t > msg( k_EMsgGCUnwrapGiftRequest );
		msg.Body().m_unItemID = pItem->GetItemID();
		GCClientSystem()->BSendMessage( msg );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, pItem, "unwrapped_gift" );
	}	
}

void PerformToolAction_UnwrapGift( vgui::Panel* pParent, CEconItemView *pGiftItem )
{
	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_UnwrapGift_Title", "#TF_UnwrapGift_Text", 
		"#GameUI_OK", "#Cancel", 
		&UnwrapGiftConfirm );
	pDialog->AddStringToken( "item_name", pGiftItem->GetItemName() );
	pDialog->SetContext( pGiftItem );
}
