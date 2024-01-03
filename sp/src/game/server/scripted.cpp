//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation of entities that cause NPCs to participate in
//			scripted events. These entities find and temporarily possess NPCs
//			within a given search radius.
//
//			Multiple scripts with the same targetname will start frame-synchronized.
//
//			Scripts will find available NPCs by name or class name and grab them
//			to play the script. If the NPC is already playing a script, the
//			new script may enqueue itself unless there is already a non critical
//			script in the queue.
//
//=============================================================================//

#include "cbase.h"
#include "ai_schedule.h"
#include "ai_default.h"
#include "ai_motor.h"
#include "ai_hint.h"
#include "ai_networkmanager.h"
#include "ai_network.h"
#include "engine/IEngineSound.h"
#include "animation.h"
#include "scripted.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar ai_task_pre_script(  "ai_task_pre_script", "0", FCVAR_NONE );

// New macros introduced for Mapbase's console message color changes.
#ifdef MAPBASE
#define ScriptMsg( lvl, msg ) 					CGMsg( lvl, CON_GROUP_NPC_SCRIPTS, msg )
#define ScriptMsg1( lvl, msg, a ) 				CGMsg( lvl, CON_GROUP_NPC_SCRIPTS, msg, a )
#define ScriptMsg2( lvl, msg, a, b ) 			CGMsg( lvl, CON_GROUP_NPC_SCRIPTS, msg, a, b )
#define ScriptMsg3( lvl, msg, a, b, c ) 		CGMsg( lvl, CON_GROUP_NPC_SCRIPTS, msg, a, b, c )
#define ScriptMsg4( lvl, msg, a, b, c, d )		CGMsg( lvl, CON_GROUP_NPC_SCRIPTS, msg, a, b, c, d )
#define ScriptMsg5( lvl, msg, a, b, c, d, e )	CGMsg( lvl, CON_GROUP_NPC_SCRIPTS, msg, a, b, c, d, e )
#else
#define ScriptMsg( lvl, msg ) 					DevMsg( lvl, msg )
#define ScriptMsg1( lvl, msg, a ) 				DevMsg( lvl, msg, a )
#define ScriptMsg2( lvl, msg, a, b ) 			DevMsg( lvl, msg, a, b )
#define ScriptMsg3( lvl, msg, a, b, c ) 		DevMsg( lvl, msg, a, b, c )
#define ScriptMsg4( lvl, msg, a, b, c, d )		DevMsg( lvl, msg, a, b, c, d )
#define ScriptMsg5( lvl, msg, a, b, c, d, e )	DevMsg( lvl, msg, a, b, c, d, e )
#endif


//
// targetname "me" - there can be more than one with the same name, and they act in concert
// target "the_entity_I_want_to_start_playing" or "class entity_classname" will pick the closest inactive scientist
// play "name_of_sequence"
// idle "name of idle sequence to play before starting"
// moveto - if set the NPC first moves to this nodes position
// range # - only search this far to find the target
// spawnflags - (stop if blocked, stop if player seen)
//

BEGIN_DATADESC( CAI_ScriptedSequence )

	DEFINE_KEYFIELD( m_iszEntry, FIELD_STRING, "m_iszEntry" ),
	DEFINE_KEYFIELD( m_iszPreIdle, FIELD_STRING, "m_iszIdle" ),
	DEFINE_KEYFIELD( m_iszPlay, FIELD_STRING, "m_iszPlay" ),
	DEFINE_KEYFIELD( m_iszPostIdle, FIELD_STRING, "m_iszPostIdle" ),
	DEFINE_KEYFIELD( m_iszCustomMove, FIELD_STRING, "m_iszCustomMove" ),
	DEFINE_KEYFIELD( m_iszNextScript, FIELD_STRING, "m_iszNextScript" ),
	DEFINE_KEYFIELD( m_iszEntity, FIELD_STRING, "m_iszEntity" ),
	DEFINE_KEYFIELD( m_fMoveTo, FIELD_INTEGER, "m_fMoveTo" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "m_flRadius" ),
	DEFINE_KEYFIELD( m_flRepeat, FIELD_FLOAT, "m_flRepeat" ),

	DEFINE_FIELD( m_bIsPlayingEntry, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bLoopActionSequence, FIELD_BOOLEAN, "m_bLoopActionSequence" ),
	DEFINE_KEYFIELD( m_bSynchPostIdles, FIELD_BOOLEAN, "m_bSynchPostIdles" ),
	DEFINE_KEYFIELD( m_bIgnoreGravity, FIELD_BOOLEAN, "m_bIgnoreGravity" ),
	DEFINE_KEYFIELD( m_bDisableNPCCollisions, FIELD_BOOLEAN, "m_bDisableNPCCollisions" ),

	DEFINE_FIELD( m_iDelay, FIELD_INTEGER ),
	DEFINE_FIELD( m_bDelayed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_startTime, FIELD_TIME ),
	DEFINE_FIELD( m_bWaitForBeginSequence, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_saved_effects, FIELD_INTEGER ),
	DEFINE_FIELD( m_savedFlags, FIELD_INTEGER ),
	DEFINE_FIELD( m_savedCollisionGroup, FIELD_INTEGER ),
	
	DEFINE_FIELD( m_interruptable, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_sequenceStarted, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hTargetEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hNextCine, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLastFoundEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hForcedTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bDontCancelOtherSequences, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bForceSynch, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( m_bThinking, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInitiatedSelfDelete, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_bIsTeleportingDueToMoveTo, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_matInteractionPosition, FIELD_VMATRIX ),
	DEFINE_FIELD( m_hInteractionRelativeEntity, FIELD_EHANDLE ),

	DEFINE_FIELD( m_bTargetWasAsleep, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_THINKFUNC( ScriptThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "MoveToPosition", InputMoveToPosition ),
	DEFINE_INPUTFUNC( FIELD_VOID, "BeginSequence", InputBeginSequence ),
	DEFINE_INPUTFUNC( FIELD_VOID, "CancelSequence", InputCancelSequence ),

	DEFINE_KEYFIELD( m_iPlayerDeathBehavior, FIELD_INTEGER, "onplayerdeath" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ScriptPlayerDeath", InputScriptPlayerDeath ),

#ifdef MAPBASE
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),
#endif

	// Outputs
	DEFINE_OUTPUT(m_OnBeginSequence, "OnBeginSequence"),
	DEFINE_OUTPUT(m_OnEndSequence, "OnEndSequence"),
	DEFINE_OUTPUT(m_OnPostIdleEndSequence, "OnPostIdleEndSequence"),
	DEFINE_OUTPUT(m_OnCancelSequence, "OnCancelSequence"),
	DEFINE_OUTPUT(m_OnCancelFailedSequence, "OnCancelFailedSequence"),
	DEFINE_OUTPUT(m_OnScriptEvent[0], "OnScriptEvent01"),
	DEFINE_OUTPUT(m_OnScriptEvent[1], "OnScriptEvent02"),
	DEFINE_OUTPUT(m_OnScriptEvent[2], "OnScriptEvent03"),
	DEFINE_OUTPUT(m_OnScriptEvent[3], "OnScriptEvent04"),
	DEFINE_OUTPUT(m_OnScriptEvent[4], "OnScriptEvent05"),
	DEFINE_OUTPUT(m_OnScriptEvent[5], "OnScriptEvent06"),
	DEFINE_OUTPUT(m_OnScriptEvent[6], "OnScriptEvent07"),
	DEFINE_OUTPUT(m_OnScriptEvent[7], "OnScriptEvent08"),
#ifdef MAPBASE
	DEFINE_OUTPUT(m_OnEntrySequence, "OnEntrySequence"),
	DEFINE_OUTPUT(m_OnActionSequence, "OnActionSequence"),
	DEFINE_OUTPUT(m_OnPreIdleSequence, "OnPreIdleSequence"),
	DEFINE_OUTPUT(m_OnFoundNPC, "OnFoundNPC"),
#endif

END_DATADESC()


LINK_ENTITY_TO_CLASS( scripted_sequence, CAI_ScriptedSequence );
#define CLASSNAME "scripted_sequence"

//-----------------------------------------------------------------------------
// Purpose: Cancels the given scripted sequence.
// Input  : pentCine - 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::ScriptEntityCancel( CBaseEntity *pentCine, bool bPretendSuccess )
{
	// make sure they are a scripted_sequence
	if ( FClassnameIs( pentCine, CLASSNAME ) )
	{
		CAI_ScriptedSequence *pCineTarget = (CAI_ScriptedSequence *)pentCine;

		// make sure they have a NPC in mind for the script
		CBaseEntity *pEntity = pCineTarget->GetTarget();
		CAI_BaseNPC	*pTarget = NULL;
		if ( pEntity )
			pTarget = pEntity->MyNPCPointer();

		if (pTarget)
		{
			// make sure their NPC is actually playing a script
			if ( pTarget->m_NPCState == NPC_STATE_SCRIPT )
			{
				// tell them do die
				pTarget->m_scriptState = CAI_BaseNPC::SCRIPT_CLEANUP;

				// We have to save off the flags here, because the NPC's m_hCine is cleared in CineCleanup()
				int iSavedFlags = (pTarget->m_hCine ? pTarget->m_hCine->m_savedFlags : 0);

#ifdef HL1_DLL
				//if we didn't have FL_FLY before the script, remove it
				// for some reason hl2 doesn't have to do this *before* 
				// restoring the position ( which checks FL_FLY ) in CineCleanup
				// Let's not risk breaking anything at this stage and just remove it.
				pCineTarget->FixFlyFlag( pTarget, iSavedFlags );
#endif
				// do it now				
				pTarget->CineCleanup( );
				pCineTarget->FixScriptNPCSchedule( pTarget, iSavedFlags );
			}
			else
			{
				// Robin HACK: If a script is started and then cancelled before an NPC gets to
				//		 think, we have to manually clear it out of scripted state, or it'll never recover.
				pCineTarget->SetTarget( NULL );
				pTarget->SetEffects( pCineTarget->m_saved_effects );
				pTarget->m_hCine = NULL;
				pTarget->SetTarget( NULL );
				pTarget->SetGoalEnt( NULL );
				pTarget->SetIdealState( NPC_STATE_IDLE );
			}
		}

		// FIXME: this needs to be done in a cine cleanup function
		pCineTarget->m_iDelay = 0;

		if ( bPretendSuccess )
		{
			// We need to pretend that this sequence actually finished fully
#ifdef MAPBASE
			pCineTarget->m_OnEndSequence.FireOutput(pEntity, pCineTarget);
			pCineTarget->m_OnPostIdleEndSequence.FireOutput(pEntity, pCineTarget);
#else
			pCineTarget->m_OnEndSequence.FireOutput(NULL, pCineTarget);
			pCineTarget->m_OnPostIdleEndSequence.FireOutput(NULL, pCineTarget);
#endif
		}
		else
		{
			// Fire the cancel
#ifdef MAPBASE
			pCineTarget->m_OnCancelSequence.FireOutput(pEntity, pCineTarget);
#else
 			pCineTarget->m_OnCancelSequence.FireOutput(NULL, pCineTarget);
#endif

			if ( pCineTarget->m_startTime == 0 )
			{
				// If start time is 0, this sequence never actually ran. Fire the failed output.
#ifdef MAPBASE
				pCineTarget->m_OnCancelFailedSequence.FireOutput(pEntity, pCineTarget);
#else
				pCineTarget->m_OnCancelFailedSequence.FireOutput(NULL, pCineTarget);
#endif
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after keyvalues have been parsed.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::Spawn( void )
{
	SetSolid( SOLID_NONE );

	//
	// If we have no name or we are set to start immediately, find the NPC and
	// have them move to their script position now.
	//
	if ( !GetEntityName() || ( m_spawnflags & SF_SCRIPT_START_ON_SPAWN ) )
	{
		StartThink();
		SetNextThink( gpGlobals->curtime + 1.0f );

		//
		// If we have a name, wait for a BeginSequence input to play the
		// action animation. Otherwise, we'll play the action animation
		// as soon as the NPC reaches the script position.
		//
		if ( GetEntityName() != NULL_STRING )
		{
			m_bWaitForBeginSequence = true;
		}
	}

	if ( m_spawnflags & SF_SCRIPT_NOINTERRUPT )
	{
		m_interruptable = false;
	}
	else
	{
		m_interruptable = true;
	}

	m_sequenceStarted = false;
	m_startTime = 0;
	m_hNextCine = NULL;

	m_hLastFoundEntity = NULL;
}

//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::UpdateOnRemove(void)
{
	ScriptEntityCancel( this );
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::StartThink()
{
	m_sequenceStarted = false;
	m_bThinking = true;
	SetThink( &CAI_ScriptedSequence::ScriptThink );
}

//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::StopThink()
{
	if ( m_bThinking )
	{
		Assert( !m_bInitiatedSelfDelete );
		SetThink( NULL);
		m_bThinking = false;
	}
}
//-----------------------------------------------------------------------------
// Purpose: Returns true if this scripted sequence can possess entities
//			regardless of state.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::FCanOverrideState( void )
{
	if ( m_spawnflags & SF_SCRIPT_OVERRIDESTATE )
		return true;
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Fires a script event by number.
// Input  : nEvent - One based index of the script event from the , from 1 to 8.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::FireScriptEvent( int nEvent )
{
	if ( ( nEvent >= 1 ) && ( nEvent <= MAX_SCRIPT_EVENTS ) )
	{
		m_OnScriptEvent[nEvent - 1].FireOutput( this, this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that causes the NPC to move to the script position.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::InputMoveToPosition( inputdata_t &inputdata )
{
	if ( m_bInitiatedSelfDelete )
		return;
		
	// Have I already grabbed an NPC?
	CBaseEntity *pEntity = GetTarget();
	CAI_BaseNPC	*pTarget = NULL;

	if ( pEntity )
	{
		pTarget = pEntity->MyNPCPointer();
	}

	if ( pTarget != NULL ) 
	{
		// Yes, are they already playing this script?
		if ( pTarget->m_scriptState == CAI_BaseNPC::SCRIPT_PLAYING || pTarget->m_scriptState == CAI_BaseNPC::SCRIPT_POST_IDLE )
		{
			// Yes, see if we can enqueue ourselves.
			if ( pTarget->CanPlaySequence( FCanOverrideState(), SS_INTERRUPT_BY_NAME ) )
			{
				StartScript();
				m_bWaitForBeginSequence = true;
			}
		}

		// No, presumably they are moving to position or waiting for the BeginSequence input.
	}
	else
	{
		// No, grab the NPC but make them wait until BeginSequence is fired. They'll play
		// their pre-action idle animation until BeginSequence is fired.
		StartThink();
		SetNextThink( gpGlobals->curtime );
		m_bWaitForBeginSequence = true;
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Sets our target NPC with the generic SetTarget input.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::InputSetTarget( inputdata_t &inputdata )
{
	m_hActivator = inputdata.pActivator;
	m_iszEntity = AllocPooledString(inputdata.value.String());
	m_hTargetEnt = NULL;
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Input handler that activates the scripted sequence.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::InputBeginSequence( inputdata_t &inputdata )
{
	if ( m_bInitiatedSelfDelete )
		return;

	// Start the script as soon as possible.
	m_bWaitForBeginSequence = false;

#ifdef MAPBASE
	m_hActivator = inputdata.pActivator;

	// TODO: Investigate whether this is necessary
	//if (FStrEq(STRING(m_iszEntity), "!activator"))
	//	SetTarget(NULL);
#endif
		
	// do I already know who I should use?
	CBaseEntity *pEntity = GetTarget();
	CAI_BaseNPC	*pTarget = NULL;

	if ( !pEntity && m_hForcedTarget )
	{
		if ( FindEntity() )
		{
			pEntity = GetTarget();
		}
	}

	if ( pEntity )
	{
		pTarget = pEntity->MyNPCPointer();
	}

	if ( pTarget ) 
	{
		// Are they already playing a script?
		if ( pTarget->m_scriptState == CAI_BaseNPC::SCRIPT_PLAYING || pTarget->m_scriptState == CAI_BaseNPC::SCRIPT_POST_IDLE )
		{
			// See if we can enqueue ourselves after the current script.
			if ( pTarget->CanPlaySequence( FCanOverrideState(), SS_INTERRUPT_BY_NAME ) )
			{
				StartScript();
			}
		}
	}
	else
	{
		// if not, try finding them
		StartThink();

		// Because we are directly calling the new "think" function ScriptThink, assume we're done 
		// This fixes the following bug (along with the WokeThisTick() code herein:
		//  A zombie is created in the asleep state and then, the mapper fires both Wake and BeginSequence
		//  messages to have it jump up out of the slime, e.g.  What happens before this change is that
		//  the Wake code removed EF_NODRAW, but so the zombie is transmitted to the client, but the script
		//  hasn't started and won't start until the next Think time (2 ticks on xbox) at which time the
		//  actual sequence starts causing the zombie to quickly lie down.
		// The changes here are to track what tick we "awoke" on and get rid of the lag between Wake and
		// ScriptThink by actually calling ScriptThink directly on the same frame and checking for the
		//  zombie having woken up and been instructed to play a sequence in the same frame.
		SetNextThink( TICK_NEVER_THINK );
		ScriptThink();
	}

}


//-----------------------------------------------------------------------------
// Purpose: Input handler that activates the scripted sequence.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::InputCancelSequence( inputdata_t &inputdata )
{
	if ( m_bInitiatedSelfDelete )
		return;

	//
	// We don't call CancelScript because entity I/O will handle dispatching
	// this input to all other scripts with our same name.
	//
	ScriptMsg1( 2,  "InputCancelScript: Cancelling script '%s'\n", STRING( m_iszPlay ));
	StopThink();
	ScriptEntityCancel( this );
}

void CAI_ScriptedSequence::InputScriptPlayerDeath( inputdata_t &inputdata )
{
    if ( m_iPlayerDeathBehavior == SCRIPT_CANCEL )
	{
		if ( m_bInitiatedSelfDelete )
			return;

		//
		// We don't call CancelScript because entity I/O will handle dispatching
		// this input to all other scripts with our same name.
		//
		ScriptMsg1( 2,  "InputCancelScript: Cancelling script '%s'\n", STRING( m_iszPlay ));
		StopThink();
		ScriptEntityCancel( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if it is time for this script to start, false if the
//			NPC should continue waiting.
//
//			Scripts wait for two reasons:
//
//			1. To frame-syncronize with other scripts of the same name.
//			2. To wait indefinitely for the BeginSequence input after the NPC
//				moves to the script position.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::IsTimeToStart( void )
{
	Assert( !m_bWaitForBeginSequence );

	return ( m_iDelay == 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the script is still waiting to call StartScript()
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::IsWaitingForBegin( void )
{
	return m_bWaitForBeginSequence;
}

//-----------------------------------------------------------------------------
// Purpose: This doesn't really make sense since only MOVETYPE_PUSH get 'Blocked' events
// Input  : pOther - The entity blocking us.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::Blocked( CBaseEntity *pOther )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pOther - The entity touching us.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::Touch( CBaseEntity *pOther )
{
/*
	ScriptMsg( 2,  "Cine Touch\n" );
	if (m_pentTarget && OFFSET(pOther->pev) == OFFSET(m_pentTarget))
	{
		CAI_BaseNPC *pTarget = GetClassPtr((CAI_BaseNPC *)VARS(m_pentTarget));
		pTarget->m_NPCState == NPC_STATE_SCRIPT;
	}
*/
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::Die( void )
{
	SetThink( &CAI_ScriptedSequence::SUB_Remove );
	m_bThinking = false;
	m_bInitiatedSelfDelete = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::Pain( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eMode - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
CAI_BaseNPC *CAI_ScriptedSequence::FindScriptEntity( )
{
	CAI_BaseNPC *pEnqueueNPC = NULL;

	CBaseEntity *pEntity;
	int interrupt;
	if ( m_hForcedTarget )
	{
		interrupt = SS_INTERRUPT_BY_NAME;
		pEntity = m_hForcedTarget;
	}
	else
	{
		interrupt = SS_INTERRUPT_BY_NAME;
		
#ifdef MAPBASE
		pEntity = gEntList.FindEntityByNameWithin( m_hLastFoundEntity, STRING( m_iszEntity ), GetAbsOrigin(), m_flRadius, this, m_hActivator );
#else
		pEntity = gEntList.FindEntityByNameWithin( m_hLastFoundEntity, STRING( m_iszEntity ), GetAbsOrigin(), m_flRadius );
#endif
		if (!pEntity)
		{
			pEntity = gEntList.FindEntityByClassnameWithin( m_hLastFoundEntity, STRING( m_iszEntity ), GetAbsOrigin(), m_flRadius );
			interrupt = SS_INTERRUPT_BY_CLASS;
		}
	}

	while ( pEntity != NULL )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer( );
		if ( pNPC )
		{
			//
			// If they can play the sequence...
			//
			CanPlaySequence_t eCanPlay = pNPC->CanPlaySequence( FCanOverrideState(), interrupt );
			if ( eCanPlay == CAN_PLAY_NOW )
			{
				// If they can play it now, we're done!
				return pNPC;
			}
			else if ( eCanPlay == CAN_PLAY_ENQUEUED )
			{
				// They can play it, but only enqueued. We'll use them as a last resort.
				pEnqueueNPC = pNPC;
			}
			else if (!(m_spawnflags & SF_SCRIPT_NO_COMPLAINTS))
			{
				// They cannot play the script.
				ScriptMsg1( 1, "Found %s, but can't play!\n", STRING( m_iszEntity ));
			}
		}

		if ( m_hForcedTarget )
		{
			Warning( "Code forced %s(%s), to be the target of scripted sequence %s, but it can't play it.\n", 
						pEntity->GetClassname(), pEntity->GetDebugName(), GetDebugName() );
			pEntity = NULL;
			UTIL_Remove( this );
			return NULL;
		}
		else
		{		
			if ( interrupt == SS_INTERRUPT_BY_NAME )
				pEntity = gEntList.FindEntityByNameWithin( pEntity, STRING( m_iszEntity ), GetAbsOrigin(), m_flRadius );
			else
				pEntity = gEntList.FindEntityByClassnameWithin( pEntity, STRING( m_iszEntity ), GetAbsOrigin(), m_flRadius );
		}
	}

	//
	// If we found an NPC that will enqueue the script, use them.
	//
	if ( pEnqueueNPC != NULL )
	{
		return pEnqueueNPC;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::FindEntity( void )
{
	CAI_BaseNPC *pTarget = FindScriptEntity( );

	if ( (m_spawnflags & SF_SCRIPT_SEARCH_CYCLICALLY))
	{
		// next time this is called, start searching from the one found last time
		m_hLastFoundEntity = pTarget;
	}

	SetTarget( pTarget );

	return pTarget != NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Make the entity enter a scripted sequence.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::StartScript( void )
{
	CBaseEntity		*pEntity = GetTarget();
	CAI_BaseNPC	*pTarget = NULL;
	if ( pEntity )
		pTarget = pEntity->MyNPCPointer();

	if ( pTarget )
	{
		pTarget->RemoveSpawnFlags( SF_NPC_WAIT_FOR_SCRIPT );

		//
		// If the NPC is in another script, just enqueue ourselves and bail out.
		// We'll possess the NPC when the current script finishes with the NPC.
		// Note that we only enqueue one deep currently, so if there is someone
		// else in line we'll stomp them.
		//
		if ( pTarget->m_hCine != NULL )
		{
			if ( pTarget->m_hCine->m_hNextCine != NULL )
			{
				//
				// Kicking another script out of the queue.
				//
				CAI_ScriptedSequence *pCine = ( CAI_ScriptedSequence * )pTarget->m_hCine->m_hNextCine.Get();

				if (pTarget->m_hCine->m_hNextCine != pTarget->m_hCine)
				{
					// Don't clear the currently playing script's target!
					pCine->SetTarget( NULL );
				}
				ScriptMsg2( 2, "script \"%s\" kicking script \"%s\" out of the queue\n", GetDebugName(), pCine->GetDebugName() );
			}

			pTarget->m_hCine->m_hNextCine = this;
			return;
		}

		//
		// If no next script is specified, clear it out so other scripts can enqueue themselves
		// after us.
		//
		if ( !m_iszNextScript )
		{
			m_hNextCine = NULL;
		}

		// UNDONE: Do this to sync up multi-entity scripts?
		//pTarget->SetNextThink( gpGlobals->curtime );

		pTarget->SetGoalEnt( this );
		pTarget->ForceDecisionThink();
		pTarget->m_hCine = this;
		pTarget->SetTarget( this );
		
		// Notify the NPC tat we're stomping them into a scene!
		pTarget->OnStartScene();

		{
			m_bTargetWasAsleep = ( pTarget->GetSleepState() != AISS_AWAKE ) ? true : false;
			bool justAwoke = pTarget->WokeThisTick();
			if ( m_bTargetWasAsleep || justAwoke )
			{
				// Note, Wake() will remove the EF_NODRAW flag, but if we are starting a seq on a hidden entity
				//  we don't want it to draw on the client until the sequence actually starts to play
				// Make sure it stays hidden!!!
				if ( m_bTargetWasAsleep )
				{
					pTarget->Wake();
				}
				m_bTargetWasAsleep = true;

				// Even if awakened this frame, temporarily keep the entity hidden for now
				pTarget->AddEffects( EF_NODRAW );
			}
		}

		// If the entity was asleep at the start, make sure we don't make it invisible
		// AFTER the script finishes (can't think of a case where you'd want that to happen)
		m_saved_effects = pTarget->GetEffects() & ~EF_NODRAW;
		pTarget->AddEffects( GetEffects() );
		m_savedFlags = pTarget->GetFlags();
		m_savedCollisionGroup = pTarget->GetCollisionGroup();
		
		if ( m_bDisableNPCCollisions )
		{
			pTarget->SetCollisionGroup( COLLISION_GROUP_NPC_SCRIPTED );
		}

		switch (m_fMoveTo)
		{
		case CINE_MOVETO_WAIT: 
		case CINE_MOVETO_WAIT_FACING:
			pTarget->m_scriptState = CAI_BaseNPC::SCRIPT_WAIT; 

			if ( m_bIgnoreGravity )
			{
				pTarget->AddFlag( FL_FLY );
				pTarget->SetGroundEntity( NULL );
			}

			break;

		case CINE_MOVETO_WALK:
			pTarget->m_scriptState = CAI_BaseNPC::SCRIPT_WALK_TO_MARK;
			break;

		case CINE_MOVETO_RUN: 
			pTarget->m_scriptState = CAI_BaseNPC::SCRIPT_RUN_TO_MARK; 
			break;

		case CINE_MOVETO_CUSTOM:
			pTarget->m_scriptState = CAI_BaseNPC::SCRIPT_CUSTOM_MOVE_TO_MARK; 
			break;

		case CINE_MOVETO_TELEPORT: 
			m_bIsTeleportingDueToMoveTo = true;
			pTarget->Teleport( &GetAbsOrigin(), NULL, &vec3_origin );
			m_bIsTeleportingDueToMoveTo = false;
			pTarget->GetMotor()->SetIdealYaw( GetLocalAngles().y );
			pTarget->SetLocalAngularVelocity( vec3_angle );
			pTarget->IncrementInterpolationFrame();
			QAngle angles = pTarget->GetLocalAngles();
			angles.y = GetLocalAngles().y;
			pTarget->SetLocalAngles( angles );
			pTarget->m_scriptState = CAI_BaseNPC::SCRIPT_WAIT;

			if ( m_bIgnoreGravity )
			{
				pTarget->AddFlag( FL_FLY );
				pTarget->SetGroundEntity( NULL );
			}

			// UNDONE: Add a flag to do this so people can fixup physics after teleporting NPCs
			//pTarget->SetGroundEntity( NULL );
			break;
		}
		//ScriptMsg2( 2,  "\"%s\" found and used (INT: %s)\n", STRING( pTarget->m_iName ), FBitSet(m_spawnflags, SF_SCRIPT_NOINTERRUPT)?"No":"Yes" );


		// Wait until all scripts of the same name are ready to play.
		m_bDelayed = false;
		DelayStart( true ); 

		pTarget->SetIdealState(NPC_STATE_SCRIPT);

		// FIXME: not sure why this is happening, or what to do about truely dormant NPCs
		if ( pTarget->IsEFlagSet( EFL_NO_THINK_FUNCTION ) && pTarget->GetNextThink() != TICK_NEVER_THINK )
		{
			DevWarning( "scripted_sequence %d:%s - restarting dormant entity %d:%s : %.1f:%.1f\n", entindex(), GetDebugName(), pTarget->entindex(), pTarget->GetDebugName(), gpGlobals->curtime, pTarget->GetNextThink() );
			pTarget->SetNextThink( gpGlobals->curtime );
		}

#ifdef MAPBASE
		m_OnFoundNPC.FireOutput( pTarget, this );
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: First think after activation. Grabs an NPC and makes it do things.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::ScriptThink( void )
{
	if ( g_pAINetworkManager && !g_pAINetworkManager->IsInitialized() )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else if (FindEntity())
	{
		StartScript( );
		ScriptMsg5( 2,  "scripted_sequence %d:\"%s\" using NPC %d:\"%s\"(%s)\n", entindex(), GetDebugName(), GetTarget()->entindex(), GetTarget()->GetEntityName().ToCStr(), STRING( m_iszEntity ) );
	}
	else
	{
		CancelScript( );
		ScriptMsg3( 2,  "scripted_sequence %d:\"%s\" can't find NPC \"%s\"\n", entindex(), GetDebugName(), STRING( m_iszEntity ) );
		// FIXME: just trying again is bad.  This should fire an output instead.
		// FIXME: Think about puting output triggers in both StartScript() and CancelScript().
		SetNextThink( gpGlobals->curtime + 1.0f );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Callback for firing the begin sequence output. Called by the NPC that
//			is running the script as it starts the action seqeunce.
//-----------------------------------------------------------------------------
#ifdef MAPBASE
void CAI_ScriptedSequence::OnBeginSequence( CBaseEntity *pActor )
{
	m_OnBeginSequence.FireOutput( pActor, this );
}

void CAI_ScriptedSequence::OnEntrySequence( CBaseEntity *pActor )
{
	m_OnEntrySequence.FireOutput( pActor, this );
}

void CAI_ScriptedSequence::OnActionSequence( CBaseEntity *pActor )
{
	m_OnActionSequence.FireOutput( pActor, this );
}

void CAI_ScriptedSequence::OnPreIdleSequence( CBaseEntity *pActor )
{
	m_OnPreIdleSequence.FireOutput( pActor, this );
}
#else
void CAI_ScriptedSequence::OnBeginSequence( void )
{
	m_OnBeginSequence.FireOutput( this, this );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Look up a sequence name and setup the target NPC to play it.
// Input  : pTarget - 
//			iszSeq - 
//			completeOnEmpty - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::StartSequence( CAI_BaseNPC *pTarget, string_t iszSeq, bool completeOnEmpty )
{
	Assert( pTarget );
	m_sequenceStarted = true;
	m_bIsPlayingEntry = (iszSeq == m_iszEntry);

	if ( !iszSeq && completeOnEmpty )
	{
		SequenceDone( pTarget );
		return false;
	}

	int nSequence = pTarget->LookupSequence( STRING( iszSeq ) );
	if (nSequence == -1)
	{
		Warning( "%s: unknown scripted sequence \"%s\"\n", pTarget->GetDebugName(), STRING( iszSeq ));
		nSequence = 0;
	}

	// look for the activity that this represents
	Activity act = pTarget->GetSequenceActivity( nSequence );
	if (act == ACT_INVALID)
		act = ACT_IDLE;

	pTarget->SetActivityAndSequence( act, nSequence, act, act );

	// If the target was hidden even though we woke it up, only make it drawable if we're not still on the preidle seq...
	if ( m_bTargetWasAsleep && 
		iszSeq != m_iszPreIdle )
	{
		m_bTargetWasAsleep = false;
		// Show it
		pTarget->RemoveEffects( EF_NODRAW );
		// Don't blend...
		pTarget->IncrementInterpolationFrame();
	}
	//ScriptMsg4( 2, "%s (%s): started \"%s\":INT:%s\n", STRING( pTarget->m_iName ), pTarget->GetClassname(), STRING( iszSeq), (m_spawnflags & SF_SCRIPT_NOINTERRUPT) ? "No" : "Yes" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a scripted sequence is ready to start playing the sequence
// Input  : pNPC - Pointer to the NPC that the sequence possesses.
//-----------------------------------------------------------------------------

void CAI_ScriptedSequence::SynchronizeSequence( CAI_BaseNPC *pNPC )
{
	//Msg("%s (for %s) called SynchronizeSequence() at %0.2f\n", GetTarget()->GetDebugName(), GetDebugName(), gpGlobals->curtime);

	Assert( m_iDelay == 0 );
	Assert( m_bWaitForBeginSequence == false );
	m_bForceSynch = false;

	// Reset cycle position
	float flCycleRate = pNPC->GetSequenceCycleRate( pNPC->GetSequence() );
	float flInterval = gpGlobals->curtime - m_startTime;

	// Msg("%.2f \"%s\"  %s : %f (%f): interval %f\n", gpGlobals->curtime, GetEntityName().ToCStr(), pNPC->GetClassname(), pNPC->m_flAnimTime.Get(), m_startTime, flInterval );
	//Assert( flInterval >= 0.0 && flInterval <= 0.15 );
	flInterval = clamp( flInterval, 0.f, 0.15f );

	if (flInterval == 0)
		return;

	// do the movement for the missed portion of the sequence
	pNPC->SetCycle( 0.0f );
	pNPC->AutoMovement( flInterval );

	// reset the cycle to a common basis
	pNPC->SetCycle( flInterval * flCycleRate );
}

//-----------------------------------------------------------------------------
// Purpose: Moves to the next action sequence if the scripted_sequence wants to,
//			or returns true if it wants to leave the action sequence
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::FinishedActionSequence( CAI_BaseNPC *pNPC )
{
	// Restart the action sequence when the entry finishes, or when the action
	// finishes and we're set to loop it.
	if ( IsPlayingEntry() )
	{
		if ( GetEntityName() != NULL_STRING )
		{
			// Force all matching ss's to synchronize their action sequences
			SynchNewSequence( CAI_BaseNPC::SCRIPT_PLAYING, m_iszPlay, true );
		}
		else
		{
			StartSequence( pNPC, m_iszPlay, true );
		}
		return false;
	}

	// Let the core action sequence continue to loop
	if ( ShouldLoopActionSequence() )
	{
		// If the NPC has reached post idle state, we need to stop looping the action sequence
		if ( pNPC->m_scriptState == CAI_BaseNPC::SCRIPT_POST_IDLE )
			return true;

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a scripted sequence animation sequence is done playing
//			(or when an AI Scripted Sequence doesn't supply an animation sequence
//			to play).
// Input  : pNPC - Pointer to the NPC that the sequence possesses.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::SequenceDone( CAI_BaseNPC *pNPC )
{
	//ScriptMsg1( 2, "Sequence %s finished\n", STRING( pNPC->m_hCine->m_iszPlay ) );

	//Msg("%s SequenceDone() at %0.2f\n", pNPC->GetDebugName(), gpGlobals->curtime );

	// If we're part of a synchronised post-idle sequence, we need to do things differently
	if ( m_bSynchPostIdles && GetEntityName() != NULL_STRING )
	{
		// If we're already in POST_IDLE state, then one of the other scripted
		// sequences we're synching with has already kicked us into running
		// the post idle sequence, so we do nothing.
		if ( pNPC->m_scriptState != CAI_BaseNPC::SCRIPT_POST_IDLE )
		{
			if ( ( m_iszPostIdle != NULL_STRING ) && ( m_hNextCine == NULL ) )
			{
				SynchNewSequence( CAI_BaseNPC::SCRIPT_POST_IDLE, m_iszPostIdle, true );
			}
			else
			{
				PostIdleDone( pNPC );
			}
		}
	}
	else
	{
		//
		// If we have a post idle set, and no other script is in the queue for this
		// NPC, start playing the idle now.
		//
		if ( ( m_iszPostIdle != NULL_STRING ) && ( m_hNextCine == NULL ) )
		{
			//
			// First time we've gotten here for this script. Start playing the post idle
			// if one is specified.
			//
			pNPC->m_scriptState = CAI_BaseNPC::SCRIPT_POST_IDLE; 
			StartSequence( pNPC, m_iszPostIdle, false ); // false to prevent recursion here!
		}
		else
		{
			PostIdleDone( pNPC );
		}
	}

#ifdef MAPBASE
	m_OnEndSequence.FireOutput(pNPC, this);
#else
	m_OnEndSequence.FireOutput(NULL, this);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::SynchNewSequence( CAI_BaseNPC::SCRIPTSTATE newState, string_t iszSequence, bool bSynchOtherScenes )
{
	// Do we need to synchronize all our matching scripted scenes?
 	if ( bSynchOtherScenes )
	{
		//Msg("%s (for %s) forcing synch of %s at %0.2f\n", GetTarget()->GetDebugName(), GetDebugName(), iszSequence, gpGlobals->curtime);

 		CBaseEntity *pentCine = gEntList.FindEntityByName( NULL, GetEntityName(), NULL );
		while ( pentCine )
		{
			CAI_ScriptedSequence *pScene = dynamic_cast<CAI_ScriptedSequence *>(pentCine);
			if ( pScene && pScene != this )
			{
				pScene->SynchNewSequence( newState, iszSequence, false );
			}
			pentCine = gEntList.FindEntityByName( pentCine, GetEntityName(), NULL );
		}
	}

	// Now force this one to start the post idle?
	if ( !GetTarget() )
		return;
	CAI_BaseNPC *pNPC = GetTarget()->MyNPCPointer();
	if ( !pNPC )
		return;

 	//Msg("%s (for %s) starting %s seq at %0.2f\n", pNPC->GetDebugName(), GetDebugName(), iszSequence, gpGlobals->curtime);

	m_startTime = gpGlobals->curtime;
	pNPC->m_scriptState = newState; 
 	StartSequence( pNPC, iszSequence, false );

	// Force the NPC to synchronize animations on their next think
	m_bForceSynch = true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a scripted sequence animation sequence is done playing
//			(or when an AI Scripted Sequence doesn't supply an animation sequence
//			to play).
// Input  : pNPC - Pointer to the NPC that the sequence possesses.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::PostIdleDone( CAI_BaseNPC *pNPC )
{
	//
	// If we're set to keep the NPC in a scripted idle, hack them back into the script,
	// but allow another scripted sequence to take control of the NPC if it wants to,
	// unless another script has stolen them from us.
	//
	if ( ( m_iszPostIdle != NULL_STRING ) && ( m_spawnflags & SF_SCRIPT_LOOP_IN_POST_IDLE ) && ( m_hNextCine == NULL ) )
	{
		//
		// First time we've gotten here for this script. Start playing the post idle
		// if one is specified.
		// Only do so if we're selected, to prevent spam
		if ( pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT )
		{
			ScriptMsg2( 2, "Post Idle %s finished for %s\n", STRING( pNPC->m_hCine->m_iszPostIdle ), pNPC->GetDebugName() );
		}

		pNPC->m_scriptState = CAI_BaseNPC::SCRIPT_POST_IDLE; 
		StartSequence( pNPC, m_iszPostIdle, false );
	}
	else
	{
		if ( !( m_spawnflags & SF_SCRIPT_REPEATABLE ) )
		{
			SetThink( &CAI_ScriptedSequence::SUB_Remove );
			SetNextThink( gpGlobals->curtime + 0.1f );
			m_bThinking = false;
			m_bInitiatedSelfDelete = true;
		}

		//
		// This is done so that another sequence can take over the NPC when triggered
		// by the first.
		//
		pNPC->CineCleanup();

		// We have to pass in the flags here, because the NPC's m_hCine was cleared in CineCleanup()
		FixScriptNPCSchedule( pNPC, m_savedFlags );

		//
		// If another script is waiting to grab this NPC, start the next script
		// immediately.
		//
		if ( m_hNextCine != NULL )
		{
			CAI_ScriptedSequence *pNextCine = ( CAI_ScriptedSequence * )m_hNextCine.Get();

			//
			// Don't link ourselves in if we are going to be deleted.
			// TODO: use EHANDLEs instead of pointers to scripts!
			//
			if ( ( pNextCine != this ) || ( m_spawnflags & SF_SCRIPT_REPEATABLE ) )
			{
				pNextCine->SetTarget( pNPC );
				pNextCine->StartScript();
			}
		}
	}

	//Msg("%s finished post idle at %0.2f\n", pNPC->GetDebugName(), gpGlobals->curtime );
#ifdef MAPBASE
	m_OnPostIdleEndSequence.FireOutput(pNPC, this);
#else
	m_OnPostIdleEndSequence.FireOutput(NULL, this);
#endif
}


//-----------------------------------------------------------------------------
// Purpose: When a NPC finishes a scripted sequence, we have to fix up its state
//			and schedule for it to return to a normal AI NPC.
//			Scripted sequences just dirty the Schedule and drop the NPC in Idle State.
// Input  : *pNPC - 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::FixScriptNPCSchedule( CAI_BaseNPC *pNPC, int iSavedCineFlags )
{
	if ( pNPC->GetIdealState() != NPC_STATE_DEAD )
	{
		pNPC->SetIdealState( NPC_STATE_IDLE );
	}

	if ( pNPC == NULL )
		 return;

	FixFlyFlag( pNPC, iSavedCineFlags );

	pNPC->ClearSchedule( "Finished scripted sequence" );
}

void CAI_ScriptedSequence::FixFlyFlag( CAI_BaseNPC *pNPC, int iSavedCineFlags )
{
	//Adrian: We NEED to clear this or the NPC's FL_FLY flag will never be removed cause of ClearSchedule!
	if ( pNPC->GetTask() && ( pNPC->GetTask()->iTask == TASK_PLAY_SCRIPT || pNPC->GetTask()->iTask == TASK_PLAY_SCRIPT_POST_IDLE ) )
	{
		if ( !(iSavedCineFlags & FL_FLY) )
		{
			if ( pNPC->GetFlags() & FL_FLY )
			{
				 pNPC->RemoveFlag( FL_FLY );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fAllow - 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::AllowInterrupt( bool fAllow )
{
	if ( m_spawnflags & SF_SCRIPT_NOINTERRUPT )
		return;
	m_interruptable = fAllow;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::CanInterrupt( void )
{
	if ( !m_interruptable )
		return false;

	CBaseEntity *pTarget = GetTarget();

	if ( pTarget != NULL && pTarget->IsAlive() )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::RemoveIgnoredConditions( void )
{
	if ( CanInterrupt() ) 
	{
		return;
	}

	CBaseEntity *pEntity = GetTarget();
	if ( pEntity == NULL )
	{
		return;
	}

	CAI_BaseNPC	*pTarget = pEntity->MyNPCPointer();
	if ( pTarget == NULL )
	{
		return;
	}


	if ( pTarget )
	{
		pTarget->ClearCondition(COND_LIGHT_DAMAGE);
		pTarget->ClearCondition(COND_HEAVY_DAMAGE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if another script is allowed to enqueue itself after
//			this script. The enqueued script will begin immediately after the
//			current script without dropping the NPC into AI.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSequence::CanEnqueueAfter( void )
{
	if ( m_hNextCine == NULL )
	{
		return true;
	}

	if ( m_iszNextScript != NULL_STRING )
	{
		ScriptMsg1( 2, "%s is specified as the 'Next Script' and cannot be kicked out of the queue\n",  m_hNextCine->GetDebugName() );
		return false;
	}

	if ( !m_hNextCine->HasSpawnFlags( SF_SCRIPT_HIGH_PRIORITY ) )
	{
		return true;
	}

	ScriptMsg1( 2, "%s is a priority script and cannot be kicked out of the queue\n",  m_hNextCine->GetDebugName() );

	return false;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::StopActionLoop( bool bStopSynchronizedScenes )
{
	// Stop looping our action sequence. Next time the loop finishes,
	// we'll move to the post idle sequence instead.
	m_bLoopActionSequence = false;

	// If we have synchronized scenes, and we're supposed to stop them, do so
	if ( !bStopSynchronizedScenes || GetEntityName() == NULL_STRING )
		return;

	CBaseEntity *pentCine = gEntList.FindEntityByName( NULL, GetEntityName(), NULL );
	while ( pentCine )
	{
		CAI_ScriptedSequence *pScene = dynamic_cast<CAI_ScriptedSequence *>(pentCine);
		if ( pScene && pScene != this )
		{
			pScene->StopActionLoop( false );
		}

		pentCine = gEntList.FindEntityByName( pentCine, GetEntityName(), NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Code method of forcing a scripted sequence entity to use a particular NPC.
//			Useful when you don't know if the NPC has a unique targetname.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::ForceSetTargetEntity( CAI_BaseNPC *pTarget, bool bDontCancelOtherSequences )
{
	m_hForcedTarget = pTarget;
	m_iszEntity = m_hForcedTarget->GetEntityName(); // Not guaranteed to be unique
	m_bDontCancelOtherSequences = bDontCancelOtherSequences;
}

//-----------------------------------------------------------------------------
// Purpose: Setup this scripted sequence to maintain the desired position offset
//			to the other NPC in the scripted interaction.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::SetupInteractionPosition( CBaseEntity *pRelativeEntity, VMatrix &matDesiredLocalToWorld )
{
	m_matInteractionPosition = matDesiredLocalToWorld;
	m_hInteractionRelativeEntity = pRelativeEntity;
}

extern ConVar ai_debug_dyninteractions;
//-----------------------------------------------------------------------------
// Purpose: Modify the target AutoMovement() position before the NPC moves.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::ModifyScriptedAutoMovement( Vector *vecNewPos )
{  
   	if ( m_hInteractionRelativeEntity )
	{
		// If we have an entry animation, only do it on the entry
   		if ( m_iszEntry != NULL_STRING && !m_bIsPlayingEntry )
			return;

		Vector vecRelativeOrigin = m_hInteractionRelativeEntity->GetAbsOrigin();
		QAngle angRelativeAngles = m_hInteractionRelativeEntity->GetAbsAngles();

		CAI_BaseNPC *pNPC = m_hInteractionRelativeEntity->MyNPCPointer();
		if ( pNPC )
		{
			angRelativeAngles[YAW] = pNPC->GetInteractionYaw();
		}

		bool bDebug = ai_debug_dyninteractions.GetInt() == 2;
		if ( bDebug )
		{
			Msg("--\n%s current org: %f %f\n", m_hTargetEnt->GetDebugName(), m_hTargetEnt->GetAbsOrigin().x, m_hTargetEnt->GetAbsOrigin().y );
			Msg("%s current org: %f %f", m_hInteractionRelativeEntity->GetDebugName(), vecRelativeOrigin.x, vecRelativeOrigin.y );
		}

		CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(m_hInteractionRelativeEntity.Get());
		if ( pAnimating )
		{
			Vector vecDeltaPos;
			QAngle vecDeltaAngles;
 			pAnimating->GetSequenceMovement( pAnimating->GetSequence(), 0.0f, pAnimating->GetCycle(), vecDeltaPos, vecDeltaAngles );
 			VectorYawRotate( vecDeltaPos, pAnimating->GetLocalAngles().y, vecDeltaPos );

			if ( bDebug )
			{
				NDebugOverlay::Box( vecRelativeOrigin, -Vector(2,2,2), Vector(2,2,2), 0,255,0, 8, 0.1 );
			}
			vecRelativeOrigin -= vecDeltaPos;
			if ( bDebug )
			{
				Msg(", relative to sequence start: %f %f\n", vecRelativeOrigin.x, vecRelativeOrigin.y );
				NDebugOverlay::Box( vecRelativeOrigin, -Vector(3,3,3), Vector(3,3,3), 255,0,0, 8, 0.1 );
			}
		}

		VMatrix matInteractionPosition = m_matInteractionPosition;

#ifdef MAPBASE
		// Account for our own sequence movement
		pAnimating = m_hTargetEnt->GetBaseAnimating();
		if (pAnimating)
		{
			Vector vecDeltaPos;
			QAngle angDeltaAngles;

			pAnimating->GetSequenceMovement( pAnimating->GetSequence(), 0.0f, pAnimating->GetCycle(), vecDeltaPos, angDeltaAngles );
			if (!vecDeltaPos.IsZero())
			{
				VMatrix matLocalMovement;
				matLocalMovement.SetupMatrixOrgAngles( vecDeltaPos, angDeltaAngles );
				MatrixMultiply( m_matInteractionPosition, matLocalMovement, matInteractionPosition );
			}
		}
#endif

		// We've been asked to maintain a specific position relative to the other NPC
		// we're interacting with. Lerp towards the relative position.
 		VMatrix matMeToWorld, matLocalToWorld;
		matMeToWorld.SetupMatrixOrgAngles( vecRelativeOrigin, angRelativeAngles );
		MatrixMultiply( matMeToWorld, matInteractionPosition, matLocalToWorld );

		// Get the desired NPC position in worldspace
		Vector vecOrigin;
		QAngle angAngles;
		vecOrigin = matLocalToWorld.GetTranslation();
 		MatrixToAngles( matLocalToWorld, angAngles );

		if ( bDebug )
		{
			Msg("Desired Origin for %s: %f %f\n", m_hTargetEnt->GetDebugName(), vecOrigin.x, vecOrigin.y );
			NDebugOverlay::Axis( vecOrigin, angAngles, 5, true, 0.1 );
		}

		// Lerp to it over time
   		Vector vecToTarget = (vecOrigin - *vecNewPos);
		if ( bDebug )
		{
			Msg("Automovement's output origin: %f %f\n", (*vecNewPos).x, (*vecNewPos).y );
			Msg("Vector from automovement to desired: %f %f\n", vecToTarget.x, vecToTarget.y );
		}
		*vecNewPos += (vecToTarget * pAnimating->GetCycle());
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find all the cinematic entities with my targetname and stop them
//			from playing.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::CancelScript( void )
{
	ScriptMsg1( 2,  "Cancelling script: %s\n", STRING( m_iszPlay ));
	
	// Don't cancel matching sequences if we're asked not to, unless we didn't actually
	// succeed in starting, in which case we should always cancel. This fixes
	// dynamic interactions where an NPC was killed the same frame another NPC
	// started a dynamic interaction with him.
	bool bDontCancelOther = ((m_bDontCancelOtherSequences || HasSpawnFlags( SF_SCRIPT_ALLOW_DEATH ) )&& (m_startTime != 0));
	if ( bDontCancelOther || !GetEntityName() )
	{
		ScriptEntityCancel( this );
		return;
	}

	CBaseEntity *pentCineTarget = gEntList.FindEntityByName( NULL, GetEntityName() );

	while ( pentCineTarget )
	{
		ScriptEntityCancel( pentCineTarget );
		pentCineTarget = gEntList.FindEntityByName( pentCineTarget, GetEntityName() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Find all the cinematic entities with my targetname and tell them to
//			wait before starting. This ensures that all scripted sequences with
//			the same name are frame-synchronized.
//
//			When triggered, scripts call this first with a state of 1 to indicate that
//			they are not ready to play (while NPCs move to their cue positions, etc).
//			Once they are ready to play, they call it with a state of 0. When all
//			the scripts are ready, they all are told to start.
//
// Input  : bDelay - true means this script is not ready, false means it is ready.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::DelayStart( bool bDelay )
{
	//Msg("SSEQ: %.2f \"%s\" (%d) DelayStart( %d ). Current m_iDelay is: %d\n", gpGlobals->curtime, GetDebugName(), entindex(), bDelay, m_iDelay );
	
	if ( ai_task_pre_script.GetBool() )
	{
		if ( bDelay == m_bDelayed )
			return;
	
		m_bDelayed = bDelay;
	}

	// Without a name, we cannot synchronize with anything else
	if ( GetEntityName() == NULL_STRING )
	{
		m_iDelay = bDelay;
		m_startTime = gpGlobals->curtime;
		return;
	}

	CBaseEntity *pentCine = gEntList.FindEntityByName( NULL, GetEntityName() );

	while ( pentCine )
	{
		if ( FClassnameIs( pentCine, "scripted_sequence" ) )
		{
			CAI_ScriptedSequence *pTarget = (CAI_ScriptedSequence *)pentCine;
			if (bDelay)
			{
				// if delaying, add up the number of other scripts in the group
				m_iDelay++;

				//Msg("SSEQ: (%d) Found matching SS (%d). Incrementing MY m_iDelay to %d.\n", entindex(), pTarget->entindex(), m_iDelay );
			}
			else
			{
				// if ready, decrement each of other scripts in the group
				// members not yet delayed will decrement below zero.
				pTarget->m_iDelay--;

				//Msg("SSEQ: (%d) Found matching SS (%d). Decrementing THEIR m_iDelay to %d.\n", entindex(), pTarget->entindex(), pTarget->m_iDelay );

				// once everything is balanced, everyone will start.
				if (pTarget->m_iDelay == 0)
				{
					pTarget->m_startTime = gpGlobals->curtime;

					//Msg("SSEQ: STARTING SEQUENCE for \"%s\" (%d) (m_iDelay reached 0).\n", pTarget->GetDebugName(), pTarget->entindex() );
				}
			}
		}
		pentCine = gEntList.FindEntityByName( pentCine, GetEntityName() );
	}

	//Msg("SSEQ: Exited DelayStart() with m_iDelay of: %d.\n", m_iDelay );
}


//-----------------------------------------------------------------------------
// Purpose: Find an entity that I'm interested in and precache the sounds he'll
//			need in the sequence.
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::Activate( void )
{
	BaseClass::Activate();

	//
	// See if there is another script specified to run immediately after this one.
	//
	m_hNextCine = gEntList.FindEntityByName( NULL, m_iszNextScript );
	if ( m_hNextCine == NULL )
	{
		m_iszNextScript = NULL_STRING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSequence::DrawDebugGeometryOverlays( void )
{
	BaseClass::DrawDebugGeometryOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		if ( GetTarget() )
		{
			NDebugOverlay::HorzArrow( GetAbsOrigin(), GetTarget()->GetAbsOrigin(), 16, 0, 255, 0, 64, true, 0.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_ScriptedSequence::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"Target: %s", GetTarget() ? GetTarget()->GetDebugName() : "None" ) ;
		EntityText(text_offset,tempstr,0);
		text_offset++;

		switch (m_fMoveTo)
		{
		case CINE_MOVETO_WAIT: 
			Q_snprintf(tempstr,sizeof(tempstr),"Moveto: Wait" );
			break;
		case CINE_MOVETO_WAIT_FACING:
			Q_snprintf(tempstr,sizeof(tempstr),"Moveto: Wait Facing" );
			break;
		case CINE_MOVETO_WALK:
			Q_snprintf(tempstr,sizeof(tempstr),"Moveto: Walk to Mark" );
			break;
		case CINE_MOVETO_RUN: 
			Q_snprintf(tempstr,sizeof(tempstr),"Moveto: Run to Mark" );
			break;
		case CINE_MOVETO_CUSTOM:
			Q_snprintf(tempstr,sizeof(tempstr),"Moveto: Custom move to Mark" );
			break;
		case CINE_MOVETO_TELEPORT: 
			Q_snprintf(tempstr,sizeof(tempstr),"Moveto: Teleport to Mark" );
			break;
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Thinking: %s", m_bThinking ? "Yes" : "No" ) ;
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if ( GetEntityName() != NULL_STRING )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Delay: %d", m_iDelay ) ;
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		Q_snprintf(tempstr,sizeof(tempstr),"Start Time: %f", m_startTime ) ;
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Sequence has started: %s", m_sequenceStarted ? "Yes" : "No" ) ;
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Cancel Other Sequences: %s", m_bDontCancelOtherSequences ? "No" : "Yes" ) ;
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if ( m_bWaitForBeginSequence )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Is waiting for BeingSequence" );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		if ( m_bIsPlayingEntry )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Is playing entry" );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		if ( m_bLoopActionSequence )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Will loop action sequence" );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		if ( m_bSynchPostIdles )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Will synch post idles" );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}
	}

	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Modifies an NPC's AI state without taking it out of its AI.
//-----------------------------------------------------------------------------

class CAI_ScriptedSchedule : public CBaseEntity
{
	DECLARE_CLASS( CAI_ScriptedSchedule, CBaseEntity );
public:
	CAI_ScriptedSchedule( void );

private:

	void StartSchedule( CAI_BaseNPC *pTarget );
	void StopSchedule( CAI_BaseNPC *pTarget );
	void ScriptThink( void );

	// Input handlers
	void InputStartSchedule( inputdata_t &inputdata );
	void InputStopSchedule( inputdata_t &inputdata );
#ifdef MAPBASE
	void InputSetTarget( inputdata_t &inputdata );
#endif

	CAI_BaseNPC *FindScriptEntity(  bool bCyclic );

	//---------------------------------
	
	enum Schedule_t
	{
		SCHED_SCRIPT_NONE = 0,
		SCHED_SCRIPT_WALK_TO_GOAL,
		SCHED_SCRIPT_RUN_TO_GOAL,
		SCHED_SCRIPT_ENEMY_IS_GOAL,
		SCHED_SCRIPT_WALK_PATH_GOAL,
		SCHED_SCRIPT_RUN_PATH_GOAL,
		SCHED_SCRIPT_ENEMY_IS_GOAL_AND_RUN_TO_GOAL,
	};
	
	//---------------------------------

	EHANDLE 	m_hLastFoundEntity;
	EHANDLE		m_hActivator;		// Held from the input to allow procedural calls

	string_t 	m_iszEntity;		// Entity that is wanted for this script
	float 		m_flRadius;			// Range to search for an NPC to possess.

	string_t 	m_sGoalEnt;
	Schedule_t	m_nSchedule;
	int 		m_nForceState;
	
	bool		m_bGrabAll;

	Interruptability_t m_Interruptability;

	bool		m_bDidFireOnce;
	
	//---------------------------------

	DECLARE_DATADESC();

};

BEGIN_DATADESC( CAI_ScriptedSchedule )

	DEFINE_FIELD( m_hLastFoundEntity, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "m_flRadius" ),
	
	DEFINE_KEYFIELD( m_iszEntity, FIELD_STRING, "m_iszEntity" ),
	DEFINE_KEYFIELD( m_nSchedule, FIELD_INTEGER, "schedule" ),
	DEFINE_KEYFIELD( m_nForceState, FIELD_INTEGER, "forcestate" ),
	DEFINE_KEYFIELD( m_sGoalEnt, FIELD_STRING, "goalent" ),
	DEFINE_KEYFIELD( m_bGrabAll, FIELD_BOOLEAN, "graball" ),
	DEFINE_KEYFIELD( m_Interruptability, FIELD_INTEGER, "interruptability"),

	DEFINE_FIELD( m_bDidFireOnce, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),

	DEFINE_THINKFUNC( ScriptThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartSchedule", InputStartSchedule ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopSchedule", InputStopSchedule ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( aiscripted_schedule, CAI_ScriptedSchedule );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_ScriptedSchedule::CAI_ScriptedSchedule( void ) : m_hActivator( NULL )
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_ScriptedSchedule::ScriptThink( void )
{
	bool success = false;
	CAI_BaseNPC *pTarget;
	
	if ( !m_bGrabAll )
	{
		pTarget = FindScriptEntity( (m_spawnflags & SF_SCRIPT_SEARCH_CYCLICALLY) != 0 );
		if ( pTarget )
		{
			ScriptMsg3( 2,  "scripted_schedule \"%s\" using NPC \"%s\"(%s)\n", GetDebugName(), STRING( m_iszEntity ), pTarget->GetEntityName().ToCStr() );
			StartSchedule( pTarget );
			success = true;
		}
	}
	else
	{
		m_hLastFoundEntity = NULL;
		while ( ( pTarget = FindScriptEntity( true ) ) != NULL )
		{
			ScriptMsg3( 2,  "scripted_schedule \"%s\" using NPC \"%s\"(%s)\n", GetDebugName(), pTarget->GetEntityName().ToCStr(), STRING( m_iszEntity ) );
			StartSchedule( pTarget );
			success = true;
		}
	}
	
	if ( !success )
	{
		ScriptMsg2( 2,  "scripted_schedule \"%s\" can't find NPC \"%s\"\n", GetDebugName(), STRING( m_iszEntity ) );
		// FIXME: just trying again is bad.  This should fire an output instead.
		// FIXME: Think about puting output triggers on success true and sucess false
		// FIXME: also needs to check the result of StartSchedule(), which can fail and not complain
		SetNextThink( gpGlobals->curtime + 1.0f );
	}
	else
	{
		m_bDidFireOnce = true;
	}
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_BaseNPC *CAI_ScriptedSchedule::FindScriptEntity( bool bCyclic )
{
	CBaseEntity *pEntity = gEntList.FindEntityGenericWithin( m_hLastFoundEntity, STRING( m_iszEntity ), GetAbsOrigin(), m_flRadius, this, m_hActivator );

	while ( pEntity != NULL )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if ( pNPC && pNPC->IsAlive() && pNPC->IsInterruptable())
		{
			if ( bCyclic )
			{
				// next time this is called, start searching from the one found last time
				m_hLastFoundEntity = pNPC;
			}

			return pNPC;
		}

		pEntity = gEntList.FindEntityGenericWithin( pEntity, STRING( m_iszEntity ), GetAbsOrigin(), m_flRadius, this, NULL );
	}

	m_hLastFoundEntity = NULL;
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Make the entity carry out the scripted instructions, but without
//			destroying the NPC's state.
//-----------------------------------------------------------------------------
void CAI_ScriptedSchedule::StartSchedule( CAI_BaseNPC *pTarget )
{
	if ( pTarget == NULL )
		return;

	CBaseEntity *pGoalEnt = gEntList.FindEntityGeneric( NULL, STRING( m_sGoalEnt ), this, NULL );

	// NOTE: !!! all possible choices require a goal ent currently
	if ( !pGoalEnt ) 
	{
		CHintCriteria hintCriteria;
		hintCriteria.SetGroup( m_sGoalEnt );
		hintCriteria.SetHintType( HINT_ANY );
		hintCriteria.AddIncludePosition( pTarget->GetAbsOrigin(), FLT_MAX );
		CAI_Hint *pHint = CAI_HintManager::FindHint( pTarget->GetAbsOrigin(), hintCriteria );
		if ( !pHint )
		{
			ScriptMsg2( 1, "Can't find goal entity %s\nCan't execute script %s\n", STRING(m_sGoalEnt), GetDebugName() );
			return;
		}
		pGoalEnt = pHint;
	}
	
	static NPC_STATE forcedStatesMap[] = 
	{
		NPC_STATE_NONE,
		NPC_STATE_IDLE,
		NPC_STATE_ALERT,
		NPC_STATE_COMBAT
	};

	if ( pTarget->GetSleepState() > AISS_AWAKE )
		pTarget->Wake();
	
	pTarget->ForceDecisionThink();

	Assert( m_nForceState >= 0 && m_nForceState < ARRAYSIZE(forcedStatesMap) );
	
	NPC_STATE forcedState = forcedStatesMap[m_nForceState];

	// trap if this isn't a legal thing to do
	Assert( pTarget->IsInterruptable() );

	if ( forcedState != NPC_STATE_NONE )
		pTarget->SetState( forcedState );

	//
	// Set enemy and make the NPC aware of the enemy's current position.
	//
	if ( m_nSchedule == SCHED_SCRIPT_ENEMY_IS_GOAL || m_nSchedule == SCHED_SCRIPT_ENEMY_IS_GOAL_AND_RUN_TO_GOAL )
	{
		if ( pGoalEnt && pGoalEnt->MyCombatCharacterPointer() )
		{
			pTarget->SetEnemy( pGoalEnt );
			pTarget->UpdateEnemyMemory( pGoalEnt, pGoalEnt->GetAbsOrigin() );
			pTarget->SetCondition( COND_SCHEDULE_DONE );
		}
		else
			ScriptMsg2( 1, "Scripted schedule %s specified an invalid enemy %s\n", STRING( GetEntityName() ), STRING( m_sGoalEnt ) );
	}

	bool bDidSetSchedule = false;

	switch ( m_nSchedule )
	{
		//
		// Walk or run to position.
		//
		case SCHED_SCRIPT_WALK_TO_GOAL:
		case SCHED_SCRIPT_RUN_TO_GOAL:
		case SCHED_SCRIPT_ENEMY_IS_GOAL_AND_RUN_TO_GOAL:
		{
			Activity movementActivity = ( m_nSchedule == SCHED_SCRIPT_WALK_TO_GOAL ) ? ACT_WALK : ACT_RUN;
			bool bIsFlying = (pTarget->GetMoveType() == MOVETYPE_FLY) || (pTarget->GetMoveType() == MOVETYPE_FLYGRAVITY);
			if ( bIsFlying )
			{
				movementActivity = ACT_FLY;
			}

			if (!pTarget->ScheduledMoveToGoalEntity( SCHED_IDLE_WALK, pGoalEnt, movementActivity ))
			{
				if (!(m_spawnflags & SF_SCRIPT_NO_COMPLAINTS))
				{
					ScriptMsg2( 1, "ScheduledMoveToGoalEntity to goal entity %s failed\nCan't execute script %s\n", STRING(m_sGoalEnt), GetDebugName() );
				}
				return;
			}
			bDidSetSchedule = true;

			break;
		}

		case SCHED_SCRIPT_WALK_PATH_GOAL:
		case SCHED_SCRIPT_RUN_PATH_GOAL:
		{
			Activity movementActivity = ( m_nSchedule == SCHED_SCRIPT_WALK_PATH_GOAL ) ? ACT_WALK : ACT_RUN;
			bool bIsFlying = (pTarget->GetMoveType() == MOVETYPE_FLY) || (pTarget->GetMoveType() == MOVETYPE_FLYGRAVITY);
			if ( bIsFlying )
			{
				movementActivity = ACT_FLY;
			}
			if (!pTarget->ScheduledFollowPath( SCHED_IDLE_WALK, pGoalEnt, movementActivity ))
			{
				if (!(m_spawnflags & SF_SCRIPT_NO_COMPLAINTS))
				{
					ScriptMsg2( 1, "ScheduledFollowPath to goal entity %s failed\nCan't execute script %s\n", STRING(m_sGoalEnt), GetDebugName() );
				}
				return;
			}
			bDidSetSchedule = true;
			break;
		}
	}

	if ( bDidSetSchedule )
	{
		// Chain this to the target so that it can add the base and any custom interrupts to this
		pTarget->SetScriptedScheduleIgnoreConditions( m_Interruptability );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to activate the scripted schedule. Finds the NPC to
//			act on and sets a think for the near future to do the real work.
//-----------------------------------------------------------------------------
void CAI_ScriptedSchedule::InputStartSchedule( inputdata_t &inputdata )
{
	if (( m_nForceState == 0 ) && ( m_nSchedule == 0 ))
	{
		ScriptMsg( 2,  "aiscripted_schedule - no schedule or state has been set!\n" );
	}
	
	if ( !m_bDidFireOnce || ( m_spawnflags & SF_SCRIPT_REPEATABLE ) )
	{
		// DVS TODO: Is the NPC already playing the script?
		m_hActivator = inputdata.pActivator;
		SetThink( &CAI_ScriptedSchedule::ScriptThink );
		SetNextThink( gpGlobals->curtime );
	}
	else
	{
		ScriptMsg( 2, "aiscripted_schedule - not playing schedule again: not flagged to repeat\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler to stop a previously activated scripted schedule. 
//-----------------------------------------------------------------------------
void CAI_ScriptedSchedule::InputStopSchedule( inputdata_t &inputdata )
{
	if ( !m_bDidFireOnce )
	{
		ScriptMsg( 2, "aiscripted_schedule - StopSchedule called, but schedule's never started.\n" );
		return;
	}

	CAI_BaseNPC *pTarget;
	if ( !m_bGrabAll )
	{
		pTarget = FindScriptEntity( (m_spawnflags & SF_SCRIPT_SEARCH_CYCLICALLY) != 0 );
		if ( pTarget )
		{
			StopSchedule( pTarget );
		}
	}
	else
	{
		m_hLastFoundEntity = NULL;
		while ( ( pTarget = FindScriptEntity( true ) ) != NULL )
		{
			StopSchedule( pTarget );
		}
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Sets our target NPC with the generic SetTarget input.
//-----------------------------------------------------------------------------
void CAI_ScriptedSchedule::InputSetTarget( inputdata_t &inputdata )
{
	m_hActivator = inputdata.pActivator;
	m_iszEntity = AllocPooledString( inputdata.value.String() );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: If the target entity appears to be running this scripted schedule break it
//-----------------------------------------------------------------------------
void CAI_ScriptedSchedule::StopSchedule( CAI_BaseNPC *pTarget )
{
	if ( pTarget->IsCurSchedule( SCHED_IDLE_WALK ) )
	{
		ScriptMsg3( 2, "%s (%s): StopSchedule called on NPC %s.\n", GetClassname(), GetDebugName(), pTarget->GetDebugName() );
		pTarget->ClearSchedule( "Stopping scripted schedule" );
	}
}

class CAI_ScriptedSentence : public CPointEntity
{
public:
	DECLARE_CLASS( CAI_ScriptedSentence, CPointEntity );

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void FindThink( void );
	void DelayThink( void );
	int	 ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	// Input handlers
	void InputBeginSentence( inputdata_t &inputdata );

	DECLARE_DATADESC();

	CAI_BaseNPC *FindEntity( void );
	bool AcceptableSpeaker( CAI_BaseNPC *pNPC );
	int StartSentence( CAI_BaseNPC *pTarget );

private:
	string_t m_iszSentence;		// string index for sentence name
	string_t m_iszEntity;	// entity that is wanted for this sentence
	float	m_flRadius;		// range to search
	float	m_flDelay;	// How long the sentence lasts
	float	m_flRepeat;	// repeat rate
	soundlevel_t	m_iSoundLevel;
	int		m_TempAttenuation;
	float	m_flVolume;
	bool	m_active;
	string_t m_iszListener;	// name of entity to look at while talking
	CBaseEntity *m_pActivator;

	COutputEvent m_OnBeginSentence;
	COutputEvent m_OnEndSentence;
};


#define SF_SENTENCE_ONCE				0x0001
#define SF_SENTENCE_FOLLOWERS			0x0002	// only say if following player
#define SF_SENTENCE_INTERRUPT			0x0004	// force talking except when dead
#define SF_SENTENCE_CONCURRENT			0x0008	// allow other people to keep talking
#define SF_SENTENCE_SPEAKTOACTIVATOR	0x0010

BEGIN_DATADESC( CAI_ScriptedSentence )

	DEFINE_KEYFIELD( m_iszSentence, FIELD_STRING, "sentence" ),
	DEFINE_KEYFIELD( m_iszEntity, FIELD_STRING, "entity" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_flDelay, FIELD_FLOAT, "delay" ),
	DEFINE_KEYFIELD( m_flRepeat, FIELD_FLOAT, "refire" ),
	DEFINE_KEYFIELD( m_iszListener, FIELD_STRING, "listener" ),

	DEFINE_KEYFIELD( m_TempAttenuation, FIELD_INTEGER, "attenuation" ),

	DEFINE_FIELD( m_iSoundLevel, FIELD_INTEGER ),
	DEFINE_FIELD( m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( m_active, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_pActivator, FIELD_EHANDLE ),

	// Function Pointers
	DEFINE_FUNCTION( FindThink ),
	DEFINE_FUNCTION( DelayThink ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "BeginSentence", InputBeginSentence),

	// Outputs
	DEFINE_OUTPUT(m_OnBeginSentence, "OnBeginSentence"),
	DEFINE_OUTPUT(m_OnEndSentence, "OnEndSentence"),

END_DATADESC()



LINK_ENTITY_TO_CLASS( scripted_sentence, CAI_ScriptedSentence );


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKeyName - 
//			szValue - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSentence::KeyValue( const char *szKeyName, const char *szValue )
{
	if(FStrEq(szKeyName, "volume"))
	{
		m_flVolume = atof( szValue ) * 0.1;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for starting the scripted sentence.
//-----------------------------------------------------------------------------
void CAI_ScriptedSentence::InputBeginSentence( inputdata_t &inputdata )
{
	if ( !m_active )
		return;

	m_pActivator = inputdata.pActivator;

	//Msg( "Firing sentence: %s\n", STRING( m_iszSentence ));
	SetThink( &CAI_ScriptedSentence::FindThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSentence::Spawn( void )
{
	SetSolid( SOLID_NONE );
	
	m_active = true;
	// if no targetname, start now
	if ( !GetEntityName() )
	{
		SetThink( &CAI_ScriptedSentence::FindThink );
		SetNextThink( gpGlobals->curtime + 1.0f );
	}

	switch( m_TempAttenuation )
	{
	case 1: // Medium radius
		m_iSoundLevel = SNDLVL_80dB;
		break;
	
	case 2:	// Large radius
		m_iSoundLevel = SNDLVL_85dB;
		break;

	case 3:	//EVERYWHERE
		m_iSoundLevel = SNDLVL_NONE;
		break;
	
	default:
	case 0: // Small radius
		m_iSoundLevel = SNDLVL_70dB;
		break;
	}
	m_TempAttenuation = 0;

	// No volume, use normal
	if ( m_flVolume <= 0 )
		m_flVolume = 1.0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSentence::FindThink( void )
{
	CAI_BaseNPC *pNPC = FindEntity();
	if ( pNPC )
	{
		int index = StartSentence( pNPC );
		float length = engine->SentenceLength(index);
		
		m_OnEndSentence.FireOutput(NULL, this, length + m_flRepeat);

		if ( m_spawnflags & SF_SENTENCE_ONCE )
			UTIL_Remove( this );
		
		float delay = m_flDelay + length + 0.1;
		if ( delay < 0 )
			delay = 0;

		SetThink( &CAI_ScriptedSentence::DelayThink );
		// calculate delay dynamically because this could play a sentence group
		// rather than a single sentence.
		// add 0.1 because the sound engine mixes ahead -- the sentence will actually start ~0.1 secs from now
		SetNextThink( gpGlobals->curtime + delay + m_flRepeat );
		m_active = false;
		//Msg( "%s: found NPC %s\n", STRING(m_iszSentence), STRING(m_iszEntity) );
	}
	else
	{
		//Msg( "%s: can't find NPC %s\n", STRING(m_iszSentence), STRING(m_iszEntity) );
		SetNextThink( gpGlobals->curtime + m_flRepeat + 0.5 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ScriptedSentence::DelayThink( void )
{
	m_active = true;
	if ( !GetEntityName() )
		SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink( &CAI_ScriptedSentence::FindThink );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pNPC - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_ScriptedSentence::AcceptableSpeaker( CAI_BaseNPC *pNPC )
{
	if ( pNPC )
	{
		if ( m_spawnflags & SF_SENTENCE_FOLLOWERS )
		{
			if ( pNPC->GetTarget() == NULL || !pNPC->GetTarget()->IsPlayer() )
				return false;
		}
		bool override;
		if ( m_spawnflags & SF_SENTENCE_INTERRUPT )
			override = true;
		else
			override = false;
		if ( pNPC->CanPlaySentence( override ) )
			return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CAI_BaseNPC *CAI_ScriptedSentence::FindEntity( void )
{
	CBaseEntity *pentTarget;
	CAI_BaseNPC *pNPC;

	pentTarget = gEntList.FindEntityByName( NULL, m_iszEntity );
	pNPC = NULL;

	while (pentTarget)
	{
		pNPC = pentTarget->MyNPCPointer();
		if ( pNPC != NULL )
		{
			if ( AcceptableSpeaker( pNPC ) )
				return pNPC;
			//Msg( "%s (%s), not acceptable\n", pNPC->GetClassname(), pNPC->GetDebugName() );
		}
		pentTarget = gEntList.FindEntityByName( pentTarget, m_iszEntity );
	}
	
	CBaseEntity *pEntity = NULL;
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), m_flRadius, FL_NPC ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if (FClassnameIs( pEntity, STRING(m_iszEntity)))
		{
			pNPC = pEntity->MyNPCPointer( );
			if ( AcceptableSpeaker( pNPC ) )
				return pNPC;
		}
	}
	
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTarget - 
// Output : 
//-----------------------------------------------------------------------------
int CAI_ScriptedSentence::StartSentence( CAI_BaseNPC *pTarget )
{
	if ( !pTarget )
	{
		ScriptMsg1( 2, "Not Playing sentence %s\n", STRING(m_iszSentence) );
		return -1;
	}

	bool bConcurrent = false;
	if ( !(m_spawnflags & SF_SENTENCE_CONCURRENT) )
		bConcurrent = true;

	CBaseEntity *pListener = NULL;

	if ( m_spawnflags & SF_SENTENCE_SPEAKTOACTIVATOR )
	{
		pListener = m_pActivator;
	}
	else if (m_iszListener != NULL_STRING)
	{
		float radius = m_flRadius;

		if ( FStrEq( STRING(m_iszListener ), "!player" ) )
			radius = MAX_TRACE_LENGTH;	// Always find the player

		pListener = gEntList.FindEntityGenericNearest( STRING( m_iszListener ), pTarget->GetAbsOrigin(), radius, this, NULL );
	}

	int sentenceIndex = pTarget->PlayScriptedSentence( STRING(m_iszSentence), m_flDelay,  m_flVolume, m_iSoundLevel, bConcurrent, pListener );
	ScriptMsg1( 2, "Playing sentence %s\n", STRING(m_iszSentence) );

	m_OnBeginSentence.FireOutput(NULL, this);

	return sentenceIndex;
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// This isn't exclusive to NPCs, so it could be moved if needed.
//-----------------------------------------------------------------------------
class CScriptedSound : public CPointEntity
{
public:
	DECLARE_CLASS( CScriptedSound, CPointEntity );
	DECLARE_DATADESC();

	void Precache();

	CBaseEntity *GetTarget(inputdata_t &inputdata);

	// Input handlers
	void InputPlaySound( inputdata_t &inputdata );
	void InputPlaySoundOnEntity( inputdata_t &inputdata );
	void InputStopSound( inputdata_t &inputdata );
	void InputSetSound( inputdata_t &inputdata );

private:
	string_t m_message;

	bool m_bGrabAll;
};


BEGIN_DATADESC( CScriptedSound )

	DEFINE_KEYFIELD( m_message, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( m_bGrabAll, FIELD_BOOLEAN, "GrabAll" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "PlaySound", InputPlaySound),
	DEFINE_INPUTFUNC(FIELD_EHANDLE, "PlaySoundOnEntity", InputPlaySoundOnEntity),
	DEFINE_INPUTFUNC(FIELD_VOID, "StopSound", InputStopSound),
	DEFINE_INPUTFUNC(FIELD_STRING, "SetSound", InputSetSound),

END_DATADESC()



LINK_ENTITY_TO_CLASS( scripted_sound, CScriptedSound );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CScriptedSound::Precache()
{
	//PrecacheScriptSound(STRING(m_message));

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
CBaseEntity *CScriptedSound::GetTarget(inputdata_t &inputdata)
{
	CBaseEntity *pEntity = NULL;
	if (m_target == NULL_STRING)
	{
		// Use this as the default source entity
		pEntity = this;
		m_bGrabAll = false;
	}
	else
	{
		pEntity = gEntList.FindEntityGeneric(NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
	}

	return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CScriptedSound::InputPlaySound( inputdata_t &inputdata )
{
	PrecacheScriptSound(STRING(m_message));

	CBaseEntity *pEntity = GetTarget(inputdata);
	const char *sound = STRING(m_message);
	if (m_bGrabAll)
	{
		//if (pEntity)
		//{
		//	pEntity->PrecacheScriptSound(sound);
		//}

		while (pEntity)
		{
			pEntity->EmitSound(sound);
			pEntity = gEntList.FindEntityGeneric(pEntity, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
		}
	}
	else if (pEntity)
	{
		//pEntity->PrecacheScriptSound(sound);
		pEntity->EmitSound(sound);
	}
	else
	{
		Warning("%s unable to find target entity %s!\n", GetDebugName(), STRING(m_target));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CScriptedSound::InputPlaySoundOnEntity( inputdata_t &inputdata )
{
	if (inputdata.value.Entity())
	{
		inputdata.value.Entity()->PrecacheScriptSound(STRING(m_message));
		inputdata.value.Entity()->EmitSound(STRING(m_message));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CScriptedSound::InputStopSound( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = GetTarget(inputdata);
	const char *sound = STRING(m_message);
	if (m_bGrabAll)
	{
		while (pEntity)
		{
			pEntity->StopSound(sound);
			pEntity = gEntList.FindEntityGeneric(pEntity, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);
		}
	}
	else if (pEntity)
	{
		pEntity->StopSound(sound);
	}
	else
	{
		Warning("%s unable to find target entity %s!\n", GetDebugName(), STRING(m_target));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : 
//-----------------------------------------------------------------------------
void CScriptedSound::InputSetSound( inputdata_t &inputdata )
{
	PrecacheScriptSound(inputdata.value.String());
	m_message = inputdata.value.StringID();
}
#endif



// HACKHACK: This is a little expensive with the dynamic_cast<> and all, but it lets us solve
// the problem of matching scripts back to entities without new state.
const char *CAI_ScriptedSequence::GetSpawnPreIdleSequenceForScript( CBaseEntity *pEntity )
{
	CAI_ScriptedSequence *pScript = gEntList.NextEntByClass( (CAI_ScriptedSequence *)NULL );
	while ( pScript )
	{
		if ( pScript->HasSpawnFlags( SF_SCRIPT_START_ON_SPAWN ) && pScript->m_iszEntity == pEntity->GetEntityName() )
		{
			if ( pScript->m_iszPreIdle != NULL_STRING )
			{
				return STRING(pScript->m_iszPreIdle);
			}
			return NULL;
		}
		pScript = gEntList.NextEntByClass( pScript );
	}
	return NULL;
}

