//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Container that allows client & server access to data in player inventories & loadouts
//
//=============================================================================

#ifndef ITEM_INVENTORY_H
#define ITEM_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "econ_entity.h"
#include "gamestringpool.h"
#include "econ_item_view.h"
#include "UtlSortVector.h"
#include "econ_gcmessages.h"
#include "gc_clientsystem.h"

#if !defined(NO_STEAM)
#include "steam/steam_api.h"
#include "gcsdk/gcclientsdk.h"
#endif // NO_STEAM


class CPlayerInventory;
class CEconItem;
struct baseitemcriteria_t;
class CEconItemViewHandle;
#ifdef CLIENT_DLL
class ITexture;
#endif

// Inventory Less function.
// Used to sort the inventory items into their positions.
class CInventoryListLess
{
public:
	bool Less( const CEconItemView &src1, const CEconItemView &src2, void *pCtx );
};

// A class that wants notifications when an inventory is updated
class IInventoryUpdateListener : public GCSDK::ISharedObjectListener
{
public:
	virtual void InventoryUpdated( CPlayerInventory *pInventory ) = 0;

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { InventoryUpdated( NULL ); }
	virtual void PreSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { InventoryUpdated( NULL ); }
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { InventoryUpdated( NULL ); }
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { InventoryUpdated( NULL ); }
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { InventoryUpdated( NULL ); }
};

//-----------------------------------------------------------------------------
// Purpose: A single player's inventory. 
//		On the client, the inventory manager contains an instance of this for the local player.
//		On the server, each player contains an instance of this.
//-----------------------------------------------------------------------------
class CPlayerInventory : public GCSDK::ISharedObjectListener
{
	DECLARE_CLASS_NOBASE( CPlayerInventory );
public:
	CPlayerInventory();
	virtual ~CPlayerInventory();

	void				Clear();

	// Returns true if this inventory has been filled out by Steam.
	bool				RetrievedInventoryFromSteam( void ) { return m_bGotItemsFromSteam; }
	bool				IsWaitingForSteam( void ) { return (m_iPendingRequests > 0); }

	// Inventory access
	CSteamID			&GetOwner( void ) { return m_OwnerID; }
	int					GetItemCount( void ) const { return m_aInventoryItems.Count(); }
	virtual bool		CanPurchaseItems( int iItemCount ) const { return GetMaxItemCount() - GetItemCount() >= iItemCount; }
	virtual int			GetMaxItemCount( void ) const { return DEFAULT_NUM_BACKPACK_SLOTS; }
	CEconItemView		*GetItem( int i ) { return &m_aInventoryItems[i]; }

	virtual CEconItemView	*GetItemInLoadout( int iClass, int iSlot ) { AssertMsg( 0, "Implement me!" ); return NULL; }
	
	// Get the item object cache data for the specified item
	CEconItem			*GetSOCDataForItem( itemid_t iItemID );
	GCSDK::CGCClientSharedObjectCache	*GetSOC( void ) { return m_pSOCache; }

	// tells the GC systems to forget about this listener
	void				RemoveListener( GCSDK::ISharedObjectListener *pListener );

	// Finds the item in our inventory that matches the specified global index
	CEconItemView		*GetInventoryItemByItemID( itemid_t iIndex, int *pIndex = NULL );

	// Finds the item in our inventory that matches the specified global original id
	CEconItemView		*GetInventoryItemByOriginalID( itemid_t iOriginalID, int *pIndex = NULL );

	// Finds the item in our inventory in the specified position
	CEconItemView		*GetItemByPosition( int iPosition, int *pIndex = NULL );

	// Finds the first item in our backpack with match itemdef
	CEconItemView		*FindFirstItembyItemDef( item_definition_index_t iItemDef );

	// Used to reject items on the backend for inclusion into this inventory.
	// Mostly used for division of bags into different in-game inventories.
	virtual bool		ItemShouldBeIncluded( int iItemPosition ) { return true; }

	// Debugging
	virtual void		DumpInventoryToConsole( bool bRoot );

	// Extracts the position that should be used to sort items in the inventory from the backend position.
	// Necessary if your inventory packs a bunch of info into the position instead of using it just as a position.
	virtual int			ExtractInventorySortPosition( uint32 iBackendPosition ) { return iBackendPosition; }

	// Recipe access
	int									 GetRecipeCount( void ) const;
	const CEconCraftingRecipeDefinition *GetRecipeDef( int iIndex );
	const CEconCraftingRecipeDefinition *GetRecipeDefByDefIndex( uint16 iDefIndex );

	// Item previews
	virtual int			GetPreviewItemDef( void ) const { return 0; };

	// Access helpers
	virtual void		SOClear();

	virtual void		NotifyHasNewItems() {}

	void				AddItemHandle( CEconItemViewHandle* pHandle );
	void				RemoveItemHandle( CEconItemViewHandle* pHandle );

	const CCopyableUtlVector< itemid_t >& GetItemsWithDefindex( item_definition_index_t defindex ) ;
	const CCopyableUtlVector< itemid_t >& GetItemsWithPaintkitDefindex( uint32 nDefindex );

#ifdef CLIENT_DLL
	virtual ITexture	*GetWeaponSkinBaseLowRes( itemid_t nItemId, int iTeam ) const { return NULL; }
#endif


protected:
	// Inventory updating, called by the Inventory Manager only. If you want an inventory updated,
	// use the SteamRequestX functions in CInventoryManager.
	void				RequestInventory( CSteamID pSteamID );
	void				AddListener( GCSDK::ISharedObjectListener *pListener );
	virtual bool		AddEconItem( CEconItem * pItem, bool bUpdateAckFile, bool bWriteAckFile, bool bCheckForNewItems );
	virtual void		RemoveItem( itemid_t iItemID );
	bool				FilloutItemFromEconItem( CEconItemView *pScriptItem, CEconItem *pEconItem );
	void				SendInventoryUpdateEvent();
	virtual void		OnHasNewItems() {}
	virtual void		OnItemChangedPosition( CEconItemView *pItem, uint32 iOldPos ) { return; }

	virtual void		SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void		PreSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void		SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void		PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { /* do nothing */ }
	virtual void		SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void		SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void		SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	void				ResortInventory( void ) { m_aInventoryItems.RedoSort( true ); }
	virtual void		ValidateInventoryPositions( void );

	// Derived inventory hooks
	virtual void		ItemHasBeenUpdated( CEconItemView *pItem, bool bUpdateAckFile, bool bWriteAckFile );
	virtual void		ItemIsBeingRemoved( CEconItemView *pItem ) { return; }

	// Get the index for the item in our inventory utlvector
	int					GetIndexForItem( CEconItemView *pItem );

	void				DirtyItemHandles();

protected:
	// The Steam Id of the player who owns this inventory
	CSteamID	m_OwnerID;

	CUtlMap< item_definition_index_t, CCopyableUtlVector< itemid_t > > m_mapItemDefsToItems;
	CUtlMap< uint32, CCopyableUtlVector< itemid_t > > m_mapPaintkitsToItems;

	// The items the player has in his inventory, received from steam.
	CUtlSortVector<CEconItemView,CInventoryListLess>		m_aInventoryItems;

	int			m_iPendingRequests;
	bool		m_bGotItemsFromSteam;

	GCSDK::CGCClientSharedObjectCache	  *m_pSOCache;

	CUtlVector<GCSDK::ISharedObjectListener *> m_vecListeners;

	CUtlVector< CEconItemViewHandle* > m_vecItemHandles;

	friend class CInventoryManager;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CInventoryManager : public CAutoGameSystemPerFrame
{
	DECLARE_CLASS_GAMEROOT( CInventoryManager, CAutoGameSystem );
public:
	CInventoryManager( void );

	// Adds the inventory to the list of inventories that should be maintained.
	// This causes the game to load the items for the SteamID into this inventory.
	// NOTE: This fires off a request to Steam. The data will not be filled out immediately.
	void SteamRequestInventory( CPlayerInventory *pInventory, CSteamID pSteamID, IInventoryUpdateListener *pListener = NULL );

	void PreInitGC();
	void PostInitGC();

#ifdef CLIENT_DLL
	void	DropItem( itemid_t iItemID );
	int		DeleteUnknowns( CPlayerInventory *pInventory );
#endif

public:
	//-----------------------------------------------------------------------
	// IAutoServerSystem
	//-----------------------------------------------------------------------
	virtual bool Init( void ) OVERRIDE;
	virtual void PostInit( void ) OVERRIDE;
	virtual void Shutdown() OVERRIDE;
	virtual void LevelInitPreEntity( void ) OVERRIDE;
	virtual void LevelShutdownPostEntity( void ) OVERRIDE;

#ifdef CLIENT_DLL
	// Gets called each frame
	virtual void Update( float frametime ) OVERRIDE;
#endif

	void GameServerSteamAPIActivated();

	virtual CPlayerInventory *GetInventoryForAccount( uint32 iAccountID );

	// We're generating a base item. We need to add the game-specific keys to the criteria so that it'll find the right base item.
	virtual void		AddBaseItemCriteria( baseitemcriteria_t *pCriteria, CItemSelectionCriteria *pSelectionCriteria ) { return; }

#ifdef CLIENT_DLL
	// Must be implemented by derived class
	virtual bool		EquipItemInLoadout( int iClass, int iSlot, itemid_t iItemID ) = 0;

	virtual CPlayerInventory *GeneratePlayerInventoryObject() const { return new CPlayerInventory; }

	//-----------------------------------------------------------------------
	// ITEM PRESETS
	//-----------------------------------------------------------------------

	// Is the given preset index valid?
	bool				IsPresetIndexValid( equipped_preset_t unPreset );

	// Equip all items for the given class and preset (all the work is done on the GC -- this just
	// sends the message up)
	virtual bool		LoadPreset( equipped_class_t unClass, equipped_preset_t unPreset );

	//-----------------------------------------------------------------------
	// LOCAL INVENTORY
	//
	// On the client, we have a single inventory for the local player. Stored here, instead of in the
	// local player entity, because players need to access it while not being connected to a server.
	// Override GetLocalInventory() in your inventory manager and return your custom local inventory.
	//-----------------------------------------------------------------------
	virtual void				UpdateLocalInventory( void );
	virtual CPlayerInventory	*GetLocalInventory( void ) { return NULL; }

	// The local inventory is used to track discards & responses to. We need to
	// make a decision about inventory space right after sending a delete request,
	// so we predict the request will work.
	void				OnItemDeleted( CPlayerInventory *pInventory ) { if ( pInventory == GetLocalInventory() ) m_iPredictedDiscards--; }

	virtual void		PersonaName_Precache( uint32 unAccountID );
	virtual const char *PersonaName_Get( uint32 unAccountID );
	virtual void		PersonaName_Store( uint32 unAccountID, const char *pPersonaName );

	static void			SendItemSystemConnectedEvent( void );

	// Returns the item at the specified backpack position
	virtual CEconItemView	*GetItemByBackpackPosition( int iBackpackPosition );

	// Moves the item to the specified backpack position. If there's another item as that spot, it swaps positions with it.
	virtual void		MoveItemToBackpackPosition( CEconItemView *pItem, int iBackpackPosition );

	// Tries to set the item to the specified backpack position. Passing in 0 will find the first empty position.
	// FAILS if the backpack is full, or if that spot isn't clear. Returns false in that case.
	virtual bool		SetItemBackpackPosition( CEconItemView *pItem, uint32 iPosition = 0, bool bForceUnequip = false, bool bAllowOverflow = false );

	// Sort the backpack items by the specified type
	virtual void		SortBackpackBy( uint32 iSortType );
	void				SortBackpackFinished( void );
	bool				IsInBackpackSort( void ) { return m_bInBackpackSort; }

	void				PredictedBackpackPosFilled( int iBackpackPos ) { m_PredictedFilledSlots.FindAndRemove( iBackpackPos ); }

	// Tell the backend to move an item to a specified backend position
	virtual void		UpdateInventoryPosition( CPlayerInventory *pInventory, uint64 ulItemID, uint32 unNewInventoryPos );

	virtual void		UpdateInventoryEquippedState( CPlayerInventory *pInventory, uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot );


	//-----------------------------------------------------------------------
	// CLIENT PICKUP UI HANDLING
	//-----------------------------------------------------------------------

	// Get the number of items picked up
	virtual int			GetNumItemPickedUpItems( void ) { return 0; }

	// Show the player a pickup screen with any items they've collected recently, if any
	virtual bool		ShowItemsPickedUp( bool bForce = false, bool bReturnToGame = true, bool bNoPanel = false );

	// Show the player a pickup screen with the items they've crafted
	virtual void		ShowItemsCrafted( CUtlVector<itemid_t> *vecCraftedIndices ) { return; }

	// Force the player to discard an item to make room for a new item, if they have one.
	// Returns true if the discard panel has been brought up, and the player will be forced to discard an item.
	virtual bool		CheckForRoomAndForceDiscard( void );
	
	//-----------------------------------------------------------------------
	// CLIENT ITEM PICKUP ACKNOWLEDGEMENT FILES
	//
	// This system avoids showing multiple pickups for items that we've found, but haven't been 
	// able to move out of unack'd position due to the GC being unavailable. We keep a list of
	// items we've ack'd in a client file, and don't re-show pickups for them. When a GC item
	// update tells us the item has moved out of the unack'd position, we remove it from our file.
	//-----------------------------------------------------------------------

	virtual void		AcknowledgeItem ( CEconItemView *pItem, bool bMoveToBackpack = true );		// Client Acknowledges an item and moves it in to the backpack
	bool				HasBeenAckedByClient( CEconItemView *pItem );		// Returns true if it's in our client file
	void				SetAckedByClient( CEconItemView *pItem );			// Adds it to our client file
	void				SetAckedByGC( CEconItemView *pItem, bool bSave );	// Removes it from our client file
	KeyValues			*GetAckKeyForItem( CEconItemView *pItem );
	void				CleanAckFile( void );
	void				SaveAckFile( void );

private:
	void				VerifyAckFileLoaded( void );
	KeyValues			*m_pkvItemClientAckFile;
	bool				m_bClientAckDirty;

private:
	// As we move items around in batches (on pickups usually) we need to know what slots will be filled
	// by items we've moved, and haven't received a response from Steam.
	CUtlVector<int>				m_PredictedFilledSlots;
#endif
	
public:
	virtual int			GetBackpackPositionFromBackend( uint32 iBackendPosition ) { return ExtractBackpackPositionFromBackend(iBackendPosition); }

private:
	//-----------------------------------------------------------------------
	// Pending inventory requests
	struct pendingreq_t
	{
		CPlayerInventory *pInventory;
		CSteamID		 pID;
	};
	CUtlVector<pendingreq_t>	m_hPendingInventoryRequests;
	void		RemovePendingRequest( CSteamID *pSteamID );

protected:
	//-----------------------------------------------------------------------
	// Inventory registry
	void DeregisterInventory( CPlayerInventory *pInventory );
	struct inventories_t
	{
		CPlayerInventory			*pInventory;
		IInventoryUpdateListener	*pListener;
	};
	CUtlVector<inventories_t>	m_pInventories;

	friend class CPlayerInventory;

	bool			IsValidPlayerClass( equipped_class_t unClass );

#ifdef CLIENT_DLL
	// Keep track of the number of items we've tried to discard, but haven't recieved responses on
	int			m_iPredictedDiscards;

	typedef CUtlMap< uint32, CUtlString, int > tPersonaNamesByAccountID;
	tPersonaNamesByAccountID m_mapPersonaNamesCache;

	bool		m_bInBackpackSort;

	float		m_flNextLoadPresetChange;

	CMsgSetItemPositions m_msgPendingSetItemPositions;
	CMsgLookupMultipleAccountNames m_msgPendingLookupAccountNames;

	void OnPersonaStateChanged( PersonaStateChange_t *info );
	CCallback< CInventoryManager, PersonaStateChange_t, false > m_sPersonaStateChangedCallback;
	CUtlMap< uint64, bool > m_personaNameRequests;

#endif
};

//=================================================================================
// Implement these functions in your game code to create custom derived versions
CInventoryManager *InventoryManager( void );

CBasePlayer *GetPlayerBySteamID( const CSteamID &steamID );

//-----------------------------------------------------------------------------
// Purpose: Maintains a handle to an CEconItemView within an inventory.  When
//			the inventory gets updated and shuffles CEconItemViews around, this
//			handle automatically updates its pointer to point to the new
//			CEconItemView that has the same item_id
//-----------------------------------------------------------------------------
class CEconItemViewHandle
{
public:
	CEconItemViewHandle()
		: m_pItem( NULL )
		, m_pInv( NULL )
		, m_bPointerDirty( false )
	{}

	CEconItemViewHandle( CEconItemView* pItem )
		: m_pItem( pItem )
		, m_pInv( NULL )
		, m_bPointerDirty( false )
	{
		SetItem( pItem );
	}

	virtual ~CEconItemViewHandle()
	{
		// Unregister us
		if ( m_pInv )
		{
			m_pInv->RemoveItemHandle( this );
		}
	}

	void SetItem( CEconItemView* pItem );

	operator CEconItemView *( void ) const
	{ 
		return Get();
	}

	CEconItemView* operator->( void ) const
	{
		return Get();
	}

	void ItemIsBeingDeleted( const CEconItemView* pItem )
	{
		m_bPointerDirty = true;

		// Inventory told us the item is going away
		if ( m_pItem == pItem )
		{
			m_pItem = NULL;
		}
	}

	void InventoryIsBeingDeleted()
	{
		m_pInv = NULL;
		m_pItem = NULL;
		m_bPointerDirty = false;	// So we dont keep trying to look up the item
	}

	void MarkDirty()
	{
		m_bPointerDirty = true;
	}

private:

	CEconItemView* Get() const;

	mutable bool m_bPointerDirty;		// Used to mark when m_pItem is no longer valid 
	CPlayerInventory *m_pInv;			// Inventory the item belongs to.  Used to look up new CEconItemView 
	mutable CEconItemView* m_pItem;		// The item. 
	uint64 m_nItemID;					// ID of the item
	CSteamID m_OwnerSteamID;			// Steam ID of the item owner
};


#endif // ITEM_INVENTORY_H
