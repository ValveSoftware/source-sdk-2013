//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHEETSIMULATOR_H
#define SHEETSIMULATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "utlvector.h"


// Uncomment this for client-side simulation
//#define CLIENT_SIDE_SIMULATION 1

//-----------------------------------------------------------------------------
// Simulates a sheet
//-----------------------------------------------------------------------------

class CGameTrace;
typedef CGameTrace trace_t;
//struct trace_t;
typedef void (*TraceLineFunc_t)(const Vector &vecStart, const Vector &vecEnd, 
								unsigned int mask, int collisionGroup, trace_t *ptr);
typedef void (*TraceHullFunc_t)(const Vector &vecStart, const Vector &vecEnd, 
								const Vector &hullMin, const Vector &hullMax, 
								unsigned int mask, int collisionGroup, trace_t *ptr);


class CSheetSimulator
{
public:
	CSheetSimulator( TraceLineFunc_t traceline, TraceHullFunc_t traceHull );
	~CSheetSimulator();

	void Init( int w, int h, int fixedPointCount );

	// orientation
	void SetPosition( const Vector& origin, const QAngle& angles );

	// Makes a spring
	void AddSpring( int p1, int p2, float restLength );
	void AddFixedPointSpring( int fixedPoint, int p, float restLength );

	// spring constants....
	void SetPointSpringConstant( float constant );
	void SetFixedSpringConstant( float constant );

	// Used for both kinds of springs
	void SetSpringDampConstant( float damp );
	void SetViscousDrag( float drag );

	// Sets the collision group
	void SetCollisionGroup( int group );

	// Sets the bounding box used for collision vs world
	void SetBoundingBox( Vector& mins, Vector& maxs );

	// Computes the bounding box
	void ComputeBounds( Vector& mins, Vector& maxs );

	// simulation
	void Simulate( float dt );
	void Simulate( float dt, int steps );

	// get at the points
	int NumHorizontal() const;
	int NumVertical() const;
	int PointCount() const;

	// Fixed points
	Vector& GetFixedPoint( int i );

	// Point masses
	const Vector& GetPoint( int x, int y ) const;
	const Vector& GetPoint( int i ) const;

	// For iterative collision detection
	void DetectCollision( int i, float flOffset );
	void InitPosition( int i );

	// For offseting the control points
	void SetControlPointOffset( const Vector& offset );

	// Gravity
	void SetGravityConstant( float g );
	void AddGravityForce( int particle );

protected:
	struct Particle_t
	{
		float	m_Mass;
		Vector	m_Position;
		Vector	m_Velocity;
		Vector	m_Force;
		int		m_Collided;
		int		m_CollisionPlane;
		float	m_CollisionDist;
	};

	struct Spring_t
	{
		int m_Particle1;
		int m_Particle2;
		float m_RestLength;
	};

	inline int NumParticles() const
	{
		return m_HorizontalCount * m_VerticalCount; 
	}

	// simulator
	void			EulerStep( float dt );
	void			ComputeControlPoints();
	void			ClearForces();
	void			ComputeForces();
	void			TestVertAgainstPlane( int vert, int plane, bool bFarTest = true );
	void			SatisfyCollisionConstraints();
	void			DetermineBestCollisionPlane( bool bFarTest = true );
	void			ClampPointsToCollisionPlanes();

	// How many particles horiz + vert?
	int m_HorizontalCount;
	int m_VerticalCount;

	// The particles
	Particle_t*	m_Particle;

	// Output position after simulation
	Vector*	m_OutputPosition;

	// fixed points
	int	m_FixedPointCount;
	Vector* m_pFixedPoint;
	Vector* m_ControlPoints;
	
	CUtlVector<Spring_t>	m_Springs;
	CUtlVector<int>			m_Gravity;

	// raycasting methods
	TraceLineFunc_t		m_TraceLine;
	TraceHullFunc_t		m_TraceHull;

	// Spring constants
	float	m_FixedSpringConstant;
	float	m_PointSpringConstant;
	float	m_DampConstant;
	float	m_ViscousDrag;

	// Collision group
	int		m_CollisionGroup;

	// position + orientation
	Vector	m_Origin;
	QAngle	m_Angles;

	// collision box
	Vector  m_FrustumBoxMin;
	Vector	m_FrustumBoxMax;

	// Collision planes
	cplane_t*	m_pCollisionPlanes;
	bool*		m_pValidCollisionPlane;

	// Control point offset
	Vector		m_ControlPointOffset;

	// Gravity
	float		m_GravityConstant;
};


//-----------------------------------------------------------------------------
// Class to help dealing with the iterative computation
//-----------------------------------------------------------------------------

class CIterativeSheetSimulator : public CSheetSimulator
{
public:
	CIterativeSheetSimulator( TraceLineFunc_t traceline, TraceHullFunc_t traceHull );

	void BeginSimulation( float dt, int steps, int substeps, int collisionCount );

	// Returns true if it just did a simulation step
	bool Think( );
	bool IsDone() const { return m_SimulationSteps == 0; }

	int StepsRemaining( ) const { return m_SimulationSteps; }

private:
	CIterativeSheetSimulator( const CIterativeSheetSimulator & ); // not defined, not accessible

	// Iterative collision detection 
	void DetectCollisions( void );

	float	m_TimeStep;
	float	m_SubSteps;
	char	m_TotalSteps;
	char	m_SimulationSteps;
	char	m_CollisionCount;
	char	m_CurrentCollisionPt;
	bool	m_InitialPass;
};


#endif // TF_SHIELD_SHARED_H
