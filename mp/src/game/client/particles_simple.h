//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef PARTICLES_SIMPLE_H
#define PARTICLES_SIMPLE_H
#ifdef _WIN32
#pragma once
#endif

#include "particlemgr.h"
#include "particlesphererenderer.h"
#include "smartptr.h"


// ------------------------------------------------------------------------------------------------ //
// CParticleEffect is the base class that you can derive from to make a particle effect.
// These can be used two ways:
//
// 1. Allocate a CParticleEffect-based object using the class's static Create() function. This gives
//    you back a smart pointer that handles the reference counting for you.
//
// 2. Contain a CParticleEffect object in your class.
// ------------------------------------------------------------------------------------------------ //

class CParticleEffect : public IParticleEffect
{
public:
	DECLARE_CLASS_NOBASE( CParticleEffect );

	friend class CRefCountAccessor;

	// Call this before adding a bunch of particles to give it a rough estimate of where
	// your particles are for sorting amongst other translucent entities.
	void				SetSortOrigin( const Vector &vSortOrigin );

	PMaterialHandle		GetPMaterial(const char *name);
	
	Particle*			AddParticle( unsigned int particleSize, PMaterialHandle material, const Vector &origin );

	CParticleEffectBinding&	GetBinding()	{ return m_ParticleEffect; }

	const char *GetEffectName();

	void AddFlags( int iFlags ) { m_Flags |= iFlags; }
	void RemoveFlags( int iFlags ) { m_Flags &= ~iFlags; }

	void SetDontRemove( bool bSet )
	{
		if( bSet )
			AddFlags( FLAG_DONT_REMOVE );
		else
			RemoveFlags( FLAG_DONT_REMOVE );
	}

// IParticleEffect overrides
public:

	virtual void				SetParticleCullRadius( float radius );
	virtual void				NotifyRemove( void );
	virtual const Vector &		GetSortOrigin();
	virtual void				NotifyDestroyParticle( Particle* pParticle );
	virtual void				Update( float flTimeDelta );

	// All Create() functions should call this so the effect deletes itself
	// when it is removed from the particle manager.
	void						SetDynamicallyAllocated( bool bDynamic=true );

	virtual bool				ShouldSimulate() const { return m_bSimulate; }
	virtual void				SetShouldSimulate( bool bSim ) { m_bSimulate = bSim; }

	int							AllocateToolParticleEffectId();
	int							GetToolParticleEffectId() const;
protected:
								CParticleEffect( const char *pDebugName );
	virtual						~CParticleEffect();

	// Returns nonzero if Release() has been called.
	int							IsReleased();
	
	enum
	{
		FLAG_ALLOCATED = (1<<1),	// Most of the CParticleEffects are dynamically allocated but
								// some are member variables of a class. If they're member variables.
		FLAG_DONT_REMOVE = (1<<2),
	};

	// Used to track down bugs.
	char const					*m_pDebugName;

	CParticleEffectBinding		m_ParticleEffect;
	Vector						m_vSortOrigin;
	
	int							m_Flags;		// Combination of CParticleEffect::FLAG_

	bool						m_bSimulate;
	int							m_nToolParticleEffectId;

private:
	// Update the reference count.
	void						AddRef();
	void						Release();
	
	int							m_RefCount;		// When this goes to zero and the effect has no more active
												// particles, (and it's dynamically allocated), it will delete itself.

	CParticleEffect( const CParticleEffect & ); // not defined, not accessible
};

inline int CParticleEffect::GetToolParticleEffectId() const
{
	return m_nToolParticleEffectId;
}

inline int CParticleEffect::AllocateToolParticleEffectId()
{
	m_nToolParticleEffectId = ParticleMgr()->AllocateToolParticleEffectId();
	return m_nToolParticleEffectId;
}


//-----------------------------------------------------------------------------
// Particle flags
//-----------------------------------------------------------------------------
enum SimpleParticleFlag_t
{
	SIMPLE_PARTICLE_FLAG_WINDBLOWN = 0x1,
	SIMPLE_PARTICLE_FLAG_NO_VEL_DECAY = 0x2	// Used by the blood spray emitter. By default, it decays the
											// particle velocity.
};

class SimpleParticle : public Particle
{
public:
	SimpleParticle() : m_iFlags(0) {}

	// AddSimpleParticle automatically initializes these fields.
	Vector		m_vecVelocity;
	float		m_flRoll;
	float		m_flDieTime;	// How long it lives for.
	float		m_flLifetime;	// How long it has been alive for so far.
	unsigned char	m_uchColor[3];
	unsigned char	m_uchStartAlpha;
	unsigned char	m_uchEndAlpha;
	unsigned char	m_uchStartSize;
	unsigned char	m_uchEndSize;
	unsigned char 	m_iFlags;	// See SimpleParticleFlag_t above
	float		m_flRollDelta;
};



// CSimpleEmitter implements a common way to simulate and render particles.
//
// Effects can add particles to the particle manager and point at CSimpleEmitter
// for the effect so they don't have to implement the simulation code. It simulates
// velocity, and fades their alpha from invisible to solid and back to invisible over their lifetime.
//
// Particles you add using this effect must use the class CParticleSimple::Particle.
class CSimpleEmitter : public CParticleEffect
{
// IParticleEffect overrides.
public:

	DECLARE_CLASS( CSimpleEmitter, CParticleEffect );

	static CSmartPtr<CSimpleEmitter>	Create( const char *pDebugName );

	virtual void	SimulateParticles( CParticleSimulateIterator *pIterator );
	virtual void	RenderParticles( CParticleRenderIterator *pIterator );

	void			SetNearClip( float nearClipMin, float nearClipMax );

	void			SetDrawBeforeViewModel( bool state = true );

	SimpleParticle*	AddSimpleParticle( PMaterialHandle hMaterial, const Vector &vOrigin, float flDieTime=3, unsigned char uchSize=10 );
	
// Overridables for variants like CEmberEffect.
protected:
					CSimpleEmitter( const char *pDebugName = NULL );
	virtual			~CSimpleEmitter();

	virtual	float	UpdateAlpha( const SimpleParticle *pParticle );
	virtual float	UpdateScale( const SimpleParticle *pParticle );
	virtual	float	UpdateRoll( SimpleParticle *pParticle, float timeDelta );
	virtual	void	UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual Vector	UpdateColor( const SimpleParticle *pParticle );

	float			m_flNearClipMin;
	float			m_flNearClipMax;

private:
	CSimpleEmitter( const CSimpleEmitter & ); // not defined, not accessible
};

//==================================================
// EmberEffect
//==================================================

class CEmberEffect : public CSimpleEmitter
{
public:
							CEmberEffect( const char *pDebugName );
	static CSmartPtr<CEmberEffect>	Create( const char *pDebugName );

	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual Vector UpdateColor( const SimpleParticle *pParticle );

private:
	CEmberEffect( const CEmberEffect & ); // not defined, not accessible
};


//==================================================
// FireSmokeEffect
//==================================================

class CFireSmokeEffect : public CSimpleEmitter
{
public:
								CFireSmokeEffect( const char *pDebugName );
	static CSmartPtr<CFireSmokeEffect>	Create( const char *pDebugName );

	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual float UpdateAlpha( const SimpleParticle *pParticle );

protected:
	VPlane	m_planeClip;

private:
	CFireSmokeEffect( const CFireSmokeEffect & ); // not defined, not accessible
};


//==================================================
// CFireParticle
//==================================================

class CFireParticle : public CSimpleEmitter
{
public:
							CFireParticle( const char *pDebugName );
	static CSmartPtr<CFireParticle>	Create( const char *pDebugName );
	
	virtual Vector UpdateColor( const SimpleParticle *pParticle );

private:
	CFireParticle( const CFireParticle & ); // not defined, not accessible
};


#endif // PARTICLES_SIMPLE_H
