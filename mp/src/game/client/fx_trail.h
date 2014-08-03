//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FX_TRAIL_H
#define FX_TRAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "particle_util.h"
#include "baseparticleentity.h"
#include "particle_prototype.h"

class C_ParticleTrail : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:

	DECLARE_CLASS( C_ParticleTrail, C_BaseParticleEntity );

				C_ParticleTrail( void );
	virtual		~C_ParticleTrail( void );

	void		GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles );
	
	void		SetEmit( bool bEmit );
	bool		ShouldEmit( void ) { return m_bEmit; }

	void		SetSpawnRate( float rate );


// C_BaseEntity.
public:
	virtual	void	OnDataChanged(DataUpdateType_t updateType);

	virtual void	Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs );

	int				m_nAttachment;
	float			m_flLifetime;			// How long this effect will last

private:

	float			m_SpawnRate;			// How many particles per second.
	bool			m_bEmit;				// Keep emitting particles?

	TimedEvent		m_ParticleSpawn;
	CParticleMgr	*m_pParticleMgr;

private:

	C_ParticleTrail( const C_ParticleTrail & );
};

#endif // FX_TRAIL_H
