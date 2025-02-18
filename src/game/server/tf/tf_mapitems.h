//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_MAPITEMS_H
#define TF_MAPITEMS_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_shareddefs.h"


class CTFPlayer;


/*==================================================*/
/* CTF Support defines 								*/
/*==================================================*/
#define CTF_FLAG1 		1
#define CTF_FLAG2 		2
#define CTF_DROPOFF1 	3
#define CTF_DROPOFF2 	4
#define CTF_SCORE1   	5
#define CTF_SCORE2   	6


// Defines for GoalItem Removing from Player Methods
#define GI_DROP_PLAYERDEATH	  0		// Dropped by a dying player
#define GI_DROP_REMOVEGOAL	  1		// Removed by a Goal
#define GI_DROP_PLAYERDROP	  2		// Dropped by a player


// Defines for methods of GoalItem returning
#define GI_RET_DROP_DEAD 	0		// Dropped by a dead player
#define GI_RET_DROP_LIVING 	1		// Dropped by a living player
#define GI_RET_GOAL			2		// Returned by a Goal
#define GI_RET_TIME			3		// Returned due to timeout


// Defines for Goal States
#define TFGS_ACTIVE		1 
#define TFGS_INACTIVE	2 
#define TFGS_REMOVED	3 
#define TFGS_DELAYED	4


// Defines for Goal Result types : goal_result
#define TFGR_SINGLE				1  // Goal can only be activated once
#define TFGR_ADD_BONUSES		2 	// Any Goals activated by this one give their bonuses
#define TFGR_ENDGAME			4 	// Goal fires Intermission, displays scores, and ends level
#define TFGR_NO_ITEM_RESULTS	8	// GoalItems given by this Goal don't do results
#define TFGR_REMOVE_DISGUISE	16 // Prevent/Remove undercover from any Spy
#define TFGR_FORCE_RESPAWN		32 // Forces the player to teleport to a respawn point
#define TFGR_DESTROY_BUILDINGS	64 // Destroys this player's buildings, if anys


// Defines for Goal Item types, : goal_activation (in items)
#define TFGI_GLOW			1   // Players carrying this GoalItem will glow
#define TFGI_SLOW			2   // Players carrying this GoalItem will move at half-speed
#define TFGI_DROP			4   // Players dying with this item will drop it
#define TFGI_RETURN_DROP	8   // Return if a player with it dies
#define TFGI_RETURN_GOAL	16  // Return if a player with it has it removed by a goal's activation
#define TFGI_RETURN_REMOVE	32  // Return if it is removed by TFGI_REMOVE
#define TFGI_REVERSE_AP		64  // Only pickup if the player _doesn't_ match AP Details
#define TFGI_REMOVE			128 // Remove if left untouched for 2 minutes after being dropped
#define TFGI_KEEP			256 // Players keep this item even when they die
#define TFGI_ITEMGLOWS		512	// Item glows when on the ground
#define TFGI_DONTREMOVERES	1024 // Don't remove results when the item is removed
#define TFGI_DROPTOGROUND	2048 // Drop To Ground when spawning
#define TFGI_CANBEDROPPED	4096 // Can be voluntarily dropped by players
#define TFGI_SOLID			8192 // Is solid... blocks bullets, etc


// For all these defines, see the tfortmap.txt that came with the zip
// for complete descriptions.
// Defines for Goal Activation types : goal_activation (in goals)
#define TFGA_TOUCH			1  // Activated when touched
#define TFGA_TOUCH_DETPACK	2  // Activated when touched by a detpack explosion
#define TFGA_REVERSE_AP		4  // Activated when AP details are _not_ met
#define TFGA_SPANNER		8  // Activated when hit by an engineer's spanner
#define TFGA_DROPTOGROUND	2048 // Drop to Ground when spawning


// Defines for Goal Effects types : goal_effect
#define TFGE_AP				  1  // AP is affected. Default.
#define TFGE_AP_TEAM		  2  // All of the AP's team.
#define TFGE_NOT_AP_TEAM	  4  // All except AP's team.
#define TFGE_NOT_AP			  8  // All except AP.
#define TFGE_WALL			  16 // If set, walls stop the Radius effects
#define TFGE_SAME_ENVIRONMENT 32 // If set, players in a different environment to the Goal are not affected
#define TFGE_TIMER_CHECK_AP	  64 // If set, Timer Goals check their critera for all players fitting their effects

#if 0

class CTFBaseItem : public CBaseAnimating
{
public:

	bool	CheckExistence();


public:

	int group_no;
	int goal_no;
	int goal_state;		// TFGS_
	// Goal/Timer/GoalItem/Trigger existence checking
	int		ex_skill_min;			// Exists when the skill is >= this value
	int		ex_skill_max;			// Exists when the skill is <= this value
	string_t teamcheck;			 // TeamCheck entity that should be checked
};


class CTFGoal : public CTFBaseItem
{
public:
	DECLARE_CLASS( CTFGoal, CBaseAnimating );
	DECLARE_DATADESC();

	void	Spawn( void );
	void	StartGoal( void );
	void	PlaceGoal( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	tfgoal_touch( CBaseEntity *pOther );

	void	DelayedResult();

	Class_T	Classify ( void ) { return CLASS_TFGOAL; }

	void	SetObjectCollisionBox( void );

	void tfgoal_timer_tick();
	void DoRespawn();


public:
	//TFTODO: lots of these variables need to be put in the FGD file.
	int goal_effects;	// TFGE_
	int goal_result;	// TFGR_

	int playerclass; // One of the PC_ defines.

	float	t_length;			// Goal Criteria radius check

	// NOTE: In CTFGoal, these are overridden to mean: if they're not zero, then this goal only
	// affects players...
	int maxammo_shells;	// ... with that team number
	int maxammo_nails;	// ... without that team number

	int ammo_shells;
	int ammo_nails;
	int ammo_rockets;
	int ammo_cells;
	int ammo_medikit;
	int ammo_detpack;
	int no_grenades_1;
	int no_grenades_2;

	// Item Displaying details
	int		 display_item_status[4]; // Goal displays the status of these items
	string_t team_str_home;				// Displayed when the item is at home base
	string_t team_str_moved;		  // Displayed when the item has been moved
	string_t team_str_carried;     // Displayed when the item is being carried
	string_t non_team_str_home;			// Displayed when the item is at home base
	string_t non_team_str_moved;	  // Displayed when the item has been moved
	string_t non_team_str_carried; // Displayed when the item is being carried

	float invincible_finished;
	float invisible_finished;
	float super_damage_finished;
	float radsuit_finished;

	int lives;
	int frags;
	float	wait;

	float	search_time;		// Timer goal delay
	int	    item_list;			// Used to keep track of which goalitems are 
								// affecting the player at any time.
								// GoalItems use it to keep track of their own 
								// mask to apply to a player's item_list

	float	drop_time;		// Time spent untouched before item return
	float		armortype;
	int			armorvalue;
	int			armorclass;			// Type of armor being worn;

	int		count;			// Change teamscores
	
	// Goal Size
	Vector  goal_min;
	Vector  goal_max;

	bool m_bAddBonuses;
	
	int items;
	int items_allowed;
	
	int else_goal;
	int if_goal_is_active;
	int if_goal_is_inactive;
	int if_goal_is_removed;
	int if_group_is_active;
	int if_group_is_inactive;
	int if_group_is_removed;

	int		speed_reduction;
	
	int return_item_no;
	int if_item_has_moved;
	int if_item_hasnt_moved;
	
	int has_item_from_group;
	int hasnt_item_from_group;
	
	int goal_activation;
	int delay_time;
	int weapon;
	string_t owned_by_teamcheck;
	int owned_by;
	string_t noise;
	
	// Spawnpoint behaviour
	int     remove_spawnpoint;
	int     restore_spawnpoint;
	int     remove_spawngroup;
	int     restore_spawngroup;

	// These are the old centerprinting methods.
	// They now print using the large fancy text
	string_t broadcast;						// Centerprinted to all, overridden by the next two
	string_t team_broadcast;				// Centerprinted to AP's team members, but not the AP
	string_t non_team_broadcast; 			// Centerprinted to non AP's team members
	string_t owners_team_broadcast;			// Centerprinted to the members of the team that own the Goal/Item
	string_t non_owners_team_broadcast;		// Centerprinted to the members of the team that don't own the Goal/Item
	string_t team_drop;						// Centerprinted to item owners team
	string_t non_team_drop;					// Centerprinted to everone not on item owners team

	// These are new fields that print the old fashioned centerprint method
	string_t org_broadcast;						// Centerprinted to all, overridden by the next two
	string_t org_team_broadcast;				// Centerprinted to AP's team members, but not the AP
	string_t org_non_team_broadcast; 			// Centerprinted to non AP's team members
	string_t org_owners_team_broadcast;			// Centerprinted to the members of the team that own the Goal/Item
	string_t org_non_owners_team_broadcast;		// Centerprinted to the members of the team that don't own the Goal/Item
	string_t org_team_drop;						// Centerprinted to item owners team
	string_t org_non_team_drop;					// Centerprinted to everone not on item owners team
	string_t org_message;						// Centerprinted to the AP upon activation
	string_t org_noise3;
	string_t org_noise4;
	// These still print the old centerprint fashion
	string_t netname_broadcast;				// same as above, prepended by AP netname and bprinted
	string_t netname_team_broadcast;			// same as above, prepended by AP netname and bprinted
	string_t netname_non_team_broadcast; 	// same as above, prepended by AP netname and bprinted
	string_t netname_owners_team_broadcast;  // same as above, prepended by AP netname and bprinted
	string_t netname_team_drop;			// same as above, prepended by AP netname and bprinted
	string_t netname_non_team_drop;  // same as above, prepended by AP netname and bprinted
	string_t speak;							// VOX Spoken to Everyone
	string_t AP_speak;						// VOX Spoken the AP
	string_t team_speak;					// VOX Spoken to AP's team_members, including the AP
	string_t non_team_speak; 				// VOX Spoken to non AP's team_members
	string_t owners_team_speak;				// VOX Spoken to members of the team that own this Goal
	string_t non_owners_team_speak;			// VOX Spoken to everyone bit the members of the team that own this Goal

	float	 m_flEndRoundTime;
	string_t m_iszEndRoundMsg_Team1_Win;
	string_t m_iszEndRoundMsg_Team2_Win;
	string_t m_iszEndRoundMsg_Team3_Win;
	string_t m_iszEndRoundMsg_Team4_Win;
	string_t m_iszEndRoundMsg_Team1_Lose;
	string_t m_iszEndRoundMsg_Team2_Lose;
	string_t m_iszEndRoundMsg_Team3_Lose;
	string_t m_iszEndRoundMsg_Team4_Lose;
	string_t m_iszEndRoundMsg_Team1;
	string_t m_iszEndRoundMsg_Team2;
	string_t m_iszEndRoundMsg_Team3;
	string_t m_iszEndRoundMsg_Team4;
	string_t m_iszEndRoundMsg_OwnedBy;
	string_t m_iszEndRoundMsg_NonOwnedBy;

	int 	all_active;
	int		last_impulse;		// The previous impulse command from this player

	int	    activate_goal_no;
	int	    inactivate_goal_no;
	int	    remove_goal_no;
	int	    restore_goal_no;
	int	    activate_group_no;
	int	    inactivate_group_no;
	int	    remove_group_no;
	int	    restore_group_no;

	BOOL    do_triggerwork;		// Overrides for trigger handling in TF Goals
	string_t killtarget;	// Remove ents with this target
	string_t	target;	

	string_t	message;

	// Score increases
	int		increase_team[4];			// Increase the scores of teams
	int		increase_team_owned_by;		// Increase the score of the team that owns this entity

	EHANDLE enemy;

	Vector oldorigin;
	int		axhitme;		// Remove item from AP

	int remove_item_group;
};
 

class CTFGoalItem : public CTFGoal
{
public:
	void	Spawn( void );
	void	StartItem( void );
	void	PlaceItem( void );

	Class_T	Classify ( void ) { return CLASS_TFGOAL_ITEM; }

	void item_tfgoal_touch( CBaseEntity *pOther );
	void tfgoalitem_droptouch();
	void tfgoalitem_dropthink();
	void tfgoalitem_remove();

	void DoDrop( Vector vecOrigin );


public:

	float m_flDroppedAt;

	float speed;
	int speed_reduction;

	float distance;
	float pain_finished;
	float attack_finished;

	Vector	redrop_origin;	// Original drop position
	int		redrop_count;	// Number of time's we redropped.
};


class CTFTimerGoal : public CTFGoal
{
public:
	void	Spawn( void );

	Class_T	Classify ( void ) { return CLASS_TFGOAL_TIMER; }
};


#if 0
class CTFSpawn : public CTFBaseItem
{
public:
	void	Spawn( void );
	void	Activate( void );
	Class_T	Classify ( void ) { return CLASS_TFSPAWN; }
	BOOL	CheckTeam( int iTeamNo );

	EHANDLE m_pTeamCheck;
};
#endif


class CBaseDelay : public CTFGoal
{
public:
	DECLARE_CLASS( CBaseDelay, CTFGoal );

	void SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
	void DelayThink( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

public:
	float m_flDelay;
	string_t m_iszKillTarget;
	int button;
};


class CTeamCheck : public CBaseDelay
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	BOOL TeamMatches( int iTeam );
};


// Global functions.
CTFGoalItem* Finditem(int ino);
void DisplayItemStatus(CTFGoal *Goal, CTFPlayer *Player, CTFGoalItem *Item);

#endif

#endif // TF_MAPITEMS_H
