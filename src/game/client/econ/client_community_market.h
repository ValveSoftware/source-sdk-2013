//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef CLIENT_COMMUNITY_MARKET_H
#define CLIENT_COMMUNITY_MARKET_H
#ifdef _WIN32
#pragma once
#endif

struct client_market_data_t
{
	uint32 m_unQuantityAvailable;
	float m_unLowestPrice;
};

const client_market_data_t *GetClientMarketData( item_definition_index_t iItemDef, uint8 unQuality );

const client_market_data_t *GetClientMarketData( const steam_market_gc_identifier_t& ident );

#endif // CLIENT_COMMUNITY_MARKET_H
