//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


#include "rope_physics.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBaseRopePhysics::CBaseRopePhysics( CSimplePhysics::CNode *pNodes, int nNodes, CRopeSpring *pSprings, float *flSpringDistsSqr )
{
	m_pNodes = pNodes;
	m_pSprings = pSprings;
	m_flNodeSpringDistsSqr = flSpringDistsSqr;
	m_flSpringDist = m_flSpringDistSqr = 1;
	Restart();

	// Initialize the nodes.
	for ( int i=0; i < nNodes; i++ )
	{
		pNodes[i].m_vPos.Init();
		pNodes[i].m_vPrevPos.Init();
		pNodes[i].m_vPredicted.Init();
	}

	SetNumNodes( nNodes );

	m_pDelegate = NULL;
}


void CBaseRopePhysics::SetNumNodes( int nNodes )
{
	m_nNodes = nNodes;

	// Setup the springs.
	for( int i=0; i < NumSprings(); i++ )
	{
		m_pSprings[i].m_pNode1 = &m_pNodes[i].m_vPos;
		m_pSprings[i].m_pNode2 = &m_pNodes[i+1].m_vPos;
		Assert( m_pSprings[i].m_pNode1->IsValid() ); 
		Assert( m_pSprings[i].m_pNode2->IsValid() ); 

		m_flNodeSpringDistsSqr[i] = m_flSpringDistSqr / NumSprings();
	}
}


void CBaseRopePhysics::Restart()
{
	m_Physics.Init( 1.0 / 50 );
}


void CBaseRopePhysics::ResetSpringLength( float flSpringDist )
{
	m_flSpringDist = max( flSpringDist, 0.f );
	m_flSpringDistSqr = m_flSpringDist * m_flSpringDist;

	for( int i=0; i < NumSprings(); i++ )
	{
		m_flNodeSpringDistsSqr[i] = m_flSpringDistSqr / NumSprings();
	}
}

float CBaseRopePhysics::GetSpringLength() const
{
	return m_flSpringDist;
}

void CBaseRopePhysics::ResetNodeSpringLength( int iStartNode, float flSpringDist )
{
	m_flNodeSpringDistsSqr[iStartNode] = flSpringDist * flSpringDist;
}

void CBaseRopePhysics::SetupSimulation( float flSpringDist, CSimplePhysics::IHelper *pDelegate )
{
	ResetSpringLength( flSpringDist );
	SetDelegate( pDelegate );
}


void CBaseRopePhysics::SetDelegate( CSimplePhysics::IHelper *pDelegate )
{
	m_pDelegate = pDelegate;
}


void CBaseRopePhysics::Simulate( float dt )
{
	static float flEnergy = 0.98;
	m_Physics.Simulate( m_pNodes, m_nNodes, this, dt, flEnergy );
}


void CBaseRopePhysics::GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
{
	if( m_pDelegate )
		m_pDelegate->GetNodeForces( pNodes, iNode, pAccel );
	else
		pAccel->Init( 0, 0, 0 );
}


void CBaseRopePhysics::ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
{
	// Handle springs..
	//
	// Iterate multiple times here. If we don't, then gravity tends to
	// win over the constraint solver and it's impossible to get straight ropes.
	static int nIterations = 3;
	for( int iIteration=0; iIteration < nIterations; iIteration++ )
	{
		for( int i=0; i < NumSprings(); i++ )
		{
			CRopeSpring *s = &m_pSprings[i];

			Vector vTo = *s->m_pNode1 - *s->m_pNode2;

			float flDistSqr = vTo.LengthSqr();

			// If we don't have an overall spring distance, see if we have a per-node one
			float flSpringDist = m_flSpringDistSqr;
			if ( !flSpringDist )
			{
				// TODO: This still isn't enough. Ropes with different spring lengths
				// per-node will oscillate forever.
				flSpringDist = m_flNodeSpringDistsSqr[i];
			}

			if( flDistSqr > flSpringDist )
			{
				float flDist = (float)sqrt( flDistSqr );
				vTo *= 1 - (m_flSpringDist / flDist);

				*s->m_pNode1 -= vTo * 0.5f;
				*s->m_pNode2 += vTo * 0.5f;
			}
		}

		if( m_pDelegate )
			m_pDelegate->ApplyConstraints( pNodes, nNodes );
	}
}


