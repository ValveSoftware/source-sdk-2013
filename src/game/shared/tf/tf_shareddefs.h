//========= Copyright Valve Corporation, All rights reserved. ============//
//
// NOTE: Inserting to most or all of the enums in this file will BREAK DEMOS -
// please add to the end instead.
//
//=============================================================================
#ifndef TF_SHAREDDEFS_H
#define TF_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "mp_shareddefs.h"
#include "tf_matchmaking_shared.h"

// Using MAP_DEBUG mode?
#ifdef MAP_DEBUG
	#define MDEBUG(x) x
#else
	#define MDEBUG(x)
#endif

		   
#define	MAX_MVM_WAVE_STRING 256


//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
enum
{
	TF_TEAM_RED = LAST_SHARED_TEAM+1,
	TF_TEAM_BLUE,
	TF_TEAM_COUNT
};

enum
{
	CRATETYPE_NORMAL = 0,
	CRATETYPE_ROBO = 1,
	CRATETYPE_WINTER = 2,
};

#define TF_TEAM_AUTOASSIGN (TF_TEAM_COUNT + 1 )

#define TF_TEAM_HALLOWEEN	TF_TEAM_AUTOASSIGN

#define TF_TEAM_PVE_INVADERS	TF_TEAM_BLUE		// invading bot team in mann vs machine
#define TF_TEAM_PVE_DEFENDERS	TF_TEAM_RED			// defending player team in mann vs machine

#define TF_TEAM_PVE_INVADERS_GIANTS 4				// hack for replacing visuals via itemdef

extern const char *g_aTeamNames[TF_TEAM_COUNT];
extern color32 g_aTeamColors[TF_TEAM_COUNT];

#define COLOR_TF_SPECTATOR	Color( 245, 229, 196, 255 )
#define COLOR_TF_RED		Color( 175, 73, 73, 255 )
#define COLOR_TF_BLUE		Color( 79, 117, 143, 255 )

#define CONTENTS_REDTEAM	CONTENTS_TEAM1
#define CONTENTS_BLUETEAM	CONTENTS_TEAM2

enum 
{
	TF_ARENA_NOTIFICATION_CAREFUL = 0,
	TF_ARENA_NOTIFICATION_SITOUT,
	TF_ARENA_NOTIFICATION_NOPLAYERS,
};

// Team roles
enum 
{
	TEAM_ROLE_NONE = 0,
	TEAM_ROLE_DEFENDERS,
	TEAM_ROLE_ATTACKERS,

	NUM_TEAM_ROLES,
};

enum HalloweenBossType
{
	HALLOWEEN_BOSS_INVALID = 0,
	HALLOWEEN_BOSS_HHH = 1,
	HALLOWEEN_BOSS_MONOCULUS = 2,
	HALLOWEEN_BOSS_MERASMUS = 3,
};

// common utility
inline int GetEnemyTeam( int team )
{
	if ( team == TF_TEAM_RED )
		return TF_TEAM_BLUE;

	if ( team == TF_TEAM_BLUE )
		return TF_TEAM_RED;

	// no enemy team
	return team;
}

// Is this a active in-game team, vs spectator/unassigned/special-values/etc.
inline bool BIsGameTeam( int team )
{
	return team >= FIRST_GAME_TEAM;
}

inline bool BAreTeamsEnemies( int team, int otherTeam )
{
	// Right now all game teams are enemies in TF -- We may want to change this in the future for special mechanics/etc.
	return BIsGameTeam( team ) && BIsGameTeam( otherTeam ) && team != otherTeam;
}

enum PowerupBottleType_t
{
	POWERUP_BOTTLE_NONE,

	POWERUP_BOTTLE_CRITBOOST,
	POWERUP_BOTTLE_UBERCHARGE,
	POWERUP_BOTTLE_RECALL,
	POWERUP_BOTTLE_REFILL_AMMO,
	POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE,
	POWERUP_BOTTLE_RADIUS_STEALTH,

	POWERUP_BOTTLE_TOTAL
};

enum 
{
	MVM_EVENT_POPFILE_NONE = 0,
	MVM_EVENT_POPFILE_HALLOWEEN,

	MVM_EVENT_POPFILE_MAX_TYPES,
};

enum
{
	DRAW_ARROW_UP,
	DRAW_ARROW_LEFT,
	DRAW_ARROW_RIGHT
};

//-----------------------------------------------------------------------------
// CVar replacements
//-----------------------------------------------------------------------------
#define TF_DAMAGE_CRIT_CHANCE				0.02f
#define TF_DAMAGE_CRIT_CHANCE_RAPID			0.02f
#define TF_DAMAGE_CRIT_DURATION_RAPID		2.0f
#define TF_DAMAGE_CRIT_CHANCE_MELEE			0.15f

#define TF_DAMAGE_CRITMOD_MAXTIME			20
#define TF_DAMAGE_CRITMOD_MINTIME			2
#define TF_DAMAGE_CRITMOD_DAMAGE			800
#define TF_DAMAGE_CRITMOD_MAXMULT			6

#define TF_DAMAGE_CRIT_MULTIPLIER			3.0f
#define TF_DAMAGE_MINICRIT_MULTIPLIER		1.35f

//-----------------------------------------------------------------------------
// TF-specific viewport panels
//-----------------------------------------------------------------------------
#define PANEL_CLASS_BLUE		"class_blue"
#define PANEL_CLASS_RED			"class_red"
#define PANEL_MAPINFO			"mapinfo"
#define PANEL_ROUNDINFO			"roundinfo"

#define PANEL_CHARACTER_INFO	"character_info"

#define PANEL_ARENA_WIN		"arenawinpanel"
#define PANEL_ARENA_TEAM	"arenateampanel"

#define PANEL_PVE_WIN		"pvewinpanel"

#define PANEL_GIVEAWAY_ITEM		"giveaway_item"

#define PANEL_MAINMENUOVERRIDE	"MainMenuOverride"

// file we'll save our list of viewed intro movies in
#define MOVIES_FILE				"viewed.res"

#define TF_WEAPON_TAUNT_FRONTIER_JUSTICE_GUITAR_MODEL	"models/player/items/engineer/guitar.mdl"

//-----------------------------------------------------------------------------
// Used in calculating the health percentage of a player
//-----------------------------------------------------------------------------
#define TF_HEALTH_UNDEFINED		1

//-----------------------------------------------------------------------------
// Used to mark a spy's disguise attribute (team or class) as "unused"
//-----------------------------------------------------------------------------
#define TF_SPY_UNDEFINED		TEAM_UNASSIGNED

//-----------------------------------------------------------------------------
// Player Classes.
//-----------------------------------------------------------------------------
#define TF_CLASS_COUNT			( TF_CLASS_COUNT_ALL )

#define TF_FIRST_NORMAL_CLASS	( TF_CLASS_UNDEFINED + 1 )
#define TF_LAST_NORMAL_CLASS	( TF_CLASS_CIVILIAN )

#define	TF_CLASS_MENU_BUTTONS	( TF_CLASS_RANDOM + 1 )

enum ETFClass
{
	TF_CLASS_UNDEFINED = 0,

	TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
    TF_CLASS_SNIPER,
    TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,		

	// Add any new classes after Engineer
	TF_CLASS_CIVILIAN,		// TF_LAST_NORMAL_CLASS
	TF_CLASS_COUNT_ALL,

	TF_CLASS_RANDOM
};

inline bool IsValidTFPlayerClass( int iClass ) { return iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS; }
inline bool IsValidTFTeam( int iTeam ) { return iTeam == TF_TEAM_RED || iTeam == TF_TEAM_BLUE; }

#define FOR_EACH_NORMAL_PLAYER_CLASS( _i ) for ( int _i = TF_FIRST_NORMAL_CLASS; _i < TF_LAST_NORMAL_CLASS; _i++ )

extern const char *g_aPlayerClassNames[TF_CLASS_MENU_BUTTONS];				// localization keys
extern const char *g_aPlayerClassNames_NonLocalized[TF_CLASS_MENU_BUTTONS];	// non-localized class names
extern const char *g_aRawPlayerClassNamesShort[TF_CLASS_MENU_BUTTONS];		// raw class names, useful for formatting resource/material filenames - "heavy" instead of "heavyweapons" and "demo" instead of "demoman"
extern const char *g_aRawPlayerClassNames[TF_CLASS_MENU_BUTTONS];			// raw class names, useful for formatting resource/material filenames

extern const char g_szPlayerRobotModels[TF_LAST_NORMAL_CLASS][ MAX_PATH ];

extern const char g_szBotModels[TF_LAST_NORMAL_CLASS][ MAX_PATH ];
extern const char g_szBotBossModels[TF_LAST_NORMAL_CLASS][ MAX_PATH ];
extern const char g_szBotBossSentryBusterModel[ MAX_PATH ];

extern const char g_szRomePromoItems_Hat[TF_LAST_NORMAL_CLASS][ MAX_PATH ];
extern const char g_szRomePromoItems_Misc[TF_LAST_NORMAL_CLASS][ MAX_PATH ];

int GetClassIndexFromString( const char *pClassName, int nLastClassIndex = TF_LAST_NORMAL_CLASS );

// menu buttons are not in the same order as the defines
extern int iRemapIndexToClass[TF_CLASS_MENU_BUTTONS];
int GetRemappedMenuIndexForClass( int iClass );

extern const char *g_pszBreadModels[];

//-----------------------------------------------------------------------------
// For entity_capture_flags to use when placed in the world
// NOTE: Inserting to most or all of the enums in this file will BREAK DEMOS -
// please add to the end instead.
//-----------------------------------------------------------------------------
enum ETFFlagType
{
	TF_FLAGTYPE_CTF = 0,
	TF_FLAGTYPE_ATTACK_DEFEND,
	TF_FLAGTYPE_TERRITORY_CONTROL,
	TF_FLAGTYPE_INVADE,
	TF_FLAGTYPE_RESOURCE_CONTROL,
	TF_FLAGTYPE_ROBOT_DESTRUCTION,
	TF_FLAGTYPE_PLAYER_DESTRUCTION

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

//-----------------------------------------------------------------------------
// For the game rules to determine which type of game we're playing
//
// NOTE: Inserting to most or all of the enums in this file will BREAK DEMOS -
// please add to the end instead.
//-----------------------------------------------------------------------------
enum ETFGameType
{
	TF_GAMETYPE_UNDEFINED = 0,
	TF_GAMETYPE_CTF,
	TF_GAMETYPE_CP,
	TF_GAMETYPE_ESCORT,
	TF_GAMETYPE_ARENA,
	TF_GAMETYPE_MVM,
	TF_GAMETYPE_RD,
	TF_GAMETYPE_PASSTIME,
	TF_GAMETYPE_PD,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
	TF_GAMETYPE_COUNT
};
const char *GetGameTypeName( ETFGameType gameType );
const char *GetEnumGameTypeName( ETFGameType gameType );
ETFGameType GetGameTypeFromName( const char *pszGameType );

//=============================================================================
// HPE_BEGIN:
// [msmith]	Added a HUD type so that we can have the hud independent from the
//			game type.  This is useful in training where we want a training hud
//			Instead of the other types of HUD.
//=============================================================================
enum
{
	TF_HUDTYPE_UNDEFINED = 0,
	TF_HUDTYPE_CTF,
	TF_HUDTYPE_CP,
	TF_HUDTYPE_ESCORT,
	TF_HUDTYPE_ARENA,
	TF_HUDTYPE_TRAINING,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};
//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Buildings.
//-----------------------------------------------------------------------------
enum
{
	TF_BUILDING_SENTRY				= (1<<0),
	TF_BUILDING_DISPENSER			= (1<<1),
	TF_BUILDING_TELEPORT			= (1<<2),

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

//-----------------------------------------------------------------------------
// Items.
//-----------------------------------------------------------------------------
enum
{
	TF_ITEM_UNDEFINED		= 0,
	TF_ITEM_CAPTURE_FLAG	= (1<<0),
	TF_ITEM_HEALTH_KIT		= (1<<1),
	TF_ITEM_ARMOR			= (1<<2),
	TF_ITEM_AMMO_PACK		= (1<<3),
	TF_ITEM_GRENADE_PACK	= (1<<4),

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
enum ETFAmmoType
{
	TF_AMMO_DUMMY = 0,	// Dummy index to make the CAmmoDef indices correct for the other ammo types.
	TF_AMMO_PRIMARY,
	TF_AMMO_SECONDARY,
	TF_AMMO_METAL,
	TF_AMMO_GRENADES1,
	TF_AMMO_GRENADES2,
	TF_AMMO_GRENADES3,	// Utility Slot Grenades
	TF_AMMO_COUNT,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};
const char *GetAmmoName( int iAmmoType );

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
#define TF_PLAYER_WEAPON_COUNT		6
#define TF_PLAYER_GRENADE_COUNT		2
#define TF_PLAYER_BUILDABLE_COUNT	3
#define TF_PLAYER_BLUEPRINT_COUNT	6

#define TF_WEAPON_PRIMARY_MODE		0
#define TF_WEAPON_SECONDARY_MODE	1

#define TF_WEAPON_GRENADE_FRICTION						0.6f
#define TF_WEAPON_GRENADE_GRAVITY						0.81f
#define TF_WEAPON_GRENADE_INITPRIME						0.8f
#define TF_WEAPON_GRENADE_CONCUSSION_TIME				15.0f
#define TF_WEAPON_GRENADE_MIRV_BOMB_COUNT				4
#define TF_WEAPON_GRENADE_CALTROP_TIME					8.0f

#define TF_WEAPON_PIPEBOMB_WORLD_COUNT					15
#define TF_WEAPON_PIPEBOMB_COUNT						8
#define TF_WEAPON_PIPEBOMB_INTERVAL						0.6f

#define TF_WEAPON_ROCKET_INTERVAL						0.8f

#define TF_WEAPON_FLAMETHROWER_INTERVAL					0.15f
#define TF_WEAPON_FLAMETHROWER_ROCKET_INTERVAL			0.8f

#define TF_WEAPON_ZOOM_FOV								20

//
// NOTE: Inserting to most or all of the enums in this file will BREAK DEMOS -
// please add to the end instead.
//
enum ETFWeaponType
{
	TF_WEAPON_NONE = 0,
	TF_WEAPON_BAT,
	TF_WEAPON_BAT_WOOD,
	TF_WEAPON_BOTTLE, 
	TF_WEAPON_FIREAXE,
	TF_WEAPON_CLUB,
	TF_WEAPON_CROWBAR,
	TF_WEAPON_KNIFE,
	TF_WEAPON_FISTS,
	TF_WEAPON_SHOVEL,
	TF_WEAPON_WRENCH,
	TF_WEAPON_BONESAW,
	TF_WEAPON_SHOTGUN_PRIMARY,
	TF_WEAPON_SHOTGUN_SOLDIER,
	TF_WEAPON_SHOTGUN_HWG,
	TF_WEAPON_SHOTGUN_PYRO,
	TF_WEAPON_SCATTERGUN,
	TF_WEAPON_SNIPERRIFLE,
	TF_WEAPON_MINIGUN,
	TF_WEAPON_SMG,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_TRANQ,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_FLAMETHROWER,
	TF_WEAPON_GRENADE_NORMAL,
	TF_WEAPON_GRENADE_CONCUSSION,
	TF_WEAPON_GRENADE_NAIL,
	TF_WEAPON_GRENADE_MIRV,
	TF_WEAPON_GRENADE_MIRV_DEMOMAN,
	TF_WEAPON_GRENADE_NAPALM,
	TF_WEAPON_GRENADE_GAS,
	TF_WEAPON_GRENADE_EMP,
	TF_WEAPON_GRENADE_CALTROP,
	TF_WEAPON_GRENADE_PIPEBOMB,
	TF_WEAPON_GRENADE_SMOKE_BOMB,
	TF_WEAPON_GRENADE_HEAL,
	TF_WEAPON_GRENADE_STUNBALL,
	TF_WEAPON_GRENADE_JAR,
	TF_WEAPON_GRENADE_JAR_MILK,
	TF_WEAPON_PISTOL,
	TF_WEAPON_PISTOL_SCOUT,
	TF_WEAPON_REVOLVER,
	TF_WEAPON_NAILGUN,
	TF_WEAPON_PDA,
	TF_WEAPON_PDA_ENGINEER_BUILD,
	TF_WEAPON_PDA_ENGINEER_DESTROY,
	TF_WEAPON_PDA_SPY,
	TF_WEAPON_BUILDER,
	TF_WEAPON_MEDIGUN,
	TF_WEAPON_GRENADE_MIRVBOMB,
	TF_WEAPON_FLAMETHROWER_ROCKET,
	TF_WEAPON_GRENADE_DEMOMAN,
	TF_WEAPON_SENTRY_BULLET,
	TF_WEAPON_SENTRY_ROCKET,
	TF_WEAPON_DISPENSER,
	TF_WEAPON_INVIS,
	TF_WEAPON_FLAREGUN,
	TF_WEAPON_LUNCHBOX,
	TF_WEAPON_JAR,
	TF_WEAPON_COMPOUND_BOW,
	TF_WEAPON_BUFF_ITEM,
	TF_WEAPON_PUMPKIN_BOMB,
	TF_WEAPON_SWORD, 
	TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT,
	TF_WEAPON_LIFELINE,
	TF_WEAPON_LASER_POINTER,
	TF_WEAPON_DISPENSER_GUN,
	TF_WEAPON_SENTRY_REVENGE,
	TF_WEAPON_JAR_MILK,
	TF_WEAPON_HANDGUN_SCOUT_PRIMARY,
	TF_WEAPON_BAT_FISH,
	TF_WEAPON_CROSSBOW,
	TF_WEAPON_STICKBOMB,
	TF_WEAPON_HANDGUN_SCOUT_SECONDARY,
	TF_WEAPON_SODA_POPPER,
	TF_WEAPON_SNIPERRIFLE_DECAP,
	TF_WEAPON_RAYGUN,
	TF_WEAPON_PARTICLE_CANNON,
	TF_WEAPON_MECHANICAL_ARM,
	TF_WEAPON_DRG_POMSON,
	TF_WEAPON_BAT_GIFTWRAP,
	TF_WEAPON_GRENADE_ORNAMENT_BALL,
	TF_WEAPON_FLAREGUN_REVENGE,
	TF_WEAPON_PEP_BRAWLER_BLASTER,
	TF_WEAPON_CLEAVER,
	TF_WEAPON_GRENADE_CLEAVER,
	TF_WEAPON_STICKY_BALL_LAUNCHER,
	TF_WEAPON_GRENADE_STICKY_BALL,
	TF_WEAPON_SHOTGUN_BUILDING_RESCUE,
	TF_WEAPON_CANNON,
	TF_WEAPON_THROWABLE,
	TF_WEAPON_GRENADE_THROWABLE,
	TF_WEAPON_PDA_SPY_BUILD,
	TF_WEAPON_GRENADE_WATERBALLOON,
	TF_WEAPON_HARVESTER_SAW,
	TF_WEAPON_SPELLBOOK,
	TF_WEAPON_SPELLBOOK_PROJECTILE,
	TF_WEAPON_SNIPERRIFLE_CLASSIC,
	TF_WEAPON_PARACHUTE,
	TF_WEAPON_GRAPPLINGHOOK,
	TF_WEAPON_PASSTIME_GUN,
	TF_WEAPON_CHARGED_SMG,
	TF_WEAPON_BREAKABLE_SIGN,
	TF_WEAPON_ROCKETPACK,
	TF_WEAPON_SLAP,
	TF_WEAPON_JAR_GAS,
	TF_WEAPON_GRENADE_JAR_GAS,
	TF_WEAPON_FLAME_BALL,


	//
	// ADD NEW WEAPONS HERE TO AVOID BREAKING DEMOS
	//

	TF_WEAPON_COUNT
};

extern int g_aWeaponDamageTypes[TF_WEAPON_COUNT];

const char *GetWeaponIDName( int iWeaponID );
int GetWeaponId( const char *pszWeaponName );
#ifdef GAME_DLL
int GetWeaponFromDamage( const CTakeDamageInfo &info );
#endif
int GetBuildableId( const char *pszBuildableName );
const char *WeaponIdToAlias( int iWeapon );

// Only TF specific classes that derives from CBaseProjectile should OVERRIDE GetBaseProjectileType()
enum BaseProjectileType_t
{
	TF_BASE_PROJECTILE_GRENADE, // CTFWeaponBaseGrenadeProj

	// add new entries here!
};

// Item definitions look into this array by hard-coded indices, so do not add entries
// anywhere except the end!
enum ProjectileType_t
{
	TF_PROJECTILE_NONE,
	TF_PROJECTILE_BULLET,
	TF_PROJECTILE_ROCKET,
	TF_PROJECTILE_PIPEBOMB,
	TF_PROJECTILE_PIPEBOMB_REMOTE,
	TF_PROJECTILE_SYRINGE,
	TF_PROJECTILE_FLARE,
	TF_PROJECTILE_JAR,
	TF_PROJECTILE_ARROW,
	TF_PROJECTILE_FLAME_ROCKET,
	TF_PROJECTILE_JAR_MILK,
	TF_PROJECTILE_HEALING_BOLT,
	TF_PROJECTILE_ENERGY_BALL,
	TF_PROJECTILE_ENERGY_RING,
	TF_PROJECTILE_PIPEBOMB_PRACTICE,
	TF_PROJECTILE_CLEAVER,
	TF_PROJECTILE_STICKY_BALL,
	TF_PROJECTILE_CANNONBALL,
	TF_PROJECTILE_BUILDING_REPAIR_BOLT,
	TF_PROJECTILE_FESTIVE_ARROW,
	TF_PROJECTILE_THROWABLE,
	TF_PROJECTILE_SPELL,
	TF_PROJECTILE_FESTIVE_JAR,
	TF_PROJECTILE_FESTIVE_HEALING_BOLT,
	TF_PROJECTILE_BREADMONSTER_JARATE,
	TF_PROJECTILE_BREADMONSTER_MADMILK,
	TF_PROJECTILE_GRAPPLINGHOOK,
	TF_PROJECTILE_SENTRY_ROCKET,
	TF_PROJECTILE_BREAD_MONSTER,
	TF_PROJECTILE_JAR_GAS,
	TF_PROJECTILE_FLAME_BALL,	

	// Add new entries here!

	TF_NUM_PROJECTILES
};

extern const char *g_szProjectileNames[];

enum medigun_charge_types
{
	MEDIGUN_CHARGE_INVALID = -1,
	MEDIGUN_CHARGE_INVULN = 0,
	MEDIGUN_CHARGE_CRITICALBOOST,
	MEDIGUN_CHARGE_MEGAHEAL,
	MEDIGUN_CHARGE_BULLET_RESIST,
	MEDIGUN_CHARGE_BLAST_RESIST,
	MEDIGUN_CHARGE_FIRE_RESIST,

	MEDIGUN_NUM_CHARGE_TYPES,
};

enum arrow_models
{
	MODEL_ARROW_REGULAR,
	MODEL_ARROW_BUILDING_REPAIR,
	MODEL_SNOWBALL,
	MODEL_FESTIVE_ARROW_REGULAR,
	MODEL_SYRINGE,
	MODEL_FESTIVE_HEALING_BOLT,
	MODEL_BREAD_MONSTER,
	MODEL_GRAPPLINGHOOK,

	// Staging
	MODEL_THROWING_KNIFE,

	TF_ARROW_MODEL_COUNT
};

extern const char *g_pszArrowModels[];

//-----------------------------------------------------------------------------
// Dead Calling Cards
//-----------------------------------------------------------------------------
enum death_calling_card_models
{
	CALLING_CARD_NONE = 0,			// Empty at zero
	CALLING_CARD_MILKMAN = 1,	// Scout PolyCount Set
	CALLING_CARD_CROC,			// Sniper PolyCount Set
	CALLING_CARD_TANKBUSTER,	// Solider PolyCount Set
	CALLING_CARD_GASJOCKEY,		// Pyro PolyCount Set 

	TF_CALLING_CARD_MODEL_COUNT
};
extern const char *g_pszDeathCallingCardModels[TF_CALLING_CARD_MODEL_COUNT];

#ifdef TF_CLIENT_DLL
extern const char *g_pszInvasionMaps[];
bool IsPlayingInvasionMap( void );

#define SCOREBOARD_DOMINATION_ICONS		17
#define SCOREBOARD_CLASS_ICONS			19
#define SCOREBOARD_PING_ICONS			6

extern const char *g_pszClassIcons[];

extern const char *g_pszClassIconsAlt[];

extern const char *g_pszCompetitiveMedalImages[];

extern const char *g_pszItemClassImagesRed[];

extern const char *g_pszItemClassImagesBlue[];

#endif // TF_CLIENT_DLL

//-----------------------------------------------------------------------------
// Attributes.
//-----------------------------------------------------------------------------
#define TF_PLAYER_VIEW_OFFSET	Vector( 0, 0, 64.0 ) //--> see GetViewVectors()

//-----------------------------------------------------------------------------
// TF Player Condition.
//-----------------------------------------------------------------------------

// Burning
#define TF_BURNING_FREQUENCY		0.5f
#define TF_BURNING_FLAME_LIFE		10.0
#define TF_BURNING_FLAME_LIFE_PYRO	0.25
#define TF_BURNING_DMG				4

// Bleeding
#define TF_BLEEDING_FREQUENCY		0.5f
#define TF_BLEEDING_DMG				4

// disguising
#define TF_TIME_TO_DISGUISE			2.0f
#define TF_TIME_TO_QUICK_DISGUISE	0.5f
#define TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT 5.0

// phasing

#define TF_SCOUT_NUMBEROFPHASEATTACHMENTS	5

#define SHOW_DISGUISE_EFFECT 1

//
// ADD NEW WEAPONS HERE TO AVOID BREAKING DEMOS
//
enum ETFCond
{
	TF_COND_INVALID                          = -1,
	TF_COND_AIMING                           = 0, // Sniper aiming, Heavy minigun.
	TF_COND_ZOOMED                           = 1,
	TF_COND_DISGUISING                       = 2,
	TF_COND_DISGUISED                        = 3,
	TF_COND_STEALTHED                        = 4, // Spy specific
	TF_COND_INVULNERABLE                     = 5,
	TF_COND_TELEPORTED                       = 6,
	TF_COND_TAUNTING                         = 7,
	TF_COND_INVULNERABLE_WEARINGOFF          = 8,
	TF_COND_STEALTHED_BLINK                  = 9,
	TF_COND_SELECTED_TO_TELEPORT             = 10,
	TF_COND_CRITBOOSTED                      = 11, // DO NOT RE-USE THIS -- THIS IS FOR KRITZKRIEG AND REVENGE CRITS ONLY
	TF_COND_TMPDAMAGEBONUS                   = 12,
	TF_COND_FEIGN_DEATH                      = 13,
	TF_COND_PHASE                            = 14,
	TF_COND_STUNNED                          = 15, // Any type of stun. Check iStunFlags for more info.
	TF_COND_OFFENSEBUFF                      = 16,
	TF_COND_SHIELD_CHARGE                    = 17,
	TF_COND_DEMO_BUFF                        = 18,
	TF_COND_ENERGY_BUFF                      = 19,
	TF_COND_RADIUSHEAL                       = 20,
	TF_COND_HEALTH_BUFF                      = 21,
	TF_COND_BURNING                          = 22,
	TF_COND_HEALTH_OVERHEALED                = 23,
	TF_COND_URINE                            = 24,
	TF_COND_BLEEDING                         = 25,
	TF_COND_DEFENSEBUFF                      = 26, // 35% defense! No crit damage.
	TF_COND_MAD_MILK                         = 27,
	TF_COND_MEGAHEAL                         = 28,
	TF_COND_REGENONDAMAGEBUFF                = 29,
	TF_COND_MARKEDFORDEATH                   = 30,
	TF_COND_NOHEALINGDAMAGEBUFF              = 31,
	TF_COND_SPEED_BOOST                      = 32, // = 32
	TF_COND_CRITBOOSTED_PUMPKIN              = 33, // Brandon hates bits
	TF_COND_CRITBOOSTED_USER_BUFF            = 34,
	TF_COND_CRITBOOSTED_DEMO_CHARGE          = 35,
	TF_COND_SODAPOPPER_HYPE                  = 36,
	TF_COND_CRITBOOSTED_FIRST_BLOOD          = 37, // arena mode first blood
	TF_COND_CRITBOOSTED_BONUS_TIME           = 38,
	TF_COND_CRITBOOSTED_CTF_CAPTURE          = 39,
	TF_COND_CRITBOOSTED_ON_KILL              = 40, // =40. KGB, etc.
	TF_COND_CANNOT_SWITCH_FROM_MELEE         = 41,
	TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK        = 42, // 35% defense! Still damaged by crits.
	TF_COND_REPROGRAMMED                     = 43, // Bots only
	TF_COND_CRITBOOSTED_RAGE_BUFF            = 44,
	TF_COND_DEFENSEBUFF_HIGH                 = 45, // 75% defense! Still damaged by crits.
	TF_COND_SNIPERCHARGE_RAGE_BUFF           = 46, // Sniper Rage - Charge time speed up
	TF_COND_DISGUISE_WEARINGOFF              = 47, // Applied for half-second post-disguise
	TF_COND_MARKEDFORDEATH_SILENT            = 48, // Sans sound
	TF_COND_DISGUISED_AS_DISPENSER           = 49,
	TF_COND_SAPPED                           = 50, // =50. Bots only
	TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED = 51,
	TF_COND_INVULNERABLE_USER_BUFF           = 52,
	TF_COND_HALLOWEEN_BOMB_HEAD              = 53,
	TF_COND_HALLOWEEN_THRILLER               = 54,
	TF_COND_RADIUSHEAL_ON_DAMAGE             = 55,
	TF_COND_CRITBOOSTED_CARD_EFFECT          = 56,
	TF_COND_INVULNERABLE_CARD_EFFECT         = 57,
	TF_COND_MEDIGUN_UBER_BULLET_RESIST       = 58,
	TF_COND_MEDIGUN_UBER_BLAST_RESIST        = 59,
	TF_COND_MEDIGUN_UBER_FIRE_RESIST         = 60, // =60
	TF_COND_MEDIGUN_SMALL_BULLET_RESIST      = 61,
	TF_COND_MEDIGUN_SMALL_BLAST_RESIST       = 62,
	TF_COND_MEDIGUN_SMALL_FIRE_RESIST        = 63,
	TF_COND_STEALTHED_USER_BUFF              = 64, // Any class can have this
	TF_COND_MEDIGUN_DEBUFF                   = 65,
	TF_COND_STEALTHED_USER_BUFF_FADING       = 66,
	TF_COND_BULLET_IMMUNE                    = 67,
	TF_COND_BLAST_IMMUNE                     = 68,
	TF_COND_FIRE_IMMUNE                      = 69,
	TF_COND_PREVENT_DEATH                    = 70, // =70
	TF_COND_MVM_BOT_STUN_RADIOWAVE           = 71, // Bots only
	TF_COND_HALLOWEEN_SPEED_BOOST            = 72,
	TF_COND_HALLOWEEN_QUICK_HEAL             = 73,
	TF_COND_HALLOWEEN_GIANT                  = 74,
	TF_COND_HALLOWEEN_TINY                   = 75,
	TF_COND_HALLOWEEN_IN_HELL                = 76,
	TF_COND_HALLOWEEN_GHOST_MODE             = 77, // =77
	TF_COND_MINICRITBOOSTED_ON_KILL          = 78,
	TF_COND_OBSCURED_SMOKE                   = 79,
	TF_COND_PARACHUTE_ACTIVE                 = 80, // actively being used (not retracted)
	TF_COND_BLASTJUMPING                     = 81,
	TF_COND_HALLOWEEN_KART                   = 82,
	TF_COND_HALLOWEEN_KART_DASH              = 83,
	TF_COND_BALLOON_HEAD                     = 84, // =84 larger head, lower-gravity-feeling jumps
	TF_COND_MELEE_ONLY                       = 85, // =85 melee only
	TF_COND_SWIMMING_CURSE                   = 86, // player movement become swimming movement
	TF_COND_FREEZE_INPUT                     = 87, // freezes player input
	TF_COND_HALLOWEEN_KART_CAGE              = 88, // attach cage model to player while in kart
	TF_COND_DONOTUSE_0                       = 89,
	TF_COND_RUNE_STRENGTH                    = 90,
	TF_COND_RUNE_HASTE                       = 91,
	TF_COND_RUNE_REGEN                       = 92,
	TF_COND_RUNE_RESIST                      = 93,
	TF_COND_RUNE_VAMPIRE                     = 94,
	TF_COND_RUNE_REFLECT                     = 95,
	TF_COND_RUNE_PRECISION                   = 96,
	TF_COND_RUNE_AGILITY                     = 97,
	TF_COND_GRAPPLINGHOOK                    = 98,
	TF_COND_GRAPPLINGHOOK_SAFEFALL           = 99,
	TF_COND_GRAPPLINGHOOK_LATCHED            = 100,
	TF_COND_GRAPPLINGHOOK_BLEEDING           = 101,
	TF_COND_AFTERBURN_IMMUNE                 = 102,
	TF_COND_RUNE_KNOCKOUT                    = 103,
	TF_COND_RUNE_IMBALANCE                   = 104,
	TF_COND_CRITBOOSTED_RUNE_TEMP            = 105,
	TF_COND_PASSTIME_INTERCEPTION            = 106,
	TF_COND_SWIMMING_NO_EFFECTS              = 107, // =107_DNOC_FT
	TF_COND_PURGATORY                        = 108,
	TF_COND_RUNE_KING                        = 109,
	TF_COND_RUNE_PLAGUE                      = 110,
	TF_COND_RUNE_SUPERNOVA                   = 111,
	TF_COND_PLAGUE                           = 112,
	TF_COND_KING_BUFFED                      = 113,
	TF_COND_TEAM_GLOWS                       = 114, // used to show team glows to living players
	TF_COND_KNOCKED_INTO_AIR                 = 115,
	TF_COND_COMPETITIVE_WINNER               = 116,
	TF_COND_COMPETITIVE_LOSER                = 117,
	TF_COND_HEALING_DEBUFF                   = 118,
	TF_COND_PASSTIME_PENALTY_DEBUFF          = 119, // when carrying the ball without any teammates nearby
	TF_COND_GRAPPLED_TO_PLAYER               = 120,
	TF_COND_GRAPPLED_BY_PLAYER               = 121,
	TF_COND_PARACHUTE_DEPLOYED               = 122, // activated at least once while player's been airborne, but not does mean it's active now (see TF_COND_PARACHUTE_ACTIVE)
	TF_COND_GAS                              = 123,
	TF_COND_BURNING_PYRO                     = 124,
	TF_COND_ROCKETPACK                       = 125,
	// Players who lose their footing have lessened friction and don't re-stick to the ground unless they're below a
	// tf_movement_lost_footing_restick speed
	TF_COND_LOST_FOOTING                     = 126,
	// When in the air, slide up/along surfaces with momentum as if caught up in a... blast of air of some sort.
	// Reduces air control as well.  See tf_movement_aircurrent convars.  Removed upon touching ground.
	TF_COND_AIR_CURRENT                      = 127,
	TF_COND_HALLOWEEN_HELL_HEAL              = 128,
	TF_COND_POWERUPMODE_DOMINANT			 = 129,
	TF_COND_IMMUNE_TO_PUSHBACK				 = 130,
		//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	// ******** Keep this block last! ********
	// Keep experimental conditions below and graduate out of it before shipping

	TF_COND_LAST
};

const char *GetTFConditionName( ETFCond eCond );
ETFCond GetTFConditionFromName( const char *pszCondName );
inline ETFCond TFCondIndexToEnum( int nCond )
{
	if ( nCond >= TF_COND_AIMING && nCond < TF_COND_LAST )
		return (ETFCond) nCond;

	return TF_COND_INVALID;
}

// If you want your condition to expire faster under healing,
// add it to this function in tf_shareddefs.cpp
bool ConditionExpiresFast( ETFCond eCond );

// Some attributes specify conditions to be or'd. The problem there is that if we add conditions
// to the above list, they get hosed. So we maintain this separate list as a translation table.
// When you add conditions to the above list, add them TO THE BOTTOM of this list.
extern ETFCond condition_to_attribute_translation[];

extern ETFCond g_aDebuffConditions[];
//-----------------------------------------------------------------------------
// TF Player State.
//-----------------------------------------------------------------------------
enum 
{
	TF_STATE_ACTIVE = 0,		// Happily running around in the game.
	TF_STATE_WELCOME,			// First entering the server (shows level intro screen).
	TF_STATE_OBSERVER,			// Game observer mode.
	TF_STATE_DYING,				// Player is dying.
	TF_STATE_COUNT

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

//-----------------------------------------------------------------------------
// TF FlagInfo State.
//-----------------------------------------------------------------------------
#define TF_FLAGINFO_HOME		0
#define TF_FLAGINFO_STOLEN		(1<<0)
#define TF_FLAGINFO_DROPPED		(1<<1)

enum ETFFlagEventTypes
{
	TF_FLAGEVENT_PICKUP = 1,
	TF_FLAGEVENT_CAPTURE,
	TF_FLAGEVENT_DEFEND,
	TF_FLAGEVENT_DROPPED,
	TF_FLAGEVENT_RETURNED,

	
	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
	TF_NUM_FLAG_EVENTS
};

const char *GetCTFEventName( ETFFlagEventTypes iEventType );
ETFFlagEventTypes GetCTFEventTypeFromName( const char *pszName );

//-----------------------------------------------------------------------------
// TF Robot Destruction Score Methods
//-----------------------------------------------------------------------------
enum RDScoreMethod_t
{
	SCORE_UNDEFINED = -1,
	SCORE_REACTOR_CAPTURED,
	SCORE_CORES_COLLECTED,
	SCORE_REACTOR_RETURNED,
	SCORE_REACTOR_STEAL,
		
	NUM_SCORE_TYPES
};

const char *GetRDScoreMethodName( RDScoreMethod_t iScoreMethod );
RDScoreMethod_t GetRDScoreMethodFromName( const char *pszName );

//-----------------------------------------------------------------------------
// Class data
//-----------------------------------------------------------------------------
#define TF_REGEN_TIME			1.0			// Number of seconds between each regen.
#define TF_REGEN_AMOUNT			3 			// Amount of health regenerated each regen.
#define TF_REGEN_TIME_RUNE		0.25		// Number of seconds between each regen generated by a powerup.

//-----------------------------------------------------------------------------
// Assist-damage constants
//-----------------------------------------------------------------------------
#define TF_TIME_ASSIST_KILL				3.0f	// Time window for a recent damager to get credit for an assist for a kill
#define TF_TIME_SUICIDE_KILL_CREDIT		10.0f	// Time window for a recent damager to get credit for a kill if target suicides

//-----------------------------------------------------------------------------
// Domination/nemesis constants
//-----------------------------------------------------------------------------
#define TF_KILLS_DOMINATION				4		// # of unanswered kills to dominate another player

//-----------------------------------------------------------------------------
// Taunt attacks
//-----------------------------------------------------------------------------
enum taunt_attack_t
{
	TAUNTATK_NONE = 0,
	TAUNTATK_PYRO_HADOUKEN,
	TAUNTATK_HEAVY_EAT,
	TAUNTATK_HEAVY_RADIAL_BUFF,
	TAUNTATK_HEAVY_HIGH_NOON,
	TAUNTATK_SCOUT_DRINK,
	TAUNTATK_SCOUT_GRAND_SLAM,
	TAUNTATK_MEDIC_INHALE,
	TAUNTATK_SPY_FENCING_SLASH_A,
	TAUNTATK_SPY_FENCING_SLASH_B,
	TAUNTATK_SPY_FENCING_STAB,
	TAUNTATK_RPS_KILL,
	TAUNTATK_SNIPER_ARROW_STAB_IMPALE,
	TAUNTATK_SNIPER_ARROW_STAB_KILL,
	TAUNTATK_SOLDIER_GRENADE_KILL,
	TAUNTATK_DEMOMAN_BARBARIAN_SWING,
	TAUNTATK_MEDIC_UBERSLICE_IMPALE,
	TAUNTATK_MEDIC_UBERSLICE_KILL,
	TAUNTATK_FLIP_LAND_PARTICLE,
	TAUNTATK_RPS_PARTICLE,
	TAUNTATK_HIGHFIVE_PARTICLE,
	TAUNTATK_ENGINEER_GUITAR_SMASH,
	TAUNTATK_ENGINEER_ARM_IMPALE,
	TAUNTATK_ENGINEER_ARM_KILL,
	TAUNTATK_ENGINEER_ARM_BLEND,
	TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN,
	TAUNTATK_SHOW_ITEM,
	TAUNTATK_MEDIC_RELEASE_DOVES,
	TAUNTATK_PYRO_ARMAGEDDON,
	TAUNTATK_PYRO_SCORCHSHOT,
	TAUNTATK_ALLCLASS_GUITAR_RIFF,
	TAUNTATK_MEDIC_HEROIC_TAUNT,
	TAUNTATK_PYRO_GASBLAST,
	TAUNTATK_ENGINEER_TRICKSHOT,

	//
	// INSERT NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	TAUNTATK_COUNT
};

taunt_attack_t GetTauntAttackByName( const char* pszTauntAttackName );

//-----------------------------------------------------------------------------
// TF Hints
//-----------------------------------------------------------------------------
enum
{
	HINT_FRIEND_SEEN = 0,				// #Hint_spotted_a_friend
	HINT_ENEMY_SEEN,					// #Hint_spotted_an_enemy
	HINT_ENEMY_KILLED,					// #Hint_killing_enemies_is_good
	HINT_AMMO_EXHAUSTED,				// #Hint_out_of_ammo
	HINT_TURN_OFF_HINTS,				// #Hint_turn_off_hints
	HINT_PICKUP_AMMO,					// #Hint_pickup_ammo
	HINT_CANNOT_TELE_WITH_FLAG,			// #Hint_Cannot_Teleport_With_Flag
	HINT_CANNOT_CLOAK_WITH_FLAG,		// #Hint_Cannot_Cloak_With_Flag
	HINT_CANNOT_DISGUISE_WITH_FLAG,		// #Hint_Cannot_Disguise_With_Flag
	HINT_CANNOT_ATTACK_WHILE_CLOAKED,	// #Hint_Cannot_Attack_While_Cloaked
	HINT_CLASSMENU,						// #Hint_ClassMenu

	// Grenades
	HINT_GREN_CALTROPS,					// #Hint_gren_caltrops
	HINT_GREN_CONCUSSION,				// #Hint_gren_concussion
	HINT_GREN_EMP,						// #Hint_gren_emp
	HINT_GREN_GAS,						// #Hint_gren_gas
	HINT_GREN_MIRV,						// #Hint_gren_mirv
	HINT_GREN_NAIL,						// #Hint_gren_nail
	HINT_GREN_NAPALM,					// #Hint_gren_napalm
	HINT_GREN_NORMAL,					// #Hint_gren_normal

	// Weapon alt-fires
	HINT_ALTFIRE_SNIPERRIFLE,			// #Hint_altfire_sniperrifle
	HINT_ALTFIRE_FLAMETHROWER,			// #Hint_altfire_flamethrower
	HINT_ALTFIRE_GRENADELAUNCHER,		// #Hint_altfire_grenadelauncher
	HINT_ALTFIRE_PIPEBOMBLAUNCHER,		// #Hint_altfire_pipebomblauncher
	HINT_ALTFIRE_ROTATE_BUILDING,		// #Hint_altfire_rotate_building

	// Class specific
	// Soldier
	HINT_SOLDIER_RPG_RELOAD,			// #Hint_Soldier_rpg_reload

	// Engineer
	HINT_ENGINEER_USE_WRENCH_ONOWN,		// "#Hint_Engineer_use_wrench_onown",
	HINT_ENGINEER_USE_WRENCH_ONOTHER,	// "#Hint_Engineer_use_wrench_onother",
	HINT_ENGINEER_USE_WRENCH_FRIEND,	// "#Hint_Engineer_use_wrench_onfriend",
	HINT_ENGINEER_BUILD_SENTRYGUN,		// "#Hint_Engineer_build_sentrygun"
	HINT_ENGINEER_BUILD_DISPENSER,		// "#Hint_Engineer_build_dispenser"
	HINT_ENGINEER_BUILD_TELEPORTERS,	// "#Hint_Engineer_build_teleporters"
	HINT_ENGINEER_PICKUP_METAL,			// "#Hint_Engineer_pickup_metal"
	HINT_ENGINEER_REPAIR_OBJECT,		// "#Hint_Engineer_repair_object"
	HINT_ENGINEER_METAL_TO_UPGRADE,		// "#Hint_Engineer_metal_to_upgrade"
	HINT_ENGINEER_UPGRADE_SENTRYGUN,	// "#Hint_Engineer_upgrade_sentrygun"

	HINT_OBJECT_HAS_SAPPER,				// "#Hint_object_has_sapper"

	HINT_OBJECT_YOUR_OBJECT_SAPPED,		// "#Hint_object_your_object_sapped"
	HINT_OBJECT_ENEMY_USING_DISPENSER,	// "#Hint_enemy_using_dispenser"
	HINT_OBJECT_ENEMY_USING_TP_ENTRANCE,	// "#Hint_enemy_using_tp_entrance"
	HINT_OBJECT_ENEMY_USING_TP_EXIT,	// "#Hint_enemy_using_tp_exit"

	HINT_CANNOT_PHASE_WITH_FLAG,		// #Hint_Cannot_Phase_With_Flag

	HINT_CANNOT_ATTACK_WHILE_FEIGN_ARMED,	// #Hint_Cannot_Attack_While_Feign_Armed

	HINT_CANNOT_ARM_FEIGN_NOW,			// #Hint_Cannot_Arm_Feign_Now

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	NUM_HINTS
};
extern const char *g_pszHintMessages[];



/*======================*/
//      Menu stuff      //
/*======================*/

#define MENU_DEFAULT				1
#define MENU_TEAM 					2
#define MENU_CLASS 					3
#define MENU_MAPBRIEFING			4
#define MENU_INTRO 					5
#define MENU_CLASSHELP				6
#define MENU_CLASSHELP2 			7
#define MENU_REPEATHELP 			8

#define MENU_SPECHELP				9


#define MENU_SPY					12
#define MENU_SPY_SKIN				13
#define MENU_SPY_COLOR				14
#define MENU_ENGINEER				15
#define MENU_ENGINEER_FIX_DISPENSER	16
#define MENU_ENGINEER_FIX_SENTRYGUN	17
#define MENU_ENGINEER_FIX_MORTAR	18
#define MENU_DISPENSER				19
#define MENU_CLASS_CHANGE			20
#define MENU_TEAM_CHANGE			21

#define MENU_REFRESH_RATE 			25

#define MENU_VOICETWEAK				50

// Additional classes
// NOTE: adding them onto the Class_T's in baseentity.h is cheesy, but so is
// having an #ifdef for each mod in baseentity.h.
#define CLASS_TFGOAL				((Class_T)NUM_AI_CLASSES)
#define CLASS_TFGOAL_TIMER			((Class_T)(NUM_AI_CLASSES+1))
#define CLASS_TFGOAL_ITEM			((Class_T)(NUM_AI_CLASSES+2))
#define CLASS_TFSPAWN				((Class_T)(NUM_AI_CLASSES+3))
#define CLASS_MACHINE				((Class_T)(NUM_AI_CLASSES+4))

// TeamFortress State Flags
#define TFSTATE_GRENPRIMED		0x000001 // Whether the player has a primed grenade
#define TFSTATE_RELOADING		0x000002 // Whether the player is reloading
#define TFSTATE_ALTKILL			0x000004 // #TRUE if killed with a weapon not in self.weapon: NOT USED ANYMORE
#define TFSTATE_RANDOMPC		0x000008 // Whether Playerclass is random, new one each respawn
#define TFSTATE_INFECTED		0x000010 // set when player is infected by the bioweapon
#define TFSTATE_INVINCIBLE		0x000020 // Player has permanent Invincibility (Usually by GoalItem)
#define TFSTATE_INVISIBLE		0x000040 // Player has permanent Invisibility (Usually by GoalItem)
#define TFSTATE_QUAD			0x000080 // Player has permanent Quad Damage (Usually by GoalItem)
#define TFSTATE_RADSUIT			0x000100 // Player has permanent Radsuit (Usually by GoalItem)
#define TFSTATE_BURNING			0x000200 // Is on fire
#define TFSTATE_GRENTHROWING	0x000400  // is throwing a grenade
#define TFSTATE_AIMING			0x000800  // is using the laser sight
#define TFSTATE_ZOOMOFF			0x001000  // doesn't want the FOV changed when zooming
#define TFSTATE_RESPAWN_READY	0x002000  // is waiting for respawn, and has pressed fire
#define TFSTATE_HALLUCINATING	0x004000  // set when player is hallucinating
#define TFSTATE_TRANQUILISED	0x008000  // set when player is tranquilised
#define TFSTATE_CANT_MOVE		0x010000  // player isn't allowed to move
#define TFSTATE_RESET_FLAMETIME 0x020000 // set when the player has to have his flames increased in health
#define TFSTATE_HIGHEST_VALUE	TFSTATE_RESET_FLAMETIME

// items
#define IT_SHOTGUN				(1<<0)
#define IT_SUPER_SHOTGUN		(1<<1) 
#define IT_NAILGUN				(1<<2) 
#define IT_SUPER_NAILGUN		(1<<3) 
#define IT_GRENADE_LAUNCHER		(1<<4) 
#define IT_ROCKET_LAUNCHER		(1<<5) 
#define IT_LIGHTNING			(1<<6) 
#define IT_EXTRA_WEAPON			(1<<7) 

#define IT_SHELLS				(1<<8) 
#define IT_BULLETS				(1<<9) 
#define IT_ROCKETS				(1<<10) 
#define IT_CELLS				(1<<11) 
#define IT_AXE					(1<<12) 

#define IT_ARMOR1				(1<<13) 
#define IT_ARMOR2				(1<<14) 
#define IT_ARMOR3				(1<<15) 
#define IT_SUPERHEALTH			(1<<16) 

#define IT_KEY1					(1<<17) 
#define IT_KEY2					(1<<18) 

#define IT_INVISIBILITY			(1<<19) 
#define IT_INVULNERABILITY		(1<<20) 
#define IT_SUIT					(1<<21)
#define IT_QUAD					(1<<22) 
#define IT_HOOK					(1<<23)

#define IT_KEY3					(1<<24)	// Stomp invisibility
#define IT_KEY4					(1<<25)	// Stomp invulnerability
#define IT_LAST_ITEM			IT_KEY4

/*==================================================*/
/* New Weapon Related Defines		                */
/*==================================================*/

// Medikit
#define WEAP_MEDIKIT_OVERHEAL 50 // Amount of superhealth over max_health the medikit will dispense
#define WEAP_MEDIKIT_HEAL	200  // Amount medikit heals per hit

//--------------
// TF Specific damage flags
//--------------
//#define DMG_UNUSED					(DMG_LASTGENERICFLAG<<2)
// We can't add anymore dmg flags, because we'd be over the 32 bit limit.
// So lets re-use some of the old dmg flags in TF
#define DMG_USE_HITLOCATIONS					(DMG_AIRBOAT)
#define DMG_HALF_FALLOFF						(DMG_RADIATION)
#define DMG_CRITICAL							(DMG_ACID)
#define DMG_RADIUS_MAX							(DMG_ENERGYBEAM)
#define DMG_IGNITE								(DMG_PLASMA)
#define DMG_USEDISTANCEMOD						(DMG_SLOWBURN)		// NEED TO REMOVE CALTROPS
#define DMG_NOCLOSEDISTANCEMOD					(DMG_POISON)
#define DMG_FROM_OTHER_SAPPER					(DMG_IGNITE)		// USED TO DAMAGE SAPPERS ON MATCHED TELEPORTERS
#define DMG_MELEE								(DMG_BLAST_SURFACE)
#define DMG_DONT_COUNT_DAMAGE_TOWARDS_CRIT_RATE	(DMG_DISSOLVE)		// DON'T USE THIS FOR EXPLOSION DAMAGE YOU WILL MAKE BRANDON SAD AND KYLE SADDER

// This can only ever be used on a TakeHealth call, since it re-uses a dmg flag that means something else
#define DMG_IGNORE_MAXHEALTH	(DMG_BULLET)
#define DMG_IGNORE_DEBUFFS		(DMG_SLASH)

// Special Damage types
// Also update g_szSpecialDamageNames
enum ETFDmgCustom
{
	TF_DMG_CUSTOM_NONE = 0,
	TF_DMG_CUSTOM_HEADSHOT,
	TF_DMG_CUSTOM_BACKSTAB,
	TF_DMG_CUSTOM_BURNING,
	TF_DMG_WRENCH_FIX,
	TF_DMG_CUSTOM_MINIGUN,
	TF_DMG_CUSTOM_SUICIDE,
	TF_DMG_CUSTOM_TAUNTATK_HADOUKEN,
	TF_DMG_CUSTOM_BURNING_FLARE,
	TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON,
	TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM,
	TF_DMG_CUSTOM_PENETRATE_MY_TEAM,
	TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS,
	TF_DMG_CUSTOM_TAUNTATK_FENCING,
	TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE,
	TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB,
	TF_DMG_CUSTOM_TELEFRAG,
	TF_DMG_CUSTOM_BURNING_ARROW,
	TF_DMG_CUSTOM_FLYINGBURN,
	TF_DMG_CUSTOM_PUMPKIN_BOMB,
	TF_DMG_CUSTOM_DECAPITATION,
	TF_DMG_CUSTOM_TAUNTATK_GRENADE,
	TF_DMG_CUSTOM_BASEBALL,
	TF_DMG_CUSTOM_CHARGE_IMPACT,
	TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING,
	TF_DMG_CUSTOM_AIR_STICKY_BURST,
	TF_DMG_CUSTOM_DEFENSIVE_STICKY,
	TF_DMG_CUSTOM_PICKAXE,
	TF_DMG_CUSTOM_ROCKET_DIRECTHIT,
	TF_DMG_CUSTOM_TAUNTATK_UBERSLICE,
	TF_DMG_CUSTOM_PLAYER_SENTRY,
	TF_DMG_CUSTOM_STANDARD_STICKY,
	TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT,
	TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH,
	TF_DMG_CUSTOM_BLEEDING,
	TF_DMG_CUSTOM_GOLD_WRENCH,
	TF_DMG_CUSTOM_CARRIED_BUILDING,
	TF_DMG_CUSTOM_COMBO_PUNCH,
	TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL,
	TF_DMG_CUSTOM_FISH_KILL,
	TF_DMG_CUSTOM_TRIGGER_HURT,
	TF_DMG_CUSTOM_DECAPITATION_BOSS,
	TF_DMG_CUSTOM_STICKBOMB_EXPLOSION,
	TF_DMG_CUSTOM_AEGIS_ROUND,
	TF_DMG_CUSTOM_FLARE_EXPLOSION,
	TF_DMG_CUSTOM_BOOTS_STOMP,
	TF_DMG_CUSTOM_PLASMA,
	TF_DMG_CUSTOM_PLASMA_CHARGED,
	TF_DMG_CUSTOM_PLASMA_GIB,
	TF_DMG_CUSTOM_PRACTICE_STICKY,
	TF_DMG_CUSTOM_EYEBALL_ROCKET,
	TF_DMG_CUSTOM_HEADSHOT_DECAPITATION,
	TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON,
	TF_DMG_CUSTOM_FLARE_PELLET,
	TF_DMG_CUSTOM_CLEAVER,
	TF_DMG_CUSTOM_CLEAVER_CRIT,
	TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH,
	TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB,
	TF_DMG_CUSTOM_MERASMUS_GRENADE,
	TF_DMG_CUSTOM_MERASMUS_ZAP,
	TF_DMG_CUSTOM_MERASMUS_DECAPITATION,
	TF_DMG_CUSTOM_CANNONBALL_PUSH,
	TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF,
	TF_DMG_CUSTOM_THROWABLE,
	TF_DMG_CUSTOM_THROWABLE_KILL,
	TF_DMG_CUSTOM_SPELL_TELEPORT,
	TF_DMG_CUSTOM_SPELL_SKELETON,
	TF_DMG_CUSTOM_SPELL_MIRV,
	TF_DMG_CUSTOM_SPELL_METEOR,
	TF_DMG_CUSTOM_SPELL_LIGHTNING,
	TF_DMG_CUSTOM_SPELL_FIREBALL,
	TF_DMG_CUSTOM_SPELL_MONOCULUS,
	TF_DMG_CUSTOM_SPELL_BLASTJUMP,
	TF_DMG_CUSTOM_SPELL_BATS,
	TF_DMG_CUSTOM_SPELL_TINY,
	TF_DMG_CUSTOM_KART,
	TF_DMG_CUSTOM_GIANT_HAMMER,
	TF_DMG_CUSTOM_RUNE_REFLECT,
	TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE,
	TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING,
	TF_DMG_CUSTOM_SLAP_KILL,
	TF_DMG_CUSTOM_CROC,
	TF_DMG_CUSTOM_TAUNTATK_GASBLAST,
	TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED,
	TF_DMG_CUSTOM_KRAMPUS_MELEE,
	TF_DMG_CUSTOM_KRAMPUS_RANGED,
	TF_DMG_CUSTOM_TAUNTATK_TRICKSHOT,
	//
	// INSERT NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	TF_DMG_CUSTOM_END // END
};

const char *GetCustomDamageName( ETFDmgCustom eDmgCustom );
ETFDmgCustom GetCustomDamageFromName( const char *pszCustomDmgName );

inline bool IsTauntDmg( int iType )
{
	return (iType == TF_DMG_CUSTOM_TAUNTATK_HADOUKEN ||
			iType == TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON ||
			iType == TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM || 
			iType == TF_DMG_CUSTOM_TAUNTATK_FENCING ||
			iType == TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB ||
			iType == TF_DMG_CUSTOM_TAUNTATK_GRENADE ||
			iType == TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING ||
			iType == TF_DMG_CUSTOM_TAUNTATK_UBERSLICE ||
			iType == TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH || 
			iType == TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON ||
			iType == TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF || 
			iType == TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL ||
			iType == TF_DMG_CUSTOM_TAUNTATK_GASBLAST ||
			iType == TF_DMG_CUSTOM_TAUNTATK_TRICKSHOT );
}
inline bool IsDOTDmg( int iType )
{
	if ( iType == TF_DMG_CUSTOM_BURNING ||
		 iType == TF_DMG_CUSTOM_BURNING_FLARE ||
		 iType == TF_DMG_CUSTOM_BURNING_ARROW ||
		 iType == TF_DMG_CUSTOM_BLEEDING )
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline bool IsHeadshot( int iType ) 
{
	return (iType == TF_DMG_CUSTOM_HEADSHOT || iType == TF_DMG_CUSTOM_HEADSHOT_DECAPITATION);
}

enum
{
	TF_COLLISIONGROUP_GRENADES = LAST_SHARED_COLLISION_GROUP,
	TFCOLLISION_GROUP_OBJECT,
	TFCOLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT,
	TFCOLLISION_GROUP_COMBATOBJECT,
	TFCOLLISION_GROUP_ROCKETS,		// Solid to players, but not player movement. ensures touch calls are originating from rocket
	TFCOLLISION_GROUP_RESPAWNROOMS,
	TFCOLLISION_GROUP_TANK,
	TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

// Stun flags
#define TF_STUN_NONE						0
#define TF_STUN_MOVEMENT					(1<<0)
#define	TF_STUN_CONTROLS					(1<<1)
#define TF_STUN_MOVEMENT_FORWARD_ONLY		(1<<2)
#define TF_STUN_SPECIAL_SOUND				(1<<3)
#define TF_STUN_DODGE_COOLDOWN				(1<<4)
#define TF_STUN_NO_EFFECTS					(1<<5)
#define TF_STUN_LOSER_STATE					(1<<6)
#define TF_STUN_BY_TRIGGER					(1<<7)
#define TF_STUN_BOTH						TF_STUN_MOVEMENT | TF_STUN_CONTROLS
#define TF_STUN_SOUND						(1<<8)


//-----------------
// TF Objects Info
//-----------------

#define SENTRYGUN_UPGRADE_COST			130
#define SENTRYGUN_UPGRADE_METAL			200
#define SENTRYGUN_EYE_OFFSET_LEVEL_1	Vector( 0, 0, 32 )
#define SENTRYGUN_EYE_OFFSET_LEVEL_2	Vector( 0, 0, 40 )
#define SENTRYGUN_EYE_OFFSET_LEVEL_3	Vector( 0, 0, 46 )
#define SENTRYGUN_MAX_SHELLS_1			150
#define SENTRYGUN_MAX_SHELLS_2			200
#define SENTRYGUN_MAX_SHELLS_3			200
#define SENTRYGUN_MAX_ROCKETS			20

// Dispenser's maximum carrying capability
#define DISPENSER_MAX_METAL_AMMO		400
#define MINI_DISPENSER_MAX_METAL	200

//--------------------------------------------------------------------------
// OBJECTS
//--------------------------------------------------------------------------
enum ObjectType_t
{
	OBJ_DISPENSER=0,
	OBJ_TELEPORTER,
	OBJ_SENTRYGUN,

	// Attachment Objects
	OBJ_ATTACHMENT_SAPPER,

	// If you add a new object, you need to add it to the g_ObjectInfos array 
	// in tf_shareddefs.cpp, and add it's data to the scripts/object.txt

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	OBJ_LAST,
};

#define BUILDING_MODE_ANY -1

enum
{
	MODE_TELEPORTER_ENTRANCE=0,
	MODE_TELEPORTER_EXIT,
};

enum
{
	MODE_SENTRYGUN_NORMAL = 0,
	MODE_SENTRYGUN_DISPOSABLE,
};

enum
{
	MODE_SAPPER_NORMAL = 0,
	MODE_SAPPER_ANTI_ROBOT,
	MODE_SAPPER_ANTI_ROBOT_RADIUS,
};

enum ESpyTrapType_t
{
	MODE_SPY_TRAP_RADIUS_STEALTH = 0,
	MODE_SPY_TRAP_REPROGRAM,
	MODE_SPY_TRAP_MAGNET,
	// MODE_SPY_TRAP_REPULSOR,
};

// Warning levels for buildings in the building hud, in priority order
typedef enum
{
	BUILDING_HUD_ALERT_NONE = 0,
	BUILDING_HUD_ALERT_LOW_AMMO,
	BUILDING_HUD_ALERT_LOW_HEALTH,
	BUILDING_HUD_ALERT_VERY_LOW_AMMO,
	BUILDING_HUD_ALERT_VERY_LOW_HEALTH,
	BUILDING_HUD_ALERT_SAPPER,	

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	MAX_BUILDING_HUD_ALERT_LEVEL
} BuildingHudAlert_t;

typedef enum
{
	BUILDING_DAMAGE_LEVEL_NONE = 0,		// 100%
	BUILDING_DAMAGE_LEVEL_LIGHT,		// 75% - 99%
	BUILDING_DAMAGE_LEVEL_MEDIUM,		// 50% - 76%
	BUILDING_DAMAGE_LEVEL_HEAVY,		// 25% - 49%	
	BUILDING_DAMAGE_LEVEL_CRITICAL,		// 0% - 24%

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	MAX_BUILDING_DAMAGE_LEVEL
} BuildingDamageLevel_t;

//--------------
// Scoring
//--------------

#define TF_SCORE_KILL							1
#define TF_SCORE_DEATH							0
#define TF_SCORE_CAPTURE						2
#define TF_SCORE_DEFEND							1
#define TF_SCORE_DESTROY_BUILDING				1
#define TF_SCORE_HEADSHOT_DIVISOR				2
#define TF_SCORE_BACKSTAB						1
#define TF_SCORE_INVULN							1
#define TF_SCORE_REVENGE						1
#define TF_SCORE_KILL_ASSISTS_PER_POINT			2
#define TF_SCORE_TELEPORTS_PER_POINT			2
#define TF_SCORE_HEAL_HEALTHUNITS_PER_POINT		600
#define TF_SCORE_BONUS_POINT_DIVISOR			10
#define TF_SCORE_DAMAGE							250
#define TF_SCORE_CURRENCY_COLLECTED				20
#define TF_SCORE_CAPTURE_POWERUPMODE			5 // With these CTF rules capturing flags is tougher, hence the higher scoring for flag events
#define TF_SCORE_FLAG_RETURN					4
#define TF_SCORE_KILL_RUNECARRIER				1


//-------------------------
// Shared Teleporter State
//-------------------------
enum
{
	DISPENSER_STATE_IDLE,
	DISPENSER_STATE_UPGRADING,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

//-------------------------
// Shared Teleporter State
//-------------------------
enum
{
	TELEPORTER_STATE_BUILDING = 0,				// Building, not active yet
	TELEPORTER_STATE_IDLE,						// Does not have a matching teleporter yet
	TELEPORTER_STATE_READY,						// Found match, charged and ready
	TELEPORTER_STATE_SENDING,					// Teleporting a player away
	TELEPORTER_STATE_RECEIVING,					
	TELEPORTER_STATE_RECEIVING_RELEASE,
	TELEPORTER_STATE_RECHARGING,				// Waiting for recharge
	TELEPORTER_STATE_UPGRADING,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

#define TELEPORTER_TYPE_ENTRANCE	0
#define TELEPORTER_TYPE_EXIT		1

//-------------------------
// Shared Sentry State
//-------------------------
enum
{
	SENTRY_STATE_INACTIVE = 0,
	SENTRY_STATE_SEARCHING,
	SENTRY_STATE_ATTACKING,
	SENTRY_STATE_UPGRADING,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	SENTRY_NUM_STATES,
};

//--------------------------------------------------------------------------
// OBJECT FLAGS
//--------------------------------------------------------------------------
enum
{
	OF_ALLOW_REPEAT_PLACEMENT				= 0x01,
	OF_MUST_BE_BUILT_ON_ATTACHMENT			= 0x02,
	OF_DOESNT_HAVE_A_MODEL					= 0x04,
	OF_PLAYER_DESTRUCTION					= 0x08,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	OF_BIT_COUNT	= 4
};

//--------------------------------------------------------------------------
// Builder "weapon" states
//--------------------------------------------------------------------------
enum 
{
	BS_IDLE = 0,
	BS_SELECTING,
	BS_PLACING,
	BS_PLACING_INVALID

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};


//--------------------------------------------------------------------------
// Builder object id...
//--------------------------------------------------------------------------
enum
{
	BUILDER_OBJECT_BITS = 8,
	BUILDER_INVALID_OBJECT = ((1 << BUILDER_OBJECT_BITS) - 1)
};

// Analyzer state
enum
{
	AS_INACTIVE = 0,
	AS_SUBVERTING,
	AS_ANALYZING
};

// Max number of objects a team can have
#define MAX_OBJECTS_PER_PLAYER	6
//#define MAX_OBJECTS_PER_TEAM	128

// sanity check that commands send via user command are somewhat valid
#define MAX_OBJECT_SCREEN_INPUT_DISTANCE	100

//--------------------------------------------------------------------------
// BUILDING
//--------------------------------------------------------------------------
// Build checks will return one of these for a player
enum
{
	CB_CAN_BUILD,			// Player is allowed to build this object
	CB_CANNOT_BUILD,		// Player is not allowed to build this object
	CB_LIMIT_REACHED,		// Player has reached the limit of the number of these objects allowed
	CB_NEED_RESOURCES,		// Player doesn't have enough resources to build this object
	CB_NEED_ADRENALIN,		// Commando doesn't have enough adrenalin to build a rally flag
	CB_UNKNOWN_OBJECT,		// Error message, tried to build unknown object

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

// Build animation events
#define TF_OBJ_ENABLEBODYGROUP			6000
#define TF_OBJ_DISABLEBODYGROUP			6001
#define TF_OBJ_ENABLEALLBODYGROUPS		6002
#define TF_OBJ_DISABLEALLBODYGROUPS		6003
#define TF_OBJ_PLAYBUILDSOUND			6004

#define TF_AE_CIGARETTE_THROW			7000
#define TF_AE_GUN_SALUTE				7001
#define TF_AE_PICKAXE_THROW				7002
#define TF_AE_HEAD_THROW				7003

#define OBJECT_COST_MULTIPLIER_PER_OBJECT			3
#define OBJECT_UPGRADE_COST_MULTIPLIER_PER_LEVEL	3

//--------------------------------------------------------------------------
// Powerups
//--------------------------------------------------------------------------
enum
{
	POWERUP_BOOST,		// Medic, buff station
	POWERUP_EMP,		// Technician
	POWERUP_RUSH,		// Rally flag
	POWERUP_POWER,		// Object power

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	MAX_POWERUPS
};

#define	MAX_CABLE_CONNECTIONS 4

bool IsObjectAnUpgrade( int iObjectType );
bool IsObjectAVehicle( int iObjectType );
bool IsObjectADefensiveBuilding( int iObjectType );

class CHudTexture;

#define OBJECT_MAX_GIB_MODELS	9
#define OBJECT_MAX_MODES		3

// This should be moved into its own header.
class CObjectInfo
{
public:
	CObjectInfo( const char *pObjectName );	
	~CObjectInfo();

	// This is initialized by the code and matched with a section in objects.txt
	const char	*m_pObjectName;

	// This stuff all comes from objects.txt
	char	*m_pClassName;					// Code classname (in LINK_ENTITY_TO_CLASS).
	char	*m_pStatusName;					// Shows up when crosshairs are on the object.
	float	m_flBuildTime;
	int		m_nMaxObjects;					// Maximum number of objects per player
	int		m_Cost;							// Base object resource cost
	float	m_CostMultiplierPerInstance;	// Cost multiplier
	int		m_UpgradeCost;					// Base object resource cost for upgrading
	int		m_MaxUpgradeLevel;				// Max object upgrade level
	char	*m_pBuilderWeaponName;			// Names shown for each object onscreen when using the builder weapon
	char	*m_pBuilderPlacementString;		// String shown to player during placement of this object
	int		m_SelectionSlot;				// Weapon selection slots for objects
	int		m_SelectionPosition;			// Weapon selection positions for objects
	bool	m_bSolidToPlayerMovement;
	bool	m_bUseItemInfo;					// Use default item appearance info.
	char    *m_pViewModel;					// View model to show in builder weapon for this object
	char    *m_pPlayerModel;				// World model to show attached to the player
	int		m_iDisplayPriority;				// Priority for ordering in the hud display ( higher is closer to top )
	bool	m_bVisibleInWeaponSelection;	// should show up and be selectable via the weapon selection?
	char	*m_pExplodeSound;				// gamesound to play when object explodes
	char	*m_pExplosionParticleEffect;	// particle effect to play when object explodes
	bool	m_bAutoSwitchTo;				// should we let players switch back to the builder weapon representing this?
	char	*m_pUpgradeSound;				// gamesound to play when object is upgraded
	float	m_flUpgradeDuration;			// time it takes to upgrade to the next level
	int		m_iBuildCount;					// number of these that can be carried at one time
	int		m_iNumAltModes;					// whether the item has more than one mode (ex: teleporter exit/entrance)

	struct
	{
		char* pszStatusName;
		char* pszModeName;
		char* pszIconMenu;
	}		m_AltModes[OBJECT_MAX_MODES];

	// HUD weapon selection menu icon ( from hud_textures.txt )
	char	*m_pIconActive;
	char	*m_pIconInactive;
	char	*m_pIconMenu;

	// HUD building status icon
	char	*m_pHudStatusIcon;

	// gibs
	int		m_iMetalToDropInGibs;

	// unique builder
	bool	m_bRequiresOwnBuilder;			// if object needs to instantiate its' own builder
};

// Loads the objects.txt script.
class IBaseFileSystem;
void LoadObjectInfos( IBaseFileSystem *pFileSystem );

// Get a CObjectInfo from a TFOBJ_ define.
const CObjectInfo* GetObjectInfo( int iObject );

// Object utility funcs
bool	ClassCanBuild( int iClass, int iObjectType );
int		InternalCalculateObjectCost( int iObjectType /*, int iNumberOfObjects, int iTeam, bool bLast = false*/ );
int		CalculateObjectUpgrade( int iObjectType, int iObjectLevel );

// Shell ejections
enum
{
	EJECTBRASS_PISTOL,
	EJECTBRASS_MINIGUN,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

// Win panel styles
enum
{
	WINPANEL_BASIC = 0,
};

#define TF_DEATH_ANIMATION_TIME			2.0


enum HudNotification_t
{
	HUD_NOTIFY_YOUR_FLAG_TAKEN,
	HUD_NOTIFY_YOUR_FLAG_DROPPED,
	HUD_NOTIFY_YOUR_FLAG_RETURNED,
	HUD_NOTIFY_YOUR_FLAG_CAPTURED,

	HUD_NOTIFY_ENEMY_FLAG_TAKEN,
	HUD_NOTIFY_ENEMY_FLAG_DROPPED,
	HUD_NOTIFY_ENEMY_FLAG_RETURNED,
	HUD_NOTIFY_ENEMY_FLAG_CAPTURED,

	HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP,

	HUD_NOTIFY_NO_INVULN_WITH_FLAG,
	HUD_NOTIFY_NO_TELE_WITH_FLAG,

	HUD_NOTIFY_SPECIAL,

	HUD_NOTIFY_GOLDEN_WRENCH,

	HUD_NOTIFY_RD_ROBOT_UNDER_ATTACK,

	HUD_NOTIFY_HOW_TO_CONTROL_GHOST,
	HUD_NOTIFY_HOW_TO_CONTROL_KART,

	HUD_NOTIFY_PASSTIME_HOWTO,
	HUD_NOTIFY_PASSTIME_NO_TELE,
	HUD_NOTIFY_PASSTIME_NO_CARRY,
	HUD_NOTIFY_PASSTIME_NO_INVULN,
	HUD_NOTIFY_PASSTIME_NO_DISGUISE, 
	HUD_NOTIFY_PASSTIME_NO_CLOAK, 
	HUD_NOTIFY_PASSTIME_NO_OOB, // out of bounds
	HUD_NOTIFY_PASSTIME_NO_HOLSTER,
	HUD_NOTIFY_PASSTIME_NO_TAUNT,

	HUD_NOTIFY_COMPETITIVE_GC_DOWN,

	HUD_NOTIFY_TRUCE_START,
	HUD_NOTIFY_TRUCE_END,

	HUD_NOTIFY_HOW_TO_CONTROL_GHOST_NO_RESPAWN,
	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	NUM_STOCK_NOTIFICATIONS
};

// HudAlerts don't set HIDEHUD_MISCSTATUS, so they appear with the win panel, etc
typedef enum
{
	HUD_ALERT_SCRAMBLE_TEAMS,

	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//

	NUM_STOCK_ALERTS
} HudAlert_t;

//-----------------------------------------------------------------------------
// Item testing
//-----------------------------------------------------------------------------
enum testitem_botanims_t
{
	TI_BOTANIM_IDLE,
	TI_BOTANIM_CROUCH,
	TI_BOTANIM_RUN,
	TI_BOTANIM_CROUCH_WALK,
	TI_BOTANIM_JUMP,

	TI_BOTANIM_COUNT,
};

#define TF_DEATH_DOMINATION				0x0001	// killer is dominating victim
#define TF_DEATH_ASSISTER_DOMINATION	0x0002	// assister is dominating victim
#define TF_DEATH_REVENGE				0x0004	// killer got revenge on victim
#define TF_DEATH_ASSISTER_REVENGE		0x0008	// assister got revenge on victim
#define TF_DEATH_FIRST_BLOOD			0x0010  // death triggered a first blood
#define TF_DEATH_FEIGN_DEATH			0x0020  // feign death
#define TF_DEATH_INTERRUPTED			0x0040	// interrupted a player doing an important game event (like capping or carrying flag)
#define TF_DEATH_GIBBED					0x0080	// player was gibbed
#define TF_DEATH_PURGATORY				0x0100	// player died while in purgatory
#define TF_DEATH_MINIBOSS				0x0200	// player killed was a miniboss
#define TF_DEATH_AUSTRALIUM				0x0400	// player killed by a Australium Weapon

#define MAX_DECAPITATIONS		4

extern const char *TranslateWeaponEntForClass( const char *pszName, int iClass );

// Item Giveaways
#define PLAYER_ROLL_MIN			1
#define PLAYER_ROLL_MAX			500

// Generalized Jump State
#define TF_PLAYER_ROCKET_JUMPED		( 1 << 0 )
#define TF_PLAYER_STICKY_JUMPED		( 1 << 1 )
#define TF_PLAYER_ENEMY_BLASTED_ME	( 1 << 2 )

enum taunts_t
{
	TAUNT_BASE_WEAPON,		// The standard taunt we shipped with. Taunts based on your currently held weapon
	TAUNT_MISC_ITEM,		// Taunts based on the item you have equipped in your Misc slot.
	TAUNT_SHOW_ITEM,		// Show off an item to everyone nearby
	TAUNT_LONG,				// Press-and-hold taunt
	TAUNT_SPECIAL,			// Special-case taunts called explicitly from code
	//
	// ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
	//
};

enum eEurekaTeleportTargets
{
	EUREKA_FIRST_TARGET = 0,

	EUREKA_TELEPORT_HOME = 0,
	EUREKA_TELEPORT_TELEPORTER_EXIT,

	EUREKA_LAST_TARGET = EUREKA_TELEPORT_TELEPORTER_EXIT,
		
	EUREKA_NUM_TARGETS
};

#define TF_HIGHFIVE_HINT_MASK			( 0x10100 ) // annotations have id ( TF_HIGHFIVE_HINT_MASK | entindex )

enum BombDeployingState_t
{
	TF_BOMB_DEPLOYING_NONE,
	TF_BOMB_DEPLOYING_DELAY,
	TF_BOMB_DEPLOYING_ANIMATING,
	TF_BOMB_DEPLOYING_COMPLETE,

	TF_BOMB_DEPLOYING_NOT_COUNT,
};

enum EHorriblePyroVisionHack
{
	kHorriblePyroVisionHack_KillAssisterType_Default						= 0,

	kHorriblePyroVisionHack_KillAssisterType_CustomName						= 'a',
	kHorriblePyroVisionHack_KillAssisterType_LocalizationString				= 'b',
	kHorriblePyroVisionHack_KillAssisterType_CustomName_First				= 'c',
	kHorriblePyroVisionHack_KillAssisterType_LocalizationString_First		= 'd',
};

enum EAttackBonusEffects_t
{
	kBonusEffect_Crit = 0,
	kBonusEffect_MiniCrit,
	kBonusEffect_DoubleDonk,
	kBonusEffect_WaterBalloonSploosh,
	kBonusEffect_None = 4, // Must be 4.  Yep.
	kBonusEffect_DragonsFury,
	kBonusEffect_Stomp,

	kBonusEffect_Count, // Must be 2nd to last
};


//-----------------------------------------------------------------------------
// PVE MODE
//-----------------------------------------------------------------------------
// In-game currency
enum CurrencyRewards_t
{
	TF_CURRENCY_KILLED_PLAYER,
	TF_CURRENCY_KILLED_OBJECT,
	TF_CURRENCY_ASSISTED_PLAYER,
	TF_CURRENCY_BONUS_POINTS,
	TF_CURRENCY_CAPTURED_OBJECTIVE,
	TF_CURRENCY_ESCORT_REWARD,
	TF_CURRENCY_PACK_SMALL,
	TF_CURRENCY_PACK_MEDIUM,
	TF_CURRENCY_PACK_LARGE,
	TF_CURRENCY_PACK_CUSTOM,
	TF_CURRENCY_TIME_REWARD,
	TF_CURRENCY_WAVE_COLLECTION_BONUS,
};

enum mvm_announcement_t
{
	TF_MVM_ANNOUNCEMENT_WAVE_COMPLETE,
	TF_MVM_ANNOUNCEMENT_WAVE_FAILED,

	TF_MVM_ANNOUNCEMENT_TOTAL
};

#define RD_MAX_ROBOT_GROUPS_PER_TEAM 6

#define MAX_RAIDMODE_UPGRADES		60

enum mvm_upgrade_uigroups_t
{
	UIGROUP_UPGRADE_ATTACHED_TO_ITEM = 0,
	UIGROUP_UPGRADE_ATTACHED_TO_PLAYER,
	UIGROUP_POWERUPBOTTLE,
};

enum
{
	MVM_UPGRADE_QUALITY_LOW = 1,
	MVM_UPGRADE_QUALITY_NORMAL,		// Default
	MVM_UPGRADE_QAULITY_HIGH,
};

#define MVM_BUYBACK_COST_PER_SEC		5

#define MVM_CLASS_TYPES_PER_WAVE_MAX			12
// this is ugly, but we need to increase the max types per wave and changing the old define will break demos
#define MVM_CLASS_TYPES_PER_WAVE_MAX_NEW		( MVM_CLASS_TYPES_PER_WAVE_MAX * 2 )

#define MVM_CLASS_FLAG_NONE				0
#define MVM_CLASS_FLAG_NORMAL			(1<<0)
#define MVM_CLASS_FLAG_SUPPORT			(1<<1)
#define MVM_CLASS_FLAG_MISSION			(1<<2)
#define MVM_CLASS_FLAG_MINIBOSS			(1<<3)
#define MVM_CLASS_FLAG_ALWAYSCRIT		(1<<4)
#define MVM_CLASS_FLAG_SUPPORT_LIMITED	(1<<5)



enum MedicCallerType
{
	CALLER_TYPE_NORMAL,
	CALLER_TYPE_AUTO,
	CALLER_TYPE_REVIVE_EASY,		// The more someone is revived, the harder
	CALLER_TYPE_REVIVE_MEDIUM,		// subsequent revives become.
	CALLER_TYPE_REVIVE_HARD,
};

//-----------------------------------------------------------------------------
// Additional TF achievement packs
//-----------------------------------------------------------------------------
#define ACHIEVEMENT_TF_COMPLETE_TRAINING			(ACHIEVEMENT_TF_LAST_ORANGEBOX + 1)
#define ACHIEVEMENT_TF_FIRE_WATERJUMP				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 2)
#define ACHIEVEMENT_TF_KILL_BALLOONICORN_OWNERS		(ACHIEVEMENT_TF_LAST_ORANGEBOX + 3)
#define ACHIEVEMENT_TF_MULTIPLE_BFF					(ACHIEVEMENT_TF_LAST_ORANGEBOX + 4)
#define ACHIEVEMENT_TF_TEAM_PYROVISION				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 5)
#define ACHIEVEMENT_TF_DOMINATE_FOR_GOGGLES			(ACHIEVEMENT_TF_LAST_ORANGEBOX + 6)
#define ACHIEVEMENT_TF_PARACHUTE_KILL_GRIND				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 7)
#define ACHIEVEMENT_TF_MELEE_KILL_CLASSIC_RIFLE_SNIPER	(ACHIEVEMENT_TF_LAST_ORANGEBOX + 8)
#define ACHIEVEMENT_TF_KILL_CHARGING_DEMO				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 9)
#define ACHIEVEMENT_TF_TAUNT_CONGA_KILL				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 10)
#define ACHIEVEMENT_TF_TAUNT_CONGA_LINE				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 11)
#define ACHIEVEMENT_TF_TAUNT_RPS_ROCK				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 12)
#define ACHIEVEMENT_TF_TAUNT_RPS_SCISSORS			(ACHIEVEMENT_TF_LAST_ORANGEBOX + 13)
#define ACHIEVEMENT_TF_TAUNT_DOSIDO_MELLE_KILL		(ACHIEVEMENT_TF_LAST_ORANGEBOX + 14)
#define ACHIEVEMENT_TF_TAUNT_WHILE_CAPPING			(ACHIEVEMENT_TF_LAST_ORANGEBOX + 15)
#define ACHIEVEMENT_TF_PASS_TIME_HAT				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 16)
#define ACHIEVEMENT_TF_PASS_TIME_GRIND				(ACHIEVEMENT_TF_LAST_ORANGEBOX + 17)


#define ACHIEVEMENT_START_CLASS_SPECIFIC			1000

// Scout
#define ACHIEVEMENT_TF_SCOUT_START_RANGE			1001
#define ACHIEVEMENT_TF_SCOUT_FIRST_BLOOD			1001
#define ACHIEVEMENT_TF_SCOUT_FIRST_BLOOD_KILL		1002
#define ACHIEVEMENT_TF_SCOUT_WELL_EARLY_KILL		1003
#define ACHIEVEMENT_TF_SCOUT_LIFETIME_KILLS			1004
#define ACHIEVEMENT_TF_SCOUT_IRON_MAN_KILLS			1005
#define ACHIEVEMENT_TF_SCOUT_DESTROY_TELEPORTERS	1006
#define ACHIEVEMENT_TF_SCOUT_DESTROY_BUILDINGS_BEING_BUILT	1007
#define ACHIEVEMENT_TF_SCOUT_DESTROY_SENTRY_WITH_PISTOL		1008
#define ACHIEVEMENT_TF_SCOUT_DOUBLE_JUMPS			1009
#define ACHIEVEMENT_TF_SCOUT_ASSIST_MEDIC			1010
#define ACHIEVEMENT_TF_SCOUT_STEAL_SANDWICH			1011
#define ACHIEVEMENT_TF_SCOUT_KILL_CHARGED_MEDICS	1012
#define ACHIEVEMENT_TF_SCOUT_SURVIVE_DAMAGE			1013
#define ACHIEVEMENT_TF_SCOUT_THREE_FLAGCAPS			1014
#define ACHIEVEMENT_TF_SCOUT_DOUBLEJUMP_KILL		1015
#define ACHIEVEMENT_TF_SCOUT_FLAG_CAP_GRIND			1016
#define ACHIEVEMENT_TF_SCOUT_DODGE_DAMAGE			1017
#define ACHIEVEMENT_TF_SCOUT_KNOCK_INTO_TRAIN		1018
#define ACHIEVEMENT_TF_SCOUT_KILL_STUNNED			1019
#define ACHIEVEMENT_TF_SCOUT_STUN_INTO_TRAIN		1020
#define ACHIEVEMENT_TF_SCOUT_STUN_UBER_ENEMIES		1021
#define ACHIEVEMENT_TF_SCOUT_STUN_CAPPING_ENEMIES	1022
#define ACHIEVEMENT_TF_SCOUT_MAX_STUNS				1023
#define ACHIEVEMENT_TF_SCOUT_STUN_SCOUT_WITH_THEIR_BALL		1024
#define ACHIEVEMENT_TF_SCOUT_KILL_IN_DODGE_COOLDOWN	1025
#define ACHIEVEMENT_TF_SCOUT_KILL_FROM_BEHIND		1026
#define ACHIEVEMENT_TF_SCOUT_CAPTURE_LAST_POINT		1027
#define ACHIEVEMENT_TF_SCOUT_CAPTURE_THREE_POINTS	1028
#define ACHIEVEMENT_TF_SCOUT_FAST_CAP				1029
#define ACHIEVEMENT_TF_SCOUT_START_AND_FINISH_CAP	1030
#define ACHIEVEMENT_TF_SCOUT_BLOCK_CAPS				1031
#define	ACHIEVEMENT_TF_SCOUT_CARRIER_KILL_CARRIER	1032
#define ACHIEVEMENT_TF_SCOUT_CAP_FLAG_WITHOUT_ATTACKING 1033
#define ACHIEVEMENT_TF_SCOUT_LONG_DISTANCE_RUNNER	1034
#define ACHIEVEMENT_TF_SCOUT_TAUNT_KILL				1035
#define ACHIEVEMENT_TF_SCOUT_ACHIEVE_PROGRESS1		1036
#define ACHIEVEMENT_TF_SCOUT_ACHIEVE_PROGRESS2		1037
#define ACHIEVEMENT_TF_SCOUT_ACHIEVE_PROGRESS3		1038
#define ACHIEVEMENT_TF_SCOUT_BACKSCATTER_KILL_SPY_GRIND			1039
#define ACHIEVEMENT_TF_SCOUT_BACKSCATTER_KILL_HEAVY_MEDIC_PAIR	1040
#define ACHIEVEMENT_TF_SCOUT_BACKSCATTER_KILL_FRIENDS_GRIND		1041
#define ACHIEVEMENT_TF_SCOUT_END_RANGE				1041

// Sniper
#define ACHIEVEMENT_TF_SNIPER_START_RANGE			1101
#define ACHIEVEMENT_TF_SNIPER_JARATE_DOMINATED		1101
#define ACHIEVEMENT_TF_SNIPER_FREEZECAM_HAT			1102
#define ACHIEVEMENT_TF_SNIPER_RIFLE_NO_MISSING		1103
#define ACHIEVEMENT_TF_SNIPER_CAPTURE_FLAG			1104
#define ACHIEVEMENT_TF_SNIPER_KILL_GRIND			1105
#define ACHIEVEMENT_TF_SNIPER_KILL_WEAPONS			1106
#define ACHIEVEMENT_TF_SNIPER_HEADSHOT_SNIPERS		1107
#define ACHIEVEMENT_TF_SNIPER_DESTROY_SENTRYGUNS	1108
#define ACHIEVEMENT_TF_SNIPER_KILL_INVIS_SPY		1109
#define ACHIEVEMENT_TF_SNIPER_KILL_MIDAIR_SCOUT		1110
#define ACHIEVEMENT_TF_SNIPER_FREEZECAM_WAVE		1111
#define ACHIEVEMENT_TF_SNIPER_DOMINATE_SNIPER		1112
#define ACHIEVEMENT_TF_SNIPER_KILL_SPIES_MELEE		1113
#define ACHIEVEMENT_TF_SNIPER_HEADSHOT_DEMOMAN		1114
#define ACHIEVEMENT_TF_SNIPER_KILL_RJER				1115
#define ACHIEVEMENT_TF_SNIPER_KILL_OBJECTIVES		1116
#define ACHIEVEMENT_TF_SNIPER_KILL_UNSCOPED			1117
#define ACHIEVEMENT_TF_SNIPER_KILL_CHARGED_MEDIC	1118
#define ACHIEVEMENT_TF_SNIPER_GET_BACKSTABBED		1119
#define ACHIEVEMENT_TF_SNIPER_KILL_AT_ROUNDSTART	1120
#define ACHIEVEMENT_TF_SNIPER_HEADSHOT_POST_INVULN	1121
#define ACHIEVEMENT_TF_SNIPER_TOP_SCOREBOARD_GRIND	1122
#define ACHIEVEMENT_TF_SNIPER_KILL_FAILED_SPY		1123
#define ACHIEVEMENT_TF_SNIPER_JARATE_GROUP			1124
#define ACHIEVEMENT_TF_SNIPER_JARATE_MEDIC_PAIR		1125
#define ACHIEVEMENT_TF_SNIPER_JARATE_REVEAL_SPY		1126
#define ACHIEVEMENT_TF_SNIPER_JARATE_EXTINGUISH		1127
#define ACHIEVEMENT_TF_SNIPER_BOW_PIN_HEAVY			1128
#define ACHIEVEMENT_TF_SNIPER_BOW_KILL_MEDIC_PAIR	1129
#define ACHIEVEMENT_TF_SNIPER_BOW_KILL_FLAGCARRIER	1130
#define ACHIEVEMENT_TF_SNIPER_BOW_PINCUSHION		1131
#define ACHIEVEMENT_TF_SNIPER_JARATE_ASSISTS		1132
#define ACHIEVEMENT_TF_SNIPER_JARATE_KILL_MELEE		1133
#define ACHIEVEMENT_TF_SNIPER_TAUNT_KILL			1134
#define ACHIEVEMENT_TF_SNIPER_BOW_KILL_WHILEDEAD	1135
#define ACHIEVEMENT_TF_SNIPER_ACHIEVE_PROGRESS1		1136
#define ACHIEVEMENT_TF_SNIPER_ACHIEVE_PROGRESS2		1137
#define ACHIEVEMENT_TF_SNIPER_ACHIEVE_PROGRESS3		1138
#define ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_NOSCOPE_HEADSHOT		1139
#define ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_HEADSHOT_JUMPER			1140
#define ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_GIB_GRIND				1141
#define ACHIEVEMENT_TF_SNIPER_PARACHUTE_GRIND						1142
#define ACHIEVEMENT_TF_SNIPER_END_RANGE				1142

// Soldier
#define ACHIEVEMENT_TF_SOLDIER_START_RANGE			1201
#define ACHIEVEMENT_TF_SOLDIER_RJ_EQUALIZER_KILL	1201
#define ACHIEVEMENT_TF_SOLDIER_BUFF_TEAMMATES		1202
#define ACHIEVEMENT_TF_SOLDIER_KILL_DEMOMAN_GRIND	1203
#define ACHIEVEMENT_TF_SOLDIER_KILL_ENGY			1204
#define ACHIEVEMENT_TF_SOLDIER_KILL_PYRO			1205
#define ACHIEVEMENT_TF_SOLDIER_NEMESIS_SHOVEL_KILL	1206
#define ACHIEVEMENT_TF_SOLDIER_DESTROY_STICKIES		1207
#define ACHIEVEMENT_TF_SOLDIER_CROUCH_ROCKET_JUMP	1208
#define ACHIEVEMENT_TF_SOLDIER_EQUALIZER_STREAK		1209
#define ACHIEVEMENT_TF_SOLDIER_BUFF_FRIENDS			1210
#define ACHIEVEMENT_TF_SOLDIER_KILL_GROUP_WITH_CROCKET				1211
#define ACHIEVEMENT_TF_SOLDIER_KILL_TWO_DURING_ROCKET_JUMP			1212
#define ACHIEVEMENT_TF_SOLDIER_KILL_TAUNT			1213
#define ACHIEVEMENT_TF_SOLDIER_DEFEND_MEDIC			1214
#define ACHIEVEMENT_TF_SOLDIER_KILL_WITH_EQUALIZER_WHILE_HURT		1215
#define ACHIEVEMENT_TF_SOLDIER_KILL_AIRBORNE_TARGET_WHILE_AIRBORNE	1216
#define ACHIEVEMENT_TF_SOLDIER_BOUNCE_THEN_SHOTGUN	1217
#define ACHIEVEMENT_TF_SOLDIER_KILL_AIRBORNE_WITH_DIRECT_HIT		1218
#define ACHIEVEMENT_TF_SOLDIER_KILL_SNIPER_WHILE_DEAD				1219
#define ACHIEVEMENT_TF_SOLDIER_DESTROY_SENTRY_OUT_OF_RANGE			1220
#define ACHIEVEMENT_TF_SOLDIER_DUO_SOLDIER_KILLS	1221
#define ACHIEVEMENT_TF_SOLDIER_MVP					1222
#define ACHIEVEMENT_TF_SOLDIER_ASSIST_MEDIC_UBER	1223
#define ACHIEVEMENT_TF_SOLDIER_SHOOT_MULT_CRITS		1224
#define ACHIEVEMENT_TF_SOLDIER_KILL_DEFENSELESS		1225
#define ACHIEVEMENT_TF_SOLDIER_KILL_ON_FIRE			1226
#define ACHIEVEMENT_TF_SOLDIER_FREEZECAM_TAUNT		1227
#define ACHIEVEMENT_TF_SOLDIER_FREEZECAM_GIBS		1228
#define ACHIEVEMENT_TF_SOLDIER_KILL_SPY_KILLER		1229
#define ACHIEVEMENT_TF_SOLDIER_GIB_GRIND			1230
#define ACHIEVEMENT_TF_SOLDIER_THREE_DOMINATIONS	1231
#define ACHIEVEMENT_TF_SOLDIER_RIDE_THE_CART		1232
#define ACHIEVEMENT_TF_SOLDIER_KILL_TWENTY_FROM_ABOVE			1233
#define ACHIEVEMENT_TF_SOLDIER_KILL_FIVE_STUNNED				1234
#define ACHIEVEMENT_TF_SOLDIER_DEFEND_CAP_THIRTY_TIMES			1235
#define ACHIEVEMENT_TF_SOLDIER_ACHIEVE_PROGRESS1				1236
#define ACHIEVEMENT_TF_SOLDIER_ACHIEVE_PROGRESS2				1237
#define ACHIEVEMENT_TF_SOLDIER_ACHIEVE_PROGRESS3				1238
#define ACHIEVEMENT_TF_SOLDIER_PARACHUTE_KILL_GROUP				1239
#define ACHIEVEMENT_TF_SOLDIER_PARACHUTE_DISTANCE				1240
#define ACHIEVEMENT_TF_SOLDIER_PARACHUTE_KILL_PARACHUTE			1241
#define ACHIEVEMENT_TF_SOLDIER_AIRSTRIKE_MAX_CLIP				1242
#define ACHIEVEMENT_TF_SOLDIER_AIRSTRIKE_GROUP_KILL				1243
#define ACHIEVEMENT_TF_SOLDIER_END_RANGE						1243

// Demoman
#define ACHIEVEMENT_TF_DEMOMAN_START_RANGE						1301
#define ACHIEVEMENT_TF_DEMOMAN_KILL_SOLDIER_GRIND				1301
#define ACHIEVEMENT_TF_DEMOMAN_DESTROY_BUILDINGS_WITH_MEDIC		1302
#define ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_CLOAKED_SPY			1303
#define ACHIEVEMENT_TF_DEMOMAN_KILL_X_WITH_DIRECTPIPE			1304
#define ACHIEVEMENT_TF_DEMOMAN_BOUNCE_AND_KILL					1305
#define ACHIEVEMENT_TF_DEMOMAN_KILL_TWO_DURING_STICKYJUMP		1306
#define ACHIEVEMENT_TF_DEMOMAN_KILL_PLAYER_AFTER_TP				1307
#define ACHIEVEMENT_TF_DEMOMAN_DOMINATE_THREE_ENGINEERS			1308
#define ACHIEVEMENT_TF_DEMOMAN_KILL_BUILDING_DIRECT_HIT			1309
#define ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_PLAYERS				1310
#define ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_PLAYERS_FAST			1311
#define ACHIEVEMENT_TF_DEMOMAN_DUO_DEMOMAN_KILLS				1312
#define ACHIEVEMENT_TF_DEMOMAN_MELEE_KILL_WHILE_STICKYJUMPING	1313
#define ACHIEVEMENT_TF_DEMOMAN_KILL_ENGI_SENTRY_DISPENSER		1314
#define ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_EQUALIZER				1315
#define ACHIEVEMENT_TF_DEMOMAN_DECAPITATE_NEMESIS				1316
#define ACHIEVEMENT_TF_DEMOMAN_DAMAGE_GRIND						1317
#define ACHIEVEMENT_TF_DEMOMAN_KILL_X_CAPPING_ONEDET			1318
#define ACHIEVEMENT_TF_DEMOMAN_KILL_X_DEFENDING					1319
#define ACHIEVEMENT_TF_DEMOMAN_DESTROY_BUILDINGS_GRIND			1320
#define ACHIEVEMENT_TF_DEMOMAN_KILL_X_HEAVIES_FULLHP_ONEDET		1321
#define ACHIEVEMENT_TF_DEMOMAN_KILL_X_SCOUTS_PYROS				1322
#define ACHIEVEMENT_TF_DEMOMAN_TAUNT_KILL						1323
#define ACHIEVEMENT_TF_DEMOMAN_CHARGE_KILL						1324
#define ACHIEVEMENT_TF_DEMOMAN_CRIT_SWORD_KILL					1325
#define ACHIEVEMENT_TF_DEMOMAN_AIR_BURST_KILLS					1326
#define ACHIEVEMENT_TF_DEMOMAN_STICKYJUMP_CAP					1327
#define ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_SMILE					1328
#define ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_RUMP					1329
#define ACHIEVEMENT_TF_DEMOMAN_ENVIRONMENTAL_KILL				1330
#define ACHIEVEMENT_TF_DEMOMAN_DESTROY_X_STICKYBOMBS			1331
#define ACHIEVEMENT_TF_DEMOMAN_STICKJUMP_DISTANCE				1332
#define ACHIEVEMENT_TF_DEMOMAN_KILL3_WITH_DETONATION			1333
#define ACHIEVEMENT_TF_DEMOMAN_KILLXSAPPINGSPIES				1334
#define ACHIEVEMENT_TF_DEMOMAN_KILL3_WITH_PIPE_SETUPS			1335
#define ACHIEVEMENT_TF_DEMOMAN_ACHIEVE_PROGRESS1				1336
#define ACHIEVEMENT_TF_DEMOMAN_ACHIEVE_PROGRESS2				1337
#define ACHIEVEMENT_TF_DEMOMAN_ACHIEVE_PROGRESS3				1338
#define ACHIEVEMENT_TF_DEMOMAN_PARACHUTE_KILL_GROUP				1339
#define ACHIEVEMENT_TF_DEMOMAN_PARACHUTE_DISTANCE				1340
#define ACHIEVEMENT_TF_DEMOMAN_PARACHUTE_KILL_PARACHUTE			1341
#define ACHIEVEMENT_TF_DEMOMAN_KILL_PLAYER_YOU_DIDNT_SEE		1342
#define ACHIEVEMENT_TF_DEMOMAN_QUICK_KILLS						1343
#define ACHIEVEMENT_TF_DEMOMAN_CHARGE_KILL_CHARGING_DEMO		1344
#define ACHIEVEMENT_TF_DEMOMAN_END_RANGE						1344

// Medic
#define ACHIEVEMENT_TF_MEDIC_START_RANGE			1401
#define ACHIEVEMENT_TF_MEDIC_TOP_SCOREBOARD			1401
#define ACHIEVEMENT_TF_MEDIC_HEAL_UNDER_FIRE		1402
#define ACHIEVEMENT_TF_MEDIC_SIMUL_CHARGE			1403
#define ACHIEVEMENT_TF_MEDIC_SETUP_CHARGE			1404
#define ACHIEVEMENT_TF_MEDIC_RAPID_CHARGE			1405
#define ACHIEVEMENT_TF_MEDIC_COUNTER_CHARGE			1406
#define ACHIEVEMENT_TF_MEDIC_SWITCH_TO_MEDIC		1407
#define ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE			1408
#define ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER			1409
#define ACHIEVEMENT_TF_MEDIC_ASSIST_MEDIC			1410
#define ACHIEVEMENT_TF_MEDIC_SYRINGE_SCOUTS			1411
#define ACHIEVEMENT_TF_MEDIC_BONESAW_MEDICS			1412
#define ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY_LONG		1413
#define ACHIEVEMENT_TF_MEDIC_ASSIST_SCOUT			1414
#define ACHIEVEMENT_TF_MEDIC_ASSIST_PYRO			1415
#define ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY			1416
#define ACHIEVEMENT_TF_MEDIC_ASSIST_DEMOMAN			1417
#define ACHIEVEMENT_TF_MEDIC_ASSIST_SOLDIER			1418
#define ACHIEVEMENT_TF_MEDIC_HEAL_ENGINEER			1419
#define ACHIEVEMENT_TF_MEDIC_ASSIST_CAPTURER		1420
#define ACHIEVEMENT_TF_MEDIC_HEAL_CALLERS			1421
#define ACHIEVEMENT_TF_MEDIC_EXTINGUISH_TEAMMATES	1422
#define ACHIEVEMENT_TF_MEDIC_ASSIST_VS_NEMESES		1423
#define ACHIEVEMENT_TF_MEDIC_KILL_WHILE_CHARGED		1424
#define ACHIEVEMENT_TF_MEDIC_BONESAW_NOMISSES		1425
#define ACHIEVEMENT_TF_MEDIC_HEAL_LARGE				1426
#define ACHIEVEMENT_TF_MEDIC_HEAL_HUGE				1427
#define ACHIEVEMENT_TF_MEDIC_HEAL_GRIND				1428
#define ACHIEVEMENT_TF_MEDIC_KILL_HEALED_SPY		1429
#define ACHIEVEMENT_TF_MEDIC_SAVE_FALLING_TEAMMATE	1430
#define ACHIEVEMENT_TF_MEDIC_CHARGE_JUGGLE			1431
#define ACHIEVEMENT_TF_MEDIC_FREEZECAM_RAGDOLL		1432
#define ACHIEVEMENT_TF_MEDIC_BONESAW_SPY_CALLERS	1433
#define ACHIEVEMENT_TF_MEDIC_CHARGE_FRIENDS			1434
#define ACHIEVEMENT_TF_MEDIC_INVITE_JOIN_CHARGE		1435
#define ACHIEVEMENT_TF_MEDIC_HEAL_ACHIEVER			1436
#define ACHIEVEMENT_TF_MEDIC_ACHIEVE_PROGRESS1		1437
#define ACHIEVEMENT_TF_MEDIC_ACHIEVE_PROGRESS2		1438
#define ACHIEVEMENT_TF_MEDIC_ACHIEVE_PROGRESS3		1439
#define ACHIEVEMENT_TF_MEDIC_END_RANGE				1439

// Heavy
#define ACHIEVEMENT_TF_HEAVY_START_RANGE			1501
#define ACHIEVEMENT_TF_HEAVY_DAMAGE_TAKEN			1501
#define ACHIEVEMENT_TF_HEAVY_DEFEND_MEDIC			1502
#define ACHIEVEMENT_TF_HEAVY_ASSIST_MEDIC_LARGE		1503
#define ACHIEVEMENT_TF_HEAVY_EARN_MEDIC_DOMINATION	1504
#define ACHIEVEMENT_TF_HEAVY_KILL_TAUNT				1505
#define ACHIEVEMENT_TF_HEAVY_KILL_FLAG_CARRIERS		1506
#define ACHIEVEMENT_TF_HEAVY_KILL_MEDIC_PAIR		1507
#define ACHIEVEMENT_TF_HEAVY_BLOCK_INVULN_HEAVY		1508
#define ACHIEVEMENT_TF_HEAVY_BLOCK_CART				1509
#define ACHIEVEMENT_TF_HEAVY_RECEIVE_UBER_GRIND		1510
#define ACHIEVEMENT_TF_HEAVY_STAND_NEAR_DISPENSER	1511
#define ACHIEVEMENT_TF_HEAVY_ASSIST_HEAVY_GRIND		1512
#define ACHIEVEMENT_TF_HEAVY_CLEAR_STICKYBOMBS		1513
#define ACHIEVEMENT_TF_HEAVY_KILL_UNDERWATER		1514
#define ACHIEVEMENT_TF_HEAVY_TAKE_MULTI_DAMAGE		1515
#define ACHIEVEMENT_TF_HEAVY_KILL_DOMINATED			1516
#define ACHIEVEMENT_TF_HEAVY_SURVIVE_CROCKET		1517
#define ACHIEVEMENT_TF_HEAVY_ASSIST_GRIND			1518
#define ACHIEVEMENT_TF_HEAVY_UNCOVER_SPIES			1519
#define ACHIEVEMENT_TF_HEAVY_KILL_WHILE_SPUNUP		1520
#define ACHIEVEMENT_TF_HEAVY_FIRE_LOTS				1521
#define ACHIEVEMENT_TF_HEAVY_KILL_CRIT_PUNCH		1522
#define ACHIEVEMENT_TF_HEAVY_HEAL_MEDIKITS			1523
#define ACHIEVEMENT_TF_HEAVY_KILL_SHOTGUN			1524
#define ACHIEVEMENT_TF_HEAVY_FIRST_TO_CAP			1525
#define ACHIEVEMENT_TF_HEAVY_PAYLOAD_CAP_GRIND		1526
#define ACHIEVEMENT_TF_HEAVY_KILL_MIDAIR_MINIGUN	1527
#define ACHIEVEMENT_TF_HEAVY_DEFEND_CONTROL_POINT	1528
#define ACHIEVEMENT_TF_HEAVY_KILL_CAPPING_ENEMIES	1529
#define ACHIEVEMENT_TF_HEAVY_REVENGE_ASSIST			1530
#define ACHIEVEMENT_TF_HEAVY_TELEPORT_FAST_KILL		1531
#define ACHIEVEMENT_TF_HEAVY_FREEZECAM_TAUNT		1532
#define ACHIEVEMENT_TF_HEAVY_EAT_SANDWICHES			1533
#define ACHIEVEMENT_TF_HEAVY_KILL_SCOUTS			1534
#define ACHIEVEMENT_TF_HEAVY_KILL_HEAVIES_GLOVES	1535
#define ACHIEVEMENT_TF_HEAVY_ACHIEVE_PROGRESS1		1537
#define ACHIEVEMENT_TF_HEAVY_ACHIEVE_PROGRESS2		1538
#define ACHIEVEMENT_TF_HEAVY_ACHIEVE_PROGRESS3		1539
#define ACHIEVEMENT_TF_HEAVY_END_RANGE				1539

// Pyro
#define ACHIEVEMENT_TF_PYRO_START_RANGE				1601
#define ACHIEVEMENT_TF_PYRO_KILL_MULTIWEAPONS		1601
#define ACHIEVEMENT_TF_PYRO_SIMULBURN_SCOUTS		1602
#define ACHIEVEMENT_TF_PYRO_FORCE_WATERJUMP			1603
#define ACHIEVEMENT_TF_PYRO_KILL_POSTDEATH			1604
#define ACHIEVEMENT_TF_PYRO_KILL_SPIES				1605
#define ACHIEVEMENT_TF_PYRO_KILL_CARRIERS			1606
#define ACHIEVEMENT_TF_PYRO_REVEAL_SPIES			1607
#define ACHIEVEMENT_TF_PYRO_CAMP_TELEPORTERS		1608
#define ACHIEVEMENT_TF_PYRO_CAMP_POSITION			1609
#define ACHIEVEMENT_TF_PYRO_KILL_AXE_SMALL			1610
#define ACHIEVEMENT_TF_PYRO_KILL_AXE_LARGE			1611
#define ACHIEVEMENT_TF_PYRO_KILL_FROM_BEHIND		1612
#define ACHIEVEMENT_TF_PYRO_BURN_SPIES_AS_YOU		1613
#define ACHIEVEMENT_TF_PYRO_BURN_SNIPERS_ZOOMED		1614
#define ACHIEVEMENT_TF_PYRO_BURN_MEDICS_CHARGED		1615
#define ACHIEVEMENT_TF_PYRO_REFLECT_PROJECTILES		1616
#define ACHIEVEMENT_TF_PYRO_KILL_HEAVIES			1617
#define ACHIEVEMENT_TF_PYRO_KILL_UNDERWATER			1618
#define ACHIEVEMENT_TF_PYRO_KILL_UBERCHARGE			1619
#define ACHIEVEMENT_TF_PYRO_DESTROY_BUILDINGS		1620
#define ACHIEVEMENT_TF_PYRO_DEFEND_POINTS			1621
#define ACHIEVEMENT_TF_PYRO_KILL_GRIND				1622
#define ACHIEVEMENT_TF_PYRO_DAMAGE_GRIND			1623
#define ACHIEVEMENT_TF_PYRO_BURN_MEDICPAIR			1624
#define ACHIEVEMENT_TF_PYRO_KILL_TAUNT				1625
#define ACHIEVEMENT_TF_PYRO_KILL_TEAMWORK			1626
#define ACHIEVEMENT_TF_PYRO_BURN_SPY_TAUNT			1627
#define ACHIEVEMENT_TF_PYRO_DOMINATE_LEAVESVR		1628
#define ACHIEVEMENT_TF_PYRO_REFLECT_CROCKET_KILL	1629
#define ACHIEVEMENT_TF_PYRO_KILL_TAUNTERS			1630
#define ACHIEVEMENT_TF_PYRO_DOUBLE_KO				1631
#define ACHIEVEMENT_TF_PYRO_BURN_RJ_SOLDIER			1632
#define ACHIEVEMENT_TF_PYRO_FREEZECAM_TAUNTS		1633
#define ACHIEVEMENT_TF_PYRO_KILL_GRIND_LARGE		1634
#define ACHIEVEMENT_TF_PYRO_IGNITE_FLAREGUN			1635
#define ACHIEVEMENT_TF_PYRO_ACHIEVE_PROGRESS1		1637
#define ACHIEVEMENT_TF_PYRO_ACHIEVE_PROGRESS2		1638
#define ACHIEVEMENT_TF_PYRO_ACHIEVE_PROGRESS3		1639
#define ACHIEVEMENT_TF_PYRO_IGNITE_WITH_RAINBOW		1640
#define ACHIEVEMENT_TF_PYRO_IGNITE_PLAYER_BEING_FLIPPED 1641
#define ACHIEVEMENT_TF_PYRO_END_RANGE				1641

// Spy
#define ACHIEVEMENT_TF_SPY_START_RANGE				1701
#define ACHIEVEMENT_TF_SPY_BACKSTAB_SNIPERS			1701
#define ACHIEVEMENT_TF_SPY_FREEZECAM_FLICK			1702
#define ACHIEVEMENT_TF_SPY_BACKSTAB_DISGUISED_SPY	1703
#define ACHIEVEMENT_TF_SPY_BACKSTAB_DISGUISE_TARGET	1704
#define ACHIEVEMENT_TF_SPY_TAUNT_KILL				1705
#define ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_HEALING_YOU	1706
#define ACHIEVEMENT_TF_SPY_BACKSTAB_ENGY_SAP_BUILDING	1707
#define ACHIEVEMENT_TF_SPY_SAP_BUILDING_BACKSTAB_ENGY	1708
#define ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_PAIR		1709
#define ACHIEVEMENT_TF_SPY_BACKSTAB_FRIENDS			1710
#define ACHIEVEMENT_TF_SPY_DOMINATE_SNIPER			1711
#define ACHIEVEMENT_TF_SPY_BUMP_CLOAKED_SPY			1712
#define ACHIEVEMENT_TF_SPY_KILL_SPY_WITH_KNIFE		1713
#define ACHIEVEMENT_TF_SPY_SURVIVE_BURNING			1714
#define ACHIEVEMENT_TF_SPY_BREAK_SHIELD_KILL_SNIPER	1715
#define ACHIEVEMENT_TF_SPY_KILL_WORKING_ENGY		1716
#define ACHIEVEMENT_TF_SPY_FAST_CAP					1717
#define ACHIEVEMENT_TF_SPY_MEDIC_HEALING_KILL_ENEMY	1718
#define ACHIEVEMENT_TF_SPY_KILL_CP_DEFENDERS		1719
#define ACHIEVEMENT_TF_SPY_BACKSTAB_DOMINATING_ENEMY	1720
#define ACHIEVEMENT_TF_SPY_REVENGE_WITH_BACKSTAB	1721
#define ACHIEVEMENT_TF_SPY_KNIFE_KILL_WHILE_JARATED	1722
#define ACHIEVEMENT_TF_SPY_BACKSTAB_QUICK_KILLS		1723
#define ACHIEVEMENT_TF_SPY_BACKSTAB_GRIND			1724
#define ACHIEVEMENT_TF_SPY_SAPPER_GRIND				1725
#define ACHIEVEMENT_TF_SPY_SAPPER_TEAMWORK			1726
#define ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_CHARGED	1727
#define ACHIEVEMENT_TF_SPY_BACKSTAB_CAPPING_ENEMIES	1728
#define ACHIEVEMENT_TF_SPY_BACKSTAB_ENEMY_SWITCH_PYRO	1729
#define ACHIEVEMENT_TF_SPY_AMBASSADOR_GRIND			1730
#define ACHIEVEMENT_TF_SPY_AMBASSADOR_SNIPER_GRIND	1731
#define ACHIEVEMENT_TF_SPY_FEIGN_DEATH_KILL			1732
#define ACHIEVEMENT_TF_SPY_AMBASSADOR_SCOUT_GRIND	1733
#define ACHIEVEMENT_TF_SPY_CAMP_POSITION			1734
#define ACHIEVEMENT_TF_SPY_ACHIEVE_PROGRESS1		1735
#define ACHIEVEMENT_TF_SPY_ACHIEVE_PROGRESS2		1736
#define ACHIEVEMENT_TF_SPY_ACHIEVE_PROGRESS3		1737
#define ACHIEVEMENT_TF_SPY_KILL_BACKSCATTER_SCOUT	1738
#define ACHIEVEMENT_TF_SPY_END_RANGE				1738

// Engineer
#define ACHIEVEMENT_TF_ENGINEER_START_RANGE					1801
#define ACHIEVEMENT_TF_ENGINEER_ACHIEVE_PROGRESS1			1801
#define ACHIEVEMENT_TF_ENGINEER_ACHIEVE_PROGRESS2			1802
#define ACHIEVEMENT_TF_ENGINEER_ACHIEVE_PROGRESS3			1803
#define ACHIEVEMENT_TF_ENGINEER_REVENGE_CRIT_SENTRY_KILLER	1804
#define ACHIEVEMENT_TF_ENGINEER_TELEPORT_GRIND				1805
#define ACHIEVEMENT_TF_ENGINEER_DISPENSER_EXTINGUISH		1806
#define ACHIEVEMENT_TF_ENGINEER_MANUAL_CLOAKED_SPY_KILL		1807
#define ACHIEVEMENT_TF_ENGINEER_REVENGE_CRIT_LIFE			1808
#define ACHIEVEMENT_TF_ENGINEER_MANUAL_SENTRY_ABSORB_DMG				1809
#define ACHIEVEMENT_TF_ENGINEER_MANUAL_SENTRY_KILLS_BEYOND_RANGE		1810
#define ACHIEVEMENT_TF_ENGINEER_UPGRADE_BUILDINGS			1811
#define ACHIEVEMENT_TF_ENGINEER_DISPENSER_HEAL_GROUP		1812
#define ACHIEVEMENT_TF_ENGINEER_SENTRY_KILL_LIFETIME_GRIND	1813
#define ACHIEVEMENT_TF_ENGINEER_WASTE_METAL_GRIND			1814
#define ACHIEVEMENT_TF_ENGINEER_KILL_FLAG_CARRIERS			1815
#define ACHIEVEMENT_TF_ENGINEER_KILL_DISGUISED_SPY			1816
#define ACHIEVEMENT_TF_ENGINEER_HELP_BUILD_STRUCTURE		1817
#define ACHIEVEMENT_TF_ENGINEER_FREEZECAM_TAUNT				1818
#define ACHIEVEMENT_TF_ENGINEER_FREEZECAM_SENTRY			1819
#define ACHIEVEMENT_TF_ENGINEER_SHOTGUN_KILL_PREV_SENTRY_TARGET	1820
#define ACHIEVEMENT_TF_ENGINEER_SENTRY_AVENGES_YOU			1821
#define ACHIEVEMENT_TF_ENGINEER_REPAIR_SENTRY_W_MEDIC		1822
#define ACHIEVEMENT_TF_ENGINEER_SENTRY_KILL_CAPS			1823
#define ACHIEVEMENT_TF_ENGINEER_DESTROY_SAPPERS				1824
#define ACHIEVEMENT_TF_ENGINEER_KILL_SPY_TWO_SAPPERS		1825
#define ACHIEVEMENT_TF_ENGINEER_REMOVE_SAPPER_LONG_DIST		1826
#define ACHIEVEMENT_TF_ENGINEER_TAUNT_KILL					1827
#define ACHIEVEMENT_TF_ENGINEER_DESTROY_STICKIES			1828
#define ACHIEVEMENT_TF_ENGINEER_HELP_MANUAL_SENTRY			1829
#define ACHIEVEMENT_TF_ENGINEER_KILL_SNIPERS_SENTRY			1830
#define ACHIEVEMENT_TF_ENGINEER_KILL_SENTRY_WITH_SENTRY		1831
#define ACHIEVEMENT_TF_ENGINEER_MOVE_SENTRY_GET_KILL		1832
#define ACHIEVEMENT_TF_ENGINEER_BUILDING_CARRY				1833
#define ACHIEVEMENT_TF_ENGINEER_KILL_ASSIST					1834
#define ACHIEVEMENT_TF_ENGINEER_DISPENSER_HEAL_GRIND		1835
#define ACHIEVEMENT_TF_ENGINEER_REPAIR_TEAM_GRIND			1836
#define ACHIEVEMENT_TF_ENGINEER_TANK_DAMAGE					1837
#define ACHIEVEMENT_TF_ENGINEER_HEAVY_ASSIST				1838
#define ACHIEVEMENT_TF_ENGINEER_END_RANGE					1838

#define ACHIEVEMENT_END_CLASS_SPECIFIC			1899
 
// Halloween Events
#define ACHIEVEMENT_TF_HALLOWEEN_START_RANGE					1901
#define ACHIEVEMENT_TF_HALLOWEEN_COLLECT_PUMPKINS				1901
#define ACHIEVEMENT_TF_HALLOWEEN_DOMINATE_FOR_HAT				1902
#define ACHIEVEMENT_TF_HALLOWEEN_KILL_SCARED_PLAYER				1903
#define ACHIEVEMENT_TF_HALLOWEEN_PUMPKIN_KILL					1904
#define ACHIEVEMENT_TF_HALLOWEEN_DISGUISED_SPY_KILL				1905
#define ACHIEVEMENT_TF_HALLOWEEN_BOSS_KILL						1906
#define ACHIEVEMENT_TF_HALLOWEEN_COLLECT_GOODY_BAG				1907
#define ACHIEVEMENT_TF_HALLOWEEN_CRAFT_SAXTON_MASK				1908
#define ACHIEVEMENT_TF_HALLOWEEN_BOSS_KILL_MELEE				1909
#define ACHIEVEMENT_TF_HALLOWEEN_EYEBOSS_KILL					1910
#define ACHIEVEMENT_TF_HALLOWEEN_LOOT_ISLAND					1911
#define ACHIEVEMENT_TF_HALLOWEEN_MERASMUS_COLLECT_LOOT			1912
#define ACHIEVEMENT_TF_HALLOWEEN_MERASMUS_KILL					1913
// These six achievements are part of the milestone for Helltower. Order matters here.
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_RARE_SPELL			1914
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_WIN_ROUNDS			1915
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_ENVIRONMENTAL_KILLS	1916
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKELETON_GRIND		1917
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_KILL_GRIND			1918
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_KILL_BROTHERS		1919
// This is the milestone for Helltower. Order matters here. 4/6 of the above achievements are required.
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_MILESTONE			1920
#define ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKULL_ISLAND_REWARD	1921
// These six achievements are part of the milestone for Doomsday. Order matters here.
#define ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_KILL_KARTS			1922
#define ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_COLLECT_DUCKS			1923
#define ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_SCORE_GOALS			1924
#define ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_RESPAWN_TEAMMATES		1925
#define ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_TINY_SMASHER			1926
#define ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_WIN_MINIGAMES			1927
// This is the milestone for Doomsday. Order matters here. 4/6 of the above achievements are required.
#define ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_MILESTONE				1928
#define ACHIEVEMENT_TF_HALLOWEEN_END_RANGE						1928

// Replay Achievements
#define ACHIEVEMENT_TF_REPLAY_START_RANGE			2001
#define ACHIEVEMENT_TF_REPLAY_SAVE_REPLAY			2001
#define ACHIEVEMENT_TF_REPLAY_PERFORMANCE_MODE		2002
#define ACHIEVEMENT_TF_REPLAY_BROWSE_REPLAYS		2003
#define ACHIEVEMENT_TF_REPLAY_EDIT_TIME				2004
#define ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_TIER1	2005
#define ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_TIER2	2006
#define ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_TIER3	2007
#define ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_HIGHEST	2008
#define ACHIEVEMENT_TF_REPLAY_END_RANGE				2008

// Christmas Event
#define ACHIEVEMENT_TF_CHRISTMAS_START_RANGE		2101
#define ACHIEVEMENT_TF_CHRISTMAS_COLLECT_GIFTS		2101
#define ACHIEVEMENT_TF_CHRISTMAS_END_RANGE			2101

// Foundry Achievements
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_START_RANGE				2201
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_KILL_CAPPING_ENEMY		2201
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_PLAY_GAME_FRIENDS		2202
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_WIN_MINTIME				2203
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_WIN_ROUNDS				2204
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_FAST_FINAL_CAP			2205
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_TELEPORT_AND_CAP		2206
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_PUSH_INTO_CAULDRON		2207
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_PUSH_BACK_AND_WIN		2208
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_PLAY_EACH_CLASS			2209
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_KILL_ENEMY_ON_ROOF		2210
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_BACK_AND_FORTH_BATTLE	2211
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_ACHIEVE_PROGRESS1		2212
#define ACHIEVEMENT_TF_MAPS_FOUNDRY_END_RANGE				2212

// MvM Achievements
#define ACHIEVEMENT_TF_MVM_START_RANGE						2301
#define ACHIEVEMENT_TF_MVM_COMPLETE_POP_FILE				2301
#define ACHIEVEMENT_TF_MVM_EARN_MONEY_BONUS					2302
#define ACHIEVEMENT_TF_MVM_ADVANCED_EARN_ALL_BONUSES		2303
#define ACHIEVEMENT_TF_MVM_PICKUP_MONEY_ABOUT_TO_EXPIRE		2304
#define ACHIEVEMENT_TF_MVM_COLLECT_MONEY_GRIND				2305
#define ACHIEVEMENT_TF_MVM_PLAY_GAME_FRIENDS				2306
#define ACHIEVEMENT_TF_MVM_PLAY_EACH_CLASS					2307
#define ACHIEVEMENT_TF_MVM_DESTROY_TWO_TANKS				2308
#define ACHIEVEMENT_TF_MVM_DESTROY_TANK_WHILE_DEPLOYING		2309
#define ACHIEVEMENT_TF_MVM_DESTROY_TANK_QUICKLY				2310
#define ACHIEVEMENT_TF_MVM_DEFEND_CAP						2311
#define ACHIEVEMENT_TF_MVM_KILL_BOMB_CARRIERS				2312
#define ACHIEVEMENT_TF_MVM_COMPLETE_WAVE_WITHOUT_DYING		2313
#define ACHIEVEMENT_TF_MVM_COMPLETE_TOUR					2314
#define ACHIEVEMENT_TF_MVM_USE_TELEPORT_BOTTLE				2315
#define ACHIEVEMENT_TF_MVM_USE_CRIT_BOTTLE					2316
#define ACHIEVEMENT_TF_MVM_USE_UBER_BOTTLE					2317
#define ACHIEVEMENT_TF_MVM_USE_BUILD_BOTTLE					2318
#define ACHIEVEMENT_TF_MVM_USE_AMMO_BOTTLE					2319
#define ACHIEVEMENT_TF_MVM_MAX_PRIMARY_UPGRADES				2320
#define ACHIEVEMENT_TF_MVM_MAX_PLAYER_RESISTANCES			2321
#define ACHIEVEMENT_TF_MVM_NO_ALARMS_IN_FINAL_WAVE			2322
#define ACHIEVEMENT_TF_MVM_KILL_MEDICS_CHARGED				2323
#define ACHIEVEMENT_TF_MVM_KILL_ROBOT_GRIND					2324
#define ACHIEVEMENT_TF_MVM_KILL_ROBOT_MEGA_GRIND			2325
#define ACHIEVEMENT_TF_MVM_KILL_SENTRY_BUSTER				2326
#define ACHIEVEMENT_TF_MVM_SPY_SAP_ROBOTS					2327
#define ACHIEVEMENT_TF_MVM_SOLDIER_BUFF_TEAM				2328
#define ACHIEVEMENT_TF_MVM_HEAVY_RAGE_PUSH_DEPLOYING_ROBOT	2329
#define ACHIEVEMENT_TF_MVM_MEDIC_SHARE_BOTTLES				2330
#define ACHIEVEMENT_TF_MVM_DEMO_GROUP_KILL					2331
#define ACHIEVEMENT_TF_MVM_SCOUT_MARK_FOR_DEATH				2332
#define ACHIEVEMENT_TF_MVM_SNIPER_KILL_GROUP				2333
#define ACHIEVEMENT_TF_MVM_PYRO_BOMB_RESET					2334
#define ACHIEVEMENT_TF_MVM_ENGINEER_ESCAPE_SENTRY_BUSTER	2335
#define ACHIEVEMENT_TF_MVM_ACHIEVE_PROGRESS1				2336
#define ACHIEVEMENT_TF_MVM_ACHIEVE_PROGRESS2				2337
#define ACHIEVEMENT_TF_MVM_ACHIEVE_PROGRESS3				2338
#define ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_TANK				2339
#define ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_BOMB				2340
#define ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_PIT_GRIND		2341
#define ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_PIT				2342
#define ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_MYSTERY			2343
#define ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_NO_GATES			2344
#define ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_STUN_RADIOWAVE	2345
#define ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_BOMB_BOT_GRIND	2346
#define ACHIEVEMENT_TF_MVM_SENTRY_BUSTER_FRIENDLY_FIRE		2347
#define ACHIEVEMENT_TF_MVM_SNIPER_COLLECT_HEADSHOT_MONEY	2348
#define ACHIEVEMENT_TF_MVM_MEDIC_SHIELD_BLOCK_DAMAGE		2349
#define ACHIEVEMENT_TF_MVM_MEDIC_REVIVE_TEAMMATES			2350
#define ACHIEVEMENT_TF_MVM_ROCKET_SPECIALIST_KILL_GRIND		2351
#define ACHIEVEMENT_TF_MVM_ROCKET_SPECIALIST_STUN_GRIND		2352
#define ACHIEVEMENT_TF_MVM_END_RANGE						2352

// Doomsday Achievements
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_START_RANGE				2401
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_SOLO_CAPTURE				2401
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_PLAY_GAME_FRIENDS			2402
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_WIN_ROUNDS					2403
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_PLAY_EACH_CLASS			2404
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_KILL_ENEMIES_ON_ELEVATOR	2405
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_PUSH_BACK_AND_WIN			2406
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_KILL_CARRIERS				2407
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_RIDE_THE_ELEVATOR			2408
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_DENY_NEUTRAL_PICKUP		2409
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_PUSH_INTO_EXHAUST			2410
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_DEFEND_CARRIER				2411
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_ACHIEVE_PROGRESS1			2412
#define ACHIEVEMENT_TF_MAPS_DOOMSDAY_END_RANGE					2412

// Process Achievement
#define ACHIEVEMENT_TF_MAPS_PROCESS_START_RANGE				2501
#define ACHIEVEMENT_TF_MAPS_PROCESS_WIN_ROUNDS				2501
#define ACHIEVEMENT_TF_MAPS_PROCESS_END_RANGE				2501

// Standin Achievement
#define ACHIEVEMENT_TF_MAPS_STANDIN_START_RANGE				2601
#define ACHIEVEMENT_TF_MAPS_STANDIN_WIN_ROUNDS				2601
#define ACHIEVEMENT_TF_MAPS_STANDIN_END_RANGE				2601

// Snakewater Achievements
#define ACHIEVEMENT_TF_MAPS_SNAKEWATER_START_RANGE				2701
#define ACHIEVEMENT_TF_MAPS_SNAKEWATER_WIN_ROUNDS				2701
#define ACHIEVEMENT_TF_MAPS_SNAKEWATER_PUSH_BACK_AND_WIN		2702
#define ACHIEVEMENT_TF_MAPS_SNAKEWATER_TEAM_KILL				2703
#define ACHIEVEMENT_TF_MAPS_SNAKEWATER_DOUBLE_AIR_DEATHS		2704
#define ACHIEVEMENT_TF_MAPS_SNAKEWATER_KILL_ENEMIES_IN_MIDDLE	2705
#define ACHIEVEMENT_TF_MAPS_SNAKEWATER_END_RANGE				2705

// Powerhouse Achievements
#define ACHIEVEMENT_TF_MAPS_POWERHOUSE_START_RANGE				2801
#define ACHIEVEMENT_TF_MAPS_POWERHOUSE_WIN_ROUNDS				2801
#define ACHIEVEMENT_TF_MAPS_POWERHOUSE_PUSH_BACK_AND_WIN		2802
#define ACHIEVEMENT_TF_MAPS_POWERHOUSE_FAST_FINAL_CAP			2803
#define ACHIEVEMENT_TF_MAPS_POWERHOUSE_KILL_CAPPING_PLAYER		2804
#define ACHIEVEMENT_TF_MAPS_POWERHOUSE_KILL_ENEMY_IN_WATER		2805
#define ACHIEVEMENT_TF_MAPS_POWERHOUSE_END_RANGE				2805

const unsigned char *GetTFEncryptionKey( void );

//-----------------------------------------------------------------------------
// Sapping Events
//-----------------------------------------------------------------------------
enum
{
	TF_SAPEVENT_NONE = 0,	
	TF_SAPEVENT_PLACED,
	TF_SAPEVENT_DONE,
};

//-----------------------------------------------------------------------------
// Helper code shared between client/server.
//-----------------------------------------------------------------------------
inline bool BIsAttributeValueWithDeltaOverCap( float flCurAttributeValue, float flAttrDeltaValue, float flCap )
{
	return AlmostEqual( flCurAttributeValue, flCap )
		|| ( flAttrDeltaValue > 0 && flCurAttributeValue >= flCap )
		|| ( flAttrDeltaValue < 0 && flCurAttributeValue <= flCap );
}

//-----------------------------------------------------------------------------
// Helltower Announcer lines for Redmond and Blutarch
//-----------------------------------------------------------------------------
enum
{
	HELLTOWER_VO_RED_MISC = 0,
	HELLTOWER_VO_BLUE_MISC,
	HELLTOWER_VO_RED_MISC_RARE,
	HELLTOWER_VO_BLUE_MISC_RARE,
	HELLTOWER_VO_RED_WINNING,
	HELLTOWER_VO_BLUE_WINNING,
	HELLTOWER_VO_RED_WINNING_RARE,
	HELLTOWER_VO_BLUE_WINNING_RARE,
	HELLTOWER_VO_RED_LOSING,
	HELLTOWER_VO_BLUE_LOSING,
	HELLTOWER_VO_RED_LOSING_RARE,
	HELLTOWER_VO_BLUE_LOSING_RARE,
	HELLTOWER_VO_RED_WIN,
	HELLTOWER_VO_BLUE_WIN,
	HELLTOWER_VO_RED_WIN_RARE,
	HELLTOWER_VO_BLUE_WIN_RARE,
	HELLTOWER_VO_RED_LOSE,
	HELLTOWER_VO_BLUE_LOSE,
	HELLTOWER_VO_RED_LOSE_RARE,
	HELLTOWER_VO_BLUE_LOSE_RARE,
	HELLTOWER_VO_RED_ROUNDSTART,
	HELLTOWER_VO_BLUE_ROUNDSTART,
	HELLTOWER_VO_RED_ROUNDSTART_RARE,
	HELLTOWER_VO_BLUE_ROUNDSTART_RARE,
	HELLTOWER_VO_RED_SKELETON_KING,
	HELLTOWER_VO_BLUE_SKELETON_KING,
	HELLTOWER_VO_RED_NEAR_WIN,
	HELLTOWER_VO_BLUE_NEAR_WIN,
	HELLTOWER_VO_RED_NEAR_LOSE,
	HELLTOWER_VO_BLUE_NEAR_LOSE,
	
	HELLTOWER_VO_COUNT,
};

struct helltower_vo_t
{
	const char *m_pszFormatString;
	int			m_nCount;
};

extern helltower_vo_t g_pszHelltowerAnnouncerLines[];

// flags to ignore certain check in CanAttack function
#define TF_CAN_ATTACK_FLAG_NONE				0
#define TF_CAN_ATTACK_FLAG_GRAPPLINGHOOK	0x01

struct PlayerHistoryInfo_t
{
	CSteamID steamID;
	float flTime;		// Time last seen
	int nTeam;			// Last team they were on
};

// When adding new RuneTypes_t, make sure to add new condition codes to 
// ETFCond and also update GetConditionFromRuneType and GetRuneTypeFromCondition.
enum RuneTypes_t
{
	RUNE_NONE = -1,
	RUNE_STRENGTH,
	RUNE_HASTE,
	RUNE_REGEN,
	RUNE_RESIST,
	RUNE_VAMPIRE,
	RUNE_REFLECT,
	RUNE_PRECISION,
	RUNE_AGILITY,
	RUNE_KNOCKOUT,
	RUNE_KING,
	RUNE_PLAGUE,
	RUNE_SUPERNOVA,

	// ADD NEW RUNE TYPE HERE, DO NOT RE-ORDER

	RUNE_TYPES_MAX
};

inline ETFCond GetConditionFromRuneType( RuneTypes_t rt )
{
	switch ( rt )
	{ 
	case RUNE_NONE:			return TF_COND_INVALID;
	case RUNE_STRENGTH:		return TF_COND_RUNE_STRENGTH;
	case RUNE_HASTE:		return TF_COND_RUNE_HASTE;
	case RUNE_REGEN:		return TF_COND_RUNE_REGEN;
	case RUNE_RESIST:		return TF_COND_RUNE_RESIST;
	case RUNE_VAMPIRE:		return TF_COND_RUNE_VAMPIRE;
	case RUNE_REFLECT:		return TF_COND_RUNE_REFLECT;
	case RUNE_PRECISION:	return TF_COND_RUNE_PRECISION;
	case RUNE_AGILITY:		return TF_COND_RUNE_AGILITY;
	case RUNE_KNOCKOUT:		return TF_COND_RUNE_KNOCKOUT;
	case RUNE_KING:			return TF_COND_RUNE_KING;
	case RUNE_PLAGUE:		return TF_COND_RUNE_PLAGUE;
	case RUNE_SUPERNOVA:	return TF_COND_RUNE_SUPERNOVA;
	default:
		AssertMsg( 0, "Unexpected rune_type rt (%d) in GetConditionFromRuneType", rt );	
	}

	return TF_COND_INVALID;
}

enum TemporaryRuneTypes_t
{
	RUNETYPE_TEMP_NONE = 0,

	RUNETYPE_TEMP_CRIT,
	RUNETYPE_TEMP_UBER,

	RUNETYPE_TEMP_MAX,
};

const char *GetPowerupIconName( RuneTypes_t type, int iTeam );

#define TOURNAMENT_NOCANCEL_TIME 10

#define TF_WEAPON_PICKUP_RANGE 150

enum CampaignMedalDisplayType_t
{
	CAMPAIGN_MEDAL_DISPLAY_TYPE_NONE = 0,

	// Gun Mettle Campaign
	CAMPAIGN_MEDAL_DISPLAY_TYPE_SUMMER2015_GRAVEL,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_SUMMER2015_BRONZE,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_SUMMER2015_SILVER,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_SUMMER2015_GOLD,

	// Invasion Community Update
	CAMPAIGN_MEDAL_DISPLAY_TYPE_INVASION,

	// Halloween
	CAMPAIGN_MEDAL_DISPLAY_TYPE_HALLOWEEN_GRAVEL,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_HALLOWEEN_BRONZE,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_HALLOWEEN_SILVER,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_HALLOWEEN_GOLD,

	// Tough Break Campaign
	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_GRAVEL1,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_BRONZE1,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_SILVER1,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_GOLD1,

	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_GRAVEL2,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_BRONZE2,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_SILVER2,
	CAMPAIGN_MEDAL_DISPLAY_TYPE_WINTER2016_GOLD2,

	CAMPAIGN_MEDAL_DISPLAY_TYPE_COUNT,
};

enum CampaignMedalBaseType_t
{
	CAMPAIGN_MEDAL_SUMMER2015		= 0x001,
	CAMPAIGN_MEDAL_INVASION			= 0x002,
	CAMPAIGN_MEDAL_HALLOWEEN		= 0x004,
	CAMPAIGN_MEDAL_WINTER2016		= 0x008,
};

extern const char *g_pszCampaignMedalIcons[];

// Rage buffs - how is rage generated with respect to a buff ID?
enum
{
	kRageBuffFlag_None = 0x00,
	kRageBuffFlag_OnDamageDealt = 0x01,
	kRageBuffFlag_OnDamageReceived = 0x02,
	kRageBuffFlag_OnMedicHealingReceived = 0x04,
	kRageBuffFlag_OnBurnDamageDealt = 0x08,
	kRageBuffFlag_OnHeal = 0x10
};

struct RageBuffType
{
	unsigned int m_iBuffFlags;
	float m_fRageScale;
	int m_nMaxPulses;
};

static const RageBuffType g_RageBuffTypes[] =
{
	{ kRageBuffFlag_None,					0.f,	10 },	// default/unknown buff type
	{ kRageBuffFlag_OnDamageDealt,			1.f,	10 },	// buff type 1
	{ kRageBuffFlag_OnDamageDealt,			1.f,	10 },	// buff type 2
	{ kRageBuffFlag_OnDamageDealt,			1.25f,	10 },	// buff type 3
	{ kRageBuffFlag_OnMedicHealingReceived,	1.f,	10 },	// buff type 4
	{ kRageBuffFlag_OnBurnDamageDealt,		1.f,	10 },	// pyro rage
	{ kRageBuffFlag_OnHeal,					1.f,	10 },	// medic healing
};

enum
{
	DRAWING_PANEL_TYPE_NONE,
	DRAWING_PANEL_TYPE_CRAFTING,
	DRAWING_PANEL_TYPE_MATCH_SUMMARY,

	DRAWING_PANEL_TYPE_MAX,
};

enum RankStatType_t
{
	RankStat_Invalid = -1,
	RankStat_Score = 0,
	RankStat_Kills,
	RankStat_Damage,
	RankStat_Healing,
	RankStat_Support,
	RankStat_Deaths,
};

enum StatMedal_t
{
	StatMedal_None = 0,
	StatMedal_Bronze = 1,
	StatMedal_Silver = 2,
	StatMedal_Gold = 3,

	StatMedal_Max
};

#define SF_TEAMSPAWN_SCOUT		1
#define SF_TEAMSPAWN_SNIPER		2
#define	SF_TEAMSPAWN_SOLDIER	4
#define SF_TEAMSPAWN_DEMOMAN	8
#define SF_TEAMSPAWN_MEDIC		16
#define SF_TEAMSPAWN_HEAVY		32
#define SF_TEAMSPAWN_PYRO		64
#define	SF_TEAMSPAWN_SPY		128
#define	SF_TEAMSPAWN_ENGINEER	256

enum MM_PlayerConnectionState_t
{
	MM_DISCONNECTED = 0,
	MM_CONNECTED,
	MM_CONNECTING, // the server knows that this player is coming
	MM_LOADING, // loading into the server
	MM_WAITING_FOR_PLAYER
};

enum AttributeMeter_Type_t
{
	ATTRIBUTE_METER_TYPE_NONE = 0,
	ATTRIBUTE_METER_TYPE_TIME,
	ATTRIBUTE_METER_TYPE_DAMAGE,
	ATTRIBUTE_METER_TYPE_COMBO,
};

#define PYRO_AFTERBURN_HEALING_REDUCTION 0.2f

#define TF_GRENADE_PROJECTILE_MINS	Vector( -2.0f, -2.0f, -2.0f )
#define TF_GRENADE_PROJECTILE_MAXS	Vector( 2.0f, 2.0f, 2.0f )

#endif // TF_SHAREDDEFS_H
  
