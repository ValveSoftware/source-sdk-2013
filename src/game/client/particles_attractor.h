//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PARTICLES_ATTRACTOR_H
#define PARTICLES_ATTRACTOR_H
#ifdef _WIN32
#pragma once
#endif

#include "particles_simple.h"

class CParticleAttractor : public CSimpleEmitter
{
public:

	CParticleAttractor( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	//Create
	static CParticleAttractor *Create( const Vector &center, const char *pDebugName = "attractor" );

	void	UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	float	UpdateScale( const SimpleParticle *pParticle );
	float	UpdateAlpha( const SimpleParticle *pParticle );
	void	SetAttractorOrigin( const Vector &origin );

private:

	CParticleAttractor( const CParticleAttractor & ); // not defined, not accessible

	Vector	m_vecAttractorOrigin;
};

#endif // PARTICLES_ATTRACTOR_H
