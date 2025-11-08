//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tier1/KeyValues.h"
#include "econ_gcmessages.h"
#include "econ_item_system.h"
#include "econ_item_inventory.h"
#include "game_item_schema.h"
#include "gc_clientsystem.h"

#include "utldict.h"
#include "filesystem.h"
#include "steam/isteamhttp.h"


#if defined(CLIENT_DLL) || defined(GAME_DLL)
#include "gamestringpool.h"
#include "ihasattributes.h"
#include "tier0/icommandline.h"
#endif

#if defined(CLIENT_DLL)
#include "igameevents.h"
#endif

// FIXME FIXME FIXME
#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	#include "tf_item_system.h"
#endif // defined(TF_DLL) || defined(TF_CLIENT_DLL)

#if defined (DOTA_CLIENT_DLL) || defined (DOTA_DLL)
	#include "econ/dota_item_system.h"
#endif // defined (DOTA_CLIENT_DLL) || defined (DOTA_DLL)

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if ( defined( GAME_DLL ) || defined( CLIENT_DLL ) ) && ( defined( _DEBUG ) || defined( STAGING_ONLY ) )
ConVar item_debug( "item_debug", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar items_game_use_gc_copy( "items_game_use_gc_copy", "0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_ARCHIVE, "If set, items_game.txt will be stomped by the GC." );
ConVar item_debug_validation( "item_debug_validation", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE, "If set, CEconEntity::ValidateEntityAttachedToPlayer behaves as it would in release builds and also allows bot players to take the same code path as real players." );
#endif

static ConVar item_quality_chance_unique( "item_quality_chance_unique", "0.1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Percentage chance that a random item is unique." );
static ConVar item_quality_chance_rare( "item_quality_chance_rare", "0.5", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Percentage chance that a random item is a rare." );
static ConVar item_quality_chance_common( "item_quality_chance_common", "1.0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Percentage chance that a random item is common." );


//-----------------------------------------------------------------------------
// Purpose: Get at the global item system
//-----------------------------------------------------------------------------
CEconItemSystem *ItemSystem( void )
{
	static GameItemSystem_t *pSystem = NULL;
	if ( !pSystem )
	{
		pSystem = new GameItemSystem_t();
	}

	return pSystem;
}


//-----------------------------------------------------------------------------
// Purpose: Global schema access, declared in game_item_schema.h
//-----------------------------------------------------------------------------
GameItemSchema_t *GetItemSchema()
{
	return ItemSystem()->GetItemSchema();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemSystem::CEconItemSystem( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemSystem::~CEconItemSystem( void )
{
	Shutdown();
}


//-----------------------------------------------------------------------------
// Purpose: Parse in our data files.
//-----------------------------------------------------------------------------
void CEconItemSystem::Init( void )
{
#ifdef USES_ECON_ITEMS
	ParseItemSchemaFile( "scripts/items/items_game.txt" );
#endif // USES_ECON_ITEMS
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemSystem::PostInit( void )
{
#ifdef USES_ECON_ITEMS
	CUtlVector< CUtlString > vecErrors;
	bool bSuccess = m_itemSchema.BPostSchemaInit( &vecErrors );

	if( !bSuccess )
	{
		FOR_EACH_VEC( vecErrors, nError )
		{
			Warning( "%s\n", vecErrors[nError].String() );
		}
	}
#endif // USES_ECON_ITEMS

#ifdef CLIENT_DLL
	IGameEvent *event = gameeventmanager->CreateEvent( "item_schema_initialized" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemSystem::Shutdown( void )
{
}

extern ConVar mp_tournament;

#ifdef GAME_DLL
ConVar mp_tournament_whitelist( "mp_tournament_whitelist", "item_whitelist.txt", FCVAR_NONE, "Specifies the item whitelist file to use." );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemSystem::ReloadWhitelist( void )
{
	// Default state of items depends on whether we're in tourney mode, and whether there's a whitelist
	bool bDefault = true;
	bool bFoundWhitelist = false;

	KeyValues *pWhitelistKV = new KeyValues( "item_whitelist" );

#ifdef GAME_DLL
	if ( mp_tournament.GetBool() && mp_tournament_whitelist.GetString() )
	{
		const char *pszWhitelistFile = mp_tournament_whitelist.GetString();
		if ( pWhitelistKV->LoadFromFile( filesystem, pszWhitelistFile ) )
		{
			// Allow the whitelist to override the default, so they can turn it into a blacklist if they want to
			bDefault = pWhitelistKV->GetBool( "unlisted_items_default_to" );
			bFoundWhitelist = true;
		}
		else if ( pszWhitelistFile && pszWhitelistFile[0] )
		{
			Msg("Item Whitelist file '%s' could not be found. All items will be allowed.\n", pszWhitelistFile );
		}
	}
#endif

	const CEconItemSchema::ItemDefinitionMap_t& mapItemDefs = m_itemSchema.GetItemDefinitionMap();
	FOR_EACH_MAP_FAST( mapItemDefs, i )
	{
		mapItemDefs[i]->SetAllowedInMatch( bDefault );
	}

	// If we didn't find a file, we're done.
	if ( !bFoundWhitelist )
	{
		pWhitelistKV->deleteThis();
		return;
	}

	// Otherwise, go through the KVs and turn on the matching items.
	Msg("Parsing item whitelist (default: %s)\n", bDefault ? "allowed" : "disallowed" );
	KeyValues* ownerWhitelistKV = pWhitelistKV;
	pWhitelistKV = pWhitelistKV->GetFirstSubKey();
	while ( pWhitelistKV )
	{
		bool bAllow = pWhitelistKV->GetBool();

		const char *pszItemName = pWhitelistKV->GetName();
		if ( pszItemName && pszItemName[0] && !FStrEq("unlisted_items_default_to", pszItemName) )
		{
			CEconItemDefinition *pItemDef = m_itemSchema.GetItemDefinitionByName( pszItemName );
			if ( pItemDef )
			{
				pItemDef->SetAllowedInMatch( bAllow );
				Msg(" -> %s '%s'\n", bAllow ? "Allowing" : "Removing", pszItemName );
			}
			else
			{
				Warning(" -> Could not find an item definition named '%s'\n", pszItemName );
			}
		}

		pWhitelistKV = pWhitelistKV->GetNextKey();
	}
	Msg("Finished.\n");

	ownerWhitelistKV->deleteThis();
}

#ifdef GAME_DLL
CON_COMMAND_F( item_show_whitelistable_definitions, "Lists the item definitions that can be whitelisted in the item_whitelist.txt file in tournament mode.", FCVAR_CHEAT )
{
	Msg("Available item definitions for whitelisting:\n");
	const CEconItemSchema::SortedItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetSortedItemDefinitionMap();
	FOR_EACH_MAP( mapItemDefs, i )
	{
		const CEconItemDefinition *pItemDef = mapItemDefs[i];
		if ( pItemDef && pItemDef->GetQuality() != AE_NORMAL && !pItemDef->IsHidden() )
		{
			Msg("   '%s'\n", pItemDef->GetDefinitionName() );
		}
	}
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconItemSystem::ResetAttribStringCache( void )
{
	const CUtlMap<int, CEconItemAttributeDefinition, int> &mapDefs = m_itemSchema.GetAttributeDefinitionMap();
	FOR_EACH_MAP_FAST( mapDefs, i )
	{
		mapDefs[i].ClearStringCache();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconItemSystem::DecryptItemFiles( KeyValues *pKV, const char *pName )
{
	char szFullName[512];
	Q_snprintf(szFullName,sizeof(szFullName), "%s.ctx", pName );

	FileHandle_t f = filesystem->Open( szFullName, "rb", "MOD" );

	if (!f)
	{
#if !defined(CSTRIKE_DLL)
		Warning("No %s file found. May be unable to create items.\n", pName );
#endif // CSTRIKE_DLL
		return false;
	}

	int fileSize = filesystem->Size(f);
	char *buffer = (char*)MemAllocScratch(fileSize + 1);

	Assert(buffer);

	filesystem->Read(buffer, fileSize, f); // read into local buffer
	buffer[fileSize] = 0; // null terminate file as EOF
	filesystem->Close( f );	// close file after reading

	UTIL_DecodeICE( (unsigned char*)buffer, fileSize, GetEncryptionKey() );

	bool retOK = pKV->LoadFromBuffer( szFullName, buffer, filesystem );

	MemFreeScratch();

	if ( !retOK )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Read the specified item schema file. Init the item schema with the contents
//-----------------------------------------------------------------------------
void CEconItemSystem::ParseItemSchemaFile( const char *pFilename )
{
	CUtlVector< CUtlString > vecErrors;

	bool bSuccess = m_itemSchema.BInit(pFilename, "GAME", &vecErrors);

	if( !bSuccess )
	{
		FOR_EACH_VEC( vecErrors, nError )
		{
			Warning( "%s\n", vecErrors[nError].String() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Generate a random item matching the specified criteria
//-----------------------------------------------------------------------------
item_definition_index_t CEconItemSystem::GenerateRandomItem( CItemSelectionCriteria *pCriteria, entityquality_t *outEntityQuality )
{
	// First, pick a random item quality (use the one passed in first)
	if ( !pCriteria->BQualitySet() )
	{
		pCriteria->SetQuality( GetRandomQualityForItem() );
	}

	pCriteria->SetIgnoreEnabledFlag( true );

	// Determine which item templates match the criteria
	CUtlVector<item_definition_index_t> vecMatches;
	const CEconItemSchema::ItemDefinitionMap_t &mapDefs = m_itemSchema.GetItemDefinitionMap();

HackMakeValidList:
	FOR_EACH_MAP_FAST( mapDefs, i )
	{
		if ( pCriteria->BEvaluate( mapDefs[i] ) )
		{
			vecMatches.AddToTail( mapDefs.Key( i ) );
		}
	}

	// No valid items?
	int iValidItems = vecMatches.Count();
	if ( !iValidItems )
	{
		// If we were searching for a unique item, drop back to a non-unique
		if ( pCriteria->GetQuality() == AE_UNIQUE )
		{
			pCriteria->SetQuality( GetRandomQualityForItem( true ) );
			goto HackMakeValidList;
		}
		return INVALID_ITEM_DEF_INDEX;
	}

	// Choose a random match
	int iChosenIdx = RandomInt( 0, (iValidItems-1) );
	item_definition_index_t iChosenItem = vecMatches[iChosenIdx];

	const CEconItemDefinition *pItemDef = m_itemSchema.GetItemDefinition( iChosenItem );
	if ( !pItemDef )
		return INVALID_ITEM_DEF_INDEX;

	// If we haven't specified an entity quality, we want to use the item's specified one
	if ( pCriteria->GetQuality() == AE_USE_SCRIPT_VALUE )
	{
		int32 iScriptQuality = pItemDef->GetQuality();
		pCriteria->SetQuality( iScriptQuality == AE_UNDEFINED ? GetRandomQualityForItem( true ) : iScriptQuality );
	}

	// If we haven't specified an item level, we want to use the item's specified one.
	if ( !pCriteria->BItemLevelSet() )
	{
		pCriteria->SetItemLevel( RandomInt( pItemDef->GetMinLevel(), pItemDef->GetMaxLevel() ) );
	}

	if ( outEntityQuality )
	{
		*outEntityQuality = pCriteria->GetQuality();
	}
	return iChosenItem;
}

//-----------------------------------------------------------------------------
// Purpose: Return a random quality for the item specified
//-----------------------------------------------------------------------------
entityquality_t CEconItemSystem::GetRandomQualityForItem( bool bPreventUnique )
{
	// Start on the rarest, and work backwards
	if ( !bPreventUnique )
	{
		if ( RandomFloat(0,1) < item_quality_chance_unique.GetFloat() )
			return AE_UNIQUE;
	}

	if ( RandomFloat(0,1) < item_quality_chance_rare.GetFloat() )
		return AE_RARITY2;

	if ( RandomFloat(0,1) < item_quality_chance_common.GetFloat() )
		return AE_RARITY1;

	return AE_NORMAL;
}

static ISteamHTTP *GetISteamHTTP()
{
	if ( steamapicontext != NULL && steamapicontext->SteamHTTP() )
	{
		return steamapicontext->SteamHTTP();
	}
	#ifndef CLIENT_DLL
		if ( steamgameserverapicontext != NULL )
		{
			return steamgameserverapicontext->SteamHTTP();
		}
	#endif
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Common functionality for using our raw buffer data to initialize
// the schema when safe.
//-----------------------------------------------------------------------------
bool IDelayedSchemaData::InitializeSchemaInternal( CEconItemSchema *pItemSchema, CUtlBuffer& bufRawData, bool bInitAsBinary, uint32 nExpectedVersion )
{
	Msg( "Applying new item schema, version %08X\n", nExpectedVersion );

	CUtlVector<CUtlString> vecErrors;
	bool bSuccess = bInitAsBinary
				  ? pItemSchema->BInitBinaryBuffer( bufRawData, &vecErrors )
				  : pItemSchema->BInitTextBuffer( bufRawData, &vecErrors );
	if( bSuccess )
	{
		// Sanity-check that we received the version that they sent us
		uint32 nOurVersion = pItemSchema->GetVersion();
		if ( nExpectedVersion != 0 && nOurVersion != nExpectedVersion )
		{
			Warning( "**WARNING** Item schema mismatch after update!\n" );
			Warning( "GC told us to expect %08X, we got %08X\n", nExpectedVersion, nOurVersion );
		}
	}
	else
	{
		Warning( "**WARNING** Failed to apply item schema!\n" );
		FOR_EACH_VEC( vecErrors, nError )
		{
			Warning( "%s\n", vecErrors[nError].Get() );
		}

		// Try to fall-back to the local copy.
		Msg( "Falling back to item schema from local file.\n" );
		KeyValuesAD pItemsGameKV( "ItemsGameFile" );
		if ( pItemsGameKV->LoadFromFile( g_pFullFileSystem, "scripts/items/items_game.txt", "GAME" ) )
		{
			CUtlBuffer buffer;
			pItemsGameKV->WriteAsBinary( buffer );

			vecErrors.PurgeAndDeleteElements();
			bSuccess = pItemSchema->BInitBinaryBuffer( buffer, &vecErrors );
			if ( !bSuccess )
			{
				FOR_EACH_VEC( vecErrors, nError )
				{
					Warning( "%s\n", vecErrors[ nError ].Get() );
				}
			}
		}
	}
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: The GC sent us a single block of binary data.
//-----------------------------------------------------------------------------
class DelayedSchemaData_GCDirectData : public IDelayedSchemaData
{
public:
	DelayedSchemaData_GCDirectData( const std::string& strBuffer )
		: m_bufRawData( strBuffer.data(), strBuffer.size(), CUtlBuffer::READ_ONLY )
	{
		//
	}

	virtual bool InitializeSchema( CEconItemSchema *pItemSchema )
	{
		return InitializeSchemaInternal( pItemSchema, m_bufRawData, true, 0 );
	}

private:
	CUtlBuffer m_bufRawData;
};

extern bool CheckValveSignature( const void *data, uint32 nDataSize, const void *signature, uint32 nSignatureSize );

//-----------------------------------------------------------------------------
// Purpose: We received a text file from an HTML request.
//-----------------------------------------------------------------------------
class DelayedSchemaData_HTTPResponseData : public IDelayedSchemaData
{
public:
	DelayedSchemaData_HTTPResponseData( ISteamHTTP *pHTTP, HTTPRequestHandle handleHTTPRequest, uint32 unBodySize, uint32 nExpectedVersion, const std::string &sSignature )
		: m_nExpectedVersion( nExpectedVersion )
	{
		Assert( pHTTP );

		m_bufRawData.SetBufferType( true, true );
		m_bufRawData.SeekPut( CUtlBuffer::SEEK_HEAD, unBodySize );

		m_bValid = pHTTP->GetHTTPResponseBodyData( handleHTTPRequest, (uint8*)m_bufRawData.Base(), m_bufRawData.TellPut() );
		if ( m_bValid )
			m_bValid = CheckValveSignature( m_bufRawData.Base(), m_bufRawData.TellPut(), sSignature.c_str(), sSignature.length() );
	}

	virtual bool InitializeSchema( CEconItemSchema *pItemSchema )
	{
		if ( !m_bValid )
			return false;

		return InitializeSchemaInternal( pItemSchema, m_bufRawData, false, m_nExpectedVersion );
	}

private:
	bool m_bValid;
	CUtlBuffer m_bufRawData;
	uint32 m_nExpectedVersion;
};

#define GC_ITEM_SCHEMA_UPDATE_APPLIED "Applied updated item schema from GC. %d bytes, version %08X.\n"
#define GC_ITEM_SCHEMA_UPDATE_QUEUED "Received %d bytes item schema version %08X direct data; update is queued.\n"

//-----------------------------------------------------------------------------
// Purpose: Update the item schema from the GC
//-----------------------------------------------------------------------------
class CGCUpdateItemSchema : public GCSDK::CGCClientJob
{
public:
	CGCUpdateItemSchema( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {
		m_szUrl[0] = '\0';
		m_nExpectedVersion = 0;
		bHTTPCompleted = false;
	}

	char m_szUrl[512];
	uint32 m_nExpectedVersion;
	bool bHTTPCompleted;
	CCallResult< CGCUpdateItemSchema, HTTPRequestCompleted_t > callback;
	std::string m_sSignature;

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgUpdateItemSchema > msg( pNetPacket );

#if ( defined( GAME_DLL ) || defined( CLIENT_DLL ) ) && ( defined( _DEBUG ) || defined( STAGING_ONLY ) )
		const bool bUseGCCopy = items_game_use_gc_copy.GetBool();
#else
		const bool bUseGCCopy = true;
#endif

		if ( bUseGCCopy == false && k_EUniversePublic != GetUniverse() )
		{
			Msg( "Loading item schema from local file.\n" );
			KeyValuesAD pItemsGameKV( "ItemsGameFile" );
			if ( pItemsGameKV->LoadFromFile( g_pFullFileSystem, "scripts/items/items_game.txt", "GAME" ) )
			{
				CUtlBuffer buffer;
				pItemsGameKV->WriteAsBinary( buffer );

				CUtlVector< CUtlString > vecErrors;
				bool bSuccess = ItemSystem()->GetItemSchema()->BInitBinaryBuffer( buffer, &vecErrors );
				if( !bSuccess )
				{
					FOR_EACH_VEC( vecErrors, nError )
					{
						Warning( "%s\n", vecErrors[nError].Get() );
					}
				}
			}

			return true;
		}

		// Check if we're already up-to-date
		m_nExpectedVersion = msg.Body().item_schema_version();
		uint32 nCurrentSchemaVersion = ItemSystem()->GetItemSchema()->GetVersion();
		if ( m_nExpectedVersion != 0 && m_nExpectedVersion == nCurrentSchemaVersion )
		{
			Msg( "Current item schema is up-to-date with version %08X.\n", nCurrentSchemaVersion );
			return true;
		}

		m_sSignature = msg.Body().signature();

		// !TEST!
		//const char *szURL = "http://cdn.beta.steampowered.com/apps/440/scripts/items/items_game.b8b7a85b4dd98b139957004b86ec0bc070a59d18.txt";
		if ( msg.Body().has_items_game() )
		{
			bool bDidInit = ItemSystem()->GetItemSchema()->MaybeInitFromBuffer( new DelayedSchemaData_GCDirectData( msg.Body().items_game() ) );
			Msg( bDidInit ? GC_ITEM_SCHEMA_UPDATE_APPLIED : GC_ITEM_SCHEMA_UPDATE_QUEUED, (int)msg.Body().items_game().size(), m_nExpectedVersion );
		}
		else
		{
			// Remember URL
			const char *szURL = msg.Body().items_game_url().c_str();
			if ( !szURL || !szURL[0] )
			{
				Warning( "GC sent malformed CGCUpdateItemSchema message: No schema data, no URL\n" );
			}
			else
			{
				Q_strncpy( m_szUrl, szURL, sizeof( m_szUrl ) );
				//Msg( "Fetching %s to update item schema\n", m_szUrl );

				// Send an HTTP request for the file
				ISteamHTTP *pHTTP = GetISteamHTTP();
				if ( !pHTTP )
				{
					//Warning( "Can't get ISteamHTTP to update item schema\n");
					return true;
				}
				HTTPRequestHandle hReq = pHTTP->CreateHTTPRequest( k_EHTTPMethodGET, m_szUrl );
				pHTTP->SetHTTPRequestNetworkActivityTimeout( hReq, 10 );
				SteamAPICall_t hCall;
				if ( !pHTTP->SendHTTPRequest( hReq, &hCall ) )
				{
					Warning( "Failed to update item schema: couldn't fetch %s\n", m_szUrl );
					return true;
				}

				//
				// *Wait* for completion.
				//
				// This is important.  The GC needs to be able to safely assume that
				// we will not process the next message until we have finished
				// dealing with this one.
				//
				bHTTPCompleted = false;
				#ifndef CLIENT_DLL
					if ( steamgameserverapicontext != NULL && pHTTP == steamgameserverapicontext->SteamHTTP() )
					{
						callback.SetGameserverFlag();
					}
				#endif
				callback.Set( hCall, this, &CGCUpdateItemSchema::OnHTTPCompleted );

				// Wait for it to finish.
				while ( !bHTTPCompleted )
				{
					BYieldingWaitOneFrame();
				}
			}
		}

		return true;
	}

	void OnHTTPCompleted( HTTPRequestCompleted_t *arg, bool bFailed )
	{
		// Clear flag, no matter what else, so we can stop yielding
		bHTTPCompleted = true;

		ISteamHTTP *pHTTP = GetISteamHTTP();
		Assert( pHTTP );
		if ( !pHTTP ) return;

		if ( arg->m_eStatusCode != k_EHTTPStatusCode200OK )
		{
			Warning( "Failed to update item schema: HTTP status %d fetching %s\n", arg->m_eStatusCode, m_szUrl );
		}
		else
		{
			if ( !arg->m_bRequestSuccessful )
			{
				bFailed = true;
			}
			if ( !bFailed )
			{
				uint32 unBodySize;
				if ( !pHTTP->GetHTTPResponseBodySize( arg->m_hRequest, &unBodySize ) )
				{
					Assert( false );
					bFailed = true;
				}
				else
				{
					bool bDidInit = ItemSystem()->GetItemSchema()->MaybeInitFromBuffer( new DelayedSchemaData_HTTPResponseData( pHTTP, arg->m_hRequest, unBodySize, m_nExpectedVersion, m_sSignature ) );
					Msg( bDidInit ? GC_ITEM_SCHEMA_UPDATE_APPLIED : GC_ITEM_SCHEMA_UPDATE_QUEUED, unBodySize, m_nExpectedVersion );
				}
			}

			if ( bFailed )
			{
				Warning( "Failed to update item schema from %s\n", m_szUrl );
			}
		}

		pHTTP->ReleaseHTTPRequest( arg->m_hRequest );
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCUpdateItemSchema, "CGCUpdateItemSchema", k_EMsgGCUpdateItemSchema, GCSDK::k_EServerTypeGCClient );

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Update the item schema from the GC
//-----------------------------------------------------------------------------
CON_COMMAND_F( econ_show_items_with_tag, "Lists the item definitions that have a specified tag.", FCVAR_CLIENTDLL )
{
	if ( args.ArgC() != 2 )
		return;

	econ_tag_handle_t tagHandle = GetItemSchema()->GetHandleForTag( args.Arg( 1 ) );
	FOR_EACH_MAP( GetItemSchema()->GetSortedItemDefinitionMap(), i )
	{
		const CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinitionMap()[i];

		if ( pItemDef->HasEconTag( tagHandle ) )
		{
			Msg("   '%s'\n", pItemDef->GetDefinitionName() );
		}
	}
}
#endif // CLIENT_DLL

