//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SIMPLE_PHYSICS_H
#define SIMPLE_PHYSICS_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib/vector.h"


// CSimplePhysics is a framework for simplified physics simulation.
// It simulates at a fixed timestep and uses the Verlet integrator.
//
// To use it, create your nodes and implement your constraints and
// forces in an IHelper, then call Simulate each frame. 
// CSimplePhysics will figure out how many timesteps to run and will
// provide predicted positions of things for you.
class CSimplePhysics
{
public:

	class CNode
	{
	public:
		
		// Call this when initializing the nodes with their starting positions.
		void		Init( const Vector &vPos )
		{
			m_vPos = m_vPrevPos = m_vPredicted = vPos;
		}
		
		Vector		m_vPos;			// At time t
		Vector		m_vPrevPos;		// At time t - m_flTimeStep
		Vector		m_vPredicted;	// Predicted position
	};

	class IHelper
	{
	public:
		virtual void	GetNodeForces( CNode *pNodes, int iNode, Vector *pAccel ) = 0;
		virtual void	ApplyConstraints( CNode *pNodes, int nNodes ) = 0;
	};


public:

				CSimplePhysics();
	
	void		Init( float flTimeStep );
	
	void		Simulate( 
		CNode *pNodes, 
		int nNodes, 
		IHelper *pHelper, 
		float dt,
		float flDamp );


private:

	double		GetCurTime()		{ return m_flTimeStep * m_iCurTimeStep; }


private:

	double		m_flPredictedTime;	// (GetCurTime()-m_flTimeStep) <= m_flPredictedTime <= GetCurTime()
	int			m_iCurTimeStep;
	
	float		m_flTimeStep;
	float		m_flTimeStepMul;	// dt*dt*0.5
};

 
#endif // SIMPLE_PHYSICS_H
