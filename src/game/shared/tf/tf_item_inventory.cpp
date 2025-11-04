//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_item_inventory.h"
#include "econ_entity_creation.h"
#include "tf_item_system.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "character_info_panel.h"
#include "ienginevgui.h"
#include "c_tf_gamestats.h"
#include "econ_notifications.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "econ/econ_item_preset.h"
#include "tf_shared_content_manager.h"
#include "c_playerresource.h"
#include "materialsystem/itexture.h"

#include "tf_gc_client.h"

#else
#include "tf_player.h"
#endif
#include "gc_clientsystem.h"
#include "econ_game_account_client.h"
#include "gcsdk/gcclientsdk.h"
#include "econ_gcmessages.h"
#include "tf_gamerules.h"
#include "tf_gcmessages.h"
#include "econ_item.h"
#include "game_item_schema.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace GCSDK;

#define LOCAL_LOADOUT_FILE		"cfg/local_loadout.txt"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool AreSlotsConsideredIdentical( EEquipType_t eEquipType, int iBaseSlot, int iTestSlot )
{
	if ( eEquipType == EQUIP_TYPE_CLASS )
	{
		if ( iBaseSlot == LOADOUT_POSITION_MISC )
		{
			return IsMiscSlot( iTestSlot );
		}

		if ( iBaseSlot == LOADOUT_POSITION_BUILDING )
		{
			return IsBuildingSlot( iTestSlot );
		}

		if ( iBaseSlot == LOADOUT_POSITION_TAUNT )
		{
			return IsTauntSlot( iTestSlot );
		}
	}
	else if ( eEquipType == EQUIP_TYPE_ACCOUNT )
	{
		if ( iBaseSlot == ACCOUNT_LOADOUT_POSITION_ACCOUNT1 )
		{
			return IsQuestSlot( iTestSlot );
		}
	}

	return iBaseSlot == iTestSlot;
}

//-----------------------------------------------------------------------------
CTFInventoryManager g_TFInventoryManager;
CInventoryManager *InventoryManager( void )
{
	return &g_TFInventoryManager;
}
CTFInventoryManager *TFInventoryManager( void )
{
	return &g_TFInventoryManager;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFInventoryManager::CTFInventoryManager( void )

{
}

CTFInventoryManager::~CTFInventoryManager( void )
{
	m_pBaseLoadoutItems.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFInventoryManager::PostInit( void )
{
	BaseClass::PostInit();
	GenerateBaseItems();
}

//-----------------------------------------------------------------------------
// Purpose: Generate & store the base item details for each class & loadout slot
//-----------------------------------------------------------------------------
void CTFInventoryManager::GenerateBaseItems( void )
{
	// Purge our lists and make new
	m_pBaseLoadoutItems.PurgeAndDeleteElements();
	
	// Load a base top level invalid item
	{
		m_pDefaultItem = new CEconItemView;
		m_pDefaultItem->Invalidate();
	}
	//
	const CEconItemSchema::BaseItemDefinitionMap_t& mapItems = GetItemSchema()->GetBaseItemDefinitionMap();
	int iStart = 0;
	for ( int it = iStart; it != mapItems.InvalidIndex(); it = mapItems.NextInorder( it ) )
	{
		CEconItemView *pItem = new CEconItemView;
		pItem->Init( mapItems[it]->GetDefinitionIndex(), NULL );
		m_pBaseLoadoutItems.AddToTail( pItem );
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFInventoryManager::EquipItemInLoadout( int iClass, int iSlot, itemid_t iItemID )
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
		return false;

	// If they pass in a INVALID_ITEM_ID item id, we're just clearing the loadout slot
	if ( iItemID == INVALID_ITEM_ID )
		return m_LocalInventory.ClearLoadoutSlot( iClass, iSlot );

	CEconItemView *pItem = m_LocalInventory.GetInventoryItemByItemID( iItemID );
	if ( !pItem )
		return false;

	// We check for validity on the GC when we equip items, but we can't really trust anyone
	// and so we check here as well.
	if ( !AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItem->GetStaticData()->GetLoadoutSlot(iClass), iSlot ) )
	{
		return false;
	}

	if ( !pItem->GetStaticData()->CanBeUsedByClass( iClass ) )
	{
		return false;
	}

	// Equip the new item
	UpdateInventoryEquippedState( &m_LocalInventory, iItemID, iClass, iSlot );

	// TODO: Prediction
	// Item has been moved, so update our loadout.
	//m_LoadoutItems[iClass][iSlot] = iItemID;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Fills out pList with all inventory items that could fit into the specified loadout slot for a given class
//-----------------------------------------------------------------------------
int	CTFInventoryManager::GetAllUsableItemsForSlot( int iClass, int iSlot, CUtlVector<CEconItemView*> *pList )
{
	bool bIsAccountIndex = iClass == GEconItemSchema().GetAccountIndex();
	if ( bIsAccountIndex )
	{
		Assert( IsQuestSlot( iSlot ) );
	}
	else
	{
		Assert( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT );
		Assert( iSlot >= -1 && iSlot < CLASS_LOADOUT_POSITION_COUNT );
	}

	int iCount = m_LocalInventory.GetItemCount();
	for ( int i = 0; i < iCount; i++ )
	{
		CEconItemView *pItem = m_LocalInventory.GetItem(i);
		CTFItemDefinition *pItemData = pItem->GetStaticData();

		if ( bIsAccountIndex != ( pItemData->GetEquipType() == EEquipType_t::EQUIP_TYPE_ACCOUNT ) )
			continue;

		if ( !bIsAccountIndex && !pItemData->CanBeUsedByClass(iClass) )
			continue;

		// Passing in iSlot of -1 finds all items usable by the class
		if ( iSlot >= 0 && pItem->GetStaticData()->GetLoadoutSlot( iClass ) != iSlot )
			continue;

		pList->AddToTail( m_LocalInventory.GetItem(i) );
	}

	return pList->Count();
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CTFInventoryManager::GetItemInLoadoutForClass( int iClass, int iSlot, CSteamID *pID )
{
#ifdef CLIENT_DLL
	CSteamID localSteamID;
	if ( !pID )
	{
		// If they didn't specify a steamID, use the local player
		if ( !steamapicontext || !steamapicontext->SteamUser() )
			return NULL;

		localSteamID = steamapicontext->SteamUser()->GetSteamID();
		pID = &localSteamID;
	}
#endif

	CTFPlayerInventory *pInv = GetInventoryForPlayer( *pID );
	if ( !pInv )
		return GetBaseItemForClass( iClass, iSlot );

	return pInv->GetItemInLoadout( iClass, iSlot );
}

CEconItemView *CTFInventoryManager::GetItemInLoadoutForAccount( int iSlot, CSteamID *pID )
{
#ifdef CLIENT_DLL
	if ( !pID )
	{
		// If they didn't specify a steamID, use the local player
		if ( !steamapicontext || !steamapicontext->SteamUser() )
			return NULL;

		CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
		pID = &localSteamID;
	}
#endif

	CTFPlayerInventory *pInv = GetInventoryForPlayer( *pID );
	if ( !pInv )
		return NULL;

	return pInv->GetItemInLoadout( GEconItemSchema().GetAccountIndex(), iSlot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerInventory *CTFInventoryManager::GetInventoryForPlayer( const CSteamID &playerID )
{
	for ( int i = 0; i < m_pInventories.Count(); i++ )
	{
		if ( m_pInventories[i].pInventory->GetOwner() != playerID )
			continue;

		return assert_cast<CTFPlayerInventory*>( m_pInventories[i].pInventory );
	}

	return NULL;
}

#ifdef CLIENT_DLL

void CTFInventoryManager::Update( float frametime )
{
	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 );
	m_LocalInventory.UpdateWeaponSkinRequest();

	BaseClass::Update( frametime );
}

#endif

bool CTFInventoryManager::SlotContainsBaseItems( EEquipType_t eType, int iSlot )
{
	Assert( eType != EEquipType_t::EQUIP_TYPE_INVALID );

	if ( eType == EEquipType_t::EQUIP_TYPE_ACCOUNT )
	{
		return !IsQuestSlot( iSlot );
	}

	// Passtime gun
	if ( (iSlot == LOADOUT_POSITION_UTILITY) && TFGameRules() && TFGameRules()->IsPasstimeMode() )
	{
		return true;
	}

	// Halloween spellbook
	if ( iSlot == LOADOUT_POSITION_ACTION )
	{
		if ( TFGameRules() && TFGameRules()->IsUsingSpells() )
			return true;

		if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() )
			return true;
	}
	// Normal game
	return iSlot < LOADOUT_POSITION_HEAD;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the item data for the base item in the loadout slot for a given class
//-----------------------------------------------------------------------------
CEconItemView *CTFInventoryManager::GetBaseItemForClass( int iClass, int iSlot )
{
	// There is no base account item
	if ( iClass == GEconItemSchema().GetAccountIndex() )
		return m_pDefaultItem;

	if ( !HushAsserts() )
	{
		AssertMsg( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT, "Invalid TF_CLASS_: %d", iClass );
	}
	Assert( iSlot >= 0 && iSlot < CLASS_LOADOUT_POSITION_COUNT );

	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_CLASS_COUNT || iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return m_pDefaultItem;

	// Halloween spellbook
	if ( iSlot == LOADOUT_POSITION_ACTION )
	{
		CUtlVector< item_definition_index_t > stockActionItemDefIndices;

		static CSchemaItemDefHandle pItemDef_SpellBook( "TF_WEAPON_SPELLBOOK" );
		if ( TFGameRules() && TFGameRules()->IsUsingSpells() && pItemDef_SpellBook )
		{
			stockActionItemDefIndices.AddToTail( pItemDef_SpellBook->GetDefinitionIndex() );
		}

		static CSchemaItemDefHandle pItemDef_GrapplingHook( "TF_WEAPON_GRAPPLINGHOOK" );
		if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() && pItemDef_GrapplingHook )
		{
			stockActionItemDefIndices.AddToTail( pItemDef_GrapplingHook->GetDefinitionIndex() );
		}

		static CSchemaItemDefHandle pItemDef_MvMCanteen( "Default Power Up Canteen (MvM)" );
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pItemDef_MvMCanteen )
		{
			stockActionItemDefIndices.AddToTail( pItemDef_MvMCanteen->GetDefinitionIndex() );
		}
		
		// Traverse List
		for ( CEconItemView *pActionItem : m_pBaseLoadoutItems )
		{
			for ( item_definition_index_t defIndex : stockActionItemDefIndices )
			{
				if ( pActionItem->GetItemDefIndex() == defIndex )
					return pActionItem;
			}
		}
	}

	if ( iSlot >= LOADOUT_POSITION_HEAD )
		return m_pDefaultItem;

	// Traverse List
	FOR_EACH_VEC( m_pBaseLoadoutItems, iItem )
	{
		if ( m_pBaseLoadoutItems[iItem]->GetItemDefinition()->GetLoadoutSlot( iClass ) == iSlot )
			return m_pBaseLoadoutItems[iItem];
	}

	return m_pDefaultItem;
}

//-----------------------------------------------------------------------------
// Purpose: We're generating a base item. We need to add the game-specific keys to the criteria so that it'll find the right base item.
//-----------------------------------------------------------------------------
void CTFInventoryManager::AddBaseItemCriteria( baseitemcriteria_t *pCriteria, CItemSelectionCriteria *pSelectionCriteria )
{
	pSelectionCriteria->BAddCondition( "used_by_classes", k_EOperator_Subkey_Contains, ItemSystem()->GetItemSchema()->GetClassUsabilityStrings()[pCriteria->iClass], true );
	pSelectionCriteria->BAddCondition( "item_slot", k_EOperator_String_EQ, ItemSystem()->GetItemSchema()->GetLoadoutStrings( EEquipType_t::EQUIP_TYPE_CLASS )[pCriteria->iSlot], true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerInventory::CTFPlayerInventory()
{
#ifdef CLIENT_DLL
	for ( int i = 0; i < TF_TEAM_COUNT; ++i ) 
		m_CachedBaseTextureLowRes[ i ].SetLessFunc( DefLessFunc( itemid_t ) );

	memset(m_ActivePreset, LOADOUT_SLOT_USE_BASE_ITEM, sizeof(m_ActivePreset));
	memset(m_PresetItems, LOADOUT_SLOT_USE_BASE_ITEM, sizeof(m_PresetItems));
#endif

	memset( m_LoadoutItems, LOADOUT_SLOT_USE_BASE_ITEM, sizeof( m_LoadoutItems ) );
	memset( m_AccountLoadoutItems, LOADOUT_SLOT_USE_BASE_ITEM, sizeof( m_AccountLoadoutItems ) );
	ClearClassLoadoutChangeTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerInventory::~CTFPlayerInventory()
{
#ifdef CLIENT_DLL
	for ( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		FOR_EACH_MAP_FAST( m_CachedBaseTextureLowRes[ iTeam ], i )
			m_CachedBaseTextureLowRes[ iTeam ][ i ]->Release();
		m_CachedBaseTextureLowRes[ iTeam ].RemoveAll();
	}
#endif
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::CheckSaxtonMaskAchievement( const CEconItem *pEconItem )
{
	if ( pEconItem )
	{
		if ( pEconItem->GetDefinitionIndex() == 277 && pEconItem->GetOrigin() == kEconItemOrigin_Crafted )	// Saxton mask is item index 277
		{
			g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HALLOWEEN_CRAFT_SAXTON_MASK );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::UpdateCachedServerLoadoutItems()
{
	V_memcpy( m_CachedServerLoadoutItems, m_LoadoutItems, sizeof( itemid_t ) * ARRAYSIZE( m_CachedServerLoadoutItems ) * ARRAYSIZE( m_CachedServerLoadoutItems[0] ) );
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::UpdateRealTFLoadoutItems()
{
	V_memcpy( m_RealTFLoadoutItems, m_LoadoutItems, sizeof( itemid_t ) * ARRAYSIZE( m_RealTFLoadoutItems ) * ARRAYSIZE( m_RealTFLoadoutItems[0] ) );
}

void CTFPlayerInventory::LoadLocalLoadout()
{
	if (GetOwner() != steamapicontext->SteamUser()->GetSteamID())
		return;

	if (!g_pFullFileSystem) {
		return;
	}

	KeyValues *pLoadoutKV = new KeyValues("local_loadout");
	if (!pLoadoutKV->LoadFromFile(g_pFullFileSystem, LOCAL_LOADOUT_FILE, "MOD"))
	{
		SaveLocalLoadout( true, true );

		if ( !pLoadoutKV->LoadFromFile( g_pFullFileSystem, LOCAL_LOADOUT_FILE, "MOD" ) )
		{
			Warning( "Unable to parse local_loadout.txt into keyvalues.\n" );
			return;
		}
	}

	KeyValues *pActivePresetKV = pLoadoutKV->FindKey("active_preset");
	if (pActivePresetKV) 
	{
		for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
		{
			const char* pszClassName = g_aPlayerClassNames_NonLocalized[iClass];
			int activePreset = pActivePresetKV->GetInt(pszClassName);
			m_ActivePreset[iClass] = activePreset;
		}
	}

	int numPresets = static_cast<int>(GetItemSchema()->GetNumAllowedItemPresets());
	for (int iPreset = 0; iPreset < numPresets; ++iPreset)
	{
		char szPreset[256];
		V_snprintf(szPreset, sizeof(szPreset), "%i", iPreset);
		KeyValues* pPresetKV = pLoadoutKV->FindKey(szPreset);
		if (!pPresetKV)
			continue;

		FOR_EACH_TRUE_SUBKEY(pPresetKV, pClassKey)
		{
			const char *pszClassName = pClassKey->GetName();
			const int iClass = GetClassIndexFromString(pszClassName, TF_CLASS_COUNT_ALL);

			FOR_EACH_SUBKEY(pClassKey, pLoadoutEntry)
			{
				const int iSlot = V_atoi(pLoadoutEntry->GetName());
				const itemid_t uItemId = pLoadoutEntry->GetUint64();

				m_PresetItems[iPreset][iClass][iSlot] = uItemId;

				if (iPreset == m_ActivePreset[iClass]) {
					m_LoadoutItems[iClass][iSlot] = uItemId;

					CEconItemView *pItem = GetInventoryItemByItemID(uItemId);
					if (pItem) {
						pItem->GetSOCData()->Equip(iClass, iSlot);
					}
				}
			}
		}
	}

	pLoadoutKV->deleteThis();

	GTFGCClientSystem()->LocalInventoryChanged();
}

//-----------------------------------------------------------------------------
// Purpose: If we are in mod mode, we track loadout changes locally.
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SaveLocalLoadout( bool bReset, bool bDefaultToGC )
{
	if (GetOwner() != steamapicontext->SteamUser()->GetSteamID())
		return;

	if (!g_pFullFileSystem) {
		return;
	}

	KeyValues *pLoadoutKV = new KeyValues("local_loadout");

	KeyValues *pActivePresetKV = new KeyValues("active_preset");
	for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
	{
		const char* pszClassName = g_aPlayerClassNames_NonLocalized[iClass];
		pActivePresetKV->SetInt(pszClassName, m_ActivePreset[iClass]);
	}
	pLoadoutKV->AddSubKey(pActivePresetKV);

	int numPresets = static_cast<int>(GetItemSchema()->GetNumAllowedItemPresets());
	for (int iPreset = 0; iPreset < numPresets; ++iPreset)
	{
		char szPreset[256];
		V_snprintf(szPreset, sizeof(szPreset), "%i", iPreset);
		KeyValues *pPresetKV = new KeyValues(szPreset);
		pLoadoutKV->AddSubKey(pPresetKV);

		for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
		{
			const char* pszClassName = g_aPlayerClassNames_NonLocalized[iClass];

			KeyValues *pClassKV = new KeyValues(pszClassName);
			pPresetKV->AddSubKey(pClassKV);

			for (int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot)
			{
				char szSlot[256];
				V_snprintf(szSlot, sizeof(szSlot), "%i", iSlot);

				itemid_t uItemId = m_PresetItems[iPreset][iClass][iSlot];
				//itemid_t uItemId = m_LoadoutItems[iClass][iSlot];
				if (bReset) {
					uItemId = ( bDefaultToGC && iPreset == 0 ) ? m_RealTFLoadoutItems[iClass][iSlot] : 0;
				}

				pClassKV->SetUint64(szSlot, uItemId);
			}
		}
	}

	pLoadoutKV->SaveToFile(g_pFullFileSystem, LOCAL_LOADOUT_FILE, "MOD");

	pLoadoutKV->deleteThis();
}


#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: If we are in mod mode, we track loadout changes locally.
//-----------------------------------------------------------------------------
void CTFPlayerInventory::EquipLocal(uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot)
{
	// These interactions normally result from a round-trip with the GC.
	// We will never get those messages, so we do everything locally.

	// Unequip whatever was previously in the slot.
	{
		itemid_t ulPreviousItem = m_LoadoutItems[unClass][unSlot];
		CEconItemView *pPreviousItem = GetInventoryItemByItemID(ulPreviousItem);
		if (pPreviousItem) {
			pPreviousItem->GetSOCData()->UnequipFromClass(unClass);
		}
	}

	// Equip the new item and add it to our loadout.
	CEconItemView *pItem = GetInventoryItemByItemID(ulItemID);
	if ( pItem )
	{
		pItem->GetSOCData()->Equip(unClass, unSlot);
	}

	m_LoadoutItems[unClass][unSlot] = ulItemID;

#ifdef CLIENT_DLL
	int activePreset = m_ActivePreset[unClass];
	m_PresetItems[activePreset][unClass][unSlot] = ulItemID;

	GTFGCClientSystem()->LocalInventoryChanged();
#endif
}

void CTFPlayerInventory::UnequipLocal(uint64 ulItemID)
{
	for (int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass)
	{
		for (int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot)
		{
			if (m_LoadoutItems[iClass][iSlot] == ulItemID) {
				m_LoadoutItems[iClass][iSlot] = 0;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	BaseClass::SOUpdated( steamIDOwner, pObject, eEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a script item and associates it with this econ item
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	BaseClass::SOCreated( steamIDOwner, pObject, eEvent );

	if ( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;

	CEconItem *pEconItem = (CEconItem *)pObject;
 //	CEconItem *pEconItem = assert_cast<CEconItem*>( pObject );

#ifdef CLIENT_DLL
	if ( InventoryManager()->GetLocalInventory() == this && GetOwner() == steamIDOwner )
	{
		if ( pObject->GetTypeID() == CEconItem::k_nTypeID )
		{
			CheckSaxtonMaskAchievement( (CEconItem*)pObject );
		}
	}

//	CSteamID ownerSteamID( pEconItem->GetAccountID(), GetUniverse(), k_EAccountTypeIndividual );
//	if ( ownerSteamID == ClientSteamContext().GetLocalPlayerSteamID() )
//	{
//		CheckSaxtonMaskAchievement( pEconItem );
//	}

	static CSchemaItemDefHandle pItemDef_HardyLaurel( "The Hardy Laurel" );
	if ( pEconItem->GetItemDefinition() == pItemDef_HardyLaurel )
	{
		if ( TFSharedContentManager() )
		{
			TFSharedContentManager()->OfferSharedVision( TF_VISION_FILTER_ROME, pEconItem->GetAccountID() );
		}
	}
 #else
	// Summer 2015 Operation Pass so players can display the coin
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_Summer2015Operation( "Activated Summer 2015 Operation Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_Summer2015Operation )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_SUMMER2015 );
		}
	}

	// Invasion Community Update Pass so players can display the coin
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_InvasionPass( "Activated Invasion Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_InvasionPass )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_INVASION );
		}
	}

	// Halloween Pass so players can display the coin
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_HalloweenPass( "Activated Halloween Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_HalloweenPass )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_HALLOWEEN );
		}
	}

	// Winter2016 Pass so players can display the stamp
	// remove this when we have a coin equip slot
	static CSchemaItemDefHandle pItemDef_Winter2016Pass( "Activated Operation Tough Break Pass" );
	if ( pEconItem->GetItemDefinition() == pItemDef_Winter2016Pass )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( GetOwner() ) );
		if ( pPlayer )
		{
			pPlayer->SetCampaignMedalActive( CAMPAIGN_MEDAL_WINTER2016 );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::DumpInventoryToConsole( bool bRoot )
{
	if ( bRoot )
	{
		Msg("========================================\n");
#ifdef CLIENT_DLL
		Msg("(CLIENT) Inventory:\n");
#else
		Msg("(SERVER) Inventory for account (%d):\n", m_OwnerID.GetAccountID() );
#endif
		Msg("  Version: %llu:\n", m_pSOCache ? m_pSOCache->GetVersion() : -1 );
	}

	int iCount = m_aInventoryItems.Count();
	Msg("   Num items: %d\n", iCount );
	for ( int i = 0; i < iCount; i++ )
	{
		Msg("      %s (ID %llu) at backpack slot %d\n", m_aInventoryItems[i].GetStaticData()->GetDefinitionName(), m_aInventoryItems[i].GetItemID(), 0 );

		int iEquipped = 0;
		for ( equipped_class_t eq = TF_FIRST_NORMAL_CLASS; eq < TF_LAST_NORMAL_CLASS; eq++ )
		{
			if ( m_aInventoryItems[i].IsEquippedForClass( eq ) )
			{
				if ( iEquipped == 0 )
				{
					iEquipped++;
					Msg("         -> EQUIPPED: ");
				}
				Msg("%s ", g_aPlayerClassNames_NonLocalized[eq] );
			}
		}

		if ( iEquipped )
		{
			Msg("\n");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ClearClassLoadoutChangeTracking( void )
{
	memset(m_bLoadoutChanged,0, sizeof(m_bLoadoutChanged));
#ifdef CLIENT_DLL
	UpdateCachedServerLoadoutItems();
#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Find the low res 
//-----------------------------------------------------------------------------
ITexture *CTFPlayerInventory::GetWeaponSkinBaseLowRes( itemid_t nItemId, int iTeam ) const
{
	Assert( iTeam >= 0 && iTeam < TF_TEAM_COUNT );
	int index = m_CachedBaseTextureLowRes[ iTeam ].Find( nItemId );
	if ( index == m_CachedBaseTextureLowRes[ iTeam ].InvalidIndex() )
		return NULL;

	return m_CachedBaseTextureLowRes[ iTeam ][ index ];
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Find the first item in the local user's inventory with the given def index
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayerInventory::GetFirstItemOfItemDef( item_definition_index_t nDefIndex, CPlayerInventory* pInventory )
{
#ifdef CLIENT_DLL
	if ( pInventory == NULL )
	{
		pInventory = InventoryManager()->GetLocalInventory();
	}
#endif

	if ( !pInventory )
		return NULL;

	for ( int i = 0; i < pInventory->GetItemCount(); i++ )
	{
		CEconItemView *pItem = pInventory->GetItem(i);
		if ( pItem->GetItemDefIndex() == nDefIndex )
		{
			return pItem;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the item in the specified loadout slot for a given class
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayerInventory::GetItemInLoadout( int iClass, int iSlot )
{
	if ( iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return NULL;

	if ( iClass == GEconItemSchema().GetAccountIndex() )
	{
		return GetInventoryItemByItemID( m_AccountLoadoutItems[ iSlot ] );
	}
	else
	{
		if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS  )
			return NULL;

		// If we don't have an item in the loadout at that slot, we return the base item
		if ( m_LoadoutItems[iClass][iSlot] != LOADOUT_SLOT_USE_BASE_ITEM )
		{
			CEconItemView *pItem = GetInventoryItemByItemID( m_LoadoutItems[iClass][iSlot] );

			// To protect against users lying to the backend about the position of their items,
			// we need to validate their position on the server when we retrieve them.
			if ( pItem && AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItem->GetStaticData()->GetLoadoutSlot( iClass ), iSlot ) )
				return pItem;
		}
	}

	return TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
}

#ifdef CLIENT_DLL
CEconItemView *CTFPlayerInventory::GetCacheServerItemInLoadout( int iClass, int iSlot )
{
	if ( iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return NULL;
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
		return NULL;

	// If we don't have an item in the loadout at that slot, we return the base item
	if ( m_CachedServerLoadoutItems[iClass][iSlot] != LOADOUT_SLOT_USE_BASE_ITEM )
	{
		CEconItemView *pItem = GetInventoryItemByItemID( m_CachedServerLoadoutItems[iClass][iSlot] );

		// To protect against users lying to the backend about the position of their items,
		// we need to validate their position on the server when we retrieve them.
		if ( pItem && AreSlotsConsideredIdentical( pItem->GetStaticData()->GetEquipType(), pItem->GetStaticData()->GetLoadoutSlot( iClass ), iSlot ) )
			return pItem;
	}

	return TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static CEconGameAccountClient *GetSOCacheGameAccountClient( CGCClientSharedObjectCache *pSOCache )
{
	if ( !pSOCache )
		return NULL;

	return pSOCache->GetSingleton<CEconGameAccountClient>();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerInventory::GetPreviewItemDef( void ) const
{
	CEconGameAccountClient *pGameAccountClient = GetSOCacheGameAccountClient( m_pSOCache );
	if ( !pGameAccountClient  )
		return 0;

	return pGameAccountClient->Obj().preview_item_def();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::CanPurchaseItems( int iItemCount ) const
{
	// If we're not a free trial account, we fall back to our default logic of "do
	// we have enough empty slots?".
	CEconGameAccountClient *pGameAccountClient = GetSOCacheGameAccountClient( m_pSOCache );
	if ( !pGameAccountClient || !pGameAccountClient->Obj().trial_account() )
		return BaseClass::CanPurchaseItems( iItemCount );

	// We're a free trial account, so when we purchase these items, our inventory
	// will actually expand. We check to make sure that we have room for these
	// items against what will be our new maximum backpack size, not our current
	// backpack limit.
	int iNewItemCount			   = GetItemCount() + iItemCount,
		iAfterPurchaseMaxItemCount = DEFAULT_NUM_BACKPACK_SLOTS
								   + (pGameAccountClient ? pGameAccountClient->Obj().additional_backpack_slots() : 0);

	return iNewItemCount <= iAfterPurchaseMaxItemCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerInventory::GetMaxItemCount( void ) const
{
	int iMaxItems = DEFAULT_NUM_BACKPACK_SLOTS;
	CEconGameAccountClient *pGameAccountClient = GetSOCacheGameAccountClient( m_pSOCache );
	if ( pGameAccountClient )
	{
		if ( pGameAccountClient->Obj().trial_account() )
		{
			iMaxItems = DEFAULT_NUM_BACKPACK_SLOTS_FREE_TRIAL_ACCOUNT;
		}
		iMaxItems += pGameAccountClient->Obj().additional_backpack_slots();
	}
	return MIN( iMaxItems, MAX_NUM_BACKPACK_SLOTS );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Removes any item in a loadout slot. If the slot has a base item,
//			the player essentially returns to using that item.
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::ClearLoadoutSlot( int iClass, int iSlot )
{
	if ( iSlot < 0 || iSlot >= CLASS_LOADOUT_POSITION_COUNT )
		return false;

	if ( iClass == GEconItemSchema().GetAccountIndex() )
	{
		if ( m_AccountLoadoutItems[iSlot] == LOADOUT_SLOT_USE_BASE_ITEM )
			return false;
	}
	else
	{
		if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
			return false;

		if ( m_LoadoutItems[iClass][iSlot] == LOADOUT_SLOT_USE_BASE_ITEM )
			return false;
	}
	
	CEconItemView *pItemInSlot = GetItemInLoadout( iClass, iSlot );
	if ( !pItemInSlot )
		return false;

	InventoryManager()->UpdateInventoryEquippedState( this, INVALID_ITEM_ID, iClass, iSlot );

	// TODO: Prediction
	// It's been moved to the backpack, so clear out loadout entry
	//m_LoadoutItems[iClass][iSlot] = LOADOUT_SLOT_USE_BASE_ITEM;
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Update weapon skin request list
//-----------------------------------------------------------------------------
void CTFPlayerInventory::UpdateWeaponSkinRequest()
{
	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 );

	FOR_EACH_VEC_BACK( m_vecWeaponSkinRequestList, i )
	{
		SkinRequest_t &req = m_vecWeaponSkinRequestList[i];
		CEconItemView *pItem = GetInventoryItemByItemID( req.m_nID );
		Assert( pItem );
		if ( !pItem ) 
		{
			m_vecWeaponSkinRequestList.Remove( i );
			continue;
		}

		// Have we loaded this one yet? If not, do it here.
		if ( mdlcache->IsErrorModel( req.m_hModel ) )
		{
			const char *pszModelName = pItem->GetPlayerDisplayModel( 0, 0 );
			if ( pszModelName )
			{
				MDLHandle_t hItemMDL = mdlcache->FindMDL( pszModelName );
				if ( mdlcache->IsErrorModel( hItemMDL ) )
				{
					AssertMsg( 0, "failed to find %s from mdlcache", pszModelName );
					m_vecWeaponSkinRequestList.Remove( i );
					continue;
				}

				req.m_hModel = hItemMDL;
			}
			else
			{
				AssertMsg( 0, "failed to precache weapon skin because there's no model" );
				m_vecWeaponSkinRequestList.Remove( i );
			}
		}
		
		// Draw!
		{
			CMDL mdl;
			mdl.SetMDL( req.m_hModel );
			mdl.m_pProxyData = static_cast<IClientRenderable*>( pItem );

			pItem->SetWeaponSkinUseLowRes( true );
			int nRestoreTeam = pItem->GetTeamNumber();
			pItem->SetTeamNumber( req.m_nTeam );

			matrix3x4_t matIdentity;
			SetIdentityMatrix( matIdentity );
			IMaterial *pOverrideMaterial = pItem->GetMaterialOverride( pItem->GetTeamNumber() );

			if ( pOverrideMaterial )
				modelrender->ForcedMaterialOverride( pOverrideMaterial );

			mdl.Draw( matIdentity );

			if ( pOverrideMaterial )
				modelrender->ForcedMaterialOverride( NULL );

			pItem->SetTeamNumber( nRestoreTeam );
			pItem->SetWeaponSkinUseLowRes( false );
		}

		// Don't remove until it's complete. 
		if ( pItem->GetWeaponSkinBase() )
		{
			Assert( pItem->GetWeaponSkinBaseCompositor() == NULL );
			ITexture* pTex = pItem->GetWeaponSkinBase();
			if ( pTex )
			{
				pTex->AddRef(); // We need to hold a ref to the texture.
				m_CachedBaseTextureLowRes[ req.m_nTeam ].Insert( req.m_nID, pTex );
				pItem->SetWeaponSkinBase( NULL ); // Clean up.
			}

			// We do RED, then BLUE. Then drop it out of the list. 
			if ( req.m_nTeam == TF_TEAM_BLUE )
			{
				Assert( !mdlcache->IsErrorModel( req.m_hModel ) );
				mdlcache->Release( req.m_hModel );
				m_vecWeaponSkinRequestList.Remove( i );
			}
			else
			{
				Assert( req.m_nTeam == TF_TEAM_RED );
				req.m_nTeam = TF_TEAM_BLUE;
			}
		}

		// Only do one of these per frame to avoid hitching.
		break;
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ItemHasBeenUpdated( CEconItemView *pItem, bool bUpdateAckFile, bool bWriteAckFile )
{
	Assert( pItem );

	BaseClass::ItemHasBeenUpdated( pItem, bUpdateAckFile, bWriteAckFile );
	const CEconItem *pEconItem = pItem->GetSOCData();

#ifdef CLIENT_DLL
	bool bLocalInv = InventoryManager()->GetLocalInventory() == this;
#endif // CLIENT_DLL

	for ( equipped_class_t iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		equipped_slot_t unSlot = pEconItem ? pEconItem->GetEquippedPositionForClass( iClass ) : INVALID_EQUIPPED_SLOT;

		Assert( GetInventoryItemByItemID( pItem->GetItemID() ) );

		bool bThisClassLoadoutChanged = UpdateEquipStateForClass( pItem->GetItemID(), unSlot, m_LoadoutItems[iClass], ARRAYSIZE( m_LoadoutItems[iClass] ) );

		if ( bThisClassLoadoutChanged )
		{
			m_bLoadoutChanged[iClass] = true;
		}
	}

	// Update account items as well
	equipped_slot_t unSlot = pEconItem ? pEconItem->GetEquippedPositionForClass( GEconItemSchema().GetAccountIndex() ) : INVALID_EQUIPPED_SLOT;
	UpdateEquipStateForClass( pItem->GetItemID(), unSlot, m_AccountLoadoutItems, ARRAYSIZE( m_AccountLoadoutItems ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::UpdateEquipStateForClass( const itemid_t& itemID, equipped_slot_t nSlot, itemid_t *pLoadout, int nCount )
{
	bool bThisClassLoadoutChanged = false;

	// For each slot this item may have been equipped in, set to the base item, unless that slot
	// is the current slot. (ie., if an item moves from slot 10 to slot 12, slot 10 will be set to
	// base item, and slot 12 will be set to this item ID)
	for ( int i = 0; i < nCount; i++ )
	{
		itemid_t& refLoadoutItemID = *( pLoadout + i );

		if ( i == nSlot )
		{
			// This may be an invalid slot for the item, but CTFPlayerInventory::InventoryReceived()
			// will have detected that and sent off a request already to move it. The response
			// to that will clear this loadout slot.
			refLoadoutItemID = itemID;
			bThisClassLoadoutChanged = true;
		}
		else if ( refLoadoutItemID == itemID )
		{
			refLoadoutItemID = LOADOUT_SLOT_USE_BASE_ITEM;
			bThisClassLoadoutChanged = true;
		}
	}

	return bThisClassLoadoutChanged;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::PostSOUpdate( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	BaseClass::PostSOUpdate( steamIDOwner, eEvent );

	VerifyChangedLoadoutsAreValid();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	BaseClass::SOCacheSubscribed( steamIDOwner, eEvent );

	UpdateRealTFLoadoutItems();
	LoadLocalLoadout();

	VerifyChangedLoadoutsAreValid();
	UpdateCachedServerLoadoutItems();
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add a new item for a econ item
//-----------------------------------------------------------------------------
bool CTFPlayerInventory::AddEconItem( CEconItem * pItem, bool bUpdateAckFile, bool bWriteAckFile, bool bCheckForNewItems )
{
	if ( BaseClass::AddEconItem( pItem, bUpdateAckFile, bWriteAckFile, bCheckForNewItems ) )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::VerifyLoadoutItemsAreValid( int iClass )
{
	// Important note: currently this function walks the loadout slots in order causing slots towards
	// the beginning of the list to take priority over slots later in the list. This means that currently
	// weapons take priority over hats, hats take priority over misc slots, etc. which is all well and good.
	// If later we want the order in which slots claim their equip regions to change, we'll want to change
	// the iteration order here and also change GenerateEquipRegionMaskUpToSlot(), which is used for
	// filling out the UI.
	equip_region_mask_t unCumulativeRegionMask = 0;
	for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
	{
		CEconItemView *pEquippedItemView = GetItemInLoadout( iClass, i );
		if ( !pEquippedItemView )
			continue;

		// Does this item use the same regions as some item that we already have equipped?
		equip_region_mask_t unItemEquipMask = pEquippedItemView->GetItemDefinition()->GetEquipRegionMask();
		if ( unItemEquipMask & unCumulativeRegionMask )
		{
			// Unequip this item. This will wind up calling into ::ItemHasBeenUpdated() once the
			// unequip makes it to the GC and back.
			InventoryManager()->UpdateInventoryEquippedState( this, INVALID_ITEM_ID, iClass, pEquippedItemView->GetEquippedPositionForClass( iClass ) );
		}
		else
		{
			unCumulativeRegionMask |= unItemEquipMask;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::VerifyChangedLoadoutsAreValid()
{
	// We assume that each local client will verify their own inventory, but we don't want my client to verify that
    // your loadout is valid -- we expect that'll be done either by the server or the GC or, worst case, your local
    // client.
    //
    // A note on this: this change is being made because some malicious users are intentionally equipping invalid sets
    // of items on bots (and players?) and joining servers. Later on, this code calls InventoryManager() functions to
    // remove the problematic items, but the inventory manager is a global for the local player. This means that another
    // played connected with an invalid inventory can cause changes in your inventory because we don't distinguish
    // self/other state past this point.
    if ( GetOwner() != steamapicontext->SteamUser()->GetSteamID() )
        return;

	// We maybe changed equip state for an item and it's possible if we're sending bad messages and/or
	// hacking state that we'll now have conflicting items equipped. Walk all of our items for this class
	// to verify our state is good.
	for ( equipped_class_t iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( m_bLoadoutChanged[iClass] )
		{
			VerifyLoadoutItemsAreValid( iClass );
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerInventory::ItemIsBeingRemoved( CEconItemView *pItem )
{
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( pItem->IsEquippedForClass( iClass ) )
		{
			for ( int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; iSlot++ )
			{
				if ( m_LoadoutItems[iClass][iSlot] == pItem->GetItemID() )
				{
					m_LoadoutItems[iClass][iSlot] = LOADOUT_SLOT_USE_BASE_ITEM;
					m_bLoadoutChanged[iClass] = true;
				}
			}
		}
	}
}


//=======================================================================================================================
// TF PLAYER INVENTORY
//=======================================================================================================================

#ifdef CLIENT_DLL
// WTF: Declaring this inline caused a compiler bug.
CTFPlayerInventory	*CTFInventoryManager::GetLocalTFInventory( void ) 
{ 
	return &m_LocalInventory; 
}
#endif

#ifdef CLIENT_DLL
void CTFInventoryManager::UpdateInventoryEquippedState(CPlayerInventory *pInventory, uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot)
{
	BaseClass::UpdateInventoryEquippedState(pInventory, ulItemID, unClass, unSlot);

	CTFPlayerInventory* pTFInventory = dynamic_cast<CTFPlayerInventory*>(pInventory);
	if (pTFInventory) 
	{
		pTFInventory->EquipLocal(ulItemID, unClass, unSlot);
		pTFInventory->SaveLocalLoadout();
	}
}

#endif

#ifdef _DEBUG
#if defined(CLIENT_DLL)
CON_COMMAND_F( item_dumpinv_other, "Dumps the contents of a specified client inventory. Format: item_dumpinv_other <player index>", FCVAR_NONE )
{
	if ( args.ArgC() > 1 )
	{
		C_TFPlayer *pOther = ToTFPlayer( UTIL_PlayerByIndex( atoi(args[1]) ) );
		if ( pOther )
		{
			pOther->Inventory()->DumpInventoryToConsole( true );
			return;
		}
	}
	Msg("Couldn't find specified player.\nFormat: item_dumpinv_other <player index>\n");
}
#endif
#endif // _DEBUG

#ifdef _DEBUG
#if defined(CLIENT_DLL)
CON_COMMAND_F( item_dumpinv, "Dumps the contents of a specified client inventory. Format: item_dumpinv", FCVAR_CHEAT )
#else
CON_COMMAND_F( item_dumpinv_sv, "Dumps the contents of a specified server inventory. Format: item_dumpinv_sv", FCVAR_CHEAT )
#endif
{
#if defined(CLIENT_DLL)
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
#else
	CSteamID steamID;
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() ); 
	pPlayer->GetSteamID( &steamID );
#endif

	CPlayerInventory *pInventory = TFInventoryManager()->GetInventoryForPlayer( steamID );
	if ( !pInventory )
	{
		Msg("No inventory for that player.\n");
		return;
	}

	pInventory->DumpInventoryToConsole( true );
}
#endif

#ifdef _DEBUG
#if defined(CLIENT_DLL)
CON_COMMAND_F( item_deleteunknowns, "Deletes all items in your inventory that we don't have static data for. Useful for removing items that have been removed from the backend.", FCVAR_CHEAT )
{
	int iDeleted = TFInventoryManager()->DeleteUnknowns( InventoryManager()->GetLocalInventory() );
	Msg("Deleted %d unknown items.\n", iDeleted);
}
#endif
#endif


#if defined( TF_CLIENT_DLL )
CON_COMMAND( load_itempreset, "Equip all items for a given preset on the player." )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( args.ArgC() != 2 )
	{
		Msg( "Usage: \"load_itempreset <preset index>\" - <preset index> can be 0 to 3." );
		return;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return;

	equipped_class_t unClass = pPlayer->GetPlayerClass()->GetClassIndex();
	equipped_preset_t unPreset = atoi( args[1] );
	if ( TFInventoryManager()->LoadPreset( unClass, unPreset ) )
	{
		// Tell the GC to tell server that we should respawn if we're in a respawn room
		extern ConVar tf_respawn_on_loadoutchanges;
		if ( tf_respawn_on_loadoutchanges.GetBool() )
		{
			GCSDK::CGCMsg< ::MsgGCEmpty_t > msg( k_EMsgGCRespawnPostLoadoutChange );
			GCClientSystem()->BSendMessage( msg );
		}
	}
}
#endif	// TF_CLIENT_DLL

#if defined( TF_CLIENT_DLL ) && INVENTORY_VIA_WEBAPI
CON_COMMAND(save_loadout, "Save local loadout.")
{
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	CTFPlayerInventory* pInventory = TFInventoryManager()->GetInventoryForPlayer(steamID);
	if (!pInventory)
		return;

	pInventory->SaveLocalLoadout();
}

CON_COMMAND(reset_loadout, "Reset local loadout to what is active on TF2.")
{
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	CTFPlayerInventory* pInventory = TFInventoryManager()->GetInventoryForPlayer(steamID);
	if (!pInventory)
		return;

	pInventory->SaveLocalLoadout(true, true);
	pInventory->LoadLocalLoadout();
}

CON_COMMAND(clear_loadout, "Clear local loadout back to defaults")
{
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();

	CTFPlayerInventory* pInventory = TFInventoryManager()->GetInventoryForPlayer(steamID);
	if (!pInventory)
		return;

	pInventory->SaveLocalLoadout(true, false);
	pInventory->LoadLocalLoadout();
}

CON_COMMAND(reset_loadout_ui, "Reset local loadout to what is active on TF2 (show confirmation)")
{
	ShowConfirmDialog( "#TF_SDK_ResetLoadout_Title",
		"#TF_SDK_ResetLoadout_Desc",
		"#MessageBox_OK",
		"#cancel", []( bool bConfirmed, void* pContext )
		{
			if ( bConfirmed )
				engine->ClientCmd_Unrestricted( "reset_loadout\n" );
		});
}

CON_COMMAND(clear_loadout_ui, "Clear local loadout back to stock defaults (show confirmation)")
{
	ShowConfirmDialog( "#TF_SDK_ClearLoadout_Title",
		"#TF_SDK_ClearLoadout_Desc",
		"#MessageBox_OK",
		"#cancel", []( bool bConfirmed, void* pContext )
		{
			if ( bConfirmed )
				engine->ClientCmd_Unrestricted( "clear_loadout\n" );
		} );
}
#endif	// TF_CLIENT_DLL

#if defined( TF_CLIENT_DLL ) && INVENTORY_VIA_WEBAPI
bool CTFInventoryManager::LoadPreset(equipped_class_t unClass, equipped_preset_t unPreset)
{
	if (!IsValidPlayerClass(unClass))
		return false;

	if (!IsPresetIndexValid(unPreset))
		return false;

	if (!GetLocalInventory()->GetSOC())
		return false;

	if (!steamapicontext || !steamapicontext->SteamUser())
		return false;

	CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
	CTFPlayerInventory *pInv = GetInventoryForPlayer(localSteamID);
	if (!pInv)
		return false;

	if (m_flNextLoadPresetChange > gpGlobals->realtime)
	{
		Msg("Loadout change denied. Changing presets too quickly.\n");
		return false;
	}

	m_flNextLoadPresetChange = gpGlobals->realtime + 0.5f;

	return pInv->EquipLocalPreset(unClass, unPreset);
}

bool CTFPlayerInventory::EquipLocalPreset(equipped_class_t unClass, equipped_preset_t unPreset)
{
	if (!InventoryManager()->IsPresetIndexValid(unPreset))
		return false;

	m_ActivePreset[unClass] = unPreset;

	for (int iSlot = 0; iSlot < CLASS_LOADOUT_POSITION_COUNT; ++iSlot)
	{
		itemid_t uItemId = m_PresetItems[unPreset][unClass][iSlot];
		EquipLocal(uItemId, unClass, iSlot);
	}

	SaveLocalLoadout();

	return true;
}
#endif
