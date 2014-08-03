//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

//Gamestats was built for ep1, so this file is going to be amazingly short seeing as how ep1 set the standard

#include "cbase.h"
#include "ep1_gamestats.h"
#include "tier1/utlbuffer.h"
static CEP1GameStats s_CEP1GS_ThisJustSitsInMemory;

// A bit of a hack to redirect the gamestats API for ep2 (ep3, etc.)
extern CBaseGameStats *g_pEP2GameStats;

CEP1GameStats::CEP1GameStats( void )
{
	gamestats = &s_CEP1GS_ThisJustSitsInMemory;
}

CBaseGameStats *CEP1GameStats::OnInit( CBaseGameStats *pCurrentGameStats, char const *gamedir )
{
	if ( !Q_stricmp( gamedir, "ep2" ) )
	{
		return g_pEP2GameStats;
	}

	return pCurrentGameStats;
}

const char *CEP1GameStats::GetStatSaveFileName( void )
{
	return "ep1_gamestats.dat"; //overriding the default for backwards compatibility with release stat tracking code
}

const char *CEP1GameStats::GetStatUploadRegistryKeyName( void )
{
	return "GameStatsUpload_Ep1"; //overriding the default for backwards compatibility with release stat tracking code
}


static char const *ep1Maps[] =
{
		"ep1_citadel_00",
		"ep1_citadel_01",
		"ep1_citadel_02",
		"ep1_citadel_02b",
		"ep1_citadel_03",
		"ep1_citadel_04",
		"ep1_c17_00",
		"ep1_c17_00a",
		"ep1_c17_01",
		"ep1_c17_02",
		"ep1_c17_02b",
		"ep1_c17_02a",
		"ep1_c17_05",
		"ep1_c17_06",
};


bool CEP1GameStats::UserPlayedAllTheMaps( void )
{
	int c = ARRAYSIZE( ep1Maps );
	for ( int i = 0; i < c; ++i )
	{
		int idx = m_BasicStats.m_MapTotals.Find( ep1Maps[ i ] );
		if( idx == m_BasicStats.m_MapTotals.InvalidIndex() )
			return false;
	}

	return true;
}

