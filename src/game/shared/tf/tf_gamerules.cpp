//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_classdata.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tf_weaponbase.h"
#include "tf_weaponbase_gun.h"
#include "tier0/icommandline.h"
#include "convar_serverbounded.h"
#include "econ_item_system.h"
#include "tf_weapon_grenadelauncher.h"
#include "tf_logic_robot_destruction.h"
#include "tf_logic_player_destruction.h"
#include "tf_matchmaking_shared.h"
#include "tf_progression_description.h"

#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tf_player.h"
	#include "c_tf_objective_resource.h"
	#include <filesystem.h>
	#include "c_tf_team.h"
	#include "dt_utlvector_recv.h"
	#include "tf_autorp.h"
	#include "player_vs_environment/c_tf_upgrades.h"
	#include "video/ivideoservices.h"
	#include "tf_gc_client.h"
	#include "c_tf_playerresource.h"
#else
	#include "basemultiplayerplayer.h"
	#include "voice_gamemgr.h"
	#include "items.h"
	#include "team.h"
	#include "game.h"
	#include "tf_bot_temp.h"
	#include "tf_player.h"
	#include "tf_team.h"
	#include "player_resource.h"
	#include "entity_tfstart.h"
	#include "filesystem.h"
	#include "minigames/tf_duel.h"
	#include "tf_obj.h"
	#include "tf_objective_resource.h"
	#include "tf_player_resource.h"
	#include "team_control_point_master.h"
	#include "team_train_watcher.h"
	#include "playerclass_info_parse.h"
	#include "team_control_point_master.h"
	#include "coordsize.h"
	#include "entity_healthkit.h"
	#include "tf_gamestats.h"
	#include "entity_capture_flag.h"
	#include "tf_player_resource.h"
	#include "tf_obj_sentrygun.h"
	#include "activitylist.h"
	#include "AI_ResponseSystem.h"
	#include "hl2orange.spa.h"
	#include "hltvdirector.h"
	#include "tf_projectile_arrow.h"
	#include "func_suggested_build.h"
	#include "tf_weaponbase_grenadeproj.h"
	#include "engine/IEngineSound.h"
	#include "soundenvelope.h"
	#include "dt_utlvector_send.h"
	#include "tf_tactical_mission.h"
	#include "nav_mesh/tf_nav_area.h"
	#include "bot/tf_bot.h"
	#include "bot/tf_bot_manager.h"
	#include "bot/map_entities/tf_bot_roster.h"
	#include "econ_gcmessages.h"
	#include "vgui/ILocalize.h"
	#include "tier3/tier3.h"
	#include "tf_ammo_pack.h"
	#include "tf_gcmessages.h"
	#include "vote_controller.h"
	#include "tf_voteissues.h"
	#include "halloween/headless_hatman.h"
	#include "halloween/ghost/ghost.h"
	#include "halloween/eyeball_boss/eyeball_boss.h"
	#include "halloween/merasmus/merasmus.h"
	#include "halloween/merasmus/merasmus_dancer.h"
	#include "tf_extra_map_entity.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "tf_weapon_flaregun.h"
	#include "tf_weapon_sniperrifle.h"
	#include "tf_weapon_knife.h"
	#include "tf_weapon_jar.h"
	#include "halloween/tf_weapon_spellbook.h"
	
	#include "player_vs_environment/tf_population_manager.h"
	#include "player_vs_environment/monster_resource.h"
	#include "util_shared.h"
	#include "gc_clientsystem.h"

	#include "raid/tf_raid_logic.h"
	#include "player_vs_environment/tf_boss_battle_logic.h"
	#include "player_vs_environment/tf_mann_vs_machine_logic.h"
	#include "player_vs_environment/tf_upgrades.h"

	#include "tf_wheel_of_doom.h"
	#include "tf_halloween_souls_pickup.h"
	#include "halloween/zombie/zombie.h"
	#include "teamplay_round_timer.h"
	#include "halloween/spell/tf_spell_pickup.h"
	#include "tf_weapon_laser_pointer.h"
	#include "effect_dispatch_data.h"
	#include "tf_fx.h"
	#include "econ_game_account_server.h"
	#include "tf_logic_halloween_2014.h"
	#include "tf_obj_sentrygun.h"
	#include "entity_halloween_pickup.h"
	#include "entity_rune.h"
	#include "func_powerupvolume.h"
	#include "workshop/maps_workshop.h"
	#include "tf_passtime_logic.h"
	#include "cdll_int.h"
	#include "halloween/halloween_gift_spawn_locations.h"
	#include "tf_weapon_invis.h"
	#include "tf_gc_server.h"
	#include "gcsdk/msgprotobuf.h"
	#include "tf_party.h"
	#include "tf_autobalance.h"
	#include "player_voice_listener.h"
#endif

#include "tf_mann_vs_machine_stats.h"
#include "tf_upgrades_shared.h"

#include "tf_item_powerup_bottle.h"
#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_melee.h"
#include "tf_wearable_weapons.h"
#include "tf_weapon_buff_item.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_medigun.h"

#include "econ_holidays.h"
#include "rtime.h"
#include "tf_revive.h"
#include "tf_duckleaderboard.h"

#include "passtime_convars.h"

#include "tier3/tier3.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ITEM_RESPAWN_TIME	10.0f
#define MASK_RADIUS_DAMAGE  ( MASK_SHOT & ~( CONTENTS_HITBOX ) )

// Halloween 2013 VO defines for plr_hightower_event
#define HELLTOWER_TIMER_INTERVAL	( 60 + RandomInt( -30, 30 )	)
#define HELLTOWER_RARE_LINE_CHANCE	0.15	// 15%
#define HELLTOWER_MISC_CHANCE		0.50	// 50%

static int g_TauntCamRagdollAchievements[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	0,		// TF_CLASS_SNIPER,
	0,		// TF_CLASS_SOLDIER,
	0,		// TF_CLASS_DEMOMAN,
	ACHIEVEMENT_TF_MEDIC_FREEZECAM_RAGDOLL,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	ACHIEVEMENT_TF_SPY_FREEZECAM_FLICK,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_COUNT_ALL,
};

static int g_TauntCamAchievements[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	ACHIEVEMENT_TF_SNIPER_FREEZECAM_HAT,		// TF_CLASS_SNIPER,
	ACHIEVEMENT_TF_SOLDIER_FREEZECAM_GIBS,		// TF_CLASS_SOLDIER,		(extra check to count the number of gibs onscreen)
	ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_SMILE,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	ACHIEVEMENT_TF_HEAVY_FREEZECAM_TAUNT,		// TF_CLASS_HEAVYWEAPONS,  (there's an extra check on this one to see if we're also invuln)
	ACHIEVEMENT_TF_PYRO_FREEZECAM_TAUNTS,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	ACHIEVEMENT_TF_ENGINEER_FREEZECAM_TAUNT,	// TF_CLASS_ENGINEER,
	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_COUNT_ALL,
};

// used for classes that have more than one freeze cam achievement (example: Sniper)
static int g_TauntCamAchievements2[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	ACHIEVEMENT_TF_SNIPER_FREEZECAM_WAVE,		// TF_CLASS_SNIPER,
	ACHIEVEMENT_TF_SOLDIER_FREEZECAM_TAUNT,		// TF_CLASS_SOLDIER,
	ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_RUMP,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_COUNT_ALL,
};

struct StatueInfo_t
{
	const char	*pDiskName;
	Vector		vec_origin;
	QAngle		vec_angle;
};

static StatueInfo_t s_StatueMaps[] = {
	{ "ctf_2fort",				Vector( 483, 613, 0 ),			QAngle( 0, 180, 0 ) },
	{ "cp_dustbowl",			Vector( -596, 2650, -256 ),		QAngle( 0, 180, 0 ) },
	{ "cp_granary",				Vector( -544, -510, -416 ),		QAngle( 0, 180, 0 ) },
	{ "cp_well",				Vector( 1255, 515, -512 ),		QAngle( 0, 180, 0 ) },
	{ "cp_foundry",				Vector( -85, 912, 0 ),			QAngle( 0, -90, 0 ) },
	{ "cp_gravelpit",			Vector( -4624, 660, -512 ),		QAngle( 0, 0, 0 ) },
	{ "ctf_well",				Vector( 1000, -240, -512 ),		QAngle( 0, 180, 0 ) },
	{ "cp_badlands",			Vector( 808, -1079, 64 ),		QAngle( 0, 135, 0 ) },
	{ "pl_goldrush",			Vector( -2780, -650, 0 ),		QAngle( 0, 90, 0 ) },
	{ "pl_badwater",			Vector( 2690, -416, 131 ),		QAngle( 0, -90, 0 ) },
	{ "plr_pipeline",			Vector( 220, -2527, 128 ),		QAngle( 0, 90, 0 ) },
	{ "cp_gorge",				Vector( -6970, 5920, -42 ),		QAngle( 0, 0, 0 ) },
	{ "ctf_doublecross",		Vector( 1304, -206, 8 ),		QAngle( 0, 180, 0 ) },
	{ "pl_thundermountain",		Vector( -720, -1058, 128 ),		QAngle( 0, -90, 0 ) },
	{ "cp_mountainlab",			Vector( -2930, 1606, -1069 ),	QAngle( 0, 90, 0 ) },
	{ "cp_degrootkeep",			Vector( -1000, 4580, -255 ),	QAngle( 0, -25, 0 ) },
	{ "pl_barnblitz",			Vector( 3415, -2144, -54 ),		QAngle( 0, 90, 0 ) },
	{ "pl_upward",				Vector( -736, -2275, 63 ),		QAngle( 0, 0, 0 ) },
	{ "plr_hightower",			Vector( 5632, 7747, 8 ),		QAngle( 0, 0, 0 ) },
	{ "koth_viaduct",			Vector( -979, 0, 240 ),			QAngle( 0, 180, 0 ) },
	{ "koth_king",				Vector( 715, -395, -224 ),		QAngle( 0, 135, 0 ) },
	{ "sd_doomsday",			Vector( -1025, 675, 128 ),		QAngle( 0, 90, 0 ) },
	{ "cp_mercenarypark",		Vector( -2800, -775, -40 ),		QAngle( 0, 0, 0 ) },
	{ "ctf_turbine",			Vector( 718, 0, -256 ),			QAngle( 0, 180, 0 ) },
	{ "koth_harvest_final",		Vector( -1428, 220, -15 ),		QAngle( 0, 0, 0 ) },
	{ "pl_swiftwater_final1",	Vector( 706, -2785, -934 ),		QAngle( 0, 0, 0 ) },
	{ "pl_frontier_final",		Vector( 3070, -3013, -193 ),	QAngle( 0, -90, 0 ) },
	{ "cp_process_final",		Vector( 650, -980, 535 ),		QAngle( 0, 90, 0 ) },
	{ "cp_gullywash_final1",	Vector( 200, 83, 47 ),			QAngle( 0, -102, 0 ) },
	{ "cp_sunshine",			Vector( -4725, 5860, 65 ),		QAngle( 0, 180, 0 ) },
};

struct MapInfo_t
{
	const char	*pDiskName;
	const char	*pDisplayName;
	const char	*pGameType;
};

static MapInfo_t s_ValveMaps[] = {
	{ "ctf_2fort",	"2Fort",		"#Gametype_CTF" },
	{ "cp_dustbowl",	"Dustbowl",		"#TF_AttackDefend" },
	{ "cp_granary",	"Granary",		"#Gametype_CP" },
	{ "cp_well",		"Well",			"#Gametype_CP" },
	{ "cp_foundry",	"Foundry",		"#Gametype_CP" },
	{ "cp_gravelpit", "Gravel Pit",	"#TF_AttackDefend" },
	{ "tc_hydro",		"Hydro",		"#TF_TerritoryControl" },
	{ "ctf_well",		"Well",			"#Gametype_CTF" },
	{ "cp_badlands",	"Badlands",		"#Gametype_CP" },
	{ "pl_goldrush",	"Gold Rush",	"#Gametype_Escort" },
	{ "pl_badwater",	"Badwater Basin",	"#Gametype_Escort" },
	{ "plr_pipeline",	"Pipeline",		"#Gametype_EscortRace" },
	{ "cp_gorge",		"Gorge",		"#TF_AttackDefend" },
	{ "ctf_doublecross",		"Double Cross",		"#Gametype_CTF" },
	{ "pl_thundermountain",	"Thunder Mountain",	"#Gametype_Escort" },
	{ "tr_target",	"Target",		"#GameType_Training" },
	{ "tr_dustbowl",	"Dustbowl",		"#GameType_Training" },
	{ "cp_manor_event",	"Mann Manor",	"#TF_AttackDefend" },
	{ "cp_mountainlab",	"Mountain Lab",	"#TF_AttackDefend" },
	{ "cp_degrootkeep",	"DeGroot Keep",	"#TF_MedievalAttackDefend" },
	{ "pl_barnblitz",	"Barnblitz",	"#Gametype_Escort" },
	{ "pl_upward",	"Upward",	"#Gametype_Escort" },
	{ "plr_hightower",	"Hightower",	"#Gametype_EscortRace" },
	{ "koth_viaduct",	"Viaduct",	"#Gametype_Koth" },
	{ "koth_viaduct_event",	"Eyeaduct",	"#Gametype_Koth" },
	{ "koth_king",	"Kong King",	"#Gametype_Koth" },
	{ "koth_lakeside_event",	"Ghost Fort",	"#Gametype_Koth" },
	{ "plr_hightower_event",	"Helltower",	"#Gametype_EscortRace" },
	{ "rd_asteroid",	"Asteroid",	"#Gametype_RobotDestruction" },
	{ "pl_cactuscanyon",	"Cactus Canyon",	"#Gametype_Escort" },
	{ "sd_doomsday",	"Doomsday",	"#Gametype_SD" },
	{ "sd_doomsday_event",	"Carnival of Carnage",	"#Gametype_SD" },
	{ "cp_mercenarypark",	"Mercenary Park",	"#TF_AttackDefend" },
};

static MapInfo_t s_CommunityMaps[] = {
	{ "pl_borneo", "Borneo", "#Gametype_Escort" },
	{ "koth_suijin", "Suijin", "#Gametype_Koth" },
	{ "cp_snowplow", "Snowplow", "#TF_AttackDefend" },
	{ "koth_probed", "Probed", "#Gametype_Koth" },
	{ "pd_watergate", "Watergate", "#Gametype_PlayerDestruction" },
	{ "arena_byre", "Byre", "#Gametype_Arena" },
	{ "ctf_2fort_invasion", "2Fort Invasion", "#Gametype_CTF" },
	{ "cp_sunshine_event", "Sinshine", "#Gametype_CP" },
	{ "pl_millstone_event", "Hellstone", "#Gametype_Escort" },
	{ "cp_gorge_event", "Gorge Event", "#TF_AttackDefend" },
	{ "koth_moonshine_event", "Moonshine Event", "#Gametype_Koth" },
	{ "pl_snowycoast", "Snowycoast", "#Gametype_Escort" },
	{ "cp_vanguard", "Vanguard", "#Gametype_CP" },
	{ "ctf_landfall", "Landfall", "#Gametype_CTF" },
	{ "koth_highpass", "Highpass", "#Gametype_Koth" },
	{ "koth_maple_ridge_event", "Maple Ridge Event", "#Gametype_Koth" },
	{ "pl_fifthcurve_event", "Brimstone", "#Gametype_Escort" },
	{ "pd_pit_of_death_event", "Pit of Death", "#Gametype_PlayerDestruction" },
	{ "cp_mossrock", "Mossrock", "#TF_AttackDefend" },
	{ "koth_lazarus", "Lazarus", "#Gametype_Koth" },
	{ "plr_bananabay", "Banana Bay", "#Gametype_EscortRace" },
	{ "pl_enclosure_final", "Enclosure", "#Gametype_Escort" },
	{ "koth_brazil", "Brazil", "#Gametype_Koth" },
	{ "koth_bagel_event", "Cauldron", "#Gametype_Koth" },
	{ "pl_rumble_event", "Gravestone", "#Gametype_Escort" },
	{ "koth_slasher", "Slasher", "#Gametype_Koth" },
	{ "pd_cursed_cove_event", "Cursed Cove", "#Gametype_PlayerDestruction" },
	{ "pd_monster_bash", "Monster Bash", "#Gametype_PlayerDestruction" },
	{ "koth_slaughter_event", "Laughter", "#Gametype_Koth" },
	{ "pl_precipice_event_final", "Precipice", "#Gametype_Escort" },
	{ "koth_megalo", "Megalo", "#Gametype_Koth" },
	{ "pl_hasslecastle", "Hassle Castle", "#Gametype_Escort" },
	{ "pl_bloodwater", "Bloodwater", "#Gametype_Escort" },
	{ "koth_undergrove_event", "Moldergrove", "#Gametype_Koth" },
	{ "pl_pier", "Pier", "#Gametype_Escort" },
	{ "pd_snowville_event", "SnowVille", "#Gametype_PlayerDestruction" },
	{ "ctf_snowfall_final", "Snowfall", "#Gametype_CTF" },
	{ "pl_wutville_event", "Wutville", "#Gametype_Escort" },
	{ "pd_farmageddon", "Farmageddon", "#Gametype_PlayerDestruction" },
	{ "koth_los_muertos", "Los Muertos", "#Gametype_Koth" },
	{ "cp_ambush_event", "Erebus", "#TF_AttackDefend" },
	{ "pl_terror_event", "Terror", "#Gametype_Escort" },
	{ "arena_lumberyard_event", "Graveyard", "#Gametype_Arena" },
	{ "koth_synthetic_event", "Sinthetic", "#Gametype_Koth" },
	{ "pl_coal_event", "Polar", "#Gametype_Escort" },
	{ "pl_breadspace", "Bread Space", "#Gametype_Escort" },
	{ "pl_chilly", "Chilly", "#Gametype_Escort" },
	{ "koth_cascade", "Cascade", "#Gametype_Koth" },
	{ "cp_altitude", "Altitude", "#TF_AttackDefend" },
	{ "ctf_doublecross_snowy", "Doublefrost", "#Gametype_CTF" },
	{ "ctf_crasher", "Crasher!", "#Gametype_CTF" },
	{ "ctf_helltrain_event", "Helltrain", "#Gametype_CTF" },
	{ "pl_sludgepit_event", "Ghoulpit", "#Gametype_Escort" },
	{ "cp_spookeyridge", "Spookeyridge", "#TF_AttackDefend" },
	{ "koth_sawmill_event", "Soul-Mill", "#Gametype_Koth" },
	{ "plr_hacksaw_event", "Bonesaw", "#Gametype_EscortRace" },
	{ "cp_frostwatch", "Frostwatch", "#TF_AttackDefend" },
	{ "pl_frostcliff", "Frostcliff", "#Gametype_Escort" },
	{ "pl_rumford_event", "Rumford", "#Gametype_Escort" },
	{ "ctf_frosty", "Frosty", "#Gametype_CTF" },
	{ "cp_gravelpit_snowy", "Coal Pit", "#TF_AttackDefend" },
	{ "koth_sharkbay", "Sharkbay", "#Gametype_Koth" },
	{ "koth_rotunda", "Rotunda", "#Gametype_Koth" },
	{ "pl_phoenix", "Phoenix", "#Gametype_Escort" },
	{ "pl_cashworks", "Cashworks", "#Gametype_Escort" },
	{ "pl_venice", "Venice", "#Gametype_Escort" },
	{ "cp_reckoner", "Reckoner", "#Gametype_CP" },
	{ "cp_sulfur", "Sulfur", "#TF_AttackDefend" },
	{ "cp_hardwood_final", "Hardwood", "#TF_AttackDefend" },
	{ "ctf_pelican_peak", "Pelican Peak", "#Gametype_CTF" },
	{ "pd_selbyen", "Selbyen", "#Gametype_PlayerDestruction" },
	{ "vsh_tinyrock", "Tiny Rock", "#GameType_VSH" },
	{ "vsh_distillery", "Distillery", "#GameType_VSH" },
	{ "vsh_skirmish", "Skirmish", "#GameType_VSH" },
	{ "vsh_nucleus", "Nucleus VSH", "#GameType_VSH" },
	{ "arena_perks", "Perks", "#Gametype_Arena" },
	{ "koth_slime", "Slime", "#Gametype_Koth" },
	{ "cp_lavapit_final", "Lava Pit", "#TF_AttackDefend" },
	{ "pd_mannsylvania", "Mannsylvania", "#Gametype_PlayerDestruction" },
	{ "cp_degrootkeep_rats", "Sandcastle", "#TF_MedievalAttackDefend" },
	{ "pl_spineyard", "Spineyard", "#Gametype_Escort" },
	{ "pl_corruption", "Corruption", "#Gametype_Escort" },
	{ "zi_murky", "Murky", "#GameType_ZI" },
	{ "zi_atoll", "Atoll", "#GameType_ZI" },
	{ "zi_woods", "Woods", "#GameType_ZI" },
	{ "zi_sanitarium", "Sanitarium", "#GameType_ZI" },
	{ "zi_devastation_final1", "Devastation", "#GameType_ZI" },
	{ "koth_snowtower", "Snowtower", "#Gametype_Koth" },
	{ "koth_krampus", "Krampus", "#Gametype_Koth" },
	{ "ctf_haarp", "Haarp", "#TF_AttackDefend" },
	{ "cp_brew", "Brew", "#TF_AttackDefend" },
	{ "plr_hacksaw", "Hacksaw", "#Gametype_EscortRace" },
	{ "ctf_turbine_winter", "Turbine Event", "#Gametype_CTF" },
	{ "cp_carrier", "Carrier", "#TF_AttackDefend" },
	{ "pd_galleria", "Galleria", "#Gametype_PlayerDestruction" },
	{ "pl_emerge", "Emerge", "#Gametype_Escort" },
	{ "pl_camber", "Camber", "#Gametype_Escort" },
	{ "pl_embargo", "Embargo", "#Gametype_Escort" },
	{ "pl_odyssey", "Odyssey", "#Gametype_Escort" },
	{ "koth_megaton", "Megaton", "#Gametype_Koth" },
	{ "koth_cachoeira", "Cachoeira", "#Gametype_Koth" },
	{ "cp_overgrown", "Overgrown", "#TF_AttackDefend" },
	{ "cp_hadal", "Hadal", "#TF_AttackDefend" },
	{ "ctf_applejack", "Applejack", "#Gametype_CTF" },
	{ "pd_atom_smash", "Atom Smash", "#Gametype_PlayerDestruction" },
	{ "cp_canaveral_5cp", "Canaveral", "#Gametype_CP" },
	{ "cp_burghausen", "Burghausen", "#TF_MedievalAttackDefend" },
	{ "koth_toxic", "Toxic", "#Gametype_Koth" },
	{ "cp_darkmarsh", "Darkmarsh", "#TF_AttackDefend" },
	{ "cp_freaky_fair", "Freaky Fair", "#Gametype_CP" },
	{ "tow_dynamite", "Dynamite", "#GameType_TOW" },
	{ "pd_circus", "Circus", "#Gametype_PlayerDestruction" },
	{ "vsh_outburst", "Outburst", "#GameType_VSH" },
	{ "zi_blazehattan", "Blazehattan", "#GameType_ZI" },
	{ "koth_overcast_final", "Overcast", "#Gametype_Koth" },
	{ "cp_fortezza", "Fortezza", "#TF_AttackDefend" },
	{ "ctf_penguin_peak", "Penguin Peak", "#Gametype_CTF" },
	{ "pl_patagonia", "Patagonia", "#Gametype_Escort" },
	{ "plr_cutter", "Cutter", "#Gametype_EscortRace" },
	{ "vsh_maul", "Maul", "#GameType_VSH" },
	{ "pl_citadel", "Citadel", "#Gametype_Escort" },
	{ "pl_aquarius", "Aquarius", "#Gametype_Escort" },
	{ "cp_fulgur", "Fulgur", "#TF_AttackDefend" },
	{ "cp_cargo", "Cargo", "#TF_AttackDefend" },
	{ "cp_conifer", "Conifer", "#TF_AttackDefend" },
	{ "koth_boardwalk", "Boardwalk", "#Gametype_Koth" },
	{ "koth_blowout", "Blowout", "#Gametype_Koth" },
	{ "koth_mannhole", "Mannhole", "#Gametype_Koth" },
	{ "koth_demolition", "Demolition", "#Gametype_Koth" },
	{ "ctf_pressure", "Pressure", "#Gametype_CTF" },
	{ "cp_cowerhouse", "Cowerhouse", "#Gametype_CP" },
	{ "koth_dusker", "Dusker", "#Gametype_Koth" },
	{ "arena_afterlife", "Afterlife", "#Gametype_Arena" },
	{ "ctf_doublecross_event", "Devilcross", "#Gametype_CTF" },
	{ "sd_marshlands", "Marshlands", "#GameType_HTF" },
};

/*

  !! Commented out until we use this data, but we should keep updating it so its current when we need it

struct FeaturedWorkshopMap_t
{
	// The name the map was shipped as in our files (e.g. it was available as maps/<this>.bsp)
	// NOTE After maps are un-shipped we leave them here and don't change this value, this allows mapcycles that have
	// this map to proper redirect to the workshop in the future
	const char *pShippedName;
	// The workshop ID of the map (The public one, not the the private upload they use to send us the sources)
	PublishedFileId_t nWorkshopID;
};

static FeaturedWorkshopMap_t s_FeaturedWorkshopMaps[] = {
	// !! DO NOT remove these after we stop shipping the file -- they exist to ensure users can refer to
	// !! e.g. "koth_suijin" and get redirected to the workshop map once the file is no longer in our depots.

	// Summer 2015 Operation
	{ "pl_borneo",             454139147 },
	{ "koth_suijin",           454188876 },
	{ "cp_snowplow",           454116615 },

	// September 2015 Invasion Community Update
	{ "koth_probed",           454139808 },
	{ "pd_watergate",          456016898 },
	{ "arena_byre",            454142123 },
	{ "ctf_2fort_invasion",    FIXME     }, // No public workshop entry yet

	// Halloween 2015
	{ "cp_sunshine_event",     532473747 },
	{ "pl_millstone_event",    531384846 },
	{ "cp_gorge_event",        527145539 },
	{ "koth_moonshine_event",  534874830 },

	// December 2015 Campaign
	{ "pl_snowycoast",         469072819 },
	{ "cp_vanguard",           462908782 },
	{ "ctf_landfall",          459651881 },
	{ "koth_highpass",         463803443 },

	// Halloween 2016
	{ "koth_maple_ridge_event",	537540619 },
	{ "pl_fifthcurve_event",	764966851 },
	{ "pd_pit_of_death_event",	537319626 },

	// Campaign 3
	{ "cp_mossrock",			956975347 },
	{ "koth_lazarus",			922476326 },
	{ "plr_bananabay",			951657912 },
	{ "pl_enclosure_final",		851316292 },
	{ "koth_brazil",			649797811 },

	// Halloween 2018
	{ "koth_bagel_event",		1159639999 },
	{ "pl_rumble_event",		1142333364 },
	{ "koth_slasher",			782407483 },
	{ "pd_cursed_cove_event",	1498584149 },
	{ "pd_monster_bash",		1171267245 },

	// Halloween 2019
	{ "koth_slaughter_event", 		1872236402 },
	{ "pl_precipice_event_final", 	1822483095 },

	// Halloween 2020
	{ "koth_megalo", 			1164127973 },
	{ "pl_hasslecastle", 		1185816939 },
	{ "pl_bloodwater", 			766145891 },
	{ "koth_undergrove_event", 	1870343974 },

	// Smissmas 2020
	{ "pl_pier", 				454117739 },
	{ "pd_snowville_event", 	567055331 },
	{ "ctf_snowfall_final", 	1915450727 },
	{ "pl_wutville_event", 		816887895 },

	// Halloween 2021
	{ "pd_farmageddon", 		2237224308 },
	{ "koth_los_muertos", 		2588447761 },
	{ "cp_ambush_event", 		2140326607 },
	{ "pl_terror_event", 		2237031915 },
	{ "arena_lumberyard_event", 2590347649 },
	{ "koth_synthetic_event", 	1878543768 },

	// Smissmas 2021
	{ "pl_coal_event", 			2628069759 },
	{ "pl_breadspace", 			2243948848 },
	{ "pl_chilly", 				2646789704 },
	{ "koth_cascade", 			1133407330 },
	{ "cp_altitude", 			2642977253 },
	{ "ctf_doublecross_snowy",	2658301974 },

	// Halloween 2022
	{ "ctf_crasher", 			2858316394 },
	{ "ctf_helltrain_event", 	1452570835 },
	{ "pl_sludgepit_event", 	2550992151 },
	{ "cp_spookeyridge", 		1156801718 },
	{ "koth_sawmill_event", 	2845152717 },
	{ "plr_hacksaw_event",		2858395944 },

	// Smissmas 2022
	{ "cp_frostwatch", 			2643481899 },
	{ "pl_frostcliff", 			2885695341 },
	{ "pl_rumford_event", 		2886531763 },
	{ "ctf_frosty", 			1505740227 },
	{ "cp_gravelpit_snowy", 	2302187131 },

	// Summer 2023
	{ "koth_sharkbay", 			2404226979 },
	{ "koth_rotunda", 			2964421291 },
	{ "pl_phoenix", 			2969509359 },
	{ "pl_cashworks", 			2904065412 },
	{ "pl_venice", 				2969631229 },
	{ "cp_reckoner", 			674719999 },
	{ "cp_sulfur", 				619869471 },
	{ "cp_hardwood_final", 		2944867157 },
	{ "ctf_pelican_peak", 		2886563496 },
	{ "pd_selbyen", 			2889125525 },
	{ "vsh_tinyrock", 			2959976540 },
	{ "vsh_distillery", 		2967856987 },
	{ "vsh_skirmish", 			2965961179 },
	{ "vsh_nucleus", 			2973250677 },

	// Halloween 2023
	{ "arena_perks", 			3029918524 },
	{ "koth_slime", 			3028227103 },
	{ "cp_lavapit_final", 		2612605992 },
	{ "pd_mannsylvania", 		3029652598 },
	{ "cp_degrootkeep_rats", 	3031036719 },
	{ "pl_spineyard", 			3028181847 },
	{ "pl_corruption", 			2858934869 },
	{ "zi_murky", 				3030503176 },
	{ "zi_atoll", 				3030515121 },
	{ "zi_woods", 				3030549855 },
	{ "zi_sanitarium", 			3030548510 },
	{ "zi_devastation_final1", 	3031246748 },

	// Smissmas 2023
	{ "koth_snowtower",			2781286631 },
	{ "koth_krampus",			3063014386 },
	{ "ctf_haarp",				456481752 },
	{ "cp_brew",				2962755338 },
	{ "plr_hacksaw",			2885653656 },
	{ "ctf_turbine_winter",		2887605754 },
	{ "cp_carrier",				2888176898 },
	{ "pd_galleria",			3080331323 },
	{ "pl_emerge",				888027758 },
	{ "pl_camber",				1375766014 },

	// Summer 2024
	{ "pl_embargo",				3237930087 },
	{ "pl_odyssey",				3236834988 },
	{ "koth_megaton",			3237391660 },
	{ "koth_cachoeira",			3237877144 },
	{ "cp_overgrown",			503939302 },
	{ "cp_hadal",				804251853 },
	{ "ctf_applejack",			3219571335 },
	{ "pd_atom_smash",			2890208830 },
	{ "cp_canaveral_5cp",		2966121685 },
	{ "cp_burghausen",			454268748 },

	// Halloween 2024
	{ "koth_toxic",				3319147412 },
	{ "cp_darkmarsh",			2860559688 },
	{ "cp_freaky_fair",			3326591381 },
	{ "tow_dynamite",			3320549037 },
	{ "pd_circus",				3025095795 },
	{ "vsh_outburst",			3028713660 },
	{ "zi_blazehattan",			3031862054 },

	// Smissmas 2024
	{ "koth_overcast_final",	3089995488 },
	{ "cp_fortezza",			3358494285 },
	{ "ctf_penguin_peak",		2888338683 },
	{ "pl_patagonia",			3236427113 },
	{ "plr_cutter",				3363801747 },
	{ "vsh_maul",				3069796653 },

	// Summer 2025
	{ "pl_citadel",				3474587494 },
	{ "pl_aquarius",			3478583193 },
	{ "cp_fulgur",				2068252300 },
	{ "cp_cargo",				3488669143 },
	{ "cp_conifer",				1419048064 },
	{ "koth_boardwalk",			3475789229 },
	{ "koth_blowout",			3473248257 },
	{ "koth_mannhole",			3478225408 },
	{ "koth_demolition",		3473618662 },
	{ "ctf_pressure",			3480634190 },

	// Halloween 2025
	{ "cp_cowerhouse",			3028277335 },
	{ "koth_dusker",			3562630084 },
	{ "arena_afterlife",		3557320996 },
	{ "ctf_doublecross_event",	3024700002 },
	{ "sd_marshlands",			3565681202 },
};

*/

bool IsValveMap( const char *pMapName )
{
	for ( int i = 0; i < ARRAYSIZE( s_ValveMaps ); ++i )
	{
		if ( !Q_stricmp( s_ValveMaps[i].pDiskName, pMapName ) )
		{
			return true;
		}
	}
	return false;
}


bool IsCommunityMap( const char *pMapName )
{
	for ( int i = 0; i < ARRAYSIZE( s_CommunityMaps ); ++i )
	{
		if ( !Q_stricmp( s_CommunityMaps[i].pDiskName, pMapName ) )
		{
			return true;
		}
	}
	return false;
}

extern ConVar mp_capstyle;
extern ConVar sv_turbophysics;

extern ConVar tf_vaccinator_small_resist;
extern ConVar tf_vaccinator_uber_resist;

extern ConVar tf_teleporter_fov_time;
extern ConVar tf_teleporter_fov_start;

#ifdef GAME_DLL
extern ConVar mp_holiday_nogifts;
extern ConVar tf_debug_damage;
extern ConVar tf_damage_range;
extern ConVar tf_damage_disablespread;
extern ConVar tf_populator_damage_multiplier;
extern ConVar tf_mm_trusted;
extern ConVar tf_weapon_criticals;
extern ConVar tf_weapon_criticals_melee;
extern ConVar mp_idledealmethod;
extern ConVar mp_idlemaxtime;

extern ConVar tf_mm_strict;
extern ConVar mp_autoteambalance;

// STAGING_SPY
ConVar tf_feign_death_activate_damage_scale( "tf_feign_death_activate_damage_scale", "0.25", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_feign_death_damage_scale( "tf_feign_death_damage_scale", "0.35", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_stealth_damage_reduction( "tf_stealth_damage_reduction", "0.8", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

// training
ConVar training_class( "training_class", "3", FCVAR_REPLICATED, "Class to use in training." );
ConVar training_can_build_sentry( "training_can_build_sentry", "1", FCVAR_REPLICATED, "Player can build sentry as engineer." );
ConVar training_can_build_dispenser( "training_can_build_dispenser", "1", FCVAR_REPLICATED, "Player can build dispenser as engineer." );
ConVar training_can_build_tele_entrance( "training_can_build_tele_entrance", "1", FCVAR_REPLICATED, "Player can build teleporter entrance as engineer." );
ConVar training_can_build_tele_exit( "training_can_build_tele_exit", "1", FCVAR_REPLICATED, "Player can build teleporter exit as engineer." );
ConVar training_can_destroy_buildings( "training_can_destroy_buildings", "1", FCVAR_REPLICATED, "Player can destroy buildings as engineer." );
ConVar training_can_pickup_sentry( "training_can_pickup_sentry", "1", FCVAR_REPLICATED, "Player can pickup sentry gun as engineer." );
ConVar training_can_pickup_dispenser( "training_can_pickup_dispenser", "1", FCVAR_REPLICATED, "Player can pickup dispenser as engineer." );
ConVar training_can_pickup_tele_entrance( "training_can_pickup_tele_entrance", "1", FCVAR_REPLICATED, "Player can pickup teleporter entrance as engineer." );
ConVar training_can_pickup_tele_exit( "training_can_pickup_tele_exit", "1", FCVAR_REPLICATED, "Player can pickup teleporter entrance as engineer." );
ConVar training_can_select_weapon_primary	( "training_can_select_weapon_primary", "1", FCVAR_REPLICATED, "In training player select primary weapon." );
ConVar training_can_select_weapon_secondary	( "training_can_select_weapon_secondary", "1", FCVAR_REPLICATED, "In training player select secondary weapon." );
ConVar training_can_select_weapon_melee		( "training_can_select_weapon_melee", "1", FCVAR_REPLICATED, "In training player select melee weapon." );
ConVar training_can_select_weapon_building	( "training_can_select_weapon_building", "1", FCVAR_REPLICATED, "In training player select building tool." );
ConVar training_can_select_weapon_pda		( "training_can_select_weapon_pda", "1", FCVAR_REPLICATED, "In training player select pda." );
ConVar training_can_select_weapon_item1		( "training_can_select_weapon_item1", "1", FCVAR_REPLICATED, "In training player select item 1." );
ConVar training_can_select_weapon_item2		( "training_can_select_weapon_item2", "1", FCVAR_REPLICATED, "In training player select item 2." );

ConVar tf_birthday_ball_chance( "tf_birthday_ball_chance", "100", FCVAR_REPLICATED, "Percent chance of a birthday beach ball spawning at each round start" );

ConVar tf_halloween_boss_spawn_interval( "tf_halloween_boss_spawn_interval", "480", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_boss_spawn_interval_variation( "tf_halloween_boss_spawn_interval_variation", "60", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_eyeball_boss_spawn_interval( "tf_halloween_eyeball_boss_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_eyeball_boss_spawn_interval_variation( "tf_halloween_eyeball_boss_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_merasmus_spawn_interval( "tf_merasmus_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_merasmus_spawn_interval_variation( "tf_merasmus_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_zombie_mob_enabled( "tf_halloween_zombie_mob_enabled", "0", FCVAR_CHEAT, "If set to 1, spawn zombie mobs on non-Halloween Valve maps" );
ConVar tf_halloween_zombie_mob_spawn_interval( "tf_halloween_zombie_mob_spawn_interval", "180", FCVAR_CHEAT, "Average interval between zombie mob spawns, in seconds" );
ConVar tf_halloween_zombie_mob_spawn_count( "tf_halloween_zombie_mob_spawn_count", "20", FCVAR_CHEAT, "How many zombies to spawn" );

ConVar tf_halloween_allow_truce_during_boss_event( "tf_halloween_allow_truce_during_boss_event", "0", FCVAR_NOTIFY, "Determines if RED and BLU can damage each other while fighting Monoculus or Merasmus on non-Valve maps." );

ConVar tf_player_spell_drop_on_death_rate( "tf_player_spell_drop_on_death_rate", "0", FCVAR_REPLICATED );
ConVar tf_player_drop_bonus_ducks( "tf_player_drop_bonus_ducks", "-1", FCVAR_REPLICATED, "-1 Default (Holiday-based)\n0 - Force off\n1 - Force on" );

ConVar tf_allow_player_name_change( "tf_allow_player_name_change", "1", FCVAR_NOTIFY, "Allow player name changes." );

ConVar tf_weapon_criticals_distance_falloff( "tf_weapon_criticals_distance_falloff", "0", FCVAR_CHEAT, "Critical weapon damage will take distance into account." );
ConVar tf_weapon_minicrits_distance_falloff( "tf_weapon_minicrits_distance_falloff", "0", FCVAR_CHEAT, "Mini-crit weapon damage will take distance into account." );

ConVar mp_spectators_restricted( "mp_spectators_restricted", "0", FCVAR_NONE, "Prevent players on game teams from joining team spectator if it would unbalance the teams." );

ConVar tf_test_special_ducks( "tf_test_special_ducks", "1", FCVAR_DEVELOPMENTONLY );

ConVar tf_mm_abandoned_players_per_team_max( "tf_mm_abandoned_players_per_team_max", "1", FCVAR_DEVELOPMENTONLY );
#endif // GAME_DLL
ConVar tf_mm_next_map_vote_time( "tf_mm_next_map_vote_time", "15", FCVAR_REPLICATED );


static float g_fEternaweenAutodisableTime = 0.0f;

ConVar tf_spec_xray( "tf_spec_xray", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows spectators to see player glows. 1 = same team, 2 = both teams" );
ConVar tf_spawn_glows_duration( "tf_spawn_glows_duration", "10", FCVAR_NOTIFY | FCVAR_REPLICATED, "How long should teammates glow after respawning\n" );

#ifdef GAME_DLL
void cc_tf_forced_holiday_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	// Tell the listeners to recalculate the holiday
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}
#endif // GAME_DLL

ConVar tf_forced_holiday( "tf_forced_holiday", "0", FCVAR_REPLICATED, "Forced holiday, \n   Birthday = 1\n   Halloween = 2\n" //  Christmas = 3\n   Valentines = 4\n   MeetThePyro = 5\n   FullMoon=6
#ifdef GAME_DLL
, cc_tf_forced_holiday_changed
#endif // GAME_DLL
);
ConVar tf_item_based_forced_holiday( "tf_item_based_forced_holiday", "0", FCVAR_REPLICATED, "" 	// like a clone of tf_forced_holiday, but controlled by client consumable item use
#ifdef GAME_DLL
	, cc_tf_forced_holiday_changed
#endif // GAME_DLL
);
ConVar tf_force_holidays_off( "tf_force_holidays_off", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, ""
#ifdef GAME_DLL
, cc_tf_forced_holiday_changed
#endif // GAME_DLL
);
ConVar tf_birthday( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_spells_enabled( "tf_spells_enabled", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Enable to Allow Halloween Spells to be dropped and used by players" );

ConVar tf_caplinear( "tf_caplinear", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time that players are allowed to change class in stalemates." );
ConVar mp_tournament_redteamname( "mp_tournament_redteamname", "RED", FCVAR_REPLICATED | FCVAR_HIDDEN );
ConVar mp_tournament_blueteamname( "mp_tournament_blueteamname", "BLU", FCVAR_REPLICATED | FCVAR_HIDDEN );

ConVar tf_attack_defend_map( "tf_attack_defend_map", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

#ifdef GAME_DLL
void cc_tf_stopwatch_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "stop_watch_changed" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}
#endif // GAME_DLL
ConVar mp_tournament_stopwatch( "mp_tournament_stopwatch", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Use Stopwatch mode while using Tournament mode (mp_tournament)"
#ifdef GAME_DLL
	, cc_tf_stopwatch_changed 
#endif
);
ConVar mp_tournament_readymode( "mp_tournament_readymode", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable per-player ready status for tournament mode." );
ConVar mp_tournament_readymode_min( "mp_tournament_readymode_min", "2", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required on the server before players can toggle ready status." );
ConVar mp_tournament_readymode_team_size( "mp_tournament_readymode_team_size", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required to be ready per-team before the game can begin." );
ConVar mp_tournament_readymode_countdown( "mp_tournament_readymode_countdown", "10", FCVAR_REPLICATED | FCVAR_NOTIFY, "The number of seconds before a match begins when both teams are ready." );
#ifdef GAME_DLL
ConVar mp_tournament_prevent_team_switch_on_readyup( "mp_tournament_prevent_team_switch_on_readyup", "1", FCVAR_NONE, "Prevent switching teams on ready-up for subsequent rounds in tournament mode." );
#endif

ConVar mp_windifference( "mp_windifference", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Score difference between teams before server changes maps", true, 0, false, 0 );
ConVar mp_windifference_min( "mp_windifference_min", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum score needed for mp_windifference to be applied", true, 0, false, 0 );

ConVar tf_tournament_classlimit_scout( "tf_tournament_classlimit_scout", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Scouts.\n" );
ConVar tf_tournament_classlimit_sniper( "tf_tournament_classlimit_sniper", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Snipers.\n" );
ConVar tf_tournament_classlimit_soldier( "tf_tournament_classlimit_soldier", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Soldiers.\n" );
ConVar tf_tournament_classlimit_demoman( "tf_tournament_classlimit_demoman", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Demomenz.\n" );
ConVar tf_tournament_classlimit_medic( "tf_tournament_classlimit_medic", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Medics.\n" );
ConVar tf_tournament_classlimit_heavy( "tf_tournament_classlimit_heavy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Heavies.\n" );
ConVar tf_tournament_classlimit_pyro( "tf_tournament_classlimit_pyro", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Pyros.\n" );
ConVar tf_tournament_classlimit_spy( "tf_tournament_classlimit_spy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Spies.\n" );
ConVar tf_tournament_classlimit_engineer( "tf_tournament_classlimit_engineer", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Engineers.\n" );
ConVar tf_tournament_classchange_allowed( "tf_tournament_classchange_allowed", "1", FCVAR_REPLICATED, "Allow players to change class while the game is active?.\n" );
ConVar tf_tournament_classchange_ready_allowed( "tf_tournament_classchange_ready_allowed", "1", FCVAR_REPLICATED, "Allow players to change class after they are READY?.\n" );

ConVar tf_classlimit( "tf_classlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Limit on how many players can be any class (i.e. tf_class_limit 2 would limit 2 players per class).\n", true, 0.f, false, 0.f );
ConVar tf_player_movement_restart_freeze( "tf_player_movement_restart_freeze", "1", FCVAR_REPLICATED, "When set, prevent player movement during round restart" );

ConVar tf_autobalance_ask_candidates_maxtime( "tf_autobalance_ask_candidates_maxtime", "10", FCVAR_REPLICATED );
ConVar tf_autobalance_dead_candidates_maxtime( "tf_autobalance_dead_candidates_maxtime", "15", FCVAR_REPLICATED );
ConVar tf_autobalance_force_candidates_maxtime( "tf_autobalance_force_candidates_maxtime", "5", FCVAR_REPLICATED );
ConVar tf_autobalance_xp_bonus( "tf_autobalance_xp_bonus", "500", FCVAR_REPLICATED );


#ifdef GAME_DLL

static const float g_flStrangeEventBatchProcessInterval = 30.0f;

ConVar mp_humans_must_join_team("mp_humans_must_join_team", "any", FCVAR_REPLICATED, "Restricts human players to a single team {any, blue, red, spectator}" );

void cc_tf_medieval_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
	bool bOldValue = flOldValue > 0;
	if ( var.IsValid() && ( bOldValue != var.GetBool() ) )
	{
		Msg( "Medieval mode changes take effect after the next map change.\n" );
	}
}

#endif
ConVar tf_medieval( "tf_medieval", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable Medieval Mode.\n", true, 0, true, 1
#ifdef GAME_DLL
				   , cc_tf_medieval_changed
#endif 
				    );

ConVar tf_medieval_autorp( "tf_medieval_autorp", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable Medieval Mode auto-roleplaying.\n", true, 0, true, 1 );

ConVar tf_sticky_radius_ramp_time( "tf_sticky_radius_ramp_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT | FCVAR_REPLICATED, "Amount of time to get full radius after arming" );
ConVar tf_sticky_airdet_radius( "tf_sticky_airdet_radius", "0.85", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT | FCVAR_REPLICATED, "Radius Scale if detonated in the air" );


#ifndef GAME_DLL
extern ConVar cl_burninggibs;
extern ConVar english;
ConVar tf_particles_disable_weather( "tf_particles_disable_weather", "0", FCVAR_ARCHIVE, "Disable particles related to weather effects." );
#endif

// arena mode cvars
ConVar tf_arena_force_class( "tf_arena_force_class", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Forces players to play a random class each time they spawn." );
ConVar tf_arena_change_limit( "tf_arena_change_limit", "1", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Number of times players can change their class when mp_force_random_class is being used." );
ConVar tf_arena_override_cap_enable_time( "tf_arena_override_cap_enable_time", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Overrides the time (in seconds) it takes for the capture point to become enable, -1 uses the level designer specified time." );
ConVar tf_arena_override_team_size( "tf_arena_override_team_size", "0", FCVAR_REPLICATED, "Overrides the maximum team size in arena mode. Set to zero to keep the default behavior of 1/3 maxplayers.");
ConVar tf_arena_first_blood( "tf_arena_first_blood", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Rewards the first player to get a kill each round." );
extern ConVar tf_arena_preround_time;
extern ConVar tf_arena_max_streak;
#if defined( _DEBUG ) || defined( STAGING_ONLY )
extern ConVar mp_developer;
#endif // _DEBUG || STAGING_ONLY

//=============================================================================
// HPE_BEGIN
// [msmith] Used for the client to tell the server that we're watching a movie or not.
//			Also contains the name of a movie if it's an in game video.
//=============================================================================
// Training mode cvars
ConVar tf_training_client_message( "tf_training_client_message", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "A simple way for the training client to communicate with the server." );
//=============================================================================
// HPE_END
//=============================================================================

#define TF_ARENA_MODE_FIRST_BLOOD_CRIT_TIME 5.0f
#define TF_ARENA_MODE_FAST_FIRST_BLOOD_TIME 20.0f
#define TF_ARENA_MODE_SLOW_FIRST_BLOOD_TIME 50.0f

#ifdef TF_RAID_MODE
// Raid mode
ConVar tf_gamemode_raid( "tf_gamemode_raid", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );		// client needs access to this for IsRaidMode()
ConVar tf_raid_enforce_unique_classes( "tf_raid_enforce_unique_classes", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar tf_raid_respawn_time( "tf_raid_respawn_time", "5", FCVAR_REPLICATED | FCVAR_NOTIFY /*| FCVAR_CHEAT*/, "How long it takes for a Raider to respawn with his team after death." );
ConVar tf_raid_allow_all_classes( "tf_raid_allow_all_classes", "1", FCVAR_REPLICATED | FCVAR_NOTIFY );

ConVar tf_gamemode_boss_battle( "tf_gamemode_boss_battle", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );

#ifdef GAME_DLL
ConVar tf_raid_allow_overtime( "tf_raid_allow_overtime", "0"/*, FCVAR_CHEAT*/ );
#endif // GAME_DLL
#endif // TF_RAID_MODE

ConVar tf_mvm_defenders_team_size( "tf_mvm_defenders_team_size", "6", FCVAR_REPLICATED | FCVAR_NOTIFY, "Maximum number of defenders in MvM" );
ConVar tf_mvm_max_connected_players( "tf_mvm_max_connected_players", "10", FCVAR_GAMEDLL, "Maximum number of connected real players in MvM" );
ConVar tf_mvm_max_invaders( "tf_mvm_max_invaders", "22", FCVAR_GAMEDLL, "Maximum number of invaders in MvM" );

ConVar tf_mvm_min_players_to_start( "tf_mvm_min_players_to_start", "3", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players connected to start a countdown timer" );
ConVar tf_mvm_respec_enabled( "tf_mvm_respec_enabled", "1", FCVAR_CHEAT | FCVAR_REPLICATED, "Allow players to refund credits spent on player and item upgrades." );
ConVar tf_mvm_respec_limit( "tf_mvm_respec_limit", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "The total number of respecs a player can earn.  Default: 0 (no limit).", true, 0.f, true, 100.f );
ConVar tf_mvm_respec_credit_goal( "tf_mvm_respec_credit_goal", "2000", FCVAR_CHEAT | FCVAR_REPLICATED, "When tf_mvm_respec_limit is non-zero, the total amount of money the team must collect to earn a respec credit." );
ConVar tf_mvm_buybacks_method( "tf_mvm_buybacks_method", "0", FCVAR_REPLICATED | FCVAR_HIDDEN, "When set to 0, use the traditional, currency-based system.  When set to 1, use finite, charge-based system.", true, 0.0, true, 1.0 );
ConVar tf_mvm_buybacks_per_wave( "tf_mvm_buybacks_per_wave", "3", FCVAR_REPLICATED | FCVAR_HIDDEN, "The fixed number of buybacks players can use per-wave." );


#ifdef GAME_DLL
enum { kMVM_CurrencyPackMinSize = 1, };
#endif // GAME_DLL

extern ConVar mp_tournament;
extern ConVar mp_tournament_post_match_period;

extern ConVar tf_flag_return_on_touch;
extern ConVar tf_flag_return_time_credit_factor;
ConVar tf_grapplinghook_enable( "tf_grapplinghook_enable", "0", FCVAR_REPLICATED );

#ifdef GAME_DLL
CUtlString s_strNextMvMPopFile;
CON_COMMAND_F( tf_mvm_popfile, "Change to a target popfile for MvM", FCVAR_GAMEDLL )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() <= 1 )
	{
		if ( TFGameRules() && g_pPopulationManager )
		{
			const char *pszFile = g_pPopulationManager->GetPopulationFilename();
			if ( pszFile && pszFile[0] )
			{
				Msg( "Current popfile is: %s\n", pszFile );
				return;
			}
		}

		Msg( "Missing Popfile name\n" );
		return;
	}

	const char *pszShortName = args.Arg(1);

	if ( !TFGameRules() || !g_pPopulationManager )
	{
		Warning( "Cannot set population file before map load.\n" );
		return;
	}

	// Make sure we have a file system
	if ( !g_pFullFileSystem )
	{
		Msg( "No File System to find Popfile to load\n" );
		return;
	}

	// Form full path
	CUtlString fullPath;

	if ( g_pPopulationManager->FindPopulationFileByShortName( pszShortName, fullPath ) && g_pPopulationManager->IsValidPopfile( fullPath ) )
	{
		g_pPopulationManager->SetPopulationFilename( fullPath );
		g_pPopulationManager->ResetMap();
		return;
	}

	// Give them a message to make it clear what file we were looking for
	Warning( "Could not find a valid population file matching: %s.\n", pszShortName );
}

#endif

static bool BIsCvarIndicatingHolidayIsActive( int iCvarValue, /*EHoliday*/ int eHoliday )
{
	if ( iCvarValue == 0 )
		return false;

	// Unfortunately Holidays are not a proper bitfield
	switch ( eHoliday )
	{
	case kHoliday_TFBirthday:						return iCvarValue == kHoliday_TFBirthday;
	case kHoliday_Halloween:						return iCvarValue == kHoliday_Halloween || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
	case kHoliday_Christmas:						return iCvarValue == kHoliday_Christmas;
	case kHoliday_Valentines:						return iCvarValue == kHoliday_Valentines || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
	case kHoliday_MeetThePyro:						return iCvarValue == kHoliday_MeetThePyro;
	case kHoliday_FullMoon:							return iCvarValue == kHoliday_FullMoon || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
	case kHoliday_HalloweenOrFullMoon:				return iCvarValue == kHoliday_Halloween || iCvarValue == kHoliday_FullMoon || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
	case kHoliday_HalloweenOrFullMoonOrValentines:	return iCvarValue == kHoliday_Halloween || iCvarValue == kHoliday_FullMoon || iCvarValue == kHoliday_Valentines || iCvarValue == kHoliday_HalloweenOrFullMoon || iCvarValue == kHoliday_HalloweenOrFullMoonOrValentines;
	case kHoliday_AprilFools:						return iCvarValue == kHoliday_AprilFools;
	case kHoliday_EOTL:								return iCvarValue == kHoliday_EOTL;
	case kHoliday_CommunityUpdate:					return iCvarValue == kHoliday_CommunityUpdate;
	case kHoliday_Soldier:							return iCvarValue == kHoliday_Soldier;
	case kHoliday_Summer:							return iCvarValue == kHoliday_Summer;
	}

	return false;
}

#ifdef GAME_DLL
bool IsCustomGameMode( const char *pszMapName )
{
	return ( MapHasPrefix( pszMapName, "vsh_" ) || MapHasPrefix( pszMapName, "zi_" ) );
}

bool IsCustomGameMode()
{
	return IsCustomGameMode( STRING( gpGlobals->mapname ) );
}
#endif

// Fetch holiday setting taking into account convars, etc, but NOT
// taking into consideration the current game rules, map, etc.
//
// This version can be used outside of gameplay, ie., for matchmaking
bool TF_IsHolidayActive( /*EHoliday*/ int eHoliday )
{
	if ( IsX360() || tf_force_holidays_off.GetBool() )
		return false;

	if ( BIsCvarIndicatingHolidayIsActive( tf_forced_holiday.GetInt(), eHoliday ) )
		return true;

	if ( BIsCvarIndicatingHolidayIsActive( tf_item_based_forced_holiday.GetInt(), eHoliday ) )
		return true;

	if ( ( eHoliday == kHoliday_TFBirthday ) && tf_birthday.GetBool() )
		return true;

	if ( TFGameRules() )
	{
		if ( eHoliday == kHoliday_HalloweenOrFullMoon )
		{
			if ( TFGameRules()->IsHolidayMap( kHoliday_Halloween ) )
				return true;
			if ( TFGameRules()->IsHolidayMap( kHoliday_FullMoon ) )
				return true;
		}
		if ( TFGameRules()->IsHolidayMap( eHoliday ) )
		{
			return true;
		}
	}

	return UTIL_IsHolidayActive( eHoliday );
}

#ifdef CLIENT_DLL
bool BInEndOfMatch()
{
	const bool bInEndOfMatch = TFGameRules() &&
		TFGameRules()->State_Get() == GR_STATE_GAME_OVER &&
		GTFGCClientSystem()->BConnectedToMatchServer( false );

	return bInEndOfMatch;
}
#endif

#ifdef TF_CREEP_MODE
ConVar tf_gamemode_creep_wave( "tf_gamemode_creep_wave", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar tf_creep_wave_player_respawn_time( "tf_creep_wave_player_respawn_time", "10", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_CHEAT, "How long it takes for a player to respawn with his team after death." );
#endif

#ifdef GAME_DLL
// TF overrides the default value of this convar

#ifdef _DEBUG
#define WAITING_FOR_PLAYERS_FLAGS	0
#else
#define WAITING_FOR_PLAYERS_FLAGS	FCVAR_DEVELOPMENTONLY
#endif

ConVar hide_server( "hide_server", "0", FCVAR_GAMEDLL, "Whether the server should be hidden from the master server" );

ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", (IsX360()?"15":"30"), FCVAR_GAMEDLL | WAITING_FOR_PLAYERS_FLAGS, "WaitingForPlayers time length in seconds" );

ConVar tf_gamemode_arena ( "tf_gamemode_arena", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_cp ( "tf_gamemode_cp", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_ctf ( "tf_gamemode_ctf", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_sd ( "tf_gamemode_sd", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_rd ( "tf_gamemode_rd", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_pd ( "tf_gamemode_pd", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_tc ( "tf_gamemode_tc", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_beta_content ( "tf_beta_content", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_payload ( "tf_gamemode_payload", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_mvm ( "tf_gamemode_mvm", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_passtime ( "tf_gamemode_passtime", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
ConVar tf_gamemode_misc ( "tf_gamemode_misc", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

ConVar tf_bot_count( "tf_bot_count", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

#ifdef _DEBUG
ConVar tf_debug_ammo_and_health( "tf_debug_ammo_and_health", "0", FCVAR_CHEAT );
#endif // _DEBUG

static Vector s_BotSpawnPosition;

ConVar tf_gravetalk( "tf_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat.", true, 0, true, 1 );

ConVar tf_ctf_bonus_time ( "tf_ctf_bonus_time", "10", FCVAR_NOTIFY, "Length of team crit time for CTF capture." );

#ifdef _DEBUG
ConVar mp_scrambleteams_debug( "mp_scrambleteams_debug", "0", FCVAR_NONE, "Debug spew." );
#endif // _DEBUG


extern ConVar tf_mm_servermode;
extern ConVar tf_flag_caps_per_round;

void cc_competitive_mode( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "competitive_state_changed" );
	if ( event )
	{
		// Server-side here.  Client-side down below in the RecvProxy
		gameeventmanager->FireEvent( event, true );
	}
}
ConVar tf_competitive_preround_duration( "tf_competitive_preround_duration", "3", FCVAR_REPLICATED, "How long we stay in pre-round when in competitive games." );
ConVar tf_competitive_preround_countdown_duration( "tf_competitive_preround_countdown_duration", "10.5", FCVAR_HIDDEN, "How long we stay in countdown when in competitive games." );
ConVar tf_competitive_abandon_method( "tf_competitive_abandon_method", "0", FCVAR_HIDDEN );
ConVar tf_competitive_required_late_join_timeout( "tf_competitive_required_late_join_timeout", "120", FCVAR_DEVELOPMENTONLY,
                                                  "How long to wait for late joiners in matches requiring full player counts before canceling the match" );
ConVar tf_competitive_required_late_join_confirm_timeout( "tf_competitive_required_late_join_confirm_timeout", "30", FCVAR_DEVELOPMENTONLY,
                                                          "How long to wait for the GC to confirm we're in the late join pool before canceling the match" );
#endif // GAME_DLL

ConVar tf_gamemode_community ( "tf_gamemode_community", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );

ConVar tf_voice_command_suspension_mode( "tf_voice_command_suspension_mode", "2", FCVAR_REPLICATED, "0 = None | 1 = No Voice Commands | 2 = Rate Limited" );

#ifdef GAME_DLL

ConVar tf_voice_command_suspension_rate_limit_bucket_count( "tf_voice_command_suspension_rate_limit_bucket_count", "5" ); // Bucket size of 5.
ConVar tf_voice_command_suspension_rate_limit_bucket_refill_rate( "tf_voice_command_suspension_rate_limit_bucket_refill_rate", "6" ); // 6s

void cc_powerup_mode( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
	if ( var.IsValid() )
	{
		if ( !TFGameRules() )
			return;

		if ( var.GetBool() )
		{
			if ( TFGameRules()->IsMannVsMachineMode() )
				return;
		}

		TFGameRules()->SetPowerupMode( var.GetBool() );
		TFGameRules()->State_Transition( GR_STATE_PREROUND );
		tf_flag_caps_per_round.SetValue( var.GetBool() ? 7 : 3 );	// Hack
	}
}

ConVar tf_powerup_mode( "tf_powerup_mode", "0", FCVAR_NOTIFY, "Enable/disable powerup mode. Not compatible with Mann Vs Machine mode", cc_powerup_mode );
ConVar tf_powerup_mode_imbalance_delta( "tf_powerup_mode_imbalance_delta", "24", FCVAR_REPLICATED, "Powerup kill score lead one team must have before imbalance measures are initiated" );
ConVar tf_powerup_mode_imbalance_consecutive_min_players( "tf_powerup_mode_imbalance_consecutive_min_players", "10", FCVAR_REPLICATED, "Minimum number of players on the server before consecutive imbalance measures trigger team balancing" );
ConVar tf_powerup_mode_imbalance_consecutive_time( "tf_powerup_mode_imbalance_consecutive_time", "1200", FCVAR_REPLICATED, "Teams are balanced if consecutive imbalance measures for the same team are triggered in less time (seconds)" );
ConVar tf_powerup_mode_dominant_multiplier( "tf_powerup_mode_dominant_multiplier", "3", FCVAR_REPLICATED, "The multiple by which a player must exceed the median kills by in order to be considered dominant" );
ConVar tf_powerup_mode_killcount_timer_length( "tf_powerup_mode_killcount_timer_length", "300", FCVAR_REPLICATED, "How long to wait between kill count tests that determine if a player is dominating" ); //should be a multiple of 60 because we use this to calculate an integer

ConVar tf_skillrating_update_interval( "tf_skillrating_update_interval", "180", FCVAR_ARCHIVE, "How often to update the GC and OGS." );

extern ConVar mp_teams_unbalance_limit;

static bool g_bRandomMap = false;

void cc_RandomMap( const CCommand& args )
{
	CTFGameRules *pRules = TFGameRules();
	if ( pRules )
	{
		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;

		g_bRandomMap = true;
	}
	else
	{
		// There's no game rules object yet, so let's load the map cycle and pick a map.
		char mapcfile[MAX_PATH];
		CMultiplayRules::DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), true );
		if ( !mapcfile[0] )
		{
			Msg( "No mapcyclefile specified. Cannot pick a random map.\n" );
			return;
		}

		CUtlVector<char*> mapList;
		// No gamerules entity yet, since we don't need the fixups to find a map just use the base version
		CMultiplayRules::RawLoadMapCycleFileIntoVector ( mapcfile, mapList );
		if ( !mapList.Count() )
		{
			Msg( "Map cycle file \"%s\" contains no valid maps or cannot be read. Cannot pick a random map.\n", mapcfile );
			return;
		}

		int iMapIndex = RandomInt( 0, mapList.Count() - 1 );
		Msg ( "randommap: selecting map %i out of %i\n", iMapIndex + 1, mapList.Count() );
		engine->ServerCommand( UTIL_VarArgs( "map %s\n", mapList[iMapIndex] ) );

		CMultiplayRules::FreeMapCycleFileVector( mapList );
	}
}

// Simple class for tracking previous gamemode across level transitions
// Allows clean-up of UI/state when going between things like MvM and PvP
class CTFGameModeHistory : public CAutoGameSystem
{
public:
	virtual bool Init()
	{
		m_nPrevState = 0;
		return true;
	}
	void SetPrevState( int nState ) { m_nPrevState = nState; }
	int	 GetPrevState( void ) { return m_nPrevState; }
private:
	int m_nPrevState;
} g_TFGameModeHistory;

static ConCommand randommap( "randommap", cc_RandomMap, "Changelevel to a random map in the mapcycle file" );
#endif	// GAME_DLL

#ifdef GAME_DLL
static bool PlayerHasDuckStreaks( CTFPlayer *pPlayer )
{
	CEconItemView *pActionItem = pPlayer->GetEquippedItemForLoadoutSlot( LOADOUT_POSITION_ACTION );
	if ( !pActionItem )
		return false;

	// Duck Badge Cooldown is based on badge level.  Noisemaker is more like an easter egg
	static CSchemaAttributeDefHandle pAttr_DuckStreaks( "duckstreaks active" );
	uint32 iDuckStreaksActive = 0;

	// Don't care about the level, just if the attribute is found
	return FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pActionItem, pAttr_DuckStreaks, &iDuckStreaksActive ) && iDuckStreaksActive > 0;
}

void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
}

static void Coaching_Start( CTFPlayer *pCoach, CTFPlayer *pStudent )
{
	pCoach->SetIsCoaching( true );
	pCoach->ForceChangeTeam( TEAM_SPECTATOR );
	pCoach->SetObserverTarget( pStudent );
	pCoach->StartObserverMode( OBS_MODE_CHASE );
	pCoach->SetStudent( pStudent );
	pStudent->SetCoach( pCoach );
}

static void Coaching_Stop( CTFPlayer *pCoach )
{
	CTFPlayer *pStudent = pCoach->GetStudent();
	if ( pStudent )
	{
		pStudent->SetCoach( NULL );
	}
	pCoach->SetIsCoaching( false );
	pCoach->SetStudent( NULL );
	pCoach->ForceChangeTeam( TEAM_SPECTATOR );
}

#endif	

ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of captures per round on CTF and PASS Time maps. Set to 0 to disable.", true, 0, false, 0
#ifdef GAME_DLL
							  , ValidateCapturesPerRound
#endif
							  );


/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),		//VEC_VIEW (m_vView) eye position
							
	Vector(-24, -24, 0 ),	//VEC_HULL_MIN (m_vHullMin) hull min
	Vector( 24,  24, 82 ),	//VEC_HULL_MAX (m_vHullMax) hull max
												
	Vector(-24, -24, 0 ),	//VEC_DUCK_HULL_MIN (m_vDuckHullMin) duck hull min
	Vector( 24,  24, 62 ),	//VEC_DUCK_HULL_MAX	(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),		//VEC_DUCK_VIEW		(m_vDuckView) duck view
												
	Vector( -10, -10, -10 ),	//VEC_OBS_HULL_MIN	(m_vObsHullMin) observer hull min
	Vector(  10,  10,  10 ),	//VEC_OBS_HULL_MAX	(m_vObsHullMax) observer hull max
												
	Vector( 0, 0, 14 )		//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight) dead view height
);							

Vector g_TFClassViewVectors[11] =
{
	Vector( 0, 0, 72 ),		// TF_CLASS_UNDEFINED

	Vector( 0, 0, 65 ),		// TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
	Vector( 0, 0, 75 ),		// TF_CLASS_SNIPER,
	Vector( 0, 0, 68 ),		// TF_CLASS_SOLDIER,
	Vector( 0, 0, 68 ),		// TF_CLASS_DEMOMAN,
	Vector( 0, 0, 75 ),		// TF_CLASS_MEDIC,
	Vector( 0, 0, 75 ),		// TF_CLASS_HEAVYWEAPONS,
	Vector( 0, 0, 68 ),		// TF_CLASS_PYRO,
	Vector( 0, 0, 75 ),		// TF_CLASS_SPY,
	Vector( 0, 0, 68 ),		// TF_CLASS_ENGINEER,

	Vector( 0, 0, 65 ),		// TF_CLASS_CIVILIAN,		// TF_LAST_NORMAL_CLASS
};

const CViewVectors *CTFGameRules::GetViewVectors() const
{
	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTFGameRules );

#ifdef CLIENT_DLL
void RecvProxy_MatchSummary( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	bool bMatchSummary = ( pData->m_Value.m_Int > 0 );
	if ( bMatchSummary && !(*(bool*)(pOut)))
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->TurnOffTauntCam();
			pLocalPlayer->TurnOffTauntCam_Finish();
		}
	}

	*(bool*)(pOut) = bMatchSummary;
}

void RecvProxy_CompetitiveMode( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*(bool*)(pOut) = ( pData->m_Value.m_Int > 0 );

	IGameEvent *event = gameeventmanager->CreateEvent( "competitive_state_changed" );
	if ( event )
	{
		// Client-side once it's actually happened
		gameeventmanager->FireEventClientSide( event );
	}
}

void RecvProxy_PlayerVotedForMap( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	if ( *(int*)(pOut) != pData->m_Value.m_Int )
	{
		*(int*)(pOut) = pData->m_Value.m_Int;

		IGameEvent *event = gameeventmanager->CreateEvent( "player_next_map_vote_change" );
		if ( event )
		{
			event->SetInt( "map_index", pData->m_Value.m_Int );
			event->SetInt( "vote", pData->m_Value.m_Int );
			// Client-side once it's actually happened
			gameeventmanager->FireEventClientSide( event );
		}
	}
}

void RecvProxy_NewMapVoteStateChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	bool bChange = *(int*)(pOut) != pData->m_Value.m_Int;
	*(int*)(pOut) = pData->m_Value.m_Int;

	if ( bChange )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "vote_maps_changed" );
		if ( event )
		{
			// Client-side once it's actually happened
			gameeventmanager->FireEventClientSide( event );
		}
	}
}
#endif

BEGIN_NETWORK_TABLE_NOBASE( CTFGameRules, DT_TFGameRules )
#ifdef CLIENT_DLL

	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropInt( RECVINFO( m_nStopWatchState ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropTime( RECVINFO( m_flCapturePointEnableTime ) ),

//=============================================================================
// HPE_BEGIN:
// [msmith]	Training status and HUD Type.
//=============================================================================
	RecvPropInt( RECVINFO( m_nHudType ) ),
	RecvPropBool( RECVINFO( m_bIsInTraining ) ),
	RecvPropBool( RECVINFO( m_bAllowTrainingAchievements ) ),
	RecvPropBool( RECVINFO( m_bIsWaitingForTrainingContinue ) ),
//=============================================================================
// HPE_END
//=============================================================================
	RecvPropBool( RECVINFO( m_bIsTrainingHUDVisible ) ),
	RecvPropBool( RECVINFO( m_bIsInItemTestingMode ) ),

	RecvPropEHandle( RECVINFO( m_hBonusLogic ) ),
	RecvPropBool( RECVINFO( m_bPlayingKoth ) ),
	RecvPropBool( RECVINFO( m_bPlayingMedieval ) ),
	RecvPropBool( RECVINFO( m_bPlayingHybrid_CTF_CP ) ),
	RecvPropBool( RECVINFO( m_bPlayingSpecialDeliveryMode ) ),
	RecvPropBool( RECVINFO( m_bPlayingRobotDestructionMode ) ),
	RecvPropEHandle( RECVINFO( m_hRedKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_hBlueKothTimer ) ),
	RecvPropInt( RECVINFO( m_nMapHolidayType ) ),

	RecvPropEHandle( RECVINFO( m_itHandle ) ),
	RecvPropBool( RECVINFO( m_bPlayingMannVsMachine ) ),
	RecvPropEHandle( RECVINFO( m_hBirthdayPlayer ) ),

	RecvPropInt( RECVINFO( m_nBossHealth ) ),
	RecvPropInt( RECVINFO( m_nMaxBossHealth ) ),
	RecvPropInt( RECVINFO( m_fBossNormalizedTravelDistance ) ),
	RecvPropBool( RECVINFO( m_bMannVsMachineAlarmStatus ) ),
	RecvPropBool( RECVINFO( m_bHaveMinPlayersToEnableReady ) ),

	RecvPropBool( RECVINFO( m_bBountyModeEnabled ) ),

	RecvPropInt( RECVINFO( m_nHalloweenEffect ) ),
	RecvPropFloat( RECVINFO( m_fHalloweenEffectStartTime ) ),
	RecvPropFloat( RECVINFO( m_fHalloweenEffectDuration ) ),
	RecvPropInt( RECVINFO( m_halloweenScenario ) ),
	RecvPropBool( RECVINFO( m_bHelltowerPlayersInHell ) ),
	RecvPropBool( RECVINFO( m_bIsUsingSpells ) ),
	RecvPropBool( RECVINFO( m_bCompetitiveMode ), 0, RecvProxy_CompetitiveMode ),
	RecvPropInt( RECVINFO( m_nMatchGroupType ) ),
	RecvPropBool( RECVINFO( m_bMatchEnded ) ),
	RecvPropBool( RECVINFO( m_bPowerupMode ) ),
	RecvPropString( RECVINFO( m_pszCustomUpgradesFile ) ),
	RecvPropBool( RECVINFO( m_bTruceActive ) ),
	RecvPropBool( RECVINFO( m_bShowMatchSummary ), 0, RecvProxy_MatchSummary ),
	RecvPropBool( RECVINFO_NAME( m_bShowMatchSummary, "m_bShowCompetitiveMatchSummary" ), 0, RecvProxy_MatchSummary ),     // Renamed
	RecvPropBool( RECVINFO( m_bTeamsSwitched ) ),
	RecvPropBool( RECVINFO( m_bMapHasMatchSummaryStage ) ),
	RecvPropBool( RECVINFO( m_bPlayersAreOnMatchSummaryStage ) ),
	RecvPropBool( RECVINFO( m_bStopWatchWinner ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_ePlayerWantsRematch), RecvPropInt( RECVINFO(m_ePlayerWantsRematch[0]), 0, RecvProxy_PlayerVotedForMap ) ),
	RecvPropInt( RECVINFO( m_eRematchState ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_nNextMapVoteOptions), RecvPropInt( RECVINFO(m_nNextMapVoteOptions[0]), 0, RecvProxy_NewMapVoteStateChanged ) ),

	RecvPropInt( RECVINFO( m_nForceUpgrades ) ),
	RecvPropInt( RECVINFO( m_nForceEscortPushLogic ) ),

	RecvPropBool( RECVINFO( m_bRopesHolidayLightsAllowed ) ),
#else

	SendPropInt( SENDINFO( m_nGameType ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nStopWatchState ), 3, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropTime( SENDINFO( m_flCapturePointEnableTime ) ),

//=============================================================================
// HPE_BEGIN:
// [msmith]	Training status and hud type.
//=============================================================================
	SendPropInt( SENDINFO( m_nHudType ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bIsInTraining ) ),
	SendPropBool( SENDINFO( m_bAllowTrainingAchievements ) ),
	SendPropBool( SENDINFO( m_bIsWaitingForTrainingContinue ) ),
//=============================================================================
// HPE_END
//=============================================================================
	SendPropBool( SENDINFO( m_bIsTrainingHUDVisible ) ),
	SendPropBool( SENDINFO( m_bIsInItemTestingMode ) ),

	SendPropEHandle( SENDINFO( m_hBonusLogic ) ),
	SendPropBool( SENDINFO( m_bPlayingKoth ) ),
	SendPropBool( SENDINFO( m_bPlayingMedieval ) ),
	SendPropBool( SENDINFO( m_bPlayingHybrid_CTF_CP ) ),
	SendPropBool( SENDINFO( m_bPlayingSpecialDeliveryMode ) ),
	SendPropBool( SENDINFO( m_bPlayingRobotDestructionMode ) ),
	SendPropEHandle( SENDINFO( m_hRedKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hBlueKothTimer ) ),
	SendPropInt( SENDINFO( m_nMapHolidayType ), 3, SPROP_UNSIGNED ),

	SendPropEHandle( SENDINFO( m_itHandle ) ),
	SendPropBool( SENDINFO( m_bPlayingMannVsMachine ) ),
	SendPropEHandle( SENDINFO( m_hBirthdayPlayer ) ),

	SendPropInt( SENDINFO( m_nBossHealth ) ),
	SendPropInt( SENDINFO( m_nMaxBossHealth ) ),
	SendPropInt( SENDINFO( m_fBossNormalizedTravelDistance ) ),
	SendPropBool( SENDINFO( m_bMannVsMachineAlarmStatus ) ),
	SendPropBool( SENDINFO( m_bHaveMinPlayersToEnableReady ) ),

	SendPropBool( SENDINFO( m_bBountyModeEnabled ) ),

	SendPropInt( SENDINFO( m_nHalloweenEffect ) ),
	SendPropFloat( SENDINFO( m_fHalloweenEffectStartTime ) ),
	SendPropFloat( SENDINFO( m_fHalloweenEffectDuration ) ),
	SendPropInt( SENDINFO( m_halloweenScenario ) ),
	SendPropBool( SENDINFO( m_bHelltowerPlayersInHell ) ),
	SendPropBool( SENDINFO( m_bIsUsingSpells ) ),
	SendPropBool( SENDINFO( m_bCompetitiveMode ) ),
	SendPropBool( SENDINFO( m_bPowerupMode ) ),
	SendPropInt( SENDINFO( m_nMatchGroupType ) ),
	SendPropBool( SENDINFO( m_bMatchEnded ) ),
	SendPropString( SENDINFO( m_pszCustomUpgradesFile ) ),
	SendPropBool( SENDINFO( m_bTruceActive ) ),
	SendPropBool( SENDINFO( m_bShowMatchSummary ) ),
	SendPropBool( SENDINFO( m_bTeamsSwitched ) ),
	SendPropBool( SENDINFO( m_bMapHasMatchSummaryStage ) ),
	SendPropBool( SENDINFO( m_bPlayersAreOnMatchSummaryStage ) ),
	SendPropBool( SENDINFO( m_bStopWatchWinner ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_ePlayerWantsRematch), SendPropInt( SENDINFO_ARRAY(m_ePlayerWantsRematch), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropInt( SENDINFO( m_eRematchState ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_nNextMapVoteOptions), SendPropInt( SENDINFO_ARRAY(m_nNextMapVoteOptions), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),

	SendPropInt( SENDINFO( m_nForceUpgrades ) ),
	SendPropInt( SENDINFO( m_nForceEscortPushLogic ) ),

	SendPropBool( SENDINFO( m_bRopesHolidayLightsAllowed ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_gamerules, CTFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TFGameRulesProxy, DT_TFGameRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFGameRules ), RecvProxy_TFGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TFGameRules ), SendProxy_TFGameRules )
	END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGameRulesProxy )
//=============================================================================
// HPE_BEGIN:
// [msmith]	Training HUD type
//=============================================================================
	DEFINE_KEYFIELD( m_nHudType, FIELD_INTEGER, "hud_type" ),
//=============================================================================
// HPE_END
//=============================================================================

	DEFINE_KEYFIELD( m_bOvertimeAllowedForCTF, FIELD_BOOLEAN, "ctf_overtime" ),
	DEFINE_KEYFIELD( m_bRopesHolidayLightsAllowed, FIELD_BOOLEAN, "ropes_holiday_lights_allowed" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRequiredObserverTarget", InputSetRequiredObserverTarget ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTeamScore", InputAddRedTeamScore ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTeamScore", InputAddBlueTeamScore ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetRedKothClockActive", InputSetRedKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetBlueKothClockActive", InputSetBlueKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCTFCaptureBonusTime", InputSetCTFCaptureBonusTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),
	DEFINE_INPUTFUNC( FIELD_STRING, "HandleMapEvent", InputHandleMapEvent ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCustomUpgradesFile", InputSetCustomUpgradesFile ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRoundRespawnFreezeEnabled", InputSetRoundRespawnFreezeEnabled ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetMapForcedTruceDuringBossFight", InputSetMapForcedTruceDuringBossFight ),

	DEFINE_OUTPUT( m_OnWonByTeam1,	"OnWonByTeam1" ),
	DEFINE_OUTPUT( m_OnWonByTeam2,	"OnWonByTeam2" ),
	DEFINE_OUTPUT( m_Team1PlayersChanged,	"Team1PlayersChanged" ),
	DEFINE_OUTPUT( m_Team2PlayersChanged,	"Team2PlayersChanged" ),
	DEFINE_OUTPUT( m_OnPowerupImbalanceTeam1, "OnPowerupImbalanceTeam1" ),
	DEFINE_OUTPUT( m_OnPowerupImbalanceTeam2, "OnPowerupImbalanceTeam2" ),
	DEFINE_OUTPUT( m_OnPowerupImbalanceMeasuresOver, "OnPowerupImbalanceMeasuresOver" ),
	DEFINE_OUTPUT( m_OnStateEnterRoundRunning, "OnStateEnterRoundRunning" ),
	DEFINE_OUTPUT( m_OnStateEnterBetweenRounds, "OnStateEnterBetweenRounds" ),
	DEFINE_OUTPUT( m_OnStateEnterPreRound, "OnStateEnterPreRound" ),
	DEFINE_OUTPUT( m_OnStateExitPreRound, "OnStateExitPreRound" ),
	DEFINE_OUTPUT( m_OnMatchSummaryStart, "OnMatchSummaryStart" ),
	DEFINE_OUTPUT( m_OnTruceStart, "OnTruceStart" ),
	DEFINE_OUTPUT( m_OnTruceEnd, "OnTruceEnd" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRulesProxy::CTFGameRulesProxy()
{
	m_nHudType = TF_HUDTYPE_UNDEFINED;
	m_bOvertimeAllowedForCTF = true;
	m_bRopesHolidayLightsAllowed = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamScore( inputdata_t &inputdata )
{
	TFTeamMgr()->AddTeamScore( TF_TEAM_RED, inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamScore( inputdata_t &inputdata )
{
	TFTeamMgr()->AddTeamScore( TF_TEAM_BLUE, inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedKothClockActive( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		variant_t sVariant;
		CTeamRoundTimer *pTimer = TFGameRules()->GetKothTeamTimer( TF_TEAM_BLUE );
		if ( pTimer )
		{
			pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
		}

		pTimer = TFGameRules()->GetKothTeamTimer( TF_TEAM_RED );
		if ( pTimer )
		{
			pTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetCTFCaptureBonusTime( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->SetCTFCaptureBonusTime( inputdata.value.Float() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueKothClockActive( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		variant_t sVariant;
		CTeamRoundTimer *pTimer = TFGameRules()->GetKothTeamTimer( TF_TEAM_BLUE );
		if ( pTimer )
		{
			pTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
		}

		pTimer = TFGameRules()->GetKothTeamTimer( TF_TEAM_RED );
		if ( pTimer )
		{
			pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRequiredObserverTarget( inputdata_t &inputdata )
{
	const char *pszEntName = inputdata.value.String();
	CBaseEntity *pEnt = NULL;

	if ( pszEntName && pszEntName[0] )
	{
		pEnt = gEntList.FindEntityByName( NULL, pszEntName );
	}

	TFGameRules()->SetRequiredObserverTarget( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_RED, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_BLUE, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pass in a VO sound entry to play for RED
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVORed( inputdata_t &inputdata )
{
	const char *szSoundName = inputdata.value.String();
	if ( szSoundName )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_RED, szSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pass in a VO sound entry to play for BLUE
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVOBlue( inputdata_t &inputdata )
{
	const char *szSoundName = inputdata.value.String();
	if ( szSoundName )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, szSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pass in a VO sound entry to play
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVO( inputdata_t &inputdata )
{
	const char *szSoundName = inputdata.value.String();
	if ( szSoundName )
	{
		TFGameRules()->BroadcastSound( 255, szSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputHandleMapEvent( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->HandleMapEvent( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetCustomUpgradesFile( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->SetCustomUpgradesFile( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata )
{
	tf_player_movement_restart_freeze.SetValue( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetMapForcedTruceDuringBossFight( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->SetMapForcedTruceDuringBossFight( inputdata.value.Bool() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::Activate()
{
	TFGameRules()->Activate();

//=============================================================================
// HPE_BEGIN:
// [msmith]	We added a HUDTYPE so that the objective and the HUD are independent.
//			This lets us have a non training HUD on a training map.  And a training
//			HUD on another type of map.
//=============================================================================
	if ( m_nHudType != TF_HUDTYPE_UNDEFINED )
	{
		TFGameRules()->SetHUDType( m_nHudType );
	}
//=============================================================================
// HPE_END
//=============================================================================

	TFGameRules()->SetOvertimeAllowedForCTF( m_bOvertimeAllowedForCTF );

	TFGameRules()->SetRopesHolidayLightsAllowed( m_bRopesHolidayLightsAllowed );

	ListenForGameEvent( "teamplay_round_win" );

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::TeamPlayerCountChanged( CTFTeam *pTeam )
{
	if ( pTeam == TFTeamMgr()->GetTeam( TF_TEAM_RED ) )
	{
		m_Team1PlayersChanged.Set( pTeam->GetNumPlayers(), this, this );
	}
	else if ( pTeam == TFTeamMgr()->GetTeam( TF_TEAM_BLUE ) )
	{
		m_Team2PlayersChanged.Set( pTeam->GetNumPlayers(), this, this );
	}

	// Tell the clients
	IGameEvent *event = gameeventmanager->CreateEvent( "teams_changed" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::PowerupTeamImbalance( int nTeam )
{
	switch ( nTeam )
	{
	case TF_TEAM_RED:
		m_OnPowerupImbalanceTeam1.FireOutput( this, this );
		break;
	case TF_TEAM_BLUE:
		m_OnPowerupImbalanceTeam2.FireOutput( this, this );
		break;
	default:
		m_OnPowerupImbalanceMeasuresOver.FireOutput( this, this );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateEnterRoundRunning( void )
{
	m_OnStateEnterRoundRunning.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateEnterBetweenRounds( void )
{
	m_OnStateEnterBetweenRounds.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateEnterPreRound( void )
{
	m_OnStateEnterPreRound.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::StateExitPreRound( void )
{
	m_OnStateExitPreRound.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::MatchSummaryStart( void )
{
	m_OnMatchSummaryStart.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::TruceStart( void )
{
	m_OnTruceStart.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::TruceEnd( void )
{
	m_OnTruceEnd.FireOutput( this, this );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::FireGameEvent( IGameEvent *event )
{
#ifdef GAME_DLL
	const char *pszEventName = event->GetName();

	if ( FStrEq( pszEventName, "teamplay_round_win" ) )
	{
		int iWinningTeam = event->GetInt( "team" );

		switch( iWinningTeam )
		{
		case TF_TEAM_RED:
			m_OnWonByTeam1.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_OnWonByTeam2.FireOutput( this, this );
			break;
		default:
			break;
		}
	}
#endif
}

// (We clamp ammo ourselves elsewhere).
ConVar ammo_max( "ammo_max", "5000", FCVAR_REPLICATED );

#ifndef CLIENT_DLL
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		// Very lame that the base code needs this defined
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Return true if we are playing a PvE mode
//-----------------------------------------------------------------------------
bool CTFGameRules::IsPVEModeActive( void ) const
{
#ifdef TF_RAID_MODE
	if ( IsRaidMode() || IsBossBattleMode() )
		return true;
#endif

	if ( IsMannVsMachineMode() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Return true for PvE opponents (ie: enemy bot team)
//-----------------------------------------------------------------------------
bool CTFGameRules::IsPVEModeControlled( CBaseEntity *who ) const
{
	if ( !who )
	{
		return false;
	}

#ifdef GAME_DLL
	if ( IsMannVsMachineMode() )
	{
		return who->GetTeamNumber() == TF_TEAM_PVE_INVADERS ? true : false;
	}

	if ( IsPVEModeActive() )
	{
		return who->GetTeamNumber() == TF_TEAM_RED ? true : false;
	}
#endif

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Return true if engineers can build quickly now
//-----------------------------------------------------------------------------
bool CTFGameRules::IsQuickBuildTime( void )
{
	return IsMannVsMachineMode() && ( InSetup() || TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::GameModeUsesUpgrades( void )
{
	if ( m_nForceUpgrades == 1 )
		return false;

	if ( m_nForceUpgrades == 2 )
		return true;

	if ( IsMannVsMachineMode() || IsBountyMode() )
		return true;

	return false;
}

bool CTFGameRules::GameModeUsesEscortPushLogic( void )
{
	if ( m_nForceEscortPushLogic == 1 )
		return false;

	if ( m_nForceEscortPushLogic == 2 )
		return true;

	if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanPlayerUseRespec( CTFPlayer *pTFPlayer )
{
	if ( !pTFPlayer )
		return false;

	bool bAllowed = IsMannVsMachineRespecEnabled() && State_Get() == GR_STATE_BETWEEN_RNDS;

#ifdef GAME_DLL
	if ( !g_pPopulationManager )
		return false;

	bAllowed &= ( g_pPopulationManager->GetNumRespecsAvailableForPlayer( pTFPlayer ) ) ? true : false;
#else
	if ( !g_TF_PR )
		return false;

	bAllowed &= ( g_TF_PR->GetNumRespecCredits( pTFPlayer->entindex() ) ) ? true : false;
#endif
	
	return bAllowed;
}

bool CTFGameRules::IsCommunityGameMode( void ) const
{
	return tf_gamemode_community.GetBool();
}

bool CTFGameRules::IsCompetitiveMode( void ) const
{
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		return pMatchDesc->GetMatchType() == MATCH_TYPE_COMPETITIVE
			|| pMatchDesc->GetMatchType() == MATCH_TYPE_CASUAL;
	}

	return false;
}

bool CTFGameRules::IsMatchTypeCasual( void ) const
{
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		return ( pMatchDesc->GetMatchType() == MATCH_TYPE_CASUAL );
	}

	return false;
}

bool CTFGameRules::IsMatchTypeCompetitive( void ) const
{
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		return ( pMatchDesc->GetMatchType() == MATCH_TYPE_COMPETITIVE );
	}

	return false;
}

bool CTFGameRules::BInMatchStartCountdown() const
{
	if ( IsCompetitiveMode() )
	{
		float flTime = GetRoundRestartTime();
		if ( ( flTime > 0.f ) && ( (int)( flTime - gpGlobals->curtime ) <= mp_tournament_readymode_countdown.GetInt() ) )
		{
			return true;
		}
	}

	return false;
}

ETFMatchGroup CTFGameRules::GetCurrentMatchGroup() const
{

#ifdef GAME_DLL
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	return pMatch ? pMatch->m_eMatchGroup : k_eTFMatchGroup_Invalid;
#else
	// Client
	// We only care about what the server says if we are in an MM match.  We pass false
	// into BConnectedToMatch because we want the match group of the server EVEN IF
	// the match is over, but we're still connected.
	return GTFGCClientSystem()->BConnectedToMatchServer( false ) ? (ETFMatchGroup)m_nMatchGroupType.Get() : k_eTFMatchGroup_Invalid;
#endif
}

bool CTFGameRules::IsManagedMatchEnded() const
{
#ifdef GAME_DLL
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	return !pMatch || pMatch->BMatchTerminated();
#else
	// Client
	// We only care about what the server says if we are in an MM match.  (Note that the GC client system calls this to
	// determine if an MM server considers the match over, so beware circular logic)
	return !GTFGCClientSystem()->BConnectedToMatchServer( true ) || m_bMatchEnded;
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
void CTFGameRules::SyncMatchSettings()
{
	// These mirror the MatchInfo for the client's sake.
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();

	m_nMatchGroupType.Set( pMatch ? pMatch->m_eMatchGroup : k_eTFMatchGroup_Invalid );
	m_bMatchEnded.Set( IsManagedMatchEnded() );
}

//-----------------------------------------------------------------------------
bool CTFGameRules::StartManagedMatch()
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( !pMatch )
	{
		Warning( "Starting a managed match with no match info\n" );
		return false;
	}

	// Cleanup
	m_eRematchState = NEXT_MAP_VOTE_STATE_NONE;

	/// Sync these before level change, so there's no race condition where clients may connect during/before the
	/// changelevel and see that the match is ended or wrong.
	SyncMatchSettings();

	// Change the the correct map from the match.  If no match specified, perform a fresh load of the current map
	const char *pszMap = pMatch->GetMatchMap();
	if ( !pszMap )
	{
		pszMap = STRING( gpGlobals->mapname );
		Warning( "Managed match did not specify map, using current map (%s)\n", pszMap );
	}

	engine->ServerCommand( CFmtStr( "changelevel %s\n", pszMap ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetCompetitiveMode( bool bValue )
{
	m_bCompetitiveMode.Set( bValue );
	//// Competitive mode is only supported on official servers.
	//// It requires matchmaking, and doesn't allow ad-hoc connections.
	//// If cheats are ever enabled, we force this mode off.

	//tf_mm_trusted.SetValue( bValue );
	//mp_tournament.SetValue( bValue );
	//mp_tournament_readymode.SetValue( bValue );

	//// No ad-hoc connections
	//tf_mm_strict.SetValue( bValue );

	//if ( bValue )
	//{
	//	engine->ServerCommand( "exec server_ladder.cfg\n" );

	//	Assert( tf_mm_servermode.GetInt() == 1 );
	//}

	//// Any state toggle is a reset (so don't spam this call)
	//State_Transition( GR_STATE_PREROUND );
	//SetInWaitingForPlayers( bValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::StartCompetitiveMatch( void )
{
	m_flSafeToLeaveTimer = -1.f;

	SetInWaitingForPlayers( false );
	RoundRespawn();
	State_Transition( GR_STATE_RESTART );
	ResetPlayerAndTeamReadyState();
}

//-----------------------------------------------------------------------------
// Purpose: Stops a match for some specified reason
//-----------------------------------------------------------------------------
void CTFGameRules::StopCompetitiveMatch( CMsgGC_Match_Result_Status nCode )
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	int nActiveMatchPlayers = pMatch->GetNumActiveMatchPlayers();
	if ( BAttemptMapVoteRollingMatch() &&
		nCode == CMsgGC_Match_Result_Status_MATCH_SUCCEEDED &&
		nActiveMatchPlayers > 0 )
	{
		ChooseNextMapVoteOptions();
	}
	else
	{
		// If we're not attempting a rolling match, end it
		// TODO ROLLING MATCHES: If we bail between now and RequestNewMatchForLobby, we need to call this or we'll get stuck.
		if ( !IsManagedMatchEnded() )
		{
			GTFGCClientSystem()->EndManagedMatch();
			Assert( IsManagedMatchEnded() );
			m_bMatchEnded.Set( true );
		}
	}

	if ( nCode == CMsgGC_Match_Result_Status_MATCH_SUCCEEDED )
	{
		IGameEvent *winEvent = gameeventmanager->CreateEvent( "competitive_victory" );
		if ( winEvent )
		{
			gameeventmanager->FireEvent( winEvent );
		}

		//
		// This determines new ratings and creates the GC messages
		//

		CMatchInfo *pInfo = GTFGCClientSystem()->GetMatch();
		if ( pInfo )
		{
			pInfo->CalculateMatchSkillRatingAdjustments( m_iWinningTeam );

			// Performance ranking with medals is currently server-side
			if ( pInfo->CalculatePlayerMatchRankData() )
			{
				// Send scoreboard event with final data
				int nPlayers = pInfo->GetNumTotalMatchPlayers();
				for ( int idx = 0; idx < nPlayers; idx++ )
				{
					IGameEvent *pEvent = gameeventmanager->CreateEvent( "competitive_stats_update" );
					if ( pEvent )
					{
						CMatchInfo::PlayerMatchData_t *pMatchRankData = pInfo->GetMatchDataForPlayer( idx );

						CBasePlayer *pPlayer = UTIL_PlayerBySteamID( pMatchRankData->steamID );
						if ( !pPlayer )
							continue;

						pEvent->SetInt( "index", pPlayer->entindex() );
						pEvent->SetInt( "score_rank", pMatchRankData ? pMatchRankData->nScoreMedal : 0 );		// medal won (if any)
						pEvent->SetInt( "kills_rank", pMatchRankData ? pMatchRankData->nKillsMedal : 0 );		//
						pEvent->SetInt( "damage_rank", pMatchRankData ? pMatchRankData->nDamageMedal : 0 );		//
						pEvent->SetInt( "healing_rank", pMatchRankData ? pMatchRankData->nHealingMedal : 0 );	//
						pEvent->SetInt( "support_rank", pMatchRankData ? pMatchRankData->nSupportMedal : 0 );	//
						gameeventmanager->FireEvent( pEvent );
					}
				}
			}
		}
		else
		{
			Warning( "CalculatePlayerMatchRankData(): General failure (investigate).\n" );
		}

		ReportMatchResultsToGC( nCode );
	}
	else if ( nCode == CMsgGC_Match_Result_Status_MATCH_FAILED_ABANDON )
	{
		// This generates a "safe to leave" notification on clients
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_abandoned_match" );
		if ( pEvent )
		{
			pEvent->SetBool( "game_over", ( tf_competitive_abandon_method.GetBool() || State_Get() == GR_STATE_BETWEEN_RNDS ) );
			gameeventmanager->FireEvent( pEvent );
		}

		ReportMatchResultsToGC( nCode );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fully completes the match
//-----------------------------------------------------------------------------
void CTFGameRules::EndCompetitiveMatch( void )
{
	MatchSummaryEnd();

	Log( "Competitive match ended.  Kicking all players.\n" );
	engine->ServerCommand( "kickall #TF_Competitive_Disconnect\n" );

	// Prepare for next match
	g_fGameOver = false;
	if ( !IsCommunityGameMode() )
		m_bAllowBetweenRounds = true;
	State_Transition( GR_STATE_RESTART );
	SetInWaitingForPlayers( true );
}

//-----------------------------------------------------------------------------
// Purpose: Called during CTFGameRules::Think()
//-----------------------------------------------------------------------------
void CTFGameRules::ManageCompetitiveMode( void )
{
	if ( !IsCompetitiveMode() )
		return;

	// Bring this back when we ship?
// 	// Security check
// 	if ( !tf_skillrating_debug.GetBool() )
// 	{
// 		m_bCompetitiveMode &= tf_mm_trusted.GetBool() &&
// 							  IsInTournamentMode() &&
// 							  !HaveCheatsBeenEnabledDuringLevel();
// 	}

	// We lost trusted status
	if ( !tf_mm_trusted.GetBool() )
	{
		m_nMatchGroupType.Set( k_eTFMatchGroup_Invalid );
		StopCompetitiveMatch( CMsgGC_Match_Result_Status_MATCH_FAILED_TRUSTED );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#TF_Matchmaking_Exit_Competitive_Mode" );
		Log( "Server lost trusted status.  Exiting Competitive Mode!" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::ReportMatchResultsToGC( CMsgGC_Match_Result_Status nCode )
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( !pMatch )
		return false;

	GCSDK::CProtoBufMsg< CMsgGC_Match_Result > *pMsg = new GCSDK::CProtoBufMsg< CMsgGC_Match_Result >( k_EMsgGC_Match_Result );

	pMsg->Body().set_match_id( pMatch->m_nMatchID );
	pMsg->Body().set_match_group( pMatch->m_eMatchGroup );
	pMsg->Body().set_status( nCode );
	pMsg->Body().set_duration( CTF_GameStats.m_currentMap.m_Header.m_iTotalTime + ( gpGlobals->curtime - m_flRoundStartTime ) );

	CTeam *pTeam = GetGlobalTeam( TF_TEAM_RED );
	pMsg->Body().set_red_score( pTeam ? pTeam->GetScore() : (uint32)-1 );
	pTeam = GetGlobalTeam( TF_TEAM_BLUE );
	pMsg->Body().set_blue_score( pTeam ? pTeam->GetScore() : (uint32)-1 );
	Assert( m_iWinningTeam >= 0 );
	pMsg->Body().set_winning_team( Max( 0, (int)m_iWinningTeam ) );
	const MapDef_t *pMap = GetItemSchema()->GetMasterMapDefByName( STRING( gpGlobals->mapname ) );
	pMsg->Body().set_map_index( ( pMap ) ? pMap->m_nDefIndex : 0 );
	pMsg->Body().set_game_type( 1 );	// TODO: eMapGameType

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( pMatch->m_eMatchGroup );
	if( !pMatchDesc || !pMatchDesc->m_pProgressionDesc )
		return false;

	int nTotalScore = 0;

	CTFPlayerResource *pTFResource = dynamic_cast< CTFPlayerResource* >( g_pPlayerResource );
	if ( pTFResource )
	{
		pTFResource->UpdatePlayerData();

		for ( int i=0; i < MAX_PLAYERS; ++i )
		{
			nTotalScore += pTFResource->GetTotalScore( i );
		}
	}

	float flBlueScoreRatio = 0.5f;

	const CTFTeam* pTFTeamRed = GetGlobalTFTeam( TF_TEAM_RED );
	const CTFTeam* pTFTeamBlue = GetGlobalTFTeam( TF_TEAM_BLUE );

	// Figure out how much XP to give each team based on the game mode played
	if ( HasMultipleTrains() )
	{
		// In PLR we want to use the distance along the tracks each of the 
		// trains were at the end of each round
		flBlueScoreRatio = RemapValClamped( pTFTeamBlue->GetTotalPLRTrackPercentTraveled(), 0.f, pTFTeamBlue->GetTotalPLRTrackPercentTraveled() + pTFTeamRed->GetTotalPLRTrackPercentTraveled(), 0.f, 1.f );
	}
	else if ( !m_bPlayingKoth && !m_bPowerupMode && ( tf_gamemode_cp.GetInt() || tf_gamemode_sd.GetInt() || tf_gamemode_payload.GetInt() ) )
	{
		// Rounds Won
		// CP -	Points can flow back and forever, so we can't count total caps. And the
		//		state of the game is always the same at match end, so rounds won is our
		//		only real metric.
		// SD -	A flag cap is a round, so we could count caps or rounds here.  Again, the
		//		flag can go back and forth forever, but the match end state is always the same.
		// PL - Count the points captured by each team.  We do this A/D style where each team has
		//		a chance to score points.
		flBlueScoreRatio = RemapValClamped( pTFTeamBlue->GetScore(), 0.f, pTFTeamBlue->GetScore() + pTFTeamRed->GetScore() , 0.f, 1.f );
	}
	else if ( tf_gamemode_ctf.GetInt() || m_bPowerupMode || tf_gamemode_passtime.GetInt() )
	{
		// Flag captures
		// Mannpower is a variant of CTF and Passtime effectively is CTF. In all of these modes
		// we don't use rounds so our best metric is individual flag captures.
		flBlueScoreRatio = RemapValClamped( pTFTeamBlue->GetTotalFlagCaptures(), 0.f, pTFTeamBlue->GetTotalFlagCaptures() + pTFTeamRed->GetTotalFlagCaptures(), 0.f, 1.f );
	}
	else if ( m_bPlayingKoth )
	{
		// Time capped
		// Looking at the capped time for each team will let us give xp in a fair way.  We can
		// actually get as close as 50/50
		float flBlueTimeCapped = pTFTeamBlue->GetKOTHTime();
		float flRedTiemCapped = pTFTeamRed->GetKOTHTime();

		flBlueScoreRatio = RemapValClamped( flBlueTimeCapped, 0.f, flBlueTimeCapped + flRedTiemCapped, 0.f, 1.f );
	}
	else if ( tf_gamemode_rd.GetInt() || tf_gamemode_pd.GetInt() )
	{
		CTFRobotDestructionLogic* pRDLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();

		// Count bottles/cores scored by each team
		flBlueScoreRatio = RemapValClamped( pRDLogic->GetScore( TF_TEAM_BLUE ), 0.f, pRDLogic->GetScore( TF_TEAM_BLUE ) + pRDLogic->GetScore( TF_TEAM_RED ), 0.f, 1.f );
	}
	else if ( tf_gamemode_tc.GetInt() )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		
		int nBlueMiniRounds = 0;
		int nRedMiniRounds = 0;

		// Use the number of mini-rounds won by each team
		for( int i=0; i < pMaster->GetNumRounds(); ++ i )
		{
			const CTeamControlPointRound* pRound = pMaster->GetRoundByIndex( i );
			if ( pRound->RoundOwnedByTeam( TF_TEAM_RED ) )
			{
				++nRedMiniRounds;
			}
			else if ( pRound->RoundOwnedByTeam( TF_TEAM_BLUE ) )
			{
				++nBlueMiniRounds;
			}
			else
			{
				Assert( false );
			}
		}


		flBlueScoreRatio = RemapValClamped( nBlueMiniRounds, 0.f, nBlueMiniRounds + nRedMiniRounds, 0.f, 1.f );
	}
	else
	{
		Assert( !"Game mode not handled for team XP bonus!" );
	}

	const float flRedScoreRatio = 1.f - flBlueScoreRatio;
	const int nBlueTeamObjectiveBonus = flBlueScoreRatio * nTotalScore;
	const int nRedTeamObjectiveBonus = flRedScoreRatio * nTotalScore;

	// Player info
	for ( int idxPlayer = 0; idxPlayer < pMatch->GetNumTotalMatchPlayers(); idxPlayer++ )
	{
		CMatchInfo::PlayerMatchData_t *pMatchPlayer = pMatch->GetMatchDataForPlayer( idxPlayer );
		if ( !pMatchPlayer->steamID.BIndividualAccount() )
		{
			Assert( false );
			continue;
		}

		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerBySteamID( pMatchPlayer->steamID ) );
		PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );

		CMsgGC_Match_Result_Player *pMsgPlayer = pMsg->Body().add_players();
		int nTeam = GetGameTeamForGCTeam( pMatchPlayer->eGCTeam );
		pMsgPlayer->set_steam_id( pMatchPlayer->steamID.ConvertToUint64() );
		pMsgPlayer->set_team( nTeam );
		if ( pTFResource && pTFPlayer )
		{
			pMsgPlayer->set_score( pTFResource->GetTotalScore( pTFPlayer->entindex() ) );
		}
		else
		{
			// They left
		}

		int nPing = 0;
		int nPacketLoss = 0;
		if ( pTFPlayer )
		{
			UTIL_GetPlayerConnectionInfo( pTFPlayer->entindex(), nPing, nPacketLoss );
		}
		pMsgPlayer->set_ping( nPing );
		uint32 unPlayerFlags = 0U;
		if ( pMatchPlayer->bDropped )
		{
			unPlayerFlags |= MATCH_FLAG_PLAYER_LEAVER;
		}
		if ( pMatchPlayer->bLateJoin )
		{
			unPlayerFlags |= MATCH_FLAG_PLAYER_LATEJOIN;
		}
		if ( pMatchPlayer->BDropWasAbandon() )
		{
			unPlayerFlags |= MATCH_FLAG_PLAYER_ABANDONER;
		}
		if ( pMatchPlayer->bPlayed )
		{
			unPlayerFlags |= MATCH_FLAG_PLAYER_PLAYED;
		}
		if ( !pMatchPlayer->bEverConnected )
		{
			unPlayerFlags |= MATCH_FLAG_PLAYER_NEVER_CONNECTED;
		}
		if ( !pMatchPlayer->bEverActive )
		{
			unPlayerFlags |= MATCH_FLAG_PLAYER_NEVER_ACTIVE;
		}
		if ( !pMatchPlayer->bEverDisconnected )
		{
			unPlayerFlags |= MATCH_FLAG_PLAYER_NEVER_DISCONNECTED;
		}

		pMsgPlayer->set_flags( unPlayerFlags );
		// server-side skill system
		pMsgPlayer->set_classes_played( pMatchPlayer->unClassesPlayed );
		pMsgPlayer->set_kills( pStats ? pStats->statsAccumulated.m_iStat[TFSTAT_KILLS] : 0 );
		pMsgPlayer->set_damage( pStats ? pStats->statsAccumulated.m_iStat[TFSTAT_DAMAGE] : 0 );
		pMsgPlayer->set_healing( pStats ? pStats->statsAccumulated.m_iStat[TFSTAT_HEALING] : 0 );
		pMsgPlayer->set_support( pStats ? CalcPlayerSupportScore( &pStats->statsAccumulated, pTFPlayer->entindex() ) : 0 );
		pMsgPlayer->set_score_medal( pMatchPlayer->nScoreMedal );
		pMsgPlayer->set_kills_medal( pMatchPlayer->nKillsMedal );
		pMsgPlayer->set_damage_medal( pMatchPlayer->nDamageMedal );
		pMsgPlayer->set_healing_medal( pMatchPlayer->nHealingMedal );
		pMsgPlayer->set_support_medal( pMatchPlayer->nSupportMedal );
		pMsgPlayer->set_rank( pMatchPlayer->nRank );
		pMsgPlayer->set_deaths( pStats ? pStats->statsAccumulated.m_iStat[TFSTAT_DEATHS] : 0 );
		pMsgPlayer->set_original_party_id( pMatchPlayer->uOriginalPartyID );
		uint32 unLeaveTime = ( pMatchPlayer && ( pMatchPlayer->bDropped || pMatchPlayer->BDropWasAbandon() ) ) ? 
							   pMatchPlayer->GetLastActiveEventTime() : 0u;
		pMsgPlayer->set_leave_time( unLeaveTime );
		pMsgPlayer->set_leave_reason( pMatchPlayer->GetDropReason() );
		pMsgPlayer->set_connect_time( pMatchPlayer->rtJoinedMatch );

		// Somebody won! Match finish bonus
		if ( nCode == CMsgGC_Match_Result_Status_MATCH_SUCCEEDED )
		{
			// Give points based on team performance
			int nPerformanceScore = RemapValClamped( pMsgPlayer->score(), 0, nTotalScore / 24, 0, nTeam == TF_TEAM_RED ? nRedTeamObjectiveBonus : nBlueTeamObjectiveBonus );
			pMatch->GiveXPRewardToPlayerForAction( pMatchPlayer->steamID, CMsgTFXPSource::SOURCE_OBJECTIVE_BONUS, nPerformanceScore );
		
			// Everyone gets base completion points
			int nCompletionScore = RemapValClamped( pMsgPlayer->score(), 0, nTotalScore / 24, 0, nTotalScore );
			pMatch->GiveXPRewardToPlayerForAction( pMatchPlayer->steamID, CMsgTFXPSource::SOURCE_COMPLETED_MATCH, nCompletionScore );
		}

		// Copy any pending XP sources they had ready to send up
		for( int i=0; i < pMatchPlayer->GetXPSources().sources_size(); ++i )
		{
			CMsgTFXPSource* pXPSource = pMsgPlayer->add_xp_breakdown();
			pXPSource->CopyFrom( pMatchPlayer->GetXPSources().sources( i ) );
		}
	}

	pMsg->Body().set_win_reason( GetWinReason() );
	uint32 unMatchFlags = 0u;

	if ( pMatch->m_uLobbyFlags & LOBBY_FLAG_LOWPRIORITY )
	{
		unMatchFlags |= MATCH_FLAG_LOWPRIORITY;
	}

	if ( pMatch->m_uLobbyFlags & LOBBY_FLAG_REMATCH )
	{
		unMatchFlags |= MATCH_FLAG_REMATCH;
	}

	pMsg->Body().set_flags( unMatchFlags );
	pMsg->Body().set_bots( pMatch->m_nBotsAdded );

	GTFGCClientSystem()->SendCompetitiveMatchResult( pMsg );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::MatchmakingShouldUseStopwatchMode( void )
{
	return IsAttackDefenseMode();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::IsAttackDefenseMode( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	bool bRetVal = !HasMultipleTrains() && ( tf_gamemode_payload.GetBool() || ( pMaster && ( pMaster->PlayingMiniRounds() || pMaster->ShouldSwitchTeamsOnRoundWin() ) ) );

	tf_attack_defend_map.SetValue( bRetVal );
	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameRules::SetPowerupMode( bool bValue )
{
	// Powerup mode uses grapple and changes some gamerule variables.

	if ( bValue )
	{
		tf_grapplinghook_enable.SetValue( 1 );
		tf_flag_return_time_credit_factor.SetValue( 0 );
	}
	else
	{
		tf_grapplinghook_enable.SetValue( 0 );
		tf_flag_return_time_credit_factor.SetValue( 1 );
	}

	m_bPowerupMode = bValue;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
void CTFGameRules::EndManagedMvMMatch( bool bKickPlayersToParties )
{
	// Primarily a pass through so we can ensure our match end state is sync'd -- CPopulationManager manages most of the
	// MvM meta round state
	if ( !IsManagedMatchEnded() )
	{
		GTFGCClientSystem()->EndManagedMatch();
		Assert( IsManagedMatchEnded() );
		m_bMatchEnded.Set( true );
	}
}
#endif // GAME_DLL


#endif // STAGING_ONLY

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::UsePlayerReadyStatusMode( void )
{
	if ( IsMannVsMachineMode() )
		return true;

	if ( IsCompetitiveMode() )
		return true;

	if ( mp_tournament.GetBool() && mp_tournament_readymode.GetBool() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerReadyStatus_HaveMinPlayersToEnable( void )
{
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );

	// we always have enough players if the match wants players to autoready
	if ( pMatchDesc && pMatchDesc->BUsesAutoReady() )
		return true;

#ifdef GAME_DLL
	// Count connected players
	int nNumPlayers = 0;
	CUtlVector< CTFPlayer* > playerVector;
	CollectPlayers( &playerVector );
	FOR_EACH_VEC( playerVector, i )
	{
		if ( !playerVector[i] )
			continue;

		if ( playerVector[i]->IsFakeClient() )
			continue;

		if ( playerVector[i]->IsBot() )
			continue;

		if ( playerVector[i]->IsHLTV() )
			continue;

		if ( playerVector[i]->IsReplay() )
			continue;

		nNumPlayers++;
	}

	// Default
	int nMinPlayers = 1;
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();

	if ( pMatch && !pMatch->BMatchTerminated() && pMatchDesc->BRequiresCompleteMatches() )
	{
		nMinPlayers = pMatch->GetCanonicalMatchSize();
	}
	else if ( IsMannVsMachineMode() &&
	          ( engine->IsDedicatedServer() || ( !engine->IsDedicatedServer() && nNumPlayers > 1 ) ) )
	{
		nMinPlayers = tf_mvm_min_players_to_start.GetInt();
	}
	else if ( UsePlayerReadyStatusMode() && engine->IsDedicatedServer() )
	{
		nMinPlayers = mp_tournament_readymode_min.GetInt();
	}

	// Should be renamed to m_bEnableReady, not sure why we encoded our criteria in the names of all associated functions and variables...
	m_bHaveMinPlayersToEnableReady.Set( nNumPlayers >= nMinPlayers );

#endif

	return m_bHaveMinPlayersToEnableReady;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerReadyStatus_ArePlayersOnTeamReady( int iTeam )
{
	if ( IsMannVsMachineMode() && iTeam == TF_TEAM_PVE_INVADERS )
		return true;

	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch )
	{
		int nMatchPlayers = pMatch->GetNumTotalMatchPlayers();
		if ( nMatchPlayers <= 0 )
			return false;

		int iPlayerReadyCount = 0;
		for ( int i = 0; i < nMatchPlayers; i++ )
		{
			CMatchInfo::PlayerMatchData_t *pPlayerData = pMatch->GetMatchDataForPlayer( i );
			if ( !pPlayerData->bDropped && GetGameTeamForGCTeam( pPlayerData->eGCTeam ) == iTeam )
			{
				CBasePlayer *pPlayer = UTIL_PlayerBySteamID( pPlayerData->steamID );
				// XXX(JohnS): Not quite valid yet, We let them join first onto spectate, which is probably a bit
				//             confusing
				//
				// AssertMsg( !pPlayer || ToTFPlayer( pPlayer )->GetTeamNumber() == GetGameTeamForGCTeam( pPlayerData->eGCTeam ),
				//            "Player's GC assigned team does not match their current team" );
				if ( !pPlayer || !m_bPlayerReady[ pPlayer->entindex() ] )
					return false;

				iPlayerReadyCount++;
			}
		}

		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
		if ( pMatchDesc && pMatchDesc->BUsesAutoReady() )
		{
			return iPlayerReadyCount > 0 || pMatch->GetNumTotalMatchPlayers() == 1 ;
		}
		else
		{
			int iTeamSize = IsMannVsMachineMode() ? pMatch->GetCanonicalMatchSize() : pMatch->GetCanonicalMatchSize() / 2;
			return iPlayerReadyCount >= iTeamSize;
		}
	}

	// Non-match
	bool bAtLeastOneReady = false;
	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer || ToTFPlayer( pPlayer )->GetTeamNumber() != iTeam )
			continue;

		if ( !m_bPlayerReady[i] )
		{
			return false;
		}
		else
		{
			bAtLeastOneReady = true;
		}
	}

	// Team isn't ready if there was nobody on it.
	return bAtLeastOneReady;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerReadyStatus_ShouldStartCountdown( void )
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();


	if ( IsMannVsMachineMode() )
	{
		if ( !IsTeamReady( TF_TEAM_PVE_DEFENDERS ) && m_flRestartRoundTime >= gpGlobals->curtime + mp_tournament_readymode_countdown.GetInt() )
		{
			bool bIsTeamReady = PlayerReadyStatus_ArePlayersOnTeamReady( TF_TEAM_PVE_DEFENDERS );
			if ( bIsTeamReady )
			{
				SetTeamReadyState( true, TF_TEAM_PVE_DEFENDERS );
				return true;
			}
		}
	}
	else if ( pMatch &&
	          PlayerReadyStatus_ArePlayersOnTeamReady( TF_TEAM_RED ) &&
	          PlayerReadyStatus_ArePlayersOnTeamReady( TF_TEAM_BLUE ) )
	{
		return true;
	}
	else if ( IsTeamReady( TF_TEAM_RED ) && IsTeamReady( TF_TEAM_BLUE ) )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerReadyStatus_ResetState( void )
{
	// Reset the players
	ResetPlayerAndTeamReadyState();

	// Reset the team
	SetTeamReadyState( false, TF_TEAM_RED );
	SetTeamReadyState( false, TF_TEAM_BLUE );

	m_flRestartRoundTime.Set( -1.f );
	mp_restartgame.SetValue( 0 );
	m_bAwaitingReadyRestart = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerReadyStatus_UpdatePlayerState( CTFPlayer *pTFPlayer, bool bState )
{
	if ( !UsePlayerReadyStatusMode() )
		return;

	if ( !pTFPlayer )
		return;

	if ( pTFPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		return;

	if ( State_Get() != GR_STATE_BETWEEN_RNDS )
		return;

	// Don't allow toggling state in the final countdown
	if ( GetRoundRestartTime() > 0.f && GetRoundRestartTime() <= gpGlobals->curtime + TOURNAMENT_NOCANCEL_TIME )
		return;

	// Make sure we have enough to allow ready mode commands
	if ( !PlayerReadyStatus_HaveMinPlayersToEnable() )
		return;

	int nEntIndex = pTFPlayer->entindex();
	if ( !IsIndexIntoPlayerArrayValid(nEntIndex) )
		return;

	// Already this state
	if ( bState == IsPlayerReady( nEntIndex ) )
		return;

	SetPlayerReadyState( nEntIndex, bState );

	if ( !bState )
	{
		// Slam team status to Not Ready for any player that sets Not Ready
		m_bTeamReady.Set( pTFPlayer->GetTeamNumber(), false );

		// If everyone cancels ready state, stop the clock
		bool bAnyoneReady = false;
		for ( int i = 1; i <= MAX_PLAYERS; ++i )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			if ( m_bPlayerReady[i] )
			{
				bAnyoneReady = true;
				break;
			}
		}

		if ( !bAnyoneReady )
		{
			m_flRestartRoundTime.Set( -1.f );
			mp_restartgame.SetValue( 0 );
			ResetPlayerAndTeamReadyState();
		}
	}
	else
	{
		if ( IsMannVsMachineMode() || IsCompetitiveMode() )
		{
			// Reduce timer as each player hits Ready, but only once per-player
			if ( !m_bPlayerReadyBefore[nEntIndex] && m_flRestartRoundTime > gpGlobals->curtime + 60.f )
			{
				float flReduceBy = 30.f;
				if ( m_flRestartRoundTime < gpGlobals->curtime + 90.f )
				{
					// Never reduce below 60 seconds remaining
					flReduceBy = m_flRestartRoundTime - gpGlobals->curtime - 60.f;
				}

				m_flRestartRoundTime -= flReduceBy;
			}
			else if ( m_flRestartRoundTime < 0 && !PlayerReadyStatus_ShouldStartCountdown() )
			{
				m_flRestartRoundTime.Set( gpGlobals->curtime + 150.f );
				m_bAwaitingReadyRestart = false;

				IGameEvent* pEvent = gameeventmanager->CreateEvent( "teamplay_round_restart_seconds" );
				if ( pEvent )
				{
					pEvent->SetInt( "seconds", 150 );
					gameeventmanager->FireEvent( pEvent );
				}
			}
		}

		// Unofficial modes set team ready state here
		CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
		if ( !pMatch && !IsMannVsMachineMode() )
		{
			int nRed = 0;
			int nRedCount = 0;
			int nBlue = 0;
			int nBlueCount = 0;

			for ( int iTeam = FIRST_GAME_TEAM; iTeam < TFTeamMgr()->GetTeamCount(); iTeam++ )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
				if ( pTeam )
				{
					Assert( pTeam->GetTeamNumber() == TF_TEAM_RED || pTeam->GetTeamNumber() == TF_TEAM_BLUE );

					for ( int i = 0; i < pTeam->GetNumPlayers(); ++i )
					{
						if ( !pTeam->GetPlayer(i) )
							continue;

						if ( pTeam->GetTeamNumber() == TF_TEAM_RED && IsPlayerReady( pTeam->GetPlayer(i)->entindex() ) )
						{
							if ( !nRedCount )
							{
								nRedCount = pTeam->GetNumPlayers();
							}

							nRed++;
						}
						else if ( pTeam->GetTeamNumber() == TF_TEAM_BLUE && IsPlayerReady( pTeam->GetPlayer(i)->entindex() ) )
						{
							if ( !nBlueCount )
							{
								nBlueCount = pTeam->GetNumPlayers();
							}

							nBlue++;
						}
					}
				}
			}

			// Check for the convar that requires min team size, or just go with whatever each team has
			int nRedMin = ( mp_tournament_readymode_team_size.GetInt() > 0 ) ? mp_tournament_readymode_team_size.GetInt() : Max( nRedCount, 1 );
			int nBlueMin = ( mp_tournament_readymode_team_size.GetInt() > 0 ) ? mp_tournament_readymode_team_size.GetInt() : Max( nBlueCount, 1 );

			SetTeamReadyState( ( nRed == nRedMin ), TF_TEAM_RED );
			SetTeamReadyState( ( nBlue == nBlueMin ), TF_TEAM_BLUE );
		}

		m_bPlayerReadyBefore[nEntIndex] = true;
	}
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFGameRules::IsDefaultGameMode( void )
{
	if ( IsMannVsMachineMode() )
		return false;

	if ( IsInArenaMode() )
		return false;

	if ( IsInMedievalMode() )
		return false;

	if ( IsCompetitiveMode() )
		return false;

	if ( IsInTournamentMode() )
		return false;

	if ( IsInTraining() )
		return false;

	if ( IsInItemTestingMode() )
		return false;


#ifdef TF_RAID_MODE
	if ( IsRaidMode() )
		return false;

	if ( IsBossBattleMode() )
		return false;
#endif // TF_RAID_MODE

#ifdef TF_CREEP_MODE
	if ( IsCreepWaveMode() )
		return false;
#endif // TF_CREEP_MODE

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Allows us to give situational discounts, such as secondary weapons
//-----------------------------------------------------------------------------
int CTFGameRules::GetCostForUpgrade( CMannVsMachineUpgrades *pUpgrade, int iItemSlot, int nClass, CTFPlayer *pPurchaser /*= NULL*/ )
{
	Assert( pUpgrade );

	if ( !pUpgrade )
		return 0;

	int iCost = pUpgrade->nCost;

	CEconItemSchema *pSchema = ItemSystem()->GetItemSchema();
	if ( pSchema )
	{
		const CEconItemAttributeDefinition *pAttr = pSchema->GetAttributeDefinitionByName( pUpgrade->szAttrib );
		if ( pAttr )
		{
			if ( ( iItemSlot == LOADOUT_POSITION_PRIMARY && nClass == TF_CLASS_ENGINEER ) ||
				 ( iItemSlot == LOADOUT_POSITION_SECONDARY && nClass != TF_CLASS_DEMOMAN ) ||
				 ( iItemSlot == LOADOUT_POSITION_MELEE && nClass != TF_CLASS_SPY && nClass != TF_CLASS_ENGINEER ) )
			{
				switch ( pAttr->GetDefinitionIndex() )
				{
				case 4:		// clip size bonus
				case 6:		// fire rate bonus
				case 78:	// maxammo secondary increased
				case 180:	// heal on kill
				case 266:	// projectile penetration
				case 335:	// clip size bonus upgrade
				case 397:	// projectile penetration heavy
					iCost *= 0.5f;
					break;
				default:
					break;
				}
			}
			else if ( iItemSlot == LOADOUT_POSITION_ACTION )
			{
				if ( pPurchaser )
				{
					int iCanteenSpec = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( pPurchaser, iCanteenSpec, canteen_specialist );
					if ( iCanteenSpec )
					{

// 						iCost *= ( 1.f - ( iCanteenSpec * 0.1f ) );
// 						iCost = Max( 1, ( iCost - ( iCost % 5 ) ) );

						iCost -= ( 10 * iCanteenSpec );
						iCost = Max( 5, iCost );
					}
				}
			}
		}
	}

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::CTFGameRules()
#ifdef GAME_DLL
	: m_mapCoachToStudentMap( DefLessFunc(uint32) )
	, m_flNextStrangeEventProcessTime( g_flStrangeEventBatchProcessInterval )
	, m_mapTeleportLocations( DefLessFunc(string_t) )
	, m_bMapCycleNeedsUpdate( false )
	, m_flSafeToLeaveTimer( -1.f )
	, m_flCompModeRespawnPlayersAtMatchStart( -1.f )
	, m_bMapForcedTruceDuringBossFight( false )
	, m_flNextHalloweenGiftUpdateTime( -1 )
#else
	: m_bRecievedBaseline( false )
#endif
{
#ifdef GAME_DLL

	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	// Create the team managers
//	for ( int i = 0; i < ARRAYSIZE( teamnames ); i++ )
//	{
//		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "tf_team" ));
//		pTeam->Init( sTeamNames[i], i );
//
//		g_Teams.AddToTail( pTeam );
//	}

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "player_escort_score" );
	ListenForGameEvent( "player_disconnect" );
	ListenForGameEvent( "teamplay_setup_finished" );
	ListenForGameEvent( "recalculate_truce" );

	Q_memset( m_vecPlayerPositions,0, sizeof(m_vecPlayerPositions) );

	m_iPrevRoundState = -1;
	m_iCurrentRoundState = -1;
	m_iCurrentMiniRoundMask = 0;

	m_iPreviousTeamSize = 0;

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[MAX_PATH] = { 0 };
	// Map names cannot contain quotes or control characters so this is safe but silly that we have to do it.
	Q_snprintf( szCommand, sizeof( szCommand ), "exec \"%s.cfg\"\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

	m_flSendNotificationTime = 0.0f;

	m_bOvertimeAllowedForCTF = true;

	m_redPayloadToPush = NULL;
	m_bluePayloadToPush = NULL;
	m_redPayloadToBlock = NULL;
	m_bluePayloadToBlock = NULL;

	SetCTFCaptureBonusTime( -1 );

	m_hasSpawnedToy = false;

	m_flCapInProgressBuffer = 0.f;

	m_bMannVsMachineAlarmStatus = false;
	m_flNextFlagAlarm = 0.0f;
	m_flNextFlagAlert = 0.0f;

	m_doomsdaySetupTimer.Invalidate();
	StopDoomsdayTicketsTimer();

	m_nPowerupKillsRedTeam = 0;
	m_nPowerupKillsBlueTeam = 0;
	m_flTimeToRunImbalanceMeasures = 120.f;
	m_flTimeToStopImbalanceMeasures = 0.f;
	m_bPowerupImbalanceMeasuresRunning = false;

	m_hRequiredObserverTarget = NULL;
	m_bStopWatchWinner.Set( false );

#else // GAME_DLL

	// Vision Filter Translations for swapping out particle effects and models
	SetUpVisionFilterKeyValues();

	m_bSillyGibs = CommandLine()->FindParm( "-sillygibs" ) ? true : false;
	if ( m_bSillyGibs )
	{
		cl_burninggibs.SetValue( 0 );
	}
	// @todo Tom Bui: game_newmap doesn't seem to be used...
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "overtime_nag" );
	ListenForGameEvent( "recalculate_holidays" );
	
#endif

	ClearHalloweenEffectStatus();

	// Initialize the game type
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	m_bPlayingMannVsMachine.Set( false );
	m_bMannVsMachineAlarmStatus.Set( false );
	m_bBountyModeEnabled.Set( false );

	m_bPlayingKoth.Set( false );
	m_bPlayingMedieval.Set( false );
	m_bPlayingHybrid_CTF_CP.Set( false );
	m_bPlayingSpecialDeliveryMode.Set( false );
	m_bPlayingRobotDestructionMode.Set( false );
	m_bPowerupMode.Set( false );

	m_bHelltowerPlayersInHell.Set( false );
	m_bIsUsingSpells.Set( false );
	m_bTruceActive.Set( false );
	m_bTeamsSwitched.Set( false );

	m_halloweenScenario.Set( HALLOWEEN_SCENARIO_NONE );
	m_iGlobalAttributeCacheVersion = 0;

	m_bRopesHolidayLightsAllowed.Set( true );

//=============================================================================
// HPE_BEGIN
// [msmith] HUD type
//=============================================================================
	m_nHudType.Set( TF_HUDTYPE_UNDEFINED );
	m_bIsInTraining.Set( false );
	m_bAllowTrainingAchievements.Set( false );
	m_bIsWaitingForTrainingContinue.Set( false );
//=============================================================================
// HPE_END
//=============================================================================
	m_bIsTrainingHUDVisible.Set( false );

	m_bIsInItemTestingMode.Set( false );

	// Set turbo physics on.  Do it here for now.
	sv_turbophysics.SetValue( 1 );

	// Initialize the team manager here, etc...

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	COMPILE_TIME_ASSERT( TF_WEAPON_COUNT == ARRAYSIZE( g_aWeaponDamageTypes ) );

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';

	m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );

	mp_tournament_redteamname.Revert();
	mp_tournament_blueteamname.Revert();

	m_flCapturePointEnableTime = 0.0f;

	m_itHandle = 0;

	m_hBirthdayPlayer = NULL;

	m_nBossHealth = 0;
	m_nMaxBossHealth = 0;
	m_fBossNormalizedTravelDistance = 0.0f;

	m_areHealthAndAmmoVectorsReady = false;

	m_flGravityMultiplier.Set( 1.0 );

	m_pszCustomUpgradesFile.GetForModify()[0] = '\0';

	m_bShowMatchSummary.Set( false );
	m_bMapHasMatchSummaryStage.Set( false );
	m_bPlayersAreOnMatchSummaryStage.Set( false );

	m_bUseMatchHUD = false;
	m_bUsePreRoundDoors = false;

	m_nMatchGroupType.Set( k_eTFMatchGroup_Invalid );
	m_bMatchEnded.Set( true );

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		m_ePlayerWantsRematch.Set( i, USER_NEXT_MAP_VOTE_UNDECIDED );
	}

	m_eRematchState = NEXT_MAP_VOTE_STATE_NONE;

#ifdef GAME_DLL

	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch )
	{
		SyncMatchSettings();
	}

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_bHasSpawnedSoccerBall[i] = false;
	}

	m_flCheckPlayersConnectingTime = 0;

	m_pUpgrades = NULL;

	// Determine if a halloween event map is active
	// Map active always turns on Halloween
	{
		char szCurrentMap[MAX_MAP_NAME];
		Q_strncpy( szCurrentMap, STRING( gpGlobals->mapname ), sizeof( szCurrentMap ) );

		if ( !Q_stricmp( szCurrentMap, "cp_manor_event" ) )
		{
			m_halloweenScenario.Set( HALLOWEEN_SCENARIO_MANN_MANOR );
		}
		else if ( !Q_stricmp( szCurrentMap, "koth_viaduct_event" ) )
		{
			m_halloweenScenario.Set( HALLOWEEN_SCENARIO_VIADUCT );
		}
		else if ( !Q_stricmp( szCurrentMap, "koth_lakeside_event" ) )
		{
			m_halloweenScenario.Set( HALLOWEEN_SCENARIO_LAKESIDE );
		}
		else if( !Q_stricmp( szCurrentMap, "plr_hightower_event" ) )
		{
			m_halloweenScenario.Set( HALLOWEEN_SCENARIO_HIGHTOWER );
		}
		else if ( !Q_stricmp( szCurrentMap, "sd_doomsday_event" ) )
		{
			m_halloweenScenario.Set( HALLOWEEN_SCENARIO_DOOMSDAY );
		}
	}

	// Our hook for LoadMapCycleFile wont run during the base class constructor that does this initially
	TrackWorkshopMapsInMapCycle();
#endif
}

#ifdef GAME_DLL
void CTFGameRules::Precache( void )
{
	BaseClass::Precache();

	// The Halloween bosses get spawned in code, so they don't get a chance to precache
	// when the map loads.  We'll do the precaching for them here.
	if( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
	{
		CMerasmus::PrecacheMerasmus();
	}
	else if( IsHalloweenScenario( HALLOWEEN_SCENARIO_VIADUCT ) )
	{
		CEyeballBoss::PrecacheEyeballBoss();
	}
	else if( IsHalloweenScenario( HALLOWEEN_SCENARIO_MANN_MANOR ) )
	{
		CHeadlessHatman::PrecacheHeadlessHatman();
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		CEyeballBoss::PrecacheEyeballBoss();
		CGhost::PrecacheGhost();
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		CTFPlayer::PrecacheKart();
		CGhost::PrecacheGhost();
		CHeadlessHatman::PrecacheHeadlessHatman();
		CMerasmus::PrecacheMerasmus();
	}

	if ( MapHasPrefix( STRING( gpGlobals->mapname ), "mvm_" ) )
	{
		CTFPlayer::PrecacheMvM();
	}

	CTFPlayer::m_bTFPlayerNeedsPrecache = true;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//#ifdef GAME_DLL
//extern void AddHalloweenGiftPositionsForMap( const char *pszMapName, CUtlVector<Vector> &vLocations );
//#endif

void CTFGameRules::LevelInitPostEntity( void )
{
	BaseClass::LevelInitPostEntity();

#ifdef GAME_DLL
	// Refind our proxy, because we might have had it deleted due to a mapmaker placed one
	m_hGamerulesProxy = dynamic_cast<CTFGameRulesProxy*>( gEntList.FindEntityByClassname( NULL, "tf_gamerules" ) );

	// Halloween
	// Flush Halloween Gift Location and grab locations if applicable.  NonHalloween maps will have these as zero
	m_halloweenGiftSpawnLocations.Purge();

	if ( IsHolidayActive( kHoliday_Halloween ) )
	{
		for ( int i=0; i<IHalloweenGiftSpawnAutoList::AutoList().Count(); ++i )
		{
			CHalloweenGiftSpawnLocation* pGift = static_cast< CHalloweenGiftSpawnLocation* >( IHalloweenGiftSpawnAutoList::AutoList()[i] );
			m_halloweenGiftSpawnLocations.AddToTail( pGift->GetAbsOrigin() );
			UTIL_Remove( pGift );
		}

		// Ask Halloween System if there are any locations
		AddHalloweenGiftPositionsForMap( STRING(gpGlobals->mapname), m_halloweenGiftSpawnLocations );
	}

	m_flMatchSummaryTeleportTime = -1.f;


	const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		pMatchDesc->InitGameRulesSettingsPostEntity();
	}

#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::GetRespawnTimeScalar( int iTeam )
{
	// In PvE mode, we don't modify respawn times
	if ( IsPVEModeActive() )
		return 1.0;

	return BaseClass::GetRespawnTimeScalar( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers )
{
	bool bScale = bScaleWithNumPlayers;

#ifdef TF_RAID_MODE
	if ( IsRaidMode() )
	{
		return tf_raid_respawn_time.GetFloat();
	}
#endif // TF_RAID_MODE

#ifdef TF_CREEP_MODE
	if ( IsCreepWaveMode() )
	{
		return tf_creep_wave_player_respawn_time.GetFloat();
	}
#endif

	if ( IsMannVsMachineMode() )
	{
		bScale = false;
	}

	float flTime = BaseClass::GetRespawnWaveMaxLength( iTeam, bScale );

	CTFRobotDestructionLogic* pRoboLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();
	if ( pRoboLogic )
	{
		flTime *= ( 1.f - pRoboLogic->GetRespawnScaleForTeam( iTeam ) );
	}

	return flTime;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FlagsMayBeCapped( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_PREROUND )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldDrawHeadLabels()
{
	if ( IsInTournamentMode() )
	{
		bool bConnectedToMatchServer = false;
#ifdef CLIENT_DLL
		bConnectedToMatchServer = GTFGCClientSystem() && GTFGCClientSystem()->BConnectedToMatchServer( false );
#else
		bConnectedToMatchServer = GTFGCClientSystem() && GTFGCClientSystem()->GetMatch();
#endif
		if ( !bConnectedToMatchServer )
		{
			return false;
		}
	}

	return BaseClass::ShouldDrawHeadLabels();
}
 

//-----------------------------------------------------------------------------
// Purpose: Return which Halloween scenario is currently running
//-----------------------------------------------------------------------------
CTFGameRules::HalloweenScenarioType CTFGameRules::GetHalloweenScenario( void ) const
{
	if ( !const_cast< CTFGameRules * >( this )->IsHolidayActive( kHoliday_Halloween ) )
		return HALLOWEEN_SCENARIO_NONE;

	return m_halloweenScenario;
}
 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsUsingSpells( void ) const
{
	if ( tf_spells_enabled.GetBool() )
		return true;

	if ( IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
		return true;

	return m_bIsUsingSpells;
}

//-----------------------------------------------------------------------------
bool CTFGameRules::IsUsingGrapplingHook( void ) const
{
	return tf_grapplinghook_enable.GetBool();
}

//-----------------------------------------------------------------------------
bool CTFGameRules::IsTruceActive( void ) const
{
	return m_bTruceActive;
}

//-----------------------------------------------------------------------------
bool CTFGameRules::CanInitiateDuels( void )
{
	if ( IsInWaitingForPlayers() )
		return false;

	gamerules_roundstate_t roundState = State_Get();
	if ( ( roundState != GR_STATE_RND_RUNNING ) && ( roundState != GR_STATE_PREROUND ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetGameTeamForGCTeam( TF_GC_TEAM nGCTeam )
{
	if ( nGCTeam == TF_GC_TEAM_INVADERS )
	{
		if ( IsCompetitiveMode() )
		{
			return ( m_bTeamsSwitched ) ? TF_TEAM_RED : TF_TEAM_BLUE;
		}

		return TF_TEAM_BLUE;
	}
	else if ( nGCTeam == TF_GC_TEAM_DEFENDERS )
	{
		if ( IsCompetitiveMode() )
		{
			return ( m_bTeamsSwitched ) ? TF_TEAM_BLUE : TF_TEAM_RED;
		}

		return TF_TEAM_RED;
	}

	return TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
TF_GC_TEAM CTFGameRules::GetGCTeamForGameTeam( int nGameTeam )
{
	if ( nGameTeam == TF_TEAM_BLUE )
	{
		if ( IsCompetitiveMode() )
		{
			return ( m_bTeamsSwitched ) ? TF_GC_TEAM_DEFENDERS : TF_GC_TEAM_INVADERS;
		}

		return TF_GC_TEAM_INVADERS;
	}
	else if ( nGameTeam == TF_TEAM_RED )
	{
		if ( IsCompetitiveMode() )
		{
			return ( m_bTeamsSwitched ) ? TF_GC_TEAM_INVADERS : TF_GC_TEAM_DEFENDERS;
		}

		return TF_GC_TEAM_DEFENDERS;
	}

	return TF_GC_TEAM_NOTEAM;
}

CTFGameRules::EUserNextMapVote CTFGameRules::GetWinningVote( int (&nVotes)[ EUserNextMapVote::NUM_VOTE_STATES ] ) const
{
	// We assume "undecided" is the index just after the last vote option
	COMPILE_TIME_ASSERT( USER_NEXT_MAP_VOTE_UNDECIDED == NEXT_MAP_VOTE_OPTIONS );
	memset( nVotes, 0, sizeof( nVotes ) );
	int nTotalPlayers = 0;

	// Tally up votes.  
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
#ifdef CLIENT_DLL

		if ( !g_PR || !g_PR->IsConnected( iPlayerIndex ) ) 
			continue;

#else // GAME_DLL
		// We care about those that are still here.  If you leave, you don't count towards the vote total
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( !pTFPlayer || pTFPlayer->IsBot() )
			continue;


		if ( !pTFPlayer->IsConnected() )
			continue;		

		CSteamID steamID;
		pTFPlayer->GetSteamID( &steamID );

		// People without parties *should* be getting a new one soon.  Count them as undecided
		// until their party shows up and they're allowed to make a real vote.
		CTFParty* pParty = GTFGCClientSystem()->GetPartyForPlayer( steamID );
		if ( !pParty )
		{
			++nVotes[ EUserNextMapVote::USER_NEXT_MAP_VOTE_UNDECIDED ];
			++nTotalPlayers;
			continue;
		}

		const CMatchInfo* pMatch = GTFGCClientSystem()->GetMatch();
		Assert( pMatch );
		if ( !pMatch )
		{
			continue;
		}

		// Need to be a match players
		const CMatchInfo::PlayerMatchData_t* pPlayerMatchData = pMatch->GetMatchDataForPlayer( steamID );
		Assert( pPlayerMatchData );
		if ( !pPlayerMatchData )
		{
			// How'd you get here?
			continue;
		}
#endif

		nTotalPlayers++;
		nVotes[ TFGameRules()->PlayerNextMapVoteState( iPlayerIndex ) ]++;
	}

	if ( nVotes[ USER_NEXT_MAP_VOTE_UNDECIDED ] == nTotalPlayers )
	{
		return USER_NEXT_MAP_VOTE_UNDECIDED;
	}
	else
	{
		EUserNextMapVote eWinningVote = USER_NEXT_MAP_VOTE_MAP_0;

		for( int i = 0; i < NEXT_MAP_VOTE_OPTIONS; ++i )
		{
			// The current map is in slot 0.  >= so we favor change.  
			eWinningVote = nVotes[ i ] >= nVotes[ eWinningVote ] 
						 ? (EUserNextMapVote)i
						 : eWinningVote;
		}

		return eWinningVote;
	}	
}

#ifdef GAME_DLL
void CTFGameRules::KickPlayersNewMatchIDRequestFailed()
{
	Assert( m_eRematchState == NEXT_MAP_VOTE_STATE_MAP_CHOSEN_PAUSE );

	// Let everyone know the rematch failed.
	if ( m_eRematchState == NEXT_MAP_VOTE_STATE_MAP_CHOSEN_PAUSE )
	{
		CBroadcastRecipientFilter filter;
		UTIL_ClientPrintFilter( filter, HUD_PRINTTALK, "#TF_Matchmaking_RollingQueue_NewRematch_GCFail" );

		IGameEvent *pEvent = gameeventmanager->CreateEvent( "rematch_failed_to_create" );
		if ( pEvent )
		{
			gameeventmanager->FireEvent( pEvent );
		}
	}

	// The GC failed to get a new MatchID for us.  Let's clear out and reset.
	engine->ServerCommand( "kickall #TF_Competitive_Disconnect\n" );

	// Tell the GC System to end the managed match mode -- we skipped this in StopCompetitiveMatch so we could roll the
	// managed match into a new one.
	Assert( !IsManagedMatchEnded() );
	if ( !IsManagedMatchEnded() )
	{
		GTFGCClientSystem()->EndManagedMatch();
		Assert( IsManagedMatchEnded() );
		m_bMatchEnded.Set( true );
	}

	// Prepare for next match
	g_fGameOver = false;
	if ( !IsCommunityGameMode() )
		m_bAllowBetweenRounds = true;
	State_Transition( GR_STATE_RESTART );
	SetInWaitingForPlayers( true );
}

//-----------------------------------------------------------------------------
void CTFGameRules::CheckAndSetPartyLeader( CTFPlayer *pTFPlayer, int iTeam )
{
	if ( !pTFPlayer )
		return;

	Assert( iTeam >= FIRST_GAME_TEAM );
	if ( iTeam < FIRST_GAME_TEAM )
		return;

	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( !pMatch )
		return;

	CSteamID steamID;
	if ( !pTFPlayer->GetSteamID( &steamID ) )
		return;

	// TODO:  Whenever a lobby is updated, look at the CTFLobbyPlayers and see if
	//		  everyone has the same partyID and then set whoever is the leader to
	//		  have their name be the team name

	//CMatchInfo::PlayerMatchData_t *pMatchPlayer = pMatch->GetMatchDataForPlayer( steamID );
	//if ( pMatchPlayer && pMatchPlayer->bPremadeLeader )
	//{
	//	CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource* >( g_pPlayerResource );
	//	if ( pResource )
	//	{
	//		pResource->SetPartyLeaderIndex( iTeam, pTFPlayer->entindex() );
	//	}
	//}
}

//-----------------------------------------------------------------------------
// Purpose: Sets current boss victim
//-----------------------------------------------------------------------------
void CTFGameRules::SetIT( CBaseEntity *who )
{
	if ( IsHolidayActive( kHoliday_Halloween ) && !IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		CTFPlayer* newIT = ToTFPlayer( who );

		if ( newIT && newIT != m_itHandle.Get() )
		{
			// new IT victim - warn them
			ClientPrint( newIT, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", newIT->GetPlayerName() );
			ClientPrint( newIT, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", newIT->GetPlayerName() );

			CSingleUserReliableRecipientFilter filter( newIT );
			newIT->EmitSound( filter, newIT->entindex(), "Player.YouAreIT" );

			// force them to scream when they become it
			newIT->EmitSound( "Halloween.PlayerScream" );
		}

		CTFPlayer *oldIT = ToTFPlayer( m_itHandle );

		if ( oldIT && oldIT != who && oldIT->IsAlive() )
		{
			// tell old IT player they are safe
			CSingleUserReliableRecipientFilter filter( oldIT );
			oldIT->EmitSound( filter, oldIT->entindex(), "Player.TaggedOtherIT" );

			ClientPrint( oldIT, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
			ClientPrint( oldIT, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
		}
	}

	m_itHandle = who;
}


//-----------------------------------------------------------------------------
// Purpose: Sets current birthday player
//-----------------------------------------------------------------------------
void CTFGameRules::SetBirthdayPlayer( CBaseEntity *pEntity )
{
/*
	if ( IsBirthday() )
	{
		if ( pEntity && pEntity->IsPlayer() && pEntity != m_hBirthdayPlayer.Get() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
			if ( pTFPlayer )
			{
				// new IT victim - warn them
//				ClientPrint( pTFPlayer, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", player->GetPlayerName() );
//				ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", player->GetPlayerName() );

				CSingleUserReliableRecipientFilter filter( pTFPlayer );
				pTFPlayer->EmitSound( filter, pTFPlayer->entindex(), "Game.HappyBirthday" );

				// force them to scream when they become it
//				pTFPlayer->EmitSound( "Halloween.PlayerScream" );
			}
		}

// 		CTFPlayer *oldIT = ToTFPlayer( m_itHandle );
// 
// 		if ( oldIT && oldIT != who && oldIT->IsAlive() )
// 		{
// 			// tell old IT player they are safe
// 			CSingleUserReliableRecipientFilter filter( oldIT );
// 			oldIT->EmitSound( filter, oldIT->entindex(), "Player.TaggedOtherIT" );
// 
// 			ClientPrint( oldIT, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
// 			ClientPrint( oldIT, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
// 		}

		m_hBirthdayPlayer = pEntity;
	}
	else
	{
		m_hBirthdayPlayer = NULL;
	}
*/
}


#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: remove all projectiles in the world
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllProjectiles()
{
	for ( int i=0; i<IBaseProjectileAutoList::AutoList().Count(); ++i )
	{
		UTIL_Remove( static_cast< CBaseProjectile* >( IBaseProjectileAutoList::AutoList()[i] ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: remove all buildings in the world
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllBuildings( bool bExplodeBuildings /*= false*/ )
{
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( !pObj->IsMapPlaced() )
		{
			// this is separate from the object_destroyed event, which does
			// not get sent when we remove the objects from the world
			IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
			if ( event )
			{
				CTFPlayer *pOwner = pObj->GetOwner();
				event->SetInt( "userid", pOwner ? pOwner->GetUserID() : -1 ); // user ID of the object owner
				event->SetInt( "objecttype", pObj->GetType() ); // type of object removed
				event->SetInt( "index", pObj->entindex() ); // index of the object removed
				gameeventmanager->FireEvent( event );
			}

			if ( bExplodeBuildings )
			{
				pObj->DetonateObject();
			}
			else
			{
				// This fixes a bug in Raid mode where we could spawn where our sentry was but 
				// we didn't get the weapons because they couldn't trace to us in FVisible
				pObj->SetSolid( SOLID_NONE );
				UTIL_Remove( pObj );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: remove all sentries ammo
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllSentriesAmmo()
{
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->GetType() == OBJ_SENTRYGUN )
		{
			CObjectSentrygun *pSentry = assert_cast< CObjectSentrygun* >( pObj );
			pSentry->RemoveAllAmmo();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes all projectiles and buildings from world
//-----------------------------------------------------------------------------
void CTFGameRules::RemoveAllProjectilesAndBuildings( bool bExplodeBuildings /*= false*/ )
{
	RemoveAllProjectiles();
	RemoveAllBuildings( bExplodeBuildings );
}
#endif // GAME_DLL


//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	// we only want to deny a map change triggered by mp_timelimit if we're not forcing a map reset,
	// we're playing mini-rounds, and the master says we need to play all of them before changing (for maps like Dustbowl)
	if ( !m_bForceMapReset && pMaster && pMaster->PlayingMiniRounds() && pMaster->ShouldPlayAllControlPointRounds() )
	{
		if ( pMaster->NumPlayableControlPointRounds() > 0 )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanGoToStalemate( void )
{
	// In CTF, don't go to stalemate if one of the flags isn't at home
	if ( m_nGameType == TF_GAMETYPE_CTF )
	{
		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
			if ( pFlag->IsDropped() || pFlag->IsStolen() )
				return false;
		}

		// check that one team hasn't won by capping
		if ( CheckCapsPerRound() )
			return false;
	}

	return BaseClass::CanGoToStalemate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RestoreActiveTimer( void )
{
	BaseClass::RestoreActiveTimer();

	if ( IsInKothMode() )
	{
		CTeamRoundTimer *pTimer = GetBlueKothRoundTimer();
		if ( pTimer )
		{
			pTimer->SetShowInHud( true );
		}

		pTimer = GetRedKothRoundTimer();
		if ( pTimer )
		{
			pTimer->SetShowInHud( true );
		}
	}
}

// Classnames of entities that are preserved across round restarts
static const char *s_PreserveEnts[] =
{
	"tf_gamerules",
	"tf_team_manager",
	"tf_player_manager",
	"tf_team",
	"tf_objective_resource",
	"keyframe_rope",
	"move_rope",
	"tf_viewmodel",
	"tf_logic_training",
	"tf_logic_training_mode",
#ifdef TF_RAID_MODE
	"tf_logic_raid",
#endif // TF_RAID_MODE
	"tf_powerup_bottle",
	"tf_mann_vs_machine_stats",
	"tf_wearable",
	"tf_wearable_demoshield",
	"tf_wearable_robot_arm",
	"tf_wearable_vm",
	"tf_logic_bonusround",
	"vote_controller",
	"monster_resource",
	"tf_logic_medieval",
	"tf_logic_cp_timer",
	"tf_logic_tower_defense",	// legacy
	"tf_logic_mann_vs_machine",
	"func_upgradestation",
	"entity_rocket",
	"entity_carrier",
	"entity_sign",
	"entity_saucer",
	"tf_halloween_gift_pickup",
	"tf_logic_competitive",
	"tf_wearable_razorback",
	"entity_soldier_statue",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Activate()
{
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	tf_gamemode_arena.SetValue( 0 );
	tf_gamemode_community.SetValue( 0 );
	tf_gamemode_cp.SetValue( 0 );
	tf_gamemode_ctf.SetValue( 0 );
	tf_gamemode_sd.SetValue( 0 );
	tf_gamemode_payload.SetValue( 0 );
	tf_gamemode_mvm.SetValue( 0 );
	tf_gamemode_rd.SetValue( 0 );
	tf_gamemode_pd.SetValue( 0 );
	tf_gamemode_tc.SetValue( 0 );
	tf_beta_content.SetValue( 0 );
	tf_gamemode_passtime.SetValue( 0 );
	tf_gamemode_misc.SetValue( 0 );

	tf_bot_count.SetValue( 0 );

#ifdef TF_RAID_MODE
	tf_gamemode_raid.SetValue( 0 );
	tf_gamemode_boss_battle.SetValue( 0 );
#endif
	m_bPlayingMannVsMachine.Set( false );
	m_bBountyModeEnabled.Set( false );
	m_nCurrencyAccumulator = 0;
	m_iCurrencyPool = 0;
	m_bMannVsMachineAlarmStatus.Set( false );
	m_bPlayingKoth.Set( false );
	m_bPlayingMedieval.Set( false );
	m_bPlayingHybrid_CTF_CP.Set( false );
	m_bPlayingSpecialDeliveryMode.Set( false );
	m_bPlayingRobotDestructionMode.Set( false );
	m_bPowerupMode.Set( false );

	m_redPayloadToPush = NULL;
	m_bluePayloadToPush = NULL;
	m_redPayloadToBlock = NULL;
	m_bluePayloadToBlock = NULL;
	
	m_zombieMobTimer.Invalidate();
	m_zombiesLeftToSpawn = 0;
	m_nForceUpgrades = 0;
	m_nForceEscortPushLogic = 0;

	m_CPTimerEnts.RemoveAll();

	m_nMapHolidayType.Set( kHoliday_None );

	CArenaLogic *pArenaLogic = dynamic_cast< CArenaLogic * > (gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );

	if ( pArenaLogic != NULL )
	{
		m_hArenaEntity = pArenaLogic;

		m_nGameType.Set( TF_GAMETYPE_ARENA );

		tf_gamemode_arena.SetValue( 1 );

		Msg( "Executing server arena config file\n" );
		engine->ServerCommand( "exec config_arena.cfg\n" );
	}

#ifdef TF_RAID_MODE
	CRaidLogic *pRaidLogic = dynamic_cast< CRaidLogic * >( gEntList.FindEntityByClassname( NULL, "tf_logic_raid" ) );
	if ( pRaidLogic )
	{
		m_hRaidLogic = pRaidLogic;

		tf_gamemode_raid.SetValue( 1 );

		Msg( "Executing server raid game mode config file\n" );
		engine->ServerCommand( "exec config_raid.cfg\n" );
	}

	CBossBattleLogic *pBossBattleLogic = dynamic_cast< CBossBattleLogic * >( gEntList.FindEntityByClassname( NULL, "tf_logic_boss_battle" ) );
	if ( pBossBattleLogic )
	{
		m_hBossBattleLogic = pBossBattleLogic;

		tf_gamemode_boss_battle.SetValue( 1 );
	}
#endif // TF_RAID_MODE

	// This is beta content if this map has "beta" as a tag in the schema
	{
		const MapDef_t* pMap = GetItemSchema()->GetMasterMapDefByName( STRING( gpGlobals->mapname ) );
		if ( pMap && pMap->vecTags.HasElement( GetItemSchema()->GetHandleForTag( "beta" ) ) )
		{
			tf_beta_content.SetValue( 1 );
		}
	}

	if ( !Q_strncmp( STRING( gpGlobals->mapname ), "tc_", 3 )  )
	{
		tf_gamemode_tc.SetValue( 1 );
	}
	
	CMannVsMachineLogic *pMannVsMachineLogic = dynamic_cast< CMannVsMachineLogic * >( gEntList.FindEntityByClassname( NULL, "tf_logic_mann_vs_machine" ) );
	CTeamTrainWatcher *pTrainWatch = dynamic_cast<CTeamTrainWatcher*> ( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	bool bFlag = ICaptureFlagAutoList::AutoList().Count() > 0;
	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		m_bPlayingRobotDestructionMode.Set( true );
		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic()->GetType() == CTFRobotDestructionLogic::TYPE_ROBOT_DESTRUCTION )
		{
			tf_gamemode_rd.SetValue( 1 );
			m_nGameType.Set( TF_GAMETYPE_RD );
			tf_beta_content.SetValue( 1 );
		}
		else
		{
			tf_gamemode_pd.SetValue( 1 );
			m_nGameType.Set( TF_GAMETYPE_PD );
		}
	}
	else if ( pMannVsMachineLogic )
	{
		m_bPlayingMannVsMachine.Set( true );
		tf_gamemode_mvm.SetValue( 1 );
		m_nGameType.Set( TF_GAMETYPE_MVM );
	}
	else if ( MapHasPrefix( STRING( gpGlobals->mapname ), "sd_" ) )
	{
		m_bPlayingSpecialDeliveryMode.Set( true );
		tf_gamemode_sd.SetValue( 1 );
	}
	else if ( bFlag && !CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		m_nGameType.Set( TF_GAMETYPE_CTF );
		tf_gamemode_ctf.SetValue( 1 );
	}
	else if ( pTrainWatch )
	{
		m_nGameType.Set( TF_GAMETYPE_ESCORT );
		tf_gamemode_payload.SetValue( 1 );
		
		CMultipleEscort *pMultipleEscort = dynamic_cast<CMultipleEscort*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_multiple_escort" ) );
		SetMultipleTrains( pMultipleEscort != NULL );
	}
	else if ( g_hControlPointMasters.Count() && m_nGameType != TF_GAMETYPE_ARENA ) // We have cap points in arena but we're not CP
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		tf_gamemode_cp.SetValue( 1 );
	}
	
	auto *pPasstime = dynamic_cast<CTFPasstimeLogic*> ( gEntList.FindEntityByClassname( NULL, "passtime_logic" ) );
	if ( pPasstime )
	{
		m_nGameType.Set( TF_GAMETYPE_PASSTIME );
		tf_gamemode_passtime.SetValue( 1 );
	}

	// the game is in training mode if this entity is found
	m_hTrainingModeLogic = dynamic_cast< CTrainingModeLogic * > ( gEntList.FindEntityByClassname( NULL, "tf_logic_training_mode" ) );
	if ( NULL != m_hTrainingModeLogic )
	{
		m_bIsInTraining.Set( true );
		m_bAllowTrainingAchievements.Set( false );
		mp_humans_must_join_team.SetValue( "blue" );
		m_bIsTrainingHUDVisible.Set( true );
		tf_training_client_message.SetValue( (int)TRAINING_CLIENT_MESSAGE_NONE );
	}

	m_bIsInItemTestingMode.Set( false );

	CKothLogic *pKoth = dynamic_cast<CKothLogic*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_koth" ) );
	if ( pKoth )
	{
		m_bPlayingKoth.Set( true );
	}

	CMedievalLogic *pMedieval = dynamic_cast<CMedievalLogic*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_medieval" ) );
	if ( pMedieval || tf_medieval.GetBool() )
	{
		m_bPlayingMedieval.Set( true );
	}

	CCompetitiveLogic *pCompLogic = dynamic_cast< CCompetitiveLogic* > ( gEntList.FindEntityByClassname( NULL, "tf_logic_competitive" ) );
	if ( pCompLogic )
	{
		m_hCompetitiveLogicEntity = pCompLogic;
	}

	CHybridMap_CTF_CP *pHybridMap_CTF_CP = dynamic_cast<CHybridMap_CTF_CP*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_hybrid_ctf_cp" ) );
	if ( pHybridMap_CTF_CP )
	{
		m_bPlayingHybrid_CTF_CP.Set( true );
	}

	CTFHolidayEntity *pHolidayEntity = dynamic_cast<CTFHolidayEntity*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_holiday" ) );
	if ( pHolidayEntity )
	{
		m_nMapHolidayType.Set( pHolidayEntity->GetHolidayType() );
	}

	// bot roster
	m_hBlueBotRoster = NULL;
	m_hRedBotRoster = NULL;
	CHandle<CTFBotRoster> hBotRoster = dynamic_cast< CTFBotRoster* >( gEntList.FindEntityByClassname( NULL, "bot_roster" ) );
	while ( hBotRoster != NULL )
	{
		if ( FStrEq( hBotRoster->m_teamName.ToCStr(), "blue" ) )
		{
			m_hBlueBotRoster = hBotRoster;
		}
		else if ( FStrEq( hBotRoster->m_teamName.ToCStr(), "red" ) )
		{
			m_hRedBotRoster = hBotRoster;
		}
		else
		{
			if ( m_hBlueBotRoster == NULL )
			{
				m_hBlueBotRoster = hBotRoster;
			}
			if ( m_hRedBotRoster == NULL )
			{
				m_hRedBotRoster = hBotRoster;
			}
		}
		hBotRoster = dynamic_cast< CTFBotRoster* >( gEntList.FindEntityByClassname( hBotRoster, "bot_roster" ) );
	}

	CHandle<CCPTimerLogic> hCPTimer = dynamic_cast< CCPTimerLogic* >( gEntList.FindEntityByClassname( NULL, "tf_logic_cp_timer" ) );
	while ( hCPTimer != NULL )
	{
		m_CPTimerEnts.AddToTail( hCPTimer );
		hCPTimer = dynamic_cast< CCPTimerLogic* >( gEntList.FindEntityByClassname( hCPTimer, "tf_logic_cp_timer" ) );
	}

	// hide from the master server if this game is a training game
	// or offline practice
	if ( IsInTraining() || TheTFBots().IsInOfflinePractice() || IsInItemTestingMode() )
	{
		hide_server.SetValue( true );
	}

	m_bVoteCalled = false;
	m_bServerVoteOnReset = false;
	m_flVoteCheckThrottle = 0;


	if ( tf_powerup_mode.GetBool()  )
	{
		if ( !IsPowerupMode() )
		{
			SetPowerupMode( true );
		}
	}

// 	if ( !IsInTournamentMode() )
// 	{
// 		CExtraMapEntity::SpawnExtraModel();
// 	}

	// If leaving MvM for any other game mode, clean up any sticky UI/state
	if ( IsInTournamentMode() && m_nGameType != TF_GAMETYPE_MVM && g_TFGameModeHistory.GetPrevState() == TF_GAMETYPE_MVM )
	{
		mp_tournament.SetValue( false );
	}

	if ( GameModeUsesUpgrades() && g_pPopulationManager == NULL )
	{
		(CPopulationManager *)CreateEntityByName( "info_populator" );
	}

	if ( tf_gamemode_tc.GetBool() || tf_gamemode_sd.GetBool() || tf_gamemode_pd.GetBool() || m_bPlayingMedieval )
	{
		tf_gamemode_misc.SetValue( 1 );
	}

	CBaseEntity *pStageLogic = gEntList.FindEntityByName( NULL, "competitive_stage_logic_case" );
	if ( pStageLogic )
	{
		m_bMapHasMatchSummaryStage.Set( true );
	}

	m_bCompetitiveMode.Set( false );

	const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		pMatchDesc->InitGameRulesSettings();
	}

	CLogicMannPower *pLogicMannPower = dynamic_cast< CLogicMannPower* > ( gEntList.FindEntityByClassname( NULL, "tf_logic_mannpower" ) );
	tf_powerup_mode.SetValue( pLogicMannPower ? 1 : 0 );

	if ( !IsInTraining() && IsHolidayActive( kHoliday_Soldier ) )
	{
		CreateSoldierStatue();
	}

	if ( IsCompetitiveMode() && IsCustomGameMode() )
	{
		m_bAwaitingReadyRestart.Set( false );
		tf_gamemode_community.SetValue( 1 );
		tf_gamemode_misc.SetValue( 1 );
		mp_tournament.SetValue( false );
		mp_tournament_readymode.SetValue( false );
		SetAllowBetweenRounds( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	bool bRetVal = true;

	if ( ( State_Get() == GR_STATE_TEAM_WIN ) && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );

			// we don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time
			if ( pTrigger && pTrigger->UsesFilter() )
			{
				bRetVal = false;
			}
		}
	}

	// no player-on-player damage during a truce
	if ( IsTruceActive() && pVictim && pVictim->IsPlayer() && info.GetAttacker() && info.GetAttacker()->IsPlayer() && ( info.GetAttacker() != pVictim ) )
	{
		bRetVal = false;
	}

	return bRetVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TF_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
}

//=============================================================================
// HPE_BEGIN:
// [msmith]	Added a HUD type so that we can have the hud independent from the
//			game type.  This is useful in training where we want a training hud
//			Instead of the other types of HUD.
//=============================================================================
//-----------------------------------------------------------------------------
// Purpose: Set a HUD type.
//-----------------------------------------------------------------------------
void CTFGameRules::SetHUDType( int nHudType )
{
	if ( nHudType != TF_HUDTYPE_ARENA )
	{
		if ( nHudType >= TF_HUDTYPE_UNDEFINED && nHudType <= TF_HUDTYPE_TRAINING )
		{
			m_nHudType.Set( nHudType );
		}
	}
}
//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	//There has got to be a better way of doing this.
	if ( Q_strstr( pEnt->GetClassname(), "tf_weapon_" ) )
		return true;

	return BaseClass::RoundCleanupShouldIgnore( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCreateEntity( const char *pszClassName )
{
	if ( FindInList( s_PreserveEnts, pszClassName ) )
		return false;

	return BaseClass::ShouldCreateEntity( pszClassName );
}

const char* CTFGameRules::GetStalemateSong( int nTeam )
{
	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		return (nTeam == TF_TEAM_RED) 
			? "Announcer.Helltower_Hell_Red_Stalemate" 
			: "Announcer.Helltower_Hell_Blue_Stalemate";
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		return "Announcer.SD_Event_MurderedToStalemate";
	}

	return "Game.Stalemate";
}

const char* CTFGameRules::WinSongName( int nTeam )
{
	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		return (nTeam == TF_TEAM_RED) 
			? "Announcer.Helltower_Hell_Red_Win" 
			: "Announcer.Helltower_Hell_Blue_Win";
	}

	return "Game.YourTeamWon"; 
}


const char* CTFGameRules::LoseSongName( int nTeam )
{
	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		return (nTeam == TF_TEAM_RED) 
			? "Announcer.Helltower_Hell_Red_Lose" 
			: "Announcer.Helltower_Hell_Blue_Lose";
	}
	else if ( IsMannVsMachineMode() )
	{
		return "music.mvm_lost_wave";
	}
	else
	{
		return BaseClass::LoseSongName( nTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CleanUpMap( void )
{
#ifdef GAME_DLL
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		// Remove all player conditions to prevent some dependency bugs
		pTFPlayer->m_Shared.RemoveAllCond();
	}
#endif // GAME_DLL

	BaseClass::CleanUpMap();

	if ( HLTVDirector() )
	{
		HLTVDirector()->BuildCameraList();
	}

#ifdef GAME_DLL
	m_hasSpawnedToy = false;
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_bHasSpawnedSoccerBall[i] = false;
	}

	// If we're in a mode with upgrades, we force the players to recreate their weapons on next spawn.
	// This clears out any weapon upgrades they had in the previous round.
	if ( g_hUpgradeEntity )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pTFPlayer )
				continue;

			pTFPlayer->ForceItemRemovalOnRespawn();

			// Remove all player upgrades as well
			pTFPlayer->RemovePlayerAttributes( false );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RecalculateControlPointState( void )
{
	Assert( ObjectiveResource() );

	if ( !g_hControlPointMasters.Count() )
		return;

	if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
		return;

	for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
		if ( iFarthestPoint == -1 )
			continue;

		// Now enable all spawn points for that spawn point
		for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
		{
			CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );
			if ( pTFSpawn->GetControlPoint() )
			{
				if ( pTFSpawn->GetTeamNumber() == iTeam )
				{
					if ( pTFSpawn->GetControlPoint()->GetPointIndex() == iFarthestPoint )
					{
						pTFSpawn->SetDisabled( false );
					}
					else
					{
						pTFSpawn->SetDisabled( true );
					}
				}
			}
		}
	}
}

DECLARE_AUTO_LIST( ITFTeleportLocationAutoList )
class CTFTeleportLocation : public CPointEntity, public ITFTeleportLocationAutoList
{
public:
	DECLARE_CLASS( CTFTeleportLocation, CPointEntity );
};

IMPLEMENT_AUTO_LIST( ITFTeleportLocationAutoList );

LINK_ENTITY_TO_CLASS( tf_teleport_location, CTFTeleportLocation );

void SpawnRunes( void )
{
	// Spawn power-up runes when the round starts. Choice of spawn location and Powerup Type is random
	CUtlVector< CTFInfoPowerupSpawn* > vecSpawnPoints; 
	for ( int i = 0; i < IInfoPowerupSpawnAutoList::AutoList().Count(); i++ )
	{
		CTFInfoPowerupSpawn *pSpawnPoint = static_cast< CTFInfoPowerupSpawn* >( IInfoPowerupSpawnAutoList::AutoList()[i] );
		if ( !pSpawnPoint->IsDisabled() && !pSpawnPoint->HasRune() ) 
		{
			vecSpawnPoints.AddToTail( pSpawnPoint );
		}
	}

	Assert( vecSpawnPoints.Count() > 0 ); // We need at least one valid info_powerup_spawn

	// Warn if there aren't enough valid info_powerup_spawns to spawn all powerups
	if ( vecSpawnPoints.Count() < RUNE_TYPES_MAX )
	{
		Warning( "Warning: Not enough valid info_powerup_spawn locations found. You need a minimum of %i valid locations to spawn all Powerups.\n", RUNE_TYPES_MAX );
	}

	// try to spawn each rune type
	for ( int nRuneTypes = 0; nRuneTypes < RUNE_TYPES_MAX && vecSpawnPoints.Count() > 0; nRuneTypes++ )
	{
		int index = RandomInt( 0, vecSpawnPoints.Count() - 1 );
		CTFInfoPowerupSpawn *pSpawnPoint = vecSpawnPoints[index];
		CTFRune *pNewRune = CTFRune::CreateRune( pSpawnPoint->GetAbsOrigin(), (RuneTypes_t) nRuneTypes, TEAM_ANY, false, false );
		pSpawnPoint->SetRune( pNewRune );
		vecSpawnPoints.Remove( index );
	}
}


void CTFGameRules::RespawnPlayers( bool bForceRespawn, bool bTeam, int iTeam )
{
	// Skip the respawn at the beginning of a round in casual/comp mode since we already
	// handled it when the pre-round doors closed over the players' views
	if ( IsCompetitiveMode() && ( GetRoundsPlayed() == 0 ) && bForceRespawn && ( State_Get() == GR_STATE_BETWEEN_RNDS || State_Get() == GR_STATE_PREROUND ) )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( !pMaster || !pMaster->PlayingMiniRounds() || ( pMaster->GetCurrentRoundIndex() == 0 ) )
			return;
	}

	BaseClass::RespawnPlayers( bForceRespawn, bTeam, iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundStart( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		ObjectiveResource()->SetBaseCP( -1, i );
	}

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_iNumCaps[i] = 0;
	}

	SetOvertime( false );

	m_hRedKothTimer.Set( NULL );
	m_hBlueKothTimer.Set( NULL );

	// Let all entities know that a new round is starting
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	// All entities have been spawned, now activate them
	m_areHealthAndAmmoVectorsReady = false;
	m_ammoVector.RemoveAll();
	m_healthVector.RemoveAll();

	pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
	{
		// Find all the control points with associated spawnpoints
		memset( m_bControlSpawnsPerTeam, 0, sizeof(bool) * MAX_TEAMS * MAX_CONTROL_POINTS );
		for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
		{
			CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );
			if ( pTFSpawn->GetControlPoint() )
			{
				m_bControlSpawnsPerTeam[ pTFSpawn->GetTeamNumber() ][ pTFSpawn->GetControlPoint()->GetPointIndex() ] = true;
				pTFSpawn->SetDisabled( true );
			}
		}

		RecalculateControlPointState();

		SetRoundOverlayDetails();
	}

	//Do any round specific setup for training logic (resetting the score, messages, etc).
	if ( IsInTraining() )
	{
		if ( m_hTrainingModeLogic )
		{
			m_hTrainingModeLogic->SetupOnRoundStart();
		}
	}

	m_szMostRecentCappers[0] = 0;
	m_halloweenBossTimer.Invalidate();
	m_ghostVector.RemoveAll();

	m_zombieMobTimer.Invalidate();
	m_zombiesLeftToSpawn = 0;

	SetIT( NULL );
	SetBirthdayPlayer( NULL );

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->HideBossHealthMeter();
	}

#ifdef TF_RAID_MODE
	if ( IsBossBattleMode() )
	{
		CTFTeam *enemyTeam = GetGlobalTFTeam( TF_TEAM_RED );
		for( int i=0; i<enemyTeam->GetNumPlayers(); ++i )
		{
			CTFPlayer *who = ToTFPlayer( enemyTeam->GetPlayer( i ) );

			who->ChangeTeam( TEAM_SPECTATOR, false, true );
			who->RemoveAllObjects();
		}
	}
#endif // TF_RAID_MODE

	if ( IsMannVsMachineMode() )
	{
		if ( g_hMannVsMachineLogic )
		{
			g_hMannVsMachineLogic->SetupOnRoundStart();
		}
	}

	m_redPayloadToPush = NULL;
	m_bluePayloadToPush = NULL;
	m_redPayloadToBlock = NULL;
	m_bluePayloadToBlock = NULL;

	// Tell the clients to recalculate the holiday
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	UTIL_CalculateHolidays();

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		m_helltowerTimer.Start( HELLTOWER_TIMER_INTERVAL );
	}
	
	// reset hell state
	SetPlayersInHell( false );

	if ( IsPowerupMode() )
	{
		// Reset imbalance detection scores
		m_nPowerupKillsBlueTeam = 0;		
		m_nPowerupKillsRedTeam = 0;
		SpawnRunes();
	}

	m_hHolidayLogic = dynamic_cast<CTFHolidayEntity*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_holiday" ) );
	if ( m_hHolidayLogic.IsValid() )
	{
		m_hHolidayLogic->ResetWinner();
	}

	// cache off teleport locations and remove entities to save edicts
	m_mapTeleportLocations.PurgeAndDeleteElements();
	for ( int i=0; i<ITFTeleportLocationAutoList::AutoList().Count(); ++i )
	{
		CTFTeleportLocation *pLocation = static_cast< CTFTeleportLocation* >( ITFTeleportLocationAutoList::AutoList()[i] );

		int iMap = m_mapTeleportLocations.Find( pLocation->GetEntityName() );
		if ( !m_mapTeleportLocations.IsValidIndex( iMap ) )
		{
			CUtlVector< TeleportLocation_t > *pNew = new CUtlVector< TeleportLocation_t >;
			iMap = m_mapTeleportLocations.Insert( pLocation->GetEntityName(), pNew );
		}

		CUtlVector< TeleportLocation_t > *pLocations = m_mapTeleportLocations[iMap];
		int iLocation = pLocations->AddToTail();
		pLocations->Element( iLocation ).m_vecPosition = pLocation->GetAbsOrigin();
		pLocations->Element( iLocation ).m_qAngles = pLocation->GetAbsAngles();

		UTIL_Remove( pLocation );
	}

	// swap our train model for the EOTL holiday
	if ( IsHolidayActive( kHoliday_EOTL ) )
	{
		for ( int i = 0; i < IPhysicsPropAutoList::AutoList().Count(); i++ )
		{
			CPhysicsProp *pPhysicsProp = static_cast<CPhysicsProp*>( IPhysicsPropAutoList::AutoList()[i] );
			const char *pszTemp = pPhysicsProp->GetModelName().ToCStr();

			if ( FStrEq( pszTemp, "models/props_trainyard/bomb_cart.mdl" ) )
			{
				pPhysicsProp->SetModel( "models/props_trainyard/bomb_eotl_blue.mdl" );
			}
			else if ( FStrEq( pszTemp, "models/props_trainyard/bomb_cart_red.mdl" ) )
			{
				pPhysicsProp->SetModel( "models/props_trainyard/bomb_eotl_red.mdl" );
			}
		}
	}

	m_flMatchSummaryTeleportTime = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RestartTournament( void )
{
	BaseClass::RestartTournament();

	if ( GetStopWatchTimer() )
	{
		UTIL_Remove( GetStopWatchTimer() );
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer || !pTFPlayer->IsAlive() )
			continue;

		pTFPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED, true );
		pTFPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_BONUS_TIME, true );
		pTFPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_CTF_CAPTURE, true );
	}

	ItemSystem()->ReloadWhitelist();

	ResetPlayerAndTeamReadyState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleTeamScoreModify( int iTeam, int iScore )
{
	BaseClass::HandleTeamScoreModify( iTeam, iScore );

	if ( IsInStopWatch() == true )
	{
		if ( GetStopWatchTimer() )
		{
			if ( GetStopWatchTimer()->IsWatchingTimeStamps() == true )
			{
				GetStopWatchTimer()->SetStopWatchTimeStamp();
			}
	
			StopWatchModeThink();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::StopWatchShouldBeTimedWin_Calculate( void )
{
	m_bStopWatchShouldBeTimedWin = false;

	if ( IsInTournamentMode() && IsInStopWatch() && ObjectiveResource() )
	{
		int iStopWatchTimer = ObjectiveResource()->GetStopWatchTimer();
		CTeamRoundTimer *pStopWatch = dynamic_cast< CTeamRoundTimer* >( UTIL_EntityByIndex( iStopWatchTimer ) );
		if ( pStopWatch && !pStopWatch->IsWatchingTimeStamps() )
		{
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

			if ( pMaster == NULL )
				return;

			int iNumPoints = pMaster->GetNumPoints();

			CTFTeam *pAttacker = NULL;
			CTFTeam *pDefender = NULL;

			for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( i );

				if ( pTeam )
				{
					if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
					{
						pDefender = pTeam;
					}

					if ( pTeam->GetRole() == TEAM_ROLE_ATTACKERS )
					{
						pAttacker = pTeam;
					}
				}
			}

			if ( pAttacker == NULL || pDefender == NULL )
				return;

			m_bStopWatchShouldBeTimedWin = ( pDefender->GetScore() == iNumPoints );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::StopWatchShouldBeTimedWin( void )
{
	StopWatchShouldBeTimedWin_Calculate();
	return m_bStopWatchShouldBeTimedWin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::StopWatchModeThink( void )
{
	if ( IsInTournamentMode() == false || IsInStopWatch() == false )
		return;

	if ( GetStopWatchTimer() == NULL )
		return;

	CTeamRoundTimer *pTimer = GetStopWatchTimer();

	bool bWatchingCaps = pTimer->IsWatchingTimeStamps();
	
	CTFTeam *pAttacker = NULL;
	CTFTeam *pDefender = NULL;

	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( i );

		if ( pTeam )
		{
			if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
			{
				pDefender = pTeam;
			}

			if ( pTeam->GetRole() == TEAM_ROLE_ATTACKERS )
			{
				pAttacker = pTeam;
			}
		}
	}

	if ( pAttacker == NULL || pDefender == NULL )
		return;

	m_bStopWatchWinner.Set( false );

	if ( bWatchingCaps == false )
	{
		if ( pTimer->GetTimeRemaining() <= 0.0f )
		{
			if ( StopWatchShouldBeTimedWin() )
			{
				if ( pAttacker->GetScore() < pDefender->GetScore() )
				{
					m_bStopWatchWinner.Set( true );
					SetWinningTeam( pDefender->GetTeamNumber(), WINREASON_DEFEND_UNTIL_TIME_LIMIT, true, true );
				}
			}
			else
			{
				if ( pAttacker->GetScore() > pDefender->GetScore() )
				{
					m_bStopWatchWinner.Set( true );
					SetWinningTeam( pAttacker->GetTeamNumber(), WINREASON_ALL_POINTS_CAPTURED, true, true );	
				}
			}

			if ( pTimer->IsTimerPaused() == false )
			{
				variant_t sVariant;
				pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			}

			m_nStopWatchState.Set( STOPWATCH_OVERTIME );
		}
		else
		{
			if ( pAttacker->GetScore() >= pDefender->GetScore() )
			{
				m_bStopWatchWinner.Set( true );
				SetWinningTeam( pAttacker->GetTeamNumber(), WINREASON_ALL_POINTS_CAPTURED, true, true );
			}
		}
	}
	else
	{
		if ( pTimer->GetTimeRemaining() <= 0.0f )
		{
			m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );
		}
		else
		{
			m_nStopWatchState.Set( STOPWATCH_RUNNING );
		}
	}

	if ( m_bStopWatchWinner == true )
	{
		UTIL_Remove( pTimer	);
		m_hStopWatchTimer = NULL;
		m_flStopWatchTotalTime = -1.0f;
		m_bStopWatch = false;
		m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );

		ShouldResetRoundsPlayed( false );
		ShouldResetScores( true, false );
		ResetScores();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ManageStopwatchTimer( bool bInSetup ) 
{
	if ( IsInTournamentMode() == false )
		return;

	if ( mp_tournament_stopwatch.GetBool() == false )
		return;

	bool bAttacking = false;
	bool bDefending = false;

	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( i );

		if ( pTeam )
		{
			if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
			{
				bDefending = true;
			}

			if ( pTeam->GetRole() == TEAM_ROLE_ATTACKERS )
			{
				bAttacking = true;
			}
		}
	}

	if ( bDefending == true && bAttacking == true )
	{
		SetInStopWatch( true );
	}
	else
	{
		SetInStopWatch( false );
	}

	if ( IsInStopWatch() == true )
	{
		if ( m_hStopWatchTimer == NULL )
		{
			variant_t sVariant;
			CTeamRoundTimer *pStopWatch = (CTeamRoundTimer*)CBaseEntity::CreateNoSpawn( "team_round_timer", vec3_origin, vec3_angle );
			m_hStopWatchTimer = pStopWatch;

			pStopWatch->SetName( MAKE_STRING("zz_stopwatch_timer") );
			pStopWatch->SetShowInHud( false );
			pStopWatch->SetStopWatch( true );

			if ( m_flStopWatchTotalTime < 0.0f )
			{
				pStopWatch->SetCaptureWatchState( true );
				DispatchSpawn( pStopWatch );
			
				pStopWatch->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
			}
			else
			{
				DispatchSpawn( pStopWatch );
				pStopWatch->SetCaptureWatchState( false );
				

				sVariant.SetInt( m_flStopWatchTotalTime );
				pStopWatch->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
				pStopWatch->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				pStopWatch->SetAutoCountdown( true );
			}

			if ( bInSetup == true )
			{
				pStopWatch->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			}

			ObjectiveResource()->SetStopWatchTimer( pStopWatch );
		}
		else
		{
			if ( bInSetup == false )
			{
				variant_t sVariant;
				m_hStopWatchTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			}
			else
			{
				variant_t sVariant;
				m_hStopWatchTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetSetup( bool bSetup ) 
{ 
	if ( m_bInSetup == bSetup )
		return;

	BaseClass::SetSetup( bSetup );

	ManageStopwatchTimer( bSetup );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundRunning( void )
{
	// Let out control point masters know that the round has started
	for ( int i = 0; i < g_hControlPointMasters.Count(); i++ )
	{
		variant_t emptyVariant;
		if ( g_hControlPointMasters[i] )
		{
			g_hControlPointMasters[i]->AcceptInput( "RoundStart", NULL, NULL, emptyVariant, 0 );
		}
	}

	// Reset player speeds after preround lock
	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();
		if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
		{
			if ( !IsInWaitingForPlayers() )
			{
				pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
			}

		}
		else if ( !IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			// Use comp voice lines only for 6v6.  The guys talk about "Sixes" a lot, so it doesn't make sense to
			// use in other competitive modes.
			if ( GetCurrentMatchGroup() == k_eTFMatchGroup_Ladder_6v6 )
			{
				pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START_COMP );
			}
			else
			{
				pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
			}
		}

		// warp the coach to student
		if ( pPlayer->GetStudent() )
		{
			pPlayer->SetObserverTarget( pPlayer->GetStudent() );
			pPlayer->StartObserverMode( OBS_MODE_CHASE );
		}

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		pPlayer->m_Shared.ResetRoundScores();

		// Store the class they started the round as (tf_player captures everything after this)
		if ( pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
		{
			CSteamID steamID;
			pPlayer->GetSteamID( &steamID );

			CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
			if ( pMatch )
			{
				CMatchInfo::PlayerMatchData_t *pMatchPlayer = pMatch->GetMatchDataForPlayer( steamID );
				if ( pMatchPlayer )
				{
					pMatchPlayer->UpdateClassesPlayed( pPlayer->GetPlayerClass()->GetClassIndex() );
					pMatchPlayer->bPlayed = true;
				}
			}
		}
	}

	if ( IsPlayingSpecialDeliveryMode() && !IsInWaitingForPlayers() )
	{
		if ( !IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			BroadcastSound( 255, "Announcer.SD_RoundStart" );
		}
	}

	if ( IsMannVsMachineMode() && g_pPopulationManager )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "mvm_begin_wave" );
		if ( event )
		{
			event->SetInt( "wave_index", g_pPopulationManager->GetWaveNumber() );
			event->SetInt( "max_waves", g_pPopulationManager->GetTotalWaveCount() );
			event->SetInt( "advanced", g_pPopulationManager->IsAdvancedPopFile() ? 1 : 0 );
			gameeventmanager->FireEvent( event );
		}
	}

	if ( IsCompetitiveMode() && !( GetActiveRoundTimer() && ( GetActiveRoundTimer()->GetSetupTimeLength() > 0 ) ) )
	{
		// Announcer VO
		if ( ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() == ( mp_winlimit.GetInt() - 1 ) ) &&
			 ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() == ( mp_winlimit.GetInt() - 1 ) ) )
		{
			BroadcastSound( 255, "Announcer.CompFinalGameBeginsFight" );
		}
		else
		{
			BroadcastSound( 255, "Announcer.CompGameBeginsFight" );
		}
	}

	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch && IsMatchTypeCompetitive() )
	{
		static ConVarRef tf_bot_quota( "tf_bot_quota" );
		tf_bot_quota.SetValue( (int)pMatch->GetCanonicalMatchSize() );
		static ConVarRef tf_bot_quota_mode( "tf_bot_quota_mode" );
		tf_bot_quota_mode.SetValue( "fill" );
	}

	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->StateEnterRoundRunning();
	}

	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		PowerupModeInitKillCountTimer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end)
//-----------------------------------------------------------------------------
void CTFGameRules::PreviousRoundEnd( void )
{
	// before we enter a new round, fire the "end output" for the previous round
	if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
	{
		g_hControlPointMasters[0]->FireRoundEndOutput();
	}

	m_iPreviousRoundWinners = GetWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out)
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateStart( void )
{
	// Remove everyone's objects
	RemoveAllProjectilesAndBuildings();
	
	if ( IsInArenaMode() == false )
	{
		// Respawn all the players
		RespawnPlayers( true );

		// Disable all the active health packs in the world
		m_hDisabledHealthKits.Purge();
		CHealthKit *pHealthPack = gEntList.NextEntByClass( (CHealthKit *)NULL );
		while ( pHealthPack )
		{
			if ( !pHealthPack->IsDisabled() )
			{
				pHealthPack->SetDisabled( true );
				m_hDisabledHealthKits.AddToTail( pHealthPack );
			}
			pHealthPack = gEntList.NextEntByClass( pHealthPack );
		}
	}
	else
	{
		CArenaLogic *pArenaLogic = dynamic_cast< CArenaLogic * > (gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );

		if ( pArenaLogic )
		{
			pArenaLogic->m_OnArenaRoundStart.FireOutput( pArenaLogic, pArenaLogic );

			if ( tf_arena_override_cap_enable_time.GetFloat() > 0 )
			{
				m_flCapturePointEnableTime = gpGlobals->curtime + tf_arena_override_cap_enable_time.GetFloat();
			}
			else
			{
				m_flCapturePointEnableTime = gpGlobals->curtime + pArenaLogic->m_flTimeToEnableCapPoint;
			}

			IGameEvent *event = gameeventmanager->CreateEvent( "arena_round_start" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}

			BroadcastSound( 255, "Announcer.AM_RoundStartRandom" );
		}
	}

	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( IsInArenaMode() == true )
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
			pPlayer->TeamFortress_SetSpeed();
		}
		else
		{
			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
		}
	}

	m_flStalemateStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateEnd( void )
{
	// Reenable all the health packs we disabled
	for ( int i = 0; i < m_hDisabledHealthKits.Count(); i++ )
	{
		if ( m_hDisabledHealthKits[i] )
		{
			m_hDisabledHealthKits[i]->SetDisabled( false );
		}
	}

	m_hDisabledHealthKits.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitTeams( void )
{
	BaseClass::InitTeams();

	// clear the player class data
	ResetFilePlayerClassInfoDatabase();
}


class CTraceFilterIgnoreTeammatesWithException : public CTraceFilterSimple
{
	DECLARE_CLASS( CTraceFilterIgnoreTeammatesWithException, CTraceFilterSimple );
public:
	CTraceFilterIgnoreTeammatesWithException( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, const IHandleEntity *pExceptionEntity )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
		m_pExceptionEntity = pExceptionEntity;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pServerEntity == m_pExceptionEntity )
			return true;

		if ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
		{
			return false;
		}

		return true;
	}

	int m_iIgnoreTeam;
	const IHandleEntity *m_pExceptionEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( CTFRadiusDamageInfo &info )
{
	float flRadSqr = (info.flRadius * info.flRadius);

	int iDamageEnemies = 0;
	int nDamageDealt = 0;
	// Some weapons pass a radius of 0, since their only goal is to give blast jumping ability
	if ( info.flRadius > 0 )
	{
		// Find all the entities in the radius, and attempt to damage them.
		CBaseEntity *pEntity = NULL;
		for ( CEntitySphereQuery sphere( info.vecSrc, info.flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// Skip the attacker, if we have a RJ radius set. We'll do it post.
			if ( info.flRJRadius && pEntity == info.dmgInfo->GetAttacker() )
				continue;

			// CEntitySphereQuery actually does a box test. So we need to make sure the distance is less than the radius first.
			Vector vecPos;
			pEntity->CollisionProp()->CalcNearestPoint( info.vecSrc, &vecPos );
			if ( (info.vecSrc - vecPos).LengthSqr() > flRadSqr )
				continue;

			int iDamageToEntity = info.ApplyToEntity( pEntity );
			if ( iDamageToEntity )
			{
				// Keep track of any enemies we damaged
				if ( pEntity->IsPlayer() && !pEntity->InSameTeam( info.dmgInfo->GetAttacker() ) )
				{
					nDamageDealt+= iDamageToEntity;
					iDamageEnemies++;
				}
			}
		}
	}

	// If we have a set RJ radius, use it to affect the inflictor. This way Rocket Launchers 
	// with modified damage/radius perform like the base rocket launcher when it comes to RJing.
	if ( info.flRJRadius && info.dmgInfo->GetAttacker() )
	{
		// Set our radius & damage to the base RL
		info.flRadius = info.flRJRadius;
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.dmgInfo->GetWeapon());
		if ( pWeapon )
		{
			float flBaseDamage = pWeapon->GetTFWpnData().GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage;
			info.dmgInfo->SetDamage( flBaseDamage );
			info.dmgInfo->CopyDamageToBaseDamage();
			info.dmgInfo->SetDamagedOtherPlayers( iDamageEnemies );

			// If we dealt damage, check radius life leech
			if ( nDamageDealt > 0 )
			{
				// Heal on hits
				int iModHealthOnHit = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.dmgInfo->GetWeapon(), iModHealthOnHit, add_health_on_radius_damage );
				if ( iModHealthOnHit )
				{
					// Scale Health mod with damage dealt, input being the maximum amount of health possible
					float flScale = Clamp( nDamageDealt / flBaseDamage, 0.f, 1.0f );
					iModHealthOnHit = (int)( (float)iModHealthOnHit * flScale );
					int iHealed = info.dmgInfo->GetAttacker()->TakeHealth( iModHealthOnHit, DMG_GENERIC );
					if ( iHealed )
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
						if ( event )
						{
							event->SetInt( "amount", iHealed );
							event->SetInt( "entindex", info.dmgInfo->GetAttacker()->entindex() );
							item_definition_index_t healingItemDef = INVALID_ITEM_DEF_INDEX;
							if ( pWeapon->GetAttributeContainer() && pWeapon->GetAttributeContainer()->GetItem() )
							{
								healingItemDef = pWeapon->GetAttributeContainer()->GetItem()->GetItemDefIndex();
							}
							event->SetInt( "weapon_def_index", healingItemDef );
							gameeventmanager->FireEvent( event );
						}
					}
				}
			}
		}

		// Apply ourselves to our attacker, if we're within range
		Vector vecPos;
		info.dmgInfo->GetAttacker()->CollisionProp()->CalcNearestPoint( info.vecSrc, &vecPos );
		if ( (info.vecSrc - vecPos).LengthSqr() <= (info.flRadius * info.flRadius) )
		{
			info.ApplyToEntity( info.dmgInfo->GetAttacker() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the damage falloff over distance
//-----------------------------------------------------------------------------
void CTFRadiusDamageInfo::CalculateFalloff( void )
{
	if ( dmgInfo->GetDamageType() & DMG_RADIUS_MAX )
		flFalloff = 0.f;
	else if ( dmgInfo->GetDamageType() & DMG_HALF_FALLOFF )
		flFalloff = 0.5f;
	else if ( flRadius )
		flFalloff = dmgInfo->GetDamage() / flRadius;
	else
		flFalloff = 1.f;

	CBaseEntity *pWeapon = dmgInfo->GetWeapon();
	if ( pWeapon != NULL )
	{
		float flFalloffMod = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flFalloffMod, mult_dmg_falloff );
		if ( flFalloffMod != 1.f )
		{
			flFalloff += flFalloffMod;
		}
	}

	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		CTFPlayer *pOwner = ToTFPlayer( dmgInfo->GetAttacker() );
		if ( pOwner && pOwner->m_Shared.GetCarryingRuneType() == RUNE_PRECISION && !pOwner->m_bIsInMannpowerDominantCondition )
		{
			flFalloff = 1.0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to apply the radius damage to the specified entity
//-----------------------------------------------------------------------------
int CTFRadiusDamageInfo::ApplyToEntity( CBaseEntity *pEntity )
{
	if ( pEntity == pEntityIgnore || pEntity->m_takedamage == DAMAGE_NO )
		return 0;

	trace_t	tr;
	CBaseEntity *pInflictor = dmgInfo->GetInflictor();

	// Check that the explosion can 'see' this entity.
	Vector vecSpot = pEntity->BodyTarget( vecSrc, false );
	CTraceFilterIgnorePlayers filterPlayers( pInflictor, COLLISION_GROUP_PROJECTILE );
	CTraceFilterIgnoreProjectiles filterProjectiles( pInflictor, COLLISION_GROUP_PROJECTILE );
	CTraceFilterIgnoreFriendlyCombatItems filterCombatItems( pInflictor, COLLISION_GROUP_PROJECTILE, pInflictor->GetTeamNumber() );
	CTraceFilterChain filterPlayersAndProjectiles( &filterPlayers, &filterProjectiles );
	CTraceFilterChain filter( &filterPlayersAndProjectiles, &filterCombatItems );

	UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );
	if ( tr.startsolid && tr.m_pEnt )
	{
		// Return when inside an enemy combat shield and tracing against a player of that team ("absorbed")
		if ( tr.m_pEnt->IsCombatItem() && pEntity->InSameTeam( tr.m_pEnt ) && ( pEntity != tr.m_pEnt ) )
			return 0;

		filterPlayers.SetPassEntity( tr.m_pEnt );
		CTraceFilterChain filterSelf( &filterPlayers, &filterCombatItems );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filterSelf, &tr );
	}

	// If we don't trace the whole way to the target, and we didn't hit the target entity, we're blocked
	if ( tr.fraction != 1.f && tr.m_pEnt != pEntity )
	{
		// Don't let projectiles block damage
		return 0;
	}

	// Adjust the damage - apply falloff.
	float flAdjustedDamage = 0.0f;
	float flDistanceToEntity;

	// Rockets store the ent they hit as the enemy and have already dealt full damage to them by this time
	if ( pInflictor && ( pEntity == pInflictor->GetEnemy() ) )
	{
		// Full damage, we hit this entity directly
		flDistanceToEntity = 0;
	}
	else if ( pEntity->IsPlayer() )
	{
		// Use whichever is closer, absorigin or worldspacecenter
		float flToWorldSpaceCenter = ( vecSrc - pEntity->WorldSpaceCenter() ).Length();
		float flToOrigin = ( vecSrc - pEntity->GetAbsOrigin() ).Length();

		flDistanceToEntity = MIN( flToWorldSpaceCenter, flToOrigin );
	}
	else
	{
		flDistanceToEntity = ( vecSrc - tr.endpos ).Length();
	}

	flAdjustedDamage = RemapValClamped( flDistanceToEntity, 0, flRadius, dmgInfo->GetDamage(), dmgInfo->GetDamage() * flFalloff );

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(dmgInfo->GetWeapon());
	
	// Grenades & Pipebombs do less damage to ourselves.
	if ( pEntity == dmgInfo->GetAttacker() && pWeapon )
	{
		switch( pWeapon->GetWeaponID() )
		{
			case TF_WEAPON_PIPEBOMBLAUNCHER :
			case TF_WEAPON_GRENADELAUNCHER :
			case TF_WEAPON_CANNON :
			case TF_WEAPON_STICKBOMB :
				flAdjustedDamage *= 0.75f;
				break;
		}
	}

	// If we end up doing 0 damage, exit now.
	if ( flAdjustedDamage <= 0.f )
		return 0;

	// the explosion can 'see' this entity, so hurt them!
	if (tr.startsolid)
	{
		// if we're stuck inside them, fixup the position and distance
		tr.endpos = vecSrc;
		tr.fraction = 0.f;
	}

	CTakeDamageInfo adjustedInfo = *dmgInfo;
	adjustedInfo.SetDamage( flAdjustedDamage );

	Vector dir = vecSpot - vecSrc;
	VectorNormalize( dir );

	// If we don't have a damage force, manufacture one
	if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
	{
		CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc );
	}
	else
	{
		// Assume the force passed in is the maximum force. Decay it based on falloff.
		float flForce = adjustedInfo.GetDamageForce().Length() * flFalloff;
		adjustedInfo.SetDamageForce( dir * flForce );
		adjustedInfo.SetDamagePosition( vecSrc );
	}

	adjustedInfo.ScaleDamageForce( m_flForceScale );

	int nDamageTaken = 0;
	if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
	{
		ClearMultiDamage( );
		pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
		ApplyMultiDamage();
	}
	else
	{
		nDamageTaken = pEntity->TakeDamage( adjustedInfo );
	}

	// Now hit all triggers along the way that respond to damage.
	pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );

	// Tell the projectile how many enemy players it hit
	if ( pEntity->IsPlayer() && dmgInfo->GetInflictor() )
	{
		CBaseProjectile* pProjectile = dynamic_cast< CBaseProjectile *>( dmgInfo->GetInflictor() );
		if ( pProjectile )
		{
			pProjectile->RecordEnemyPlayerHit( pEntity, false );
		}
	}

	return nDamageTaken;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	// This shouldn't be used. Call the version above that takes a CTFRadiusDamageInfo pointer.
	Assert(0);

	CTakeDamageInfo dmgInfo = info;
	Vector vecSrc = vecSrcIn;
	CTFRadiusDamageInfo radiusinfo( &dmgInfo, vecSrc, flRadius, pEntityIgnore );
	RadiusDamage(radiusinfo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ApplyOnDamageModifyRules( CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, bool bAllowDamage )
{
	info.SetDamageForForceCalc( info.GetDamage() );
	bool bDebug = tf_debug_damage.GetBool();

	CTFPlayer *pVictim = ToTFPlayer( pVictimBaseEntity );
	CBaseEntity *pAttacker = info.GetAttacker();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );

	int iAttackIgnoresResists = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iAttackIgnoresResists, mod_pierce_resists_absorbs );

	// damage may not come from a weapon (ie: Bosses, etc)
	// The existing code below already checked for NULL pWeapon, anyways
	float flDamage = info.GetDamage();

	bool bShowDisguisedCrit = false;
	bool bAllSeeCrit = false;
	EAttackBonusEffects_t eBonusEffect = kBonusEffect_None;

	if ( pVictim )
	{
		pVictim->SetSeeCrit( false, false, false );
		pVictim->SetAttackBonusEffect( kBonusEffect_None );
	}

	int bitsDamage = info.GetDamageType();

	// Capture this before anybody mucks with it
	if ( !info.BaseDamageIsValid() )
	{
		info.CopyDamageToBaseDamage();
	}

	// Damage type was already crit (Flares / headshot)
	if ( bitsDamage & DMG_CRITICAL )
	{
		info.SetCritType( CTakeDamageInfo::CRIT_FULL );
	}

	// First figure out whether this is going to be a full forced crit for some specific reason. It's
	// important that we do this before figuring out whether we're going to be a minicrit or not.

	// Allow attributes to force critical hits on players with specific conditions
	if ( pVictim )
	{
		// Crit against players that have these conditions
		int iCritDamageTypes = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritDamageTypes, or_crit_vs_playercond );

		if ( iCritDamageTypes )
		{
			// iCritDamageTypes is an or'd list of types. We need to pull each bit out and
			// then test against what that bit in the items_master file maps to.
			for ( int i = 0; condition_to_attribute_translation[i] != TF_COND_LAST; i++ )
			{
				if ( iCritDamageTypes & ( 1 << i ) )
				{
					if ( pVictim->m_Shared.InCond( condition_to_attribute_translation[ i ] ) )
					{
						bitsDamage |= DMG_CRITICAL;
						info.AddDamageType( DMG_CRITICAL );
						info.SetCritType( CTakeDamageInfo::CRIT_FULL );

						if ( condition_to_attribute_translation[i] == TF_COND_DISGUISED || 
							 condition_to_attribute_translation[i] == TF_COND_DISGUISING )
						{
							// if our attribute specifically crits disguised enemies we need to show it on the client
							bShowDisguisedCrit = true;
						}
						break;
					}
				}
			}
		}

		int iCritVsWet = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritVsWet, crit_vs_wet_players );
		if ( iCritVsWet )
		{
			float flWaterExitTime = pVictim->GetWaterExitTime();

			if ( pVictim->m_Shared.InCond( TF_COND_URINE ) ||
				 pVictim->m_Shared.InCond( TF_COND_MAD_MILK ) ||
				 pVictim->m_Shared.InCond( TF_COND_GAS ) ||
			   ( pVictim->GetWaterLevel() > WL_NotInWater ) ||
			   ( ( flWaterExitTime > 0 ) && ( gpGlobals->curtime - flWaterExitTime < 5.0f ) ) ) // or they exited the water in the last few seconds
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType( DMG_CRITICAL );
				info.SetCritType( CTakeDamageInfo::CRIT_FULL );

				if ( pWeapon && ( pWeapon->GetWeaponID() == TF_WEAPON_BREAKABLE_SIGN ) )
				{
					pWeapon->SetBroken( true );
				}
			}
		}
 
		// Crit against players that don't have these conditions
		int iCritDamageNotTypes = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritDamageNotTypes, or_crit_vs_not_playercond );

		if ( iCritDamageNotTypes )
		{
			// iCritDamageTypes is an or'd list of types. We need to pull each bit out and
			// then test against what that bit in the items_master file maps to.
			for ( int i = 0; condition_to_attribute_translation[i] != TF_COND_LAST; i++ )
			{
				if ( iCritDamageNotTypes & ( 1 << i ) )
				{
					if ( !pVictim->m_Shared.InCond( condition_to_attribute_translation[ i ] ) )
					{
						bitsDamage |= DMG_CRITICAL;
						info.AddDamageType( DMG_CRITICAL );
						info.SetCritType( CTakeDamageInfo::CRIT_FULL );

						if ( condition_to_attribute_translation[ i ] == TF_COND_DISGUISED || 
							 condition_to_attribute_translation[ i ] == TF_COND_DISGUISING )
						{
							// if our attribute specifically crits disguised enemies we need to show it on the client
							bShowDisguisedCrit = true;
						}
						break;
					}
				}
			}
		}

		// Crit burning behind
		int iCritBurning = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritBurning, axtinguisher_properties );
		if ( iCritBurning && pVictim->m_Shared.InCond( TF_COND_BURNING ) )
		{
			// Full crit in back, mini in front
			Vector toEnt = pVictim->GetAbsOrigin() - pTFAttacker->GetAbsOrigin();
			{
				Vector entForward;
				AngleVectors( pVictim->EyeAngles(), &entForward );
				toEnt.z = 0;
				toEnt.NormalizeInPlace();

				if ( DotProduct( toEnt, entForward ) > 0.0f )	// 90 degrees from center (total of 180)
				{
					bitsDamage |= DMG_CRITICAL;
					info.AddDamageType( DMG_CRITICAL );
					info.SetCritType( CTakeDamageInfo::CRIT_FULL );
				}
				else
				{
					bAllSeeCrit = true;
					info.SetCritType( CTakeDamageInfo::CRIT_MINI );
					eBonusEffect = kBonusEffect_MiniCrit;
				}
			}
		}
	}
	// no airborne crit bonus in Mannpower
	if ( !IsPowerupMode() )
	{
		int iCritWhileAirborne = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritWhileAirborne, crit_while_airborne );
		if ( iCritWhileAirborne && pTFAttacker )
		{
			if ( pTFAttacker->InAirDueToExplosion() )
			{
				bitsDamage |= DMG_CRITICAL;
				info.AddDamageType( DMG_CRITICAL );
				info.SetCritType( CTakeDamageInfo::CRIT_FULL );
			}
		}
	}
	
	// For awarding assist damage stat later
	ETFCond eDamageBonusCond = TF_COND_LAST;

	// Some forms of damage override long range damage falloff
	bool bIgnoreLongRangeDmgEffects = false;

	// Figure out if it's a minicrit or not
	// But we never minicrit ourselves.
	if ( pAttacker != pVictimBaseEntity )
	{
		// attack_minicrits_and_consumes_burning
		if ( pWeapon && pTFAttacker && pVictim && pVictim->m_Shared.InCond( TF_COND_BURNING ) )
		{
			int iConsumeFlames = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker->GetActiveWeapon(), iConsumeFlames, attack_minicrits_and_consumes_burning );
			if ( iConsumeFlames && pWeapon == pTFAttacker->GetActiveWeapon() && ( info.GetDamageType() & DMG_MELEE ) )
			{
				float flConsumeBonus = RemapValClamped( pVictim->m_Shared.GetAfterburnDuration(), 0.5f, (float)TF_BURNING_FLAME_LIFE, 20.f, (float)( TF_BURNING_DMG * 20 ) );
				flDamage += flConsumeBonus;
				pVictim->m_Shared.RemoveCond( TF_COND_BURNING );
				pVictim->EmitSound( "TFPlayer.FlameOut" );

				if ( info.GetCritType() == CTakeDamageInfo::CRIT_NONE )
				{
					info.SetCritType( CTakeDamageInfo::CRIT_MINI );
					eBonusEffect = kBonusEffect_MiniCrit;
				}

				info.SetDamageCustom( TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED );
			}
		}

		if ( info.GetCritType() == CTakeDamageInfo::CRIT_NONE )
		{
			CBaseEntity *pInflictor = info.GetInflictor();
			CTFGrenadePipebombProjectile *pBaseGrenade = dynamic_cast< CTFGrenadePipebombProjectile* >( pInflictor );
			CTFBaseRocket *pBaseRocket = dynamic_cast< CTFBaseRocket* >( pInflictor );

			if ( pVictim && ( pVictim->m_Shared.InCond( TF_COND_URINE ) ||
			                  pVictim->m_Shared.InCond( TF_COND_MARKEDFORDEATH ) ||
			                  pVictim->m_Shared.InCond( TF_COND_MARKEDFORDEATH_SILENT ) ||
			                  pVictim->m_Shared.InCond( TF_COND_PASSTIME_PENALTY_DEBUFF ) ) )
			{
				bAllSeeCrit = true;
				info.SetCritType( CTakeDamageInfo::CRIT_MINI );
				eBonusEffect = kBonusEffect_MiniCrit;

				if ( !pVictim->m_Shared.InCond( TF_COND_MARKEDFORDEATH_SILENT ) )
				{
					eDamageBonusCond = pVictim->m_Shared.InCond( TF_COND_URINE ) ? TF_COND_URINE : TF_COND_MARKEDFORDEATH;
				}
			}
			else if ( pTFAttacker && ( pTFAttacker->m_Shared.InCond( TF_COND_OFFENSEBUFF ) || pTFAttacker->m_Shared.InCond( TF_COND_NOHEALINGDAMAGEBUFF ) ) )
			{
				// Attackers buffed by the soldier do mini-crits.
				bAllSeeCrit = true;
				info.SetCritType( CTakeDamageInfo::CRIT_MINI );
				eBonusEffect = kBonusEffect_MiniCrit;

				if ( pTFAttacker->m_Shared.InCond( TF_COND_OFFENSEBUFF ) )
				{
					eDamageBonusCond = TF_COND_OFFENSEBUFF;
				}
			}
			else if ( pTFAttacker && (bitsDamage & DMG_RADIUS_MAX) && pWeapon && ( (pWeapon->GetWeaponID() == TF_WEAPON_SWORD) || (pWeapon->GetWeaponID() == TF_WEAPON_BOTTLE)|| (pWeapon->GetWeaponID() == TF_WEAPON_WRENCH) ) )
			{
				// First sword or bottle attack after a charge is a mini-crit.
				bAllSeeCrit = true;
				info.SetCritType( CTakeDamageInfo::CRIT_MINI );
				eBonusEffect = kBonusEffect_MiniCrit;
			}
			else if ( ( pInflictor && pInflictor->IsPlayer() == false ) && ( ( pBaseRocket && pBaseRocket->GetDeflected() ) || ( pBaseGrenade && pBaseGrenade->GetDeflected() && ( pBaseGrenade->ShouldMiniCritOnReflect() ) ) ) )
			{
				// Reflected rockets, grenades (non-remote detonate), arrows always mini-crit
				info.SetCritType( CTakeDamageInfo::CRIT_MINI );
				eBonusEffect = kBonusEffect_MiniCrit;
			}
			else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_PLASMA_CHARGED )
			{
				// Charged plasma shots do minicrits.
				info.SetCritType( CTakeDamageInfo::CRIT_MINI );
				eBonusEffect = kBonusEffect_MiniCrit;
			}
			else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_CLEAVER_CRIT )
			{
				// Long range cleaver hit
				info.SetCritType( CTakeDamageInfo::CRIT_MINI );
				eBonusEffect = kBonusEffect_MiniCrit;
			}
			else if ( pTFAttacker && ( pTFAttacker->m_Shared.InCond( TF_COND_ENERGY_BUFF ) ) )
			{
				// Scouts using crit drink do mini-crits, as well as receive them
				info.SetCritType( CTakeDamageInfo::CRIT_MINI );
				eBonusEffect = kBonusEffect_MiniCrit;
			}
			else if ( ( info.GetDamageType() & DMG_IGNITE ) && pVictim && pVictim->m_Shared.InCond( TF_COND_BURNING ) && info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING_FLARE )
			{
				CTFFlareGun *pFlareGun = dynamic_cast< CTFFlareGun* >( pWeapon );
				if ( pFlareGun && pFlareGun->GetFlareGunType() != FLAREGUN_GRORDBORT )
				{
					info.SetCritType( CTakeDamageInfo::CRIT_MINI );
					eBonusEffect = kBonusEffect_MiniCrit;
				}
			}
			else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_FLARE_PELLET )
			{
				CBaseEntity *pInflictor = info.GetInflictor();
				CTFProjectile_Flare *pFlare = dynamic_cast< CTFProjectile_Flare* >( pInflictor );
				if ( pFlare && pFlare->IsFromTaunt() && pFlare->GetTimeAlive() < 0.05f )
				{
					// Taunt crits fired from the scorch shot at short range are super powerful!
					flDamage += Max( 400.f, flDamage );
				}
			}
			else if( pTFAttacker && pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_CANNON && ( info.GetDamageType() & DMG_BLAST ) )
			{
				CTFGrenadeLauncher* pGrenadeLauncher = static_cast<CTFGrenadeLauncher*>( pWeapon );
				if( pGrenadeLauncher->IsDoubleDonk( pVictim ) )
				{
					info.SetCritType( CTakeDamageInfo::CRIT_MINI );
					eBonusEffect = kBonusEffect_DoubleDonk;
					flDamage = Max( flDamage, info.GetMaxDamage() ); // Double donk victims score max damage
					EconEntity_OnOwnerKillEaterEvent( pGrenadeLauncher, pTFAttacker, pVictim, kKillEaterEvent_DoubleDonks );
				}
			}
			else if ( pTFAttacker && pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAME_BALL && info.GetDamageCustom() == TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING )
			{
				eBonusEffect = kBonusEffect_DragonsFury;
			}
			else if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_SCOUT ) && !( pTFAttacker->GetFlags() & FL_ONGROUND ) )
			{
				// Make sure the weapon that did this damage is the same as the one that grants mini-crits
				if ( info.GetWeapon() == pTFAttacker->GetActiveTFWeapon() )
				{
					int iDashCount = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker->GetActiveTFWeapon(), iDashCount, air_dash_count );
					if ( iDashCount )
					{
						info.SetCritType( CTakeDamageInfo::CRIT_MINI );
						eBonusEffect = kBonusEffect_MiniCrit;
					}
				}
			}
			else if ( pVictim && pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_SNIPER ) && pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
			{
				if ( IsHeadshot( info.GetDamageCustom() ) || pVictim->LastHitGroup() == HITGROUP_HEAD )
				{
					CTFSniperRifle *pSniper = static_cast< CTFSniperRifle* >( pWeapon );
					if ( pSniper->IsZoomed() && pSniper->GetJarateTime() )
					{
						float flJarateTime = pSniper->GetJarateTime();
						if ( flJarateTime >= 1.f )
						{
							info.SetCritType( CTakeDamageInfo::CRIT_MINI );
							eBonusEffect = kBonusEffect_MiniCrit;
						}
					}
				}
			}
			else
			{
				// Allow Attributes to shortcut out if found, no need to check all of them
				for ( int i = 0; i < 1; ++i )
				{
					// Some weapons force minicrits on burning targets.
					// Does not work for burn but works for ignite
					int iForceMiniCritOnBurning = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iForceMiniCritOnBurning, or_minicrit_vs_playercond_burning );
					if ( iForceMiniCritOnBurning == 1 && pVictim && pVictim->m_Shared.InCond( TF_COND_BURNING ) && !( info.GetDamageType() & DMG_BURN ) )
					{
						bAllSeeCrit = true;
						info.SetCritType( CTakeDamageInfo::CRIT_MINI );
						eBonusEffect = kBonusEffect_MiniCrit;
						break;
					}

					// Some weapons mini-crit airborne targets. Airborne targets are any target that has been knocked 
					// into the air by an explosive force from an enemy.
					// no airborne crits or mini crits in Mannpower since the whole idea is to fly around. It's too easy to score crits against grappling players, and we don't want to penalize airborne targets
					if ( !IsPowerupMode() )
					{
						int iMiniCritAirborne = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritAirborne, mini_crit_airborne );
						if ( iMiniCritAirborne == 1 &&	pVictim &&	( pVictim->InAirDueToExplosion() ) )
						{
							bAllSeeCrit = true;
							info.SetCritType( CTakeDamageInfo::CRIT_MINI );
							eBonusEffect = kBonusEffect_MiniCrit;
							break;
						}
					}

					//// Some weapons minicrit *any* target in the air, regardless of how they got there.
					//int iMiniCritAirborneDeploy = 0;
					//CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritAirborneDeploy, mini_crit_airborne_deploy );
					//if ( iMiniCritAirborneDeploy > 0 &&
					//	 pWeapon &&
					//	 ( gpGlobals->curtime - pWeapon->GetLastDeployTime() ) < iMiniCritAirborneDeploy &&
					//	 //

					//	 pVictim && !( pVictim->GetFlags() & FL_ONGROUND ) &&
					//	 ( pVictim->GetWaterLevel() == WL_NotInWater ) )
					//{
					//	bAllSeeCrit = true;
					//	info.SetCritType( CTakeDamageInfo::CRIT_MINI );
					//	eBonusEffect = kBonusEffect_MiniCrit;
					//	break;
					//}
				}
			}

			// Don't do long range distance falloff when pAttacker has Rocket Specialist attrib and directly hits an enemy
			if ( pBaseRocket && pBaseRocket->GetEnemy() && pBaseRocket->GetEnemy() == pVictimBaseEntity )
			{
				int iRocketSpecialist = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iRocketSpecialist, rocket_specialist );
				if ( iRocketSpecialist )
					bIgnoreLongRangeDmgEffects = true;
			}

			// Some Powerups remove distance damage falloff
			if ( pTFAttacker && ( pTFAttacker->m_Shared.GetCarryingRuneType() == RUNE_STRENGTH || pTFAttacker->m_Shared.GetCarryingRuneType() == RUNE_PRECISION ) )
			{
				bIgnoreLongRangeDmgEffects = true;
			}

			if ( pTFAttacker && pVictim )
			{
				// MiniCrit a victims back at close range
				int iMiniCritBackAttack = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritBackAttack, closerange_backattack_minicrits );
				Vector toEnt = pVictim->GetAbsOrigin() - pTFAttacker->GetAbsOrigin();
				if ( iMiniCritBackAttack == 1 && toEnt.LengthSqr() < Square( 512.0f ) )
				{
					Vector entForward;
					AngleVectors( pVictim->EyeAngles(), &entForward );
					toEnt.z = 0;
					toEnt.NormalizeInPlace();

					if ( DotProduct( toEnt, entForward ) > 0.259f )	// 75 degrees from center (total of 150)
					{
						bAllSeeCrit = true;
						info.SetCritType( CTakeDamageInfo::CRIT_MINI );
						eBonusEffect = kBonusEffect_MiniCrit;
					}
				}
			}
		}
	}

	if ( info.GetCritType() == CTakeDamageInfo::CRIT_MINI )
	{
		if ( IsPowerupMode() && ( info.GetDamageType() & DMG_MELEE ) )
		{
			flDamage /= 1.3;
		}
		int iPromoteMiniCritToCrit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iPromoteMiniCritToCrit, minicrits_become_crits );
		if ( iPromoteMiniCritToCrit == 1 )
		{
			info.SetCritType( CTakeDamageInfo::CRIT_FULL );
			eBonusEffect = kBonusEffect_Crit;
			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType( DMG_CRITICAL );
		}
	}

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BOOTS_STOMP )
	{
		eBonusEffect = kBonusEffect_Stomp;
	}

	if ( pVictim )
	{
		pVictim->SetSeeCrit( bAllSeeCrit, info.GetCritType() == CTakeDamageInfo::CRIT_MINI, bShowDisguisedCrit );
		pVictim->SetAttackBonusEffect( eBonusEffect );
	}

	// If we're invulnerable, force ourselves to only take damage events only, so we still get pushed
	if ( pVictim && pVictim->m_Shared.IsInvulnerable() )
	{
		if ( !bAllowDamage )
		{
			int iOldTakeDamage = pVictim->m_takedamage;
			pVictim->m_takedamage = DAMAGE_EVENTS_ONLY;
			// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice
			pVictim->CBaseCombatCharacter::OnTakeDamage( info );
			pVictim->m_takedamage = iOldTakeDamage;

			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) )
			{
				pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT );
			}

			// If this is critical explosive damage, and the Medic giving us invuln triggered 
			// it in the last second, he's earned himself an achievement. 
			if ( (bitsDamage & DMG_CRITICAL) && (bitsDamage & DMG_BLAST) )
			{
				pVictim->m_Shared.CheckForAchievement( ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE );
			}

			return false;
		}
	}

	// Apply attributes that increase damage vs players
	if ( pWeapon )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flDamage, mult_dmg_vs_players );

		// Check if we're to boost damage against the same class
		if( pVictim && pTFAttacker )
		{
			int nVictimClass	= pVictim->GetPlayerClass()->GetClassIndex();
			int nAttackerClass	= pTFAttacker->GetPlayerClass()->GetClassIndex();

			// Same class?
			if( nVictimClass == nAttackerClass )
			{
				// Potentially boost damage
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flDamage, mult_dmg_vs_same_class );
			}
		}
	}

	if ( pVictim && !pVictim->m_Shared.InCond( TF_COND_BURNING ) )
	{
		if ( bitsDamage & DMG_CRITICAL )
		{
			if ( pTFAttacker && !pTFAttacker->m_Shared.IsCritBoosted() )
			{
				int iNonBurningCritsDisabled = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iNonBurningCritsDisabled, set_nocrit_vs_nonburning );
				if ( iNonBurningCritsDisabled )
				{
					bitsDamage &= ~DMG_CRITICAL;
					info.SetDamageType( info.GetDamageType() & (~DMG_CRITICAL) );
					info.SetCritType( CTakeDamageInfo::CRIT_NONE );
				}
			}
		}

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flDamage, mult_dmg_vs_nonburning );
	}

	// Alien Isolation SetBonus Checking
	if ( pVictim && pTFAttacker && pWeapon )
	{
		// Alien->Merc melee bonus
		if ( ( info.GetDamageType() & (DMG_CLUB|DMG_SLASH) ) && info.GetDamageCustom() != TF_DMG_CUSTOM_BASEBALL )
		{
			CTFWeaponBaseMelee *pMelee = dynamic_cast<CTFWeaponBaseMelee*>( pWeapon );
			if ( pMelee )
			{
				int iAttackerAlien = 0;
				int iVictimMerc = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iAttackerAlien, alien_isolation_xeno_bonus_pos );
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pVictim, iVictimMerc, alien_isolation_merc_bonus_pos );

				if ( iAttackerAlien && iVictimMerc )
				{
					flDamage *= 5.f;
				}
			}
		}

		// Merc->Alien MK50 damage, aka flamethrower
		if ( ( info.GetDamageType() & DMG_IGNITE ) && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
		{
			int iAttackerMerc = 0;
			int iVictimAlien = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iAttackerMerc, alien_isolation_merc_bonus_pos );
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pVictim, iVictimAlien, alien_isolation_xeno_bonus_pos );

			if ( iAttackerMerc && iVictimAlien )
			{
				flDamage *= 3.f;
			}
		}
	}

	// Use defense buffs if it's not a backstab or direct crush damage (telefrage, etc.)
	if ( pVictim && info.GetDamageCustom() != TF_DMG_CUSTOM_BACKSTAB && ( info.GetDamageType() & DMG_CRUSH ) == 0 )
	{
		if ( pVictim->m_Shared.InCond( TF_COND_DEFENSEBUFF ) )
		{
			// We take no crits of any kind...
			if( eBonusEffect == kBonusEffect_MiniCrit || eBonusEffect == kBonusEffect_Crit )
				eBonusEffect = kBonusEffect_None;
			info.SetCritType( CTakeDamageInfo::CRIT_NONE );
			bAllSeeCrit = false;
			bShowDisguisedCrit = false;

			pVictim->SetSeeCrit( bAllSeeCrit, false, bShowDisguisedCrit );
			pVictim->SetAttackBonusEffect( eBonusEffect );

			bitsDamage &= ~DMG_CRITICAL;
			info.SetDamageType( bitsDamage );
			info.SetCritType( CTakeDamageInfo::CRIT_NONE );
		}

		if ( !iAttackIgnoresResists )
		{
			// If we are defense buffed...
			if ( pVictim->m_Shared.InCond( TF_COND_DEFENSEBUFF_HIGH ) )
			{
				// We take 75% less damage... still take crits
				flDamage *= 0.25f;
			}
			else if ( pVictim->m_Shared.InCond( TF_COND_DEFENSEBUFF ) || pVictim->m_Shared.InCond( TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK ) )
			{
				// defense buffs gives 50% to sentry dmg and 35% from all other sources
				CObjectSentrygun *pSentry = ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() ) ? dynamic_cast< CObjectSentrygun* >( info.GetInflictor() ) : NULL;
				if ( pSentry )
				{
					flDamage *= 0.50f;
				}
				else
				{
					// And we take 35% less damage...
					flDamage *= 0.65f;
				}
			}
		}
	}

	// A note about why crits now go through the randomness/variance code:
	// Normally critical damage is not affected by variance.  However, we always want to measure what that variance 
	// would have been so that we can lump it into the DamageBonus value inside the info.  This means crits actually
	// boost more than 3X when you factor the reduction we avoided.  Example: a rocket that normally would do 50
	// damage due to range now does the original 100, which is then multiplied by 3, resulting in a 6x increase.
	bool bCrit = ( bitsDamage & DMG_CRITICAL ) ?  true : false;

	// If we're not damaging ourselves, apply randomness
	if ( pAttacker != pVictimBaseEntity && !(bitsDamage & (DMG_DROWN | DMG_FALL)) ) 
	{
		float flDmgVariance = 0.f;

		int iForceCritDmgFalloff = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iForceCritDmgFalloff, crit_dmg_falloff );

		// Minicrits still get short range damage bonus
		bool bForceCritFalloff = ( bitsDamage & DMG_USEDISTANCEMOD ) && 
								 ( ( bCrit && tf_weapon_criticals_distance_falloff.GetBool() ) || 
								 ( info.GetCritType() == CTakeDamageInfo::CRIT_MINI && tf_weapon_minicrits_distance_falloff.GetBool() ) || 
								 ( iForceCritDmgFalloff ) );
		bool bDoShortRangeDistanceIncrease = !bCrit || info.GetCritType() == CTakeDamageInfo::CRIT_MINI ;
		bool bDoLongRangeDistanceDecrease = !bIgnoreLongRangeDmgEffects && ( bForceCritFalloff || ( !bCrit && info.GetCritType() != CTakeDamageInfo::CRIT_MINI  ) );

		// If we're doing any distance modification, we need to do that first
		float flRandomDamage = info.GetDamage() * tf_damage_range.GetFloat();

		float flRandomDamageSpread = 0.10f;
		float flMin = 0.5 - flRandomDamageSpread;
		float flMax = 0.5 + flRandomDamageSpread;

		if ( bitsDamage & DMG_USEDISTANCEMOD )
		{
			Vector vAttackerPos = pAttacker->WorldSpaceCenter();
			float flOptimalDistance = 512.0;

			// Use Sentry position for distance mod
			CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( info.GetInflictor() );
			if ( pSentry )
			{
				vAttackerPos = pSentry->WorldSpaceCenter();
				// Sentries have a much further optimal distance
				flOptimalDistance = SENTRY_MAX_RANGE;
			}
			// The base sniper rifle doesn't have DMG_USEDISTANCEMOD, so this isn't used. Unlockable rifle had it for a bit.
			else if ( pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
			{
				flOptimalDistance *= 2.5f;
			}

			float flDistance = MAX( 1.0, ( pVictimBaseEntity->WorldSpaceCenter() - vAttackerPos).Length() );
				
			float flCenter = RemapValClamped( flDistance / flOptimalDistance, 0.0, 2.0, 1.0, 0.0 );
			if ( ( flCenter > 0.5 && bDoShortRangeDistanceIncrease ) || flCenter <= 0.5 )
			{
				if ( bitsDamage & DMG_NOCLOSEDISTANCEMOD )
				{
					if ( flCenter > 0.5 )
					{
						// Reduce the damage bonus at close range
						flCenter = RemapVal( flCenter, 0.5, 1.0, 0.5, 0.65 );
					}
				}
				flMin = MAX( 0.0, flCenter - flRandomDamageSpread );
				flMax = MIN( 1.0, flCenter + flRandomDamageSpread );

				if ( bDebug )
				{
					Warning("    RANDOM: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
				}
			}
			else
			{
				if ( bDebug )
				{
					Warning("    NO DISTANCE MOD: Dist %.2f, Ctr: %.2f, Min: %.2f, Max: %.2f\n", flDistance, flCenter, flMin, flMax );
				}
			}
		}
		//Msg("Range: %.2f - %.2f\n", flMin, flMax );
		float flRandomRangeVal;
		if ( tf_damage_disablespread.GetBool() || ( pTFAttacker && pTFAttacker->m_Shared.GetCarryingRuneType() == RUNE_PRECISION ) )
		{
			flRandomRangeVal = flMin + flRandomDamageSpread;
		}
		else
		{
			flRandomRangeVal = RandomFloat( flMin, flMax );
		}

		//if ( bDebug )
		//{
		//	Warning( "            Val: %.2f\n", flRandomRangeVal );
		//}

		// Weapon Based Damage Mod
		if ( pWeapon && pAttacker && pAttacker->IsPlayer() )
		{
			switch ( pWeapon->GetWeaponID() )
			{
			// Rocket launcher only has half the bonus of the other weapons at short range
			// Rocket Launchers
			case TF_WEAPON_ROCKETLAUNCHER :
			case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT :
			case TF_WEAPON_PARTICLE_CANNON :
				if ( flRandomRangeVal > 0.5 )
				{
					flRandomDamage *= 0.5f;
				}
				break;
			case TF_WEAPON_PIPEBOMBLAUNCHER :	// Stickies
			case TF_WEAPON_GRENADELAUNCHER :
			case TF_WEAPON_CANNON :
			case TF_WEAPON_STICKBOMB:
				if ( !( bitsDamage & DMG_NOCLOSEDISTANCEMOD ) )
				{
					flRandomDamage *= 0.2f;
				}
				break;
			case TF_WEAPON_SCATTERGUN :
			case TF_WEAPON_SODA_POPPER :
			case TF_WEAPON_PEP_BRAWLER_BLASTER :
			//case TF_WEAPON_HANDGUN_SCOUT_PRIMARY :		// Shortstop
				// Scattergun gets 50% bonus at short range
				if ( flRandomRangeVal > 0.5 )
				{
					flRandomDamage *= 1.5f;
				}
				break;
			}
		}

		// Random damage variance.
		flDmgVariance = SimpleSplineRemapValClamped( flRandomRangeVal, 0, 1, -flRandomDamage, flRandomDamage );
		if ( ( bDoShortRangeDistanceIncrease && flDmgVariance > 0.f ) || bDoLongRangeDistanceDecrease )
		{
			flDamage += flDmgVariance;
		}

		if ( bDebug )
		{
			Warning("            Out: %.2f -> Final %.2f\n", flDmgVariance, flDamage );
		}

		/*
		for ( float flVal = flMin; flVal <= flMax; flVal += 0.05 )
		{
			float flOut = SimpleSplineRemapValClamped( flVal, 0, 1, -flRandomDamage, flRandomDamage );
			Msg("Val: %.2f, Out: %.2f, Dmg: %.2f\n", flVal, flOut, info.GetDamage() + flOut );
		}
		*/

		// Burn sounds are handled in ConditionThink()
		if ( !(bitsDamage & DMG_BURN ) && pVictim )
		{
			pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT );
		}


		// Save any bonus damage as a separate value
		float flCritDamage = 0.f;
		// Yes, it's weird that we sometimes fabs flDmgVariance.  Here's why: In the case of a crit rocket, we
		// know that number will generally be negative due to dist or randomness.  In this case, we want to track
		// that effect - even if we don't apply it.  In the case of our crit rocket that normally would lose 50 
		// damage, we fabs'd so that we can account for it as a bonus - since it's present in a crit.
		float flBonusDamage = bForceCritFalloff ? 0.f : fabs( flDmgVariance );
		CTFPlayer *pProvider = NULL;

		auto lambdaDoMinicrit = [&]( bool bDemote )
		{
			// We should never have both of these flags set or Weird Things will happen with the damage numbers
			// that aren't clear to the players. Or us, really.
			Assert( !(bitsDamage & DMG_CRITICAL) );

			if ( bDebug )
			{
				Warning( "    MINICRIT: Dmg %.2f -> ", flDamage );
			}

			COMPILE_TIME_ASSERT( TF_DAMAGE_MINICRIT_MULTIPLIER > 1.f );
			flCritDamage = ( TF_DAMAGE_MINICRIT_MULTIPLIER - 1.f ) * flDamage;

			bitsDamage |= DMG_CRITICAL;
			info.AddDamageType( DMG_CRITICAL );
			info.SetCritType( CTakeDamageInfo::CRIT_MINI );
			if ( pVictim && bDemote )
			{
				pVictim->SetAttackBonusEffect( kBonusEffect_MiniCrit );
			}

			// Any condition assist stats to send out?
			if ( eDamageBonusCond < TF_COND_LAST )
			{
				if ( pVictim )
				{
					pProvider = ToTFPlayer( pVictim->m_Shared.GetConditionProvider( eDamageBonusCond ) );
					if ( pProvider )
					{
						CTF_GameStats.Event_PlayerDamageAssist( pProvider, flCritDamage + flBonusDamage );
					}
				}
				if ( pTFAttacker )
				{
					pProvider = ToTFPlayer( pTFAttacker->m_Shared.GetConditionProvider( eDamageBonusCond ) );
					if ( pProvider && pProvider != pTFAttacker )
					{
						CTF_GameStats.Event_PlayerDamageAssist( pProvider, flCritDamage + flBonusDamage );
					}
				}
			}

			if ( bDebug )
			{
				Warning( "reduced to %.2f before crit mult\n", flDamage );
			}
		};

		auto lambdaDoFullCrit = [&]()
		{
			if ( info.GetCritType() != CTakeDamageInfo::CRIT_MINI  )
			{
				COMPILE_TIME_ASSERT( TF_DAMAGE_CRIT_MULTIPLIER > 1.f );
				flCritDamage = ( TF_DAMAGE_CRIT_MULTIPLIER - 1.f ) * flDamage;
			}

			if ( bDebug )
			{
				Warning( "    CRITICAL! Damage: %.2f\n", flDamage );
			}

			// Burn sounds are handled in ConditionThink()
			if ( !(bitsDamage & DMG_BURN ) && pVictim )
			{
				pVictim->SpeakConceptIfAllowed( MP_CONCEPT_HURT, "damagecritical:1" );
			}

			if ( pTFAttacker && pTFAttacker->m_Shared.IsCritBoosted() )
			{
				pProvider = ToTFPlayer( pTFAttacker->m_Shared.GetConditionProvider( TF_COND_CRITBOOSTED ) );
				if ( pProvider && pTFAttacker && pProvider != pTFAttacker )
				{
					CTF_GameStats.Event_PlayerDamageAssist( pProvider, flCritDamage + flBonusDamage );	
				}
			}
		};

		if ( info.GetCritType() == CTakeDamageInfo::CRIT_MINI )
		{
			lambdaDoMinicrit( false );
		}
		else if ( bCrit )
		{
			int iDemoteCritToMinicrit = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iDemoteCritToMinicrit, crits_become_minicrits );
			if ( iDemoteCritToMinicrit != 0 )
			{
				bitsDamage &= ~DMG_CRITICAL; // this is to shutup the assert in lambdaDoMinicrit
				lambdaDoMinicrit( true );
			}
			else
			{
				lambdaDoFullCrit();
			}
		}
		
		if ( pAttacker && pAttacker->IsPlayer() )
		{
			// Modify damage based on bonuses
			flDamage *= pTFAttacker->m_Shared.GetTmpDamageBonus();
		}

		// Store the extra damage and update actual damage
		if ( bCrit || info.GetCritType() == CTakeDamageInfo::CRIT_MINI  )
		{
			info.SetDamageBonus( flCritDamage + flBonusDamage, pProvider );	// Order-of-operations sensitive, but fine as long as TF_COND_CRITBOOSTED is last
		}

		// Crit-A-Cola and Steak Sandwich - only increase normal damage
		if ( pVictim && pVictim->m_Shared.InCond( TF_COND_ENERGY_BUFF ) && !bCrit && info.GetCritType() != CTakeDamageInfo::CRIT_MINI  )
		{
			float flDmgMult = 1.f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flDmgMult, energy_buff_dmg_taken_multiplier );
			flDamage *= flDmgMult;
		}

		flDamage += flCritDamage;
	}

	if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( pTFAttacker->GetActiveWeapon() )
		{
			int iAddCloakOnHit = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker->GetActiveWeapon(), iAddCloakOnHit, add_cloak_on_hit );
			if ( iAddCloakOnHit > 0 )
			{
				pTFAttacker->m_Shared.AddToSpyCloakMeter( iAddCloakOnHit, true );
			}
		}
	}

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
	{
		// Jarate backstabber
		int iJarateBackstabber = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pVictim, iJarateBackstabber, jarate_backstabber );
		if ( iJarateBackstabber > 0 && pTFAttacker )
		{
			pTFAttacker->m_Shared.AddCond( TF_COND_URINE, 10.0f, pVictim );
			pTFAttacker->m_Shared.SetPeeAttacker( pVictim );
			pTFAttacker->SpeakConceptIfAllowed( MP_CONCEPT_JARATE_HIT );
		}

		if ( pVictim && pVictim->CheckBlockBackstab( pTFAttacker ) )
		{
			// The backstab was absorbed by a shield.
			flDamage = 0.f;

			// Shake nearby players' screens.
			UTIL_ScreenShake( pVictim->GetAbsOrigin(), 25.f, 150.0, 1.0, 50.f, SHAKE_START );

			// Play the notification sound.
			pVictim->EmitSound( "Player.Spy_Shield_Break" );

			// Unzoom the sniper.
			CTFWeaponBase *pWeapon = pVictim->GetActiveTFWeapon();
			if ( pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
			{
				CTFSniperRifle *pSniperRifle = static_cast<CTFSniperRifle*>( pWeapon );
				if ( pSniperRifle->IsZoomed() )
				{
					pSniperRifle->ZoomOut();
				}
			}

			// Vibrate the spy's knife.
			if ( pTFAttacker && pTFAttacker->GetActiveWeapon() )
			{
				CTFKnife *pKnife = (CTFKnife *)pTFAttacker->GetActiveWeapon();
				if ( pKnife )
				{
					pKnife->BackstabBlocked();
				}
			}

			// Tell the clients involved in the jarate
			CRecipientFilter involved_filter;
			involved_filter.AddRecipient( pVictim );
			involved_filter.AddRecipient( pTFAttacker );

			UserMessageBegin( involved_filter, "PlayerShieldBlocked" );
			WRITE_BYTE( pTFAttacker->entindex() );
			WRITE_BYTE( pVictim->entindex() );
			MessageEnd();
		}
	}

	info.SetDamage( flDamage );

	// Apply on-hit attributes (after damage has been updated)
	if ( pVictim && pAttacker && pAttacker->GetTeam() != pVictim->GetTeam() && pAttacker->IsPlayer() && pWeapon )
	{
		pWeapon->ApplyOnHitAttributes( pVictimBaseEntity, pTFAttacker, info );
	}

	// Give assist points to the provider of any stun on the victim - up to half the damage, based on the amount of stun
	if ( pVictim && pVictim->m_Shared.InCond( TF_COND_STUNNED ) )
	{
		CTFPlayer *pProvider = ToTFPlayer( pVictim->m_Shared.GetConditionProvider( TF_COND_STUNNED ) );
		if ( pProvider && pTFAttacker && pProvider != pTFAttacker )
		{
			float flStunAmount = pVictim->m_Shared.GetAmountStunned( TF_STUN_MOVEMENT );
			if ( flStunAmount < 1.f && pVictim->m_Shared.IsControlStunned() )
				flStunAmount = 1.f;

			int nAssistPoints = RemapValClamped( flStunAmount, 0.1f, 1.f, 1, ( info.GetDamage() / 2 ) );
			if ( nAssistPoints )
			{
				CTF_GameStats.Event_PlayerDamageAssist( pProvider, nAssistPoints );	
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool CheckForDamageTypeImmunity( int nDamageType, CTFPlayer* pVictim, float &flDamageBase, float &flCritBonusDamage )
{
	bool bImmune = false;
	if( nDamageType & (DMG_BURN|DMG_IGNITE) )
	{
		bImmune = pVictim->m_Shared.InCond( TF_COND_FIRE_IMMUNE );
	}
	else if( nDamageType & (DMG_BULLET|DMG_BUCKSHOT) )
	{
		bImmune = pVictim->m_Shared.InCond( TF_COND_BULLET_IMMUNE );
	}
	else if( nDamageType & DMG_BLAST )
	{
		bImmune = pVictim->m_Shared.InCond( TF_COND_BLAST_IMMUNE );
	}

	if( bImmune )
	{
		flDamageBase = flCritBonusDamage = 0.f;

		IGameEvent* event = gameeventmanager->CreateEvent( "damage_resisted" );
		if ( event )
		{
			event->SetInt( "entindex", pVictim->entindex() );
			gameeventmanager->FireEvent( event ); 
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool CheckMedicResist( ETFCond ePassiveCond, ETFCond eDeployedCond, CTFPlayer* pVictim, float flRawDamage, float& flDamageBase, bool bCrit, float& flCritBonusDamage )
{
	// Might be a tank or some other object that's getting shot
	if( !pVictim || !pVictim->IsPlayer() )
		return false;

	ETFCond eActiveCond;
	// Be in the condition!
	if( pVictim->m_Shared.InCond( eDeployedCond ) )
	{
		eActiveCond = eDeployedCond;
	}
	else if( pVictim->m_Shared.InCond( ePassiveCond ) )
	{
		eActiveCond = ePassiveCond;
	}
	else
	{
		return false;
	}

	// Get our condition provider
	CBaseEntity* pProvider = pVictim->m_Shared.GetConditionProvider( eActiveCond );
	CTFPlayer* pTFProvider = ToTFPlayer( pProvider );
	Assert( pTFProvider );
	
	float flDamageScale = 0.f;
	//float flCritBarDeplete = 0.f;
	bool bUberResist = false;

	if( pTFProvider )
	{
		switch( eActiveCond )
		{
		case TF_COND_MEDIGUN_UBER_BULLET_RESIST:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flDamageScale, medigun_bullet_resist_deployed );
			bUberResist = true;
//			if ( bCrit )
//			{
//				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flCritBarDeplete, medigun_crit_bullet_percent_bar_deplete );
//			}
			break;

		case TF_COND_MEDIGUN_SMALL_BULLET_RESIST:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flDamageScale, medigun_bullet_resist_passive );
			break;

		case TF_COND_MEDIGUN_UBER_BLAST_RESIST:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flDamageScale, medigun_blast_resist_deployed );
			bUberResist = true;
			//if( bCrit )
			//{
			//	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flCritBarDeplete, medigun_crit_blast_percent_bar_deplete );
			//}
			break;

		case TF_COND_MEDIGUN_SMALL_BLAST_RESIST:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flDamageScale, medigun_blast_resist_passive );
			break;

		case TF_COND_MEDIGUN_UBER_FIRE_RESIST:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flDamageScale, medigun_fire_resist_deployed );
			bUberResist = true;
//			if( bCrit )
//			{
//				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flCritBarDeplete, medigun_crit_fire_percent_bar_deplete );
//			}
			break;

		case TF_COND_MEDIGUN_SMALL_FIRE_RESIST:
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFProvider, flDamageScale, medigun_fire_resist_passive );
			break;
		}
	}

	if ( bUberResist && pVictim->m_Shared.InCond( TF_COND_HEALING_DEBUFF ) )
	{
		flDamageScale *= ( 1.f - PYRO_AFTERBURN_HEALING_REDUCTION );
	}

	flDamageScale = 1.f - flDamageScale;
	if( flDamageScale < 0.f ) 
		flDamageScale = 0.f;

	//// Dont let the medic heal themselves when they take damage
	//if( pTFProvider && pTFProvider != pVictim )
	//{
	//	// Heal the medic for 10% of the incoming damage
	//	int nHeal = flRawDamage * 0.10f;
	//	// Heal!
	//	int iHealed = pTFProvider->TakeHealth( nHeal, DMG_GENERIC );

	//	// Tell them about it!
	//	if ( iHealed )
	//	{
	//		CTF_GameStats.Event_PlayerHealedOther( pTFProvider, iHealed );
	//	}

	//	IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
	//	if ( event )
	//	{
	//		event->SetInt( "amount", nHeal );
	//		event->SetInt( "entindex", pTFProvider->entindex() );
	//		gameeventmanager->FireEvent( event ); 
	//	}
	//}

	IGameEvent* event = gameeventmanager->CreateEvent( "damage_resisted" );
	if ( event )
	{
		event->SetInt( "entindex", pVictim->entindex() );
		gameeventmanager->FireEvent( event ); 
	}

	if ( bCrit && pTFProvider && bUberResist )
	{
		flCritBonusDamage = ( pVictim->m_Shared.InCond( TF_COND_HEALING_DEBUFF ) ) ? flCritBonusDamage * PYRO_AFTERBURN_HEALING_REDUCTION : 0.f;

		//CWeaponMedigun* pMedigun = dynamic_cast<CWeaponMedigun*>( pTFProvider->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		//if( pMedigun )
		//{
		//	if( pMedigun->GetChargeLevel() > 0.f && pMedigun->IsReleasingCharge() )
		//	{
		//		flCritBonusDamage = 0;
		//	}
		//	pMedigun->SubtractCharge( flCritBarDeplete );
		//}
	}

	// Scale the damage!
	flDamageBase = flDamageBase * flDamageScale;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void PotentiallyDamageMitigatedEvent( const CTFPlayer* pMitigator, const CTFPlayer* pDamaged, const CEconEntity* pMitigationProvidingEconItem, float flBeforeDamage, float flAfterDamage )
{
	int nAmount = flBeforeDamage - flAfterDamage;
	// Nothing mitigated!
	if ( nAmount == 0 )
		return;

	IGameEvent *pEvent = gameeventmanager->CreateEvent( "damage_mitigated" );
	if ( pEvent )
	{
		item_definition_index_t nDefIndex = INVALID_ITEM_DEF_INDEX;
		if ( pMitigationProvidingEconItem && pMitigationProvidingEconItem->GetAttributeContainer() && pMitigationProvidingEconItem->GetAttributeContainer()->GetItem() )
		{
			nDefIndex = pMitigationProvidingEconItem->GetAttributeContainer()->GetItem()->GetItemDefinition()->GetDefinitionIndex();
		}

		pEvent->SetInt( "mitigator", pMitigator ? pMitigator->GetUserID() : -1 );
		pEvent->SetInt( "damaged", pDamaged ? pDamaged->GetUserID() : -1 );
		pEvent->SetInt( "amount", nAmount );
		pEvent->SetInt( "itemdefindex", nDefIndex );
		gameeventmanager->FireEvent( pEvent, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::ApplyOnDamageAliveModifyRules( const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, DamageModifyExtras_t& outParams )
{
	CTFPlayer *pVictim = ToTFPlayer( pVictimBaseEntity );
	CBaseEntity *pAttacker = info.GetAttacker();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );

	float flRealDamage = info.GetDamage();

	int iAttackIgnoresResists = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iAttackIgnoresResists, mod_pierce_resists_absorbs );

	if ( pVictimBaseEntity && pVictimBaseEntity->m_takedamage != DAMAGE_EVENTS_ONLY && pVictim )
	{
		int iDamageTypeBits = info.GetDamageType() & DMG_IGNITE;

		// Handle attributes that want to change our damage type, but only if we're taking damage from a non-DOT. This
		// stops fire DOT damage from constantly reigniting us. This will also prevent ignites from happening on the
		// damage *from-a-bleed-DOT*, but not from the bleed application attack.
		if ( !IsDOTDmg( info.GetDamageCustom() ) )
		{
			int iAddBurningDamageType = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iAddBurningDamageType, set_dmgtype_ignite );
			if ( iAddBurningDamageType )
			{
				iDamageTypeBits |= DMG_IGNITE;
			}
		}

		// Start burning if we took ignition damage
		outParams.bIgniting = ( ( iDamageTypeBits & DMG_IGNITE ) && ( !pVictim || pVictim->GetWaterLevel() < WL_Waist ) );

		if ( outParams.bIgniting && pVictim )
		{
			if ( pVictim->m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				int iDisguiseNoBurn = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pVictim, iDisguiseNoBurn, disguise_no_burn );
				if ( iDisguiseNoBurn == 1 )
				{
					// Do a hard out in the caller
					return -1;
				}
			}

			if ( pVictim->m_Shared.InCond( TF_COND_FIRE_IMMUNE ) )
			{
				// Do a hard out in the caller
				return -1;
			}
		}

		// When obscured by smoke, attacks have a chance to miss
		if ( pVictim && pVictim->m_Shared.InCond( TF_COND_OBSCURED_SMOKE ) )
		{
			if ( RandomInt( 1, 4 ) >= 2 )
			{
				flRealDamage = 0.f;

				pVictim->SpeakConceptIfAllowed( MP_CONCEPT_DODGE_SHOT );

				if ( pTFAttacker )
				{
					CEffectData	data;
					data.m_nHitBox = GetParticleSystemIndex( "miss_text" );
					data.m_vOrigin = pVictim->WorldSpaceCenter() + Vector( 0.f , 0.f, 32.f );
					data.m_vAngles = vec3_angle;
					data.m_nEntIndex = 0;

					CSingleUserRecipientFilter filter( pTFAttacker );
					te->DispatchEffect( filter, 0.f, data.m_vOrigin, "ParticleEffect", data );
				}

				// No damage
				return -1.f;
			}
		}

		// Proc invicibility upon being hit
		float flUberChance = 0.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flUberChance, uber_on_damage_taken );
		if( RandomFloat() < flUberChance )
		{
			pVictim->m_Shared.AddCond( TF_COND_INVULNERABLE_CARD_EFFECT, 3.f );
			// Make sure we don't take any damage
			flRealDamage = 0.f;
		}

		// Resists and Boosts
		float flDamageBonus = info.GetDamageBonus();
		float flDamageBase = flRealDamage - flDamageBonus;

		if ( sv_cheats && !sv_cheats->GetBool() )
		{
			Assert( flDamageBase >= 0.f );
		}

		int iPierceResists = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iPierceResists, mod_pierce_resists_absorbs );

		// This raw damage wont get scaled.  Used for determining how much health to give resist medics.
		float flRawDamage = flDamageBase;
		
		// Check if we're immune
		outParams.bPlayDamageReductionSound = CheckForDamageTypeImmunity( info.GetDamageType(), pVictim, flDamageBase, flDamageBonus );

		if ( !iPierceResists )
		{
			// Reduce only the crit portion of the damage with crit resist
			bool bCrit = ( info.GetDamageType() & DMG_CRITICAL ) > 0;
			if ( bCrit )
			{
				// Break the damage down and reassemble
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flDamageBonus, mult_dmgtaken_from_crit );
			}

			// Apply general dmg type reductions. Should we only ever apply one of these? (Flaregun is DMG_BULLET|DMG_IGNITE, for instance)
			if ( info.GetDamageType() & (DMG_BURN|DMG_IGNITE) )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flDamageBase, mult_dmgtaken_from_fire );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim->GetActiveWeapon(), flDamageBase, mult_dmgtaken_from_fire_active );

				// Check for medic resist
				outParams.bPlayDamageReductionSound = CheckMedicResist( TF_COND_MEDIGUN_SMALL_FIRE_RESIST, TF_COND_MEDIGUN_UBER_FIRE_RESIST, pVictim, flRawDamage, flDamageBase, bCrit, flDamageBonus );
			}

			if ( pTFAttacker && pVictim && pVictim->m_Shared.InCond( TF_COND_BURNING ) )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFAttacker->GetActiveWeapon(), flDamageBase, mult_dmg_vs_burning );
			}

			if ( (info.GetDamageType() & (DMG_BLAST) ) )
			{
				bool bReduceBlast = false;

				// If someone else shot us or we're in MvM
				if( pAttacker != pVictimBaseEntity || IsMannVsMachineMode() )
				{
					bReduceBlast = true;
				}

				if ( bReduceBlast )
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flDamageBase, mult_dmgtaken_from_explosions );

					// Check for medic resist
					outParams.bPlayDamageReductionSound = CheckMedicResist( TF_COND_MEDIGUN_SMALL_BLAST_RESIST, TF_COND_MEDIGUN_UBER_BLAST_RESIST, pVictim, flRawDamage, flDamageBase, bCrit, flDamageBonus );
				}
			}

			if ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT) )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flDamageBase, mult_dmgtaken_from_bullets );

				// Check for medic resist
				outParams.bPlayDamageReductionSound = CheckMedicResist( TF_COND_MEDIGUN_SMALL_BULLET_RESIST, TF_COND_MEDIGUN_UBER_BULLET_RESIST, pVictim, flRawDamage, flDamageBase, bCrit, flDamageBonus );
			}

			if ( info.GetDamageType() & DMG_MELEE )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flDamageBase, mult_dmgtaken_from_melee );
			}

			if ( pVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && pVictim->m_Shared.InCond( TF_COND_AIMING ) && ( ( pVictim->GetHealth() - flRealDamage ) / pVictim->GetMaxHealth() ) <= 0.5f )
			{
				float flOriginalDamage = flDamageBase;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flDamageBase, spunup_damage_resistance );
				if ( flOriginalDamage != flDamageBase )
				{
					pVictim->PlayDamageResistSound( flOriginalDamage, flDamageBase );
				}
			}
		}

		// If the damage changed at all play the resist sound
		if ( flDamageBase != flRawDamage )
		{
			outParams.bPlayDamageReductionSound = true;
		}

		// Stomp flRealDamage with resist adjusted values
		flRealDamage = flDamageBase + flDamageBonus;

		// Some Powerups apply a damage multiplier. Backstabs are immune to resist protection
		if ( ( pVictim && info.GetDamageCustom() != TF_DMG_CUSTOM_BACKSTAB ) )
		{
			// Plague bleed damage is immune from resist calculation
			if ( ( !pVictim->m_Shared.InCond( TF_COND_PLAGUE ) && info.GetDamageCustom() != TF_DMG_CUSTOM_BLEEDING ) )
			{
				if ( pVictim->m_Shared.GetCarryingRuneType() == RUNE_RESIST )
				{
					flRealDamage *= ( pVictim->m_bIsInMannpowerDominantCondition ? 0.65f : 0.5f );
					outParams.bPlayDamageReductionSound = true;
					IGameEvent* event = gameeventmanager->CreateEvent( "damage_resisted" );
					if ( event )
					{
						event->SetInt( "entindex", pVictim->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
				else if ( ( pVictim->m_Shared.GetCarryingRuneType() == RUNE_VAMPIRE ) && !pVictim->m_bIsInMannpowerDominantCondition )
				{
					flRealDamage *= 0.75f;
					outParams.bPlayDamageReductionSound = true;
				}
				//Plague powerup carrier is resistant to infected enemies
				else if ( pTFAttacker && ( pVictim->m_Shared.GetCarryingRuneType() == RUNE_PLAGUE ) && pTFAttacker->m_Shared.InCond( TF_COND_PLAGUE ) )
				{
					outParams.bPlayDamageReductionSound = true;
					if ( pVictim->m_bIsInMannpowerDominantCondition ) //dominant plague carrying players get less resistance to infected attackers
					{
						flRealDamage *= 0.80f;
					}
					else
					{
						flRealDamage *= 0.5f;
					}
				}
			}
		}

		// End Resists

		// Increased damage taken from all sources
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flRealDamage, mult_dmgtaken );
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim->GetActiveTFWeapon(), flRealDamage, mult_dmgtaken_active );

		if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			CObjectSentrygun* pSentry = dynamic_cast<CObjectSentrygun*>( info.GetInflictor() );
			if ( pSentry )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim, flRealDamage, dmg_from_sentry_reduced );
			}
		}

		if ( IsMannVsMachineMode() )
		{
			if ( pTFAttacker && pTFAttacker->IsBot() && pAttacker != pVictimBaseEntity && pVictim && !pVictim->IsBot() )
			{
				flRealDamage *= g_pPopulationManager ? g_pPopulationManager->GetDamageMultiplier() : tf_populator_damage_multiplier.GetFloat();
			}
		}

		// Heavy rage-based knockback+stun effect that also reduces their damage output
		if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			int iRage = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iRage, generate_rage_on_dmg );
			if ( iRage && pTFAttacker->m_Shared.IsRageDraining() )
			{
				flRealDamage *= 0.5f;
			}
		}

		if ( pVictim && pTFAttacker && info.GetWeapon() )
		{
			CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
			if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE && info.GetWeapon() == pWeapon )
			{
				CTFSniperRifle *pRifle = static_cast< CTFSniperRifle* >( info.GetWeapon() );

				float flStun = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pRifle, flStun, applies_snare_effect );
				if ( flStun != 1.0f )
				{
					float flDuration = pRifle->GetJarateTime();
					pVictim->m_Shared.StunPlayer( flDuration, flStun, TF_STUN_MOVEMENT, pTFAttacker );
				}
			}
		}

		if ( pVictim && pVictim->GetActiveTFWeapon() && !iAttackIgnoresResists )
		{
			if ( info.GetDamageType() & (DMG_CLUB) )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim->GetActiveTFWeapon(), flRealDamage, dmg_from_melee );
			}
			else if ( info.GetDamageType() & (DMG_BLAST|DMG_BULLET|DMG_BUCKSHOT|DMG_IGNITE|DMG_SONIC) )
			{
				float flBeforeDamage = flRealDamage;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pVictim->GetActiveTFWeapon(), flRealDamage, dmg_from_ranged );
				PotentiallyDamageMitigatedEvent( pVictim, pVictim, pVictim->GetActiveTFWeapon(), flBeforeDamage, flRealDamage );
			}
		}

		outParams.bSendPreFeignDamage = false;
		if ( pVictim && pVictim->IsPlayerClass( TF_CLASS_SPY ) && ( info.GetDamageCustom() != TF_DMG_CUSTOM_TELEFRAG ) && !pVictim->IsTaunting() && !iAttackIgnoresResists )
		{
			// Reduce damage taken if we have recently feigned death.
			if ( pVictim->m_Shared.InCond( TF_COND_FEIGN_DEATH ) || pVictim->m_Shared.IsFeignDeathReady() )
			{
				// Damage reduction is proportional to cloak remaining (60%->20%)
				float flDamageReduction = RemapValClamped( pVictim->m_Shared.GetSpyCloakMeter(), 50.0f, 0.0f, tf_feign_death_damage_scale.GetFloat(), tf_stealth_damage_reduction.GetFloat() );

				// On Activate Reduce Remaining Cloak by 50%
				if ( pVictim->m_Shared.IsFeignDeathReady() )
				{
					flDamageReduction = tf_feign_death_activate_damage_scale.GetFloat();
				}
				outParams.bSendPreFeignDamage = true;

				float flBeforeflRealDamage = flRealDamage;

				flRealDamage *= flDamageReduction;

				CTFWeaponInvis *pWatch = (CTFWeaponInvis *) pVictim->Weapon_OwnsThisID( TF_WEAPON_INVIS );
				PotentiallyDamageMitigatedEvent( pVictim, pVictim, pWatch, flBeforeflRealDamage, flRealDamage );

				// Original damage would've killed the player, but the reduced damage wont
				if ( flBeforeflRealDamage >= pVictim->GetHealth() && flRealDamage < pVictim->GetHealth() )
				{
					IGameEvent *pEvent = gameeventmanager->CreateEvent( "deadringer_cheat_death" );
					if ( pEvent )
					{
						pEvent->SetInt( "spy", pVictim->GetUserID() );
						pEvent->SetInt( "attacker", pTFAttacker ? pTFAttacker->GetUserID() : -1 );
						gameeventmanager->FireEvent( pEvent, true );
					}
				}
			}
			// Standard Stealth gives small damage reduction
			else if ( pVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
			{
				flRealDamage *= tf_stealth_damage_reduction.GetFloat();
			}
		}

		if ( sv_cheats && !sv_cheats->GetBool() )
		{
			if ( flRealDamage <= 0.0f )
			{
				// Do a hard out in the caller
				return -1;
			}
		}
		else
		{
			// allow negative health values for things like the hurtme command
			if ( flRealDamage == 0.0f )
			{
				// Do a hard out in the caller
				return -1;
			}
		}

		if ( ( pAttacker == pVictimBaseEntity ) &&
			 ( ( info.GetDamageType() & DMG_BLAST ) || ( info.GetDamageCustom() == TF_DMG_CUSTOM_FLARE_EXPLOSION ) ) &&
			 ( info.GetDamagedOtherPlayers() == 0 ) && 
			 ( info.GetDamageCustom() != TF_DMG_CUSTOM_TAUNTATK_GRENADE ) )
		{
			// If we attacked ourselves, hurt no other players, and it is a blast,
			// check the attribute that reduces rocket jump damage.
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetAttacker(), flRealDamage, rocket_jump_dmg_reduction );
			outParams.bSelfBlastDmg = true;
		}

		if ( pAttacker == pVictimBaseEntity )
		{
			enum
			{
				kSelfBlastResponse_IgnoreProjectilesFromThisWeapon = 1,		// the sticky jumper doesn't disable damage from other explosive weapons
				kSelfBlastResponse_IgnoreProjectilesFromAllWeapons = 2,		// the rocket jumper doesn't have a special projectile type and so ignores all self-inflicted damage from explosive sources
			};

			if ( info.GetWeapon() )
			{
				int iNoSelfBlastDamage = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iNoSelfBlastDamage, no_self_blast_dmg );

				const bool bIgnoreThisSelfDamage = ( iNoSelfBlastDamage == kSelfBlastResponse_IgnoreProjectilesFromAllWeapons )
					|| ( (iNoSelfBlastDamage == kSelfBlastResponse_IgnoreProjectilesFromThisWeapon) && (info.GetDamageCustom() == TF_DMG_CUSTOM_PRACTICE_STICKY) );
				if ( bIgnoreThisSelfDamage )
				{
					flRealDamage = 0;
				}

				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetWeapon(), flRealDamage, blast_dmg_to_self );
			}
		}

		// Precision Powerup removes self damage
		if ( pTFAttacker == pVictim && pTFAttacker->m_Shared.GetCarryingRuneType() == RUNE_PRECISION )
		{
			flRealDamage = 0.f;
		}

		if ( pTFAttacker && ( pTFAttacker != pVictim ) )
		{
			// Vampire Powerup collects health based on damage received on victim. Does not apply to self damage. Do it here to factor in victim resistance calculations
			if ( pTFAttacker->m_Shared.GetCarryingRuneType() == RUNE_VAMPIRE )
			{
				if ( flRealDamage > 0 )
				{
					if ( pTFAttacker->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MINIGUN || pTFAttacker->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
					{
						pTFAttacker->TakeHealth( ( flRealDamage * 0.6f ), DMG_GENERIC );
					}
					else if ( info.GetDamageType() & DMG_MELEE && pVictim->m_Shared.GetCarryingRuneType() != RUNE_RESIST ) //resist doesn't give the melee bonus
					{
						pTFAttacker->TakeHealth( ( flRealDamage * 1.25f ), DMG_GENERIC );
					}
					else if ( info.GetDamageType() & DMG_BLAST )
					{
						int iMaxHealthOverboost = 120;
						if ( ( pTFAttacker->GetHealth() - pTFAttacker->GetMaxHealth() ) < iMaxHealthOverboost )
						{
							int iMaxHealthToAdd = ( iMaxHealthOverboost + pTFAttacker->GetMaxHealth() ) - pTFAttacker->GetHealth();
							if ( flRealDamage < iMaxHealthToAdd )
							{
								pTFAttacker->TakeHealth( flRealDamage, DMG_IGNORE_MAXHEALTH );
							}
							else
								pTFAttacker->TakeHealth( iMaxHealthToAdd, DMG_IGNORE_MAXHEALTH );
						}
					}
					else
					{
						pTFAttacker->TakeHealth( flRealDamage, DMG_GENERIC );
					}
				}
			}

			int iHypeOnDamage = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iHypeOnDamage, hype_on_damage );
			if ( iHypeOnDamage )
			{
				float flHype = RemapValClamped( flRealDamage, 1.f, 200.f, 1.f, 50.f );
				pTFAttacker->m_Shared.SetScoutHypeMeter( Min( 100.f, flHype + pTFAttacker->m_Shared.GetScoutHypeMeter() ) );
			}
		}
	}

	return flRealDamage;
}

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		return TFGameRules()->TFVoiceManager( pListener, pTalker );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

// Load the objects.txt file.
class CObjectsFileLoad : public CAutoGameSystem
{
public:
	virtual bool Init()
	{
		LoadObjectInfos( filesystem );
		return true;
	}
} g_ObjectsFileLoad;

// --------------------------------------------------------------------------------------------------- //
// Globals.
// --------------------------------------------------------------------------------------------------- //
/*
// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TF_TEAM_RED, TF_TEAM_BLUE, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue"
};
*/
// --------------------------------------------------------------------------------------------------- //
// Global helper functions.
// --------------------------------------------------------------------------------------------------- //
	
// World.cpp calls this but we don't use it in TF.
void InitBodyQue()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::~CTFGameRules()
{
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	TFTeamMgr()->Shutdown();
	ShutdownCustomResponseRulesDicts();

	// clean up cached teleport locations
	m_mapTeleportLocations.PurgeAndDeleteElements();

	// reset this only if we quit MvM to minimize the risk of breaking pub tournament
	if ( IsMannVsMachineMode() )
	{
		mp_tournament.SetValue( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CheckTauntAchievement( CTFPlayer *pAchiever, int nGibs, int *pTauntCamAchievements )
{
	if ( !pAchiever || !pAchiever->GetPlayerClass() )
		return;

	int iClass = pAchiever->GetPlayerClass()->GetClassIndex();
	if ( pTauntCamAchievements[ iClass ] )
	{
		bool bAwardAchievement = true;

		// for the Heavy achievement, the player needs to also be invuln
		if ( iClass == TF_CLASS_HEAVYWEAPONS && pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_HEAVY_FREEZECAM_TAUNT )
		{
			if ( !pAchiever->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) && !pAchiever->m_Shared.InCond( TF_COND_INVULNERABLE ) )
			{
				bAwardAchievement = false;
			}
		}

		// for the Spy achievement, we must be in the cig lighter taunt
		if ( iClass == TF_CLASS_SPY && pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SPY_FREEZECAM_FLICK )
		{
			if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_PDA_SPY )
			{
				bAwardAchievement = false;
			}
		}

		// for the two Sniper achievements, we need to check for specific taunts
		if ( iClass == TF_CLASS_SNIPER )
		{
			if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SNIPER_FREEZECAM_HAT )
			{
				if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_CLUB )
				{
					bAwardAchievement = false;
				}
			}
			else if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SNIPER_FREEZECAM_WAVE  )
			{
				if ( pAchiever->GetActiveTFWeapon() && WeaponID_IsSniperRifle( pAchiever->GetActiveTFWeapon()->GetWeaponID() ) )
				{
					bAwardAchievement = false;
				}
			}
		}

		// For the Soldier achievements, we need to be doing a specific taunt, or have enough gibs onscreen
		if ( iClass == TF_CLASS_SOLDIER )
		{
			if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SOLDIER_FREEZECAM_TAUNT )
			{
				if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_SHOTGUN_SOLDIER )
				{
					bAwardAchievement = false;
				}
			}
			else if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_SOLDIER_FREEZECAM_GIBS )
			{
				// Need at least 3 gibs on-screen
				if ( nGibs < 3 )
				{
					bAwardAchievement = false;
				}
			}
		}

		// for the two Demoman achievements, we need to check for specific taunts
		if ( iClass == TF_CLASS_DEMOMAN )
		{
			if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_SMILE )
			{
				if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_GRENADELAUNCHER )
				{
					bAwardAchievement = false;
				}
			}
			else if ( pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_DEMOMAN_FREEZECAM_RUMP )
			{
				if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetAttributeContainer() )
				{
					// Needs to be the Scottish Defender
					CEconItemView *pItem = pAchiever->GetActiveTFWeapon()->GetAttributeContainer()->GetItem();
					if ( pItem && pItem->IsValid() && pItem->GetItemDefIndex() != 130 )	// Scottish Defender is item index 130
					{
						bAwardAchievement = false;
					}
				}
			}
		}

		// for the Engineer achievement, we must be in the guitar taunt
		if ( iClass == TF_CLASS_ENGINEER && pTauntCamAchievements[ iClass ] == ACHIEVEMENT_TF_ENGINEER_FREEZECAM_TAUNT )
		{
			if ( pAchiever->GetActiveTFWeapon() && pAchiever->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_SENTRY_REVENGE )
			{
				bAwardAchievement = false;
			}
		}

		if ( bAwardAchievement )
		{
			pAchiever->AwardAchievement( pTauntCamAchievements[ iClass ] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TFVoiceManager( CBasePlayer *pListener, CBasePlayer *pTalker )
{
	if ( pTalker && pTalker->BHaveChatSuspensionInCurrentMatch() )
		return false;

	// check coaching--we only want coaches and students to talk and listen to each other!
	CTFPlayer* pTFListener = (CTFPlayer*)pListener;
	CTFPlayer* pTFTalker = (CTFPlayer*)pTalker;
	if ( pTFListener->GetStudent() || pTFListener->GetCoach() || 
			pTFTalker->GetStudent() || pTFTalker->GetCoach() )
	{
		if ( pTFListener->GetStudent() == pTFTalker || pTFTalker->GetStudent() == pTFListener ||
				pTFListener->GetCoach() == pTalker || pTFTalker->GetCoach() == pTFListener )
		{
			return true;
		}
		return false;
	}

	// Always allow teams to hear each other in TD mode
	if ( IsMannVsMachineMode() )
		return true;

	if ( !tf_gravetalk.GetBool() )
	{
		// Dead players can only be heard by other dead team mates but only if a match is in progress
		if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_GAME_OVER ) 
		{
			if ( pTalker->IsAlive() == false )
			{
				if ( pListener->IsAlive() == false )
					return ( pListener->InSameTeam( pTalker ) );

				return false;
			}
		}
	}

	return ( pListener->InSameTeam( pTalker ) );

}

//-----------------------------------------------------------------------------
// Purpose: TF2 Specific Client Commands
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CTFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CTFPlayer *pPlayer = ToTFPlayer( pEdict );

	const char *pcmd = args[0];

	if ( IsInTournamentMode() == true && IsInPreMatch() == true )
	{
		if ( FStrEq( pcmd, "tournament_readystate" ) )
		{
			if ( IsMannVsMachineMode() )
				return true;

			if ( UsePlayerReadyStatusMode() )
				return true;

			if ( args.ArgC() < 2 )
				return true;

			if ( pPlayer->GetTeamNumber() <= LAST_SHARED_TEAM )
				return true;

			int iReadyState = atoi( args[1] );

			//Already this state
			if ( iReadyState == (int)IsTeamReady( pPlayer->GetTeamNumber() ) )
				return true;

			SetTeamReadyState( iReadyState == 1, pPlayer->GetTeamNumber() );

			IGameEvent * event = gameeventmanager->CreateEvent( "tournament_stateupdate" );

			if ( event )
			{
				event->SetInt( "userid", pPlayer->entindex() );
				event->SetInt( "readystate", iReadyState );
				event->SetBool( "namechange", 0 );
				event->SetString( "oldname", " " );
				event->SetString( "newname", " " );

				gameeventmanager->FireEvent( event );
			}

			if ( iReadyState == 0 )
			{
				m_flRestartRoundTime.Set( -1.f ); 
				m_bAwaitingReadyRestart = true;
			}

			return true;
		}

		if ( FStrEq( pcmd, "tournament_teamname" ) )
		{
			if ( IsMannVsMachineMode() )
				return true;

			if ( IsCompetitiveMode() )
				return true;

			if ( args.ArgC() < 2 )
				return true;

			if ( pPlayer->GetTeamNumber() <= LAST_SHARED_TEAM )
				return true;

			const char *commandline = args.GetCommandString();

			// find the rest of the command line past the bot index
			commandline = strstr( commandline, args[1] );
			Assert( commandline );

			char szName[MAX_TEAMNAME_STRING + 1] = { 0 };
			Q_strncpy( szName, commandline, sizeof( szName ));

			if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				if ( FStrEq( szName, mp_tournament_blueteamname.GetString() ) == true )
					return true;

				mp_tournament_blueteamname.SetValue( szName );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				if ( FStrEq( szName, mp_tournament_redteamname.GetString() ) == true )
					return true;

				mp_tournament_redteamname.SetValue( szName );
			}

			IGameEvent * event = gameeventmanager->CreateEvent( "tournament_stateupdate" );

			if ( event )
			{
				event->SetInt( "userid", pPlayer->entindex() );
				event->SetBool( "readystate", 0 );
				event->SetBool( "namechange", 1 );
				event->SetString( "newname", szName );

				gameeventmanager->FireEvent( event );
			}

			return true;
		}

		if ( FStrEq( pcmd, "tournament_player_readystate" ) )
		{
			if ( State_Get() != GR_STATE_BETWEEN_RNDS )
				return true;

			const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
			if ( pMatchDesc && pMatchDesc->BUsesAutoReady() )
				return true;

			// Make sure we have enough to allow ready mode commands
			if ( !PlayerReadyStatus_HaveMinPlayersToEnable() )
				return true;

			if ( args.ArgC() < 2 )
				return true;

			bool bReady = ( atoi( args[1] ) == 1 );
			PlayerReadyStatus_UpdatePlayerState( pPlayer, bReady );
			if ( bReady )
			{
				pPlayer->PlayReadySound();
			}

			return true;
		}
	}

	if ( FStrEq( pcmd, "objcmd" ) )
	{
		if ( args.ArgC() < 3 )
			return true;

		int entindex = atoi( args[1] );
		edict_t* pEdict = INDEXENT(entindex);
		if ( pEdict )
		{
			CBaseEntity* pBaseEntity = GetContainingEntity(pEdict);
			CBaseObject* pObject = dynamic_cast<CBaseObject*>(pBaseEntity);

			if ( pObject )
			{
				// We have to be relatively close to the object too...

				// BUG! Some commands need to get sent without the player being near the object, 
				// eg delayed dismantle commands. Come up with a better way to ensure players aren't
				// entering these commands in the console.

				//float flDistSq = pObject->GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );
				//if (flDistSq <= (MAX_OBJECT_SCREEN_INPUT_DISTANCE * MAX_OBJECT_SCREEN_INPUT_DISTANCE))
				{
					// Strip off the 1st two arguments and make a new argument string
					CCommand objectArgs( args.ArgC() - 2, &args.ArgV()[2]);
					pObject->ClientCommand( pPlayer, objectArgs );
				}
			}
		}

		return true;
	}

	// Handle some player commands here as they relate more directly to gamerules state
	if ( FStrEq( pcmd, "nextmap" ) )
	{
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			char szNextMap[MAX_MAP_NAME];

			if ( nextlevel.GetString() && *nextlevel.GetString() )
			{
				Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
			}
			else
			{
				GetNextLevelName( szNextMap, sizeof( szNextMap ) );
			}

			ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_nextmap", szNextMap);

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}

		return true;
	}
	else if ( FStrEq( pcmd, "timeleft" ) )
	{	
		if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
		{
			if ( mp_timelimit.GetInt() > 0 )
			{
				int iTimeLeft = GetTimeLeft();

				char szMinutes[5];
				char szSeconds[3];

				if ( iTimeLeft <= 0 )
				{
					Q_snprintf( szMinutes, sizeof(szMinutes), "0" );
					Q_snprintf( szSeconds, sizeof(szSeconds), "00" );
				}
				else
				{
					Q_snprintf( szMinutes, sizeof(szMinutes), "%d", iTimeLeft / 60 );
					Q_snprintf( szSeconds, sizeof(szSeconds), "%02d", iTimeLeft % 60 );
				}

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft", szMinutes, szSeconds );
			}
			else
			{
				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft_nolimit" );
			}

			pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
		}
		return true;
	}
	else if( pPlayer->ClientCommand( args ) )
	{
        return true;
	}

	return BaseClass::ClientCommand( pEdict, args );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::LevelShutdown()
{
	if ( IsInTraining() )
	{
		mp_humans_must_join_team.SetValue( "any" );
		training_can_build_sentry.Revert();
		training_can_build_dispenser.Revert();
		training_can_build_tele_entrance.Revert();
		training_can_build_tele_exit.Revert();
		training_can_destroy_buildings.Revert();
		training_can_pickup_sentry.Revert();
		training_can_pickup_dispenser.Revert();
		training_can_pickup_tele_entrance.Revert();
		training_can_pickup_tele_exit.Revert();
		training_can_select_weapon_primary.Revert();
		training_can_select_weapon_secondary.Revert();
		training_can_select_weapon_melee.Revert();
		training_can_select_weapon_building.Revert();
		training_can_select_weapon_pda.Revert();
		training_can_select_weapon_item1.Revert();
		training_can_select_weapon_item2.Revert();
		tf_training_client_message.Revert();
	}
	TheTFBots().LevelShutdown();
	hide_server.Revert();

	DuelMiniGame_LevelShutdown();

	g_TFGameModeHistory.SetPrevState( m_nGameType );

	if ( m_pUpgrades )
	{
		UTIL_Remove( m_pUpgrades );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Think()
{
	if ( m_bMapCycleNeedsUpdate )
	{
		m_bMapCycleNeedsUpdate = false;
		LoadMapCycleFile();
	}

	if ( g_fGameOver )
	{
		if ( IsCompetitiveMode() && !IsMannVsMachineMode() )
		{
			const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );

			static int nLastTimeSent = -1;
			int nTimeLeft = ( m_flStateTransitionTime - gpGlobals->curtime );
			int nTimePassed = gpGlobals->curtime - m_flLastRoundStateChangeTime;
			if ( pMatchDesc && pMatchDesc->GetMatchEndKickWarning() && nTimeLeft <= 50 && nTimeLeft % 10 == 0 && nTimeLeft != nLastTimeSent )
			{
				nLastTimeSent = nTimeLeft;
				CBroadcastRecipientFilter filter;
				UTIL_ClientPrintFilter( filter, HUD_PRINTTALK, pMatchDesc->GetMatchEndKickWarning(), CFmtStr( "%d", nTimeLeft ) );
			}

			if ( BAttemptMapVoteRollingMatch() )
			{
				const CMatchInfo* pMatch = GTFGCClientSystem()->GetMatch();
				if ( pMatch && pMatch->GetNumActiveMatchPlayers() == 0 )
				{
					Msg( "All players left during next map voting period.  Ending match.\n" );
					GTFGCClientSystem()->EndManagedMatch();
					Assert( IsManagedMatchEnded() );
					m_bMatchEnded.Set( true );
					return;
				}

				if ( m_eRematchState == NEXT_MAP_VOTE_STATE_WAITING_FOR_USERS_TO_VOTE )
				{
					bool bVotePeriodExpired = false;
					// Judgment time has arrived.  Force a result below
					if ( nTimePassed == tf_mm_next_map_vote_time.GetInt() )
					{
						bVotePeriodExpired = true;
					}

					int nVotes[ EUserNextMapVote::NUM_VOTE_STATES ];
					EUserNextMapVote eWinningVote = GetWinningVote( nVotes );

					if ( bVotePeriodExpired ||
						( nVotes[ USER_NEXT_MAP_VOTE_UNDECIDED ] == 0 && eWinningVote != USER_NEXT_MAP_VOTE_UNDECIDED ) )
					{
						CBroadcastRecipientFilter filter;

						const MapDef_t *pMap = NULL;
						if ( eWinningVote == USER_NEXT_MAP_VOTE_UNDECIDED )
						{
							// Nobody voted!  We're playing on the same map again by default
							pMap = GetItemSchema()->GetMasterMapDefByName( STRING( gpGlobals->mapname ) );
							Log( "Nobody voted for the next map.  Defaulting to current map.\n" );
						}
						else
						{
							pMap = GetItemSchema()->GetMasterMapDefByIndex( GetNextMapVoteOption( eWinningVote ) );
							if ( pMap )
							{
								Log( "Next map vote winner is candidate %d, '%s'\n", (int)eWinningVote, pMap->pszMapName );
							}
							else
							{
								Log( "Next map vote for candidate %d resulted in invalid map.\n", (int)eWinningVote );
							}
						}

						if ( pMap == NULL )
						{
							Assert( false );
							Log( "We didn't pick a new map to rotate to!  Default to the current one, '%s'\n",  STRING( gpGlobals->mapname ) );
							pMap = GetItemSchema()->GetMasterMapDefByName( STRING( gpGlobals->mapname ) );
						}

						if ( pMap )
						{
							m_eRematchState = NEXT_MAP_VOTE_STATE_MAP_CHOSEN_PAUSE;
							GTFGCClientSystem()->RequestNewMatchForLobby( pMap );
							Log( "Next map is '%s'.\n", pMap->pszMapName );
						}
					}
				}
				else if ( m_eRematchState == NEXT_MAP_VOTE_STATE_MAP_CHOSEN_PAUSE )
				{
					// CTFGCServerSystem is in control at this point
				}
			}

			if ( gpGlobals->curtime > m_flStateTransitionTime || !BHavePlayers() )
			{
				nLastTimeSent = -1;
				if ( pMatchDesc )
				{
					// Matchmaking path
					pMatchDesc->PostMatchClearServerSettings();
				}
				else
				{
					// Readymode (Tournament) path
					g_fGameOver = false;
					if ( !IsCommunityGameMode() )
						m_bAllowBetweenRounds = true;
					State_Transition( GR_STATE_RESTART );
					SetInWaitingForPlayers( true );
				}
				return;
			}
		}

		if ( ( m_flMatchSummaryTeleportTime > 0 ) && ( gpGlobals->curtime > m_flMatchSummaryTeleportTime ) )
		{
			m_flMatchSummaryTeleportTime = -1.f;
			MatchSummaryTeleport();
		}
	}
	else
	{
		if ( gpGlobals->curtime > m_flNextPeriodicThink )
		{
			if ( State_Get() != GR_STATE_BONUS && State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_GAME_OVER && IsInWaitingForPlayers() == false )
			{
				if ( CheckCapsPerRound() )
					return;
			}
		}

		// These network variables mirror the MM system's match state for client's sake. Gamerules should still
		// be aware of when these change, either because we caused it or via a callback.  This warning will
		// detect desync. (Ideally we'd have the ability to just share between the client GC system and server
		// GC system directly without passing things through gamerules)
		if ( m_bMatchEnded != IsManagedMatchEnded() )
		{
			Assert( false );
			Warning( "Mirrored Match parameters on gamerules don't match MatchInfo\n" );
			m_bMatchEnded.Set( IsManagedMatchEnded() );
		}

		if ( GTFGCClientSystem()->GetMatch() && GetCurrentMatchGroup() != (ETFMatchGroup)m_nMatchGroupType.Get() )
		{
			Assert( false );
			Warning( "Mirrored Match parameters on gamerules don't match MatchInfo\n" );
			m_nMatchGroupType.Set( GetCurrentMatchGroup() );
		}

		// Managed matches (MvM and competitive) abandon thing
		CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
		bool bEveryoneSafeToLeave = true;
		if ( pMatch )
		{
			// Send current safe-to-leave flags down from the GCServerSystem
			for ( int i = 1; i <= gpGlobals->maxClients; ++i )
			{
				CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( !pPlayer )
					{ continue; }

				CSteamID steamID;
				bool bSafe = !pMatch || !pPlayer->GetSteamID( &steamID ) || pMatch->BPlayerSafeToLeaveMatch( steamID );

				pPlayer->SetMatchSafeToLeave( bSafe );
				bEveryoneSafeToLeave = bEveryoneSafeToLeave && bSafe;
			}
		}

		if ( IsCompetitiveMode() )
		{
			const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
			Assert( pMatch ); // Should not be in competitive mode without a match

			//
			// Check if this is mode requires a complete match, but doesn't have one
			//
			bool bEndMatch = false;
			int nActiveMatchPlayers = pMatch->GetNumActiveMatchPlayers();
			int nMissingPlayers = pMatch->GetCanonicalMatchSize() - nActiveMatchPlayers;
			if ( pMatchDesc->BRequiresCompleteMatches() &&
			     !IsManagedMatchEnded() &&
			     nMissingPlayers )
			{
				// See if we are requesting late join right now, and give that time to work
				if ( pMatchDesc->ShouldRequestLateJoin() )
				{
					// End match if GC system didn't request late join in response to players leaving
					auto *pGCSys = GTFGCClientSystem();
					double flRequestedLateJoin = pGCSys->GetTimeRequestedLateJoin();

					if ( flRequestedLateJoin == -1.f )
					{
						bEndMatch = true;
						Msg( "Failed to request late join, ending competitive match\n" );
					}
					else
					{
						// Otherwise, since we can't proceed without players, apply a timeout after which we'll
						// cancel the match and release these players. The time to wait is shorter if the GC
						// hasn't confirmed our late join request, so we're not spending the full time waiting
						// when the GC is just non-responsive.
						double flTimeWaitingForLateJoin = CRTime::RTime32TimeCur() - flRequestedLateJoin;
						bool bGotLateJoin = pGCSys->BLateJoinEligible();
						double flWaitLimit = bGotLateJoin ? tf_competitive_required_late_join_timeout.GetFloat()
							                                : tf_competitive_required_late_join_confirm_timeout.GetFloat();
						if ( flTimeWaitingForLateJoin > flWaitLimit )
						{
							Msg( "Exceeded wait time limit for late joiners, canceling match\n" );
							bEndMatch = true;
						}
					}
				}
				else
				{
					// Can't request late joiners, tank match if number of active players get below some threshold
					int iRedActive = 0;
					int iBlueActive = 0;
					for ( int idxPlayer = 0; idxPlayer < pMatch->GetNumTotalMatchPlayers(); idxPlayer++ )
					{
						CMatchInfo::PlayerMatchData_t *pMatchPlayer = pMatch->GetMatchDataForPlayer( idxPlayer );
						if ( !pMatchPlayer->bDropped )
						{
							int iTeam = GetGameTeamForGCTeam( pMatchPlayer->eGCTeam );
							if ( iTeam == TF_TEAM_RED )
								iRedActive++;
							else
								iBlueActive++;
						}
					}

					int iTeamSize = pMatch->GetCanonicalMatchSize() / 2;
					if ( iRedActive == 0 || iBlueActive == 0 || ( iTeamSize - iRedActive ) > tf_mm_abandoned_players_per_team_max.GetInt() || ( iTeamSize - iBlueActive ) > tf_mm_abandoned_players_per_team_max.GetInt() )
					{
						Msg( "Match type requires a complete match, but there are not enough active players left and we are not requesting late join.  Stopping match.\n" );
						bEndMatch = true;
					}
				}
			}
			else if ( !IsManagedMatchEnded() && nActiveMatchPlayers < 1 )
			{
				// For non-complete mode, just stop the match if we lose all players
				Msg( "Competitive managed match in progress, but no remaining match players.  Stopping match.\n" );
				bEndMatch = true;
			}

			if ( bEndMatch )
			{
				StopCompetitiveMatch( CMsgGC_Match_Result_Status_MATCH_FAILED_ABANDON );
			}
			else
			{
				// If the match was ended but we're still playing, kick off a timer to remind people that
				// they're in a dead match.
				AssertMsg( !IsManagedMatchEnded() || ( pMatch->BMatchTerminated() && bEveryoneSafeToLeave ),
					        "Expect everyone to be safe to leave and the match info to reflect that after the match is over" );
				bool bGameRunning = ( State_Get() == GR_STATE_BETWEEN_RNDS || State_Get() == GR_STATE_RND_RUNNING );
				bool bDeadMatch = bGameRunning && IsManagedMatchEnded() && pMatch->BMatchTerminated() && bEveryoneSafeToLeave;
				if ( bDeadMatch && ( m_flSafeToLeaveTimer == -.1f ||
					                    m_flSafeToLeaveTimer - gpGlobals->curtime <= 0.f ) )
				{
					// Periodic nag event
					m_flSafeToLeaveTimer = gpGlobals->curtime + 30.f;
					IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_abandoned_match" );
					if ( pEvent )
					{
						pEvent->SetBool( "game_over", false );
						gameeventmanager->FireEvent( pEvent );
					}
				}
			}

			// Handle re-spawning the players after the doors have shut at the beginning of a match
			if ( ( m_flCompModeRespawnPlayersAtMatchStart > 0 ) && ( m_flCompModeRespawnPlayersAtMatchStart < gpGlobals->curtime ) )
			{
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CTFPlayer *pPlayer = static_cast<CTFPlayer*>( UTIL_PlayerByIndex( i ) );
					if ( !pPlayer )
						continue;

					pPlayer->RemoveAllOwnedEntitiesFromWorld();
					pPlayer->ForceRespawn();
				}

				m_flCompModeRespawnPlayersAtMatchStart = -1.f;
			}
		}
	}

	if ( g_bRandomMap == true )
	{
		g_bRandomMap = false;

		char szNextMap[MAX_MAP_NAME];
		GetNextLevelName( szNextMap, sizeof(szNextMap), true );
		IncrementMapCycleIndex();

		ChangeLevelToMap( szNextMap );

		return;
	}

	if ( IsInArenaMode() == true )
	{
		if ( m_flSendNotificationTime > 0.0f && m_flSendNotificationTime <= gpGlobals->curtime )
		{
			Arena_SendPlayerNotifications();
		}
	}

	// periodically count up the fake clients and set the bot_count cvar to update server tags
	if ( m_botCountTimer.IsElapsed() )
	{
		m_botCountTimer.Start( 5.0f );

		int botCount = 0;
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *player = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( player && player->IsFakeClient() )
			{
				++botCount;
			}
		}

		tf_bot_count.SetValue( botCount );
	}

#ifdef GAME_DLL
#ifdef _DEBUG
	if ( tf_debug_ammo_and_health.GetBool() )
	{
		CBaseEntity *ent;

		for( int i=0; i<m_healthVector.Count(); ++i )
		{
			ent = m_healthVector[i];
			if ( ent )
			{
				NDebugOverlay::Cross3D( ent->WorldSpaceCenter(), 10.0f, 0, 255, 0, true, 0.1f );
			}
		}

		for( int i=0; i<m_ammoVector.Count(); ++i )
		{
			ent = m_ammoVector[i];
			if ( ent )
			{
				NDebugOverlay::Cross3D( ent->WorldSpaceCenter(), 10.0f, 0, 0, 255, true, 0.1f );
			}
		}
	}
#endif // _DEBUG

	// Josh:
	// This is global because it handles maps and stuff.
	if ( g_voteControllerGlobal )
	{
		ManageServerSideVoteCreation();
	}

	// ...
	if ( tf_item_based_forced_holiday.GetInt() == kHoliday_Halloween && engine->Time() >= g_fEternaweenAutodisableTime )
	{
		if ( GCClientSystem() )
		{
			GCSDK::CProtoBufMsg<CMsgGC_GameServer_ServerModificationItemExpired> msg( k_EMsgGC_GameServer_ServerModificationItemExpired );
			msg.Body().set_modification_type( kGameServerModificationItem_Halloween );
			GCClientSystem()->BSendMessage( msg );
		}

		tf_item_based_forced_holiday.SetValue( kHoliday_None );
		FlushAllAttributeCaches();
	}	

	// play the bomb alarm if we need to
	if ( m_bMannVsMachineAlarmStatus )
	{
		if ( m_flNextFlagAlert < gpGlobals->curtime )
		{
			if ( PlayThrottledAlert( 255, "Announcer.MVM_Bomb_Alert_Near_Hatch", 5.0f ) )
			{
				m_flNextFlagAlarm = gpGlobals->curtime + 3.0;
				m_flNextFlagAlert = gpGlobals->curtime + 20.0f;
			}
		}

		if ( m_flNextFlagAlarm < gpGlobals->curtime )
		{
			m_flNextFlagAlarm = gpGlobals->curtime + 3.0;

			BroadcastSound( 255, "MVM.BombWarning" );
		}
	}
	else if ( m_flNextFlagAlarm > 0.0f )
	{
		m_flNextFlagAlarm = 0.0f;
		m_flNextFlagAlert = gpGlobals->curtime + 5.0f;
	}

	if ( m_bPowerupImbalanceMeasuresRunning )
	{
		if ( m_flTimeToStopImbalanceMeasures < gpGlobals->curtime )
		{
			PowerupTeamImbalance( TEAM_UNASSIGNED ); // passing TEAM_UNASSIGNED will fire the ImbalanceMeasuresOver output
			m_bPowerupImbalanceMeasuresRunning = false;
		}
	}
	if ( gpGlobals->curtime > m_flNextPowerupModeKillCountTimer )
	{
		PowerupModeKillCountCompare();
	}

	PeriodicHalloweenUpdate();
	SpawnHalloweenBoss();

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		if ( ( State_Get() == GR_STATE_RND_RUNNING ) && ( !m_bHelltowerPlayersInHell ) && ( m_helltowerTimer.IsElapsed() ) )
		{
			// Play our Halloween winning/losing lines for the teams
			int iWinningTeam = TEAM_UNASSIGNED;
			bool bRareLine = ( RandomFloat( 0, 1 ) < HELLTOWER_RARE_LINE_CHANCE );
			float flRedProgress = 0.0f, flBlueProgress = 0.0f;
			for ( int i = 0 ; i < ITFTeamTrainWatcher::AutoList().Count() ; ++i )
			{
				CTeamTrainWatcher *pTrainWatcher = static_cast< CTeamTrainWatcher* >( ITFTeamTrainWatcher::AutoList()[i] );
				if ( !pTrainWatcher->IsDisabled() )
				{
					if ( pTrainWatcher->GetTeamNumber() == TF_TEAM_RED )
					{
						flRedProgress = pTrainWatcher->GetTrainDistanceAlongTrack();
					}
					else
					{
						flBlueProgress = pTrainWatcher->GetTrainDistanceAlongTrack();
					}
				}
			}

			if ( flRedProgress > flBlueProgress )
			{
				iWinningTeam = TF_TEAM_RED;
			}
			else if ( flBlueProgress > flRedProgress )
			{
				iWinningTeam = TF_TEAM_BLUE;
			}

			int iRedLine = HELLTOWER_VO_RED_MISC;
			int iBlueLine = HELLTOWER_VO_BLUE_MISC;

			// should we play the misc lines or the winning/losing lines?
			if ( ( iWinningTeam == TEAM_UNASSIGNED ) || ( RandomFloat( 0, 1 ) < HELLTOWER_MISC_CHANCE ) )
			{
				if ( bRareLine )
				{
					iRedLine = HELLTOWER_VO_RED_MISC_RARE;
					iBlueLine = HELLTOWER_VO_BLUE_MISC_RARE;
				}
			}
			else
			{
				// play a winning/losing line
				iRedLine = ( iWinningTeam == TF_TEAM_RED ) ? HELLTOWER_VO_RED_WINNING : HELLTOWER_VO_RED_LOSING;
				iBlueLine = ( iWinningTeam == TF_TEAM_BLUE ) ? HELLTOWER_VO_BLUE_WINNING : HELLTOWER_VO_BLUE_LOSING;

				if ( bRareLine )
				{
					iRedLine = ( iWinningTeam == TF_TEAM_RED ) ? HELLTOWER_VO_RED_WINNING_RARE : HELLTOWER_VO_RED_LOSING_RARE;
					iBlueLine = ( iWinningTeam == TF_TEAM_BLUE ) ? HELLTOWER_VO_BLUE_WINNING_RARE : HELLTOWER_VO_BLUE_LOSING_RARE;

				}
			}

			PlayHelltowerAnnouncerVO( iRedLine, iBlueLine );
			m_helltowerTimer.Start( HELLTOWER_TIMER_INTERVAL );
		}
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		if ( ( State_Get() == GR_STATE_RND_RUNNING ) && m_doomsdaySetupTimer.HasStarted() && m_doomsdaySetupTimer.IsElapsed() )
		{
			m_doomsdaySetupTimer.Invalidate();

			const char *pszSound = NULL;
			switch( GetRoundsPlayed() )
			{
			case 0:
				pszSound = "sf14.Merasmus.Start.FirstRound";
				if ( RandomInt( 1, 10 ) == 1 )
				{
					pszSound = "sf14.Merasmus.Start.FirstRoundRare";
				}
				break;
			case 1:
				pszSound = "sf14.Merasmus.Start.SecondRound";
				break;
			case 2:
			default: 
				pszSound = "sf14.Merasmus.Start.ThirdRoundAndBeyond";
				break;
			}

			if ( pszSound && pszSound[0] )
			{
				BroadcastSound( 255, pszSound );
			}
		}
	}

#ifdef TF_RAID_MODE
	// This check is here for Boss battles that don't have a tf_raid_logic entity
	if ( IsBossBattleMode() && !IsInWaitingForPlayers() && State_Get() == GR_STATE_RND_RUNNING )
	{
		CUtlVector< CTFPlayer * > alivePlayerVector;
		CollectPlayers( &alivePlayerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );

		// if everyone is dead at the same time, they lose
		if ( alivePlayerVector.Count() == 0 )
		{
			SetWinningTeam( TF_TEAM_RED, WINREASON_OPPONENTS_DEAD );
		}
	}
#endif // TF_RAID_MODE

	// Batched strange event message processing?
	if ( engine->Time() > m_flNextStrangeEventProcessTime )
	{
		KillEaterEvents_FlushBatches();
		m_flNextStrangeEventProcessTime = engine->Time() + g_flStrangeEventBatchProcessInterval;
	}

	ManageCompetitiveMode();

#endif // GAME_DLL

//=============================================================================
// HPE_BEGIN:
// [msmith]	Do training summary screen logic.
//=============================================================================
	if ( IsInTraining() == true )
	{
		if ( m_flStateTransitionTime > gpGlobals->curtime )
		{
			int client_message = tf_training_client_message.GetInt();
			switch ( client_message )
			{
				case TRAINING_CLIENT_MESSAGE_IN_SUMMARY_SCREEN:
				{
					//Keep adding time to the restart while we are in the end screen menu so that we never restart the round until
					//the player presses the replay button (or next button to go to the next map).
					m_flStateTransitionTime = gpGlobals->curtime + 5.0f;
				}
				break;
				case TRAINING_CLIENT_MESSAGE_NEXT_MAP:
				{
					LoadNextTrainingMap();
					tf_training_client_message.SetValue( (int)TRAINING_CLIENT_MESSAGE_NONE );
				}
				break;
				case TRAINING_CLIENT_MESSAGE_REPLAY:
				{
					// Reload the map
					engine->ChangeLevel( STRING( gpGlobals->mapname ), NULL );
					tf_training_client_message.SetValue( (int)TRAINING_CLIENT_MESSAGE_NONE );
				}
				break;
			} // switch
		}
	}
//=============================================================================
// HPE_END
//=============================================================================

	// This is ugly, but, population manager needs to sometimes think when we're not simulating, but it is an entity.
	// Really, we need to better split out some kind of "sub-gamerules" class for modes like this.
	if ( IsMannVsMachineMode() && g_pPopulationManager )
	{
		g_pPopulationManager->GameRulesThink();
	}

	BaseClass::Think();
}

#ifdef GAME_DLL

void CTFGameRules::PeriodicHalloweenUpdate()
{
	// DEBUG

	// Are we on a Halloween Map?
	// Do we have Halloween Contracts?
	if ( !IsHolidayActive( kHoliday_Halloween ) )
		return;
	
	// Loop through each player that has a quest and spawn them a gift
	if ( m_halloweenGiftSpawnLocations.Count() == 0 )
		return;
	
	// If we've never given out gifts before, set the time
	if ( m_flNextHalloweenGiftUpdateTime < 0 )
	{
		m_flNextHalloweenGiftUpdateTime = gpGlobals->curtime + RandomInt( 7, 12 ) * 60;
		return;
	}

	if ( m_flNextHalloweenGiftUpdateTime > gpGlobals->curtime )
		return;

	m_flNextHalloweenGiftUpdateTime = gpGlobals->curtime + RandomInt( 7, 12 ) * 60;

	CUtlVector< CTFPlayer* > playerVector;
	CollectPlayers( &playerVector );
	FOR_EACH_VEC( playerVector, i )
	{
		Vector vLocation = m_halloweenGiftSpawnLocations.Element( RandomInt( 0, m_halloweenGiftSpawnLocations.Count() - 1 ) );
		CHalloweenGiftPickup *pGift = assert_cast<CHalloweenGiftPickup*>( CBaseEntity::CreateNoSpawn( "tf_halloween_gift_pickup", vLocation, vec3_angle, NULL ) );
		if ( pGift )
		{
			pGift->SetTargetPlayer( playerVector[i] );
			DispatchSpawn( pGift );
		}
	}
	
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	if ( pPlayer )
	{
		CBaseCombatWeapon *lastWeapon = ToTFPlayer( pPlayer )->GetLastWeapon();	

		if ( lastWeapon != NULL && lastWeapon->HasAnyAmmo() )
		{
			return pPlayer->Weapon_Switch( lastWeapon );
		}
	}

	return BaseClass::SwitchToNextBestWeapon( pPlayer, pCurrentWeapon );
}


#ifdef GAME_DLL

extern bool IsSpaceToSpawnHere( const Vector &where );

static bool isZombieMobForceSpawning = false;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SpawnZombieMob( void )
{
	if ( !tf_halloween_zombie_mob_enabled.GetBool() )
	{
		return;
	}

	// timer was started and has elapsed - time to spawn the boss
	if ( InSetup() || IsInWaitingForPlayers() )
	{
		m_zombieMobTimer.Start( tf_halloween_zombie_mob_spawn_interval.GetFloat() );
		return;
	}

	if ( isZombieMobForceSpawning )
	{
		isZombieMobForceSpawning = false;
		m_zombieMobTimer.Invalidate();
	}

	// spawn pending mob members
	if ( m_zombiesLeftToSpawn > 0 )
	{
		if ( IsSpaceToSpawnHere( m_zombieSpawnSpot ) )
		{
			if ( CZombie::SpawnAtPos( m_zombieSpawnSpot ) )
			{
				--m_zombiesLeftToSpawn;
			}
		}
	}

	// require a minimum number of human players in the game before the boss appears
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	// only count humans
	int totalPlayers = 0;
	for( int i=0; i<playerVector.Count(); ++i )
	{
		if ( !playerVector[i]->IsBot() )
		{
			++totalPlayers;
		}
	}

	if ( totalPlayers == 0 )
	{
		return;
	}

	// spawn a mob
	if ( m_zombieMobTimer.IsElapsed() )
	{
		m_zombieMobTimer.Start( tf_halloween_zombie_mob_spawn_interval.GetFloat() );

		CUtlVector< CTFNavArea * > ambushVector;	// vector of hidden but near-to-victim areas

		for( int i=0; i<playerVector.Count(); ++i )
		{
			CTFPlayer *player = playerVector[i];

			if ( player->IsBot() )
			{
				continue;
			}

			if ( !player->GetLastKnownArea() )
			{
				continue;
			}

			const float maxSurroundTravelRange = 2000.0f;

			CUtlVector< CNavArea * > areaVector;

			// collect walkable areas surrounding this player
			CollectSurroundingAreas( &areaVector, player->GetLastKnownArea(), maxSurroundTravelRange, StepHeight, StepHeight );

			// keep subset that isn't visible to any player
			for( int j=0; j<areaVector.Count(); ++j )
			{
				CTFNavArea *area = (CTFNavArea *)areaVector[j];

				if ( !area->IsValidForWanderingPopulation() )
				{
					continue;
				}

				if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) || area->IsPotentiallyVisibleToTeam( TF_TEAM_RED ) )
				{
					continue;
				}

				ambushVector.AddToTail( area );
			}
		}

		if ( ambushVector.Count() == 0 )
		{
			// no place to spawn the mob this time
			return;
		}

		for( int retry=0; retry<10; ++retry )
		{
			int which = RandomInt( 0, ambushVector.Count()-1 );
			m_zombieSpawnSpot = ambushVector[ which ]->GetCenter() + Vector( 0, 0, StepHeight );

			if ( !IsSpaceToSpawnHere( m_zombieSpawnSpot ) )
			{
				continue;
			}

			// spawn a mob here
			m_zombiesLeftToSpawn = tf_halloween_zombie_mob_spawn_count.GetInt();
				
			break;
		}
	}
}

//---------------------------------------------------------------------------------------------------------
static bool isBossForceSpawning = false;

// force the boss to spawn where our cursor is pointing
CON_COMMAND_F( tf_halloween_force_boss_spawn, "For testing.", FCVAR_CHEAT )
{
	isBossForceSpawning = true;
}


CON_COMMAND_F( cc_spawn_merasmus_at_level, "Force Merasmus to spawn at a specific difficulty level", FCVAR_CHEAT )
{
	if( args.ArgC() != 2 )
	{
		DevMsg( "Must specify a level\n" );
		return;
	}

	CMerasmus::DBG_SetLevel( atoi(args[1]) );
	
	tf_halloween_force_boss_spawn( args );
}


extern ConVar tf_halloween_bot_min_player_count;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SpawnHalloweenBoss( void )
{
	if ( !IsHolidayActive( kHoliday_Halloween ) )
		return;

	// only spawn the Halloween Boss on our Halloween maps
	HalloweenBossType bossType = HALLOWEEN_BOSS_INVALID;

	float bossInterval = 0.0f;
	float bossIntervalVariation = 0.0f;

	HalloweenScenarioType scenario = GetHalloweenScenario();
	if ( scenario == HALLOWEEN_SCENARIO_MANN_MANOR )
	{
		bossType = HALLOWEEN_BOSS_HHH;
		bossInterval = tf_halloween_boss_spawn_interval.GetFloat();
		bossIntervalVariation = tf_halloween_boss_spawn_interval_variation.GetFloat();

	}
	else if ( scenario == HALLOWEEN_SCENARIO_VIADUCT )
	{
		bossType = HALLOWEEN_BOSS_MONOCULUS;
		bossInterval = tf_halloween_eyeball_boss_spawn_interval.GetFloat();
		bossIntervalVariation = tf_halloween_eyeball_boss_spawn_interval_variation.GetFloat();
	}
	else if ( scenario == HALLOWEEN_SCENARIO_LAKESIDE )
	{
		bossType = HALLOWEEN_BOSS_MERASMUS;

		if ( CMerasmus::GetMerasmusLevel() <= 3 )
		{
			bossInterval = tf_merasmus_spawn_interval.GetFloat();
			bossIntervalVariation = tf_merasmus_spawn_interval_variation.GetFloat();
		}
		else
		{
			// after level 3, spawn Merasmus every 60 secs
			bossInterval = 60;
			bossIntervalVariation = 0;
		}

		// check if the wheel is still spinning
		CWheelOfDoom* pWheel = assert_cast< CWheelOfDoom* >( gEntList.FindEntityByClassname( NULL, "wheel_of_doom" ) );
		if ( pWheel && !pWheel->IsDoneBoardcastingEffectSound() )
		{
			return;
		}
	}
	//else if ( scenario == HALLOWEEN_SCENARIO_HIGHTOWER )
	//{
	//	bool bWasEnabled = tf_halloween_zombie_mob_enabled.GetBool();
	//	tf_halloween_zombie_mob_enabled.SetValue( true );

	//	// not a boss battle map
	//	SpawnZombieMob();

	//	tf_halloween_zombie_mob_enabled.SetValue( bWasEnabled );

	//	bossType = "eyeball_boss";
	//	bossInterval = tf_halloween_eyeball_boss_spawn_interval.GetFloat();
	//	bossIntervalVariation = tf_halloween_eyeball_boss_spawn_interval_variation.GetFloat();
	//}
	else
	{
		// not a boss battle map
		SpawnZombieMob();

		return;
	}

	// only one boss at a time
	if ( GetActiveBoss() )
	{
		// boss is still out there - restart the timer
		StartHalloweenBossTimer( bossInterval, bossIntervalVariation );
		isBossForceSpawning = false;
		return;
	}

	if ( !m_halloweenBossTimer.IsElapsed() && !isBossForceSpawning )
		return;

	// boss timer has elapsed
	if ( m_halloweenBossTimer.HasStarted() || isBossForceSpawning )
	{
		if ( !isBossForceSpawning )
		{
			// timer was started and has elapsed - time to spawn the boss
			if ( InSetup() || IsInWaitingForPlayers() )
				return;

			// require a minimum number of human players in the game before the boss appears
			CUtlVector< CTFPlayer * > playerVector;
			CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
			CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

			// only count humans
			int totalPlayers = 0;
			for( int i=0; i<playerVector.Count(); ++i )
			{
				if ( !playerVector[i]->IsBot() )
				{
					++totalPlayers;
				}
			}

			if ( totalPlayers < tf_halloween_bot_min_player_count.GetInt() )
				return;
		}

		Vector bossSpawnPos = vec3_origin;

		// spawn on the currently contested point
		CTeamControlPoint *contestedPoint = NULL;
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster )
		{
			for( int i=0; i<pMaster->GetNumPoints(); ++i )
			{
				contestedPoint = pMaster->GetControlPoint( i );
				if ( contestedPoint && pMaster->IsInRound( contestedPoint ) )
				{
					if ( ObjectiveResource()->GetOwningTeam( contestedPoint->GetPointIndex() ) == TF_TEAM_BLUE )
						continue;

					// blue are the invaders
					if ( !TeamplayGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, contestedPoint->GetPointIndex() ) )
						continue;

					break;
				}
			}
		}

		CBaseEntity *pCustomSpawnBossPos = gEntList.FindEntityByClassname( NULL, "spawn_boss" );
		if ( pCustomSpawnBossPos )
		{
			bossSpawnPos = pCustomSpawnBossPos->GetAbsOrigin();
		}
		else if ( contestedPoint )
		{
			bossSpawnPos = contestedPoint->GetAbsOrigin();

			if ( scenario == HALLOWEEN_SCENARIO_VIADUCT || scenario == HALLOWEEN_SCENARIO_LAKESIDE )
			{
				// revert ownership of point to neutral
				contestedPoint->ForceOwner( 0 );

				// pause the timers
				if ( IsInKothMode() )
				{
					variant_t sVariant;
					CTeamRoundTimer *pTimer = GetKothTeamTimer( TF_TEAM_BLUE );
					if ( pTimer )
					{
						pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
					}

					pTimer = GetKothTeamTimer( TF_TEAM_RED );
					if ( pTimer )
					{
						pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
					}
				}
			}
		}
		else
		{
			// pick a random spot
			CUtlVector< CTFNavArea * > spawnAreaVector;
			for( int i=0; i<TheNavAreas.Count(); ++i )
			{
				CTFNavArea *area = (CTFNavArea *)TheNavAreas[i];

				if ( area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED | TF_NAV_SPAWN_ROOM_EXIT ) )
				{
					// don't spawn in team spawn rooms
					continue;
				}

				// don't use small nav areas
				const float goodSize = 100.0f;
				if ( area->GetSizeX() < goodSize || area->GetSizeY() < goodSize )
				{
					continue;
				}

				spawnAreaVector.AddToTail( area );
			}

			if ( spawnAreaVector.Count() == 0 )
			{
				// no place to spawn (!)
				return;
			}

			int which = RandomInt( 0, spawnAreaVector.Count()-1 );
			bossSpawnPos = spawnAreaVector[ which ]->GetCenter();
		}

		CHalloweenBaseBoss::SpawnBossAtPos( bossType, bossSpawnPos );

		// pick next spawn time
		StartHalloweenBossTimer( bossInterval, bossIntervalVariation );
	
		isBossForceSpawning = false;
	}
	else
	{
		// Merasmus has a more reliable initial spawn time
		if( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
		{
			StartHalloweenBossTimer( bossInterval, bossIntervalVariation );
		}
		else
		{
			// initial spawn time
			m_halloweenBossTimer.Start( 0.5f * RandomFloat( 0.0f, bossInterval + bossIntervalVariation ) );	
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BeginHaunting( int nDesiredCount, float flMinDuration, float flMaxDuration )
{
	if ( !IsHolidayActive( kHoliday_Halloween ) )
		return;

	if ( !IsHalloweenScenario( HALLOWEEN_SCENARIO_VIADUCT ) && !IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
	{
		CTFHolidayEntity *pHolidayEntity = dynamic_cast<CTFHolidayEntity*> ( gEntList.FindEntityByClassname( NULL, "tf_logic_holiday" ) );
		if ( !pHolidayEntity || !pHolidayEntity->ShouldAllowHaunting() )
			return;
	}

	const int desiredGhostCount = nDesiredCount;

	// if there are existing ghosts, extend their time
	CUtlVector< CGhost * > priorGhostVector;
	for( int g=0; g<m_ghostVector.Count(); ++g )
	{
		if ( m_ghostVector[g] != NULL )
		{
			priorGhostVector.AddToTail( m_ghostVector[g] );
		}
	}

	m_ghostVector.RemoveAll();		

	for( int g=0; g<priorGhostVector.Count(); ++g )
	{
		priorGhostVector[g]->SetLifetime( RandomFloat( flMinDuration, flMaxDuration ) );

		m_ghostVector.AddToTail( priorGhostVector[g] );
	}

	if ( m_ghostVector.Count() >= desiredGhostCount )
		return;

	// spawn ghosts away from players
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	const int ghostCount = desiredGhostCount - m_ghostVector.Count();

	CUtlVector< Vector > spawnVector;

	for( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)TheNavAreas.Element(i);

		if ( area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
		{
			// keep out of spawn rooms
			continue;
		}

		Vector spot = area->GetCenter();

		// don't spawn near players (so they aren't instantly scared)
		int p;
		for( p=0; p<playerVector.Count(); ++p )
		{
			if ( ( playerVector[p]->GetAbsOrigin() - spot ).IsLengthLessThan( 1.25f * GHOST_SCARE_RADIUS ) )
			{
				break;
			}
		}

		if ( p == playerVector.Count() )
		{
			spawnVector.AddToTail( spot );
		}
	}

	if ( spawnVector.Count() == 0 )
	{
		return;
	}

	for( int g=0; g<ghostCount; ++g )
	{
		int which = RandomInt( 0, spawnVector.Count()-1 );

		CGhost *ghost = SpawnGhost( spawnVector[ which ], vec3_angle, RandomFloat( flMinDuration, flMaxDuration ) );

		m_ghostVector.AddToTail( ghost );
	}
}

static const int k_RecentPlayerInfoMaxTime = 7200;	// 2 hours

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerHistory_AddPlayer( CTFPlayer *pTFPlayer )
{
	if ( !pTFPlayer )
		return;

	CSteamID steamID;
	pTFPlayer->GetSteamID( &steamID );
	if ( !steamID.IsValid() || !steamID.BIndividualAccount() )
		return;

	// Exists?
	FOR_EACH_VEC_BACK( m_vecPlayerHistory, i )
	{
		if ( m_vecPlayerHistory[i].steamID == steamID )
		{
			m_vecPlayerHistory[i].flTime = Plat_FloatTime();
			return;
		}

		// Do maintenance here.
		if ( Plat_FloatTime() - m_vecPlayerHistory[i].flTime >= (float)k_RecentPlayerInfoMaxTime )
		{
			m_vecPlayerHistory.Remove( i );
		}
	}

	PlayerHistoryInfo_t info = 
	{ 
		steamID, 
		(float) Plat_FloatTime(), 
		pTFPlayer->GetTeamNumber()
	};
	m_vecPlayerHistory.AddToTail( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PlayerHistoryInfo_t *CTFGameRules::PlayerHistory_GetPlayerInfo( CTFPlayer *pTFPlayer )
{
	if ( !pTFPlayer )
		return NULL;

	CSteamID steamID;
	pTFPlayer->GetSteamID( &steamID );
	if ( !steamID.IsValid() || !steamID.BIndividualAccount() )
		return NULL;

	FOR_EACH_VEC_BACK( m_vecPlayerHistory, i )
	{
		if ( m_vecPlayerHistory[i].steamID == steamID )
			return &m_vecPlayerHistory[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns -1 if we don't have the data
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerHistory_GetTimeSinceLastSeen( CTFPlayer *pTFPlayer )
{
	PlayerHistoryInfo_t *pInfo = PlayerHistory_GetPlayerInfo( pTFPlayer );
	if ( !pInfo )
		return -1;

	// We only care about whole seconds
	return (int)( Plat_FloatTime() - pInfo->flTime );
}
#endif

//=============================================================================
// HPE_BEGIN:
// [msmith]	TEMP CODE: needs to be replaced with the final solution for our training mission navigation.
//=============================================================================
void CTFGameRules::LoadNextTrainingMap()
{
	if ( m_hTrainingModeLogic )
	{
		g_fGameOver = true;
		const char* pNextMap = m_hTrainingModeLogic->GetNextMap();
		if ( pNextMap && FStrEq( pNextMap, "" ) == false )
		{
			Msg( "CHANGE LEVEL: %s\n", pNextMap );
			engine->ChangeLevel( pNextMap, NULL );
		}
		else
		{
			Msg( "CHANGE LEVEL: %s\n", STRING( gpGlobals->mapname ) );
			engine->ChangeLevel( STRING( gpGlobals->mapname ), NULL );
		}
		return;
	}
		
	Msg( "CHANGE LEVEL: %s\n", STRING( gpGlobals->mapname ) );
	engine->ChangeLevel( STRING( gpGlobals->mapname ), NULL );
}
//=============================================================================
// HPE_END
//=============================================================================

//Runs think for all player's conditions
//Need to do this here instead of the player so players that crash still run their important thinks
void CTFGameRules::RunPlayerConditionThink ( void )
{
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->m_Shared.ConditionGameRulesThink();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FrameUpdatePostEntityThink()
{
	BaseClass::FrameUpdatePostEntityThink();

	RunPlayerConditionThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CheckCapsPerRound()
{
	return IsPasstimeMode() 
		? SetPasstimeWinningTeam()
		: SetCtfWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::SetPasstimeWinningTeam()
{
	Assert( IsPasstimeMode() );

	int iScoreLimit = tf_passtime_scores_per_round.GetInt();
	if ( iScoreLimit <= 0 )
	{
		// no score limit set, play forever
		return false;
	}

	// TODO need to generalize the "flag captures" parts of CTFTeam to avoid 
	// this confusing overload of the flag captures concept, or maybe defer 
	// this logic to the specific object that manages the mode.
	CTFTeamManager *pTeamMgr = TFTeamMgr();
	int iBlueScore = pTeamMgr->GetFlagCaptures( TF_TEAM_BLUE );
	int iRedScore = pTeamMgr->GetFlagCaptures( TF_TEAM_RED );
	if ( ( iBlueScore < iScoreLimit ) && ( iRedScore < iScoreLimit ) )
	{
		// no team has exceeded the score limit
		return false;
	}

	int iWinnerTeam = ( iBlueScore > iRedScore )
		? TF_TEAM_BLUE
		: TF_TEAM_RED;
	SetWinningTeam( iWinnerTeam, WINREASON_SCORED );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::SetCtfWinningTeam()
{
	Assert( !IsPasstimeMode() );
	if ( tf_flag_caps_per_round.GetInt() > 0 )
	{
		int iMaxCaps = -1;
		CTFTeam *pMaxTeam = NULL;

		// check to see if any team has won a "round"
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			// we might have more than one team over the caps limit (if the server op lowered the limit)
			// so loop through to see who has the most among teams over the limit
			if ( pTeam->GetFlagCaptures() >= tf_flag_caps_per_round.GetInt() )
			{
				if ( pTeam->GetFlagCaptures() > iMaxCaps )
				{
					iMaxCaps = pTeam->GetFlagCaptures();
					pMaxTeam = pTeam;
				}
			}
		}

		if ( iMaxCaps != -1 && pMaxTeam != NULL )
		{
			SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CheckWinLimit( bool bAllowEnd /*= true*/, int nAddValueWhenChecking /*= 0*/ )
{
	if ( IsInPreMatch() )
		return false;

	bool bWinner = false;
	int iTeam = TEAM_UNASSIGNED;
	int iReason = WINREASON_NONE;
	const char *pszReason = "";

	if ( mp_winlimit.GetInt() != 0 )
	{
		if ( ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() + nAddValueWhenChecking ) >= mp_winlimit.GetInt() )
		{
			pszReason = "Team \"BLUE\" triggered \"Intermission_Win_Limit\"\n";
			bWinner = true;
			iTeam = TF_TEAM_BLUE;
			iReason = WINREASON_WINLIMIT;
		}
		else if ( ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() + nAddValueWhenChecking ) >= mp_winlimit.GetInt() )
		{
			pszReason = "Team \"RED\" triggered \"Intermission_Win_Limit\"\n";
			bWinner = true;
			iTeam = TF_TEAM_RED;
			iReason = WINREASON_WINLIMIT;
		}
	}

	// has one team go far enough ahead of the other team to trigger the win difference?
	if ( !bWinner )
	{
		int iWinLimit = mp_windifference.GetInt();
		if ( iWinLimit > 0 )
		{
			int iBlueScore = TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore();
			int iRedScore = TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore();

			if ( (iBlueScore - iRedScore) >= iWinLimit )
			{
				if ( (mp_windifference_min.GetInt() == 0) || (iBlueScore >= mp_windifference_min.GetInt()) )
				{
					pszReason = "Team \"BLUE\" triggered \"Intermission_Win_Limit\" due to mp_windifference\n";
					bWinner = true;
					iTeam = TF_TEAM_BLUE;
					iReason = WINREASON_WINDIFFLIMIT;
				}
			}
			else if ( (iRedScore - iBlueScore) >= iWinLimit )
			{
				if ( (mp_windifference_min.GetInt() == 0) || (iRedScore >= mp_windifference_min.GetInt()) )
				{
					pszReason = "Team \"RED\" triggered \"Intermission_Win_Limit\" due to mp_windifference\n";
					bWinner = true;
					iTeam = TF_TEAM_RED;
					iReason = WINREASON_WINDIFFLIMIT;
				}
			}
		}
	}

	if ( bWinner )
	{
		if ( bAllowEnd )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
			if ( event )
			{
				if ( iReason == WINREASON_WINDIFFLIMIT )
				{
					event->SetString( "reason", "Reached Win Difference Limit" );
				}
				else
				{
					event->SetString( "reason", "Reached Win Limit" );
				}
				gameeventmanager->FireEvent( event );
			}

			if ( IsInTournamentMode() == true )
			{
				SetWinningTeam( iTeam, iReason, true, false, true );
			}
			else
			{
				GoToIntermission();
			}

			Assert( V_strlen( pszReason ) );
			UTIL_LogPrintf( "%s", pszReason );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CheckRespawnWaves()
{
	BaseClass::CheckRespawnWaves();

	// Look for overrides
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		if ( pTFPlayer->IsAlive() )
			continue; 

		if ( m_iRoundState == GR_STATE_PREROUND )
			continue;

		// Triggers can force a player to spawn at a specific time
		if ( pTFPlayer->GetRespawnTimeOverride() != -1.f && 
				gpGlobals->curtime > pTFPlayer->GetDeathTime() + pTFPlayer->GetRespawnTimeOverride() )
		{
			pTFPlayer->ForceRespawn();
		}
		else if ( IsPVEModeActive() )
		{
			// special stuff for PVE mode
			if ( !ShouldRespawnQuickly( pTFPlayer ) )
				continue;

			// If the player hasn't been dead the minimum respawn time, he
			// waits until the next wave.
			if ( !HasPassedMinRespawnTime( pTFPlayer ) )
				continue;

			if ( !pTFPlayer->IsReadyToSpawn() )
			{
				// Let the player spawn immediately when they do pick a class
				if ( pTFPlayer->ShouldGainInstantSpawn() )
				{
					pTFPlayer->AllowInstantSpawn();
				}
				continue;
			}

			// Respawn this player
			pTFPlayer->ForceRespawn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayWinSong( int team )
{
	if ( IsPlayingSpecialDeliveryMode() )
		return;

	bool bGameOver = IsGameOver();

	if ( !IsInStopWatch() || bGameOver )
	{
		// Give the match a chance to play something custom.  It returns true if it handled everything
		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
		if ( pMatchDesc && pMatchDesc->BPlayWinMusic( team, bGameOver ) )
		{
			return;
		}
	}

	if ( IsInTournamentMode() && IsInStopWatch() && ObjectiveResource() )
	{
		int iStopWatchTimer = ObjectiveResource()->GetStopWatchTimer();
		CTeamRoundTimer *pStopWatch = dynamic_cast< CTeamRoundTimer* >( UTIL_EntityByIndex( iStopWatchTimer ) );
		if ( ( pStopWatch && pStopWatch->IsWatchingTimeStamps() ) || ( !m_bForceMapReset ) )
		{
			BroadcastSound( 255, "MatchMaking.RoundEndStalemateMusic" );
			return;
		}
	}

	CTeamplayRoundBasedRules::PlayWinSong( team );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetWinningTeam( int team, int iWinReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false*/, bool bDontAddScore /* = false*/, bool bFinal /*= false*/ )
{
	// matching the value calculated in CTeamplayRoundBasedRules::State_Enter_TEAM_WIN() 
	// for m_flStateTransitionTime and adding 1 second to make sure we're covered
	int nTime = GetBonusRoundTime( bFinal ) + 1;  

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		// store our team for the response rules at the next round start
		// (teams might be switched for attack/defend maps)
		pTFPlayer->SetPrevRoundTeamNum( pTFPlayer->GetTeamNumber() );

		if ( team != TEAM_UNASSIGNED )
		{
			if ( pTFPlayer->GetTeamNumber() == team )
			{
				if ( pTFPlayer->IsAlive() )
				{
					pTFPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_BONUS_TIME, nTime );
				}
			}
			else
			{
				pTFPlayer->ClearExpression();
#ifdef GAME_DLL
				// Loser karts get max Damage and stun
				if ( pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
				{
					pTFPlayer->AddKartDamage( 666 );
					pTFPlayer->m_Shared.StunPlayer( 1.5f, 1.0f, TF_STUN_BOTH );		// Short full stun then slow
					pTFPlayer->m_Shared.StunPlayer( 10.0f, 0.25f, TF_STUN_MOVEMENT );
				}
#endif //GAME_DLL
			}
		}
	}

	DuelMiniGame_AssignWinners();

#ifdef TF_RAID_MODE
	if ( !IsBossBattleMode() )
	{
		// Don't do a full reset in Raid mode if the defending team didn't win
		if ( IsRaidMode() && team != TF_TEAM_PVE_DEFENDERS )
		{
			bForceMapReset = false;
		}
	}
#endif // TF_RAID_MODE

	SetBirthdayPlayer( NULL );

#ifdef GAME_DLL
	if ( m_bPlayingKoth )
	{
		// Increment BLUE KOTH cap time
		CTeamRoundTimer *pKOTHTimer = TFGameRules()->GetBlueKothRoundTimer();
		GetGlobalTFTeam( TF_TEAM_BLUE )->AddKOTHTime( pKOTHTimer->GetTimerMaxLength() - pKOTHTimer->GetTimeRemaining() );

		// Increment RED KOTH cap time
		pKOTHTimer = TFGameRules()->GetRedKothRoundTimer();
		GetGlobalTFTeam( TF_TEAM_RED )->AddKOTHTime( pKOTHTimer->GetTimerMaxLength() - pKOTHTimer->GetTimeRemaining() );
	}
	else if ( HasMultipleTrains() )
	{
		for ( int i = 0 ; i < ITFTeamTrainWatcher::AutoList().Count() ; ++i )
		{
			CTeamTrainWatcher *pTrainWatcher = static_cast< CTeamTrainWatcher* >( ITFTeamTrainWatcher::AutoList()[i] );
			if ( !pTrainWatcher->IsDisabled() )
			{
				if ( pTrainWatcher->GetTeamNumber() == TF_TEAM_RED )
				{
					GetGlobalTFTeam( TF_TEAM_RED )->AddPLRTrack( pTrainWatcher->GetTrainProgress() );
				}
				else
				{
					GetGlobalTFTeam( TF_TEAM_BLUE )->AddPLRTrack( pTrainWatcher->GetTrainProgress() );
				}
			}
		}
	}
#endif

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) && CTFMinigameLogic::GetMinigameLogic() )
	{
		CTFMiniGame *pMiniGame = CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame();
		if ( pMiniGame )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "minigame_win" );
			if ( event )
			{
				event->SetInt( "team", team );
				event->SetInt( "type", (int)( pMiniGame->GetMinigameType() ) );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	if ( IsPasstimeMode() )
	{
		CTF_GameStats.m_passtimeStats.summary.nRoundEndReason = iWinReason;
		CTF_GameStats.m_passtimeStats.summary.nRoundRemainingSec = GetActiveRoundTimer() ? (int) GetActiveRoundTimer()->GetTimeRemaining() : 0;
		CTF_GameStats.m_passtimeStats.summary.nScoreBlue = GetGlobalTFTeam( TF_TEAM_BLUE )->GetFlagCaptures();
		CTF_GameStats.m_passtimeStats.summary.nScoreRed = GetGlobalTFTeam( TF_TEAM_RED )->GetFlagCaptures();

		// stats reporting happens as a result of BaseClass::SetWinningTeam, but we need to make sure to
		// update ball carry data before stats are reported.
		// FIXME: refactor this so we're not calling it just for its side effects :/
		CPasstimeBall *pBall = g_pPasstimeLogic->GetBall();
		if ( pBall )
		{
			pBall->SetStateOutOfPlay();
		}
	}

	CTeamplayRoundBasedRules::SetWinningTeam( team, iWinReason, bForceMapReset, bSwitchTeams, bDontAddScore, bFinal );

	if ( IsCompetitiveMode() )
	{
		HaveAllPlayersSpeakConceptIfAllowed( IsGameOver() ? MP_CONCEPT_MATCH_OVER_COMP : MP_CONCEPT_GAME_OVER_COMP );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetStalemate( int iReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false */ )
{
	DuelMiniGame_AssignWinners();

	if ( IsPasstimeMode() )
	{
		CTF_GameStats.m_passtimeStats.summary.bStalemate = true;
		CTF_GameStats.m_passtimeStats.summary.bSuddenDeath = mp_stalemate_enable.GetBool();
		CTF_GameStats.m_passtimeStats.summary.bMeleeOnlySuddenDeath = mp_stalemate_meleeonly.GetBool();
	}

	BaseClass::SetStalemate( iReason, bForceMapReset, bSwitchTeams );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::GetPreMatchEndTime() const
{
	//TFTODO: implement this.
	return gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::GoToIntermission( void )
{
	// Tell the clients to recalculate the holiday
	IGameEvent *event = gameeventmanager->CreateEvent( "recalculate_holidays" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	UTIL_CalculateHolidays();

	BaseClass::GoToIntermission();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RecalculateTruce( void )
{
	bool bTruceActive = false;

	// Call a truce if the teams are fighting a Halloween boss
	if ( IsHolidayActive( kHoliday_Halloween ) )
	{
		if ( ( IMerasmusAutoList::AutoList().Count() > 0 ) || ( IEyeballBossAutoList::AutoList().Count() > 0 ) )
		{
			bool bHaveActiveBoss = false;

			for ( int i = 0; i < IMerasmusAutoList::AutoList().Count(); ++i )
			{
				CMerasmus *pBoss = static_cast< CMerasmus* >( IMerasmusAutoList::AutoList()[i] );
				if ( !pBoss->IsMarkedForDeletion() )
				{
					bHaveActiveBoss = true;
				}
			}

			for ( int i = 0; i < IEyeballBossAutoList::AutoList().Count(); ++i )
			{
				CEyeballBoss *pBoss = static_cast< CEyeballBoss* >( IEyeballBossAutoList::AutoList()[i] );
				if ( !pBoss->IsMarkedForDeletion() )
				{
					if ( ( pBoss->GetTeamNumber() != TF_TEAM_RED ) && ( pBoss->GetTeamNumber() != TF_TEAM_BLUE ) )
					{
						bHaveActiveBoss = true;
					}
				}
			}

			if ( bHaveActiveBoss && ( IsValveMap() || tf_halloween_allow_truce_during_boss_event.GetBool() || IsMapForcedTruceDuringBossFight() ) )
			{
				bTruceActive = true;
			}
		}
	}


	if ( m_bTruceActive != bTruceActive )
	{
		m_bTruceActive.Set( bTruceActive );

		CReliableBroadcastRecipientFilter filter;
		if ( m_bTruceActive )
		{
			SendHudNotification( filter, HUD_NOTIFY_TRUCE_START, true );
			if ( m_hGamerulesProxy )
			{
				m_hGamerulesProxy->TruceStart();
			}
		}
		else
		{
			SendHudNotification( filter, HUD_NOTIFY_TRUCE_END, true );
			if ( m_hGamerulesProxy )
			{
				m_hGamerulesProxy->TruceEnd();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	// guard against NULL pointers if players disconnect
	if ( !pPlayer || !pAttacker )
		return false;

	if ( IsTruceActive() && ( pPlayer != pAttacker ) && ( pPlayer->GetTeamNumber() != pAttacker->GetTeamNumber() ) )
	{
		if ( ( ( pAttacker->GetTeamNumber() == TF_TEAM_RED ) && ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) ) || ( ( pAttacker->GetTeamNumber() == TF_TEAM_BLUE ) && ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ) )
		{
			CBaseEntity *pInflictor = info.GetInflictor();
			if ( pInflictor )
			{
				return !( pInflictor->IsTruceValidForEnt() || pAttacker->IsTruceValidForEnt() );
			}
			else
			{
				return !pAttacker->IsTruceValidForEnt();
			}
		}
	}

	// if pAttacker is an object, we can only do damage if pPlayer is our builder
	if ( pAttacker->IsBaseObject() )
	{
		CBaseObject *pObj = ( CBaseObject *)pAttacker;

		if ( pObj->GetBuilder() == pPlayer || pPlayer->GetTeamNumber() != pObj->GetTeamNumber() )
		{
			// Builder and enemies
			return true;
		}
		else
		{
			// Teammates of the builder
			return false;
		}
	}

	// prevent eyeball rockets from hurting teammates if it's a spell
	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_MONOCULUS && pAttacker->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
		return false;
	}

	// in PvE modes, if entities are on the same team, they can't hurt each other
	// this is needed since not all entities will be players
	if ( IsPVEModeActive() && 
			pPlayer->GetTeamNumber() == pAttacker->GetTeamNumber() && 
			pPlayer != pAttacker && 
			!info.IsForceFriendlyFire() )
	{
		return false;
	}

	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker, info );
}

Vector DropToGround( 
	CBaseEntity *pMainEnt,
	const Vector &vPos, 
	const Vector &vMins, 
	const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while( pSpot )
	{
		// trace a box here
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );


// -------------------------------------------------------------------------------- //

void cc_ShowRespawnTimes()
{
	CTFGameRules *pRules = TFGameRules();
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	if ( pRules && pPlayer )
	{
		float flRedMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] : mp_respawnwavetime.GetFloat() );
		float flRedScalar = pRules->GetRespawnTimeScalar( TF_TEAM_RED );
		float flNextRedRespawn = pRules->GetNextRespawnWave( TF_TEAM_RED, NULL ) - gpGlobals->curtime;

		float flBlueMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] : mp_respawnwavetime.GetFloat() );
		float flBlueScalar = pRules->GetRespawnTimeScalar( TF_TEAM_BLUE );
		float flNextBlueRespawn = pRules->GetNextRespawnWave( TF_TEAM_BLUE, NULL ) - gpGlobals->curtime;

		char tempRed[128];
		Q_snprintf( tempRed, sizeof( tempRed ),   "Red:  Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flRedMin, flRedScalar, flNextRedRespawn );

		char tempBlue[128];
		Q_snprintf( tempBlue, sizeof( tempBlue ), "Blue: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flBlueMin, flBlueScalar, flNextBlueRespawn );

		ClientPrint( pPlayer, HUD_PRINTTALK, tempRed );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempBlue );
	}
}

ConCommand mp_showrespawntimes( "mp_showrespawntimes", cc_ShowRespawnTimes, "Show the min respawn times for the teams", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( IsInItemTestingMode() && pTFPlayer->m_bItemTestingRespawn )
	{
		pTFPlayer->m_bItemTestingRespawn = false;
		return NULL;
	}

	// get valid spawn point
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	// drop down to ground
	Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN_SCALED( pTFPlayer ), VEC_HULL_MAX_SCALED( pTFPlayer ) );

	// Move the player to the place it said.
	pPlayer->SetLocalOrigin( GroundPos + Vector(0,0,1) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers, PlayerTeamSpawnMode_t nSpawnMode /* = 0*/ )
{
	bool bMatchSummary = ShowMatchSummary();

	// Check the team.
	// In Item Testing mode, bots all use the Red team spawns, and the player uses Blue
	if ( IsInItemTestingMode() )
	{
		if ( pSpot->GetTeamNumber() != (pPlayer->IsFakeClient() ? TF_TEAM_RED : TF_TEAM_BLUE) )
			return false;
	}
	else
	{
		if ( !bMatchSummary )
		{
			if ( pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
				return false;
		}
	}

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;

		if ( pCTFSpawn->GetTeamSpawnMode() && pCTFSpawn->GetTeamSpawnMode() != nSpawnMode )
			return false;

		if ( bMatchSummary )
		{
			if ( pCTFSpawn->AlreadyUsedForMatchSummary() )
				return false; 

			if ( pCTFSpawn->GetMatchSummaryType() == PlayerTeamSpawn_MatchSummary_Winner )
			{
				if ( pPlayer->GetTeamNumber() != GetWinningTeam() )
					return false;
			}
			else if ( pCTFSpawn->GetMatchSummaryType() == PlayerTeamSpawn_MatchSummary_Loser )
			{
				if ( pPlayer->GetTeamNumber() == GetWinningTeam() )
					return false;
			}
			else
			{
				return false;
			}
		}
		else
		{
			if ( pCTFSpawn->GetMatchSummaryType() != PlayerTeamSpawn_MatchSummary_None )
				return false;
		}
	}

	Vector mins = VEC_HULL_MIN_SCALED( pPlayer );
	Vector maxs = VEC_HULL_MAX_SCALED( pPlayer );

	if ( !bIgnorePlayers && !bMatchSummary )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	if ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) )
	{
		if ( bMatchSummary )
		{
			if ( pCTFSpawn )
			{
				pCTFSpawn->SetAlreadyUsedForMatchSummary();
			}
		}

		return true;
	}

	return false;
}

Vector CTFGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTFGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

int CTFGameRules::ItemShouldRespawn( CItem *pItem )
{
	return BaseClass::ItemShouldRespawn( pItem );
}

float CTFGameRules::FlItemRespawnTime( CItem *pItem )
{
	return ITEM_RESPAWN_TIME;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// coach?
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer->IsCoaching() )
	{
		pszFormat = "TF_Chat_Coach";
	}
	// team only
	else if ( bTeamOnly == true )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TF_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TF_Chat_Team";
				}
			}
		}
	}
	// everyone
	else
	{	
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";	
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_AllDead";
			}
			else
			{
				pszFormat = "TF_Chat_All";	
			}
		}
	}

	return pszFormat;
}

VoiceCommandMenuItem_t *CTFGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer )
	{
		if ( pTFPlayer->BHaveChatSuspensionInCurrentMatch() )
		{
			if ( tf_voice_command_suspension_mode.GetInt() == 1 )
			{
				return NULL;
			}
		}

		if ( pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		{
			engine->ClientCommand( pTFPlayer->edict(), "boo" );
			return NULL;
		}
	}

	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );

	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );

		if ( iActivity != ACT_INVALID )
		{
			if ( pTFPlayer )
			{
				pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );

				// Note when we call for a Medic.
				// Hardcoding this for menu 0, item 0 is an ugly hack, but we don't have a good way to 
				// translate this at this level without plumbing a through bunch of stuff (MSB)
				if ( iMenu == 0 && iItem == 0 )
				{
					pTFPlayer->NoteMedicCall();
				}
			}
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTFGameRules::ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName )
{
	if ( !tf_allow_player_name_change.GetBool() )
		return;

	const char *pszOldName = pPlayer->GetPlayerName();

	// Check if they can change their name
	// don't show when bots change their names in PvE mode
	if ( State_Get() != GR_STATE_STALEMATE && !( IsPVEModeActive() && pPlayer->IsBot() ) )
	{
		CReliableBroadcastRecipientFilter filter;
		UTIL_SayText2Filter( filter, pPlayer, false, "#TF_Name_Change", pszOldName, pszNewName );

		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszNewName );
			gameeventmanager->FireEvent( event );
		}
	}

	pPlayer->SetPlayerName( pszNewName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strncmp( pszOldName, pszName, MAX_PLAYER_NAME_LENGTH-1 ) )		
	{
		ChangePlayerName( pTFPlayer, pszName );
	}

	// keep track of their hud_classautokill value
	int nClassAutoKill = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "hud_classautokill" ) );
	pTFPlayer->SetHudClassAutoKill( nClassAutoKill > 0 ? true : false );

	// keep track of their tf_medigun_autoheal value
	pTFPlayer->SetMedigunAutoHeal( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_medigun_autoheal" ) ) > 0 );

	// keep track of their cl_autorezoom value
	pTFPlayer->SetAutoRezoom( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autorezoom" ) ) > 0 );
	pTFPlayer->SetAutoReload( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autoreload" ) ) > 0 );

	// keep track of their tf_remember_lastswitched value
	pTFPlayer->SetRememberActiveWeapon( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_remember_activeweapon" ) ) > 0 );
	pTFPlayer->SetRememberLastWeapon( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_remember_lastswitched" ) ) > 0 );

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	int iFov = atoi(pszFov);
	iFov = clamp( iFov, 75, MAX_FOV );

	pTFPlayer->SetDefaultFOV( iFov );

	pTFPlayer->m_bFlipViewModels = Q_strcmp( engine->GetClientConVarValue( pPlayer->entindex(), "cl_flipviewmodels" ), "1" ) == 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CTFGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;

		if ( pTFPlayer )
		{
			// Get the max carrying capacity for this ammo
			int iMaxCarry = pTFPlayer->GetMaxAmmo(iAmmoIndex);

			// Does the player have room for more of this type of ammo?
			if ( pTFPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool EconEntity_KillEaterEventPassesRestrictionCheck( const IEconItemInterface *pEconInterface, const CEconItemAttributeDefinition *pRestrictionAttribDef, const CEconItemAttributeDefinition *pRestrictionValueAttribDef, const CSteamID VictimSteamID )
{
	uint32 unRestrictionType = kStrangeEventRestriction_None;
	attrib_value_t unRestrictionValue;
	DbgVerify( pEconInterface->FindAttribute( pRestrictionAttribDef, &unRestrictionType ) ==
			   pEconInterface->FindAttribute( pRestrictionValueAttribDef, &unRestrictionValue ) );

	switch (unRestrictionType)
	{
	case kStrangeEventRestriction_None:
		return true;

	case kStrangeEventRestriction_Map:
	{
		const MapDef_t *pMap = GetItemSchema()->GetMasterMapDefByName(STRING(gpGlobals->mapname));
		return pMap && pMap->m_nDefIndex == unRestrictionValue;
	}
	case kStrangeEventRestriction_VictimSteamAccount:
		return VictimSteamID.GetAccountID() == unRestrictionValue;
	case kStrangeEventRestriction_Competitive:
	{
		if ( TFGameRules() && TFGameRules()->IsMatchTypeCompetitive() )
		{
			// Match Season unRestrictionValue == Season Number
			return true;
		}
		return false;
	}
	} // end Switch

	AssertMsg1( false, "Unhandled strange event restriction %u!", unRestrictionType );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static CSteamID GetSteamIDForKillEaterScoring( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsBot() )
		return k_steamIDNil;

	CSteamID ret;
	if ( !pPlayer->GetSteamID( &ret ) )
		return k_steamIDNil;

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CStrangeEventValidator
{
public:
	CStrangeEventValidator( IEconItemInterface *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim, kill_eater_event_t eEventType )
		: m_unItemID( INVALID_ITEM_ID )
		, m_eEventType( eEventType )
	{
		m_bIsValid = BInitEventParams( pEconEntity, pOwner, pVictim );
	}

	bool BIsValidEvent() const { return m_bIsValid; }
	CSteamID GetKillerSteamID() const { Assert( m_bIsValid ); return m_KillerSteamID; }
	CSteamID GetVictimSteamID() const { Assert( m_bIsValid ); return m_VictimSteamID; }
	itemid_t GetItemID() const { Assert( m_bIsValid ); return m_unItemID; }
	kill_eater_event_t GetEventType() const { Assert( m_bIsValid ); return m_eEventType; }

private:
	bool BInitEventParams( IEconItemInterface *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim )
	{
		static CSchemaAttributeDefHandle pAttrDef_KillEater( "kill eater" );

		// Kill-eater weapons.
		if ( !pEconEntity )
			return false;

		if ( !pOwner )
			return false;

		if ( !pVictim )
			return false;

		// Ignore events where we're affecting ourself.
		if ( pOwner == pVictim )
			return false;

		// Store off our item ID for reference post-init.
		m_unItemID = pEconEntity->GetID();

		// Always require that we have at least the base kill eater attribute before sending any messages
		// to the GC.
		if ( !pEconEntity->FindAttribute( pAttrDef_KillEater ) )
			return false;

		// Don't bother sending a message to the GC if either party is a bot, unless we're tracking events against
		// bots specifically.
		if ( !pOwner->GetSteamID( &m_KillerSteamID ) )
			return false;
	
		m_VictimSteamID = GetSteamIDForKillEaterScoring( pVictim );
		if ( !m_VictimSteamID.IsValid() )
		{
			if ( !GetItemSchema()->GetKillEaterScoreTypeAllowsBotVictims( m_eEventType ) )
				return false;
		}

		// Also require that we have whatever event type we're looking for, unless we're looking for regular
		// player kills in which case we may or may not have a field to describe that.
		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			const CEconItemAttributeDefinition *pScoreAttribDef = GetKillEaterAttr_Score(i);
			if ( !pScoreAttribDef )
				continue;

			// If we don't have this attribute, move on. It's possible to be missing this attribute but still
			// have the next one in the list if we have user-customized tracking types.
			if ( !pEconEntity->FindAttribute( pScoreAttribDef ) )
				continue;
		
			const CEconItemAttributeDefinition *pScoreTypeAttribDef = GetKillEaterAttr_Type(i);
			if ( !pScoreTypeAttribDef )
				continue;
		
			// If we're missing our type attribute, we interpret that as "track kills" -- none of the original
			// kill eaters have score type attributes.
			float fKillEaterEventType;
			kill_eater_event_t eMatchEvent = FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconEntity, pScoreTypeAttribDef, &fKillEaterEventType )
										   ? (kill_eater_event_t)(int)fKillEaterEventType
										   : kKillEaterEvent_PlayerKill;

			if ( m_eEventType == eMatchEvent )
			{
				// Does this attribute also specify a restriction (ie., "has to be on this specific map" or "has to be
				// against this specific account"? If so, only count this as a match if the restriction check passes.
				if ( EconEntity_KillEaterEventPassesRestrictionCheck( pEconEntity, GetKillEaterAttr_Restriction(i), GetKillEaterAttr_RestrictionValue(i), m_VictimSteamID ) )
					return true;
			}
		}

		return false;
	}

private:
	bool m_bIsValid;
	CSteamID m_KillerSteamID;
	CSteamID m_VictimSteamID;
	itemid_t m_unItemID;
	kill_eater_event_t m_eEventType;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename tMsgType >
static void FillOutBaseKillEaterMessage( tMsgType *out_pMsg, const CStrangeEventValidator& Validator )
{
	Assert( out_pMsg );
	Assert( Validator.BIsValidEvent() );

	out_pMsg->set_killer_steam_id( Validator.GetKillerSteamID().ConvertToUint64() );
	out_pMsg->set_victim_steam_id( Validator.GetVictimSteamID().ConvertToUint64() );
	out_pMsg->set_item_id( Validator.GetItemID() );
	out_pMsg->set_event_type( Validator.GetEventType() );
}

//-----------------------------------------------------------------------------
// Purpose: Get the actual killeater weapon from a CTakeDamageInfo
//-----------------------------------------------------------------------------
CTFWeaponBase *GetKilleaterWeaponFromDamageInfo( const CTakeDamageInfo *pInfo )
{
	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase *>( pInfo->GetWeapon() );
	CBaseEntity* pInflictor = pInfo->GetInflictor();

	// If there's no weapon specified, it might have been from a sentry
	if ( !pTFWeapon )
	{
		// If a sentry did the damage with bullets, it's the inflictor
		CObjectSentrygun *pSentrygun = dynamic_cast<CObjectSentrygun *>( pInflictor );
		if ( !pSentrygun )
		{
			// If a sentry's rocket did the damage, then the rocket is the inflictor, and the sentry is the rocket's owner
			pSentrygun = pInflictor ? dynamic_cast<CObjectSentrygun *>( pInflictor->GetOwnerEntity() ) : NULL;
		}

		if ( pSentrygun )
		{
			// Maybe they were using a Wrangler?  The builder is the player who owns the sentry
			CTFPlayer *pBuilder = pSentrygun->GetBuilder();
			if ( pBuilder )
			{
				// Check if they have a Wrangler and were aiming with it
				CTFLaserPointer* pLaserPointer = dynamic_cast< CTFLaserPointer * >( pBuilder->GetEntityForLoadoutSlot( LOADOUT_POSITION_SECONDARY ) );
				if ( pLaserPointer && pLaserPointer->HasLaserDot() )
				{
					pTFWeapon =  pLaserPointer;
				}
			}
		}
	}

	// need to check for deflected projectiles so the weapon that did the most recent
	// deflection will get the killeater credit instead of the original launcher
	if ( pInflictor )
	{
		CTFWeaponBaseGrenadeProj *pBaseGrenade = dynamic_cast< CTFWeaponBaseGrenadeProj* >( pInflictor );
		if ( pBaseGrenade )
		{
			if ( pBaseGrenade->GetDeflected() && pBaseGrenade->GetLauncher() )
			{
				pTFWeapon = dynamic_cast< CTFWeaponBase* >( pBaseGrenade->GetLauncher() );
			}
		}
		else
		{
			CTFBaseRocket *pRocket = dynamic_cast< CTFBaseRocket* >( pInflictor );
			if ( pRocket )
			{
				if ( pRocket->GetDeflected() && pRocket->GetLauncher() )
				{
					pTFWeapon = dynamic_cast< CTFWeaponBase* >( pRocket->GetLauncher() );
				}
			}
		}
	}

	return pTFWeapon;
}

//-----------------------------------------------------------------------------
void EconEntity_ValidateAndSendStrangeMessageToGC( IEconItemInterface *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	CStrangeEventValidator Validator( pEconEntity, pOwner, pVictim, eEventType );

	if ( !Validator.BIsValidEvent() )
		return;

	GCSDK::CProtoBufMsg<CMsgIncrementKillCountAttribute> msg( k_EMsgGC_IncrementKillCountAttribute );
	FillOutBaseKillEaterMessage( &msg.Body(), Validator );

	if ( nIncrementValue != 1 )
	{
		msg.Body().set_increment_value( nIncrementValue );
	}

	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void EconEntity_OnOwnerKillEaterEvent( CEconEntity *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	if ( !pEconEntity )
		return;

	EconItemInterface_OnOwnerKillEaterEvent( pEconEntity->GetAttributeContainer()->GetItem(), pOwner, pVictim, eEventType, nIncrementValue );
}
//-----------------------------------------------------------------------------
void EconItemInterface_OnOwnerKillEaterEvent( IEconItemInterface *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	bool bEconEntityHandled = false;

	// Fire the kill eater event on all wearables
	// Wearables can have all Strange parts
	for ( int i = 0; i < pOwner->GetNumWearables(); ++i )
	{
		CTFWearable *pWearableItem = dynamic_cast<CTFWearable *>( pOwner->GetWearable( i ) );
		if ( !pWearableItem )
			continue;

		if ( !pWearableItem->GetAttributeContainer() )
			continue;

		EconEntity_ValidateAndSendStrangeMessageToGC( pWearableItem->GetAttributeContainer()->GetItem(), pOwner, pVictim, eEventType, nIncrementValue );

		if ( pWearableItem->GetAttributeContainer()->GetItem() == pEconEntity )
		{
			bEconEntityHandled = true;
		}
	}

	if ( !bEconEntityHandled )
	{
		EconEntity_ValidateAndSendStrangeMessageToGC( pEconEntity, pOwner, pVictim, eEventType, nIncrementValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void EconEntity_OnOwnerKillEaterEventNoPartner( CEconEntity *pEconEntity, CTFPlayer *pOwner, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	if ( !pEconEntity )
		return;

	EconItemInterface_OnOwnerKillEaterEventNoPartner( pEconEntity->GetAttributeContainer()->GetItem(), pOwner,  eEventType, nIncrementValue );
}
//-----------------------------------------------------------------------------
void EconItemInterface_OnOwnerKillEaterEventNoPartner( IEconItemInterface *pEconEntity, CTFPlayer *pOwner, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	// Look for another player to pass in. We do two passes over the list of candidates.
	// First we look for anyone that we know is a real player. If we don't find any of
	// those then we pass in a known bot. This means that things that allow bots will
	// still find one and work, but event types that don't allow bots won't be dependent
	// on the order of players in the list.
	for( int iPass = 1; iPass <= 2; iPass++ )
	{
		for( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
		{
			CTFPlayer *pOtherPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
			if ( !pOtherPlayer )
				continue;

			// Ignore ourself.
			if ( pOtherPlayer == pOwner )
				continue;

		
			const bool bCandidatePasses = (iPass == 1 && GetSteamIDForKillEaterScoring( pOtherPlayer ).IsValid())	// if we have a real Steam ID we're golden
									   || (iPass == 2);																// or if we made it to the second pass, any valid player pointer is as good as any other
			if ( bCandidatePasses )
			{
				EconItemInterface_OnOwnerKillEaterEvent( pEconEntity, pOwner, pOtherPlayer, eEventType, nIncrementValue );
				return;
			}
		}
	}

	// We couldn't find anyone else at all. This is hard. Let's give up.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static GCSDK::CProtoBufMsg<CMsgIncrementKillCountAttribute_Multiple> *s_pmsgIncrementKillCountMessageBatch = NULL;
//-----------------------------------------------------------------------------
void EconEntity_ValidateAndSendStrangeMessageToGC_Batched( IEconItemInterface *pEconInterface, class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	CStrangeEventValidator Validator( pEconInterface, pOwner, pVictim, eEventType );

	if ( !Validator.BIsValidEvent() )
		return;

	// This has to be dynamically allocated because we can't static-init a protobuf message at startup type.
	if ( !s_pmsgIncrementKillCountMessageBatch )
	{
		s_pmsgIncrementKillCountMessageBatch = new GCSDK::CProtoBufMsg<CMsgIncrementKillCountAttribute_Multiple>( k_EMsgGC_IncrementKillCountAttribute_Multiple );
	}

	// Look for an existing message that matches this user and this event.
	for ( int i = 0; i < s_pmsgIncrementKillCountMessageBatch->Body().msgs_size(); i++ )
	{
		CMsgIncrementKillCountAttribute *pMsg = s_pmsgIncrementKillCountMessageBatch->Body().mutable_msgs( i );
		Assert( pMsg );

		if ( pMsg->killer_steam_id() == Validator.GetKillerSteamID().ConvertToUint64() &&
			pMsg->victim_steam_id() == Validator.GetVictimSteamID().ConvertToUint64() &&
			pMsg->item_id() == Validator.GetItemID() &&
			(kill_eater_event_t)pMsg->event_type() == Validator.GetEventType() )
		{
			// We found an existing entry for this message. All we want to do is add to the stat we're tracking
			// but still send up this single message.
			Assert( pMsg->has_increment_value() );

			pMsg->set_increment_value( pMsg->increment_value() + nIncrementValue );
			return;
		}
	}

	// We don't have an existing entry, so make a new one.
	CMsgIncrementKillCountAttribute *pNewMsg = s_pmsgIncrementKillCountMessageBatch->Body().add_msgs();

	FillOutBaseKillEaterMessage( pNewMsg, Validator );
	pNewMsg->set_increment_value( nIncrementValue );
}
//-----------------------------------------------------------------------------
void EconEntity_OnOwnerKillEaterEvent_Batched( CEconEntity *pEconEntity, class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	if ( !pEconEntity )
		return;

	EconItemInterface_OnOwnerKillEaterEvent_Batched( pEconEntity->GetAttributeContainer()->GetItem(), pOwner, pVictim, eEventType, nIncrementValue );
}
//-----------------------------------------------------------------------------
void EconItemInterface_OnOwnerKillEaterEvent_Batched( IEconItemInterface *pEconInterface, class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue /*= 1*/ )
{
	for ( int i = 0; i < pOwner->GetNumWearables(); ++i )
	{
		CTFWearable *pWearableItem = dynamic_cast<CTFWearable *>( pOwner->GetWearable( i ) );
		if ( !pWearableItem )
			continue;

		if ( !pWearableItem->GetAttributeContainer() )
			continue;

		CEconItemView *pEconItemView = pWearableItem->GetAttributeContainer()->GetItem();
		if ( !pEconItemView )
			continue;

		EconEntity_ValidateAndSendStrangeMessageToGC_Batched( pWearableItem->GetAttributeContainer()->GetItem(), pOwner, pVictim, eEventType, nIncrementValue );
	}

	EconEntity_ValidateAndSendStrangeMessageToGC_Batched( pEconInterface, pOwner, pVictim, eEventType, nIncrementValue );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void KillEaterEvents_FlushBatches()
{
	if ( s_pmsgIncrementKillCountMessageBatch )
	{
		Assert( s_pmsgIncrementKillCountMessageBatch->Body().msgs_size() > 0 );

		GCClientSystem()->BSendMessage( *s_pmsgIncrementKillCountMessageBatch );

		delete s_pmsgIncrementKillCountMessageBatch;
		s_pmsgIncrementKillCountMessageBatch = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void HatAndMiscEconEntities_OnOwnerKillEaterEventNoParter( class CTFPlayer *pOwner, kill_eater_event_t eEventType, int nIncrementValue )
{
	EconItemInterface_OnOwnerKillEaterEventNoPartner( NULL, pOwner, eEventType, nIncrementValue );
}

//-----------------------------------------------------------------------------
void HatAndMiscEconEntities_OnOwnerKillEaterEvent( class CTFPlayer *pOwner, class CTFPlayer *pVictim, kill_eater_event_t eEventType, int nIncrementValue )
{
	EconItemInterface_OnOwnerKillEaterEvent( NULL, pOwner, pVictim, eEventType, nIncrementValue );
}

//-----------------------------------------------------------------------------
void EconEntity_OnOwnerUniqueEconEvent( IEconItemInterface *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim, kill_eater_event_t eEventType )
{
	CStrangeEventValidator Validator( pEconEntity, pOwner, pVictim, eEventType );

	if ( !Validator.BIsValidEvent() )
		return;

	GCSDK::CProtoBufMsg<CMsgTrackUniquePlayerPairEvent> msg( k_EMsgGC_TrackUniquePlayerPairEvent );
	FillOutBaseKillEaterMessage( &msg.Body(), Validator );

	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
void EconEntity_OnOwnerUniqueEconEvent( CEconEntity *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim, kill_eater_event_t eEventType )
{
	if ( !pEconEntity )
		return;
	EconEntity_OnOwnerUniqueEconEvent( pEconEntity->GetAttributeContainer()->GetItem(), pOwner, pVictim, eEventType );
}

//-----------------------------------------------------------------------------
void EconEntity_NonEquippedItemKillTracking( CTFPlayer *pOwner, CTFPlayer *pVictim, item_definition_index_t iDefIndex, kill_eater_event_t eEventType, int nIncrementValue = 1 )
{
	// Validate
	if ( !pOwner || !pVictim || pOwner == pVictim )
		return;

	if ( pOwner->IsBot() || pVictim->IsBot() )
		return;

	CSteamID pOwnerSteamID;
	CSteamID pVictimSteamID;
	if ( !pOwner->GetSteamID( &pOwnerSteamID ) || !pVictim->GetSteamID( &pVictimSteamID ) )
		return;
	
	// Current date
	static CSchemaAttributeDefHandle pAttrDef_DeactiveDate( "deactive date" );
	uint32 unCurrentDate = CRTime::RTime32TimeCur();

	CEconItemView *pNonEquippedItem = pOwner->Inventory()->GetFirstItemOfItemDef( iDefIndex, pOwner->Inventory() );
	if ( pNonEquippedItem )
	{
		// Check the date
		uint32 unDeactiveDate = 0;
		if ( !pNonEquippedItem->FindAttribute( pAttrDef_DeactiveDate, &unDeactiveDate ) || unDeactiveDate > unCurrentDate )
		{
			GCSDK::CProtoBufMsg<CMsgIncrementKillCountAttribute> msg( k_EMsgGC_IncrementKillCountAttribute );
			msg.Body().set_killer_steam_id( pOwnerSteamID.ConvertToUint64() );
			msg.Body().set_victim_steam_id( pVictimSteamID.ConvertToUint64() );
			
			if ( nIncrementValue > 1 )
			{
				msg.Body().set_increment_value( nIncrementValue );
			}

			msg.Body().set_item_id( pNonEquippedItem->GetID() );
			msg.Body().set_event_type( eEventType );
			GCClientSystem()->BSendMessage( msg );
		}
	}

}

//-----------------------------------------------------------------------------
void EconEntity_NonEquippedItemKillTracking_NoPartner( CTFPlayer *pOwner, item_definition_index_t iDefIndex, kill_eater_event_t eEventType, int nIncrementValue )
{
	// Find a partner or give up
	for( int iPass = 1; iPass <= 2; iPass++ )
	{
		for( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
		{
			CTFPlayer *pOtherPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
			if ( !pOtherPlayer )
				continue;

			// Ignore ourself.
			if ( pOtherPlayer == pOwner )
				continue;


			const bool bCandidatePasses = (iPass == 1 && GetSteamIDForKillEaterScoring( pOtherPlayer ).IsValid())	// if we have a real Steam ID we're golden
				|| (iPass == 2);																// or if we made it to the second pass, any valid player pointer is as good as any other

			if ( bCandidatePasses )
			{
				EconEntity_NonEquippedItemKillTracking( pOwner, pOtherPlayer, iDefIndex, eEventType, nIncrementValue );
				return;
			}
		}
	}
}

void EconEntity_NonEquippedItemKillTracking_NoPartnerBatched( class CTFPlayer *pOwner, item_definition_index_t iDefIndex, kill_eater_event_t eEventType, int nIncrementValue )
{
	// Find a partner or give up
	for ( int iPass = 1; iPass <= 2; iPass++ )
	{
		for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
		{
			CTFPlayer *pOtherPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
			if ( !pOtherPlayer )
				continue;

			// Ignore ourself.
			if ( pOtherPlayer == pOwner )
				continue;


			const bool bCandidatePasses = ( iPass == 1 && GetSteamIDForKillEaterScoring( pOtherPlayer ).IsValid() )	// if we have a real Steam ID we're golden
				|| ( iPass == 2 );																// or if we made it to the second pass, any valid player pointer is as good as any other

			if ( bCandidatePasses )
			{
				// find the item
				// Current date
				static CSchemaAttributeDefHandle pAttrDef_DeactiveDate( "deactive date" );
				uint32 unCurrentDate = CRTime::RTime32TimeCur();

				CEconItemView *pNonEquippedItem = pOwner->Inventory()->GetFirstItemOfItemDef( iDefIndex, pOwner->Inventory() );
				if ( pNonEquippedItem )
				{
					uint32 unDeactiveDate = 0;
					if ( !pNonEquippedItem->FindAttribute( pAttrDef_DeactiveDate, &unDeactiveDate ) || unDeactiveDate > unCurrentDate )
					{
						EconEntity_ValidateAndSendStrangeMessageToGC_Batched( pNonEquippedItem, pOwner, pOtherPlayer, eEventType, nIncrementValue );
					}
				}
				
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// TODO: Remove this call and only use the one above
//-----------------------------------------------------------------------------
void EconEntity_NonEquippedItemKillTracking( CTFPlayer *pOwner, CTFPlayer *pVictim, int nIncrementValue )
{
	// Validate
	if ( !pOwner || !pVictim || pOwner == pVictim )
		return;

	if ( pOwner->IsBot() || pVictim->IsBot() )
		return;

	CSteamID pOwnerSteamID;
	CSteamID pVictimSteamID;
	if ( ! pOwner->GetSteamID( &pOwnerSteamID ) || !pVictim->GetSteamID( &pVictimSteamID ) )
		return;

	// Find any active operation
	CUtlVector< item_definition_index_t > vecProcessedItems;
	const auto& dictOperations = GetItemSchema()->GetOperationDefinitions();
	FOR_EACH_DICT_FAST( dictOperations, i )
	{
		CEconOperationDefinition* pOperation = dictOperations[i];
		if ( pOperation->IsActive() && pOperation->IsCampaign() )
		{
			// we only want to process each required item once, but some operations have the same required item
			const item_definition_index_t unRequiredItemIndex = pOperation->GetRequiredItemDefIndex();
			if ( ( unRequiredItemIndex != INVALID_ITEM_DEF_INDEX ) && ( vecProcessedItems.Find( unRequiredItemIndex ) != vecProcessedItems.InvalidIndex() ) )
				continue;

			vecProcessedItems.AddToTail( unRequiredItemIndex );
			EconEntity_NonEquippedItemKillTracking( pOwner, pVictim, unRequiredItemIndex, kKillEaterEvent_CosmeticOperationKills, nIncrementValue );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sends a soul with the specified value to every living member of the
//			specified team, originating from vecPosition.
//-----------------------------------------------------------------------------
void CTFGameRules::DropHalloweenSoulPackToTeam( int nAmount, const Vector& vecPosition, int nTeamNumber, int nSourceTeam )
{
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( !pTFPlayer || !pTFPlayer->IsConnected() )
			continue;

		if ( pTFPlayer->GetTeamNumber() != nTeamNumber || !pTFPlayer->IsAlive() || pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			continue;

		DropHalloweenSoulPack( nAmount, vecPosition, pTFPlayer, nSourceTeam );
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::DropHalloweenSoulPack( int nAmount, const Vector& vecSource, CBaseEntity *pTarget, int nSourceTeam )
{
	QAngle angles(0,0,0);
	CHalloweenSoulPack *pSoulsPack = assert_cast<CHalloweenSoulPack*>( CBaseEntity::CreateNoSpawn( "halloween_souls_pack", vecSource, angles, NULL ) );

	if ( pSoulsPack )
	{	
		pSoulsPack->SetTarget( pTarget );
		pSoulsPack->SetAmount( nAmount );
		pSoulsPack->ChangeTeam( nSourceTeam );

		DispatchSpawn( pSoulsPack );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldDropSpellPickup()
{
	if ( IsUsingSpells() )
	{
		return RandomFloat() <= tf_player_spell_drop_on_death_rate.GetFloat();
	}
	return false;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::DropSpellPickup( const Vector& vPosition, int nTier /*= 0*/ ) const
{
	if ( !IsUsingSpells() )
		return;

	CSpellPickup *pSpellPickup = assert_cast<CSpellPickup*>( CBaseEntity::CreateNoSpawn( "tf_spell_pickup", vPosition + Vector( 0, 0, 50 ), vec3_angle, NULL ) );
	if ( pSpellPickup )
	{
		pSpellPickup->SetTier( nTier );

		DispatchSpawn( pSpellPickup );

		Vector vecImpulse = RandomVector( -0.5f, 0.5f );
		vecImpulse.z = 1.f;
		VectorNormalize( vecImpulse );

		Vector vecVelocity = vecImpulse * 500.f;
		pSpellPickup->DropSingleInstance( vecVelocity, NULL, 0 );
	}
}

//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldDropBonusDuck( void )
{
	if ( tf_player_drop_bonus_ducks.GetInt() < 0 )
	{
		return IsHolidayActive( kHoliday_EOTL );
	}
	else if ( tf_player_drop_bonus_ducks.GetInt() > 0 )
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldDropBonusDuckFromPlayer( CTFPlayer *pTFScorer, CTFPlayer *pTFVictim )
{
	if ( !pTFScorer || !pTFVictim )
		return false;

	// Only drop if bot is not involved
	if ( pTFScorer->IsBot() || pTFVictim->IsBot() || pTFScorer == pTFVictim )
		return false;

	return ShouldDropBonusDuck();
}

//-----------------------------------------------------------------------------
int CTFGameRules::GetDuckSkinForClass( int nTeam, int nClass ) const
{
	int nSkin = 0;
	if ( nTeam >= FIRST_GAME_TEAM )
	{
		bool bRed = ( nTeam == TF_TEAM_RED );

		switch ( nClass )
		{
		case TF_CLASS_SCOUT:
			nSkin = bRed ? 3 : 12;
			break;
		case TF_CLASS_SNIPER:
			nSkin = bRed ? 4 : 13;
			break;
		case TF_CLASS_SOLDIER:
			nSkin = bRed ? 5 : 14;
			break;
		case TF_CLASS_DEMOMAN:
			nSkin = bRed ? 6 : 15;
			break;
		case TF_CLASS_MEDIC:
			nSkin = bRed ? 7 : 16;
			break;
		case TF_CLASS_HEAVYWEAPONS:
			nSkin = bRed ? 8 : 17;
			break;
		case TF_CLASS_PYRO:
			nSkin = bRed ? 9 : 18;
			break;
		case TF_CLASS_SPY:
			nSkin = bRed ? 10 : 19;
			break;
		case TF_CLASS_ENGINEER:
			nSkin = bRed ? 11 : 20;
			break;
		default:
			nSkin = bRed ? 1 : 2;
			break;
		}
	}

	return nSkin;
}

//-----------------------------------------------------------------------------
//
ConVar tf_duck_edict_limit( "tf_duck_edict_limit", "1900", FCVAR_REPLICATED, "Maximum number of edicts allowed before spawning a duck" );
ConVar tf_duck_edict_warning( "tf_duck_edict_warning", "1800", FCVAR_REPLICATED, "Maximum number of edicts allowed before slowing duck spawn rate" );
void CTFGameRules::DropBonusDuck( const Vector& vPosition, CTFPlayer *pTFCreator /*=NULL*/, CTFPlayer *pAssister /*=NULL*/, CTFPlayer *pTFVictim /*=NULL*/, bool bCrit /*=false*/, bool bObjective /*=false*/) const
{
	if ( gEntList.NumberOfEdicts() > tf_duck_edict_limit.GetInt() )
	{
		Warning( "Warning: High level of Edicts, Not spawning Ducks \n" );
		return;
	}

	static CSchemaAttributeDefHandle pAttr_DuckLevelBadge( "duck badge level" );

	// Find Badge and increase number of ducks based on badge level (1 duck per 5 levels)
	// Look through equipped items, if one is a duck badge use its level
	int iDuckFlags = 0;
 	if ( pTFCreator == NULL || bObjective )
	{
		iDuckFlags |= EDuckFlags::DUCK_FLAG_OBJECTIVE;
	}
	uint32 iDuckBadgeLevel = 0;

	if ( pTFCreator )
	{
		for ( int i = 0; i < pTFCreator->GetNumWearables(); ++i )
		{
			CTFWearable* pWearable = dynamic_cast<CTFWearable*>( pTFCreator->GetWearable( i ) );
			if ( !pWearable )
				continue;

			//if ( pWearable->GetAttributeContainer() && pWearable->GetAttributeContainer()->GetItem() )
			{
				//CEconItemView *pItem = pWearable->GetAttributeContainer()->GetItem();
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWearable, iDuckBadgeLevel, duck_badge_level );
				//if ( pItem && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttr_DuckLevelBadge, &iDuckBadgeLevel ) )
				{
					iDuckBadgeLevel++;
				}
			}			
		}
	}
	// Badges only go to max of 5 now instead of 10 so just doubling the output value
	int iDuckPower = Min( (int)iDuckBadgeLevel * 2, 11 );
	float flBias = RemapValClamped( (float)iDuckPower, 0.0f, 10.0f, 0.2f, 0.5f);
	float flBiasScale = 3.0f;
	int iMinimum = 0;
	bool bSpecial = false;

	// Drop a few bonus ducks, extra ducks for Crits!
	//tf_duck_droprate_bias
	int iDuckCount = (int)( Bias( RandomFloat( 0, 1 ), flBias ) * ( flBiasScale + iDuckPower ) ) + iMinimum;
	iDuckCount = Max( iDuckCount, (int)iDuckBadgeLevel );	// min ducks for a badge

	if ( bCrit )
	{
		iDuckCount += RandomInt( 1, 2 );
	}

	if ( pTFCreator && ( iDuckBadgeLevel > 0 ) )
	{
		if ( RandomInt( 0, 600 ) <= iDuckPower )
		{
			// MEGA BONUS DUCKS
			iDuckCount += iDuckPower;
			bSpecial = true;
		}
	}

	if ( iDuckCount > 0 )
	{
		// Max of 50 ducks, which is a lot of ducks
		iDuckCount = Min( 50, iDuckCount );

		// High edict count, slow generation
		if ( gEntList.NumberOfEdicts() > tf_duck_edict_warning.GetInt() )
		{
			Warning( "Warning: High level of Edicts, Not spawning as many ducks\n" );
			iDuckCount /= 2;
		}

		int iCreatorId = pTFCreator ? pTFCreator->entindex() : -1;
		int iAssisterId = pAssister ? pAssister->entindex() : -1;

		int iVictimId = -1;
		int iDuckTeam = TEAM_UNASSIGNED;
		if ( pTFVictim )
		{
			iVictimId = pTFVictim->entindex();
			iDuckTeam = pTFVictim->GetTeamNumber();
		}

		for ( int i = 0; i < iDuckCount; ++i )
		{
			Vector vecOrigin = vPosition + Vector( 0, 0, 50 );
			CBonusDuckPickup *pDuckPickup = assert_cast<CBonusDuckPickup*>( CBaseEntity::CreateNoSpawn( "tf_bonus_duck_pickup", vecOrigin, vec3_angle, NULL ) );
			if ( pDuckPickup )
			{
				DispatchSpawn( pDuckPickup );

				Vector vecImpulse = RandomVector( -0.5f, 0.5f );
				vecImpulse.z = 1.f;
				VectorNormalize( vecImpulse );

				Vector vecVelocity = vecImpulse * RandomFloat( 350.0f, 450.0f );
				pDuckPickup->DropSingleInstance( vecVelocity, NULL, 1.0f );
				pDuckPickup->SetCreatorId( iCreatorId );
				pDuckPickup->SetVictimId( iVictimId );
				pDuckPickup->SetAssisterId( iAssisterId );
				pDuckPickup->ChangeTeam( iDuckTeam );
				pDuckPickup->SetDuckFlag( iDuckFlags );
				// random chance to have a special duck appear
				// Bonus duck implies atleast 1 Saxton
				if ( bSpecial || ( RandomInt( 1, 100 ) <= tf_test_special_ducks.GetInt() ) )
				{
					bSpecial = false;
					pDuckPickup->m_nSkin = 21; // Quackston Hale
					pDuckPickup->SetSpecial();

					CSingleUserRecipientFilter filter( pTFCreator );
					UserMessageBegin( filter, "BonusDucks" );
					WRITE_BYTE( iCreatorId );
					WRITE_BYTE( true );
					MessageEnd();
				}
				else
				{
					if ( iDuckPower > 0 )
					{
						pDuckPickup->m_nSkin = GetDuckSkinForClass( iDuckTeam, ( pTFVictim && pTFVictim->GetPlayerClass() ) ? pTFVictim->GetPlayerClass()->GetClassIndex() : -1 );
					}
					else
					{
						// Scorer without a badge only make normal ducks
						pDuckPickup->m_nSkin = iDuckTeam == TF_TEAM_RED ? 1 : 2;
					}
					pDuckPickup->SetModelScale( 0.7f );
				}
			}
		}

		// Update duckstreak count on player and assister if they have badges
		if ( pTFCreator && PlayerHasDuckStreaks( pTFCreator ) )
		{
			pTFCreator->m_Shared.IncrementStreak( CTFPlayerShared::kTFStreak_Ducks, iDuckCount );
		}
		if ( pAssister && PlayerHasDuckStreaks( pAssister ) )
		{
			pAssister->m_Shared.IncrementStreak( CTFPlayerShared::kTFStreak_Ducks, iDuckCount );
		}

		// Duck UserMessage
		if ( pTFCreator && pTFVictim && !TFGameRules()->HaveCheatsBeenEnabledDuringLevel() )
		{
			if ( IsHolidayActive( kHoliday_EOTL ) )
			{
				// Send a Message to Creator
				{
					// IsCreated, ID of Creator, ID of Victim, Count, IsGolden
					CSingleUserRecipientFilter userfilter( pTFCreator );
					UserMessageBegin( userfilter, "EOTLDuckEvent" );
					WRITE_BYTE( true );
					WRITE_BYTE( iCreatorId );
					WRITE_BYTE( iVictimId );
					WRITE_BYTE( 0 );
					WRITE_BYTE( iDuckTeam );
					WRITE_BYTE( iDuckCount );
					WRITE_BYTE( iDuckFlags );
					MessageEnd();
				}

				// To assister
				if ( pAssister )
				{
					// IsCreated, ID of Creator, ID of Victim, Count, IsGolden
					CSingleUserRecipientFilter userfilter( pAssister );
					UserMessageBegin( userfilter, "EOTLDuckEvent" );
					WRITE_BYTE( true );
					WRITE_BYTE( pAssister->entindex() );
					WRITE_BYTE( iVictimId );
					WRITE_BYTE( 0 );
					WRITE_BYTE( iDuckTeam );
					WRITE_BYTE( iDuckCount );
					WRITE_BYTE( iDuckFlags );
					MessageEnd();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static kill_eater_event_t g_eClassKillEvents[] =
{
	kKillEaterEvent_ScoutKill,					// TF_CLASS_SCOUT
	kKillEaterEvent_SniperKill,					// TF_CLASS_SNIPER
	kKillEaterEvent_SoldierKill,				// TF_CLASS_SOLDIER
	kKillEaterEvent_DemomanKill,				// TF_CLASS_DEMOMAN
	kKillEaterEvent_MedicKill,					// TF_CLASS_MEDIC
	kKillEaterEvent_HeavyKill,					// TF_CLASS_HEAVYWEAPONS
	kKillEaterEvent_PyroKill,					// TF_CLASS_PYRO
	kKillEaterEvent_SpyKill,					// TF_CLASS_SPY
	kKillEaterEvent_EngineerKill,				// TF_CLASS_ENGINEER
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_eClassKillEvents ) == (TF_LAST_NORMAL_CLASS - TF_FIRST_NORMAL_CLASS) );

//-----------------------------------------------------------------------------
static kill_eater_event_t g_eRobotClassKillEvents[] =
{
	kKillEaterEvent_RobotScoutKill,					// TF_CLASS_SCOUT
	kKillEaterEvent_RobotSniperKill,				// TF_CLASS_SNIPER
	kKillEaterEvent_RobotSoldierKill,				// TF_CLASS_SOLDIER
	kKillEaterEvent_RobotDemomanKill,				// TF_CLASS_DEMOMAN
	kKillEaterEvent_RobotMedicKill,					// TF_CLASS_MEDIC
	kKillEaterEvent_RobotHeavyKill,					// TF_CLASS_HEAVYWEAPONS
	kKillEaterEvent_RobotPyroKill,					// TF_CLASS_PYRO
	kKillEaterEvent_RobotSpyKill,					// TF_CLASS_SPY
	kKillEaterEvent_RobotEngineerKill,				// TF_CLASS_ENGINEER
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_eRobotClassKillEvents ) == (TF_LAST_NORMAL_CLASS - TF_FIRST_NORMAL_CLASS) );

static bool BHasWearableOfSpecificQualityEquipped( /*const*/ CTFPlayer *pTFPlayer, EEconItemQuality eQuality )
{
	Assert( pTFPlayer );

	// Fire the kill eater event on all wearables
	for ( int i = 0; i < pTFPlayer->GetNumWearables(); ++i )
	{
		CTFWearable *pWearableItem = dynamic_cast<CTFWearable *>( pTFPlayer->GetWearable( i ) );
		if ( !pWearableItem )
			continue;

		if ( !pWearableItem->GetAttributeContainer() )
			continue;

		CEconItemView *pEconItemView = pWearableItem->GetAttributeContainer()->GetItem();
		if ( !pEconItemView )
			continue;

		if ( pEconItemView->GetQuality() == eQuality )
			return true;
	}

	return false;
}

void CTFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer *pScorer = ToBaseMultiplayerPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFPlayer *pAssister = NULL;
	CBaseObject *pObject = NULL;

	// if inflictor or killer is a base object, tell them that they got a kill
	// ( depends if a sentry rocket got the kill, sentry may be inflictor or killer )
	if ( pInflictor )
	{
		if ( pInflictor->IsBaseObject() )
		{
			pObject = dynamic_cast<CBaseObject *>( pInflictor );
		}
		else 
		{
			CBaseEntity *pInflictorOwner = pInflictor->GetOwnerEntity();
			if ( pInflictorOwner && pInflictorOwner->IsBaseObject() )
			{
				pObject = dynamic_cast<CBaseObject *>( pInflictorOwner );
			}
		}
		
	}
	else if( pKiller && pKiller->IsBaseObject() )
	{
		pObject = dynamic_cast<CBaseObject *>( pKiller );
	}

	if ( pObject )
	{
		if ( pObject->GetBuilder() != pVictim )
		{
			pObject->IncrementKills();

			// minibosses count for 5 kills
			if ( IsMannVsMachineMode() && pVictim && pVictim->IsPlayer() )
			{
				CTFPlayer *playerVictim = ToTFPlayer( pVictim );
				if ( playerVictim->IsMiniBoss() )
				{
					pObject->IncrementKills();
					pObject->IncrementKills();
					pObject->IncrementKills();
					pObject->IncrementKills();
				}
			}
		}
		pInflictor = pObject;

		if ( pObject->ObjectType() == OBJ_SENTRYGUN )
		{
			// notify the sentry
			CObjectSentrygun *pSentrygun = dynamic_cast<CObjectSentrygun *>( pObject );
			if ( pSentrygun )
			{
				pSentrygun->OnKilledEnemy( pVictim );
			}

			CTFPlayer *pOwner = pObject->GetOwner();
			if ( pOwner )
			{
				int iKills = pObject->GetKills();

				// keep track of max kills per a single sentry gun in the player object
				if ( pOwner->GetMaxSentryKills() < iKills )
				{
					pOwner->SetMaxSentryKills( iKills );
				}

				// if we just got 10 kills with one sentry, tell the owner's client, which will award achievement if it doesn't have it already
				if ( iKills == 10 )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_GET_TURRETKILLS );
				}
			}
		}
	}

	// if not killed by suicide or killed by world, see if the scorer had an assister, and if so give the assister credit
	if ( ( pVictim != pScorer ) && pKiller )
	{
		pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );

		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pAssister && pTFVictim )
		{
			EntityHistory_t* entHist = pTFVictim->m_AchievementData.IsSentryDamagerInHistory( pAssister, 5.0 );
			if ( entHist )
			{
				CBaseObject *pObj = dynamic_cast<CBaseObject*>( entHist->hObject.Get() );
				if ( pObj )
				{
					pObj->IncrementAssists();
				}
			}

			// Rage On Assists
			// Sniper Kill Rage
			if ( pAssister->IsPlayerClass( TF_CLASS_SNIPER ) )
			{
				// Item attribute
				// Add Sniper Rage On Assists
				float flRageGain = 0;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pAssister, flRageGain, rage_on_assists );
				if (flRageGain != 0)
				{
					pAssister->m_Shared.ModifyRage(flRageGain);
				}
			}
		}
	}

	if ( pVictim ) 
	{
		if ( ShouldDropSpellPickup() )
		{
			DropSpellPickup( pVictim->GetAbsOrigin() );
		}

		CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFScorer && pTFVictim )
		{
			if ( ShouldDropBonusDuckFromPlayer( pTFScorer, pTFVictim ) )
			{
				DropBonusDuck( pTFVictim->GetAbsOrigin(), pTFScorer, pAssister, pTFVictim, ( info.GetDamageType() & DMG_CRITICAL ) != 0 );
			}
		}

		// Drop a halloween soul!
		if ( IsHolidayActive( kHoliday_Halloween ) )
		{
			CBaseCombatCharacter* pBaseCombatScorer = dynamic_cast< CBaseCombatCharacter*>( pScorer ? pScorer : pKiller );
			// No souls for a pure suicide
			if ( pTFVictim != pBaseCombatScorer )
			{
				// Only spawn a soul if the target is a base combat character. 
				if ( pTFVictim && pBaseCombatScorer )
				{
					DropHalloweenSoulPack( 1, pVictim->EyePosition(), pBaseCombatScorer, pTFVictim->GetTeamNumber() );
				}

				// Also spawn one for the assister
				if ( pAssister )
				{
					DropHalloweenSoulPack( 1, pVictim->EyePosition(), pAssister, pTFVictim->GetTeamNumber() );
				}
			}
		}
	}

	//find the area the player is in and see if his death causes a block
	CBaseMultiplayerPlayer *pMultiplayerPlayer = ToBaseMultiplayerPlayer(pVictim);
	for ( int i=0; i<ITriggerAreaCaptureAutoList::AutoList().Count(); ++i )
	{
		CTriggerAreaCapture *pArea = static_cast< CTriggerAreaCapture * >( ITriggerAreaCaptureAutoList::AutoList()[i] );
		if ( pArea->IsTouching( pMultiplayerPlayer ) )
		{
			// ACHIEVEMENT_TF_MEDIC_ASSIST_CAPTURER
			// kill 3 players 
			if ( pAssister && pAssister->IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				// the victim is touching a cap point, see if they own it
				if ( pMultiplayerPlayer->GetTeamNumber() == pArea->GetOwningTeam() )
				{
					int iDefenderKills = pAssister->GetPerLifeCounterKV( "medic_defender_kills" );

					if ( ++iDefenderKills == 3 )
					{
						pAssister->AwardAchievement( ACHIEVEMENT_TF_MEDIC_ASSIST_CAPTURER );
					}

					pAssister->SetPerLifeCounterKV( "medic_defender_kills", iDefenderKills );
				}
			}

			if ( pArea->CheckIfDeathCausesBlock( pMultiplayerPlayer, pScorer ) )
				break;
		}
	}

	// determine if this kill affected a nemesis relationship
	int iDeathFlags = 0;
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CTFPlayer *pTFPlayerScorer = ToTFPlayer( pScorer );
	if ( pScorer )
	{	
		CalcDominationAndRevenge( pTFPlayerScorer, info.GetWeapon(), pTFPlayerVictim, false, &iDeathFlags );
		if ( pAssister )
		{
			CalcDominationAndRevenge( pAssister, info.GetWeapon(), pTFPlayerVictim, true, &iDeathFlags );
		}
	}
	pTFPlayerVictim->SetDeathFlags( iDeathFlags );	

	CTFPlayer *pTFPlayerAssister = ToTFPlayer( pAssister );
	if ( pTFPlayerAssister )
	{
		CTF_GameStats.Event_AssistKill( pTFPlayerAssister, pVictim );
	}

	// check for achievements
	PlayerKilledCheckAchievements( pTFPlayerScorer, pTFPlayerVictim );

	if ( IsInTraining() && GetTrainingModeLogic() )
	{
		if ( pVictim->IsFakeClient() == false )
		{
			GetTrainingModeLogic()->OnPlayerDied( ToTFPlayer( pVictim ), pKiller );
		}
		else
		{
			GetTrainingModeLogic()->OnBotDied( ToTFPlayer( pVictim ), pKiller );
		}
	}

	// credit for dueling
	if ( pTFPlayerScorer != NULL && pTFPlayerScorer != pTFPlayerVictim )
	{
		DuelMiniGame_NotifyKill( pTFPlayerScorer, pTFPlayerVictim );
	}
	if ( pTFPlayerAssister && pTFPlayerAssister != pTFPlayerVictim )
	{
		DuelMiniGame_NotifyAssist( pTFPlayerAssister, pTFPlayerVictim );
	}

	// Count kills from powerup carriers to detect imbalances
	if ( IFuncPowerupVolumeAutoList::AutoList().Count() != 0 ) // If there are no func_powerupvolumes in the map, there's no point in checking for imbalances
	{
		if ( pTFPlayerScorer && pTFPlayerScorer != pTFPlayerVictim && pTFPlayerScorer->m_Shared.IsCarryingRune() && !pTFPlayerScorer->m_Shared.InCond( TF_COND_RUNE_IMBALANCE ) )
		{
			if ( !m_bPowerupImbalanceMeasuresRunning && !PowerupModeFlagStandoffActive() ) // Only count if imbalance measures aren't running and there is no flag standoff 
			{
				if ( pTFPlayerScorer->GetTeamNumber() == TF_TEAM_BLUE )
				{
					m_nPowerupKillsBlueTeam++;		// Blue team score increases if a powered up blue player makes a kill
				}
				else if ( pTFPlayerScorer->GetTeamNumber() == TF_TEAM_RED )
				{
					m_nPowerupKillsRedTeam++;
				}

				int nBlueAdvantage = m_nPowerupKillsBlueTeam - m_nPowerupKillsRedTeam;		// How far ahead is this team?
//				Msg( "\nnBlueAdvantage = %d\n", nBlueAdvantage );
				int nRedAdvantage = m_nPowerupKillsRedTeam - m_nPowerupKillsBlueTeam;
//				Msg( "nRedAdvantage = %d\n\n", nRedAdvantage );

				if ( nRedAdvantage >= tf_powerup_mode_imbalance_delta.GetInt() )
				{
					PowerupTeamImbalance( TF_TEAM_BLUE );		// Fire the output to map logic
				}
				else if ( nBlueAdvantage >= tf_powerup_mode_imbalance_delta.GetInt() )
				{
					PowerupTeamImbalance( TF_TEAM_RED );
				}
			}
		}
	}

	// credit for kill-eating weapons and anything else that might care
	if ( pTFPlayerScorer && pTFPlayerVictim && pTFPlayerScorer != pTFPlayerVictim )
	{
		// Increment the server-side kill count for this weapon -- this is used for honorbound
		// weapons and has nothing to do with strange weapons/stats.
		pTFPlayerScorer->IncrementKillCountSinceLastDeploy( info );

		CTFWeaponBase *pTFWeapon = GetKilleaterWeaponFromDamageInfo( &info );
		
		kill_eater_event_t eKillEaterEvent = pTFWeapon														// if we have a weapon...
											? pTFWeapon->GetKillEaterKillEventType()							// ...then ask it what type of event to report
											: info.GetDamageCustom() == TF_DMG_CUSTOM_BOOTS_STOMP			// if we don't have a weapon, and we're hacking for kill types from wearables (!)...
											? kKillEaterEvent_PlayerKillByBootStomp							// ...then use our hard-coded event
											: kKillEaterEvent_PlayerKill;									// otherwise default to a normal player kill
		
		CEconEntity *pAttackerEconWeapon = NULL;

		// Cosmetic Kill like manntreads or demo shield
		if ( !pTFWeapon )
		{
			pAttackerEconWeapon = dynamic_cast< CEconEntity * >( info.GetWeapon() );
		}
		else
		{
			pAttackerEconWeapon = dynamic_cast< CEconEntity * >( pTFWeapon );
		}
		

		if ( pAttackerEconWeapon )
		{
			if ( !( IsPVEModeActive() && pTFPlayerVictim->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
			{
				// Any type of non-robot kill!
				EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, eKillEaterEvent );

				// Cosmetic any kill type tracking
				{
					HatAndMiscEconEntities_OnOwnerKillEaterEvent( pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_CosmeticKills );

					// Halloween silliness: overworld kills for Merasmus carnival.
					if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_DOOMSDAY ) && !pTFPlayerScorer->m_Shared.InCond( TF_COND_HALLOWEEN_IN_HELL ) )
					{
						HatAndMiscEconEntities_OnOwnerKillEaterEvent( pTFPlayerScorer,
																	  pTFPlayerVictim,
																	  kKillEaterEvent_Halloween_OverworldKills );
					}
				}

				// Operation Kills
				EconEntity_NonEquippedItemKillTracking( pTFPlayerScorer, pTFPlayerVictim, 1 );

				// Unique kill tracking?
				// XXX(JohnS) - Disabling.  No longer collecting this on GC, never shipped code using this. See ECON_UNIQUE_EVENT_SUPPORT define
				// EconEntity_OnOwnerUniqueEconEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_UniqueEvent__KilledAccountWithItem );

				// Optional Taunt Kill tracking
				if ( IsTauntDmg( info.GetDamageCustom() ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_TauntKill );
				}
				// Scorch Shot Taunt is Different
				else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_FLARE_PELLET )
				{
					CBaseEntity *pInflictor = info.GetInflictor();
					CTFProjectile_Flare *pFlare = dynamic_cast<CTFProjectile_Flare*>(pInflictor);
					if ( pFlare && pFlare->IsFromTaunt() )
					{
						EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_TauntKill );
					}
				}

				// Optional: also track "killed X players of this specific class".
				int iVictimClassIndex = pTFPlayerVictim->GetPlayerClass()->GetClassIndex();
				if ( iVictimClassIndex >= TF_FIRST_NORMAL_CLASS && iVictimClassIndex <= TF_LAST_NORMAL_CLASS )
				{
					const kill_eater_event_t eClassKillType = g_eClassKillEvents[ iVictimClassIndex - TF_FIRST_NORMAL_CLASS ];
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, eClassKillType );
				}

				// Optional : Kills while in Victory / Bonus time
				if ( State_Get() == GR_STATE_TEAM_WIN && GetWinningTeam() != pTFPlayerVictim->GetTeamNumber() )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_VictoryTimeKill );
				}

				// Optional: also track "killed X players while they were in the air".
				if ( !(pTFPlayerVictim->GetFlags() & FL_ONGROUND) && (pTFPlayerVictim->GetWaterLevel() == WL_NotInWater) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_AirborneEnemyKill );
				}

				// Optional: also track "killed X players with headshots".
				if ( IsHeadshot( info.GetDamageCustom() ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_HeadshotKill );
				}

				// Optional: also track "gibbed X players".
				if ( pTFPlayerVictim->ShouldGib( info ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_GibKill );
				}

				// Optional: also track "killed X players during full moons". We intentionally don't call into the
				// game rules here because we don't want server variables to override this. Setting local time on the
				// server will still allow it to happen but it at least takes a little effort.
				if ( UTIL_IsHolidayActive( kHoliday_FullMoon ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_PlayerKillDuringFullMoon );
				}

				// Optional: also track kills we make while we're dead (afterburn, pre-death-fired rocket, etc.)
				if ( !pTFPlayerScorer->IsAlive() )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_PlayerKillPosthumous );
				}

				// Optional: also track kills that were specifically criticals.
				if ( (info.GetDamageType() & DMG_CRITICAL) != 0 )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_PlayerKillCritical );
				}
				else 
				{
					// Not special at all kill
					if ( pTFPlayerVictim->GetAttackBonusEffect() == kBonusEffect_None )
					{
						EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_NonCritKills );
					}
				}

				// Optional: also track kills that were made while we were launched into the air from an explosion (ie., rocket-jumping).
				if ( pTFPlayerScorer->InAirDueToExplosion() )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_PlayerKillWhileExplosiveJumping );
				}

				// Optional: also track kills where the victim was a spy who was invisible at the time of death.
				if ( iVictimClassIndex == TF_CLASS_SPY && pTFPlayerVictim->m_Shared.GetPercentInvisible() > 0 )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_InvisibleSpiesKilled );
				}

				// Optional: also track kills where the victim was a medic with 100% uber.
				if ( pTFPlayerVictim->MedicGetChargeLevel() >= 1.0f )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_MedicsWithFullUberKilled );
				}

				// Optional: also track kills where the killer was at low health when they dealt the final damage. Don't count
				// kills with 0 or fewer health -- those would be post-mortem kills instead.
				if ( ((float)pTFPlayerScorer->GetHealth() / (float)pTFPlayerScorer->GetMaxHealth()) <= 0.1f && pTFPlayerScorer->GetHealth() > 0 )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_KillWhileLowHealth );
				}

				// Optional: also track kills that take place during the Halloween holiday. We intentionally *do* check against
				// the game rules logic here -- if folks want to enable Halloween mode on a server and play around, I don't see any
				// reason why we benefit from stopping their fun.
				if ( TF_IsHolidayActive( kHoliday_Halloween ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_HalloweenKill );
				}

				// Optional: also track kills where the victim was completely underwater.
				if ( pTFPlayerVictim->GetWaterLevel() >= WL_Eyes )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_UnderwaterKill );
				}

				// Optional: also track kills where we're under the effects of a medic's ubercharge.
				if ( pTFPlayerScorer->m_Shared.InCond( TF_COND_INVULNERABLE ) || pTFPlayerScorer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_KillWhileUbercharged );
				}

				// Optional: also track kills where the victim was in the process of carrying the intel, capturing a point, or pushing
				// the cart. The actual logic for "killed a flag carrier" is handled elsewhere because by this point we've forgotten
				// if we had a flag before we died.
				if ( pTFPlayerVictim->IsCapturingPoint() )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_DefenderKill );
				}

				// Optional: also track kills where the victim was at least N units from the person dealing the damage, where N is "however far
				// you have to be to get the crowd chear noise". This also specifically checks to make sure the damage dealer is alive to avoid
				// casing where you spectate far away, etc.
				if ( pTFPlayerScorer->IsAlive() && (pTFPlayerScorer->GetAbsOrigin() - pTFPlayerVictim->GetAbsOrigin()).Length() >= 2000.0f )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_LongDistanceKill );
				}

				// Optional: also track kills where the victim was wearing at least one unusual-quality item.
				if ( BHasWearableOfSpecificQualityEquipped( pTFPlayerVictim, AE_UNUSUAL ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_PlayersWearingUnusualKill );
				}

				// Optional: also track kills where the victim was on fire at the time that they died. We can't check the condition flag
				// here because that may or may not be active after they die, but instead we test to see whether they still had a burn queued
				// up for the future as a proxy for "was on fire at this point".
				if ( pTFPlayerVictim->m_Shared.GetFlameBurnTime() >= gpGlobals->curtime )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_BurningEnemyKill );
				}

				// Optional: also track kills where this kill ended someone else's killstreak, where "killstreak" starts when the first
				// announcement happens. If Mike gets to hard-code constants, I do, too.
				enum { kFirstKillStreakAnnouncement = 5 };
				if ( pTFPlayerVictim->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Kills ) >= kFirstKillStreakAnnouncement )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_KillstreaksEnded );
				}

				// Optional: also track kills where the killer and the victim were basically right next to each other. This is hard-coded to
				// 1.5x default melee swing range.
				if ( pTFPlayerScorer->IsAlive() && (pTFPlayerScorer->GetAbsOrigin() - pTFPlayerVictim->GetAbsOrigin()).Length() <= 72.0f )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_PointBlankKills );
				}

				// Optional : Fullhealth kills
				if ( pTFPlayerScorer->GetHealth() >= pTFPlayerScorer->GetMaxHealth() )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_FullHealthKills );
				}

				// Optional : Taunting Player kills
				if ( pTFPlayerVictim->IsTaunting() )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_TauntingPlayerKills );
				}
			}
			else
			{
				Assert( pTFPlayerVictim->GetTeamNumber() == TF_TEAM_PVE_INVADERS );

				// Optional: also track kills where the victim was a robot in MvM
				EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_RobotsDestroyed );

				// Optional: also track "killed X Robots of this specific class".
				int iVictimClassIndex = pTFPlayerVictim->GetPlayerClass()->GetClassIndex();
				if ( iVictimClassIndex >= TF_FIRST_NORMAL_CLASS && iVictimClassIndex <= TF_LAST_NORMAL_CLASS )
				{
					const kill_eater_event_t eClassKillType = g_eRobotClassKillEvents[ iVictimClassIndex - TF_FIRST_NORMAL_CLASS ];
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, eClassKillType );
				}

				// Optional: specifically track miniboss kills separately from base robot kills.
				if ( pTFPlayerVictim->IsMiniBoss() )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_MinibossRobotsDestroyed );
				}

				// Optional: track whether this shot killed a robot *after* penetrating something else.
				if ( info.GetPlayerPenetrationCount() > 0 )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_RobotsDestroyedAfterPenetration );
				}

				// Optional: also track "killed X players with headshots", but for robots specifically.
				if ( IsHeadshot( info.GetDamageCustom() ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_RobotHeadshotKills );
				}

				// Optional: also track kills that take place during the Halloween holiday. See note for kKillEaterEvent_HalloweenKill
				// regarding calling into game rules here.
				if ( TF_IsHolidayActive( kHoliday_Halloween ) )
				{
					EconEntity_OnOwnerKillEaterEvent( pAttackerEconWeapon, pTFPlayerScorer, pTFPlayerVictim, kKillEaterEvent_HalloweenKillRobot );
				}
			}
		}
	}

	BaseClass::PlayerKilled( pVictim, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerKilledCheckAchievements( CTFPlayer *pAttacker, CTFPlayer *pVictim )
{
	if ( !pAttacker || !pVictim )
		return;

	// HEAVY WEAPONS GUY
	if ( pAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		if ( GetGameType() == TF_GAMETYPE_CP )
		{
			// ACHIEVEMENT_TF_HEAVY_DEFEND_CONTROL_POINT
			CTriggerAreaCapture *pAreaTrigger = pAttacker->GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP )
				{
					if ( pCP->GetOwner() == pAttacker->GetTeamNumber() )
					{
						// no suicides!
						if ( pAttacker != pVictim )
						{
							pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_DEFEND_CONTROL_POINT );
						}
					}
				}
			}

			// ACHIEVEMENT_TF_HEAVY_KILL_CAPPING_ENEMIES
			pAreaTrigger = pVictim->GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP )
				{
					if ( pCP->GetOwner() == pAttacker->GetTeamNumber() && 
						 TeamMayCapturePoint( pVictim->GetTeamNumber(), pCP->GetPointIndex() ) &&
						 PlayerMayCapturePoint( pVictim, pCP->GetPointIndex() ) )
					{
						pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_KILL_CAPPING_ENEMIES );
					}
				}
			}
		}

		// ACHIEVEMENT_TF_HEAVY_CLEAR_STICKYBOMBS
		if ( pVictim->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			int iPipes = pVictim->GetNumActivePipebombs();

			for (int i = 0; i < iPipes; i++)
			{
				pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_CLEAR_STICKYBOMBS );
			}
		}

		// ACHIEVEMENT_TF_HEAVY_DEFEND_MEDIC
		int i;
		int iNumHealers = pAttacker->m_Shared.GetNumHealers();
		for ( i = 0 ; i < iNumHealers ; i++ )
		{
			CTFPlayer *pMedic = ToTFPlayer( pAttacker->m_Shared.GetHealerByIndex( i ) );
			if ( pMedic && pMedic->m_AchievementData.IsDamagerInHistory( pVictim, 3.0 ) )
			{
				pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_DEFEND_MEDIC );
				break; // just award it once for each kill...even if the victim attacked more than one medic attached to the killer
			}
		}

		// ACHIEVEMENT_TF_HEAVY_STAND_NEAR_DISPENSER
		for ( i = 0 ; i < iNumHealers ; i++ )
		{
			if ( pAttacker->m_Shared.HealerIsDispenser( i ) )
			{
				pAttacker->AwardAchievement( ACHIEVEMENT_TF_HEAVY_STAND_NEAR_DISPENSER );
				break; // just award it once for each kill...even if the attacker is being healed by more than one dispenser
			}
		}
	}

	int i;
	int iNumHealers = pAttacker->m_Shared.GetNumHealers();
	for ( i = 0 ; i < iNumHealers ; i++ )
	{
		CTFPlayer *pMedic = ToTFPlayer( pAttacker->m_Shared.GetHealerByIndex( i ) );
		if ( pMedic && pMedic->m_AchievementData.IsDamagerInHistory( pVictim, 10.0 ) )
		{
			IGameEvent * event = gameeventmanager->CreateEvent( "medic_defended" );
			if ( event )
			{
				event->SetInt( "userid", pAttacker->GetUserID() );
				event->SetInt( "medic", pMedic->GetUserID() );
				gameeventmanager->FireEvent( event );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge
//-----------------------------------------------------------------------------
void CTFGameRules::CalcDominationAndRevenge( CTFPlayer *pAttacker, CBaseEntity *pWeapon, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
//=============================================================================
// HPE_BEGIN:
// [msmith]	If we're in training, we want to disable this domination stuff.
//=============================================================================
	if ( IsInTraining() )
		return;
//=============================================================================
// HPE_END
//=============================================================================

	// don't do domination stuff in powerup mode
	if ( IsPowerupMode() )
		return;

	// no dominations/revenge in competitive mode
	if ( IsMatchTypeCompetitive() )
		return;

	PlayerStats_t *pStatsVictim = CTF_GameStats.FindPlayerStats( pVictim );

	// no dominations/revenge in PvE mode
	if ( IsPVEModeActive() )
		return;

	CEconEntity *pEconWeapon = dynamic_cast<CEconEntity *>( pWeapon );
	
	int nAttackerEntIdx = pAttacker->entindex();
	if ( !IsIndexIntoPlayerArrayValid(nAttackerEntIdx) )
		return;

	// calculate # of unanswered kills between killer & victim - add 1 to include current kill
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[nAttackerEntIdx] + 1;		
	if ( TF_KILLS_DOMINATION == iKillsUnanswered )
	{			
		// this is the Nth unanswered kill between killer and victim, killer is now dominating victim
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_DOMINATION : TF_DEATH_DOMINATION );
		// set victim to be dominated by killer
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );

		int iCurrentlyDominated = pAttacker->GetNumberofDominations() + 1;
		pAttacker->SetNumberofDominations( iCurrentlyDominated );

		// record stats
		CTF_GameStats.Event_PlayerDominatedOther( pAttacker );

		// strange weapon stat tracking?
		EconEntity_OnOwnerKillEaterEvent( pEconWeapon, pAttacker, pVictim, kKillEaterEvent_PlayerKillStartDomination );

		IGameEvent *pDominationEvent = gameeventmanager->CreateEvent( "player_domination" );
		if ( pDominationEvent )
		{
			pDominationEvent->SetInt( "dominator", pAttacker->GetUserID() );
			pDominationEvent->SetInt( "dominated", pVictim->GetUserID() );
			pDominationEvent->SetInt( "dominations", iCurrentlyDominated );

			// Send the event
			gameeventmanager->FireEvent( pDominationEvent );
		}
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// the killer killed someone who was dominating him, gains revenge
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_REVENGE : TF_DEATH_REVENGE );
		// set victim to no longer be dominating the killer
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );

		int iCurrentlyDominated = pVictim->GetNumberofDominations() - 1;
		if (iCurrentlyDominated < 0)
		{
			iCurrentlyDominated = 0;
		}
		pVictim->SetNumberofDominations( iCurrentlyDominated );

		// record stats
		CTF_GameStats.Event_PlayerRevenge( pAttacker );

		// strange weapon stat tracking?
		EconEntity_OnOwnerKillEaterEvent( pEconWeapon, pAttacker, pVictim, kKillEaterEvent_PlayerKillRevenge );
	}
	else if ( pAttacker->m_Shared.IsPlayerDominated( pVictim->entindex() ) )
	{
		// the killer killed someone who they were already dominating

		// strange weapon stat tracking?
		EconEntity_OnOwnerKillEaterEvent( pEconWeapon, pAttacker, pVictim, kKillEaterEvent_PlayerKillAlreadyDominated );
	}
}

template< typename TIssue >
void NewTeamIssue()
{
	new TIssue( g_voteControllerRed );
	new TIssue( g_voteControllerBlu );
}

template< typename TIssue >
void NewGlobalIssue()
{
	new TIssue( g_voteControllerGlobal );
}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CTFGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource
	g_pObjectiveResource = (CTFObjectiveResource *)CBaseEntity::Create( "tf_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	// Create the monster resource for PvE battles
	g_pMonsterResource = (CMonsterResource *)CBaseEntity::Create( "monster_resource", vec3_origin, vec3_angle );

	MannVsMachineStats_Init();

	// Create the entity that will send our data to the client.
	m_hGamerulesProxy = assert_cast<CTFGameRulesProxy*>(CBaseEntity::Create( "tf_gamerules", vec3_origin, vec3_angle ));
	Assert( m_hGamerulesProxy.Get() );
	m_hGamerulesProxy->SetName( AllocPooledString("tf_gamerules" ) );
	
	g_voteControllerGlobal	=	static_cast< CVoteController *>( CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle ) );
	g_voteControllerRed		=	static_cast< CVoteController *>( CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle ) );
	g_voteControllerBlu		=	static_cast< CVoteController *>( CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle ) );

	// Vote Issue classes are handled/cleaned-up by g_voteControllers
	NewTeamIssue< CKickIssue >();
	NewGlobalIssue< CRestartGameIssue >();
	NewGlobalIssue< CChangeLevelIssue >();
	NewGlobalIssue< CNextLevelIssue >();
	NewGlobalIssue< CExtendLevelIssue >();
	NewGlobalIssue< CScrambleTeams >();
	NewGlobalIssue< CMannVsMachineChangeChallengeIssue >();
	NewGlobalIssue< CEnableTemporaryHalloweenIssue >();
	NewGlobalIssue< CTeamAutoBalanceIssue >();
	NewGlobalIssue< CClassLimitsIssue >();
	NewGlobalIssue< CPauseGameIssue >();
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, int *iWeaponID )
{
 	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );
	CTFPlayer *pTFKiller = ToTFPlayer( pKiller );

	const char *killer_weapon_name = "world";
	*iWeaponID = TF_WEAPON_NONE;

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "tf_weapon_flamethrower";
		*iWeaponID = TF_WEAPON_FLAMETHROWER;

		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( info.GetWeapon() );
		if ( pWeapon )
		{
			CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
			if ( pItem && pItem->GetStaticData() && pItem->GetStaticData()->GetIconClassname() )
			{
				killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
				*iWeaponID = TF_WEAPON_NONE;
			}
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING_FLARE )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "tf_weapon_flaregun";
		*iWeaponID = TF_WEAPON_FLAREGUN;

		if ( pInflictor && pInflictor->IsPlayer() == false )
		{
			CTFBaseRocket *pBaseRocket = dynamic_cast<CTFBaseRocket*>( pInflictor );

			if ( pBaseRocket )
			{
				if ( pBaseRocket->GetDeflected() )
				{
					killer_weapon_name = "deflect_flare";
				}
			}
		}

		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( info.GetWeapon() );
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAREGUN_REVENGE )
		{
			CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
			if ( pItem && pItem->GetStaticData() && pItem->GetStaticData()->GetIconClassname() )
			{
				killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
				*iWeaponID = pWeapon->GetWeaponID();
			}
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_FLARE_EXPLOSION )
	{
		killer_weapon_name = "tf_weapon_detonator";
		*iWeaponID = TF_WEAPON_FLAREGUN;

		if ( pInflictor && pInflictor->IsPlayer() == false )
		{
			CTFBaseRocket *pBaseRocket = dynamic_cast<CTFBaseRocket*>( pInflictor );
			if ( pBaseRocket )
			{
				if ( pBaseRocket->GetDeflected() )
				{
					killer_weapon_name = "deflect_flare_detonator";
				}
			}
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_CHARGE_IMPACT )
	{
		CTFWearable *pWearable = dynamic_cast< CTFWearable * >( info.GetWeapon() );
		if ( pWearable )
		{
			CEconItemView *pItem = pWearable->GetAttributeContainer()->GetItem();
			if ( pItem && pItem->GetStaticData() && pItem->GetStaticData()->GetIconClassname() )
			{
				killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
				*iWeaponID = TF_WEAPON_NONE;
			}
		}
	}
	else if ( info.GetPlayerPenetrationCount() > 0 )
	{
		// This may be stomped later if the kill was a headshot.
		killer_weapon_name = "player_penetration";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_PICKAXE )
	{
		killer_weapon_name = "pickaxe";
		*iWeaponID = TF_WEAPON_SHOVEL;
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_CARRIED_BUILDING )
	{
		killer_weapon_name = "tf_weapon_building_carried_destroyed";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_HADOUKEN )
	{
		killer_weapon_name = "tf_weapon_taunt_pyro";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON )
	{
		killer_weapon_name = "tf_weapon_taunt_heavy";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM )
	{
		killer_weapon_name = "tf_weapon_taunt_scout";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING )
	{
		killer_weapon_name = "tf_weapon_taunt_demoman";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_UBERSLICE )
	{
		killer_weapon_name = "tf_weapon_taunt_medic";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_FENCING )
	{
		killer_weapon_name = "tf_weapon_taunt_spy";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB )
	{
		killer_weapon_name = "tf_weapon_taunt_sniper";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_GRENADE )
	{
		if ( pTFKiller && pTFKiller->IsWormsGearEquipped() )
		{
			killer_weapon_name = "tf_weapon_taunt_soldier_lumbricus";
		}
		else
		{
			killer_weapon_name = "tf_weapon_taunt_soldier";
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH )
	{
		killer_weapon_name = "tf_weapon_taunt_guitar_kill";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF )
	{
		killer_weapon_name = "tf_weapon_taunt_guitar_riff_kill";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL )
	{
		killer_weapon_name = "robot_arm_blender_kill";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TELEFRAG )
	{
		killer_weapon_name = "telefrag";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_CROC )
	{
		const char *pszMap = gpGlobals->mapname.ToCStr();
		if ( FStrEq( pszMap, "koth_sharkbay" ) )
		{
			killer_weapon_name = "shark";
		}
		else if ( FStrEq( pszMap, "koth_cachoeira" ) )
		{
			killer_weapon_name = "piranha";
		}
		else
		{
			killer_weapon_name = "crocodile";
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BOOTS_STOMP )
	{
		killer_weapon_name = ( pTFKiller && pTFKiller->m_Shared.InCond( TF_COND_ROCKETPACK ) ) ? "rocketpack_stomp" : "mantreads";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BASEBALL )
	{
		killer_weapon_name = "ball";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_COMBO_PUNCH )
	{
		killer_weapon_name = "robot_arm_combo_kill";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BLEEDING )
	{
		killer_weapon_name = "tf_weapon_bleed_kill";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_PLAYER_SENTRY )
	{
		int nGigerCounter = 0; 
		if ( pScorer )
		{
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pScorer, nGigerCounter, is_giger_counter );
		}
			
		if ( nGigerCounter > 0 )
		{
			killer_weapon_name = "giger_counter";
		}
		else
		{
			killer_weapon_name = "wrangler_kill";
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_DECAPITATION_BOSS )
	{
		killer_weapon_name = "headtaker";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_EYEBALL_ROCKET )
	{
		killer_weapon_name = "eyeball_rocket";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_STICKBOMB_EXPLOSION )
	{
		killer_weapon_name = "ullapool_caber_explosion";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON )
	{
		killer_weapon_name = "armageddon";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_GASBLAST )
	{
		killer_weapon_name = "gas_blast";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH )
	{
		killer_weapon_name = "recorder";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_MERASMUS_DECAPITATION )
	{
		killer_weapon_name = "merasmus_decap";
	}	
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_MERASMUS_ZAP )
	{
		killer_weapon_name = "merasmus_zap";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_MERASMUS_GRENADE )
	{
		killer_weapon_name = "merasmus_grenade";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB )
	{
		killer_weapon_name = "merasmus_player_bomb";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_CANNONBALL_PUSH )
	{
		killer_weapon_name = "loose_cannon_impact";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_TELEPORT )
	{
		killer_weapon_name = "spellbook_teleport";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_SKELETON )
	{
		if ( FStrEq( gpGlobals->mapname.ToCStr(), "koth_slime" ) )
		{
			killer_weapon_name = "salmann";
		}
		else
		{
			killer_weapon_name = "spellbook_skeleton";
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_MIRV )
	{
		killer_weapon_name = "spellbook_mirv";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_METEOR )
	{
		killer_weapon_name = "spellbook_meteor";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_LIGHTNING )
	{
		killer_weapon_name = "spellbook_lightning";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_FIREBALL )
	{
		killer_weapon_name = "spellbook_fireball";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_MONOCULUS )
	{
		killer_weapon_name = "spellbook_boss";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_BLASTJUMP )
	{
		killer_weapon_name = "spellbook_blastjump";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_BATS )
	{
		killer_weapon_name = "spellbook_bats";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_SPELL_TINY )
	{
		killer_weapon_name = "spellbook_athletic";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING )
	{
		killer_weapon_name = "dragons_fury_bonus";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_KRAMPUS_MELEE )
	{
		killer_weapon_name = "krampus_melee";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_KRAMPUS_RANGED )
	{
		killer_weapon_name = "krampus_ranged";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_THROWABLE ||
			  info.GetDamageCustom() == TF_DMG_CUSTOM_THROWABLE_KILL )			// Throwables
	{
		if ( pVictim && pVictim->GetHealth() <= 0 )
		{
			killer_weapon_name = "water_balloon_kill";	
		}
		else
		{
			killer_weapon_name = "water_balloon_hit";
		}
		*iWeaponID = TF_WEAPON_THROWABLE;
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TRIGGER_HURT )
	{
		if ( FStrEq( gpGlobals->mapname.ToCStr(), "koth_cachoeira" ) )
		{
			killer_weapon_name = "helicopter";
		}
		else if ( FStrEq( gpGlobals->mapname.ToCStr(), "koth_megaton" ) )
		{
			killer_weapon_name = "megaton";
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TAUNTATK_TRICKSHOT )
	{
		killer_weapon_name = "tf_weapon_taunt_trickshot";
	}
	else if ( pScorer && pInflictor && ( pInflictor == pScorer ) )
	{
		// If this is not a suicide
		if ( pVictim != pScorer )
		{
			// If the inflictor is the killer, then it must be their current weapon doing the damage
			if ( pScorer->GetActiveWeapon() )
			{
				killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname(); 
				if ( pScorer->IsPlayer() )
				{
					*iWeaponID = ToTFPlayer(pScorer)->GetActiveTFWeapon()->GetWeaponID();
				}
			}
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );

		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pInflictor );
		if ( pWeapon )
		{
			*iWeaponID = pWeapon->GetWeaponID();
		}
		else
		{
			CTFBaseRocket *pBaseRocket = dynamic_cast<CTFBaseRocket*>( pInflictor );
			if ( pBaseRocket )
			{
				*iWeaponID = pBaseRocket->GetWeaponID();

				if ( pBaseRocket->GetDeflected() )
				{
					if ( *iWeaponID == TF_WEAPON_ROCKETLAUNCHER || *iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT )
					{
						killer_weapon_name = "deflect_rocket";
					}
					else if ( *iWeaponID == TF_WEAPON_COMPOUND_BOW )
					{
						CTFProjectile_Arrow* pArrow = dynamic_cast<CTFProjectile_Arrow*>( pBaseRocket );
						if ( pArrow && pArrow->IsAlight() )
						{
							killer_weapon_name = "deflect_huntsman_flyingburn";
						}
						else
						{
							killer_weapon_name = "deflect_arrow";
						}
					}
					else if ( *iWeaponID == TF_WEAPON_SHOTGUN_BUILDING_RESCUE )
					{
						killer_weapon_name = "rescue_ranger_reflect";
					}
				}
				else if ( *iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT )
				{
					killer_weapon_name = "rocketlauncher_directhit";
				}
			}
			else
			{
				CTFWeaponBaseGrenadeProj *pBaseGrenade = dynamic_cast<CTFWeaponBaseGrenadeProj*>( pInflictor );
				if ( pBaseGrenade )
				{
					*iWeaponID = pBaseGrenade->GetWeaponID();

					if ( pBaseGrenade->GetDeflected() )
					{
						if ( *iWeaponID == TF_WEAPON_GRENADE_PIPEBOMB )
						{
							killer_weapon_name = "deflect_sticky";
						}
						else if ( *iWeaponID == TF_WEAPON_GRENADE_DEMOMAN )
						{
							killer_weapon_name = "deflect_promode";
						}
						else if ( *iWeaponID == TF_WEAPON_BAT_WOOD )
						{
							killer_weapon_name = "deflect_ball";
						}
						else if ( *iWeaponID == TF_WEAPON_CANNON )
						{
							killer_weapon_name = "loose_cannon_reflect";
						}
					}
				}
			}
		}
	}

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_DEFENSIVE_STICKY )
	{
		killer_weapon_name = "sticky_resistance";
	}

	// strip certain prefixes from inflictor's classname
	const char *prefix[] = { "tf_weapon_grenade_", "tf_weapon_", "NPC_", "func_" };
	for ( int i = 0; i< ARRAYSIZE( prefix ); i++ )
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = Q_strlen( prefix[i] );
		if ( strncmp( killer_weapon_name, prefix[i], len ) == 0 )
		{
			killer_weapon_name += len;
			break;
		}
	}

	// look out for sentry rocket as weapon and map it to sentry gun, so we get the sentry death icon based off level

	if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) )
	{
		killer_weapon_name = "obj_sentrygun3";
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "obj_sentrygun" ) )
	{
		CObjectSentrygun *pSentrygun = assert_cast<CObjectSentrygun*>( pInflictor );
		if ( pSentrygun )
		{
			if ( pSentrygun->IsMiniBuilding() )
			{
				killer_weapon_name = "obj_minisentry";
			}
			else
			{
				int iSentryLevel = pSentrygun->GetUpgradeLevel();
				switch( iSentryLevel)
				{
				case 1:
					killer_weapon_name = "obj_sentrygun";
					break;
				case 2:
					killer_weapon_name = "obj_sentrygun2";
					break;
				case 3:
					killer_weapon_name = "obj_sentrygun3";
					break;
				default:
					killer_weapon_name = "obj_sentrygun";
					break;
				}
			}
		}
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_healing_bolt" ) )
	{
		killer_weapon_name = "crusaders_crossbow";
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_pipe" ) )
	{
		// let's look-up the primary weapon to see what type of grenade launcher it is
		if ( pScorer )
		{
			CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
			if ( pTFScorer )
			{
				CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pTFScorer->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY ) );
				if ( pWeapon && ( pWeapon->GetWeaponID() == TF_WEAPON_GRENADELAUNCHER ) && pWeapon->GetAttributeContainer() )
				{
					CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
					if ( pItem && pItem->GetStaticData() )
					{
						if ( pItem->GetStaticData()->GetIconClassname() )
						{
							killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
							*iWeaponID = TF_WEAPON_NONE;
						}
					}
				}
			}
		}
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_energy_ring" ) )
	{
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
		if ( pWeapon )
		{
			CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
			if ( pItem && pItem->GetStaticData() && pItem->GetStaticData()->GetIconClassname() )
			{
				killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
				*iWeaponID = TF_WEAPON_NONE;
			}
		}
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_balloffire" ) )
	{
		killer_weapon_name = "dragons_fury";
	}
	else if ( 0 == Q_strcmp( killer_weapon_name, "obj_attachment_sapper" ) )
	{
		// let's look-up the sapper weapon to see what type it is
		if ( pScorer )
		{
			CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
			if ( pTFScorer )
			{
				CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( pTFScorer->GetEntityForLoadoutSlot( LOADOUT_POSITION_BUILDING ) );

				if ( pWeapon && ( pWeapon->GetWeaponID() == TF_WEAPON_BUILDER ) && pWeapon->GetAttributeContainer() )
				{
					CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
					if ( pItem && pItem->GetStaticData() )
					{
						if ( pItem->GetStaticData()->GetIconClassname() )
						{
							killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
							*iWeaponID = TF_WEAPON_NONE;
						}
					}
				}
			}
		}
	}
	else if ( ( info.GetDamageCustom() == TF_DMG_CUSTOM_STANDARD_STICKY ) || ( info.GetDamageCustom() == TF_DMG_CUSTOM_AIR_STICKY_BURST ) )
	{
		// let's look-up the secondary weapon to see what type of sticky launcher it is
		if ( pScorer )
		{
			CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
			if ( pTFScorer )
			{
				CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pTFScorer->Weapon_GetWeaponByType( TF_WPN_TYPE_SECONDARY ) );
				if ( pWeapon && ( pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER ) && pWeapon->GetAttributeContainer() )
				{
					CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
					if ( pItem && pItem->GetStaticData() )
					{
						if ( pItem->GetStaticData()->GetIconClassname() )
						{
							killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
							*iWeaponID = TF_WEAPON_NONE;
						}
					}
				}
			}
		}
	}

	return killer_weapon_name;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor )
{
	CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
	CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
	if ( pTFScorer && pTFVictim )
	{
		// if victim killed himself, don't award an assist to anyone else, even if there was a recent damager
		if ( pTFScorer == pTFVictim )
			return NULL;

		// If an assist has been specified already, use it.
		if ( pTFVictim->m_Shared.GetAssist() )
		{
			return pTFVictim->m_Shared.GetAssist();
		}

		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( pTFScorer->m_Shared.GetFirstHealer() );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( TF_CLASS_MEDIC == pHealer->GetPlayerClass()->GetClassIndex() ) && ( NULL == dynamic_cast<CObjectSentrygun *>( pInflictor ) ) )
		{
			return pHealer;
		}

		// If we're under the effect of a condition that grants assists, give one to the player that buffed us
		CTFPlayer *pCondAssister = ToTFPlayer( pTFScorer->m_Shared.GetConditionAssistFromAttacker() );
		if ( pCondAssister )
			return pCondAssister;

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
		CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 1, TF_TIME_ASSIST_KILL );
		if ( pRecentDamager && ( pRecentDamager != pScorer ) )
			return pRecentDamager;

		// if a teammate has recently helped this sentry (ie: wrench hit), they assisted in the kill
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( pInflictor );
		if ( sentry )
		{
			CTFPlayer *pAssister = sentry->GetAssistingTeammate( TF_TIME_ASSIST_KILL );
			if ( pAssister )
				return pAssister;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specified recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CTFPlayer *CTFGameRules::GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed )
{
	EntityHistory_t *damagerHistory = pVictim->m_AchievementData.GetDamagerHistory( iDamager );
	if ( !damagerHistory )
		return NULL;

	if ( damagerHistory->hEntity && ( gpGlobals->curtime - damagerHistory->flTimeDamage <= flMaxElapsed ) )
	{
		CTFPlayer *pRecentDamager = ToTFPlayer( damagerHistory->hEntity );
		if ( pRecentDamager )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	if ( ( pKiller == pVictim ) && ( pKiller == pInflictor ) )
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window.  If so, award this as a kill to the damager.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 0, TF_TIME_SUICIDE_KILL_CREDIT );
			if ( pRecentDamager )
				return pRecentDamager;
		}
	}

	//Handle Pyro's Deflection credit.
	CTFWeaponBaseGrenadeProj *pBaseGrenade = dynamic_cast<CTFWeaponBaseGrenadeProj*>( pInflictor );

	if ( pBaseGrenade )
	{
		if ( pBaseGrenade->GetDeflected() )
		{
			if ( pBaseGrenade->GetWeaponID() == TF_WEAPON_GRENADE_PIPEBOMB )
			{
				CTFPlayer *pDeflectOwner = ToTFPlayer( pBaseGrenade->GetDeflectOwner() );

				if ( pDeflectOwner )
				{
					if ( pDeflectOwner->InSameTeam( pVictim ) == false )
						 return pDeflectOwner;
					else
					{
						pBaseGrenade->ResetDeflected();
						pBaseGrenade->SetDeflectOwner( NULL );
					}
				}
			}
		}
	}

	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	DeathNotice( pVictim, info, "player_death" );
}

void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info, const char* eventName )
{
	int killer_ID = 0;

	// Find the killer & the scorer
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFPlayer *pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );
	
	// You can't assist yourself!
	Assert( pScorer != pAssister || !pScorer );
	if ( pScorer == pAssister && pScorer )
	{
		pAssister = NULL;
	}

	// Silent killers cause no death notices.
	bool bSilentKill = false;
	CTFPlayer *pAttacker = (CTFPlayer*)ToTFPlayer( info.GetAttacker() );
	if ( pAttacker )
	{
		CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();
		if ( pWpn && pWpn->IsSilentKiller() && ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB ) )
			bSilentKill = true;
	}

	// Determine whether it's a feign death fake death notice
	bool bFeignDeath = pTFPlayerVictim->IsGoingFeignDeath();
	if ( bFeignDeath )
	{
		CTFPlayer *pDisguiseTarget = pTFPlayerVictim->m_Shared.GetDisguiseTarget();
		if ( pDisguiseTarget && (pTFPlayerVictim->GetTeamNumber() == pDisguiseTarget->GetTeamNumber()) )
		{
			// We're disguised as a team mate. Pretend to die as that player instead of us.
			pVictim = pTFPlayerVictim = pDisguiseTarget;
		}
	}

	// Work out what killed the player, and send a message to all clients about it
	int iWeaponID;
	const char *killer_weapon_name = GetKillingWeaponName( info, pTFPlayerVictim, &iWeaponID );
	const char *killer_weapon_log_name = killer_weapon_name;

	// Kill eater events.
	{
		// Was this a sentry kill? If the sentry did the kill itself with bullets then it'll be the inflictor.
		// If it got the kill by firing a rocket, the rocket will be the inflictor and the sentry will be the
		// owner of the rocket.
		//
		// dynamic_cast quagmire of sadness below.
		CObjectSentrygun *pSentrygun = dynamic_cast<CObjectSentrygun *>( pInflictor );
		if ( !pSentrygun )
		{
			pSentrygun = pInflictor ? dynamic_cast<CObjectSentrygun *>( pInflictor->GetOwnerEntity() ) : NULL;
		}

		if ( pSentrygun )
		{
			// Try to grab the wrench that the engineer has equipped right now. We destroy sentries when wrenches
			// get changed so whatever they have equipped right now counts for the wrench that built this sentry.
			CTFPlayer *pBuilder = pSentrygun->GetBuilder();
			if ( pBuilder )
			{
				EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pBuilder->GetEntityForLoadoutSlot( LOADOUT_POSITION_MELEE ) ), pScorer, pTFPlayerVictim, kKillEaterEvent_PlayerKillsBySentry );
				// PDA's Also count Sentry kills
				EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pBuilder->GetEntityForLoadoutSlot( LOADOUT_POSITION_PDA ) ), pScorer, pTFPlayerVictim, kKillEaterEvent_PlayerKillsBySentry );

				// Check if the engineer is using a Wrangler on this sentry
				CTFLaserPointer* pLaserPointer = dynamic_cast< CTFLaserPointer * >( pBuilder->GetEntityForLoadoutSlot( LOADOUT_POSITION_SECONDARY ) );
				if ( pLaserPointer && pLaserPointer->HasLaserDot() )
				{
					EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pLaserPointer ), pScorer, pTFPlayerVictim, kKillEaterEvent_PlayerKillsByManualControlOfSentry );
				}
			}
		}

		// Should we award an assist kill to someone?
		if ( pAssister )
		{
			EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pAssister->GetActiveWeapon() ), pAssister, pTFPlayerVictim, kKillEaterEvent_PlayerKillAssist );
			HatAndMiscEconEntities_OnOwnerKillEaterEvent( pAssister, pTFPlayerVictim, kKillEaterEvent_CosmeticAssists );
		}
	}

	if ( pScorer )	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
	}

	CTFWeaponBase *pScorerWeapon = NULL;
	if ( pScorer )
	{
		pScorerWeapon = dynamic_cast< CTFWeaponBase * >( pScorer->Weapon_OwnsThisID( iWeaponID ) );
		if ( pScorerWeapon )
		{
			CEconItemView *pItem = pScorerWeapon->GetAttributeContainer()->GetItem();

			if ( pItem )
			{
				if ( pItem->GetStaticData()->GetIconClassname() )
				{
					killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
				}
			
				if ( pItem->GetStaticData()->GetLogClassname() )
				{
					killer_weapon_log_name = pItem->GetStaticData()->GetLogClassname();
				}
			}
		}
	}

	// In Arena mode award first blood to the first player that kills an enemy.
	bool bKillWasFirstBlood = false;
	if ( IsFirstBloodAllowed() )
	{
		if ( pScorer && pVictim && pScorer != pVictim )
		{
			if ( !FStrEq( eventName, "fish_notice" ) && !FStrEq( eventName, "fish_notice__arm" ) && !FStrEq( eventName, "slap_notice" ) && !FStrEq( eventName, "throwable_hit" ) )
			{
#ifndef _DEBUG
				if ( GetGlobalTeam( pVictim->GetTeamNumber() ) && GetGlobalTeam( pVictim->GetTeamNumber() )->GetNumPlayers() > 1 )
#endif // !DEBUG
				{
					float flFastTime = IsCompetitiveMode() ? 120.f : TF_ARENA_MODE_FAST_FIRST_BLOOD_TIME;
					float flSlowTime = IsCompetitiveMode() ? 300.f : TF_ARENA_MODE_SLOW_FIRST_BLOOD_TIME;

					if ( ( gpGlobals->curtime - m_flRoundStartTime ) <= flFastTime )
					{
						BroadcastSound( 255, "Announcer.AM_FirstBloodFast" );
					}
					else if ( ( gpGlobals->curtime - m_flRoundStartTime ) >= flSlowTime )
					{
						BroadcastSound( 255, "Announcer.AM_FirstBloodFinally" );
					}
					else
					{
						BroadcastSound( 255, "Announcer.AM_FirstBloodRandom" );
					}

					m_bArenaFirstBlood = true;
					bKillWasFirstBlood = true;

					if ( IsCompetitiveMode() )
					{
// 						CTF_GameStats.Event_PlayerAwardBonusPoints( pScorer, pVictim, 10 );
// 						
// 						CUtlVector< CTFPlayer* > vecPlayers;
// 						CollectPlayers( &vecPlayers, pScorer->GetTeamNumber(), false );
// 						FOR_EACH_VEC( vecPlayers, i )
// 						{
// 							if ( !vecPlayers[i] )
// 								continue;
// 
// 							if ( vecPlayers[i] == pScorer )
// 								continue;
// 
// 							CTF_GameStats.Event_PlayerAwardBonusPoints( vecPlayers[i], pVictim, 5 );
// 						}
					}
					else
					{
						pScorer->m_Shared.AddCond( TF_COND_CRITBOOSTED_FIRST_BLOOD, TF_ARENA_MODE_FIRST_BLOOD_CRIT_TIME );
					}
				}
			}
		}
	}
	else
	{
		// so you can't turn on the ConVar in the middle of a round and get the first blood reward
		m_bArenaFirstBlood = true;
	}

	// Awesome hack for pyroland silliness: if there was no other assister, and the person that got
	// the kill has some sort of "pet" item (balloonicorn, brainslug, etc.), we send the name of
	// that item down as the assister. We'll use a custom name if available and fall back to the
	// localization token (localized on the client) if not.
	CUtlConstString sAssisterOverrideDesc;

	if ( pScorer && !pAssister )
	{
		// Find out whether the killer has at least one item that will ask for kill assist credit.
		int iKillerHasPetItem = 0;
		CUtlVector<CBaseEntity *> vecItems;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER_WITH_ITEMS( pScorer, iKillerHasPetItem, &vecItems, counts_as_assister );

		FOR_EACH_VEC( vecItems, i )
		{
			CEconEntity *pEconEntity = dynamic_cast<CEconEntity *>( vecItems[i] );
			if ( !pEconEntity )
				continue;

			CEconItemView *pEconItemView = pEconEntity->GetAttributeContainer()->GetItem();
			if ( !pEconItemView )
				continue;

			if ( pEconItemView->GetCustomName() )
			{
				sAssisterOverrideDesc = CFmtStr( "%c%s", iKillerHasPetItem == 2 ? kHorriblePyroVisionHack_KillAssisterType_CustomName_First : kHorriblePyroVisionHack_KillAssisterType_CustomName, pEconItemView->GetCustomName() );
			}
			else
			{
				sAssisterOverrideDesc = CFmtStr( "%c%s", iKillerHasPetItem == 2 ? kHorriblePyroVisionHack_KillAssisterType_LocalizationString_First : kHorriblePyroVisionHack_KillAssisterType_LocalizationString, pEconItemView->GetItemDefinition()->GetItemBaseName() ).Get();
			}
			break;
		}
	}

	IGameEvent * event = gameeventmanager->CreateEvent( eventName /* "player_death" */ );

	//if ( event && FStrEq( eventName, "throwable_hit" ) )
	//{
	//	int iHitCount = 1;
	//	PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pScorer );
	//	if ( pStats )
	//	{
	//		iHitCount = pStats->statsAccumulated.Get( TFSTAT_THROWABLEHIT );
	//	}

	//	event->SetInt( "totalhits", iHitCount );
	//}

	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "victim_entindex", pVictim->entindex() );
		event->SetInt( "attacker", killer_ID );
		event->SetInt( "assister", pAssister ? pAssister->GetUserID() : -1 );
		event->SetString( "weapon", killer_weapon_name );
		event->SetString( "weapon_logclassname", killer_weapon_log_name );
		event->SetInt( "weaponid", iWeaponID );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );
		event->SetInt( "inflictor_entindex", pInflictor ? pInflictor->entindex() : -1 );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted

		if ( info.GetPlayerPenetrationCount() > 0 )
		{
			event->SetInt( "playerpenetratecount", info.GetPlayerPenetrationCount() );
		}

		if ( !sAssisterOverrideDesc.IsEmpty() )
		{
			event->SetString( "assister_fallback", sAssisterOverrideDesc.Get() );
		}

		event->SetBool( "silent_kill", bSilentKill );

		int iDeathFlags = pTFPlayerVictim->GetDeathFlags();

		if ( bKillWasFirstBlood )
		{
			iDeathFlags |= TF_DEATH_FIRST_BLOOD;
		}

		if ( bFeignDeath )
		{
			iDeathFlags |= TF_DEATH_FEIGN_DEATH;
		}

		if ( pTFPlayerVictim->WasGibbedOnLastDeath() )
		{
			iDeathFlags |= TF_DEATH_GIBBED;
		}

		if ( pTFPlayerVictim->IsInPurgatory() )
		{
			iDeathFlags |= TF_DEATH_PURGATORY;
		}

		if ( pTFPlayerVictim->IsMiniBoss() )
		{
			iDeathFlags |= TF_DEATH_MINIBOSS;
		}

		// Australium Guns get a Gold Background
		IHasAttributes *pAttribInterface = GetAttribInterface( info.GetWeapon() );
		if ( pAttribInterface )
		{
			int iIsAustralium = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iIsAustralium, is_australium_item );
			if ( iIsAustralium )
			{
				iDeathFlags |= TF_DEATH_AUSTRALIUM;
			}

			int iIsGoldenWeapon = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iIsGoldenWeapon, set_turn_to_gold );
			if ( iIsGoldenWeapon )
			{
				iDeathFlags |= TF_DEATH_AUSTRALIUM;
			}
		}

		// We call this directly since we need more information than provided in the event alone.
		if ( FStrEq( eventName, "player_death" ) )
		{
			CTF_GameStats.Event_KillDetail( pScorer, pTFPlayerVictim, pAssister, event, info );
			event->SetInt( "kill_streak_victim", pTFPlayerVictim->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Kills ) );
			event->SetBool( "rocket_jump", ( pTFPlayerVictim->RocketJumped() == 1 ) );
			event->SetInt( "crit_type", info.GetCritType() );

			// Kill streak updating
			if ( pTFPlayerVictim && pScorer && pTFPlayerVictim != pScorer )
			{
				// Propagate duckstreaks
				event->SetInt( "duck_streak_victim", pTFPlayerVictim->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Ducks ) );
				event->SetInt( "duck_streak_total", pScorer->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Ducks ) );
				event->SetInt( "ducks_streaked", pScorer->m_Shared.GetLastDuckStreakIncrement() );

				// Check if they have the appropriate attribute.
				int iKillStreak = 0;
				int iKills = 0;
				CBaseEntity *pKillStreakTarget = NULL;
				if ( !pAttribInterface )
				{
					// Check if you are a sentry and if so, use the wrench
					// For Sentries Inflictor can be the sentry (bullets) or the Sentry Rocket

					CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pInflictor );
					if ( !pSentry && pInflictor )
					{
						pSentry = dynamic_cast<CObjectSentrygun*>( pInflictor->GetOwnerEntity() );
					}

					if ( pSentry )
					{
						pKillStreakTarget = dynamic_cast<CTFWeaponBase*>( pScorer->GetEntityForLoadoutSlot( LOADOUT_POSITION_MELEE ) );
					}
				}
				else
				{
					pKillStreakTarget = info.GetWeapon();
				}

				if ( pKillStreakTarget )
				{
					CALL_ATTRIB_HOOK_INT_ON_OTHER( pKillStreakTarget, iKillStreak, killstreak_tier );
					// Always track killstreak regardless of the attribute for data collection purposes
					pScorer->m_Shared.IncrementStreak( CTFPlayerShared::kTFStreak_KillsAll, 1 );
					if ( iKillStreak )
					{
						iKills = pScorer->m_Shared.IncrementStreak( CTFPlayerShared::kTFStreak_Kills, 1 );
						event->SetInt( "kill_streak_total", iKills );

						int iWepKills = 0;
						CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>( pKillStreakTarget );
						if ( pWeapon )
						{
							iWepKills = pWeapon->GetKillStreak() + 1;
							pWeapon->SetKillStreak( iWepKills );
						}
						else
						{
							CTFWearable *pWearable = dynamic_cast<CTFWearable*>( pKillStreakTarget );
							if ( pWearable )
							{
								iWepKills = pWearable->GetKillStreak() + 1;
								pWearable->SetKillStreak( iWepKills );
							}
						}

						event->SetInt( "kill_streak_wep", iWepKills );

						// Track each player's max streak per-round
						CTF_GameStats.Event_PlayerEarnedKillStreak( pScorer );
					}
				}

				if ( pAssister )
				{
					event->SetInt( "duck_streak_assist", pAssister->m_Shared.GetStreak( CTFPlayerShared::kTFStreak_Ducks ) );

					// Only allow assists for Mediguns
					CTFWeaponBase *pAssisterWpn = pAssister->GetActiveTFWeapon();
					if ( pAssisterWpn && pAssister->IsPlayerClass( TF_CLASS_MEDIC ) )
					{
						CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>( pAssisterWpn );
						if ( pMedigun )
						{
							iKillStreak = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER( pAssisterWpn, iKillStreak, killstreak_tier );
							if ( iKillStreak )
							{
								iKills = pAssister->m_Shared.IncrementStreak( CTFPlayerShared::kTFStreak_Kills, 1 );
								event->SetInt( "kill_streak_assist", iKills );

								int iWepKills = pAssisterWpn->GetKillStreak() + 1;
								pAssisterWpn->SetKillStreak( iWepKills );

								// Track each player's max streak per-round
								CTF_GameStats.Event_PlayerEarnedKillStreak( pAssister );
							}
						}
					}
				}
			}
		}

		event->SetInt( "death_flags", iDeathFlags );	
		event->SetInt( "stun_flags", pTFPlayerVictim->m_iOldStunFlags );

		item_definition_index_t weaponDefIndex = INVALID_ITEM_DEF_INDEX;
		if ( pScorerWeapon )
		{
			CEconItemView *pItem = pScorerWeapon->GetAttributeContainer()->GetItem();
			if ( pItem )
			{
				weaponDefIndex = pItem->GetItemDefIndex();
			}
		}
		else if ( pScorer && pScorer->GetActiveTFWeapon() )
		{
			// get from active weapon instead
			CEconItemView *pItem = pScorer->GetActiveTFWeapon()->GetAttributeContainer()->GetItem();
			if ( pItem )
			{
				weaponDefIndex = pItem->GetItemDefIndex();
			}
		}
		event->SetInt( "weapon_def_index", weaponDefIndex );

		pTFPlayerVictim->m_iOldStunFlags = 0;

		gameeventmanager->FireEvent( event );
	}	
}

void CTFGameRules::ClientDisconnected( edict_t *pClient )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetContainingEntity( pClient ) );
	if ( pPlayer )
	{
		// ACHIEVEMENT_TF_PYRO_DOMINATE_LEAVESVR - Pyro causes a dominated player to leave the server
		for ( int i = 1; i <= gpGlobals->maxClients ; i++ )
		{
			if ( pPlayer->m_Shared.IsPlayerDominatingMe(i) )
			{
				CTFPlayer *pDominatingPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( pDominatingPlayer && pDominatingPlayer != pPlayer )
				{
					if ( pDominatingPlayer->IsPlayerClass(TF_CLASS_PYRO) )
					{
						pDominatingPlayer->AwardAchievement( ACHIEVEMENT_TF_PYRO_DOMINATE_LEAVESVR );
					}
				}
			}
		}

		CTFPlayerResource *pTFResource = dynamic_cast< CTFPlayerResource* >( g_pPlayerResource );		
		if ( pTFResource )
		{
			if ( pPlayer->entindex() == pTFResource->GetPartyLeaderIndex( pPlayer->GetTeamNumber() ) )
			{
				// the leader is leaving so reset the player resource index
				pTFResource->SetPartyLeaderIndex( pPlayer->GetTeamNumber(), 0 ); 
			}
		}

		// Notify gamestats that the player left.
		CTF_GameStats.Event_PlayerDisconnectedTF( pPlayer );

		// Check Ready status for the player
		if ( UsePlayerReadyStatusMode() )
		{
			if ( !pPlayer->IsBot() && State_Get() != GR_STATE_RND_RUNNING )
			{
				const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
				if ( !pMatchDesc || !pMatchDesc->BUsesAutoReady() )
				{
					// Always reset when a player leaves this type of match if it isn't MvM
					if ( !IsMannVsMachineMode() )
					{
						PlayerReadyStatus_ResetState();
					}
				}
				else if ( !IsTeamReady( pPlayer->GetTeamNumber() ) )
				{
					if ( IsPlayerReady( pPlayer->entindex() ) )
					{
						// Clear the ready status so it doesn't block the rest of the team
						PlayerReadyStatus_UpdatePlayerState( pPlayer, false );
					}
					else
					{
						// Disconnecting player wasn't ready, but is the rest of the team?
						// If so, we want to cancel the ready_state so it doesn't start right away when this player disconnects
						bool bEveryoneReady = true;
						CUtlVector<CTFPlayer *> playerVector;
						CollectPlayers( &playerVector, pPlayer->GetTeamNumber() );
						FOR_EACH_VEC( playerVector, i )
						{
							if ( !playerVector[i] )
								continue;

							if ( playerVector[i]->IsBot() )
								continue;

							if ( playerVector[i] == pPlayer )
								continue;

							if ( !IsPlayerReady( playerVector[i]->entindex() ) )
							{
								bEveryoneReady = false;
							}
						}

						if ( bEveryoneReady )
						{
							PlayerReadyStatus_ResetState();
						}
					}
				}
				else
				{
					// If we're currently in a countdown we should cancel it
					if ( ( m_flRestartRoundTime > 0 ) || ( mp_restartgame.GetInt() > 0 ) )
					{
						PlayerReadyStatus_ResetState();
					}
				}
			}
		}

		// clean up anything they left behind
		pPlayer->TeamFortress_ClientDisconnected();
		Arena_ClientDisconnect( pPlayer->GetPlayerName() );
	}

	BaseClass::ClientDisconnected( pClient );

	// are any of the spies disguising as this player?
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != pPlayer )
		{
			if ( pTemp->m_Shared.GetDisguiseTarget() == pPlayer )
			{
				// choose someone else...
				pTemp->m_Shared.FindDisguiseTarget();
			}
		}
	}
}

// Falling damage stuff.
#define TF_PLAYER_MAX_SAFE_FALL_SPEED	650		

ConVar tf_fall_damage_disablespread( "tf_fall_damage_disablespread", "0", FCVAR_NONE );

float CTFGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return 0;

	// Karts don't take fall damage
	if ( pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		return 0;
	}

	// grappling hook don't take fall damage
	if ( pTFPlayer->GetGrapplingHookTarget() )
	{
		return 0;
	}

	if ( pTFPlayer->m_Shared.GetCarryingRuneType() == RUNE_AGILITY || ( IsPowerupMode() && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT ) )
	{
		return 0;
	}

	if ( pPlayer->m_Local.m_flFallVelocity > TF_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		// Old TFC damage formula
		float flFallDamage = 5 * (pPlayer->m_Local.m_flFallVelocity / 300);

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;

		if ( !tf_fall_damage_disablespread.GetBool() )
			flFallDamage *= random->RandomFloat( 0.8, 1.2 );

		int iCancelFallingDamage = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iCancelFallingDamage, cancel_falling_damage );
		if ( iCancelFallingDamage > 0 )
			flFallDamage = 0;

		return flFallDamage;
	}

	// Fall caused no damage
	return 0;
}

void CTFGameRules::SendArenaWinPanelInfo( void )
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "arena_win_panel" );

	if ( winEvent )
	{
		int iBlueScore = GetGlobalTeam( TF_TEAM_BLUE )->GetScore();
		int iRedScore = GetGlobalTeam( TF_TEAM_RED )->GetScore();
		int iBlueScorePrev = iBlueScore;
		int iRedScorePrev = iRedScore;

		// if this is a complete round, calc team scores prior to this win
		switch ( m_iWinningTeam )
		{
		case TF_TEAM_BLUE:
			{
				iBlueScorePrev = ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;

				if ( IsInTournamentMode() == false )
				{
					iRedScore = 0;
				}
			}
			break;

		case TF_TEAM_RED:
			{
				iRedScorePrev = ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;

				if ( IsInTournamentMode() == false )
				{
					iBlueScore = 0;
				}

				break;
			}
		case TEAM_UNASSIGNED:
			break;	// stalemate; nothing to do
		}

		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers",  ( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ? m_szMostRecentCappers : "" );
		winEvent->SetInt( "blue_score", iBlueScore );
		winEvent->SetInt( "red_score", iRedScore );
		winEvent->SetInt( "blue_score_prev", iBlueScorePrev );
		winEvent->SetInt( "red_score_prev", iRedScorePrev );
		
		CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );
		if ( !pResource )
			return;

		// build a vector of players & round scores
		CUtlVector<PlayerArenaRoundScore_t> vecPlayerScore;
		int iPlayerIndex;
		for( iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;
			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( ( iPlayerTeam < FIRST_GAME_TEAM ) )
				continue;

			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			PlayerArenaRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];

			playerRoundScore.iPlayerIndex = iPlayerIndex;

			if ( pStats )
			{
				playerRoundScore.iTotalDamage = pStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE];
				playerRoundScore.iTotalHealing = pStats->statsCurrentRound.m_iStat[TFSTAT_HEALING];

				if ( pTFPlayer->IsAlive() == true )
				{
					playerRoundScore.iTimeAlive = (int)gpGlobals->curtime - pTFPlayer->GetSpawnTime();
				}
				else
				{
					playerRoundScore.iTimeAlive = (int)pTFPlayer->GetDeathTime() - pTFPlayer->GetSpawnTime();
				}
					
				playerRoundScore.iKillingBlows = pStats->statsCurrentRound.m_iStat[TFSTAT_KILLS];
				playerRoundScore.iScore = CalcPlayerScore( &pStats->statsCurrentRound, pTFPlayer );
			}
		}

		// sort the players by round score
		vecPlayerScore.Sort( PlayerArenaRoundScoreSortFunc );

		// set the top (up to) 6 players by round score in the event data
		int numPlayers = 6;
		int iPlayersAdded = 0;

		// Add winners first
		for ( int i = 0; i < vecPlayerScore.Count(); i++ )
		{
			if (  GetWinningTeam() == TEAM_UNASSIGNED )
			{
				if ( iPlayersAdded >= 6 )
					break;
			}
			else
			{
				if ( iPlayersAdded >= 3 )
					break;
			}

			int iPlayerIndex = vecPlayerScore[i].iPlayerIndex;

			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );

			if ( pTFPlayer && pTFPlayer->GetTeamNumber() != GetWinningTeam() && GetWinningTeam() != TEAM_UNASSIGNED )
				continue;

			winEvent->SetInt( UTIL_VarArgs( "player_%d", iPlayersAdded + 1 ), vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( UTIL_VarArgs( "player_%d_damage", iPlayersAdded + 1 ), vecPlayerScore[i].iTotalDamage );				
			winEvent->SetInt( UTIL_VarArgs( "player_%d_healing", iPlayersAdded + 1 ), vecPlayerScore[i].iTotalHealing );
			winEvent->SetInt( UTIL_VarArgs( "player_%d_lifetime", iPlayersAdded + 1 ), vecPlayerScore[i].iTimeAlive );
			winEvent->SetInt( UTIL_VarArgs( "player_%d_kills", iPlayersAdded + 1 ), vecPlayerScore[i].iKillingBlows );

			iPlayersAdded++;
		}

		if ( GetWinningTeam() != TEAM_UNASSIGNED )
		{
			//Now add the rest
			iPlayersAdded = 3;

			for ( int i = 0; i < vecPlayerScore.Count(); i++ )
			{
				if ( iPlayersAdded >= numPlayers )
					break;

				int iIndex = iPlayersAdded + 1;

				int iPlayerIndex = vecPlayerScore[i].iPlayerIndex;

				CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );

				if ( pTFPlayer && pTFPlayer->GetTeamNumber() == GetWinningTeam() )
					continue;

				winEvent->SetInt( UTIL_VarArgs( "player_%d", iIndex ), vecPlayerScore[i].iPlayerIndex );
				winEvent->SetInt( UTIL_VarArgs( "player_%d_damage", iIndex ), vecPlayerScore[i].iTotalDamage );				
				winEvent->SetInt( UTIL_VarArgs( "player_%d_healing", iIndex ), vecPlayerScore[i].iTotalHealing );
				winEvent->SetInt( UTIL_VarArgs( "player_%d_lifetime", iIndex ), vecPlayerScore[i].iTimeAlive );
				winEvent->SetInt( UTIL_VarArgs( "player_%d_kills", iIndex ), vecPlayerScore[i].iKillingBlows );

				iPlayersAdded++;
			}
		}

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

void CTFGameRules::SendPVEWinPanelInfo( void )
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "pve_win_panel" );

	if ( winEvent )
	{
		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", 0 );

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}

	/*CBroadcastRecipientFilter filter;
	filter.MakeReliable();
	UserMessageBegin( filter, "MVMAnnouncement" );
		WRITE_CHAR( TF_MVM_ANNOUNCEMENT_WAVE_FAILED );
		WRITE_CHAR( -1 );
	MessageEnd();*/
}

void CTFGameRules::SendWinPanelInfo( bool bGameOver )
{
	if ( IsInArenaMode() == true )
	{
		SendArenaWinPanelInfo();
		return;
	}

	if ( IsPVEModeActive() )
	{
		SendPVEWinPanelInfo();
		return;
	}

	IGameEvent *winEvent = gameeventmanager->CreateEvent( "teamplay_win_panel" );

	if ( winEvent )
	{
		int iBlueScore = GetGlobalTeam( TF_TEAM_BLUE )->GetScore();
		int iRedScore = GetGlobalTeam( TF_TEAM_RED )->GetScore();
		int iBlueScorePrev = iBlueScore;
		int iRedScorePrev = iRedScore;

		bool bRoundComplete = m_bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		bool bScoringPerCapture = ( pMaster ) ? ( pMaster->ShouldScorePerCapture() ) : false;

		if ( m_nGameType == TF_GAMETYPE_CTF )
		{
			if ( tf_flag_caps_per_round.GetInt() == 0 )
			{
				bScoringPerCapture = true;
			}
		}

		if ( bRoundComplete && !bScoringPerCapture && m_bUseAddScoreAnim )
		{
			// if this is a complete round, calc team scores prior to this win
			switch ( m_iWinningTeam )
			{
			case TF_TEAM_BLUE:
				iBlueScorePrev = ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TF_TEAM_RED:
				iRedScorePrev = ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TEAM_UNASSIGNED:
				break;	// stalemate; nothing to do
			}
		}
				
		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers",  ( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ?
							 m_szMostRecentCappers : "" );
		winEvent->SetInt( "flagcaplimit", IsPasstimeMode() ? tf_passtime_scores_per_round.GetInt() : tf_flag_caps_per_round.GetInt() );
		winEvent->SetInt( "blue_score", iBlueScore );
		winEvent->SetInt( "red_score", iRedScore );
		winEvent->SetInt( "blue_score_prev", iBlueScorePrev );
		winEvent->SetInt( "red_score_prev", iRedScorePrev );
		winEvent->SetInt( "round_complete", bRoundComplete );

		CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );
		if ( !pResource )
			return;

		// Highest killstreak
		int nMaxStreakPlayerIndex = 0;
		int nMaxStreakCount = 0;
	 
		// determine the 3 players on winning team who scored the most points this round

		// build a vector of players & round scores
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;
		int iPlayerIndex;
		for( iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;
			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( ( iPlayerTeam < FIRST_GAME_TEAM ) || ( m_iWinningTeam != TEAM_UNASSIGNED && ( m_iWinningTeam != iPlayerTeam ) ) )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pStats )
			{
				iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound, pTFPlayer );
				iTotalScore = CalcPlayerScore( &pStats->statsAccumulated, pTFPlayer );
			}

			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];

			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iTotalScore = iTotalScore;

			// Highest killstreak?
			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pPlayerStats ) 
			{
				int nMax = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_KILLSTREAK_MAX];
				if ( nMax > nMaxStreakCount )
				{
					nMaxStreakCount = nMax;
					nMaxStreakPlayerIndex = iPlayerIndex;
				}
			}
		}

		// sort the players by round score
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// set the top (up to) 6 players by round score in the event data
		int numPlayers = MIN( 6, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// set the player index and their round score in the event
			char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i+ 1 );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i+ 1 );
			winEvent->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );				
		}

		winEvent->SetInt( "killstreak_player_1", nMaxStreakPlayerIndex );
		winEvent->SetInt( "killstreak_player_1_count", nMaxStreakCount );

#ifdef TF_RAID_MODE
		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) && !IsRaidMode() )
#else
		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) )
#endif // TF_RAID_MODE
		{
			// is this our new payload race game mode?
			if ( ( m_nGameType == TF_GAMETYPE_ESCORT ) && ( m_bMultipleTrains == true ) )
			{
				if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] && g_hControlPointMasters[0]->PlayingMiniRounds() )
				{
					int nRoundsRemaining = g_hControlPointMasters[0]->NumPlayableControlPointRounds();
					if ( nRoundsRemaining > 0 )
					{
						winEvent->SetInt( "rounds_remaining", nRoundsRemaining );
					}
				}
			}
			else
			{
				// if this was not a full round ending, include how many mini-rounds remain for winning team to win
				if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
				{
					winEvent->SetInt( "rounds_remaining", g_hControlPointMasters[0]->CalcNumRoundsRemaining( m_iWinningTeam ) );
				}
			}
		}

		winEvent->SetBool( "game_over", bGameOver );

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// sort first by round score	
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// if round scores are the same, sort next by total score
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by arena stats
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerArenaRoundScoreSortFunc( const PlayerArenaRoundScore_t *pRoundScore1, const PlayerArenaRoundScore_t *pRoundScore2 )
{
	//Compare Total points first
	//This gives us a rough estimate of performance
	if ( pRoundScore1->iScore != pRoundScore2->iScore )
		return pRoundScore2->iScore - pRoundScore1->iScore;

	//Compare healing
	if ( pRoundScore1->iTotalHealing != pRoundScore2->iTotalHealing )
		return pRoundScore2->iTotalHealing - pRoundScore1->iTotalHealing;

	//Compare damage
	if ( pRoundScore1->iTotalDamage != pRoundScore2->iTotalDamage )
		return pRoundScore2->iTotalDamage - pRoundScore1->iTotalDamage;

	//Compare time alive
	if ( pRoundScore1->iTimeAlive != pRoundScore2->iTimeAlive )
		return pRoundScore2->iTimeAlive - pRoundScore1->iTimeAlive;

	//Compare killing blows
	if ( pRoundScore1->iKillingBlows != pRoundScore2->iKillingBlows )
		return pRoundScore2->iKillingBlows - pRoundScore1->iKillingBlows;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the teamplay_round_win event is about to be sent, gives
//			this method a chance to add more data to it
//-----------------------------------------------------------------------------
void CTFGameRules::FillOutTeamplayRoundWinEvent( IGameEvent *event )
{
	event->SetInt( "flagcaplimit", IsPasstimeMode() ? tf_passtime_scores_per_round.GetInt() : tf_flag_caps_per_round.GetInt() );

	// determine the losing team
	int iLosingTeam;

	switch( event->GetInt( "team" ) )
	{
	case TF_TEAM_RED:
		iLosingTeam = TF_TEAM_BLUE;
		break;
	case TF_TEAM_BLUE:
		iLosingTeam = TF_TEAM_RED;
		break;
	case TEAM_UNASSIGNED:
	default:
		iLosingTeam = TEAM_UNASSIGNED;
		break;
	}

	// set the number of caps that team got any time during the round
	event->SetInt( "losing_team_num_caps", m_iNumCaps[iLosingTeam] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupSpawnPointsForRound( void )
{
	if ( !g_hControlPointMasters.Count() || !g_hControlPointMasters[0] || !g_hControlPointMasters[0]->PlayingMiniRounds() )
		return;

	CTeamControlPointRound *pCurrentRound = g_hControlPointMasters[0]->GetCurrentRound();
	if ( !pCurrentRound )
	{
		return;
	}

	// loop through the spawn points in the map and find which ones are associated with this round or the control points in this round
	for ( int i=0; i<ITFTeamSpawnAutoList::AutoList().Count(); ++i )
	{
		CTFTeamSpawn *pTFSpawn = static_cast< CTFTeamSpawn* >( ITFTeamSpawnAutoList::AutoList()[i] );

		CHandle<CTeamControlPoint> hControlPoint = pTFSpawn->GetControlPoint();
		CHandle<CTeamControlPointRound> hRoundBlue = pTFSpawn->GetRoundBlueSpawn();
		CHandle<CTeamControlPointRound> hRoundRed = pTFSpawn->GetRoundRedSpawn();

		if ( hControlPoint && pCurrentRound->IsControlPointInRound( hControlPoint ) )
		{
			// this spawn is associated with one of our control points
			pTFSpawn->SetDisabled( false );
			pTFSpawn->ChangeTeam( hControlPoint->GetOwner() );
		}
		else if ( hRoundBlue && ( hRoundBlue == pCurrentRound ) )
		{
			pTFSpawn->SetDisabled( false );
			pTFSpawn->ChangeTeam( TF_TEAM_BLUE );
		}
		else if ( hRoundRed && ( hRoundRed == pCurrentRound ) )
		{
			pTFSpawn->SetDisabled( false );
			pTFSpawn->ChangeTeam( TF_TEAM_RED );
		}
		else
		{
			// this spawn isn't associated with this round or the control points in this round
			pTFSpawn->SetDisabled( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::SetCurrentRoundStateBitString( void )
{
	m_iPrevRoundState = m_iCurrentRoundState;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( !pMaster )
	{
		return 0;
	}

	int iState = 0;

	for ( int i=0; i<pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		if ( pPoint->GetOwner() == TF_TEAM_BLUE )
		{
			// Set index to 1 for the point being owned by blue
			iState |= ( 1<<i );
		}
	}

	m_iCurrentRoundState = iState;

	return iState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetMiniRoundBitMask( int iMask )
{
	m_iCurrentMiniRoundMask = iMask;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsFirstBloodAllowed( void )
{
	// Already granted
	if ( m_bArenaFirstBlood )
		return false;

	if ( IsInArenaMode() && tf_arena_first_blood.GetBool() )
		return true;

	if ( IsCompetitiveMode() && ( State_Get() == GR_STATE_RND_RUNNING ) )
	{
		if ( IsMatchTypeCompetitive() )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: NULL pPlayer means show the panel to everyone
//-----------------------------------------------------------------------------
void CTFGameRules::ShowRoundInfoPanel( CTFPlayer *pPlayer /* = NULL */ )
{
	KeyValues *data = new KeyValues( "data" );

	if ( m_iCurrentRoundState < 0 )
	{
		// Haven't set up the round state yet
		return;
	}

	// if prev and cur are equal, we are starting from a fresh round
	if ( m_iPrevRoundState >= 0 && pPlayer == NULL )	// we have data about a previous state
	{
		data->SetInt( "prev", m_iPrevRoundState );
	}
	else
	{
		// don't send a delta if this is just to one player, they are joining mid-round
		data->SetInt( "prev", m_iCurrentRoundState );	
	}

	data->SetInt( "cur", m_iCurrentRoundState );

	// get bitmask representing the current miniround
	data->SetInt( "round", m_iCurrentMiniRoundMask );

	if ( pPlayer )
	{
		pPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
	}
	else
	{
		for ( int i = 1;  i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
			{
				pTFPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TimerMayExpire( void )
{
	// Prevent timers expiring while control points are contested
	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int iPoint = 0; iPoint < iNumControlPoints; iPoint ++ )
	{
		if ( ObjectiveResource()->GetCappingTeam(iPoint) )
		{
			m_flCapInProgressBuffer = gpGlobals->curtime + 0.1f;
			return false;
		}
	}

	// This delay prevents an order-of-operations issue with caps that
	// fire an AddTime output, and round timers' OnFinished output
	if ( m_flCapInProgressBuffer >= gpGlobals->curtime )
		return false;

	if ( GetOvertimeAllowedForCTF() )
	{
		// Prevent timers expiring while flags are stolen/dropped
		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
			if ( !pFlag->IsDisabled() && !pFlag->IsHome() )
				return false;
		}
	}

	for ( int i = 0 ; i < m_CPTimerEnts.Count() ; i++ )
	{
		CCPTimerLogic *pTimer = m_CPTimerEnts[i];
		if ( pTimer )
		{
			if ( pTimer->TimerMayExpire() == false )
				return false;
		}
	}

#ifdef TF_RAID_MODE
	if ( IsRaidMode() && IsBossBattleMode() && tf_raid_allow_overtime.GetBool() )
	{
		CUtlVector< CTFPlayer * > alivePlayerVector;
		CollectPlayers( &alivePlayerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );

		// if anyone is alive, go into overtime
		if ( alivePlayerVector.Count() > 0 )
		{
			return false;
		}
	}
#endif

	return BaseClass::TimerMayExpire();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::BHavePlayers( void )
{
	CMatchInfo *pInfo = GTFGCClientSystem()->GetMatch();
	if ( pInfo )
	{
		// When we have a match, we start reporting we're active as soon as the first person loads, and never stop.
		return pInfo->m_bFirstPersonActive;
	}

	if ( IsInArenaMode() )
	{
		// At least two in queue, we're always able to play
		if ( m_hArenaPlayerQueue.Count() >= 2 )
			return true;

		// Otherwise, return false if nobody is actually on a team, regardless of players ready-to-play state.
		if ( GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() == 0 || GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() == 0 )
			return false;

		// Otherwise, fall through to base logic (e.g. 1v0 but already running)
	}

	return BaseClass::BHavePlayers();
}

int SortPlayerSpectatorQueue( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	float flTime1 = (*p1)->GetTeamJoinTime();
	float flTime2 = (*p2)->GetTeamJoinTime();

	if ( flTime1 == flTime2 )
	{
		flTime1 = (*p1)->GetConnectionTime();
		flTime2 = (*p2)->GetConnectionTime();

		if ( flTime1 < flTime2 )
			return -1;
	}
	else
	{
		if ( flTime1 > flTime2 )
			return -1;
	}
		
	if ( flTime1 == flTime2 )
		return 0;

	return 1;
}

int SortPlayersScoreBased( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );

	if ( pResource )
	{
		int nScore2 = pResource->GetTotalScore( ( *p2 )->entindex() );
		int nScore1 = pResource->GetTotalScore( ( *p1 )->entindex() );

		// check the priority
		if ( nScore2 > nScore1 )
		{
			return 1;
		}
	}

	return -1;
}

// sort function for the list of players that we're going to use to scramble the teams
int ScramblePlayersSort( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );

	if ( pResource )
	{
		int nScore2 = pResource->GetTotalScore( (*p2)->entindex() );
		int nScore1 = pResource->GetTotalScore( (*p1)->entindex() );

		// check the priority
		if ( nScore2 > nScore1 )
		{
			return 1;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_ClientDisconnect( const char *playername )
{
	if ( IsInArenaMode() == false )
		return;

	if ( IsInWaitingForPlayers() == true )
		return;

	if ( m_iRoundState != GR_STATE_PREROUND )
		return;

	if ( IsInTournamentMode() == true )
		return;

	int iLight, iHeavy;

	if ( AreTeamsUnbalanced( iHeavy, iLight ) == true )
	{
		CTeam *pTeamHeavy = GetGlobalTeam( iHeavy );
		CTeam *pTeamLight = GetGlobalTeam( iLight );

		if ( pTeamHeavy == NULL || pTeamLight == NULL )
			return;

		int iPlayersNeeded = pTeamHeavy->GetNumPlayers() - pTeamLight->GetNumPlayers();

		if ( m_hArenaPlayerQueue.Count() == 0 )
			return;

		for ( int iPlayers = 0; iPlayers < iPlayersNeeded; iPlayers++ )
		{
			CTFPlayer *pPlayer = m_hArenaPlayerQueue[iPlayers];

			if ( pPlayer && pPlayer->IsHLTV() == false && pPlayer->IsReplay() == false )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );

				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_ClientDisconnect", pPlayer->GetPlayerName(), GetGlobalTeam( pPlayer->GetTeamNumber() )->GetName(), playername  );

				if ( pPlayer->DidPlayerJustPlay() == false )
				{
					pPlayer->MarkTeamJoinTime();
				}

				m_hArenaPlayerQueue.FindAndRemove( pPlayer );
				pPlayer->PlayerJustPlayed( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_ResetLosersScore( bool bResetAll )
{
	//Winner gets to keep their score
	for ( int i = 0; i < GetNumberOfTeams(); i++ )
	{
		if ( ( i != GetWinningTeam() && GetWinningTeam() > LAST_SHARED_TEAM ) || bResetAll == true )
		{
			GetGlobalTeam( i )->ResetScores();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_PrepareNewPlayerQueue( bool bResetAll )
{
	CUtlVector<CTFPlayer*> pTempPlayerQueue;

	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && pPlayer->IsHLTV() == false && pPlayer->IsReplay() == false )
		{
			if ( ( GetWinningTeam() > LAST_SHARED_TEAM && pPlayer->GetTeamNumber() != GetWinningTeam() && pPlayer->IsReadyToPlay() ) || bResetAll == true  )
			{
				pTempPlayerQueue.AddToTail( pPlayer );
			}
		}
	}

	//Sort the players going into spectator
	//Players that have been longer on the server should go in front of newer players
	//We have to do this since player slots are reused for new clients, so looping through players
	//using their entindex doesn't work.

	if ( bResetAll == true )
	{
		pTempPlayerQueue.Sort( ScramblePlayersSort );
	}
	else
	{
		pTempPlayerQueue.Sort( SortPlayerSpectatorQueue );
	}

	for ( int i = 0; i < pTempPlayerQueue.Count(); i++ )
	{
		CTFPlayer *pPlayer = pTempPlayerQueue[i];

		if ( pPlayer && pPlayer->IsReadyToPlay() )
		{
			//Changing into Spectator Team puts the player at the end of the queue
			//Use ForceChangeTeam if you want to move them to the FRONT of the queue
			pPlayer->ChangeTeam( TEAM_SPECTATOR );
			pPlayer->PlayerJustPlayed( true );
		}
	}
}

#define TF_ARENA_TEAM_COUNT 3

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGameRules::Arena_PlayersNeededForMatch( void )
{
	int iMaxPlayers = gpGlobals->maxClients;

	if ( HLTVDirector()->IsActive() )
	{
		iMaxPlayers -= 1;
	}

	int iTeamSize;
	if ( tf_arena_override_team_size.GetInt() > 0 )
		iTeamSize = tf_arena_override_team_size.GetInt();
	else
		iTeamSize = floor( ( (float)iMaxPlayers / TF_ARENA_TEAM_COUNT ) + 0.5f );

	int iPlayersNeeded = iTeamSize * 2;
	int iDesiredTeamSize = iTeamSize;
	int iPlayersInWinningTeam = 0;
	bool bRebalanceWinners = false;

	int iPlayerNumber = 0;

	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && pPlayer->IsReadyToPlay() )
		{
			iPlayerNumber++;
		}
	}

	int iBalancedTeamSize = floor( ((m_hArenaPlayerQueue.Count() + iPlayerNumber ) * 0.5f ) );

	iPlayersNeeded = (iBalancedTeamSize - iPlayerNumber) + iBalancedTeamSize;

	if ( (iPlayersNeeded + iPlayerNumber) > iDesiredTeamSize*2 )
	{
		iPlayersNeeded = (iDesiredTeamSize*2) - iPlayerNumber;

		if ( iPlayersNeeded < 0 )
		{
			iPlayersNeeded = 0;
		}
	}

	// If the last round was won by a team, then let's figure out how many players we need.
	// Also, if the winning team has more players than are available, then let's have one of them switch teams.
	if ( GetWinningTeam() > LAST_SHARED_TEAM )
	{
		iPlayersInWinningTeam = GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers();
		
		if ( iPlayersInWinningTeam > iTeamSize ) 
		{
			bRebalanceWinners = true;
			iDesiredTeamSize = iTeamSize;
		}
		else if ( iPlayersInWinningTeam > iBalancedTeamSize )
		{
			bRebalanceWinners = true;
			iDesiredTeamSize = iBalancedTeamSize;
		}
	}

//	Msg( "iPlayerNumber: %d - InQueue: %d - iPlayersInWinningTeam: %d - iDesiredTeamSize: %d - iBalancedTeamSize: %d - iPlayersNeeded: %d - bRebalanceWinners %d\n", iPlayerNumber, m_hArenaPlayerQueue.Count(), iPlayersInWinningTeam, iDesiredTeamSize, iBalancedTeamSize, iPlayersNeeded, bRebalanceWinners );


	if ( bRebalanceWinners == true )
	{
		while ( iPlayersInWinningTeam > iDesiredTeamSize )
		{
			CTFPlayer *pBalancedWinner = NULL;
			float flShortestTeamJoinTime = 9999.9f;

			for ( int i = 1;  i <= MAX_PLAYERS; i++ )
			{
				CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

				if ( pPlayer && pPlayer->GetTeamNumber() == GetWinningTeam() && pPlayer->IsHLTV() == false && pPlayer->IsReplay() == false )
				{
					if ( bRebalanceWinners == true )
					{
						//Find the newest guy that joined this team and flag him.
						if ( (gpGlobals->curtime - pPlayer->GetTeamJoinTime()) < flShortestTeamJoinTime )
						{
							flShortestTeamJoinTime = (gpGlobals->curtime - pPlayer->GetTeamJoinTime());
							pBalancedWinner = pPlayer;
						}
					}
				}
			}

			if ( pBalancedWinner )
			{
				pBalancedWinner->ForceChangeTeam( TEAM_SPECTATOR );
				pBalancedWinner->MarkTeamJoinTime();

				if ( iPlayersNeeded < iDesiredTeamSize )
				{
					iPlayersNeeded++;
				}

				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
				if ( event )
				{
					event->SetInt( "player", pBalancedWinner->entindex() );
					event->SetInt( "team", GetWinningTeam() == TF_TEAM_BLUE ? TF_TEAM_RED : TF_TEAM_BLUE );
					gameeventmanager->FireEvent( event );
				}

				// tell people that we've switched this player
				UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", pBalancedWinner->GetPlayerName() );
			}

			iPlayersInWinningTeam = GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers();
		}
	}
	
	return iPlayersNeeded;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_CleanupPlayerQueue( void )
{
	//One more loop to remove players that are currently playing from the queue 
	//And to mark everyone as not having just played.
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && pPlayer->IsHLTV() == false && pPlayer->IsReplay() == false )
		{
			if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
			{
				m_hArenaPlayerQueue.FindAndRemove( pPlayer );
			}

			pPlayer->PlayerJustPlayed( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_RunTeamLogic( void )
{
	if ( IsInArenaMode() == false )
		return;

	if ( IsInWaitingForPlayers() == true )
		return;

	bool bGameNotReady = !BHavePlayers();
	bool bStreaksReached = false;

	if ( tf_arena_use_queue.GetBool() == false )
	{
		if ( bGameNotReady == true || GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() == 0 || GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() == 0 )
		{
			State_Transition( GR_STATE_PREGAME );
		}

		return;
	}

	if ( tf_arena_max_streak.GetInt() > 0 )
	{
		if ( GetWinningTeam() != TEAM_UNASSIGNED )
		{
			if ( GetGlobalTFTeam( GetWinningTeam() )->GetScore() >= tf_arena_max_streak.GetInt() )
			{
				bStreaksReached = true;

				IGameEvent *event = gameeventmanager->CreateEvent( "arena_match_maxstreak" );
				if ( event )
				{
					event->SetInt( "team", GetWinningTeam() );
					event->SetInt( "streak", tf_arena_max_streak.GetInt() );
					gameeventmanager->FireEvent( event );
				}	

				BroadcastSound( 255, "Announcer.AM_TeamScrambleRandom" );

				m_iWinningTeam = TEAM_UNASSIGNED;
			}
		}
	}

	if ( IsInTournamentMode() == false )
	{
		Arena_ResetLosersScore( bGameNotReady || bStreaksReached );
	}

	Arena_PrepareNewPlayerQueue(  bGameNotReady || bStreaksReached );
	
	if ( bGameNotReady == true )
	{
		State_Transition( GR_STATE_PREGAME );
		return;
	}

	int iPlayersNeeded = Arena_PlayersNeededForMatch();
	
	//Let's add people to teams
	//But only do this if there's people in the actual game and teams are unbalanced 
	//(which they should be since winners are in and everyone else is spectating)
	int iLight, iHeavy;

	if ( AreTeamsUnbalanced( iHeavy, iLight ) == true && iPlayersNeeded > 0 )
	{
		if ( iPlayersNeeded > m_hArenaPlayerQueue.Count() )
		{
			iPlayersNeeded = m_hArenaPlayerQueue.Count();
		}
	
//		Msg( "iTeamSize: %d\n", iTeamSize );

		int iTeam = GetWinningTeam();

		int iSwitch = floor( ((GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers() + iPlayersNeeded) * 0.5f ) - GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers() );

		if ( GetWinningTeam() == TEAM_UNASSIGNED )
		{
			iTeam = TF_TEAM_AUTOASSIGN;
		}

		//Move people in the queue into a team.
		for ( int iPlayers = 0; iPlayers < iPlayersNeeded; iPlayers++ )
		{
			CTFPlayer *pPlayer = m_hArenaPlayerQueue[iPlayers];

			if ( iPlayers >= iSwitch )
			{
				iTeam = TF_TEAM_AUTOASSIGN;
			}

			if ( pPlayer )
			{
				pPlayer->ForceChangeTeam( iTeam );

//				Msg( "Moving Player to game: %s - team: %d\n", pPlayer->GetPlayerName(), pPlayer->GetTeamNumber() );

				if ( pPlayer->DidPlayerJustPlay() == false )
				{
					pPlayer->MarkTeamJoinTime();
				}
			}
		}
	}

	

	// show the class composition panel

	m_flSendNotificationTime = gpGlobals->curtime + 1.0f;

	Arena_CleanupPlayerQueue();
	Arena_NotifyTeamSizeChange();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_NotifyTeamSizeChange( void )
{
	int iTeamSize = GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers();

	if ( iTeamSize == m_iPreviousTeamSize )
		return;

	if ( m_iPreviousTeamSize == 0 )
	{
		m_iPreviousTeamSize = iTeamSize;
		return;
	}

	if ( m_iPreviousTeamSize > iTeamSize )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeDecreased", UTIL_VarArgs( "%d", iTeamSize  ) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeIncreased", UTIL_VarArgs( "%d", iTeamSize  ) );
	}

	m_iPreviousTeamSize = iTeamSize;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_SendPlayerNotifications( void )
{
	int iTeamPlayers = GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() + GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers();
	int iNumPlayers = 0;

	m_flSendNotificationTime = 0.0f;

	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && pPlayer->IsHLTV() == false && pPlayer->IsReplay() == false && pPlayer->GetDesiredPlayerClassIndex() > TF_CLASS_UNDEFINED )
		{
			iNumPlayers++;
			
			if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR && pPlayer->GetPreviousTeam() != TEAM_UNASSIGNED )
			{
				CSingleUserRecipientFilter filter( pPlayer );

				UserMessageBegin( filter, "HudArenaNotify" );
					WRITE_BYTE( pPlayer->entindex() );
					WRITE_BYTE( TF_ARENA_NOTIFICATION_SITOUT );
				MessageEnd();
			}
		}
	}

	if ( iTeamPlayers == iNumPlayers ) 
		return;

	int iExtras = iNumPlayers - iTeamPlayers;

	for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; iTeam++ )
	{
		CUtlVector<CTFPlayer*> pTempPlayerQueue;
		
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );

		if ( pTeam )
		{
			for ( int iPlayer = 0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
			{
				CTFPlayer *pPlayer = ToTFPlayer( pTeam->GetPlayer( iPlayer ) );

				if ( pPlayer && pPlayer->IsHLTV() == false && pPlayer->IsReplay() == false )
				{
					pTempPlayerQueue.AddToTail( pPlayer );
				}
			}
		}
	
		pTempPlayerQueue.Sort( SortPlayerSpectatorQueue );

		for ( int i = pTempPlayerQueue.Count(); --i >= 0; )
		{
			if ( pTempPlayerQueue.Count() - i > iExtras )
				continue;

			CTFPlayer *pPlayer = pTempPlayerQueue[i];

			CSingleUserRecipientFilter filter( pPlayer );
			UserMessageBegin( filter, "HudArenaNotify" );
				WRITE_BYTE( pPlayer->entindex() );
				WRITE_BYTE( TF_ARENA_NOTIFICATION_CAREFUL );
			MessageEnd();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle maps that may be in our map cycle/etc changing name or availability
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
void CTFGameRules::OnWorkshopMapUpdated( PublishedFileId_t nUpdatedWorkshopID )
{
	// Check if this map is in the mapcycle under a different name, reload mapcycle if so. We want the up-to-date names
	// in the map cycle as it is used for user-facing things such as votes.
	CTFMapsWorkshop *pWorkshop = TFMapsWorkshop();
	if ( pWorkshop )
	{
		FOR_EACH_VEC( m_MapList, i )
		{
			// Check if this represents a workshop map
			PublishedFileId_t nWorkshopID = pWorkshop->MapIDFromName( m_MapList[ i ] );
			if ( nWorkshopID == nUpdatedWorkshopID )
			{
				CUtlString newName;
				if ( pWorkshop->GetMapName( nWorkshopID, newName ) == CTFMapsWorkshop::eName_Canon &&
				     newName != m_MapList[ i ] )
				{
					// We can't just fixup the name here, as the primary mapcycle is also mirrored to a string
					// table. This queues a reload, which will check for workshop names at that point.
					m_bMapCycleNeedsUpdate = true;
					break;
				}
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Hook new map cycle file loads
//-----------------------------------------------------------------------------
void CTFGameRules::LoadMapCycleFile( void )
{
	BaseClass::LoadMapCycleFile();
#ifdef GAME_DLL
	// The underlying LoadMapCycleFileIntoVector fixes up workshop names, but for loading the primary map cycle file, we
	// also want to tell the workshop to track them. See also: TFGameRules::OnWorkshopMapChanged
	TrackWorkshopMapsInMapCycle();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Hook new map cycle file loads
//-----------------------------------------------------------------------------
void CTFGameRules::TrackWorkshopMapsInMapCycle( void )
{
	CTFMapsWorkshop *pWorkshop = TFMapsWorkshop();
	if ( pWorkshop )
	{
		unsigned int nAddedMaps = 0;
		unsigned int nWorkshopMaps = 0;
		FOR_EACH_VEC( m_MapList, i )
		{
			// Check if this represents a workshop map
			PublishedFileId_t nWorkshopID = pWorkshop->MapIDFromName( m_MapList[ i ] );
			if ( nWorkshopID != k_PublishedFileIdInvalid )
			{
				nWorkshopMaps++;
				// Track it if we're not
				if ( pWorkshop->AddMap( nWorkshopID ) )
				{
					nAddedMaps++;
				}
			}
		}

		if ( nAddedMaps )
		{
			Msg( "Tracking %u new workshop maps from map cycle (%u already tracked)\n", nAddedMaps, nWorkshopMaps - nAddedMaps );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hook new map cycle file loads
//-----------------------------------------------------------------------------
void CTFGameRules::LoadMapCycleFileIntoVector( const char *pszMapCycleFile, CUtlVector<char *> &mapList )
{
	BaseClass::LoadMapCycleFileIntoVector( pszMapCycleFile, mapList );

#ifdef GAME_DLL
	// Fixup workshop map names if known. E.g. workshop/12345 -> workshop/cp_foo.ugc12345
	CTFMapsWorkshop *pWorkshop = TFMapsWorkshop();
	if ( pWorkshop )
	{
		FOR_EACH_VEC( mapList, i )
		{
			// Check if this represents a workshop map
			PublishedFileId_t nWorkshopID = pWorkshop->MapIDFromName( mapList[ i ] );
			if ( nWorkshopID != k_PublishedFileIdInvalid )
			{
				// Workshop map, update name
				CUtlString newName;
				pWorkshop->GetMapName( nWorkshopID, newName );
				if ( newName.Length() )
				{
					// Alloc replacement
					size_t nNewSize = newName.Length() + 1;
					char *pNew = new char[ nNewSize ];
					V_strncpy( pNew, newName.Get(), nNewSize );

					// Replace
					delete [] mapList[ i ];
					mapList[ i ] = pNew;
				}
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:  Server-side vote creation
//-----------------------------------------------------------------------------
void CTFGameRules::ManageServerSideVoteCreation( void )
{
	if ( gpGlobals->curtime < m_flVoteCheckThrottle )
		return;

	if ( IsInTournamentMode() )
		return;

	if ( IsInArenaMode() )
		return;

	if ( IsInWaitingForPlayers() )
		return;

	if ( m_bInSetup )
		return;

	if ( IsInTraining() )
		return;

	if ( IsInItemTestingMode() )
		return;

	if ( m_MapList.Count() < 2 )
		return;
	
	// Ask players which map they would prefer to play next, based
	// on "n" lowest playtime from server stats

	ConVarRef sv_vote_issue_nextlevel_allowed( "sv_vote_issue_nextlevel_allowed" );
	ConVarRef sv_vote_issue_nextlevel_choicesmode( "sv_vote_issue_nextlevel_choicesmode" );

	if ( sv_vote_issue_nextlevel_allowed.GetBool() && sv_vote_issue_nextlevel_choicesmode.GetBool() )
	{
		// Don't do this if we already have a nextlevel set
		if ( nextlevel.GetString() && *nextlevel.GetString() )
			return;

		if ( !m_bServerVoteOnReset && !m_bVoteCalled )
		{
			// If we have any round or win limit, ignore time
			if ( mp_winlimit.GetInt() || mp_maxrounds.GetInt() )
			{
				int nBlueScore = TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore();
				int nRedScore = TFTeamMgr()->GetTeam( TF_TEAM_RED)->GetScore();
				int nWinLimit = mp_winlimit.GetInt();
				if ( ( nWinLimit - nBlueScore ) == 1 || ( nWinLimit - nRedScore ) == 1 )
				{
					m_bServerVoteOnReset = true;
				}

				int nRoundsPlayed = GetRoundsPlayed();
				if ( ( mp_maxrounds.GetInt() - nRoundsPlayed ) == 1 )
				{
					m_bServerVoteOnReset = true;
				}
			}
			else if ( mp_timelimit.GetInt() > 0 )
			{
				int nTimeLeft = GetTimeLeft();
				if ( nTimeLeft <= 120 && !m_bServerVoteOnReset )
				{
					if ( g_voteControllerGlobal )
					{
						g_voteControllerGlobal->CreateVote( DEDICATED_SERVER, "nextlevel", "" );
					}
					m_bVoteCalled = true;
				}
			}
		}
	}

	m_flVoteCheckThrottle = gpGlobals->curtime + 0.5f;
}


//-----------------------------------------------------------------------------
// Purpose: Figures out how much money to put in a custom currency pack drop
//-----------------------------------------------------------------------------
int CTFGameRules::CalculateCurrencyAmount_CustomPack( int nAmount )
{
	// Entities and events that specify a custom currency value should pass in the amount
	// they're worth, and we figure out if there's enough currency to generate a pack.
	// If the amount passed in isn't enough to generate a pack, we store it in an accumulator.
	
	int nMinDrop = kMVM_CurrencyPackMinSize;
	if ( nMinDrop > 1 )
	{
		// If we're on the last spawn, drop everything
		if ( TFObjectiveResource()->GetMannVsMachineWaveEnemyCount() == 1 )
		{
			nMinDrop = m_nCurrencyAccumulator + nAmount;
			m_nCurrencyAccumulator = 0;
			return nMinDrop;
		}

		// If we're passing in a value above mindrop, just drop it
		if ( nAmount >= nMinDrop )
			return nAmount;

		// Accumulate currency if we're getting values below nMinDrop
		m_nCurrencyAccumulator += nAmount;
		if ( m_nCurrencyAccumulator >= nMinDrop )
		{
			m_nCurrencyAccumulator -= nMinDrop;
			//DevMsg( "*MIN REACHED* -- %d left\n", m_nCurrencyAccumulator );
			return nMinDrop;
		}
		else
		{
			// We don't have enough yet, drop nothing
			//DevMsg( "*STORE* -- %d stored\n", m_nCurrencyAccumulator );
			return 0;
		}
	}
	else
	{
		// We don't have a PopManager - return the amount passed in
		return nAmount;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Figures out how much to give for pre-definied events or types
//-----------------------------------------------------------------------------
int CTFGameRules::CalculateCurrencyAmount_ByType( CurrencyRewards_t nType )
{
	// CUSTOM values are usually determined by CalculateCurrencyAmount_CustomPack() and set via CCurrencyPack::SetValue()
	Assert ( nType != TF_CURRENCY_PACK_CUSTOM );

	int nAmount = 0;

	switch ( nType )
	{
	case TF_CURRENCY_KILLED_PLAYER:
		nAmount = 40;
		break;

	case TF_CURRENCY_KILLED_OBJECT:
		nAmount = 40;
		break;

	case TF_CURRENCY_ASSISTED_PLAYER:
		nAmount = 20;
		break;

	case TF_CURRENCY_BONUS_POINTS:
		nAmount = 1;
		break;

	case TF_CURRENCY_CAPTURED_OBJECTIVE:
		nAmount = 100;
		break;

	case TF_CURRENCY_ESCORT_REWARD:
		nAmount = 10;
		break;

	case TF_CURRENCY_PACK_SMALL:
		nAmount = 5;
		break;

	case TF_CURRENCY_PACK_MEDIUM:
		nAmount = 10;
		break;

	case TF_CURRENCY_PACK_LARGE:
		nAmount = 25;
		break;

	case TF_CURRENCY_TIME_REWARD:
		nAmount = 5;
		break;

	case TF_CURRENCY_WAVE_COLLECTION_BONUS:
		nAmount = 100;
		break;

	default:
		Assert( 0 );	// Unknown type
	};

	return nAmount;
}

//-----------------------------------------------------------------------------
// Purpose: Gives money directly to a team or specific player
//-----------------------------------------------------------------------------
int CTFGameRules::DistributeCurrencyAmount( int nAmount, CTFPlayer *pTFPlayer /* = NULL */, bool bShared /* = true */, bool bCountAsDropped /*= false */, bool bIsBonus /*= false */ )
{
	// Group distribution (default)
	if ( bShared )
	{
		CUtlVector<CTFPlayer *> playerVector;

		if ( IsMannVsMachineMode() )
		{
			CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
		}

		// Money
		FOR_EACH_VEC( playerVector, i )
		{
			if ( playerVector[i] )
			{

				playerVector[i]->AddCurrency( nAmount );
			}
		}
	}
	// Individual distribution
	else if ( pTFPlayer )
	{

		pTFPlayer->AddCurrency( nAmount );
	}

	// Accounting
	if ( IsMannVsMachineMode() && g_pPopulationManager )
	{
		g_pPopulationManager->OnCurrencyCollected( nAmount, bCountAsDropped, bIsBonus );
	}

	return nAmount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RoundRespawn( void )
{
	m_hasSpawnedToy = false;
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_bHasSpawnedSoccerBall[i] = false;
	}

	// remove any buildings, grenades, rockets, etc. the player put into the world
	RemoveAllProjectilesAndBuildings();

	// re-enable any sentry guns the losing team has built (and not hidden!)
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() == OBJ_SENTRYGUN && pObj->IsEffectActive( EF_NODRAW ) == false && pObj->GetTeamNumber() != m_iWinningTeam )
		{
			pObj->SetDisabled( false );
		}
	}

#ifdef TF_RAID_MODE
	// Raid mode: clean up any Red buildings that might be left behind from the previous round
	if ( IsRaidMode() )
	{
		CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
		if ( pTeam )
		{
			int nTeamObjectCount = pTeam->GetNumObjects();

			for ( int iObject = 0; iObject < nTeamObjectCount; ++iObject )
			{
				CBaseObject *pObj = pTeam->GetObject( iObject );

				if ( pObj )
				{
					pObj->SetThink( &CBaseEntity::SUB_Remove );
					pObj->SetNextThink( gpGlobals->curtime );
					pObj->SetTouch( NULL );
					pObj->AddEffects( EF_NODRAW );
				}
			}
		}
	}
	else if ( IsBossBattleMode() )
	{
		// unspawn entire red team
		CTeam *defendingTeam = GetGlobalTeam( TF_TEAM_RED );
		int i;
		for( i=0; i<defendingTeam->GetNumPlayers(); ++i )
		{
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", defendingTeam->GetPlayer(i)->GetUserID() ) );
		}
	}
#endif // TF_RAID_MODE

	if ( IsInTournamentMode() == false )
	{
		Arena_RunTeamLogic();
	}
	
	m_bArenaFirstBlood = false;
	
	// reset the flag captures
	int nTeamCount = TFTeamMgr()->GetTeamCount();
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->SetFlagCaptures( 0 );
	}

	if ( !IsMannVsMachineMode() )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_update" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}

	// reset player per-round stats
	CTF_GameStats.ResetRoundStats();

	BaseClass::RoundRespawn();

	if ( m_bForceMapReset || m_bPrevRoundWasWaitingForPlayers )
	{
		// reset meter charges to their default values
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer )
			{
				pPlayer->m_Shared.SetDefaultItemChargeMeters();
			}
		}
	}

	// ** AFTER WE'VE BEEN THROUGH THE ROUND RESPAWN, SHOW THE ROUNDINFO PANEL
	if ( !IsInWaitingForPlayers() )
	{
		ShowRoundInfoPanel();
	}

	// We've hit some condition where a server-side vote should be called on respawn
	if ( m_bServerVoteOnReset )
	{
		if ( g_voteControllerGlobal )
		{
			g_voteControllerGlobal->CreateVote( DEDICATED_SERVER, "nextlevel", "" );
		}
		m_bVoteCalled = true;
		m_bServerVoteOnReset = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldSwitchTeams( void )
{
	if ( IsPVEModeActive() )
		return false;

	return BaseClass::ShouldSwitchTeams();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScrambleTeams( void )
{
	if ( IsPVEModeActive() )
		return false;

	if ( IsCompetitiveMode() )
		return false;

	return BaseClass::ShouldScrambleTeams();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( CBaseEntity::Instance( pEntity ) );

	if ( !pTFPlayer )
		return;

	char const *pszCommand = pKeyValues->GetName();
	if ( pszCommand && pszCommand[0] )
	{
		if ( FStrEq( pszCommand, "FreezeCamTaunt" ) )
		{
			CTFPlayer *pAchiever = ToTFPlayer( UTIL_PlayerByUserId( pKeyValues->GetInt( "achiever" ) ) );
			if ( pAchiever )
			{
				const char *pszCommand = pKeyValues->GetString( "command" );
				if ( pszCommand && pszCommand[0] )
				{
					int nGibs = pKeyValues->GetInt( "gibs" );

					if ( FStrEq( pszCommand, "freezecam_taunt" ) )
					{	
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamAchievements );
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamAchievements2 );
						HatAndMiscEconEntities_OnOwnerKillEaterEventNoParter( pAchiever, kKillEaterEvent_KillcamTaunts );
					}
					else if ( FStrEq( pszCommand, "freezecam_tauntrag" ) )
					{	
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamRagdollAchievements );
					}
					else if ( FStrEq( pszCommand, "freezecam_tauntgibs" ) )
					{	
						CheckTauntAchievement( pAchiever, nGibs, g_TauntCamAchievements );
					}
					else if ( FStrEq( pszCommand, "freezecam_tauntsentry" ) )
					{
						// Maybe should also require a taunt? Currently too easy to get?
						pAchiever->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_FREEZECAM_SENTRY );
					}
				}
			}
		}
		else if ( FStrEq( pszCommand, "UsingVRHeadset" ) )
		{
			static CSchemaItemDefHandle pItemDef_OculusRiftHeadset( "The TF2VRH" );

			// make sure they actually have the headset equipped before setting the flag
			// we're only using this message to set the alt model for the TF2VRH model on the client
			for ( int nWearable = 0; nWearable < pTFPlayer->GetNumWearables(); nWearable++ )
			{
				CEconWearable *pWearable = pTFPlayer->GetWearable( nWearable );
				if ( pWearable && pWearable->GetAttributeContainer() )
				{
					CEconItemView *pItem = pWearable->GetAttributeContainer()->GetItem();
					if ( pItem && pItem->GetStaticData() == pItemDef_OculusRiftHeadset )
					{
						pTFPlayer->SetUsingVRHeadset( true );
						break;
					}
				}
			}
		}
		else if ( FStrEq( pszCommand, "TestItems" ) )
		{
			pTFPlayer->ItemTesting_Start( pKeyValues );
		}
		else if ( FStrEq( pszCommand, "TestItemsBotUpdate" ) )
		{
			ItemTesting_SetupFromKV( pKeyValues );
		}
		else if ( FStrEq( pszCommand, "MVM_Upgrade" ) )
		{
			if ( GameModeUsesUpgrades() )
			{
				if ( IsMannVsMachineMode() )
				{
					if ( sv_cheats && !sv_cheats->GetBool() && !pTFPlayer->m_Shared.IsInUpgradeZone() )
						return;
				}

				if ( g_hUpgradeEntity )
				{
					// First sell everything we want to sell
					KeyValues *pSubKey = pKeyValues->GetFirstTrueSubKey();
					while ( pSubKey )
					{
						int iCount = pSubKey->GetInt("count");
						if ( iCount < 0 )
						{
							int iItemSlot = pSubKey->GetInt("itemslot");
							int iUpgrade = pSubKey->GetInt("upgrade");
							bool bFree = pSubKey->GetBool( "free", false );

							// Stop attempting once no more purchases are possible to prevent spoofed messages DoSing
							// the server.
							bool bAllowed = true;
							while ( bAllowed && iCount < 0 )
							{
								bAllowed = g_hUpgradeEntity->PlayerPurchasingUpgrade( pTFPlayer, iItemSlot, iUpgrade, true, bFree );
								++iCount;
							}
						}

						pSubKey = pSubKey->GetNextTrueSubKey();
					}

					// Now buy everything we want to buy
					pSubKey = pKeyValues->GetFirstTrueSubKey();
					while ( pSubKey )
					{
						int iCount = pSubKey->GetInt("count");
						if ( iCount > 0 )
						{
							int iItemSlot = pSubKey->GetInt("itemslot");
							int iUpgrade = pSubKey->GetInt("upgrade");
							bool bFree = ( sv_cheats && sv_cheats->GetBool() ) ? pSubKey->GetBool( "free", false ) : false;	// Never let a client set "free" without sv_cheats 1

							// Stop attempting once no more purchases are possible to prevent spoofed messages DoSing
							// the server.
							bool bAllowed = true;
							while ( bAllowed && iCount > 0 )
							{
								bAllowed = g_hUpgradeEntity->PlayerPurchasingUpgrade( pTFPlayer, iItemSlot, iUpgrade, false, bFree );
								--iCount;
							}
						}

						pSubKey = pSubKey->GetNextTrueSubKey();
					}
				}
			}
		}
		else if ( FStrEq( pszCommand, "MvM_UpgradesBegin" ) )
		{
			pTFPlayer->BeginPurchasableUpgrades();
		}
		else if ( FStrEq( pszCommand, "MvM_UpgradesDone" ) )
		{
			pTFPlayer->EndPurchasableUpgrades();

			if ( IsMannVsMachineMode() && pKeyValues->GetInt( "num_upgrades", 0 ) > 0 )
			{
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_UPGRADE_COMPLETE );
			}
		}
		else if ( FStrEq( pszCommand, "MVM_Revive_Response" ) )
		{
			CTFReviveMarker *pReviveMarker = pTFPlayer->GetReviveMarker();
			if ( pReviveMarker )
			{
				if ( pKeyValues->GetBool( "accepted", 0 ) )
				{
					pReviveMarker->ReviveOwner();
				}
				else
				{
					// They hit cancel after their spawn timer was up
					if ( HasPassedMinRespawnTime( pTFPlayer ) )
					{
						pTFPlayer->ForceRespawn();
					}
	
					UTIL_Remove( pReviveMarker );
				}
			}
		}
		else if ( FStrEq( pszCommand, "MVM_Respec" ) )
		{
			if ( GameModeUsesUpgrades() && IsMannVsMachineRespecEnabled() && CanPlayerUseRespec( pTFPlayer ) )
			{
				if ( IsMannVsMachineMode() )
				{
					if ( sv_cheats && !sv_cheats->GetBool() && !pTFPlayer->m_Shared.IsInUpgradeZone() )
						return;
				}

				if ( g_hUpgradeEntity && g_pPopulationManager )
				{
					// Consume a respec credit
					g_pPopulationManager->RemoveRespecFromPlayer( pTFPlayer );

					// Remove the appropriate upgrade info from upgrade histories
					g_pPopulationManager->RemovePlayerAndItemUpgradesFromHistory( pTFPlayer );

					// Remove upgrade attributes from the player and their items
					g_hUpgradeEntity->GrantOrRemoveAllUpgrades( pTFPlayer, true );

					pTFPlayer->ForceRespawn();
				}
			}
		}
		else if ( FStrEq( pszCommand, "use_action_slot_item_server" ) )
		{
			if ( pTFPlayer->ShouldRunRateLimitedCommand( "use_action_slot_item_server" ) )
			{
				pTFPlayer->UseActionSlotItemPressed();
				pTFPlayer->UseActionSlotItemReleased();
			}
		}
		else if ( FStrEq( pszCommand, "+use_action_slot_item_server" ) )
		{
			if ( pTFPlayer->ShouldRunRateLimitedCommand( "use_action_slot_item_server" ) ) // intentionally using the same check as above
			{
				if ( !pTFPlayer->IsUsingActionSlot() )
				{
					pTFPlayer->UseActionSlotItemPressed();
				}
			}
		}
		else if ( FStrEq( pszCommand, "-use_action_slot_item_server" ) )
		{
			if ( pTFPlayer->IsUsingActionSlot() )
			{
				pTFPlayer->UseActionSlotItemReleased();
			}
		}
		else if ( FStrEq( pszCommand, "+inspect_server" ) )
		{
			pTFPlayer->InspectButtonPressed();
		}
		else if ( FStrEq( pszCommand, "-inspect_server" ) )
		{
			pTFPlayer->InspectButtonReleased();
		}
		else if ( FStrEq( pszCommand, "+helpme_server" ) )
		{
			pTFPlayer->HelpmeButtonPressed();
		}
		else if ( FStrEq( pszCommand, "-helpme_server" ) )
		{
			pTFPlayer->HelpmeButtonReleased();
		}
		else if ( FStrEq( pszCommand, "cl_drawline" ) )
		{
			BroadcastDrawLine( pTFPlayer, pKeyValues );
		}
		else if ( FStrEq( pszCommand, "sdk_inventory" ) )
		{
			CSteamID steamID;
			if ( !pTFPlayer->GetSteamID( &steamID ) )
				return;

			GTFGCClientSystem()->ProcessPlayerInventoryRequest( steamID, pKeyValues );
		}
		else
		{
			BaseClass::ClientCommandKeyValues( pEntity, pKeyValues );
		}
	}
}

#ifdef GAME_DLL
void CTFGameRules::RequestClientInventory( CSteamID steamID )
{
	// Get the player for that steam id
	CTFPlayer *pPlayer = nullptr;
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPotentialPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPotentialPlayer )
		{
			CSteamID playerSteamID;
			if ( pPotentialPlayer->GetSteamID( &playerSteamID )
				&& playerSteamID == steamID )
			{
				pPlayer = pPotentialPlayer;
				break;
			}
		}
	}
	
	if ( !pPlayer )
		return;

	// Send them a user message to ask them to send us their inventory
	// It will come back via a KeyValues message "sdk_inventory".
	CSingleUserRecipientFilter filter( pPlayer );
	UserMessageBegin( filter, "SdkRequestEquipment" );
	MessageEnd();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BroadcastDrawLine( CTFPlayer *pTFPlayer, KeyValues *pKeyValues )
{
	if ( !IsMatchTypeCompetitive() || !m_bPlayersAreOnMatchSummaryStage || pTFPlayer->BHaveChatSuspensionInCurrentMatch() )
		return;

	int paneltype = clamp( pKeyValues->GetInt( "panel", DRAWING_PANEL_TYPE_NONE ), DRAWING_PANEL_TYPE_NONE, DRAWING_PANEL_TYPE_MAX - 1 );

	if ( paneltype >= DRAWING_PANEL_TYPE_MATCH_SUMMARY )
	{
		int linetype = clamp( pKeyValues->GetInt( "line", 0 ), 0, 1 );
		float x = pKeyValues->GetFloat( "x", 0.f );
		float y = pKeyValues->GetFloat( "y", 0.f );

		IGameEvent *event = gameeventmanager->CreateEvent( "cl_drawline" );
		if ( event )
		{
			event->SetInt( "player", pTFPlayer->entindex() );
			event->SetInt( "panel",paneltype );
			event->SetInt( "line", linetype );
			event->SetFloat( "x", x );
			event->SetFloat( "y", y );

			gameeventmanager->FireEvent( event );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RespawnTeam( int iTeam )
{
	BaseClass::RespawnTeam( iTeam );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SpawnPlayerInHell( CTFPlayer *pPlayer, const char *pszSpawnPointName )
{
	Vector vTeleportPosition;
	QAngle qTeleportAngles;

	int iCachedLocations = m_mapTeleportLocations.Find( MAKE_STRING( pszSpawnPointName ) );
	if ( m_mapTeleportLocations.IsValidIndex( iCachedLocations ) )
	{
		CUtlVector< TeleportLocation_t > *pLocations = m_mapTeleportLocations[iCachedLocations];
		Assert( pLocations );
		if ( !pLocations )
			return;
		
		const TeleportLocation_t& location = pLocations->Element( RandomInt( 0, pLocations->Count() - 1 ) );
		vTeleportPosition = location.m_vecPosition;
		qTeleportAngles = location.m_qAngles;
	}
	else
	{
		CUtlVector< CBaseEntity* > m_vecPossibleSpawns;
		CBaseEntity* pSpawn = NULL;
		while( (pSpawn = gEntList.FindEntityByName( pSpawn, pszSpawnPointName ) ) != NULL )
		{
			m_vecPossibleSpawns.AddToTail( pSpawn );
		}

		// There had better be a spawnpoint in this map!
		Assert( m_vecPossibleSpawns.Count() );
		if ( m_vecPossibleSpawns.Count() == 0 )
			return;

		// Randomly choose one
		pSpawn = m_vecPossibleSpawns[ RandomInt( 0, m_vecPossibleSpawns.Count() - 1 ) ];
		vTeleportPosition = pSpawn->GetAbsOrigin();
		qTeleportAngles = pSpawn->GetAbsAngles();
	}

	// Teleport them to hell
	pPlayer->Teleport( &vTeleportPosition, &qTeleportAngles, &vec3_origin );
	pPlayer->pl.v_angle = qTeleportAngles;

	// Send us to hell as a ghost!
	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_GHOST_MODE );
	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_IN_HELL );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern ISoundEmitterSystemBase *soundemitterbase;
void CTFGameRules::PlayHelltowerAnnouncerVO( int iRedLine, int iBlueLine )
{
	static float flRedAnnouncerTalkingUntil = 0.00f, flBlueAnnouncerTalkingUntil = 0.00f;
	
	// 01 is the first line for the VO
	int iRandomVORed = RandomInt( 1, g_pszHelltowerAnnouncerLines[iRedLine].m_nCount );	
	int iRandomVOBlue = RandomInt( 1, g_pszHelltowerAnnouncerLines[iBlueLine].m_nCount );

	// the misc lines should match up so we'll play the same lines to both teams
	if ( ( iRedLine <= HELLTOWER_VO_BLUE_MISC_RARE ) && ( iBlueLine <= HELLTOWER_VO_BLUE_MISC_RARE ) )
	{
		// can we safely set blue to the same value?
		if ( iRandomVORed <= g_pszHelltowerAnnouncerLines[iBlueLine].m_nCount )
		{
			iRandomVOBlue = iRandomVORed;
		}
	}

	char szRedAudio[128];
	char szBlueAudio[128];
	V_sprintf_safe( szRedAudio, g_pszHelltowerAnnouncerLines[iRedLine].m_pszFormatString, iRandomVORed );
	V_sprintf_safe( szBlueAudio, g_pszHelltowerAnnouncerLines[iBlueLine].m_pszFormatString, iRandomVOBlue );

	bool bForceVO = false;
	switch (iRedLine)
	{
		case HELLTOWER_VO_RED_WIN:
		case HELLTOWER_VO_RED_WIN_RARE:
		case HELLTOWER_VO_RED_LOSE:
		case HELLTOWER_VO_RED_LOSE_RARE:
			bForceVO = true;
	}
	switch (iBlueLine)
	{
		case HELLTOWER_VO_BLUE_WIN:
		case HELLTOWER_VO_BLUE_WIN_RARE:
		case HELLTOWER_VO_BLUE_LOSE:
		case HELLTOWER_VO_BLUE_LOSE_RARE:
			bForceVO = true;
	}	
	
	CSoundParameters params;
	float flSoundDuration = 0;

	if ( gpGlobals->curtime > flRedAnnouncerTalkingUntil || bForceVO )
	{
		BroadcastSound( TF_TEAM_RED, szRedAudio );
		if ( soundemitterbase->GetParametersForSound( szRedAudio, params, GENDER_NONE ) )
		{
			//flSoundDuration = enginesound->GetSoundDuration( params.soundname );
			flRedAnnouncerTalkingUntil = gpGlobals->curtime + flSoundDuration;
		}
		else
		{
			flRedAnnouncerTalkingUntil = 0.00;
		}	
	}
	if ( gpGlobals->curtime > flBlueAnnouncerTalkingUntil || bForceVO )
	{
		BroadcastSound( TF_TEAM_BLUE, szBlueAudio );
		if ( soundemitterbase->GetParametersForSound( szBlueAudio, params, GENDER_NONE ) )
		{
			//flSoundDuration = enginesound->GetSoundDuration( params.soundname );
			flBlueAnnouncerTalkingUntil = gpGlobals->curtime + flSoundDuration;
		}
		else
		{
			flBlueAnnouncerTalkingUntil = 0.00;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Based on connected match players, chooses the 3 maps players can
//			vote on as their next map
//-----------------------------------------------------------------------------
void CTFGameRules::ChooseNextMapVoteOptions()
{
	// Copy chosen maps into the actual fields we're networking to clients
	for( int i=0; i < NEXT_MAP_VOTE_OPTIONS; ++i )
	{
		const MapDef_t* pMap = GTFGCClientSystem()->GetNextMapVoteByIndex( i );
		if ( !pMap )
		{
			Warning( "Invalid NextMap list, substituting current map" );
			pMap = GetItemSchema()->GetMasterMapDefByName( STRING( gpGlobals->mapname ) );
		}

		MapDefIndex_t nIndex = pMap ? pMap->m_nDefIndex : 0;
		m_nNextMapVoteOptions.Set( i, nIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CheckHelltowerCartAchievement( int iTeam )
{
	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, iTeam );

		FOR_EACH_VEC( playerVector, i )
		{
			CTFPlayer *pPlayer = playerVector[i];
			if ( pPlayer && ( pPlayer->GetObserverMode() <= OBS_MODE_DEATHCAM ) ) // they might be killed by the explosion, so check if they are OBS_MODE_NONE OR OBS_MODE_DEATHCAM
			{
				CTriggerAreaCapture *pAreaTrigger = pPlayer->GetControlPointStandingOn();
				if ( pAreaTrigger && pAreaTrigger->TeamCanCap( iTeam ) )
				{
					pPlayer->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_KILL_BROTHERS );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleMapEvent( inputdata_t &inputdata )
{
	if ( FStrEq( "sd_doomsday", STRING( gpGlobals->mapname ) ) )
	{
		// find the flag in the map
		CCaptureFlag *pFlag = NULL;
		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
			if ( !pFlag->IsDisabled() )
			{
				break;
			}
		}

		// make sure it's being carried by one of the teams
		if ( pFlag && pFlag->IsStolen() )
		{
			CTFPlayer *pFlagCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
			if ( pFlagCarrier )
			{
				// let everyone know which team has opened the rocket
				IGameEvent *event = gameeventmanager->CreateEvent( "doomsday_rocket_open" );
				if ( event )
				{
					event->SetInt( "team", pFlagCarrier->GetTeamNumber() );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		const char *pszEvent = inputdata.value.String();
		if ( FStrEq( pszEvent, "midnight" ) )
		{
			HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_HELLTOWER_MIDNIGHT );
		}
		else if ( FStrEq( pszEvent, "horde" ) )
		{
			HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_PLAYER_SKELETON_KING_APPEAR );
			PlayHelltowerAnnouncerVO( HELLTOWER_VO_RED_SKELETON_KING, HELLTOWER_VO_BLUE_SKELETON_KING );
		}
		else if ( FStrEq( pszEvent, "red_capture" ) )
		{
			CheckHelltowerCartAchievement( TF_TEAM_RED );
			if ( RandomFloat( 0, 1 ) < HELLTOWER_RARE_LINE_CHANCE )
			{
				PlayHelltowerAnnouncerVO( HELLTOWER_VO_RED_WIN_RARE, HELLTOWER_VO_BLUE_LOSE_RARE );
			}
			else
			{
				PlayHelltowerAnnouncerVO( HELLTOWER_VO_RED_WIN, HELLTOWER_VO_BLUE_LOSE );
			}
		}
		else if ( FStrEq( pszEvent, "blue_capture" ) )
		{
			CheckHelltowerCartAchievement( TF_TEAM_BLUE );
			if ( RandomFloat( 0, 1 ) < HELLTOWER_RARE_LINE_CHANCE )
			{
				PlayHelltowerAnnouncerVO( HELLTOWER_VO_RED_LOSE_RARE, HELLTOWER_VO_BLUE_WIN_RARE );
			}
			else
			{
				PlayHelltowerAnnouncerVO( HELLTOWER_VO_RED_LOSE, HELLTOWER_VO_BLUE_WIN );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetCustomUpgradesFile( inputdata_t &inputdata )
{
	const char *pszPath = inputdata.value.String();

	// Reload
	g_MannVsMachineUpgrades.LoadUpgradesFileFromPath( pszPath );

	// Tell future clients to load from this path
	V_strncpy( m_pszCustomUpgradesFile.GetForModify(), pszPath, MAX_PATH );

	// Tell connected clients to reload
	IGameEvent *pEvent = gameeventmanager->CreateEvent( "upgrades_file_changed" );
	if ( pEvent )
	{
		pEvent->SetString( "path", pszPath );
		gameeventmanager->FireEvent( pEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldWaitToStartRecording( void )
{
	if ( IsMannVsMachineMode() )
	{
		// Don't wait for the WaitingForPlayers period to end if we're MvM
		return false;
	}
	
	return BaseClass::ShouldWaitToStartRecording(); 
}

//-----------------------------------------------------------------------------
// Purpose: return true if this flag is currently allowed to be captured
//-----------------------------------------------------------------------------
bool CTFGameRules::CanFlagBeCaptured( CBaseEntity *pOther )
{
	if ( pOther && ( tf_flag_return_on_touch.GetBool() || IsPowerupMode() ) )
	{
		for ( int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pListFlag = static_cast<CCaptureFlag*>( ICaptureFlagAutoList::AutoList()[i] );
			if ( ( pListFlag->GetType() == TF_FLAGTYPE_CTF ) && !pListFlag->IsDisabled() && ( pOther->GetTeamNumber() == pListFlag->GetTeamNumber() ) && !pListFlag->IsHome() )
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: return true if the game is in a state where both flags are stolen and poisonous
//-----------------------------------------------------------------------------
bool CTFGameRules::PowerupModeFlagStandoffActive( void )
{
	if ( IsPowerupMode() )
	{
		int nQualifyingFlags = 0; // All flags need to be stolen and poisonous (poisonous time delay gives the flag carriers a chance to get out of the enemy base)
		int nEnabledFlags = 0; // Some flags might be in the autolist but be out of play. We don't want them included in the count
		for ( int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast<CCaptureFlag*>( ICaptureFlagAutoList::AutoList()[i] );
			if ( !pFlag->IsDisabled() )
				nEnabledFlags++;
			if ( pFlag->IsPoisonous() && pFlag->IsStolen() )
				nQualifyingFlags++;
		}
		if ( nQualifyingFlags == nEnabledFlags )
		{
			return true;
		}
	}
	return false;
}

void CTFGameRules::TeleportPlayersToTargetEntities( int iTeam, const char *pszEntTargetName, CUtlVector< CTFPlayer * > *pTeleportedPlayers )
{
	CUtlVector< CTFPlayer * > vecPlayers;
	CUtlVector< CTFPlayer * > vecRandomOrderPlayers;

	CollectPlayers( &vecPlayers, iTeam );

	FOR_EACH_VEC( vecPlayers, i )
	{
		CTFPlayer *pPlayer = vecPlayers[i];

		// Skip players not on a team or who have not chosen a class
		if ( ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
			|| pPlayer->IsPlayerClass( TF_CLASS_UNDEFINED ) )
			continue;

		vecRandomOrderPlayers.AddToTail( pPlayer );
	}

	// Randomize the order so players dont go to the same spot every time
	vecRandomOrderPlayers.Shuffle();

	string_t sName = MAKE_STRING( pszEntTargetName );
	int iCachedLocationIndex = m_mapTeleportLocations.Find( sName );
	CUtlVector< TeleportLocation_t > *pCachedLocations = NULL;
	// is there any cached entities to teleport to
	if ( m_mapTeleportLocations.IsValidIndex( iCachedLocationIndex ) )
	{
		pCachedLocations = m_mapTeleportLocations[iCachedLocationIndex];
	}

	
	int iCurrentTeleportLocation = 0;
	CBaseEntity *pSpawnPoint = NULL;
	FOR_EACH_VEC_BACK( vecRandomOrderPlayers, i )
	{
		// don't do anything if we run out of spawn point
		Vector vTeleportPosition;
		QAngle qTeleportAngles;

		// if we have cached locations, use them
		if ( pCachedLocations )
		{
			Assert( iCurrentTeleportLocation < pCachedLocations->Count() );
			if ( iCurrentTeleportLocation < pCachedLocations->Count() )
			{
				const TeleportLocation_t& location = pCachedLocations->Element( iCurrentTeleportLocation );
				vTeleportPosition = location.m_vecPosition;
				qTeleportAngles = location.m_qAngles;
				iCurrentTeleportLocation++;
			}
			else
			{
				// we need to add more teleport location in the map for players to teleport to
				continue;
			}
		}
		else // use old search for entities by name
		{
			pSpawnPoint = gEntList.FindEntityByName( pSpawnPoint, pszEntTargetName );

			Assert( pSpawnPoint );
			if ( pSpawnPoint )
			{
				vTeleportPosition = pSpawnPoint->GetAbsOrigin();
				qTeleportAngles = pSpawnPoint->GetAbsAngles();
			}
			else
			{
				// we need to add more teleport location in the map for players to teleport to
				continue;
			}
		}

		CTFPlayer *pPlayer = vecRandomOrderPlayers[ i ];
		pPlayer->m_Shared.RemoveAllCond();
		
		// Respawn dead players
		if ( !pPlayer->IsAlive() )
		{
			pPlayer->ForceRespawn();
		}

		// Unzoom if we are a sniper zoomed!
		pPlayer->m_Shared.InstantlySniperUnzoom();

		// Teleport
		pPlayer->Teleport( &vTeleportPosition, &qTeleportAngles, &vec3_origin );
		pPlayer->SnapEyeAngles( qTeleportAngles );

		// Force client to update all view angles (including kart and taunt yaw)
		pPlayer->ForcePlayerViewAngles( qTeleportAngles );

		// fill in the teleported player vector
		if ( pTeleportedPlayers )
		{
			pTeleportedPlayers->AddToTail( pPlayer );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed)
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( pPlayer->GetTeamNumber() != iWinningTeam )
				{
					pPlayer->RemoveInvisibility();
//					pPlayer->RemoveDisguise();

					if ( pPlayer->HasTheFlag() )
					{
						pPlayer->DropFlag();
					}
				}

				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->GetTeamNumber() != iWinningTeam )
		{
			// Stop placing any carried objects or else they will float around
			// at our feet at the end of the round
			if( pObj->IsPlacing() )
			{
				pObj->StopPlacement();
			}

			// Disable sentry guns that the losing team has built
			if( pObj->GetType() == OBJ_SENTRYGUN )
			{
				pObj->SetDisabled( true );
			}
		}
	}

	if ( m_bForceMapReset )
	{
		m_iPrevRoundState = -1;
		m_iCurrentRoundState = -1;
		m_iCurrentMiniRoundMask = 0;
	}

	if ( IsInStopWatch() == true && GetStopWatchTimer() )
	{
		variant_t sVariant;
		GetStopWatchTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );

		if ( m_bForceMapReset )
		{
			if ( GetStopWatchTimer()->IsWatchingTimeStamps() == true )
			{
				m_flStopWatchTotalTime = GetStopWatchTimer()->GetStopWatchTotalTime();
			}
			else
			{
				ShouldResetScores( true, false );
				UTIL_Remove( m_hStopWatchTimer	);
				m_hStopWatchTimer = NULL;
				m_flStopWatchTotalTime = -1.0f;
				m_bStopWatch = false;
				m_nStopWatchState.Set( STOPWATCH_CAPTURE_TIME_NOT_SET );
			}
		}
	}

	if ( GetHalloweenScenario() == HALLOWEEN_SCENARIO_VIADUCT )
	{
		// send everyone to the underworld!
		BroadcastSound( 255, "Halloween.PlayerEscapedUnderworld" );

		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

		CUtlVector< CBaseEntity * > spawnVector;

		CBaseEntity *spawnPoint = NULL;
		while( ( spawnPoint = gEntList.FindEntityByClassname( spawnPoint, "info_target" ) ) != NULL )
		{
			if ( FStrEq( STRING( spawnPoint->GetEntityName() ), "spawn_warcrimes" ) )
			{
				spawnVector.AddToTail( spawnPoint );
			}
		}

		if ( spawnVector.Count() > 0 )
		{
			// shuffle the order of the spawns
			int n = spawnVector.Count();
			while( n > 1 )
			{
				int k = RandomInt( 0, n-1 );
				n--;

				CBaseEntity *tmp = spawnVector[n];
				spawnVector[n] = spawnVector[k];
				spawnVector[k] = tmp;
			}

			color32 fadeColor = { 255, 255, 255, 100 };

			// send players to the underworld
			for( int i=0; i<playerVector.Count(); ++i )
			{
				CTFPlayer *player = playerVector[i];

				player->SetLocalOrigin( spawnVector[n]->GetAbsOrigin() + Vector( 0, 0, 20.0f ) );
				player->SetAbsVelocity( vec3_origin );
				player->SetLocalAngles( spawnVector[n]->GetAbsAngles() );
				player->m_Local.m_vecPunchAngle = vec3_angle;
				player->m_Local.m_vecPunchAngleVel = vec3_angle;
				player->SnapEyeAngles( spawnVector[n]->GetAbsAngles() );

				// give them full health since purgatory damages them over time
				player->SetHealth( player->GetMaxHealth() );

				UTIL_ScreenFade( player, fadeColor, 0.25, 0.4, FFADE_IN );

				n = ( n + 1 ) % spawnVector.Count();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper function for scramble teams
//-----------------------------------------------------------------------------
int FindScoreDifferenceBetweenTeams( CUtlVector< CTFPlayer* > &vecSource, CTFPlayerResource *pPR, int &nRedScore, int &nBlueScore )
{
	if ( !pPR )
		return false;

	nRedScore = 0;
	nBlueScore = 0;

	FOR_EACH_VEC( vecSource, i )
	{
		if ( !vecSource[i] )
			continue;

		if ( vecSource[i]->GetTeamNumber() == TF_TEAM_RED )
		{
			nRedScore += pPR->GetTotalScore( vecSource[i]->entindex() );
		}
		else
		{
			nBlueScore += pPR->GetTotalScore( vecSource[i]->entindex() );
		}
	}

	return abs( nRedScore - nBlueScore );
}

//-----------------------------------------------------------------------------
// Purpose: Helper function for scramble teams
//-----------------------------------------------------------------------------
bool FindAndSwapPlayersToBalanceTeams( CUtlVector< CTFPlayer* > &vecSource, int &nDelta, CTFPlayerResource *pPR )
{
	if ( !pPR )
		return false;

	int nTeamScoreRed = 0;
	int nTeamScoreBlue = 0;
	FindScoreDifferenceBetweenTeams( vecSource, pPR, nTeamScoreRed, nTeamScoreBlue );

	FOR_EACH_VEC( vecSource, i )
	{
		if ( !vecSource[i] )
			continue;

		if ( vecSource[i]->GetTeamNumber() != TF_TEAM_RED )
			continue;

		// Check against players on the other team
		FOR_EACH_VEC( vecSource, j )
		{
			if ( !vecSource[j] )
				continue;

			if ( vecSource[j]->GetTeamNumber() != TF_TEAM_BLUE )
				continue;

			if ( vecSource[i] == vecSource[j] )
				continue;

			int nRedPlayerScore = pPR->GetTotalScore( vecSource[i]->entindex() );
			int nBluePlayerScore = pPR->GetTotalScore( vecSource[j]->entindex() );

			int nPlayerDiff = abs( nRedPlayerScore - nBluePlayerScore );
			if ( nPlayerDiff )
			{
				int nNewRedScore = nTeamScoreRed;
				int nNewBlueScore = nTeamScoreBlue;

				if ( nRedPlayerScore > nBluePlayerScore )
				{
					nNewRedScore -= nPlayerDiff;
					nNewBlueScore += nPlayerDiff;
				}
				else
				{
					nNewRedScore += nPlayerDiff;
					nNewBlueScore -= nPlayerDiff;
				}

				int nNewDelta = abs( nNewRedScore - nNewBlueScore );
				if ( nNewDelta < nDelta )
				{
					// Swap and recheck
					vecSource[i]->ForceChangeTeam( TF_TEAM_BLUE );
					vecSource[j]->ForceChangeTeam( TF_TEAM_RED );

					nDelta = FindScoreDifferenceBetweenTeams( vecSource, pPR, nTeamScoreRed, nTeamScoreBlue );
					return true;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleScrambleTeams( void )
{
	static CUtlVector< CTFPlayer* > playerVector;
	playerVector.RemoveAll();

	CollectPlayers( &playerVector, TF_TEAM_RED );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, false, APPEND_PLAYERS );

	// Sort by player score.
	playerVector.Sort( ScramblePlayersSort );

	// Put everyone on Spectator to clear the teams (or the autoteam step won't work correctly)
	FOR_EACH_VEC_BACK( playerVector, i )
	{
		if ( !playerVector[i] )
		{
			playerVector.Remove( i );
			continue;
		}
		else if ( DuelMiniGame_IsInDuel( playerVector[i] ) ) // don't include them if they're in a duel
		{
			playerVector.Remove( i );
			continue;
		}

		playerVector[i]->ForceChangeTeam( TEAM_SPECTATOR );
	}

	// Assign players using the original, quick method.
	FOR_EACH_VEC( playerVector, i )
	{
		if ( !playerVector[i] )
			continue;

		if ( playerVector[i]->GetTeamNumber() >= FIRST_GAME_TEAM ) // are they already on a game team?
			continue;

		playerVector[i]->ForceChangeTeam( TF_TEAM_AUTOASSIGN );
	}

	// New method
	if ( playerVector.Count() > 2 )
	{
		CTFPlayerResource *pPR = dynamic_cast< CTFPlayerResource* >( g_pPlayerResource );
		if ( pPR )
		{
			int nTeamScoreRed = 0;
			int nTeamScoreBlue = 0;
			int nDelta = FindScoreDifferenceBetweenTeams( playerVector, pPR, nTeamScoreRed, nTeamScoreBlue );

#ifdef _DEBUG
			if ( mp_scrambleteams_debug.GetBool() )
			{
				DevMsg( "FIRST PASS -- Team1: %i || Team2: %i || Diff: %i\n", 
						nTeamScoreRed, 
						nTeamScoreBlue, 
						nDelta );
			}
#endif // _DEBUG

			// Try swapping players to bring scores closer
			if ( nDelta > 1 )
			{
				int nOrigValue = mp_teams_unbalance_limit.GetInt();
				mp_teams_unbalance_limit.SetValue( 0 );

				static const int nPassLimit = 8;
				for ( int i = 0; i < nPassLimit && FindAndSwapPlayersToBalanceTeams( playerVector, nDelta, pPR ); ++i )
				{
#ifdef _DEBUG
					if ( mp_scrambleteams_debug.GetBool() )
					{
						nTeamScoreRed = 0;
						nTeamScoreBlue = 0;
						DevMsg( "EXTRA PASS -- Team1: %i || Team2: %i || Diff: %i\n", 
								nTeamScoreRed, 
								nTeamScoreBlue, 
								FindScoreDifferenceBetweenTeams( playerVector, pPR, nTeamScoreRed, nTeamScoreBlue ) );
					}
#endif // _DEBUG
				}
			
				mp_teams_unbalance_limit.SetValue( nOrigValue );
			}
		}
	}

	// scrambleteams_auto tracking
	ResetTeamsRoundWinTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::TeamPlayerCountChanged( CTFTeam *pTeam )
{
	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->TeamPlayerCountChanged( pTeam );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Should we attempt to roll into a new match for the current match
//-----------------------------------------------------------------------------
bool CTFGameRules::BAttemptMapVoteRollingMatch()
{
	if ( IsManagedMatchEnded() )
		{ return false; }

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	return pMatchDesc && GTFGCClientSystem()->CanRequestNewMatchForLobby() && pMatchDesc->BUsesMapVoteAfterMatchEnds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::BIsManagedMatchEndImminent( void )
{
/*
	if ( IsCompetitiveMode() )
	{
		if ( State_Get() == GR_STATE_RND_RUNNING )
		{
			bool bPotentiallyTheFinalRound = ( CheckWinLimit( false, 1 ) || CheckMaxRounds( false, 1 ) );
			if ( bPotentiallyTheFinalRound )
			{
				bool bPlayingMiniRounds = ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] && g_hControlPointMasters[0]->PlayingMiniRounds() );

				switch( m_nGameType )
				{
				case TF_GAMETYPE_ESCORT:
				{
					if ( HasMultipleTrains() )
					{


					}
					else
					{



					}
				}
					break;
				case TF_GAMETYPE_CP:

					break;
				case TF_GAMETYPE_CTF:
					if ( tf_flag_caps_per_round.GetInt() > 0 )
					{
						for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; iTeam++ )
						{
							C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );
							if ( pTeam )
							{
								if ( ( tf_flag_caps_per_round.GetInt() - pTeam->GetFlagCaptures() ) <= 1 )
								{
									CCaptureFlag *pFlag = NULL;
									for ( int iFlag = 0; iFlag < ICaptureFlagAutoList::AutoList().Count(); ++iFlag )
									{
										pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[iFlag] );
										if ( !pFlag->IsDisabled() && ( pFlag->GetTeamNumber() == iTeam ) && !pFlag->IsHome() )
											return true;
									}
								}
							}
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}*/

	return false;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CTFGameRules::PowerupTeamImbalance_PlayerChangeTeam( CTFPlayer *pTFPlayer, int nTeam )
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetLiveMatch();
	if ( pMatch )
	{
		CSteamID steamID;
		pTFPlayer->GetSteamID( &steamID );
		GTFGCClientSystem()->ChangeMatchPlayerTeam( steamID, TFGameRules()->GetGCTeamForGameTeam( nTeam ) );
	}

	pTFPlayer->ChangeTeam( nTeam, false, false, true );
	pTFPlayer->ForceRespawn();
	pTFPlayer->SetLastAutobalanceTime( gpGlobals->curtime );

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
	if ( event )
	{
		event->SetInt( "player", pTFPlayer->entindex() );
		event->SetInt( "team", nTeam );
		gameeventmanager->FireEvent( event );
	}

	// tell people that we've switched this player
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", pTFPlayer->GetPlayerName() );
}

//-----------------------------------------------------------------------------
// Purpose: Swaps the highest scorer on the dominating team with the lowest scorer on the other team
//-----------------------------------------------------------------------------
void CTFGameRules::PowerupTeamImbalance_SwapPlayers( int nLosingTeam )
{
	CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );
	if ( !pResource )
		return;

	int nDominatingTeam = ( nLosingTeam == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
	CTFTeam *pLosingTeam = TFTeamMgr()->GetTeam( nLosingTeam );
	CTFTeam *pDominatingTeam = TFTeamMgr()->GetTeam( nDominatingTeam );
	if ( !pLosingTeam || !pDominatingTeam )
		return;

	int nTopScore = -1;
	CTFPlayer *pDominatingTarget = nullptr;
	for ( int i = 0; i < pDominatingTeam->GetNumPlayers(); ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pDominatingTeam->GetPlayer( i ) );
		if ( pPlayer )
		{
			int nScore = pPlayer->m_nMannpowerKills;
			if ( nScore > nTopScore )
			{
				pDominatingTarget = pPlayer;
				nTopScore = nScore;
			}
		}
	}

	int nBottomScore = 99999;
	CTFPlayer *pLosingTarget = nullptr;
	for ( int i = 0; i < pLosingTeam->GetNumPlayers(); ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pLosingTeam->GetPlayer( i ) );
		if ( pPlayer )
		{
			int nScore = pResource->GetTotalScore( pPlayer->entindex() );
			if ( nScore < nBottomScore )
			{
				pLosingTarget = pPlayer;
				nBottomScore = nScore;
			}
		}
	}

	if ( pDominatingTarget && pLosingTarget )
	{
		PowerupTeamImbalance_PlayerChangeTeam( pDominatingTarget, nLosingTeam );
		PowerupTeamImbalance_PlayerChangeTeam( pLosingTarget, nDominatingTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PowerupTeamImbalance( int nTeam )
{
	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->PowerupTeamImbalance( nTeam );

		if ( nTeam == TEAM_UNASSIGNED )
		{
			m_bPowerupImbalanceMeasuresRunning = false;
			m_flLastPowerUpImbalanceTime = gpGlobals->curtime; // store the last time an imbalance period ended
			m_flPowerUpImbalanceVictimTeamTime = gpGlobals->curtime; // we don't reset this if a team player swap isn't done
		}
		else
		{
			m_bPowerupImbalanceMeasuresRunning = true;
			m_flTimeToStopImbalanceMeasures = gpGlobals->curtime + m_flTimeToRunImbalanceMeasures;
			
			BroadcastSound( nTeam, "Announcer.Powerup.Volume.Starting" );
 			CTeamRecipientFilter filter( nTeam, true );
			UTIL_ClientPrintFilter( filter, HUD_PRINTCENTER, "#TF_Powerupvolume_Available" );

			CUtlVector< CTFPlayer* > playerVector;
			CollectPlayers( &playerVector, TF_TEAM_RED );
			CollectPlayers( &playerVector, TF_TEAM_BLUE, false, APPEND_PLAYERS );
			if ( playerVector.Count() >= tf_powerup_mode_imbalance_consecutive_min_players.GetInt() )
			{
				// if this is the second consecutive imbalance period for this team within the tf_powerup_mode_imbalance_consecutive_time
				// time period, let's try to switch the top player on the winning team to the losing team
				if ( m_nLastPowerUpImbalanceTeam == nTeam )
				{
					if ( gpGlobals->curtime - m_flLastPowerUpImbalanceTime < tf_powerup_mode_imbalance_consecutive_time.GetFloat() )
					{
						PowerupTeamImbalance_SwapPlayers( nTeam );
						m_nPowerUpImbalanceVictimTeam = TEAM_UNASSIGNED; // no victim team now because team player swap happened
						m_flPowerUpImbalanceVictimTeamTime = -1.f;
					}
				}
				else
				{
					m_nLastPowerUpImbalanceTeam = nTeam;
					m_nPowerUpImbalanceVictimTeam = nTeam;
				}
			}
			else
			{
				// reset everything (prevents initiating another team swap if three imbalance events are triggered in a row )
				m_nLastPowerUpImbalanceTeam = TEAM_UNASSIGNED;
				m_flLastPowerUpImbalanceTime = -1.f;
				// a team has been victimized enough to receive the imbalance powerup, but no player swap happened so they might still be weaker. 
				// We skip dominant tests on individual players on the victim team
				m_nPowerUpImbalanceVictimTeam = nTeam; 
			}
		}
		
		m_nPowerupKillsBlueTeam = 0;		// Reset both scores
		m_nPowerupKillsRedTeam = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Restrict team human players can join
//-----------------------------------------------------------------------------
int CTFGameRules::GetAssignedHumanTeam( void )
{
	if ( FStrEq( "blue", mp_humans_must_join_team.GetString() ) )
	{
		return TF_TEAM_BLUE;
	}
	else if ( FStrEq( "red", mp_humans_must_join_team.GetString() ) )
	{
		return TF_TEAM_RED;
	}
	else if ( FStrEq( "spectator", mp_humans_must_join_team.GetString() ) )
	{
		return TEAM_SPECTATOR;
	}
	else
	{
		return TEAM_ANY;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleSwitchTeams( void )
{
	if ( IsPVEModeActive() )
		return;

	m_bTeamsSwitched.Set( !m_bTeamsSwitched );

	// switch this as well
	if ( FStrEq( mp_humans_must_join_team.GetString(), "blue" ) )
	{
		mp_humans_must_join_team.SetValue( "red" );
	}
	else if ( FStrEq( mp_humans_must_join_team.GetString(), "red" ) )
	{
		mp_humans_must_join_team.SetValue( "blue" );
	}

	int i = 0;

	// remove everyone's projectiles and objects
	RemoveAllProjectilesAndBuildings();

	// respawn the players
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_BLUE, true );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_RED, true );
			}
		}
	}

	// switch the team scores
	CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pRedTeam && pBlueTeam )
	{
		int nRed = pRedTeam->GetScore();
		int nBlue = pBlueTeam->GetScore();

		pRedTeam->SetScore( nBlue );
		pBlueTeam->SetScore( nRed );

		if ( IsInTournamentMode() == true )
		{
			char szBlueName[16];
			char szRedName[16];

			Q_strncpy( szBlueName, mp_tournament_blueteamname.GetString(), sizeof ( szBlueName ) );
			Q_strncpy( szRedName, mp_tournament_redteamname.GetString(), sizeof ( szRedName ) );

			mp_tournament_redteamname.SetValue( szBlueName );
			mp_tournament_blueteamname.SetValue( szRedName );
		}
	}

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_TeamsSwitched" );

	CMatchInfo *pMatchInfo = GTFGCClientSystem()->GetMatch();
	if ( pMatchInfo )
	{
		CTFPlayerResource *pTFResource = dynamic_cast< CTFPlayerResource* >( g_pPlayerResource );
		if ( pTFResource )
		{
			uint32 unEventTeamStatus = pTFResource->GetEventTeamStatus();

			if ( unEventTeamStatus && m_bTeamsSwitched )
			{
				const uint32 unInvadersArePyro = 1u;
				const uint32 unInvadersAreHeavy = 2u;
				unEventTeamStatus = ( unEventTeamStatus == unInvadersArePyro ) ? unInvadersAreHeavy : unInvadersArePyro;
			}

			pMatchInfo->m_unEventTeamStatus = unEventTeamStatus;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangeClassInStalemate( void ) 
{ 
	return (gpGlobals->curtime < (m_flStalemateStartTime + tf_stalematechangeclasstime.GetFloat())); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangeTeam( int iCurrentTeam ) const
{
	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		if ( ( iCurrentTeam == TF_TEAM_RED ) || ( iCurrentTeam == TF_TEAM_BLUE ) )
		{
			return !ArePlayersInHell();
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetRoundOverlayDetails( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( pMaster && pMaster->PlayingMiniRounds() )
	{
		CTeamControlPointRound *pRound = pMaster->GetCurrentRound();

		if ( pRound )
		{
			CHandle<CTeamControlPoint> pRedPoint = pRound->GetPointOwnedBy( TF_TEAM_RED );
			CHandle<CTeamControlPoint> pBluePoint = pRound->GetPointOwnedBy( TF_TEAM_BLUE );

			// do we have opposing points in this round?
			if ( pRedPoint && pBluePoint )
			{
				int iMiniRoundMask = ( 1<<pBluePoint->GetPointIndex() ) | ( 1<<pRedPoint->GetPointIndex() );
				SetMiniRoundBitMask( iMiniRoundMask );
			}
			else
			{
				SetMiniRoundBitMask( 0 );
			}

			SetCurrentRoundStateBitString();
		}
	}

	BaseClass::SetRoundOverlayDetails();
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a team should score for each captured point
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScorePerRound( void )
{ 
	bool bRetVal = true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->ShouldScorePerCapture() )
	{
		bRetVal = false;
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsValveMap( void )
{ 
	char szCurrentMap[MAX_MAP_NAME];
	Q_strncpy( szCurrentMap, STRING( gpGlobals->mapname ), sizeof( szCurrentMap ) );

	if ( ::IsValveMap( szCurrentMap ) )
	{
		return true;
	}

	return BaseClass::IsValveMap();
}

bool CTFGameRules::IsOfficialMap( void )
{ 
	char szCurrentMap[MAX_MAP_NAME];
	Q_strncpy( szCurrentMap, STRING( gpGlobals->mapname ), sizeof( szCurrentMap ) );

	if ( ::IsValveMap( szCurrentMap ) || ::IsCommunityMap( szCurrentMap ) )
	{
		return true;
	}

	return BaseClass::IsOfficialMap();
}

void CTFGameRules::PlayTrainCaptureAlert( CTeamControlPoint *pPoint, bool bFinalPointInMap )
{
	if ( !pPoint )
		return;
	
	if ( State_Get() != GR_STATE_RND_RUNNING )
		return;

	const char *pszAlert = TEAM_TRAIN_ALERT;

	// is this the final control point in the map?
	if ( bFinalPointInMap )
	{
		pszAlert = TEAM_TRAIN_FINAL_ALERT;
	}

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		if ( bFinalPointInMap )
		{
			int iWinningTeam = TEAM_UNASSIGNED;
			float flRedProgress = 0.0f, flBlueProgress = 0.0f;
			for ( int i = 0 ; i < ITFTeamTrainWatcher::AutoList().Count() ; ++i )
			{
				CTeamTrainWatcher *pTrainWatcher = static_cast< CTeamTrainWatcher* >( ITFTeamTrainWatcher::AutoList()[i] );
				if ( !pTrainWatcher->IsDisabled() )
				{
					if ( pTrainWatcher->GetTeamNumber() == TF_TEAM_RED )
					{
						flRedProgress = pTrainWatcher->GetTrainDistanceAlongTrack();
					}
					else
					{
						flBlueProgress = pTrainWatcher->GetTrainDistanceAlongTrack();
					}
				}
			}

			if ( flRedProgress > flBlueProgress )
			{
				iWinningTeam = TF_TEAM_RED;
			}
			else if ( flBlueProgress > flRedProgress )
			{
				iWinningTeam = TF_TEAM_BLUE;
			}

			if (  iWinningTeam != TEAM_UNASSIGNED ) 
			{
				int iRedLine, iBlueLine;
				iRedLine = ( iWinningTeam == TF_TEAM_RED ) ? HELLTOWER_VO_RED_NEAR_WIN : HELLTOWER_VO_RED_NEAR_LOSE;
				iBlueLine = ( iWinningTeam == TF_TEAM_BLUE ) ? HELLTOWER_VO_BLUE_NEAR_WIN : HELLTOWER_VO_BLUE_NEAR_LOSE;
				PlayHelltowerAnnouncerVO( iRedLine, iBlueLine );
			}
		}
		return;
	}

	CBroadcastRecipientFilter filter;
	pPoint->EmitSound( filter, pPoint->entindex(), pszAlert );
}

#endif  // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
	if ( iOwnedEnd == -1 )
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints-1;
	if ( iOwnedEnd != 0 )
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
	{
		// If we've hit a point we don't own, we're done
		if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
			break;

		if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
			continue;

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TeamMayCapturePoint( int iTeam, int iPointIndex ) 
{ 
	if ( !tf_caplinear.GetBool() )
		return true; 

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, 0 );

	// Points set to require themselves are always cappable 
	if ( iPointNeeded == iPointIndex )
		return true;

	if ( IsInKothMode() && IsInWaitingForPlayers() )
		return false;

	// Is the point locked?
	if ( ObjectiveResource()->GetCPLocked( iPointIndex ) )
		return false;

	// No required points specified? Require all previous points.
	if ( iPointNeeded == -1 )
	{
		if ( IsInArenaMode() == true )
		{

#ifdef CLIENT_DLL

			if ( m_flCapturePointEnableTime - 5.0f <= gpGlobals->curtime && State_Get() == GR_STATE_STALEMATE )
				return true;
#endif

			if ( m_flCapturePointEnableTime <= gpGlobals->curtime && State_Get() == GR_STATE_STALEMATE )
				return true;

			return false;
		}

		if ( !ObjectiveResource()->PlayingMiniRounds() )
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, false );
			return (abs(iFarthestPoint - iPointIndex) <= 1);
		}
		else
		{
			// No custom previous point, team must own all previous points in the current mini-round
			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for ( int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++ )
	{
		iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, iPrevPoint );
		if ( iPointNeeded != -1 )
		{
			if ( ObjectiveResource()->GetOwningTeam( iPointNeeded ) != iTeam )
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( !pTFPlayer )
	{
		return false;
	}

	// Disguised and invisible spies cannot capture points
	if ( pTFPlayer->m_Shared.IsStealthed() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );
		}
		return false;
	}
	if ( ( pTFPlayer->m_Shared.IsInvulnerable() || pTFPlayer->m_Shared.InCond( TF_COND_MEGAHEAL ) ) && !IsMannVsMachineMode() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return false;
	}

	if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}

		return false;
	}

	if ( pTFPlayer->m_Shared.IsControlStunned() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stunned" );
		}

		return false;
	}

	// spies disguised as the enemy team cannot capture points
 	if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pTFPlayer->m_Shared.GetDisguiseTeam() != pTFPlayer->GetTeamNumber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_disguised" );
		}
		return false;
	}

#ifdef GAME_DLL
	if ( IsInTraining() && pTFPlayer->IsBotOfType( TF_BOT_TYPE ) )
	{
		switch( GetGameType() )
		{
		case TF_GAMETYPE_CP:
			{
				// in training mode, bots cannot initiate a capture
				float flCapPerc = ObjectiveResource()->GetCPCapPercentage( iPointIndex );
				if ( flCapPerc <= 0.0f )
				{
					return false;
				}
				break;
			}

		case TF_GAMETYPE_ESCORT:
			{
				// in training mode, the player must push the cart for the first time
				// after that, bots can start it moving again

				// assume only one cart in the map
				CTeamTrainWatcher *watcher = NULL;
				while( ( watcher = dynamic_cast< CTeamTrainWatcher * >( gEntList.FindEntityByClassname( watcher, "team_train_watcher" ) ) ) != NULL )
				{
					if ( !watcher->IsDisabled() )
					{
						break;
					}
				}

				if ( watcher && !watcher->IsDisabled() )
				{
					return !watcher->IsTrainAtStart();
				}
				break;
			}
		}
	}

#ifdef TF_CREEP_MODE
	if ( IsCreepWaveMode() )
	{
		CTFBot *bot = ToTFBot( pTFPlayer );

		if ( !bot || !bot->HasAttribute( CTFBot::IS_NPC ) )
		{
			// only creeps can capture points
			return false;
		}
	}
#endif

#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

#ifdef GAME_DLL
#ifdef TF_CREEP_MODE
	if ( IsCreepWaveMode() )
	{
		CTFBot *bot = ToTFBot( pTFPlayer );

		if ( !bot || !bot->HasAttribute( CTFBot::IS_NPC ) )
		{
			// only creeps can block points
			return false;
		}
	}
#endif
#endif

	// Invuln players can block points
	if ( pTFPlayer->m_Shared.IsInvulnerable() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTFGameRules::CalcPlayerScore( RoundStats_t *pRoundStats, CTFPlayer *pPlayer )
{
	Assert( pRoundStats );
	if ( !pRoundStats )
		return 0;

	// defensive fix for the moment for bug where healing value becomes bogus sometimes: if bogus, slam it to 0
	int iHealing = pRoundStats->m_iStat[TFSTAT_HEALING];
	Assert( iHealing >= 0 );
	Assert( iHealing <= 10000000 );
	if ( iHealing < 0 || iHealing > 10000000 )
	{
		iHealing = 0;
	}

	bool bMvM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();
	bool bPowerupMode = TFGameRules() && TFGameRules()->IsPowerupMode();

	int iScore =	( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL ) + 
					( pRoundStats->m_iStat[TFSTAT_KILLS_RUNECARRIER] * TF_SCORE_KILL_RUNECARRIER ) + // Kill someone who is carrying a rune
					( pRoundStats->m_iStat[TFSTAT_CAPTURES] * ( ( bPowerupMode ) ? TF_SCORE_CAPTURE_POWERUPMODE : TF_SCORE_CAPTURE ) ) +
					( pRoundStats->m_iStat[TFSTAT_FLAGRETURNS] * TF_SCORE_FLAG_RETURN ) +
					( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TF_SCORE_DEFEND ) + 
					( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TF_SCORE_DESTROY_BUILDING ) + 
					( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] / TF_SCORE_HEADSHOT_DIVISOR ) + 
					( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TF_SCORE_BACKSTAB ) + 
					( iHealing / ( ( bMvM ) ? TF_SCORE_DAMAGE : TF_SCORE_HEAL_HEALTHUNITS_PER_POINT ) ) +  // MvM values healing more than PvP
					( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TF_SCORE_KILL_ASSISTS_PER_POINT ) + 
					( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TF_SCORE_TELEPORTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_INVULNS] / TF_SCORE_INVULN ) +
					( pRoundStats->m_iStat[TFSTAT_REVENGE] / TF_SCORE_REVENGE ) +
					( pRoundStats->m_iStat[TFSTAT_BONUS_POINTS] / TF_SCORE_BONUS_POINT_DIVISOR ) +
					( pRoundStats->m_iStat[TFSTAT_CURRENCY_COLLECTED] / TF_SCORE_CURRENCY_COLLECTED );

	if ( pPlayer )
	{
		int nScoreboardMinigame = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nScoreboardMinigame, scoreboard_minigame );
		if ( nScoreboardMinigame > 0 )
		{
			// Increment Score
			iScore +=
				( pRoundStats->m_iStat[TFSTAT_KILLS] ) + 
				( pRoundStats->m_iStat[TFSTAT_CAPTURES] ) + 
				( pRoundStats->m_iStat[TFSTAT_DEFENSES] ) + 
				( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] );

			// Subtract Deaths
			iScore -= pRoundStats->m_iStat[TFSTAT_DEATHS] * 3;
		}
	}

	// Previously MvM-only
	const int nDivisor = ( bMvM ) ? TF_SCORE_DAMAGE : TF_SCORE_HEAL_HEALTHUNITS_PER_POINT;
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE_ASSIST] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE_BOSS] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_HEALING_ASSIST] / nDivisor );
	iScore += ( pRoundStats->m_iStat[TFSTAT_DAMAGE_BLOCKED] / nDivisor );

	return Max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int	CTFGameRules::CalcPlayerSupportScore( RoundStats_t *pRoundStats, int iPlayerIdx )
{
#ifdef GAME_DLL
	Assert( pRoundStats );
	if ( !pRoundStats )
		return 0;

	return ( pRoundStats->m_iStat[TFSTAT_DAMAGE_ASSIST] +
		   pRoundStats->m_iStat[TFSTAT_HEALING_ASSIST] +
		   pRoundStats->m_iStat[TFSTAT_DAMAGE_BLOCKED] +
		   ( pRoundStats->m_iStat[TFSTAT_BONUS_POINTS] * 25 ) );
#else
	Assert( g_TF_PR );
	if ( !g_TF_PR )
		return 0;

	return	g_TF_PR->GetDamageAssist( iPlayerIdx ) +
			g_TF_PR->GetHealingAssist( iPlayerIdx ) +
			g_TF_PR->GetDamageBlocked( iPlayerIdx ) +
			( g_TF_PR->GetBonusPoints( iPlayerIdx ) * 25 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsBirthday( void ) const
{
	if ( IsX360() )
		return false;

	return tf_birthday.GetBool() || IsHolidayActive( kHoliday_TFBirthday );
}

bool CTFGameRules::IsBirthdayOrPyroVision( void ) const
{
	if ( IsBirthday() )
		return true;

#ifdef CLIENT_DLL
	// Use birthday fun if the local player has an item that allows them to see it (Pyro Goggles)
	if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) )
	{
		return true;
	}
#endif

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsHolidayActive( /*EHoliday*/ int eHoliday ) const
{
	//if ( IsPVEModeActive() )
	//	return false;

	return TF_IsHolidayActive( eHoliday );
}

#ifndef GAME_DLL
void CTFGameRules::SetUpVisionFilterKeyValues( void )
{
	m_pkvVisionFilterShadersMapWhitelist = new KeyValues( "VisionFilterShadersMapWhitelist" );
	m_pkvVisionFilterShadersMapWhitelist->LoadFromFile( g_pFullFileSystem, "cfg/mtp.cfg", "MOD" );

	m_pkvVisionFilterTranslations = new KeyValues( "VisionFilterTranslations" );

	// **************************************************************************************************
	// PARTICLES
	KeyValues *pKVBlock = new KeyValues( "particles" );
	m_pkvVisionFilterTranslations->AddSubKey( pKVBlock );

	// No special vision
	KeyValues *pKVFlag = new KeyValues( "0" );
	pKVFlag->SetString( "flamethrower_rainbow_new_flame", "new_flame" );						
	//pKVFlag->SetString( "flamethrower_rainbow_FP", "flamethrower" );
	pKVBlock->AddSubKey( pKVFlag );

	// Pyrovision
	pKVFlag = new KeyValues( "1" );		//TF_VISION_FILTER_PYRO
	pKVFlag->SetString( "new_flame", "flamethrower_rainbow_new_flame");							// We weren't changing the default flamethrower previously; let's do that going forward
	pKVFlag->SetString( "flamethrower_rainbow_new_flame", "flamethrower_rainbow_new_flame" );	// Rainblower defaults to rainbows and we want to ensure that we use it
	//pKVFlag->SetString( "flamethrower_rainbow_FP", "flamethrower_rainbow_FP" );
	pKVFlag->SetString( "projectile_fireball", "projectile_fireball_pyrovision" );
	pKVFlag->SetString( "taunt_pyro_gasblast_fireblast", "taunt_pyro_gasblast_rainbow" );
	pKVFlag->SetString( "burningplayer_blue", "burningplayer_rainbow_blue" );
	pKVFlag->SetString( "burningplayer_red", "burningplayer_rainbow_red" );
	pKVFlag->SetString( "burningplayer_corpse", "burningplayer_corpse_rainbow" );
	pKVFlag->SetString( "water_blood_impact_red_01", "pyrovision_blood" );
	pKVFlag->SetString( "blood_impact_red_01", "pyrovision_blood" );
	pKVFlag->SetString( "blood_spray_red_01", "pyrovision_blood" );
	pKVFlag->SetString( "blood_spray_red_01_far", "pyrovision_blood" );
	pKVFlag->SetString( "ExplosionCore_wall", "pyrovision_explosion" );
	pKVFlag->SetString( "ExplosionCore_buildings", "pyrovision_explosion" );
	pKVFlag->SetString( "ExplosionCore_MidAir", "pyrovision_explosion" );
	pKVFlag->SetString( "rockettrail", "pyrovision_rockettrail" );
	pKVFlag->SetString( "rockettrail_!", "pyrovision_rockettrail" );
	pKVFlag->SetString( "sentry_rocket", "pyrovision_rockettrail" );
	pKVFlag->SetString( "flaregun_trail_blue", "pyrovision_flaregun_trail_blue" );
	pKVFlag->SetString( "flaregun_trail_red", "pyrovision_flaregun_trail_red" );
	pKVFlag->SetString( "flaregun_trail_crit_blue", "pyrovision_flaregun_trail_crit_blue" );
	pKVFlag->SetString( "flaregun_trail_crit_red", "pyrovision_flaregun_trail_crit_red" );
	pKVFlag->SetString( "flaregun_destroyed", "pyrovision_flaregun_destroyed" );
	pKVFlag->SetString( "scorchshot_trail_blue", "pyrovision_scorchshot_trail_blue" );
	pKVFlag->SetString( "scorchshot_trail_red", "pyrovision_scorchshot_trail_red" );
	pKVFlag->SetString( "scorchshot_trail_crit_blue", "pyrovision_scorchshot_trail_crit_blue" );
	pKVFlag->SetString( "scorchshot_trail_crit_red", "pyrovision_scorchshot_trail_crit_red" );
	pKVFlag->SetString( "ExplosionCore_MidAir_Flare", "pyrovision_flaregun_destroyed" );
	pKVFlag->SetString( "pyrotaunt_rainbow_norainbow", "pyrotaunt_rainbow" );
	pKVFlag->SetString( "pyrotaunt_rainbow_bubbles_flame", "pyrotaunt_rainbow_bubbles" );
	pKVFlag->SetString( "v_flaming_arrow", "pyrovision_v_flaming_arrow" );
	pKVFlag->SetString( "flaming_arrow", "pyrovision_flaming_arrow" );
	pKVFlag->SetString( "flying_flaming_arrow", "pyrovision_flying_flaming_arrow" );
	pKVFlag->SetString( "taunt_pyro_balloon", "taunt_pyro_balloon_vision" );
	pKVFlag->SetString( "taunt_pyro_balloon_explosion", "taunt_pyro_balloon_explosion_vision" );

	pKVBlock->AddSubKey( pKVFlag );

	//// Spooky Vision
	//pKVFlag = new KeyValues( "2" );	//TF_VISION_FILTER_HALLOWEEN
	//pKVBlock->AddSubKey( pKVFlag );

	// **************************************************************************************************
	// SOUNDS
	pKVBlock = new KeyValues( "sounds" );
	m_pkvVisionFilterTranslations->AddSubKey( pKVBlock );

	// No special vision
	pKVFlag = new KeyValues( "0" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_start.wav", "weapons/flame_thrower_start.wav" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_loop.wav", "weapons/flame_thrower_loop.wav" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_end.wav", "weapons/flame_thrower_end.wav" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_hit.wav", "weapons/flame_thrower_fire_hit.wav" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_pilot.wav", "weapons/flame_thrower_pilot.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_start.wav", ")weapons/flame_thrower_start.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_loop.wav", ")weapons/flame_thrower_loop.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_end.wav", ")weapons/flame_thrower_end.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_hit.wav", ")weapons/flame_thrower_fire_hit.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_pilot.wav", "weapons/flame_thrower_pilot.wav" );
	pKVFlag->SetString( "Weapon_Rainblower.Fire", "Weapon_FlameThrower.Fire" );
	pKVFlag->SetString( "Weapon_Rainblower.FireLoop", "Weapon_FlameThrower.FireLoop" );
	pKVFlag->SetString( "Weapon_Rainblower.WindDown", "Weapon_FlameThrower.WindDown" );
	pKVFlag->SetString( "Weapon_Rainblower.FireHit", "Weapon_FlameThrower.FireHit" );
	pKVFlag->SetString( "Weapon_Rainblower.PilotLoop", "Weapon_FlameThrower.PilotLoop" );
	pKVFlag->SetString( "Taunt.PyroBalloonicorn", "Taunt.PyroHellicorn" );
	pKVFlag->SetString( ")items/pyro_music_tube.wav", "common/null.wav" );
	pKVBlock->AddSubKey( pKVFlag );

	// Pyrovision
	pKVFlag = new KeyValues( "1" ); //TF_VISION_FILTER_PYRO
	pKVFlag->SetString( "weapons/rainblower/rainblower_start.wav", "weapons/rainblower/rainblower_start.wav" );		// If we're in PyroVision, explicitly set to ensure this is saved
	pKVFlag->SetString( "weapons/rainblower/rainblower_loop.wav", "weapons/rainblower/rainblower_loop.wav" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_end.wav", "weapons/rainblower/rainblower_end.wav" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_hit.wav", "weapons/rainblower/rainblower_hit.wav" );
	pKVFlag->SetString( "weapons/rainblower/rainblower_pilot.wav", "weapons/rainblower/rainblower_pilot.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_start.wav", ")weapons/rainblower/rainblower_start.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_loop.wav", ")weapons/rainblower/rainblower_loop.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_end.wav", ")weapons/rainblower/rainblower_end.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_hit.wav", ")weapons/rainblower/rainblower_hit.wav" );
	pKVFlag->SetString( ")weapons/rainblower/rainblower_pilot.wav", ")weapons/rainblower/rainblower_pilot.wav" );
	pKVFlag->SetString( "Weapon_Rainblower.Fire", "Weapon_Rainblower.Fire" );
	pKVFlag->SetString( "Weapon_Rainblower.FireLoop", "Weapon_Rainblower.FireLoop" );
	pKVFlag->SetString( "Weapon_Rainblower.WindDown", "Weapon_Rainblower.WindDown" );
	pKVFlag->SetString( "Weapon_Rainblower.FireHit", "Weapon_Rainblower.FireHit" );
	pKVFlag->SetString( "Weapon_Rainblower.PilotLoop", "Weapon_Rainblower.PilotLoop" );
	pKVFlag->SetString( "Taunt.PyroBalloonicorn", "Taunt.PyroBalloonicorn" );
	pKVFlag->SetString( ")items/pyro_music_tube.wav", ")items/pyro_music_tube.wav" );
	pKVFlag->SetString( "Taunt.Party_Trick", "Taunt.Party_Trick_Pyro_Vision" );
	pKVFlag->SetString( "Taunt.GasBlast", "Taunt.GasBlastPyrovision" );

	pKVFlag->SetString( "vo/demoman_PainCrticialDeath01.mp3", "vo/demoman_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/demoman_PainCrticialDeath02.mp3", "vo/demoman_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/demoman_PainCrticialDeath03.mp3", "vo/demoman_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/demoman_PainCrticialDeath04.mp3", "vo/demoman_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/demoman_PainCrticialDeath05.mp3", "vo/demoman_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSevere01.mp3", "vo/demoman_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSevere02.mp3", "vo/demoman_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSevere03.mp3", "vo/demoman_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSevere04.mp3", "vo/demoman_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSharp01.mp3", "vo/demoman_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSharp02.mp3", "vo/demoman_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSharp03.mp3", "vo/demoman_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSharp04.mp3", "vo/demoman_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSharp05.mp3", "vo/demoman_LaughShort05.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSharp06.mp3", "vo/demoman_LaughShort06.mp3" );
	pKVFlag->SetString( "vo/demoman_PainSharp07.mp3", "vo/demoman_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/demoman_AutoOnFire01.mp3", "vo/demoman_PositiveVocalization02.mp3" );
	pKVFlag->SetString( "vo/demoman_AutoOnFire02.mp3", "vo/demoman_PositiveVocalization03.mp3" );
	pKVFlag->SetString( "vo/demoman_AutoOnFire03.mp3", "vo/demoman_PositiveVocalization04.mp3" );

	pKVFlag->SetString( "vo/engineer_PainCrticialDeath01.mp3", "vo/engineer_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainCrticialDeath02.mp3", "vo/engineer_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/engineer_PainCrticialDeath03.mp3", "vo/engineer_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainCrticialDeath04.mp3", "vo/engineer_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/engineer_PainCrticialDeath05.mp3", "vo/engineer_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainCrticialDeath06.mp3", "vo/engineer_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSevere01.mp3", "vo/engineer_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSevere02.mp3", "vo/engineer_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSevere03.mp3", "vo/engineer_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSevere04.mp3", "vo/engineer_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSevere05.mp3", "vo/engineer_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSevere06.mp3", "vo/engineer_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSevere07.mp3", "vo/engineer_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp01.mp3", "vo/engineer_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp02.mp3", "vo/engineer_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp03.mp3", "vo/engineer_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp04.mp3", "vo/engineer_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp05.mp3", "vo/engineer_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp06.mp3", "vo/engineer_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp07.mp3", "vo/engineer_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/engineer_PainSharp08.mp3", "vo/engineer_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/engineer_AutoOnFire01.mp3", "vo/engineer_PositiveVocalization01.mp3" );
	pKVFlag->SetString( "vo/engineer_AutoOnFire02.mp3", "vo/engineer_Cheers01.mp3" );
	pKVFlag->SetString( "vo/engineer_AutoOnFire03.mp3", "vo/engineer_Cheers02.mp3" );

	pKVFlag->SetString( "vo/heavy_PainCrticialDeath01.mp3", "vo/heavy_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/heavy_PainCrticialDeath02.mp3", "vo/heavy_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/heavy_PainCrticialDeath03.mp3", "vo/heavy_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSevere01.mp3", "vo/heavy_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSevere02.mp3", "vo/heavy_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSevere03.mp3", "vo/heavy_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSharp01.mp3", "vo/heavy_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSharp02.mp3", "vo/heavy_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSharp03.mp3", "vo/heavy_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSharp04.mp3", "vo/heavy_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/heavy_PainSharp05.mp3", "vo/heavy_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/heavy_AutoOnFire01.mp3", "vo/heavy_PositiveVocalization01.mp3" );
	pKVFlag->SetString( "vo/heavy_AutoOnFire02.mp3", "vo/heavy_PositiveVocalization02.mp3" );
	pKVFlag->SetString( "vo/heavy_AutoOnFire03.mp3", "vo/heavy_PositiveVocalization03.mp3" );
	pKVFlag->SetString( "vo/heavy_AutoOnFire04.mp3", "vo/heavy_PositiveVocalization04.mp3" );
	pKVFlag->SetString( "vo/heavy_AutoOnFire05.mp3", "vo/heavy_PositiveVocalization05.mp3" );

	pKVFlag->SetString( "vo/medic_PainCrticialDeath01.mp3", "vo/medic_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/medic_PainCrticialDeath02.mp3", "vo/medic_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/medic_PainCrticialDeath03.mp3", "vo/medic_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/medic_PainCrticialDeath04.mp3", "vo/medic_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/medic_PainSevere01.mp3", "vo/medic_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/medic_PainSevere02.mp3", "vo/medic_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/medic_PainSevere03.mp3", "vo/medic_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/medic_PainSevere04.mp3", "vo/medic_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp01.mp3", "vo/medic_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp02.mp3", "vo/medic_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp03.mp3", "vo/medic_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp04.mp3", "vo/medic_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp05.mp3", "vo/medic_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp06.mp3", "vo/medic_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp07.mp3", "vo/medic_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/medic_PainSharp08.mp3", "vo/medic_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/medic_AutoOnFire01.mp3", "vo/medic_PositiveVocalization01.mp3" );
	pKVFlag->SetString( "vo/medic_AutoOnFire02.mp3", "vo/medic_PositiveVocalization02.mp3" );
	pKVFlag->SetString( "vo/medic_AutoOnFire03.mp3", "vo/medic_PositiveVocalization03.mp3" );
	pKVFlag->SetString( "vo/medic_AutoOnFire04.mp3", "vo/medic_PositiveVocalization04.mp3" );
	pKVFlag->SetString( "vo/medic_AutoOnFire05.mp3", "vo/medic_PositiveVocalization05.mp3" );

	pKVFlag->SetString( "vo/pyro_PainCrticialDeath01.mp3", "vo/pyro_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainCrticialDeath02.mp3", "vo/pyro_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainCrticialDeath03.mp3", "vo/pyro_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainSevere01.mp3", "vo/pyro_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainSevere02.mp3", "vo/pyro_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainSevere03.mp3", "vo/pyro_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainSevere04.mp3", "vo/pyro_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainSevere05.mp3", "vo/pyro_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/pyro_PainSevere06.mp3", "vo/pyro_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/pyro_AutoOnFire01.mp3", "vo/pyro_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/pyro_AutoOnFire02.mp3", "vo/pyro_LaughHappy01.mp3" );

	pKVFlag->SetString( "vo/scout_PainCrticialDeath01.mp3", "vo/scout_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/scout_PainCrticialDeath02.mp3", "vo/scout_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/scout_PainCrticialDeath03.mp3", "vo/scout_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/scout_PainSevere01.mp3", "vo/scout_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/scout_PainSevere02.mp3", "vo/scout_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/scout_PainSevere03.mp3", "vo/scout_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/scout_PainSevere04.mp3", "vo/scout_LaughHappy04.mp3" );
	pKVFlag->SetString( "vo/scout_PainSevere05.mp3", "vo/scout_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/scout_PainSevere06.mp3", "vo/scout_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp01.mp3", "vo/scout_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp02.mp3", "vo/scout_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp03.mp3", "vo/scout_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp04.mp3", "vo/scout_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp05.mp3", "vo/scout_LaughShort05.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp06.mp3", "vo/scout_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp07.mp3", "vo/scout_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/scout_PainSharp08.mp3", "vo/scout_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/scout_AutoOnFire01.mp3", "vo/scout_PositiveVocalization02.mp3" );
	pKVFlag->SetString( "vo/scout_AutoOnFire02.mp3", "vo/scout_PositiveVocalization03.mp3" );

	pKVFlag->SetString( "vo/sniper_PainCrticialDeath01.mp3", "vo/sniper_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/sniper_PainCrticialDeath02.mp3", "vo/sniper_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/sniper_PainCrticialDeath03.mp3", "vo/sniper_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/sniper_PainCrticialDeath04.mp3", "vo/sniper_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSevere01.mp3", "vo/sniper_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSevere02.mp3", "vo/sniper_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSevere03.mp3", "vo/sniper_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSevere04.mp3", "vo/sniper_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSharp01.mp3", "vo/sniper_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSharp02.mp3", "vo/sniper_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSharp03.mp3", "vo/sniper_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/sniper_PainSharp04.mp3", "vo/sniper_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/sniper_AutoOnFire01.mp3", "vo/sniper_PositiveVocalization02.mp3" );
	pKVFlag->SetString( "vo/sniper_AutoOnFire02.mp3", "vo/sniper_PositiveVocalization03.mp3" );
	pKVFlag->SetString( "vo/sniper_AutoOnFire03.mp3", "vo/sniper_PositiveVocalization05.mp3" );

	pKVFlag->SetString( "vo/soldier_PainCrticialDeath01.mp3", "vo/soldier_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/soldier_PainCrticialDeath02.mp3", "vo/soldier_LaughLong02.mp3" );
	pKVFlag->SetString( "vo/soldier_PainCrticialDeath03.mp3", "vo/soldier_LaughLong03.mp3" );
	pKVFlag->SetString( "vo/soldier_PainCrticialDeath04.mp3", "vo/soldier_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSevere01.mp3", "vo/soldier_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSevere02.mp3", "vo/soldier_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSevere03.mp3", "vo/soldier_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSevere04.mp3", "vo/soldier_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSevere05.mp3", "vo/soldier_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSevere06.mp3", "vo/soldier_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp01.mp3", "vo/soldier_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp02.mp3", "vo/soldier_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp03.mp3", "vo/soldier_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp04.mp3", "vo/soldier_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp05.mp3", "vo/soldier_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp06.mp3", "vo/soldier_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp07.mp3", "vo/soldier_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/soldier_PainSharp08.mp3", "vo/soldier_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/soldier_AutoOnFire01.mp3", "vo/soldier_PositiveVocalization01.mp3" );
	pKVFlag->SetString( "vo/soldier_AutoOnFire02.mp3", "vo/soldier_PositiveVocalization02.mp3" );
	pKVFlag->SetString( "vo/soldier_AutoOnFire03.mp3", "vo/soldier_PositiveVocalization03.mp3" );

	pKVFlag->SetString( "vo/spy_PainCrticialDeath01.mp3", "vo/spy_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/spy_PainCrticialDeath02.mp3", "vo/spy_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/spy_PainCrticialDeath03.mp3", "vo/spy_LaughLong01.mp3" );
	pKVFlag->SetString( "vo/spy_PainSevere01.mp3", "vo/spy_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/spy_PainSevere02.mp3", "vo/spy_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/spy_PainSevere03.mp3", "vo/spy_LaughHappy03.mp3" );
	pKVFlag->SetString( "vo/spy_PainSevere04.mp3", "vo/spy_LaughHappy01.mp3" );
	pKVFlag->SetString( "vo/spy_PainSevere05.mp3", "vo/spy_LaughHappy02.mp3" );
	pKVFlag->SetString( "vo/spy_PainSharp01.mp3", "vo/spy_LaughShort01.mp3" );
	pKVFlag->SetString( "vo/spy_PainSharp02.mp3", "vo/spy_LaughShort02.mp3" );
	pKVFlag->SetString( "vo/spy_PainSharp03.mp3", "vo/spy_LaughShort03.mp3" );
	pKVFlag->SetString( "vo/spy_PainSharp04.mp3", "vo/spy_LaughShort04.mp3" );
	pKVFlag->SetString( "vo/spy_AutoOnFire01.mp3", "vo/spy_PositiveVocalization02.mp3" );
	pKVFlag->SetString( "vo/spy_AutoOnFire02.mp3", "vo/spy_PositiveVocalization04.mp3" );
	pKVFlag->SetString( "vo/spy_AutoOnFire03.mp3", "vo/spy_PositiveVocalization05.mp3" );

	pKVBlock->AddSubKey( pKVFlag );

	// Spooky Vision
	//pKVFlag = new KeyValues( "2" ); // TF_VISION_FILTER_HALLOWEEN
	//pKVBlock->AddSubKey( pKVFlag );

	// **************************************************************************************************
	// WEAPONS
	pKVBlock = new KeyValues( "weapons" );
	m_pkvVisionFilterTranslations->AddSubKey( pKVBlock );

	// No special vision
	pKVFlag = new KeyValues( "0" );
	pKVFlag->SetString( "models/weapons/c_models/c_rainblower/c_rainblower.mdl", "models/weapons/c_models/c_flamethrower/c_flamethrower.mdl" );
	pKVFlag->SetString( "models/weapons/c_models/c_lollichop/c_lollichop.mdl", "models/weapons/w_models/w_fireaxe.mdl" );
	pKVBlock->AddSubKey( pKVFlag );

	// Pyrovision
	pKVFlag = new KeyValues( "1" );
	pKVFlag->SetString( "models/weapons/c_models/c_rainblower/c_rainblower.mdl", "models/weapons/c_models/c_rainblower/c_rainblower.mdl" );		//explicit set for pyrovision
	pKVFlag->SetString( "models/weapons/c_models/c_lollichop/c_lollichop.mdl", "models/weapons/c_models/c_lollichop/c_lollichop.mdl" );

	pKVFlag->SetString( "models/player/items/demo/demo_bombs.mdl", "models/player/items/all_class/mtp_bottle_demo.mdl" );
	pKVFlag->SetString( "models/player/items/pyro/xms_pyro_bells.mdl", "models/player/items/all_class/mtp_bottle_pyro.mdl" );
	pKVFlag->SetString( "models/player/items/soldier/ds_can_grenades.mdl", "models/player/items/all_class/mtp_bottle_soldier.mdl" );
	pKVBlock->AddSubKey( pKVFlag );

	// Spooky Vision
	//pKVFlag = new KeyValues( "2" );
	//pKVBlock->AddSubKey( pKVFlag );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::UseSillyGibs( void )
{
	// Use silly gibs if the local player has an item that allows them to see it (Pyro Goggles)
	if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) )
		return true;
	if ( UTIL_IsLowViolence() )
		return true;

	return m_bSillyGibs;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowWeatherParticles( void )
{
	return ( tf_particles_disable_weather.GetBool() == false );
}

bool CTFGameRules::AllowMapVisionFilterShaders( void )
{
	if ( !m_pkvVisionFilterShadersMapWhitelist )
		return false;

	const char *pMapName = engine->GetLevelName();
	while ( pMapName && pMapName[ 0 ] != '\\' && pMapName[ 0 ] != '/' )
	{
		pMapName++;
	}

	if ( !pMapName )
		return false;

	pMapName++;

	return  m_pkvVisionFilterShadersMapWhitelist->GetInt( pMapName, 0 ) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFGameRules::TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName )
{
	if ( !pchEffectType || !pchEffectName )
	{
		return pchEffectName;
	}

	CUtlVector<const char *> vecNames;
	vecNames.AddToTail( pchEffectName );

	// Swap the effect if the local player has an item that allows them to see it (Pyro Goggles)
	bool bWeaponsOnly = FStrEq( pchEffectType, "weapons" );
	int nVisionOptInFlags = GetLocalPlayerVisionFilterFlags( bWeaponsOnly );

	KeyValues *pkvParticles = m_pkvVisionFilterTranslations->FindKey( pchEffectType );
	if ( pkvParticles )
	{
		for ( KeyValues *pkvFlag = pkvParticles->GetFirstTrueSubKey(); pkvFlag != NULL; pkvFlag = pkvFlag->GetNextTrueSubKey() )
		{
			int nFlag = atoi( pkvFlag->GetName() );

			// Grab any potential translations for this item
			// Always grab default (No vision) names if they exist
			if ( ( nVisionOptInFlags & nFlag ) != 0 || nFlag == 0 )
			{
				// We either have this vision flag or we have no flags and this is the no flag block
				const char *pchTranslatedString = pkvFlag->GetString( pchEffectName, NULL );

				if ( pchTranslatedString )
				{
					vecNames.AddToHead( pchTranslatedString );
				}
			}
		}
	}

	// Return the top item in the list.  Currently Halloween replacements would out prioritize Pyrovision if there is any overlap
	return vecNames.Head();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ModifySentChat( char *pBuf, int iBufSize )
{
	if ( IsInMedievalMode() && tf_medieval_autorp.GetBool() && english.GetBool() )
	{
		AutoRP()->ApplyRPTo( pBuf, iBufSize );
	}

	// replace all " with ' to prevent exploits related to chat text
	// example:   ";exit
	for ( char *ch = pBuf; *ch != 0; ch++ )
	{
		if ( *ch == '"' )
		{
			*ch = '\'';
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowMapParticleEffect( const char *pszParticleEffect )
{
	static const char *s_WeatherEffects[] =
	{
		"tf_gamerules",
		"env_rain_001",
		"env_rain_002_256",
		"env_rain_ripples",
		"env_snow_light_001",
		"env_rain_gutterdrip",
		"env_rain_guttersplash",
		"", // END Marker
	};

	if ( !AllowWeatherParticles() )
	{
		if ( FindInList( s_WeatherEffects, pszParticleEffect ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::GetTeamGlowColor( int nTeam, float &r, float &g, float &b )
{
	if ( nTeam == TF_TEAM_RED )
	{
		r = 0.74f;
		g = 0.23f;
		b = 0.23f;
	}
	else if ( nTeam == TF_TEAM_BLUE )
	{
		r = 0.49f;
		g = 0.66f;
		b = 0.77f;
	}
	else
	{
		r = 0.76f;
		g = 0.76f;
		b = 0.76f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldConfirmOnDisconnect()
{
	// Add any game mode which uses matchmaking here. Note that the disconnect dialog checks if it should be showing
	// abandons and such.
	return GTFGCClientSystem()->BConnectedToMatchServer( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldShowPreRoundDoors() const
{
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		return pMatchDesc->BUsesPreRoundDoors();
	}

	return false;
}

#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetClassLimit( int iClass )
{
	if ( IsInTournamentMode() || IsPasstimeMode() )
	{
		switch ( iClass )
		{
		case TF_CLASS_SCOUT: return tf_tournament_classlimit_scout.GetInt(); break;
		case TF_CLASS_SNIPER: return tf_tournament_classlimit_sniper.GetInt(); break;
		case TF_CLASS_SOLDIER: return tf_tournament_classlimit_soldier.GetInt(); break;
		case TF_CLASS_DEMOMAN: return tf_tournament_classlimit_demoman.GetInt(); break;
		case TF_CLASS_MEDIC: return tf_tournament_classlimit_medic.GetInt(); break;
		case TF_CLASS_HEAVYWEAPONS: return tf_tournament_classlimit_heavy.GetInt(); break;
		case TF_CLASS_PYRO: return tf_tournament_classlimit_pyro.GetInt(); break;
		case TF_CLASS_SPY: return tf_tournament_classlimit_spy.GetInt(); break;
		case TF_CLASS_ENGINEER: return tf_tournament_classlimit_engineer.GetInt(); break;
		default:
			break;
		}
	}
	else if ( IsInHighlanderMode() )
	{
		return 1;
	}
	else if ( tf_classlimit.GetInt() )
	{
		return tf_classlimit.GetInt();
	}

	return NO_CLASS_LIMIT;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanPlayerChooseClass( CBasePlayer *pPlayer, int iClass )
{
	int iClassLimit = GetClassLimit( iClass );

#ifdef TF_RAID_MODE
	if ( IsRaidMode() && !pPlayer->IsBot() )
	{
		// bots are exempt from class limits, to allow for additional support bot "friends"
		if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			if ( tf_raid_allow_all_classes.GetBool() == false )
			{
				if ( iClass == TF_CLASS_SCOUT )
					return false;

				if ( iClass == TF_CLASS_SPY )
					return false;
			}

			if ( tf_raid_enforce_unique_classes.GetBool() )
			{
				// only one of each class on the raiding team
				iClassLimit = 1;
			}
		}
	}
	else if ( IsBossBattleMode() )
	{
		return true;
	}
	else
#endif // TF_RAID_MODE

	if ( iClassLimit == NO_CLASS_LIMIT )
		return true;

	if ( pPlayer->GetTeamNumber() != TF_TEAM_BLUE && pPlayer->GetTeamNumber() != TF_TEAM_RED )
		return true;
#ifdef GAME_DLL
	CTFTeam *pTeam = assert_cast<CTFTeam*>(pPlayer->GetTeam());
#else
	C_TFTeam *pTeam = assert_cast<C_TFTeam*>(pPlayer->GetTeam());
#endif
	if ( !pTeam )
		return true;

	int iTeamClassCount = 0;
	for ( int iPlayer = 0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pTeam->GetPlayer( iPlayer ) );
		if ( pTFPlayer && pTFPlayer != pPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == iClass )
		{
			iTeamClassCount++;
		}
	}

	return ( iTeamClassCount < iClassLimit );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldBalanceTeams( void )
{
	// never autobalance the teams for managed matches using the old system
	if ( GetMatchGroupDescription( GetCurrentMatchGroup() ) )
		return false;

	bool bDisableBalancing = false;

	if ( IsPVEModeActive() || bDisableBalancing )
		return false;

	// don't balance the teams while players are in hell
	if ( ArePlayersInHell() )
		return false;

	return BaseClass::ShouldBalanceTeams();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetBonusRoundTime( bool bGameOver /* = false*/ )
{
	if ( IsMannVsMachineMode() )
	{
		return 5;
	}
	else if ( IsCompetitiveMode() && bGameOver )
	{
		if ( IsMatchTypeCompetitive() )
		{
			return 5;
		}
	}

	return BaseClass::GetBonusRoundTime( bGameOver );
}


#ifdef GAME_DLL

bool CTFGameRules::CanBotChangeClass( CBasePlayer* pPlayer )
{
	// if there's a roster for this bot's team, check to see if the level designer has allowed the bot to change class
	// used when the bot dies and wants to see if it can change class
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer && pTFPlayer->GetPlayerClass() && pTFPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED )
	{
		switch ( pPlayer->GetTeamNumber() )
		{					  
		case TF_TEAM_RED:  return m_hRedBotRoster ? m_hRedBotRoster->IsClassChangeAllowed() : true; break;
		case TF_TEAM_BLUE: return m_hBlueBotRoster ? m_hBlueBotRoster->IsClassChangeAllowed() : true; break;
		}
	}
	return true;
}

bool CTFGameRules::CanBotChooseClass( CBasePlayer *pPlayer, int iClass )
{
	// if there's a roster for this bot's team, then check to see if the class the bot has requested is allowed by the roster
	bool bCanChooseClass = CanPlayerChooseClass( pPlayer, iClass );
	if ( bCanChooseClass )
	{
		// now check rosters...
		switch ( pPlayer->GetTeamNumber() )
		{					  
		case TF_TEAM_RED:  
			bCanChooseClass = m_hRedBotRoster ? m_hRedBotRoster->IsClassAllowed( iClass ) : true; 
			break;
		case TF_TEAM_BLUE: 
			bCanChooseClass = m_hBlueBotRoster ? m_hBlueBotRoster->IsClassAllowed( iClass ) : true; 
			break;
		default: 
			// no roster - spectator team
			bCanChooseClass = true; 
			break;
		}
	}
	return bCanChooseClass;
}

//-----------------------------------------------------------------------------
// Populate vector with set of control points the player needs to capture
void CTFGameRules::CollectCapturePoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *captureVector ) const
{
	if ( !captureVector )
		return;

	captureVector->RemoveAll();

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		// special case hack for KotH mode to use control points that are locked at the start of the round
		if ( IsInKothMode() && pMaster->GetNumPoints() == 1 )
		{
			captureVector->AddToTail( pMaster->GetControlPoint( 0 ) );
			return;
		}

		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = pMaster->GetControlPoint( i );
			if ( point && pMaster->IsInRound( point ) )
			{
				if ( ObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) == player->GetTeamNumber() )
					continue;

				if ( player && player->IsBot() && point->ShouldBotsIgnore() )
					continue;

				if ( ObjectiveResource()->TeamCanCapPoint( point->GetPointIndex(), player->GetTeamNumber() ) )
				{
					if ( TeamplayGameRules()->TeamMayCapturePoint( player->GetTeamNumber(), point->GetPointIndex() ) )
					{
						// unlocked point not on our team available to capture
						captureVector->AddToTail( point );
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Populate vector with set of control points the player needs to defend from capture
void CTFGameRules::CollectDefendPoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *defendVector ) const
{
	if ( !defendVector )
		return;

	defendVector->RemoveAll();

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *point = pMaster->GetControlPoint( i );
			if ( point && pMaster->IsInRound( point ) )
			{
				if ( ObjectiveResource()->GetOwningTeam( point->GetPointIndex() ) != player->GetTeamNumber() )
					continue;
					
				if ( player && player->IsBot() && point->ShouldBotsIgnore() )
					continue;

				if ( ObjectiveResource()->TeamCanCapPoint( point->GetPointIndex(), GetEnemyTeam( player->GetTeamNumber() ) ) )
				{
					if ( TeamplayGameRules()->TeamMayCapturePoint( GetEnemyTeam( player->GetTeamNumber() ), point->GetPointIndex() ) )
					{
						// unlocked point on our team vulnerable to capture
						defendVector->AddToTail( point );
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
CObjectSentrygun *CTFGameRules::FindSentryGunWithMostKills( int team ) const
{
	CObjectSentrygun *dangerousSentry = NULL;
	int dangerousSentryKills = -1;

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() == OBJ_SENTRYGUN && pObj->GetTeamNumber() == team && pObj->GetKills() >= dangerousSentryKills )
		{
			dangerousSentryKills = pObj->GetKills();
			dangerousSentry = static_cast<CObjectSentrygun *>( pObj );
		}
	}

	return dangerousSentry;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	if ( IsMannVsMachineMode() )
	{
		int nCount = 0;
		CTFPlayer *pPlayer;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( !pPlayer || pPlayer->IsFakeClient() )
				continue;

			nCount++;
		}

		if ( nCount >= tf_mvm_max_connected_players.GetInt() )
			return false;
	}

	bool bRet = BaseClass::ClientConnected( pEntity, pszName, pszAddress, reject, maxrejectlen );
	if ( bRet )
	{
		const CSteamID *steamID = engine->GetClientSteamID( pEntity );
		if ( steamID && steamID->IsValid() )
		{
			// Invalid steamIDs wont be known to the GC system, but it has a SteamIDAllowedToConnect() hook that would
			// allow it to reject the connect in the first place in a matchmaking scenario where cares.
			GTFGCClientSystem()->ClientConnected( *steamID, pEntity );
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldMakeChristmasAmmoPack( void )
{
	if ( IsInTournamentMode() && !IsMatchTypeCasual() )
		return false;

	if ( IsMannVsMachineMode() )
		return false;

	if ( mp_holiday_nogifts.GetBool() == true )
		return false;

	if ( IsHolidayActive( kHoliday_Christmas ) == false )
		return false;
 
	return ( RandomInt( 0, 100 ) < 10 );
}

//-----------------------------------------------------------------------------
// Purpose: **** DO NOT SHIP THIS IN REL/HL2 ****
//-----------------------------------------------------------------------------
void CTFGameRules::UpdatePeriodicEvent( CTFPlayer *pPlayer, eEconPeriodicScoreEvents eEvent, uint32 nCount )
{
	CSteamID steamID;
	if ( !pPlayer || !pPlayer->GetSteamID( &steamID ) || !steamID.IsValid() )
		return;

	GCSDK::CProtoBufMsg<CMsgUpdatePeriodicEvent> msg( k_EMsgGC_UpdatePeriodicEvent );
	msg.Body().set_account_id( steamID.GetAccountID() );
	msg.Body().set_event_type( eEvent );
	msg.Body().set_amount( nCount );
	GCClientSystem()->BSendMessage( msg );
}

#endif // GAME_DLL

#ifndef CLIENT_DLL

void CTFGameRules::Status( void (*print) (const char *fmt, ...) )
{
#if defined( _DEBUG )
	print( " == GameStats ==\n" );
	print( "Total Time: %d seconds\n", CTF_GameStats.m_currentMap.m_Header.m_iTotalTime );
	print( "Blue Team Wins: %d\n", CTF_GameStats.m_currentMap.m_Header.m_iBlueWins );
	print( "Red Team Wins: %d\n", CTF_GameStats.m_currentMap.m_Header.m_iRedWins );
	print( "Stalemates: %d\n", CTF_GameStats.m_currentMap.m_Header.m_iStalemates );

	print( "         Spawns Points Kills Deaths Assists\n" );
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
	{
		TF_Gamestats_ClassStats_t &Stats = CTF_GameStats.m_currentMap.m_aClassStats[ iClass ];

		print( "%-8s %6d %6d %5d %6d %7d\n",
			g_aPlayerClassNames_NonLocalized[ iClass ],
			Stats.iSpawns, Stats.iScore, Stats.iKills, Stats.iDeaths, Stats.iAssists );
	}
	print( "\n" );
#endif // defined( _DEBUG )
}

#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		::V_swap( collisionGroup0, collisionGroup1 );
	}
	
	//Don't stand on COLLISION_GROUP_WEAPONs
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// Don't stand on projectiles
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return true;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PROJECTILE ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS || collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TFCOLLISION_GROUP_ROCKETS ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) &&
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS ) )
		return false;

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	// Respawn rooms only collide with players
	if ( collisionGroup1 == TFCOLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );
	
/*	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
 	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more
		// Nor with objects or grenades
		switch ( collisionGroup0 )
		{
		default:
			break;
		case COLLISION_GROUP_PLAYER:
			return false;
		}
	}
*/
	// don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.)
	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		 ( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
	{
		return false;
	}


	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 ==  COLLISION_GROUP_PLAYER_MOVEMENT ) &&
		 collisionGroup1 == TFCOLLISION_GROUP_TANK )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of this player towards capturing a point
//-----------------------------------------------------------------------------
int	CTFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
{
#ifdef GAME_DLL
#ifdef TF_CREEP_MODE
	if ( IsCreepWaveMode() )
	{
		CTFBot *bot = ToTFBot( pPlayer );

		if ( !bot || !bot->HasAttribute( CTFBot::IS_NPC ) )
		{
			// only creeps can influence points
			return 0;
		}
	}
#endif
#endif

	int nValue = BaseClass::GetCaptureValueForPlayer( pPlayer );


	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Scouts count for 2 people in timebased capping.
			nValue = 2;
		}
		else
		{
			// Scouts can cap all points on their own.
			nValue = 10;
		}
	}

	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nValue, add_player_capturevalue );

	return nValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	int iTime = (int)(flMapChangeTime - gpGlobals->curtime);
	if ( iTime < 0 )
	{
		iTime = 0;
	}

	return ( iTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

#ifdef GAME_DLL
	if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
		if ( IsMannVsMachineMode() )
			return;

		// keep track of how many times each team caps
		int iTeam = event->GetInt( "team" );
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		m_iNumCaps[iTeam]++;

		// award a capture to all capping players
		const char *cappers = event->GetString( "cappers" );

		Q_strncpy( m_szMostRecentCappers, cappers, ARRAYSIZE( m_szMostRecentCappers ) );	
		for ( int i =0; i < Q_strlen( cappers ); i++ )
		{
			int iPlayerIndex = (int) cappers[i];
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );	

				if ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && GetGameType() == TF_GAMETYPE_ESCORT )
				{
					pPlayer->AwardAchievement( ACHIEVEMENT_TF_HEAVY_PAYLOAD_CAP_GRIND );
				}

				// Give money and experience
				int nAmount = CalculateCurrencyAmount_ByType( TF_CURRENCY_CAPTURED_OBJECTIVE );
				DistributeCurrencyAmount( nAmount, pPlayer, false );
			}
		}

		// Halloween 2012 doesn't want ghosts to spawn when the point is captured
		if( !IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
		{
			// for 2011 Halloween map
			BeginHaunting( 4, 25.f, 35.f );
		}
	}
	else if ( !Q_strcmp( eventName, "teamplay_capture_blocked" ) )
	{
		int iPlayerIndex = event->GetInt( "blocker" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );

		pPlayer->m_Shared.CheckForAchievement( ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER );
	}	
	else if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
		int iWinningTeam = event->GetInt( "team" );
		bool bFullRound = event->GetBool( "full_round" );
		float flRoundTime = event->GetFloat( "round_time" );
		bool bWasSuddenDeath = event->GetBool( "was_sudden_death" );
		CTF_GameStats.Event_RoundEnd( iWinningTeam, bFullRound, flRoundTime, bWasSuddenDeath );
	}
	else if ( !Q_strcmp( eventName, "teamplay_setup_finished" ) )
	{
		if ( IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			m_doomsdaySetupTimer.Start( 1 );
		}
	}
	else if ( !Q_strcmp( eventName, "teamplay_flag_event" ) )
	{
		// if this is a capture event, remember the player who made the capture		
		int iEventType = event->GetInt( "eventtype" );
		if ( TF_FLAGEVENT_CAPTURE == iEventType )
		{
			int iPlayerIndex = event->GetInt( "player" );
			m_szMostRecentCappers[0] = iPlayerIndex;
			m_szMostRecentCappers[1] = 0;
		}
	}
	else if ( !Q_strcmp( eventName, "player_escort_score" ) )
	{
		int iPlayer = event->GetInt( "player", 0 );
		int iPoints = event->GetInt( "points", 0 );

		if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex(iPlayer) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerScoresEscortPoints( pPlayer, iPoints );

				int nAmount = CalculateCurrencyAmount_ByType( TF_CURRENCY_ESCORT_REWARD );
				DistributeCurrencyAmount( ( nAmount * iPoints ), pPlayer, false );

				if ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && GetGameType() == TF_GAMETYPE_ESCORT )
				{
					for ( int i = 0 ; i < iPoints ; i++ )
					{
						pPlayer->AwardAchievement( ACHIEVEMENT_TF_HEAVY_PAYLOAD_CAP_GRIND );
					}
				}
			}
		}
	}
	else if ( !Q_strcmp( eventName, "player_disconnect" ) )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt("userid") ) );

		// @note Tom Bui: this really sucks, but we don't know the reason other than the string...
		const char *pReason = event->GetString( "reason" );
		if ( !Q_strncmp( pReason, "Kicked", ARRAYSIZE( "Kicked" ) - 1 ) )
		{
			if ( pPlayer )
			{
				DuelMiniGame_NotifyPlayerDisconnect( pPlayer, true );
			}
		}
	}
	else if ( !Q_strcmp( eventName, "teamplay_round_start" ) )
	{
		if ( IsMannVsMachineMode() )
		{
			if ( g_pPopulationManager )
			{
				g_pPopulationManager->RestorePlayerCurrency();
				
				// make sure all invaders are removed
				CUtlVector< CTFPlayer * > playerVector;
				CollectPlayers( &playerVector, TF_TEAM_PVE_INVADERS );

				for( int i=0; i<playerVector.Count(); ++i )
				{
					playerVector[i]->ChangeTeam( TEAM_SPECTATOR, false, true );
				}
			}
		}
	}
	else if ( !Q_strcmp( eventName, "recalculate_truce" ) )
	{
		RecalculateTruce();
	}
#else	// CLIENT_DLL
	if ( !Q_strcmp( eventName, "overtime_nag" ) )
	{
		HandleOvertimeBegin();
	}
	else if ( !Q_strcmp( eventName, "recalculate_holidays" ) )
	{
		UTIL_CalculateHolidays();
	}
#endif

	BaseClass::FireGameEvent( event );
}


const char *CTFGameRules::GetGameTypeName( void )
{
	return ::GetGameTypeName( m_nGameType.Get() );
}


void CTFGameRules::ClientSpawned( edict_t * pPlayer )
{
}

void CTFGameRules::OnFileReceived( const char * fileName, unsigned int transferID )
{
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;
		
		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for ( int i=1; i < TF_AMMO_COUNT; i++ )
		{
			const char *pszAmmoName = GetAmmoName( i );
			def.AddAmmoType( pszAmmoName, DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD, TRACER_LINE, 0, 0, "ammo_max", 2400, 10, 14 );
			Assert( def.Index( pszAmmoName ) == i );
		}
	}

	return &def;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetTeamGoalString( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_pszTeamGoalStringRed.Get();
	if ( iTeam == TF_TEAM_BLUE )
		return m_pszTeamGoalStringBlue.Get();
	return NULL;
}

#ifdef GAME_DLL

	Vector MaybeDropToGround( 
							CBaseEntity *pMainEnt, 
							bool bDropToGround, 
							const Vector &vPos, 
							const Vector &vMins, 
							const Vector &vMaxs )
	{
		if ( bDropToGround )
		{
			trace_t trace;
			UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
			return trace.endpos;
		}
		else
		{
			return vPos;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: This function can be used to find a valid placement location for an entity.
	//			Given an origin to start looking from and a minimum radius to place the entity at,
	//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
	//			where mins and maxs will fit.
	// Input  : *pMainEnt - Entity to place
	//			&vOrigin - Point to search around
	//			fRadius - Radius to search within
	//			nTries - Number of tries to attempt
	//			&mins - mins of the Entity
	//			&maxs - maxs of the Entity
	//			&outPos - Return point
	// Output : Returns true and fills in outPos if it found a spot.
	//-----------------------------------------------------------------------------
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
	{
		// This function moves the box out in each dimension in each step trying to find empty space like this:
		//
		//											  X  
		//							   X			  X  
		// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
		//							   X 			  X  
		//											  X  
		//

		Vector mins, maxs;
		pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		mins -= pMainEnt->GetAbsOrigin();
		maxs -= pMainEnt->GetAbsOrigin();

		// Put some padding on their bbox.
		float flPadSize = 5;
		Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
		Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
		{
			outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
			return true;
		}

		Vector vDims = vTestMaxs - vTestMins;


		// Keep branching out until we get too far.
		int iCurIteration = 0;
		int nMaxIterations = 15;

		int offset = 0;
		do
		{
			for ( int iDim=0; iDim < 3; iDim++ )
			{
				float flCurOffset = offset * vDims[iDim];

				for ( int iSign=0; iSign < 2; iSign++ )
				{
					Vector vBase = vOrigin;
					vBase[iDim] += (iSign*2-1) * flCurOffset;

					if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
					{
						// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
						// (Useful for keeping things from spawning behind walls near a spawn point)
						trace_t tr;
						UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

						if ( tr.fraction != 1.0 )
						{
							continue;
						}

						outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
						return true;
					}
				}
			}

			++offset;
		} while ( iCurIteration++ < nMaxIterations );

		//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
		return false;
	}

#else // GAME_DLL

CTFGameRules::~CTFGameRules()
{
	if ( m_pkvVisionFilterTranslations )
	{
		m_pkvVisionFilterTranslations->deleteThis();
		m_pkvVisionFilterTranslations = NULL;
	}

	if ( m_pkvVisionFilterShadersMapWhitelist )
	{
		m_pkvVisionFilterShadersMapWhitelist->deleteThis();
		m_pkvVisionFilterShadersMapWhitelist = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_bRecievedBaseline |= updateType == DATA_UPDATE_CREATED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleOvertimeBegin()
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pTFPlayer )
	{
		pTFPlayer->EmitSound( "Game.Overtime" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldShowTeamGoal( void )
{

//=============================================================================
// HPE_BEGIN
// [msmith] We always show the team goal when in training.
//=============================================================================	
	bool showDuringSetup = InSetup();

	if ( IsInTraining() || IsInItemTestingMode() )
	{
		showDuringSetup = false;
	}

	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || showDuringSetup )
		return true;

//=============================================================================
// HPE_END
//=============================================================================
	return false;
}

#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		int nRuleCount = m_ResponseRules.Count();
		for ( int iRule = 0; iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}
		m_ResponseRules.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TF_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );

			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_sprintf_safe( szName, "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] = BuildCustomResponseSystemGivenCriteria( "scripts/talker/response_rules.txt", szName, criteriaSet, flCriteriaScore );
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef _DEBUG
CON_COMMAND( hud_notify, "Show a hud notification." )
{
	if ( args.ArgC() < 2 )
	{
		Warning( "Requires one argument, HudNotification_t, between 0 and %i\n", NUM_STOCK_NOTIFICATIONS );
		return;
	}

	if ( !TFGameRules() )
	{
		Warning( "Can't do that right now\n" );
		return;
	}

	CRecipientFilter filter;
	filter.AddAllPlayers();	
	TFGameRules()->SendHudNotification( filter, (HudNotification_t) V_atoi(args.Arg(1)) );
}
#endif

void CTFGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType, bool bForceShow /*= false*/ )
{
	if ( !bForceShow && IsInWaitingForPlayers() )
		return;

	UserMessageBegin( filter, "HudNotify" );
		WRITE_BYTE( iType );
		WRITE_BOOL( bForceShow );	// Display in cl_hud_minmode
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	if ( IsInWaitingForPlayers() )
		return;

	UserMessageBegin( filter, "HudNotifyCustom" );
		WRITE_STRING( pszText );
		WRITE_STRING( pszIcon );
		WRITE_BYTE( iTeam );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTFGameRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer ); 

	return ( gpGlobals->curtime > flMinSpawnTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldRespawnQuickly( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( IsPVEModeActive() && pTFPlayer && pTFPlayer->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT )
	{
		return true;
	}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return true;
#endif // _DEBUG || STAGING_ONLY

	if ( IsCompetitiveMode() && State_Get() == GR_STATE_BETWEEN_RNDS )
		return true;

	return BaseClass::ShouldRespawnQuickly( pPlayer );
}

typedef bool (*BIgnoreConvarChangeFunc)(void);

struct convar_tags_t
{
	const char *pszConVar;
	const char *pszTag;
	BIgnoreConvarChangeFunc ignoreConvarFunc;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool BIgnoreConvarChangeInPVEMode(void)
{
	return TFGameRules() && TFGameRules()->IsPVEModeActive();
}

// The list of convars that automatically turn on tags when they're changed.
// Convars in this list need to have the FCVAR_NOTIFY flag set on them, so the
// tags are recalculated and uploaded to the master server when the convar is changed.
convar_tags_t convars_to_check_for_tags[] =
{
	{ "mp_friendlyfire", "friendlyfire", NULL },
	{ "tf_birthday", "birthday", NULL },
	{ "mp_respawnwavetime", "respawntimes", NULL },
	{ "mp_fadetoblack", "fadetoblack", NULL },
	{ "tf_weapon_criticals", "nocrits", NULL },
	{ "mp_disable_respawn_times", "norespawntime", NULL },
	{ "tf_gamemode_arena", "arena", NULL },
	{ "tf_gamemode_cp", "cp", NULL },
	{ "tf_gamemode_ctf", "ctf", NULL },
	{ "tf_gamemode_sd", "sd", NULL },
	{ "tf_gamemode_mvm", "mvm", NULL },
	{ "tf_gamemode_payload", "payload", NULL },
	{ "tf_gamemode_rd",	"rd", NULL },
	{ "tf_gamemode_pd",	"pd", NULL },
	{ "tf_gamemode_tc",	"tc", NULL },
	{ "tf_beta_content", "beta", NULL },
	{ "tf_damage_disablespread", "dmgspread", NULL },
	{ "mp_highlander", "highlander", NULL },
	{ "tf_bot_count", "bots", &BIgnoreConvarChangeInPVEMode },
	{ "tf_pve_mode", "pve" },
	{ "sv_registration_successful", "_registered", NULL },
	{ "tf_server_identity_disable_quickplay", "noquickplay", NULL },
	{ "tf_mm_strict", "hidden", NULL },
	{ "tf_medieval", "medieval", NULL },
	{ "mp_holiday_nogifts", "nogifts" },
	{ "tf_powerup_mode", "powerup", NULL },
	{ "tf_gamemode_passtime", "passtime", NULL },
	{ "tf_gamemode_misc", "misc", NULL }, // catch-all for matchmaking to identify sd, tc, and pd servers via sv_tags
};

//-----------------------------------------------------------------------------
// Purpose: Engine asks for the list of convars that should tag the server
//-----------------------------------------------------------------------------
void CTFGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	BaseClass::GetTaggedConVarList( pCvarTagList );

	for ( int i = 0; i < ARRAYSIZE(convars_to_check_for_tags); i++ )
	{
		if ( convars_to_check_for_tags[i].ignoreConvarFunc && convars_to_check_for_tags[i].ignoreConvarFunc() )
			continue;

		KeyValues *pKV = new KeyValues( "tag" );
		pKV->SetString( "convar", convars_to_check_for_tags[i].pszConVar );
		pKV->SetString( "tag", convars_to_check_for_tags[i].pszTag );

		pCvarTagList->AddSubKey( pKV );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlaySpecialCapSounds( int iCappingTeam, CTeamControlPoint *pPoint )
{
	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
	{
		return;
	}

	if ( GetGameType() == TF_GAMETYPE_CP )
	{
		bool bPlayControlPointCappedSound = IsInKothMode();
		if ( !bPlayControlPointCappedSound )
		{
			if ( pPoint && ShouldScorePerRound() )
			{
				CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
				if ( pMaster && !pMaster->WouldNewCPOwnerWinGame( pPoint, iCappingTeam ) )
				{
					bPlayControlPointCappedSound = true;
				}
			}
		}

		if ( bPlayControlPointCappedSound )
		{
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				if ( IsInKothMode() )
				{
					BroadcastSound( i, "Hud.PointCaptured" );
				}

				if ( i == iCappingTeam )
				{
					BroadcastSound( i, "Announcer.Success" );
				}
				else
				{
					BroadcastSound( i, "Announcer.Failure" );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Factory to create TF-specific mission manager singleton
//-----------------------------------------------------------------------------
CTacticalMissionManager *CTFGameRules::TacticalMissionManagerFactory( void )
{
	return new CTFTacticalMissionManager;
}

#endif


//-----------------------------------------------------------------------------
// Purpose: Get the video file name for the current map.
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
const char *CTFGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];
	mapname[0] = 0;

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	if ( mapname[0] == 0 )
	{
		return NULL;
	}
	
	Q_strlower( mapname );
	return FormatVideoName( (const char *)mapname, bWithExtension );
}

//=============================================================================
// HPE_BEGIN
// [msmith] Used for the client to tell the server that we're watching a movie or not.
//			Also contains the name of a movie if it's an in game video.
//=============================================================================
//-----------------------------------------------------------------------------
// Purpose: Format a video file name from the name passed in.
//-----------------------------------------------------------------------------
const char *CTFGameRules::FormatVideoName( const char *videoName, bool bWithExtension /*= true*/ )
{

	static char strFullpath[MAX_PATH];		// this buffer is returned to the caller
	
#ifdef _X360
	// Should we be modifying a const buffer?
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( videoName, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory

	if ( Q_strstr( videoName, "arena_" ) )
	{
		char strTempPath[MAX_PATH];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );	

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to arena_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof(strFullpath), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "arena_intro", MAX_PATH );
		}
	}
	else if ( Q_strstr( videoName, "mvm_" ) )
	{
		char strTempPath[MAX_PATH];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );	

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to mvm_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof(strFullpath), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "mvm_intro", MAX_PATH );
		}
	}
	else if ( Q_strstr( videoName, "zi_" ) )
	{
		char strTempPath[ MAX_PATH ];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to zi_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof( strFullpath ), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "zi_intro", MAX_PATH );
		}
	}
	else if ( Q_strstr( videoName, "vsh_" ) )
	{
		char strTempPath[ MAX_PATH ];
		Q_strncpy( strTempPath, "media/", MAX_PATH );
		Q_strncat( strTempPath, videoName, MAX_PATH );
		Q_strncat( strTempPath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );

		VideoSystem_t vSystem = VideoSystem::NONE;

		// default to vsh_intro video if we can't find the specified video
		if ( !g_pVideo || g_pVideo->LocatePlayableVideoFile( strTempPath, "GAME", &vSystem, strFullpath, sizeof( strFullpath ), VideoSystemFeature::PLAY_VIDEO_FILE_IN_MATERIAL ) != VideoResult::SUCCESS )
		{
			V_strncpy( strFullpath, "media/" "vsh_intro", MAX_PATH );
		}
	}
	else
	{
		Q_strncat( strFullpath, videoName, MAX_PATH );
	}

	if ( bWithExtension )
	{
		Q_strncat( strFullpath, FILE_EXTENSION_ANY_MATCHING_VIDEO, MAX_PATH );		// Don't assume any specific video type, let the video services find it
	}

	return strFullpath;
}
#endif
//=============================================================================
// HPE_END
//=============================================================================

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapDisplayName( const char *mapName, bool bTitleCase /* = false */ )
{
	static char szDisplayName[256];
	char szTempName[256];
	const char *pszSrc = NULL;

	szDisplayName[0] = '\0';

	if ( !mapName )
		return szDisplayName;

	// check our lookup table
	Q_strncpy( szTempName, mapName, sizeof( szTempName ) );
	Q_strlower( szTempName );
	pszSrc = szTempName;

	for ( int i = 0; i < ARRAYSIZE( s_ValveMaps ); ++i )
	{
		if ( !Q_stricmp( s_ValveMaps[i].pDiskName, pszSrc ) )
		{
			return s_ValveMaps[i].pDisplayName;
		}
	}

	// check the community maps that we've featured
	for ( int i = 0; i < ARRAYSIZE( s_CommunityMaps ); ++i )
	{
		if ( !Q_stricmp( s_CommunityMaps[i].pDiskName, pszSrc ) )
		{
			return s_CommunityMaps[i].pDisplayName;
		}
	}

	char *pszFinal = Q_strstr( pszSrc, "_final" );
	if ( pszFinal )
	{
		// truncate the _final (or _final1) part of the filename if it's at the end of the name
		char *pszNextChar = pszFinal + Q_strlen( "_final" );
		if ( pszNextChar )
		{
			if ( ( *pszNextChar == '\0' ) ||
				 ( ( *pszNextChar == '1' ) && ( *(pszNextChar+1) == '\0' ) ) )
			{
				*pszFinal = '\0';
			}
		}
	}

	// Our workshop maps will be of the format workshop/cp_somemap.ugc12345
	const char szWorkshop[] = "workshop/";
	if ( V_strncmp( pszSrc, szWorkshop, sizeof( szWorkshop ) - 1 ) == 0 )
	{
		pszSrc += sizeof( szWorkshop ) - 1;
		char *pszUGC = V_strstr( pszSrc, ".ugc" );
		int nUGCLen = pszUGC ? strlen( pszUGC ) : 0;
		if ( pszUGC && nUGCLen > 4 )
		{
			int i;
			for ( i = 4; i < nUGCLen; i ++ )
			{
				if ( pszUGC[i] < '0' || pszUGC[i] > '9' )
				{
					break;
				}
			}

			if ( i == nUGCLen )
			{
				*pszUGC = '\0';
			}
		}
	}

	// we haven't found a "friendly" map name, so let's just clean up what we have
	if ( !Q_strncmp( pszSrc, "cp_", 3 ) ||
		 !Q_strncmp( pszSrc, "tc_", 3 ) ||
		 !Q_strncmp( pszSrc, "pl_", 3 ) ||
		 !Q_strncmp( pszSrc, "ad_", 3 ) ||
		 !Q_strncmp( pszSrc, "sd_", 3 ) || 
		 !Q_strncmp( pszSrc, "rd_", 3 ) ||
		 !Q_strncmp( pszSrc, "pd_", 3 ) )
	{
		pszSrc +=  3;
	}
	else if ( !Q_strncmp( pszSrc, "ctf_", 4 ) ||
		      !Q_strncmp( pszSrc, "plr_", 4 ) )
	{
		pszSrc +=  4;
	}
	else if ( !Q_strncmp( szTempName, "koth_", 5 ) ||
			  !Q_strncmp( szTempName, "pass_", 5 ) )
	{
		pszSrc +=  5;
	}
#ifdef TF_RAID_MODE
	else if ( !Q_strncmp( pszSrc, "raid_", 5 ) )
	{
		pszSrc +=  5;
	}
#endif // TF_RAID_MODE
	else if ( !Q_strncmp( pszSrc, "mvm_", 4 ) )
	{
		pszSrc +=  4;
	}
	else if ( !Q_strncmp( pszSrc, "arena_", 6 ) )
	{
		pszSrc +=  6;
	}

	Q_strncpy( szDisplayName, pszSrc, sizeof( szDisplayName ) );

	// replace underscores with spaces
	for ( char *pszUnderscore = szDisplayName ; pszUnderscore != NULL && *pszUnderscore != 0 ; pszUnderscore++ )
	{
		// Replace it with a space
		if ( *pszUnderscore == '_' )
		{
			*pszUnderscore = ' ';
		}
	}

	if ( bTitleCase )
	{
		V_strtitlecase( szDisplayName );
	}
	else
	{
		// Default behavior - tf maps are LOUD
		Q_strupr( szDisplayName );
	}

	return szDisplayName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *GetMapType( const char *mapName )
{
	int i;

	if ( mapName )
	{
		for ( i = 0; i < ARRAYSIZE( s_ValveMaps ); ++i )
		{
			if ( !Q_stricmp( s_ValveMaps[i].pDiskName, mapName ) )
			{
				return s_ValveMaps[i].pGameType;
			}
		}

		// check the community maps that we've featured
		for ( i = 0; i < ARRAYSIZE( s_CommunityMaps ); ++i )
		{
			if ( !Q_stricmp( s_CommunityMaps[i].pDiskName, mapName ) )
			{
				return s_CommunityMaps[i].pGameType;
			}
		}
	}

	if ( !IsX360() )
	{
		// we haven't found a "friendly" map name, so let's just clean up what we have
		if ( !Q_strnicmp( mapName, "cp_", 3 ) )
		{
			return "#Gametype_CP";
		}
		else if ( !Q_strnicmp( mapName, "tc_", 3 ) )
		{
			return "#TF_TerritoryControl";
		}
		else if ( !Q_strnicmp( mapName, "pl_", 3 ) ) 
		{
			return "#Gametype_Escort";
		}
		else if ( !Q_strnicmp( mapName, "plr_", 4 ) ) 
		{
			return "#Gametype_EscortRace";
		}
		else if ( !Q_strnicmp( mapName, "ad_", 3 ) )
		{
			return "#TF_AttackDefend";
		}
		else if ( !Q_strnicmp( mapName, "ctf_", 4 ) )
		{
			return "#Gametype_CTF";
		}
		else if ( !Q_strnicmp( mapName, "koth_", 5 ) )
		{
			return "#Gametype_Koth";
		}
		else if ( !Q_strnicmp( mapName, "arena_", 6 ) )
		{
			return "#Gametype_Arena";
		}
		else if ( !Q_strnicmp( mapName, "sd_", 3 ) )
		{
			return "#Gametype_SD";
		}
#ifdef TF_RAID_MODE
		else if ( !Q_strnicmp( mapName, "raid_", 5 ) )
		{
			return "#Gametype_Raid";
		}
#endif // TF_RAID_MODE
		else if ( !Q_strnicmp( mapName, "mvm_", 4 ) )
		{
			return "#Gametype_MVM";
		}
		else if ( !Q_strnicmp( mapName, "pass_", 5 ) )
		{
			return "#GameType_Passtime";
		}
		else if ( !Q_strnicmp( mapName, "rd_", 3 ) )
		{
			return "#Gametype_RobotDestruction";
		}
		else if ( !Q_strnicmp( mapName, "pd_", 3 ) )
		{
			return "#Gametype_PlayerDestruction";
		}
		else
		{
			if ( TFGameRules() )
			{
				return TFGameRules()->GetGameTypeName();
			}
		}
	}

	return "";
}
#endif

//Arena Mode
bool CTFGameRules::IsInArenaMode( void ) const
{
	return m_nGameType == TF_GAMETYPE_ARENA;
}

#ifdef GAME_DLL

//==================================================================================================================
// ARENA LOGIC
BEGIN_DATADESC( CArenaLogic )
	DEFINE_OUTPUT( m_OnArenaRoundStart, "OnArenaRoundStart" ),
	DEFINE_OUTPUT( m_OnCapEnabled, "OnCapEnabled" ),
	DEFINE_KEYFIELD( m_flTimeToEnableCapPoint, FIELD_FLOAT, "CapEnableDelay" ),
	DEFINE_FUNCTION( ArenaLogicThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_arena, CArenaLogic );


void CArenaLogic::Spawn( void )
{
	BaseClass::Spawn();

	SetThink( &CArenaLogic::ArenaLogicThink );
	SetNextThink( gpGlobals->curtime );
}

void CArenaLogic::ArenaLogicThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( TFGameRules()->State_Get() != GR_STATE_STALEMATE )
		return;

	if ( TFGameRules() && TFGameRules()->GetCapturePointTime() <= gpGlobals->curtime )
	{
		if ( m_bFiredOutput == false )
		{
			m_bFiredOutput = true;
			m_OnCapEnabled.FireOutput( this, this );
		}
	}
	else if ( TFGameRules() && TFGameRules()->GetCapturePointTime() > gpGlobals->curtime )
	{
		m_bFiredOutput = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: COMPETITIVE LOGIC (why are we shouting?)
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CCompetitiveLogic )
DEFINE_OUTPUT( m_OnSpawnRoomDoorsShouldLock, "OnSpawnRoomDoorsShouldLock" ),
DEFINE_OUTPUT( m_OnSpawnRoomDoorsShouldUnlock, "OnSpawnRoomDoorsShouldUnlock" ),
END_DATADESC()
LINK_ENTITY_TO_CLASS( tf_logic_competitive, CCompetitiveLogic );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompetitiveLogic::OnSpawnRoomDoorsShouldLock( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsCompetitiveMode() )
		return; 

	m_OnSpawnRoomDoorsShouldLock.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCompetitiveLogic::OnSpawnRoomDoorsShouldUnlock( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsCompetitiveMode() )
		return; 

	m_OnSpawnRoomDoorsShouldUnlock.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CLogicMannPower )
END_DATADESC()
LINK_ENTITY_TO_CLASS( tf_logic_mannpower, CLogicMannPower );

//=============================================================================
// Training Mode
CON_COMMAND( training_continue, "Tells training that it should continue." )
{
	if ( TFGameRules() == NULL || TFGameRules()->IsInTraining() == false || TFGameRules()->GetTrainingModeLogic() == NULL )
	{
		return;
	}
	TFGameRules()->GetTrainingModeLogic()->OnPlayerWantsToContinue();
}

#define SF_TF_DYNAMICPROP_GRENADE_COLLISION			512
class CTFTrainingDynamicProp : public CDynamicProp
{
	DECLARE_CLASS( CTFTrainingDynamicProp, CDynamicProp );
public:
};
LINK_ENTITY_TO_CLASS( training_prop_dynamic, CTFTrainingDynamicProp );

bool PropDynamic_CollidesWithGrenades( CBaseEntity *pBaseEntity )
{
	CTFTrainingDynamicProp *pTrainingDynamicProp = dynamic_cast< CTFTrainingDynamicProp* >( pBaseEntity );
	return ( pTrainingDynamicProp && pTrainingDynamicProp->HasSpawnFlags( SF_TF_DYNAMICPROP_GRENADE_COLLISION) );
}

BEGIN_DATADESC( CTrainingModeLogic )
	DEFINE_KEYFIELD( m_nextMapName, FIELD_STRING, "nextMap" ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ShowTrainingMsg", InputShowTrainingMsg ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ShowTrainingObjective", InputShowTrainingObjective ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForcePlayerSpawnAsClassOutput", InputForcePlayerSpawnAsClassOutput ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KickBots", InputKickAllBots ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ShowTrainingHUD", InputShowTrainingHUD ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HideTrainingHUD", InputHideTrainingHUD ),
	DEFINE_INPUTFUNC( FIELD_STRING, "EndTraining", InputEndTraining ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlaySoundOnPlayer", InputPlaySoundOnPlayer ),
	DEFINE_INPUTFUNC( FIELD_STRING, "WaitForTimerOrKeypress", InputWaitForTimerOrKeypress ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetNextMap", InputSetNextMap ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ForcePlayerSwapToWeapon", InputForcePlayerSwapToWeapon ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsScout, "OnPlayerSpawnAsScout" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsSniper, "OnPlayerSpawnAsSniper" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsSoldier, "OnPlayerSpawnAsSoldier" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsDemoman, "OnPlayerSpawnAsDemoman" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsMedic, "OnPlayerSpawnAsMedic" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsHeavy, "OnPlayerSpawnAsHeavy" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsPyro, "OnPlayerSpawnAsPyro" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsSpy, "OnPlayerSpawnAsSpy" ),
	DEFINE_OUTPUT( m_outputOnPlayerSpawnAsEngineer, "OnPlayerSpawnAsEngineer" ),
	DEFINE_OUTPUT( m_outputOnPlayerDied, "OnPlayerDied" ),
	DEFINE_OUTPUT( m_outputOnBotDied, "OnBotDied" ),
	DEFINE_OUTPUT( m_outputOnPlayerSwappedToWeaponSlotPrimary, "OnPlayerSwappedToPrimary" ),
	DEFINE_OUTPUT( m_outputOnPlayerSwappedToWeaponSlotSecondary, "OnPlayerSwappedToSecondary" ),
	DEFINE_OUTPUT( m_outputOnPlayerSwappedToWeaponSlotMelee, "OnPlayerSwappedToMelee" ),
	DEFINE_OUTPUT( m_outputOnPlayerSwappedToWeaponSlotBuilding, "OnPlayerSwappedToBuilding" ),
	DEFINE_OUTPUT( m_outputOnPlayerSwappedToWeaponSlotPDA, "OnPlayerSwappedToPDA" ),
	DEFINE_OUTPUT( m_outputOnPlayerBuiltOutsideSuggestedArea, "OnBuildOutsideArea" ),
	DEFINE_OUTPUT( m_outputOnPlayerDetonateBuilding, "OnPlayerDetonateBuilding" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_training_mode, CTrainingModeLogic );

void CTrainingModeLogic::SetupOnRoundStart()
{
	m_objText[0] = 0;
	SetTrainingMsg( "" );
}

void CTrainingModeLogic::SetTrainingMsg(const char *msg)
{
	CBroadcastRecipientFilter allusers;
	allusers.MakeReliable();
	UserMessageBegin( allusers, "TrainingMsg" );
	WRITE_STRING( msg );
	MessageEnd();
}

void CTrainingModeLogic::SetTrainingObjective(const char *text)
{
	CBroadcastRecipientFilter allusers;
	allusers.MakeReliable();
	UserMessageBegin( allusers, "TrainingObjective" );
	WRITE_STRING( text );
	MessageEnd();
}

void CTrainingModeLogic::OnPlayerSpawned( CTFPlayer* pPlayer )
{
	if ( pPlayer->GetDesiredPlayerClassIndex() == TF_CLASS_UNDEFINED )
	{
		return;
	}
	if ( pPlayer->IsFakeClient() )
	{
		return;
	}
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	switch ( iClass )
	{
	case TF_CLASS_SCOUT:		m_outputOnPlayerSpawnAsScout.FireOutput( this, this ); break;
	case TF_CLASS_SNIPER:		m_outputOnPlayerSpawnAsSniper.FireOutput( this, this ); break;
	case TF_CLASS_SOLDIER:		m_outputOnPlayerSpawnAsSoldier.FireOutput( this, this ); break;
	case TF_CLASS_DEMOMAN:		m_outputOnPlayerSpawnAsDemoman.FireOutput( this, this ); break;
	case TF_CLASS_MEDIC:		m_outputOnPlayerSpawnAsMedic.FireOutput( this, this ); break;
	case TF_CLASS_HEAVYWEAPONS:	m_outputOnPlayerSpawnAsHeavy.FireOutput( this, this ); break;
	case TF_CLASS_PYRO:			m_outputOnPlayerSpawnAsPyro.FireOutput( this, this ); break;
	case TF_CLASS_SPY:			m_outputOnPlayerSpawnAsSpy.FireOutput( this, this ); break;
	case TF_CLASS_ENGINEER:		m_outputOnPlayerSpawnAsEngineer.FireOutput( this, this ); break;
	}
}

void CTrainingModeLogic::OnPlayerDied( CTFPlayer *pPlayer, CBaseEntity *pKiller )
{
	m_outputOnPlayerDied.FireOutput( pKiller, this );
}

void CTrainingModeLogic::OnBotDied( CTFPlayer *pPlayer, CBaseEntity *pKiller )
{
	m_outputOnBotDied.FireOutput( pKiller, this );
}

void CTrainingModeLogic::OnPlayerSwitchedWeapons( CTFPlayer *pPlayer )
{
	CTFWeaponBase *pWeapon = (CTFWeaponBase*)pPlayer->GetActiveWeapon();
	if ( pWeapon == NULL )
	{
		return;
	}
	switch ( pWeapon->GetTFWpnData().m_iWeaponType )
	{
	case TF_WPN_TYPE_PRIMARY:	m_outputOnPlayerSwappedToWeaponSlotPrimary.FireOutput( this, this ); break;
	case TF_WPN_TYPE_SECONDARY:	m_outputOnPlayerSwappedToWeaponSlotSecondary.FireOutput( this, this ); break;
	case TF_WPN_TYPE_MELEE:		m_outputOnPlayerSwappedToWeaponSlotMelee.FireOutput( this, this ); break;
	case TF_WPN_TYPE_BUILDING:	m_outputOnPlayerSwappedToWeaponSlotBuilding.FireOutput( this, this ); break;
	case TF_WPN_TYPE_PDA:		m_outputOnPlayerSwappedToWeaponSlotPDA.FireOutput( this, this ); break;
	}
}

void CTrainingModeLogic::OnPlayerWantsToContinue()
{
	if ( m_waitingForKeypressTimer.Get() != NULL )
	{
		m_waitingForKeypressTimer->FireNamedOutput( "OnTimer", variant_t(), this, this );
		m_waitingForKeypressTimer = NULL;
		TFGameRules()->SetIsWaitingForTrainingContinue( false );
	}
}

void CTrainingModeLogic::OnPlayerBuiltBuilding( CTFPlayer *pPlayer, CBaseObject *pBaseObject )
{
	if ( pBaseObject && NotifyObjectBuiltInSuggestedArea( *pBaseObject ) == false )
	{
		m_outputOnPlayerBuiltOutsideSuggestedArea.FireOutput( pBaseObject, this );
	}
}

void CTrainingModeLogic::OnPlayerUpgradedBuilding( CTFPlayer *pPlayer, CBaseObject *pBaseObject )
{
	if ( pBaseObject )
	{
		NotifyObjectUpgradedInSuggestedArea( *pBaseObject );
	}
}

void CTrainingModeLogic::OnPlayerDetonateBuilding( CTFPlayer *pPlayer, CBaseObject *pBaseObject )
{
	m_outputOnPlayerDetonateBuilding.FireOutput( pPlayer, pBaseObject );
}

void CTrainingModeLogic::UpdateHUDObjective()
{
	if ( m_objText[0] != 0 )
	{
		SetTrainingObjective( m_objText );
	}
	else
	{
		SetTrainingObjective("");
	}
}

const char* CTrainingModeLogic::GetNextMap()
{
	return m_nextMapName.ToCStr();
}

const char* CTrainingModeLogic::GetTrainingEndText()
{
	return m_endTrainingText.ToCStr();
}

int CTrainingModeLogic::GetDesiredClass() const
{
	return training_class.GetInt();
}

void CTrainingModeLogic::InputForcePlayerSpawnAsClassOutput( inputdata_t &inputdata )
{
	// This is a bit weird, but we will call this for every player--bots should be ignored
	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;
		OnPlayerSpawned( pPlayer );
	}
}

void CTrainingModeLogic::InputKickAllBots( inputdata_t &inputdata )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsFakeClient() )
		{
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );
		}
	}
}

void CTrainingModeLogic::InputShowTrainingMsg( inputdata_t &inputdata )
{
	if ( TFGameRules()->IsInTraining() )
	{
		SetTrainingMsg( inputdata.value.String() );
	}
}

void CTrainingModeLogic::InputShowTrainingObjective( inputdata_t &inputdata )
{
	if ( !TFGameRules()->IsInTraining() )
		return;
  
	//First try to find the unicode string to send over.
	wchar_t *strPtr = NULL;
	strPtr = g_pVGuiLocalize ? g_pVGuiLocalize->Find( inputdata.value.String() ) : NULL;

	if (NULL == strPtr)
	{
		V_strcpy_safe(m_objText, inputdata.value.String());
	}
	else
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI(strPtr, m_objText, kMaxLengthObjectiveText);
	}
	
	UpdateHUDObjective();
}

void CTrainingModeLogic::InputShowTrainingHUD( inputdata_t &inputdata )
{
	if ( !TFGameRules()->IsInTraining() )
		return;
	TFGameRules()->SetTrainingHUDVisible( true );
}

void CTrainingModeLogic::InputHideTrainingHUD( inputdata_t &inputdata )
{
	if ( !TFGameRules()->IsInTraining() )
		return;
	TFGameRules()->SetTrainingHUDVisible( false );
}

void CTrainingModeLogic::InputEndTraining( inputdata_t &inputdata )
{
	if ( !TFGameRules()->IsInTraining() )
		return;

	TFGameRules()->SetAllowTrainingAchievements( true );

	m_endTrainingText = inputdata.value.StringID();

	CTFPlayer* pHumanPlayer = ToTFPlayer( UTIL_GetListenServerHost() );
	    
	if (NULL == pHumanPlayer) return;
	
	int iTeam = pHumanPlayer->GetTeamNumber();
	
	bool force_map_reset = true;
	CTeamplayRoundBasedRules *pGameRules = dynamic_cast<CTeamplayRoundBasedRules *>( GameRules() );
	pGameRules->SetWinningTeam( iTeam, pGameRules->GetWinReason(), force_map_reset );

	// Show a training win screen so send that event instead.
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "training_complete" );
	if ( winEvent )
	{
		winEvent->SetString( "map", STRING( gpGlobals->mapname ) );
		winEvent->SetString( "next_map", GetNextMap() );
		winEvent->SetString( "text", GetTrainingEndText() );
		
		gameeventmanager->FireEvent( winEvent );
	}
}

void CTrainingModeLogic::InputPlaySoundOnPlayer( inputdata_t &inputdata )
{
	if ( !TFGameRules()->IsInTraining() )
		return;
	CTFPlayer* pHumanPlayer = ToTFPlayer( UTIL_GetListenServerHost() );

	if (NULL == pHumanPlayer) 
		return;

	pHumanPlayer->EmitSound( inputdata.value.String() );
}

void CTrainingModeLogic::InputWaitForTimerOrKeypress( inputdata_t &inputdata )
{
	if ( !TFGameRules()->IsInTraining() )
		return;

	m_waitingForKeypressTimer = gEntList.FindEntityByName( NULL, inputdata.value.String() );
	TFGameRules()->SetIsWaitingForTrainingContinue( m_waitingForKeypressTimer.Get() != NULL );
}

void CTrainingModeLogic::InputSetNextMap( inputdata_t &inputdata )
{
	m_nextMapName = AllocPooledString( inputdata.value.String() );
}

void CTrainingModeLogic::InputForcePlayerSwapToWeapon( inputdata_t &inputdata )
{
	CTFPlayer* pHumanPlayer = ToTFPlayer( UTIL_GetListenServerHost() );

	if (NULL == pHumanPlayer) 
		return;

	CBaseCombatWeapon *pWeapon = NULL;

	if ( FStrEq( inputdata.value.String(), "primary" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
	}
	else if ( FStrEq( inputdata.value.String(), "secondary" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY );
	}
	else if ( FStrEq( inputdata.value.String(), "melee" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	}
	else if ( FStrEq( inputdata.value.String(), "grenade" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_GRENADE );
	}
	else if ( FStrEq( inputdata.value.String(), "building" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_BUILDING );
	}
	else if ( FStrEq( inputdata.value.String(), "pda" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_PDA );
	}
	else if ( FStrEq( inputdata.value.String(), "item1" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_ITEM1 );
	}
	else if ( FStrEq( inputdata.value.String(), "item2" ) )
	{
		pWeapon = pHumanPlayer->Weapon_GetSlot( TF_WPN_TYPE_ITEM2 );
	}

	if ( pWeapon )
	{
		pHumanPlayer->Weapon_Switch( pWeapon );
	}

}

LINK_ENTITY_TO_CLASS( tf_logic_multiple_escort, CMultipleEscort );
LINK_ENTITY_TO_CLASS( tf_logic_hybrid_ctf_cp, CHybridMap_CTF_CP );
LINK_ENTITY_TO_CLASS( tf_logic_medieval, CMedievalLogic );


BEGIN_DATADESC(CTFHolidayEntity)
	DEFINE_KEYFIELD( m_nHolidayType, FIELD_INTEGER,	"holiday_type" ),
	DEFINE_KEYFIELD( m_nTauntInHell, FIELD_INTEGER,	"tauntInHell" ),
	DEFINE_KEYFIELD( m_nAllowHaunting, FIELD_INTEGER, "allowHaunting" ),
	
	DEFINE_INPUTFUNC( FIELD_INTEGER, "HalloweenSetUsingSpells", InputHalloweenSetUsingSpells ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"Halloween2013TeleportToHell", InputHalloweenTeleportToHell ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_logic_holiday, CTFHolidayEntity );


void CTFHolidayEntity::InputHalloweenSetUsingSpells( inputdata_t &inputdata )
{
	if ( !TFGameRules() )
		return;

	TFGameRules()->SetUsingSpells( ( inputdata.value.Int() == 0 ) ? false : true );
}

void CTFHolidayEntity::InputHalloweenTeleportToHell( inputdata_t &inputdata )
{
	m_nWinningTeam = FStrEq( "red", inputdata.value.String() ) ? TF_TEAM_RED : TF_TEAM_BLUE;

	CUtlVector< CTFPlayer * > vecPlayers;
	CollectPlayers( &vecPlayers, TEAM_ANY, false );

	FOR_EACH_VEC( vecPlayers, i )
	{
		CTFPlayer *pPlayer = vecPlayers[i];
		// Only do these effects if the player is alive
		if ( !pPlayer->IsAlive() )
			continue;

		// Fade to white
		color32 fadeColor = {255,255,255,255};
		UTIL_ScreenFade( pPlayer, fadeColor, 2.f, 0.5, FFADE_OUT | FFADE_PURGE );

		// Do a zoom in effect
		pPlayer->SetFOV( pPlayer, 10.f, 2.5f, 0.f );
		// Rumble like something important happened
		UTIL_ScreenShake(pPlayer->GetAbsOrigin(), 100.f, 150, 4.f, 0.f, SHAKE_START, true );
	}

	// Play a sound for all players
	TFGameRules()->BroadcastSound( 255, "Halloween.hellride" );

	SetContextThink( &CTFHolidayEntity::Teleport, gpGlobals->curtime + 2.5f, "TeleportToHell" );
}

void CTFHolidayEntity::Teleport() 
{
	RemoveAll2013HalloweenTeleportSpellsInMidFlight();

	const char *pszRedString	= ( m_nWinningTeam == TF_TEAM_RED ) ? "winner" : "loser";
	const char *pszBlueString	= ( m_nWinningTeam == TF_TEAM_BLUE ) ? "winner" : "loser";

	CUtlVector< CTFPlayer* > vecTeleportedPlayers;
	TFGameRules()->TeleportPlayersToTargetEntities( TF_TEAM_RED, CFmtStr( "spawn_loot_%s" , pszRedString ), &vecTeleportedPlayers );
	TFGameRules()->TeleportPlayersToTargetEntities( TF_TEAM_BLUE, CFmtStr( "spawn_loot_%s" , pszBlueString ), &vecTeleportedPlayers );

	// clear dancer
	m_vecDancers.RemoveAll();

	// remove players' projectiles and buildings from world
	TFGameRules()->RemoveAllProjectilesAndBuildings();

	FOR_EACH_VEC( vecTeleportedPlayers, i )
	{
		CTFPlayer *pPlayer = vecTeleportedPlayers[i];

		// Roll a new, low-tier spell
		CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pPlayer->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
		if ( pSpellBook )
		{
			pSpellBook->ClearSpell();
			if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
			{
				pSpellBook->RollNewSpell( 0, true );
			}
		}

		// Do a zoom effect
		pPlayer->SetFOV( pPlayer, tf_teleporter_fov_start.GetInt() );
		pPlayer->SetFOV( pPlayer, 0, 1.f, tf_teleporter_fov_start.GetInt() );

		// Screen flash
		color32 fadeColor = {255,255,255,100};
		UTIL_ScreenFade( pPlayer, fadeColor, 0.25, 0.4, FFADE_IN );

		const float flDanceTime = 6.f;

		if ( ShouldTauntInHell() || ( TFGameRules()->GetHalloweenScenario() == CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
		{
			pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_THRILLER, flDanceTime );
		}

		pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_IN_HELL );

		// Losers get their health set to max.  Winners get overhealed
		bool bIsWinner = ( pPlayer->GetTeamNumber() == m_nWinningTeam );
		float flMax = bIsWinner ? ( pPlayer->GetMaxHealth() * 1.6f ) : ( pPlayer->GetMaxHealth() * 1.1 );
		float flToHeal = flMax - pPlayer->GetHealth();
		// Overheal the winning team, and just restore the losing team to full health
		pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_HELL_HEAL, flDanceTime );
		pPlayer->m_Shared.Heal( pPlayer, flToHeal / flDanceTime, bIsWinner ? 1.5f : 1.f, 1.0f );

		// Give them full ammo
		pPlayer->GiveAmmo( 1000, TF_AMMO_PRIMARY );
		pPlayer->GiveAmmo( 1000, TF_AMMO_SECONDARY );
		pPlayer->GiveAmmo( 1000, TF_AMMO_METAL );
		pPlayer->GiveAmmo( 1000, TF_AMMO_GRENADES1 );
		pPlayer->GiveAmmo( 1000, TF_AMMO_GRENADES2 );
		pPlayer->GiveAmmo( 1000, TF_AMMO_GRENADES3 );

		// Refills weapon clips, too
		for ( int i = 0; i < MAX_WEAPONS; i++ )
		{
			CTFWeaponBase *pWeapon = assert_cast< CTFWeaponBase* >( pPlayer->GetWeapon( i ) );
			if ( !pWeapon )
				continue;

			pWeapon->GiveDefaultAmmo();

			if ( pWeapon->IsEnergyWeapon() )
			{
				pWeapon->WeaponRegenerate();
			}
		}

		m_vecDancers.AddToTail( pPlayer );
	}

	// Set this flag.  Lets us check elsewhere if it's hell time
	if ( TFGameRules() )
	{
		TFGameRules()->SetPlayersInHell( true );
	}

	if ( ShouldTauntInHell() || ( TFGameRules()->GetHalloweenScenario() == CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		const float flThinkDelay = 0.5f;
		const float flDanceDuration = 2.75f;

		SetContextThink( &CTFHolidayEntity::HalloweenTeleportToHellDanceThink, gpGlobals->curtime + flThinkDelay, "DanceThink1" );
		SetContextThink( &CTFHolidayEntity::HalloweenTeleportToHellDanceThink, gpGlobals->curtime + flThinkDelay + flDanceDuration, "DanceThink2" );
	}
}


void CTFHolidayEntity::HalloweenTeleportToHellDanceThink( void )
{
	FOR_EACH_VEC( m_vecDancers, i )
	{
		CTFPlayer* pPlayer = m_vecDancers[i];
		if ( !pPlayer )
			continue;

		// Dance
		pPlayer->Taunt();
	}
}

void CTFHolidayEntity::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

#ifdef GAME_DLL
	if ( !Q_strcmp( eventName, "player_turned_to_ghost" ) 
		|| !Q_strcmp( eventName, "player_disconnect" )
		|| !Q_strcmp( eventName, "player_team" ))
	{
		if ( TFGameRules()->ArePlayersInHell() )
		{
			CUtlVector< CTFPlayer * > vecPlayers;
			CollectPlayers( &vecPlayers, TF_TEAM_RED, true );
			CollectPlayers( &vecPlayers, TF_TEAM_BLUE, true, true );

			FOR_EACH_VEC( vecPlayers, i )
			{
				// If everyone is a ghost
				if ( !vecPlayers[i]->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
					return;
			}

			// Everyone is a ghost.  Stalemate!
			TFGameRules()->SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, true, false );
		}
	}
#endif
}

BEGIN_DATADESC(CKothLogic)
	DEFINE_KEYFIELD( m_nTimerInitialLength,		FIELD_INTEGER,	"timer_length" ),
	DEFINE_KEYFIELD( m_nTimeToUnlockPoint,		FIELD_INTEGER,	"unlock_point" ),

	DEFINE_INPUTFUNC( FIELD_VOID,		"RoundSpawn",		InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"RoundActivate",	InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetRedTimer",		InputSetRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetBlueTimer",		InputSetBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"AddRedTimer",		InputAddRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"AddBlueTimer",		InputAddBlueTimer ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_logic_koth, CKothLogic );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKothLogic::InputRoundSpawn( inputdata_t &input )
{
	if ( TFGameRules() && TFGameRules()->IsInKothMode() )
	{
		// create the koth team_round_timer entities
		variant_t sVariant;
		sVariant.SetInt( m_nTimerInitialLength );

		CTeamRoundTimer *pTimer = NULL;

		pTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
		if ( pTimer )
		{
			TFGameRules()->SetKothTeamTimer( TF_TEAM_BLUE, pTimer );
			pTimer->SetName( MAKE_STRING( "zz_blue_koth_timer" ) );
			pTimer->SetShowInHud( true );
			pTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );

			m_hBlueTimer = pTimer;
		}

		pTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
		if ( pTimer )
		{
			TFGameRules()->SetKothTeamTimer( TF_TEAM_RED, pTimer );
			pTimer->SetName( MAKE_STRING( "zz_red_koth_timer" ) );
			pTimer->SetShowInHud( true );
			pTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			pTimer->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );

			m_hRedTimer = pTimer;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKothLogic::InputRoundActivate( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsInKothMode() )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster )
		{
			variant_t sVariant;
			sVariant.SetInt( m_nTimeToUnlockPoint );

			for ( int i = 0 ; i < pMaster->GetNumPoints() ; i++ )
			{
				CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
				if ( pPoint )
				{
					pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );

					if ( m_nTimeToUnlockPoint > 0 )
					{
						pPoint->AcceptInput( "SetUnlockTime", NULL, NULL, sVariant, 0 );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKothLogic::InputSetRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsInKothMode() )
	{
		if ( m_hRedTimer )
		{
			m_hRedTimer->SetTimeRemaining( inputdata.value.Int() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKothLogic::InputSetBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsInKothMode() )
	{
		if ( m_hBlueTimer )
		{
			m_hBlueTimer->SetTimeRemaining( inputdata.value.Int() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKothLogic::InputAddRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsInKothMode() )
	{
		if ( m_hRedTimer )
		{
			m_hRedTimer->AddTimerSeconds( inputdata.value.Int() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CKothLogic::InputAddBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsInKothMode() )
	{
		if ( m_hBlueTimer )
		{
			m_hBlueTimer->AddTimerSeconds( inputdata.value.Int() );
		}
	}
}

BEGIN_DATADESC(CCPTimerLogic)
	DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),
	DEFINE_KEYFIELD( m_nTimerLength, FIELD_INTEGER,	"timer_length" ),
	DEFINE_KEYFIELD( m_nTimerTeam, FIELD_INTEGER, "team_number" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	DEFINE_OUTPUT( m_onCountdownStart, "OnCountdownStart" ),
	DEFINE_OUTPUT( m_onCountdown15SecRemain, "OnCountdown15SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown10SecRemain, "OnCountdown10SecRemain" ),
	DEFINE_OUTPUT( m_onCountdown5SecRemain, "OnCountdown5SecRemain" ),
	DEFINE_OUTPUT( m_onCountdownEnd, "OnCountdownEnd" ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_logic_cp_timer, CCPTimerLogic );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCPTimerLogic::InputRoundSpawn( inputdata_t &input )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		// We need to re-find our control point, because they're recreated over round restarts
		m_hControlPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( !m_hControlPoint )
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCPTimerLogic::TimerMayExpire( void )
{
	if ( m_hControlPoint )
	{
		if ( TeamplayGameRules()->TeamMayCapturePoint( m_nTimerTeam, m_hControlPoint->GetPointIndex() ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCPTimerLogic::Think( void )
{
	if ( !TFGameRules() || !ObjectiveResource() )
		return;
	
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		// game has already been won, our job is done
		m_pointTimer.Invalidate();
		SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, CP_TIMER_THINK );
	}

	if ( m_hControlPoint )
	{
		if ( TeamplayGameRules()->TeamMayCapturePoint( m_nTimerTeam, m_hControlPoint->GetPointIndex() ) )
		{
			if ( !m_pointTimer.HasStarted() )
			{
				m_pointTimer.Start( m_nTimerLength );
				m_onCountdownStart.FireOutput( this, this );

				ObjectiveResource()->SetCPTimerTime( m_hControlPoint->GetPointIndex(), gpGlobals->curtime + m_nTimerLength );
			}
			else
			{
				if ( m_pointTimer.IsElapsed() )
				{
					// the point must be fully owned by the owner before we reset
					if ( ObjectiveResource()->GetCappingTeam( m_hControlPoint->GetPointIndex() ) == TEAM_UNASSIGNED )
					{
						m_pointTimer.Invalidate();
						m_onCountdownEnd.FireOutput( this, this );
						m_bFire15SecRemain = m_bFire10SecRemain = m_bFire5SecRemain = true;

						ObjectiveResource()->SetCPTimerTime( m_hControlPoint->GetPointIndex(), -1.0f );
					}
				}
				else 
				{
					float flRemainingTime = m_pointTimer.GetRemainingTime();

					if ( flRemainingTime <= 15.0f && m_bFire15SecRemain )
					{
						m_bFire15SecRemain = false;
					}
					else if ( flRemainingTime <= 10.0f && m_bFire10SecRemain )
					{
						m_bFire10SecRemain = false;
					}
					else if ( flRemainingTime <= 5.0f && m_bFire5SecRemain )
					{
						m_bFire5SecRemain = false;
					}
				}
			}
		}
		else
		{
			m_pointTimer.Invalidate();
		}
	}

	SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, CP_TIMER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::AddPlayerToQueue( CTFPlayer *pPlayer )
{
	//Already in Queue
	if ( m_hArenaPlayerQueue.Find( pPlayer ) != m_hArenaPlayerQueue.InvalidIndex() )
		return;

	if ( pPlayer->IsArenaSpectator() == true )
		return;

//	Msg( "AddPlayerToQueue:: Adding to queue: %s\n", pPlayer->GetPlayerName() );

	m_hArenaPlayerQueue.AddToTail( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::AddPlayerToQueueHead( CTFPlayer *pPlayer )
{
	//Already in Queue
	if ( m_hArenaPlayerQueue.Find( pPlayer ) != m_hArenaPlayerQueue.InvalidIndex() )
		return;

	m_hArenaPlayerQueue.AddToHead( pPlayer );

//	Msg( "AddPlayerToQueueHead:: Adding to queue: %s\n", pPlayer->GetPlayerName() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RemovePlayerFromQueue( CTFPlayer *pPlayer )
{
	//Not in list?
	if ( m_hArenaPlayerQueue.Find( pPlayer ) == m_hArenaPlayerQueue.InvalidIndex() )
		return;

	m_hArenaPlayerQueue.FindAndRemove( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnNavMeshLoad( void )
{
	TheNavMesh->SetPlayerSpawnName( "info_player_teamspawn" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDispenserBuilt( CBaseEntity *dispenser )
{
	if ( !m_healthVector.HasElement( dispenser ) )
	{
		m_healthVector.AddToTail( dispenser );
	}

	if ( !m_ammoVector.HasElement( dispenser ) )
	{
		m_ammoVector.AddToTail( dispenser );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDispenserDestroyed( CBaseEntity *dispenser )
{
	m_healthVector.FindAndFastRemove( dispenser );
	m_ammoVector.FindAndFastRemove( dispenser );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhysicsProp *CreateBeachBall( const Vector &vSpawnPos, const QAngle &qSpawnAngles );
CPhysicsProp *CreateSoccerBall( const Vector &vSpawnPos, const QAngle &qSpawnAngles );


static bool CanFindBallSpawnLocation( const Vector& vSearchOrigin, Vector *out_pvDropSpot )
{
	// find clear space to drop the ball
	for( float angle = 0.0f; angle < 2.0f * M_PI; angle += 0.2f )
	{
		Vector forward;
		FastSinCos( angle, &forward.y, &forward.x );
		forward.z = 0.0f;					

		const float ballRadius = 16.0f;
		const float playerRadius = 20.0f;

		Vector hullMins( -ballRadius, -ballRadius, -ballRadius );
		Vector hullMaxs( ballRadius, ballRadius, ballRadius );

		Vector dropSpot = vSearchOrigin + 1.2f * ( playerRadius + ballRadius ) * forward;

		trace_t result;
		UTIL_TraceHull( dropSpot, dropSpot, hullMins, hullMaxs, MASK_PLAYERSOLID, NULL, &result );

		if ( !result.DidHit() )
		{
			*out_pvDropSpot = dropSpot;
			return true;
		}
	}

	return false;
}

void CTFGameRules::OnPlayerSpawned( CTFPlayer *pPlayer )
{
	// coach?
	CSteamID steamIDForPlayer;
	if ( pPlayer->GetSteamID( &steamIDForPlayer ) )
	{
		// find out if we are supposed to coach
		int idx = m_mapCoachToStudentMap.Find( steamIDForPlayer.GetAccountID() );
		if ( m_mapCoachToStudentMap.IsValidIndex( idx ) )
		{
			// find student
			uint32 studentAccountID = m_mapCoachToStudentMap[idx];
			CSteamID steamIDForStudent;
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CTFPlayer *pPotentialStudent = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( NULL == pPotentialStudent )
				{
					continue;
				}
				// is this the student?
				if ( pPotentialStudent->GetSteamID( &steamIDForStudent ) && steamIDForStudent.GetAccountID() == studentAccountID )
				{
					Coaching_Start( pPlayer, pPotentialStudent );
					// @todo (Tom Bui): Not sure this is required--nothing seems to use it
					// engine->ClientCommand( pPlayer->edict(), "cl_spec_mode %d", OBS_MODE_IN_EYE );
					// finally, notify the GC
					GCSDK::CProtoBufMsg< CMsgTFCoaching_CoachJoined > msg( k_EMsgGCCoaching_CoachJoined );
					msg.Body().set_account_id_coach( steamIDForPlayer.GetAccountID() );
					GCClientSystem()->BSendMessage( msg );
					break;
				}
			}
			// remove from the map now so the coach can join later as a normal player if they DC
			m_mapCoachToStudentMap.RemoveAt( idx );
		}
	}
	// warp coach to student?
	if ( pPlayer->GetCoach() )
	{
		// warp the coach to student
		pPlayer->GetCoach()->SetObserverTarget( pPlayer );
		pPlayer->GetCoach()->StartObserverMode( OBS_MODE_CHASE );
	}

	// notify training
	if ( m_hTrainingModeLogic )
	{
		m_hTrainingModeLogic->OnPlayerSpawned( pPlayer );
	}

#ifdef GAME_DLL
	if ( !IsInTraining() )
	{
		// Birthday beachball ball spawning.
		if ( IsBirthday() &&
			 !m_hasSpawnedToy &&
			 pPlayer->GetTeamNumber() == TF_TEAM_BLUE &&				// always give ball to first blue player, since they are often trapped during setup
			 RandomInt( 0, 100 ) < tf_birthday_ball_chance.GetInt() )
		{
			Vector vDropSpot;
			if ( CanFindBallSpawnLocation( pPlayer->WorldSpaceCenter(), &vDropSpot ) )
			{
				CPhysicsProp *ball = CreateBeachBall( vDropSpot, pPlayer->GetAbsAngles() );
				if ( ball )
				{
					m_hasSpawnedToy = true;

					// turn on the birthday skin
					ball->m_nSkin = 1;
				}
			}
		}

		// Soccer ball spawning if wearing soccer cleats.
		if ( !m_bHasSpawnedSoccerBall[ pPlayer->GetTeamNumber() ] )
		{
			enum
			{
				kSpawnWith_Nothing = 0,
				kSpawnWith_SoccerBall = 1,
			};

			int iSpawnWithPhysicsToy = kSpawnWith_Nothing;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iSpawnWithPhysicsToy, spawn_with_physics_toy );
			if ( iSpawnWithPhysicsToy == kSpawnWith_SoccerBall )
			{
				Vector vDropSpot;
				if ( CanFindBallSpawnLocation( pPlayer->WorldSpaceCenter(), &vDropSpot ) )
				{
					CPhysicsProp *ball = CreateSoccerBall( vDropSpot, pPlayer->GetAbsAngles() );
					if ( ball )
					{
						m_bHasSpawnedSoccerBall[ pPlayer->GetTeamNumber() ] = true;

						// turn on the birthday skin
						ball->m_nSkin = pPlayer->GetTeamNumber() == TF_TEAM_BLUE ? 1 : 0;
					}
				}
			}
		}
	}
#endif
}


class CGCCoaching_CoachJoining : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_CoachJoining( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgTFCoaching_CoachJoining > msg( pNetPacket );
		if ( TFGameRules() )
		{
			TFGameRules()->OnCoachJoining( msg.Body().account_id_coach(), msg.Body().account_id_student() );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_CoachJoining, "CGCCoaching_CoachJoining", k_EMsgGCCoaching_CoachJoining, GCSDK::k_EServerTypeGCClient );

class CGCCoaching_RemoveCurrentCoach : public GCSDK::CGCClientJob
{
public:
	CGCCoaching_RemoveCurrentCoach( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgTFCoaching_RemoveCurrentCoach > msg( pNetPacket );
		if ( TFGameRules() )
		{
			TFGameRules()->OnRemoveCoach( msg.Body().account_id_coach() );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCCoaching_RemoveCurrentCoach, "CGCCoaching_RemoveCurrentCoach", k_EMsgGCCoaching_RemoveCurrentCoach, GCSDK::k_EServerTypeGCClient );

class CGCUseServerModificationItemJob : public GCSDK::CGCClientJob
{
public:
	CGCUseServerModificationItemJob( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGC_GameServer_UseServerModificationItem> msg( pNetPacket );

		// If this server doesn't have the capability to call a vote right now for whatever reason, we
		// give up and return immediate failure to the GC. If the vote gets called, we'll send up pass/fail
		// when it finishes.
		if ( !g_voteControllerGlobal || !g_voteControllerGlobal->CreateVote( DEDICATED_SERVER, "eternaween", "" ) )
		{
			GCSDK::CProtoBufMsg<CMsgGC_GameServer_UseServerModificationItem_Response> msgResponse( k_EMsgGC_GameServer_UseServerModificationItem_Response );
			msgResponse.Body().set_server_response_code( CMsgGC_GameServer_UseServerModificationItem_Response::kServerModificationItemServerResponse_NoVoteCalled );
			m_pGCClient->BSendMessage( msgResponse );
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCUseServerModificationItemJob, "CGCUseServerModificationItemJob", k_EMsgGC_GameServer_UseServerModificationItem, GCSDK::k_EServerTypeGCClient );

class CGCUpdateServerModificationItemStateJob : public GCSDK::CGCClientJob
{
public:
	CGCUpdateServerModificationItemStateJob( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGC_GameServer_ServerModificationItem> msg( pNetPacket );

		switch ( msg.Body().modification_type() )
		{
		case kGameServerModificationItem_Halloween:
			tf_item_based_forced_holiday.SetValue( msg.Body().active() ? kHoliday_Halloween : kHoliday_None );
			g_fEternaweenAutodisableTime = engine->Time() + (SERVER_MODIFICATION_ITEM_DURATION_IN_MINUTES * 60.0f);
			if ( TFGameRules() )
			{
				TFGameRules()->FlushAllAttributeCaches();
			}
			break;
		default:
			Warning( "%s: unknown modification type %u for server item.\n", __FUNCTION__, msg.Body().modification_type() );
			break;
		}
		
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCUpdateServerModificationItemStateJob, "CGCUpdateServerModificationItemStateJob", k_EMsgGC_GameServer_ModificationItemState, GCSDK::k_EServerTypeGCClient );

#ifdef _DEBUG
CON_COMMAND( coaching_stop, "Stop coaching" )
{
	CTFPlayer* pCoach = ToTFPlayer( UTIL_GetListenServerHost() );
	Coaching_Stop( pCoach );
}

CON_COMMAND( coaching_remove_coach, "Remove current coach" )
{
	CTFPlayer* pStudent = ToTFPlayer( UTIL_GetListenServerHost() );
	CTFPlayer *pCoach = pStudent->GetCoach();
	if ( pCoach )
	{
		Coaching_Stop( pCoach );
	}
}

CON_COMMAND( coaching_force_coach, "Force self as coach" )
{
	CTFPlayer* pCoachPlayer = ToTFPlayer( UTIL_GetListenServerHost() );
	if ( pCoachPlayer && TFGameRules() )
	{
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pStudentPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pStudentPlayer != pCoachPlayer )
			{
				Coaching_Start( pCoachPlayer, pStudentPlayer );
				break;
			}
		}
	}
}

CON_COMMAND( coaching_force_student, "Force self as student" )
{
	CTFPlayer* pStudentPlayer = ToTFPlayer( UTIL_GetListenServerHost() );
	if ( pStudentPlayer && TFGameRules() )
	{
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pCoachPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pCoachPlayer != pStudentPlayer )
			{
				Coaching_Start( pCoachPlayer, pStudentPlayer );
				break;
			}
		}
	}
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnCoachJoining( uint32 unCoachAccountID, uint32 unStudentAccountID )
{
	m_mapCoachToStudentMap.Insert( unCoachAccountID, unStudentAccountID );
	// see if the coach is on the server already
	CSteamID steamIDForCoach;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPotentialCoach = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( NULL == pPotentialCoach )
		{
			continue;
		}
		// coach is here, force them to respawn, which will set them up as a coach
		if ( pPotentialCoach->GetSteamID( &steamIDForCoach ) && steamIDForCoach.GetAccountID() == unCoachAccountID )
		{
			pPotentialCoach->ForceRespawn();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnRemoveCoach( uint32 unCoachAccountID )
{
	m_mapCoachToStudentMap.Remove( unCoachAccountID );
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		CSteamID steamID;
		if ( pPlayer && pPlayer->GetSteamID( &steamID ) && steamID.GetAccountID() == unCoachAccountID )
		{
			Coaching_Stop( pPlayer );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activates 100% crits for an entire team for a short period of time
//-----------------------------------------------------------------------------
void CTFGameRules::HandleCTFCaptureBonus( int nTeam )
{
	float flBonusTime = GetCTFCaptureBonusTime();
	
	if ( flBonusTime <= 0 )
		return;

	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() == nTeam )
		{
			pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_CTF_CAPTURE, flBonusTime );
		}
	}
}
#endif

int CTFGameRules::GetStatsMinimumPlayers( void )
{
	if ( IsInArenaMode() == true )
	{
		return 1;
	}

	return 3;
}

int CTFGameRules::GetStatsMinimumPlayedTime( void )
{
	if ( IsInArenaMode() == true )
	{
		return tf_arena_preround_time.GetFloat();
	}

	return 4 * 60; //Default of 4 minutes
}

bool CTFGameRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
	CTFPlayer* pTFPlayer = NULL;
#ifdef GAME_DLL
	pTFPlayer = ToTFPlayer( pPlayer );
#else
	pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
#endif

	if( pTFPlayer )
	{
		// We can change if we're not alive
		if( pTFPlayer->m_lifeState != LIFE_ALIVE )
			return true;
		
		// We can change if we're not on team red or blue
		int iPlayerTeam = pTFPlayer->GetTeamNumber();
		if( ( iPlayerTeam != TF_TEAM_RED ) && ( iPlayerTeam != TF_TEAM_BLUE ) )
			return true;
		
		// We can change if we've respawned/changed classes within the last 2 seconds.
		// This allows for <classname>.cfg files to change these types of convars
		float flRespawnTime = 0.f;
#ifdef GAME_DLL
		flRespawnTime = pTFPlayer->GetSpawnTime();	// Called everytime the player respawns
#else
		flRespawnTime = pTFPlayer->GetClassChangeTime(); // Called when the player changes class and respawns
#endif
		if( ( gpGlobals->curtime - flRespawnTime ) < 2.f )
			return true;

		// CTFPlayerShared has an option to suppress prediction. If the client is trying to change itself to match that
		// requested state, it needs to be allowed to avoid prediction desync.
		bool bShouldBePredicting = !pTFPlayer->m_Shared.ShouldSuppressPrediction();

#ifdef GAME_DLL
		// server - if client's new userinfo is attempting to change their prediction state to the value CTFPlayerShared
		// wants, allow it regardless.
		bool bIsPredicting = pTFPlayer->m_bRequestPredict;
		bool bRequestingPredict = Q_atoi( engine->GetClientConVarValue( pTFPlayer->entindex(), "cl_predict" ) ) != 0;
		if ( bShouldBePredicting != bIsPredicting && bRequestingPredict == bShouldBePredicting )
			{ return true; }
#else
		// client - If we're trying to change our cl_predict to what CTFPlayerShared wants, it's allowed
		static ConVarRef cl_predict( "cl_predict" );
		bool bIsPredicting = cl_predict.GetBool();
		if ( bShouldBePredicting != bIsPredicting )
		{
			return true;
		}
#endif
	}

	return false;
}

//========================================================================================================================
// BONUS ROUND HANDLING
//========================================================================================================================
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldGoToBonusRound( void )
{
	// Only do this on a Valve official server. Use the presence of our DLL as a key.
	return false;

	if ( IsInTournamentMode() )
		return false;

	// Don't do this on empty servers
	if ( !BHavePlayers() )
		return false;
	if ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetNumPlayers() <= 0 )
		return false;
	if ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetNumPlayers() <= 0 )
		return false;

	// Random chance per round, based on time.
	float flRoundTime = gpGlobals->curtime - m_flRoundStartTime;
	float flChance = RemapValClamped( flRoundTime, (3 * 60), (30 * 60), 0.0, 0.75 );	// 75% chance for > 30 min rounds, down to 0% at 3 min rounds
	float flRoll = RandomFloat( 0, 1 );
	return ( flRoll < flChance );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnBonusStart( void )
{
	m_hBonusLogic.Set( dynamic_cast<CBonusRoundLogic*>(CreateEntityByName( "tf_logic_bonusround" )) );
	if ( !m_hBonusLogic.Get()->InitBonusRound() )
	{
		State_Transition( GR_STATE_PREROUND );
		return;
	}

	// Bring up the giveaway panel on all the players
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_GIVEAWAY_ITEM );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnBonusEnd( void )
{
	if ( m_hBonusLogic.Get() )
	{
		UTIL_Remove( m_hBonusLogic.Get() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BonusStateThink( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: We need to abort the bonus state because our item generation failed.
//			Steam is probably down. 
//-----------------------------------------------------------------------------
void CTFGameRules::BonusStateAbort( void )
{
	if ( m_hBonusLogic.Get() )
	{
		m_hBonusLogic.Get()->SetBonusStateAborted( true );
	}

	State_Transition( GR_STATE_PREROUND );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BetweenRounds_Start( void )
{
	SetSetup( true );

	if ( IsMannVsMachineMode() )
	{
		mp_tournament.SetValue( true );
		RestartTournament();
		SetInStopWatch( false );

		char szName[16];
		Q_strncpy( szName, "ROBOTS", MAX_TEAMNAME_STRING + 1 );
		mp_tournament_blueteamname.SetValue( szName );
		Q_strncpy( szName, "MANNCO", MAX_TEAMNAME_STRING + 1 );
		mp_tournament_redteamname.SetValue( szName );
		SetTeamReadyState( true, TF_TEAM_PVE_INVADERS );
	}

	for ( int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast<CBaseObject*>( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->IsDisposableBuilding() || pObj->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			pObj->DetonateObject();
		}
	}

	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->StateEnterBetweenRounds();
	}

	if ( m_hCompetitiveLogicEntity )
	{
		m_hCompetitiveLogicEntity->OnSpawnRoomDoorsShouldUnlock();
	}

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc && pMatchDesc->BUsesAutoReady() )
	{
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pPlayer = static_cast<CTFPlayer*>( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			if ( IsValidTFTeam( pPlayer->GetTeamNumber() ) && pPlayer->GetPlayerClass() && IsValidTFPlayerClass( pPlayer->GetPlayerClass()->GetClassIndex() ) )
			{
				PlayerReadyStatus_UpdatePlayerState( pPlayer, true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BetweenRounds_End( void )
{
	SetInWaitingForPlayers( false );
	SetSetup( false );

	if ( IsMannVsMachineMode() )
	{
		SetInStopWatch( false );
		mp_tournament_stopwatch.SetValue( false );
	}

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		// We don't consider inactivity during BetweenRounds as idle
		pPlayer->ResetIdleCheck();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BetweenRounds_Think( void )
{
	if ( UsePlayerReadyStatusMode() )
	{
		// Everyone is ready, or the drop-dead timer naturally ticked down to mp_tournament_readymode_countdown
		bool bStartFinalCountdown = ( PlayerReadyStatus_ShouldStartCountdown() || ( m_flRestartRoundTime > 0 && (int)( m_flRestartRoundTime - gpGlobals->curtime ) == mp_tournament_readymode_countdown.GetInt() ) );

		// It's the FINAL COUNTDOOOWWWNNnnnnnnnnn
		float flDropDeadTime = gpGlobals->curtime + mp_tournament_readymode_countdown.GetFloat() + 0.1f;
		if ( bStartFinalCountdown && ( m_flRestartRoundTime < 0 || m_flRestartRoundTime >= flDropDeadTime ) )
		{
			float flDelay = IsMannVsMachineMode() ? 10.f : mp_tournament_readymode_countdown.GetFloat();
			m_flRestartRoundTime.Set( gpGlobals->curtime + flDelay );
			ShouldResetScores( true, true );
			ShouldResetRoundsPlayed( true );

			if ( IsCompetitiveMode() )
			{
				m_flCompModeRespawnPlayersAtMatchStart = gpGlobals->curtime + 2.0;
			}
		}

		// Required for UI state
		if ( PlayerReadyStatus_HaveMinPlayersToEnable() )
		{
			CheckReadyRestart();
		}
	}
	
	CheckRespawnWaves();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PreRound_Start( void )
{
	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->StateEnterPreRound();
	}

	if ( m_hCompetitiveLogicEntity )
	{
		m_hCompetitiveLogicEntity->OnSpawnRoomDoorsShouldLock();
	}

	BaseClass::PreRound_Start();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PreRound_End( void )
{
	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_HIGHTOWER ) && !IsInWaitingForPlayers() )
	{
		if ( RandomFloat( 0, 1 ) < HELLTOWER_RARE_LINE_CHANCE )
		{
			PlayHelltowerAnnouncerVO( HELLTOWER_VO_RED_ROUNDSTART_RARE, HELLTOWER_VO_BLUE_ROUNDSTART_RARE );
		}
		else
		{
			PlayHelltowerAnnouncerVO( HELLTOWER_VO_RED_ROUNDSTART, HELLTOWER_VO_BLUE_ROUNDSTART );
		}
	}

	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->StateExitPreRound();
	}

	if ( m_hCompetitiveLogicEntity )
	{
		m_hCompetitiveLogicEntity->OnSpawnRoomDoorsShouldUnlock();
	}

	BaseClass::PreRound_End();
}

//-----------------------------------------------------------------------------
// Purpose: Compute internal vectors of health and ammo locations
//-----------------------------------------------------------------------------
void CTFGameRules::ComputeHealthAndAmmoVectors( void )
{
	m_ammoVector.RemoveAll();
	m_healthVector.RemoveAll();

	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		if ( pEnt->ClassMatches( "func_regenerate" ) || pEnt->ClassMatches( "item_healthkit*" ) )
		{
			m_healthVector.AddToTail( pEnt );
		}

		if ( pEnt->ClassMatches( "func_regenerate" ) || pEnt->ClassMatches( "item_ammopack*" ) )
		{
			m_ammoVector.AddToTail( pEnt );
		}

		pEnt = gEntList.NextEnt( pEnt );
	}

	m_areHealthAndAmmoVectorsReady = true;
}


//-----------------------------------------------------------------------------
// Purpose: Return vector of health entities
//-----------------------------------------------------------------------------
const CUtlVector< CHandle< CBaseEntity > > &CTFGameRules::GetHealthEntityVector( void )
{
	// lazy-populate health and ammo vector since some maps (Dario!) move these entities around between stages
	if ( !m_areHealthAndAmmoVectorsReady )
	{
		ComputeHealthAndAmmoVectors();
	}

	return m_healthVector;
}


//-----------------------------------------------------------------------------
// Purpose: Return vector of ammo entities
//-----------------------------------------------------------------------------
const CUtlVector< CHandle< CBaseEntity > > &CTFGameRules::GetAmmoEntityVector( void )
{
	// lazy-populate health and ammo vector since some maps (Dario!) move these entities around between stages
	if ( !m_areHealthAndAmmoVectorsReady )
	{
		ComputeHealthAndAmmoVectors();
	}

	return m_ammoVector;
}


//-----------------------------------------------------------------------------
// Purpose: Return the Payload cart the given team needs to push to win, or NULL if none currently exists
//-----------------------------------------------------------------------------
CHandle< CTeamTrainWatcher > CTFGameRules::GetPayloadToPush( int pushingTeam ) const
{
	if ( TFGameRules()->GetGameType() != TF_GAMETYPE_ESCORT )
		return NULL;

	if ( pushingTeam == TF_TEAM_RED )
	{
		if ( m_redPayloadToPush == NULL )
		{
			// find our cart!
			if ( TFGameRules()->HasMultipleTrains() )
			{
				// find the red cart
			}
			else
			{
				// normal Escort scenario, red always blocks
				return NULL;
			}
		}

		return m_redPayloadToPush;
	}

	if ( pushingTeam == TF_TEAM_BLUE )
	{
		if ( m_bluePayloadToPush == NULL )
		{
			if ( TFGameRules()->HasMultipleTrains() )
			{
				// find the blue cart
			}
			else
			{
				// only one cart in the map, and we need to push it
				CTeamTrainWatcher *watcher = NULL;
				while( ( watcher = dynamic_cast< CTeamTrainWatcher * >( gEntList.FindEntityByClassname( watcher, "team_train_watcher" ) ) ) != NULL )
				{
					if ( !watcher->IsDisabled() )
					{
						m_bluePayloadToPush = watcher;
						break;
					}
				}
			}
		}

		return m_bluePayloadToPush;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Return the Payload cart the given player needs to block from advancing, or NULL if none currently exists
//-----------------------------------------------------------------------------
CHandle< CTeamTrainWatcher > CTFGameRules::GetPayloadToBlock( int blockingTeam ) const
{
	if ( TFGameRules()->GetGameType() != TF_GAMETYPE_ESCORT )
		return NULL;

	if ( blockingTeam == TF_TEAM_RED )
	{
		if ( m_redPayloadToBlock == NULL )
		{
			// find our cart!
			if ( TFGameRules()->HasMultipleTrains() )
			{
				// find the red cart
			}
			else
			{
				// normal Escort scenario, red always blocks
				CTeamTrainWatcher *watcher = NULL;
				while( ( watcher = dynamic_cast< CTeamTrainWatcher * >( gEntList.FindEntityByClassname( watcher, "team_train_watcher" ) ) ) != NULL )
				{
					if ( !watcher->IsDisabled() )
					{
						m_redPayloadToBlock = watcher;
						break;
					}
				}
			}
		}

		return m_redPayloadToBlock;
	}

	if ( blockingTeam == TF_TEAM_BLUE )
	{
		if ( m_bluePayloadToBlock == NULL )
		{
			if ( TFGameRules()->HasMultipleTrains() )
			{
				// find the blue cart
			}
			else
			{
				// normal Payload, blue never blocks
				return NULL;
			}
		}

		return m_bluePayloadToBlock;
	}

	return NULL;
}


#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BuildBonusPlayerList( void ) 
{ 
	if ( m_hBonusLogic.Get() )
	{
		m_hBonusLogic.Get()->BuildBonusPlayerList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Item testing bot controls
//-----------------------------------------------------------------------------
void CTFGameRules::ItemTesting_SetupFromKV( KeyValues *pKV )
{
	m_iItemTesting_BotAnim = pKV->GetInt( "bot_anim", TI_BOTANIM_IDLE );

	int iAnimSpeed = pKV->GetInt( "bot_animspeed", 100 );
	m_flItemTesting_BotAnimSpeed = ( (float)iAnimSpeed / 100.0f );

	m_bItemTesting_BotForceFire = pKV->GetBool( "bot_force_fire" );
	m_bItemTesting_BotTurntable = pKV->GetBool( "bot_turntable" );
	m_bItemTesting_BotViewScan = pKV->GetBool( "bot_view_scan" );
}

//==================================================================================================================
// BONUS ROUND LOGIC
#ifndef CLIENT_DLL
EXTERN_SEND_TABLE( DT_ScriptCreatedItem );
#else
EXTERN_RECV_TABLE( DT_ScriptCreatedItem );
#endif

BEGIN_NETWORK_TABLE_NOBASE( CBonusRoundLogic, DT_BonusRoundLogic )
#ifdef CLIENT_DLL
	RecvPropUtlVector( RECVINFO_UTLVECTOR( m_aBonusPlayerRoll ), MAX_PLAYERS, RecvPropInt( NULL, 0, 4 ) ),
	RecvPropEHandle( RECVINFO( m_hBonusWinner ) ),
	RecvPropDataTable(RECVINFO_DT(m_Item), 0, &REFERENCE_RECV_TABLE(DT_ScriptCreatedItem)),
#else
	SendPropUtlVector( SENDINFO_UTLVECTOR( m_aBonusPlayerRoll ), MAX_PLAYERS, SendPropInt( NULL, 0, 4, 12, SPROP_UNSIGNED ) ),
	SendPropEHandle( SENDINFO( m_hBonusWinner ) ),
	SendPropDataTable(SENDINFO_DT(m_Item), &REFERENCE_SEND_TABLE(DT_ScriptCreatedItem)),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_logic_bonusround, CBonusRoundLogic );
IMPLEMENT_NETWORKCLASS_ALIASED( BonusRoundLogic, DT_BonusRoundLogic )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusRoundLogic::BuildBonusPlayerList( void )
{
	m_aBonusPlayerList.Purge();

	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( !pPlayer )
			continue;

		m_aBonusPlayerList.InsertNoSort( pPlayer );
	}

	m_aBonusPlayerList.RedoSort( true );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBonusRoundLogic::InitBonusRound( void )
{
	m_bAbortedBonusRound = false;
	SetBonusItem( 0 );
	m_hBonusWinner = NULL;

	BuildBonusPlayerList();

	// We actually precalculate the whole shebang.
	m_aBonusPlayerRoll.SetSize( m_aBonusPlayerList.Count() );
	for ( int i = 0; i < m_aBonusPlayerRoll.Count(); i++ )
	{
		m_aBonusPlayerRoll[i] = RandomInt( PLAYER_ROLL_MIN, PLAYER_ROLL_MAX );
	}

	// Sum up the bonus chances 
	int iTotal = 0;
	CUtlVector<int>	aBonusPlayerTotals;
	aBonusPlayerTotals.SetSize( m_aBonusPlayerList.Count() );
	for ( int i = 0; i < aBonusPlayerTotals.Count(); i++ )
	{
		aBonusPlayerTotals[i] = iTotal + m_aBonusPlayerRoll[i] + m_aBonusPlayerList[i]->m_Shared.GetItemFindBonus();
		iTotal = aBonusPlayerTotals[i];
	}

	// Roll for who gets the item.
	int iRoll = RandomInt( 0, iTotal );
	for ( int i = 0; i < aBonusPlayerTotals.Count(); i++ )
	{
		if ( iRoll < aBonusPlayerTotals[i] )
		{
			m_hBonusWinner = m_aBonusPlayerList[i];
			break;
		}
	}

	if ( !m_hBonusWinner.Get() )
		return false;

	// Generate the item on the server for now
	//CSteamID steamID;
	//if ( !m_hBonusWinner.Get()->GetSteamID( &steamID ) )
	const CSteamID *steamID = engine->GetGameServerSteamID();
	if ( !steamID || !steamID->IsValid() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusRoundLogic::SetBonusItem( itemid_t iItemID ) 
{
	m_iBonusItemID = iItemID;
	if ( !m_iBonusItemID )
	{
		m_Item.Invalidate();
		return;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetBonusItem( itemid_t iItemID ) 
{ 
	if ( m_hBonusLogic.Get() )
	{
		m_hBonusLogic.Get()->SetBonusItem( iItemID );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ProcessVerboseLogOutput( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && ( pPlayer->GetTeamNumber() > TEAM_UNASSIGNED ) )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" position_report (position \"%d %d %d\")\n",   
				pPlayer->GetPlayerName(),
				pPlayer->GetUserID(),
				pPlayer->GetNetworkIDString(),
				pPlayer->GetTeam()->GetName(),
				(int)pPlayer->GetAbsOrigin().x, 
				(int)pPlayer->GetAbsOrigin().y,
				(int)pPlayer->GetAbsOrigin().z );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::MatchSummaryTeleport()
{
	bool bUseMatchSummaryStage = false;

	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( GetCurrentMatchGroup() );
	if ( pMatchDesc && pMatchDesc->BUseMatchSummaryStage() )
	{
		bUseMatchSummaryStage = true;
	}

	if ( bUseMatchSummaryStage && m_bMapHasMatchSummaryStage )
	{
		RespawnPlayers( true );

		// find the observer target for the stage
		CObserverPoint *pObserverPoint = dynamic_cast<CObserverPoint*>( gEntList.FindEntityByClassname( NULL, "info_observer_point" ) );
		while( pObserverPoint )
		{
			if ( pObserverPoint->IsMatchSummary() )
			{
				pObserverPoint->SetDisabled( false );
				SetRequiredObserverTarget( pObserverPoint );
				break;
			}

			pObserverPoint = dynamic_cast<CObserverPoint*>( gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" ) );
		}

		// need to do this AFTER we respawn the players above or the conditions will be cleared
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			pPlayer->AddFlag( FL_FROZEN );

			if ( pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )  // spectators automatically get the RequiredObserverTarget that was set above
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
				if ( pTFPlayer )
				{
					pTFPlayer->m_Shared.AddCond( ( pTFPlayer->GetTeamNumber() == GetWinningTeam() ) ? TF_COND_COMPETITIVE_WINNER : TF_COND_COMPETITIVE_LOSER );

					if ( pObserverPoint )
					{
						pTFPlayer->SetViewEntity( pObserverPoint );
						pTFPlayer->SetViewOffset( vec3_origin );
						pTFPlayer->SetFOV( pObserverPoint, pObserverPoint->m_flFOV );
					}

					// use this to force the client player anim to face the right direction
					pTFPlayer->SetTauntYaw( pTFPlayer->GetAbsAngles()[YAW] );
				}
			}
		}

		m_bPlayersAreOnMatchSummaryStage.Set( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::MatchSummaryStart( void )
{
	if ( BAttemptMapVoteRollingMatch() )
	{
		// Grab the final list of maps for users to vote on
		ChooseNextMapVoteOptions();
		m_eRematchState = NEXT_MAP_VOTE_STATE_WAITING_FOR_USERS_TO_VOTE;
	}

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			pPlayer->AddFlag( FL_FROZEN );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "show_match_summary" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	m_bShowMatchSummary.Set( true );
	m_flMatchSummaryTeleportTime = gpGlobals->curtime + 2.f;

	if ( m_hGamerulesProxy )
	{
		m_hGamerulesProxy->MatchSummaryStart();
	}

	CBaseEntity *pLogicCase = NULL;
	while ( ( pLogicCase = gEntList.FindEntityByName( pLogicCase, "competitive_stage_logic_case" ) ) != NULL )
	{
		if ( pLogicCase )
		{
			variant_t sVariant;
			sVariant.SetInt( GetWinningTeam() );
			pLogicCase->AcceptInput( "InValue", NULL, NULL, sVariant, 0 );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::MatchSummaryEnd( void )
{
	m_bShowMatchSummary.Set( false );
	m_bPlayersAreOnMatchSummaryStage.Set( false );

	SetRequiredObserverTarget( NULL );

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		pPlayer->RemoveFlag( FL_FROZEN );
		pPlayer->SetViewEntity( NULL );
		pPlayer->SetFOV( pPlayer, 0 );
	}

	// reset bot convars here
	static ConVarRef tf_bot_quota( "tf_bot_quota" );
	tf_bot_quota.SetValue( tf_bot_quota.GetDefault() );
	static ConVarRef tf_bot_quota_mode( "tf_bot_quota_mode" );
	tf_bot_quota_mode.SetValue( tf_bot_quota_mode.GetDefault() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetTeamAssignmentOverride( CTFPlayer *pTFPlayer, int iDesiredTeam, bool bAutoBalance /*= false*/ )
{
	int iTeam = iDesiredTeam;

	// Look up GC managed match info
	CSteamID steamID;
	pTFPlayer->GetSteamID( &steamID );
	CMatchInfo *pMatch = GTFGCClientSystem()->GetLiveMatch();
	int nMatchPlayers = pMatch ? pMatch->GetNumActiveMatchPlayers() : 0;
	CMatchInfo::PlayerMatchData_t *pMatchPlayer = ( pMatch && steamID.IsValid() ) ? pMatch->GetMatchDataForPlayer( steamID ) : NULL;

	if ( IsMannVsMachineMode() )
	{
		if ( !pTFPlayer->IsBot() && iTeam != TEAM_SPECTATOR )
		{
			if ( pMatchPlayer && !pMatchPlayer->bDropped )
			{
				// Part of the lobby match
				Log( "MVM assigned %s to defending team (player is in lobby)\n", pTFPlayer->GetPlayerName() );
				return TF_TEAM_PVE_DEFENDERS;
			}

			// Count ad-hoc players on defenders team
			int nAdHocDefenders = 0;
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( !pPlayer || ( pPlayer->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS ) )
					{ continue; }

				CSteamID steamID;
				if ( pPlayer->GetSteamID( &steamID ) && GTFGCClientSystem()->GetLiveMatchPlayer( steamID ) )
					{ continue; }

				// Player on defenders that doesn't have a live match entry
				nAdHocDefenders++;
			}

			// Bootcamp mode can mix a lobby with ad-hoc joins
			int nSlotsLeft = tf_mvm_defenders_team_size.GetInt() - nMatchPlayers - nAdHocDefenders;
			if ( nSlotsLeft >= 1 )
			{
				Log( "MVM assigned %s to defending team (%d more slots remaining after us)\n", pTFPlayer->GetPlayerName(), nSlotsLeft-1 );

				// Set Their Currency
				int nRoundCurrency = MannVsMachineStats_GetAcquiredCredits();
				nRoundCurrency += g_pPopulationManager->GetStartingCurrency();

				// deduct any cash that has already been spent
				int spentCurrency = g_pPopulationManager->GetPlayerCurrencySpent( pTFPlayer );
				pTFPlayer->SetCurrency( nRoundCurrency - spentCurrency );

				iTeam = TF_TEAM_PVE_DEFENDERS;
			}
			else
			{
				// no room
				Log( "MVM assigned %s to spectator, all slots for defending team are in use, or reserved for lobby members\n",
				     pTFPlayer->GetPlayerName() );
				iTeam = TEAM_SPECTATOR;
			}
		}
	}
	else if ( pMatch )
	{
		if ( !bAutoBalance )
		{
			CSteamID steamID;
			if ( pMatchPlayer )
			{
				iTeam = GetGameTeamForGCTeam( pMatchPlayer->eGCTeam );
				if ( iTeam < FIRST_GAME_TEAM )
				{
					// We should always have a team assigned by the GC
					Warning( "Competitive mode: Lobby player with invalid GC team %i in MatchGroup %i\n", iTeam, (int)pMatch->m_eMatchGroup );
				}
				CheckAndSetPartyLeader( pTFPlayer, iTeam );
			}
		}
	}

	return iTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static CPhysicsProp *CreatePhysicsToy( const char* pszModelName, const Vector &vSpawnPos, const QAngle &qSpawnAngles )
{
	if ( pszModelName == NULL )
		return NULL;

	CPhysicsProp *pProp = NULL;

	MDLCACHE_CRITICAL_SECTION();

	MDLHandle_t h = mdlcache->FindMDL( pszModelName );
	if ( h != MDLHANDLE_INVALID )
	{
		// Must have vphysics to place as a physics prop
		studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( h );
		if ( pStudioHdr && mdlcache->GetVCollide( h ) )
		{
			bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
			CBaseEntity::SetAllowPrecache( true );
			
			// Try to create entity
			pProp = dynamic_cast< CPhysicsProp * >( CreateEntityByName( "prop_physics_override" ) );
			if ( pProp )
			{
				pProp->SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );
				// so it can be pushed by airblast
				pProp->AddFlag( FL_GRENADE );
				// so that it will always be interactable with the player
				//pProp->SetPhysicsMode( PHYSICS_MULTIPLAYER_SOLID );//PHYSICS_MULTIPLAYER_NON_SOLID );
				char buf[512];
				// Pass in standard key values
				Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", vSpawnPos.x, vSpawnPos.y, vSpawnPos.z );
				pProp->KeyValue( "origin", buf );
				Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", qSpawnAngles.x, qSpawnAngles.y, qSpawnAngles.z );
				pProp->KeyValue( "angles", buf );
				pProp->KeyValue( "model", pszModelName );
				pProp->KeyValue( "fademindist", "-1" );
				pProp->KeyValue( "fademaxdist", "0" );
				pProp->KeyValue( "fadescale", "1" );
				pProp->KeyValue( "inertiaScale", "1.0" );
				pProp->KeyValue( "physdamagescale", "0.1" );
				pProp->Precache();
				DispatchSpawn( pProp );
				pProp->m_takedamage = DAMAGE_YES;	// Take damage, otherwise this can block trains
				pProp->SetHealth( 5000 );
				pProp->Activate();
			}

			CBaseEntity::SetAllowPrecache( bAllowPrecache );
		}

		mdlcache->Release( h ); // counterbalance addref from within FindMDL
	}
	return pProp;
}

CPhysicsProp *CreateBeachBall( const Vector &vSpawnPos, const QAngle &qSpawnAngles )
{
	return CreatePhysicsToy( "models/props_gameplay/ball001.mdl", vSpawnPos, qSpawnAngles );
}

CPhysicsProp *CreateSoccerBall( const Vector &vSpawnPos, const QAngle &qSpawnAngles )
{
	return CreatePhysicsToy( "models/player/items/scout/soccer_ball.mdl", vSpawnPos, qSpawnAngles );
}


void CTFGameRules::PushAllPlayersAway( const Vector& vFromThisPoint, float flRange, float flForce, int nTeam, CUtlVector< CTFPlayer* > *pPushedPlayers /*= NULL*/ )
{
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, nTeam, COLLECT_ONLY_LIVING_PLAYERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *pPlayer = playerVector[i];

		Vector toPlayer = pPlayer->EyePosition() - vFromThisPoint;

		if ( toPlayer.LengthSqr() < flRange * flRange )
		{
			// send the player flying
			// make sure we push players up and away
			toPlayer.z = 0.0f;
			toPlayer.NormalizeInPlace();
			toPlayer.z = 1.0f;

			Vector vPush = flForce * toPlayer;

			pPlayer->ApplyAbsVelocityImpulse( vPush );

			if ( pPushedPlayers )
			{
				pPushedPlayers->AddToTail( pPlayer );
			}
		}
	}
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanUpgradeWithAttrib( CTFPlayer *pPlayer, int iWeaponSlot, attrib_definition_index_t iAttribIndex, CMannVsMachineUpgrades *pUpgrade )
{
	if ( !pPlayer )
		return false;

	Assert ( pUpgrade );

	// Upgrades on players are considered active at all times
	if ( pUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER )
	{
		switch ( iAttribIndex )
		{
		case 113:	// "metal regen"
			{
				return ( pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) );
			}
			break;
		}

		return true;
	}

	// Get the item entity. We use the entity, not the item in the loadout, because we want
	// the dynamic attributes that have already been purchases and attached.
	CEconEntity *pEntity;
	CEconItemView *pCurItemData = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( pPlayer, iWeaponSlot, &pEntity );
	if ( !pCurItemData || !pEntity )
		return false;

	// bottles can only hold things in the appropriate ui group
	if ( dynamic_cast< CTFPowerupBottle *>( pEntity ) )
	{
		if ( pUpgrade->nUIGroup == UIGROUP_POWERUPBOTTLE )
		{
			switch ( iAttribIndex )
			{
			case 327:	// "building instant upgrade"
				{
					return ( pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) );
				}
#ifndef _DEBUG
			case 480:	// "radius stealth"
				{
					return ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) );
				}
#endif // !_DEBUG
		}
			return true;
		}

		return false;
	}
	else if ( pUpgrade->nUIGroup == UIGROUP_POWERUPBOTTLE )
	{
		return false;
	}

	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* > ( pEntity );
	CTFWeaponBaseGun *pWeaponGun = dynamic_cast< CTFWeaponBaseGun* > ( pEntity );
	int iWeaponID = ( pWeapon ) ? pWeapon->GetWeaponID() : TF_WEAPON_NONE;
	CTFWearableDemoShield *pShield = ( pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) ) ? dynamic_cast< CTFWearableDemoShield* >( pEntity ) : NULL;
	bool bShield = ( pShield ) ? true : false;
	bool bRocketPack = ( iWeaponID == TF_WEAPON_ROCKETPACK );

	if ( iWeaponID == TF_WEAPON_PARACHUTE )
		return false;

	// Hack to simplify excluding non-weapons from damage upgrades
	bool bHideDmgUpgrades = iWeaponID == TF_WEAPON_NONE || 
							iWeaponID == TF_WEAPON_LASER_POINTER || 
							iWeaponID == TF_WEAPON_MEDIGUN || 
							iWeaponID == TF_WEAPON_BUFF_ITEM ||
							iWeaponID == TF_WEAPON_BUILDER ||
							iWeaponID == TF_WEAPON_PDA_ENGINEER_BUILD ||
							iWeaponID == TF_WEAPON_INVIS ||
							iWeaponID == TF_WEAPON_SPELLBOOK ||
							iWeaponID == TF_WEAPON_JAR_GAS ||
							iWeaponID == TF_WEAPON_LUNCHBOX ||
							bRocketPack;

	// What tier upgrade is it?
	int nQuality = pUpgrade->nQuality;

	// This is bad, but it's hopefully more maintainable than the allowed attributes block for all current & future items
	switch ( iAttribIndex )
	{
	case 2:		// "damage bonus"
		{
			if ( bHideDmgUpgrades )
				return false;

			// Some classes get a weaker dmg upgrade
			if ( nQuality == MVM_UPGRADE_QUALITY_LOW )
			{
				if ( pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) && !bShield )
				{
					return ( iWeaponSlot == TF_WPN_TYPE_PRIMARY || iWeaponSlot == TF_WPN_TYPE_SECONDARY );
				}
				else
				{
					return false;
				}
			}

			bool bShieldEquipped = false;
			if ( pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) )
			{
				for ( int i = 0; i < pPlayer->GetNumWearables(); ++i )
				{
					CTFWearableDemoShield *pWearableShield = dynamic_cast< CTFWearableDemoShield* >( pPlayer->GetWearable( i ) );
					if ( pWearableShield )
					{
						bShieldEquipped = true;
					}
				}
			}

			return ( ( iWeaponSlot == TF_WPN_TYPE_PRIMARY && 
					( pPlayer->IsPlayerClass( TF_CLASS_SCOUT ) || 
					pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) || 
					pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) || 
					pPlayer->IsPlayerClass( TF_CLASS_PYRO ) ) ) || 
					( iWeaponID == TF_WEAPON_SWORD && bShieldEquipped ) );
		}
		break;
	case 6:		// "fire rate bonus"
		{
			// Heavy's version of firing speed costs more
			bool bMinigun = iWeaponID == TF_WEAPON_MINIGUN;
			if ( nQuality == MVM_UPGRADE_QUALITY_LOW )
			{
				return bMinigun;
			}
			else if ( iWeaponID == TF_WEAPON_GRENADELAUNCHER && pWeapon && pWeapon->AutoFiresFullClipAllAtOnce() )
			{
				return false;
			}

			// Non-melee version
			return ( dynamic_cast< CTFWeaponBaseMelee* >( pEntity ) == NULL && 
				iWeaponID != TF_WEAPON_NONE && !bHideDmgUpgrades && 
				iWeaponID != TF_WEAPON_FLAMETHROWER && 
				iWeaponID != TF_WEAPON_FLAME_BALL &&
				!WeaponID_IsSniperRifleOrBow( iWeaponID ) && 
				!( pWeapon && pWeapon->HasEffectBarRegeneration() ) &&
				!bMinigun );
		}
		break;
	// case 8:		// "heal rate bonus"
	case 10:	// "ubercharge rate bonus"
	case 314:	// "uber duration bonus"
	case 481:	// "canteen specialist"
	case 482:	// "overheal expert"
	case 483:	// "medic machinery beam"
	case 493:	// "healing mastery"
		{
			return ( iWeaponID == TF_WEAPON_MEDIGUN );
		}
		break;
	case 31:	// "critboost on kill"
		{
			CTFWeaponBaseMelee *pMelee = dynamic_cast<CTFWeaponBaseMelee *> ( pEntity );
			return ( pMelee && (
				pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) || 
				pPlayer->IsPlayerClass( TF_CLASS_SPY ) ) );
		}
	case 71:	// weapon burn dmg increased
	case 73:	// weapon burn time increased
		{
			return ( iWeaponID == TF_WEAPON_FLAMETHROWER || iWeaponID == TF_WEAPON_FLAREGUN );
		}
		break;
	case 76:	// "maxammo primary increased"
		{
			return ( pWeapon && pWeapon->GetPrimaryAmmoType() == TF_AMMO_PRIMARY && !pWeapon->IsEnergyWeapon() );
		}
		break;
	case 78:	// "maxammo secondary increased"
		{
			return ( pWeapon && pWeapon->GetPrimaryAmmoType() == TF_AMMO_SECONDARY && !pWeapon->IsEnergyWeapon() );
		}
		break;
	case 90:	// "SRifle Charge rate increased"
		{
			return WeaponID_IsSniperRifle( iWeaponID );
		}
		break;
	case 103:	// "Projectile speed increased"
		{
			if ( pWeaponGun )
			{
				return ( pWeaponGun->GetWeaponProjectileType() == TF_PROJECTILE_PIPEBOMB );
			}

			return false;
		}
		break;
	case 149:	// "bleed duration"
	case 523:	// "arrow mastery"
		{
			return ( iWeaponID == TF_WEAPON_COMPOUND_BOW );
		}
		break;
	case 218:	// "mark for death"
		{
			return ( iWeaponID == TF_WEAPON_BAT_WOOD );
		}
		break;
	case 249:	// "charge recharge rate increased"
	case 252:   // "damage force reduction"
		{
			return bShield;
		}
		break;
	case 255:	// "airblast pushback scale"
		{
			return ( iWeaponID == TF_WEAPON_FLAME_BALL || 
					 ( iWeaponID == TF_WEAPON_FLAMETHROWER && pWeaponGun && assert_cast< CTFFlameThrower* >( pWeaponGun )->CanAirBlastPushPlayer() ) );
		}
		break;
	case 266:	// "projectile penetration"
		{
			if ( pWeaponGun && !bHideDmgUpgrades )
			{
				if ( !( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && iWeaponSlot == TF_WPN_TYPE_PRIMARY ) )
				{
					int iProjectile = pWeaponGun->GetWeaponProjectileType();
					return ( iProjectile == TF_PROJECTILE_ARROW || iProjectile == TF_PROJECTILE_BULLET || iProjectile == TF_PROJECTILE_HEALING_BOLT || iProjectile == TF_PROJECTILE_FESTIVE_ARROW || iProjectile == TF_PROJECTILE_FESTIVE_HEALING_BOLT );
				}
			}

			return false;
		}
		break;
	case 80:	// "maxammo metal increased"
	case 276:	// "bidirectional teleport"
	case 286:	// "engy building health bonus"
	case 343:	// "engy sentry fire rate increased"
	case 345:	// "engy dispenser radius increased"
	case 351:	// "engy disposable sentries"
		{
			return ( pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) && iWeaponID == TF_WEAPON_PDA_ENGINEER_BUILD );
		}
		break;
	case 278:	// "effect bar recharge rate increased"
		{
			return ( pWeapon && pWeapon->HasEffectBarRegeneration() && iWeaponID != TF_WEAPON_BUILDER && iWeaponID != TF_WEAPON_SPELLBOOK );
		}
		break;
	case 279:	// "maxammo grenades1 increased"
		{
			return ( iWeaponID == TF_WEAPON_BAT_WOOD || iWeaponID == TF_WEAPON_BAT_GIFTWRAP );
		}
		break;
	case 313:	// "applies snare effect"
		{
// 			if ( nQuality == MVM_UPGRADE_QUALITY_LOW )
// 			{
// 				if ( iWeaponID == TF_WEAPON_SNIPERRIFLE )
// 				{
// 					CTFSniperRifle *pRifle = static_cast< CTFSniperRifle* >( pEntity );
// 					return ( pRifle->GetRifleType() == RIFLE_JARATE );
// 				}
// 				return false;
// 			}

			return ( iWeaponID == TF_WEAPON_JAR || iWeaponID == TF_WEAPON_JAR_MILK );
		}
		break;
	case 318:	// "faster reload rate"
		{
			return ( ( pWeapon && pWeapon->ReloadsSingly() ) || 
				WeaponID_IsSniperRifleOrBow( iWeaponID ) || 
				iWeaponID == TF_WEAPON_FLAREGUN );
		}
		break;
	case 319:	// "increase buff duration"
		{
			return ( iWeaponID == TF_WEAPON_BUFF_ITEM );
		}
		break;
	case 320:	// "robo sapper"
		{
			return ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && iWeaponID == TF_WEAPON_BUILDER );
		}
		break;
	case 323:	// "attack projectiles"
		{
			return ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && iWeaponSlot == TF_WPN_TYPE_PRIMARY );
		}
		break;
	case 335:		// "clip size bonus upgrade"
		{
			return ( pWeapon && !pWeapon->IsBlastImpactWeapon() && pWeapon->UsesClipsForAmmo1() && pWeapon->GetMaxClip1() > 1 
					 && iWeaponID != TF_WEAPON_FLAREGUN_REVENGE && iWeaponID != TF_WEAPON_SPELLBOOK );
		}
		break;
	case 375:		// "generate rage on damage"
		{
			return ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && iWeaponSlot == TF_WPN_TYPE_PRIMARY );
		}
		break;
	case 395:		// "explosive sniper shot"
		{
			return ( pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) && iWeaponSlot == TF_WPN_TYPE_PRIMARY && 
					 pWeaponGun && pWeaponGun->GetWeaponProjectileType() == TF_PROJECTILE_BULLET );
		}
		break;
	case 396:		// "melee attack rate bonus"
		{
			bool bAllowed = ( !bRocketPack && 
							iWeaponID != TF_WEAPON_BAT_WOOD &&
							iWeaponID != TF_WEAPON_BUFF_ITEM && 
							!( pWeapon && pWeapon->HasEffectBarRegeneration() ) &&
							dynamic_cast< CTFWeaponBaseMelee* >( pEntity ) );
			return bAllowed;
		}
		break;
	case 397:	// "projectile penetration heavy"
		{
			if ( !bHideDmgUpgrades )
			{
				return ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && iWeaponSlot == TF_WPN_TYPE_PRIMARY );
			}
		}
		break;
	case 399:	// "armor piercing"
		{
			return ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && iWeaponID == TF_WEAPON_KNIFE );
		}
		break;
	case 440:	// "clip size upgrade atomic"
		{
			return pWeapon && pWeapon->IsBlastImpactWeapon();
		}
		break;
	case 484:	// "mad milk syringes"
		{
			return ( iWeaponID == TF_WEAPON_SYRINGEGUN_MEDIC );
		}
	case 488:	// "rocket specialist"
		if ( pWeaponGun )
		{
			return ( pWeaponGun->GetWeaponProjectileType() == TF_PROJECTILE_ROCKET || pWeaponGun->GetWeaponProjectileType() == TF_PROJECTILE_ENERGY_BALL );
		}
	case 499:	// generate rage on heal (shield)
	case 554:	// revive
	case 555:	// medigun specialist
		{
			return ( iWeaponID == TF_WEAPON_MEDIGUN );
		}
	case 871:	// falling_impact_radius_stun
	case 872:	// thermal_thruster_air_launch
		{
			return bRocketPack;
		}
	case 874:	// mult_item_meter_charge_rate
		{
			attrib_value_t eChargeType = ATTRIBUTE_METER_TYPE_NONE;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pEntity, eChargeType, item_meter_charge_type );
			if ( eChargeType != ATTRIBUTE_METER_TYPE_NONE )
			{
				return ( iWeaponID != TF_WEAPON_FLAME_BALL );
			}
		}
	case 875:	// explode_on_ignite
		{
			return ( iWeaponID == TF_WEAPON_JAR_GAS );
		}
	}

	// All weapon related attributes require an item that does damage
	if ( pUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM )
	{
		// All guns
		if ( pWeaponGun )
			return ( iWeaponID != TF_WEAPON_NONE && !bHideDmgUpgrades && 
			!( pWeapon && pWeapon->HasEffectBarRegeneration() ) );

		CTFWeaponBaseMelee *pMelee = dynamic_cast< CTFWeaponBaseMelee* >( pEntity );
		if ( pMelee )
		{
			// All melee weapons except buff banners
			return ( iWeaponID != TF_WEAPON_BUFF_ITEM && !bHideDmgUpgrades && !bRocketPack );
		}

		return false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CTFGameRules::GetUpgradeTier( int iUpgrade )
{
	if ( !GameModeUsesUpgrades() || iUpgrade < 0 || iUpgrade >= g_MannVsMachineUpgrades.m_Upgrades.Count() )
		return 0;
	
	return g_MannVsMachineUpgrades.m_Upgrades[iUpgrade].nTier;
}

//-----------------------------------------------------------------------------
//  Given an upgrade and slot, see if its' tier is enabled/available
//-----------------------------------------------------------------------------
bool CTFGameRules::IsUpgradeTierEnabled( CTFPlayer *pTFPlayer, int iItemSlot, int iUpgrade )
{
	if ( !pTFPlayer )
		return false;

	// If the upgrade has a tier, it's mutually exclusive with upgrades of the same tier for the same slot
	int nTier = GetUpgradeTier( iUpgrade );
	if ( !nTier )
		return false;

	bool bIsAvailable = true;
	CEconItemView *pItem = NULL;
	float flValue = 0.f;

	// Go through the upgrades and see if it could apply, and if we already have it
	for ( int i = 0; i < g_MannVsMachineUpgrades.m_Upgrades.Count(); i++ )
	{
		CMannVsMachineUpgrades upgrade = g_MannVsMachineUpgrades.m_Upgrades[i];

		// Same upgrade
// 		if ( !V_strcmp( upgrade.szAttrib, g_MannVsMachineUpgrades.m_Upgrades[iUpgrade].szAttrib ) )
// 			continue;

		// Different tier
		if ( upgrade.nTier != nTier )
			continue;

		// Wrong type
		if ( upgrade.nUIGroup != g_MannVsMachineUpgrades.m_Upgrades[iUpgrade].nUIGroup )
			continue;

		CEconItemAttributeDefinition *pAttribDef = ItemSystem()->GetStaticDataForAttributeByName( upgrade.szAttrib );
		if ( !pAttribDef )
			continue;
			
		// Can't use
		if ( !CanUpgradeWithAttrib( pTFPlayer, iItemSlot, pAttribDef->GetDefinitionIndex(), &upgrade ) )
			continue;

		if ( upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM )
		{
			pItem = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( pTFPlayer, iItemSlot );
			if ( pItem )
			{
				::FindAttribute_UnsafeBitwiseCast< attrib_value_t >( pItem->GetAttributeList(), pAttribDef, &flValue );
			}
		}
		else if ( upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER )
		{
			::FindAttribute_UnsafeBitwiseCast< attrib_value_t >( pTFPlayer->GetAttributeList(), pAttribDef, &flValue );
		}

		if ( flValue > 0.f )
		{
			bIsAvailable = false;
			break;
		}
	}
	
	return bIsAvailable;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------
void CTFGameRules::SetNextMvMPopfile ( const char * next )
{
	s_strNextMvMPopFile = next;
}

//-----------------------------------------------------------------------------
const char * CTFGameRules::GetNextMvMPopfile ( )
{
	return s_strNextMvMPopFile.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BalanceTeams( bool bRequireSwitcheesToBeDead )
{
	// are we playing a managed match via matchmaking?
	if ( GetMatchGroupDescription( GetCurrentMatchGroup() ) )
		return;
		
	if ( mp_autoteambalance.GetInt() == 2 )
		return;

	BaseClass::BalanceTeams( bRequireSwitcheesToBeDead );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PointsMayBeCaptured( void )
{
#ifdef GAME_DLL
	if ( IsHolidayActive( kHoliday_Halloween ) && GetActiveBoss() )
	{
		switch ( GetHalloweenScenario() )
		{
		case HALLOWEEN_SCENARIO_VIADUCT:
		{
			// the eyeball prevents point capturing while he's in play
			if ( assert_cast< CEyeballBoss * >( GetActiveBoss() ) )
			{
				return false;
			}
		}
		break;
		case HALLOWEEN_SCENARIO_LAKESIDE:
		{
			// merasmus prevents point capturing while he's in play
			if ( assert_cast< CMerasmus * >( GetActiveBoss() ) )
			{
				return false;
			}
		}
		break;
		}
	}
#endif // GAME_DLL

	if ( IsMannVsMachineMode() )
		return true;

	if ( GetActiveRoundTimer() && InSetup() )
		return false;

	return BaseClass::PointsMayBeCaptured();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CreateSoldierStatue()
{
	if ( m_hSoldierStatue )
		return;

	if ( !IsHolidayActive( kHoliday_Soldier ) )
		return;

	if ( IsMatchTypeCompetitive() )
		return;

	char szCurrentMap[MAX_MAP_NAME];
	Q_strncpy( szCurrentMap, STRING( gpGlobals->mapname ), sizeof( szCurrentMap ) );

	for ( int iIndex = 0; iIndex < ARRAYSIZE( s_StatueMaps ); ++iIndex )
	{
		if ( !Q_stricmp( s_StatueMaps[iIndex].pDiskName, szCurrentMap ) )
		{
			m_hSoldierStatue = dynamic_cast< CEntitySoldierStatue * >( CreateEntityByName( "entity_soldier_statue" ) );
			if ( m_hSoldierStatue )
			{
				m_hSoldierStatue->SetAbsOrigin( s_StatueMaps[iIndex].vec_origin );
				m_hSoldierStatue->SetAbsAngles( s_StatueMaps[iIndex].vec_angle );
				DispatchSpawn( m_hSoldierStatue );
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initiate timer and reset player data (since we only care about data from the following interval)
//-----------------------------------------------------------------------------
void CTFGameRules::PowerupModeInitKillCountTimer()
{
	for ( int i = 0; i < MAX_PLAYERS; ++i )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer || !pTFPlayer->IsConnected() )
			continue;

		pTFPlayer->m_nMannpowerKills = 0;
		pTFPlayer->m_nMannpowerDeaths = 0;
		pTFPlayer->m_bMannpowerHereForFullInterval = true;
	}

	m_flNextPowerupModeKillCountTimer = gpGlobals->curtime + tf_powerup_mode_killcount_timer_length.GetFloat();

	// clean up our vector of dominant players that might have quit
	FOR_EACH_VEC_BACK( m_PowerupModeDominantDisconnect, nIndex )
	{
		if ( m_PowerupModeDominantDisconnect[nIndex].m_flRemoveDominantConditionTime <= gpGlobals->curtime )
		{
			m_PowerupModeDominantDisconnect.Remove( nIndex );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGameRules::CheckPowerupModeDominantDisconnect( CSteamID steamID )
{
	float flRemoveDominantConditionTime = -1.f;

	FOR_EACH_VEC( m_PowerupModeDominantDisconnect, nIndex )
	{
		if ( m_PowerupModeDominantDisconnect[nIndex].m_steamID == steamID )
		{
			if ( m_PowerupModeDominantDisconnect[nIndex].m_flRemoveDominantConditionTime > gpGlobals->curtime )
			{
				flRemoveDominantConditionTime = m_PowerupModeDominantDisconnect[nIndex].m_flRemoveDominantConditionTime;
			}
			m_PowerupModeDominantDisconnect.Remove( nIndex );
			break;
		}
	}

	return flRemoveDominantConditionTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PowerupModeDominantDisconnect( CSteamID steamID, float flRemoveDominantConditionTime )
{
	int nIndex = -1;

	// make sure they're not already in the list. they shouldn't be but let's be sure.
	FOR_EACH_VEC( m_PowerupModeDominantDisconnect, i )
	{
		if ( m_PowerupModeDominantDisconnect[i].m_steamID == steamID )
		{
			nIndex = i;
			break;
		}
	}

	if ( nIndex == -1 )
	{
		nIndex = m_PowerupModeDominantDisconnect.AddToTail();
	}

	m_PowerupModeDominantDisconnect[nIndex].m_steamID = steamID;
	m_PowerupModeDominantDisconnect[nIndex].m_flRemoveDominantConditionTime = flRemoveDominantConditionTime;
}

//-----------------------------------------------------------------------------
// Purpose: analyze player kill count data to determine if any players are dominant
//-----------------------------------------------------------------------------
void CTFGameRules::PowerupModeKillCountCompare()
{
	int nTotalParticipants = 0; //we want a minimum number of participants to have confidence in the median number
	int nNumberOfDominantPlayers = 0; //used for stat gathering
	CUtlVector<int> vecKillCounts;

	for ( int i = 0; i < MAX_PLAYERS; ++i )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer || !pTFPlayer->IsConnected() ||
			!pTFPlayer->m_bMannpowerHereForFullInterval ||
			( pTFPlayer->m_nMannpowerKills <= 2 && pTFPlayer->m_nMannpowerDeaths <= 2 ) ) // player is in the game but not meaningfully participating. Kills and deaths are too few
			continue;
		
		int nKills = pTFPlayer->m_nMannpowerKills;
		if ( nKills < 1 ) //we don't want to store zeros in the array since we calculate the median later and don't want a chance for it to be zero since we use it to multiply off of. 1 is close enough to zero for this purpose
		{
			nKills = 1;
		}
		vecKillCounts.AddToTail( nKills );
		nTotalParticipants++;
	}

	// we want a minimum number of participants to have confidence in the median
 	if ( nTotalParticipants >= 6 )
	{
		int nMinimumKillsPerTimerInterval = 14; 
		int nMedianKillCount = 1;
		bool bDominantPlayerOnRedTeam = false;
		bool bDominantPlayerOnBlueTeam = false;
		int nImbalanceVictimTeam = TEAM_UNASSIGNED; // this team has had an imbalance event with no team swap, so might still be struggling
		
		// An imbalance powerup recently triggered for this team so we don't test players on that team for dominance
		if ( m_nPowerUpImbalanceVictimTeam && ( gpGlobals->curtime - m_flPowerUpImbalanceVictimTeamTime < 600 ) )
		{
			nImbalanceVictimTeam = m_nPowerUpImbalanceVictimTeam;
		}

		//Calculate Median of vecKillCounts	
		vecKillCounts.Sort();
		if ( nTotalParticipants % 2 != 0 ) // we have an odd number of participants, select the middle value
		{
			nMedianKillCount = vecKillCounts[( nTotalParticipants - 1 ) / 2];
		}
		else //we have an even number of participants, so find the average of the two middle values
		{
			nMedianKillCount = ( vecKillCounts[( nTotalParticipants / 2 ) - 1] + vecKillCounts[nTotalParticipants / 2] ) / 2;
		}

		//test each player for dominance. 
		for ( int i = 0; i < MAX_PLAYERS; ++i )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( !pTFPlayer || 
				!pTFPlayer->IsConnected() || 
				pTFPlayer->GetTeamNumber() == nImbalanceVictimTeam ||
				!pTFPlayer->m_bMannpowerHereForFullInterval || 
				pTFPlayer->m_nMannpowerKills < nMinimumKillsPerTimerInterval )
				continue;
			
			//player's kill count exceeds the median by the required amount
			else if ( pTFPlayer->m_nMannpowerKills >= nMedianKillCount * tf_powerup_mode_dominant_multiplier.GetFloat() )
			{
//				float flMedianMultiple = 0.f;
				pTFPlayer->EmitSound( "Mannpower.PlayerIsDominant" );
				pTFPlayer->StartPowerupModeDominant( pTFPlayer->m_bIsInMannpowerDominantCondition );
				nNumberOfDominantPlayers++;
				switch ( pTFPlayer->GetTeamNumber() )
				{
				case TF_TEAM_BLUE:
					bDominantPlayerOnBlueTeam = true;
					break;
				case TF_TEAM_RED:
					bDominantPlayerOnRedTeam = true;
					break;
				}
//				flMedianMultiple = pTFPlayer->m_nMannpowerKills / (float)nMedianKillCount;

				// Write to DB
// 				CSQLAccess sqlAccessMPDP;
// 				CSteamID steamID;
// 				CSchMannpowerDominantPlayer schMannpowerDominantPlayer;
// 				
// 				sqlAccessMSKC.BBeginTransaction( "YieldingInsertMannpowerDominantPlayer" );
// 
// 				schMannpowerDominantPlayer.m_unClientAccountID = pTFPlayer->GetSteamID( &steamID );
// 				schMannpowerDominantPlayer.m_unServerAccountID = tf_server_identity_account_id.GetInt(); 
// 				schMannpowerDominantPlayer.m_RTime32CurTime = gpGlobals->curtime; 
// 				schMannpowerDominantPlayer.m_flMedianMultiple = flMedianMultiple;
// 
// 				sqlAccessMSKC.BYieldingInsertRecord( &schMannpowerDominantPlayer );
// 
// 				if ( !sqlAccess.BCommitTransaction() )
// 				{
// 					Result.m_fmtError.sprintf( "YieldingInsertMannpowerDominantPlayer: Failed to commit transaction to SQL\n" );
// 				}
			}
			else
				continue;
		}

		// Write to DB 
// 		CSQLAccess sqlAccessMSKC;
// 		CSchMannpowerServerKillCount schMannpowerServerKillCount;
// 
// 		sqlAccessMSKC.BBeginTransaction( "YieldingInsertMannpowerServerKillCount" );
// 
// 		schMannpowerServerKillCount.m_unServerAccountID = tf_server_identity_account_id.GetInt();
// 		schMannpowerServerKillCount.m_RTime32CurTime = gpGlobals->curtime; 
// 		schMannpowerServerKillCount.m_nMedianKillCount = nMedianKillCount;
// 		schMannpowerServerKillCount.m_nTotalParticipants = nTotalParticipants;
// 		schMannpowerServerKillCount.m_nNumberOfDominantPlayers = nNumberOfDominantPlayers;
// 
// 		sqlAccessMSKC.BYieldingInsertRecord( &schMannpowerServerKillCount );
// 
// 		if ( !sqlAccess.BCommitTransaction() )
// 		{
// 			Result.m_fmtError.sprintf( "YieldingInsertMannpowerServerKillCount: Failed to commit transaction to SQL\n" );
// 		}

		if ( bDominantPlayerOnBlueTeam ) //tell the red team an enemy has been identified as dominant
		{
			for ( int i = 0; i < MAX_PLAYERS; ++i )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( pTFPlayer && !pTFPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) && pTFPlayer->GetTeamNumber() == TF_TEAM_RED )
				{
					ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Dominant_Other_Team" );
					ClientPrint( pTFPlayer, HUD_PRINTTALK, "#TF_Powerup_Dominant_Other_Team" );
				}
			}

			BroadcastSound( TF_TEAM_RED, "Mannpower.DominantPlayerOtherTeam" );
		}
		else if ( bDominantPlayerOnRedTeam ) //tell the blue team an enemy has been identified as dominant
		{
			for ( int i = 0; i < MAX_PLAYERS; ++i )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if ( pTFPlayer && !pTFPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) && pTFPlayer->GetTeamNumber() == TF_TEAM_BLUE )
				{
					ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Dominant_Other_Team" );
					ClientPrint( pTFPlayer, HUD_PRINTTALK, "#TF_Powerup_Dominant_Other_Team" );
				}
			}

			BroadcastSound( TF_TEAM_BLUE, "Mannpower.DominantPlayerOtherTeam" );
		}
	}

	PowerupModeInitKillCountTimer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags /* = 0 */, CBasePlayer *pPlayer /* = NULL */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	//send it to everyone
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_broadcast_audio" );
	if ( event )
	{
		event->SetInt( "team", iTeam );
		event->SetString( "sound", sound );
		event->SetInt( "additional_flags", iAdditionalSoundFlags );
		event->SetInt( "player", pTFPlayer ? pTFPlayer->entindex() : -1 );
		gameeventmanager->FireEvent( event );
	}
}

#define TF_GAMERULES_SCRIPT_FUNC( function, desc ) \
	ScriptRegisterFunctionNamed( g_pScriptVM, Script##function, #function, desc )

int		ScriptGetRoundState()										{ return TFGameRules()->GetRoundState(); }
bool	ScriptIsInWaitingForPlayers()								{ return TFGameRules()->IsInWaitingForPlayers(); }
int		ScriptGetWinningTeam()										{ return TFGameRules()->GetWinningTeam(); }
bool	ScriptInOvertime()											{ return TFGameRules()->InOvertime(); }
bool	ScriptIsBirthday()											{ return TFGameRules()->IsBirthday(); }
bool	ScriptIsHolidayActive( int eHoliday )						{ return TFGameRules()->IsHolidayActive( eHoliday ); }
bool	ScriptPointsMayBeCaptured()									{ return TFGameRules()->PointsMayBeCaptured(); }
int		ScriptGetClassLimit( int iClass )							{ return TFGameRules()->GetClassLimit( iClass ); }
bool	ScriptFlagsMayBeCapped()									{ return TFGameRules()->FlagsMayBeCapped(); }
int		ScriptGetStopWatchState()									{ return TFGameRules()->GetStopWatchState(); }
bool	ScriptIsInArenaMode()										{ return TFGameRules()->IsInArenaMode(); }
bool	ScriptIsInKothMode()										{ return TFGameRules()->IsInKothMode(); }
bool	ScriptIsInMedievalMode()									{ return TFGameRules()->IsInMedievalMode(); }
bool	ScriptIsHolidayMap( int nHoliday )							{ return TFGameRules()->IsHolidayMap( nHoliday ); }
bool	ScriptIsMannVsMachineMode()									{ return TFGameRules()->IsMannVsMachineMode(); }
bool	ScriptGetMannVsMachineAlarmStatus()							{ return TFGameRules()->GetMannVsMachineAlarmStatus(); }
void	ScriptSetMannVsMachineAlarmStatus( bool bEnabled )			{ return TFGameRules()->SetMannVsMachineAlarmStatus( bEnabled ); }
bool	ScriptIsQuickBuildTime()									{ return TFGameRules()->IsQuickBuildTime(); }
bool	ScriptGameModeUsesUpgrades()								{ return TFGameRules()->GameModeUsesUpgrades(); }
bool	ScriptGameModeUsesCurrency()								{ return TFGameRules()->GameModeUsesCurrency(); }
bool	ScriptGameModeUsesMiniBosses()								{ return TFGameRules()->GameModeUsesMiniBosses(); }
bool	ScriptIsPasstimeMode()										{ return TFGameRules()->IsPasstimeMode(); }
bool	ScriptIsMannVsMachineRespecEnabled()						{ return TFGameRules()->IsMannVsMachineRespecEnabled(); }
bool	ScriptIsPowerupMode()										{ return TFGameRules()->IsPowerupMode(); }
bool	ScriptIsCompetitiveMode()									{ return TFGameRules()->IsCompetitiveMode(); }
bool	ScriptIsMatchTypeCasual()									{ return TFGameRules()->IsMatchTypeCasual(); }
bool	ScriptIsMatchTypeCompetitive()								{ return TFGameRules()->IsMatchTypeCompetitive(); }
bool	ScriptInMatchStartCountdown()								{ return TFGameRules()->InMatchStartCountdown(); }
bool	ScriptMatchmakingShouldUseStopwatchMode()					{ return TFGameRules()->MatchmakingShouldUseStopwatchMode(); }
bool	ScriptIsAttackDefenseMode()									{ return TFGameRules()->IsAttackDefenseMode(); }
bool	ScriptUsePlayerReadyStatusMode()							{ return TFGameRules()->UsePlayerReadyStatusMode(); }
bool	ScriptPlayerReadyStatus_HaveMinPlayersToEnable()			{ return TFGameRules()->PlayerReadyStatus_HaveMinPlayersToEnable(); }
bool	ScriptPlayerReadyStatus_ArePlayersOnTeamReady(int iTeam)	{ return TFGameRules()->PlayerReadyStatus_ArePlayersOnTeamReady( iTeam ); }
void	ScriptPlayerReadyStatus_ResetState()						{ TFGameRules()->PlayerReadyStatus_ResetState(); }
bool	ScriptIsDefaultGameMode()									{ return TFGameRules()->IsDefaultGameMode(); }
bool	ScriptIsPVEModeActive()										{ return TFGameRules()->IsPVEModeActive(); }
bool	ScriptAllowThirdPersonCamera()								{ return TFGameRules()->AllowThirdPersonCamera(); }
void	ScriptSetGravityMultiplier( float flMultiplier )			{ return TFGameRules()->SetGravityMultiplier( flMultiplier ); }
float	ScriptGetGravityMultiplier()								{ return TFGameRules()->GetGravityMultiplier(); }
void	ScriptSetPlayersInHell( bool bInHell )						{ return TFGameRules()->SetPlayersInHell( bInHell ); }
bool	ScriptArePlayersInHell()									{ return TFGameRules()->ArePlayersInHell(); }
void	ScriptSetUsingSpells( bool bUsingSpells )					{ return TFGameRules()->SetUsingSpells( bUsingSpells ); }
bool	ScriptIsUsingSpells()										{ return TFGameRules()->IsUsingSpells(); }
bool	ScriptIsUsingGrapplingHook()								{ return TFGameRules()->IsUsingGrapplingHook(); }
bool	ScriptIsTruceActive()										{ return TFGameRules()->IsTruceActive(); }
bool	ScriptMapHasMatchSummaryStage()								{ return TFGameRules()->MapHasMatchSummaryStage(); }
bool	ScriptPlayersAreOnMatchSummaryStage()						{ return TFGameRules()->PlayersAreOnMatchSummaryStage(); }
bool	ScriptHaveStopWatchWinner()									{ return TFGameRules()->HaveStopWatchWinner(); }
void	ScriptSetOvertimeAllowedForCTF( bool bAllowed )				{ TFGameRules()->SetOvertimeAllowedForCTF( bAllowed ); }
bool	ScriptGetOvertimeAllowedForCTF()							{ return TFGameRules()->GetOvertimeAllowedForCTF(); }

void	ScriptForceEnableUpgrades( int nState )						{ TFGameRules()->ForceEnableUpgrades( nState ); }
void	ScriptForceEscortPushLogic( int nState )					{ TFGameRules()->ForceEscortPushLogic( nState ); }

void CTFGameRules::RegisterScriptFunctions()
{
	TF_GAMERULES_SCRIPT_FUNC( GetRoundState,							"Get current round state. See Constants.ERoundState" );
	TF_GAMERULES_SCRIPT_FUNC( IsInWaitingForPlayers,					"Are we waiting for some stragglers?" );
	TF_GAMERULES_SCRIPT_FUNC( GetWinningTeam,							"Who won!" );
	TF_GAMERULES_SCRIPT_FUNC( InOvertime,								"Currently in overtime?" );

	TF_GAMERULES_SCRIPT_FUNC( IsBirthday,								"Are we in birthday mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsHolidayActive,							"Is the given holiday active? See Constants.EHoliday" );
	TF_GAMERULES_SCRIPT_FUNC( PointsMayBeCaptured,						"Are points able to be captured?" );
	TF_GAMERULES_SCRIPT_FUNC( GetClassLimit,							"Get class limit for class. See Constants.ETFClass" );
	TF_GAMERULES_SCRIPT_FUNC( FlagsMayBeCapped,							"May a flag be captured?" );
	TF_GAMERULES_SCRIPT_FUNC( GetStopWatchState,						"Get the current stopwatch state. See Constants.EStopwatchState" );
	TF_GAMERULES_SCRIPT_FUNC( IsInArenaMode,							"Playing arena mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsInKothMode,								"Playing king of the hill mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsInMedievalMode,							"Playing medieval mode?" );
	TF_GAMERULES_SCRIPT_FUNC( IsHolidayMap,								"Playing a holiday map? See Constants.EHoliday" );
	TF_GAMERULES_SCRIPT_FUNC( IsMannVsMachineMode,						"Playing MvM? Beep boop" );
	TF_GAMERULES_SCRIPT_FUNC( GetMannVsMachineAlarmStatus,				"" );
	TF_GAMERULES_SCRIPT_FUNC( SetMannVsMachineAlarmStatus,				"" );
	TF_GAMERULES_SCRIPT_FUNC( IsQuickBuildTime,							"If an engie places a building, will it immediately upgrade? Eg. MvM pre-round etc." );
	TF_GAMERULES_SCRIPT_FUNC( GameModeUsesUpgrades,						"Does the current gamemode have upgrades?" );
	TF_GAMERULES_SCRIPT_FUNC( GameModeUsesCurrency,						"Does the current gamemode have currency?" );
	TF_GAMERULES_SCRIPT_FUNC( GameModeUsesMiniBosses,					"Does the current gamemode have minibosses?" );
	TF_GAMERULES_SCRIPT_FUNC( IsPasstimeMode,							"No ball games." );
	TF_GAMERULES_SCRIPT_FUNC( IsMannVsMachineRespecEnabled,				"Are players allowed to refund their upgrades?" );
	TF_GAMERULES_SCRIPT_FUNC( IsPowerupMode,							"Playing powerup mode? Not compatible with MvM" );
	TF_GAMERULES_SCRIPT_FUNC( IsCompetitiveMode,						"Playing competitive?" );
	TF_GAMERULES_SCRIPT_FUNC( IsMatchTypeCasual,						"Playing casual?" );
	TF_GAMERULES_SCRIPT_FUNC( IsMatchTypeCompetitive,					"Playing competitive?" );
	TF_GAMERULES_SCRIPT_FUNC( InMatchStartCountdown,					"Are we in the pre-match state?" );
	TF_GAMERULES_SCRIPT_FUNC( MatchmakingShouldUseStopwatchMode,		"" );
	TF_GAMERULES_SCRIPT_FUNC( IsAttackDefenseMode,						"" );
	TF_GAMERULES_SCRIPT_FUNC( UsePlayerReadyStatusMode,					"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_HaveMinPlayersToEnable,	"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_ArePlayersOnTeamReady,	"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayerReadyStatus_ResetState,				"" );
	TF_GAMERULES_SCRIPT_FUNC( IsDefaultGameMode,						"The absence of arena, mvm, tournament mode, etc" );
	TF_GAMERULES_SCRIPT_FUNC( IsPVEModeActive,							"" );
	TF_GAMERULES_SCRIPT_FUNC( AllowThirdPersonCamera,					"" );
	TF_GAMERULES_SCRIPT_FUNC( SetGravityMultiplier,						"" );
	TF_GAMERULES_SCRIPT_FUNC( GetGravityMultiplier,						"" );
	TF_GAMERULES_SCRIPT_FUNC( SetPlayersInHell,							"" );
	TF_GAMERULES_SCRIPT_FUNC( ArePlayersInHell,							"" );
	TF_GAMERULES_SCRIPT_FUNC( SetUsingSpells,							"" );
	TF_GAMERULES_SCRIPT_FUNC( IsUsingSpells,							"" );
	TF_GAMERULES_SCRIPT_FUNC( IsUsingGrapplingHook,						"" );
	TF_GAMERULES_SCRIPT_FUNC( IsTruceActive,							"" );
	TF_GAMERULES_SCRIPT_FUNC( MapHasMatchSummaryStage,					"" );
	TF_GAMERULES_SCRIPT_FUNC( PlayersAreOnMatchSummaryStage,			"" );
	TF_GAMERULES_SCRIPT_FUNC( HaveStopWatchWinner,						"" );
	TF_GAMERULES_SCRIPT_FUNC( GetOvertimeAllowedForCTF,					"" );
	TF_GAMERULES_SCRIPT_FUNC( SetOvertimeAllowedForCTF,					"" );

	TF_GAMERULES_SCRIPT_FUNC( ForceEnableUpgrades,						"Whether to force on MvM-styled upgrades on/off. 0 -> default, 1 -> force off, 2 -> force on" );
	TF_GAMERULES_SCRIPT_FUNC( ForceEscortPushLogic,						"Forces payload pushing logic. 0 -> default, 1 -> force off, 2 -> force on" );

	g_pScriptVM->RegisterInstance( &PlayerVoiceListener(), "PlayerVoiceListener" );
}

#endif // GAME_DLL
