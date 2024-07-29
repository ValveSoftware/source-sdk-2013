//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_ENVPROJECTEDTEXTURE_H
#define C_ENVPROJECTEDTEXTURE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "basetypes.h"

class C_LightOrigin : public C_BaseEntity
{
	DECLARE_CLASS(C_LightOrigin, C_BaseEntity);
public:
	DECLARE_CLIENTCLASS();


private:
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

class C_EnvCascadeLight : public C_BaseEntity
{
	DECLARE_CLASS(C_EnvCascadeLight, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_EnvCascadeLight();
	~C_EnvCascadeLight();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	void	ShutDownLightHandle( void );

	virtual void Simulate();

	void	UpdateLight( bool bForceUpdate );

	bool	ShadowsEnabled();

	float	GetFOV();

private:

	ClientShadowHandle_t m_LightHandle;

	EHANDLE	m_hTargetEntity;
	bool	m_bState;
	float	m_flLightFOV;
	bool	m_bEnableShadows;
	bool	m_bLightOnlyTarget;
	bool	m_bLightWorld;
	bool	m_bCameraSpace;
	color32	m_cLightColor;
	float	m_flAmbient;
	char	m_SpotlightTextureName[ MAX_PATH ];
	int		m_nSpotlightTextureFrame;
	int		m_nShadowQuality;
	bool	m_bCurrentShadow;

public:
	C_EnvCascadeLight*m_pNext;
};

C_EnvCascadeLight* GetEnvProjectedTextureList();

#endif // C_ENVPROJECTEDTEXTURE_H
