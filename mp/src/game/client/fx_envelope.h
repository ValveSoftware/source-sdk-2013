//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FX_ENVELOPE_H
#define FX_ENVELOPE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "fx.h"
#include "view.h"
#include "view_scene.h"
#include "materialsystem/imaterialvar.h"

class C_EnvelopeFX : public CDefaultClientRenderable
{
public:
	typedef CDefaultClientRenderable	BaseClass;

	C_EnvelopeFX();
	virtual ~C_EnvelopeFX();

	virtual void Update( void );

	// IClientRenderable
	virtual const Vector&			GetRenderOrigin( void ) { return m_worldPosition; }
	virtual void					SetRenderOrigin( const Vector &origin ) { m_worldPosition = origin; }
	virtual const QAngle&			GetRenderAngles( void ) { return vec3_angle; }
	virtual const matrix3x4_t &		RenderableToWorldTransform();
	virtual bool					ShouldDraw( void ) { return true; }
	virtual bool					IsTransparent( void ) { return true; }
	virtual bool					ShouldReceiveProjectedTextures( int flags ) { return false; }

	void	SetTime( float t ) { m_t = t; }
	void	LimitTime( float tmax ) { m_tMax = tmax; }
	void	SetActive( bool state = true ) { m_active = state; }
	bool	IsActive( void ) const { return m_active; }

	virtual void EffectInit( int entityIndex, int attachment );
	virtual void EffectShutdown( void );

protected:

	void RemoveRenderable();


	int						m_entityIndex;
	int						m_attachment;
	bool					m_active;
	float					m_t;
	float					m_tMax;
	Vector					m_worldPosition;
};

#endif // FX_ENVELOPE_H
