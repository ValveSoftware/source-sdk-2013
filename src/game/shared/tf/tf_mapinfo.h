//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_MAPINFO_H
#define TF_MAPINFO_H
#ifdef _WIN32
#pragma once
#endif

/**
 * @param unAccountID
 * @param pLevelName
 * @return how many times the player has donated
 */
int MapInfo_GetDonationAmount( uint32 unAccountID, const char *pLevelName );

/**
 * @param unAccountID
 * @param pLevelName
 * return true if the player donated to the current map, false otherwise
 */
bool MapInfo_DidPlayerDonate( uint32 unAccountID, const char *pLevelName );

#endif // TF_MAPINFO_H
