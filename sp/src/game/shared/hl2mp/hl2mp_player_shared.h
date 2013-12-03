//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef HL2MP_PLAYER_SHARED_H
#define HL2MP_PLAYER_SHARED_H
#pragma once

#define HL2MP_PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)
#include "studio.h"


enum
{
	PLAYER_SOUNDS_CITIZEN = 0,
	PLAYER_SOUNDS_COMBINESOLDIER,
	PLAYER_SOUNDS_METROPOLICE,
	PLAYER_SOUNDS_MAX,
};

enum HL2MPPlayerState
{
	// Happily running around in the game.
	STATE_ACTIVE=0,
	STATE_OBSERVER_MODE,		// Noclipping around, watching players, etc.
	NUM_PLAYER_STATES
};


#if defined( CLIENT_DLL )
#define CHL2MP_Player C_HL2MP_Player
#endif

class CPlayerAnimState
{
public:
	enum
	{
		TURN_NONE = 0,
		TURN_LEFT,
		TURN_RIGHT
	};

	CPlayerAnimState( CHL2MP_Player *outer );

	Activity			BodyYawTranslateActivity( Activity activity );

	void				Update();

	const QAngle&		GetRenderAngles();
				
	void				GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );

	CHL2MP_Player		*GetOuter();

private:
	void				GetOuterAbsVelocity( Vector& vel );

	int					ConvergeAngles( float goal,float maxrate, float dt, float& current );

	void				EstimateYaw( void );
	void				ComputePoseParam_BodyYaw( void );
	void				ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr );
	void				ComputePoseParam_BodyLookYaw( void );

	void				ComputePlaybackRate();

	CHL2MP_Player		*m_pOuter;

	float				m_flGaitYaw;
	float				m_flStoredCycle;

	// The following variables are used for tweaking the yaw of the upper body when standing still and
	//  making sure that it smoothly blends in and out once the player starts moving
	// Direction feet were facing when we stopped moving
	float				m_flGoalFeetYaw;
	float				m_flCurrentFeetYaw;

	float				m_flCurrentTorsoYaw;

	// To check if they are rotating in place
	float				m_flLastYaw;
	// Time when we stopped moving
	float				m_flLastTurnTime;

	// One of the above enums
	int					m_nTurningInPlace;

	QAngle				m_angRender;

	float				m_flTurnCorrectionTime;
};

#endif //HL2MP_PLAYER_SHARED_h
