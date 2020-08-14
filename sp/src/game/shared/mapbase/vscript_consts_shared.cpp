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
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//=============================================================================
//=============================================================================

BEGIN_SCRIPTENUM( IN, "Button mask bindings" )

	DEFINE_ENUMCONST_NAMED( IN_ATTACK, "ATTACK", "" )
	DEFINE_ENUMCONST_NAMED( IN_JUMP, "JUMP", "" )
	DEFINE_ENUMCONST_NAMED( IN_DUCK, "DUCK", "" )
	DEFINE_ENUMCONST_NAMED( IN_FORWARD, "FORWARD", "" )
	DEFINE_ENUMCONST_NAMED( IN_BACK, "BACK", "" )
	DEFINE_ENUMCONST_NAMED( IN_USE, "USE", "" )
	DEFINE_ENUMCONST_NAMED( IN_CANCEL, "CANCEL", "" )
	DEFINE_ENUMCONST_NAMED( IN_LEFT, "LEFT", "" )
	DEFINE_ENUMCONST_NAMED( IN_RIGHT, "RIGHT", "" )
	DEFINE_ENUMCONST_NAMED( IN_MOVELEFT, "MOVELEFT", "" )
	DEFINE_ENUMCONST_NAMED( IN_MOVERIGHT, "MOVERIGHT", "" )
	DEFINE_ENUMCONST_NAMED( IN_ATTACK2, "ATTACK2", "" )
	DEFINE_ENUMCONST_NAMED( IN_RUN, "RUN", "" )
	DEFINE_ENUMCONST_NAMED( IN_RELOAD, "RELOAD", "" )
	DEFINE_ENUMCONST_NAMED( IN_ALT1, "ALT1", "" )
	DEFINE_ENUMCONST_NAMED( IN_ALT2, "ALT2", "" )
	DEFINE_ENUMCONST_NAMED( IN_SCORE, "SCORE", "" )
	DEFINE_ENUMCONST_NAMED( IN_SPEED, "SPEED", "" )
	DEFINE_ENUMCONST_NAMED( IN_WALK, "WALK", "" )
	DEFINE_ENUMCONST_NAMED( IN_ZOOM, "ZOOM", "" )
	DEFINE_ENUMCONST_NAMED( IN_WEAPON1, "WEAPON1", "" )
	DEFINE_ENUMCONST_NAMED( IN_WEAPON2, "WEAPON2", "" )
	DEFINE_ENUMCONST_NAMED( IN_BULLRUSH, "BULLRUSH", "" )
	DEFINE_ENUMCONST_NAMED( IN_GRENADE1, "GRENADE1", "" )
	DEFINE_ENUMCONST_NAMED( IN_GRENADE2, "GRENADE2", "" )
	DEFINE_ENUMCONST_NAMED( IN_ATTACK3, "ATTACK3", "" )

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
	ScriptRegisterConstant( g_pScriptVM, DMG_GENERIC, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_CRUSH, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_BULLET, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_SLASH, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_BURN, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_VEHICLE, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_FALL, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_BLAST, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_CLUB, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_SHOCK, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_SONIC, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_ENERGYBEAM, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_PREVENT_PHYSICS_FORCE, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_NEVERGIB, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_ALWAYSGIB, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_DROWN, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_PARALYZE, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_NERVEGAS, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_POISON, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_RADIATION, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_DROWNRECOVER, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_ACID, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_SLOWBURN, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_REMOVENORAGDOLL, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_PHYSGUN, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_PLASMA, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_AIRBOAT, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_DISSOLVE, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_BLAST_SURFACE, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_DIRECT, "" );
	ScriptRegisterConstant( g_pScriptVM, DMG_BUCKSHOT, "" );

	// 
	// Trace Contents/Masks
	// 
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_EMPTY, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_SOLID, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_WINDOW, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_AUX, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_GRATE, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_SLIME, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_WATER, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_BLOCKLOS, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_OPAQUE, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_TESTFOGVOLUME, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_TEAM1, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_TEAM2, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_IGNORE_NODRAW_OPAQUE, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_MOVEABLE, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_AREAPORTAL, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_PLAYERCLIP, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_MONSTERCLIP, "" );

	ScriptRegisterConstant( g_pScriptVM, CONTENTS_CURRENT_0, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_CURRENT_90, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_CURRENT_180, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_CURRENT_270, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_CURRENT_UP, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_CURRENT_DOWN, "" );

	ScriptRegisterConstant( g_pScriptVM, CONTENTS_ORIGIN, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_MONSTER, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_DEBRIS, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_DETAIL, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_TRANSLUCENT, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_LADDER, "" );
	ScriptRegisterConstant( g_pScriptVM, CONTENTS_HITBOX, "" );

	ScriptRegisterConstant( g_pScriptVM, LAST_VISIBLE_CONTENTS, "" );
	ScriptRegisterConstant( g_pScriptVM, ALL_VISIBLE_CONTENTS, "" );

	ScriptRegisterConstant( g_pScriptVM, MASK_SOLID, "Spatial content mask representing solid objects (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_PLAYERSOLID, "Spatial content mask representing objects solid to the player, including player clips (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_NPCSOLID, "Spatial content mask representing objects solid to NPCs, including NPC clips (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_WATER, "Spatial content mask representing water and slime solids (CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_OPAQUE, "Spatial content mask representing objects which block lighting (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_OPAQUE_AND_NPCS, "Spatial content mask equivalent to MASK_OPAQUE, but also including NPCs (MASK_OPAQUE|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_BLOCKLOS, "Spatial content mask representing objects which block LOS for AI (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_BLOCKLOS_AND_NPCS, "Spatial content mask equivalent to MASK_BLOCKLOS, but also including NPCs (MASK_BLOCKLOS|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_VISIBLE, "Spatial content mask representing objects which block LOS for players (MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_VISIBLE_AND_NPCS, "Spatial content mask equivalent to MASK_VISIBLE, but also including NPCs (MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_SHOT, "Spatial content mask representing objects solid to bullets (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_SHOT_HULL, "Spatial content mask representing objects solid to non-raycasted weapons, including grates (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_SHOT_PORTAL, "Spatial content mask equivalent to MASK_SHOT, but excluding debris and not using expensive hitbox calculations (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_SOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_SOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_PLAYERSOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_PLAYERSOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_NPCSOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_NPCSOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_NPCWORLDSTATIC, "Spatial content mask representing objects static to NPCs, used for nodegraph rebuilding (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( g_pScriptVM, MASK_SPLITAREAPORTAL, "Spatial content mask representing objects which can split areaportals (CONTENTS_WATER|CONTENTS_SLIME)" );

	// 
	// Collision Groups
	// 
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NONE, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DEBRIS, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DEBRIS_TRIGGER, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_INTERACTIVE_DEBRIS, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_INTERACTIVE, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PLAYER, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_BREAKABLE_GLASS, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_VEHICLE, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PLAYER_MOVEMENT, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NPC, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_IN_VEHICLE, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_WEAPON, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_VEHICLE_CLIP, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PROJECTILE, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DOOR_BLOCKER, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PASSABLE_DOOR, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_DISSOLVING, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_PUSHAWAY, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NPC_ACTOR, "" );
	ScriptRegisterConstant( g_pScriptVM, COLLISION_GROUP_NPC_SCRIPTED, "" );

	// 
	// Entity Flags
	// 
	ScriptRegisterConstant( g_pScriptVM, EFL_KILLME, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DORMANT, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NOCLIP_ACTIVE, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_SETTING_UP_BONES, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_KEEP_ON_RECREATE_ENTITIES, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_HAS_PLAYER_CHILD, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_SHADOWUPDATE, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NOTIFY, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_FORCE_CHECK_TRANSMIT, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_BOT_FROZEN, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_SERVER_ONLY, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_AUTO_EDICT_ATTACH, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_ABSTRANSFORM, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_ABSVELOCITY, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_ABSANGVELOCITY, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DIRTY_SPATIAL_PARTITION, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_PLUGIN_BASED_BOT, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_IN_SKYBOX, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_USE_PARTITION_WHEN_NOT_SOLID, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_TOUCHING_FLUID, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_IS_BEING_LIFTED_BY_BARNACLE, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_ROTORWASH_PUSH, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_THINK_FUNCTION, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_GAME_PHYSICS_SIMULATION, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_CHECK_UNTOUCH, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DONTBLOCKLOS, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_DONTWALKON, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_DISSOLVE, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_MEGAPHYSCANNON_RAGDOLL, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_WATER_VELOCITY_CHANGE, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_PHYSCANNON_INTERACTION, "" );
	ScriptRegisterConstant( g_pScriptVM, EFL_NO_DAMAGE_FORCES, "" );

	// 
	// Effects
	// 
	ScriptRegisterConstant( g_pScriptVM, EF_BONEMERGE, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_BRIGHTLIGHT, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_DIMLIGHT, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_NOINTERP, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_NOSHADOW, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_NODRAW, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_NORECEIVESHADOW, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_BONEMERGE_FASTCULL, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_ITEM_BLINK, "" );
	ScriptRegisterConstant( g_pScriptVM, EF_PARENT_ANIMATES, "" );

	// 
	// Solid Flags
	// 
	ScriptRegisterConstant( g_pScriptVM, FSOLID_CUSTOMRAYTEST, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_CUSTOMBOXTEST, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_NOT_SOLID, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_TRIGGER, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_NOT_STANDABLE, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_VOLUME_CONTENTS, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_FORCE_WORLD_ALIGNED, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_USE_TRIGGER_BOUNDS, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_ROOT_PARENT_ALIGNED, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_TRIGGER_TOUCH_DEBRIS, "" );
	ScriptRegisterConstant( g_pScriptVM, FSOLID_COLLIDE_WITH_OWNER, "" );

	// 
	// Movetypes
	// 
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_NONE, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_ISOMETRIC, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_WALK, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_STEP, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_FLY, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_FLYGRAVITY, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_VPHYSICS, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_PUSH, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_NOCLIP, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_LADDER, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_OBSERVER, "" );
	ScriptRegisterConstant( g_pScriptVM, MOVETYPE_CUSTOM, "" );

#ifdef GAME_DLL
	// 
	// Sound Types, Contexts, and Channels
	// (QueryHearSound hook can use these)
	// 
	ScriptRegisterConstant( g_pScriptVM, SOUND_NONE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_COMBAT, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_WORLD, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PLAYER, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_DANGER, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_BULLET_IMPACT, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CARCASS, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_MEAT, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_GARBAGE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_THUMPER, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_BUGBAIT, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PHYSICS_DANGER, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_DANGER_SNIPERONLY, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_MOVE_AWAY, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_PLAYER_VEHICLE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_LOW, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_MEDIUM, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_READINESS_HIGH, "" );

	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_FROM_SNIPER, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_GUNFIRE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_MORTAR, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_COMBINE_ONLY, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_REACT_TO_SOURCE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_EXPLOSION, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_EXCLUDE_COMBINE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_DANGER_APPROACH, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_ALLIES_ONLY, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_PLAYER_VEHICLE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUND_CONTEXT_OWNER_ALLIES, "" );

	ScriptRegisterConstant( g_pScriptVM, ALL_CONTEXTS, "" );
	ScriptRegisterConstant( g_pScriptVM, ALL_SCENTS, "" );
	ScriptRegisterConstant( g_pScriptVM, ALL_SOUNDS, "" );

	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_UNSPECIFIED, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATING, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATED_DANGER, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_REPEATED_PHYSICS_DANGER, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_WEAPON, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_INJURY, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_BULLET_IMPACT, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_NPC_FOOTSTEP, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_SPOOKY_NOISE, "" );
	ScriptRegisterConstant( g_pScriptVM, SOUNDENT_CHANNEL_ZOMBINE_GRENADE, "" );

	// 
	// Capabilities
	// 
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_GROUND, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_JUMP, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_FLY, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_CLIMB, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_SWIM, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_CRAWL, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MOVE_SHOOT, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SKIP_NAV_GROUND_CHECK, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE, "" );
	//ScriptRegisterConstant( g_pScriptVM, bits_CAP_HEAR, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_AUTO_DOORS, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_OPEN_DOORS, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_TURN_HEAD, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_RANGE_ATTACK1, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_RANGE_ATTACK2, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_MELEE_ATTACK1, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_WEAPON_MELEE_ATTACK2, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_RANGE_ATTACK1, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_RANGE_ATTACK2, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_MELEE_ATTACK1, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_INNATE_MELEE_ATTACK2, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE_WEAPONS, "" );
	//ScriptRegisterConstant( g_pScriptVM, bits_CAP_STRAFE, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_ANIMATEDFACE, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_USE_SHOT_REGULATOR, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_FRIENDLY_DMG_IMMUNE, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SQUAD, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_DUCK, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_NO_HIT_PLAYER, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_AIM_GUN, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_NO_HIT_SQUADMATES, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_SIMPLE_RADIUS_DAMAGE, "" );

	ScriptRegisterConstant( g_pScriptVM, bits_CAP_DOORS_GROUP, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_RANGE_ATTACK_GROUP, "" );
	ScriptRegisterConstant( g_pScriptVM, bits_CAP_MELEE_ATTACK_GROUP, "" );
#endif
}
