//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_ladder_data.h"
#include "gcsdk/enumutils.h"
#include "tf_match_description.h"

#ifdef CLIENT_DLL
#include "econ/confirm_dialog.h"
#include "tf_matchmaking_shared.h"
#include "c_tf_player.h"
#endif

using namespace GCSDK;


//-----------------------------------------------------------------------------
// Purpose: Get player's ladder stat data by steamID.  Returns nullptr if it doesn't exist
//-----------------------------------------------------------------------------
CSOTFLadderData *YieldingGetPlayerLadderDataBySteamID( const CSteamID &steamID, ETFMatchGroup nMatchGroup )
{
	GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
	if ( pSOCache )
	{
		auto *pTypeCache = pSOCache->FindTypeCache( CSOTFLadderData::k_nTypeID );
		if ( pTypeCache )
		{
			for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
			{
				CSOTFLadderData *pLadderData = (CSOTFLadderData*)pTypeCache->GetObject( i );
				if ( nMatchGroup == (ETFMatchGroup)pLadderData->Obj().match_group() )
				{
					return pLadderData;
				}
			}
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Get the local player's Ladder Data.  Returns NULL if it doesn't exist (no GC)
//-----------------------------------------------------------------------------
const CSOTFLadderData *GetLocalPlayerLadderData( ETFMatchGroup nMatchGroup )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		return YieldingGetPlayerLadderDataBySteamID( steamapicontext->SteamUser()->GetSteamID(), nMatchGroup );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSOTFLadderData::CSOTFLadderData()
{
	Obj().set_account_id( 0 );
	Obj().set_match_group( k_eTFMatchGroup_Invalid );
	Obj().set_season_id( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSOTFLadderData::CSOTFLadderData( uint32 unAccountID, ETFMatchGroup eMatchGroup  )
{
	Obj().set_account_id( unAccountID );
	Obj().set_match_group( eMatchGroup );
	Obj().set_season_id( 1 );
}

void GetLocalPlayerMatchHistory( ETFMatchGroup nMatchGroup, CUtlVector < CSOTFMatchResultPlayerStats > &vecMatchesOut )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
		if ( pSOCache )
		{
			GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( CSOTFMatchResultPlayerInfo::k_nTypeID );
			if ( pTypeCache )
			{
				for ( uint32 i = 0; i < pTypeCache->GetCount(); ++i )
				{
					CSOTFMatchResultPlayerInfo *pMatchStats = (CSOTFMatchResultPlayerInfo*)pTypeCache->GetObject( i );
					if ( nMatchGroup == (ETFMatchGroup)pMatchStats->Obj().match_group() )
					{
						vecMatchesOut.AddToTail( pMatchStats->Obj() );
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSOTFMatchResultPlayerInfo::CSOTFMatchResultPlayerInfo()
{
	Obj().set_account_id( 0 );
}

