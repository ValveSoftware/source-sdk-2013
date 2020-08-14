//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CAMERA_H
#define CAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointCamera : public CBaseEntity
{
public:
	DECLARE_CLASS( CPointCamera, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	CPointCamera();
	~CPointCamera();

	void Spawn( void );

	// Tell the client that this camera needs to be rendered
	void SetActive( bool bActive );
	int  UpdateTransmitState(void);

	void ChangeFOVThink( void );

	void InputChangeFOV( inputdata_t &inputdata );
	void InputSetOnAndTurnOthersOff( inputdata_t &inputdata );
	void InputSetOn( inputdata_t &inputdata );
	void InputSetOff( inputdata_t &inputdata );
#ifdef MAPBASE
	void InputSetSkyMode( inputdata_t &inputdata ) { m_iSkyMode = inputdata.value.Int(); }
	void InputSetRenderTarget( inputdata_t &inputdata ) { m_iszRenderTarget = inputdata.value.StringID(); }

	float GetFOV() const { return m_FOV; }
#endif

private:
	float m_TargetFOV;
	float m_DegreesPerSecond;

	CNetworkVar( float, m_FOV );
	CNetworkVar( float, m_Resolution );
	CNetworkVar( bool, m_bFogEnable );
	CNetworkColor32( m_FogColor );
	CNetworkVar( float, m_flFogStart );
	CNetworkVar( float, m_flFogEnd );
	CNetworkVar( float, m_flFogMaxDensity );
	CNetworkVar( bool, m_bActive );
	CNetworkVar( bool, m_bUseScreenAspectRatio );
#ifdef MAPBASE
	CNetworkVar( int, m_iSkyMode );
	CNetworkVar( string_t, m_iszRenderTarget );
#endif

	// Allows the mapmaker to control whether a camera is active or not
	bool	m_bIsOn;

public:
	CPointCamera	*m_pNext;
};

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointCameraOrtho : public CPointCamera
{
public:
	DECLARE_CLASS( CPointCameraOrtho, CPointCamera );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	CPointCameraOrtho();
	~CPointCameraOrtho();

	enum
	{
		ORTHO_TOP,
		ORTHO_BOTTOM,
		ORTHO_LEFT,
		ORTHO_RIGHT,

		NUM_ORTHO_DIMENSIONS
	};

	void Spawn( void );

	bool KeyValue( const char *szKeyName, const char *szValue );
	bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );

	void ChangeOrtho( int iType, const char *szChange );
	void ChangeOrthoThink( void );

	void InputSetOrthoEnabled( inputdata_t &inputdata ) { m_bOrtho = inputdata.value.Bool(); }
	void InputScaleOrtho( inputdata_t &inputdata );
	void InputSetOrthoTop( inputdata_t &inputdata ) { ChangeOrtho(ORTHO_TOP, inputdata.value.String()); }
	void InputSetOrthoBottom( inputdata_t &inputdata ) { ChangeOrtho( ORTHO_BOTTOM, inputdata.value.String() ); }
	void InputSetOrthoLeft( inputdata_t &inputdata ) { ChangeOrtho( ORTHO_LEFT, inputdata.value.String() ); }
	void InputSetOrthoRight( inputdata_t &inputdata ) { ChangeOrtho( ORTHO_RIGHT, inputdata.value.String() ); }

private:
	float m_TargetOrtho[NUM_ORTHO_DIMENSIONS];
	float m_TargetOrthoDPS;

	CNetworkVar( bool, m_bOrtho );
	CNetworkArray( float, m_OrthoDimensions, NUM_ORTHO_DIMENSIONS );
};
#endif

CPointCamera *GetPointCameraList();
#endif // CAMERA_H
