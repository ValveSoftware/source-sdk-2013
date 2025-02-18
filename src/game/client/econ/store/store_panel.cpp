//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/store_panel.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui/IInput.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "econ_item_inventory.h"
#include <vgui/ILocalize.h>
#include "econ_item_system.h"
#include "store_page_new.h"
#include "store_viewcart.h"
#include "confirm_dialog.h"
#include <vgui_controls/AnimationController.h>
#include "econ_ui.h"
#include "gc_clientsystem.h"
#include "steamworks_gamestats.h"
#include "econ/econ_storecategory.h"

#ifdef TF_CLIENT_DLL
#include "tf_mapinfo.h"
#include "c_tf_freeaccount.h"
#include "tf_hud_statpanel.h"
#include "rtime.h"
#include "item_ad_panel.h"
#include "tf_matchmaking_dashboard.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>



#ifdef TF_CLIENT_DLL
// Dec 4st 2012
#define TF_STORE_STAMP_UPSELL_BASELINE 1354579200
// TEMP VALUE FOR TESTING! Nov 20th 2012
//#define TF_STORE_STAMP_UPSELL_BASELINE 1353369600

// 15 days
#define TF_STORE_STAMP_UPSELL_COOLDOWN ( 60 /*sec*/ * 60 /*min*/ * 24 /*hr*/ * 7 /*day*/ )
// TEMP VALUE FOR TESTING! 1 day
//#define TF_STORE_STAMP_UPSELL_COOLDOWN ( 60 /*sec*/ * 60 /*min*/ * 24 /*hr*/ )

#define TF_STORE_STAMP_UPSELL_GROUPS 15

ConVar tf_store_stamp_donation_add_timestamp( "tf_store_stamp_donation_add_timestamp", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE | FCVAR_HIDDEN, "Remember the last time that we offered a stamp donation at checkout." );
#endif

bool CStorePanel::m_bPricesheetLoaded = false;
bool CStorePanel::m_bShowWarnings = false;

class CServerNotConnectedToSteamDialog;
CServerNotConnectedToSteamDialog *OpenServerNotConnectedToSteamDialog( vgui::Panel *pParent );

class CStampUpsellDialog : public CTFGenericConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CStampUpsellDialog, CTFGenericConfirmDialog );
public:
	CStampUpsellDialog( const char *pTitle, const wchar_t *pTextText, const wchar_t *pTextText2, CSchemaItemDefHandle hMapToken, const char *pItemDefName2 ) 
		: CTFGenericConfirmDialog( pTitle, pTextText, NULL, NULL, NULL, NULL )
		, hItemDef( hMapToken ), hItemDef2( pItemDefName2 )
	{
		V_wcsncpy( m_wszBuffer2, pTextText2, sizeof( m_wszBuffer2 ) );
		m_flCreationTime = gpGlobals->curtime;
	}

	virtual const char *GetResFile() OVERRIDE
	{
		return "Resource/UI/StampDonationAdd.res";
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings(pScheme);

		vgui::ImagePanel *pItemImagePanel = dynamic_cast<vgui::ImagePanel *>( FindChildByName( "ItemImagePanel", true ) ); Assert( pItemImagePanel );
		if ( pItemImagePanel && hItemDef )
		{
			pItemImagePanel->SetImage( CFmtStr( "../%s_large", hItemDef->GetInventoryImage() ) );
		}

		vgui::ImagePanel *pItemImagePanel2 = dynamic_cast<vgui::ImagePanel *>( FindChildByName( "ItemImagePanel2", true ) ); Assert( pItemImagePanel );
		if ( pItemImagePanel2 && hItemDef2 )
		{
			pItemImagePanel2->SetImage( CFmtStr( "../%s_large", hItemDef2->GetInventoryImage() ) );
		}

		// Now go through the string and find the escape characters telling us where the color changes are
		ColorizeLabel( static_cast< vgui::Label* >( FindChildByName( "ExplanationLabel" ) ), m_wszBuffer );
		ColorizeLabel( static_cast< vgui::Label* >( FindChildByName( "ExplanationLabel2" ) ), m_wszBuffer2 );

		CEconItemView itemData;
		itemData.Init( hItemDef->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
		itemData.SetItemQuantity( 1 );
		itemData.SetClientItemFlags( kEconItemFlagClient_Preview | kEconItemFlagClient_StoreItem );

		const ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();
		const econ_store_entry_t *pEntry = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( hItemDef->GetDefinitionIndex() );
		item_price_t unPrice = pEntry->GetCurrentPrice( eCurrency );

		wchar_t wzLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
		MakeMoneyString( wzLocalizedPrice, ARRAYSIZE( wzLocalizedPrice ), unPrice, EconUI()->GetStorePanel()->GetCurrency() );
		SetDialogVariable( "price", wzLocalizedPrice );

		Panel *pConfirmButton = FindChildByName( "ConfirmButton" );
		pConfirmButton->RequestFocus();
	}

	virtual void OnCommand( const char *command ) OVERRIDE
	{
		int nSecondsVisible = gpGlobals->curtime - m_flCreationTime;
		if ( V_strcmp( command, "add_stamp_to_cart" ) == 0 )
		{
			FinishUp();
			CStorePanel::ConfirmUpsellStamps( true, hItemDef, nSecondsVisible );
		}
		else if ( V_strcmp( command, "nope" ) == 0 )
		{
			FinishUp();
			CStorePanel::ConfirmUpsellStamps( false, hItemDef, nSecondsVisible );
		}
		else
		{
			BaseClass::OnCommand( command );
		}		
	}

	virtual void OnKeyCodePressed( vgui::KeyCode code )
	{
		// ESC cancels
		if ( code == KEY_XBUTTON_B )
		{
			OnCommand( "nope" );
		}
		else
		{
			BaseClass::OnKeyCodePressed( code );
		}
	}

	void ColorizeLabel( vgui::Label *pLabel, wchar_t *txt )
	{
		if ( pLabel )
		{
			pLabel->GetTextImage()->ClearColorChangeStream();

			// We change the title's text color to match the colors of the matching model panel backgrounds
			int iWChars = 0;
			Color colCustom;
			while ( txt && *txt )
			{
				switch ( *txt )
				{
				case 0x01:	// Normal color
					pLabel->GetTextImage()->AddColorChange( Color(200,80,60,255), iWChars );
					break;
				case 0x02:	// Item 1 color
					pLabel->GetTextImage()->AddColorChange( Color(255,255,255,255), iWChars );
					break;
				default:
					break;
				}
				txt++;
				iWChars++;
			}
		}
	}

	CSchemaItemDefHandle hItemDef;
	CSchemaItemDefHandle hItemDef2;

	wchar_t m_wszBuffer2[1024];
	float m_flCreationTime;
};


//-----------------------------------------------------------------------------
// Purpose: A dialog used to show the current state of store communication with steam.
//-----------------------------------------------------------------------------
class CStoreStatusDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CStoreStatusDialog, vgui::EditablePanel );

public:
	CStoreStatusDialog( vgui::Panel *pParent, const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	OnCommand( const char *command );
	void			UpdateSchemeForVersion();
	void			ShowStatusUpdate( bool bAllowed, bool bShowOnExit, bool bCancel );

private:
	bool			m_bShowOnExit;
	bool			m_bNotifyOnCancel;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void ContactSupportConfirm( bool bConfirmed, void *pContext )
{
	if ( bConfirmed && steamapicontext && steamapicontext->SteamFriends() )
	{
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "https://support.steampowered.com/" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePanel::CStorePanel( Panel *parent ) : PropertyDialog(parent, "store_panel")
, m_CallbackMicroTransactionAuthResponse( this, &CStorePanel::OnMicroTransactionAuthResponse )
{
	// Store is parented to the game UI panel
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );

	SetMoveable( false );
	SetSizeable( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "gameui_hidden" );

	m_unTransactionID = 0;
	m_bShouldFinalize = false;
	m_iStartItemDef = 0;
	m_bAddStartItemDefToCart = false;
	m_bPreventClosure = false;
	m_bOGSLogging = false;

	m_iCheckoutAttempts = 0;
	m_iLastPurchaseAttemptPrice = 0;

	m_eCurrency = k_ECurrencyUSD;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePanel::~CStorePanel()
{
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( CFmtStr( "Resource/UI/econ/store/v%i/StorePanel.res", GetStoreVersion() ) );

	SetOKButtonVisible(false);
	SetCancelButtonVisible(false);

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::PerformLayout( void ) 
{
	if ( GetVParent() )
	{
		int w,h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds(0,0,w,h);
	}

#ifdef TF_CLIENT_DLL
	bool bShowUpsellCheckbox = ( tf_store_stamp_donation_add_timestamp.GetFloat() > 0.0f && HasValidUpsellStamps() );
	SetControlVisible( "SupportCommunityMapMakersCheckButton", bShowUpsellCheckbox );
	SetControlVisible( "SupportCommunityMapMakersLabel", bShowUpsellCheckbox );
#endif

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::OnStartShopping( void )
{
	// Move to the first tab
	vgui::Panel *pPage = GetPropertySheet()->GetPage(1);
	if ( pPage )
	{
		GetPropertySheet()->SetActivePage( pPage );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::OnFindAndSelectFeaturedItem( void )
{
	const econ_store_entry_t *pEntry = GetFeaturedEntry();
	if ( !pEntry )
		return;

	FindAndSelectEntry( pEntry );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::FindAndSelectEntry( const econ_store_entry_t *pEntry )
{
	// Find the item in a store page and move to it
	int iPages = GetPropertySheet()->GetNumPages();
	for ( int i = 1; i < iPages; i++ )
	{
		CStorePage *pPage = dynamic_cast< CStorePage * >( GetPropertySheet()->GetPage(i) );
		if ( !pPage )
			continue;
		
		if ( pPage->FindAndSelectEntry( pEntry ) )
		{
			if ( GetPropertySheet()->GetActivePage() != pPage )
			{
				GetPropertySheet()->SetActivePage( pPage );
			}
			else
			{
				// VGUI doesn't tell the starting active page that it's active, so we post a pageshow to it
				ivgui()->PostMessage( pPage->GetVPanel(), new KeyValues("PageShow"), GetPropertySheet()->GetVPanel() );
			}
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Static store page factory.
//-----------------------------------------------------------------------------
CStorePage *CStorePanel::CreateStorePage( const CEconStoreCategoryManager::StoreCategory_t *pPageData )
{
	// Default, standard store page.
	return new CStorePage( this, pPageData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CStorePanel::ShouldShowDx8PurchaseWarning() const
{
	static ConVarRef mat_dxlevel( "mat_dxlevel" );
	if ( mat_dxlevel.GetInt() >= 90 )
		return false;

	// List of operations that have features that are not compatible with DX8.
	const char* cpDX8WarningItems[] = {
		"Unused Summer 2015 Operation Pass",
		"Unused Operation Tough Break Pass",
		NULL
	};

	for ( int i = 0; cpDX8WarningItems[ i ] != NULL; ++i )
	{
		if ( m_Cart.ContainsItemDefinition( ItemSystem()->GetStaticDataForItemByName( cpDX8WarningItems[ i ] )->GetDefinitionIndex() ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::OnItemLinkClicked( KeyValues *pParams )
{
	// Get the item definition index from the URL
	const char *pURL = pParams->GetString( "url" );
	int iItemDef = atoi( pURL + 7 );

	if ( EconUI()->GetStorePanel()->GetPriceSheet() )
	{
		// Look up the item definition and see if there is a store remap
		CEconItemSchema *pSchema = ItemSystem()->GetItemSchema();
		if ( pSchema )
		{
			CEconItemDefinition *pItem = pSchema->GetItemDefinition( iItemDef );
			if ( pItem )
			{
				int iStoreRemap = pItem->GetStoreRemap();
				if ( iStoreRemap > 0 )
				{
					iItemDef = pItem->GetStoreRemap();
				}
			}
		}

		const econ_store_entry_t *pEntry = GetPriceSheet()->GetEntry( iItemDef );
		if ( pEntry )
		{
			FindAndSelectEntry( pEntry );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::ShowPanel(bool bShow)
{
	m_bPreventClosure = false;

	if ( bShow )
	{
		if ( !m_bOGSLogging )
		{
			EconUI()->Gamestats_Store( IE_STORE_ENTERED );
			m_bOGSLogging = true;
		}

		ShowStorePanel();
	}
	else
	{
		if ( m_bOGSLogging )
		{
			EconUI()->Gamestats_Store( IE_STORE_EXITED );
			m_bOGSLogging = false;
		}
	}

	SetVisible( bShow );

	if ( bShow && m_bAddStartItemDefToCart )
	{
		OpenStoreViewCartPanel();
		m_bAddStartItemDefToCart = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		if ( m_bPreventClosure )
		{
			engine->ClientCmd_Unrestricted( "gameui_activate" );
		}
		else
		{
			ShowPanel( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "checkout", 8 ) )
	{
		InitiateCheckout( false );
		return;
	}
	else if ( !Q_stricmp( command, "close" ) )
	{
		ShowPanel( false );

		// If we're connected to a game server, we also close the game UI.
		if ( engine->IsInGame() )
		{
			engine->ClientCmd_Unrestricted( "gameui_hide" );
		}
		
#ifdef TF_CLIENT_DLL
		if ( IsFreeTrialAccount() )
		{
			InventoryManager()->CheckForRoomAndForceDiscard();
		}
#endif
	}
	else if ( !Q_stricmp( command, "back" ) )
	{
		ShowPanel( true );
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		if ( !m_bPreventClosure )
		{
			ShowPanel( false );
		}
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const econ_store_entry_t *CStorePanel::GetFeaturedEntry( void )
{
	const CEconStoreCategoryManager::StoreCategory_t *pFeaturedSection = GEconStoreCategoryManager()->GetFeaturedItems();
	if ( !pFeaturedSection || pFeaturedSection->m_vecEntries.Count() == 0 )
		return NULL;
	uint32 idx = MIN( m_StoreSheet.GetFeaturedItemIndex(), (uint32)( pFeaturedSection->m_vecEntries.Count() - 1 ) );
	return EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( pFeaturedSection->m_vecEntries[ idx ] );
}

//-----------------------------------------------------------------------------
// Purpose: Asks the GC to send us the price sheet.
//-----------------------------------------------------------------------------
void CStorePanel::RequestPricesheet( void )
{
	// Create the store panel the first time we request a price sheet
	EconUI()->CreateStorePanel();

	CGCClientJobGetUserData *pJob = new CGCClientJobGetUserData( GCClientSystem()->GetGCClient(), 0 );
	pJob->StartJob( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Asynchronous job for getting the price sheet from the GC
//-----------------------------------------------------------------------------
bool CGCClientJobGetUserData::BYieldingRunJob( void *pvStartParam )
{
	GCSDK::CProtoBufMsg<CMsgStoreGetUserData> msg( k_EMsgGCStoreGetUserData );
	GCSDK::CProtoBufMsg<CMsgStoreGetUserDataResponse> msgResponse;

	msg.Body().set_price_sheet_version( m_RTimeVersion );
	if ( !BYldSendMessageAndGetReply( msg, 10, &msgResponse, k_EMsgGCStoreGetUserDataResponse ) )
	{
		// No response from the GC. Show a failure message.
		if ( CStorePanel::ShouldShowWarnings() )
		{
			OpenStoreStatusDialog( NULL, "#StoreUpdate_NoGCResponse", true, false );
		}
		return false;
	}

#ifdef _DEBUG
	Msg( "CGCClientJobGetUserData - Result: %d\n", msgResponse.Body().result() );
#endif

	if ( !CStorePanel::CheckMessageResult( (EPurchaseResult)msgResponse.Body().result() ) )
		return true;

	bool bInitialLoad = !CStorePanel::IsPricesheetLoaded();

	CStorePanel *pStorePanel = EconUI()->GetStorePanel();

	// Create the store panel.
	if ( bInitialLoad )
	{
		// Close the loading status dialog.
		CloseStoreStatusDialog();
	}
	else
	{
		pStorePanel->GetPropertySheet()->DeleteAllPages();

		// Indicate to the user what happened.
		if( pStorePanel->IsVisible() )
		{
			OpenStoreStatusDialog( NULL, "#StoreUpdate_NewPriceSheetLoaded", true, false );
		}
	}

	// Set the prices & items on the store panel.
	KeyValuesAD pKVPricesheet( "prices" );

	// Allow a back-door for reading the local price sheet rather than the one from the GC.
//#define READ_LOCAL_PRICE_SHEET // DO NOT CHECK IN
#if defined( READ_LOCAL_PRICE_SHEET ) && defined( _DEBUG )
	pKVPricesheet->LoadFromFile( g_pFullFileSystem, "scripts/items/unencrypted/store.txt", "MOD" );
#else
	CUtlBuffer bufRawData( msgResponse.Body().price_sheet().data(), msgResponse.Body().price_sheet().size(), CUtlBuffer::READ_ONLY );
	pKVPricesheet->ReadAsBinary( bufRawData );
#endif

	pKVPricesheet->SetInt( "featured_item_index", msgResponse.Body().featured_item_idx() );
	pKVPricesheet->SetInt( "default_sort_type", msgResponse.Body().default_item_sort() );
	pStorePanel->LoadPricesheet( &pKVPricesheet );

	// Manually copy over the version number the GC sent down.
	Assert( pStorePanel->GetPriceSheetForEdit() );
	pStorePanel->GetPriceSheetForEdit()->SetVersionStamp( msgResponse.Body().price_sheet_version() );

	pStorePanel->ClearPopularItems();
	for ( int i=0; i<msgResponse.Body().popular_items_size(); ++i )
	{
		pStorePanel->AddPopularItem( msgResponse.Body().popular_items(i) );
	}

	// Store our experiment membership value.
	EconUI()->SetExperimentValue( msgResponse.Body().experiment_data() ); 

	// Store the currency and country code.
	if ( msgResponse.Body().currency() == k_ECurrencyInvalid )
	{
		// An invalid currency means the user must contact steam support to have their account set up.
		OpenStoreStatusDialog( NULL, "#StoreUpdate_ContactSupport", true, false );
		return true;
	}

	pStorePanel->SetCurrency( (ECurrency)msgResponse.Body().currency() );
	pStorePanel->SetCountryCode( msgResponse.Body().country().c_str() );
	// TODO: Also store the steam provided localization code (not implemented yet in the response).

	// Open the store panel.
 	if ( !bInitialLoad )
 	{
		// Not an initial load, but store was already up. Re-open it to clean it up.
		if ( pStorePanel->IsVisible() )
		{
 			pStorePanel->ShowPanel( true );
		}
 	}
	else
	{
#ifndef TF_CLIENT_DLL
		// First time we've loaded the pricesheet, so open the store. 
		// You can remove this if you choose to load the pricesheet on game startup (like TF does)
		EconUI()->OpenStorePanel( 0, false );	
#endif
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "store_pricesheet_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePage *CStorePanel::AddPageFromPriceSheet( int iPage )
{
	CStorePage* pPage = CreateStorePage( GEconStoreCategoryManager()->GetCategoryFromIndex( iPage ) );
	pPage->OnPostCreate();
	pPage->AddActionSignalTarget( this );
	AddPage( pPage, GEconStoreCategoryManager()->GetCategoryFromIndex( iPage )->m_pchName );	

	if ( iPage == 0 )
	{
		pPage->SetVisible( true );
	}

	return pPage;
}

//-----------------------------------------------------------------------------
// Purpose: Populates the storepanel with data from the GC
//-----------------------------------------------------------------------------
bool CStorePanel::LoadPricesheet( KeyValuesAD* pKVPricesheet )
{
	// Read the store KV file in, and parse it.
	Verify( m_StoreSheet.InitFromKV( *pKVPricesheet ) );

	// Add our pages
	for ( int i = 0; i < GEconStoreCategoryManager()->GetNumCategories(); i++ )
	{
		// Skip subcategories
		if ( GEconStoreCategoryManager()->GetCategoryFromIndex( i )->BIsSubcategory() )
			continue;

		AddPageFromPriceSheet( i );
	}

	// Clear the cart, since we may have new items.
	GetCart()->EmptyCart();

	m_bPricesheetLoaded = true;

	return true;
}

#ifdef _DEBUG
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::ReAddPage( int iPage )
{
	CStorePage *pPage = AddPageFromPriceSheet( iPage );
	if ( pPage )
	{
		pPage->InvalidateLayout( true, true );
		pPage->SetVisible( true );
		GetPropertySheet()->SetActivePage( pPage );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Sets currency type.
//-----------------------------------------------------------------------------
void CStorePanel::SetCurrency( ECurrency in_currency )
{
	// Do any currency related UI work here.
	m_eCurrency = in_currency;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the country code. Later this will become an enum.
//-----------------------------------------------------------------------------
void CStorePanel::SetCountryCode( const char* in_country )
{
	V_strcpy_safe( m_rgchCountry, in_country );
}

bool CStorePanel::ShouldUpsellStamps( void )
{
#ifdef TF_CLIENT_DLL
	bool bForceShow = false;

	CheckButton *pSupportCheckButton = static_cast< CheckButton* >( FindChildByName( "SupportCommunityMapMakersCheckButton" ) );
	if ( pSupportCheckButton && pSupportCheckButton->IsVisible() && pSupportCheckButton->IsSelected() )
	{
		bForceShow = true;
	}

	CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
	if ( !pCart || pCart->GetNumEntries() <= 0 )
		return false;

	if ( !bForceShow )
	{
		for ( int i = 0; i < pCart->GetNumEntries(); ++i )
		{
			cart_item_t *pItem = pCart->GetItem( i );
			if ( pItem && pItem->pEntry && pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Maps ) )
			{
				return false;
			}
		}
	}

	if ( !HasValidUpsellStamps() )
		return false;

	RTime32 rtCurrentTime = CRTime::RTime32TimeCur();
	RTime32 rtTimeStamp = tf_store_stamp_donation_add_timestamp.GetInt();

	if ( !bForceShow )
	{
		if ( rtTimeStamp > rtCurrentTime )
		{
			// Time stamp set ahead of current time
			// Set it back to the current time and save it
			tf_store_stamp_donation_add_timestamp.SetValue( (int)rtCurrentTime );
			engine->ClientCmd_Unrestricted( "host_writeconfig" );
			return false;
		}

		AccountID_t nAccountID = steamapicontext->SteamUser()->GetSteamID().GetAccountID();
		int nGroup = nAccountID % TF_STORE_STAMP_UPSELL_GROUPS;
		RTime32 nBucketTimeOffset = ( TF_STORE_STAMP_UPSELL_COOLDOWN / TF_STORE_STAMP_UPSELL_GROUPS ) * nGroup;
		RTime32 nBucketedTimeStamp = ( ( rtTimeStamp / TF_STORE_STAMP_UPSELL_COOLDOWN ) + 1 ) * TF_STORE_STAMP_UPSELL_COOLDOWN;
		nBucketedTimeStamp = MAX( nBucketedTimeStamp, TF_STORE_STAMP_UPSELL_BASELINE );
		nBucketedTimeStamp += nBucketTimeOffset;

		if ( rtCurrentTime < nBucketedTimeStamp + TF_STORE_STAMP_UPSELL_COOLDOWN )
		{
			return false;
		}
	}

	tf_store_stamp_donation_add_timestamp.SetValue( (int)rtCurrentTime );
	engine->ClientCmd_Unrestricted( "host_writeconfig" );

	return true;
#else
	return false;
#endif
}

bool CStorePanel::HasValidUpsellStamps( void )
{
#ifdef TF_CLIENT_DLL
	int nTotalDonationLevel = 0;

	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );
		if ( pMap->IsCommunityMap() )
		{
			int nDonationLevel = MapInfo_GetDonationAmount( steamapicontext->SteamUser()->GetSteamID().GetAccountID(), pMap->pszMapName );
			nTotalDonationLevel += nDonationLevel;

			// No need to upsell if they've spent $50 on stamps
			if ( nTotalDonationLevel >= 50 )
				return false;

			// No need to upsell this map if they've spent more than $15 on it
			if ( nDonationLevel > 20 )
				continue;

			// No need to upsell this map if they've played it less than an hour
			MapStats_t &mapStats = CTFStatPanel::GetMapStats( pMap->GetStatsIdentifier() );
			int nNumHours = ( mapStats.accumulated.m_iStat[TFMAPSTAT_PLAYTIME] ) / ( 60 /*sec*/ * 60 /*min*/ );

			// No need to upsell this map if they've played it less than 2 hours
			if ( nNumHours <= 1 )
				continue;

			int nRelativeDivisor = nNumHours / 4;
			float flRelativePayoff = ( nRelativeDivisor == 0 ? 0.0f : static_cast< float >( nDonationLevel ) / nRelativeDivisor );

			// No need to upsell this map if they've spend more than $1 per 10 hours
			if ( flRelativePayoff >= 1.0f )
				continue;

			return true;
		}
	}

	return false;
#else
	return false;
#endif
}

void CStorePanel::UpsellStamps( void )
{
#if defined( TF_CLIENT_DLL )
	const MapDef_t *pUpsellMap = NULL;
	float flUpsellRelativePayoff = 1000.0f;
	int nUpsellNumHours = 0;

	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByIndex( i );
		if ( pMap->IsCommunityMap() )
		{
			int nDonationLevel = MapInfo_GetDonationAmount( steamapicontext->SteamUser()->GetSteamID().GetAccountID(), pMap->pszMapName );

			MapStats_t &mapStats = CTFStatPanel::GetMapStats( pMap->GetStatsIdentifier() );
			int nNumHours = ( mapStats.accumulated.m_iStat[TFMAPSTAT_PLAYTIME] ) / ( 60 /*sec*/ * 60 /*min*/ );

			// No need to upsell this map if they've played it less than 2 hours
			if ( nNumHours <= 1 )
				continue;

			int nRelativeDivisor = nNumHours / 4;
			float flRelativePayoff = ( nRelativeDivisor == 0 ? 0.0f : static_cast< float >( nDonationLevel ) / nRelativeDivisor );

			if ( flUpsellRelativePayoff > flRelativePayoff || ( flUpsellRelativePayoff == flRelativePayoff && nUpsellNumHours < nNumHours ) )
			{
				pUpsellMap = pMap;
				flUpsellRelativePayoff = flRelativePayoff;
				nUpsellNumHours = nNumHours;
			}
		}
	}

	Assert( pUpsellMap );
	if ( !pUpsellMap )
	{
		// Should have returned false from ShouldUpsell long before we got here
		return;
	}

	wchar_t *pwchMapName = g_pVGuiLocalize->Find( pUpsellMap->pszMapNameLocKey );

	char szMapHours[ 8 ];
	V_sprintf_safe( szMapHours, "%i", nUpsellNumHours );

	wchar_t wszMapHours[ 8 ];
	g_pVGuiLocalize->ConvertANSIToUnicode( szMapHours, wszMapHours, sizeof( wszMapHours ) );

	wchar_t wchDonationDescription[ 512 ];
	g_pVGuiLocalize->ConstructString_safe( wchDonationDescription, g_pVGuiLocalize->Find( "#Store_ConfirmStampDonationAddText" ), 2, pwchMapName, wszMapHours );
	
	CStampUpsellDialog *pDialog = vgui::SETUP_PANEL( new CStampUpsellDialog( "#Store_ConfirmStampDonationAddTitle", 
		wchDonationDescription, g_pVGuiLocalize->Find( "#Store_ConfirmStampDonationAddText2" ), 
		pUpsellMap->mapStampDef, "World Traveler" ) );

	if ( pDialog )
	{
		pDialog->Show();
		vgui::surface()->PlaySound( "ui/vote_started.wav" );
	}
#else
	return;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to begin a checkout.
//-----------------------------------------------------------------------------
void CStorePanel::InitiateCheckout( bool bSkipUpsell, bool bSkipDecoderWarning /* = false */ )
{
	// If this user's TxnCC is not allowed decodable containers, check if this is a decoder and show them a warning
	//
	// Note this warning goes to ProceedCheckout_DecoderWarning, which loops back with skip set.  The below checks just
	// go straight to confirm.
	const char *pTxnCC = GCClientSystem()->GetTxnCountryCode();
	if ( !bSkipDecoderWarning && pTxnCC && !BEconCountryAllowDecodableContainers( pTxnCC ) &&
	     m_Cart.ContainsChanceRestrictedItems() )
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#Store_ConfirmHolidayRestrictionCheckoutTitle",
		                                                      "#Store_ConfirmDecoderRestrictionCheckoutText",
		                                                      "#Store_OK", "#TF_Back", &ProceedCheckout_DecoderWarning );

		if ( pDialog )
		{
			pDialog->SetContext( this );
		}
		return;
	}

	// Check for holiday-restricted items and confirm with user before allowing checkout
	if ( m_Cart.ContainsHolidayRestrictedItems() )
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#Store_ConfirmHolidayRestrictionCheckoutTitle",  "#Store_ConfirmHolidayRestrictionCheckoutText", "#Store_OK", "#TF_Back", &ConfirmCheckout );
		if ( pDialog )
		{
			pDialog->SetContext( this );
		}
		return;
	}
	else if ( ShouldShowDx8PurchaseWarning( ) )
	{
		CTFGenericConfirmDialog *pDialog = ShowConfirmDialog( "#Store_ConfirmDx8Summer2015OpPassTitle", "#Store_ConfirmDx8Summer2015OpPassText", "#Store_BuyAnyway", "#Store_NoThanks", &ConfirmCheckout );
		if ( pDialog )
		{
			pDialog->SetContext( this );
		}
		return;
	}
	else if ( !bSkipUpsell && ShouldUpsellStamps() )
	{
		UpsellStamps();
		return;
	}

	DoCheckout();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
/*static*/ void CStorePanel::ProceedCheckout_DecoderWarning( bool bConfirmed, void *pContext )
{
	CStorePanel *pStorePanel = ( CStorePanel * )pContext;
	if ( bConfirmed )
	{
		pStorePanel->InitiateCheckout( /* bSkipUpsell */ false, /* bSkipDecoderWarning */ true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
/*static*/ void CStorePanel::ConfirmCheckout( bool bConfirmed, void *pContext )
{
	CStorePanel *pStorePanel = ( CStorePanel * )pContext;
	if ( bConfirmed )
	{
		if ( pStorePanel->ShouldUpsellStamps() )
		{
			pStorePanel->UpsellStamps();
			return;
		}
		else
		{
			pStorePanel->DoCheckout();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
/*static*/ void CStorePanel::ConfirmUpsellStamps( bool bConfirmed, CSchemaItemDefHandle hItemDef, int nSecondsVisible )
{
	CStorePanel *pStorePanel = EconUI()->GetStorePanel();
	if ( bConfirmed )
	{
		// Add to cart
		CStorePage *pPage = dynamic_cast< CStorePage * >( pStorePanel->GetPropertySheet()->GetPage( 3 ) );

		KeyValues *pParams = new KeyValues( "AddItemToCart" );
		pParams->SetInt( "item_def", hItemDef->GetDefinitionIndex() );
		pParams->SetInt( "cart_add_type", kCartItem_Purchase );
		pStorePanel->PostMessage( pPage, pParams );

		pStorePanel->PostMessage( pStorePanel, new KeyValues( "DoCheckout" ), 0.5f );
	}
	else
	{
		CheckButton *pCheckbox = static_cast< CheckButton* >( pStorePanel->FindChildByName( "SupportCommunityMapMakersCheckButton" ) );
		if ( pCheckbox )
		{
			pCheckbox->SetSelected( false );
		}

		pStorePanel->DoCheckout();
	}

#if !defined(NO_STEAM)
	KeyValues *pKVData = new KeyValues( "TF2StoreStampUpsell" );

	// Create and Send the report
	// AccountID, AcceptUpsell, CartItemsCount, CartItemsPrice, CartItemsFlags, SecondsVisible, EventTime

	// ID - Auto

	// AcceptUpsell
	pKVData->SetInt( "AcceptUpsell", bConfirmed ? 1 : 0 );

	// CartItemsCount (up to 100)
	CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
	pKVData->SetInt( "CartItemsCount", pCart ? clamp( pCart->GetNumEntries(), 0, 100 ) : 0 );

	// CartItemsPriceTotal
	pKVData->SetInt( "CartItemsPrice", pCart ? pCart->GetTotalPrice() : 0 );

	// CartItemsFlags (8-bits)
	int nCartItemsFlags = 0;
	if ( pCart )
	{
		for ( int i = 0; i < pCart->GetNumEntries(); ++i )
		{
			cart_item_t *pItem = pCart->GetItem( i );
			if ( !pItem || !pItem->pEntry )
				continue;

			if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Weapons ) )			nCartItemsFlags |= 0x0001;
//			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Headgear ) )	nCartItemsFlags |= 0x0002;			// DEPRECATED
//			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Misc ) )		nCartItemsFlags |= 0x0004;			// DEPRECATED
			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Tools ) )		nCartItemsFlags |= 0x0008;
			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Maps ) )		nCartItemsFlags |= 0x0010;
			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Bundles ) )	nCartItemsFlags |= 0x0020;
			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_New ) )		nCartItemsFlags |= 0x0040;
			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Limited ) )	nCartItemsFlags |= 0x0080;
			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Cosmetics ) )	nCartItemsFlags |= 0x0100;
			else if ( pItem->pEntry->IsListedInCategory( CEconStoreCategoryManager::k_CategoryID_Taunts ) )		nCartItemsFlags |= 0x0200;
		}
	}

	pKVData->SetInt( "CartItemsFlags", nCartItemsFlags );

	// SecondsVisible (up to 2 minutes)
	pKVData->SetInt( "SecondsVisible", clamp( nSecondsVisible, 0, 120 ) );

	// EventTime
	pKVData->SetInt( "EventTime", GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() );

	// Send to DB
	GetSteamWorksSGameStatsUploader().AddStatsForUpload( pKVData );
#endif // !defined(NO_STEAM)
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CStorePanel::DoCheckout()
{
	m_iCheckoutAttempts++;
	EconUI()->Gamestats_Store( IE_STORE_CHECKOUT_ATTEMPT, NULL, NULL, 0, NULL, m_iCheckoutAttempts );

	// Create the checkout job.
	OpenStoreStatusDialog( NULL, "#StoreCheckout_Loading", true, false, true );
	CGCClientJobInitPurchase *pJob = new CGCClientJobInitPurchase( GCClientSystem()->GetGCClient() );
	pJob->StartJob( NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void ShowPurchaseInitError( const char *pszError )
{
	OpenStoreStatusDialog( NULL, pszError, true, false );
	EconUI()->Gamestats_Store( IE_STORE_CHECKOUT_FAILURE, NULL, NULL, 0, NULL, EconUI()->GetStorePanel()->GetCheckoutAttempts(), pszError );
}

//-----------------------------------------------------------------------------
// Purpose: Asynchronous job for initiating a checkout from the Steam store.
//-----------------------------------------------------------------------------
bool CGCClientJobInitPurchase::BYieldingRunJob( void *pvStartParam )
{
	GCSDK::CProtoBufMsg<CMsgGCStorePurchaseInit> msg( k_EMsgGCStorePurchaseInit );
	GCSDK::CProtoBufMsg<CMsgGCStorePurchaseInitResponse> msgResponse;

	if  ( !EconUI()->GetStorePanel() )
	{
		// This won't happen under the normal process of using the UI.
		OpenStoreStatusDialog( NULL, "#StoreCheckout_Unavailable", true, false );
		return true;
	}

	CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
	char uilanguage[ 64 ];
	uilanguage[0] = 0;
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

	// Populate the message.
	msg.Body().set_currency( EconUI()->GetStorePanel()->GetCurrency() );
	msg.Body().set_country( EconUI()->GetStorePanel()->GetCountryCode() );
	msg.Body().set_language( PchLanguageToELanguage( uilanguage ) );

	// We need to ensure there are more than 0 items in the cart, but no more than MAX_CART_ITEMS.
	int total_items = pCart->GetTotalItems();
	if ( total_items <= 0 )
	{
		ShowPurchaseInitError( "#StoreCheckout_NoItems" );
		return true;
	}

	if ( total_items > MAX_CART_ITEMS )
	{
		ShowPurchaseInitError( "#StoreCheckout_TooManyItems" );
		return true;
	}

	// Can the items we want to buy actually fit inside our backpack?
	// This check will include items that we have discovered, but haven't been revealed to the player yet.
	// So they will sometimes get this with apparently empty slots in their backpack.
	if ( !InventoryManager()->GetLocalInventory()->CanPurchaseItems( pCart->GetTotalConcreteItems() ) )
	{
		const bool bInventoryIsAtMaxSize = InventoryManager()->GetLocalInventory()->GetMaxItemCount() == MAX_NUM_BACKPACK_SLOTS;

		ShowPurchaseInitError( bInventoryIsAtMaxSize ? "#StoreCheckout_NotEnoughRoom_MaxSize" : "#StoreCheckout_NotEnoughRoom" );
		return true;
	}

	// Add the items we are requesting.
	int totalPrice = 0;
	for ( int i=0; i<pCart->GetNumEntries(); i++ )
	{
		cart_item_t *pCartItem = pCart->GetItem( i );

		CGCStorePurchaseInit_LineItem *pNewLineItem = msg.Body().add_line_items();

		pNewLineItem->set_item_def_id( pCartItem->pEntry->GetItemDefinitionIndex() );
		pNewLineItem->set_quantity( pCartItem->iQuantity );
		pNewLineItem->set_purchase_type( pCartItem->eType );

		int itemPrice = pCartItem->iQuantity * pCartItem->pEntry->GetCurrentPrice( EconUI()->GetStorePanel()->GetCurrency() );
		pNewLineItem->set_cost_in_local_currency( itemPrice );

		totalPrice += itemPrice;
	}
	EconUI()->GetStorePanel()->SetLastPurchaseAttemptPrice( totalPrice );

	// Request to init this purchase.
	if ( !BYldSendMessageAndGetReply( msg, 30, &msgResponse, k_EMsgGCStorePurchaseInitResponse ) )
	{
		// No transaction has been initialized. The GC isn't responding, so abort.
		ShowPurchaseInitError( "#StoreCheckout_Unavailable" );
		return false;
	}

#ifdef _DEBUG
	Msg( "CGCClientJobInitPurchase - Result: %d, TxnID: %llu\n", msgResponse.Body().result(), (unsigned long long) msgResponse.Body().txn_id());
#endif

	// If we fail at this point Steam hasn't opened a transaction that we need to worry about.
	if ( !CStorePanel::CheckMessageResult( (EPurchaseResult)msgResponse.Body().result() ) )
		return false;

	EconUI()->GetStorePanel()->SetTransactionID( msgResponse.Body().txn_id() );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Immediately add the item corresponding to the item def passed into
//			the cart and checkout.  This doesn't required opening the store front
//			so this can be called anywhere.
//-----------------------------------------------------------------------------
void CStorePanel::AddToCartAndCheckoutImmediately( item_definition_index_t nDefIndex )
{
	if ( GetPriceSheet() && GetCart() && steamapicontext && steamapicontext->SteamUser() )
	{
		// Add a the item to the users cart and checkout
		GetCart()->EmptyCart();
		AddItemToCartHelper( NULL, nDefIndex, kCartItem_Purchase );
		DoCheckout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Cancels a pending transaction, if possible.
//-----------------------------------------------------------------------------
void CStorePanel::CheckoutCancel( void )
{
	// The player pressed the CANCEL button on the TF2 GAME UI pop-up that appears over
	// the store interface while they would be occupied with the overlay.
	//
	// If they press the CANCEL button on the overlay's authorize dialog a Steam callback 
	// (OnMicroTransactionAuthResponse) happens not this method.
	//
	// We don't expect this to happen! Once the transaction has been finalized we have
	// a transaction ID and the GC and Steam are already doing the negotiations about
	// where the money comes from and removing it so we can't back out. Allowing users
	// to click this button results in a race condition that often results in users
	// getting their money taken with no items because the GC can't resolve the
	// "Canceled"/"Succeeded" discrepancy.
	Assert( GetTransactionID() <= 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to cancel a purchase in progress.
//-----------------------------------------------------------------------------
bool CGCClientJobCancelPurchase::BYieldingRunJob( void *pvStartParam )
{
	GCSDK::CProtoBufMsg<CMsgGCStorePurchaseCancel> msg( k_EMsgGCStorePurchaseCancel );
	GCSDK::CProtoBufMsg<CMsgGCStorePurchaseCancelResponse> msgResponse;

	if  ( !EconUI()->GetStorePanel() )
	{
		OpenStoreStatusDialog( NULL, "#StoreCheckout_Unavailable", true, false );
		return false;
	}

	msg.Body().set_txn_id( m_ulTxnID );

	if ( !BYldSendMessageAndGetReply( msg, 10, &msgResponse, k_EMsgGCStorePurchaseCancelResponse ) )
	{
		// No response from the GC. Show a failure message.
		OpenStoreStatusDialog( NULL, "#StoreUpdate_NoGCResponse", true, false );
		return false;
	}

#ifdef _DEBUG
	Msg( "CGCClientJobCancelPurchase Result: %d\n", msgResponse.Body().result() );
#endif

	// The current transaction has been canceled with the GC.
	EconUI()->GetStorePanel()->SetTransactionID( 0 );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the player completes or cancels an in-progress transaction.
//-----------------------------------------------------------------------------
void CStorePanel::OnMicroTransactionAuthResponse( MicroTxnAuthorizationResponse_t *pMicroTxnAuthResponse )
{
	Assert( steamapicontext->SteamUserStats() );
	if ( !steamapicontext->SteamUserStats() )
		return;

	if ( !pMicroTxnAuthResponse->m_bAuthorized )
	{
		const char* pszError = "#StoreCheckout_TransactionCanceled";
		EconUI()->Gamestats_Store( IE_STORE_CHECKOUT_FAILURE, NULL, NULL, 0, NULL, EconUI()->GetStorePanel()->m_iCheckoutAttempts, pszError );
		OpenStoreStatusDialog( NULL, pszError, true, false );
		CGCClientJobCancelPurchase *pJob = new CGCClientJobCancelPurchase( GCClientSystem()->GetGCClient(), GetTransactionID() );
		pJob->StartJob( NULL );
		SetTransactionID( 0 );
	}
	else
	{
		// Replace the existing dialog with one that says "we're finalizing!" and only has an "OK" button.
		// We let users close this if they want, which can be useful in exceptional circumstances like the
		// GC crashing, but we don't have them do any work on the GC side -- once the finalization is in
		// flight we can't take it back.
		OpenStoreStatusDialog( NULL, "#StoreCheckout_TransactionFinalizing", true, false, false );

		// Finalize the transaction with the GC.
		m_bShouldFinalize = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look to see if we should finalize the open transaction.
//-----------------------------------------------------------------------------
void CStorePanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( m_bShouldFinalize )
	{
		m_bShouldFinalize = false;
		FinalizeTransaction();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePanel::FinalizeTransaction( void )
{
	// Tell the GC to release the items.
	CGCClientJobFinalizePurchase *pJob = new CGCClientJobFinalizePurchase( GCClientSystem()->GetGCClient(), GetTransactionID() );
	pJob->StartJob( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Tell the GC that the purchase is finalized and we should create the items the player bought.
// TODO: If something goes wrong here we need to inform the player, but tell them
// that they MAY have been charged. If STEAM authorized & completed the transaction,
// but we weren't able to finalize with the GC we won't get our items right away
// but the player will have been charged.
//-----------------------------------------------------------------------------
bool CGCClientJobFinalizePurchase::BYieldingRunJob( void *pvStartParam )
{
	GCSDK::CProtoBufMsg<CMsgGCStorePurchaseFinalize> msg( k_EMsgGCStorePurchaseFinalize );
	GCSDK::CProtoBufMsg<CMsgGCStorePurchaseFinalizeResponse> msgResponse;

	msg.Body().set_txn_id( m_ulTxnID );

	if ( !BYldSendMessageAndGetReply( msg, 10, &msgResponse, k_EMsgGCStorePurchaseFinalizeResponse ) )
	{
		// TODO: This is bad! The store might have taken our money, but we weren't able to finalize. How do we handle this?
		// The message currently says "Unable to confirm success. If successful, your items will be delivered at a later date."
		// We need to handle this case more gracefully.
		OpenStoreStatusDialog( NULL, "#StoreCheckout_CompleteButUnfinalized", true, false );
		// @note Tom Bui & Joe Ludwig: We empty the cart here, just to make sure people don't hit checkout again
		// and end up with dupes of all their items
		if ( EconUI()->GetStorePanel() && EconUI()->GetStorePanel()->GetCart() )
		{
			EconUI()->GetStorePanel()->GetCart()->EmptyCart();
		}
		return false;
	}

	// Check the message result for errors and handle them.
	if ( !CStorePanel::CheckMessageResult( (EPurchaseResult)msgResponse.Body().result() ) )
		return false; 

	EconUI()->Gamestats_Store( IE_STORE_CHECKOUT_SUCCESS, NULL, NULL, 0, NULL, EconUI()->GetStorePanel()->GetCheckoutAttempts(), NULL, EconUI()->GetStorePanel()->GetLastPurchaseAttemptPrice(), EconUI()->GetStorePanel()->GetCurrency()+1 );
	CStoreCart* cart = EconUI()->GetStorePanel()->GetCart();
	for ( int i=0; i<cart->GetNumEntries(); ++i )
	{
		cart_item_t* item = cart->GetItem( i );
		if ( !item )
			continue;
		EconUI()->Gamestats_Store( IE_STORE_CHECKOUT_ITEM, NULL, NULL, 0, item, EconUI()->GetStorePanel()->GetCheckoutAttempts(), NULL, 0, EconUI()->GetStorePanel()->GetCurrency()+1 );
	}

#ifdef DEBUG
	Msg( "CGCClientJobFinalizePurchase Result: %d, Num Items: %i, Purchased Items:\n", msgResponse.Body().result(), msgResponse.Body().item_ids_size() );
	if ( k_EPurchaseResultOK == msgResponse.Body().result() )
	{
		for ( int i = 0; i < msgResponse.Body().item_ids_size(); i++ )
		{
			Msg( "\t%llu\n", (unsigned long long) msgResponse.Body().item_ids(i) );
		}
	}
#endif

	EconUI()->GetStorePanel()->SetMostRecentSuccessfulTransactionID( m_ulTxnID );

	// Transaction complete.
	EconUI()->GetStorePanel()->SetTransactionID( 0 );

	// Clear the cart.
	EconUI()->GetStorePanel()->GetCart()->EmptyCart();

	// If we were in the cart view, return to the store page
	CStoreViewCartPanel *pCartPanel = GetStoreViewCartPanel();
	if ( pCartPanel && pCartPanel->IsVisible() )
	{
		pCartPanel->ShowPanel( false );
	}

	// Let them know everything went well.
	OpenStoreStatusDialog( NULL, "#StoreCheckout_TransactionCompleted", true, true );

	EconUI()->GetStorePanel()->PostTransactionCompleted();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Handle an error from the GC.
//-----------------------------------------------------------------------------
bool CStorePanel::CheckMessageResult( EPurchaseResult msgResult )
{
	// Take action on the result code.
	switch ( msgResult )
	{
	case k_EPurchaseResultOK:
		return true;
		break;

		// GC / Steam errors: // These can all be combined into a single general unrecoverable error case.
	case k_EPurchaseResultFail:	// Generic error.
	case k_EPurchaseResultInvalidParam: // Invalid parameter.
		if ( CStorePanel::ShouldShowWarnings() )
		{
			OpenStoreStatusDialog( NULL, "#StoreCheckout_Unavailable", true, false );
		}
		break;

	// Internal error. Default string in English: "Unable to confirm success. If successful, your items will be
	// delivered at a later date." Basically "something went real bad wrong and we don't know whether you'll get
	// your items or not".
	case k_EPurchaseResultInternalError:
		if ( CStorePanel::ShouldShowWarnings() )
		{
			OpenStoreStatusDialog( NULL, "#StoreCheckout_CompleteButUnfinalized", true, false );
		}
		break;

	// Is the GC telling us this user does not have enough backpack space?  This check takes into account
	// any additional backpack space a user might get from upgrading to premium.
	case k_EPurchaseResultNotEnoughBackpackSpace:
		if ( CStorePanel::ShouldShowWarnings() )
		{
			OpenStoreStatusDialog( NULL, "#StoreCheckout_NotEnoughRoom", true, false );
		}
		break;

	case k_EPurchaseResultLimitedQuantityItemsUnavailable:
		if ( CStorePanel::ShouldShowWarnings() )
		{
			OpenStoreStatusDialog( NULL, "#StoreCheckout_LimitedQuantityItemsUnavailable", true, false );
		}
		break;

		// errors that should never happen
	case k_EPurchaseResultNotApproved: // Tried to finalize a transaction that has not yet been approved.
	case k_EPurchaseResultAlreadyCommitted: // Tried to finalize a transaction that has already been committed.
	case k_EPurchaseResultWrongCurrency: // Microtransaction's currency does not match user's wallet currency.
	case k_EPurchaseResultAccountError: // User's account does not exist or is temporarily unavailable.
	case k_EPurchaseResultTxnNotFound: // Could not find the transaction specified
	default:
		OpenStoreStatusDialog( NULL, "#StoreCheckout_InternalError", true, false );
		break;

	case k_EMicroTxnResultFailedFraudChecks: // Steam thinks this transaction might be fraudulent
		ShowConfirmDialog( "#StoreCheckout_ContactSupport_Dialog_Title", "#StoreCheckout_ContactSupport", "#StoreCheckout_ContactSupport_Dialog_Btn", "#Cancel", &ContactSupportConfirm );
		break;

		// User errors:
	case k_EPurchaseResultUserNotLoggedIn: // User is not logged into Steam.
		OpenStoreStatusDialog( NULL, "#StoreCheckout_NotLoggedIn", true, false );
		break;
	case k_EPurchaseResultInsufficientFunds: // User does not have wallet funds
		OpenStoreStatusDialog( NULL, "#StoreCheckout_InsufficientFunds", true, false );
		// This will happen if the user spends the money out of his wallet between funding & finalizing the transaction.
		break;
	case k_EPurchaseResultTimedOut: // Time limit for finalization has been exceeded
		OpenStoreStatusDialog( NULL, "#StoreCheckout_TimedOut", true, false );
		// TODO: We should find out what happened to the transaction, since it still may be viable.
		break;
	case k_EPurchaseResultAcctDisabled: // Steam account is disabled.
		OpenStoreStatusDialog( NULL, "#StoreCheckout_SteamAccountDisabled", true, false );
		break;
	case k_EPurchaseResultAcctCannotPurchase: // Steam account is not allowed to make a purchase
		OpenStoreStatusDialog( NULL, "#StoreCheckout_SteamAccountNoPurchase", true, false );
		break;

		// Client state discrepancies:
	case k_EPurchaseResultOldPriceSheet: // Information on the purchase didn't match the current price sheet
		OpenStoreStatusDialog( NULL, "#StoreCheckout_OldPriceSheet", false, false );

		// Request a new price sheet. This will clear the store pages and the user's cart.
		CStorePanel::RequestPricesheet();
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Shows the store panel.
//-----------------------------------------------------------------------------
void CStorePanel::ShowStorePanel( void )
{
	GetPropertySheet()->SetVisible( true );
	m_bShowWarnings = true;

	InvalidateLayout( false, true );
	Activate();

	if ( !m_bPricesheetLoaded )
		return;

	bool bStartOnHomePage = true;

	if ( m_bAddStartItemDefToCart )
	{
		// Put the specified item in the cart, and go to the cart display screen.
		GetCart()->EmptyCart();

		CStorePage *pPage = dynamic_cast< CStorePage * >( GetPropertySheet()->GetActivePage() );
		AddItemToCartHelper( pPage ? pPage->GetPageName() : NULL, m_bAddStartItemDefToCart, kCartItem_Purchase );

		if ( pPage )
		{
			pPage->UpdateCart();
		}
	}
	else
	{
		if ( m_iStartItemDef == STOREPANEL_SHOW_UPGRADESTEPS )
		{
			OpenStoreStatusDialog( NULL, "#TF_Trial_StoreUpgradeExplanation", true, false );
		}
		else if ( m_iStartItemDef )
		{
			const econ_store_entry_t *pEntry = FindEntryForItemDef( m_iStartItemDef );
			if ( pEntry )
			{
				// !KLUDGE! Post this to a message queue to be handled later, because there
				// have already been messages posted to scroll to other pages.
				// VGUI really has a pretty systematic problem of posting WAY too much stuff
				// to a queue, instead of dispatching it immediately
				ivgui()->PostMessage( GetVPanel(), new KeyValues("JumpToItem", "ItemDefIndex", m_iStartItemDef), GetVPanel() );
				//FindAndSelectEntry( pEntry );
				bStartOnHomePage = false;
			}
		}

		CExButton *pCloseButton = dynamic_cast<CExButton*>( FindChildByName("CloseButton") );
		if ( pCloseButton )
		{
			pCloseButton->RequestFocus();
		}
	}
	m_iStartItemDef = 0;

	if ( bStartOnHomePage )
	{
		vgui::Panel *pPage = GetPropertySheet()->GetPage(0);
		if ( pPage )
		{
			if ( GetPropertySheet()->GetActivePage() != pPage )
			{
				GetPropertySheet()->SetActivePage( pPage );
			}
			else
			{
				// VGUI doesn't tell the starting active page that it's active, so we post a pageshow to it
				ivgui()->PostMessage( pPage->GetVPanel(), new KeyValues("PageShow"), GetPropertySheet()->GetVPanel() );
			}
		}
	}

	vgui::Panel *pPage = GetPropertySheet()->GetActivePage();
	if ( pPage )
	{
		pPage->SetVisible( true );
	}

	// 6/13/2011
	// Note: We no longer check for new items when the player enters
	// the store. This is confusing and no longer necessary now that
	// the notification system lets us push item discoveries to the player.
	// InventoryManager()->ShowItemsPickedUp( true, false );
}

void CStorePanel::OnJumpToItem( KeyValues *pParams )
{
	const econ_store_entry_t *pEntry = FindEntryForItemDef( pParams->GetInt( "ItemDefIndex", -1 ) );
	Assert( pEntry );
	if ( pEntry )
	{
		FindAndSelectEntry( pEntry );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Open_Store( const CCommand &args )
{
	int iItemDef = ( args.ArgC() > 1 ) ? atoi(args[1]) : 0;
	bool bToFeatured = ( ( ( args.ArgC() > 2 ) ? atoi(args[2]) : 0 ) != 0 );

	EconUI()->OpenStorePanel( iItemDef, bToFeatured );	
}
ConCommand open_store( "open_store", Open_Store, "Open the in-game store", FCVAR_NONE );

//==================================================================================================
// CART
//==================================================================================================
CStoreCart::CStoreCart( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreCart::AddToCart( const econ_store_entry_t *pEntry, const char* pszPageName, ECartItemType eCartItemType )
{
	// Steam doesn't allow purchases with more than 256 items in a single transaction
	if ( GetTotalItems() >= 255 )
		return;

	if ( pEntry->m_bIsMarketItem )
	{
		CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( pEntry->GetItemDefinitionIndex() );
		Assert( pItemDef );
		if ( !pItemDef )
			return;

		if ( !CBaseAdPanel::CheckForRequiredSteamComponents( "#StoreUpdate_SteamRequired", "#MMenu_OverlayRequired" ) )
			return;

		if ( pItemDef && steamapicontext && steamapicontext->SteamFriends() )
		{
			const char *pszPrefix = "";
			if ( GetUniverse() == k_EUniverseBeta )
			{
				pszPrefix = "beta.";
			}

			static char pszItemName[256];
			g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( pItemDef->GetItemBaseName() ), pszItemName, sizeof( pszItemName ) );

			char szURL[512];
			V_sprintf_safe( szURL, "http://%ssteamcommunity.com/market/listings/%d/%s", pszPrefix, engine->GetAppID(), pszItemName );
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );
		}
		return;
	}

	int iIndex = GetIndexForEntry( pEntry, eCartItemType );
	if ( iIndex == m_Items.InvalidIndex() )
	{
		iIndex = m_Items.AddToTail();
		m_Items[iIndex].pEntry = pEntry;
		m_Items[iIndex].iQuantity = 0;
		m_Items[iIndex].eType = eCartItemType;
		m_Items[iIndex].bPreviewItem = ( eCartItemType == kCartItem_TryOutUpgrade );
	}

	m_Items[iIndex].iQuantity++;

	EconUI()->Gamestats_Store( IE_STORE_ITEM_ADDED_TO_CART, NULL, pszPageName, 0, &m_Items[iIndex] );

	IGameEvent *event = gameeventmanager->CreateEvent( "cart_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}

	vgui::surface()->PlaySound( "ui/item_store_add_to_cart.wav" );

	// find the cart button associated with the active page we are using
	CExButton *pCartButton = dynamic_cast< CExButton * >( EconUI()->GetStorePanel()->GetActivePage()->FindChildByName( "CartButton", true ) );
	if ( pCartButton )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pCartButton->GetParent(), "AddToCartBlink" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreCart::RemoveFromCart( int iIndex )
{
	if ( ( iIndex >= 0 ) && ( iIndex < m_Items.Count() ) )
	{
		int iIndexToDelete = iIndex;
		CEconItemDefinition *pItemDef = NULL;

		// play item's "drop" sound
		if ( m_Items[iIndexToDelete].pEntry )
		{
			pItemDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_Items[iIndexToDelete].pEntry->GetItemDefinitionIndex() );
			const char *soundFilename = pItemDef->GetDefinitionString( "drop_sound", "ui/item_default_drop.wav" );

			vgui::surface()->PlaySound( soundFilename );
		}

		// before we remove this item, let's see if the item is the preview item 
		// and we have a similar item in the cart that is not being previewed (the
		// previewed item will be at a discount so we want to keep the lower priced item)
		if ( m_Items[iIndexToDelete].bPreviewItem && pItemDef )
		{
			FOR_EACH_VEC( m_Items, i )
			{
				// don't compare against item we're removing
				if ( ( i != iIndexToDelete ) && m_Items[i].pEntry )
				{
					CEconItemDefinition *pTempDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_Items[i].pEntry->GetItemDefinitionIndex() );
					if ( pTempDef && ( pItemDef->GetDefinitionIndex() == pTempDef->GetDefinitionIndex() ) )
					{
						iIndexToDelete = i;
						break;
					}
				}
			}
		}

		m_Items[iIndexToDelete].iQuantity--;

		EconUI()->Gamestats_Store( IE_STORE_ITEM_REMOVED_FROM_CART, NULL, NULL, 0, &m_Items[iIndexToDelete] );

		if ( m_Items[iIndexToDelete].iQuantity <= 0 )
		{
			m_Items.Remove( iIndexToDelete );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "cart_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreCart::EmptyCart( void )
{
	m_Items.Purge();

	IGameEvent *event = gameeventmanager->CreateEvent( "cart_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CStoreCart::GetIndexForEntry( const econ_store_entry_t *pEntry, ECartItemType eCartItemType ) const
{
	FOR_EACH_VEC( m_Items, i )
	{
		if ( m_Items[i].pEntry == pEntry && m_Items[i].eType == eCartItemType )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CStoreCart::GetTotalItems( void ) const
{
	int iTotal = 0;
	FOR_EACH_VEC( m_Items, i )
	{
		const cart_item_t &item = m_Items[i];
		iTotal += item.iQuantity;
	}
	return iTotal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CStoreCart::GetTotalConcreteItems( void ) const
{
	int iTotal = 0;
	FOR_EACH_VEC( m_Items, i )
	{
		const cart_item_t &item = m_Items[i];
#ifdef TF_CLIENT_DLL
		CEconItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( item.pEntry->GetItemDefinitionIndex() );
		iTotal += pDef->GetNumConcreteItems() * item.iQuantity;
#else
		// Kyle says: this logic is totally wrong but we don't *have* a CStrike
		//			  economy so I don't feel bad using this to get the build working.
		iTotal += item.iQuantity;
#endif
	}
	return iTotal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
item_price_t cart_item_t::GetDisplayPrice() const
{
	const float flDiscount = eType == kCartItem_TryOutUpgrade
							? GetEconPriceSheet()->GetPreviewPeriodDiscount()
							: IsRentalCartItemType( eType )
							? pEntry->GetRentalPriceScale()
							: 100.0f;

	const ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();
	item_price_t unDisplayPrice = (item_price_t)( pEntry->GetCurrentPrice( eCurrency ) );

	if ( ( flDiscount < 100.0f ) && ( flDiscount > 0.0f ) )
	{
		unDisplayPrice = econ_store_entry_t::CalculateSalePrice( unDisplayPrice, eCurrency, flDiscount );
	}

	return ( unDisplayPrice * iQuantity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
item_price_t CStoreCart::GetTotalPrice( void ) const
{
	item_price_t unTotal = 0;
	FOR_EACH_VEC( m_Items, i )
	{
		unTotal += m_Items[i].GetDisplayPrice();
	}
	return unTotal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CStoreCart::ContainsChanceRestrictedItems() const
{
	FOR_EACH_VEC( m_Items, i )
	{
		CEconItemDefinition *pEconDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_Items[i].pEntry->GetItemDefinitionIndex() );
		if ( !pEconDef )
			continue;

		const GameItemDefinition_t *pItemDef = dynamic_cast<const GameItemDefinition_t *>( pEconDef );

		// All decoder ring items
		if ( pItemDef && pItemDef->GetEconTool() &&
		     ( Q_strcmp( pItemDef->GetEconTool()->GetTypeName(), "decoder_ring" ) == 0 ) )
		{
			return true;
		}

		// Explicitly flagged items
		if ( pItemDef && pItemDef->IsChanceRestricted() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CStoreCart::ContainsHolidayRestrictedItems() const
{
	FOR_EACH_VEC( m_Items, i )
	{
		CEconItemDefinition *pItemDef = ItemSystem()->GetStaticDataForItemByDefIndex( m_Items[i].pEntry->GetItemDefinitionIndex() );
		if ( !pItemDef )
			continue;

		// If the string isn't empty, assume it has a restriction
		if ( pItemDef->GetHolidayRestriction() )
			return true;

		const bundleinfo_t *pBundle = pItemDef->GetBundleInfo();
		if ( pBundle )
		{
			FOR_EACH_VEC( pBundle->vecItemDefs, j )
			{
				if ( pBundle->vecItemDefs[j]->GetHolidayRestriction() )
					return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CStoreCart::ContainsItemDefinition( item_definition_index_t unItemDef ) const
{
	FOR_EACH_VEC( m_Items, i )
	{
		if ( m_Items[i].pEntry->GetItemDefinitionIndex() == unItemDef )
			return true;
	}

	return false;
}

//================================================================================================================================
// STORE STATUS DIALOG
//================================================================================================================================
static vgui::DHANDLE<CStoreStatusDialog> g_StoreStatusPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStoreStatusDialog::CStoreStatusDialog( vgui::Panel *pParent, const char *pElementName ) : BaseClass( pParent, "StoreStatusDialog" )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );
	m_bNotifyOnCancel = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreStatusDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/econ/store/v1/StoreStatusDialog.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreStatusDialog::OnCommand( const char *command )
{
	bool bClose = false;

	if ( !Q_stricmp( command, "close" ) )
	{
		bClose = true;

		if ( m_bShowOnExit )
		{
			InventoryManager()->ShowItemsPickedUp( true );
		}
	}
	else if ( !Q_stricmp( command, "forceclose" ) )
	{
		bClose = true;
	}

	if ( bClose )
	{
		if ( m_bNotifyOnCancel )
		{
			EconUI()->GetStorePanel()->CheckoutCancel();
			m_bNotifyOnCancel = false;
		}

		m_bShowOnExit = false;
		TFModalStack()->PopModal( this );
		SetVisible( false );
		MarkForDeletion();
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreStatusDialog::UpdateSchemeForVersion()
{
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreStatusDialog::ShowStatusUpdate( bool bAllowClose, bool bShowOnExit, bool bCancel )
{
	CExButton *pButton = dynamic_cast<CExButton*>( FindChildByName("CloseButton") );
	if ( pButton )
	{
		pButton->SetVisible( bAllowClose );
		pButton->SetEnabled( bAllowClose );
		if ( bCancel )
		{
			pButton->SetText( "#Store_CANCEL" );
			m_bNotifyOnCancel = true;
		}
		else
		{
			pButton->SetText( "#Store_OK" );
			m_bNotifyOnCancel = false;
		}
	}

	m_bShowOnExit = bShowOnExit;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SetupStoreStatusDialog( vgui::Panel *pParent )
{
	// If we parent it to something, we get problems when another panel pops over it, 
	// because the modal dialog is no longer visible. So prevent parenting status dialogs.
	pParent = NULL;

	if ( !g_StoreStatusPanel.Get() )
	{
		g_StoreStatusPanel = vgui::SETUP_PANEL( new CStoreStatusDialog( pParent, NULL ) );
	}
	g_StoreStatusPanel->SetVisible( true );
	if ( !pParent )
	{
		g_StoreStatusPanel->MakePopup();
	}

	g_StoreStatusPanel->MoveToFront();
	g_StoreStatusPanel->SetKeyBoardInputEnabled(true);
	g_StoreStatusPanel->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( g_StoreStatusPanel );
}

void OpenStoreStatusDialog( vgui::Panel *pParent, const char *pszText, bool bAllowClose, bool bShowOnExit, bool bCancel )
{
	// Figure out who we should be parented to
	if ( !pParent )
	{
		CStoreViewCartPanel *pCartPanel = GetStoreViewCartPanel();
		if ( pCartPanel && pCartPanel->IsVisible() )
		{
			pParent = pCartPanel;
		}
		else if ( EconUI()->GetStorePanel() && EconUI()->GetStorePanel()->IsVisible() )
		{
			pParent = EconUI()->GetStorePanel();
		}
	}

	SetupStoreStatusDialog( pParent );
	g_StoreStatusPanel->UpdateSchemeForVersion();
	g_StoreStatusPanel->SetDialogVariable( "updatetext", g_pVGuiLocalize->Find( pszText ) );
	g_StoreStatusPanel->ShowStatusUpdate( bAllowClose, bShowOnExit, bCancel );
}

void CloseStoreStatusDialog( void )
{
	if ( g_StoreStatusPanel )
	{
		g_StoreStatusPanel->OnCommand( "forceclose" );
	}
}

#ifdef _DEBUG
CON_COMMAND( re_add_store_page, "" )
{
	CStorePanel *pStorePanel = EconUI()->GetStorePanel();
	if ( !pStorePanel )
		return;

	if ( args.ArgC() != 2 )
	{
		Warning( "Need a page index argument.\n" );
		return;
	}

	pStorePanel->ReAddPage( atoi( args[ 1 ] ) );
}
#endif

// these are disabled because they don't work anymore.
#ifdef ENABLE_TEST_PURCHASE_COMMANDS

//================================================================================================================================
//================================================================================================================================
//================================================================================================================================
// TEST JOBS
//================================================================================================================================
//================================================================================================================================
//================================================================================================================================




CON_COMMAND( store_getuserdata, "Gets the latest pricesheet from the GC" )
{
	RTime32 rTimeVersion = 0;
	if ( args.ArgC() > 1 )
	{
		rTimeVersion = V_atoi( args[1] );
	}

	CGCClientJobTESTGetUserData *pJob = new CGCClientJobTESTGetUserData( GCClientSystem()->GetGCClient(), rTimeVersion );
	pJob->StartJob( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Dev-only command and job for initiating a purchase
//-----------------------------------------------------------------------------
class CGCClientJobTESTInitPurchase : public GCSDK::CGCClientJob
{
public:
	CGCClientJobTESTInitPurchase( GCSDK::CGCClient *pGCClient ) : GCSDK::CGCClientJob( pGCClient ) {}
	virtual bool BYieldingRunJob( void *pvStartParam )
	{
		GCSDK::CGCMsg<MsgGCStorePurchaseInit_t> msg( k_EMsgGCStorePurchaseInit );
		GCSDK::CGCMsg<MsgGCStorePurchaseInitResponse_t> msgResponse;

		CStorePanel *pStore = GetStorePanel();
		if  ( !pStore )
		{
			Msg( "store_initpurchase: Store has not been initialized\n" );
			return false;
		}

		CStoreCart *pCart = pStore->GetCart();

		msg.Body().m_eCurrency = pStore->GetCurrency();
		V_strncpy( msg.Body().m_rgchCountry, pStore->GetCountryCode(), sizeof( msg.Body().m_rgchCountry ) );
		msg.Body().m_eLanguage = steamapicontext->SteamApps() ? PchLanguageToELanguage( steamapicontext->SteamApps()->GetCurrentGameLanguage() ) : k_Lang_English;
		msg.Body().m_cLineItems = pCart->GetNumEntries();

		// We really should check for zero items here and not let the purchase go through.
		// Also, GetTotalItems() needs to be < 256

		for ( int i = 0; i < pCart->GetNumEntries(); i++ )
		{
			cart_item_t *pCartItem = pCart->GetItem( i );
			msg.AddUint16Data( pCartItem->pEntry->m_usDefIndex );
			msg.AddUint8Data( pCartItem->iQuantity );
			msg.AddUint16Data( pCartItem->iQuantity * pCartItem->pEntry->GetPrice( (ECurrency)msg.Body().m_eCurrency ) );
		}

		if ( !BYldSendMessageAndGetReply( msg, 10, &msgResponse, k_EMsgGCStorePurchaseInitResponse ) )
		{
			Msg( "store_initpurchase: No response from the GC\n" );
			return false;
		}

		Msg( "Got response. Result: %d, TxnID: %llu\n", msgResponse.Body().m_eResult, msgResponse.Body().m_unTxnID );

		return true;
	}
};

CON_COMMAND( store_initpurchase, "Simulates pressing the checkout button in the store" )
{
	CGCClientJobTESTInitPurchase *pJob = new CGCClientJobTESTInitPurchase( GCClientSystem()->GetGCClient() );
	pJob->StartJob( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Dev-only command and job for canceling a purchase
//-----------------------------------------------------------------------------
class CGCClientJobTESTCancelPurchase : public GCSDK::CGCClientJob
{
public:
	CGCClientJobTESTCancelPurchase( GCSDK::CGCClient *pGCClient, uint64 ulTxnID ) : GCSDK::CGCClientJob( pGCClient ), m_ulTxnID( ulTxnID ) {}
	virtual bool BYieldingRunJob( void *pvStartParam )
	{
		GCSDK::CGCMsg<MsgGCStorePurchaseCancel_t> msg( k_EMsgGCStorePurchaseCancel );
		GCSDK::CGCMsg<MsgGCStorePurchaseCancelResponse_t> msgResponse;

		msg.Body().m_ulTxnID = m_ulTxnID;

		if ( !BYldSendMessageAndGetReply( msg, 10, &msgResponse, k_EMsgGCStorePurchaseCancelResponse ) )
		{
			Msg( "store_cancelpurchase: No response from the GC\n" );
			return false;
		}

		Msg( "Got response. Result: %d\n", msgResponse.Body().m_eResult );
		return true;
	}

private:
	uint64 m_ulTxnID;
};

CON_COMMAND( store_cancelpurchase, "<TxnID> Simulates cancelling a purchase" )
{
	
	if ( args.ArgC() < 2 )
	{
		Msg( store_cancelpurchase_command.GetHelpText() );	
	}

	uint64 ulTxnID;
	ulTxnID = V_atoui64( args[1] );

	CGCClientJobTESTCancelPurchase *pJob = new CGCClientJobTESTCancelPurchase( GCClientSystem()->GetGCClient(), ulTxnID );
	pJob->StartJob( NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Dev-only command and job for finalizing a purchase
//-----------------------------------------------------------------------------
class CGCClientJobTESTFinalizePurchase : public GCSDK::CGCClientJob
{
public:
	CGCClientJobTESTFinalizePurchase( GCSDK::CGCClient *pGCClient, uint64 ulTxnID ) : GCSDK::CGCClientJob( pGCClient ), m_ulTxnID( ulTxnID ) {}
	virtual bool BYieldingRunJob( void *pvStartParam )
	{
		GCSDK::CGCMsg<MsgGCStorePurchaseFinalize_t> msg( k_EMsgGCStorePurchaseFinalize );
		GCSDK::CGCMsg<MsgGCStorePurchaseFinalizeResponse_t> msgResponse;

		msg.Body().m_ulTxnID = m_ulTxnID;

		if ( !BYldSendMessageAndGetReply( msg, 10, &msgResponse, k_EMsgGCStorePurchaseFinalizeResponse ) )
		{
			Msg( "store_finalizepurchase: No response from the GC\n" );
			return false;
		}

		Msg( "Got response. Result: %d, Num Items: %d, Purchased Items:\n", msgResponse.Body().m_eResult, msgResponse.Body().m_cItemIDs );
		if ( k_EPurchaseResultOK == msgResponse.Body().m_eResult )
		{
			for ( uint32 i = 0; i < msgResponse.Body().m_cItemIDs; i++ )
			{
				uint64 ulItemID;
				if ( !msgResponse.BReadUint64Data( &ulItemID ) )
				{
					Msg( "Error: Underflow in msgResponse\n" );
					break;
				}

				Msg( "\t%llu\n", ulItemID );
			}
		}

		return true;
	}

private:
	uint64 m_ulTxnID;
};

CON_COMMAND( store_finalizepurchase, "<TxnID> Simulates finalizing a purchase" )
{

	if ( args.ArgC() < 2 )
	{
		Msg( store_finalizepurchase_command.GetHelpText() );	
	}

	uint64 ulTxnID;
	ulTxnID = V_atoui64( args[1] );

	CGCClientJobTESTFinalizePurchase *pJob = new CGCClientJobTESTFinalizePurchase( GCClientSystem()->GetGCClient(), ulTxnID );
	pJob->StartJob( NULL );
}

#endif
