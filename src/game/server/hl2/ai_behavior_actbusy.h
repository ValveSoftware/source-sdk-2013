//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_BEHAVIOR_ACTBUSY_H
#define AI_BEHAVIOR_ACTBUSY_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"
#include "ai_goalentity.h"

//-----------------------------------------------------------------------------
enum
{
	ACTBUSY_TYPE_DEFAULT = 0,
	ACTBUSY_TYPE_COMBAT,
};

enum busyinterrupt_t
{
	BA_INT_NONE,		// Nothing breaks us out of this
	BA_INT_DANGER,		// Only danger signals interrupts this busy anim. The player will be ignored.
	BA_INT_PLAYER,		// The Player's presence interrupts this busy anim
	BA_INT_AMBUSH,		// We're waiting to ambush enemies. Don't break on danger sounds in front of us.
	BA_INT_COMBAT,		// Only break out if we're shot at.
	BA_INT_ZOMBIESLUMP,	// Zombies who are slumped on the ground.
	BA_INT_SIEGE_DEFENSE,
};

enum busyanimparts_t
{
	BA_BUSY,
	BA_ENTRY,
	BA_EXIT,

	BA_MAX_ANIMS,
};

struct busyanim_t
{
	string_t			iszName;
	Activity			iActivities[BA_MAX_ANIMS];
	string_t			iszSequences[BA_MAX_ANIMS];
	string_t			iszSounds[BA_MAX_ANIMS];
	float				flMinTime;		// Min time spent in this busy animation
	float				flMaxTime;		// Max time spent in this busy animation. 0 means continue until interrupted.
	busyinterrupt_t		iBusyInterruptType;
	bool				bUseAutomovement;
};

struct busysafezone_t
{
	Vector	vecMins;
	Vector	vecMaxs;
};

#define NO_MAX_TIME -1

class CAI_ActBusyGoal;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAI_ActBusyBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_ActBusyBehavior, CAI_SimpleBehavior );
public:
	DECLARE_DATADESC();
	CAI_ActBusyBehavior();

	enum
	{
		// Schedules
		SCHED_ACTBUSY_START_BUSYING = BaseClass::NEXT_SCHEDULE,
		SCHED_ACTBUSY_BUSY,
		SCHED_ACTBUSY_STOP_BUSYING,
		SCHED_ACTBUSY_LEAVE,
		SCHED_ACTBUSY_TELEPORT_TO_BUSY,
		NEXT_SCHEDULE,
		
		// Tasks
		TASK_ACTBUSY_PLAY_BUSY_ANIM = BaseClass::NEXT_TASK,
		TASK_ACTBUSY_PLAY_ENTRY,
		TASK_ACTBUSY_PLAY_EXIT,
		TASK_ACTBUSY_TELEPORT_TO_BUSY,
		TASK_ACTBUSY_WALK_PATH_TO_BUSY,
		TASK_ACTBUSY_GET_PATH_TO_ACTBUSY,
		TASK_ACTBUSY_VERIFY_EXIT,
		NEXT_TASK,
		
		// Conditions
		COND_ACTBUSY_LOST_SEE_ENTITY = BaseClass::NEXT_CONDITION,
		COND_ACTBUSY_AWARE_OF_ENEMY_IN_SAFE_ZONE,
		COND_ACTBUSY_ENEMY_TOO_CLOSE,
		NEXT_CONDITION,
	};
	
	virtual const char *GetName() {	return "ActBusy"; }

	void	Enable( CAI_ActBusyGoal *pGoal, float flRange, bool bVisibleOnly );
	void	OnRestore();
	void	SetBusySearchRange( float flRange );
	void	Disable( void );
	void	ForceActBusy( CAI_ActBusyGoal *pGoal, CAI_Hint *pHintNode = NULL, float flMaxTime = NO_MAX_TIME, bool bVisibleOnly = false, bool bTeleportToBusy = false, bool bUseNearestBusy = false, CBaseEntity *pSeeEntity = NULL, Activity activity = ACT_INVALID );
	void	ForceActBusyLeave( bool bVisibleOnly = false );
	void	StopBusying( void );
	bool	IsStopBusying();
	CAI_Hint *FindActBusyHintNode( void );
	CAI_Hint *FindCombatActBusyHintNode( void );
	CAI_Hint *FindCombatActBusyTeleportHintNode( void );
	bool	CanSelectSchedule( void );
	bool	IsCurScheduleOverridable( void );
	bool	ShouldIgnoreSound( CSound *pSound );
	void	OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	GatherConditions( void );
	void	BuildScheduleTestBits( void );
	void	EndScheduleSelection( void );
	Activity NPC_TranslateActivity( Activity nActivity );
	void	HandleAnimEvent( animevent_t *pEvent );
	void	CheckAndCleanupOnExit( void );
	bool	FValidateHintType( CAI_Hint *pHint );
	bool	ActBusyNodeStillActive( void );
	bool	IsMovingToBusy( void ) { return m_bMovingToBusy; }
	bool	IsEnabled( void ) { return m_bEnabled; }
	float	GetReasonableFacingDist( void ) { return 0; }	// Actbusy ignores reasonable facing
	bool	IsInterruptable( void );
	bool	ShouldPlayerAvoid( void );
	void	SetUseRenderBounds( bool bUseBounds ) { m_bUseRenderBoundsForCollision = bUseBounds; }
	void	ComputeAndSetRenderBounds();
	bool	CanFlinch( void );
	bool	CanRunAScriptedNPCInteraction( bool bForced );
	void	OnScheduleChange();
	bool	QueryHearSound( CSound *pSound );
	void	OnSeeEntity( CBaseEntity *pEntity );
	bool	NeedsToPlayExitAnim() { return m_bNeedsToPlayExitAnim; }

	// Returns true if the current NPC is acting busy, or moving to an actbusy
	bool	IsActive( void );
	// Returns true if the current NPC is actually acting busy (i.e. inside an act busy anim)
	bool	IsInsideActBusy( void ) { return m_bBusy; }

	// Combat act busy stuff
	bool	IsCombatActBusy();
	void 	CollectSafeZoneVolumes( CAI_ActBusyGoal *pActBusyGoal );
	bool	IsInSafeZone( CBaseEntity *pEntity );
	int		CountEnemiesInSafeZone();

private:
	virtual int		SelectSchedule( void );
	int				SelectScheduleForLeaving( void );
	int				SelectScheduleWhileNotBusy( int iBase );
	int				SelectScheduleWhileBusy( void );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	void			NotifyBusyEnding( void );
	bool			HasAnimForActBusy( int iActBusy, busyanimparts_t AnimPart );
	bool			PlayAnimForActBusy( busyanimparts_t AnimPart );
	void			PlaySoundForActBusy( busyanimparts_t AnimPart ); 

private:
	bool			m_bEnabled;
	bool			m_bForceActBusy;
	Activity		m_ForcedActivity;
	bool			m_bTeleportToBusy;
	bool			m_bUseNearestBusy;
	bool			m_bLeaving;
	bool			m_bVisibleOnly;
	bool			m_bUseRenderBoundsForCollision;
	float			m_flForcedMaxTime;
	bool			m_bBusy;
	bool			m_bMovingToBusy;
	bool			m_bNeedsToPlayExitAnim;
	float			m_flNextBusySearchTime;	
	float			m_flEndBusyAt;
	float			m_flBusySearchRange;
	bool			m_bInQueue;
	int				m_iCurrentBusyAnim;
	CHandle<CAI_ActBusyGoal> m_hActBusyGoal;
	bool			m_bNeedToSetBounds;
	EHANDLE			m_hSeeEntity;
	float			m_fTimeLastSawSeeEntity;
	bool			m_bExitedBusyToDueLostSeeEntity;
	bool			m_bExitedBusyToDueSeeEnemy;

	int				m_iNumConsecutivePathFailures; // Count how many times we failed to find a path to a node, so we can consider teleporting.
	bool			m_bAutoFireWeapon;
	float			m_flDeferUntil;
	int				m_iNumEnemiesInSafeZone;

	CUtlVector<busysafezone_t>m_SafeZones;

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
};


//-----------------------------------------------------------------------------
// Purpose: A level tool to control the actbusy behavior.
//-----------------------------------------------------------------------------
class CAI_ActBusyGoal : public CAI_GoalEntity
{
	DECLARE_CLASS( CAI_ActBusyGoal, CAI_GoalEntity );
public:
	CAI_ActBusyGoal()
	{
		// Support legacy maps, where this value used to be set from a constant (with a value of 1).
		// Now designers can specify whatever they want in Hammer. Take care of old maps by setting
		// this in the constructor. (sjb)
		m_flSeeEntityTimeout = 1;
	}

	virtual void NPCMovingToBusy( CAI_BaseNPC *pNPC );
	virtual void NPCAbortedMoveTo( CAI_BaseNPC *pNPC );
	virtual void NPCStartedBusy( CAI_BaseNPC *pNPC );
	virtual void NPCStartedLeavingBusy( CAI_BaseNPC *pNPC );
	virtual void NPCFinishedBusy( CAI_BaseNPC *pNPC );
	virtual void NPCLeft( CAI_BaseNPC *pNPC );
	virtual void NPCLostSeeEntity( CAI_BaseNPC *pNPC );
	virtual void NPCSeeEnemy( CAI_BaseNPC *pNPC );

	int GetType() { return m_iType; }
	bool IsCombatActBusyTeleportAllowed()	{ return m_bAllowCombatActBusyTeleport; }

protected:
	CAI_ActBusyBehavior *GetBusyBehaviorForNPC( const char *pszActorName, CBaseEntity *pActivator, CBaseEntity *pCaller, const char *sInputName );
	CAI_ActBusyBehavior *GetBusyBehaviorForNPC( CBaseEntity *pEntity, const char *sInputName );

	void		 EnableGoal( CAI_BaseNPC *pAI );

	// Inputs
	virtual void InputActivate( inputdata_t &inputdata );
	virtual void InputDeactivate( inputdata_t &inputdata );
	void		 InputSetBusySearchRange( inputdata_t &inputdata );
	void		 InputForceNPCToActBusy( inputdata_t &inputdata );
	void		 InputForceThisNPCToActBusy( inputdata_t &inputdata );
	void		 InputForceThisNPCToLeave( inputdata_t &inputdata );

	DECLARE_DATADESC();

protected:
	float			m_flBusySearchRange;
	bool			m_bVisibleOnly;
	int				m_iType;
	bool			m_bAllowCombatActBusyTeleport;

public:
	// Let the actbusy behavior query these so we don't have to duplicate the data.
	string_t		m_iszSeeEntityName;
	float			m_flSeeEntityTimeout;
	string_t		m_iszSafeZoneVolume;
	int				m_iSightMethod;

protected:
	COutputEHANDLE	m_OnNPCStartedBusy;
	COutputEHANDLE	m_OnNPCFinishedBusy;
	COutputEHANDLE	m_OnNPCLeft;
	COutputEHANDLE	m_OnNPCLostSeeEntity;
	COutputEHANDLE	m_OnNPCSeeEnemy;
};

// Maximum number of nodes allowed in an actbusy queue
#define MAX_QUEUE_NODES		20

//-----------------------------------------------------------------------------
// Purpose: A level tool to control the actbusy behavior to create NPC queues 
//-----------------------------------------------------------------------------
class CAI_ActBusyQueueGoal : public CAI_ActBusyGoal
{
	DECLARE_CLASS( CAI_ActBusyQueueGoal, CAI_ActBusyGoal );
public:
	virtual void Spawn( void );
	virtual void DrawDebugGeometryOverlays( void );
	virtual void NPCMovingToBusy( CAI_BaseNPC *pNPC );
	virtual void NPCStartedBusy( CAI_BaseNPC *pNPC );
	virtual void NPCAbortedMoveTo( CAI_BaseNPC *pNPC );
	virtual void NPCFinishedBusy( CAI_BaseNPC *pNPC );
	virtual void NPCStartedLeavingBusy( CAI_BaseNPC *pNPC );

	virtual void InputActivate( inputdata_t &inputdata );
	void		 InputPlayerStartedBlocking( inputdata_t &inputdata );
	void		 InputPlayerStoppedBlocking( inputdata_t &inputdata );
	void		 InputMoveQueueUp( inputdata_t &inputdata );

	void		 PushNPCBackInQueue( CAI_BaseNPC *pNPC, int iStartingNode );
	void		 RemoveNPCFromQueue( CAI_BaseNPC *pNPC );
	void		 RecalculateQueueCount( void );
	void		 QueueThink( void );
  	void		 MoveQueueUp( void );
  	void		 MoveQueueUpThink( void );
	bool		 NodeIsOccupied( int i );
	CAI_BaseNPC			*GetNPCOnNode( int iNode );
	CAI_ActBusyBehavior *GetQueueBehaviorForNPC( CAI_BaseNPC	*pNPC );

	DECLARE_DATADESC();

private:
	int						m_iCurrentQueueCount;
	CHandle<CAI_Hint>		m_hNodes[ MAX_QUEUE_NODES ];
	bool					m_bPlayerBlockedNodes[ MAX_QUEUE_NODES ];
	EHANDLE					m_hExitNode;
	EHANDLE					m_hExitingNPC;
	bool					m_bForceReachFront;

	// Read from mapdata
	string_t		m_iszNodes[ MAX_QUEUE_NODES ];
	string_t		m_iszExitNode;

	// Outputs
	COutputInt		m_OnQueueMoved;
	COutputEHANDLE	m_OnNPCLeftQueue;
	COutputEHANDLE	m_OnNPCStartedLeavingQueue;
};

#endif // AI_BEHAVIOR_ACTBUSY_H
