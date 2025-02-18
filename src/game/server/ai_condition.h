//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		The default shared conditions
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	CONDITION_H
#define	CONDITION_H

// NOTE: Changing this constant will break save files!!! (changes type of CAI_ScheduleBits)
#ifndef MAX_CONDITIONS
#define	MAX_CONDITIONS 32*8
#endif

//=========================================================
// These are the default shared conditions
//=========================================================
enum SCOND_t 
{
	COND_NONE,				// A way for a function to return no condition to get
	
	COND_IN_PVS,
	COND_IDLE_INTERRUPT,	// The schedule in question is a low priority idle, and therefore a candidate for translation into something else
	
	COND_LOW_PRIMARY_AMMO,
	COND_NO_PRIMARY_AMMO,
	COND_NO_SECONDARY_AMMO,
	COND_NO_WEAPON,
	COND_SEE_HATE,
	COND_SEE_FEAR,
	COND_SEE_DISLIKE,
	COND_SEE_ENEMY,
	COND_LOST_ENEMY,
	COND_ENEMY_WENT_NULL,	// What most people think COND_LOST_ENEMY is: This condition is set in the edge case where you had an enemy last think, but don't have one this think.
	COND_ENEMY_OCCLUDED,	// Can't see m_hEnemy
	COND_TARGET_OCCLUDED,	// Can't see m_hTargetEnt
	COND_HAVE_ENEMY_LOS,
	COND_HAVE_TARGET_LOS,
	COND_LIGHT_DAMAGE,
	COND_HEAVY_DAMAGE,
	COND_PHYSICS_DAMAGE,
	COND_REPEATED_DAMAGE,	//  Damaged several times in a row

	COND_CAN_RANGE_ATTACK1,	// Hitscan weapon only
	COND_CAN_RANGE_ATTACK2,	// Grenade weapon only
	COND_CAN_MELEE_ATTACK1,
	COND_CAN_MELEE_ATTACK2,

	COND_PROVOKED,
	COND_NEW_ENEMY,

	COND_ENEMY_TOO_FAR,		//	Can we get rid of this one!?!?
	COND_ENEMY_FACING_ME,
	COND_BEHIND_ENEMY,
	COND_ENEMY_DEAD,
	COND_ENEMY_UNREACHABLE,	// Not connected to me via node graph

	COND_SEE_PLAYER,
	COND_LOST_PLAYER,
	COND_SEE_NEMESIS,
	COND_TASK_FAILED,
	COND_SCHEDULE_DONE,
	COND_SMELL,
	COND_TOO_CLOSE_TO_ATTACK, // FIXME: most of this next group are meaningless since they're shared between all attack checks!
	COND_TOO_FAR_TO_ATTACK,
	COND_NOT_FACING_ATTACK,
	COND_WEAPON_HAS_LOS,
	COND_WEAPON_BLOCKED_BY_FRIEND,	// Friend between weapon and target
	COND_WEAPON_PLAYER_IN_SPREAD,	// Player in shooting direction
	COND_WEAPON_PLAYER_NEAR_TARGET,	// Player near shooting position
	COND_WEAPON_SIGHT_OCCLUDED,
	COND_BETTER_WEAPON_AVAILABLE,
	COND_HEALTH_ITEM_AVAILABLE,		// There's a healthkit available.
	COND_GIVE_WAY,					// Another npc requested that I give way
	COND_WAY_CLEAR,					// I no longer have to give way
	COND_HEAR_DANGER,
	COND_HEAR_THUMPER,
	COND_HEAR_BUGBAIT,
	COND_HEAR_COMBAT,
	COND_HEAR_WORLD,
	COND_HEAR_PLAYER,
	COND_HEAR_BULLET_IMPACT,
	COND_HEAR_PHYSICS_DANGER,
	COND_HEAR_MOVE_AWAY,
	COND_HEAR_SPOOKY,				// Zombies make this when Alyx is in darkness mode

	COND_NO_HEAR_DANGER,			// Since we can't use ~CONDITION. Mutually exclusive with COND_HEAR_DANGER

	COND_FLOATING_OFF_GROUND,

	COND_MOBBED_BY_ENEMIES,			// Surrounded by a large number of enemies melee attacking me. (Zombies or Antlions, usually).

	// Commander stuff
	COND_RECEIVED_ORDERS,
	COND_PLAYER_ADDED_TO_SQUAD,
	COND_PLAYER_REMOVED_FROM_SQUAD,

	COND_PLAYER_PUSHING,
	COND_NPC_FREEZE,				// We received an npc_freeze command while we were unfrozen
	COND_NPC_UNFREEZE,				// We received an npc_freeze command while we were frozen

	// This is a talker condition, but done here because we need to handle it in base AI
	// due to it's interaction with behaviors.
	COND_TALKER_RESPOND_TO_QUESTION,
	
	COND_NO_CUSTOM_INTERRUPTS,		// Don't call BuildScheduleTestBits for this schedule. Used for schedules that must strictly control their interruptibility.

	// ======================================
	// IMPORTANT: This must be the last enum
	// ======================================
	LAST_SHARED_CONDITION	
};

#endif	//CONDITION_H
