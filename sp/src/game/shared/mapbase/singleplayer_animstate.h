//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Single Player animation state 'handler'. This utility is used
//            to evaluate the pose parameter value based on the direction
//            and speed of the player.
//
//====================================================================================//

#ifndef SINGLEPLAYER_ANIMSTATE_H
#define SINGLEPLAYER_ANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_baseplayer.h"
#else
#include "player.h"
#endif

#ifdef MAPBASE
// Special definition for differentiating between SP and HL2:DM anim states
#define SP_ANIM_STATE 1
#endif

class CSinglePlayerAnimState
{
public:
    enum
    {
        TURN_NONE = 0,
        TURN_LEFT,
        TURN_RIGHT
    };

    CSinglePlayerAnimState( CBasePlayer *pPlayer );

    void Init( CBasePlayer *pPlayer );

    Activity            BodyYawTranslateActivity( Activity activity );

    void                Update();

    const QAngle&        GetRenderAngles();
               
    void                GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );

    CBasePlayer        *GetBasePlayer();

    void Release();

private:
    void                GetOuterAbsVelocity( Vector& vel );

    int                    ConvergeAngles( float goal,float maxrate, float dt, float& current );

    void                EstimateYaw( void );
    void                ComputePoseParam_BodyYaw( void );
    void                ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr );
    void                ComputePoseParam_BodyLookYaw( void );
    void                ComputePoseParam_HeadPitch( CStudioHdr *pStudioHdr );
    void                ComputePlaybackRate();

    CBasePlayer        *m_pPlayer;

    float                m_flGaitYaw;
    float                m_flStoredCycle;

    float                m_flGoalFeetYaw;
    float                m_flCurrentFeetYaw;

    float                m_flCurrentTorsoYaw;

    float                m_flLastYaw;
    float                m_flLastTurnTime;

    int                    m_nTurningInPlace;

    QAngle                m_angRender;

    float                m_flTurnCorrectionTime;
};

CSinglePlayerAnimState *CreatePlayerAnimationState( CBasePlayer *pPlayer );

#endif // SINGLEPLAYER_ANIMSTATE_H
