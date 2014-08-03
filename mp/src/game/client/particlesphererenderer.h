//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PARTICLESPHERERENDERER_H
#define PARTICLESPHERERENDERER_H
#ifdef _WIN32
#pragma once
#endif


#include "particlemgr.h"
#include "particle_util.h"


class CParticleSphereRenderer
{
public:

				CParticleSphereRenderer();

	// Initialize and tell it the material you'll be using.
	void		Init( CParticleMgr *pParticleMgr, IMaterial *pMaterial );
	
	// Pass this call through from your particle system too.
	void		StartRender( VMatrix &effectMatrix );

	// Call this to	render a spherical particle.
	void		RenderParticle( 
		ParticleDraw *pDraw, 
		const Vector &vOriginalPos,
		const Vector &vTransformedPos,
		float flAlpha,			// value 0 - 255.4
		float flParticleSize,
		float flAngle = 0.0f ); 

	void		RenderParticle_AddColor( 
		ParticleDraw *pDraw, 
		const Vector &vOriginalPos,
		const Vector &vTransformedPos,
		float flAlpha,				// value 0 - 255.4
		float flParticleSize,
		const Vector &vToAdd0to1	// Add this to the color (value 0-1)
		); 

	
	// Lighting is (base color) + (ambient / dist^2) + bump(directional / dist^2)
	const Vector& GetBaseColor() const;			// Specified as 0-1
	void SetBaseColor( const Vector &vColor );
	
	const CParticleLightInfo& GetAmbientLight() const;
	void SetAmbientLight( const CParticleLightInfo &info );
	
	const CParticleLightInfo& GetDirectionalLight() const;
	void SetDirectionalLight( const CParticleLightInfo &info );


private:

	void		AddLightColor( 
		Vector const *pPos,
		Vector const *pLightPos, 
		Vector const *pLightColor, 
		float flLightIntensity,
		Vector *pOutColor );

	inline void	ClampColor( Vector &vColor );


private:

	int m_iLastTickStartRenderCalled;	// Used for debugging.
	
	CParticleMgr *m_pParticleMgr;
	
	Vector			m_vBaseColor;
	CParticleLightInfo m_AmbientLight;
	CParticleLightInfo m_DirectionalLight;
	bool			m_bUsingPixelShaders;
};


// ------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------ //

inline void CParticleSphereRenderer::AddLightColor( 
	Vector const *pPos,
	Vector const *pLightPos, 
	Vector const *pLightColor, 
	float flLightIntensity,
	Vector *pOutColor )
{
	if( flLightIntensity )
	{
		float fDist = pPos->DistToSqr( *pLightPos );
		float fAmt;
		if( fDist > 0.0001f )
			fAmt = flLightIntensity / fDist;
		else
			fAmt = 1000.f;

		*pOutColor += *pLightColor * fAmt;
	}
}


inline void CParticleSphereRenderer::ClampColor( Vector &vColor )
{
	float flMax = MAX( vColor.x, MAX( vColor.y, vColor.z ) );
	if( flMax > 1 )
	{
		vColor *= 255.0f / flMax;
	}
	else
	{
		vColor *= 255.0;
	}
}


inline const Vector& CParticleSphereRenderer::GetBaseColor() const
{
	return m_vBaseColor;
}

inline void CParticleSphereRenderer::SetBaseColor( const Vector &vColor )
{
	m_vBaseColor = vColor;
}

inline const CParticleLightInfo& CParticleSphereRenderer::GetAmbientLight() const
{
	return m_AmbientLight;
}

inline void CParticleSphereRenderer::SetAmbientLight( const CParticleLightInfo &info )
{
	m_AmbientLight = info;
}

inline const CParticleLightInfo& CParticleSphereRenderer::GetDirectionalLight() const
{
	return m_DirectionalLight;
}

inline void CParticleSphereRenderer::SetDirectionalLight( const CParticleLightInfo &info )
{
	m_DirectionalLight = info;
}

inline void CParticleSphereRenderer::RenderParticle( 
	ParticleDraw *pDraw,
	const Vector &vOriginalPos,
	const Vector &vTransformedPos,
	float flAlpha,
	float flParticleSize,
	float flAngle )
{
	// Make sure they called StartRender on us so we were able to set the directional light parameters.
#ifdef _DEBUG
	if ( pDraw->GetMeshBuilder() )
	{
		Assert( m_iLastTickStartRenderCalled == gpGlobals->tickcount );
	}
#endif

	Vector vColor = m_vBaseColor;
	AddLightColor( &vOriginalPos, &m_AmbientLight.m_vPos, &m_AmbientLight.m_vColor, m_AmbientLight.m_flIntensity, &vColor );
	
	// If the DX8 shader isn't going to handle the directional light color, then add its contribution here.
	if( !m_bUsingPixelShaders )
	{
		AddLightColor( &vOriginalPos, &m_DirectionalLight.m_vPos, &m_DirectionalLight.m_vColor, m_DirectionalLight.m_flIntensity, &vColor );
	}
	
	ClampColor( vColor );

	RenderParticle_Color255SizeNormalAngle(
		pDraw,
		vTransformedPos,
		vColor,			// ambient color
		flAlpha,		// alpha
		flParticleSize,
		vec3_origin,
		flAngle );
}

inline void CParticleSphereRenderer::RenderParticle_AddColor( 
	ParticleDraw *pDraw,
	const Vector &vOriginalPos,
	const Vector &vTransformedPos,
	float flAlpha,
	float flParticleSize,
	const Vector &vToAdd0to1
	)
{
	// Make sure they called StartRender on us so we were able to set the directional light parameters.
#ifdef _DEBUG
	if ( pDraw->GetMeshBuilder() )
	{
		Assert( m_iLastTickStartRenderCalled == gpGlobals->tickcount );
	}
#endif

	Vector vColor = m_vBaseColor + vToAdd0to1;
	AddLightColor( &vOriginalPos, &m_AmbientLight.m_vPos, &m_AmbientLight.m_vColor, m_AmbientLight.m_flIntensity, &vColor );
	
	// If the DX8 shader isn't going to handle the directional light color, then add its contribution here.
	if( !m_bUsingPixelShaders )
	{
		AddLightColor( &vOriginalPos, &m_DirectionalLight.m_vPos, &m_DirectionalLight.m_vColor, m_DirectionalLight.m_flIntensity, &vColor );
	}
	
	ClampColor( vColor );

	RenderParticle_Color255Size(
		pDraw,
		vTransformedPos,
		vColor,			// ambient color
		flAlpha,		// alpha
		flParticleSize );
}


#endif // PARTICLESPHERERENDERER_H
