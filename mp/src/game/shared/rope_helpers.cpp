//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "rope_helpers.h"
#include "basetypes.h"
#include "mathlib/mathlib.h"
#include "rope_shared.h"
#include "rope_physics.h"
#include "networkvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHangRope : public CRopePhysics<512>
{
DECLARE_CLASS( CHangRope, CRopePhysics<512> );

// CRopePhysics overrides.
public:

	virtual void	GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
	{
		pAccel->Init( ROPE_GRAVITY );
	}

	
	virtual void ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
	{
		// Apply spring forces.
		BaseClass::ApplyConstraints( pNodes, nNodes );


		// Lock the endpoints.
		pNodes[0].m_vPos = m_vEndPoints[0];
		pNodes[nNodes-1].m_vPos = m_vEndPoints[1];


		// Calculate how far it is hanging down and adjust if necessary.
		float flCurHangDist = 0;
		for ( int i=0; i < NumNodes(); i++ )
		{
			float hang = fabs( m_flStartZ - GetNode(i)->m_vPos.z );
			if ( hang > flCurHangDist )
				flCurHangDist = hang;
		}
		
		// Adjust our spring length accordingly.
		if ( flCurHangDist < m_flWantedHangDist )
			m_flCurSlack += 1;
		else
			m_flCurSlack -= 1;

		ApplyNewSpringLength();
	}


// Helpers.
public:
	
	void	ApplyNewSpringLength()
	{
		ResetSpringLength( (m_flRopeLength + m_flCurSlack + ROPESLACK_FUDGEFACTOR) / (NumNodes() - 1) );
	}


// Variables used to adjust the rope slack.
public:

	Vector	m_vEndPoints[2];
	bool	m_bAdjustSlack;
	
	float	m_flRopeLength;
	float	m_flCurSlack;

	float	m_flWantedHangDist;
	float	m_flStartZ;

};



void CalcRopeStartingConditions( 
	const Vector &vStartPos,
	const Vector &vEndPos,
	int const nNodes,
	float const desiredHang,
	float *pOutputLength,
	float *pOutputSlack
	)
{
	CHangRope rope;

	// Initialize the rope as a straight line with no slack as our first approximation.
	// We then relax the rope by adding slack until it hangs to the desired height.
	//	
	// The spring length equation is:
	// springLength = (ropeLength + slack + ROPESLACK_FUDGEFACTOR) / (nNodes - 1)
	//
	// We want our rope to be a straight line, so: 
	// springLength = ropeLength / (nNodes-1)
	//
	// Therefore our initial slack is -ROPESLACK_FUDGEFACTOR
	rope.m_flCurSlack = -ROPESLACK_FUDGEFACTOR;

	rope.m_vEndPoints[0] = vStartPos;
	rope.m_vEndPoints[1] = vEndPos;
	
	rope.m_flRopeLength = (vEndPos - vStartPos).Length();
	rope.m_flWantedHangDist = desiredHang;
	
	rope.m_flStartZ = MIN( vStartPos.z, vEndPos.z );	// Calculate hang as the Z distance from the
														// lowest endpoint to the bottom of the rope.
	
	rope.SetNumNodes( nNodes );

	// Set the node positions.
	for ( int i=0; i < rope.NumNodes(); i++ )
	{
		CSimplePhysics::CNode *pNode = rope.GetNode( i );

		float t = (float)i / (rope.NumNodes() - 1);
		VectorLerp( vStartPos, vEndPos, t, pNode->m_vPos );
		pNode->m_vPrevPos = pNode->m_vPos;
	}

	// Now simulate a little and stretch out to let it hang down.
	rope.Restart();
	rope.Simulate( 3 );

	// Set outputs.
	*pOutputLength = rope.m_flRopeLength;
	*pOutputSlack = rope.m_flCurSlack;
}


