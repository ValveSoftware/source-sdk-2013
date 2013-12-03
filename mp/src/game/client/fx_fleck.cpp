//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "fx_fleck.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// enable this to have the fleck_merge cvar as well as the current system count displayed as it changes (for profiling)
#define REPORT_MERGED_FLECKS 0

//
// class PARTICLE_MERGE
//{
//public:
//	bool MergeParticleSystems( CFleckParticles *pSystem, const char *pEffectName, const Vector &center, const Vector &extents )
//	{ merge; return true; }
//};

// a singly linked list through all particle effects of a specific type
// with a specific rule for sharing them.
// Needs a hook to the particle effect's constructor/destructor and factory method
// The factory needs to support optionally merging the new particles into a previously built particle effect
// this cuts down on lots of scene management overhead as well as rendering/batch overhead
template< class PARTICLE_EFFECT, class PARTICLE_MERGE >
class CParticleMergeList
{
public:
	CParticleMergeList() : m_pHead(NULL) {}
	void AddParticleSystem( PARTICLE_EFFECT *pSystem );
	void RemoveParticleSystem( PARTICLE_EFFECT *pRemove );
	PARTICLE_EFFECT *FindAndMergeParticleSystem( const char *pEffectName, const Vector &center, const Vector &extents );
	bool MergeParticleSystems( PARTICLE_EFFECT *pSystem, const char *pEffectName, const Vector &center, const Vector &extents );
private:
	PARTICLE_EFFECT *m_pHead;
	PARTICLE_MERGE m_merge;
};

#if REPORT_MERGED_FLECKS
ConVar fleck_merge("fleck_merge","1");
int g_PCount = 0;
#endif

template< class PARTICLE_EFFECT, class PARTICLE_MERGE >
void CParticleMergeList<PARTICLE_EFFECT,PARTICLE_MERGE>::AddParticleSystem( PARTICLE_EFFECT *pSystem )
{
#if REPORT_MERGED_FLECKS
	g_PCount++;
	Msg("PS: %d\n", g_PCount);
#endif
	pSystem->m_pNextParticleSystem = m_pHead;
	m_pHead = pSystem;
}

template< class PARTICLE_EFFECT, class PARTICLE_MERGE >
void CParticleMergeList<PARTICLE_EFFECT,PARTICLE_MERGE>::RemoveParticleSystem( PARTICLE_EFFECT *pRemove )
{
#if REPORT_MERGED_FLECKS
	g_PCount--;
	Msg("PS: %d\n", g_PCount);
#endif
	PARTICLE_EFFECT **pPrev = &m_pHead;
	PARTICLE_EFFECT *pCur = *pPrev;
	while ( pCur )
	{
		if ( pCur == pRemove )
		{
			*pPrev = pCur->m_pNextParticleSystem;
			return;
		}
		pPrev = &pCur->m_pNextParticleSystem;
		pCur = *pPrev;
	}
}

template< class PARTICLE_EFFECT, class PARTICLE_MERGE >
PARTICLE_EFFECT *CParticleMergeList<PARTICLE_EFFECT,PARTICLE_MERGE>::FindAndMergeParticleSystem( const char *pEffectName, const Vector &center, const Vector &extents )
{
#if REPORT_MERGED_FLECKS
	if ( !fleck_merge.GetBool() )
		return NULL;
#endif

	for ( PARTICLE_EFFECT *pMerge = m_pHead; pMerge != NULL; pMerge = pMerge->m_pNextParticleSystem )
	{
		if ( m_merge.MergeParticleSystems( pMerge, pEffectName, center, extents ) )
			return pMerge;
	}
	return NULL;
}

// merge anything within 10 feet
const float MAX_RADIUS_BBOX_MERGE = 120.0f;

template< class PARTICLE_EFFECT >
class CMergeSameNameBbox
{
public:
	bool MergeParticleSystems( PARTICLE_EFFECT *pSystem, const char *pEffectName, const Vector &center, const Vector &extents )
	{
		// by default, match names
		if ( !Q_stricmp(pSystem->GetEffectName(), pEffectName) )
		{
			Vector mins, maxs;
			pSystem->GetBinding().GetWorldspaceBounds( &mins, &maxs );
			AddPointToBounds( center - extents, mins, maxs );
			AddPointToBounds( center + extents, mins, maxs );
			Vector size = maxs - mins;
			float radius = size.Length();
			if ( radius < MAX_RADIUS_BBOX_MERGE )
			{
				pSystem->GetBinding().SetBBox( mins, maxs );
				// put sort origin at center of the new box
				Vector sortOrigin = 0.5f * (mins+maxs);
				pSystem->SetSortOrigin(sortOrigin);
				return true;
			}
		}
		return false;
	}
};

CParticleMergeList< CFleckParticles, CMergeSameNameBbox<CFleckParticles> > g_FleckMergeList;

//
// CFleckParticles
//
CSmartPtr<CFleckParticles> CFleckParticles::Create( const char *pDebugName, const Vector &vCenter, const Vector &extents )
{
	CFleckParticles *pMerge = g_FleckMergeList.FindAndMergeParticleSystem( pDebugName, vCenter, extents );
	if ( pMerge )
		return pMerge;

	CFleckParticles *pRet = new CFleckParticles( pDebugName );
	if ( pRet )
	{
		pRet->GetBinding().SetBBox( vCenter - extents, vCenter + extents );
		pRet->SetSortOrigin(vCenter);
	}
	return pRet;
}


CFleckParticles::CFleckParticles( const char *pDebugName ) : CSimpleEmitter( pDebugName ), m_pNextParticleSystem(NULL)
{
	g_FleckMergeList.AddParticleSystem(this);
}

CFleckParticles::~CFleckParticles()
{
	g_FleckMergeList.RemoveParticleSystem(this);
}

//-----------------------------------------------------------------------------
// Purpose: Test for surrounding collision surfaces for quick collision testing for the particle system
// Input  : &origin - starting position
//			*dir - direction of movement (if NULL, will do a point emission test in four directions)
//			angularSpread - looseness of the spread
//			minSpeed - minimum speed
//			maxSpeed - maximum speed
//			gravity - particle gravity for the sytem
//			dampen - dampening amount on collisions
//			flags - extra information
//-----------------------------------------------------------------------------
void CFleckParticles::Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags )
{
	//See if we've specified a direction
	m_ParticleCollision.Setup( origin, direction, angularSpread, minSpeed, maxSpeed, gravity, dampen );
}


void CFleckParticles::RenderParticles( CParticleRenderIterator *pIterator )
{
	const FleckParticle *pParticle = (const FleckParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		Vector	tPos;
		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, tPos );
		float sortKey = (int) tPos.z;
		
		Vector	color;
		color[0] = pParticle->m_uchColor[0] / 255.0f;
		color[1] = pParticle->m_uchColor[1] / 255.0f;
		color[2] = pParticle->m_uchColor[2] / 255.0f;
		//Render it
		RenderParticle_ColorSizeAngle(
			pIterator->GetParticleDraw(),
			tPos,
			color,
			1.0f - (pParticle->m_flLifetime / pParticle->m_flDieTime),
			pParticle->m_uchSize,
			pParticle->m_flRoll );

		pParticle = (const FleckParticle*)pIterator->GetNext( sortKey );
	}
}


void CFleckParticles::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	FleckParticle *pParticle = (FleckParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		const float	timeDelta = pIterator->GetTimeDelta();

		//Should this particle die?
		pParticle->m_flLifetime += timeDelta;

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;

			//Simulate the movement with collision
			trace_t trace;
			m_ParticleCollision.MoveParticle( pParticle->m_Pos, pParticle->m_vecVelocity, &pParticle->m_flRollDelta, timeDelta, &trace );

			// If we're in solid, then stop moving
			if ( trace.allsolid )
			{
				pParticle->m_vecVelocity = vec3_origin;
				pParticle->m_flRollDelta = 0.0f;
			}
		}

		pParticle = (FleckParticle*)pIterator->GetNext();
	}
}


