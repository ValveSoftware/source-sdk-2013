//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef WEAPON_IFMBASECAMERA_H
#define WEAPON_IFMBASECAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_ifmbase.h"

#ifdef CLIENT_DLL
#include "materialsystem/MaterialSystemUtil.h"
#endif

#if defined( CLIENT_DLL )
	#define CWeaponIFMBaseCamera C_WeaponIFMBaseCamera
#endif

class CWeaponIFMBaseCamera : public CWeaponIFMBase
{
public:
	DECLARE_CLASS( CWeaponIFMBaseCamera, CWeaponIFMBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	// Shared code
public:
	CWeaponIFMBaseCamera();

#ifdef CLIENT_DLL
	// Client code
public:
	virtual void	ViewModelDrawn( CBaseViewModel *pBaseViewModel );
	virtual void	DrawCrosshair( );
	virtual int		DrawModel( int flags );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

protected:
	// Gets the abs orientation of the camera
	virtual void ComputeAbsCameraTransform( Vector &vecAbsOrigin, QAngle &angAbsRotation );

	// Gets the bounds of the overlay to draw
	void GetOverlayBounds( int &x, int &y, int &w, int &h );

	// Gets the size of the overlay to draw
	void GetViewportSize( int &w, int &h );

	void TransmitRenderInfo();

	float m_flFOV;
	float m_flArmLength;
	Vector m_vecRelativePosition;
	QAngle m_angRelativeAngles;
	int m_nScreenWidth;
	int m_nScreenHeight;
	bool m_bFullScreen;
	CMaterialReference m_FrustumMaterial;
	CMaterialReference m_FrustumWireframeMaterial;
#endif

#ifdef GAME_DLL
	// Server code
public:
	void SetRenderInfo( float flAspectRatio, float flFOV, float flArmLength, const Vector &vecPosition, const QAngle &angles );
#endif

private:
	CNetworkVar( float, m_flRenderAspectRatio );
	CNetworkVar( float, m_flRenderFOV );
	CNetworkVar( float, m_flRenderArmLength );
	CNetworkVector( m_vecRenderPosition );
	CNetworkQAngle( m_angRenderAngles );

	CWeaponIFMBaseCamera( const CWeaponIFMBaseCamera & );
};


#endif // WEAPON_IFMBASECAMERA_H
