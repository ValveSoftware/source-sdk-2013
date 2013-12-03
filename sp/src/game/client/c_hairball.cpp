//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "simple_physics.h"
#include "mathlib/vmatrix.h"
#include "beamdraw.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_Hairball : public C_BaseEntity
{
	DECLARE_CLASS( C_Hairball, C_BaseEntity );
private:

	class CHairballDelegate : public CSimplePhysics::IHelper
	{
	public:
		virtual void	GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel );
		virtual void	ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes );
	
		C_Hairball		*m_pParent;
	};


public:
	
						C_Hairball();

	void				Init();


// IClientThinkable.
public:
	
	virtual void		ClientThink();


// IClientRenderable.
public:

	virtual int			DrawModel( int flags );



public:

	float								m_flSphereRadius;

	int									m_nHairs;
	int									m_nNodesPerHair;
	float								m_flSpringDist;				// = hair length / (m_nNodesPerHair-1)

	CUtlVector<CSimplePhysics::CNode>	m_Nodes;					// This is m_nHairs * m_nNodesPerHair large.
	CUtlVector<Vector>					m_HairPositions;			// Untransformed base hair positions, distributed on the sphere.
	CUtlVector<Vector>					m_TransformedHairPositions;	// Transformed base hair positions, distributed on the sphere.

	CHairballDelegate					m_Delegate;
	CSimplePhysics						m_Physics;

	IMaterial							*m_pMaterial;


	// Super sophisticated AI.
	float m_flSitStillTime;
	Vector m_vMoveDir;

	float	m_flSpinDuration;
	float	m_flCurSpinTime;
	float	m_flSpinRateX, m_flSpinRateY;
	
	bool	m_bFirstThink;
};


void C_Hairball::CHairballDelegate::GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
{
	pAccel->Init( 0, 0, -1500 );
}


void C_Hairball::CHairballDelegate::ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
{
	int nSegments = m_pParent->m_nNodesPerHair - 1;
	float flSpringDistSqr = m_pParent->m_flSpringDist * m_pParent->m_flSpringDist;

	static int nIterations = 1;
	for( int iIteration=0; iIteration < nIterations; iIteration++ )
	{
		for ( int iHair=0; iHair < m_pParent->m_nHairs; iHair++ )
		{
			CSimplePhysics::CNode *pBase = &pNodes[iHair * m_pParent->m_nNodesPerHair];

			for( int i=0; i < nSegments; i++ )
			{
				Vector &vNode1 = pBase[i].m_vPos;
				Vector &vNode2 = pBase[i+1].m_vPos;
				Vector vTo = vNode1 - vNode2;

				float flDistSqr = vTo.LengthSqr();
				if( flDistSqr > flSpringDistSqr )
				{
					float flDist = (float)sqrt( flDistSqr );
					vTo *= 1 - (m_pParent->m_flSpringDist / flDist);

					vNode1 -= vTo * 0.5f;
					vNode2 += vTo * 0.5f;
				}
			}

			// Lock the base of each hair to the right spot.
			pBase->m_vPos = m_pParent->m_TransformedHairPositions[iHair];
		}
	}
}


C_Hairball::C_Hairball()
{
	m_nHairs = 100;
	m_nNodesPerHair = 3;
	
	float flHairLength = 20;
	m_flSpringDist = flHairLength / (m_nNodesPerHair - 1);
	
	m_Nodes.SetSize( m_nHairs * m_nNodesPerHair );
	m_HairPositions.SetSize( m_nHairs );
	m_TransformedHairPositions.SetSize( m_nHairs );

	m_flSphereRadius = 20;
	m_vMoveDir.Init();
	
	m_flSpinDuration = 1;
	m_flCurSpinTime = 0;
	m_flSpinRateX = m_flSpinRateY = 0;

	// Distribute on the sphere (need a better random distribution for the sphere).
	for ( int i=0; i < m_HairPositions.Count(); i++ )
	{
		float theta = RandomFloat( -M_PI, M_PI );
		float phi   = RandomFloat( -M_PI/2, M_PI/2 );
		
		float cosPhi = cos( phi );

		m_HairPositions[i].Init( 
			cos(theta) * cosPhi * m_flSphereRadius, 
			sin(theta) * cosPhi * m_flSphereRadius, 
			sin(phi) * m_flSphereRadius );
	}

	m_Delegate.m_pParent = this;

	m_Physics.Init( 1.0 / 20 ); // NOTE: PLAY WITH THIS FOR EFFICIENCY
	m_pMaterial = NULL;

	m_bFirstThink = true;
}


void C_Hairball::Init()
{
	ClientEntityList().AddNonNetworkableEntity( this );
	ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	
	AddToLeafSystem( RENDER_GROUP_OPAQUE_ENTITY );

	m_pMaterial = materials->FindMaterial( "cable/cable", TEXTURE_GROUP_OTHER );
	m_flSitStillTime = 5;
}


void C_Hairball::ClientThink()
{
	// Do some AI-type stuff.. move the entity around.
	//C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	//m_vecAngles = SetAbsAngles( pPlayer->GetAbsAngles() ); // copy player angles.

	Assert( !GetMoveParent() );

	// Sophisticated AI.
	m_flCurSpinTime += gpGlobals->frametime;
	if ( m_flCurSpinTime < m_flSpinDuration )
	{
		float div = m_flCurSpinTime / m_flSpinDuration;

		QAngle angles = GetLocalAngles();

		angles.x += m_flSpinRateX * SmoothCurve( div );
		angles.y += m_flSpinRateY * SmoothCurve( div );

		SetLocalAngles( angles );
	}
	else
	{
		// Flip between stopped and starting.
		if ( fabs( m_flSpinRateX ) > 0.01f )
		{
			m_flSpinRateX = m_flSpinRateY = 0;

			m_flSpinDuration = RandomFloat( 1, 2 );
		}
		else
		{
			static float flXSpeed = 3;
			static float flYSpeed = flXSpeed * 0.1f;
			m_flSpinRateX = RandomFloat( -M_PI*flXSpeed, M_PI*flXSpeed );
			m_flSpinRateY = RandomFloat( -M_PI*flYSpeed, M_PI*flYSpeed );

			m_flSpinDuration = RandomFloat( 1, 4 );
		}

		m_flCurSpinTime = 0;
	}

	
	if ( m_flSitStillTime > 0 )
	{
		m_flSitStillTime -= gpGlobals->frametime;

		if ( m_flSitStillTime <= 0 )
		{
			// Shoot out some random lines and find the longest one.
			m_vMoveDir.Init( 1, 0, 0 );
			float flLongestFraction = 0;
			for ( int i=0; i < 15; i++ )
			{
				Vector vDir( RandomFloat( -1, 1 ), RandomFloat( -1, 1 ), RandomFloat( -1, 1 ) );
				VectorNormalize( vDir );

				trace_t trace;
				UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vDir * 10000, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &trace );

				if ( trace.fraction != 1.0 )
				{
					if ( trace.fraction > flLongestFraction )
					{
						flLongestFraction = trace.fraction;
						m_vMoveDir = vDir;
					}
				}
			}

			m_vMoveDir *= 650; // set speed.
			m_flSitStillTime = -1; // Move in this direction..
		}
	}
	else
	{
		// Move in the specified direction.
		Vector vEnd = GetAbsOrigin() + m_vMoveDir * gpGlobals->frametime;

		trace_t trace;
		UTIL_TraceLine( GetAbsOrigin(), vEnd, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &trace );

		if ( trace.fraction < 1 )
		{
			// Ok, stop moving.
			m_flSitStillTime = RandomFloat( 1, 3 );
		}
		else
		{
			SetLocalOrigin( GetLocalOrigin() + m_vMoveDir * gpGlobals->frametime );
		}
	}

	
	// Transform the base hair positions so we can lock them down.
	VMatrix mTransform;
	mTransform.SetupMatrixOrgAngles( GetLocalOrigin(), GetLocalAngles() );

	for ( int i=0; i < m_HairPositions.Count(); i++ )
	{
		Vector3DMultiplyPosition( mTransform, m_HairPositions[i], m_TransformedHairPositions[i] );
	}

	if ( m_bFirstThink )
	{
		m_bFirstThink = false;
		for ( int i=0; i < m_HairPositions.Count(); i++ )
		{
			for ( int j=0; j < m_nNodesPerHair; j++ )
			{
				m_Nodes[i*m_nNodesPerHair+j].Init( m_TransformedHairPositions[i] );
			}
		}
	}

	// Simulate the physics and apply constraints.
	m_Physics.Simulate( m_Nodes.Base(), m_Nodes.Count(), &m_Delegate, gpGlobals->frametime, 0.98 );
}


int C_Hairball::DrawModel( int flags )
{
	if ( !m_pMaterial )
		return 0;

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	for ( int iHair=0; iHair < m_nHairs; iHair++ )
	{
		CSimplePhysics::CNode *pBase = &m_Nodes[iHair * m_nNodesPerHair];
		
		CBeamSegDraw beamDraw;
		beamDraw.Start( pRenderContext, m_nNodesPerHair-1, m_pMaterial );

		for ( int i=0; i < m_nNodesPerHair; i++ )
		{
			BeamSeg_t seg;
			seg.m_vPos = pBase[i].m_vPredicted;
			seg.m_vColor.Init( 0, 0, 0 );
			seg.m_flTexCoord = 0;
			static float flHairWidth = 1;
			seg.m_flWidth = flHairWidth;
			seg.m_flAlpha = 0;

			beamDraw.NextSeg( &seg );
		}
		
		beamDraw.End();
	}

	return 1;
}


void CreateHairballCallback()
{
	for ( int i=0; i < 20; i++ )
	{
		C_Hairball *pHairball = new C_Hairball;
		pHairball->Init();
		
		// Put it a short distance in front of the player.
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		
		if ( !pPlayer )
			return;

		Vector vForward;
		AngleVectors( pPlayer->GetAbsAngles(), &vForward );
		pHairball->SetLocalOrigin( pPlayer->GetAbsOrigin() + vForward * 300 + RandomVector( 0, 100 ) );
	}
}

ConCommand cc_CreateHairball( "CreateHairball", CreateHairballCallback, 0, FCVAR_CHEAT );

