//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: VScript constants and enums shared between the server and client.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "activitylist.h"
#include "in_buttons.h"
#ifdef CLIENT_DLL
#include "c_ai_basenpc.h"
#else
#include "ai_basenpc.h"
#include "globalstate.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( IN, "Button mask bindings" )

	DEFINE_ENUMCONST_NAMED( IN_ATTACK, "ATTACK", "Button for +attack" )
	DEFINE_ENUMCONST_NAMED( IN_JUMP, "JUMP", "Button for +jump" )
	DEFINE_ENUMCONST_NAMED( IN_DUCK, "DUCK", "Button for +duck" )
	DEFINE_ENUMCONST_NAMED( IN_FORWARD, "FORWARD", "Button for +forward" )
	DEFINE_ENUMCONST_NAMED( IN_BACK, "BACK", "Button for +back" )
	DEFINE_ENUMCONST_NAMED( IN_USE, "USE", "Button for +use" )
	DEFINE_ENUMCONST_NAMED( IN_CANCEL, "CANCEL", "Special button flag for attack cancel" )
	DEFINE_ENUMCONST_NAMED( IN_LEFT, "LEFT", "Button for +left" )
	DEFINE_ENUMCONST_NAMED( IN_RIGHT, "RIGHT", "Button for +right" )
	DEFINE_ENUMCONST_NAMED( IN_MOVELEFT, "MOVELEFT", "Button for +moveleft" )
	DEFINE_ENUMCONST_NAMED( IN_MOVERIGHT, "MOVERIGHT", "Button for +moveright" )
	DEFINE_ENUMCONST_NAMED( IN_ATTACK2, "ATTACK2", "Button for +attack2" )
	DEFINE_ENUMCONST_NAMED( IN_RUN, "RUN", "Unused button (see IN.SPEED for sprint)" )
	DEFINE_ENUMCONST_NAMED( IN_RELOAD, "RELOAD", "Button for +reload" )
	DEFINE_ENUMCONST_NAMED( IN_ALT1, "ALT1", "Button for +alt1" )
	DEFINE_ENUMCONST_NAMED( IN_ALT2, "ALT2", "Button for +alt2" )
	DEFINE_ENUMCONST_NAMED( IN_SCORE, "SCORE", "Button for +score" )
	DEFINE_ENUMCONST_NAMED( IN_SPEED, "SPEED", "Button for +speed" )
	DEFINE_ENUMCONST_NAMED( IN_WALK, "WALK", "Button for +walk" )
	DEFINE_ENUMCONST_NAMED( IN_ZOOM, "ZOOM", "Button for +zoom" )
	DEFINE_ENUMCONST_NAMED( IN_WEAPON1, "WEAPON1", "Special button used by weapons themselves" )
	DEFINE_ENUMCONST_NAMED( IN_WEAPON2, "WEAPON2", "Special button used by weapons themselves" )
	DEFINE_ENUMCONST_NAMED( IN_BULLRUSH, "BULLRUSH", "Unused button" )
	DEFINE_ENUMCONST_NAMED( IN_GRENADE1, "GRENADE1", "Button for +grenade1" )
	DEFINE_ENUMCONST_NAMED( IN_GRENADE2, "GRENADE2", "Button for +grenade2" )
	DEFINE_ENUMCONST_NAMED( IN_ATTACK3, "ATTACK3", "Button for +attack3" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( RenderMode, "Render modes used by Get/SetRenderMode" )

	DEFINE_ENUMCONST_NAMED( kRenderNormal, "Normal", "" )
	DEFINE_ENUMCONST_NAMED( kRenderTransColor, "Color", "" )
	DEFINE_ENUMCONST_NAMED( kRenderTransTexture, "Texture", "" )
	DEFINE_ENUMCONST_NAMED( kRenderGlow, "Glow", "" )
	DEFINE_ENUMCONST_NAMED( kRenderTransAlpha, "Solid", "" )
	DEFINE_ENUMCONST_NAMED( kRenderTransAdd, "Additive", "" )
	DEFINE_ENUMCONST_NAMED( kRenderEnvironmental, "Environmental", "" )
	DEFINE_ENUMCONST_NAMED( kRenderTransAddFrameBlend, "AdditiveFractionalFrame", "" )
	DEFINE_ENUMCONST_NAMED( kRenderTransAlphaAdd, "Alpha Add", "" )
	DEFINE_ENUMCONST_NAMED( kRenderWorldGlow, "WorldSpaceGlow", "" )
	DEFINE_ENUMCONST_NAMED( kRenderNone, "None", "" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( Hitgroup, "Hit groups from traces" )

	DEFINE_ENUMCONST_NAMED( HITGROUP_GENERIC, "Generic", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_HEAD, "Head", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_CHEST, "Chest", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_STOMACH, "Stomach", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_LEFTARM, "LeftArm", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_RIGHTARM, "RightArm", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_LEFTLEG, "LeftLeg", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_RIGHTLEG, "RightLeg", "" )
	DEFINE_ENUMCONST_NAMED( HITGROUP_GEAR, "Gear", "" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( MapLoad, "Map load enum for GetLoadType()" )

	DEFINE_ENUMCONST_NAMED( MapLoad_NewGame, "NewGame", "Map was loaded from a new game" )
	DEFINE_ENUMCONST_NAMED( MapLoad_LoadGame, "LoadGame", "Map was loaded from a save file" )
	DEFINE_ENUMCONST_NAMED( MapLoad_Transition, "Transition", "Map was loaded from a level transition" )
	DEFINE_ENUMCONST_NAMED( MapLoad_Background, "Background", "Map was loaded as a background map" )

END_SCRIPTENUM();

//=============================================================================
//=============================================================================

void RegisterActivityConstants()
{
	// Make sure there are no activities declared yet
	if (g_pScriptVM->ValueExists( "ACT_RESET" ))
		return;

	// Register activity constants by just iterating through the entire activity list
	for (int i = 0; i < ActivityList_HighestIndex(); i++)
	{
		ScriptRegisterConstantNamed( g_pScriptVM, i, ActivityList_NameForIndex(i), "" );
	}
}

//=============================================================================
//=============================================================================

extern void RegisterWeaponScriptConstants();

void RegisterSharedScriptConstants()
{
	// 
	// Activities
	// 

	// Scripts have to use this function before using any activity constants.
	// This is because initializing 1,700+ constants every time a level loads and letting them lay around
	// usually doing nothing sounds like a bad idea.
	ScriptRegisterFunction( g_pScriptVM, RegisterActivityConstants, "Registers all activity IDs as usable constants." );


	// 
	// Damage Types
	// 
	ScriptRegisterConstant( g_pScriptVM, DMG_GENERIC, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_CRUSH, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_BULLET, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_SLASH, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_BURN, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_VEHICLE, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_FALL, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_BLAST, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_CLUB, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_SHOCK, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_SONIC, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_ENERGYBEAM, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_PREVENT_PHYSICS_FORCE, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_NEVERGIB, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_ALWAYSGIB, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_DROWN, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_PARALYZE, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_NERVEGAS, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_POISON, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_RADIATION, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_DROWNRECOVER, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_ACID, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_SLOWBURN, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_REMOVENORAGDOLL, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_PHYSGUN, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_PLASMA, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_AIRBOAT, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_DISSOLVE, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_BLAST_SURFACE, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_DIRECT, "Damage type used in damage information." );
	ScriptRegisterConstant( g_pScriptVM, DMG_BUCKSHOT, "Damage type used in damage information." );

	// 
	// Collision Groups
	// 
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NONE, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DEBRIS, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DEBRIS_TRIGGER, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_INTERACTIVE_DEBRIS, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_INTERACTIVE, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PLAYER, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_BREAKABLE_GLASS, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_VEHICLE, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PLAYER_MOVEMENT, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NPC, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_IN_VEHICLE, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_WEAPON, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_VEHICLE_CLIP, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PROJECTILE, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DOOR_BLOCKER, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PASSABLE_DOOR, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DISSOLVING, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PUSHAWAY, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NPC_ACTOR, "Collision group used in GetCollisionGroup(), etc." );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NPC_SCRIPTED, "Collision group used in GetCollisionGroup(), etc." );

	// 
	// Flags
	// 
	ScriptRegisterConstant( g_pScriptVM, FL_ONGROUND, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_DUCKING, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_WATERJUMP, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_ONTRAIN, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_INRAIN, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_FROZEN, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_ATCONTROLS, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_CLIENT, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_FAKECLIENT, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_INWATER, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_FLY, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_SWIM, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_CONVEYOR, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_NPC, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_GODMODE, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_NOTARGET, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_AIMTARGET, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_PARTIALGROUND, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_STATICPROP, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_GRAPHED, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_GRENADE, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_STEPMOVEMENT, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_DONTTOUCH, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_BASEVELOCITY, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_WORLDBRUSH, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_OBJECT, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_KILLME, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_ONFIRE, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_DISSOLVING, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_TRANSRAGDOLL, "Flag used in GetFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FL_UNBLOCKABLE_BY_PLAYER, "Flag used in GetFlags(), etc." );

	// 
	// Entity Flags
	// 
	ScriptRegisterConstant( g_pScriptVM, EFL_KILLME, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DORMANT, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NOCLIP_ACTIVE, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_SETTING_UP_BONES, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_KEEP_ON_RECREATE_ENTITIES, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_HAS_PLAYER_CHILD, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_SHADOWUPDATE, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NOTIFY, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_FORCE_CHECK_TRANSMIT, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_BOT_FROZEN, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_SERVER_ONLY, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_AUTO_EDICT_ATTACH, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_ABSTRANSFORM, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_ABSVELOCITY, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_ABSANGVELOCITY, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_SPATIAL_PARTITION, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_PLUGIN_BASED_BOT, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_IN_SKYBOX, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_USE_PARTITION_WHEN_NOT_SOLID, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_TOUCHING_FLUID, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_IS_BEING_LIFTED_BY_BARNACLE, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_ROTORWASH_PUSH, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_THINK_FUNCTION, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_GAME_PHYSICS_SIMULATION, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_CHECK_UNTOUCH, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DONTBLOCKLOS, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_DONTWALKON, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_DISSOLVE, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_MEGAPHYSCANNON_RAGDOLL, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_WATER_VELOCITY_CHANGE, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_PHYSCANNON_INTERACTION, "Entity flag used in GetEFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_DAMAGE_FORCES, "Entity flag used in GetEFlags(), etc." );

	// 
	// Effects
	// 
	ScriptRegisterConstant( g_pScriptVM, EF_BONEMERGE, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_BRIGHTLIGHT, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_DIMLIGHT, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_NOINTERP, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_NOSHADOW, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_NODRAW, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_NORECEIVESHADOW, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_BONEMERGE_FASTCULL, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_ITEM_BLINK, "Effect flag used in GetEffects(), etc." );
	ScriptRegisterConstant( g_pScriptVM, EF_PARENT_ANIMATES, "Effect flag used in GetEffects(), etc." );

	// 
	// Solid Flags
	// 
	ScriptRegisterConstant( g_pScriptVM, FSOLID_CUSTOMRAYTEST, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_CUSTOMBOXTEST, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_NOT_SOLID, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_TRIGGER, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_NOT_STANDABLE, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_VOLUME_CONTENTS, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_FORCE_WORLD_ALIGNED, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_USE_TRIGGER_BOUNDS, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_ROOT_PARENT_ALIGNED, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_TRIGGER_TOUCH_DEBRIS, "Solid flag used in GetSolidFlags(), etc." );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_COLLIDE_WITH_OWNER, "Solid flag used in GetSolidFlags(), etc." );

	// 
	// Movetypes
	// 
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_NONE, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_ISOMETRIC, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_WALK, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_STEP, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_FLY, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_FLYGRAVITY, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_VPHYSICS, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_PUSH, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_NOCLIP, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_LADDER, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_OBSERVER, "Move type used in GetMoveType(), etc." );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_CUSTOM, "Move type used in GetMoveType(), etc." );

#ifdef GAME_DLL
	// 
	// Sound Types, Contexts, and Channels
	// (QueryHearSound hook can use these)
	// 
	ScriptRegisterConstant( g_pScriptVM, SOUND_NONE, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_COMBAT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_WORLD, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PLAYER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_DANGER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_BULLET_IMPACT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CARCASS, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_MEAT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_GARBAGE, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_THUMPER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_BUGBAIT, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PHYSICS_DANGER, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_DANGER_SNIPERONLY, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_MOVE_AWAY, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PLAYER_VEHICLE, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_LOW, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_MEDIUM, "Sound type used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_HIGH, "Sound type used in QueryHearSound hooks, etc." );

	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_FROM_SNIPER, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_GUNFIRE, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_MORTAR, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_COMBINE_ONLY, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_REACT_TO_SOURCE, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_EXPLOSION, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_EXCLUDE_COMBINE, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_DANGER_APPROACH, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_ALLIES_ONLY, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_PLAYER_VEHICLE, "Sound context used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_OWNER_ALLIES, "Sound context used in QueryHearSound hooks, etc." );

	ScriptRegisterConstant( g_pScriptVM, ALL_CONTEXTS, "All sound contexts useable in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, ALL_SCENTS, "All \"scent\" sound types useable in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, ALL_SOUNDS, "All sound types useable in QueryHearSound hooks, etc." );

	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_UNSPECIFIED, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATING, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATED_DANGER, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_WEAPON, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_INJURY, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_BULLET_IMPACT, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_NPC_FOOTSTEP, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_SPOOKY_NOISE, "Sound channel used in QueryHearSound hooks, etc." );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_ZOMBINE_GRENADE, "Sound channel used in QueryHearSound hooks, etc." );

	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_MACHINEGUN, "SOUNDENT_VOLUME_MACHINEGUN", "Sound volume preset for use in InsertAISound, etc." );
	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_SHOTGUN, "SOUNDENT_VOLUME_SHOTGUN", "Sound volume preset for use in InsertAISound, etc." );
	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_PISTOL, "SOUNDENT_VOLUME_PISTOL", "Sound volume preset for use in InsertAISound, etc." );
	ScriptRegisterConstantNamed( g_pScriptVM, (int)SOUNDENT_VOLUME_EMPTY, "SOUNDENT_VOLUME_PISTOL", "Sound volume preset for use in InsertAISound, etc." );

	// 
	// Capabilities
	// 
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_GROUND, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_JUMP, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_FLY, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_CLIMB, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_SWIM, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_CRAWL, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_SHOOT, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SKIP_NAV_GROUND_CHECK, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	//ScriptRegisterConstant( g_pScriptVM, bits_CAP_HEAR, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_AUTO_DOORS, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_OPEN_DOORS, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_TURN_HEAD, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_RANGE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_RANGE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_MELEE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_MELEE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_RANGE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_RANGE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_MELEE_ATTACK1, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_MELEE_ATTACK2, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE_WEAPONS, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	//ScriptRegisterConstant( g_pScriptVM, bits_CAP_STRAFE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_ANIMATEDFACE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE_SHOT_REGULATOR, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_FRIENDLY_DMG_IMMUNE, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SQUAD, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_DUCK, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_NO_HIT_PLAYER, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_AIM_GUN, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_NO_HIT_SQUADMATES, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SIMPLE_RADIUS_DAMAGE, "NPC/player/weapon capability used in GetCapabilities(), etc." );

	ScriptRegisterConstant( g_pScriptVM, bits_CAP_DOORS_GROUP, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_RANGE_ATTACK_GROUP, "NPC/player/weapon capability used in GetCapabilities(), etc." );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MELEE_ATTACK_GROUP, "NPC/player/weapon capability used in GetCapabilities(), etc." );

	// 
	// Class_T classes
	// 
	ScriptRegisterConstant( g_pScriptVM, CLASS_NONE, "No class." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER, "Used by players." );

#ifdef HL2_DLL

	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY, "Used by citizens, hacked manhacks, and other misc. allies." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY_VITAL, "Used by Alyx, Barney, and other allies vital to HL2." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ANTLION, "Used by antlions, antlion guards, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_BARNACLE, "Used by barnacles." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_BULLSEYE, "Used by npc_bullseye." );
	//ScriptRegisterConstant( g_pScriptVM, CLASS_BULLSQUID, "Used by bullsquids." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_CITIZEN_PASSIVE, "Used by citizens when the \"gordon_precriminal\" or \"citizens_passive\" states are enabled." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_CITIZEN_REBEL, "UNUSED IN HL2. Rebels normally use CLASS_PLAYER_ALLY." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_COMBINE, "Used by Combine soldiers, Combine turrets, and other misc. Combine NPCs." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_COMBINE_GUNSHIP, "Used by Combine gunships, helicopters, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_CONSCRIPT, "UNUSED IN HL2. Would've been used by conscripts." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_HEADCRAB, "Used by headcrabs." );
	//ScriptRegisterConstant( g_pScriptVM, CLASS_HOUNDEYE, "Used by houndeyes." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_MANHACK, "Used by Combine manhacks." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_METROPOLICE, "Used by Combine metrocops." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_MILITARY, "In HL2, this is only used by npc_combinecamera and func_guntarget. This appears to be recognized as a Combine class." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_SCANNER, "Used by Combine city scanners and claw scanners." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_STALKER, "Used by Combine stalkers." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_VORTIGAUNT, "Used by vortigaunts." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ZOMBIE, "Used by zombies." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PROTOSNIPER, "Used by Combine snipers." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_MISSILE, "Used by RPG and APC missiles." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_FLARE, "Used by env_flares." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_EARTH_FAUNA, "Used by birds and other terrestrial animals." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_HACKED_ROLLERMINE, "Used by rollermines which were hacked by Alyx." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_COMBINE_HUNTER, "Used by Combine hunters." );

#elif defined( HL1_DLL )

	ScriptRegisterConstant( g_pScriptVM, CLASS_HUMAN_PASSIVE, "Used by scientists." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_HUMAN_MILITARY, "Used by HECU marines, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_MILITARY, "Used by alien grunts, alien slaves/vortigaunts, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_MONSTER, "Used by zombies, houndeyes, barnacles, and other misc. monsters." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_PREY, "Used by headcrabs, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_PREDATOR, "Used by bullsquids, etc." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_INSECT, "Used by cockroaches." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY, "Used by security guards/Barneys." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_BIOWEAPON, "Used by a player's hivehand hornets." );
	ScriptRegisterConstant( g_pScriptVM, CLASS_ALIEN_BIOWEAPON, "Used by an alien grunt's hivehand hornets." );

#else

	ScriptRegisterConstant( g_pScriptVM, CLASS_PLAYER_ALLY, "Used by player allies." );

#endif

	ScriptRegisterConstant( g_pScriptVM, NUM_AI_CLASSES, "Number of AI classes." );

	// 
	// Misc. AI
	// 
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_INVALID, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_NONE, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_IDLE, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_ALERT, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_COMBAT, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_SCRIPT, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_PLAYDEAD, "NPC state type used in GetNPCState(), etc." );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_PRONE, "When in clutches of barnacle (NPC state type used in GetNPCState(), etc.)" );
	ScriptRegisterConstant( g_pScriptVM, NPC_STATE_DEAD, "NPC state type used in GetNPCState(), etc." );

	ScriptRegisterConstant( g_pScriptVM, AISS_AWAKE, "NPC is awake. (NPC sleep state used in Get/SetSleepState())" );
	ScriptRegisterConstant( g_pScriptVM, AISS_WAITING_FOR_THREAT, "NPC is asleep and will awaken upon seeing an enemy. (NPC sleep state used in Get/SetSleepState())" );
	ScriptRegisterConstant( g_pScriptVM, AISS_WAITING_FOR_PVS, "NPC is asleep and will awaken upon entering a player's PVS. (NPC sleep state used in Get/SetSleepState())" );
	ScriptRegisterConstant( g_pScriptVM, AISS_WAITING_FOR_INPUT, "NPC is asleep and will only awaken upon receiving the Wake input. (NPC sleep state used in Get/SetSleepState())" );
	//ScriptRegisterConstant( g_pScriptVM, AISS_AUTO_PVS, "" );
	//ScriptRegisterConstant( g_pScriptVM, AISS_AUTO_PVS_AFTER_PVS, "" );
	ScriptRegisterConstant( g_pScriptVM, AI_SLEEP_FLAGS_NONE, "No sleep flags. (NPC sleep flag used in Add/Remove/HasSleepFlags())" );
	ScriptRegisterConstant( g_pScriptVM, AI_SLEEP_FLAG_AUTO_PVS, "Indicates a NPC will sleep upon exiting PVS. (NPC sleep flag used in Add/Remove/HasSleepFlags())" );
	ScriptRegisterConstant( g_pScriptVM, AI_SLEEP_FLAG_AUTO_PVS_AFTER_PVS, "Indicates a NPC will sleep upon exiting PVS after entering PVS for the first time(?????) (NPC sleep flag used in Add/Remove/HasSleepFlags())" );

	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_PLAYING, "SCRIPT_PLAYING", "Playing the action animation." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_WAIT, "SCRIPT_WAIT", "Waiting on everyone in the script to be ready. Plays the pre idle animation if there is one." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_POST_IDLE, "SCRIPT_POST_IDLE", "Playing the post idle animation after playing the action animation." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_CLEANUP, "SCRIPT_CLEANUP", "Cancelling the script / cleaning up." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_WALK_TO_MARK, "SCRIPT_WALK_TO_MARK", "Walking to the scripted sequence position." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_RUN_TO_MARK, "SCRIPT_RUN_TO_MARK", "Running to the scripted sequence position." );
	ScriptRegisterConstantNamed( g_pScriptVM, CAI_BaseNPC::SCRIPT_PLAYING, "SCRIPT_PLAYING", "Moving to the scripted sequence position while playing a custom movement animation." );
#endif

	// 
	// Misc. General
	// 
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_NO, "Don't take damage (Use with GetTakeDamage/SetTakeDamage)" );
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_EVENTS_ONLY, "Call damage functions, but don't modify health (Use with GetTakeDamage/SetTakeDamage)" );
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_YES, "Allow damage to be taken (Use with GetTakeDamage/SetTakeDamage)" );
	ScriptRegisterConstant( g_pScriptVM, DAMAGE_AIM, "(Use with GetTakeDamage/SetTakeDamage)" );

#ifdef GAME_DLL
	ScriptRegisterConstant( g_pScriptVM, GLOBAL_OFF, "Global state used by the Globals singleton." );
	ScriptRegisterConstant( g_pScriptVM, GLOBAL_ON, "Global state used by the Globals singleton." );
	ScriptRegisterConstant( g_pScriptVM, GLOBAL_DEAD, "Global state used by the Globals singleton." );
#endif

	RegisterWeaponScriptConstants();
}
