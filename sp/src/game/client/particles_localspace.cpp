//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Particles which are simulated locally to some space (attachment, bone, etc)
//
//=============================================================================//

#include "cbase.h"
#include "particles_simple.h"
#include "particles_localspace.h"

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CLocalSpaceEmitter::CLocalSpaceEmitter( const char *pDebugName ) :
	CSimpleEmitter( pDebugName )
{
}


inline const matrix3x4_t& CLocalSpaceEmitter::GetTransformMatrix() const
{
	return m_ParticleEffect.GetLocalSpaceTransform();
}


//-----------------------------------------------------------------------------
// Purpose: Creates a local space emitter
//-----------------------------------------------------------------------------
CSmartPtr<CLocalSpaceEmitter> CLocalSpaceEmitter::Create( const char *pDebugName, 
	ClientEntityHandle_t hEntity, int nAttachment, int fFlags )
{
	CLocalSpaceEmitter *pRet = new CLocalSpaceEmitter( pDebugName );
	pRet->SetDynamicallyAllocated( true );
	pRet->m_hEntity = hEntity;
	pRet->m_nAttachment = nAttachment;
	pRet->m_fFlags = fFlags;
	
	pRet->SetupTransformMatrix();

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: Used to build the transformation matrix for this frame
//-----------------------------------------------------------------------------
void CLocalSpaceEmitter::Update( float flTimeDelta )
{
	SetupTransformMatrix();
}

extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );


void CLocalSpaceEmitter::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	float timeDelta = pIterator->GetTimeDelta();

	SimpleParticle *pParticle = (SimpleParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Update velocity
		UpdateVelocity( pParticle, timeDelta );
		pParticle->m_Pos += pParticle->m_vecVelocity * timeDelta;

		// Should this particle die?
		pParticle->m_flLifetime += timeDelta;
		UpdateRoll( pParticle, timeDelta );

		// If we're dead, we're done
		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		{
			pIterator->RemoveParticle( pParticle );
		}
	
		pParticle = (SimpleParticle*)pIterator->GetNext();
	}
}


void CLocalSpaceEmitter::RenderParticles( CParticleRenderIterator *pIterator )
{
	const matrix3x4_t &mLocalToWorld = GetTransformMatrix();
	const VMatrix &mModelView = ParticleMgr()->GetModelView();

	const SimpleParticle *pParticle = (const SimpleParticle *)pIterator->GetFirst();
	while ( pParticle )
	{
		// Transform it
		Vector screenPos, worldPos;
		VectorTransform( pParticle->m_Pos, mLocalToWorld, worldPos );
		
		// Correct viewmodel squashing
		if ( m_fFlags & FLE_VIEWMODEL )
		{
			FormatViewModelAttachment( worldPos, false );
		}

		TransformParticle( mModelView, worldPos, screenPos );
		
		float sortKey = (int) screenPos.z;

		// Render it
		RenderParticle_ColorSizeAngle(
			pIterator->GetParticleDraw(),
			screenPos,
			UpdateColor( pParticle ),
			UpdateAlpha( pParticle ) * GetAlphaDistanceFade( screenPos, m_flNearClipMin, m_flNearClipMax ),
			UpdateScale( pParticle ),
			pParticle->m_flRoll 
			);

		pParticle = (const SimpleParticle *)pIterator->GetNext( sortKey );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Create the matrix by which we'll transform the particle's local 
//			space into world space, via the attachment's transform
//-----------------------------------------------------------------------------
void CLocalSpaceEmitter::SetupTransformMatrix( void )
{
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( m_hEntity );
	if ( pRenderable )
	{
		matrix3x4_t mat;
		if ( pRenderable->GetAttachment( m_nAttachment, mat ) == false )
		{
			// This attachment is bogus!
			Assert(0);
		}
	
		// Tell the particle effect so it knows
		Vector origin;
		MatrixGetColumn( mat, 3, origin );
		m_ParticleEffect.SetLocalSpaceTransform( mat );
		SetSortOrigin( origin );

		C_BaseEntity *pEnt = pRenderable->GetIClientUnknown()->GetBaseEntity();
		if ( pEnt )
		{
			Vector vWorldMins, vWorldMaxs;
			float scale = pEnt->CollisionProp()->BoundingRadius();
			vWorldMins[0] = origin[0] - scale;
			vWorldMins[1] = origin[1] - scale;
			vWorldMins[2] = origin[2] - scale;
			vWorldMaxs[0] = origin[0] + scale;
			vWorldMaxs[1] = origin[1] + scale;
			vWorldMaxs[2] = origin[2] + scale;
			GetBinding().SetBBox( vWorldMins, vWorldMaxs, true );
		}
	}

	// We preapply the local transform because we need to squash it for viewmodel FOV.
	m_ParticleEffect.SetAutoApplyLocalTransform( false );
}

