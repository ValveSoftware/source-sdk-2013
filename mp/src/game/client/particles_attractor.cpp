//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "particles_attractor.h"

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &center - 
//			"attractor" - 
// Output : CParticleAttractor
//-----------------------------------------------------------------------------
CParticleAttractor *CParticleAttractor::Create( const Vector &center, const char *pDebugName )
{
	CParticleAttractor *pSystem = new CParticleAttractor( pDebugName );

	pSystem->SetAttractorOrigin( center );

	return pSystem;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
void CParticleAttractor::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	float	speed = VectorNormalize( pParticle->m_vecVelocity );
	Vector	offset;

	Vector dir = ( m_vecAttractorOrigin - pParticle->m_Pos );
	VectorNormalize( dir );
	
	speed = clamp( (speed+speed*0.2f), 0.f, 1024.f );
	pParticle->m_vecVelocity += dir * speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float CParticleAttractor::UpdateScale( const SimpleParticle *pParticle )
{
	return ( ((float)pParticle->m_uchStartSize) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float CParticleAttractor::UpdateAlpha( const SimpleParticle *pParticle )
{
	return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//-----------------------------------------------------------------------------
void CParticleAttractor::SetAttractorOrigin( const Vector &origin )
{
	m_vecAttractorOrigin = origin;
}
