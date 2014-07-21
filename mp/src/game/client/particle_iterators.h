//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side CTFTeam class
//
// $NoKeywords: $
//=============================================================================//

#ifndef PARTICLE_ITERATORS_H
#define PARTICLE_ITERATORS_H
#ifdef _WIN32
#pragma once
#endif


#include "materialsystem/imesh.h"
#include "particledraw.h"


#define NUM_PARTICLES_PER_BATCH 200
#ifndef _XBOX
#define MAX_TOTAL_PARTICLES		2048	// Max particles in the world
#else
#define MAX_TOTAL_PARTICLES		1024
#endif


//
// Iterate the particles like this:
//
// Particle *pCur = pIterator->GetFirst();
// while ( pCur )
// {
//     ... render the particle here and figure out the sort key and position
//     pCur = pIterator->GetNext( sortKey, pCur->m_Pos );
// }
//
class CParticleRenderIterator
{
friend class CParticleMgr;
friend class CParticleEffectBinding;
public:
	CParticleRenderIterator();

	// The sort key is used to sort the particles incrementally as they're rendered.
	// They only get sorted in the main rendered view (ie: not in reflections or monitors).
	// These return const because you should only modify particles during their simulation.
	const Particle* GetFirst();
	const Particle* GetNext( float sortKey );

	// Use this to render. This can return NULL, in which case you shouldn't render.
	// This being NULL is a carryover from when particles rendered and simulated together and
	// it should GO AWAY SOON!
	ParticleDraw* GetParticleDraw() const;


private:

	void TestFlushBatch();


private:
	// Set by CParticleMgr.
	CParticleEffectBinding *m_pEffectBinding;
	CEffectMaterial *m_pMaterial;
	ParticleDraw *m_pParticleDraw;
	CMeshBuilder *m_pMeshBuilder;
	IMesh *m_pMesh;
	bool m_bBucketSort;
	
	// Output after rendering.
	float m_MinZ;
	float m_MaxZ;
	float m_zCoords[MAX_TOTAL_PARTICLES];
	int m_nZCoords;
	
	Particle *m_pCur;
	bool m_bGotFirst;
	float m_flPrevZ;
	int m_nParticlesInCurrentBatch;
};


//
// Iterate the particles like this:
//
// Particle *pCur = pIterator->GetFirst();
// while ( pCur )
// {
//     ... simulate here.. call pIterator->RemoveParticle if you want the particle to go away
//     pCur = pIterator->GetNext();
// }
//
class CParticleSimulateIterator
{
friend class CParticleMgr;
friend class CParticleEffectBinding;
public:
	CParticleSimulateIterator();
	
	// Iterate through the particles, simulate them, and remove them if necessary.
	Particle* GetFirst();
	Particle* GetNext();
	float GetTimeDelta() const;

	void RemoveParticle( Particle *pParticle );
	void RemoveAllParticles();

private:
	CParticleEffectBinding *m_pEffectBinding;
	CEffectMaterial *m_pMaterial;
	float m_flTimeDelta;

	bool m_bGotFirst;
	Particle *m_pNextParticle;
};


// -------------------------------------------------------------------------------------------------------- //
// CParticleRenderIterator inlines
// -------------------------------------------------------------------------------------------------------- //

inline CParticleRenderIterator::CParticleRenderIterator()
{
	m_pCur = NULL;
	m_bGotFirst = false;
	m_flPrevZ = 0;
	m_nParticlesInCurrentBatch = 0;
	m_MinZ = 1e24;
	m_MaxZ = -1e24;
	m_nZCoords = 0;
}

inline const Particle* CParticleRenderIterator::GetFirst()
{
	Assert( !m_bGotFirst );
	m_bGotFirst = true;

	m_pCur = m_pMaterial->m_Particles.m_pNext;
	if ( m_pCur == &m_pMaterial->m_Particles )
		return NULL;

	m_pParticleDraw->m_pSubTexture = m_pCur->m_pSubTexture;
	return m_pCur;
}

inline void CParticleRenderIterator::TestFlushBatch()
{
	++m_nParticlesInCurrentBatch;
	if( m_nParticlesInCurrentBatch >= NUM_PARTICLES_PER_BATCH )
	{
		m_pMeshBuilder->End( false, true );
		m_pMeshBuilder->Begin( m_pMesh, MATERIAL_QUADS, NUM_PARTICLES_PER_BATCH * 4 );

		m_nParticlesInCurrentBatch = 0;
	}
}

inline const Particle* CParticleRenderIterator::GetNext( float sortKey )
{
	Assert( m_bGotFirst );
	Assert( m_pCur );

	TestFlushBatch();

	Particle *pNext = m_pCur->m_pNext;

	// Update the incremental sort.
	if( m_bBucketSort )
	{
		m_MinZ = MIN( sortKey, m_MinZ );
		m_MaxZ = MAX( sortKey, m_MaxZ );
		
		m_zCoords[m_nZCoords] = sortKey;
		++m_nZCoords;
	}
	else
	{
		// Swap with the previous particle (incremental sort)?
		if( m_pCur != m_pMaterial->m_Particles.m_pNext && m_flPrevZ > sortKey )
		{
			SwapParticles( m_pCur->m_pPrev, m_pCur );
		}
		else
		{
			m_flPrevZ = sortKey;
		}
	}

	m_pCur = pNext;
	if ( m_pCur == &m_pMaterial->m_Particles )
		return NULL;

	m_pParticleDraw->m_pSubTexture = m_pCur->m_pSubTexture;
	return m_pCur;
}

inline ParticleDraw* CParticleRenderIterator::GetParticleDraw() const
{
	return m_pParticleDraw;
}


// -------------------------------------------------------------------------------------------------------- //
// CParticleSimulateIterator inlines
// -------------------------------------------------------------------------------------------------------- //

inline CParticleSimulateIterator::CParticleSimulateIterator()
{
	m_pNextParticle = NULL;
#ifdef _DEBUG
	m_bGotFirst = false;
#endif
}

inline Particle* CParticleSimulateIterator::GetFirst()
{
#ifdef _DEBUG
	// Make sure they're either starting out fresh or that the previous guy iterated through all the particles.
	if ( m_bGotFirst )
	{
		Assert( m_pNextParticle == &m_pMaterial->m_Particles );
	}
#endif

	Particle *pRet = m_pMaterial->m_Particles.m_pNext;
	if ( pRet == &m_pMaterial->m_Particles )
		return NULL;

#ifdef _DEBUG
	m_bGotFirst = true;
#endif

	m_pNextParticle = pRet->m_pNext;
	return pRet;
}

inline Particle* CParticleSimulateIterator::GetNext()
{
	Particle *pRet = m_pNextParticle;

	if ( pRet == &m_pMaterial->m_Particles )
		return NULL;
	
	m_pNextParticle = pRet->m_pNext;
	return pRet;
}

inline void CParticleSimulateIterator::RemoveParticle( Particle *pParticle )
{
	m_pEffectBinding->RemoveParticle( pParticle );
}

inline void CParticleSimulateIterator::RemoveAllParticles()
{
	Particle *pParticle = GetFirst();
	while ( pParticle )
	{
		RemoveParticle( pParticle );
		pParticle = GetNext();
	}
}

inline float CParticleSimulateIterator::GetTimeDelta() const
{
	return m_flTimeDelta;
}


#endif // PARTICLE_ITERATORS_H

