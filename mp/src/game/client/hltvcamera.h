//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HLTVCAMERA_H
#define HLTVCAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"

class C_HLTVCamera : CGameEventListener
{
public:
	C_HLTVCamera();
	virtual ~C_HLTVCamera();

	void Init();
	void Reset();

	void CalcView(Vector& origin, QAngle& angles, float& fov);
	void FireGameEvent( IGameEvent *event );

	void SetMode(int iMode);
	void SetChaseCamParams( float flOffset, float flDistance, float flTheta, float flPhi  );
	void SpecNextPlayer( bool bInverse );
	void SpecNamedPlayer( const char *szPlayerName );
	void ToggleChaseAsFirstPerson();
	bool IsPVSLocked();
	void SetAutoDirector( bool bActive );
	
	int  GetMode();	// returns current camera mode
	C_BaseEntity *GetPrimaryTarget();  // return primary target
	void SetPrimaryTarget( int nEntity); // set the primary obs target
	C_BaseEntity *GetCameraMan();  // return camera entity if any

	void CreateMove(CUserCmd *cmd);
	void FixupMovmentParents();
	void PostEntityPacketReceived();
	const char* GetTitleText() { return m_szTitleText; }
	int  GetNumSpectators() { return m_nNumSpectators; }
			
protected:

	void CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void CalcFixedView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);

	void SmoothCameraAngle( QAngle& targetAngle );
	void SetCameraAngle( QAngle& targetAngle );
	void Accelerate( Vector& wishdir, float wishspeed, float accel );

	int			m_nCameraMode; // current camera mode
	int			m_iCameraMan; // camera man entindex or 0
	Vector		m_vCamOrigin;  //current camera origin
	QAngle		m_aCamAngle;   //current camera angle
	int			m_iTraget1;	// first tracked target or 0
	int			m_iTraget2; // second tracked target or 0
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
};


extern C_HLTVCamera *HLTVCamera();	// get Singleton



#endif // HLTVCAMERA_H
