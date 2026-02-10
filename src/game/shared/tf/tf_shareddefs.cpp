//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_shareddefs.h"
#include "KeyValues.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"
#include "filesystem.h"
#include "tf_matchmaking_shared.h"

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
const char *g_aTeamNames[TF_TEAM_COUNT] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue"
};

color32 g_aTeamColors[TF_TEAM_COUNT] = 
{
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 255, 0, 0, 0 },
	{ 0, 0, 255, 0 }
};

//-----------------------------------------------------------------------------
// Classes.
//-----------------------------------------------------------------------------

const char *g_aPlayerClassNames[TF_CLASS_MENU_BUTTONS] =
{
	"#TF_Class_Name_Undefined",
	"#TF_Class_Name_Scout",
	"#TF_Class_Name_Sniper",
	"#TF_Class_Name_Soldier",
	"#TF_Class_Name_Demoman",
	"#TF_Class_Name_Medic",
	"#TF_Class_Name_HWGuy",
	"#TF_Class_Name_Pyro",
	"#TF_Class_Name_Spy",
	"#TF_Class_Name_Engineer",
	"#TF_Class_Name_Civilian",
	"",
	"#TF_Random"
};

const char *g_aPlayerClassNames_NonLocalized[TF_CLASS_MENU_BUTTONS] =
{
	"Undefined",
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer",
	"Civilian",
	"",
	"Random"
};

const char *g_aRawPlayerClassNamesShort[TF_CLASS_MENU_BUTTONS] =
{
	"undefined",
	"scout",
	"sniper",
	"soldier",
	"demo",	// short
	"medic",
	"heavy",// short
	"pyro",
	"spy",
	"engineer",
	"civilian",
	"",
	"random"
};

const char *g_aRawPlayerClassNames[TF_CLASS_MENU_BUTTONS] =
{
	"undefined",
	"scout",
	"sniper",
	"soldier",
	"demoman",
	"medic",
	"heavyweapons",
	"pyro",
	"spy",
	"engineer",
	"civilian",
	"",
	"random"
};

const char g_szBotModels[][ MAX_PATH ] = 
{
	"", //TF_CLASS_UNDEFINED

	"models/bots/scout/bot_scout.mdl",
	"models/bots/sniper/bot_sniper.mdl",
	"models/bots/soldier/bot_soldier.mdl",
	"models/bots/demo/bot_demo.mdl",
	"models/bots/medic/bot_medic.mdl",
	"models/bots/heavy/bot_heavy.mdl",
	"models/bots/pyro/bot_pyro.mdl",
	"models/bots/spy/bot_spy.mdl",
	"models/bots/engineer/bot_engineer.mdl",
};

const char g_szPlayerRobotModels[][MAX_PATH] =
{
	"", //TF_CLASS_UNDEFINED

	"models/bots/scout/bot_scout_human_anim.mdl",
	"models/bots/sniper/bot_sniper_human_anim.mdl",
	"models/bots/soldier/bot_soldier_human_anim.mdl",
	"models/bots/demo/bot_demo_human_anim.mdl",
	"models/bots/medic/bot_medic_human_anims.mdl",
	"models/bots/heavy/bot_heavy_human_anims.mdl",
	"models/bots/pyro/bot_pyro_human_anim.mdl",
	"models/bots/spy/bot_spy_human_anims.mdl",
	"models/bots/engineer/bot_engineer_human_anim.mdl",
};

const char g_szBotBossModels[][ MAX_PATH ] = 
{
	"", //TF_CLASS_UNDEFINED

	"models/bots/scout_boss/bot_scout_boss.mdl",
	"models/bots/sniper/bot_sniper.mdl",
	"models/bots/soldier_boss/bot_soldier_boss.mdl",
	"models/bots/demo_boss/bot_demo_boss.mdl",
	"models/bots/medic/bot_medic.mdl",
	"models/bots/heavy_boss/bot_heavy_boss.mdl",
	"models/bots/pyro_boss/bot_pyro_boss.mdl",
	"models/bots/spy/bot_spy.mdl",
	"models/bots/engineer/bot_engineer.mdl",
};

const char g_szBotBossSentryBusterModel[ MAX_PATH ] = "models/bots/demo/bot_sentry_buster.mdl";

// Rome 2 promo models
const char g_szRomePromoItems_Hat[][ MAX_PATH ] = 
{
	"", //TF_CLASS_UNDEFINED

	"tw_scoutbot_hat",
	"tw_sniperbot_helmet",
	"tw_soldierbot_helmet",
	"tw_demobot_helmet",
	"tw_medibot_hat",
	"tw_heavybot_helmet",
	"tw_pyrobot_helmet",
	"tw_spybot_hood",
	"tw_engineerbot_helmet",
};

const char g_szRomePromoItems_Misc[][ MAX_PATH ] = 
{
	"", //TF_CLASS_UNDEFINED

	"tw_scoutbot_armor",
	"tw_sniperbot_armor",
	"tw_soldierbot_armor",
	"tw_demobot_armor",
	"tw_medibot_chariot",
	"tw_heavybot_armor",
	"tw_pyrobot_armor",
	"tw_spybot_armor",
	"tw_engineerbot_armor",
};

const char *g_pszBreadModels[] = 
{
	"models/weapons/c_models/c_bread/c_bread_baguette.mdl",		// Spy
	"models/weapons/c_models/c_bread/c_bread_burnt.mdl",		// Pyro
	"models/weapons/c_models/c_bread/c_bread_cinnamon.mdl",		// Demo?
	"models/weapons/c_models/c_bread/c_bread_cornbread.mdl",	// Engineer
	"models/weapons/c_models/c_bread/c_bread_crumpet.mdl",		// Sniper?
	"models/weapons/c_models/c_bread/c_bread_plainloaf.mdl",	// Scout
	"models/weapons/c_models/c_bread/c_bread_pretzel.mdl",		// Medic
	"models/weapons/c_models/c_bread/c_bread_ration.mdl",		// Soldier
	"models/weapons/c_models/c_bread/c_bread_russianblack.mdl",	// Heavy?
};

int GetClassIndexFromString( const char *pClassName, int nLastClassIndex/*=TF_LAST_NORMAL_CLASS*/ )
{
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= nLastClassIndex; ++i )
	{
		// compare first N characters to allow matching both "heavy" and "heavyweapons"
		int classnameLength = V_strlen( g_aPlayerClassNames_NonLocalized[i] );

		if ( V_strlen( pClassName ) < classnameLength )
			continue;

		if ( !V_strnicmp( g_aPlayerClassNames_NonLocalized[i], pClassName, classnameLength ) )
		{
			return i;
		}
	}

	return TF_CLASS_UNDEFINED;
}

int iRemapIndexToClass[TF_CLASS_MENU_BUTTONS] =
{
	0,
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,
		TF_CLASS_PYRO,
		TF_CLASS_DEMOMAN,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_ENGINEER,
		TF_CLASS_MEDIC,
		TF_CLASS_SNIPER,
		TF_CLASS_SPY,
		0,
		0,
		TF_CLASS_RANDOM
};

int GetRemappedMenuIndexForClass( int iClass )
{
	int iIndex = 0;

	for ( int i = 0 ; i < TF_CLASS_MENU_BUTTONS ; i++ )
	{
		if ( iRemapIndexToClass[i] == iClass )
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}

ETFCond condition_to_attribute_translation[]  =
{
	TF_COND_BURNING,					// 1 (1<<0)
	TF_COND_AIMING,						// 2 (1<<1)
	TF_COND_ZOOMED,						// 4 (1<<2)
	TF_COND_DISGUISING,					// 8 (...)
	TF_COND_DISGUISED,					// 16
	TF_COND_STEALTHED,					// 32
	TF_COND_INVULNERABLE,				// 64
	TF_COND_TELEPORTED,					// 128
	TF_COND_TAUNTING,					// 256
	TF_COND_INVULNERABLE_WEARINGOFF,	// 512
	TF_COND_STEALTHED_BLINK,			// 1024
	TF_COND_SELECTED_TO_TELEPORT,		// 2048
	TF_COND_CRITBOOSTED,				// 4096
	TF_COND_TMPDAMAGEBONUS,				// 8192
	TF_COND_FEIGN_DEATH,				// 16384
	TF_COND_PHASE,						// 32768
	TF_COND_STUNNED,					// 65536
	TF_COND_HEALTH_BUFF,				// 131072
	TF_COND_HEALTH_OVERHEALED,			// 262144
	TF_COND_URINE,						// 524288
	TF_COND_ENERGY_BUFF,				// 1048576

	TF_COND_LAST				// sentinel value checked against when iterating
};

ETFCond g_aDebuffConditions[] =
{
	TF_COND_BURNING,
	TF_COND_URINE,
	TF_COND_BLEEDING,
	TF_COND_MAD_MILK,
	TF_COND_GAS,
	TF_COND_LAST
};

bool ConditionExpiresFast( ETFCond eCond )
{
	return eCond == TF_COND_BURNING
		|| eCond == TF_COND_URINE
		|| eCond == TF_COND_BLEEDING
		|| eCond == TF_COND_MAD_MILK
		|| eCond == TF_COND_GAS;
}

static const char *g_aConditionNames[] =
{
	"TF_COND_AIMING",                           // = 0 - Sniper aiming, Heavy minigun.
	"TF_COND_ZOOMED",                           // = 1
	"TF_COND_DISGUISING",                       // = 2
	"TF_COND_DISGUISED",                        // = 3
	"TF_COND_STEALTHED",                        // = 4 - Spy specific
	"TF_COND_INVULNERABLE",                     // = 5
	"TF_COND_TELEPORTED",                       // = 6
	"TF_COND_TAUNTING",                         // = 7
	"TF_COND_INVULNERABLE_WEARINGOFF",          // = 8
	"TF_COND_STEALTHED_BLINK",                  // = 9
	"TF_COND_SELECTED_TO_TELEPORT",             // = 10
	"TF_COND_CRITBOOSTED",                      // = 11 - DO NOT RE-USE THIS -- THIS IS FOR KRITZKRIEG AND REVENGE CRITS ONLY
	"TF_COND_TMPDAMAGEBONUS",                   // = 12
	"TF_COND_FEIGN_DEATH",                      // = 13
	"TF_COND_PHASE",                            // = 14
	"TF_COND_STUNNED",                          // = 15 - Any type of stun. Check iStunFlags for more info.
	"TF_COND_OFFENSEBUFF",                      // = 16
	"TF_COND_SHIELD_CHARGE",                    // = 17
	"TF_COND_DEMO_BUFF",                        // = 18
	"TF_COND_ENERGY_BUFF",                      // = 19
	"TF_COND_RADIUSHEAL",                       // = 20
	"TF_COND_HEALTH_BUFF",                      // = 21
	"TF_COND_BURNING",                          // = 22
	"TF_COND_HEALTH_OVERHEALED",                // = 23
	"TF_COND_URINE",                            // = 24
	"TF_COND_BLEEDING",                         // = 25
	"TF_COND_DEFENSEBUFF",                      // = 26 - 35% defense! No crit damage.
	"TF_COND_MAD_MILK",                         // = 27
	"TF_COND_MEGAHEAL",                         // = 28
	"TF_COND_REGENONDAMAGEBUFF",                // = 29
	"TF_COND_MARKEDFORDEATH",                   // = 30
	"TF_COND_NOHEALINGDAMAGEBUFF",              // = 31
	"TF_COND_SPEED_BOOST",                      // = 32
	"TF_COND_CRITBOOSTED_PUMPKIN",              // = 33 - Brandon hates bits
	"TF_COND_CRITBOOSTED_USER_BUFF",            // = 34
	"TF_COND_CRITBOOSTED_DEMO_CHARGE",          // = 35
	"TF_COND_SODAPOPPER_HYPE",                  // = 36
	"TF_COND_CRITBOOSTED_FIRST_BLOOD",          // = 37 - arena mode first blood
	"TF_COND_CRITBOOSTED_BONUS_TIME",           // = 38
	"TF_COND_CRITBOOSTED_CTF_CAPTURE",          // = 39
	"TF_COND_CRITBOOSTED_ON_KILL",              // = 40 - KGB, etc.
	"TF_COND_CANNOT_SWITCH_FROM_MELEE",         // = 41
	"TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK",        // = 42 - 35% defense! Still damaged by crits.
	"TF_COND_REPROGRAMMED",                     // = 43 - Bots only
	"TF_COND_CRITBOOSTED_RAGE_BUFF",            // = 44
	"TF_COND_DEFENSEBUFF_HIGH",                 // = 45 - 75% defense! Still damaged by crits.
	"TF_COND_SNIPERCHARGE_RAGE_BUFF",           // = 46 - Sniper Rage - Charge time speed up
	"TF_COND_DISGUISE_WEARINGOFF",              // = 47 - Applied for half-second post-disguise
	"TF_COND_MARKEDFORDEATH_SILENT",            // = 48 - Sans sound
	"TF_COND_DISGUISED_AS_DISPENSER",           // = 49
	"TF_COND_SAPPED",                           // = 50 - Bots only
	"TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED", // = 51
	"TF_COND_INVULNERABLE_USER_BUFF",           // = 52
	"TF_COND_HALLOWEEN_BOMB_HEAD",              // = 53
	"TF_COND_HALLOWEEN_THRILLER",               // = 54
	"TF_COND_RADIUSHEAL_ON_DAMAGE",             // = 55
	"TF_COND_CRITBOOSTED_CARD_EFFECT",          // = 56
	"TF_COND_INVULNERABLE_CARD_EFFECT",         // = 57
	"TF_COND_MEDIGUN_UBER_BULLET_RESIST",       // = 58
	"TF_COND_MEDIGUN_UBER_BLAST_RESIST",        // = 59
	"TF_COND_MEDIGUN_UBER_FIRE_RESIST",         // = 60
	"TF_COND_MEDIGUN_SMALL_BULLET_RESIST",      // = 61
	"TF_COND_MEDIGUN_SMALL_BLAST_RESIST",       // = 62
	"TF_COND_MEDIGUN_SMALL_FIRE_RESIST",        // = 63
	"TF_COND_STEALTHED_USER_BUFF",              // = 64 - Any class can have this
	"TF_COND_MEDIGUN_DEBUFF",                   // = 65
	"TF_COND_STEALTHED_USER_BUFF_FADING",       // = 66
	"TF_COND_BULLET_IMMUNE",                    // = 67
	"TF_COND_BLAST_IMMUNE",                     // = 68
	"TF_COND_FIRE_IMMUNE",                      // = 69
	"TF_COND_PREVENT_DEATH",                    // = 70
	"TF_COND_MVM_BOT_STUN_RADIOWAVE",           // = 71 - Bots only
	"TF_COND_HALLOWEEN_SPEED_BOOST",            // = 72
	"TF_COND_HALLOWEEN_QUICK_HEAL",             // = 73
	"TF_COND_HALLOWEEN_GIANT",                  // = 74
	"TF_COND_HALLOWEEN_TINY",                   // = 75
	"TF_COND_HALLOWEEN_IN_HELL",                // = 76
	"TF_COND_HALLOWEEN_GHOST_MODE",             // = 77
	"TF_COND_MINICRITBOOSTED_ON_KILL",          // = 78
	"TF_COND_OBSCURED_SMOKE",                   // = 79
	"TF_COND_PARACHUTE_ACTIVE",                 // = 80
	"TF_COND_BLASTJUMPING",                     // = 81
	"TF_COND_HALLOWEEN_KART",                   // = 82
	"TF_COND_HALLOWEEN_KART_DASH",              // = 83
	"TF_COND_BALLOON_HEAD",                     // = 84 - larger head, lower-gravity-feeling jumps
	"TF_COND_MELEE_ONLY",                       // = 85 - melee only
	"TF_COND_SWIMMING_CURSE",                   // = 86 - player movement become swimming movement
	"TF_COND_FREEZE_INPUT",                     // = 87 - freezes player input
	"TF_COND_HALLOWEEN_KART_CAGE",              // = 88 - attach cage model to player while in kart
	"TF_COND_DONOTUSE_0",                       // = 89
	"TF_COND_RUNE_STRENGTH",                    // = 90
	"TF_COND_RUNE_HASTE",                       // = 91
	"TF_COND_RUNE_REGEN",                       // = 92
	"TF_COND_RUNE_RESIST",                      // = 93
	"TF_COND_RUNE_VAMPIRE",                     // = 94
	"TF_COND_RUNE_REFLECT",                     // = 95
	"TF_COND_RUNE_PRECISION",                   // = 96
	"TF_COND_RUNE_AGILITY",                     // = 97
	"TF_COND_GRAPPLINGHOOK",                    // = 98
	"TF_COND_GRAPPLINGHOOK_SAFEFALL",           // = 99
	"TF_COND_GRAPPLINGHOOK_LATCHED",            // = 100
	"TF_COND_GRAPPLINGHOOK_BLEEDING",           // = 101
	"TF_COND_AFTERBURN_IMMUNE",                 // = 102
	"TF_COND_RUNE_KNOCKOUT",                    // = 103
	"TF_COND_RUNE_IMBALANCE",                   // = 104
	"TF_COND_CRITBOOSTED_RUNE_TEMP",            // = 105
	"TF_COND_PASSTIME_INTERCEPTION",            // = 106
	"TF_COND_SWIMMING_NO_EFFECTS",              // = 107 - =107_DNOC_FT
	"TF_COND_PURGATORY",                        // = 108
	"TF_COND_RUNE_KING",                        // = 109
	"TF_COND_RUNE_PLAGUE",                      // = 110
	"TF_COND_RUNE_SUPERNOVA",                   // = 111
	"TF_COND_PLAGUE",                           // = 112
	"TF_COND_KING_BUFFED",                      // = 113
	"TF_COND_TEAM_GLOWS",                       // = 114 - used to show team glows to living players
	"TF_COND_KNOCKED_INTO_AIR",                 // = 115
	"TF_COND_COMPETITIVE_WINNER",               // = 116
	"TF_COND_COMPETITIVE_LOSER",                // = 117
	"TF_COND_HEALING_DEBUFF",                   // = 118
	"TF_COND_PASSTIME_PENALTY_DEBUFF",          // = 119
	"TF_COND_GRAPPLED_TO_PLAYER",               // = 120
	"TF_COND_GRAPPLED_BY_PLAYER",               // = 121
	"TF_COND_PARACHUTE_DEPLOYED",               // = 122
	"TF_COND_GAS",                              // = 123
	"TF_COND_BURNING_PYRO",                     // = 124
	"TF_COND_ROCKETPACK",                       // = 125
	"TF_COND_LOST_FOOTING",                     // = 126
	"TF_COND_AIR_CURRENT",                      // = 127
	"TF_COND_HALLOWEEN_HELL_HEAL",              // = 128
	"TF_COND_POWERUPMODE_DOMINANT",             // = 129
	"TF_COND_IMMUNE_TO_PUSHBACK",				// = 130

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	// ******** Keep this block last! ********
	// Keep experimental conditions below and graduate out of it before shipping
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aConditionNames ) == TF_COND_LAST );

const char *GetTFConditionName( ETFCond eCond )
{
	if ( ( eCond >= ARRAYSIZE( g_aConditionNames ) ) || ( eCond < 0 ) )
		return NULL;

	return g_aConditionNames[eCond];
}


ETFCond GetTFConditionFromName( const char *pszCondName )
{
	for( uint i=0; i<TF_COND_LAST; i++ )
	{ 
		ETFCond eCond = (ETFCond)i;
		if ( !V_stricmp( GetTFConditionName( eCond ), pszCondName ) ) 
			return eCond;
	} 

	Assert( !!"Invalid Condition Name" );
	return TF_COND_INVALID;
}


//-----------------------------------------------------------------------------
// Gametypes.
//-----------------------------------------------------------------------------
static const char *s_aGameTypeNames[] =
{
	"Undefined",
	"#Gametype_CTF",
	"#Gametype_CP",
	"#Gametype_Escort",
	"#Gametype_Arena",
	"#Gametype_MVM",
	"#Gametype_RobotDestruction",
	"#GameType_Passtime",
	"#GameType_PlayerDestruction",
};
COMPILE_TIME_ASSERT( TF_GAMETYPE_COUNT == ARRAYSIZE( s_aGameTypeNames ) );

const char *GetGameTypeName( ETFGameType gameType )
{
	return s_aGameTypeNames[ gameType ];
}

static const char *s_aEnumGameTypeName[] =
{
	"TF_GAMETYPE_UNDEFINED",
	"TF_GAMETYPE_CTF",
	"TF_GAMETYPE_CP",
	"TF_GAMETYPE_ESCORT",
	"TF_GAMETYPE_ARENA",
	"TF_GAMETYPE_MVM",
	"TF_GAMETYPE_RD",
	"TF_GAMETYPE_PASSTIME",
	"TF_GAMETYPE_PD"
};
COMPILE_TIME_ASSERT( TF_GAMETYPE_COUNT == ARRAYSIZE( s_aEnumGameTypeName ) );

const char *GetEnumGameTypeName( ETFGameType gameType )
{
	return s_aEnumGameTypeName[ gameType ];
}

ETFGameType GetGameTypeFromName( const char *pszGameType )
{
	for ( int i=0; i<TF_GAMETYPE_COUNT; ++i )
	{
		if ( FStrEq( pszGameType, s_aEnumGameTypeName[i] ) )
			return ETFGameType(i);
	}

	return TF_GAMETYPE_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
const char *g_aAmmoNames[] =
{
	"DUMMY AMMO",
	"TF_AMMO_PRIMARY",
	"TF_AMMO_SECONDARY",
	"TF_AMMO_METAL",
	"TF_AMMO_GRENADES1",
	"TF_AMMO_GRENADES2",
	"TF_AMMO_GRENADES3"
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aAmmoNames ) == TF_AMMO_COUNT );

const char *GetAmmoName( int iAmmoType )
{
	ETFAmmoType eAmmoType = (ETFAmmoType)iAmmoType;
	return g_aAmmoNames[ eAmmoType ];
}

const char *g_aCTFEventNames[] =
{
	"",
	"TF_FLAGEVENT_PICKUP",
	"TF_FLAGEVENT_CAPTURE",
	"TF_FLAGEVENT_DEFEND",
	"TF_FLAGEVENT_DROPPED",
	"TF_FLAGEVENT_RETURNED",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aCTFEventNames ) == TF_NUM_FLAG_EVENTS );

const char *GetCTFEventName( ETFFlagEventTypes iEventType )
{
	return g_aCTFEventNames[ iEventType ];
}

ETFFlagEventTypes GetCTFEventTypeFromName( const char *pszName )
{
	for( int i=TF_FLAGEVENT_PICKUP; i < TF_NUM_FLAG_EVENTS; ++i )
	{
		if ( FStrEq( pszName, GetCTFEventName( (ETFFlagEventTypes)i ) ) )
		{
			return (ETFFlagEventTypes)i;
		}
	}
	
	Assert( false );
	return TF_NUM_FLAG_EVENTS;
}



const char *GetRDScoreMethodName( RDScoreMethod_t iScoreMethod )
{
	static const char *aRDScoreMethodNames[] =
	{
		"SCORE_UNDEFINED", // -1
		"SCORE_REACTOR_CAPTURED", // 0
		"SCORE_CORES_COLLECTED",
		"SCORE_REACTOR_RETURNED",
		"SCORE_REACTOR_STEAL",
		
		"NUM_SCORE_TYPES"
	};

	return aRDScoreMethodNames[ iScoreMethod + 1 ];
}

RDScoreMethod_t GetRDScoreMethodFromName( const char *pszName )
{
	for( int i=SCORE_UNDEFINED; i < NUM_SCORE_TYPES; ++i )
	{
		if ( FStrEq( pszName, GetRDScoreMethodName( (RDScoreMethod_t)i ) ) )
		{
			return (RDScoreMethod_t)i;
		}
	}
	
	Assert( false );
	return SCORE_UNDEFINED;
}

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
const char *g_aWeaponNames[] =
{
	"TF_WEAPON_NONE",
	"TF_WEAPON_BAT",
	"TF_WEAPON_BAT_WOOD",
	"TF_WEAPON_BOTTLE", 
	"TF_WEAPON_FIREAXE",
	"TF_WEAPON_CLUB",
	"TF_WEAPON_CROWBAR",
	"TF_WEAPON_KNIFE",
	"TF_WEAPON_FISTS",
	"TF_WEAPON_SHOVEL",
	"TF_WEAPON_WRENCH",
	"TF_WEAPON_BONESAW",
	"TF_WEAPON_SHOTGUN_PRIMARY",
	"TF_WEAPON_SHOTGUN_SOLDIER",
	"TF_WEAPON_SHOTGUN_HWG",
	"TF_WEAPON_SHOTGUN_PYRO",
	"TF_WEAPON_SCATTERGUN",
	"TF_WEAPON_SNIPERRIFLE",
	"TF_WEAPON_MINIGUN",
	"TF_WEAPON_SMG",
	"TF_WEAPON_SYRINGEGUN_MEDIC",
	"TF_WEAPON_TRANQ",
	"TF_WEAPON_ROCKETLAUNCHER",
	"TF_WEAPON_GRENADELAUNCHER",
	"TF_WEAPON_PIPEBOMBLAUNCHER",
	"TF_WEAPON_FLAMETHROWER",
	"TF_WEAPON_GRENADE_NORMAL",
	"TF_WEAPON_GRENADE_CONCUSSION",
	"TF_WEAPON_GRENADE_NAIL",
	"TF_WEAPON_GRENADE_MIRV",
	"TF_WEAPON_GRENADE_MIRV_DEMOMAN",
	"TF_WEAPON_GRENADE_NAPALM",
	"TF_WEAPON_GRENADE_GAS",
	"TF_WEAPON_GRENADE_EMP",
	"TF_WEAPON_GRENADE_CALTROP",
	"TF_WEAPON_GRENADE_PIPEBOMB",
	"TF_WEAPON_GRENADE_SMOKE_BOMB",
	"TF_WEAPON_GRENADE_HEAL",
	"TF_WEAPON_GRENADE_STUNBALL",
	"TF_WEAPON_GRENADE_JAR",
	"TF_WEAPON_GRENADE_JAR_MILK",
	"TF_WEAPON_PISTOL",
	"TF_WEAPON_PISTOL_SCOUT",
	"TF_WEAPON_REVOLVER",
	"TF_WEAPON_NAILGUN",
	"TF_WEAPON_PDA",
	"TF_WEAPON_PDA_ENGINEER_BUILD",
	"TF_WEAPON_PDA_ENGINEER_DESTROY",
	"TF_WEAPON_PDA_SPY",
	"TF_WEAPON_BUILDER",
	"TF_WEAPON_MEDIGUN",
	"TF_WEAPON_GRENADE_MIRVBOMB",
	"TF_WEAPON_FLAMETHROWER_ROCKET",
	"TF_WEAPON_GRENADE_DEMOMAN",
	"TF_WEAPON_SENTRY_BULLET",
	"TF_WEAPON_SENTRY_ROCKET",
	"TF_WEAPON_DISPENSER",
	"TF_WEAPON_INVIS",
	"TF_WEAPON_FLAREGUN",
	"TF_WEAPON_LUNCHBOX",
	"TF_WEAPON_JAR",
	"TF_WEAPON_COMPOUND_BOW",
	"TF_WEAPON_BUFF_ITEM",
	"TF_WEAPON_PUMPKIN_BOMB",
	"TF_WEAPON_SWORD",
	"TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT",
	"TF_WEAPON_LIFELINE",
	"TF_WEAPON_LASER_POINTER",
	"TF_WEAPON_DISPENSER_GUN",
	"TF_WEAPON_SENTRY_REVENGE",
	"TF_WEAPON_JAR_MILK",
	"TF_WEAPON_HANDGUN_SCOUT_PRIMARY",
	"TF_WEAPON_BAT_FISH",
	"TF_WEAPON_CROSSBOW",
	"TF_WEAPON_STICKBOMB",
	"TF_WEAPON_HANDGUN_SCOUT_SECONDARY",
	"TF_WEAPON_SODA_POPPER",
	"TF_WEAPON_SNIPERRIFLE_DECAP",
	"TF_WEAPON_RAYGUN",
	"TF_WEAPON_PARTICLE_CANNON",
	"TF_WEAPON_MECHANICAL_ARM",
	"TF_WEAPON_DRG_POMSON",
	"TF_WEAPON_BAT_GIFTWRAP",
	"TF_WEAPON_GRENADE_ORNAMENT_BALL",
	"TF_WEAPON_FLAREGUN_REVENGE",
	"TF_WEAPON_PEP_BRAWLER_BLASTER",
	"TF_WEAPON_CLEAVER",
	"TF_WEAPON_GRENADE_CLEAVER",
	"TF_WEAPON_STICKY_BALL_LAUNCHER",
	"TF_WEAPON_GRENADE_STICKY_BALL",
	"TF_WEAPON_SHOTGUN_BUILDING_RESCUE",
	"TF_WEAPON_CANNON",
	"TF_WEAPON_THROWABLE",
	"TF_WEAPON_GRENADE_THROWABLE",
	"TF_WEAPON_PDA_SPY_BUILD",
	"TF_WEAPON_GRENADE_WATERBALLOON",
	"TF_WEAPON_HARVESTER_SAW",
	"TF_WEAPON_SPELLBOOK",
	"TF_WEAPON_SPELLBOOK_PROJECTILE",
	"TF_WEAPON_SNIPERRIFLE_CLASSIC",
	"TF_WEAPON_PARACHUTE",
	"TF_WEAPON_GRAPPLINGHOOK",
	"TF_WEAPON_PASSTIME_GUN",
	"TF_WEAPON_CHARGED_SMG",
	"TF_WEAPON_BREAKABLE_SIGN",
	"TF_WEAPON_ROCKETPACK",
	"TF_WEAPON_SLAP",
	"TF_WEAPON_JAR_GAS",
	"TF_WEAPON_GRENADE_JAR_GAS",
	"TF_WEPON_FLAME_BALL",

};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_aWeaponNames ) == TF_WEAPON_COUNT );

int g_aWeaponDamageTypes[] =
{
	DMG_GENERIC,	// TF_WEAPON_NONE
	DMG_CLUB,		// TF_WEAPON_BAT,
	DMG_CLUB,		// TF_WEAPON_BAT_WOOD,
	DMG_CLUB,		// TF_WEAPON_BOTTLE, 
	DMG_CLUB,		// TF_WEAPON_FIREAXE,
	DMG_CLUB,		// TF_WEAPON_CLUB,
	DMG_CLUB,		// TF_WEAPON_CROWBAR,
	DMG_SLASH,		// TF_WEAPON_KNIFE,
	DMG_CLUB,		// TF_WEAPON_FISTS,
	DMG_CLUB,		// TF_WEAPON_SHOVEL,
	DMG_CLUB,		// TF_WEAPON_WRENCH,
	DMG_SLASH,		// TF_WEAPON_BONESAW,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_PRIMARY,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_SOLDIER,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_HWG,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_PYRO,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,  // TF_WEAPON_SCATTERGUN,
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_SNIPERRIFLE,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_MINIGUN,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_SMG,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_SYRINGEGUN_MEDIC,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE | DMG_PARALYZE,		// TF_WEAPON_TRANQ,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_ROCKETLAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_GRENADELAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_NOCLOSEDISTANCEMOD,		// TF_WEAPON_PIPEBOMBLAUNCHER,
	DMG_IGNITE | DMG_PREVENT_PHYSICS_FORCE | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_FLAMETHROWER,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_NORMAL,
	DMG_SONIC | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_CONCUSSION,
	DMG_BULLET | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_NAIL,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_MIRV,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	DMG_BURN | DMG_RADIUS_MAX,		// TF_WEAPON_GRENADE_NAPALM,
	DMG_POISON | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_GAS,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_GRENADE_EMP,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_CALTROP,
	DMG_BLAST | DMG_HALF_FALLOFF  | DMG_NOCLOSEDISTANCEMOD,		// TF_WEAPON_GRENADE_PIPEBOMB,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_SMOKE_BOMB,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_HEAL
	DMG_CLUB,		// TF_WEAPON_GRENADE_STUNBALL
	DMG_CLUB,		// TF_WEAPON_GRENADE_JAR
	DMG_CLUB,		// TF_WEAPON_GRENADE_JAR_MILK
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL_SCOUT,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_REVOLVER,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_NAILGUN,
	DMG_BULLET,		// TF_WEAPON_PDA,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_BUILD,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_DESTROY,
	DMG_BULLET,		// TF_WEAPON_PDA_SPY,
	DMG_BULLET,		// TF_WEAPON_BUILDER
	DMG_BULLET,		// TF_WEAPON_MEDIGUN
	DMG_BLAST,		// TF_WEAPON_GRENADE_MIRVBOMB
	DMG_BLAST | DMG_IGNITE | DMG_RADIUS_MAX,		// TF_WEAPON_FLAMETHROWER_ROCKET
	DMG_BLAST | DMG_HALF_FALLOFF,					// TF_WEAPON_GRENADE_DEMOMAN
	DMG_BULLET,	// TF_WEAPON_SENTRY_BULLET
	DMG_BLAST,	// TF_WEAPON_SENTRY_ROCKET
	DMG_GENERIC,	// TF_WEAPON_DISPENSER
	DMG_GENERIC,	// TF_WEAPON_INVIS
	DMG_BULLET | DMG_IGNITE,		// TF_WEAPON_FLAREGUN
	DMG_GENERIC,	// TF_WEAPON_LUNCHBOX
	DMG_GENERIC,	// TF_WEAPON_JAR
	DMG_BULLET | DMG_USE_HITLOCATIONS,		// TF_WEAPON_COMPOUND_BOW
	DMG_GENERIC,	// TF_WEAPON_BUFF_ITEM
	DMG_CLUB,		// TF_WEAPON_PUMPKIN_BOMB
	DMG_CLUB,		// TF_WEAPON_SWORD
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT,
	DMG_CLUB,		// TF_WEAPON_LIFELINE
	DMG_CLUB,		// TF_WEAPON_LASER_POINTER
	DMG_BULLET,		// TF_WEAPON_DISPENSER_GUN
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD, // TF_WEAPON_SENTRY_REVENGE
	DMG_GENERIC,	// TF_WEAPON_JAR_MILK
	DMG_BUCKSHOT | DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_HANDGUN_SCOUT_PRIMARY
	DMG_CLUB,		// TF_WEAPON_BAT_FISH
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_CROSSBOW
	DMG_CLUB, // TF_WEAPON_STICKBOMB
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_HANDGUN_SCOUT_SECONDARY
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,  // TF_WEAPON_SODA_POPPER,
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_SNIPERRIFLE_DECAP,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,	// TF_WEAPON_RAYGUN,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_PARTICLE_CANNON,
	DMG_BULLET | DMG_USEDISTANCEMOD,	// TF_WEAPON_MECHANICAL_ARM,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,	// TF_WEAPON_DRG_POMSON,
	DMG_CLUB,		// TF_WEAPON_BAT_GIFTWRAP,
	DMG_CLUB,		// TF_WEAPON_GRENADE_ORNAMENT_BALL
	DMG_BULLET | DMG_IGNITE,	// TF_WEAPON_FLAREGUN_REVENGE,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,  // TF_WEAPON_PEP_BRAWLER_BLASTER,
	DMG_GENERIC,	// TF_WEAPON_CLEAVER
	DMG_SLASH,		// TF_WEAPON_GRENADE_CLEAVER
	DMG_GENERIC,	// TF_WEAPON_STICKY_BALL_LAUNCHER,
	DMG_GENERIC,	// TF_WEAPON_GRENADE_STICKY_BALL,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN_BUILDING_RESCUE,
	DMG_BLAST | DMG_HALF_FALLOFF,	// TF_WEAPON_CANNON
	DMG_BULLET,		// TF_WEAPON_THROWABLE
	DMG_BULLET,		// TF_WEAPON_GRENADE_THROWABLE
	DMG_BULLET,		// TF_WEAPON_PDA_SPY_BUILD
	DMG_BULLET,		// TF_WEAPON_GRENADE_WATERBALLOON
	DMG_SLASH,		// TF_WEAPON_HARVESTER_SAW
	DMG_GENERIC,	// TF_WEAPON_SPELLBOOK
	DMG_GENERIC,	// TF_WEAPON_SPELLBOOK_PROJECTILE
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_SNIPERRIFLE_CLASSIC,
	DMG_GENERIC, // TF_WEAPON_PARACHUTE,
	DMG_GENERIC, // TF_WEAPON_GRAPPLINGHOOK,
	DMG_GENERIC, // TF_WEAPON_PASSTIME_GUN
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_CHARGED_SMG,
	DMG_CLUB,		// TF_WEAPON_BREAKABLE_SIGN,
	DMG_GENERIC, // TF_WEAPON_ROCKETPACK,
	DMG_CLUB, // TF_WEAPON_SLAP,
	DMG_GENERIC, // TF_WEAPON_JAR_GAS
	DMG_GENERIC, // TF_WEAPON_GRENADE_JAR_GAS
	DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE, // TF_WEAPON_FLAME_BALL

};

const char *g_szSpecialDamageNames[] =
{
	"",
	"TF_DMG_CUSTOM_HEADSHOT",
	"TF_DMG_CUSTOM_BACKSTAB",
	"TF_DMG_CUSTOM_BURNING",
	"TF_DMG_WRENCH_FIX",
	"TF_DMG_CUSTOM_MINIGUN",
	"TF_DMG_CUSTOM_SUICIDE",
	"TF_DMG_CUSTOM_TAUNTATK_HADOUKEN",
	"TF_DMG_CUSTOM_BURNING_FLARE",
	"TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON",
	"TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM",
	"TF_DMG_CUSTOM_PENETRATE_MY_TEAM",
	"TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS",
	"TF_DMG_CUSTOM_TAUNTATK_FENCING",
	"TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE",
	"TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB",
	"TF_DMG_CUSTOM_TELEFRAG",
	"TF_DMG_CUSTOM_BURNING_ARROW",
	"TF_DMG_CUSTOM_FLYINGBURN",
	"TF_DMG_CUSTOM_PUMPKIN_BOMB",
	"TF_DMG_CUSTOM_DECAPITATION",
	"TF_DMG_CUSTOM_TAUNTATK_GRENADE",
	"TF_DMG_CUSTOM_BASEBALL",
	"TF_DMG_CUSTOM_CHARGE_IMPACT",
	"TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING",
	"TF_DMG_CUSTOM_AIR_STICKY_BURST",
	"TF_DMG_CUSTOM_DEFENSIVE_STICKY",
	"TF_DMG_CUSTOM_PICKAXE",
	"TF_DMG_CUSTOM_ROCKET_DIRECTHIT",
	"TF_DMG_CUSTOM_TAUNTATK_UBERSLICE",
	"TF_DMG_CUSTOM_PLAYER_SENTRY",
	"TF_DMG_CUSTOM_STANDARD_STICKY",
	"TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT",
	"TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH",
	"TF_DMG_CUSTOM_BLEEDING",
	"TF_DMG_CUSTOM_GOLD_WRENCH",
	"TF_DMG_CUSTOM_CARRIED_BUILDING",
	"TF_DMG_CUSTOM_COMBO_PUNCH",
	"TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL",
	"TF_DMG_CUSTOM_FISH_KILL",
	"TF_DMG_CUSTOM_TRIGGER_HURT",
	"TF_DMG_CUSTOM_DECAPITATION_BOSS",
	"TF_DMG_CUSTOM_STICKBOMB_EXPLOSION",
	"TF_DMG_CUSTOM_AEGIS_ROUND",
	"TF_DMG_CUSTOM_FLARE_EXPLOSION",
	"TF_DMG_CUSTOM_BOOTS_STOMP",
	"TF_DMG_CUSTOM_PLASMA",
	"TF_DMG_CUSTOM_PLASMA_CHARGED",
	"TF_DMG_CUSTOM_PLASMA_GIB",
	"TF_DMG_CUSTOM_PRACTICE_STICKY",
	"TF_DMG_CUSTOM_EYEBALL_ROCKET",
	"TF_DMG_CUSTOM_HEADSHOT_DECAPITATION",
	"TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON",
	"TF_DMG_CUSTOM_FLARE_PELLET",
	"TF_DMG_CUSTOM_CLEAVER",
	"TF_DMG_CUSTOM_CLEAVER_CRIT",
	"TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH",
	"TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB",
	"TF_DMG_CUSTOM_MERASMUS_GRENADE",
	"TF_DMG_CUSTOM_MERASMUS_ZAP",
	"TF_DMG_CUSTOM_MERASMUS_DECAPITATION",
	"TF_DMG_CUSTOM_CANNONBALL_PUSH",
	"TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF",
	"TF_DMG_CUSTOM_THROWABLE",
	"TF_DMG_CUSTOM_THROWABLE_KILL",
	"TF_DMG_CUSTOM_SPELL_TELEPORT",
	"TF_DMG_CUSTOM_SPELL_SKELETON",
	"TF_DMG_CUSTOM_SPELL_MIRV",
	"TF_DMG_CUSTOM_SPELL_METEOR",
	"TF_DMG_CUSTOM_SPELL_LIGHTNING",
	"TF_DMG_CUSTOM_SPELL_FIREBALL",
	"TF_DMG_CUSTOM_SPELL_MONOCULUS",
	"TF_DMG_CUSTOM_SPELL_BLASTJUMP",
	"TF_DMG_CUSTOM_SPELL_BATS",
	"TF_DMG_CUSTOM_SPELL_TINY",
	"TF_DMG_CUSTOM_KART",
	"TF_DMG_CUSTOM_GIANT_HAMMER",
	"TF_DMG_CUSTOM_RUNE_REFLECT",
	"TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE",
	"TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING",
	"TF_DMG_CUSTOM_SLAP_KILL",
	"TF_DMG_CUSTOM_CROC",
	"TF_DMG_CUSTOM_TAUNTATK_GASBLAST",
	"TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED",
	"TF_DMG_CUSTOM_KRAMPUS_MELEE",
	"TF_DMG_CUSTOM_KRAMPUS_RANGED",
	"TF_DMG_CUSTOM_TAUNTATK_TRICKSHOT",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szSpecialDamageNames ) == TF_DMG_CUSTOM_END );

const char *GetCustomDamageName( ETFDmgCustom eDmgCustom )
{
	if ( ( eDmgCustom >= ARRAYSIZE( g_szSpecialDamageNames ) ) || ( eDmgCustom < 0 ) )
		return NULL;

	return g_szSpecialDamageNames[eDmgCustom];
}

ETFDmgCustom GetCustomDamageFromName( const char *pszCustomDmgName )
{
	for( uint i=0; i<TF_DMG_CUSTOM_END; i++ )
	{ 
		ETFDmgCustom eDmgCustom = (ETFDmgCustom)i;
		if ( !V_stricmp( GetCustomDamageName( eDmgCustom ), pszCustomDmgName ) ) 
			return eDmgCustom;
	} 

	Assert( !!"Invalid Custom Damage Name" );
	return TF_DMG_CUSTOM_NONE;
}

const char *g_szProjectileNames[] =
{
	"",
	"projectile_bullet",
	"projectile_rocket",
	"projectile_pipe",
	"projectile_pipe_remote",
	"projectile_syringe",
	"projectile_flare",
	"projectile_jar",
	"projectile_arrow",
	"projectile_flame_rocket",
	"projectile_jar_milk",
	"projectile_healing_bolt",
	"projectile_energy_ball",
	"projectile_energy_ring",
	"projectile_pipe_remote_practice",
	"projectile_cleaver",
	"projectile_sticky_ball",
	"projectile_cannonball",
	"projectile_building_repair_bolt",
	"projectile_festive_arrow",
	"projectile_throwable",
	"projectile_spellfireball",
	"projectile_festive_urine",
	"projectile_festive_healing_bolt",
	"projectfile_breadmonster_jarate",
	"projectfile_breadmonster_madmilk",
	"projectile_grapplinghook",
	"projectile_sentry_rocket",
	"projectile_bread_monster",
	"projectile_jar_gas",
	"tf_projectile_balloffire",

};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szProjectileNames ) == TF_NUM_PROJECTILES );

// these map to the projectiles named in g_szProjectileNames
int g_iProjectileWeapons[] = 
{
	TF_WEAPON_NONE,
	TF_WEAPON_PISTOL,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_FLAREGUN,
	TF_WEAPON_JAR,
	TF_WEAPON_COMPOUND_BOW,
	TF_PROJECTILE_FLAME_ROCKET,
	TF_WEAPON_JAR_MILK,
	TF_WEAPON_CROSSBOW,
	TF_WEAPON_PARTICLE_CANNON,
	TF_WEAPON_RAYGUN,
	TF_WEAPON_GRENADELAUNCHER,			// practice pipes should never kill anyone anyway
	TF_WEAPON_CLEAVER,
	TF_WEAPON_STICKY_BALL_LAUNCHER,
	TF_WEAPON_CANNON,
	TF_WEAPON_SHOTGUN_BUILDING_RESCUE,
	TF_WEAPON_COMPOUND_BOW,
	TF_WEAPON_THROWABLE,
	TF_WEAPON_SPELLBOOK,
	TF_WEAPON_JAR,
	TF_WEAPON_CROSSBOW,
	TF_WEAPON_JAR,
	TF_WEAPON_JAR,
	TF_PROJECTILE_GRAPPLINGHOOK,
	TF_WEAPON_SENTRY_ROCKET,
	TF_WEAPON_THROWABLE,
	TF_WEAPON_JAR_GAS,
	TF_WEAPON_FLAME_BALL,

};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_szProjectileNames ) == ARRAYSIZE( g_iProjectileWeapons ) );

//-----------------------------------------------------------------------------
// Taunt attacks
//-----------------------------------------------------------------------------
static const char* taunt_attack_name[] =
{
	"TAUNTATK_NONE",
	"TAUNTATK_PYRO_HADOUKEN",
	"TAUNTATK_HEAVY_EAT",
	"TAUNTATK_HEAVY_RADIAL_BUFF",
	"TAUNTATK_HEAVY_HIGH_NOON",
	"TAUNTATK_SCOUT_DRINK",
	"TAUNTATK_SCOUT_GRAND_SLAM",
	"TAUNTATK_MEDIC_INHALE",
	"TAUNTATK_SPY_FENCING_SLASH_A",
	"TAUNTATK_SPY_FENCING_SLASH_B",
	"TAUNTATK_SPY_FENCING_STAB",
	"TAUNTATK_RPS_KILL",
	"TAUNTATK_SNIPER_ARROW_STAB_IMPALE",
	"TAUNTATK_SNIPER_ARROW_STAB_KILL",
	"TAUNTATK_SOLDIER_GRENADE_KILL",
	"TAUNTATK_DEMOMAN_BARBARIAN_SWING",
	"TAUNTATK_MEDIC_UBERSLICE_IMPALE",
	"TAUNTATK_MEDIC_UBERSLICE_KILL",
	"TAUNTATK_FLIP_LAND_PARTICLE",
	"TAUNTATK_RPS_PARTICLE",
	"TAUNTATK_HIGHFIVE_PARTICLE",
	"TAUNTATK_ENGINEER_GUITAR_SMASH",
	"TAUNTATK_ENGINEER_ARM_IMPALE",
	"TAUNTATK_ENGINEER_ARM_KILL",
	"TAUNTATK_ENGINEER_ARM_BLEND",
	"TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN",
	"TAUNTATK_SHOW_ITEM",
	"TAUNTATK_MEDIC_RELEASE_DOVES",
	"TAUNTATK_PYRO_ARMAGEDDON",
	"TAUNTATK_PYRO_SCORCHSHOT",
	"TAUNTATK_ALLCLASS_GUITAR_RIFF",
	"TAUNTATK_MEDIC_HEROIC_TAUNT",
	"TAUNTATK_PYRO_GASBLAST",
	"TAUNTATK_ENGINEER_TRICKSHOT",

	//
	// INSERT NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

COMPILE_TIME_ASSERT( ARRAYSIZE( taunt_attack_name ) == TAUNTATK_COUNT );

taunt_attack_t GetTauntAttackByName( const char* pszTauntAttackName )
{
	if ( pszTauntAttackName )
	{
		for ( int i=0; i<ARRAYSIZE( taunt_attack_name ); ++i )
		{
			if ( !V_stricmp( pszTauntAttackName, taunt_attack_name[i] ) )
			{
				return (taunt_attack_t)i;
			}
		}
	}

	return TAUNTATK_NONE;
}

const char *g_pszHintMessages[] =
{
	"#Hint_spotted_a_friend",
	"#Hint_spotted_an_enemy",
	"#Hint_killing_enemies_is_good",
	"#Hint_out_of_ammo",
	"#Hint_turn_off_hints",
	"#Hint_pickup_ammo",
	"#Hint_Cannot_Teleport_With_Flag",
	"#Hint_Cannot_Cloak_With_Flag",
	"#Hint_Cannot_Disguise_With_Flag",
	"#Hint_Cannot_Attack_While_Feign_Armed",
	"#Hint_ClassMenu",

// Grenades
	"#Hint_gren_caltrops",
	"#Hint_gren_concussion",
	"#Hint_gren_emp",
	"#Hint_gren_gas",
	"#Hint_gren_mirv",
	"#Hint_gren_nail",
	"#Hint_gren_napalm",
	"#Hint_gren_normal",

// Altfires
	"#Hint_altfire_sniperrifle",
	"#Hint_altfire_flamethrower",
	"#Hint_altfire_grenadelauncher",
	"#Hint_altfire_pipebomblauncher",
	"#Hint_altfire_rotate_building",

// Soldier
	"#Hint_Soldier_rpg_reload",

// Engineer
	"#Hint_Engineer_use_wrench_onown",
	"#Hint_Engineer_use_wrench_onother",
	"#Hint_Engineer_use_wrench_onfriend",
	"#Hint_Engineer_build_sentrygun",
	"#Hint_Engineer_build_dispenser",
	"#Hint_Engineer_build_teleporters",
	"#Hint_Engineer_pickup_metal",
	"#Hint_Engineer_repair_object",
	"#Hint_Engineer_metal_to_upgrade",
	"#Hint_Engineer_upgrade_sentrygun",

	"#Hint_object_has_sapper",

	"#Hint_object_your_object_sapped",
	"#Hint_enemy_using_dispenser",
	"#Hint_enemy_using_tp_entrance",
	"#Hint_enemy_using_tp_exit",

	"#Hint_Cannot_Phase_With_Flag",

	"#Hint_Cannot_Attack_While_Cloaked",

	"#Hint_Cannot_Arm_Feign_Now",
};

const char *g_pszArrowModels[] = 
{
	"models/weapons/w_models/w_arrow.mdl",
	"models/weapons/w_models/w_repair_claw.mdl",
	"models/weapons/w_models/w_baseball.mdl",
	"models/weapons/w_models/w_arrow_xmas.mdl",
	"models/weapons/w_models/w_syringe_proj.mdl",
	"models/workshop/weapons/c_models/c_crusaders_crossbow/c_crusaders_crossbow_xmas_proj.mdl",
	"models/weapons/w_models/w_breadmonster/w_breadmonster.mdl",
	"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl",
	"models/workshop_partner/weapons/c_models/c_sd_cleaver/c_sd_cleaver.mdl"
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszArrowModels ) == TF_ARROW_MODEL_COUNT );

const char *g_pszCampaignMedalIcons[] =
{
	"",
	// Gun Mettle Campaign
	"../hud/coin_summer2015_gravel",
	"../hud/coin_summer2015_bronze",
	"../hud/coin_summer2015_silver",
	"../hud/coin_summer2015_gold",

	// Invasion Community Update
	"../HUD/scoreboard_invasion",

	// Halloween
	"../HUD/coin_halloween2015_gravel",
	"../HUD/coin_halloween2015_bronze",
	"../HUD/coin_halloween2015_silver",
	"../HUD/coin_halloween2015_gold",

	// Tough Break Campaign
	"../HUD/stamp_winter2016_gravel1",
	"../HUD/stamp_winter2016_bronze1",
	"../HUD/stamp_winter2016_silver1",
	"../HUD/stamp_winter2016_gold1",

	"../HUD/stamp_winter2016_gravel2",
	"../HUD/stamp_winter2016_bronze2",
	"../HUD/stamp_winter2016_silver2",
	"../HUD/stamp_winter2016_gold2",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszCampaignMedalIcons ) == CAMPAIGN_MEDAL_DISPLAY_TYPE_COUNT );

//-----------------------------------------------------------------------------
// Dead Calling Cards
const char *g_pszDeathCallingCardModels[] =
{
	"",			// Empty at zero
	"models/props_gameplay/tombstone_specialdelivery.mdl",		// Scout PolyCount Set
	"models/props_gameplay/tombstone_crocostyle.mdl",		// Sniper PolyCount Set
	"models/props_gameplay/tombstone_tankbuster.mdl",		// Solider PolyCount Set
	"models/props_gameplay/tombstone_gasjockey.mdl",		// Pyro PolyCount Set
};

const char *GetWeaponIDName( int iWeaponID )
{
	return ClampedArrayElement( g_aWeaponNames, iWeaponID );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponId( const char *pszWeaponName )
{
	// if this doesn't match, you need to add missing weapons to the array
	COMPILE_TIME_ASSERT( TF_WEAPON_COUNT == ARRAYSIZE( g_aWeaponNames ) );

	for ( int iWeapon = 0; iWeapon < ARRAYSIZE( g_aWeaponNames ); ++iWeapon )
	{
		if ( !Q_stricmp( pszWeaponName, g_aWeaponNames[iWeapon] ) )
			return iWeapon;
	}

	return TF_WEAPON_NONE;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *WeaponIdToAlias( int iWeapon )
{
	// if this doesn't match, you need to add missing weapons to the array
	COMPILE_TIME_ASSERT( TF_WEAPON_COUNT == ARRAYSIZE( g_aWeaponNames ) );

	if ( ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) ) || ( iWeapon < 0 ) )
		return NULL;

	return g_aWeaponNames[iWeapon];
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponFromDamage( const CTakeDamageInfo &info )
{
	int iWeapon = TF_WEAPON_NONE;

	const char *killer_weapon_name = "";

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pKiller, pInflictor, NULL );

	// find the weapon the killer used

	if ( pScorer )	// Is the killer a client?
	{
		if ( pInflictor )
		{
			if ( pInflictor == pScorer )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				if ( pScorer->GetActiveWeapon() )
				{
					killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
				}
			}
			else
			{
				killer_weapon_name = STRING( pInflictor->m_iClassname );  // it's just that easy
			}
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );
	}

	if ( !Q_strnicmp( killer_weapon_name, "tf_projectile", 13 ) )
	{
		for( int i = 0; i < ARRAYSIZE( g_szProjectileNames ); i++ )
		{
			if ( !Q_stricmp( &killer_weapon_name[ 3 ], g_szProjectileNames[ i ] ) )
			{
				iWeapon = g_iProjectileWeapons[ i ];
				break;
			}
		}
	}
	else
	{
		int iLen = Q_strlen( killer_weapon_name );

		// strip off _projectile from projectiles shot from other projectiles
		if ( ( iLen < 256 ) && ( iLen > 11 ) && !Q_stricmp( &killer_weapon_name[ iLen - 11 ], "_projectile" ) )
		{
			char temp[ 256 ];
			V_strcpy_safe( temp, killer_weapon_name );
			temp[ iLen - 11 ] = 0;

			// set the weapon used
			iWeapon = GetWeaponId( temp );
		}
		else
		{
			// set the weapon used
			iWeapon = GetWeaponId( killer_weapon_name );
		}
	}

	AssertMsg( iWeapon >= 0 && iWeapon < TF_WEAPON_COUNT, "Referencing a weapon not in tf_shareddefs.h.  Check to make it's defined and it's mapped correctly in g_szProjectileNames and g_iProjectileWeapons." );
	return iWeapon;
}

#endif

// ------------------------------------------------------------------------------------------------ //
// CObjectInfo tables.
// ------------------------------------------------------------------------------------------------ //

CObjectInfo::CObjectInfo( const char *pObjectName )
{
	m_pObjectName = pObjectName;
	m_pClassName = NULL;
	m_flBuildTime = -9999;
	m_nMaxObjects = -9999;
	m_Cost = -9999;
	m_CostMultiplierPerInstance = -999;
	m_flUpgradeDuration = -999;
	m_UpgradeCost = -9999;
	m_MaxUpgradeLevel = -9999;
	m_pBuilderWeaponName = NULL;
	m_pBuilderPlacementString = NULL;
	m_SelectionSlot = -9999;
	m_SelectionPosition = -9999;
	m_bSolidToPlayerMovement = false;
	m_pIconActive = NULL;
	m_pIconInactive = NULL;
	m_pIconMenu = NULL;
	m_pViewModel = NULL;
	m_pPlayerModel = NULL;
	m_iDisplayPriority = 0;
	m_bVisibleInWeaponSelection = true;
	m_pExplodeSound = NULL;
	m_pUpgradeSound = NULL;
	m_pExplosionParticleEffect = NULL;
	m_bAutoSwitchTo = false;
	m_iBuildCount = 0;
	m_iNumAltModes = 0;
	m_bRequiresOwnBuilder = false;
}


CObjectInfo::~CObjectInfo()
{
	delete [] m_pClassName;
	delete [] m_pStatusName;
	delete [] m_pBuilderWeaponName;
	delete [] m_pBuilderPlacementString;
	delete [] m_pIconActive;
	delete [] m_pIconInactive;
	delete [] m_pIconMenu;
	delete [] m_pViewModel;
	delete [] m_pPlayerModel;
	delete [] m_pExplodeSound;
	delete [] m_pUpgradeSound;
	delete [] m_pExplosionParticleEffect;
}

CObjectInfo g_ObjectInfos[OBJ_LAST] =
{
	CObjectInfo( "OBJ_DISPENSER" ),
	CObjectInfo( "OBJ_TELEPORTER" ),
	CObjectInfo( "OBJ_SENTRYGUN" ),
	CObjectInfo( "OBJ_ATTACHMENT_SAPPER" ),
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_ObjectInfos ) == OBJ_LAST );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetBuildableId( const char *pszBuildableName )
{
	for ( int iBuildable = 0; iBuildable < OBJ_LAST; ++iBuildable )
	{
		if ( !Q_stricmp( pszBuildableName, g_ObjectInfos[iBuildable].m_pObjectName ) )
			return iBuildable;
	}

	return OBJ_LAST;
}

bool AreObjectInfosLoaded()
{
	return g_ObjectInfos[0].m_pClassName != NULL;
}

static void SpewFileInfo( IBaseFileSystem *pFileSystem, const char *resourceName, const char *pathID, KeyValues *pValues )
{
	bool bFileExists = pFileSystem->FileExists( resourceName, pathID );
	bool bFileWritable = pFileSystem->IsFileWritable( resourceName, pathID );
	unsigned int nSize = pFileSystem->Size( resourceName, pathID );

	Msg( "resourceName:%s pathID:%s bFileExists:%d size:%u writeable:%d\n", resourceName, pathID, bFileExists, nSize, bFileWritable );

	unsigned int filesize = ( unsigned int )-1;
	FileHandle_t f = filesystem->Open( resourceName, "rb", pathID );
	if ( f )
	{
		filesize = filesystem->Size( f );
		filesystem->Close( f );
	}

	Msg( " FileHandle_t:%p size:%u\n", f, filesize );

	IFileSystem *pFS = 	(IFileSystem *)filesystem;

	char cwd[ MAX_PATH ];
	cwd[ 0 ] = 0;
	pFS->GetCurrentDirectory( cwd, ARRAYSIZE( cwd ) );

	bool bAvailable = pFS->IsFileImmediatelyAvailable( resourceName );

	Msg( " IsFileImmediatelyAvailable:%d cwd:%s\n", bAvailable, cwd );

	pFS->PrintSearchPaths();

	if ( pValues )
	{
		Msg( "Keys:" );
		KeyValuesDumpAsDevMsg( pValues, 2, 0 );
	}
}

void LoadObjectInfos( IBaseFileSystem *pFileSystem )
{
	const char *pFilename = "scripts/objects.txt";

	// Make sure this stuff hasn't already been loaded.
	Assert( !AreObjectInfosLoaded() );

	KeyValues *pValues = new KeyValues( "Object descriptions" );
	if ( !pValues->LoadFromFile( pFileSystem, pFilename, "GAME" ) )
	{
		// Getting "Can't open scripts/objects.txt for object info." errors. Spew file information
		//  before the Error() call which should show up in the minidumps.
		SpewFileInfo( pFileSystem, pFilename, "GAME", NULL );

		Error( "Can't open %s for object info.", pFilename );
		pValues->deleteThis();
		return;
	}

	// Now read each class's information in.
	for ( int iObj=0; iObj < ARRAYSIZE( g_ObjectInfos ); iObj++ )
	{
		CObjectInfo *pInfo = &g_ObjectInfos[iObj];
		KeyValues *pSub = pValues->FindKey( pInfo->m_pObjectName );
		if ( !pSub )
		{
			// Getting "Missing section 'OBJ_DISPENSER' from scripts/objects.txt" errors.
			SpewFileInfo( pFileSystem, pFilename, "GAME", pValues );

			// It seems that folks have corrupt files when these errors are seen in http://minidump.
			// Does it make sense to call the below Steam API so it'll force a validation next startup time?
			// Need to verify it's real corruption and not someone dorking around with their objects.txt file...
			//
			// From Martin Otten: If you have a file on disc and you’re 100% sure it’s
			//  corrupt, call ISteamApps::MarkContentCorrupt( false ), before you shutdown
			//  the game. This will cause a content validation in Steam.

			Error( "Missing section '%s' from %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		// Read all the info in.
		if ( (pInfo->m_flBuildTime = pSub->GetFloat( "BuildTime", -999 )) == -999 ||
			(pInfo->m_nMaxObjects = pSub->GetInt( "MaxObjects", -999 )) == -999 ||
			(pInfo->m_Cost = pSub->GetInt( "Cost", -999 )) == -999 ||
			(pInfo->m_CostMultiplierPerInstance = pSub->GetFloat( "CostMultiplier", -999 )) == -999 ||
			(pInfo->m_flUpgradeDuration = pSub->GetFloat( "UpgradeDuration", -999 )) == -999 ||
			(pInfo->m_UpgradeCost = pSub->GetInt( "UpgradeCost", -999 )) == -999 ||
			(pInfo->m_MaxUpgradeLevel = pSub->GetInt( "MaxUpgradeLevel", -999 )) == -999 ||
			(pInfo->m_SelectionSlot = pSub->GetInt( "SelectionSlot", -999 )) == -999 ||
			(pInfo->m_iBuildCount = pSub->GetInt( "BuildCount", -999 )) == -999 ||
			(pInfo->m_SelectionPosition = pSub->GetInt( "SelectionPosition", -999 )) == -999 )
		{
			SpewFileInfo( pFileSystem, pFilename, "GAME", pValues );

			Error( "Missing data for object '%s' in %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		pInfo->m_pClassName = ReadAndAllocStringValue( pSub, "ClassName", pFilename );
		pInfo->m_pStatusName = ReadAndAllocStringValue( pSub, "StatusName", pFilename );
		pInfo->m_pBuilderWeaponName = ReadAndAllocStringValue( pSub, "BuilderWeaponName", pFilename );
		pInfo->m_pBuilderPlacementString = ReadAndAllocStringValue( pSub, "BuilderPlacementString", pFilename );
		pInfo->m_bSolidToPlayerMovement = pSub->GetInt( "SolidToPlayerMovement", 0 ) ? true : false;
		pInfo->m_pIconActive = ReadAndAllocStringValue( pSub, "IconActive", pFilename );
		pInfo->m_pIconInactive = ReadAndAllocStringValue( pSub, "IconInactive", pFilename );
		pInfo->m_pIconMenu = ReadAndAllocStringValue( pSub, "IconMenu", pFilename );
		pInfo->m_bUseItemInfo = ( pSub->GetInt( "UseItemInfo", 0 ) > 0 );
		pInfo->m_pViewModel = ReadAndAllocStringValue( pSub, "Viewmodel", pFilename );
		pInfo->m_pPlayerModel = ReadAndAllocStringValue( pSub, "Playermodel", pFilename );
		pInfo->m_iDisplayPriority = pSub->GetInt( "DisplayPriority", 0 );
		pInfo->m_pHudStatusIcon = ReadAndAllocStringValue( pSub, "HudStatusIcon", pFilename );
		pInfo->m_bVisibleInWeaponSelection = ( pSub->GetInt( "VisibleInWeaponSelection", 1 ) > 0 );
		pInfo->m_pExplodeSound = ReadAndAllocStringValue( pSub, "ExplodeSound", pFilename );
		pInfo->m_pUpgradeSound = ReadAndAllocStringValue( pSub, "UpgradeSound", pFilename );
		pInfo->m_pExplosionParticleEffect = ReadAndAllocStringValue( pSub, "ExplodeEffect", pFilename );
		pInfo->m_bAutoSwitchTo = ( pSub->GetInt( "autoswitchto", 0 ) > 0 );
		pInfo->m_iMetalToDropInGibs = pSub->GetInt( "MetalToDropInGibs", 0 );
		pInfo->m_bRequiresOwnBuilder = pSub->GetBool( "RequiresOwnBuilder", 0 );

		// Read the other alternate object modes.
		KeyValues *pAltModesKey = pSub->FindKey( "AltModes" );
		if ( pAltModesKey )
		{
			int iIndex = 0;
			while ( iIndex<OBJECT_MAX_MODES )
			{
				char buf[256];
				Q_snprintf( buf, sizeof(buf), "AltMode%d", iIndex );
				KeyValues *pCurrentModeKey = pAltModesKey->FindKey( buf );
				if ( !pCurrentModeKey )
					break;

				pInfo->m_AltModes[iIndex].pszStatusName = ReadAndAllocStringValue( pCurrentModeKey, "StatusName", pFilename );
				pInfo->m_AltModes[iIndex].pszModeName   = ReadAndAllocStringValue( pCurrentModeKey, "ModeName",   pFilename );
				pInfo->m_AltModes[iIndex].pszIconMenu   = ReadAndAllocStringValue( pCurrentModeKey, "IconMenu",   pFilename );

				iIndex++;
			}
			pInfo->m_iNumAltModes = iIndex-1;
		}

		// Alternate mode 0 always matches the defaults.
		pInfo->m_AltModes[0].pszStatusName = pInfo->m_pStatusName;
		pInfo->m_AltModes[0].pszIconMenu   = pInfo->m_pIconMenu;
	}

	pValues->deleteThis();
}


const CObjectInfo* GetObjectInfo( int iObject )
{
	Assert( iObject >= 0 && iObject < OBJ_LAST );
	Assert( AreObjectInfosLoaded() );
	return &g_ObjectInfos[iObject];
}

ConVar tf_cheapobjects( "tf_cheapobjects","0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Set to 1 and all objects will cost 0" );

//-----------------------------------------------------------------------------
// Purpose: Return the cost of another object of the specified type
//			If bLast is set, return the cost of the last built object of the specified type
// 
// Note: Used to contain logic from tf2 that multiple instances of the same object
//       cost different amounts. See tf2/game_shared/tf_shareddefs.cpp for details
//-----------------------------------------------------------------------------
int InternalCalculateObjectCost( int iObjectType )
{
	if ( tf_cheapobjects.GetInt() )
	{
		return 0;
	}

	int iCost = GetObjectInfo( iObjectType )->m_Cost;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the cost to upgrade an object of a specific type
//-----------------------------------------------------------------------------
int	CalculateObjectUpgrade( int iObjectType, int iObjectLevel )
{
	// Max level?
	if ( iObjectLevel >= GetObjectInfo( iObjectType )->m_MaxUpgradeLevel )
		return 0;

	int iCost = GetObjectInfo( iObjectType )->m_UpgradeCost;
	for ( int i = 0; i < (iObjectLevel - 1); i++ )
	{
		iCost *= OBJECT_UPGRADE_COST_MULTIPLIER_PER_LEVEL;
	}

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified class is allowed to build the specified object type
//-----------------------------------------------------------------------------
bool ClassCanBuild( int iClass, int iObjectType )
{
	/*
	for ( int i = 0; i < OBJ_LAST; i++ )
	{
		// Hit the end?
		if ( g_TFClassInfos[iClass].m_pClassObjects[i] == OBJ_LAST )
			return false;

		// Found it?
		if ( g_TFClassInfos[iClass].m_pClassObjects[i] == iObjectType )
			return true;
	}

	return false;
	*/

	return ( iClass == TF_CLASS_ENGINEER );
}

const unsigned char *GetTFEncryptionKey( void )
{ 
	return (unsigned char *)"E2NcUkG2"; 
}

//-----------------------------------------------------------------------------
// Per-class weapon entity translations
//-----------------------------------------------------------------------------
struct wpntranslation_class_weapons_t
{
	const char *pszWpnString;
	const char *pszClassWpn[TF_LAST_NORMAL_CLASS];
};

wpntranslation_class_weapons_t pszWpnEntTranslationList[] = 
{
	{
		"tf_weapon_shotgun",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"",							// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"tf_weapon_shotgun_soldier",// TF_CLASS_SOLDIER,
			"",							// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"tf_weapon_shotgun_hwg",	// TF_CLASS_HEAVYWEAPONS,
			"tf_weapon_shotgun_pyro",	// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"tf_weapon_shotgun_primary",// TF_CLASS_ENGINEER,		
		}
	},

	{
		"tf_weapon_pistol",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"tf_weapon_pistol_scout",	// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"",							// TF_CLASS_SOLDIER,
			"",							// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"",							// TF_CLASS_HEAVYWEAPONS,
			"",							// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"tf_weapon_pistol",			// TF_CLASS_ENGINEER,		
		}
	},

	{
		"tf_weapon_shovel",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"",							// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"tf_weapon_shovel",			// TF_CLASS_SOLDIER,
			"tf_weapon_bottle",			// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"",							// TF_CLASS_HEAVYWEAPONS,
			"",							// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"",							// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_bottle",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"",							// TF_CLASS_SCOUT,
			"",							// TF_CLASS_SNIPER,
			"tf_weapon_shovel",			// TF_CLASS_SOLDIER,
			"tf_weapon_bottle",			// TF_CLASS_DEMOMAN,
			"",							// TF_CLASS_MEDIC,
			"",							// TF_CLASS_HEAVYWEAPONS,
			"",							// TF_CLASS_PYRO,
			"",							// TF_CLASS_SPY,
			"",							// TF_CLASS_ENGINEER,		
		}
	},
	{
		"saxxy",
		{
			"",							// TF_CLASS_UNDEFINED = 0,
			"tf_weapon_bat",			// TF_CLASS_SCOUT,
			"tf_weapon_club",			// TF_CLASS_SNIPER,
			"tf_weapon_shovel",			// TF_CLASS_SOLDIER,
			"tf_weapon_bottle",			// TF_CLASS_DEMOMAN,
			"tf_weapon_bonesaw",		// TF_CLASS_MEDIC,
			"tf_weapon_fireaxe",		// TF_CLASS_HEAVYWEAPONS,		HWG uses a fireaxe because he doesn't have a default melee weapon of his own; also I am a terrible person
			"tf_weapon_fireaxe",		// TF_CLASS_PYRO,
			"tf_weapon_knife",			// TF_CLASS_SPY,
			"tf_weapon_wrench",			// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_throwable",
		{
			"",											// TF_CLASS_UNDEFINED = 0,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SCOUT,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SNIPER,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SOLDIER,
			"tf_weapon_throwable_secondary",			// TF_CLASS_DEMOMAN,
			"tf_weapon_throwable_primary",				// TF_CLASS_MEDIC,
			"tf_weapon_throwable_secondary",			// TF_CLASS_HEAVYWEAPONS
			"tf_weapon_throwable_secondary",			// TF_CLASS_PYRO,
			"tf_weapon_throwable_secondary",			// TF_CLASS_SPY,
			"tf_weapon_throwable_secondary",			// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_parachute",
		{
			"",											// TF_CLASS_UNDEFINED = 0,
			"",			// TF_CLASS_SCOUT,
			"",			// TF_CLASS_SNIPER,
			"tf_weapon_parachute_secondary",			// TF_CLASS_SOLDIER,
			"tf_weapon_parachute_primary",				// TF_CLASS_DEMOMAN,
			"",			// TF_CLASS_MEDIC,
			"",			// TF_CLASS_HEAVYWEAPONS
			"",			// TF_CLASS_PYRO,
			""			// TF_CLASS_SPY,
			"",			// TF_CLASS_ENGINEER,		
		}
	},
	{
		"tf_weapon_revolver",
		{
			"",											// TF_CLASS_UNDEFINED = 0,
			"",			// TF_CLASS_SCOUT,
			"",			// TF_CLASS_SNIPER,
			"",			// TF_CLASS_SOLDIER,
			"",				// TF_CLASS_DEMOMAN,
			"",			// TF_CLASS_MEDIC,
			"",			// TF_CLASS_HEAVYWEAPONS
			"",			// TF_CLASS_PYRO,
			"tf_weapon_revolver",				// TF_CLASS_SPY,
			"tf_weapon_revolver_secondary",		// TF_CLASS_ENGINEER,		
		}
	},
};

//-----------------------------------------------------------------------------
// Purpose: We need to support players putting any shotgun into a shotgun slot, pistol into a pistol slot, etc.
//			For legacy reasons, different classes actually spawn different entities for their shotguns/pistols/etc.
//			To deal with this, we translate entities into the right one for the class we're playing.
//-----------------------------------------------------------------------------
const char *TranslateWeaponEntForClass( const char *pszName, int iClass )
{
	if ( pszName )
	{
		for ( int i = 0; i < ARRAYSIZE(pszWpnEntTranslationList); i++ )
		{
			if ( !Q_stricmp( pszName, pszWpnEntTranslationList[i].pszWpnString ) )
			{
				const char *pTransName = pszWpnEntTranslationList[i].pszClassWpn[ iClass ];
				Assert( pTransName && pTransName[0] );
				return pTransName;
			}
		}
	}

	return pszName;
}

//-----------------------------------------------------------------------------
// Helltower Announcer lines for Redmond and Blutarch
//-----------------------------------------------------------------------------
helltower_vo_t g_pszHelltowerAnnouncerLines[] =
{
// EACH MISC PAIR SHOULD HAVE THE SAME NUMBER OF LINES
	{ "Announcer.Helltower_Red_Misc%02u",				16 },
	{ "Announcer.Helltower_Blue_Misc%02u",				16 },

	{ "Announcer.Helltower_Red_Misc_Rare%02u",			21 },
	{ "Announcer.Helltower_Blue_Misc_Rare%02u",			21 },

// THESE PAIRS CAN HAVE DIFFERENT COUNTS
	{ "Announcer.Helltower_Red_Winning%02u",			12 },
	{ "Announcer.Helltower_Blue_Winning%02u",			13 },

	{ "Announcer.Helltower_Red_Winning_Rare%02u",		12 },
	{ "Announcer.Helltower_Blue_Winning_Rare%02u",		8 },

	{ "Announcer.Helltower_Red_Losing%02u",				15 },
	{ "Announcer.Helltower_Blue_Losing%02u",			16 },

	{ "Announcer.Helltower_Red_Losing_Rare%02u",		6 },
	{ "Announcer.Helltower_Blue_Losing_Rare%02u",		5 },

	{ "Announcer.Helltower_Red_Win%02u",				7 },
	{ "Announcer.Helltower_Blue_Win%02u",				7 },

	{ "Announcer.Helltower_Red_Win_Rare%02u",			1 },
	{ "Announcer.Helltower_Blue_Win_Rare%02u",			3 },

	{ "Announcer.Helltower_Red_Lose%02u",				7 },
	{ "Announcer.Helltower_Blue_Lose%02u",				7 },

	{ "Announcer.Helltower_Red_Lose_Rare%02u",			1 },
	{ "Announcer.Helltower_Blue_Lose_Rare%02u",			1 },

	{ "Announcer.Helltower_Red_RoundStart%02u",			4 },
	{ "Announcer.Helltower_Blue_RoundStart%02u",		2 },

	{ "Announcer.Helltower_Red_RoundStart_Rare%02u",	4 },
	{ "Announcer.Helltower_Blue_RoundStart_Rare%02u",	2 },

	{ "Announcer.Helltower_Red_Skeleton_King%02u",		4 },
	{ "Announcer.Helltower_Blue_Skeleton_King%02u",		4 },

	{ "Announcer.Helltower_Red_Almost_Win%02u",		1 },
	{ "Announcer.Helltower_Blue_Almost_Win%02u",		1 },

	{ "Announcer.Helltower_Red_Almost_Lose%02u",		1 },
	{ "Announcer.Helltower_Blue_Almost_Lose%02u",		1 },

};

#ifdef TF_CLIENT_DLL
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *g_pszInvasionMaps[] =
{
	"maps/ctf_2fort_invasion.bsp",
	"maps/koth_probed.bsp",
	"maps/arena_byre.bsp",
	"maps/pd_watergate.bsp"
};

bool IsPlayingInvasionMap( void )
{
	const char *pszCurrentMap = engine->GetLevelName();

	for ( int i = 0; i < ARRAYSIZE( g_pszInvasionMaps ); i++ )
	{
		if ( FStrEq( g_pszInvasionMaps[i], pszCurrentMap ) )
			return true;
	}

	return false;
}

const char *g_pszClassIcons[SCOREBOARD_CLASS_ICONS] =
{
	"",
	"../hud/leaderboard_class_scout",
	"../hud/leaderboard_class_sniper",
	"../hud/leaderboard_class_soldier",
	"../hud/leaderboard_class_demo",
	"../hud/leaderboard_class_medic",
	"../hud/leaderboard_class_heavy",
	"../hud/leaderboard_class_pyro",
	"../hud/leaderboard_class_spy",
	"../hud/leaderboard_class_engineer",
	"../hud/leaderboard_class_scout_d",
	"../hud/leaderboard_class_sniper_d",
	"../hud/leaderboard_class_soldier_d",
	"../hud/leaderboard_class_demo_d",
	"../hud/leaderboard_class_medic_d",
	"../hud/leaderboard_class_heavy_d",
	"../hud/leaderboard_class_pyro_d",
	"../hud/leaderboard_class_spy_d",
	"../hud/leaderboard_class_engineer_d",
};

const char *g_pszClassIconsAlt[SCOREBOARD_CLASS_ICONS] =
{
	"",
	"class_icons/class_icon_orange_scout",
	"class_icons/class_icon_orange_sniper",
	"class_icons/class_icon_orange_soldier",
	"class_icons/class_icon_orange_demo",
	"class_icons/class_icon_orange_medic",
	"class_icons/class_icon_orange_heavy",
	"class_icons/class_icon_orange_pyro",
	"class_icons/class_icon_orange_spy",
	"class_icons/class_icon_orange_engineer",
	"class_icons/class_icon_orange_scout_d",
	"class_icons/class_icon_orange_sniper_d",
	"class_icons/class_icon_orange_soldier_d",
	"class_icons/class_icon_orange_demo_d",
	"class_icons/class_icon_orange_medic_d",
	"class_icons/class_icon_orange_heavy_d",
	"class_icons/class_icon_orange_pyro_d",
	"class_icons/class_icon_orange_spy_d",
	"class_icons/class_icon_orange_engineer_d",
};

const char *g_pszItemClassImagesRed[] =
{
	"class_portraits/all_class",	// TF_CLASS_UNDEFINED = 0,
	"class_portraits/scout",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper",		// TF_CLASS_SNIPER,
	"class_portraits/soldier",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic",		// TF_CLASS_MEDIC,
	"class_portraits/heavy",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro",			// TF_CLASS_PYRO,
	"class_portraits/spy",			// TF_CLASS_SPY,
	"class_portraits/engineer",		// TF_CLASS_ENGINEER,
	"class_portraits/scout_grey",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper_grey",		// TF_CLASS_SNIPER,
	"class_portraits/soldier_grey",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman_grey",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic_grey",		// TF_CLASS_MEDIC,
	"class_portraits/heavy_grey",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro_grey",		// TF_CLASS_PYRO,
	"class_portraits/spy_grey",			// TF_CLASS_SPY,
	"class_portraits/engineer_grey",	// TF_CLASS_ENGINEER,
};

const char *g_pszItemClassImagesBlue[] =
{
	"class_portraits/all_class",		// TF_CLASS_UNDEFINED = 0,
	"class_portraits/scout_blue",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper_blue",		// TF_CLASS_SNIPER,
	"class_portraits/soldier_blue",		// TF_CLASS_SOLDIER,
	"class_portraits/demoman_blue",		// TF_CLASS_DEMOMAN,
	"class_portraits/medic_blue",		// TF_CLASS_MEDIC,
	"class_portraits/heavy_blue",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro_blue",		// TF_CLASS_PYRO,
	"class_portraits/spy_blue",			// TF_CLASS_SPY,
	"class_portraits/engineer_blue",	// TF_CLASS_ENGINEER,
	"class_portraits/scout_blue_grey",		// TF_CLASS_SCOUT,			
	"class_portraits/sniper_blue_grey",		// TF_CLASS_SNIPER,
	"class_portraits/soldier_blue_grey",	// TF_CLASS_SOLDIER,
	"class_portraits/demoman_blue_grey",	// TF_CLASS_DEMOMAN,
	"class_portraits/medic_blue_grey",		// TF_CLASS_MEDIC,
	"class_portraits/heavy_blue_grey",		// TF_CLASS_HEAVYWEAPONS,
	"class_portraits/pyro_blue_grey",		// TF_CLASS_PYRO,
	"class_portraits/spy_blue_grey",		// TF_CLASS_SPY,
	"class_portraits/engineer_blue_grey",	// TF_CLASS_ENGINEER,
};

const char *g_pszCompetitiveMedalImages[] =
{
	"",
	"competitive/competitive_coin_bronze",
	"competitive/competitive_coin_silver",
	"competitive/competitive_coin_gold",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszCompetitiveMedalImages ) == StatMedal_Max );

#endif // TF_CLIENT_DLL


// rune icons for each team
static const char *s_pszRuneIcons[2][RUNE_TYPES_MAX] =
{
	// RED TEAM
	{
		"powerup_icon_strength_red",
		"powerup_icon_haste_red",
		"powerup_icon_regen_red",
		"powerup_icon_resist_red",
		"powerup_icon_vampire_red",
		"powerup_icon_reflect_red",
		"powerup_icon_precision_red",
		"powerup_icon_agility_red",
		"powerup_icon_knockout_red",
		"powerup_icon_king_red",
		"powerup_icon_plague_red",
		"powerup_icon_supernova_red",
	},
	// BLUE TEAM
	{
		"powerup_icon_strength_blue",
		"powerup_icon_haste_blue",
		"powerup_icon_regen_blue",
		"powerup_icon_resist_blue",
		"powerup_icon_vampire_blue",
		"powerup_icon_reflect_blue",
		"powerup_icon_precision_blue",
		"powerup_icon_agility_blue",
		"powerup_icon_knockout_blue",
		"powerup_icon_king_blue",
		"powerup_icon_plague_blue",
		"powerup_icon_supernova_blue",
	}
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszRuneIcons[0] ) == RUNE_TYPES_MAX );
COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszRuneIcons[1] ) == RUNE_TYPES_MAX );

const char *GetPowerupIconName( RuneTypes_t type, int iTeam )
{
	int iTeamIndex = iTeam == TF_TEAM_RED ? 0 : 1;
	if ( type != RUNE_NONE && type < RUNE_TYPES_MAX )
	{
		return s_pszRuneIcons[ iTeamIndex ][ type ];
	}

	return NULL;
}
