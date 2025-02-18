//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Interface for the client to interact with the CTradingSession
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_TRADING_H
#define TF_TRADING_H
#ifdef _WIN32
#pragma once
#endif

class CEconItemView;

/**
 * @return CSteamID of the client
 */
CSteamID Trading_GetLocalPlayerSteamID();

/**
 * Request a trade session with the player by player index (i.e. in the same game)
 * @param iPlayerIdx
 */
void Trading_RequestTrade( int iPlayerIdx );

/**
 * Request a trade session with the player by CSteamID
 * @param steamID
 */
void Trading_RequestTrade( const CSteamID &steamID );

/**
 * Sends a gift to the player with the given steamID
 * @param steamID
 * @param giftItem
 */
void Trading_SendGift( const CSteamID &steamID, const CEconItemView& giftItem );

#endif // TF_TRADING_H
