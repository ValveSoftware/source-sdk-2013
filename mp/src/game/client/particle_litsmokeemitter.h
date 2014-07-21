//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PARTICLE_LITSMOKEEMITTER_H
#define PARTICLE_LITSMOKEEMITTER_H
#ifdef _WIN32
#pragma once
#endif


#include "particles_simple.h"

#include "tier0/memdbgon.h"

//==================================================
// CLitSmokeEmitter
//==================================================

class CLitSmokeEmitter : public CSimpleEmitter
{
public:
	CLitSmokeEmitter( const char *pDebugName );

	virtual void	Update( float flTimeDelta );
	virtual void	StartRender( VMatrix &effectMatrix );
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

	virtual	void	Init( const char *materialName, Vector sortOrigin );
	
	// Get the material we were initialized with.
	PMaterialHandle	GetSmokeMaterial() const;
	
	// Color values are 0-1.
	virtual	void	SetDirectionalLight( Vector position, Vector color, float intensity );
	virtual	void	SetLight( Vector position, Vector color, float intensity );

	static CSmartPtr<CLitSmokeEmitter> Create( const char *pDebugName )
	{
		return new CLitSmokeEmitter( pDebugName );
	}

	CParticleSphereRenderer	m_Renderer;

	class LitSmokeParticle : public Particle
	{
	public:
		Vector		m_vecVelocity;
		byte		m_uchColor[4];
		float		m_flLifetime;
		float		m_flDieTime;
		byte		m_uchStartSize;
		byte		m_uchEndSize;
	};

private:

	CLitSmokeEmitter( const CLitSmokeEmitter & ); // not defined, not accessible
	
private:
	
	bool m_bInitted;
	PMaterialHandle m_hSmokeMaterial;
};


inline PMaterialHandle	CLitSmokeEmitter::GetSmokeMaterial() const
{
	return m_hSmokeMaterial;
}

#include "tier0/memdbgoff.h"

#endif // PARTICLE_LITSMOKEEMITTER_H
