//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Single Player animation state 'handler'. This utility is used
//            to evaluate the pose parameter value based on the direction
//            and speed of the player.
// 
// ------------------------------------------------------------------------------
//
// This was originally based on the following VDC article:
// https://developer.valvesoftware.com/wiki/Fixing_the_player_animation_state_(Single_Player)
//
// It has been modified by Blixibon to derive from CBasePlayerAnimState instead and support 9-way blends.
// Much of the work done to make this derive from CBasePlayerAnimState utilized code from the Alien Swarm SDK.
// 
//=============================================================================//

#include "cbase.h"
#include "singleplayer_animstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "filesystem.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"

extern ConVar mp_facefronttime, mp_feetyawrate, mp_ik;

ConVar sv_playeranimstate_animtype( "sv_playeranimstate_animtype", "0", FCVAR_NONE, "The leg animation type used by the singleplayer animation state. 9way = 0, 8way = 1, GoldSrc = 2" );
ConVar sv_playeranimstate_bodyyaw( "sv_playeranimstate_bodyyaw", "45.0", FCVAR_NONE, "The maximum body yaw used by the singleplayer animation state." );
ConVar sv_playeranimstate_use_aim_sequences( "sv_playeranimstate_use_aim_sequences", "1", FCVAR_NONE, "Allows the singleplayer animation state to use aim sequences." );

#define MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION        15.0f

#define FIRESEQUENCE_LAYER		(AIMSEQUENCE_LAYER+NUM_AIMSEQUENCE_LAYERS)
#define RELOADSEQUENCE_LAYER	(FIRESEQUENCE_LAYER + 1)
#define NUM_LAYERS_WANTED		(RELOADSEQUENCE_LAYER + 1)

CSinglePlayerAnimState *CreatePlayerAnimationState( CBasePlayer *pPlayer )
{
    MDLCACHE_CRITICAL_SECTION();

    CSinglePlayerAnimState *pState = new CSinglePlayerAnimState( pPlayer );

    // Setup the movement data.
    CModAnimConfig movementData;
    movementData.m_LegAnimType = (LegAnimType_t)sv_playeranimstate_animtype.GetInt();
    movementData.m_flMaxBodyYawDegrees = sv_playeranimstate_bodyyaw.GetFloat();
    movementData.m_bUseAimSequences = sv_playeranimstate_use_aim_sequences.GetBool();

    pState->Init( pPlayer, movementData );

    return pState;
}

// Below this many degrees, slow down turning rate linearly
#define FADE_TURN_DEGREES    45.0f
// After this, need to start turning feet
#define MAX_TORSO_ANGLE        90.0f
// Below this amount, don't play a turning animation/perform IK
#define MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION        15.0f

//static ConVar tf2_feetyawrunscale( "tf2_feetyawrunscale", "2", FCVAR_REPLICATED, "Multiplier on tf2_feetyawrate to allow turning faster when running." );
extern ConVar sv_backspeed;
extern ConVar mp_feetyawrate;
extern ConVar mp_facefronttime;
extern ConVar mp_ik;

CSinglePlayerAnimState::CSinglePlayerAnimState( CBasePlayer *pPlayer ): m_pPlayer( pPlayer )
{
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CSinglePlayerAnimState::CalcMainActivity()
{
#ifdef CLIENT_DLL
    return ACT_IDLE;
#else
    float speed = GetOuter()->GetAbsVelocity().Length2D();

    if ( HandleJumping() )
	{
		return ACT_HL2MP_JUMP;
	}
    else
    {
        Activity idealActivity = ACT_HL2MP_IDLE;

        if ( GetOuter()->GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	    {
		    speed = 0;
	    }
        else
        {
            if ( GetOuter()->GetFlags() & FL_DUCKING )
			{
				if ( speed > 0 )
				{
					idealActivity = ACT_HL2MP_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE_CROUCH;
				}
			}
			else
			{
				if ( speed > 0 )
				{
#if EXPANDED_HL2DM_ACTIVITIES
					if ( m_pPlayer->GetButtons() & IN_WALK )
					{
						idealActivity = ACT_HL2MP_WALK;
					}
					else
#endif
					{
						idealActivity = ACT_HL2MP_RUN;
					}
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE;
				}
			}
        }

        return idealActivity;
    }

    //return m_pPlayer->GetActivity();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::SetPlayerAnimation( PLAYER_ANIM playerAnim )
{
    if ( playerAnim == PLAYER_ATTACK1 )
    {
        m_iFireSequence = SelectWeightedSequence( TranslateActivity( ACT_HL2MP_GESTURE_RANGE_ATTACK ) );
        m_bFiring = m_iFireSequence != -1;
        m_flFireCycle = 0;
    }
    else if ( playerAnim == PLAYER_ATTACK2 )
    {
#if EXPANDED_HL2DM_ACTIVITIES
        m_iFireSequence = SelectWeightedSequence( TranslateActivity( ACT_HL2MP_GESTURE_RANGE_ATTACK2 ) );
#else
        m_iFireSequence = SelectWeightedSequence( TranslateActivity( ACT_HL2MP_GESTURE_RANGE_ATTACK ) );
#endif
        m_bFiring = m_iFireSequence != -1;
        m_flFireCycle = 0;
    }
    else if ( playerAnim == PLAYER_JUMP )
    {
        // Play the jump animation.
        if (!m_bJumping)
        {
            m_bJumping = true;
            m_bFirstJumpFrame = true;
            m_flJumpStartTime = gpGlobals->curtime;
        }
    }
    else if ( playerAnim == PLAYER_RELOAD )
    {
        m_iReloadSequence = SelectWeightedSequence( TranslateActivity( ACT_HL2MP_GESTURE_RELOAD ) );
        if (m_iReloadSequence != -1)
        {
            // clear other events that might be playing in our layer
			m_bWeaponSwitching = false;
            m_fReloadPlaybackRate = 1.0f;
			m_bReloading = true;			
			m_flReloadCycle = 0;
        }
    }
    else if ( playerAnim == PLAYER_UNHOLSTER || playerAnim == PLAYER_HOLSTER )
    {
        m_iWeaponSwitchSequence = SelectWeightedSequence( TranslateActivity( playerAnim == PLAYER_UNHOLSTER ? ACT_ARM : ACT_DISARM ) );
        if (m_iWeaponSwitchSequence != -1)
        {
            // clear other events that might be playing in our layer
            m_bPlayingMisc = false;
            m_bReloading = false;

            m_bWeaponSwitching = true;
            m_flWeaponSwitchCycle = 0;
            m_flMiscBlendOut = 0.1f;
            m_flMiscBlendIn = 0.1f;
            m_bMiscNoOverride = false;
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Activity CSinglePlayerAnimState::TranslateActivity( Activity actDesired )
{
#ifdef CLIENT_DLL
    return actDesired;
#else
    return m_pPlayer->Weapon_TranslateActivity( actDesired );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CSinglePlayerAnimState::HandleJumping()
{
	if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		if (m_flJumpStartTime > gpGlobals->curtime)
			m_flJumpStartTime = gpGlobals->curtime;
		if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f)
		{
			if ( m_pOuter->GetFlags() & FL_ONGROUND || GetOuter()->GetGroundEntity() != NULL)
			{
				m_bJumping = false;
				RestartMainSequence();	// Reset the animation.				
			}
		}
	}

	// Are we still jumping? If so, keep playing the jump animation.
	return m_bJumping;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
    CBasePlayerAnimState::ComputeSequences(pStudioHdr);

	ComputeFireSequence();
	ComputeMiscSequence();
	ComputeReloadSequence();
	ComputeWeaponSwitchSequence();	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::AddMiscSequence( int iSequence, float flBlendIn, float flBlendOut, float flPlaybackRate, bool bHoldAtEnd, bool bOnlyWhenStill )
{
    Assert( iSequence != -1 );

	m_iMiscSequence = iSequence;
    m_flMiscBlendIn = flBlendIn;
    m_flMiscBlendOut = flBlendOut;

    m_bPlayingMisc = true;
    m_bMiscHoldAtEnd = bHoldAtEnd;
    m_bReloading = false;
    m_flMiscCycle = 0;
    m_bMiscOnlyWhenStill = bOnlyWhenStill;
    m_bMiscNoOverride = true;
    m_fMiscPlaybackRate = flPlaybackRate;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ClearAnimationState()
{
	m_bJumping = false;
	m_bFiring = false;
	m_bReloading = false;
	m_bWeaponSwitching = false;
	m_bPlayingMisc = false;
    m_flReloadBlendIn = 0.0f;
    m_flReloadBlendOut = 0.0f;
    m_flMiscBlendIn = 0.0f;
    m_flMiscBlendOut = 0.0f;
	CBasePlayerAnimState::ClearAnimationState();
}

void CSinglePlayerAnimState::ClearAnimationLayers()
{
	VPROF( "CBasePlayerAnimState::ClearAnimationLayers" );
	if ( !m_pOuter )
		return;

	m_pOuter->SetNumAnimOverlays( NUM_LAYERS_WANTED );
	for ( int i=0; i < m_pOuter->GetNumAnimOverlays(); i++ )
	{
		m_pOuter->GetAnimOverlay( i )->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#ifndef CLIENT_DLL
		m_pOuter->GetAnimOverlay( i )->m_fFlags = 0;
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSinglePlayerAnimState::CalcAimLayerSequence( float *flCycle, float *flAimSequenceWeight, bool bForceIdle )
{
    // TODO?
    return m_pOuter->LookupSequence( "soldier_Aim_9_directions" );
}

void CSinglePlayerAnimState::UpdateLayerSequenceGeneric( int iLayer, bool &bEnabled,
					float &flCurCycle, int &iSequence, bool bWaitAtEnd,
					float fBlendIn, float fBlendOut, bool bMoveBlend, float fPlaybackRate, bool bUpdateCycle /* = true */ )
{
	if ( !bEnabled )
		return;

	CStudioHdr *hdr = GetOuter()->GetModelPtr();
	if ( !hdr )
		return;

	if ( iSequence < 0 || iSequence >= hdr->GetNumSeq() )
		return;

	// Increment the fire sequence's cycle.
	if ( bUpdateCycle )
	{
		flCurCycle += m_pOuter->GetSequenceCycleRate( hdr, iSequence ) * gpGlobals->frametime * fPlaybackRate;
	}

	// temp: if the sequence is looping, don't override it - we need better handling of looping anims, 
	//  especially in misc layer from melee (right now the same melee attack is looped manually in asw_melee_system.cpp)
	bool bLooping = m_pOuter->IsSequenceLooping( hdr, iSequence );

	if ( flCurCycle > 1 && !bLooping )
	{
		if ( iLayer == RELOADSEQUENCE_LAYER )
		{
			m_bReloading = false;
		}
		if ( bWaitAtEnd )
		{
			flCurCycle = 1;
		}
		else
		{
			// Not firing anymore.
			bEnabled = false;
			iSequence = 0;
			return;
		}
	}

	// if this animation should blend out as we move, then check for dropping it completely since we're moving too fast
	float speed = 0;
	if (bMoveBlend)
	{
		Vector vel;
		GetOuterAbsVelocity( vel );

		float speed = vel.Length2D();

		if (speed > 50)
		{
			bEnabled = false;
			iSequence = 0;
			return;
		}
	}

	// Now dump the state into its animation layer.
	CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iLayer );

	pLayer->m_flCycle = flCurCycle;
	pLayer->m_nSequence = iSequence;

	pLayer->m_flPlaybackRate = fPlaybackRate;
	pLayer->m_flWeight = 1.0f;

	if (iLayer == RELOADSEQUENCE_LAYER)
	{
		// blend this layer in and out for smooth reloading
		if (flCurCycle < fBlendIn && fBlendIn>0)
		{
			pLayer->m_flWeight = ( clamp<float>(flCurCycle / fBlendIn,
				0.001f, 1.0f) );
		}
		else if (flCurCycle >= (1.0f - fBlendOut) && fBlendOut>0)
		{
			pLayer->m_flWeight = ( clamp<float>((1.0f - flCurCycle) / fBlendOut,
				0.001f, 1.0f) );
		}
		else
		{
			pLayer->m_flWeight = 1.0f;
		}
	}
	else
	{
		pLayer->m_flWeight = 1.0f;
	}
	if (bMoveBlend)		
	{
		// blend the animation out as we move faster
		if (speed <= 50)			
			pLayer->m_flWeight = ( pLayer->m_flWeight * (50.0f - speed) / 50.0f );
	}

#ifndef CLIENT_DLL
	pLayer->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	pLayer->SetOrder( iLayer );
}

void CSinglePlayerAnimState::ComputeFireSequence()
{
	UpdateLayerSequenceGeneric( FIRESEQUENCE_LAYER, m_bFiring, m_flFireCycle, m_iFireSequence, false );
}

void CSinglePlayerAnimState::ComputeReloadSequence()
{
	UpdateLayerSequenceGeneric( RELOADSEQUENCE_LAYER, m_bReloading, m_flReloadCycle, m_iReloadSequence, false, m_flReloadBlendIn, m_flReloadBlendOut, false, m_fReloadPlaybackRate );
}

void CSinglePlayerAnimState::ComputeWeaponSwitchSequence()
{
	UpdateLayerSequenceGeneric( RELOADSEQUENCE_LAYER, m_bWeaponSwitching, m_flWeaponSwitchCycle, m_iWeaponSwitchSequence, false, 0, 0.5f );
}

// does misc gestures if we're not firing
void CSinglePlayerAnimState::ComputeMiscSequence()
{
	UpdateLayerSequenceGeneric( RELOADSEQUENCE_LAYER, m_bPlayingMisc, m_flMiscCycle, m_iMiscSequence, m_bMiscHoldAtEnd, m_flMiscBlendIn, m_flMiscBlendOut, m_bMiscOnlyWhenStill, m_fMiscPlaybackRate );	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float CSinglePlayerAnimState::GetCurrentMaxGroundSpeed()
{
	CStudioHdr *pStudioHdr = GetOuter()->GetModelPtr();

	if ( pStudioHdr == NULL )
		return 1.0f;

    int iMoveX = GetOuter()->LookupPoseParameter( "move_x" );
    int iMoveY = GetOuter()->LookupPoseParameter( "move_y" );

	float prevX = GetOuter()->GetPoseParameter( iMoveX );
	float prevY = GetOuter()->GetPoseParameter( iMoveY );

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

    GetOuter()->SetPoseParameter( pStudioHdr, iMoveX, newX );
    GetOuter()->SetPoseParameter( pStudioHdr, iMoveY, newY );

	float speed = GetOuter()->GetSequenceGroundSpeed(GetOuter()->GetSequence() );

    GetOuter()->SetPoseParameter( pStudioHdr, iMoveX, prevX );
    GetOuter()->SetPoseParameter( pStudioHdr, iMoveY, prevY );

	return speed;
}

//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt -
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePoseParam_BodyYaw( void )
{
    CBasePlayerAnimState::ComputePoseParam_BodyYaw();

    //ComputePoseParam_BodyLookYaw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePoseParam_BodyLookYaw( void )
{
    // See if we even have a blender for pitch
    int upper_body_yaw = GetOuter()->LookupPoseParameter( "aim_yaw" );
    if ( upper_body_yaw < 0 )
    {
        return;
    }

    // Assume upper and lower bodies are aligned and that we're not turning
    float flGoalTorsoYaw = 0.0f;
    int turning = TURN_NONE;
    float turnrate = 360.0f;

    Vector vel;
   
    GetOuterAbsVelocity( vel );

    bool isMoving = ( vel.Length() > 1.0f ) ? true : false;

    if ( !isMoving )
    {
        // Just stopped moving, try and clamp feet
        if ( m_flLastTurnTime <= 0.0f )
        {
            m_flLastTurnTime    = gpGlobals->curtime;
            m_flLastYaw            = GetOuter()->EyeAngles().y;
            // Snap feet to be perfectly aligned with torso/eyes
            m_flGoalFeetYaw        = GetOuter()->EyeAngles().y;
            m_flCurrentFeetYaw    = m_flGoalFeetYaw;
            m_nTurningInPlace    = TURN_NONE;
        }

        // If rotating in place, update stasis timer

        if ( m_flLastYaw != GetOuter()->EyeAngles().y )
        {
            m_flLastTurnTime    = gpGlobals->curtime;
            m_flLastYaw            = GetOuter()->EyeAngles().y;
        }

        if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
        {
            m_flLastTurnTime    = gpGlobals->curtime;
        }

        turning = ConvergeAngles( m_flGoalFeetYaw, turnrate, m_AnimConfig.m_flMaxBodyYawDegrees, gpGlobals->frametime, m_flCurrentFeetYaw );

        QAngle eyeAngles = GetOuter()->EyeAngles();
        QAngle vAngle = GetOuter()->GetLocalAngles();

        // See how far off current feetyaw is from true yaw
        float yawdelta = GetOuter()->EyeAngles().y - m_flCurrentFeetYaw;
        yawdelta = AngleNormalize( yawdelta );

        bool rotated_too_far = false;

        float yawmagnitude = fabs( yawdelta );

        // If too far, then need to turn in place
        if ( yawmagnitude > 45 )
        {
            rotated_too_far = true;
        }

        // Standing still for a while, rotate feet around to face forward
        // Or rotated too far
        // FIXME:  Play an in place turning animation
        if ( rotated_too_far ||
            ( gpGlobals->curtime > m_flLastTurnTime + mp_facefronttime.GetFloat() ) )
        {
            m_flGoalFeetYaw        = GetOuter()->EyeAngles().y;
            m_flLastTurnTime    = gpGlobals->curtime;

        /*    float yd = m_flCurrentFeetYaw - m_flGoalFeetYaw;
            if ( yd > 0 )
            {
                m_nTurningInPlace = TURN_RIGHT;
            }
            else if ( yd < 0 )
            {
                m_nTurningInPlace = TURN_LEFT;
            }
            else
            {
                m_nTurningInPlace = TURN_NONE;
            }

            turning = ConvergeAngles( m_flGoalFeetYaw, turnrate, gpGlobals->frametime, m_flCurrentFeetYaw );
            yawdelta = GetOuter()->EyeAngles().y - m_flCurrentFeetYaw;*/

        }

        // Snap upper body into position since the delta is already smoothed for the feet
        flGoalTorsoYaw = yawdelta;
        m_flCurrentTorsoYaw = flGoalTorsoYaw;
    }
    else
    {
        m_flLastTurnTime = 0.0f;
        m_nTurningInPlace = TURN_NONE;
        m_flCurrentFeetYaw = m_flGoalFeetYaw = GetOuter()->EyeAngles().y;
        flGoalTorsoYaw = 0.0f;
        m_flCurrentTorsoYaw = GetOuter()->EyeAngles().y - m_flCurrentFeetYaw;
    }

    if ( turning == TURN_NONE )
    {
        m_nTurningInPlace = turning;
    }

    if ( m_nTurningInPlace != TURN_NONE )
    {
        // If we're close to finishing the turn, then turn off the turning animation
        if ( fabs( m_flCurrentFeetYaw - m_flGoalFeetYaw ) < MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION )
        {
            m_nTurningInPlace = TURN_NONE;
        }
    }

    GetOuter()->SetPoseParameter( upper_body_yaw, clamp( m_flCurrentTorsoYaw, -60.0f, 60.0f ) );

    /*
    // FIXME: Adrian, what is this?
    int body_yaw = GetOuter()->LookupPoseParameter( "body_yaw" );

    if ( body_yaw >= 0 )
    {
        GetOuter()->SetPoseParameter( body_yaw, 30 );
    }
    */

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr )
{
    // Get pitch from v_angle
    float flPitch = m_flEyePitch;

    if ( flPitch > 180.0f )
    {
        flPitch -= 360.0f;
    }
    flPitch = clamp( flPitch, -90, 90 );

    // See if we have a blender for pitch
    GetOuter()->SetPoseParameter( pStudioHdr, "aim_pitch", flPitch );

    ComputePoseParam_HeadPitch( pStudioHdr );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePoseParam_HeadPitch( CStudioHdr *pStudioHdr )
{
    // Get pitch from v_angle
    int iHeadPitch = GetOuter()->LookupPoseParameter("head_pitch");

    float flPitch = m_flEyePitch;

    if ( flPitch > 180.0f )
    {
        flPitch -= 360.0f;
    }
    flPitch = clamp( flPitch, -90, 90 );

    GetOuter()->SetPoseParameter( pStudioHdr, iHeadPitch, flPitch );
}
