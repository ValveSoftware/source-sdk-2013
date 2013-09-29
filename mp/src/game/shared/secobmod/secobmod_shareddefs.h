//=======================================//
//  Source Engine: CoOperative Base Mod  //
//=======================================//
#ifndef SecobMod__SHAREDDEFS_H
#define SecobMod__SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#define HL2_EPISODIC //Choose Episode 2 code for preference. Disabling this means you must exclude all episodic cpp/h files from your project.

/*****************/
/* Base Defines. */
/*****************/
#define SecobMod__MiscFixes //Used when a fix doesn't quite fit in any of the other defined categories. 
#define SecobMod__Enable_Fixed_Multiplayer_AI //Allow AI in your mod, also fixes numerous crashes to do with AI and related features.
#define SecobMod__PREVENT_ITEM_WEAPON_RESPAWNING //Prevent items and weapons from respawning after being picked up by a player. Actually we cheat and set the respawn time insanely high.


/***************************/
/* Singleplayer Additions. */
/***************************/
#define SecobMod__SHOW_GAME_MESSAGES_TO_ALL //Allow all players to see env_messages. Very useful for end-game strings as used in HL2.
#define SecobMod__ALLOW_SUPER_GRAVITY_GUN //Allows Super Gravity Gun. Mostly coded by Kave.
#define SecobMod__MULTIPLAYER_LEVEL_TRANSITIONS //Allow automatic level changes if all players are in the transition zone (percentage can be modified).
#define SecobMod__SAVERESTORE //Carries over loadouts from map to map. Only works if MULTIPLAYER_LEVEL_TRANSITIONS are enabled.
#define SecobMod__FIX_VEHICLE_PLAYER_CAMERA_JUDDER //Vehicle view judder removed. ESSENTIAL if you use vehicles anywhere in your mod.
#define SecobMod__ALLOW_PLAYER_MODELS_IN_VEHICLES //Shows the player model in vehicles (add your own animations-currently we just crouch).

/**************************/
/* Multiplayer Additions. */
/**************************/
#define SecobMod__FRIENDLY_FIRE_ENABLED //Enable friendly fire.
#define SecobMod__ENABLE_DYNAMIC_PLAYER_RESPAWN_CODE //Respawn where you were killed or for preference near a living player as opposed to back at the start of a map.
#define SecobMod__USE_PLAYERCLASSES //Enable player classes.
#define SecobMod__MULTIPLAYER_VIEWCONTROL_CAMERAS //Fixes the View Control Cameras so that all clients are affected by View Control cameras.
//#define SecobMod__FORCE_TEAMPLAY_AS_ALWAYS_ON //Forces teamplay to always be enabled.
#define SecobMod__ENABLE_FAKE_PASSENGER_SEATS //Allows a hacky passenger seat to be added to valves vehicles (jeep and jalopy).

/**********************/
/* Game Enhancements. */
/**********************/
#define SecobMod__STRIDERS_ALWAYS_STOMP_IMPALE_PLAYERS //Force striders to always impale players with their legs.
#define SecobMod__BARNACLES_CAN_SWALLOW_PLAYERS //Allow players to be swallowed by barnacles *schlurrrp*.
#define SecobMod__FIRST_PERSON_RAGDOLL_CAMERA_ON_PLAYER_DEATH //No thirdperson ragdoll view on being killed.
#define SecobMod__PLAYER_MOVEMENT_CAMERA_BOB //Add camera bob to a moving player.

/************************/
/* Player Enhancements. */
/************************/
#define SecobMod__IRONSIGHT_ENABLED //Allow basic version of ironsight. Use the keypad minus key to toggle the ironsight_toggle console command.
#define SecobMod__ENABLE_NIGHTVISION_FOR_HEAVY_CLASS //Give the heavy class nightvision (use the 'N' key to toggle on/off)
#define SecobMod__PLAYERS_CAN_PICKUP_OBJECTS //Players can pickup objects with their hands.
#define SecobMod__CAN_SPRINT_WITHOUT_SUIT //Players can sprint without the HEV suit.
#define SecobMod__HAS_AMMO_REGARDLESS_OF_SUIT //Players have an ammo count.
#define SecobMod__HAS_BATTERY_REGARDLESS_OF_SUIT //Players have battery charge.
#define SecobMod__HAS_DAMAGE_INDICATOR_REGARDLESS_OF_SUIT //Players see damage indication.
#define SecobMod__HAS_HEALTH_REGARDLESS_OF_SUIT //Players have health hud.
#define SecobMod__HAS_HUD_LOCATOR_REGARDLESS_OF_SUIT //Players can see the location of the ep2 jalopy.
#define SecobMod__HAS_NEUROTOXIN_INDICATOR_REGARDLESS_OF_SUIT //Players are cured against poison.
#define SecobMod__HAS_ZOOM_VIEW_FUNCTION_REGARDLESS_OF_SUIT //Players can zoon.
#define SecobMod__HAS_POSTURE_INDICATOR_REGARDLESS_OF_SUIT //No idea but we added it anyway.
#define SecobMod__HAS_SQUAD_INDICATOR_REGARDLESS_OF_SUIT //Shows if you have squad AI following or not.
#define SecobMod__HAS_WEAPONS_SELECTION_REGARDLESS_OF_SUIT //Allows usage of weapons.
#define SecobMod__HAS_POWER_INDICATOR_REGARDLESS_OF_SUIT //Shows sprint etc meter
#define SecobMod__HAS_FLASHLIGHT_REGARDLESS_OF_SUIT //Enables the flashlight except HL2 DM overrides disabling this and provides the flashlight anyway.
#define SecobMod__HAS_GEIGER_COUNTER_REGARDLESS_OF_SUIT //Allows you to detect toxic areas.
//#define SecobMod__HAS_L4D_STYLE_GLOW_EFFECTS //The glowing outline that you see used in L4D (Left4Dead).
//#define GLOWS_ENABLE //Use this to enable Valve's glow code for use with the above (both defines are required).

/**********************/
/* Map Enhancements. */
/*********************/
#define SecobMod__ENABLE_MAP_BRIEFINGS //Have map briefing VGUI windows (identical to MOTD so you can have either briefings or an motd but not both!) describing the loaded map.
#define SecobMod__ENABLE_MAP_SPECIFIC_PLAYER_MODEL_OVERRIDES //This allows mappers to override the default player models with one single player model.
//#define SecobMod__USE_CSS_LADDERS //Use the easier to map with counter strike source ladders in your maps.

/************************/
/* Server Enhancements. */
/************************/
#define SecobMod__Force_LAN_DISABLED //Disable Lan games from activating on server startup.
#define SecobMod__ALLOW_VALVE_APPROVED_CHEATING //Allow sv_cheats to work.
#define SecobMod__MULTIPLAYER_CHAT_BUBBLES //Shows a chat icon while a player is typing.
#define SecobMod__HIGH_PING_VEHICLE_FIX //Remove vehicle judder for players with high latency (lag) at the cost of forward/backwards view dampening.
//#define SecobMod__ALLOW_JEEP_HEADLIGHTS //Allows headlights to work in jeep/jalopy.

/******************************/
/* Experimental Enhancements. */
/******************************/
#define SecobMod__ENABLE_IN_TESTING_ITEMS //Enables buggy test items again, such as the Portal gun. Make sure you have Portal installed, and the portal items line enabled.
#define SecobMod__ENABLE_PORTAL_ITEMS //Currently it likes to crash alot!.

/******************/
/* SDK Bug Fixes. */
/******************/
#define SecobMod__BG_FIX //Stops the MOTD/Briefing Panel and the Class Menu from showing on main menu maps.
#define SecobMod__FIX_SHOTGUN_FAST_SWITCH_BUG //Fixes a bug in the shotgun code - Community fix submitted fix by SubZero.

#endif // SecobMod__SHAREDDEFS_H