//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SCRIPTED_H
#define SCRIPTED_H
#ifdef _WIN32
#pragma once
#endif

#ifndef SCRIPTEVENT_H
#include "scriptevent.h"
#endif

#include "ai_basenpc.h"


//
// The number of unique outputs that a script can fire from animation events.
// These are fired via SCRIPT_EVENT_FIREEVENT in CAI_BaseNPC::HandleAnimEvent.
//
#define MAX_SCRIPT_EVENTS				8


#define SF_SCRIPT_WAITTILLSEEN			1
#define SF_SCRIPT_EXITAGITATED			2
#define SF_SCRIPT_REPEATABLE			4		// Whether the script can be played more than once.
#define SF_SCRIPT_LEAVECORPSE			8
#define SF_SCRIPT_START_ON_SPAWN		16
#define SF_SCRIPT_NOINTERRUPT			32
#define SF_SCRIPT_OVERRIDESTATE			64
#define SF_SCRIPT_DONT_TELEPORT_AT_END	128		// Don't fixup end position with a teleport when the SS is finished
#define SF_SCRIPT_LOOP_IN_POST_IDLE		256		// Loop in the post idle animation after playing the action animation.
#define SF_SCRIPT_HIGH_PRIORITY			512		// If set, we don't allow other scripts to steal our spot in the queue.
#define SF_SCRIPT_SEARCH_CYCLICALLY		1024	// Start search from last entity found.
#define SF_SCRIPT_NO_COMPLAINTS			2048	// doesn't complain if it can't find anything
#define SF_SCRIPT_ALLOW_DEATH			4096	// the actor using this scripted sequence may die without interrupting the scene (used for scripted deaths)


enum script_moveto_t
{
	CINE_MOVETO_WAIT = 0,
	CINE_MOVETO_WALK = 1,
	CINE_MOVETO_RUN = 2,
	CINE_MOVETO_CUSTOM = 3,
	CINE_MOVETO_TELEPORT = 4,
	CINE_MOVETO_WAIT_FACING = 5,
};

enum SCRIPT_PLAYER_DEATH
{
	SCRIPT_DO_NOTHING = 0,
	SCRIPT_CANCEL = 1,
};


//
// Interrupt levels for grabbing NPCs to act out scripted events. These indicate
// how important it is to get a specific NPC, and can affect how they respond.
//
enum SS_INTERRUPT
{
	SS_INTERRUPT_BY_CLASS = 0,		// Indicates that we are asking for this NPC by class
	SS_INTERRUPT_BY_NAME,			// Indicates that we are asking for this NPC by name
};


// when a NPC finishes an AI scripted sequence, we can choose
// a schedule to place them in. These defines are the aliases to
// resolve worldcraft input to real schedules (sjb)
#define SCRIPT_FINISHSCHED_DEFAULT	0
#define SCRIPT_FINISHSCHED_AMBUSH	1

class CAI_ScriptedSequence : public CBaseEntity
{
	DECLARE_CLASS( CAI_ScriptedSequence, CBaseEntity );
public:
	void Spawn( void );
	virtual void Blocked( CBaseEntity *pOther );
	virtual void Touch( CBaseEntity *pOther );
	virtual int	 ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	virtual void Activate( void );
	virtual void UpdateOnRemove( void );
	void StartThink();
	void ScriptThink( void );
	void StopThink();

	DECLARE_DATADESC();

	void Pain( void );
	void Die( void );
	void DelayStart( bool bDelay );
	bool FindEntity( void );
	void StartScript( void );
	void FireScriptEvent( int nEvent );
	void OnBeginSequence( void );

	void SetTarget( CBaseEntity *pTarget ) { m_hTargetEnt = pTarget; };
	CBaseEntity *GetTarget( void ) { return m_hTargetEnt; };

	// Input handlers
	void InputBeginSequence( inputdata_t &inputdata );
	void InputCancelSequence( inputdata_t &inputdata );
	void InputMoveToPosition( inputdata_t &inputdata );

	bool IsTimeToStart( void );
	bool IsWaitingForBegin( void );
	void ReleaseEntity( CAI_BaseNPC *pEntity );
	void CancelScript( void );
	bool StartSequence( CAI_BaseNPC *pTarget, string_t iszSeq, bool completeOnEmpty );
	void SynchronizeSequence( CAI_BaseNPC *pNPC );
	bool FCanOverrideState ( void );
	void SequenceDone( CAI_BaseNPC *pNPC );
	void PostIdleDone( CAI_BaseNPC *pNPC );
	void FixScriptNPCSchedule( CAI_BaseNPC *pNPC, int iSavedCineFlags );
	void FixFlyFlag( CAI_BaseNPC *pNPC, int iSavedCineFlags );
	bool CanInterrupt( void );
	void AllowInterrupt( bool fAllow );
	void RemoveIgnoredConditions( void );
	bool PlayedSequence( void ) { return m_sequenceStarted; }
	bool CanEnqueueAfter( void );

	// Entry & Action loops
	bool IsPlayingEntry( void ) { return m_bIsPlayingEntry; }
	bool IsPlayingAction( void ) { return ( m_sequenceStarted && !m_bIsPlayingEntry ); }
	bool FinishedActionSequence( CAI_BaseNPC *pNPC );
	void SetLoopActionSequence( bool bLoop ) { m_bLoopActionSequence = bLoop; }
	bool ShouldLoopActionSequence( void ) { return m_bLoopActionSequence; }
	void StopActionLoop( bool bStopSynchronizedScenes );
	void SetSynchPostIdles( bool bSynch ) { m_bSynchPostIdles = bSynch; }
	void SynchNewSequence( CAI_BaseNPC::SCRIPTSTATE newState, string_t iszSequence, bool bSynchOtherScenes );

	// Dynamic scripted sequence spawning
	void ForceSetTargetEntity( CAI_BaseNPC *pTarget, bool bDontCancelOtherSequences );

	// Dynamic interactions
	void SetupInteractionPosition( CBaseEntity *pRelativeEntity, VMatrix &matDesiredLocalToWorld );
	void ModifyScriptedAutoMovement( Vector *vecNewPos );

	bool IsTeleportingDueToMoveTo( void ) { return m_bIsTeleportingDueToMoveTo; }

	// Debug
	virtual int DrawDebugTextOverlays( void );
	virtual void DrawDebugGeometryOverlays( void );

	void InputScriptPlayerDeath( inputdata_t &inputdata );

private:
	friend class CAI_BaseNPC;	// should probably try to eliminate this relationship

	string_t m_iszEntry;		// String index for animation that must be played before entering the main action anim
	string_t m_iszPreIdle;		// String index for idle animation to play before playing the action anim (only played while waiting for the script to begin)
	string_t m_iszPlay;			// String index for scripted action animation
	string_t m_iszPostIdle;		// String index for idle animation to play before playing the action anim
	string_t m_iszCustomMove;	// String index for custom movement animation
	string_t m_iszNextScript;	// Name of the script to run immediately after this one.
	string_t m_iszEntity;		// Entity that is wanted for this script

	int m_fMoveTo;
	bool m_bIsPlayingEntry;
	bool m_bLoopActionSequence;
	bool m_bSynchPostIdles;
	bool m_bIgnoreGravity;
	bool m_bDisableNPCCollisions;	// Used when characters must interpenetrate while riding on elevators, trains, etc.

	float m_flRadius;			// Range to search for an NPC to possess.
	float m_flRepeat;			// Repeat rate

	int m_iDelay;					// A counter indicating how many scripts are NOT ready to start.

	bool m_bDelayed;				// This moderately hacky hack ensures that we don't calls to DelayStart(true) or DelayStart(false)
									// twice in succession. This is necessary because we didn't want to remove the call to DelayStart(true)
									// from StartScript, even though DelayStart(true) is called from TASK_PRE_SCRIPT.
									// All of this is necessary in case the NPCs schedule gets cleared during the script and then they
									// reselect the schedule to play the script. Without this you can get NPCs stuck with m_iDelay = -1

	float m_startTime;				// Time when script actually started, used for synchronization
	bool m_bWaitForBeginSequence;	// Set to true when we are told to MoveToPosition. Holds the actor in the pre-action idle until BeginSequence is called.

	int m_saved_effects;
	int m_savedFlags;
	int m_savedCollisionGroup;

	bool m_interruptable;
	bool m_sequenceStarted;

	EHANDLE	m_hTargetEnt;

	EHANDLE m_hNextCine;		// The script to hand the NPC off to when we finish with them.
	
	bool	m_bThinking;
	bool 	m_bInitiatedSelfDelete;

	bool	m_bIsTeleportingDueToMoveTo;

	CAI_BaseNPC *FindScriptEntity( void );
	EHANDLE m_hLastFoundEntity;

	// Code forced us to use a specific NPC
	EHANDLE m_hForcedTarget;
	bool	m_bDontCancelOtherSequences;
	bool	m_bForceSynch;

	bool	m_bTargetWasAsleep;

	COutputEvent m_OnBeginSequence;
	COutputEvent m_OnEndSequence;
	COutputEvent m_OnPostIdleEndSequence;
	COutputEvent m_OnCancelSequence;
	COutputEvent m_OnCancelFailedSequence;	// Fired when a scene is cancelled before it's ever run
	COutputEvent m_OnScriptEvent[MAX_SCRIPT_EVENTS];

	static void ScriptEntityCancel( CBaseEntity *pentCine, bool bPretendSuccess = false );

	static const char *GetSpawnPreIdleSequenceForScript( CBaseEntity *pTargetEntity );

	// Dynamic interactions
	// For now, store just a single one of these. To synchronize positions
	// with multiple other NPCs, this needs to be an array of NPCs & desired position matrices.
	VMatrix		m_matInteractionPosition;
	EHANDLE		m_hInteractionRelativeEntity;

	int			m_iPlayerDeathBehavior;
};


#endif // SCRIPTED_H
