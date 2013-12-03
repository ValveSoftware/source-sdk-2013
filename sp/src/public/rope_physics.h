//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ROPE_PHYSICS_H
#define ROPE_PHYSICS_H
#ifdef _WIN32
#pragma once
#endif


#include "simple_physics.h"
#include "networkvar.h"


class CRopeSpring
{
public:
	Vector		*m_pNode1;
	Vector		*m_pNode2;
};


class CBaseRopePhysics : public CSimplePhysics::IHelper
{
public:
	DECLARE_CLASS_NOBASE( CBaseRopePhysics );

					CBaseRopePhysics( 
						CSimplePhysics::CNode *pNodes, 
						int nNodes, 
						CRopeSpring *pSprings,
						float *flSpringDistsSqr );

	// nNodes should be less than or equal to what you passed into the constructor.
	void			SetNumNodes( int nNodes );

	// Restart timers and such.
	void			Restart();

	void			ResetSpringLength(float flSpringDist );
	float			GetSpringLength() const;
	void			ResetNodeSpringLength( int iStartNode, float flSpringDist );

	// Set simulation parameters.
	// If you pass in a delegate, you can be called to apply constraints.
	void			SetupSimulation( float flSpringDist, CSimplePhysics::IHelper *pDelegate=0 );

	// Set the physics delegate.
	void			SetDelegate( CSimplePhysics::IHelper *pDelegate );

	void			Simulate( float dt );
	
	int						NumNodes()				{ return m_nNodes; }
	CSimplePhysics::CNode*	GetNode( int iNode )	{ return &m_pNodes[iNode]; }
	CSimplePhysics::CNode*	GetFirstNode()			{ return &m_pNodes[0]; }
	CSimplePhysics::CNode*	GetLastNode()			{ return &m_pNodes[ m_nNodes-1 ]; }



public:

	virtual void	GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel );
	virtual void	ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes );


private:

	int				NumSprings()	{return m_nNodes - 1;}


protected:

	CSimplePhysics::IHelper		*m_pDelegate;
	
	CSimplePhysics::CNode		*m_pNodes;
	int				m_nNodes;

	CRopeSpring		*m_pSprings;

  	float			m_flSpringDist;
  	float			m_flSpringDistSqr;

	// Spring lengths per node
	float			*m_flNodeSpringDistsSqr;

	CSimplePhysics	m_Physics;
};





template< int NUM_NODES >
class CRopePhysics : public CBaseRopePhysics
{
public:

								CRopePhysics();

	CSimplePhysics::CNode		m_Nodes[NUM_NODES];
	CRopeSpring		m_Springs[NUM_NODES - 1];
	float			m_SpringDistsSqr[NUM_NODES - 1];
};


template< int NUM_NODES >
CRopePhysics<NUM_NODES>::CRopePhysics() : 
	CBaseRopePhysics( m_Nodes, NUM_NODES, m_Springs, m_SpringDistsSqr )
{
}


#endif // ROPE_PHYSICS_H
