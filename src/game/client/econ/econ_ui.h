//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ECON_UI_H
#define ECON_UI_H
#ifdef _WIN32
#pragma once
#endif

enum EconBaseUIPanels_t
{
	ECONUI_BASEUI = 0,
	ECONUI_BACKPACK,
	ECONUI_CRAFTING,
	ECONUI_ARMORY,
	ECONUI_TRADING,
	ECONUI_LOADOUT,

	ECONUI_FIRST_PANEL = ECONUI_BASEUI,
	ECONUI_LAST_PANEL = ECONUI_LOADOUT
};

class CBackpackPanel;
class CCraftingPanel;
class CItemPickupPanel;
class CItemDiscardPanel;
class CStorePanel;
struct cart_item_t;
namespace vgui
{
	class Panel;
};

// Interface used to connect to the game specific implementations of the Economy UI
abstract_class IEconRootUI
{
public:
	// Open the EconUI, optionally to a specific page (Backpack/Crafting/etc)
	// If bCheckForInventorySpaceOnExit is set, On closing the EconUI should make sure the user doesn't
	// have to throw out any items to make room in their inventory.
	virtual IEconRootUI	*OpenEconUI( int iDirectToPage = 0, bool bCheckForInventorySpaceOnExit = false ) = 0;

	// Close the EconUI, and any associated sub panels.
	virtual void		CloseEconUI( void ) = 0;

	// Return true if the specified EconUI sub panel is currently visible.
	virtual bool		IsUIPanelVisible( EconBaseUIPanels_t iPanel ) = 0;

	// Some part of the EconUI might be in a state where they want to prevent the user
	// from being able to close the EconUI (in the middle of a trade, for instance)
	virtual void		SetPreventClosure( bool bPrevent ) = 0;

	// Sub panel access.
	// These are panels that are parented to the root EconUI.
	virtual CBackpackPanel *GetBackpackPanel( void ) = 0;
	virtual CCraftingPanel *GetCraftingPanel( void ) = 0;

	// Gamestats access (We should replace these with an Econ Gamestats)
	virtual void		Gamestats_ItemTransaction( int eventID, CEconItemView *item, const char *pszReason = NULL, int iQuality = 0 ) = 0;
	virtual void		Gamestats_Store( int eventID, CEconItemView* item=NULL, const char* panelName=NULL, 
		int classId=0, const cart_item_t* in_cartItem=NULL, int in_checkoutAttempts=0, const char* storeError=NULL, int in_totalPrice=0, int in_currencyCode=0 ) = 0;
	virtual void		SetExperimentValue( uint64 experimentValue ) = 0;

	// Open separate economy panels (they're not parented to the root EconUI)
	// This is here so that games can customize the implementation of these panels.
	virtual CItemPickupPanel	*OpenItemPickupPanel( void ) = 0;
	virtual CItemDiscardPanel	*OpenItemDiscardPanel( void ) = 0;
	// Store
	virtual void				CreateStorePanel( void ) = 0;
	virtual CStorePanel			*OpenStorePanel( int iItemDef, bool bAddToCart ) = 0;
	virtual CStorePanel			*GetStorePanel( void ) = 0;

	// When the root UI is closed, send an "EconUIClosed" message to pListener.
	virtual void		AddPanelCloseListener( vgui::Panel *pListener ) = 0;

	// The panel at which we want back to actually close the UI - defaults to the root panel - a negative value can be passed in for class loadout panels
	virtual void		SetClosePanel( int iPanel ) = 0;

	// Call this to set which team the class loadout should display
	virtual void		SetDefaultTeam( int iTeam ) = 0;
};

extern IEconRootUI *EconUI( void );


// IDs for Item related events in Gamestats tracking
enum ITEMEVENTS
{
	// STORE EVENTS
	IE_STORE_ENTERED,
	IE_STORE_EXITED,
	IE_STORE_TAB_CHANGED,
	IE_STORE_ITEM_SELECTED,
	IE_STORE_ITEM_PREVIEWED,
	IE_STORE_ITEM_ADDED_TO_CART,
	IE_STORE_ITEM_REMOVED_FROM_CART,
	IE_STORE_CHECKOUT_ATTEMPT,
	IE_STORE_CHECKOUT_FAILURE,
	IE_STORE_CHECKOUT_SUCCESS,
	IE_STORE_CHECKOUT_ITEM,

	// LOADOUT EVENTS
	IE_LOADOUT_ENTERED,
	IE_LOADOUT_EXITED,

	// TRADING EVENTS
	IE_TRADING_ENTERED,
	IE_TRADING_EXITED,
	IE_TRADING_WENT_TO_ARMORY,
	IE_TRADING_RETURNED_FROM_ARMORY,
	IE_TRADING_REQUEST_SENT,
	IE_TRADING_REQUEST_RECEIVED,
	IE_TRADING_REQUEST_REJECTED,
	IE_TRADING_REQUEST_ACCEPTED,
	IE_TRADING_TRADE_NEGOTIATED,
	IE_TRADING_TRADE_SUCCESS,
	IE_TRADING_TRADE_FAILURE,
	IE_TRADING_ITEM_GIVEN,
	IE_TRADING_ITEM_RECEIVED,
	IE_TRADING_ITEM_GIFTED,

	// CRAFTING EVENTS
	IE_CRAFTING_ENTERED,
	IE_CRAFTING_EXITED,
	IE_CRAFTING_WENT_TO_ARMORY,
	IE_CRAFTING_RETURNED_FROM_ARMORY,
	IE_CRAFTING_VIEW_BLUEPRINTS,
	IE_CRAFTING_TIMEOUT,
	IE_CRAFTING_FAILURE,
	IE_CRAFTING_SUCCESS,
	IE_CRAFTING_NO_RECIPE_MATCH,
	IE_CRAFTING_ATTEMPT,
	IE_CRAFTING_RECIPE_FOUND,

	// ARMORY EVENTS
	IE_ARMORY_ENTERED,
	IE_ARMORY_EXITED,
	IE_ARMORY_SELECT_ITEM,
	IE_ARMORY_BROWSE_WIKI,
	IE_ARMORY_CHANGE_FILTER,

	// TRANSACTION EVENTS
	IE_ITEM_RECEIVED,
	IE_ITEM_DISCARDED,
	IE_ITEM_DELETED,
	IE_ITEM_USED_TOOL,
	IE_ITEM_USED_CONSUMABLE,
	IE_ITEM_REMOVED_ATTRIB,
	IE_ITEM_CHANGED_STYLE,

	// NEW STORE EVENTS
	IE_STORE2_ENTERED,	// This gets written *in addition* to IE_STORE_ENTERED

	// THESE STORED AS INTEGERS IN THE DATABASE SO THESE ARE NEW
	IE_ITEM_RESET_STRANGE_COUNTERS,
	IE_ITEM_PUT_INTO_COLLECTION,

	IE_COUNT,
};

#endif // ECON_UI_H
