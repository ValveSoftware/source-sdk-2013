//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_mini.cpp
// A NextBot non-player derived actor
// Michael Booth, March 2011

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "tf_projectile_arrow.h"
#include "tf_projectile_rocket.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_ammo_pack.h"
#include "tf_obj_sentrygun.h"
#include "nav_mesh/tf_nav_area.h"
#include "bot_npc_mini.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "CRagdollMagnet.h"
#include "nav_mesh/tf_path_follower.h"
#include "bot_npc_minion.h"
#include "player_vs_environment/monster_resource.h"


extern ConVar tf_bot_npc_reaction_time;
extern ConVar tf_bot_npc_grenade_interval;
extern float ModifyBossDamage( const CTakeDamageInfo &info );


ConVar tf_raid_mini_rocket_boss_health( "tf_raid_mini_rocket_boss_health", "5000", 0/*FCVAR_CHEAT*/ );


//-----------------------------------------------------------------------------------------------------
// The Bot NPC mini-boss
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( bot_boss_mini_rockets, CBotNPCMiniRockets );
PRECACHE_REGISTER( bot_boss_mini_rockets );


//-----------------------------------------------------------------------------------------------------
void CBotNPCMiniRockets::Precache()
{
	BaseClass::Precache();

	int model = PrecacheModel( "models/bots/knight/knight_mini.mdl" );
	PrecacheGibsForModel( model );

	PrecacheScriptSound( "RobotMiniBoss.LaunchRocket" );
}


//-----------------------------------------------------------------------------------------------------
void CBotNPCMiniRockets::Spawn( void )
{
	BaseClass::Spawn();

	SetModel( "models/bots/knight/knight_mini.mdl" );

	int health = tf_raid_mini_rocket_boss_health.GetInt();
	SetHealth( health );
	SetMaxHealth( health );
}


//-----------------------------------------------------------------------------------------------------
// The Bot NPC mini-boss
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( bot_boss_mini_nuker, CBotNPCMiniNuker );
PRECACHE_REGISTER( bot_boss_mini_nuker );

ConVar tf_raid_mini_nuker_boss_health( "tf_raid_mini_nuker_boss_health", "5000", 0/*FCVAR_CHEAT*/ );


//-----------------------------------------------------------------------------------------------------
void CBotNPCMiniNuker::Precache()
{
	BaseClass::Precache();

	int model = PrecacheModel( "models/bots/knight/knight_mini.mdl" );
	PrecacheGibsForModel( model );
}


//-----------------------------------------------------------------------------------------------------
void CBotNPCMiniNuker::Spawn( void )
{
	BaseClass::Spawn();

	SetModel( "models/bots/knight/knight_mini.mdl" );

	int health = tf_raid_mini_nuker_boss_health.GetInt();
	SetHealth( health );
	SetMaxHealth( health );
}

#endif // TF_RAID_MODE
