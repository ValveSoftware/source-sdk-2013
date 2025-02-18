//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Quickplay related code shared between GC and client
//
// $NoKeywords: $
//=============================================================================//

#ifndef _INCLUDED_TF_QUICKPLAY_SHARED_H
#define _INCLUDED_TF_QUICKPLAY_SHARED_H
#ifdef _WIN32
#pragma once
#endif

class CUtlStringList;

const int kTFMaxQuickPlayServersToScore = 25;
const int kTFQuickPlayIdealMaxNumberOfPlayers = 24;
const int kTFQuickPlayMinMaxNumberOfPlayers = 18; // don't auto match to servers with max players set too low
const int kTFQuickPlayMaxPlayers = 33;

const struct SchemaMap_t *GetQuickplayMapInfoByName( const char *pMapName );

extern float QuickplayCalculateServerScore( int numHumans, int numBots, int maxPlayers, int nNumInSearchParty );

extern const char k_szQuickplayFAQ_URL[];

struct QuickplaySearchOptions
{
	EGameCategory m_eSelectedGameType;

	enum EServers
	{
		eServersOfficial,
		eServersCommunity,
		eServersDontCare
	};
	EServers m_eServers;

	enum ERandomCrits
	{
		eRandomCritsYes,
		eRandomCritsNo,
		eRandomCritsDontCare
	};
	ERandomCrits m_eRandomCrits;

	enum EDamageSpread
	{
		eDamageSpreadNo,
		eDamageSpreadYes,
		eDamageSpreadDontCare
	};
	EDamageSpread m_eDamageSpread;

	enum EMaxPlayers
	{
		eMaxPlayers24,
		eMaxPlayers30Plus,
		eMaxPlayersDontCare
	};
	EMaxPlayers m_eMaxPlayers;

	enum ERespawnTimes
	{
		eRespawnTimesDefault,
		eRespawnTimesInstant,
		eRespawnTimesDontCare
	};
	ERespawnTimes m_eRespawnTimes;

	enum EBetaContent
	{
		eBetaNo,
		eBetaYes
	};
	EBetaContent m_eBetaContent;

	CUtlString m_strMapName;
};

#endif // #ifndef _INCLUDED_TF_QUICKPLAY_SHARED_H
