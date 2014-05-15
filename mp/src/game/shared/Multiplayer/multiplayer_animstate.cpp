//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "multiplayer_animstate.h"
#include "activitylist.h"

#ifdef CLIENT_DLL
#include "c_baseplayer.h"
#include "engine/ivdebugoverlay.h"
#include "filesystem.h"
#include "eventlist.h"
ConVar anim_showmainactivity( "anim_showmainactivity", "0", FCVAR_CHEAT, "Show the idle, walk, run, and/or sprint activities." );
#else
#include "player.h"
#endif

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
#include "tf_gamerules.h"
#endif

#ifndef CALL_ATTRIB_HOOK_FLOAT_ON_OTHER
#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( o, r, n )
#endif

#define MOVING_MINIMUM_SPEED	0.5f

ConVar anim_showstate( "anim_showstate", "-1", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Show the (client) animation state for the specified entity (-1 for none)." );
ConVar anim_showstatelog( "anim_showstatelog", "0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "1 to output anim_showstate to Msg(). 2 to store in AnimState.log. 3 for both." );
ConVar mp_showgestureslots( "mp_showgestureslots", "-1", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Show multiplayer client/server gesture slot information for the specified player index (-1 for no one)." );
ConVar mp_slammoveyaw( "mp_slammoveyaw", "0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Force movement yaw along an animation path." );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CMultiPlayerAnimState::CMultiPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
#ifdef CLIENT_DLL
	: m_iv_flMaxGroundSpeed( "CMultiPlayerAnimState::m_iv_flMaxGroundSpeed" )
#endif
{
	// Pose parameters.
	m_bPoseParameterInit = false;
	m_PoseParameterData.Init();
	m_DebugAnimData.Init();

	m_pPlayer = NULL;
	m_angRender.Init();

	m_bCurrentFeetYawInitialized = false;
	m_flLastAnimationStateClearTime = 0.0f;

	m_flEyeYaw = 0.0f;
	m_flEyePitch = 0.0f;
	m_flGoalFeetYaw = 0.0f;
	m_flCurrentFeetYaw = 0.0f;
	m_flLastAimTurnTime = 0.0f;

	// Jumping.
	m_bJumping = false;
	m_flJumpStartTime = 0.0f;	
	m_bFirstJumpFrame = false;

	// Swimming
	m_bInSwim = false;
	m_bFirstSwimFrame = true;

	// Dying
	m_bDying = false;
	m_bFirstDyingFrame = true;

	m_eCurrentMainSequenceActivity = ACT_INVALID;	
	m_nSpecificMainSequence = -1;

	// Weapon data.
	m_hActiveWeapon = NULL;

	// Ground speed interpolators.
#ifdef CLIENT_DLL
	m_iv_flMaxGroundSpeed.Setup( &m_flMaxGroundSpeed, LATCH_ANIMATION_VAR | INTERPOLATE_LINEAR_ONLY );
	m_flLastGroundSpeedUpdateTime = 0.0f;
#endif

	m_flMaxGroundSpeed = 0.0f;

	m_bForceAimYaw = false;

	Init( pPlayer, movementData );

	// movement playback options
	m_nMovementSequence = -1;
	m_LegAnimType = LEGANIM_9WAY;

	InitGestureSlots();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CMultiPlayerAnimState::~CMultiPlayerAnimState()
{
	ShutdownGestureSlots();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::Init( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
{
	// Get the player this animation data works on.
	m_pPlayer = pPlayer;

	// Copy the movement data.
	memcpy( &m_MovementData, &movementData, sizeof( MultiPlayerMovementData_t ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ClearAnimationState()
{
	// Reset state.
	m_bJumping = false;
	m_bDying = false;
	m_bCurrentFeetYawInitialized = false;
	m_flLastAnimationStateClearTime = gpGlobals->curtime;
	m_nSpecificMainSequence = -1;

	ResetGestureSlots();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	switch( event )
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
			// Weapon primary fire.
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );
			break;
		}
	case PLAYERANIMEVENT_ATTACK_SECONDARY:
		{
			// Weapon secondary fire.
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
			break;
		}
	case PLAYERANIMEVENT_ATTACK_GRENADE:
		{
			// Grenade throw.
			RestartGesture( GESTURE_SLOT_GRENADE, ACT_MP_ATTACK_STAND_GRENADE );
			break;
		}
	case PLAYERANIMEVENT_RELOAD:
		{
			// Weapon reload.
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH );
			}
			else if ( m_bInSwim )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_SWIM );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND );
			}


			// Set the modified reload playback rate
			float flPlaybackRate = 1.0f;
			#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, mult_reload_time );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, mult_reload_time_hidden );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, fast_reload );
			#endif
			m_aGestureSlots[ GESTURE_SLOT_ATTACK_AND_RELOAD ].m_pAnimLayer->m_flPlaybackRate = flPlaybackRate;

			break;
		}
	case PLAYERANIMEVENT_RELOAD_LOOP:
		{
			// Weapon reload.
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP );
			}
			else if ( m_bInSwim )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_SWIM_LOOP );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP );
			}

			// Set the modified reload playback rate
			float flPlaybackRate = 1.0f;
			#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, mult_reload_time );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, mult_reload_time_hidden );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, fast_reload );
			#endif
			m_aGestureSlots[ GESTURE_SLOT_ATTACK_AND_RELOAD ].m_pAnimLayer->m_flPlaybackRate = flPlaybackRate;

			break;
		}
	case PLAYERANIMEVENT_RELOAD_END:
		{
			// Weapon reload.
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END );
			}
			else if ( m_bInSwim )
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_SWIM_END );
			}
			else
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END );
			}

			// Set the modified reload playback rate
			float flPlaybackRate = 1.0f;
			#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, mult_reload_time );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, mult_reload_time_hidden );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetBasePlayer(), flPlaybackRate, fast_reload );
			#endif
			m_aGestureSlots[ GESTURE_SLOT_ATTACK_AND_RELOAD ].m_pAnimLayer->m_flPlaybackRate = flPlaybackRate;

			break;
		}
	case PLAYERANIMEVENT_JUMP:
		{
			// Jump.
			m_bJumping = true;
			m_bFirstJumpFrame = true;
			m_flJumpStartTime = gpGlobals->curtime;

			RestartMainSequence();

			break;
		}
	case PLAYERANIMEVENT_DIE:
		{
			// Should be here - not supporting this yet!
			Assert( 0 );

			// Start playing the death animation
			m_bDying = true;

			RestartMainSequence();
			break;
		}
	case PLAYERANIMEVENT_SPAWN:
		{
			// Player has respawned. Clear flags.
			ClearAnimationState();
			break;
		}

	case PLAYERANIMEVENT_SNAP_YAW:
		m_PoseParameterData.m_flLastAimTurnTime = 0.0f;
		break;

	case PLAYERANIMEVENT_CUSTOM:
		{
			Activity iIdealActivity = TranslateActivity( (Activity)nData );
			m_nSpecificMainSequence = GetBasePlayer()->SelectWeightedSequence( iIdealActivity );
			RestartMainSequence();
		}
		break;

	case PLAYERANIMEVENT_CUSTOM_GESTURE:
		// Weapon primary fire.
		RestartGesture( GESTURE_SLOT_CUSTOM, (Activity)nData );
		break;

	case PLAYERANIMEVENT_CUSTOM_SEQUENCE:
		m_nSpecificMainSequence = nData;
		RestartMainSequence();
		break;

	case PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE:
		// Weapon primary fire.
//		RestartGestureSequence( nData, false );
		break;

	case PLAYERANIMEVENT_FLINCH_CHEST:
		PlayFlinchGesture( ACT_MP_GESTURE_FLINCH_CHEST );
		break;
	case PLAYERANIMEVENT_FLINCH_HEAD:
		PlayFlinchGesture( ACT_MP_GESTURE_FLINCH_HEAD );
		break;
	case PLAYERANIMEVENT_FLINCH_LEFTARM:
		PlayFlinchGesture( ACT_MP_GESTURE_FLINCH_LEFTARM );
		break;
	case PLAYERANIMEVENT_FLINCH_RIGHTARM:
		PlayFlinchGesture( ACT_MP_GESTURE_FLINCH_RIGHTARM );
		break;
	case PLAYERANIMEVENT_FLINCH_LEFTLEG:
		PlayFlinchGesture( ACT_MP_GESTURE_FLINCH_LEFTLEG );
		break;
	case PLAYERANIMEVENT_FLINCH_RIGHTLEG:
		PlayFlinchGesture( ACT_MP_GESTURE_FLINCH_RIGHTLEG );
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::PlayFlinchGesture( Activity iActivity )
{
	if ( !IsGestureSlotActive( GESTURE_SLOT_FLINCH ) )
	{
		// See if we have the custom flinch. If not, revert to chest
		if ( iActivity != ACT_MP_GESTURE_FLINCH_CHEST && GetBasePlayer()->SelectWeightedSequence( iActivity ) == -1 )
		{
			RestartGesture( GESTURE_SLOT_FLINCH, ACT_MP_GESTURE_FLINCH_CHEST );
		}
		else
		{
			RestartGesture( GESTURE_SLOT_FLINCH, iActivity );
		}
	}
}

//=============================================================================
//
// Multiplayer gesture code.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::InitGestureSlots( void )
{
	// Setup the number of gesture slots.
	m_aGestureSlots.AddMultipleToTail( GESTURE_SLOT_COUNT );

	// Assign all of the the CAnimationLayer pointers to null early in case we bail.
	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		m_aGestureSlots[iGesture].m_pAnimLayer = NULL;
	}

	// Get the base player.
	CBasePlayer *pPlayer = GetBasePlayer();

	// Set the number of animation overlays we will use.
	pPlayer->SetNumAnimOverlays( GESTURE_SLOT_COUNT );

	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		m_aGestureSlots[iGesture].m_pAnimLayer = pPlayer->GetAnimOverlay( iGesture );
		if ( !m_aGestureSlots[iGesture].m_pAnimLayer )
			return false;

		ResetGestureSlot( iGesture );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ShutdownGestureSlots( void )
{
	// Clean up the gesture slots.
	m_aGestureSlots.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ResetGestureSlots( void )
{
	// Clear out all the gesture slots.
	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		ResetGestureSlot( iGesture );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ResetGestureSlot( int iGestureSlot )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	if ( !VerifyAnimLayerInSlot( iGestureSlot ) )
		return;

	GestureSlot_t *pGestureSlot = &m_aGestureSlots[iGestureSlot];
	if ( pGestureSlot )
	{
#ifdef CLIENT_DLL
		// briefly set to 1.0 so we catch the events, before we reset the slot
		pGestureSlot->m_pAnimLayer->m_flCycle = 1.0;

		RunGestureSlotAnimEventsToCompletion( pGestureSlot );
#endif

		pGestureSlot->m_iGestureSlot = GESTURE_SLOT_INVALID;
		pGestureSlot->m_iActivity = ACT_INVALID;
		pGestureSlot->m_bAutoKill = false;
		pGestureSlot->m_bActive = false;
		if ( pGestureSlot->m_pAnimLayer )
		{
			pGestureSlot->m_pAnimLayer->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#ifdef CLIENT_DLL
			pGestureSlot->m_pAnimLayer->Reset();
#endif
		}
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::RunGestureSlotAnimEventsToCompletion( GestureSlot_t *pGesture )
{
	CBasePlayer *pPlayer = GetBasePlayer();
	if( !pPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Do all the anim events between previous cycle and 1.0, inclusive
	mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc( pGesture->m_pAnimLayer->m_nSequence );
	if ( seqdesc.numevents > 0 )
	{
		mstudioevent_t *pevent = seqdesc.pEvent( 0 );

		for (int i = 0; i < (int)seqdesc.numevents; i++)
		{
			if ( pevent[i].type & AE_TYPE_NEWEVENTSYSTEM )
			{
				if ( !( pevent[i].type & AE_TYPE_CLIENT ) )
					continue;
			}
			else if ( pevent[i].event < 5000 ) //Adrian - Support the old event system
				continue;

			if ( pevent[i].cycle > pGesture->m_pAnimLayer->m_flPrevCycle &&
				pevent[i].cycle <= pGesture->m_pAnimLayer->m_flCycle )
			{
				pPlayer->FireEvent( pPlayer->GetAbsOrigin(), pPlayer->GetAbsAngles(), pevent[ i ].event, pevent[ i ].pszOptions() );
			}
		}
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::IsGestureSlotActive( int iGestureSlot )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );
	return m_aGestureSlots[iGestureSlot].m_bActive;
}


//-----------------------------------------------------------------------------
// Purpose: Track down a crash
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::VerifyAnimLayerInSlot( int iGestureSlot )
{
	if ( iGestureSlot < 0 || iGestureSlot >= GESTURE_SLOT_COUNT )
	{
		return false;
	}

	if ( GetBasePlayer()->GetNumAnimOverlays() < iGestureSlot + 1 )
	{
		AssertMsg2( false, "Player %d doesn't have gesture slot %d any more.", GetBasePlayer()->entindex(), iGestureSlot );
		Msg( "Player %d doesn't have gesture slot %d any more.\n", GetBasePlayer()->entindex(), iGestureSlot );
		m_aGestureSlots[iGestureSlot].m_pAnimLayer = NULL;
		return false;
	}

	CAnimationLayer *pExpected = GetBasePlayer()->GetAnimOverlay( iGestureSlot );
	if ( m_aGestureSlots[iGestureSlot].m_pAnimLayer != pExpected )
	{
		AssertMsg3( false, "Gesture slot %d pointing to wrong address %p. Updating to new address %p.", iGestureSlot, m_aGestureSlots[iGestureSlot].m_pAnimLayer, pExpected );
		Msg( "Gesture slot %d pointing to wrong address %p. Updating to new address %p.\n", iGestureSlot, m_aGestureSlots[iGestureSlot].m_pAnimLayer, pExpected );
		m_aGestureSlots[iGestureSlot].m_pAnimLayer = pExpected;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::IsGestureSlotPlaying( int iGestureSlot, Activity iGestureActivity )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	// Check to see if the slot is active.
	if ( !IsGestureSlotActive( iGestureSlot ) )
		return false;

	return ( m_aGestureSlots[iGestureSlot].m_iActivity == iGestureActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::RestartGesture( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );
	
	if ( !VerifyAnimLayerInSlot( iGestureSlot ) )
			return;

	if ( !IsGestureSlotPlaying( iGestureSlot, iGestureActivity ) )
	{
#ifdef CLIENT_DLL
		if ( IsGestureSlotActive( iGestureSlot ) )
		{
			GestureSlot_t *pGesture = &m_aGestureSlots[iGestureSlot];
			if ( pGesture && pGesture->m_pAnimLayer )
			{
				pGesture->m_pAnimLayer->m_flCycle = 1.0; // run until the end
				RunGestureSlotAnimEventsToCompletion( &m_aGestureSlots[iGestureSlot] );
			}
		}
#endif

		Activity iIdealGestureActivity = TranslateActivity( iGestureActivity );
		AddToGestureSlot( iGestureSlot, iIdealGestureActivity, bAutoKill );
		return;
	}

	// Reset the cycle = restart the gesture.
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::AddToGestureSlot( int iGestureSlot, Activity iGestureActivity, bool bAutoKill )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	CBasePlayer *pPlayer = GetBasePlayer();
	if ( !pPlayer )
		return;

	// Make sure we have a valid animation layer to fill out.
	if ( !m_aGestureSlots[iGestureSlot].m_pAnimLayer )
		return;

	if ( !VerifyAnimLayerInSlot( iGestureSlot ) )
		return;

	// Get the sequence.
	int iGestureSequence = pPlayer->SelectWeightedSequence( iGestureActivity );
	if ( iGestureSequence <= 0 )
		return;

#ifdef CLIENT_DLL 

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerAnimtime = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerFadeOuttime = 0.0f;

	pPlayer->m_flOverlayPrevEventCycle[iGestureSlot] = -1.0;

#else

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nPriority = 0;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendIn = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendOut = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bSequenceFinished = false;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = gpGlobals->curtime;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bLooping = false;//( ( GetSequenceFlags( GetModelPtr(), iGestureSequence ) & STUDIO_LOOPING ) != 0);
	if ( bAutoKill )
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_AUTOKILL;
	}
	else
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags &= ~ANIM_LAYER_AUTOKILL;
	}
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_ACTIVE;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::AddVCDSequenceToGestureSlot( int iGestureSlot, int iGestureSequence, float flCycle, bool bAutoKill )
{
	// Sanity Check
	Assert( iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT );

	CBasePlayer *pPlayer = GetBasePlayer();
	if ( !pPlayer )
		return;

	// Make sure we have a valid animation layer to fill out.
	if ( !m_aGestureSlots[iGestureSlot].m_pAnimLayer )
		return;

	if ( !VerifyAnimLayerInSlot( iGestureSlot ) )
		return;

	// Set the activity.
	Activity iGestureActivity = ACT_MP_VCD;

#ifdef CLIENT_DLL 

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = flCycle;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerAnimtime = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerFadeOuttime = 0.0f;

	pPlayer->m_flOverlayPrevEventCycle[iGestureSlot] = -1.0;

#else

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nPriority = 0;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = flCycle;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendIn = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendOut = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bSequenceFinished = false;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = gpGlobals->curtime;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bLooping = false;//( ( GetSequenceFlags( GetModelPtr(), iGestureSequence ) & STUDIO_LOOPING ) != 0);
	if ( bAutoKill )
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_AUTOKILL;
	}
	else
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags &= ~ANIM_LAYER_AUTOKILL;
	}
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_ACTIVE;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAnimationLayer* CMultiPlayerAnimState::GetGestureSlotLayer( int iGestureSlot )
{
	return m_aGestureSlots[iGestureSlot].m_pAnimLayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ShowDebugInfo( void )
{
	if ( anim_showstate.GetInt() == GetBasePlayer()->entindex() )
	{
		DebugShowAnimStateForPlayer( GetBasePlayer()->IsServer() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Cancel the current gesture and restart the main sequence.
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::RestartMainSequence( void )
{
	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();
	if ( pPlayer )
	{
		pPlayer->m_flAnimTime = gpGlobals->curtime;
		pPlayer->SetCycle( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Check to see if we hit water and stop jumping animation.
		if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
		{
			m_bJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( GetBasePlayer()->GetFlags() & FL_ONGROUND )
			{
				m_bJumping = false;
				RestartMainSequence();
			}
		}
	}
	if ( m_bJumping )
	{
		idealActivity = ACT_MP_JUMP;
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
	{
		if ( GetOuterXYSpeed() > MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_MP_CROUCHWALK;
		}
		else
		{
			idealActivity = ACT_MP_CROUCH_IDLE;
		}
		
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	if ( GetBasePlayer()->GetWaterLevel() >= WL_Waist )
	{
		if ( m_bFirstSwimFrame )
		{
			// Reset the animation.
			RestartMainSequence();	
			m_bFirstSwimFrame = false;
		}

		idealActivity = ACT_MP_SWIM;		
		m_bInSwim = true;
		return true;
	}
	else
	{
		m_bInSwim = false;

		if ( !m_bFirstSwimFrame )
		{
			m_bFirstSwimFrame = true;
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::HandleDying( Activity &idealActivity )
{
	if ( m_bDying )
	{
		if ( m_bFirstDyingFrame )
		{
			// Reset the animation.
			RestartMainSequence();	
			m_bFirstDyingFrame = false;
		}

		idealActivity = ACT_DIESIMPLE;
		return true;
	}
	else
	{
		if ( !m_bFirstDyingFrame )
		{
			m_bFirstDyingFrame = true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	// In TF we run all the time now.
	float flSpeed = GetOuterXYSpeed();

	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		// Always assume a run.
		idealActivity = ACT_MP_RUN;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CMultiPlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	if ( HandleJumping( idealActivity ) || 
		HandleDucking( idealActivity ) || 
		HandleSwimming( idealActivity ) || 
		HandleDying( idealActivity ) )
	{
		// intentionally blank
	}
	else
	{
		HandleMoving( idealActivity );
	}

	ShowDebugInfo();

	// Client specific.
#ifdef CLIENT_DLL

	if ( anim_showmainactivity.GetBool() )
	{
		DebugShowActivity( idealActivity );
	}

#endif

	return idealActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CMultiPlayerAnimState::TranslateActivity( Activity actDesired )
{
	// Translate activities for swimming.
	if ( m_bInSwim )
	{
		switch ( actDesired )
		{
		case ACT_MP_ATTACK_STAND_PRIMARYFIRE: { actDesired = ACT_MP_ATTACK_SWIM_PRIMARYFIRE; break; }
		case ACT_MP_ATTACK_STAND_SECONDARYFIRE: { actDesired = ACT_MP_ATTACK_SWIM_SECONDARYFIRE; break; }
		case ACT_MP_ATTACK_STAND_GRENADE: { actDesired = ACT_MP_ATTACK_SWIM_GRENADE; break; }
		case ACT_MP_RELOAD_STAND: { actDesired = ACT_MP_RELOAD_SWIM; break; }
		}
	}

	return actDesired;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float CMultiPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	CStudioHdr *pStudioHdr = GetBasePlayer()->GetModelPtr();

	if ( pStudioHdr == NULL )
		return 1.0f;

	float prevX = GetBasePlayer()->GetPoseParameter( m_PoseParameterData.m_iMoveX );
	float prevY = GetBasePlayer()->GetPoseParameter( m_PoseParameterData.m_iMoveY );

	float d = MAX( fabs( prevX ), fabs( prevY ) );
	float newX, newY;
	if ( d == 0.0 )
	{ 
		newX = 1.0;
		newY = 0.0;
	}
	else
	{
		newX = prevX / d;
		newY = prevY / d;
	}

	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, newX );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, newY );

	float speed = GetBasePlayer()->GetSequenceGroundSpeed( GetBasePlayer()->GetSequence() );

	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, prevX );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, prevY );

	return speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *bIsMoving - 
// Output : float
//-----------------------------------------------------------------------------
float CMultiPlayerAnimState::CalcMovementSpeed( bool *bIsMoving )
{
	// Get the player's current velocity and speed.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );
	float flSpeed = vecVelocity.Length2D();

	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		*bIsMoving = true;
		return flSpeed;
	}

	*bIsMoving = false;
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *bIsMoving - 
// Output : float
//-----------------------------------------------------------------------------
float CMultiPlayerAnimState::CalcMovementPlaybackRate( bool *bIsMoving )
{
	float flSpeed = CalcMovementSpeed( bIsMoving );
	float flReturn = 1.0f;
	// If we are moving.
	if ( *bIsMoving )
	{
		//		float flGroundSpeed = GetInterpolatedGroundSpeed();
		float flGroundSpeed = GetCurrentMaxGroundSpeed();
		if ( flGroundSpeed < 0.001f )
		{
			flReturn = 0.01f;
		}
		else
		{
			// Note this gets set back to 1.0 if sequence changes due to ResetSequenceInfo below
			flReturn = flSpeed / flGroundSpeed;
			flReturn = clamp( flReturn, 0.01f, 10.0f );
		}
	}

	return flReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CMultiPlayerAnimState::GetInterpolatedGroundSpeed( void )
{
	return m_flMaxGroundSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
	VPROF( "CBasePlayerAnimState::ComputeSequences" );

	// Lower body (walk/run/idle).
	ComputeMainSequence();

	// The groundspeed interpolator uses the main sequence info.
	UpdateInterpolators();		
	ComputeGestureSequence( pStudioHdr );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ComputeMainSequence()
{
	VPROF( "CBasePlayerAnimState::ComputeMainSequence" );

	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();

	// Have our class or the mod-specific class determine what the current activity is.
	Activity idealActivity = CalcMainActivity();

#ifdef CLIENT_DLL
	Activity oldActivity = m_eCurrentMainSequenceActivity;
#endif
	
	// Store our current activity so the aim and fire layers know what to do.
	m_eCurrentMainSequenceActivity = idealActivity;

	// Hook to force playback of a specific requested full-body sequence
	if ( m_nSpecificMainSequence >= 0 )
	{
		if ( pPlayer->GetSequence() != m_nSpecificMainSequence )
		{
			pPlayer->ResetSequence( m_nSpecificMainSequence );
			ResetGroundSpeed();
			return;
		}
		 
		if ( !pPlayer->IsSequenceFinished() )
			return;

		m_nSpecificMainSequence = -1;
		RestartMainSequence();
		ResetGroundSpeed();
	}

	// Export to our outer class..
	int animDesired = SelectWeightedSequence( TranslateActivity( idealActivity ) );
	if ( pPlayer->GetSequenceActivity( pPlayer->GetSequence() ) == pPlayer->GetSequenceActivity( animDesired ) )
		return;

	if ( animDesired < 0 )
	{
		 animDesired = 0;
	}

	pPlayer->ResetSequence( animDesired );

#ifdef CLIENT_DLL
	// If we went from idle to walk, reset the interpolation history.
	// Kind of hacky putting this here.. it might belong outside the base class.
	if ( (oldActivity == ACT_MP_CROUCH_IDLE || oldActivity == ACT_MP_STAND_IDLE || oldActivity == ACT_MP_DEPLOYED_IDLE || oldActivity == ACT_MP_CROUCH_DEPLOYED_IDLE ) && 
		 (idealActivity == ACT_MP_WALK || idealActivity == ACT_MP_CROUCHWALK ) )
	{
		ResetGroundSpeed();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ResetGroundSpeed( void )
{
#ifdef CLIENT_DLL
		m_flMaxGroundSpeed = GetCurrentMaxGroundSpeed();
		m_iv_flMaxGroundSpeed.Reset();
		m_iv_flMaxGroundSpeed.NoteChanged( gpGlobals->curtime, 0, false );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::UpdateInterpolators()
{
	VPROF( "CBasePlayerAnimState::UpdateInterpolators" );

	// First, figure out their current max speed based on their current activity.
	float flCurMaxSpeed = GetCurrentMaxGroundSpeed();

#ifdef CLIENT_DLL
	float flGroundSpeedInterval = 0.1;

	// Only update this 10x/sec so it has an interval to interpolate over.
	if ( gpGlobals->curtime - m_flLastGroundSpeedUpdateTime >= flGroundSpeedInterval )
	{
		m_flLastGroundSpeedUpdateTime = gpGlobals->curtime;

		m_flMaxGroundSpeed = flCurMaxSpeed;
		m_iv_flMaxGroundSpeed.NoteChanged( gpGlobals->curtime, flGroundSpeedInterval, false );
	}

	m_iv_flMaxGroundSpeed.Interpolate( gpGlobals->curtime, flGroundSpeedInterval );
#else
	m_flMaxGroundSpeed = flCurMaxSpeed;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ComputeFireSequence( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ComputeGestureSequence( CStudioHdr *pStudioHdr )
{
	// Update all active gesture layers.
	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		if ( !m_aGestureSlots[iGesture].m_bActive )
			continue;

		if ( !VerifyAnimLayerInSlot( iGesture ) )
			continue;

		UpdateGestureLayer( pStudioHdr, &m_aGestureSlots[iGesture] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::UpdateGestureLayer( CStudioHdr *pStudioHdr, GestureSlot_t *pGesture )
{
	// Sanity check.
	if ( !pStudioHdr || !pGesture )
		return;

	CBasePlayer *pPlayer = GetBasePlayer();
	if( !pPlayer )
		return;

#ifdef CLIENT_DLL 

	// Get the current cycle.
	float flCycle = pGesture->m_pAnimLayer->m_flCycle;
	flCycle += pPlayer->GetSequenceCycleRate( pStudioHdr, pGesture->m_pAnimLayer->m_nSequence ) * gpGlobals->frametime * GetGesturePlaybackRate() * pGesture->m_pAnimLayer->m_flPlaybackRate;

	pGesture->m_pAnimLayer->m_flPrevCycle =	pGesture->m_pAnimLayer->m_flCycle;
	pGesture->m_pAnimLayer->m_flCycle = flCycle;

	if( flCycle > 1.0f )
	{
		RunGestureSlotAnimEventsToCompletion( pGesture );

		if ( pGesture->m_bAutoKill )
		{
			ResetGestureSlot( pGesture->m_iGestureSlot );
			return;
		}
		else
		{
			pGesture->m_pAnimLayer->m_flCycle = 1.0f;
		}
	}

#else

	if ( pGesture->m_iActivity != ACT_INVALID && pGesture->m_pAnimLayer->m_nActivity == ACT_INVALID )
	{
		ResetGestureSlot( pGesture->m_iGestureSlot );
	}

#endif
}

extern ConVar mp_facefronttime;
extern ConVar mp_feetyawrate;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eyeYaw - 
//			eyePitch - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = GetBasePlayer()->GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		// Pose parameter - what direction are the player's legs running in.
		ComputePoseParam_MoveYaw( pStudioHdr );

		// Pose parameter - Torso aiming (up/down).
		ComputePoseParam_AimPitch( pStudioHdr );

		// Pose parameter - Torso aiming (rotation).
		ComputePoseParam_AimYaw( pStudioHdr );
	}

#ifdef CLIENT_DLL
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		GetBasePlayer()->SetPlaybackRate( 1.0f );
	}
#endif

	if( mp_showgestureslots.GetInt() == GetBasePlayer()->entindex() )
	{
		DebugGestureInfo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::ShouldUpdateAnimState()
{
	// Don't update anim state if we're not visible
	if ( GetBasePlayer()->IsEffectActive( EF_NODRAW ) )
		return false;

	// By default, don't update their animation state when they're dead because they're
	// either a ragdoll or they're not drawn.
#ifdef CLIENT_DLL
	if ( GetBasePlayer()->IsDormant() )
		return false;
#endif

	return (GetBasePlayer()->IsAlive() || m_bDying);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMultiPlayerAnimState::SetupPoseParameters( CStudioHdr *pStudioHdr )
{
	// Check to see if this has already been done.
	if ( m_bPoseParameterInit )
		return true;

	// Save off the pose parameter indices.
	if ( !pStudioHdr )
		return false;

	m_bPoseParameterInit = true;

	// Look for the movement blenders.
	m_PoseParameterData.m_iMoveX = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_x" );
	m_PoseParameterData.m_iMoveY = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_y" );
	/*
	if ( ( m_PoseParameterData.m_iMoveX < 0 ) || ( m_PoseParameterData.m_iMoveY < 0 ) )
		return false;
	*/

	// Look for the aim pitch blender.
	m_PoseParameterData.m_iAimPitch = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "body_pitch" );
	/*
	if ( m_PoseParameterData.m_iAimPitch < 0 )
		return false;
	*/

	// Look for aim yaw blender.
	m_PoseParameterData.m_iAimYaw = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "body_yaw" );
	/*
	if ( m_PoseParameterData.m_iAimYaw < 0 )
		return false;
	*/

	m_PoseParameterData.m_iMoveYaw = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_yaw" );
	m_PoseParameterData.m_iMoveScale = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_scale" );
	/*
	if ( ( m_PoseParameterData.m_iMoveYaw < 0 ) || ( m_PoseParameterData.m_iMoveScale < 0 ) )
		return false;
	*/

	return true;
}

float SnapYawTo( float flValue )
{
	float flSign = 1.0f;
	if ( flValue < 0.0f )
	{
		flSign = -1.0f;
		flValue = -flValue;
	}

	if ( flValue < 23.0f )
	{
		flValue = 0.0f;
	}
	else if ( flValue < 67.0f )
	{
		flValue = 45.0f;
	}
	else if ( flValue < 113.0f )
	{
		flValue = 90.0f;
	}
	else if ( flValue < 157 )
	{
		flValue = 135.0f;
	}
	else
	{
		flValue = 180.0f;
	}

	return ( flValue * flSign );
}

//-----------------------------------------------------------------------------
// Purpose: double check that the movement animations actually have movement
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::DoMovementTest( CStudioHdr *pStudioHdr, float flX, float flY )
{
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, flX );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, flY );

#ifdef STAGING_ONLY
	float flTestSpeed = GetBasePlayer()->GetSequenceGroundSpeed( m_nMovementSequence );
	if ( flTestSpeed < 10.0f )
	{
		Warning( "%s : %s (X %.0f Y %.0f) missing movement\n", pStudioHdr->pszName(), GetBasePlayer()->GetSequenceName( m_nMovementSequence ), flX, flY );
	}
#endif

	/*
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, flX );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, flY );
	float flDuration = GetBasePlayer()->SequenceDuration( m_nMovementSequence );

	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, 1.0f );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, 0.0f );
	float flForward = GetBasePlayer()->SequenceDuration( m_nMovementSequence );

	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, 0.0f );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, 0.0f );
	float flCenter = GetBasePlayer()->SequenceDuration( m_nMovementSequence );

	if ( flDuration > flForward * 1.1f || flDuration < flForward * 0.9f )
	{
		Warning( "%s : %s (X %.0f Y %.0f) mismatched duration with forward  %.1f vs %.1f\n", pStudioHdr->pszName(), GetBasePlayer()->GetSequenceName( m_nMovementSequence ), flX, flY, flDuration, flForward );
	}

	if ( flDuration > flCenter * 1.1f || flDuration < flCenter * 0.9f )
	{
		Warning( "%s : %s (X %.0f Y %.0f) mismatched duration with center  %.1f vs %.1f\n", pStudioHdr->pszName(), GetBasePlayer()->GetSequenceName( m_nMovementSequence ), flX, flY, flDuration, flCenter );
	}
	*/
}


void CMultiPlayerAnimState::DoMovementTest( CStudioHdr *pStudioHdr )
{
	if ( m_LegAnimType == LEGANIM_9WAY )
	{
		DoMovementTest( pStudioHdr, -1.0f, -1.0f );
		DoMovementTest( pStudioHdr, -1.0f,  0.0f );
		DoMovementTest( pStudioHdr, -1.0f,  1.0f );
		DoMovementTest( pStudioHdr,  0.0f, -1.0f );
		DoMovementTest( pStudioHdr,  0.0f,  1.0f );
		DoMovementTest( pStudioHdr,  1.0f, -1.0f );
		DoMovementTest( pStudioHdr,  1.0f,  0.0f );
		DoMovementTest( pStudioHdr,  1.0f,  1.0f );
	}
}

void CMultiPlayerAnimState::GetMovementFlags( CStudioHdr *pStudioHdr )
{
	if ( m_nMovementSequence == GetBasePlayer()->GetSequence() )
	{
		return;
	}

	m_nMovementSequence = GetBasePlayer()->GetSequence(); 
	m_LegAnimType = LEGANIM_9WAY;

	KeyValues *seqKeyValues = GetBasePlayer()->GetSequenceKeyValues( m_nMovementSequence );
	// Msg("sequence %d : %s (%d)\n", sequence,  GetOuter()->GetSequenceName( sequence ), seqKeyValues != NULL );
	if (seqKeyValues)
	{
		KeyValues *pkvMovement = seqKeyValues->FindKey( "movement" );
		if (pkvMovement)
		{
			const char *szStyle = pkvMovement->GetString();
			if ( V_stricmp( szStyle, "robot2" ) == 0 )
			{
				m_LegAnimType = LEGANIM_8WAY;
			}
		}
		seqKeyValues->deleteThis();
	}

	// skip tests if it's not a movement animation
	if ( m_nMovementSequence < 0 || !( GetBasePlayer()->GetFlags() & FL_ONGROUND ) || pStudioHdr->pSeqdesc( m_nMovementSequence ).groupsize[0] == 1 )
	{
		return;
	}

	DoMovementTest( pStudioHdr );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	// Get the estimated movement yaw.
	EstimateYaw();

	// Get the view yaw.
	float flAngle = AngleNormalize( m_flEyeYaw );

	// Calc side to side turning - the view vs. movement yaw.
	float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
	flYaw = AngleNormalize( -flYaw );

	// Get the current speed the character is running.
	bool bIsMoving;
	float flSpeed = CalcMovementSpeed( &bIsMoving );
	
	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vecCurrentMoveYaw( 0.0f, 0.0f );
	if ( bIsMoving )
	{
		GetMovementFlags( pStudioHdr );

		if ( mp_slammoveyaw.GetBool() )
		{
			flYaw = SnapYawTo( flYaw );
		}

		if ( m_LegAnimType == LEGANIM_9WAY )
		{
			// convert YAW back into vector
			vecCurrentMoveYaw.x = cos( DEG2RAD( flYaw ) );
			vecCurrentMoveYaw.y = -sin( DEG2RAD( flYaw ) );
			// push edges out to -1 to 1 box
			float flInvScale = MAX( fabs( vecCurrentMoveYaw.x ), fabs( vecCurrentMoveYaw.y ) );
			if ( flInvScale != 0.0f )
			{
				vecCurrentMoveYaw.x /= flInvScale;
				vecCurrentMoveYaw.y /= flInvScale;
			}

			// find what speed was actually authored
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y );
			float flMaxSpeed = GetBasePlayer()->GetSequenceGroundSpeed( GetBasePlayer()->GetSequence() );

			// scale playback
			if ( flMaxSpeed > flSpeed )
			{
				vecCurrentMoveYaw.x *= flSpeed / flMaxSpeed;
				vecCurrentMoveYaw.y *= flSpeed / flMaxSpeed;
			}

			// Set the 9-way blend movement pose parameters.
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y );
		}
		else
		{
			// find what speed was actually authored
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveYaw, flYaw );
			GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveScale, 1.0f );
			float flMaxSpeed = GetBasePlayer()->GetSequenceGroundSpeed( GetBasePlayer()->GetSequence() );

			// scale playback
			if ( flMaxSpeed > flSpeed )
			{
				GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveScale, flSpeed / flMaxSpeed );
			}
		}
	}
	else
	{
		// Set the 9-way blend movement pose parameters.
		GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, 0.0f );
		GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, 0.0f );
	}

	m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::EstimateYaw( void )
{
	// Get the frame time.
	float flDeltaTime = gpGlobals->frametime;
	if ( flDeltaTime == 0.0f )
		return;

	// Get the player's velocity and angles.
	Vector vecEstVelocity;
	GetOuterAbsVelocity( vecEstVelocity );
	QAngle angles = GetBasePlayer()->GetLocalAngles();

	// If we are not moving, sync up the feet and eyes slowly.
	if ( vecEstVelocity.x == 0.0f && vecEstVelocity.y == 0.0f )
	{
		float flYawDelta = angles[YAW] - m_PoseParameterData.m_flEstimateYaw;
		flYawDelta = AngleNormalize( flYawDelta );

		if ( flDeltaTime < 0.25f )
		{
			flYawDelta *= ( flDeltaTime * 4.0f );
		}
		else
		{
			flYawDelta *= flDeltaTime;
		}

		m_PoseParameterData.m_flEstimateYaw += flYawDelta;
		AngleNormalize( m_PoseParameterData.m_flEstimateYaw );
	}
	else
	{
		m_PoseParameterData.m_flEstimateYaw = ( atan2( vecEstVelocity.y, vecEstVelocity.x ) * 180.0f / M_PI );
		m_PoseParameterData.m_flEstimateYaw = clamp( m_PoseParameterData.m_flEstimateYaw, -180.0f, 180.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr )
{
	// Get the view pitch.
	float flAimPitch = m_flEyePitch;

	// Set the aim pitch pose parameter and save.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, -flAimPitch );
	m_DebugAnimData.m_flAimPitch = flAimPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr )
{
	// Get the movement velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Check to see if we are moving.
	bool bMoving = ( vecVelocity.Length() > 1.0f ) ? true : false;

	// If we are moving or are prone and undeployed.
	if ( bMoving || m_bForceAimYaw )
	{
		// The feet match the eye direction when moving - the move yaw takes care of the rest.
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if ( m_PoseParameterData.m_flLastAimTurnTime <= 0.0f )
		{
			m_flGoalFeetYaw	= m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

			if ( fabs( flYawDelta ) > 45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/ )
			{
				float flSide = ( flYawDelta > 0.0f ) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += ( 45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/ * flSide );
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );
	if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
	{
		if ( m_bForceAimYaw )
		{
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		}
		else
		{
			ConvergeYawAngles( m_flGoalFeetYaw, /*DOD_BODYYAW_RATE*/720.0f, gpGlobals->frametime, m_flCurrentFeetYaw );
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	// Rotate the body into position.
	m_angRender[YAW] = m_flCurrentFeetYaw;

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize( flAimYaw );

	// Set the aim yaw and save.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, -flAimYaw );
	m_DebugAnimData.m_flAimYaw	= flAimYaw;

	// Turn off a force aim yaw - either we have already updated or we don't need to.
	m_bForceAimYaw = false;

#ifndef CLIENT_DLL
	QAngle angle = GetBasePlayer()->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	GetBasePlayer()->SetAbsAngles( angle );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flGoalYaw - 
//			flYawRate - 
//			flDeltaTime - 
//			&flCurrentYaw - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::ConvergeYawAngles( float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw )
{
#define FADE_TURN_DEGREES 60.0f

	// Find the yaw delta.
	float flDeltaYaw = flGoalYaw - flCurrentYaw;
	float flDeltaYawAbs = fabs( flDeltaYaw );
	flDeltaYaw = AngleNormalize( flDeltaYaw );

	// Always do at least a bit of the turn (1%).
	float flScale = 1.0f;
	flScale = flDeltaYawAbs / FADE_TURN_DEGREES;
	flScale = clamp( flScale, 0.01f, 1.0f );

	float flYaw = flYawRate * flDeltaTime * flScale;
	if ( flDeltaYawAbs < flYaw )
	{
		flCurrentYaw = flGoalYaw;
	}
	else
	{
		float flSide = ( flDeltaYaw < 0.0f ) ? -1.0f : 1.0f;
		flCurrentYaw += ( flYaw * flSide );
	}

	flCurrentYaw = AngleNormalize( flCurrentYaw );

#undef FADE_TURN_DEGREES
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : const QAngle&
//-----------------------------------------------------------------------------
const QAngle& CMultiPlayerAnimState::GetRenderAngles()
{
	return m_angRender;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vel - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::GetOuterAbsVelocity( Vector& vel )
{
#if defined( CLIENT_DLL )
	GetBasePlayer()->EstimateAbsVelocity( vel );
#else
	vel = GetBasePlayer()->GetAbsVelocity();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::Release( void )
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float CMultiPlayerAnimState::GetOuterXYSpeed()
{
	Vector vel;
	GetOuterAbsVelocity( vel );
	return vel.Length2D();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Anim_StateLog( const char *pMsg, ... )
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start( marker, pMsg );
	Q_vsnprintf( str, sizeof( str ), pMsg, marker );
	va_end( marker );

	// Log it?	
	if ( anim_showstatelog.GetInt() == 1 || anim_showstatelog.GetInt() == 3 )
	{
		Msg( "%s", str );
	}

	if ( anim_showstatelog.GetInt() > 1 )
	{
//		static FileHandle_t hFile = filesystem->Open( "AnimState.log", "wt" );
//		filesystem->FPrintf( hFile, "%s", str );
//		filesystem->Flush( hFile );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Anim_StatePrintf( int iLine, const char *pMsg, ... )
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start( marker, pMsg );
	Q_vsnprintf( str, sizeof( str ), pMsg, marker );
	va_end( marker );

	// Show it with Con_NPrintf.
	engine->Con_NPrintf( iLine, "%s", str );

	// Log it.
	Anim_StateLog( "%s\n", str );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::DebugShowAnimStateForPlayer( bool bIsServer )
{
	// Get the player's velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Start animation state logging.
	int iLine = 5;
	if ( bIsServer )
	{
		iLine = 12;
	}
//	Anim_StateLog( "-------------%s: frame %d -----------------\n", bIsServer ? "Server" : "Client", gpGlobals->framecount );
	Anim_StatePrintf( iLine++, "-------------%s: frame %d -----------------\n", bIsServer ? "Server" : "Client", gpGlobals->framecount );

	// Write out the main sequence and its data.
	Anim_StatePrintf( iLine++, "Main: %s, Cycle: %.2f\n", GetSequenceName( GetBasePlayer()->GetModelPtr(), GetBasePlayer()->GetSequence() ), GetBasePlayer()->GetCycle() );

#if 0
	if ( m_bPlayingGesture )
	{
		Anim_StatePrintf( iLine++, "Gesture: %s, Cycle: %.2f\n",
			GetSequenceName( GetBasePlayer()->GetModelPtr(), m_iGestureSequence ),
			m_flGestureCycle );
	}
#endif

	// Write out the layers and their data.
	for ( int iAnim = 0; iAnim < GetBasePlayer()->GetNumAnimOverlays(); ++iAnim )
	{
#ifdef CLIENT_DLL
		C_AnimationLayer *pLayer = GetBasePlayer()->GetAnimOverlay( iAnim );
		if ( pLayer && ( pLayer->m_nOrder != CBaseAnimatingOverlay::MAX_OVERLAYS ) )
		{
			Anim_StatePrintf( iLine++, "Layer %s: Weight: %.2f, Cycle: %.2f", GetSequenceName( GetBasePlayer()->GetModelPtr(), pLayer->m_nSequence ), (float)pLayer->m_flWeight, (float)pLayer->m_flCycle );
		}
#else
		CAnimationLayer *pLayer = GetBasePlayer()->GetAnimOverlay( iAnim );
		if ( pLayer && ( pLayer->m_nOrder != CBaseAnimatingOverlay::MAX_OVERLAYS ) )
		{
			Anim_StatePrintf( iLine++, "Layer %s: Weight: %.2f, Cycle: %.2f", GetSequenceName( GetBasePlayer()->GetModelPtr(), pLayer->m_nSequence ), (float)pLayer->m_flWeight, (float)pLayer->m_flCycle );
		}
#endif
	}

	// Write out the speed data.
	Anim_StatePrintf( iLine++, "Time: %.2f, Speed: %.2f, MaxSpeed: %.2f", gpGlobals->curtime, vecVelocity.Length2D(), GetCurrentMaxGroundSpeed() );

	// Write out the 9-way blend data.
	Anim_StatePrintf( iLine++, "EntityYaw: %.2f, AimYaw: %.2f, AimPitch: %.2f, MoveX: %.2f, MoveY: %.2f", m_angRender[YAW], m_DebugAnimData.m_flAimYaw, m_DebugAnimData.m_flAimPitch, m_DebugAnimData.m_vecMoveYaw.x, m_DebugAnimData.m_vecMoveYaw.y );

//	Anim_StateLog( "--------------------------------------------\n\n" );
	Anim_StatePrintf( iLine++, "--------------------------------------------\n\n" );

	DebugShowEyeYaw();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::DebugShowEyeYaw( void )
{
#ifdef _NDEBUG

	float flBaseSize = 10;
	float flHeight = 80;

	Vector vecPos = GetOuter()->GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f );
	QAngle angles( 0.0f, 0.0f, 0.0f );

	angles[YAW] = m_flEyeYaw;

	Vector vecForward, vecRight, vecUp;
	AngleVectors( angles, &vecForward, &vecRight, &vecUp );

	// Draw a red triangle on the ground for the eye yaw.
	debugoverlay->AddTriangleOverlay( ( vecPos + vecRight * flBaseSize / 2.0f ), 
		( vecPos - vecRight * flBaseSize / 2.0f ), 
		( vecPos + vecForward * flHeight, 255, 0, 0, 255, false, 0.01f );

#endif
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::DebugShowActivity( Activity activity )
{
#ifdef _DEBUG

	const char *pszActivity = "other";

	switch( activity )
	{
	case ACT_MP_STAND_IDLE:
		{
			pszActivity = "idle";
			break;
		}
	case ACT_MP_SPRINT:
		{
			pszActivity = "sprint";
			break;
		}
	case ACT_MP_WALK:
		{
			pszActivity = "walk";
			break;
		}
	case ACT_MP_RUN:
		{
			pszActivity = "run";
			break;
		}
	}

	Msg( "Activity: %s\n", pszActivity );

#endif
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iStartLine - 
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::DebugShowAnimState( int iStartLine )
{
	Vector vOuterVel;
	GetOuterAbsVelocity( vOuterVel );

	Anim_StateLog( "----------------- frame %d -----------------\n", gpGlobals->framecount );

	int iLine = iStartLine;
	Anim_StatePrintf( iLine++, "main: %s, cycle: %.2f\n", GetSequenceName( GetBasePlayer()->GetModelPtr(), GetBasePlayer()->GetSequence() ), GetBasePlayer()->GetCycle() );

#if defined( CLIENT_DLL )
	for ( int i=0; i < GetBasePlayer()->GetNumAnimOverlays()-1; i++ )
	{
		C_AnimationLayer *pLayer = GetBasePlayer()->GetAnimOverlay( i /*i+1?*/ );
		Anim_StatePrintf( iLine++, "%s, weight: %.2f, cycle: %.2f, aim (%d)", 
			pLayer->m_nOrder == CBaseAnimatingOverlay::MAX_OVERLAYS ? "--" : GetSequenceName( GetBasePlayer()->GetModelPtr(), pLayer->m_nSequence ), 
			pLayer->m_nOrder == CBaseAnimatingOverlay::MAX_OVERLAYS ? -1 :(float)pLayer->m_flWeight, 
			pLayer->m_nOrder == CBaseAnimatingOverlay::MAX_OVERLAYS ? -1 :(float)pLayer->m_flCycle, 
			i
			);
	}
#endif

	Anim_StatePrintf( iLine++, "vel: %.2f, time: %.2f, max: %.2f", 
		vOuterVel.Length2D(), gpGlobals->curtime, GetInterpolatedGroundSpeed() );
		
//	AnimStatePrintf( iLine++, "ent yaw: %.2f, body_yaw: %.2f, body_pitch: %.2f, move_x: %.2f, move_y: %.2f", 
//		m_angRender[YAW], g_flLastBodyYaw, g_flLastBodyPitch, m_vLastMovePose.x, m_vLastMovePose.y );

	Anim_StateLog( "--------------------------------------------\n\n" );

	// Draw a red triangle on the ground for the eye yaw.
	float flBaseSize = 10;
	float flHeight = 80;
	Vector vBasePos = GetBasePlayer()->GetAbsOrigin() + Vector( 0, 0, 3 );
	QAngle angles( 0, 0, 0 );
	angles[YAW] = m_flEyeYaw;
	Vector vForward, vRight, vUp;
	AngleVectors( angles, &vForward, &vRight, &vUp );
	debugoverlay->AddTriangleOverlay( vBasePos+vRight*flBaseSize/2, vBasePos-vRight*flBaseSize/2, vBasePos+vForward*flHeight, 255, 0, 0, 255, false, 0.01 );

	// Draw a blue triangle on the ground for the body yaw.
	angles[YAW] = m_angRender[YAW];
	AngleVectors( angles, &vForward, &vRight, &vUp );
	debugoverlay->AddTriangleOverlay( vBasePos+vRight*flBaseSize/2, vBasePos-vRight*flBaseSize/2, vBasePos+vForward*flHeight, 0, 0, 255, 255, false, 0.01 );	
}

// Debug!
const char *s_aGestureSlotNames[GESTURE_SLOT_COUNT] =
{
	"Attack and Reload",
	"Grenade",
	"Jump",
	"Swim",
	"Flinch",
	"VCD",
	"Custom"
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::DebugGestureInfo( void )
{
	CBasePlayer *pPlayer = GetBasePlayer();
	if ( !pPlayer )
		return;

	int iLine = ( pPlayer->IsServer() ? 12 : ( 14 + GESTURE_SLOT_COUNT ) );

	Anim_StatePrintf( iLine++, "%s\n", ( pPlayer->IsServer() ? "Server" : "Client" ) );

	for ( int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture )
	{
		GestureSlot_t *pGesture = &m_aGestureSlots[iGesture];
		if ( pGesture )
		{
			if( pGesture->m_bActive )
			{
				Anim_StatePrintf( iLine++, "Gesture Slot %d(%s): %s %s(A:%s, C:%f P:%f)\n", 
					iGesture, 
					s_aGestureSlotNames[iGesture],
					ActivityList_NameForIndex( pGesture->m_iActivity ),
					GetSequenceName( pPlayer->GetModelPtr(), pGesture->m_pAnimLayer->m_nSequence ),
					( pGesture->m_bAutoKill ? "true" : "false" ),
					(float)pGesture->m_pAnimLayer->m_flCycle, (float)pGesture->m_pAnimLayer->m_flPlaybackRate );
			}
			else
			{
				Anim_StatePrintf( iLine++, "Gesture Slot %d(%s): NOT ACTIVE!\n", iGesture, s_aGestureSlotNames[iGesture] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: New Model, init the pose parameters
//-----------------------------------------------------------------------------
void CMultiPlayerAnimState::OnNewModel( void )
{
	m_bPoseParameterInit = false;
	m_PoseParameterData.Init();
	ClearAnimationState();
}
