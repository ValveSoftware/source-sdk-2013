//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef WEAPON_IFMSTEADYCAM_H
#define WEAPON_IFMSTEADYCAM_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_ifmbasecamera.h"

#if defined( CLIENT_DLL )
	#define CWeaponIFMSteadyCam C_WeaponIFMSteadyCam
#endif

class CWeaponIFMSteadyCam : public CWeaponIFMBaseCamera
{
public:
	DECLARE_CLASS( CWeaponIFMSteadyCam, CWeaponIFMBaseCamera );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	// Shared code
	CWeaponIFMSteadyCam();
	virtual ~CWeaponIFMSteadyCam();

 	virtual void ItemPostFrame();

private:

#ifdef CLIENT_DLL

public:
	// Client code
	virtual void CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles );
	virtual void DrawCrosshair( void );
	virtual void GetToolRecordingState( KeyValues *msg );

private:
	// Purpose: Draw the weapon's crosshair
	void DrawArmLength( int x, int y, int w, int h, Color clr );
	void DrawFOV( int x, int y, int w, int h, Color clrEdges, Color clrTriangle );

	// Transmits the lock target
	void TransmitLockTarget();

	// Updates the relative orientation of the camera
	void UpdateRelativeOrientation();
	void UpdateLockedRelativeOrientation();
	void UpdateDirectRelativeOrientation();
	
	// Computes a matrix given a forward direction
	void MatrixFromForwardDirection( const Vector &vecForward, matrix3x4_t &mat );

	// Targets the camera to always look at a point
	void LockCamera();

	// Toggles to springy camera
	void ToggleSpringCamera();
	void ToggleDirectMode();

	// Compute the location of the camera for rendering
	virtual void ComputeAbsCameraTransform( Vector &origin, QAngle &angles );

	// Updates the relative orientation of the camera, spring mode
	void ComputeMouseRay( const VMatrix &steadyCamToPlayer, Vector &vecForward );

	// Updates the 2d spring
	void ComputeViewOffset();

	bool m_bIsLocked;
	bool m_bInDirectMode;
	bool m_bInSpringMode;
	Vector m_vecOffset;

	Vector m_vec2DVelocity;
	Vector m_vecActualViewOffset;
	Vector m_vecViewOffset;
	float m_flFOVOffsetY;

	vgui::HFont	 m_hFont;
	int m_nTextureId;
#endif // CLIENT_DLL

#ifdef GAME_DLL
public:
	// Server code
#endif // GAME_DLL

private:
	EHANDLE m_hLockTarget;
	
private:
	CWeaponIFMSteadyCam( const CWeaponIFMSteadyCam & );
};


#endif // WEAPON_IFMSTEADYCAM_H
