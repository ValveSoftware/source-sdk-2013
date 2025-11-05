//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "econ_item_inventory.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"
#include "econ_item_system.h"
#include "econ_item.h"
#include "econ_gcmessages.h"
#include "shareddefs.h"
#include "filesystem.h"
#include "econ_item_description.h"				// only for CSteamAccountIDAttributeCollector

#ifdef CLIENT_DLL
#include <igameevents.h>
#include "econ_game_account_client.h"
#include "ienginevgui.h"
#include "econ/econ_item_preset.h"
#include "tf_notification.h"
#else
#include "props_shared.h"
#include "basemultiplayerplayer.h"
#endif

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
#include "tf_gcmessages.h"
#include "tf_duel_summary.h"
#include "econ_contribution.h"
#include "tf_player_info.h"
#include <vgui_controls/Controls.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace GCSDK;

#ifdef _DEBUG
ConVar item_inventory_debug( "item_inventory_debug", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
#endif

#ifdef USE_DYNAMIC_ASSET_LOADING
//extern ConVar item_dynamicload;
#endif

#define ITEM_CLIENTACK_FILE			"item_clientacks.txt"

#ifdef _DEBUG
#ifdef CLIENT_DLL
ConVar item_debug_clientacks( "item_debug_clientacks", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
#endif
#endif // _DEBUG

// Result codes strings for GC results.
const char* GCResultString[8] =
{
	"k_EGCMsgResponseOK",			// Request succeeded
	"k_EGCMsgResponseDenied",		// Request denied
	"k_EGCMsgResponseServerError",	// Request failed due to a temporary server error
	"k_EGCMsgResponseTimeout",		// Request timed out
	"k_EGCMsgResponseInvalid",		// Request was corrupt
	"k_EGCMsgResponseNoMatch",		// No item definition matched the request
	"k_EGCMsgResponseUnknownError",	// Request failed with an unknown error
	"k_EGCMsgResponseNotLoggedOn",	// Client not logged on to steam
};

CBasePlayer *GetPlayerBySteamID( const CSteamID &steamID )
{
	CSteamID steamIDPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer == NULL )
			continue;

		if ( pPlayer->GetSteamID( &steamIDPlayer ) == false )
			continue;

		if ( steamIDPlayer == steamID )
			return pPlayer;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CInventoryManager::CInventoryManager( void )
#ifdef CLIENT_DLL
	: m_mapPersonaNamesCache( DefLessFunc( uint32 ) )
	, m_sPersonaStateChangedCallback( this, &CInventoryManager::OnPersonaStateChanged )
	, m_personaNameRequests( DefLessFunc( uint64 ) )
#endif
{
#ifdef CLIENT_DLL
	m_flNextLoadPresetChange = 0.0f;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::SteamRequestInventory( CPlayerInventory *pInventory, CSteamID pSteamID, IInventoryUpdateListener *pListener )
{
	// SteamID must be valid
	if ( !pSteamID.IsValid() || !pSteamID.BIndividualAccount() )
	{
		if ( !HushAsserts() )
		{
			Assert( pSteamID.IsValid() );
			Assert( pSteamID.BIndividualAccount() );
		}
		return;
	}

	// If we haven't seen this inventory before, register it
	bool bFound = false;
	for ( int i = 0; i < m_pInventories.Count(); i++ )
	{
		if ( m_pInventories[i].pInventory == pInventory )
		{
			bFound = true;
			break;
		}
	}
	if ( !bFound )
	{
		int iIdx = m_pInventories.AddToTail();
		m_pInventories[iIdx].pInventory = pInventory;
		m_pInventories[iIdx].pListener = pListener;
	}

	// Add the request to our list of pending requests
	int iIdx = m_hPendingInventoryRequests.AddToTail();
	m_hPendingInventoryRequests[iIdx].pID = pSteamID;
	m_hPendingInventoryRequests[iIdx].pInventory = pInventory;

	pInventory->RequestInventory( pSteamID );

	if( pListener )
	{
		pInventory->AddListener( pListener );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when a gameserver connects to steam.
//-----------------------------------------------------------------------------
void CInventoryManager::GameServerSteamAPIActivated()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerInventory *CInventoryManager::GetInventoryForAccount( uint32 iAccountID )
{
	FOR_EACH_VEC( m_pInventories, i )
	{
		if ( m_pInventories[i].pInventory->GetOwner().GetAccountID() == iAccountID )
			return m_pInventories[i].pInventory;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::DeregisterInventory( CPlayerInventory *pInventory )
{
	int iCount = m_pInventories.Count();
	for ( int i = iCount-1; i >= 0; i-- )
	{
		if ( m_pInventories[i].pInventory == pInventory )
		{
			m_pInventories.Remove(i);
		}
	}
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CInventoryManager::IsPresetIndexValid( equipped_preset_t unPreset )
{
	const bool bResult = GetItemSchema()->IsValidPreset( unPreset );
	AssertMsg( bResult, "Invalid preset index!" );
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CInventoryManager::LoadPreset( equipped_class_t unClass, equipped_preset_t unPreset )
{
	if ( !IsValidPlayerClass( unClass ) )
		return false;

	if ( !IsPresetIndexValid( unPreset ) )
		return false;

	if ( !GetLocalInventory()->GetSOC() )
		return false;

	if ( m_flNextLoadPresetChange > gpGlobals->realtime )
	{
		Msg( "Loadout change denied. Changing presets too quickly.\n" );
		return false;
	}

	m_flNextLoadPresetChange = gpGlobals->realtime + 0.5f;

	GCSDK::CProtoBufMsg<CMsgSelectPresetForClass> msg( k_EMsgGCPresets_SelectPresetForClass );
	msg.Body().set_class_id( unClass );
	msg.Body().set_preset_id( unPreset );
	GCClientSystem()->BSendMessage( msg );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::UpdateLocalInventory( void )
{
	if ( steamapicontext->SteamUser() && GetLocalInventory() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		if ( steamID.IsValid() ) // make sure we're logged in and we know who we are
		{
			SteamRequestInventory( GetLocalInventory(), steamID );
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::OnPersonaStateChanged( PersonaStateChange_t *info )
{
	if ( ( info->m_nChangeFlags & k_EPersonaChangeName ) != 0 )
		m_personaNameRequests.InsertOrReplace( info->m_ulSteamID, true );
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CInventoryManager::Init( void )
{
	// Initialize the item system.
	ItemSystem()->Init();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::PostInit( void )
{
	ItemSystem()->PostInit();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::PreInitGC()
{
	REG_SHARED_OBJECT_SUBCLASS( CEconItem );
	
#if defined (CLIENT_DLL)
	REG_SHARED_OBJECT_SUBCLASS( CEconGameAccountClient );
	REG_SHARED_OBJECT_SUBCLASS( CEconItemPerClassPresetData );
	REG_SHARED_OBJECT_SUBCLASS( CTFNotification );
#endif

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)

	REG_SHARED_OBJECT_SUBCLASS( CTFDuelSummary );
	REG_SHARED_OBJECT_SUBCLASS( CTFMapContribution );
	REG_SHARED_OBJECT_SUBCLASS( CTFPlayerInfo );
#endif

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::PostInitGC()
{
#ifdef CLIENT_DLL
	// The client immediately loads the local player's inventory
	UpdateLocalInventory();
#endif
}


//-----------------------------------------------------------------------------
void CInventoryManager::Shutdown()
{
	int nInventoryCount = m_pInventories.Count();
	for ( int iInventory = 0; iInventory < nInventoryCount; ++iInventory )
	{
		CPlayerInventory *pInventory = m_pInventories[iInventory].pInventory;
		if ( pInventory )
		{
			pInventory->Clear();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::LevelInitPreEntity( void )
{
	// Throw out any testitem definitions
	for ( int i = 0; i < TI_TYPE_COUNT; i++ )
	{
		int iNewDef = TESTITEM_DEFINITIONS_BEGIN_AT + i;
		ItemSystem()->GetItemSchema()->ItemTesting_DiscardTestDefinition( iNewDef );
	}

	// Precache all item models we've got
#ifdef GAME_DLL
	CUtlVector<const char *> vecPrecacheModelStrings;
#endif // GAME_DLL
	const CEconItemSchema::ItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetItemDefinitionMap();
	FOR_EACH_MAP_FAST( mapItemDefs, i )
	{
		CEconItemDefinition *pData = mapItemDefs[i];

		pData->SetHasBeenLoaded( true );

#ifdef GAME_DLL
		bool bDynamicLoad = false;
#ifdef USE_DYNAMIC_ASSET_LOADING
		bDynamicLoad = true;//item_dynamicload.GetBool();
#endif // USE_DYNAMIC_ASSET_LOADING
		pData->GeneratePrecacheModelStrings( bDynamicLoad, &vecPrecacheModelStrings );

		// Precache the models and the gibs for everything the definition requested.
		FOR_EACH_VEC( vecPrecacheModelStrings, i )
		{
			// Ignore any objects which requested an empty precache string for whatever reason.
			if ( vecPrecacheModelStrings[i] && vecPrecacheModelStrings[i][0] )
			{
				int iModelIndex = CBaseEntity::PrecacheModel( vecPrecacheModelStrings[i] );
				PrecacheGibsForModel( iModelIndex );
			}
		}

		vecPrecacheModelStrings.RemoveAll();

		pData->GeneratePrecacheSoundStrings( bDynamicLoad, &vecPrecacheModelStrings );

		// Precache the sounds for everything
		FOR_EACH_VEC( vecPrecacheModelStrings, i )
		{
			// Ignore any objects which requested an empty precache string for whatever reason.
			if ( vecPrecacheModelStrings[i] && vecPrecacheModelStrings[i][0] )
			{
				CBaseEntity::PrecacheScriptSound( vecPrecacheModelStrings[i] );
			}
		}

		vecPrecacheModelStrings.RemoveAll();
#endif
	}

	// We reset the cached attribute class strings, since it's invalidated by level changes
	ItemSystem()->ResetAttribStringCache();

#ifdef GAME_DLL
	ItemSystem()->ReloadWhitelist();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::LevelShutdownPostEntity( void )
{
	// We reset the cached attribute class strings, since it's invalidated by level changes
	ItemSystem()->ResetAttribStringCache();
}


//-----------------------------------------------------------------------------
// Purpose: Lets the client know that we're now ready to mess with inventory
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CInventoryManager::SendItemSystemConnectedEvent( void )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "econ_inventory_connected" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}
#endif


#if !defined(NO_STEAM)
//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the dev "new item" response
//-----------------------------------------------------------------------------
class CGCDev_NewItemRequestResponse : public GCSDK::CGCClientJob
{
public:
	CGCDev_NewItemRequestResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );

		if ( msg.Body().m_eResponse == k_EGCMsgResponseOK )
		{
			Msg("Received new item acknowledgement: %s\n", GCResultString[msg.Body().m_eResponse] );
		}
		else
		{
			Warning("Failed to generate new item: %s\n", GCResultString[msg.Body().m_eResponse] );
		}
		return true;
	}

};
GC_REG_JOB( GCSDK::CGCClient, CGCDev_NewItemRequestResponse, "CGCDev_NewItemRequestResponse", k_EMsgGCDev_NewItemRequestResponse, GCSDK::k_EServerTypeGCClient );

#endif	// NO_STEAM

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::RemovePendingRequest( CSteamID *pSteamID )
{
#ifdef CLIENT_DLL
	// Only the client, all requests are for the local player. Clear them all.
	m_hPendingInventoryRequests.Purge();
	return;
#endif

	// On the server, remove all requests for the specified steam id
	int iCount = m_hPendingInventoryRequests.Count();
	for ( int i = iCount-1; i >= 0; i-- )
	{
		if ( m_hPendingInventoryRequests[i].pID == *pSteamID )
		{
			m_hPendingInventoryRequests.Remove(i);
		}
	}
}

#ifdef CLIENT_DLL

void CInventoryManager::Update( float frametime )
{
	// Check if we have any pending account lookups to batch up
	if ( m_msgPendingLookupAccountNames.accountids_size() > 0 )
	{
		// !KLUDGE! It would be nice if we could just send this in one line instead of making a copy
		CProtoBufMsg< CMsgLookupMultipleAccountNames > msg( k_EMsgGCLookupMultipleAccountNames );
		msg.Body() = m_msgPendingLookupAccountNames;
		GCClientSystem()->BSendMessage( msg );

		m_msgPendingLookupAccountNames.Clear();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInventoryManager::UpdateInventoryEquippedState( CPlayerInventory *pInventory, uint64 ulItemID, equipped_class_t unClass, equipped_slot_t unSlot )
{
	// passing in INVALID_ITEM_ID means "unequip from this slot"
	if ( ulItemID != INVALID_ITEM_ID )
	{
		if ( !pInventory->GetInventoryItemByItemID( ulItemID ) )
		{
			//Warning("Attempt to update equipped state failure: %s.\n", "could not find matching item ID");
			return;
		}
		if ( !pInventory->GetSOCDataForItem( ulItemID ) )
		{
			//Warning("Attempt to update equipped state failure: %s\n", "could not find SOC data for item");
			return;
		}
	}

	CProtoBufMsg<CMsgAdjustItemEquippedState> msg( k_EMsgGCAdjustItemEquippedState );
	msg.Body().set_item_id( ulItemID );
	msg.Body().set_new_class( unClass );
	msg.Body().set_new_slot( unSlot );
	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
// Purpose: GC sent name of account down
//-----------------------------------------------------------------------------
class CGCLookupAccountNameResponse : public GCSDK::CGCClientJob
{
public:
	CGCLookupAccountNameResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCLookupAccountNameResponse_t> msg( pNetPacket );

		CUtlString playerName;
		if ( msg.BReadStr( &playerName ) )
		{
			InventoryManager()->PersonaName_Store( msg.Body().m_unAccountID, playerName.Get() );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCLookupAccountNameResponse, "CGCLookupAccountNameResponse", k_EMsgGCLookupAccountNameResponse, GCSDK::k_EServerTypeGCClient );

class CGCLookupMultipleAccountsNameResponse : public GCSDK::CGCClientJob
{
public:
	CGCLookupMultipleAccountsNameResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}
	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		CProtoBufMsg<CMsgLookupMultipleAccountNamesResponse> msg( pNetPacket );
		for ( int i = 0 ; i < msg.Body().accounts_size() ; ++i )
		{
			const CMsgLookupMultipleAccountNamesResponse_Account &account = msg.Body().accounts( i );
			InventoryManager()->PersonaName_Store( account.accountid(), account.persona().c_str() );
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCLookupMultipleAccountsNameResponse, "CGCLookupMultipleAccountsNameResponse", k_EMsgGCLookupMultipleAccountNamesResponse, GCSDK::k_EServerTypeGCClient );

void CInventoryManager::PersonaName_Precache( uint32 unAccountID )
{
	const char *pszName = PersonaName_Get( unAccountID );
	if ( pszName == NULL )
	{
		// Queue request name from GC
		m_msgPendingLookupAccountNames.add_accountids( unAccountID );

		// insert empty string so we don't ask again
		m_mapPersonaNamesCache.Insert( unAccountID, "" );
	}
}

const char *CInventoryManager::PersonaName_Get( uint32 unAccountID )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// First ask Steam if this is one of friends -- if so we can get an up-to-date persona name.
	{
		const char *pszName = NULL;

		if ( steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamFriends() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			steamID.SetAccountID( unAccountID );
			uint64 u64AccountId = steamID.ConvertToUint64();

			// We're covering three states here:
			// 1. We've never asked before. We need to queue up a RequestUserInformation.
			// 2. We've asked before, and we haven't heard back yet
			// 3. We've asked before, we heard back. Don't re-request user information.
			auto index = m_personaNameRequests.Find( u64AccountId );
			if ( !m_personaNameRequests.IsValidIndex( index ) )
			{
				// This is case 1--we've never asked before.

				// If RequestUserInformation returns false, the information is already available.
				// Otherwise, it will arrive later and we need to rebuild the description at that time.
				if ( !steamapicontext->SteamFriends()->RequestUserInformation( steamID, true ) )
				{
					pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( steamID );
					Assert( pszName ); // Guaranteed by the steam api

					if ( Q_strncmp( pszName, "[unknown]", ARRAYSIZE( "[unknown]" ) ) != 0 )
					{
						m_mapPersonaNamesCache.InsertOrReplace( unAccountID, pszName );
						return pszName;
					}
				}
				else
				{
					// This is case 2, we've asked above. 
					m_personaNameRequests.Insert( u64AccountId, false );
				}
			}
			else
			{
				if ( m_personaNameRequests[ index ] )
				{
					// This is case 3.
					pszName = steamapicontext->SteamFriends()->GetFriendPersonaName( steamID );
					Assert( pszName ); // Guaranteed by the steam api

					if ( Q_strncmp( pszName, "[unknown]", ARRAYSIZE( "[unknown]" ) ) != 0 )
					{
						m_mapPersonaNamesCache.InsertOrReplace( unAccountID, pszName );
						return pszName;
					}
				}
			}
		}
	}

	// If that didn't work, ask the server we're playing on if they know this account ID.
	CBasePlayer *pPlayer = GetPlayerByAccountID( unAccountID );
	if ( pPlayer )
	{
		const char *pszPlayerName = pPlayer->GetPlayerName();
		if ( pszPlayerName )
		{
			m_mapPersonaNamesCache.InsertOrReplace( unAccountID, pszPlayerName );
			return pszPlayerName;
		}
	}

	// If *that* didn't work, look in our cache populated by the GC (or the above paths). This
	// might be out of date but it's better than nothing.
	int idx = m_mapPersonaNamesCache.Find( unAccountID );
	if ( m_mapPersonaNamesCache.IsValidIndex( idx ) )
	{
		return m_mapPersonaNamesCache[idx].Get();
	}

	return "[unknown]";
}

void CInventoryManager::PersonaName_Store( uint32 unAccountID, const char *pPersonaName )
{
	m_mapPersonaNamesCache.InsertOrReplace( unAccountID, pPersonaName );
}

#endif // CLIENT_DLL


//=======================================================================================================================
// PLAYER INVENTORY
//=======================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerInventory::CPlayerInventory( void )
	: m_mapItemDefsToItems( DefLessFunc( item_definition_index_t ) )
	, m_mapPaintkitsToItems( DefLessFunc( uint32 ) )
{
	m_bGotItemsFromSteam = false;
	m_iPendingRequests = 0;
	m_aInventoryItems.Purge();
	m_pSOCache = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerInventory::~CPlayerInventory()
{
	FOR_EACH_VEC( m_vecItemHandles, i )
	{
		m_vecItemHandles[ i ]->InventoryIsBeingDeleted();
	}
	m_vecItemHandles.Purge();

	if ( m_iPendingRequests )
	{
		InventoryManager()->RemovePendingRequest( &m_OwnerID );
		m_iPendingRequests = 0;
	}

	SOClear();

	InventoryManager()->DeregisterInventory( this );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerInventory::SOClear()
{
	if ( m_OwnerID.IsValid() )
	{
		CGCClientSystem *pClientSystem = GCClientSystem();
		Assert ( pClientSystem != NULL );
		if ( pClientSystem != NULL )
		{
			CGCClient *pClient = pClientSystem->GetGCClient();
			Assert ( pClient != NULL );
			pClient->RemoveSOCacheListener( m_OwnerID, this );
		}
	}

	// Somebody registered as a listener through us, but now our Steam ID
	// is changing?  This is bad news.
	Assert( m_vecListeners.Count() == 0 );
	while ( m_vecListeners.Count() > 0 )
	{
		RemoveListener( m_vecListeners[0] );
	}

	// If we were subscribed, we should have gotten our unsubscribe message,
	// and that should have cleared the pointer
	Assert( m_pSOCache == NULL);
	m_pSOCache = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerInventory::AddItemHandle( CEconItemViewHandle* pHandle )
{
	FOR_EACH_VEC( m_vecItemHandles, i )
	{
		if ( m_vecItemHandles[ i ] == pHandle )
		{
			Assert( !"Item handle already in list to track!" );
			return;
		}
	}

	m_vecItemHandles.AddToTail( pHandle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerInventory::RemoveItemHandle( CEconItemViewHandle* pHandle )
{
	FOR_EACH_VEC( m_vecItemHandles, i )
	{
		if ( m_vecItemHandles[ i ] == pHandle )
		{
			m_vecItemHandles.Remove( i );
			return;
		}
	}

	Assert( !"Could not find item handle to remove!" );
}

const CCopyableUtlVector< itemid_t >& CPlayerInventory::GetItemsWithDefindex( item_definition_index_t defindex )
{
	auto idx = m_mapItemDefsToItems.Find( defindex );
	if ( idx == m_mapItemDefsToItems.InvalidIndex() )
	{
		idx = m_mapItemDefsToItems.Insert( defindex );
	}

	return m_mapItemDefsToItems[ idx ];
}

const CCopyableUtlVector< itemid_t >& CPlayerInventory::GetItemsWithPaintkitDefindex( uint32 nDefindex )
{
	auto idx = m_mapPaintkitsToItems.Find( nDefindex );
	if ( idx == m_mapPaintkitsToItems.InvalidIndex() )
	{
		idx = m_mapPaintkitsToItems.Insert( nDefindex );
	}

	return m_mapPaintkitsToItems[ idx ];
}


void	CPlayerInventory::Clear()
{
	SOClear();
	m_OwnerID = CSteamID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerInventory::RequestInventory( CSteamID pSteamID )
{
	// Make sure we don't already have somebody else's stuff
	// on hand
	if ( m_OwnerID != pSteamID )
		SOClear();

	// Remember whose inventory we're looking at
	m_OwnerID = pSteamID; 

	// SteamID must be valid
	if ( !m_OwnerID.IsValid() || !m_OwnerID.BIndividualAccount() )
	{
		Assert( m_OwnerID.IsValid() );
		Assert( m_OwnerID.BIndividualAccount() );
		return;
	}

	// If we don't already have an SO cache, then ask the GC for one,
	// and start listening to it.  We will receive our "subscribed" message
	// when the data is valid
	GCClientSystem()->GetGCClient()->AddSOCacheListener( m_OwnerID, this );
}

void CPlayerInventory::AddListener( GCSDK::ISharedObjectListener *pListener )
{
	Assert( m_OwnerID.IsValid() );
	if ( m_vecListeners.Find( pListener ) < 0 )
	{
		m_vecListeners.AddToTail( pListener );
		GCClientSystem()->GetGCClient()->AddSOCacheListener( m_OwnerID, pListener );
	}
}


void CPlayerInventory::RemoveListener( GCSDK::ISharedObjectListener *pListener )
{
	if ( m_OwnerID.IsValid() )
	{
		m_vecListeners.FindAndFastRemove( pListener );
		GCClientSystem()->GetGCClient()->RemoveSOCacheListener( m_OwnerID, pListener );
	}
	else
	{
		Assert( m_vecListeners.Count() == 0 );
	}
}

template< class MapType, class KeyType >
void AddToMapVec( MapType& mapVec, const CEconItemView* pItem, KeyType key )
{
	auto idx = mapVec.Find( key );
	if ( mapVec.InvalidIndex() == idx )
	{
		idx = mapVec.Insert( key );
	}
	mapVec[ idx ].AddToTail( pItem->GetItemID() );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add a new item for a econ item
//-----------------------------------------------------------------------------
bool CPlayerInventory::AddEconItem( CEconItem * pItem, bool bUpdateAckFile, bool bWriteAckFile, bool bCheckForNewItems )
{
	CEconItemView newItem;
	if( !FilloutItemFromEconItem( &newItem, pItem ) )
	{
		return false;
	}

	int iIdx = m_aInventoryItems.AddToTail( newItem );

	DirtyItemHandles();

	ItemHasBeenUpdated( &m_aInventoryItems[iIdx], bUpdateAckFile, bWriteAckFile );

#ifdef CLIENT_DLL
	// Update map of item defs to items
	AddToMapVec( m_mapItemDefsToItems, &m_aInventoryItems[iIdx], newItem.GetItemDefIndex() );
#endif

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Creates a script item and associates it with this econ item
//-----------------------------------------------------------------------------
void CPlayerInventory::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;
	Assert( steamIDOwner == m_OwnerID );
	if ( steamIDOwner != m_OwnerID )
		return;

	// We shouldn't get these notifications unless we're subscribed, right?
	if ( m_pSOCache == NULL)
	{
		Assert( m_pSOCache );
		return;
	}

	// Don't bother unless it's an incremental notification.
	// For mass updates, we'll do everything more efficiently in one place
	if ( eEvent != GCSDK::eSOCacheEvent_Incremental )
	{
		Assert( eEvent == GCSDK::eSOCacheEvent_Subscribed || eEvent == GCSDK::eSOCacheEvent_Resubscribed || eEvent == GCSDK::eSOCacheEvent_ListenerAdded );
		return;
	}

	CEconItem *pItem = (CEconItem *)pObject;
	AddEconItem( pItem, true, true, true );
	SendInventoryUpdateEvent();
}


//-----------------------------------------------------------------------------
// Purpose: Updates the script item associated with this econ item
//-----------------------------------------------------------------------------
void CPlayerInventory::SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;
	Assert( steamIDOwner == m_OwnerID );
	if ( steamIDOwner != m_OwnerID )
		return;

	// We shouldn't get these notifications unless we're subscribed, right?
	if ( m_pSOCache == NULL)
	{
		Assert( m_pSOCache );
		return;
	}

	// Don't bother unless it's an incremental notification.
	// For mass updates, we'll do everything more efficiently in one place
	if ( eEvent != GCSDK::eSOCacheEvent_Incremental )
	{
		Assert( eEvent == GCSDK::eSOCacheEvent_Subscribed || eEvent == GCSDK::eSOCacheEvent_Resubscribed );
		return;
	}

	CEconItem *pEconItem = (CEconItem *)pObject;

	bool bChanged = false;
	CEconItemView *pScriptItem = GetInventoryItemByItemID( pEconItem->GetItemID() );
	if ( pScriptItem )
	{
		if ( FilloutItemFromEconItem( pScriptItem, pEconItem ) )
		{
			ItemHasBeenUpdated( pScriptItem, false, false );
		}

		bChanged = true;
	}
	else
	{
		// The item isn't in this inventory right now. But it may need to be 
		// after the update, so try adding it and see if the inventory wants it.
		bChanged = AddEconItem( pEconItem, false, false, false );
	}

	if ( bChanged )
	{
		DirtyItemHandles();

		SendInventoryUpdateEvent();
#ifdef _DEBUG
		if ( item_inventory_debug.GetBool() )
		{
			DumpInventoryToConsole( true );
		}
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes the script item associated with this econ item
//-----------------------------------------------------------------------------
void CPlayerInventory::SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;
	Assert( steamIDOwner == m_OwnerID );
	if ( steamIDOwner != m_OwnerID )
		return;

	// We shouldn't get these notifications unless we're subscribed, right?
	if ( m_pSOCache == NULL)
	{
		Assert( m_pSOCache );
		return;
	}

	// Don't bother unless it's an incremental notification.
	// For mass updates, we'll do everything more efficiently in one place
	if ( eEvent != GCSDK::eSOCacheEvent_Incremental )
	{
		Assert( eEvent == GCSDK::eSOCacheEvent_Subscribed || eEvent == GCSDK::eSOCacheEvent_Resubscribed );
		return;
	}

	CEconItem *pEconItem = (CEconItem *)pObject;
	RemoveItem( pEconItem->GetItemID() );

	SendInventoryUpdateEvent();
}


//-----------------------------------------------------------------------------
// Purpose: This is our initial notification that this cache has been received
//			from the server.
//-----------------------------------------------------------------------------
void CPlayerInventory::SOCacheSubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	// Make sure we expect notifications about this guy
	Assert( steamIDOwner == m_OwnerID );
	if ( steamIDOwner != m_OwnerID )
		return;

	#ifdef _DEBUG
		Msg("CPlayerInventory::SOCacheSubscribed\n");
	#endif

	// Clear our old inventory
	m_aInventoryItems.Purge();

	DirtyItemHandles();

	// Locate the cache that was just subscribed to
	m_pSOCache = GCClientSystem()->GetSOCache( m_OwnerID );
	if ( m_pSOCache == NULL )
	{
		Assert( m_pSOCache != NULL );
		return;
	}

	// add all the items already in the inventory
	CSharedObjectTypeCache *pTypeCache = m_pSOCache->FindTypeCache( CEconItem::k_nTypeID );
	if( pTypeCache )
	{
		for( uint32 unItem = 0; unItem < pTypeCache->GetCount(); unItem++ )
		{
			CEconItem *pItem = (CEconItem *)pTypeCache->GetObject( unItem );
			AddEconItem(pItem, true, false, true );
		}
	}

	m_bGotItemsFromSteam = true;

#ifdef CLIENT_DLL
	if ( InventoryManager()->GetLocalInventory() == this )
	{
		// tell the entire client that we're ready to look at items
		CInventoryManager::SendItemSystemConnectedEvent();
	}
#endif
}

bool CInventoryManager::IsValidPlayerClass( equipped_class_t unClass )
{
	const bool bResult = ItemSystem()->GetItemSchema()->IsValidClass( unClass );
	AssertMsg( bResult, "Invalid player class!" );
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerInventory::ItemHasBeenUpdated( CEconItemView *pItem, bool bUpdateAckFile, bool bWriteAckFile )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPlayerInventory::SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	m_pSOCache = NULL;
	m_bGotItemsFromSteam = false;
	m_aInventoryItems.Purge();

	DirtyItemHandles();
}


//-----------------------------------------------------------------------------
// Purpose: On the client this sends the "inventory_updated" event. On the server
//			it does nothing.
//-----------------------------------------------------------------------------
void CPlayerInventory::SendInventoryUpdateEvent()
{
#ifdef CLIENT_DLL
	if( InventoryManager()->GetLocalInventory() == this )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "inventory_updated" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Fills out all the fields in the script item based on what's in the
//			econ item
//-----------------------------------------------------------------------------
bool CPlayerInventory::FilloutItemFromEconItem( CEconItemView *pScriptItem, CEconItem *pEconItem )
{
	pScriptItem->Init( pEconItem->GetDefinitionIndex(), pEconItem->GetAccountID() );
	if ( !pScriptItem->IsValid() )
		return false;

	pScriptItem->SetItemID( pEconItem->GetItemID() );

#if BUILD_ITEM_NAME_AND_DESC
	// Precache account names if we have any. We do this way in advance of any code that might
	// use it (ie., description text building) so that by the time we try that we already have
	// the data setup.
	//
	// We don't worry about yielding here because this inventory code only runs on game
	// clients/servers, not the GC.
	CSteamAccountIDAttributeCollector AccountIDCollector;
	pEconItem->IterateAttributes( &AccountIDCollector );

	FOR_EACH_VEC( AccountIDCollector.GetAccountIDs(), i )
	{
		InventoryManager()->PersonaName_Precache( (AccountIDCollector.GetAccountIDs())[i] );
	}
#endif

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerInventory::DumpInventoryToConsole( bool bRoot )
{
	if ( bRoot )
	{
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
		Msg("      %s (ID %llu)\n", m_aInventoryItems[i].GetStaticData()->GetDefinitionName(), m_aInventoryItems[i].GetItemID() );
	}
}

template< class MapType, class KeyType >
void RemoveItemFromVecMap( MapType& mapVec, const CEconItemView* pItem, KeyType key )
{
	auto idx = mapVec.Find( key );
	if ( mapVec.InvalidIndex() != idx )
	{
		auto idxVec = mapVec[ idx ].Find( pItem->GetItemID() );
		Assert( idxVec != mapVec[ idx ].InvalidIndex() );
		if ( idxVec != mapVec[ idx ].InvalidIndex() )
		{
			mapVec[ idx ].Remove( idxVec );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerInventory::RemoveItem( itemid_t iItemID )
{
	int iIndex;
	CEconItemView *pItem = GetInventoryItemByItemID( iItemID, &iIndex );
	if ( pItem )
	{
		ItemIsBeingRemoved( pItem );

		FOR_EACH_VEC( m_vecItemHandles, i )
		{
			m_vecItemHandles[ i ]->MarkDirty();
			m_vecItemHandles[ i ]->ItemIsBeingDeleted( pItem );
		}

		// Update map of item defs to items
		RemoveItemFromVecMap( m_mapItemDefsToItems, pItem, pItem->GetItemDefIndex() );
		
		m_aInventoryItems.Remove(iIndex);

#ifdef _DEBUG
		if ( item_inventory_debug.GetBool() )
		{
			DumpInventoryToConsole( true );
		}
#endif
	}

	// Don't need to resort because items will still be in order
}

//-----------------------------------------------------------------------------
// Purpose: Finds the item in our inventory that matches the specified global index
//-----------------------------------------------------------------------------
CEconItemView *CPlayerInventory::GetInventoryItemByItemID( itemid_t iIndex, int *pIndex )
{
	int iCount = m_aInventoryItems.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( m_aInventoryItems[i].GetItemID() == iIndex )
		{
			if ( pIndex )
			{
				*pIndex = i;
			}

			return &m_aInventoryItems[i];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Finds the item in our inventory that matches the specified global original id
//-----------------------------------------------------------------------------
CEconItemView *CPlayerInventory::GetInventoryItemByOriginalID( itemid_t iOriginalID, int *pIndex /*= NULL*/ )
{
	int iCount = m_aInventoryItems.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CEconItem *pItem = m_aInventoryItems[i].GetSOCData();
		if ( pItem && pItem->GetOriginalID() == iOriginalID )
		{
			if ( pIndex )
			{
				*pIndex = i;
			}

			return &m_aInventoryItems[i];
		}
	}

	return NULL;
}

// Finds the first item in our backpack with match itemdef
//-----------------------------------------------------------------------------
CEconItemView *CPlayerInventory::FindFirstItembyItemDef( item_definition_index_t iItemDef )
{
	int iCount = m_aInventoryItems.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		//GetItemDefIndex()
		if ( m_aInventoryItems[i].GetItemDefIndex() == iItemDef )
		{
			return &m_aInventoryItems[i];
		}
	}

	return NULL;

}

//-----------------------------------------------------------------------------
// Purpose: Get the index for the item in our inventory utlvector
//-----------------------------------------------------------------------------
int	CPlayerInventory::GetIndexForItem( CEconItemView *pItem )
{
	int iCount = m_aInventoryItems.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( m_aInventoryItems[i].GetItemID() == pItem->GetItemID() )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Dirty all the item handles that are registered with us
//-----------------------------------------------------------------------------
void CPlayerInventory::DirtyItemHandles()
{
	FOR_EACH_VEC( m_vecItemHandles, i )
	{
		m_vecItemHandles[ i ]->MarkDirty();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the item object cache data for the specified item
//-----------------------------------------------------------------------------
CEconItem	*CPlayerInventory::GetSOCDataForItem( itemid_t iItemID ) 
{ 
	if ( !m_pSOCache )
		return NULL;

	CEconItem soIndex;
	soIndex.SetItemID( iItemID );
	return (CEconItem *)m_pSOCache->FindSharedObject( soIndex );
}

#if defined (_DEBUG) && defined(CLIENT_DLL)
CON_COMMAND_F( item_deleteall, "WARNING: Removes all of the items in your inventory.", FCVAR_CHEAT )
{
	CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
	if ( !pInventory )
		return;

	int iCount = pInventory->GetItemCount();
	for ( int i = 0; i < iCount; i++ )
	{
		CEconItemView *pItem = pInventory->GetItem(i);
		if ( pItem )
		{
			InventoryManager()->DropItem( pItem->GetItemID() );
		}
	}

	InventoryManager()->UpdateLocalInventory();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemViewHandle::SetItem( CEconItemView* pItem )
{
	m_pItem = pItem;

	if ( pItem )
	{
		// Cache the item_id for lookup when our pointer gets dirtied
		m_nItemID = pItem->GetItemID();
		auto* pInv = InventoryManager()->GetInventoryForAccount( pItem->GetAccountID() );
		Assert( pInv );
		if ( m_pInv != pInv )
		{
			// If this is a different inventory, unsubscribe.  This can happen if the
			// handle gets reused
			if ( m_pInv )
			{
				m_pInv->RemoveItemHandle( this );
			}

			m_pInv = pInv;

			// Subscribe to the new inventory
			m_pInv->AddItemHandle( this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to a CEconItemView
//-----------------------------------------------------------------------------
CEconItemView* CEconItemViewHandle::Get() const
{
	// If our pointer is dirty, we need to go get a new pointer
	if ( m_bPointerDirty )
	{
		if ( m_pInv )
		{
			m_pItem = m_pInv->GetInventoryItemByItemID( m_nItemID );
			m_bPointerDirty = false;
		}
	}

	return m_pItem;
}


//-----------------------------------------------------------------------------
// Purpose: Unsubscribe us from future updates
//-----------------------------------------------------------------------------
CEconItemHandle::~CEconItemHandle()
{
	UnsubscribeFromSOEvents();
}


//-----------------------------------------------------------------------------
// Purpose: Save a pointer to the item and register us for SOCache events
//-----------------------------------------------------------------------------
void CEconItemHandle::SetItem( CEconItem* pItem )
{
	UnsubscribeFromSOEvents();

	m_pItem = NULL;
	m_iItemID = INVALID_ITEM_ID;

	if ( pItem )
	{
		auto* pInv = InventoryManager()->GetInventoryForAccount( pItem->GetAccountID() );
		if ( pInv )
		{
			m_OwnerSteamID.SetFromUint64( pInv->GetOwner().ConvertToUint64() );
			GCClientSystem()->GetGCClient()->AddSOCacheListener( m_OwnerSteamID, this );
		}

		m_pItem = pItem;
		m_iItemID = pItem->GetID();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Check if out item got deleted.  If it did, mark our pointer as NULL
//			so future dereferences will get NULL instead of a stale pointer.
//-----------------------------------------------------------------------------
void CEconItemHandle::SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{ 
	if( pObject->GetTypeID() != CEconItem::k_nTypeID || m_pItem == NULL )
		return;

	const CEconItem *pItem = (CEconItem *)pObject;

	if ( m_iItemID == pItem->GetID() )
	{
		UnsubscribeFromSOEvents();
		m_pItem = NULL;
		m_iItemID = INVALID_ITEM_ID;
	}
}

void CEconItemHandle::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;
	
	CEconItem *pItem = (CEconItem *)pObject;

	if ( m_iItemID == pItem->GetID() )
	{
		SetItem( pItem );
	}
}

void CEconItemHandle::SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() != CEconItem::k_nTypeID )
		return;

	CEconItem *pItem = (CEconItem *)pObject;

	if ( m_iItemID == pItem->GetID() )
	{
		SetItem( pItem );
	}
}

void CEconItemHandle::SOCacheUnsubscribed( const CSteamID & steamIDOwner, GCSDK::ESOCacheEvent eEvent )
{
	UnsubscribeFromSOEvents();
}

void CEconItemHandle::UnsubscribeFromSOEvents()
{
	if ( m_OwnerSteamID.GetAccountID() != 0 )
	{
		GCClientSystem()->GetGCClient()->RemoveSOCacheListener( m_OwnerSteamID, this );
	}
}


#if defined( STAGING_ONLY ) || defined( _DEBUG )
#if defined(CLIENT_DLL)
CON_COMMAND_F( item_dumpinv, "Dumps the contents of a specified client inventory.", FCVAR_CHEAT )
#else
CON_COMMAND_F( item_dumpinv_sv, "Dumps the contents of a specified server inventory.", FCVAR_CHEAT )
#endif
{
#if defined(CLIENT_DLL)
	CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
#else
	CSteamID steamID;
	CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_GetCommandClient() ); 
	pPlayer->GetSteamID( &steamID );
	CPlayerInventory *pInventory = InventoryManager()->GetInventoryForAccount( steamID.GetAccountID() );
#endif
	if ( !pInventory )
	{
		Msg("No inventory found.\n");
		return;
	}

	pInventory->DumpInventoryToConsole( true );
}

#if defined (CLIENT_DLL)

CON_COMMAND_F( item_dumpschema, "Dump the expanded schema for items to a file in sorted order suitable for diffs. Format: item_dumpschema <filename>", FCVAR_CHEAT )
{
	if ( args.ArgC() != 2 )
	{
		Msg("Usage: item_dumpschema <filename>\n");
		return;
	}

	if ( GetItemSchema()->DumpItems(args[1]) )
		Msg("Dump complete, saved in game/tf/%s\n", args[1]);
	else
		Msg("Dump failed (?)\n");
}

CON_COMMAND_F( item_giveitem, "Give an item to the local player. Format: item_giveitem <item definition name> or <item def index>", FCVAR_NONE )
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
	{
		Msg("Not connected to Steam.\n");
		return;
	}
	CSteamID steamIDForPlayer = steamapicontext->SteamUser()->GetSteamID();
	if ( !steamIDForPlayer.IsValid() )
	{
		Msg("Failed to find a valid steamID for the local player.\n");
		return;
	}

	int iItemCount = args.ArgC();
	for ( int i = 1; i < iItemCount; ++i )
	{
		// Check to see if args[1] is a number (itemdefid) and if so, translate it to actual itemname
		const char *pszItemname = NULL;
		if ( V_isdigit( args[i][0] ) )
		{
			int iDef = V_atoi( args[i] );
			CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iDef );
			if ( pItemDef )
			{
				pszItemname = pItemDef->GetItemDefinitionName();
			}
		}
		else
		{
			pszItemname = args[i];
		}

		Msg("Sending request to generate '%s' for Local Player (%llu)\n", pszItemname, steamIDForPlayer.ConvertToUint64() );

		CItemSelectionCriteria criteria;

		GCSDK::CProtoBufMsg<CMsgDevNewItemRequest> msg( k_EMsgGCDev_NewItemRequest );
		msg.Body().set_receiver( steamIDForPlayer.ConvertToUint64() );

		criteria.SetIgnoreEnabledFlag( true );
		if ( !criteria.BAddCondition( "name", k_EOperator_String_EQ, pszItemname, true ) ||
			!criteria.BSerializeToMsg( *msg.Body().mutable_criteria() ) )
		{
			Msg("Failed to add condition and/or serialize item grant request. This is probably caused by having a string that's too long.\n" );
			return;
		}
		GCClientSystem()->BSendMessage( msg );
	}
}

CON_COMMAND_F( item_rolllootlist, "Force a loot list rool for the local player. Format: item_rolllootlist <loot list definition name>", FCVAR_NONE )
{
	if ( !steamapicontext || !steamapicontext->SteamUser() )
	{
		Msg("Not connected to Steam.\n");
		return;
	}
	CSteamID steamIDForPlayer = steamapicontext->SteamUser()->GetSteamID();
	if ( !steamIDForPlayer.IsValid() )
	{
		Msg("Failed to find a valid steamID for the local player.\n");
		return;
	}

	Msg("Sending request to roll '%s' for Local Player (%llu)\n", args[1], steamIDForPlayer.ConvertToUint64() );

	GCSDK::CProtoBufMsg<CMsgDevDebugRollLootRequest> msg( k_EMsgGCDev_DebugRollLootRequest );
	msg.Body().set_receiver( steamIDForPlayer.ConvertToUint64() );
	msg.Body().set_loot_list_name( args[1] );
	GCClientSystem()->BSendMessage( msg );
}

#include "econ_item_description.h"
#include "localization_provider.h"

CON_COMMAND_F( item_generate_all_descriptions, "Generate full item descriptions for every item in your backpack. Meant as a code test.", FCVAR_CHEAT )
{
	CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();

	for ( int i = 0; i < pInventory->GetItemCount(); i++ )
	{
		CEconItemDescription desc;
		IEconItemDescription::YieldingFillOutEconItemDescription( &desc, GLocalizationProvider(), pInventory->GetItem( i ) );
	}

	Msg("Done.\n");
}
#endif // CLIENT_DLL

#endif // STAGING_ONLY || _DEBUG


