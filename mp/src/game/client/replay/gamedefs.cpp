//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#include "cbase.h"
#include "gamedefs.h"

//----------------------------------------------------------------------------------------

StatInfo_t g_pReplayDisplayGameStats[REPLAY_MAX_DISPLAY_GAMESTATS] =
{
#if defined( TF_CLIENT_DLL )

	{ TFSTAT_SHOTS_HIT,			"#Stat_ShotsHit", },
	{ TFSTAT_SHOTS_FIRED,		"#Stat_ShotsFired" },
	{ TFSTAT_DAMAGE,			"#Stat_Damage" },
	{ TFSTAT_CAPTURES,			"#Stat_Captures" },
	{ TFSTAT_DEFENSES,			"#Stat_Defenses" },
	{ TFSTAT_REVENGE,			"#Stat_Revenge" },
	{ TFSTAT_POINTSSCORED,		"#Stat_PointsScored" },
	{ TFSTAT_BUILDINGSDESTROYED,"#Stat_BuildingsDestroyed" },
	{ TFSTAT_HEADSHOTS,			"#Stat_Headshots" },
	{ TFSTAT_PLAYTIME,			"#Stat_PlayTime" },

	{ TFSTAT_HEALING,			"#Stat_Healing" },
	{ TFSTAT_INVULNS,			"#Stat_Invulns" },
	{ TFSTAT_KILLASSISTS,		"#Stat_KillAssists" },
	{ TFSTAT_BACKSTABS,			"#Stat_BackStabs" },
	{ TFSTAT_HEALTHLEACHED,		"#Stat_HealthLeached" },
	{ TFSTAT_BUILDINGSBUILT,	"#Stat_BuildingsBuilt" },
	{ TFSTAT_MAXSENTRYKILLS,	"#Stat_MaxSentryKills" },
	{ TFSTAT_TELEPORTS,			"#Stat_Teleports" },
	{ TFSTAT_FIREDAMAGE,		"#Stat_FiredDamage" },
	{ TFSTAT_BONUS_POINTS,		"#Stat_BonusPoints" },

	{ TFSTAT_BLASTDAMAGE,		"#Stat_BlastDamage" },
	{ TFSTAT_DAMAGETAKEN,		"#Stat_DamageTaken" },
	{ TFSTAT_CRITS,				"#Stat_Crits" },

#elif defined( CSTRIKE_DLL )

	{ CSSTAT_SHOTS_HIT,			"#Stat_ShotsHit" },
	{ CSSTAT_SHOTS_FIRED,		"#Stat_ShotsFired" },
	{ CSSTAT_DAMAGE,			"#Stat_Damage" },

#endif
};