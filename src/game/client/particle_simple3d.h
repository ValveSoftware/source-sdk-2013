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

#if !defined( SIMPLE3D_H )
#define SIMPLE3D_H
#ifdef _WIN32
#pragma once
#endif

#include "particles_simple.h"
#include "particle_collision.h"

// Particle3D

class Particle3D : public Particle
{
public:
	Vector		m_vecVelocity;
	QAngle		m_vAngles;
	float		m_flAngSpeed;		// Is same on all axis

	// NOTE: This particle already takes the full 64-bytes.  So fade over a hardcoded 2 seconds instead of the entire lifetime
	float		GetFadeFraction() const { return m_flLifeRemaining >= 2.0f ? 1 : (m_flLifeRemaining * 0.5f); }
	bool		IsDead() const { return m_flLifeRemaining >= 0 ? false : true; }

	float		m_flLifeRemaining;	// How long it lives for.
public:
	
	byte		m_uchFrontColor[3];	
	byte		m_uchSize;			
	byte		m_uchBackColor[3];	
	byte		m_pad;				// Pad to 8 bytes.
};

//
// CSimple3DEmitter
//

class CSimple3DEmitter : public CSimpleEmitter
{
public:
	CSimple3DEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}
	
	static CSmartPtr<CSimple3DEmitter>	Create( const char *pDebugName );

	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

	CParticleCollision	m_ParticleCollision;

private:
	CSimple3DEmitter( const CSimple3DEmitter & );

};

#endif	//SIMPLE3D_H