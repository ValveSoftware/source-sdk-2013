//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Single Player animation state 'handler'. This utility is used
//            to evaluate the pose parameter value based on the direction
//            and speed of the player.
//
//====================================================================================//

#include "cbase.h"
#include "singleplayer_animstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "filesystem.h"
#include "..\public\datacache\imdlcache.h"

extern ConVar mp_facefronttime, mp_feetyawrate, mp_ik;

#define MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION        15.0f

CSinglePlayerAnimState *CreatePlayerAnimationState( CBasePlayer *pPlayer )
{
    MDLCACHE_CRITICAL_SECTION();

    CSinglePlayerAnimState *pState = new CSinglePlayerAnimState( pPlayer );
    pState->Init(pPlayer);

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
    m_flGaitYaw = 0.0f;
    m_flGoalFeetYaw = 0.0f;
    m_flCurrentFeetYaw = 0.0f;
    m_flCurrentTorsoYaw = 0.0f;
    m_flLastYaw = 0.0f;
    m_flLastTurnTime = 0.0f;
    m_flTurnCorrectionTime = 0.0f;

    m_pPlayer = NULL;
};

void CSinglePlayerAnimState::Init( CBasePlayer *pPlayer )
{
    m_pPlayer = pPlayer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::Update()
{
    m_angRender = GetBasePlayer()->GetLocalAngles();

    ComputePoseParam_BodyYaw();
    ComputePoseParam_BodyPitch(GetBasePlayer()->GetModelPtr());
    ComputePoseParam_BodyLookYaw();
    ComputePoseParam_HeadPitch(GetBasePlayer()->GetModelPtr());

    ComputePlaybackRate();
}

void CSinglePlayerAnimState::Release()
{
    delete this;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePlaybackRate()
{
    // Determine ideal playback rate
    Vector vel;
    GetOuterAbsVelocity( vel );

    float speed = vel.Length2D();

    bool isMoving = ( speed > 0.5f ) ? true : false;

    float maxspeed = GetBasePlayer()->GetSequenceGroundSpeed( GetBasePlayer()->GetSequence() );
   
    if ( isMoving && ( maxspeed > 0.0f ) )
    {
        float flFactor = 1.0f;

        // Note this gets set back to 1.0 if sequence changes due to ResetSequenceInfo below
        GetBasePlayer()->SetPlaybackRate( ( speed * flFactor ) / maxspeed );

        // BUG BUG:
        // This stuff really should be m_flPlaybackRate = speed / m_flGroundSpeed
    }
    else
    {
        GetBasePlayer()->SetPlaybackRate( 1.0f );
    }
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *CSinglePlayerAnimState::GetBasePlayer()
{
    return m_pPlayer;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : dt -
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::EstimateYaw( void )
{
    float dt = gpGlobals->frametime;

    if ( !dt )
    {
        return;
    }

    Vector est_velocity;
    QAngle    angles;

    GetOuterAbsVelocity( est_velocity );

    angles = GetBasePlayer()->GetLocalAngles();

    if ( est_velocity[1] == 0 && est_velocity[0] == 0 )
    {
        float flYawDiff = angles[YAW] - m_flGaitYaw;
        flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
        if (flYawDiff > 180)
            flYawDiff -= 360;
        if (flYawDiff < -180)
            flYawDiff += 360;

        if (dt < 0.25)
            flYawDiff *= dt * 4;
        else
            flYawDiff *= dt;

        m_flGaitYaw += flYawDiff;
        m_flGaitYaw = m_flGaitYaw - (int)(m_flGaitYaw / 360) * 360;
    }
    else
    {
        m_flGaitYaw = (atan2(est_velocity[1], est_velocity[0]) * 180 / M_PI);

        if (m_flGaitYaw > 180)
            m_flGaitYaw = 180;
        else if (m_flGaitYaw < -180)
            m_flGaitYaw = -180;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt -
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePoseParam_BodyYaw( void )
{
    int iYaw = GetBasePlayer()->LookupPoseParameter( "move_yaw" );
    if ( iYaw < 0 )
        return;

    // view direction relative to movement
    float flYaw;     

    EstimateYaw();

    QAngle    angles = GetBasePlayer()->GetLocalAngles();
    float ang = angles[ YAW ];
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
   
    GetBasePlayer()->SetPoseParameter( iYaw, flYaw );

#ifndef CLIENT_DLL
        //Adrian: Make the model's angle match the legs so the hitboxes match on both sides.
        GetBasePlayer()->SetLocalAngles( QAngle( GetBasePlayer()->EyeAngles().x, m_flCurrentFeetYaw, 0 ) );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr )
{
    // Get pitch from v_angle
    float flPitch = GetBasePlayer()->GetLocalAngles()[ PITCH ];

    if ( flPitch > 180.0f )
    {
        flPitch -= 360.0f;
    }
    flPitch = clamp( flPitch, -90, 90 );

    QAngle absangles = GetBasePlayer()->GetAbsAngles();
    absangles.x = 0.0f;
    m_angRender = absangles;

    // See if we have a blender for pitch
    GetBasePlayer()->SetPoseParameter( pStudioHdr, "aim_pitch", flPitch );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : goal -
//            maxrate -
//            dt -
//            current -
// Output : int
//-----------------------------------------------------------------------------
int CSinglePlayerAnimState::ConvergeAngles( float goal,float maxrate, float dt, float& current )
{
    int direction = TURN_NONE;

    float anglediff = goal - current;
    float anglediffabs = fabs( anglediff );

    anglediff = AngleNormalize( anglediff );

    float scale = 1.0f;
    if ( anglediffabs <= FADE_TURN_DEGREES )
    {
        scale = anglediffabs / FADE_TURN_DEGREES;
        // Always do at least a bit of the turn ( 1% )
        scale = clamp( scale, 0.01f, 1.0f );
    }

    float maxmove = maxrate * dt * scale;

    if ( fabs( anglediff ) < maxmove )
    {
        current = goal;
    }
    else
    {
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

void CSinglePlayerAnimState::ComputePoseParam_BodyLookYaw( void )
{
    QAngle absangles = GetBasePlayer()->GetAbsAngles();
    absangles.y = AngleNormalize( absangles.y );
    m_angRender = absangles;

    // See if we even have a blender for pitch
    int upper_body_yaw = GetBasePlayer()->LookupPoseParameter( "aim_yaw" );
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
            m_flLastYaw            = GetBasePlayer()->EyeAngles().y;
            // Snap feet to be perfectly aligned with torso/eyes
            m_flGoalFeetYaw        = GetBasePlayer()->EyeAngles().y;
            m_flCurrentFeetYaw    = m_flGoalFeetYaw;
            m_nTurningInPlace    = TURN_NONE;
        }

        // If rotating in place, update stasis timer

        if ( m_flLastYaw != GetBasePlayer()->EyeAngles().y )
        {
            m_flLastTurnTime    = gpGlobals->curtime;
            m_flLastYaw            = GetBasePlayer()->EyeAngles().y;
        }

        if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
        {
            m_flLastTurnTime    = gpGlobals->curtime;
        }

        turning = ConvergeAngles( m_flGoalFeetYaw, turnrate, gpGlobals->frametime, m_flCurrentFeetYaw );

        QAngle eyeAngles = GetBasePlayer()->EyeAngles();
        QAngle vAngle = GetBasePlayer()->GetLocalAngles();

        // See how far off current feetyaw is from true yaw
        float yawdelta = GetBasePlayer()->EyeAngles().y - m_flCurrentFeetYaw;
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
            m_flGoalFeetYaw        = GetBasePlayer()->EyeAngles().y;
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
            yawdelta = GetBasePlayer()->EyeAngles().y - m_flCurrentFeetYaw;*/

        }

        // Snap upper body into position since the delta is already smoothed for the feet
        flGoalTorsoYaw = yawdelta;
        m_flCurrentTorsoYaw = flGoalTorsoYaw;
    }
    else
    {
        m_flLastTurnTime = 0.0f;
        m_nTurningInPlace = TURN_NONE;
        m_flCurrentFeetYaw = m_flGoalFeetYaw = GetBasePlayer()->EyeAngles().y;
        flGoalTorsoYaw = 0.0f;
        m_flCurrentTorsoYaw = GetBasePlayer()->EyeAngles().y - m_flCurrentFeetYaw;
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

    // Rotate entire body into position
    absangles = GetBasePlayer()->GetAbsAngles();
    absangles.y = m_flCurrentFeetYaw;
    m_angRender = absangles;

    GetBasePlayer()->SetPoseParameter( upper_body_yaw, clamp( m_flCurrentTorsoYaw, -60.0f, 60.0f ) );

    /*
    // FIXME: Adrian, what is this?
    int body_yaw = GetBasePlayer()->LookupPoseParameter( "body_yaw" );

    if ( body_yaw >= 0 )
    {
        GetBasePlayer()->SetPoseParameter( body_yaw, 30 );
    }
    */

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSinglePlayerAnimState::ComputePoseParam_HeadPitch( CStudioHdr *pStudioHdr )
{
    // Get pitch from v_angle
    int iHeadPitch = GetBasePlayer()->LookupPoseParameter("head_pitch");

    float flPitch = GetBasePlayer()->EyeAngles()[PITCH];

    if ( flPitch > 180.0f )
    {
        flPitch -= 360.0f;
    }
    flPitch = clamp( flPitch, -90, 90 );

    QAngle absangles = GetBasePlayer()->GetAbsAngles();
    absangles.x = 0.0f;
    m_angRender = absangles;

    GetBasePlayer()->SetPoseParameter( pStudioHdr, iHeadPitch, flPitch );
}

 
//-----------------------------------------------------------------------------
// Purpose:
// Input  : activity -
// Output : Activity
//-----------------------------------------------------------------------------
Activity CSinglePlayerAnimState::BodyYawTranslateActivity( Activity activity )
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
    /*
    case TURN_RIGHT:
        return ACT_TURNRIGHT45;
    case TURN_LEFT:
        return ACT_TURNLEFT45;
    */
    case TURN_RIGHT:
    case TURN_LEFT:
        return mp_ik.GetBool() ? ACT_TURN : activity;
    }

    Assert( 0 );
    return activity;
}

const QAngle& CSinglePlayerAnimState::GetRenderAngles()
{
    return m_angRender;
}

void CSinglePlayerAnimState::GetOuterAbsVelocity( Vector& vel )
{
#if defined( CLIENT_DLL )
    GetBasePlayer()->EstimateAbsVelocity( vel );
#else
    vel = GetBasePlayer()->GetAbsVelocity();
#endif
}
