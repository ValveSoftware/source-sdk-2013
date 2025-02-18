//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the tasks for default AI.
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_TASK_H
#define AI_TASK_H
#ifdef _WIN32
#pragma once
#endif

class CStringRegistry;

// ----------------------------------------------------------------------
// Failure messages
//
// UNDONE: do this diffently so when not in developer mode we can 
//		   not use any memory for these text strings
// ----------------------------------------------------------------------

// Codes are either one of the enumerated types below, or a string (similar to Windows resource IDs)
typedef intp AI_TaskFailureCode_t;

enum AI_BaseTaskFailureCodes_t : intp
{
	NO_TASK_FAILURE,
	FAIL_NO_TARGET,
	FAIL_WEAPON_OWNED,
	FAIL_ITEM_NO_FIND,
	FAIL_NO_HINT_NODE,
	FAIL_SCHEDULE_NOT_FOUND,
	FAIL_NO_ENEMY,
	FAIL_NO_BACKAWAY_NODE,
	FAIL_NO_COVER,
	FAIL_NO_FLANK,
	FAIL_NO_SHOOT,
	FAIL_NO_ROUTE,
	FAIL_NO_ROUTE_GOAL,
	FAIL_NO_ROUTE_BLOCKED,
	FAIL_NO_ROUTE_ILLEGAL,
	FAIL_NO_WALK,
	FAIL_ALREADY_LOCKED,
	FAIL_NO_SOUND,
	FAIL_NO_SCENT,
	FAIL_BAD_ACTIVITY,
	FAIL_NO_GOAL,
	FAIL_NO_PLAYER,
	FAIL_NO_REACHABLE_NODE,
	FAIL_NO_AI_NETWORK,
	FAIL_BAD_POSITION,
	FAIL_BAD_PATH_GOAL,
	FAIL_STUCK_ONTOP,
	FAIL_ITEM_TAKEN,

	NUM_FAIL_CODES,
};

inline bool IsPathTaskFailure( AI_TaskFailureCode_t code )
{
	return ( code >= FAIL_NO_ROUTE && code <= FAIL_NO_ROUTE_ILLEGAL );
}

const char *TaskFailureToString( AI_TaskFailureCode_t code );
inline AI_TaskFailureCode_t MakeFailCode( const char *pszGeneralError ) { return (AI_TaskFailureCode_t)(intp)pszGeneralError; }


enum TaskStatus_e 
{
	TASKSTATUS_NEW =			 	0,			// Just started
	TASKSTATUS_RUN_MOVE_AND_TASK =	1,			// Running task & movement
	TASKSTATUS_RUN_MOVE	=			2,			// Just running movement
	TASKSTATUS_RUN_TASK	=			3,			// Just running task
	TASKSTATUS_COMPLETE	=			4,			// Completed, get next task
};

// an array of tasks is a task list
// an array of schedules is a schedule list
struct Task_t
{
	int		iTask;
	float	flTaskData;
};

//=========================================================
// These are the shared tasks
//=========================================================
enum sharedtasks_e
{
		TASK_INVALID = 0,
		
		// Forces the activity to reset.
		TASK_RESET_ACTIVITY,

		// Waits for the specified number of seconds.
		TASK_WAIT,					

		// Make announce attack sound
		TASK_ANNOUNCE_ATTACK,

		// Waits for the specified number of seconds. Will constantly turn to 
		// face the enemy while waiting. 
		TASK_WAIT_FACE_ENEMY,

		// Waits up to the specified number of seconds. Will constantly turn to 
		// face the enemy while waiting. 
		TASK_WAIT_FACE_ENEMY_RANDOM,

		// Wait until the player enters the same PVS as this character.
		TASK_WAIT_PVS,

		// DON'T use this, it needs to go away. 
		TASK_SUGGEST_STATE,

		// Set m_hTargetEnt to nearest player
		TASK_TARGET_PLAYER,

		// Walk to m_hTargetEnt's location
		TASK_SCRIPT_WALK_TO_TARGET,

		// Run to m_hTargetEnt's location
		TASK_SCRIPT_RUN_TO_TARGET,

		// Move to m_hTargetEnt's location using the activity specified by m_hCine->m_iszCustomMove.
		TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET,

		// Move to within specified range of m_hTargetEnt
		TASK_MOVE_TO_TARGET_RANGE,

		// Move to within specified range of our nav goal
		TASK_MOVE_TO_GOAL_RANGE,

		// Path that moves the character a few steps forward of where it is.
		TASK_MOVE_AWAY_PATH,

		TASK_GET_PATH_AWAY_FROM_BEST_SOUND,

		// Set the implied goal for TASK_GET_PATH_TO_GOAL
		TASK_SET_GOAL,

		// Get the path to the goal specified by TASK_SET_GOAL
		TASK_GET_PATH_TO_GOAL,

		// Path to the enemy's location. Even if the enemy is unseen!
		TASK_GET_PATH_TO_ENEMY,
		
		// Path to the last place this character saw the enemy
		TASK_GET_PATH_TO_ENEMY_LKP,

		// Path to the enemy's location or path to a LOS with the enemy's last known position, depending on range
		TASK_GET_CHASE_PATH_TO_ENEMY,

		// Path to a LOS with the enemy's last known position
		TASK_GET_PATH_TO_ENEMY_LKP_LOS,

		// Path to the dead enemy's carcass.
		TASK_GET_PATH_TO_ENEMY_CORPSE,

		// Path to the player's origin
		TASK_GET_PATH_TO_PLAYER,

		// Path to node with line of sight to enemy
		TASK_GET_PATH_TO_ENEMY_LOS,

		// Path to node with line of sight to enemy, at least flTaskData units away from m_vSavePosition
		TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS,

		// Path to node with line of sight to enemy, at least flTaskData degrees away from m_vSavePosition from the enemy's POV
		TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS,

		// Path to the within shot range of last place this character saw the enemy
		TASK_GET_PATH_TO_RANGE_ENEMY_LKP_LOS,

		// Build a path to m_hTargetEnt
		TASK_GET_PATH_TO_TARGET,

		// Allow a little slop, and allow for some Z offset (like the target is a gun on a table).
		TASK_GET_PATH_TO_TARGET_WEAPON,

		TASK_CREATE_PENDING_WEAPON,

		// Path to nodes[ m_pHintNode ]
		TASK_GET_PATH_TO_HINTNODE,

		// Store current position for later reference
		TASK_STORE_LASTPOSITION,

		// Clear stored position
		TASK_CLEAR_LASTPOSITION,

		// Store current position for later reference
		TASK_STORE_POSITION_IN_SAVEPOSITION,

		// Store best sound position for later reference
		TASK_STORE_BESTSOUND_IN_SAVEPOSITION,
		TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION,

		TASK_REACT_TO_COMBAT_SOUND,

		// Store current enemy position in saveposition
		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION,

		// Move to the goal specified by the player in command mode.
		TASK_GET_PATH_TO_COMMAND_GOAL,

		TASK_MARK_COMMAND_GOAL_POS,

		TASK_CLEAR_COMMAND_GOAL,

		// Path to last position (Last position must be stored with TASK_STORE_LAST_POSITION)
		TASK_GET_PATH_TO_LASTPOSITION,

		// Path to saved position (Save position must by set in code or by a task)
		TASK_GET_PATH_TO_SAVEPOSITION,

		// Path to location that has line of sight to saved position (Save position must by set in code or by a task)
		TASK_GET_PATH_TO_SAVEPOSITION_LOS,

		// Path to random node
		TASK_GET_PATH_TO_RANDOM_NODE,

		// Path to source of loudest heard sound that I care about
		TASK_GET_PATH_TO_BESTSOUND,

		// Path to source of the strongest scend that I care about
		TASK_GET_PATH_TO_BESTSCENT,

		// Run the current path
		TASK_RUN_PATH,	

		// Walk the current path
		TASK_WALK_PATH,	

		// Walk the current path for a specified number of seconds
		TASK_WALK_PATH_TIMED,

		// Walk the current path until you are x units from the goal.
		TASK_WALK_PATH_WITHIN_DIST,

		// Walk the current path until for x units
		TASK_WALK_PATH_FOR_UNITS,

		// Rung the current path until you are x units from the goal.
		TASK_RUN_PATH_FLEE,

		// Run the current path for a specified number of seconds
		TASK_RUN_PATH_TIMED,

		// Run the current path until for x units
		TASK_RUN_PATH_FOR_UNITS,

		// Run the current path until you are x units from the goal.
		TASK_RUN_PATH_WITHIN_DIST,

		// Walk the current path sideways (must be supported by animation)
		TASK_STRAFE_PATH,

		// Clear m_flMoveWaitFinished (timer that inhibits movement)
		TASK_CLEAR_MOVE_WAIT,

		// Decide on the appropriate small flinch animation, and play it. 
		TASK_SMALL_FLINCH,

		// Decide on the appropriate big flinch animation, and play it. 
		TASK_BIG_FLINCH,

		// Prevent dodging for a certain amount of time.
		TASK_DEFER_DODGE,

		// Turn to face ideal yaw
		TASK_FACE_IDEAL,

		// Find an interesting direction to face. Don't face into walls, corners if you can help it.
		TASK_FACE_REASONABLE,

		// Turn to face the way I should walk or run
		TASK_FACE_PATH,

		// Turn to face a player
		TASK_FACE_PLAYER,

		// Turn to face the enemy
		TASK_FACE_ENEMY,

		// Turn to face nodes[ m_pHintNode ]
		TASK_FACE_HINTNODE,

		// Play activity associate with the current hint
		TASK_PLAY_HINT_ACTIVITY,

		// Turn to face m_hTargetEnt
		TASK_FACE_TARGET,

		// Turn to face stored last position (last position must be stored first!)
		TASK_FACE_LASTPOSITION,

		// Turn to face stored save position (save position must be stored first!)
		TASK_FACE_SAVEPOSITION,

		// Turn to face directly away from stored save position (save position must be stored first!)
		TASK_FACE_AWAY_FROM_SAVEPOSITION,

		// Set the current facing to be the ideal
		TASK_SET_IDEAL_YAW_TO_CURRENT,

		// Attack the enemy (should be facing the enemy)
		TASK_RANGE_ATTACK1,
		TASK_RANGE_ATTACK2,		
		TASK_MELEE_ATTACK1,		
		TASK_MELEE_ATTACK2,		

		// Reload weapon
		TASK_RELOAD,

		// Execute special attack (user-defined)
		TASK_SPECIAL_ATTACK1,
		TASK_SPECIAL_ATTACK2,

		TASK_FIND_HINTNODE,
		TASK_FIND_LOCK_HINTNODE,

		TASK_CLEAR_HINTNODE,

		// Claim m_pHintNode exclusively for this NPC.
		TASK_LOCK_HINTNODE,

		// Emit an angry sound
		TASK_SOUND_ANGRY,

		// Emit a dying sound
		TASK_SOUND_DEATH,

		// Emit an idle sound
		TASK_SOUND_IDLE,

		// Emit a sound because you are pissed off because you just saw someone you don't like
		TASK_SOUND_WAKE,

		// Emit a pain sound
		TASK_SOUND_PAIN,

		// Emit a death sound
		TASK_SOUND_DIE,

		// Speak a sentence
		TASK_SPEAK_SENTENCE,

		// Wait for the current sentence I'm speaking to finish
		TASK_WAIT_FOR_SPEAK_FINISH,

		// Set current animation activity to the specified activity
		TASK_SET_ACTIVITY,

		// Adjust the framerate to plus/minus N%
		TASK_RANDOMIZE_FRAMERATE,

		// Immediately change to a schedule of the specified type
		TASK_SET_SCHEDULE,

		// Set the specified schedule to execute if the current schedule fails.
		TASK_SET_FAIL_SCHEDULE,

		// How close to route goal do I need to get
		TASK_SET_TOLERANCE_DISTANCE,

		// How many seconds should I spend search for a route
		TASK_SET_ROUTE_SEARCH_TIME,

		// Return to use of default fail schedule
		TASK_CLEAR_FAIL_SCHEDULE,

		// Play the specified animation sequence before continuing
		TASK_PLAY_SEQUENCE,

		// Play the specified private animation sequence before continuing
		TASK_PLAY_PRIVATE_SEQUENCE,

		// Turn to face the enemy while playing specified animation sequence
		TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY,
		TASK_PLAY_SEQUENCE_FACE_ENEMY,
		TASK_PLAY_SEQUENCE_FACE_TARGET,

		// tries lateral cover first, then node cover
		TASK_FIND_COVER_FROM_BEST_SOUND,

		// tries lateral cover first, then node cover
		TASK_FIND_COVER_FROM_ENEMY,

		// Find a place to hide from the enemy, somewhere on either side of me
		TASK_FIND_LATERAL_COVER_FROM_ENEMY,

		// Find a place further from the saved position
		TASK_FIND_BACKAWAY_FROM_SAVEPOSITION,

		// Fine a place to hide from the enemy, anywhere. Use the node system.
		TASK_FIND_NODE_COVER_FROM_ENEMY,

		// Find a place to hide from the enemy that's within the specified distance
		TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY,

		// data for this one is there MINIMUM aceptable distance to the cover.
		TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,

		// Find a place to go that can't see to where I am now.
		TASK_FIND_COVER_FROM_ORIGIN,

		// Unhook from the AI system.
		TASK_DIE,

		// Wait until scripted sequence plays
		TASK_WAIT_FOR_SCRIPT,

		// Play scripted sequence animation
		TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY,
		TASK_PLAY_SCRIPT,
		TASK_PLAY_SCRIPT_POST_IDLE,
		TASK_ENABLE_SCRIPT,
		TASK_PLANT_ON_SCRIPT,
		TASK_FACE_SCRIPT,

		// Wait for scene to complete
		TASK_PLAY_SCENE,

		// Wait for 0 to specified number of seconds
		TASK_WAIT_RANDOM,

		// Wait forever (until this schedule is interrupted)
		TASK_WAIT_INDEFINITE,

		TASK_STOP_MOVING,
		
		// Turn left the specified number of degrees
		TASK_TURN_LEFT,

		// Turn right the specified number of degrees
		TASK_TURN_RIGHT,

		// Remember the specified piece of data
		TASK_REMEMBER,

		// Forget the specified piece of data
		TASK_FORGET,
		
		// Wait until current movement is complete. 
		TASK_WAIT_FOR_MOVEMENT,

		// Wait until a single-step movement is complete.
		TASK_WAIT_FOR_MOVEMENT_STEP,

		// Wait until I can't hear any danger sound.
		TASK_WAIT_UNTIL_NO_DANGER_SOUND,

		// Pick up new weapons:
		TASK_WEAPON_FIND,
		TASK_WEAPON_PICKUP,
		TASK_WEAPON_RUN_PATH,	// run to weapon but break if someone else picks it up
		TASK_WEAPON_CREATE,

		TASK_ITEM_PICKUP,
		TASK_ITEM_RUN_PATH,

		// Use small hull for tight navigation
		TASK_USE_SMALL_HULL,

		// wait until you are on ground
		TASK_FALL_TO_GROUND,

		// Wander for a specfied amound of time
		TASK_WANDER,

		TASK_FREEZE,

		// regather conditions at the start of a schedule (all conditions are cleared between schedules)
		TASK_GATHER_CONDITIONS,

		// Require an enemy be seen after the task is run to be considered a candidate enemy
		TASK_IGNORE_OLD_ENEMIES,
		
		TASK_DEBUG_BREAK,

		// Add a specified amount of health to this NPC
		TASK_ADD_HEALTH,

		// Add a gesture layer and wait until it's finished
		TASK_ADD_GESTURE_WAIT,

		// Add a gesture layer
		TASK_ADD_GESTURE,

		// Get a path to my forced interaction partner
		TASK_GET_PATH_TO_INTERACTION_PARTNER,
		
		// First task of all schedules for playing back scripted sequences
		TASK_PRE_SCRIPT,

		// ======================================
		// IMPORTANT: This must be the last enum
		// ======================================
		LAST_SHARED_TASK

};

#endif // AI_TASK_H
