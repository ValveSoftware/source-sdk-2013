//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseparticleentity.h"
#include "entityparticletrail_shared.h"
#include "particlemgr.h"
#include "particle_util.h"
#include "particles_simple.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Entity particle trail, client-side implementation
//-----------------------------------------------------------------------------
class C_EntityParticleTrail : public C_BaseParticleEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_EntityParticleTrail, C_BaseParticleEntity );

	C_EntityParticleTrail( );
	~C_EntityParticleTrail( );

// C_BaseEntity
	virtual void OnDataChanged( DataUpdateType_t updateType );

// IParticleEffect
	void Update( float fTimeDelta );
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

private:

	C_EntityParticleTrail( const C_EntityParticleTrail & ); // not defined, not accessible

	void Start( );
	void AddParticle( float flInitialDeltaTime, const Vector &vecMins, const Vector &vecMaxs, const matrix3x4_t &boxToWorld );

	int		m_iMaterialName;
	EntityParticleTrailInfo_t	m_Info;
	EHANDLE m_hConstraintEntity;

	PMaterialHandle		m_hMaterial;
	TimedEvent			m_teParticleSpawn;
};


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_EntityParticleTrail, DT_EntityParticleTrail, CEntityParticleTrail )
	RecvPropInt(RECVINFO(m_iMaterialName)),
	RecvPropDataTable( RECVINFO_DT( m_Info ), 0, &REFERENCE_RECV_TABLE(DT_EntityParticleTrailInfo) ),
	RecvPropEHandle(RECVINFO(m_hConstraintEntity)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_EntityParticleTrail::C_EntityParticleTrail( void )
{
}

C_EntityParticleTrail::~C_EntityParticleTrail()
{
	ParticleMgr()->RemoveEffect( &m_ParticleEffect );
}



//-----------------------------------------------------------------------------
// On data changed
//-----------------------------------------------------------------------------
void C_EntityParticleTrail::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticleMgr - 
//			*pArgs - 
//-----------------------------------------------------------------------------
void C_EntityParticleTrail::Start( )
{
	if( ParticleMgr()->AddEffect( &m_ParticleEffect, this ) == false )
		return;

	const char *pMaterialName = GetMaterialNameFromIndex( m_iMaterialName );
	if ( !pMaterialName )
		return;

	m_hMaterial	= ParticleMgr()->GetPMaterial( pMaterialName );	
	m_teParticleSpawn.Init( 150 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityParticleTrail::AddParticle( float flInitialDeltaTime, const Vector &vecMins, const Vector &vecMaxs, const matrix3x4_t &boxToWorld )
{
	// Select a random point somewhere in the hitboxes of the entity.
	Vector vecLocalPosition, vecWorldPosition;
	vecLocalPosition.x			= Lerp( random->RandomFloat( 0.0f, 1.0f ), vecMins.x, vecMaxs.x );
	vecLocalPosition.y			= Lerp( random->RandomFloat( 0.0f, 1.0f ), vecMins.y, vecMaxs.y );
	vecLocalPosition.z			= Lerp( random->RandomFloat( 0.0f, 1.0f ), vecMins.z, vecMaxs.z );
	VectorTransform( vecLocalPosition, boxToWorld, vecWorldPosition );

	// Don't emit the particle unless it's inside the model
	if ( m_hConstraintEntity.Get() )
	{
		Ray_t ray;
		trace_t tr;
		ray.Init( vecWorldPosition, vecWorldPosition );
		enginetrace->ClipRayToEntity( ray, MASK_ALL, m_hConstraintEntity, &tr );
		
		if ( !tr.startsolid )
			return;
	}

	// Make a new particle
	SimpleParticle *pParticle = (SimpleParticle *)m_ParticleEffect.AddParticle( sizeof(SimpleParticle), m_hMaterial );
	if ( pParticle == NULL )
		return;

	pParticle->m_Pos			= vecWorldPosition;
	pParticle->m_flRoll			= Helper_RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= Helper_RandomFloat( -2.0f, 2.0f );

	pParticle->m_flLifetime		= flInitialDeltaTime;
	pParticle->m_flDieTime		= m_Info.m_flLifetime;

	pParticle->m_uchColor[0]	= 64;
	pParticle->m_uchColor[1]	= 140;
	pParticle->m_uchColor[2]	= 225;
	pParticle->m_uchStartAlpha	= Helper_RandomInt( 64, 64 );
	pParticle->m_uchEndAlpha	= 0;

	pParticle->m_uchStartSize	= m_Info.m_flStartSize;
	pParticle->m_uchEndSize		= m_Info.m_flEndSize;

	pParticle->m_vecVelocity	= vec3_origin;
	VectorMA( pParticle->m_Pos, flInitialDeltaTime, pParticle->m_vecVelocity, pParticle->m_Pos );  
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
//-----------------------------------------------------------------------------
void C_EntityParticleTrail::Update( float fTimeDelta )
{
	float tempDelta = fTimeDelta;
	studiohdr_t *pStudioHdr;
	mstudiohitboxset_t *set;
	matrix3x4_t	*hitboxbones[MAXSTUDIOBONES];

	C_BaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
		return;

	C_BaseAnimating *pAnimating = pMoveParent->GetBaseAnimating();
	if (!pAnimating)
		goto trailNoHitboxes;

	if ( !pAnimating->HitboxToWorldTransforms( hitboxbones ) )
		goto trailNoHitboxes;

	pStudioHdr = modelinfo->GetStudiomodel( pAnimating->GetModel() );
	if (!pStudioHdr)
		goto trailNoHitboxes;

	set = pStudioHdr->pHitboxSet( pAnimating->GetHitboxSet() );
	if ( !set )
		goto trailNoHitboxes;

	//Add new particles
	while ( m_teParticleSpawn.NextEvent( tempDelta ) )
	{
		int nHitbox = random->RandomInt( 0, set->numhitboxes - 1 );
		mstudiobbox_t *pBox = set->pHitbox(nHitbox);

		AddParticle( tempDelta, pBox->bbmin, pBox->bbmax, *hitboxbones[pBox->bone] );
	}
	return;

trailNoHitboxes:
	while ( m_teParticleSpawn.NextEvent( tempDelta ) )
	{
		AddParticle( tempDelta, pMoveParent->CollisionProp()->OBBMins(), pMoveParent->CollisionProp()->OBBMaxs(), pMoveParent->EntityToWorldTransform() );
	}
}


inline void C_EntityParticleTrail::RenderParticles( CParticleRenderIterator *pIterator )
{
	const SimpleParticle *pParticle = (const SimpleParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		float t = pParticle->m_flLifetime / pParticle->m_flDieTime;

		// Render
		Vector	tPos;
		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, tPos );
		float sortKey = tPos.z;

		Vector	color = Vector( pParticle->m_uchColor[0] / 255.0f, pParticle->m_uchColor[1] / 255.0f, pParticle->m_uchColor[2] / 255.0f );
		float alpha = Lerp( t, pParticle->m_uchStartAlpha / 255.0f, pParticle->m_uchEndAlpha / 255.0f );
		float flSize = Lerp( t, pParticle->m_uchStartSize, pParticle->m_uchEndSize );

		// Render it
		RenderParticle_ColorSize( pIterator->GetParticleDraw(), tPos, color, alpha, flSize );
		
		pParticle = (const SimpleParticle*)pIterator->GetNext( sortKey );
	}
}


inline void C_EntityParticleTrail::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	SimpleParticle *pParticle = (SimpleParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		// Update position
		float flTimeDelta = pIterator->GetTimeDelta();
		pParticle->m_Pos += pParticle->m_vecVelocity * flTimeDelta;

		// NOTE: I'm overloading "die time" to be the actual start time.
		pParticle->m_flLifetime += flTimeDelta;

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
			pIterator->RemoveParticle( pParticle );

		pParticle = (SimpleParticle*)pIterator->GetNext();
	}
}

