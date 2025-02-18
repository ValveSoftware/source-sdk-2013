//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "econ_gcmessages.h"
#include "econ_item_system.h"
#include "econ_ui.h"
#include "store/store_panel.h"
#include "gc_clientsystem.h"
#include "client_community_market.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static const float s_fUpdateTimeInSeconds = 60.0f * 15.0f;

typedef CUtlMap< steam_market_gc_identifier_t, client_market_data_t, unsigned int >	ClientMarketDataMap_t;
static ClientMarketDataMap_t	s_mapClientMarketData;
static float					g_fClientMarketDataLastUpdateTime = -s_fUpdateTimeInSeconds;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void ClientMarketData_Refresh()
{
	if ( !EconUI() || !EconUI()->GetStorePanel() )
		return;

	GCSDK::CProtoBufMsg<CMsgGCClientMarketDataRequest> msg( k_EMsgGCClientRequestMarketData );
	msg.Body().set_user_currency( EconUI()->GetStorePanel()->GetCurrency() );
	GCClientSystem()->BSendMessage( msg );

	g_fClientMarketDataLastUpdateTime = engine->Time();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const client_market_data_t *GetClientMarketData( const steam_market_gc_identifier_t& ident )
{
	// If our data is out of date, request fresh data from the GC. We don't need up-to-the-minute
	// numbers but we don't want to fall too far behind. THe GC itself doesn't update in realtime
	// so constantly querying for updates isn't really useful. We'll still use whatever data if any
	// we have for this call.
	if ( (engine->Time() - g_fClientMarketDataLastUpdateTime) >= s_fUpdateTimeInSeconds )
	{
		ClientMarketData_Refresh();
	}

	// Not having any data on this item isn't an error. We might be requesting something for an
	// unlistable item, or we might not have current information from the GC yet.
	if ( s_mapClientMarketData.Count() == 0 )
		return NULL;

	// Remap this index?
	steam_market_gc_identifier_t searchIdent = ident;
	searchIdent.m_unDefIndex = GetItemSchema()->GetCommunityMarketRemappedDefinitionIndex( ident.m_unDefIndex );

	ClientMarketDataMap_t::IndexType_t index = s_mapClientMarketData.Find( searchIdent );
	if ( index == s_mapClientMarketData.InvalidIndex() )
		return NULL;

	return &s_mapClientMarketData[index];
}
//-----------------------------------------------------------------------------
const client_market_data_t *GetClientMarketData( item_definition_index_t iItemDef, uint8 unQuality )
{
	steam_market_gc_identifier_t ident;
	ident.m_unDefIndex = iItemDef;
	ident.m_unQuality = unQuality;

	return GetClientMarketData( ident );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CGCClientRequestMarketDataResponse : public GCSDK::CGCClientJob
{
public:
	CGCClientRequestMarketDataResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGCClientMarketData> msg( pNetPacket );

		s_mapClientMarketData.RemoveAll();
		s_mapClientMarketData.SetLessFunc( DefLessFunc( ClientMarketDataMap_t::KeyType_t ) );

		for ( int i = 0; i < msg.Body().entries_size(); i++ )
		{
			const CMsgGCClientMarketDataEntry& entry = msg.Body().entries( i );

			steam_market_gc_identifier_t ident;
			ident.m_unDefIndex = entry.item_def_index();
			ident.m_unQuality = entry.item_quality();

			client_market_data_t data;
			data.m_unQuantityAvailable = entry.item_sell_listings();
			data.m_unLowestPrice = entry.price_in_local_currency();

			s_mapClientMarketData.Insert( ident, data );
		}

		return true;
	}

};
GC_REG_JOB( GCSDK::CGCClient, CGCClientRequestMarketDataResponse, "CGCClientRequestMarketDataResponse", k_EMsgGCClientRequestMarketDataResponse, GCSDK::k_EServerTypeGCClient );