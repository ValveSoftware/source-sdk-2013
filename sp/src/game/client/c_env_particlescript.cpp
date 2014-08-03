//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "c_baseanimating.h"
#include "particlemgr.h"
#include "materialsystem/imaterialvar.h"
#include "cl_animevent.h"
#include "particle_util.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// An entity which emits other entities at points 
//-----------------------------------------------------------------------------
class C_EnvParticleScript : public C_BaseAnimating, public IParticleEffect
{
public:
	DECLARE_CLASS( C_EnvParticleScript, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_EnvParticleScript();

// IParticleEffect overrides.
public:
	virtual bool	ShouldSimulate() const { return m_bSimulate; }
	virtual void	SetShouldSimulate( bool bSim ) { m_bSimulate = bSim; }

	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

	virtual const Vector &GetSortOrigin();

// C_BaseAnimating overrides
public:
	// NOTE: Ths enclosed particle effect binding will do all the drawing
	// But we have to return true, unlike other particle systems, for the animation events to work
	virtual bool ShouldDraw() { return true; }
	virtual int	DrawModel( int flags ) { return 0; }
	virtual int	GetFxBlend( void ) { return 0; }

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	
private:

	// Creates, destroys particles attached to an attachment
	void CreateParticle( const char *pAttachmentName, const char *pSpriteName );
	void DestroyAllParticles( const char *pAttachmentName );
	void DestroyAllParticles( );

private:
	struct ParticleScriptParticle_t : public Particle
	{
		int m_nAttachment;
		float m_flSize;
	};

	CParticleEffectBinding	m_ParticleEffect;
	float m_flMaxParticleSize;
	int m_nOldSequence;
	float m_flSequenceScale;
	bool m_bSimulate;
};

REGISTER_EFFECT( C_EnvParticleScript );

//-----------------------------------------------------------------------------
// Datatable
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_EnvParticleScript, DT_EnvParticleScript, CEnvParticleScript )
	RecvPropFloat( RECVINFO(m_flSequenceScale) ),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_EnvParticleScript::C_EnvParticleScript()
{
	m_flMaxParticleSize = 0.0f;
	m_bSimulate = true;
}


//-----------------------------------------------------------------------------
// Check for changed sequence numbers
//-----------------------------------------------------------------------------
void C_EnvParticleScript::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_nOldSequence = GetSequence();
}


//-----------------------------------------------------------------------------
// Starts up the particle system
//-----------------------------------------------------------------------------
void C_EnvParticleScript::OnDataChanged( DataUpdateType_t updateType )
{		
	BaseClass::OnDataChanged( updateType );

	if(updateType == DATA_UPDATE_CREATED)
	{
		ParticleMgr()->AddEffect( &m_ParticleEffect, this );
	}

	if ( m_nOldSequence != GetSequence() )
	{
		DestroyAllParticles();
	}
}


//-----------------------------------------------------------------------------
// Creates, destroys particles attached to an attachment
//-----------------------------------------------------------------------------
void C_EnvParticleScript::CreateParticle( const char *pAttachmentName, const char *pSpriteName )
{
	// Find the attachment
	int nAttachment = LookupAttachment( pAttachmentName );
	if ( nAttachment <= 0 )
		return;

	// Get the sprite materials
	PMaterialHandle hMat = m_ParticleEffect.FindOrAddMaterial( pSpriteName );
	ParticleScriptParticle_t *pParticle = 
		(ParticleScriptParticle_t*)m_ParticleEffect.AddParticle(sizeof(ParticleScriptParticle_t), hMat);

	if ( pParticle == NULL )
		return;
	
	// Get the sprite size from the material's materialvars
	bool bFound = false;
	IMaterialVar *pMaterialVar = NULL;
	IMaterial *pMaterial = ParticleMgr()->PMaterialToIMaterial( hMat );
	if ( pMaterial )
	{
		pMaterialVar = pMaterial->FindVar( "$spritesize", &bFound, false );
	}

	if ( bFound )
	{
		pParticle->m_flSize = pMaterialVar->GetFloatValue();
	}
	else
	{
		pParticle->m_flSize = 100.0f;
	}

	// Make sure the particle cull size reflects our particles
	if ( pParticle->m_flSize > m_flMaxParticleSize )
	{
		m_flMaxParticleSize = pParticle->m_flSize;
		m_ParticleEffect.SetParticleCullRadius( m_flMaxParticleSize );
	}

	// Place the particle on the attachment specified
	pParticle->m_nAttachment = nAttachment;
	QAngle vecAngles;
	GetAttachment( nAttachment, pParticle->m_Pos, vecAngles );

	if ( m_flSequenceScale != 1.0f )
	{
		pParticle->m_Pos -= GetAbsOrigin();
		pParticle->m_Pos *= m_flSequenceScale;
		pParticle->m_Pos += GetAbsOrigin();
	}
}

void C_EnvParticleScript::DestroyAllParticles( const char *pAttachmentName )
{
	int nAttachment = LookupAttachment( pAttachmentName );
	if ( nAttachment <= 0 )
		return;

	int nCount = m_ParticleEffect.GetNumActiveParticles();
	Particle** ppParticles = (Particle**)stackalloc( nCount * sizeof(Particle*) );
	int nActualCount = m_ParticleEffect.GetActiveParticleList( nCount, ppParticles );
	Assert( nActualCount == nCount );

	for ( int i = 0; i < nActualCount; ++i )
	{
		ParticleScriptParticle_t *pParticle = (ParticleScriptParticle_t*)ppParticles[i];
		if ( pParticle->m_nAttachment == nAttachment )
		{
			// Mark for deletion
			pParticle->m_nAttachment = -1;
		}
	}
}

void C_EnvParticleScript::DestroyAllParticles( )
{
	int nCount = m_ParticleEffect.GetNumActiveParticles();
	Particle** ppParticles = (Particle**)stackalloc( nCount * sizeof(Particle*) );
	int nActualCount = m_ParticleEffect.GetActiveParticleList( nCount, ppParticles );
	Assert( nActualCount == nCount );

	for ( int i = 0; i < nActualCount; ++i )
	{
		ParticleScriptParticle_t *pParticle = (ParticleScriptParticle_t*)ppParticles[i];

		// Mark for deletion
		pParticle->m_nAttachment = -1;
	}
}


//-----------------------------------------------------------------------------
// The animation events will create particles on the attachment points
//-----------------------------------------------------------------------------
void C_EnvParticleScript::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	// Handle events to create + destroy particles
	switch( event )
	{
	case CL_EVENT_SPRITEGROUP_CREATE:
		{
			char pAttachmentName[256];
			char pSpriteName[256];
			int nArgs = sscanf( options, "%255s %255s", pAttachmentName, pSpriteName );
			if ( nArgs == 2 )
			{
				CreateParticle( pAttachmentName, pSpriteName );
			}
		}
		return;

	case CL_EVENT_SPRITEGROUP_DESTROY:
		{
			char pAttachmentName[256];
			int nArgs = sscanf( options, "%255s", pAttachmentName );
			if ( nArgs == 1 )
			{
				DestroyAllParticles( pAttachmentName );
			}
		}
		return;
	}

	// Fall back
	BaseClass::FireEvent( origin, angles, event, options );
}


//-----------------------------------------------------------------------------
// Simulate the particles
//-----------------------------------------------------------------------------
void C_EnvParticleScript::RenderParticles( CParticleRenderIterator *pIterator )
{
	const ParticleScriptParticle_t* pParticle = (const ParticleScriptParticle_t*)pIterator->GetFirst();
	while ( pParticle )
	{
		Vector vecRenderPos;
		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, vecRenderPos );
		float sortKey = vecRenderPos.z;

		Vector color( 1, 1, 1 );
		RenderParticle_ColorSize( pIterator->GetParticleDraw(), vecRenderPos, color, 1.0f, pParticle->m_flSize );
		
		pParticle = (const ParticleScriptParticle_t*)pIterator->GetNext( sortKey );
	}
}

void C_EnvParticleScript::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	ParticleScriptParticle_t* pParticle = (ParticleScriptParticle_t*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Here's how we retire particles
		if ( pParticle->m_nAttachment == -1 )
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			// Move the particle to the attachment point
			QAngle vecAngles;
			GetAttachment( pParticle->m_nAttachment, pParticle->m_Pos, vecAngles );

			if ( m_flSequenceScale != 1.0f )
			{
				pParticle->m_Pos -= GetAbsOrigin();
				pParticle->m_Pos *= m_flSequenceScale;
				pParticle->m_Pos += GetAbsOrigin();
			}
		}
		
		pParticle = (ParticleScriptParticle_t*)pIterator->GetNext();
	}
}

const Vector &C_EnvParticleScript::GetSortOrigin()
{
	return GetAbsOrigin();
}
