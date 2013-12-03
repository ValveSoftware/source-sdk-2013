//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particles_simple.h"
#include "c_tracer.h"
#include "particle_collision.h"
#include "view.h"
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	PLASMASPARK_LIFETIME 0.5
#define SPRAYS_PER_THINK		12

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectPlasmaBeam )
CLIENTEFFECT_MATERIAL( "sprites/plasmaember" )
CLIENTEFFECT_REGISTER_END()

class  C_PlasmaBeamNode;

//##################################################################
//
// > CPlasmaSpray
//
//##################################################################
class CPlasmaSpray : public CSimpleEmitter
{
public:
	CPlasmaSpray( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static CSmartPtr<CPlasmaSpray>	Create( const char *pDebugName );
	void					Think( void );
	void					UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );
	EHANDLE					m_pOwner;
	CParticleCollision		m_ParticleCollision;

private:
	CPlasmaSpray( const CPlasmaSpray & );
};

//##################################################################
//
// PlasmaBeamNode - generates plasma spray
//
//##################################################################
class C_PlasmaBeamNode : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PlasmaBeamNode, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_PlasmaBeamNode();
	~C_PlasmaBeamNode(void);

public:
	void			ClientThink(void);
	void			AddEntity( void );
	void			OnDataChanged(DataUpdateType_t updateType);
	bool			ShouldDraw();
	bool			m_bSprayOn;
	CSmartPtr<CPlasmaSpray>	m_pFirePlasmaSpray;
};

//##################################################################
//
// > CPlasmaSpray
//
//##################################################################

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
CSmartPtr<CPlasmaSpray> CPlasmaSpray::Create( const char *pDebugName )
{
	CPlasmaSpray *pRet = new CPlasmaSpray( pDebugName );
	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fTimeDelta - 
// Output : Vector
//-----------------------------------------------------------------------------
void CPlasmaSpray::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
	Vector		vGravity = Vector(0,0,-1000);
	float		flFrametime = gpGlobals->frametime;
	vGravity	= flFrametime * vGravity;
	pParticle->m_vecVelocity += vGravity;
}


void CPlasmaSpray::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	float timeDelta = pIterator->GetTimeDelta();

	SimpleParticle *pParticle = (SimpleParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		//Should this particle die?
		pParticle->m_flLifetime += timeDelta;

		C_PlasmaBeamNode* pNode = (C_PlasmaBeamNode*)((C_BaseEntity*)m_pOwner);
		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		{
			pIterator->RemoveParticle( pParticle );
		}
		// If owner is gone or spray off remove me
		else if (pNode == NULL || !pNode->m_bSprayOn)
		{
			pIterator->RemoveParticle( pParticle );
		}

		//Simulate the movement with collision
		trace_t trace;
		m_ParticleCollision.MoveParticle( pParticle->m_Pos, pParticle->m_vecVelocity, NULL, timeDelta, &trace );

		pParticle = (SimpleParticle*)pIterator->GetNext();
	}
}


void CPlasmaSpray::RenderParticles( CParticleRenderIterator *pIterator )
{
	const SimpleParticle *pParticle = (const SimpleParticle *)pIterator->GetFirst();
	while ( pParticle )
	{
		float scale = random->RandomFloat( 0.02, 0.08 );

		// NOTE: We need to do everything in screen space
		Vector  delta;
		Vector	start;
		TransformParticle(ParticleMgr()->GetModelView(), pParticle->m_Pos, start);
		float sortKey = start.z;

		Vector3DMultiply( CurrentWorldToViewMatrix(), pParticle->m_vecVelocity, delta );

		delta[0] *= scale;
		delta[1] *= scale;
		delta[2] *= scale;

		// See c_tracer.* for this method
		Tracer_Draw( pIterator->GetParticleDraw(), start, delta, random->RandomInt( 2, 8 ), 0 );

		pParticle = (const SimpleParticle *)pIterator->GetNext( sortKey );
	}
}


//##################################################################
//
// PlasmaBeamNode - generates plasma spray
//
//##################################################################

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
C_PlasmaBeamNode::C_PlasmaBeamNode(void)
{
	m_bSprayOn			= false;
	m_pFirePlasmaSpray = CPlasmaSpray::Create( "C_PlasmaBeamNode" );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
C_PlasmaBeamNode::~C_PlasmaBeamNode(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PlasmaBeamNode::AddEntity( void )
{
	m_pFirePlasmaSpray->SetSortOrigin( GetAbsOrigin() );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_PlasmaBeamNode::OnDataChanged(DataUpdateType_t updateType)
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		Vector vMoveDir = GetAbsVelocity();
		float  flVel = VectorNormalize(vMoveDir);
		m_pFirePlasmaSpray->m_ParticleCollision.Setup( GetAbsOrigin(), &vMoveDir, 0.3, 
											flVel-50, flVel+50, 800, 0.5 );
		SetNextClientThink(gpGlobals->curtime + 0.01);
	}
	C_BaseEntity::OnDataChanged(updateType);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool C_PlasmaBeamNode::ShouldDraw()
{
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_PlasmaBeamNode::ClientThink(void)
{
	if (!m_bSprayOn)
	{
		return;
	}
	
	trace_t trace;
	Vector vEndTrace = GetAbsOrigin() + (0.3*GetAbsVelocity());
	UTIL_TraceLine( GetAbsOrigin(), vEndTrace, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &trace );
	if ( trace.fraction != 1.0f || trace.startsolid)
	{
		m_bSprayOn = false;
		return;
	}

	PMaterialHandle handle = m_pFirePlasmaSpray->GetPMaterial( "sprites/plasmaember" );
	for (int i=0;i<SPRAYS_PER_THINK;i++)
	{
		SimpleParticle	*sParticle;

		//Make a new particle
		if ( random->RandomInt( 0, 2 ) == 0 )
		{
			float ranx = random->RandomFloat( -28.0f, 28.0f );
			float rany = random->RandomFloat( -28.0f, 28.0f );
			float ranz = random->RandomFloat( -28.0f, 28.0f );

			Vector vNewPos	=  GetAbsOrigin();
			Vector vAdd		=  Vector(GetAbsAngles().x,GetAbsAngles().y,GetAbsAngles().z)*random->RandomFloat(-60,120);
			vNewPos			+= vAdd;

			sParticle = (SimpleParticle *) m_pFirePlasmaSpray->AddParticle( sizeof(SimpleParticle), handle, vNewPos );
			
			sParticle->m_flLifetime		= 0.0f;
			sParticle->m_flDieTime		= PLASMASPARK_LIFETIME;

			sParticle->m_vecVelocity	= GetAbsVelocity();
			sParticle->m_vecVelocity.x	+= ranx;
			sParticle->m_vecVelocity.y	+= rany;
			sParticle->m_vecVelocity.z	+= ranz;
			m_pFirePlasmaSpray->m_pOwner	=  this;
		}
	}

	SetNextClientThink(gpGlobals->curtime + 0.05);
}

IMPLEMENT_CLIENTCLASS_DT(C_PlasmaBeamNode, DT_PlasmaBeamNode, CPlasmaBeamNode )
	RecvPropVector	(RECVINFO(m_vecVelocity), 0, RecvProxy_LocalVelocity),
	RecvPropInt		(RECVINFO(m_bSprayOn)),
END_RECV_TABLE()
