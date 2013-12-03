//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "baseparticleentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------- //
// Definitions
// ------------------------------------------------------------------------- //
#define NUM_MOVIEEXPLOSION_EMITTERS	50
#define EXPLOSION_EMITTER_LIFETIME	3
#define EMITTED_PARTICLE_LIFETIME	1


// ------------------------------------------------------------------------- //
// Classes
// ------------------------------------------------------------------------- //
class MovieExplosionEmitter
{
public:
	Vector		m_Pos;
	Vector		m_Velocity;
	float		m_Lifetime;
	TimedEvent	m_ParticleSpawn;
};


class C_MovieExplosion : public C_BaseParticleEntity, public IPrototypeAppEffect
{
public:
	DECLARE_CLASS( C_MovieExplosion, C_BaseParticleEntity );
	DECLARE_CLIENTCLASS();

					C_MovieExplosion();
					~C_MovieExplosion();

// C_BaseEntity.
public:
	virtual void	OnDataChanged(DataUpdateType_t updateType);

// IPrototypeAppEffect.
public:
	virtual void	Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs);

// IParticleEffect.
public:
	virtual void	Update(float fTimeDelta);
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );


public:
	MovieExplosionEmitter	m_Emitters[NUM_MOVIEEXPLOSION_EMITTERS];
	float					m_EmitterLifetime;

	CParticleMgr			*m_pParticleMgr;
	PMaterialHandle			m_iFireballMaterial;

	// Setup for temporary usage in SimulateAndRender.
	float					m_EmitterAlpha;

private:
					C_MovieExplosion( const C_MovieExplosion & );

};

// Expose to the particle app.
EXPOSE_PROTOTYPE_EFFECT(MovieExplosion, C_MovieExplosion);

IMPLEMENT_CLIENTCLASS_DT(C_MovieExplosion, DT_MovieExplosion, MovieExplosion)
END_RECV_TABLE()



// ------------------------------------------------------------------------- //
// C_MovieExplosion
// ------------------------------------------------------------------------- //
C_MovieExplosion::C_MovieExplosion()
{
	m_pParticleMgr = NULL;
}


C_MovieExplosion::~C_MovieExplosion()
{
	if(m_pParticleMgr)
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
}


void C_MovieExplosion::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if(updateType == DATA_UPDATE_CREATED)
	{
		Start( ParticleMgr(), NULL );
	}
}


void C_MovieExplosion::Start(CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs)
{
	if(!pParticleMgr->AddEffect(&m_ParticleEffect, this))
		return;

	// Setup our emitters.
	for(int iEmitter=0; iEmitter < NUM_MOVIEEXPLOSION_EMITTERS; iEmitter++)
	{
		MovieExplosionEmitter *pEmitter = &m_Emitters[iEmitter];
		pEmitter->m_Velocity = RandomVector(-1, 1) * 200;

		pEmitter->m_Pos = GetAbsOrigin();
			
		pEmitter->m_Lifetime = 0;
		pEmitter->m_ParticleSpawn.Init(15);
	}
	m_EmitterLifetime = 0;

	// Get our materials.
	m_iFireballMaterial = m_ParticleEffect.FindOrAddMaterial("particle/particle_sphere");

	m_pParticleMgr = pParticleMgr;
}


void C_MovieExplosion::Update(float fTimeDelta)
{
	if(!m_pParticleMgr)
		return;

	m_EmitterLifetime += fTimeDelta;
	if(m_EmitterLifetime > EXPLOSION_EMITTER_LIFETIME)
		return;

	m_EmitterAlpha = (float)sin(m_EmitterLifetime * 3.14159f / EXPLOSION_EMITTER_LIFETIME);

	// Simulate the emitters and have them spit out particles.
	for(int iEmitter=0; iEmitter < NUM_MOVIEEXPLOSION_EMITTERS; iEmitter++)
	{
		MovieExplosionEmitter *pEmitter = &m_Emitters[iEmitter];

		pEmitter->m_Pos = pEmitter->m_Pos + pEmitter->m_Velocity * fTimeDelta;
		pEmitter->m_Velocity = pEmitter->m_Velocity * 0.9;

		float tempDelta = fTimeDelta;
		while(pEmitter->m_ParticleSpawn.NextEvent(tempDelta))
		{
			StandardParticle_t *pParticle = 
				(StandardParticle_t*)m_ParticleEffect.AddParticle( sizeof(StandardParticle_t), m_iFireballMaterial);

			if(pParticle)
			{
				pParticle->m_Pos = pEmitter->m_Pos;
				pParticle->m_Velocity = pEmitter->m_Velocity * 0.2f + RandomVector(-20, 20);
			}
		}
	}
}


void C_MovieExplosion::RenderParticles( CParticleRenderIterator *pIterator )
{
	const StandardParticle_t *pParticle = (const StandardParticle_t*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Draw.
		Vector tPos;
		TransformParticle(m_pParticleMgr->GetModelView(), pParticle->m_Pos, tPos);
		float sortKey = tPos.z;

		float lifetimePercent = pParticle->m_Lifetime / EMITTED_PARTICLE_LIFETIME;
		Vector color;
		color.x = sin(lifetimePercent * 3.14159);
		color.y = color.x * 0.5f;
		color.z = 0;
		RenderParticle_ColorSize(
			pIterator->GetParticleDraw(),
			tPos,
			color,
			m_EmitterAlpha * sin(3.14159 * lifetimePercent),
			10);

		pParticle = (const StandardParticle_t*)pIterator->GetNext( sortKey );
	}
}

void C_MovieExplosion::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	StandardParticle_t *pParticle = (StandardParticle_t*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Update its lifetime.
		pParticle->m_Lifetime += pIterator->GetTimeDelta();
		if(pParticle->m_Lifetime > 1)
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{		
			// Move it (this comes after rendering to make it clear that moving the particle here won't change
			// its rendering for this frame since m_TransformedPos has already been set).
			pParticle->m_Pos = pParticle->m_Pos + pParticle->m_Velocity * pIterator->GetTimeDelta();
		}
		
		pParticle = (StandardParticle_t*)pIterator->GetNext();
	}
}

