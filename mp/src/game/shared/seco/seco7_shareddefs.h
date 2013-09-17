//====================================================//
//  Source Engine CoOperative v7.					  //
//====================================================//
#ifndef seco7_SHAREDDEFS_H
#define seco7_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------//
// Additional Searches/Information for coders. //
//-------------------------------------------//
//4WH - Mounting Code:
//4WH - Compile Issues:
//4WH - Episodic Issues:
//4WH - Null Pointers:
//4WH - Information:
//4WH - Portal Information:
//4WH - CodeAddendumms:        (these are addendums provided by others on the Valve wiki page).
//
// To those wishing to make a co-op version of the Half-Life 2/Ep1/2 maps, Mulekick on the steam forums gave this advice for older maps:
// - This does mean that you will need to start using the Everything solution file, which means more work settings things up again -
//Basically brushes that block LOS from AI's won't work with older maps.
//To fix this:
//In public/bspflags.h, change the line starting with
//#define MASK_BLOCKLOS (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
//to:
//#define MASK_BLOCKLOS (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS|CONTENTS_OPAQUE)

/*****************/
/* Base Defines. */
/*****************/
#define Seco7_Enable_Fixed_Multiplayer_AI //Allow AI in your mod, also fixes numerous crashes to do with AI and related features.
#define HL2_EPISODIC //Choose Episode 2 code for preference. Reccomended for Orange Box. Disabling this means you must exclude all episodic cpp/h files from your project (due to compile erros you'll get otherwise).
#define Seco7_USE_STATIC_MOUNT_CODE //Use static mounting code to mount multiple games. This must also be defined for dynamic mounts.
#define Seco7_USE_DYNAMIC_MOUNT_CODE //This is a much better mounting system allowing for true content mounting as though a map were that game. Maps must be named ss_hl2_, ss_ep1_ and ss_ep2_ for valid mounting.
#define Seco7_ENABLE_PORTAL_CONTENT_MOUNTING //Portal is seperate to the rest of the mounting code as it will crash anyone without Portal installed in most cases if a Portal map is loaded.
#define Seco7_PREVENT_ITEM_WEAPON_RESPAWNING //Prevent items and weapons from respawning after picked up by a player. Actually we cheat and set the respawn time insanely high.

/***************************/
/* Singleplayer Additions. */
/***************************/
#define Seco7_SHOW_GAME_MESSAGES_TO_ALL //Allow all players to see env_messages. Very useful for end-game strings as used in HL2.
#define Seco7_ALLOW_SUPER_GRAVITY_GUN //Allows Super Gravity Gun. Mostly coded by Kave.
#define Seco7_MULTIPLAYER_LEVEL_TRANSITIONS //Allow automatic level changes if all players are in the transition zone (percantage can be modified).
#define Seco7_SAVERESTORE //Carries over loadouts from map to map. Only works if MULTIPLAYER_LEVEL_TRANSITIONS are enabled.
#define Seco7_FIX_VEHICLE_PLAYER_CAMERA_JUDDER //Vehicle view judder removed. ESSENTIAL if you use vehicles anywhere in your mod.
#define Seco7_ALLOW_PLAYER_MODELS_IN_VEHICLES //Shows the player model in vehicles.

/**************************/
/* Multiplayer Additions. */
/**************************/
#define Seco7_FRIENDLY_FIRE_ENABLED //Enable friendly fire.
#define Seco7_ENABLE_DYNAMIC_PLAYER_RESPAWN_CODE //Respawn where you were killed or for preference near a living player as opposed to back at the start of a map.
#define Seco7_USE_PLAYERCLASSES //Enable player classes.
#define Seco7_MULTIPLAYER_VIEWCONTROL_CAMERAS //Fixes the View Control Cameras so that all clients are affected by View Control cameras.
#define Seco7_FORCE_TEAMPLAY_AS_ALWAYS_ON //Forces teamplay to always be enabled.
#define Seco7_ENABLE_TEAMPLAY_FIXES //Fixes errors between AI and players when teams are in use, and AI are enabled.
#define Seco7_ENABLE_FAKE_PASSENGER_SEATS //Allows a hacky passenger seat to be added to valves vehicles (jeep and jalopy).

/**********************/
/* Game Enhancements. */
/**********************/
#define Seco7_STRIDERS_ALWAYS_STOMP_IMPALE_PLAYERS //Force striders to always impale players with their legs.
#define Seco7_BARNACLES_CAN_SWALLOW_PLAYERS //Allow players to be swallowed by barnacles *schlurrrp*.
#define Seco7_FIRST_PERSON_RAGDOLL_CAMERA_ON_PLAYER_DEATH //No thirdperson ragdoll view on being killed.
#define Seco7_PLAYER_MOVEMENT_CAMERA_BOB //Add camera bob to a moving player.

/************************/
/* Player Enhancements. */
/************************/
#define Seco7_IRONSIGHT_ENABLED //Allow basic version of ironsight.
#define Seco7_ENABLE_NIGHTVISION_FOR_HEAVY_CLASS
#define Seco7_PLAYERS_CAN_PICKUP_OBJECTS //Players can pickup objects with their hands.
#define Seco7_CAN_SPRINT_WITHOUT_SUIT //Players can sprint without the HEV suit.
#define Seco7_HAS_AMMO_REGARDLESS_OF_SUIT //Players have an ammo count.
#define Seco7_HAS_BATTERY_REGARDLESS_OF_SUIT //Players have battery charge.
#define Seco7_HAS_DAMAGE_INDICATOR_REGARDLESS_OF_SUIT //Players see damage indication.
#define Seco7_HAS_HEALTH_REGARDLESS_OF_SUIT //Players have health hud.
#define Seco7_HAS_HUD_LOCATOR_REGARDLESS_OF_SUIT //Players can see the location of the ep2 jalopy.
#define Seco7_HAS_NEUROTOXIN_INDICATOR_REGARDLESS_OF_SUIT //Players are cured against poison.
#define Seco7_HAS_ZOOM_VIEW_FUNCTION_REGARDLESS_OF_SUIT //Players can zoon.
#define Seco7_HAS_POSTURE_INDICATOR_REGARDLESS_OF_SUIT //No idea but we added it anyway.
#define Seco7_HAS_SQUAD_INDICATOR_REGARDLESS_OF_SUIT //Shows if you have squad AI following or not.
#define Seco7_HAS_WEAPONS_SELECTION_REGARDLESS_OF_SUIT //Allows usage of weapons.
#define Seco7_HAS_POWER_INDICATOR_REGARDLESS_OF_SUIT //Shows sprint etc meter
#define Seco7_HAS_FLASHLIGHT_REGARDLESS_OF_SUIT //Enables the flashlight except HL2 DM overrides disabling this and provides the flashlight anyway.
#define Seco7_HAS_GEIGER_COUNTER_REGARDLESS_OF_SUIT //Allows you to detect toxic areas.

/**********************/
/* Map Enhancements. */
/*********************/
#define Seco7_ENABLE_MAP_BRIEFINGS //Have map briefing VGUI windows (identical to MOTD) describing the loaded map.
#define Seco7_ENABLE_MAP_SPECIFIC_PLAYER_MODEL_OVERRIDES //This allows mappers to override the default player models with one single player model.
#define Seco7_USE_CSS_LADDERS //Use the easier to map with counter strike source ladders in your maps.

/************************/
/* Server Enhancements. */
/************************/
#define Seco7_Force_LAN_DISABLED //Disable Lan games from activating on server startup.
#define Seco7_ALLOW_VALVE_APPROVED_CHEATING //Allow sv_cheats to work.
#define Seco7_MULTIPLAYER_CHAT_BUBBLES //Shows a chat icon while a player is typing.
#define Seco7_HIGH_PING_VEHICLE_FIX //Remove vehicle judder for players with high latency (lag) at the cost of forward/backwards view dampening.
#define Seco7_ALLOW_JEEP_HEADLIGHTS //Allows headlights to work in jeep/jalopy.

/******************************/
/* Experimental Enhancements. */
/******************************/
#define Seco7_ENABLE_IN_TESTING_ITEMS //Enables buggy test items again, such as the Portal gun. Make sure you have Portal installed, and the portal mounting line enabled. Currently it likes to crash but I don't think it'd take much editing to fix, it's working fine, it just needs it to remove/recreate a new portal whenever you fire again.

/******************/
/* SDK Bug Fixes. */
/******************/
#define Seco7_BG_MOTD_FIX //Stops the MOTD showing on main menu maps.
#define Seco7_FIX_SHOTGUN_FAST_SWITCH_BUG //Fixes a bug in the shotgun code - Community fix submitted fix by SubZero.


#endif // Seco7_SHAREDDEFS_H