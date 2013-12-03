//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hl2_gamestats.h"
#include "achievementmgr.h"

static CHL2GameStats s_HL2GameStats;

CHL2GameStats::CHL2GameStats( void )
{
	gamestats = &s_HL2GameStats;
}
