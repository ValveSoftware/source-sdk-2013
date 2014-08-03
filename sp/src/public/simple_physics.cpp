//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "simple_physics.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CSimplePhysics::CSimplePhysics()
{
	Init( 1.0f / 30.0f ); // default is 30 steps per second
}


void CSimplePhysics::Init( float flTimeStep )
{
	m_flPredictedTime = 0;
	m_iCurTimeStep = 0;
	m_flTimeStep = flTimeStep;
	m_flTimeStepMul = m_flTimeStep*m_flTimeStep*0.5f;
}


void CSimplePhysics::Simulate( 
	CSimplePhysics::CNode *pNodes, 
	int nNodes, 
	CSimplePhysics::IHelper *pHelper, 
	float dt,
	float flDamp )
{
	// Figure out how many time steps to run.
	m_flPredictedTime += dt;
	int newTimeStep = (int)ceil( m_flPredictedTime / m_flTimeStep );
	int nTimeSteps = newTimeStep - m_iCurTimeStep;
	for( int iTimeStep=0; iTimeStep < nTimeSteps; iTimeStep++ )
	{
		// Simulate everything..
		for( int iNode=0; iNode < nNodes; iNode++ )
		{
			CSimplePhysics::CNode *pNode = &pNodes[iNode];

			// Apply forces.
			Vector vAccel;
			pHelper->GetNodeForces( pNodes, iNode, &vAccel );
 			Assert( vAccel.IsValid() ); 

			Vector vPrevPos = pNode->m_vPos;
			pNode->m_vPos = pNode->m_vPos + (pNode->m_vPos - pNode->m_vPrevPos) * flDamp + vAccel * m_flTimeStepMul;
			pNode->m_vPrevPos = vPrevPos;
		}

		// Apply constraints.
		pHelper->ApplyConstraints( pNodes, nNodes );
	}
	m_iCurTimeStep = newTimeStep;

	// Setup predicted positions.
	float flInterpolant = (m_flPredictedTime - (GetCurTime() - m_flTimeStep)) / m_flTimeStep;
	for( int iNode=0; iNode < nNodes; iNode++ )
	{
		CSimplePhysics::CNode *pNode = &pNodes[iNode];
		VectorLerp( pNode->m_vPrevPos, pNode->m_vPos, flInterpolant, pNode->m_vPredicted );
	}
}


