//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "clientsideeffects.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"

#ifndef FX_QUAD_H
#define FX_QUAD_H
#ifdef _WIN32
#pragma once
#endif

// Flags
#define	FXQUAD_BIAS_SCALE	0x0001	//Bias the scale's interpolation function
#define	FXQUAD_BIAS_ALPHA	0x0002	//Bias the alpha's interpolation function
#define FXQUAD_COLOR_FADE	0x0004	//Blend the color towards black via the alpha (overcomes additive ignoring alpha)

struct FXQuadData_t
{
	FXQuadData_t( void )
	{
		m_flLifeTime	= 0.0f;
		m_flDieTime		= 0.0f;
		m_uiFlags		= 0;
	}

	void SetFlags( unsigned int flags )			{ m_uiFlags |= flags; }
	void SetOrigin( const Vector &origin )		{ m_vecOrigin = origin; }
	void SetNormal( const Vector &normal )		{ m_vecNormal = normal; }
	void SetScale( float start, float end )		{ m_flStartScale = start; m_flEndScale = end; }
	void SetAlpha( float start, float end )		{ m_flStartAlpha = start; m_flEndAlpha = end; }
	void SetLifeTime( float lifetime )			{ m_flDieTime = lifetime; }
	void SetColor( float r, float g, float b )	{ m_Color = Vector( r, g, b ); }
	void SetAlphaBias( float bias )				{ m_flAlphaBias = bias; }
	void SetScaleBias( float bias )				{ m_flScaleBias = bias; }
	void SetYaw( float yaw, float delta = 0.0f ){ m_flYaw = yaw; m_flDeltaYaw = delta; }

	void SetMaterial( const char *shader )	
	{ 
		m_pMaterial = materials->FindMaterial( shader, TEXTURE_GROUP_CLIENT_EFFECTS );

		if ( m_pMaterial != NULL )
		{
			m_pMaterial->IncrementReferenceCount();
		}
	}

	unsigned int	m_uiFlags;
	IMaterial		*m_pMaterial;
	Vector			m_vecOrigin;
	Vector			m_vecNormal;
	float			m_flStartScale;
	float			m_flEndScale;
	float			m_flDieTime;
	float			m_flLifeTime;
	float			m_flStartAlpha;
	float			m_flEndAlpha;
	Vector			m_Color;
	float			m_flYaw;
	float			m_flDeltaYaw;
	
	// Only used with FXQUAD_BIAS_ALPHA and FXQUAD_BIAS_SCALE
	float			m_flScaleBias;
	float			m_flAlphaBias;
};

class CFXQuad : public CClientSideEffect
{
public:

	CFXQuad( const FXQuadData_t &data );
	
	~CFXQuad( void );

	virtual void	Draw( double frametime );
	virtual bool	IsActive( void );
	virtual void	Destroy( void );
	virtual	void	Update( double frametime );

protected:

	FXQuadData_t	m_FXData;
};

#endif // FX_QUAD_H
