//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "base_playeranimstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "filesystem.h"


#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
	#include "engine/ivdebugoverlay.h"

	ConVar cl_showanimstate( "cl_showanimstate", "-1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show the (client) animation state for the specified entity (-1 for none)." );
	ConVar showanimstate_log( "cl_showanimstate_log", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "1 to output cl_showanimstate to Msg(). 2 to store in AnimStateClient.log. 3 for both." );
#else
	#include "player.h"
	ConVar sv_showanimstate( "sv_showanimstate", "-1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show the (server) animation state for the specified entity (-1 for none)." );
	ConVar showanimstate_log( "sv_showanimstate_log", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "1 to output sv_showanimstate to Msg(). 2 to store in AnimStateServer.log. 3 for both." );
#endif


// Below this many degrees, slow down turning rate linearly
#define FADE_TURN_DEGREES	45.0f

// After this, need to start turning feet
#define MAX_TORSO_ANGLE		70.0f

// Below this amount, don't play a turning animation/perform IK
#define MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION		15.0f


ConVar mp_feetyawrate( 
	"mp_feetyawrate", 
	"720", 
	FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, 
	"How many degrees per second that we can turn our feet or upper body." );

ConVar mp_facefronttime( 
	"mp_facefronttime", 
	"3", 
	FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, 
	"After this amount of time of standing in place but aiming to one side, go ahead and move feet to face upper body." );

ConVar mp_ik( "mp_ik", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Use IK on in-place turns." );

// Pose parameters stored for debugging.
float g_flLastBodyPitch, g_flLastBodyYaw, m_flLastMoveYaw;


// ------------------------------------------------------------------------------------------------ //
// CBasePlayerAnimState implementation.
// ------------------------------------------------------------------------------------------------ //

CBasePlayerAnimState::CBasePlayerAnimState()
{
	m_flEyeYaw = 0.0f;
	m_flEyePitch = 0.0f;
	m_bCurrentFeetYawInitialized = false;
	m_flCurrentTorsoYaw = 0.0f;
	m_nTurningInPlace = TURN_NONE;
	m_flMaxGroundSpeed = 0.0f;
	m_flStoredCycle = 0.0f;

	m_flGaitYaw = 0.0f;
	m_flGoalFeetYaw = 0.0f;
	m_flCurrentFeetYaw = 0.0f;
	m_flLastYaw = 0.0f;
	m_flLastTurnTime = 0.0f;
	m_angRender.Init();
	m_vLastMovePose.Init();
	m_iCurrent8WayIdleSequence = -1;
	m_iCurrent8WayCrouchIdleSequence = -1;

	m_pOuter = NULL;
	m_eCurrentMainSequenceActivity = ACT_IDLE;
	m_flLastAnimationStateClearTime = 0.0f;
}


CBasePlayerAnimState::~CBasePlayerAnimState()
{
}


void CBasePlayerAnimState::Init( CBaseAnimatingOverlay *pPlayer, const CModAnimConfig &config )
{
	m_pOuter = pPlayer;
	m_AnimConfig = config;
	ClearAnimationState();
}


void CBasePlayerAnimState::Release()
{
	delete this;
}


void CBasePlayerAnimState::ClearAnimationState()
{
	ClearAnimationLayers();
	m_bCurrentFeetYawInitialized = false;
	m_flLastAnimationStateClearTime = gpGlobals->curtime;
}


float CBasePlayerAnimState::TimeSinceLastAnimationStateClear() const
{
	return gpGlobals->curtime - m_flLastAnimationStateClearTime;
}


void CBasePlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	VPROF( "CBasePlayerAnimState::Update" );

	// Clear animation overlays because we're about to completely reconstruct them.
	ClearAnimationLayers();

	// Some mods don't want to update the player's animation state if they're dead and ragdolled.
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}
	
	
	CStudioHdr *pStudioHdr = GetOuter()->GetModelPtr();
	// Store these. All the calculations are based on them.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute sequences for all the layers.
	ComputeSequences( pStudioHdr );
	
	
	// Compute all the pose params.
	ComputePoseParam_BodyPitch( pStudioHdr );	// Look up/down.
	ComputePoseParam_BodyYaw();		// Torso rotation.
	ComputePoseParam_MoveYaw( pStudioHdr );		// What direction his legs are running in.

	
	ComputePlaybackRate();


#ifdef CLIENT_DLL
	if ( cl_showanimstate.GetInt() == m_pOuter->entindex() )
	{
		DebugShowAnimStateFull( 5 );
	}
	else if ( cl_showanimstate.GetInt() == -2 )
	{
		C_BasePlayer *targetPlayer = C_BasePlayer::GetLocalPlayer();

		if( targetPlayer && ( targetPlayer->GetObserverMode() == OBS_MODE_IN_EYE || targetPlayer->GetObserverMode() == OBS_MODE_CHASE ) )
		{
			C_BaseEntity *target = targetPlayer->GetObserverTarget();

			if( target && target->IsPlayer() )
			{
				targetPlayer = ToBasePlayer( target );
			}
		}

		if ( m_pOuter == targetPlayer )
		{
			DebugShowAnimStateFull( 6 );
		}
	}
#else
	if ( sv_showanimstate.GetInt() == m_pOuter->entindex() )
	{
		DebugShowAnimState( 20 );
	}
#endif
}


bool CBasePlayerAnimState::ShouldUpdateAnimState()
{
	// By default, don't update their animation state when they're dead because they're
	// either a ragdoll or they're not drawn.
	return GetOuter()->IsAlive();
}


bool CBasePlayerAnimState::ShouldChangeSequences( void ) const
{
	return true;
}


void CBasePlayerAnimState::SetOuterPoseParameter( int iParam, float flValue )
{
	// Make sure to set all the history values too, otherwise the server can overwrite them.
	GetOuter()->SetPoseParameter( iParam, flValue );
}


void CBasePlayerAnimState::ClearAnimationLayers()
{
	VPROF( "CBasePlayerAnimState::ClearAnimationLayers" );
	if ( !m_pOuter )
		return;

	m_pOuter->SetNumAnimOverlays( AIMSEQUENCE_LAYER+NUM_AIMSEQUENCE_LAYERS );
	for ( int i=0; i < m_pOuter->GetNumAnimOverlays(); i++ )
	{
		m_pOuter->GetAnimOverlay( i )->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#ifndef CLIENT_DLL
		m_pOuter->GetAnimOverlay( i )->m_fFlags = 0;
#endif
	}
}


void CBasePlayerAnimState::RestartMainSequence()
{
	CBaseAnimatingOverlay *pPlayer = GetOuter();

	pPlayer->m_flAnimTime = gpGlobals->curtime;
	pPlayer->SetCycle( 0 );
}


void CBasePlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
	VPROF( "CBasePlayerAnimState::ComputeSequences" );

	ComputeMainSequence();		// Lower body (walk/run/idle).
	UpdateInterpolators();		// The groundspeed interpolator uses the main sequence info.

	if ( m_AnimConfig.m_bUseAimSequences )
	{
		ComputeAimSequence();		// Upper body, based on weapon type.
	}
}

void CBasePlayerAnimState::ResetGroundSpeed( void )
{
	m_flMaxGroundSpeed = GetCurrentMaxGroundSpeed();
}

void CBasePlayerAnimState::ComputeMainSequence()
{
	VPROF( "CBasePlayerAnimState::ComputeMainSequence" );

	CBaseAnimatingOverlay *pPlayer = GetOuter();

	// Have our class or the mod-specific class determine what the current activity is.
	Activity idealActivity = CalcMainActivity();

#ifdef CLIENT_DLL
	Activity oldActivity = m_eCurrentMainSequenceActivity;
#endif
	
	// Store our current activity so the aim and fire layers know what to do.
	m_eCurrentMainSequenceActivity = idealActivity;

	// Export to our outer class..
	int animDesired = SelectWeightedSequence( TranslateActivity(idealActivity) );

#if !defined( HL1_CLIENT_DLL ) && !defined ( HL1_DLL )
	if ( !ShouldResetMainSequence( pPlayer->GetSequence(), animDesired ) )
		return;
#endif

	if ( animDesired < 0 )
		 animDesired = 0;

	pPlayer->ResetSequence( animDesired );

#ifdef CLIENT_DLL
	// If we went from idle to walk, reset the interpolation history.
	// Kind of hacky putting this here.. it might belong outside the base class.
	if ( (oldActivity == ACT_CROUCHIDLE || oldActivity == ACT_IDLE) && 
		 (idealActivity == ACT_WALK || idealActivity == ACT_RUN_CROUCH) )
	{
		ResetGroundSpeed();
	}
#endif
}

bool CBasePlayerAnimState::ShouldResetMainSequence( int iCurrentSequence, int iNewSequence )
{
	if ( !GetOuter() )
		return false;

	return GetOuter()->GetSequenceActivity( iCurrentSequence ) != GetOuter()->GetSequenceActivity( iNewSequence );
}


void CBasePlayerAnimState::UpdateAimSequenceLayers(
	float flCycle,
	int iFirstLayer,
	bool bForceIdle,
	CSequenceTransitioner *pTransitioner,
	float flWeightScale
	)
{
	float flAimSequenceWeight = 1;
	int iAimSequence = CalcAimLayerSequence( &flCycle, &flAimSequenceWeight, bForceIdle );
	if ( iAimSequence == -1 )
		iAimSequence = 0;

	// Feed the current state of the animation parameters to the sequence transitioner.
	// It will hand back either 1 or 2 animations in the queue to set, depending on whether
	// it's transitioning or not. We just dump those into the animation layers.
	pTransitioner->CheckForSequenceChange(
		m_pOuter->GetModelPtr(),
		iAimSequence,
		false,	// don't force transitions on the same anim
		true	// yes, interpolate when transitioning
		);

	pTransitioner->UpdateCurrent(
		m_pOuter->GetModelPtr(),
		iAimSequence,
		flCycle,
		GetOuter()->GetPlaybackRate(),
		gpGlobals->curtime
		);

	CAnimationLayer *pDest0 = m_pOuter->GetAnimOverlay( iFirstLayer );
	CAnimationLayer *pDest1 = m_pOuter->GetAnimOverlay( iFirstLayer+1 );

	if ( pTransitioner->m_animationQueue.Count() == 1 )
	{
		// If only 1 animation, then blend it in fully.
		CAnimationLayer *pSource0 = &pTransitioner->m_animationQueue[0];
		*pDest0 = *pSource0;
		
		pDest0->m_flWeight = 1;
		pDest1->m_flWeight = 0;
		pDest0->m_nOrder = iFirstLayer;

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}
	else if ( pTransitioner->m_animationQueue.Count() >= 2 )
	{
		// The first one should be fading out. Fade in the new one inversely.
		CAnimationLayer *pSource0 = &pTransitioner->m_animationQueue[0];
		CAnimationLayer *pSource1 = &pTransitioner->m_animationQueue[1];

		*pDest0 = *pSource0;
		*pDest1 = *pSource1;
		Assert( pDest0->m_flWeight >= 0.0f && pDest0->m_flWeight <= 1.0f );
		pDest1->m_flWeight = 1 - pDest0->m_flWeight;	// This layer just mirrors the other layer's weight (one fades in while the other fades out).

		pDest0->m_nOrder = iFirstLayer;
		pDest1->m_nOrder = iFirstLayer+1;

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
		pDest1->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}
	
	pDest0->m_flWeight *= flWeightScale * flAimSequenceWeight;
	pDest0->m_flWeight = clamp( (float)pDest0->m_flWeight, 0.0f, 1.0f );

	pDest1->m_flWeight *= flWeightScale * flAimSequenceWeight;
	pDest1->m_flWeight = clamp( (float)pDest1->m_flWeight, 0.0f, 1.0f );

	pDest0->m_flCycle = pDest1->m_flCycle = flCycle;
}


void CBasePlayerAnimState::OptimizeLayerWeights( int iFirstLayer, int nLayers )
{
	int i;

	// Find the total weight of the blended layers, not including the idle layer (iFirstLayer)
	float totalWeight = 0.0f;
	for ( i=1; i < nLayers; i++ )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer+i );
		if ( pLayer->IsActive() && pLayer->m_flWeight > 0.0f )
		{
			totalWeight += pLayer->m_flWeight;
		}
	}

	// Set the idle layer's weight to be 1 minus the sum of other layer weights
	CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer );
	if ( pLayer->IsActive() && pLayer->m_flWeight > 0.0f )
	{
		pLayer->m_flWeight = 1.0f - totalWeight;
		pLayer->m_flWeight = MAX( (float)pLayer->m_flWeight, 0.0f);
	}

	// This part is just an optimization. Since we have the walk/run animations weighted on top of 
	// the idle animations, all this does is disable the idle animations if the walk/runs are at
	// full weighting, which is whenever a guy is at full speed.
	//
	// So it saves us blending a couple animation layers whenever a guy is walking or running full speed.
	int iLastOne = -1;
	for ( i=0; i < nLayers; i++ )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer+i );
		if ( pLayer->IsActive() && pLayer->m_flWeight > 0.99 )
			iLastOne = i;
	}

	if ( iLastOne != -1 )
	{
		for ( int i=iLastOne-1; i >= 0; i-- )
		{
			CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer+i );
#ifdef CLIENT_DLL 
			pLayer->m_nOrder = CBaseAnimatingOverlay::MAX_OVERLAYS;
#else
			pLayer->m_nOrder.Set( CBaseAnimatingOverlay::MAX_OVERLAYS );
			pLayer->m_fFlags = 0;
#endif
		}
	}
}

bool CBasePlayerAnimState::ShouldBlendAimSequenceToIdle()
{
	Activity act = GetCurrentMainSequenceActivity();

	return (act == ACT_RUN || act == ACT_WALK || act == ACT_RUNTOIDLE || act == ACT_RUN_CROUCH);
}

void CBasePlayerAnimState::ComputeAimSequence()
{
	VPROF( "CBasePlayerAnimState::ComputeAimSequence" );

	// Synchronize the lower and upper body cycles.
	float flCycle = m_pOuter->GetCycle();

	// Figure out the new cycle time.
	UpdateAimSequenceLayers( flCycle, AIMSEQUENCE_LAYER, true, &m_IdleSequenceTransitioner, 1 );
	
	if ( ShouldBlendAimSequenceToIdle() )
	{
		// What we do here is blend between the idle upper body animation (like where he's got the dual elites
		// held out in front of him but he's not moving) and his walk/run/crouchrun upper body animation,
		// weighting it based on how fast he's moving. That way, when he's moving slowly, his upper 
		// body doesn't jiggle all around.
		bool bIsMoving;
		float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );
		if ( bIsMoving )
			UpdateAimSequenceLayers( flCycle, AIMSEQUENCE_LAYER+2, false, &m_SequenceTransitioner, flPlaybackRate );
	}

	OptimizeLayerWeights( AIMSEQUENCE_LAYER, NUM_AIMSEQUENCE_LAYERS );
}


int CBasePlayerAnimState::CalcSequenceIndex( const char *pBaseName, ... )
{
	char szFullName[512];
	va_list marker;
	va_start( marker, pBaseName );
	Q_vsnprintf( szFullName, sizeof( szFullName ), pBaseName, marker );
	va_end( marker );
	int iSequence = GetOuter()->LookupSequence( szFullName );
	
	// Show warnings if we can't find anything here.
	if ( iSequence == -1 )
	{
		static CUtlDict<int,int> dict;
		if ( dict.Find( szFullName ) == -1 )
		{
			dict.Insert( szFullName, 0 );
			Warning( "CalcSequenceIndex: can't find '%s'.\n", szFullName );
		}

		iSequence = 0;
	}

	return iSequence;
}



void CBasePlayerAnimState::UpdateInterpolators()
{
	VPROF( "CBasePlayerAnimState::UpdateInterpolators" );

	// First, figure out their current max speed based on their current activity.
	float flCurMaxSpeed = GetCurrentMaxGroundSpeed();
	m_flMaxGroundSpeed = flCurMaxSpeed;
}


float CBasePlayerAnimState::GetInterpolatedGroundSpeed()
{
	return m_flMaxGroundSpeed;
}


float CBasePlayerAnimState::CalcMovementPlaybackRate( bool *bIsMoving )
{
	// Determine ideal playback rate
	Vector vel;
	GetOuterAbsVelocity( vel );

	float speed = vel.Length2D();
	bool isMoving = ( speed > MOVING_MINIMUM_SPEED );

	*bIsMoving = false;
	float flReturnValue = 1;

	if ( isMoving && CanThePlayerMove() )
	{
		float flGroundSpeed = GetInterpolatedGroundSpeed();
		if ( flGroundSpeed < 0.001f )
		{
			flReturnValue = 0.01;
		}
		else
		{
			// Note this gets set back to 1.0 if sequence changes due to ResetSequenceInfo below
			flReturnValue = speed / flGroundSpeed;
			flReturnValue = clamp( flReturnValue, 0.01f, 10.f );	// don't go nuts here.
		}
		*bIsMoving = true;
	}
	
	return flReturnValue;
}


bool CBasePlayerAnimState::CanThePlayerMove()
{
	return true;
}


void CBasePlayerAnimState::ComputePlaybackRate()
{
	VPROF( "CBasePlayerAnimState::ComputePlaybackRate" );
	if ( m_AnimConfig.m_LegAnimType != LEGANIM_9WAY && m_AnimConfig.m_LegAnimType != LEGANIM_8WAY )
	{
		// When using a 9-way blend, playback rate is always 1 and we just scale the pose params
		// to speed up or slow down the animation.
		bool bIsMoving;
		float flRate = CalcMovementPlaybackRate( &bIsMoving );
		if ( bIsMoving )
			GetOuter()->SetPlaybackRate( flRate );
		else
			GetOuter()->SetPlaybackRate( 1 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBaseAnimatingOverlay *CBasePlayerAnimState::GetOuter() const
{
	return m_pOuter;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void CBasePlayerAnimState::EstimateYaw()
{
	Vector est_velocity;
	GetOuterAbsVelocity( est_velocity );

	float flLength = est_velocity.Length2D();
	if ( flLength > MOVING_MINIMUM_SPEED )
	{
		m_flGaitYaw = atan2( est_velocity[1], est_velocity[0] );
		m_flGaitYaw = RAD2DEG( m_flGaitYaw );
		m_flGaitYaw = AngleNormalize( m_flGaitYaw );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt - 
//-----------------------------------------------------------------------------
void CBasePlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	VPROF( "CBasePlayerAnimState::ComputePoseParam_MoveYaw" );

	//Matt: Goldsrc style animations need to not rotate the model
	if ( m_AnimConfig.m_LegAnimType == LEGANIM_GOLDSRC )
	{
#ifndef CLIENT_DLL
		//Adrian: Make the model's angle match the legs so the hitboxes match on both sides.
		GetOuter()->SetLocalAngles( QAngle( 0, m_flCurrentFeetYaw, 0 ) );
#endif
	}

	// If using goldsrc-style animations where he's moving in the direction that his feet are facing,
	// we don't use move yaw.
	if ( m_AnimConfig.m_LegAnimType != LEGANIM_9WAY && m_AnimConfig.m_LegAnimType != LEGANIM_8WAY )
		return;

	// view direction relative to movement
	float flYaw;	 

	EstimateYaw();

	float ang = m_flEyeYaw;
	if ( ang > 180.0f )
	{
		ang -= 360.0f;
	}
	else if ( ang < -180.0f )
	{
		ang += 360.0f;
	}

	// calc side to side turning
	flYaw = ang - m_flGaitYaw;
	// Invert for mapping into 8way blend
	flYaw = -flYaw;
	flYaw = flYaw - (int)(flYaw / 360) * 360;

	if (flYaw < -180)
	{
		flYaw = flYaw + 360;
	}
	else if (flYaw > 180)
	{
		flYaw = flYaw - 360;
	}

	
	if ( m_AnimConfig.m_LegAnimType == LEGANIM_9WAY )
	{
#ifndef CLIENT_DLL
		//Adrian: Make the model's angle match the legs so the hitboxes match on both sides.
		GetOuter()->SetLocalAngles( QAngle( 0, m_flCurrentFeetYaw, 0 ) );
#endif

		int iMoveX = GetOuter()->LookupPoseParameter( pStudioHdr, "move_x" );
		int iMoveY = GetOuter()->LookupPoseParameter( pStudioHdr, "move_y" );
		if ( iMoveX < 0 || iMoveY < 0 )
			return;

		bool bIsMoving;
		float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );

		// Setup the 9-way blend parameters based on our speed and direction.
		Vector2D vCurMovePose( 0, 0 );

		if ( bIsMoving )
		{
			vCurMovePose.x = cos( DEG2RAD( flYaw ) ) * flPlaybackRate;
			vCurMovePose.y = -sin( DEG2RAD( flYaw ) ) * flPlaybackRate;
		}

		GetOuter()->SetPoseParameter( pStudioHdr, iMoveX, vCurMovePose.x );
		GetOuter()->SetPoseParameter( pStudioHdr, iMoveY, vCurMovePose.y );

		m_vLastMovePose = vCurMovePose;
	}
	else
	{
		int iMoveYaw = GetOuter()->LookupPoseParameter( pStudioHdr, "move_yaw" );
		if ( iMoveYaw >= 0 )
		{
			GetOuter()->SetPoseParameter( pStudioHdr, iMoveYaw, flYaw );
			m_flLastMoveYaw = flYaw;

			// Now blend in his idle animation.
			// This makes the 8-way blend act like a 9-way blend by blending to 
			// an idle sequence as he slows down.
#if defined(CLIENT_DLL)
			bool bIsMoving;
			CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( MAIN_IDLE_SEQUENCE_LAYER );
			
			pLayer->m_flWeight = 1 - CalcMovementPlaybackRate( &bIsMoving );
			if ( !bIsMoving )
			{
				pLayer->m_flWeight = 1;
			}

			if ( ShouldChangeSequences() )
			{
				// Whenever this layer stops blending, we can choose a new idle sequence to blend to, so he 
				// doesn't always use the same idle.
				if ( pLayer->m_flWeight < 0.02f || m_iCurrent8WayIdleSequence == -1 )
				{
					m_iCurrent8WayIdleSequence = m_pOuter->SelectWeightedSequence( ACT_IDLE );
					m_iCurrent8WayCrouchIdleSequence = m_pOuter->SelectWeightedSequence( ACT_CROUCHIDLE );
				}

				if ( m_eCurrentMainSequenceActivity == ACT_CROUCHIDLE || m_eCurrentMainSequenceActivity == ACT_RUN_CROUCH )
					pLayer->m_nSequence = m_iCurrent8WayCrouchIdleSequence;
				else
					pLayer->m_nSequence = m_iCurrent8WayIdleSequence;
			}
			
			pLayer->m_flPlaybackRate = 1;
			pLayer->m_flCycle += m_pOuter->GetSequenceCycleRate( pStudioHdr, pLayer->m_nSequence ) * gpGlobals->frametime;
			pLayer->m_flCycle = fmod( pLayer->m_flCycle, 1 );
			pLayer->m_nOrder = MAIN_IDLE_SEQUENCE_LAYER;
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePlayerAnimState::ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr )
{
	VPROF( "CBasePlayerAnimState::ComputePoseParam_BodyPitch" );

	// Get pitch from v_angle
	float flPitch = m_flEyePitch;
	if ( flPitch > 180.0f )
	{
		flPitch -= 360.0f;
	}
	flPitch = clamp( flPitch, -90.f, 90.f );

	// See if we have a blender for pitch
	int pitch = GetOuter()->LookupPoseParameter( pStudioHdr, "body_pitch" );
	if ( pitch < 0 )
		return;

	GetOuter()->SetPoseParameter( pStudioHdr, pitch, flPitch );
	g_flLastBodyPitch = flPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : goal - 
//			maxrate - 
//			dt - 
//			current - 
// Output : int
//-----------------------------------------------------------------------------
int CBasePlayerAnimState::ConvergeAngles( float goal,float maxrate, float maxgap, float dt, float& current )
{
	int direction = TURN_NONE;

	float anglediff = goal - current;
	anglediff = AngleNormalize( anglediff );
	
	float anglediffabs = fabs( anglediff );

	float scale = 1.0f;
	if ( anglediffabs <= FADE_TURN_DEGREES )
	{
		scale = anglediffabs / FADE_TURN_DEGREES;
		// Always do at least a bit of the turn ( 1% )
		scale = clamp( scale, 0.01f, 1.0f );
	}

	float maxmove = maxrate * dt * scale;

	if ( anglediffabs > maxgap )
	{
		// gap is too big, jump
		maxmove = (anglediffabs - maxgap);
	}

	if ( anglediffabs < maxmove )
	{
		// we are close enought, just set the final value
		current = goal;
	}
	else
	{
		// adjust value up or down
		if ( anglediff > 0 )
		{
			current += maxmove;
			direction = TURN_LEFT;
		}
		else
		{
			current -= maxmove;
			direction = TURN_RIGHT;
		}
	}

	current = AngleNormalize( current );

	return direction;
}

void CBasePlayerAnimState::ComputePoseParam_BodyYaw()
{
	VPROF( "CBasePlayerAnimState::ComputePoseParam_BodyYaw" );

	// Find out which way he's running (m_flEyeYaw is the way he's looking).
	Vector vel;
	GetOuterAbsVelocity( vel );
	bool bIsMoving = vel.Length2D() > MOVING_MINIMUM_SPEED;

	// If we just initialized this guy (maybe he just came into the PVS), then immediately
	// set his feet in the right direction, otherwise they'll spin around from 0 to the 
	// right direction every time someone switches spectator targets.
	if ( !m_bCurrentFeetYawInitialized )
	{
		m_bCurrentFeetYawInitialized = true;
		m_flGoalFeetYaw = m_flCurrentFeetYaw = m_flEyeYaw;
		m_flLastTurnTime = 0.0f;
	}
	else if ( bIsMoving )
	{
		// player is moving, feet yaw = aiming yaw
		if ( m_AnimConfig.m_LegAnimType == LEGANIM_9WAY || m_AnimConfig.m_LegAnimType == LEGANIM_8WAY )
		{
			// His feet point in the direction his eyes are, but they can run in any direction.
			m_flGoalFeetYaw = m_flEyeYaw;
		}
		else
		{
			m_flGoalFeetYaw = RAD2DEG( atan2( vel.y, vel.x ) );

			// If he's running backwards, flip his feet backwards.
			Vector vEyeYaw( cos( DEG2RAD( m_flEyeYaw ) ), sin( DEG2RAD( m_flEyeYaw ) ), 0 );
			Vector vFeetYaw( cos( DEG2RAD( m_flGoalFeetYaw ) ), sin( DEG2RAD( m_flGoalFeetYaw ) ), 0 );
			if ( vEyeYaw.Dot( vFeetYaw ) < -0.01 )
			{
				m_flGoalFeetYaw += 180;
			}
		}

	}
	else if ( (gpGlobals->curtime - m_flLastTurnTime) > mp_facefronttime.GetFloat() )
	{
		// player didn't move & turn for quite some time
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	else
	{
		// If he's rotated his view further than the model can turn, make him face forward.
		float flDiff = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

		if ( fabs(flDiff) > m_AnimConfig.m_flMaxBodyYawDegrees )
		{
			if ( flDiff  > 0 )
				m_flGoalFeetYaw -= m_AnimConfig.m_flMaxBodyYawDegrees;
			else
				m_flGoalFeetYaw += m_AnimConfig.m_flMaxBodyYawDegrees;
		}
	}

	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );

	if ( m_flCurrentFeetYaw != m_flGoalFeetYaw )
	{
		ConvergeAngles( m_flGoalFeetYaw, mp_feetyawrate.GetFloat(), m_AnimConfig.m_flMaxBodyYawDegrees,
			 gpGlobals->frametime, m_flCurrentFeetYaw );

		m_flLastTurnTime = gpGlobals->curtime;
	}

	float flCurrentTorsoYaw = AngleNormalize( m_flEyeYaw - m_flCurrentFeetYaw );

	// Rotate entire body into position
	m_angRender[YAW] = m_flCurrentFeetYaw;
	m_angRender[PITCH] = m_angRender[ROLL] = 0;
		
	SetOuterBodyYaw( flCurrentTorsoYaw );
	g_flLastBodyYaw = flCurrentTorsoYaw;
}



float CBasePlayerAnimState::SetOuterBodyYaw( float flValue )
{
	int body_yaw = GetOuter()->LookupPoseParameter( "body_yaw" );
	if ( body_yaw < 0 )
	{
		return 0;
	}

	SetOuterPoseParameter( body_yaw, flValue );
	return flValue;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CBasePlayerAnimState::BodyYawTranslateActivity( Activity activity )
{
	// Not even standing still, sigh
	if ( activity != ACT_IDLE )
		return activity;

	// Not turning
	switch ( m_nTurningInPlace )
	{
	default:
	case TURN_NONE:
		return activity;
	case TURN_RIGHT:
	case TURN_LEFT:
		return mp_ik.GetBool() ? ACT_TURN : activity;
	}

	Assert( 0 );
	return activity;
}

const QAngle& CBasePlayerAnimState::GetRenderAngles()
{
	return m_angRender;
}


void CBasePlayerAnimState::GetOuterAbsVelocity( Vector& vel ) const
{
#if defined( CLIENT_DLL )
	GetOuter()->EstimateAbsVelocity( vel );
#else
	vel = GetOuter()->GetAbsVelocity();
#endif
}


float CBasePlayerAnimState::GetOuterXYSpeed() const
{
	Vector vel;
	GetOuterAbsVelocity( vel );
	return vel.Length2D();
}

// -----------------------------------------------------------------------------
void CBasePlayerAnimState::AnimStateLog( const char *pMsg, ... )
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start( marker, pMsg );
	Q_vsnprintf( str, sizeof( str ), pMsg, marker );
	va_end( marker );

	// Log it?	
	if ( showanimstate_log.GetInt() == 1 || showanimstate_log.GetInt() == 3 )
	{
		Msg( "%s", str );
	}

	if ( showanimstate_log.GetInt() > 1 )
	{
#ifdef CLIENT_DLL
		const char *fname = "AnimStateClient.log";
#else
		const char *fname = "AnimStateServer.log";
#endif
		static FileHandle_t hFile = filesystem->Open( fname, "wt" );
		filesystem->FPrintf( hFile, "%s", str );
		filesystem->Flush( hFile );
	}
}


// -----------------------------------------------------------------------------
void CBasePlayerAnimState::AnimStatePrintf( int iLine, const char *pMsg, ... )
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
	AnimStateLog( "%s\n", str );
}


// -----------------------------------------------------------------------------
void CBasePlayerAnimState::DebugShowAnimState( int iStartLine )
{
	Vector vOuterVel;
	GetOuterAbsVelocity( vOuterVel );

	int iLine = iStartLine;
	AnimStatePrintf( iLine++, "main: %s(%d), cycle: %.2f cyclerate: %.2f playbackrate: %.2f\n", 
		GetSequenceName( m_pOuter->GetModelPtr(), m_pOuter->GetSequence() ), 
		m_pOuter->GetSequence(),
		m_pOuter->GetCycle(), 
		m_pOuter->GetSequenceCycleRate(m_pOuter->GetModelPtr(), m_pOuter->GetSequence()),
		m_pOuter->GetPlaybackRate()
		);

	if ( m_AnimConfig.m_LegAnimType == LEGANIM_8WAY )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( MAIN_IDLE_SEQUENCE_LAYER );

		AnimStatePrintf( iLine++, "idle: %s, weight: %.2f\n",
			GetSequenceName( m_pOuter->GetModelPtr(), pLayer->m_nSequence ), 
			(float)pLayer->m_flWeight );
	}

	for ( int i=0; i < m_pOuter->GetNumAnimOverlays()-1; i++ )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( AIMSEQUENCE_LAYER + i );
#ifdef CLIENT_DLL
		AnimStatePrintf( iLine++, "%s(%d), weight: %.2f, cycle: %.2f, order (%d), aim (%d)", 
			!pLayer->IsActive() ? "-- ": (pLayer->m_nSequence == 0 ? "-- " : GetSequenceName( m_pOuter->GetModelPtr(), pLayer->m_nSequence ) ), 
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nSequence, 
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flWeight, 
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flCycle, 
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nOrder,
			i
			);
#else
		AnimStatePrintf( iLine++, "%s(%d), flags (%d), weight: %.2f, cycle: %.2f, order (%d), aim (%d)", 
			!pLayer->IsActive() ? "-- " : ( pLayer->m_nSequence == 0 ? "-- " : GetSequenceName( m_pOuter->GetModelPtr(), pLayer->m_nSequence ) ), 
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nSequence, 
			!pLayer->IsActive() ? 0 : (int)pLayer->m_fFlags,// Doesn't exist on client
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flWeight, 
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flCycle, 
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nOrder,
			i
			);
#endif
	}

	AnimStatePrintf( iLine++, "vel: %.2f, time: %.2f, max: %.2f, animspeed: %.2f", 
		vOuterVel.Length2D(), gpGlobals->curtime, GetInterpolatedGroundSpeed(), m_pOuter->GetSequenceGroundSpeed(m_pOuter->GetSequence()) );
	
	if ( m_AnimConfig.m_LegAnimType == LEGANIM_8WAY )
	{
		AnimStatePrintf( iLine++, "ent yaw: %.2f, body_yaw: %.2f, move_yaw: %.2f, gait_yaw: %.2f, body_pitch: %.2f", 
			m_angRender[YAW], g_flLastBodyYaw, m_flLastMoveYaw, m_flGaitYaw, g_flLastBodyPitch );
	}
	else
	{
		AnimStatePrintf( iLine++, "ent yaw: %.2f, body_yaw: %.2f, body_pitch: %.2f, move_x: %.2f, move_y: %.2f", 
			m_angRender[YAW], g_flLastBodyYaw, g_flLastBodyPitch, m_vLastMovePose.x, m_vLastMovePose.y );
	}

	// Draw a red triangle on the ground for the eye yaw.
	float flBaseSize = 10;
	float flHeight = 80;
	Vector vBasePos = GetOuter()->GetAbsOrigin() + Vector( 0, 0, 3 );
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

// -----------------------------------------------------------------------------
void CBasePlayerAnimState::DebugShowAnimStateFull( int iStartLine )
{
	AnimStateLog( "----------------- frame %d -----------------\n", gpGlobals->framecount );

	DebugShowAnimState( iStartLine );

	AnimStateLog( "--------------------------------------------\n\n" );
}

// -----------------------------------------------------------------------------
int CBasePlayerAnimState::SelectWeightedSequence( Activity activity ) 
{
	return GetOuter()->SelectWeightedSequence( activity ); 
}

