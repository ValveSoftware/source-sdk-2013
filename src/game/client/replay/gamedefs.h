//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#ifndef GAMEDEFS_H
#define GAMEDEFS_H
#ifdef _WIN32
#pragma once
#endif

#define REPLAY_SOURCE_FILE

#if defined( TF_CLIENT_DLL )

#  include "tf_gamestats_shared.h"

#  define REPLAY_GAMESTATS_UNDEFINED		TFSTAT_UNDEFINED
#  define REPLAY_GAMESTATS_REVENGE			TFSTAT_REVENGE
#  define REPLAY_GAMESTATS_DEATHS			TFSTAT_DEATHS
#  define REPLAY_GAMESTATS_DOMINATIONS		TFSTAT_DOMINATIONS
#  define REPLAY_GAMESTATS_POINTSSCORED		TFSTAT_POINTSSCORED
#  define REPLAY_GAMESTATS_MAX				TFSTAT_TOTAL

#  define REPLAY_TEAM_TEAM0					TF_TEAM_RED
#  define REPLAY_TEAM_TEAM1					TF_TEAM_BLUE

#  define REPLAY_CLASS_UNDEFINED			TF_CLASS_UNDEFINED
#  define REPLAY_NUM_CLASSES				TF_CLASS_MENU_BUTTONS

#  define REPLAY_DEATH_DOMINATION			TF_DEATH_DOMINATION
#  define REPLAY_DEATH_ASSISTER_DOMINATION	TF_DEATH_ASSISTER_DOMINATION
#  define REPLAY_DEATH_REVENGE				TF_DEATH_REVENGE
#  define REPLAY_DEATH_ASSISTER_REVENGE		TF_DEATH_ASSISTER_REVENGE

#  define	C_ReplayGame_PlayerResource		
#  include "c_tf_playerresource.h"
typedef C_TF_PlayerResource					C_ReplayGame_PlayerResource_t;

#  define ReplayStatType_t					TFStatType_t

#  define REPLAY_MAX_DISPLAY_GAMESTATS		23

#elif defined( CSTRIKE_DLL )

#  include "cs_gamestats_shared.h"

#  define REPLAY_GAMESTATS_UNDEFINED		CSSTAT_UNDEFINED
#  define REPLAY_GAMESTATS_REVENGE			CSSTAT_REVENGES
#  define REPLAY_GAMESTATS_DEATHS			CSSTAT_DEATHS
#  define REPLAY_GAMESTATS_DOMINATIONS		CSSTAT_DOMINATIONS
#  define REPLAY_GAMESTATS_POINTSSCORED		CSSTAT_UNDEFINED		// Sheeeeeeeeeit
#  define REPLAY_GAMESTATS_MAX				CSSTAT_MAX

#  define REPLAY_TEAM_TEAM0					TEAM_TERRORIST
#  define REPLAY_TEAM_TEAM1					TEAM_CT

#  define REPLAY_CLASS_UNDEFINED			CS_CLASS_NONE
#  define REPLAY_NUM_CLASSES				CS_NUM_CLASSES

#  define REPLAY_DEATH_DOMINATION			CS_DEATH_DOMINATION	
#  define REPLAY_DEATH_REVENGE				CS_DEATH_REVENGE

#  include "c_cs_playerresource.h"
typedef C_CS_PlayerResource					C_ReplayGame_PlayerResource_t;

#  define ReplayStatType_t					CSStatType_t
#  define RoundStats_t						StatsCollection_t

#  define REPLAY_MAX_DISPLAY_GAMESTATS		3

#endif

struct StatInfo_t
{
	ReplayStatType_t	m_nStat;
	const char			*m_pStatLocalizationToken;
};

extern StatInfo_t g_pReplayDisplayGameStats[REPLAY_MAX_DISPLAY_GAMESTATS];

#endif	// GAMEDEFS_H
