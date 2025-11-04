//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "tf_mapinfo.h"
#include <filesystem.h>
#include "GameEventListener.h"
#include "econ_item_system.h"
#include "tf_item_inventory.h"
#include "econ_contribution.h"
#include "tf_duel_summary.h"
#include "gc_clientsystem.h"
#include "tf_gamerules.h"
#include "tf_matchmaking_shared.h"

#ifdef CLIENT_DLL
#include "usermessages.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static const char *FindMapNameForContributionDefinitionIndex( item_definition_index_t unContribDefIndex )
{
	for ( int i = 0; i < GetItemSchema()->GetMapCount(); i++ )
	{
		const MapDef_t* pMapDef = GetItemSchema()->GetMasterMapDefByIndex( i );
		if ( pMapDef->mapStampDef && pMapDef->mapStampDef->GetDefinitionIndex() == unContribDefIndex )
			return pMapDef->pszMapName;
	}
	return NULL;
}

bool MapInfo_DidPlayerDonate( uint32 unAccountID, const char *pLevelName )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return false;

	CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
	CSteamID steamID = localSteamID;
	steamID.SetAccountID( unAccountID );

	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
	if ( pSOCache == NULL )
		return false;

	GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( CTFMapContribution::k_nTypeID );
	if ( pTypeCache == NULL )
		return false;

	char pchBaseMapName[ MAX_PATH ];
	Q_FileBase( pLevelName, pchBaseMapName, sizeof(pchBaseMapName) );

	for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
	{
		CTFMapContribution *pMapContribution = (CTFMapContribution*)( pTypeCache->GetObject( i ) );

		const char *pszMapName = FindMapNameForContributionDefinitionIndex( pMapContribution->Obj().def_index() );
		if ( pszMapName && FStrEq( pszMapName, pchBaseMapName ) )
			return true;
	}
	return false;
}

int MapInfo_GetDonationAmount( uint32 unAccountID, const char *pLevelName )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return 0;

	CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
	CSteamID steamID = localSteamID;
	steamID.SetAccountID( unAccountID );

	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
	if ( pSOCache == NULL )
		return 0;

	GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( CTFMapContribution::k_nTypeID );
	if ( pTypeCache == NULL )
		return 0;

	char pchBaseMapName[ MAX_PATH ];
	Q_FileBase( pLevelName, pchBaseMapName, sizeof(pchBaseMapName) );

	for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
	{
		CTFMapContribution *pMapContribution = (CTFMapContribution*)( pTypeCache->GetObject( i ) );

		const char *pszMapName = FindMapNameForContributionDefinitionIndex( pMapContribution->Obj().def_index() );
		if ( pszMapName && FStrEq( pszMapName, pchBaseMapName ) )
			return pMapContribution->Obj().contribution_level();
	}
	return 0;
}