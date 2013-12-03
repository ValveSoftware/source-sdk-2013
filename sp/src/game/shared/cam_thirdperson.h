//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef CAM_THIRDPERSON_H
#define CAM_THIRDPERSON_H

#if defined( _WIN32 )
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
#else
	#include "baseplayer.h"
#endif

#define DIST_FORWARD 0
#define DIST_RIGHT 1
#define DIST_UP 2

//-------------------------------------------------- Constants

#define CAM_MIN_DIST 30.0
#define CAM_ANGLE_MOVE .5
#define MAX_ANGLE_DIFF 10.0
#define PITCH_MAX 90.0
#define PITCH_MIN 0
#define YAW_MAX  135.0
#define YAW_MIN	 -135.0
#define	DIST	 2
#define CAM_HULL_OFFSET		14.0    // the size of the bounding hull used for collision checking

#define CAMERA_UP_OFFSET	25.0f
#define CAMERA_OFFSET_LERP_TIME 0.5f
#define CAMERA_UP_OFFSET_LERP_TIME 0.25f

class CThirdPersonManager
{
public:

	CThirdPersonManager();
	void	SetCameraOffsetAngles( Vector vecOffset ) { m_vecCameraOffset = vecOffset; }
	Vector	GetCameraOffsetAngles( void ) { return m_vecCameraOffset; }
	
	void	SetDesiredCameraOffset( Vector vecOffset ) { m_vecDesiredCameraOffset = vecOffset; }
	Vector	GetDesiredCameraOffset( void );

	Vector	GetFinalCameraOffset( void );

	void	SetCameraOrigin( Vector vecOffset ) { m_vecCameraOrigin = vecOffset; }
	Vector	GetCameraOrigin( void ) { return m_vecCameraOrigin; }

	void	Update( void );

	void	PositionCamera( CBasePlayer *pPlayer, QAngle angles );

	void	UseCameraOffsets( bool bUse ) { m_bUseCameraOffsets = bUse; }
	bool	UsingCameraOffsets( void ) { return m_bUseCameraOffsets; }

	QAngle	GetCameraViewAngles( void ) { return m_ViewAngles; }

	Vector	GetDistanceFraction( void );

	bool	WantToUseGameThirdPerson( void );

	void	SetOverridingThirdPerson( bool bOverride ) { m_bOverrideThirdPerson = bOverride; }
	bool	IsOverridingThirdPerson( void ) { return m_bOverrideThirdPerson; }

	void	Init( void );

	void	SetForcedThirdPerson( bool bForced ) { m_bForced = bForced; }
	bool	GetForcedThirdPerson() const { return m_bForced; }

private:

	// What is the current camera offset from the view origin?
	Vector		m_vecCameraOffset;
	// Distances from the center
	Vector		m_vecDesiredCameraOffset;

	Vector m_vecCameraOrigin;

	bool	m_bUseCameraOffsets;

	QAngle	m_ViewAngles;

	float	m_flFraction;
	float	m_flUpFraction;

	float	m_flTargetFraction;
	float	m_flTargetUpFraction;

	bool	m_bOverrideThirdPerson;

	bool	m_bForced;

	float	m_flUpOffset;

	float	m_flLerpTime;
	float	m_flUpLerpTime;
};

extern CThirdPersonManager g_ThirdPersonManager;

#endif // CAM_THIRDPERSON_H
