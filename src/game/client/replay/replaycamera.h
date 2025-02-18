//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#if defined( REPLAY_ENABLED )

#ifndef REPLAYCAMERA_H
#define REPLAYCAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "replay/ireplaycamera.h"
#include "GameEventListener.h"

class C_ReplayCamera : public CGameEventListener,
					   public IReplayCamera
{
public:
	C_ReplayCamera();
	virtual ~C_ReplayCamera();

	void Init();
	void Reset();

	//
	// IReplayCamera:
	//
	virtual void ClearOverrideView();

	void EnableInput( bool bEnable );

	void OverrideView( const Vector *pOrigin, const QAngle *pAngles, float flFov );
	void GetCachedView( Vector &origin, QAngle &angles, float &fov );

	void CalcView(Vector& origin, QAngle& angles, float& fov);
	void FireGameEvent( IGameEvent *event );

	void SetMode(int iMode);
	void SetChaseCamParams( float flOffset, float flDistance, float flTheta, float flPhi  );
	void SpecNextPlayer( bool bInverse );
	// See UTIL_PlayerByCommandArg for what all might go in here.
	void SpecPlayerByPredicate( const char *szPlayerSearch );
	bool IsPVSLocked();
	void SetAutoDirector( bool bActive );

	int  GetMode();	// returns current camera mode
	C_BaseEntity *GetPrimaryTarget();  // return primary target
	inline int GetPrimaryTargetIndex()	{ return m_iTarget1; }	
	void SetPrimaryTarget( int nEntity); // set the primary obs target

	void CreateMove(CUserCmd *cmd);
	void FixupMovmentParents();
	void PostEntityPacketReceived();
	const char* GetTitleText() { return m_szTitleText; }
	int  GetNumSpectators() { return m_nNumSpectators; }

	void SmoothFov( float flDelta );

	float		m_flRoamingAccel;
	float		m_flRoamingSpeed;
	float		m_flRoamingFov[2]; // FOV for roaming only - current and target - smoothing done by replay editor
	float		m_flRoamingRotFilterFactor;
	float		m_flRoamingShakeAmount;
	float		m_flRoamingShakeSpeed;
	float		m_flRoamingShakeDir;

protected:
	void InitRoamingKeys();
	bool ShouldUseDefaultRoamingSettings() const;

	void CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta );
	void CalcFixedView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta );
	void CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta );
	void CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta);

	void SmoothCameraAngle( QAngle& targetAngle );
	void SetCameraAngle( QAngle& targetAngle );
	void Accelerate( Vector& wishdir, float wishspeed, float accel, float flDelta );

	bool ShouldOverrideView( Vector& origin, QAngle& angles, float& fov );	// Fills with override data if m_bOverrideView is set

	struct View_t
	{
		Vector origin;
		QAngle angles;
		float fov;
	};

	bool		m_bInputEnabled;
	bool		m_bOverrideView;
	View_t		m_OverrideViewData;
	View_t		m_CachedView;
	float		m_flOldTime;	// Time of last CalcView() (uses gpGlobals->realtime)
	int			m_nCameraMode; // current camera mode
	Vector		m_vCamOrigin;  //current camera origin
	QAngle		m_aCamAngle;   //current camera angle
	QAngle		m_aSmoothedRoamingAngles;
	int			m_iTarget1;	// first tracked target or 0
	int			m_iTarget2; // second tracked target or 0
	float		m_flFOV; // current FOV
	float		m_flOffset;  // z-offset from target origin
	float		m_flDistance; // distance to traget origin+offset
	float		m_flLastDistance; // too smooth distance
	float		m_flTheta; // view angle horizontal 
	float		m_flPhi; // view angle vertical
	float		m_flInertia; // camera inertia 0..100
	float		m_flLastAngleUpdateTime;
	bool		m_bEntityPacketReceived;	// true after a new packet was received
	int			m_nNumSpectators;
	char		m_szTitleText[64];
	CUserCmd	m_LastCmd;
	Vector		m_vecVelocity;

	enum Dir_t
	{
		DIR_FWD,
		DIR_BACK,
		DIR_LEFT,
		DIR_RIGHT,

		DIR_UP,
		DIR_DOWN,

		NUM_DIRS
	};
	ButtonCode_t m_aMovementButtons[NUM_DIRS];

	float	m_flNoiseSample;
};

//-----------------------------------------------------------------------------

C_ReplayCamera *ReplayCamera();

//-----------------------------------------------------------------------------

#define FREE_CAM_ACCEL_MIN			1.1f
#define FREE_CAM_ACCEL_MAX			10.0f
#define FREE_CAM_SPEED_MIN			0.1f
#define FREE_CAM_SPEED_MAX			20.0f
#define FREE_CAM_FOV_MIN			10.0f
#define FREE_CAM_FOV_MAX			130.0f
#define FREE_CAM_ROT_FILTER_MIN		30.0f
#define FREE_CAM_ROT_FILTER_MAX		5.0f
#define FREE_CAM_SHAKE_SPEED_MIN	0.1f
#define FREE_CAM_SHAKE_SPEED_MAX	15.0f
#define FREE_CAM_SHAKE_AMOUNT_MIN	0.0f
#define FREE_CAM_SHAKE_AMOUNT_MAX	35.0f
#define FREE_CAM_SHAKE_DIR_MIN		-1.0f
#define FREE_CAM_SHAKE_DIR_MAX		1.0f

//-----------------------------------------------------------------------------

#endif // REPLAYCAMERA_H

#endif
