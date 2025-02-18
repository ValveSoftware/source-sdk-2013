//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#if !defined( FXFLECKS_H )
#define FXFLECKS_H
#ifdef _WIN32
#pragma once
#endif

#include "particles_simple.h"
#include "particlemgr.h"
#include "particle_collision.h"

// FleckParticle

class FleckParticle : public Particle
{
public:
	Vector		m_vecVelocity;
	float		m_flRoll;
	float		m_flRollDelta;
	float		m_flDieTime;	// How long it lives for.
	float		m_flLifetime;	// How long it has been alive for so far.
	byte		m_uchColor[3];
	byte		m_uchSize;
};

//
// CFleckParticles
//

class CFleckParticles : public CSimpleEmitter
{
public:

							CFleckParticles( const char *pDebugName );
							~CFleckParticles();
	static CSmartPtr<CFleckParticles> Create( const char *pDebugName, const Vector &vCenter, const Vector &extents );

	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

	//Setup for point emission
	virtual void		Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags = 0 );

	CParticleCollision m_ParticleCollision;

	CFleckParticles *m_pNextParticleSystem;
private:
	CFleckParticles( const CFleckParticles & ); // not defined, not accessible
};

#endif	//FXFLECKS_H
