//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Container that allows client & server access to data in player inventories & loadouts
//
//=============================================================================

#ifndef TF_ITEM_INVENTORY_H
#define TF_ITEM_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_item_inventory.h"
#include "tf_shareddefs.h"
#include "econ_item_constants.h"
#include "tf_item_constants.h"

#ifdef CLIENT_DLL
#include "econ_notifications.h"
#endif

#define LOADOUT_SLOT_USE_BASE_ITEM		0

namespace vgui
{
	class Panel;
}

struct baseitemcriteria_t;

//===============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: A single TF player's inventory. 
//		On the client, the inventory manager contains an instance of this for the local player.
//		On the server, each player contains an instance of this.
//-----------------------------------------------------------------------------
class CTFPlayerInventory : public CPlayerInventory
{
	DECLARE_CLASS( CTFPlayerInventory, CPlayerInventory );
public:
	CTFPlayerInventory();
	virtual ~CTFPlayerInventory();

	virtual CEconItemView	*GetItemInLoadout( int iClass, int iSlot );

#ifdef CLIENT_DLL
	// Removes any item in a loadout slot. If the slot has a base item,
	// the player essentially returns to using that item.
	// NOTE: This can fail if the player has no backpack space to contain the equipped item.
	bool				ClearLoadoutSlot( int iClass, int iSlot );
	CEconItemView		*GetCacheServerItemInLoadout( int iClass, int iSlot );

	void				UpdateWeaponSkinRequest();
#endif

	virtual int			GetMaxItemCount( void ) const;
	virtual bool		CanPurchaseItems( int iItemCount ) const;
	virtual int			GetPreviewItemDef( void ) const;

	// Derived inventory hooks
	virtual void		ItemHasBeenUpdated( CEconItemView *pItem, bool bUpdateAckFile, bool bWriteAckFile ) OVERRIDE;
	virtual void		ItemIsBeingRemoved( CEconItemView *pItem );
	bool				UpdateEquipStateForClass( const itemid_t& itemID, equipped_slot_t nSlot, itemid_t *pLoadout, int nCount );

	// Debugging
	virtual void		DumpInventoryToConsole( bool bRoot );

	bool				ClassLoadoutHasChanged( int iClass ) { return m_bLoadoutChanged[iClass]; }
	void				ClearClassLoadoutChangeTracking( void );

	virtual void		NotifyHasNewItems() { OnHasNewItems(); }

#ifdef CLIENT_DLL
	virtual ITexture	*GetWeaponSkinBaseLowRes( itemid_t nItemId, int iTeam ) const;

	void				LoadLocalLoadout();
	void				SaveLocalLoadout( bool bReset=false, bool bDefaultToGC=false );
	bool				EquipLocalPreset(equipped_class_t unClass, equipped_preset_t unPreset);
	int					GetActiveLocalPreset(equipped_class_t unClass) { return m_ActivePreset[unClass]; }

#endif
	void				EquipLocal(uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot);
	void				UnequipLocal(uint64 ulItemID);

	void				OnHasNewQuest();

	static CEconItemView *GetFirstItemOfItemDef( item_definition_index_t nDefIndex, CPlayerInventory* pInventory = NULL );

protected:
	virtual void		SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
#ifdef CLIENT_DLL
	// Converts an old format inventory to the new format.
	void				ConvertOldFormatInventoryToNew( void );

	virtual void		PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void		SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	virtual bool		AddEconItem( CEconItem * pItem, bool bUpdateAckFile, bool bWriteAckFile, bool bCheckForNewItems ) OVERRIDE;

	void				VerifyChangedLoadoutsAreValid();
	void				VerifyLoadoutItemsAreValid( int iClass );
#endif

	virtual void		OnHasNewItems();
	virtual void		ValidateInventoryPositions( void );

	// Extracts the position that should be used to sort items in the inventory from the backend position.
	// Necessary if your inventory packs a bunch of info into the position instead of using it just as a position.
	virtual int			ExtractInventorySortPosition( uint32 iBackendPosition )
	{ 
		// Consider unack'd items as -1, so they get stacked up before the 0th slot item
		if ( IsUnacknowledged(iBackendPosition) )
			return -1;
		return ExtractBackpackPositionFromBackend(iBackendPosition); 
	}

	virtual void		SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

#ifdef CLIENT_DLL
private:
	void				CheckSaxtonMaskAchievement( const CEconItem *pEconItem );
	void				UpdateCachedServerLoadoutItems();
	void				UpdateRealTFLoadoutItems();
#endif // CLIENT_DLL

protected:
	// Global indices of the items in our inventory in the loadout slots
#ifdef CLIENT_DLL
	struct SkinRequest_t
	{
		int m_nTeam;
		itemid_t m_nID;
		MDLHandle_t m_hModel;
	};
	CUtlVector< SkinRequest_t > m_vecWeaponSkinRequestList;


	itemid_t		m_CachedServerLoadoutItems[ TF_CLASS_COUNT ][ CLASS_LOADOUT_POSITION_COUNT ];

	CUtlMap< itemid_t, ITexture* > m_CachedBaseTextureLowRes[ TF_TEAM_COUNT ];

	int				m_ActivePreset[TF_CLASS_COUNT];
	itemid_t		m_PresetItems[CEconItemSchema::kMaxItemPresetCount][TF_CLASS_COUNT][CLASS_LOADOUT_POSITION_COUNT];
#ifdef CLIENT_DLL
	itemid_t		m_RealTFLoadoutItems[ TF_CLASS_COUNT ][ CLASS_LOADOUT_POSITION_COUNT ];
#endif

#endif // CLIENT_DLL
	itemid_t		m_LoadoutItems[ TF_CLASS_COUNT ][ CLASS_LOADOUT_POSITION_COUNT ];
	bool			m_bLoadoutChanged[ TF_CLASS_COUNT ];
	itemid_t		m_AccountLoadoutItems[ ACCOUNT_LOADOUT_POSITION_COUNT ];

	friend class CTFInventoryManager;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFInventoryManager : public CInventoryManager
{
	DECLARE_CLASS( CTFInventoryManager, CInventoryManager );
public:
	CTFInventoryManager();
	~CTFInventoryManager();

	virtual void		PostInit( void );

#ifdef CLIENT_DLL
	virtual CPlayerInventory *GeneratePlayerInventoryObject() const { return new CTFPlayerInventory; }

	//-----------------------------------------------------------------------
	// CLIENT PICKUP UI HANDLING
	//-----------------------------------------------------------------------

	// Get the number of items picked up
	virtual int			GetNumItemPickedUpItems( void );				

	// Show the player a pickup screen with any items they've collected recently, if any
	virtual bool		ShowItemsPickedUp( bool bForce = false, bool bReturnToGame = true, bool bNoPanel = false );

	// Show the player a pickup screen with the items they've crafted
	virtual void		ShowItemsCrafted( CUtlVector<itemid_t> *vecCraftedIndices );

	// Force the player to discard an item to make room for a new item, if they have one
	virtual bool		CheckForRoomAndForceDiscard( void );

	// Tells the GC that the player has acknowledged an item and attempts to move it in to the first available BP slot
	virtual void		AcknowledgeItem( CEconItemView *pItem, bool bMoveToBackpack = true );

	// Gets called each frame
	virtual void		Update( float frametime ) OVERRIDE;

	virtual bool		LoadPreset(equipped_class_t unClass, equipped_preset_t unPreset);
#endif

	// Returns the item data for the base item in the loadout slot for a given class
	CEconItemView		*GetBaseItemForClass( int iClass, int iSlot );
	void				GenerateBaseItems( void );

	// Gets the specified inventory for the steam ID
	CTFPlayerInventory	*GetInventoryForPlayer( const CSteamID &playerID );

	// Returns the item in the specified loadout slot for a given class
	CEconItemView		*GetItemInLoadoutForClass( int iClass, int iSlot, CSteamID *pID = NULL );
	
	CEconItemView		*GetItemInLoadoutForAccount( int nSlot, CSteamID *pID = NULL );

	// Fills out the vector with the sets that are currently active on the specified player & class
	void				GetActiveSets( CUtlVector<const CEconItemSetDefinition *> *pItemSets, CSteamID steamIDForPlayer, int iClass );

	// We're generating a base item. We need to add the game-specific keys to the criteria so that it'll find the right base item.
	virtual void		AddBaseItemCriteria( baseitemcriteria_t *pCriteria, CItemSelectionCriteria *pSelectionCriteria );
	
	bool				SlotContainsBaseItems( EEquipType_t eType, int iSlot );

	int					GetBaseItemCount( )			{ return m_pBaseLoadoutItems.Count(); }
	CEconItemView*		GetBaseItem( int iIndex )	{ return m_pBaseLoadoutItems[iIndex]; }

private:
	// Base items, returned for slots that the player doesn't have anything in
	CEconItemView				*m_pDefaultItem;
	CUtlVector<CEconItemView*>	m_pBaseLoadoutItems;

#ifdef CLIENT_DLL
	// On the client, we have a single inventory for the local player. Stored here, instead of in the
	// local player entity, because players need to access it while not being connected to a server.
public:
	CPlayerInventory	*GetLocalInventory( void ) { return &m_LocalInventory; }
	CTFPlayerInventory	*GetLocalTFInventory( void );

	// Try and equip the specified item in the specified class's loadout slot
	bool				EquipItemInLoadout( int iClass, int iSlot, itemid_t iItemID );

	// Fills out pList with all inventory items that could fit into the specified loadout slot for a given class
	int					GetAllUsableItemsForSlot( int iClass, int iSlot, CUtlVector<CEconItemView*> *pList );

	virtual int			GetBackpackPositionFromBackend( uint32 iBackendPosition ) { return ExtractBackpackPositionFromBackend(iBackendPosition); }

	virtual void		UpdateInventoryEquippedState(CPlayerInventory *pInventory, uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot);

private:
	CTFPlayerInventory	m_LocalInventory;
#endif // CLIENT_DLL
};

CTFInventoryManager *TFInventoryManager( void );



#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Econ Notifications
//-----------------------------------------------------------------------------
class CEconNotification_HasNewItems : public CEconNotification
{
public:
	CEconNotification_HasNewItems();
	~CEconNotification_HasNewItems();

	virtual void SetLifetime( float flSeconds )
	{
		m_flExpireTime = engine->Time() + flSeconds;
	}

	virtual float GetExpireTime() const
	{
		if ( m_flExpireTime != 0 )
			return -1.0f;
		return 0;
	}

	virtual float GetInGameLifeTime() const
	{
		return m_flExpireTime;
	}

	virtual void MarkForDeletion()
	{
		m_bHasTriggered = true;
		CEconNotification::MarkForDeletion();
	}

	virtual EType NotificationType() { return eType_Trigger; }
	virtual void Trigger()
	{
		m_bHasTriggered = true;
		TFInventoryManager()->ShowItemsPickedUp( true );
		MarkForDeletion();
	}

	virtual bool BShowInGameElements() const
	{
		return m_bShowInGame;
	}

	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast<CEconNotification_HasNewItems *>( pNotification ) != NULL; }

protected:

	bool m_bHasTriggered;
	bool m_bShowInGame;
};

//-----------------------------------------------------------------------------
class CEconNotification_HasNewItemsOnKill : public CEconNotification_HasNewItems
{
public:
	CEconNotification_HasNewItemsOnKill( int iVictimID );

	virtual EType NotificationType() { return eType_Basic; }
	virtual void Trigger() {}

	static bool HasUnacknowledgedItems();
	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast<CEconNotification_HasNewItemsOnKill *>( pNotification ) != NULL; }
};
#endif


#endif // TF_ITEM_INVENTORY_H
