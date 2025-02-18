//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "particles_ez.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Singletons for each type of particle system.
// 0 = world, 1 = skybox
static CSmartPtr<CSimpleEmitter> g_pSimpleSingleton[2];
static CSmartPtr<CEmberEffect> g_pEmberSingleton[2];
static CSmartPtr<CFireSmokeEffect> g_pFireSmokeSingleton[2];
static CSmartPtr<CFireParticle> g_pFireSingleton[2];


class CEZParticleInit : public CAutoGameSystem
{
public:
	CEZParticleInit() : CAutoGameSystem( "CEZParticleInit" )
	{
	}

	template< class T >
	CSmartPtr<T> InitSingleton( CSmartPtr<T> pEmitter )
	{
		if ( !pEmitter )
		{
			Error( "InitSingleton: pEmitter is NULL" );
		}
	
		pEmitter->GetBinding().SetDrawThruLeafSystem( false );				// Draw in DrawSingletons instead.
		pEmitter->SetSortOrigin( Vector( 0, 0, 0 ) );

		// Since we draw manually in DrawSingletons, we don't care about
		// the bbox, so don't waste cycles inserting it into the leaf system
		// when it's not going to draw through that anyway.
		// (TODO: SetDrawThruLeafSystem(false) should trigger this automatically
		// in CParticleMgr).
		pEmitter->GetBinding().SetBBox( Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
		return pEmitter;
	}

	virtual void LevelInitPreEntity()
	{
		g_pSimpleSingleton[0] = InitSingleton( CSimpleEmitter::Create( "Simple Particle Singleton" ) );
		g_pSimpleSingleton[1] = InitSingleton( CSimpleEmitter::Create( "Simple Particle Singleton [sky]" ) );
		
		g_pEmberSingleton[0] = InitSingleton( CEmberEffect::Create( "Ember Particle Singleton" ) );
		g_pEmberSingleton[1] = InitSingleton( CEmberEffect::Create( "Ember Particle Singleton [sky]" ) );
		
		g_pFireSmokeSingleton[0] = InitSingleton( CFireSmokeEffect::Create( "Fire Smoke Particle Singleton" ) );
		g_pFireSmokeSingleton[1] = InitSingleton( CFireSmokeEffect::Create( "Fire Smoke Particle Singleton [sky]" ) );
		
		g_pFireSingleton[0] = InitSingleton( CFireParticle::Create( "Fire Particle Singleton" ) );
		g_pFireSingleton[1] = InitSingleton( CFireParticle::Create( "Fire Particle Singleton [sky]" ) );
	}


	virtual void LevelShutdownPreEntity()
	{
		g_pSimpleSingleton[0] = g_pSimpleSingleton[1] = NULL;
		g_pEmberSingleton[0] = g_pEmberSingleton[1] = NULL;
		g_pFireSmokeSingleton[0] = g_pFireSmokeSingleton[1] = NULL;
		g_pFireSingleton[0] = g_pFireSingleton[1] = NULL;
	}
};

static CEZParticleInit g_EZParticleInit;


template<class T>
inline void CopyParticle( const T *pSrc, T *pDest )
{
	if ( pDest )
	{
		// Copy the particle, but don't screw up the linked list it's in.
		Particle *pPrev = pDest->m_pPrev;
		Particle *pNext = pDest->m_pNext;
		PMaterialHandle pSubTexture = pDest->m_pSubTexture;
		
		*pDest = *pSrc;
		
		pDest->m_pPrev = pPrev;
		pDest->m_pNext = pNext;
		pDest->m_pSubTexture = pSubTexture;
	}
}
		


void AddSimpleParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial, bool bInSkybox )
{
	if ( g_pSimpleSingleton[bInSkybox].IsValid() )
	{
		SimpleParticle *pNew = g_pSimpleSingleton[bInSkybox]->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void AddEmberParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial, bool bInSkybox )
{
	if ( g_pEmberSingleton[bInSkybox].IsValid() )
	{
		SimpleParticle *pNew = g_pEmberSingleton[bInSkybox]->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void AddFireSmokeParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial, bool bInSkybox )
{
	if ( g_pFireSmokeSingleton[bInSkybox].IsValid() )
	{
		SimpleParticle *pNew = g_pFireSmokeSingleton[bInSkybox]->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void AddFireParticle( const SimpleParticle *pParticle, PMaterialHandle hMaterial, bool bInSkybox )
{
	if ( g_pFireSingleton[bInSkybox].IsValid() )
	{
		SimpleParticle *pNew = g_pFireSingleton[bInSkybox]->AddSimpleParticle( hMaterial, pParticle->m_Pos );
		CopyParticle( pParticle, pNew );
	}
}


void DrawParticleSingletons( bool bInSkybox )
{
	if ( g_pSimpleSingleton[bInSkybox].IsValid() )
	{
		g_pSimpleSingleton[bInSkybox]->GetBinding().DrawModel( 1 );
	}

	if ( g_pEmberSingleton[bInSkybox].IsValid() )
	{
		g_pEmberSingleton[bInSkybox]->GetBinding().DrawModel( 1 );
	}

	if ( g_pFireSmokeSingleton[bInSkybox].IsValid() )
	{
		g_pFireSmokeSingleton[bInSkybox]->GetBinding().DrawModel( 1 );
	}
	
	if ( g_pFireSingleton[bInSkybox].IsValid() )
	{
		g_pFireSingleton[bInSkybox]->GetBinding().DrawModel( 1 );
	}
}



