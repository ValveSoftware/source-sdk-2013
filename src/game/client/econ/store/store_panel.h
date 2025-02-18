//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef STORE_PANEL_H
#define STORE_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"
#include "econ_ui.h"
#include "GameEventListener.h"
#include "store_page.h"
#include "econ_store.h"
#include "econ_gcmessages.h"
#include "steam/isteamuser.h"

#define MAX_CART_ITEMS	256

#define STOREPANEL_SHOW_UPGRADESTEPS		-1

class CStorePage;

// An "item" in the cart.
struct cart_item_t
{
	const econ_store_entry_t *pEntry;
	int						  iQuantity;
	ECartItemType			  eType;
	bool					  bPreviewItem;

	item_price_t GetDisplayPrice() const;
};

//-----------------------------------------------------------------------------
// Purpose: The cart that contains items the player is purchasing
//-----------------------------------------------------------------------------
class CStoreCart
{
public:
	CStoreCart( void );

	void		AddToCart( const econ_store_entry_t *pEntry, const char* pszPageName, ECartItemType eCartItemType );
	void		RemoveFromCart( int iEntryIndex );
	void		EmptyCart( void );

	// Returns the total number of items in the cart
	int			GetTotalItems( void ) const;
	int			GetTotalConcreteItems( void ) const;
	// Returns the number of different entries in the cart (ignoring quantities)
	int			GetNumEntries( void ) const { return m_Items.Count(); }
	cart_item_t *GetItem( int iIndex ) { return ( ( GetNumEntries() > 0 ) ? &m_Items[iIndex] : NULL ); }

	item_price_t GetTotalPrice( void ) const;

	bool		ContainsHolidayRestrictedItems() const;
	bool		ContainsChanceRestrictedItems() const;
	bool		ContainsItemDefinition( item_definition_index_t unItemDef ) const;

private:
	int			GetIndexForEntry( const econ_store_entry_t *pEntry, ECartItemType eCartItemType ) const;

private:
	CUtlVector<cart_item_t>	m_Items;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CStorePanel : public vgui::PropertyDialog, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CStorePanel, vgui::PropertyDialog );
public:
	CStorePanel( Panel *parent );
	virtual ~CStorePanel();

#ifdef _DEBUG
	void		ReAddPage( int iPage );
#endif

	// UI Layout
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void FireGameEvent( IGameEvent *event );
	void		 SetPreventClosure( bool bPrevent ) { m_bPreventClosure = bPrevent; }
	void		 StartAtItemDef( int iItemDef, bool bAddToCart ) { m_iStartItemDef = iItemDef; m_bAddStartItemDefToCart = bAddToCart; };

	virtual void OnTick();

	// Steam Interaction
	STEAM_CALLBACK( CStorePanel, OnMicroTransactionAuthResponse, MicroTxnAuthorizationResponse_t, m_CallbackMicroTransactionAuthResponse );

	// GC Management
	static bool	CheckMessageResult( EPurchaseResult msgResult );
	void		FinalizeTransaction( void );
	virtual void	PostTransactionCompleted( void ) { return; }

	// Cart Management
	CStoreCart	 *GetCart( void ) { return &m_Cart; }
	void		ShowStorePanel( void );
	bool		ShouldUpsellStamps( void );
	bool		HasValidUpsellStamps( void );
	void		UpsellStamps( void );
	static void ConfirmUpsellStamps( bool bConfirmed, CSchemaItemDefHandle hItemDef, int nSecondsVisible );
	void		InitiateCheckout( bool bSkipUpsell, bool bSkipDecoderWarning = false );
	void		CheckoutCancel( void );
	virtual void OnAddToCart( void ) {}
	void		AddToCartAndCheckoutImmediately( item_definition_index_t nDefIndex );

	// Pricesheet Management
	static bool	IsPricesheetLoaded( void ) { return CStorePanel::m_bPricesheetLoaded; }
	static bool	ShouldShowWarnings( void ) { return CStorePanel::m_bShowWarnings; }
	static void SetShouldShowWarnings( bool bShow ) { CStorePanel::m_bShowWarnings = bShow; }
	static void	RequestPricesheet( void );

	const CEconStorePriceSheet *GetPriceSheet( void ) { return &m_StoreSheet; }
	CEconStorePriceSheet *GetPriceSheetForEdit( void ) { return &m_StoreSheet; }
	bool		LoadPricesheet( KeyValuesAD* pKVPricesheet );
	void		SetCurrency( ECurrency in_currency );
	ECurrency	GetCurrency( void ) { return m_eCurrency; }
	void		SetCountryCode( const char* in_country );
	char*		GetCountryCode( void ) { return m_rgchCountry; }
	const econ_store_entry_t *GetFeaturedEntry( void );
	void		SetMostRecentSuccessfulTransactionID( uint64 inID ) { m_unMostRecentSuccessfulTransaction = inID; }
	uint64		GetMostRecentSuccessfulTransactionID() const { return m_unMostRecentSuccessfulTransaction; }
	virtual void	SetTransactionID( uint64 inID ) { m_unTransactionID = inID; }
	uint64		GetTransactionID( void ) { return m_unTransactionID; }

	int			GetCheckoutAttempts() { return m_iCheckoutAttempts; }
	void		SetLastPurchaseAttemptPrice( int totalPrice ) { m_iLastPurchaseAttemptPrice = totalPrice; }
	int			GetLastPurchaseAttemptPrice() { return m_iLastPurchaseAttemptPrice; }

	void		ClearPopularItems( void ) { m_vPopularItems.Purge(); }
	void		AddPopularItem( uint32 iItemDef ) { m_vPopularItems.AddToTail(iItemDef); }
	const CUtlVector<uint32>& GetPopularItems( void ) const { return m_vPopularItems; }

	MESSAGE_FUNC( OnStartShopping, "StartShopping" );
	MESSAGE_FUNC( OnFindAndSelectFeaturedItem, "FindAndSelectFeaturedItem" );
	MESSAGE_FUNC_PARAMS( OnItemLinkClicked, "URLClicked", pParams );
	MESSAGE_FUNC_PARAMS( OnJumpToItem, "JumpToItem", pParams );
	MESSAGE_FUNC( DoCheckout, "DoCheckout" );

protected:
	void		ParseStoreKV( void );
	CStorePage	*AddPageFromPriceSheet( int iPage );

	void		FindAndSelectEntry( const econ_store_entry_t *pEntry );
	const econ_store_entry_t *FindEntryForItemDef( int iItemDef ) { return m_StoreSheet.GetEntry( iItemDef ); }

	virtual CStorePage	*CreateStorePage( const CEconStoreCategoryManager::StoreCategory_t *pPageData );

	bool		ShouldShowDx8PurchaseWarning( ) const;

protected:
	static void ConfirmCheckout( bool bConfirmed, void *pContext );
	static void ProceedCheckout_DecoderWarning( bool bConfirmed, void *pContext );

	static bool	m_bPricesheetLoaded;
	static bool	m_bShowWarnings;
	bool		m_bPreventClosure;
	int			m_iStartItemDef;
	bool		m_bAddStartItemDefToCart;
	CStoreCart	m_Cart;
	CEconStorePriceSheet	m_StoreSheet;

	ECurrency	m_eCurrency;
	char		m_rgchCountry[3]; // This will change to an enum soon.
	uint64		m_unTransactionID;
	uint64		m_unMostRecentSuccessfulTransaction;

	bool		m_bShouldFinalize;
	bool		m_bOGSLogging;

	int			m_iCheckoutAttempts;
	int			m_iLastPurchaseAttemptPrice;

	CUtlVector<uint32> m_vPopularItems;
};

void OpenStoreStatusDialog( vgui::Panel *pParent, const char *pszText, bool bAllowClose, bool bShowOnExit, bool bCancel=false );
void CloseStoreStatusDialog( void );

//-----------------------------------------------------------------------------
// Purpose: Asynchronous job for getting the price sheet from the GC
//-----------------------------------------------------------------------------
class CGCClientJobGetUserData : public GCSDK::CGCClientJob
{
public:
	CGCClientJobGetUserData( GCSDK::CGCClient *pGCClient, RTime32 rTimeVersion ) : GCSDK::CGCClientJob( pGCClient ), m_RTimeVersion( rTimeVersion ) {}
	virtual bool BYieldingRunJob( void *pvStartParam );

private:
	RTime32 m_RTimeVersion;
};

//-----------------------------------------------------------------------------
// Purpose: Asynchronous job for initiating a checkout from the Steam store.
//-----------------------------------------------------------------------------
class CGCClientJobInitPurchase : public GCSDK::CGCClientJob
{
public:
	CGCClientJobInitPurchase( GCSDK::CGCClient *pGCClient ) : GCSDK::CGCClientJob( pGCClient ) {}
	virtual bool BYieldingRunJob( void *pvStartParam );
};

//-----------------------------------------------------------------------------
// Purpose: Asynchronous job for canceling a purchase in progress.
//-----------------------------------------------------------------------------
class CGCClientJobCancelPurchase : public GCSDK::CGCClientJob
{
public:
	CGCClientJobCancelPurchase( GCSDK::CGCClient *pGCClient, uint64 ulTxnID ) : GCSDK::CGCClientJob( pGCClient ), m_ulTxnID( ulTxnID ) {}
	virtual bool BYieldingRunJob( void *pvStartParam );

private:
	uint64 m_ulTxnID;
};

//-----------------------------------------------------------------------------
// Purpose: Asynchronous job for finalizing a purchase with the GC.
//-----------------------------------------------------------------------------
class CGCClientJobFinalizePurchase : public GCSDK::CGCClientJob
{
public:
	CGCClientJobFinalizePurchase( GCSDK::CGCClient *pGCClient, uint64 ulTxnID ) : GCSDK::CGCClientJob( pGCClient ), m_ulTxnID( ulTxnID ) {}
	virtual bool BYieldingRunJob( void *pvStartParam );

private:
	uint64 m_ulTxnID;
};

#endif // STORE_PANEL_H
