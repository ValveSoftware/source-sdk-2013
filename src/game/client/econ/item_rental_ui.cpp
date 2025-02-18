//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "rtime.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "item_rental_ui.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Confirm item preview.
//-----------------------------------------------------------------------------
class CConfirmItemPreviewDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmItemPreviewDialog, CBaseToolUsageDialog );

public:
	CConfirmItemPreviewDialog( vgui::Panel *pParent, CEconItemView *pPreviewItem );
	~CConfirmItemPreviewDialog();

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Apply( void );

private:

	CEconItemView* m_pPreviewItem;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmItemPreviewDialog::CConfirmItemPreviewDialog( vgui::Panel *parent, CEconItemView *pPreviewItem ) : CBaseToolUsageDialog( parent, "ConfirmItemPreviewDialog", pPreviewItem, pPreviewItem )
{
	m_pPreviewItem = pPreviewItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmItemPreviewDialog::~CConfirmItemPreviewDialog()
{
	delete m_pPreviewItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmItemPreviewDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/econ/ConfirmItemPreviewDialog.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pTitleLabel = dynamic_cast<vgui::Label*>( FindChildByName("TitleLabel") );
	if ( m_pTitleLabel )
	{
		wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "ItemPreviewDialogTitle" );
		if ( pszBaseString )
		{
			wchar_t	wTemp[256];
			g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, 1, m_pToolModelPanel->GetItem()->GetItemName() );
			m_pTitleLabel->SetText( wTemp );
			m_pTitleLabel->GetTextImage()->ClearColorChangeStream();
		}
	}

	m_pSubjectModelPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmItemPreviewDialog::Apply( void )
{
	// Notify the GC that the player wants to preview this item.
	GCSDK::CGCMsg< MsgGCItemPreviewRequest_t > msg( k_EMsgGCItemPreviewRequest );

	msg.Body().m_unItemDefIndex = m_pToolModelPanel->GetItem()->GetItemDefIndex();

	// OGS LOGGING HERE

	GCClientSystem()->BSendMessage( msg );

	EconUI()->SetPreventClosure( false );
}

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the item preview query response.
//-----------------------------------------------------------------------------
class CGCItemPreviewStatusResponse : public GCSDK::CGCClientJob
{
public:
	CGCItemPreviewStatusResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCItemPreviewCheckStatusResponse_t> msg( pNetPacket );

		CStorePanel *pStorePanel = EconUI()->GetStorePanel();
		if ( !pStorePanel )
			return true;

		if ( msg.Body().m_eResponse == k_EGCMsgResponseOK )
		{
			// We can preview the item.
			CEconItemView *pPreviewItem = new CEconItemView();
			pPreviewItem->Init( msg.Body().m_unItemDefIndex, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
			CConfirmItemPreviewDialog *dialog = vgui::SETUP_PANEL( new CConfirmItemPreviewDialog( pStorePanel->GetPropertySheet()->GetActivePage(), pPreviewItem ) );
			MakeModalAndBringToFront( dialog );
		}
		else
		{
#ifdef TF_CLIENT_DLL
			C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_preview_item_denied", CFmtStr( "%i", msg.Body().m_unItemDefIndex ).Access() );
#endif

			// We aren't allowed to preview an item right now.
			CTFMessageBoxDialog* pDialog = ShowMessageBox( "#ItemPreview_PreviewStartFailedTitle", "#ItemPreview_PreviewStartFailedText", "#GameUI_OK" );
			RTime32 nextTime = msg.Body().m_timePreviewTime + EconUI()->GetStorePanel()->GetPriceSheet()->GetPreviewPeriod();
			
			locchar_t wzValue[64];
			char time_buf[k_RTimeRenderBufferSize];
			GLocalizationProvider()->ConvertUTF8ToLocchar( CRTime::Render( nextTime, time_buf ), wzValue, sizeof( wzValue ) );
			pDialog->AddStringToken( "date_time", wzValue );
		}

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCItemPreviewStatusResponse, "CGCItemPreviewStatusResponse", k_EMsgGCItemPreviewStatusResponse, GCSDK::k_EServerTypeGCClient );

//-----------------------------------------------------------------------------
// Purpose: The GC is telling us our request has been granted.
//-----------------------------------------------------------------------------
class CGCItemPreviewRequestResponse : public GCSDK::CGCClientJob
{
public:
	CGCItemPreviewRequestResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	static void OnPreviewItemConfirm( bool bConfirmed, void *pContext )
	{
		InventoryManager()->ShowItemsPickedUp( true, false );

		CStorePage* pStorePage = dynamic_cast<CStorePage*>( EconUI()->GetStorePanel()->GetActivePage() );
		if ( pStorePage )
		{
			pStorePage->UpdateModelPanels();
		}
	}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCItemPreviewRequestResponse_t> msg( pNetPacket );

		CStorePanel *pStorePanel = EconUI()->GetStorePanel();
		if ( !pStorePanel )
			return true;

		if ( msg.Body().m_eResponse == k_EGCMsgResponseOK )
		{
			// The preview has started.
			ShowMessageBox( "#ItemPreview_PreviewStartedTitle", "#ItemPreview_PreviewStartedText", "#GameUI_OK", OnPreviewItemConfirm );
		}
		else
		{
			// The preview cannot start right now for some reason.
		}

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCItemPreviewRequestResponse, "CGCItemPreviewRequestResponse", k_EMsgGCItemPreviewRequestResponse, GCSDK::k_EServerTypeGCClient );

void OpenStoreToItem( bool bConfirmed, void *pContext )
{
	CEconPreviewExpiredNotification* pNotification = (CEconPreviewExpiredNotification*) pContext;
	if ( pNotification )
	{
		pNotification->SetIsInUse( false );
		pNotification->MarkForDeletion();
		if ( bConfirmed )
		{
			EconUI()->OpenStorePanel( pNotification->GetItemDefIndex(), true );
		}
	}
}

CEconPreviewNotification::CEconPreviewNotification( uint64 ulSteamID, uint32 iItemDef ) 
	: CEconNotification() 
{
	SetSteamID( ulSteamID );
	SetLifetime( 20.0f );

	m_pItemDef = GetItemSchema()->GetItemDefinition( iItemDef );
	if ( !m_pItemDef )
		return;

	AddStringToken( "item_name",  g_pVGuiLocalize->Find(m_pItemDef->GetItemBaseName()) );
}

void CEconPreviewExpiredNotification::Trigger()
{
	CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#TF_PreviewItem_Expired_Title",  "#TF_PreviewItem_Expired_Text", "#TF_PreviewItem_BuyIt", "#TF_PreviewItem_NotNow", &OpenStoreToItem );
	pDialog->SetContext( this );
	pDialog->AddStringToken( "item_name",  g_pVGuiLocalize->Find(m_pItemDef->GetItemBaseName()) );
	SetIsInUse( true );
}

//-----------------------------------------------------------------------------
// Purpose: The GC is telling us our preview item has expired.
//-----------------------------------------------------------------------------
class CGCItemPreviewExpireNotification : public GCSDK::CGCClientJob
{
public:
	CGCItemPreviewExpireNotification( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCItemPreviewExpireNotification_t> msg( pNetPacket );

		CEconPreviewExpiredNotification *pNotification = new CEconPreviewExpiredNotification( msg.Hdr().m_ulSteamID, msg.Body().m_unItemDefIndex );
		pNotification->SetText( "#TF_PreviewItem_Expired" );
		NotificationQueue_Add( pNotification );

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCItemPreviewExpireNotification, "CGCItemPreviewExpireNotification", k_EMsgGCItemPreviewExpireNotification, GCSDK::k_EServerTypeGCClient );


//-----------------------------------------------------------------------------
// Purpose: The GC is telling us we bought our preview item!
//-----------------------------------------------------------------------------
class CGCItemPreviewItemBoughtNotification : public GCSDK::CGCClientJob
{
public:
	CGCItemPreviewItemBoughtNotification( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCItemPreviewItemBoughtNotification> msg( pNetPacket );

		CEconPreviewItemBoughtNotification *pNotification = new CEconPreviewItemBoughtNotification( msg.Hdr().client_steam_id(), msg.Body().item_def_index() );
		pNotification->SetText( "#TF_PreviewItem_ItemBought" );
		NotificationQueue_Add( pNotification );

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCItemPreviewItemBoughtNotification, "CGCItemPreviewItemBoughtNotification", k_EMsgGCItemPreviewItemBoughtNotification, GCSDK::k_EServerTypeGCClient );