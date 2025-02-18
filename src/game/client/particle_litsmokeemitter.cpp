//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "particle_litsmokeemitter.h"


//
// CLitSmokeEmitter
//

CLitSmokeEmitter::CLitSmokeEmitter( const char *pDebugName ) : CSimpleEmitter( pDebugName )
{
	m_bInitted = false;
	m_hSmokeMaterial = INVALID_MATERIAL_HANDLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *materialName - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::Init( const char *materialName, Vector sortOrigin )
{
	m_hSmokeMaterial = GetPMaterial( materialName );
	
	IMaterial *pMaterial = ParticleMgr()->PMaterialToIMaterial( m_hSmokeMaterial );
	if ( pMaterial )
	{
		m_Renderer.Init( ParticleMgr(), pMaterial );
	}

	SetSortOrigin( sortOrigin );
	m_bInitted = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//			color - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::SetDirectionalLight( Vector position, Vector color, float intensity )
{
	CParticleLightInfo info;
	info.m_flIntensity = intensity;
	info.m_vColor = color;
	info.m_vPos	= position;
	
	m_Renderer.SetDirectionalLight( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//			color - 
//			intensity - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::SetLight( Vector position, Vector color, float intensity )
{
	CParticleLightInfo info;
	info.m_flIntensity = intensity;
	info.m_vColor = color;
	info.m_vPos	= position;

	m_Renderer.SetAmbientLight( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flTimeDelta - 
//-----------------------------------------------------------------------------
void CLitSmokeEmitter::Update( float flTimeDelta )
{
	if ( flTimeDelta > 0.0f )
	{
		//m_Renderer.DirectionalLight().m_vColor *= 0.9f;
	}

	CSimpleEmitter::Update( flTimeDelta );
}

void CLitSmokeEmitter::StartRender( VMatrix &effectMatrix )
{
	m_Renderer.StartRender( effectMatrix );
}

void CLitSmokeEmitter::RenderParticles( CParticleRenderIterator *pIterator )
{
	const LitSmokeParticle *pParticle = (const LitSmokeParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		float	tLifetime = pParticle->m_flLifetime / pParticle->m_flDieTime;

		// Transform.						   
		Vector tPos;

		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, tPos );
		float sortKey = tPos.z;
		
		float alpha255 = ( ( (float) pParticle->m_uchColor[3]/255.0f ) * sin( M_PI_F * tLifetime ) ) * 255.0f;

		Vector	color01 = Vector( pParticle->m_uchColor[0], pParticle->m_uchColor[1], pParticle->m_uchColor[2] ) * (tLifetime / 255.0f);

		m_Renderer.RenderParticle_AddColor (
			pIterator->GetParticleDraw(),
			pParticle->m_Pos,
			tPos,
			alpha255,
			FLerp( pParticle->m_uchStartSize, pParticle->m_uchEndSize, tLifetime ),
			color01
		);
		
		pParticle = (const LitSmokeParticle*)pIterator->GetNext( sortKey );
	}
}


void CLitSmokeEmitter::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	// Make sure they've called Init().
	Assert( m_bInitted );
	
	LitSmokeParticle *pParticle = (LitSmokeParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Should this particle die?
		pParticle->m_flLifetime += pIterator->GetTimeDelta();
		pParticle->m_Pos = pParticle->m_Pos + ( pParticle->m_vecVelocity * pIterator->GetTimeDelta() );

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
			pIterator->RemoveParticle( pParticle );

		pParticle = (LitSmokeParticle*)pIterator->GetNext();
	}
}


