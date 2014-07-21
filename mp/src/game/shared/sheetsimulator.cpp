//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:			The Escort's Shield weapon effect
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "sheetsimulator.h"
#include "edict.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define COLLISION_PLANE_OFFSET 6.0f
		  
//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CSheetSimulator::CSheetSimulator( TraceLineFunc_t traceline, 
							 TraceHullFunc_t traceHull ) : 
	m_pFixedPoint(0), m_ControlPoints(0),
	m_TraceLine(traceline), m_TraceHull(traceHull)
{
}

CSheetSimulator::~CSheetSimulator()
{
	if (m_pFixedPoint)
	{
		delete[] m_pFixedPoint;
		delete[] m_ControlPoints;
		delete[] m_pCollisionPlanes;
		delete[] m_pValidCollisionPlane;
	}
	delete[] m_Particle;
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------

void CSheetSimulator::Init( int w, int h, int fixedPointCount )
{
	m_ControlPointOffset.Init( 0, 0, 0 );
	m_HorizontalCount = w;
	m_VerticalCount = h;
	m_Particle = new Particle_t[w * h];
	m_FixedPointCount = fixedPointCount;
	if (fixedPointCount)
	{
		m_pFixedPoint = new Vector[fixedPointCount];
		m_ControlPoints = new Vector[fixedPointCount];
		m_pCollisionPlanes = new cplane_t[fixedPointCount];
		m_pValidCollisionPlane = new bool[fixedPointCount];
	}

	// Initialize distances and such
	m_Origin = Vector(0, 0, 0);
	for ( int i = 0; i < NumParticles(); ++i )
	{
		m_Particle[i].m_Mass = 1.0f;
		m_Particle[i].m_Collided = false;
		m_Particle[i].m_Position = Vector(0,0,0);
		m_Particle[i].m_Velocity = Vector(0,0,0);
	}
}

//-----------------------------------------------------------------------------
// adds springs
//-----------------------------------------------------------------------------

void CSheetSimulator::AddSpring( int p1, int p2, float restLength )
{
	int spring = m_Springs.AddToTail();
	m_Springs[spring].m_Particle1 = p1;
	m_Springs[spring].m_Particle2 = p2;
	m_Springs[spring].m_RestLength = restLength;
}

void CSheetSimulator::AddFixedPointSpring( int fixedPoint, int p, float restLength )
{
	assert( fixedPoint < m_FixedPointCount );
	int spring = m_Springs.AddToTail();
	m_Springs[spring].m_Particle1 = p;
	m_Springs[spring].m_Particle2 = -(fixedPoint+1);
	m_Springs[spring].m_RestLength = restLength;
}

//-----------------------------------------------------------------------------
// Gravity
//-----------------------------------------------------------------------------

void CSheetSimulator::SetGravityConstant( float g )
{
	m_GravityConstant = g;
}

void CSheetSimulator::AddGravityForce( int particle )
{
	m_Gravity.AddToTail( particle );
}

//-----------------------------------------------------------------------------
// spring constants....
//-----------------------------------------------------------------------------

void CSheetSimulator::SetPointSpringConstant( float constant )
{
	m_PointSpringConstant = constant;
}

void CSheetSimulator::SetFixedSpringConstant( float constant )
{
	m_FixedSpringConstant = constant;
}

void CSheetSimulator::SetViscousDrag( float drag )
{
	m_ViscousDrag = drag;
}

void CSheetSimulator::SetSpringDampConstant( float damp )
{
	m_DampConstant = damp;
}


//-----------------------------------------------------------------------------
// Sets the collision group
//-----------------------------------------------------------------------------

void CSheetSimulator::SetCollisionGroup( int group )
{
	m_CollisionGroup = group;
}


//-----------------------------------------------------------------------------
// bounding box for collision
//-----------------------------------------------------------------------------

void CSheetSimulator::SetBoundingBox( Vector& mins, Vector& maxs )
{
	m_FrustumBoxMin = mins;
	m_FrustumBoxMax = maxs;
}

//-----------------------------------------------------------------------------
// bounding box for collision
//-----------------------------------------------------------------------------
 
void CSheetSimulator::ComputeBounds( Vector& mins, Vector& maxs )
{
	VectorCopy( m_Particle[0].m_Position, mins );
	VectorCopy( m_Particle[0].m_Position, maxs );

	for (int i = 1; i < NumParticles(); ++i)
	{
		VectorMin( mins, m_Particle[i].m_Position, mins );
		VectorMax( maxs, m_Particle[i].m_Position, maxs );
	}
	mins -= m_Origin;
	maxs -= m_Origin;
}

//-----------------------------------------------------------------------------
// Set the shield position
//-----------------------------------------------------------------------------

void CSheetSimulator::SetPosition( const Vector& origin, const QAngle& angles )
{
	// FIXME: Need a better metric for position reset
	if (m_Origin.DistToSqr(origin) > 1e3)
	{
		for ( int i = 0; i < NumParticles(); ++i )
		{
			m_Particle[i].m_Position = origin;
			m_Particle[i].m_Velocity = Vector(0,0,0);
		}
	}

	m_Origin = origin;
	m_Angles = angles;
	ComputeControlPoints();
}

//-----------------------------------------------------------------------------
// get at the points
//-----------------------------------------------------------------------------

int CSheetSimulator::NumHorizontal() const
{
	return m_HorizontalCount;
}

int CSheetSimulator::NumVertical() const
{
	return m_VerticalCount;
}

int CSheetSimulator::PointCount() const
{
	return m_HorizontalCount * m_VerticalCount; 
}

const Vector& CSheetSimulator::GetPoint( int x, int y ) const
{
	return m_Particle[y * NumHorizontal() + x].m_Position;
}

const Vector& CSheetSimulator::GetPoint( int i ) const
{
	return m_Particle[i].m_Position;
}

// Fixed points
Vector& CSheetSimulator::GetFixedPoint( int i )
{
	assert( i < m_FixedPointCount );
	return m_pFixedPoint[i];
}

//-----------------------------------------------------------------------------
// For offseting the control points
//-----------------------------------------------------------------------------

void CSheetSimulator::SetControlPointOffset( const Vector& offset )
{
	VectorCopy( offset, m_ControlPointOffset );
}

//-----------------------------------------------------------------------------
// Compute the position of the fixed points
//-----------------------------------------------------------------------------

void CSheetSimulator::ComputeControlPoints()
{
	//trace_t tr;
	Vector forward, right, up;
	AngleVectors(m_Angles, &forward, &right, &up);

	for (int i = 0; i < m_FixedPointCount; ++i)
	{
		VectorAdd( m_Origin, m_ControlPointOffset, m_ControlPoints[i] );
		m_ControlPoints[i] += right * m_pFixedPoint[i].x;
		m_ControlPoints[i] += up * m_pFixedPoint[i].z;
		m_ControlPoints[i] += forward * m_pFixedPoint[i].y;
	}
}

//-----------------------------------------------------------------------------
// Clear forces + velocities affecting each point 
//-----------------------------------------------------------------------------

void CSheetSimulator::ClearForces()
{
	int i;
	for ( i = 0; i < NumParticles(); ++i)
	{
		m_Particle[i].m_Force = Vector(0,0,0);
	}
}

//-----------------------------------------------------------------------------
// Update the shield positions 
//-----------------------------------------------------------------------------

void CSheetSimulator::ComputeForces()
{

	float springConstant;
	int i;
	for ( i = 0; i < m_Springs.Size(); ++i )
	{
		// Hook's law for a damped spring:
		// got two particles, a and b with positions xa and xb and velocities va and vb
		// and l = xa - xb
		// fa = -( ks * (|l| - r) + kd * (va - vb) dot (l) / |l|) * l/|l|

		Vector dx, dv, force;
		if (m_Springs[i].m_Particle2 < 0)
		{
			// Case where we're connected to a control point
			dx = m_Particle[m_Springs[i].m_Particle1].m_Position - 
				m_ControlPoints[- m_Springs[i].m_Particle2 - 1];
			dv = m_Particle[m_Springs[i].m_Particle1].m_Velocity;

			springConstant = m_FixedSpringConstant;
		}
		else
		{
			// Case where we're connected to another part of the shield
			dx = m_Particle[m_Springs[i].m_Particle1].m_Position - 
				m_Particle[m_Springs[i].m_Particle2].m_Position;
			dv = m_Particle[m_Springs[i].m_Particle1].m_Velocity - 
				m_Particle[m_Springs[i].m_Particle2].m_Velocity;

			springConstant = m_PointSpringConstant;
		}

		float length = dx.Length();
		if (length < 1e-6)
			continue;

		dx /= length;

		float springfactor = springConstant * ( length - m_Springs[i].m_RestLength);
		float dampfactor = m_DampConstant * DotProduct( dv, dx );
		force = dx * -( springfactor + dampfactor );

		m_Particle[m_Springs[i].m_Particle1].m_Force += force;
		if (m_Springs[i].m_Particle2 >= 0)
			m_Particle[m_Springs[i].m_Particle2].m_Force -= force;

		assert( IsFinite( m_Particle[m_Springs[i].m_Particle1].m_Force.x ) &&
			IsFinite( m_Particle[m_Springs[i].m_Particle1].m_Force.y) &&
			IsFinite( m_Particle[m_Springs[i].m_Particle1].m_Force.z) );
	}

	// gravity term
	for (i = 0; i < m_Gravity.Count(); ++i)
	{
		m_Particle[m_Gravity[i]].m_Force.z -= m_Particle[m_Gravity[i]].m_Mass * m_GravityConstant;
	}

	// viscous drag term
	for (i = 0; i < NumParticles(); ++i)
	{
		// Factor out bad forces for surface contact 
		// Do this before the drag term otherwise the drag will be too large
		if ((m_Particle[i].m_CollisionPlane) >= 0)
		{
			const Vector& planeNormal = m_pCollisionPlanes[m_Particle[i].m_CollisionPlane].normal;
			float perp = DotProduct( m_Particle[i].m_Force, planeNormal );
			if (perp < 0)
				m_Particle[i].m_Force -= planeNormal * perp;
		}

		Vector drag = m_Particle[i].m_Velocity * m_ViscousDrag;
		m_Particle[i].m_Force -= drag;
	}
}


//-----------------------------------------------------------------------------
// Used for testing neighbors against a particular plane
//-----------------------------------------------------------------------------
void CSheetSimulator::TestVertAgainstPlane( int vert, int plane, bool bFarTest )
{
	if (!m_pValidCollisionPlane[plane])
		return;

	// Compute distance to the plane under consideration
	cplane_t* pPlane = &m_pCollisionPlanes[plane];

	Ray_t ray;
	ray.Init( m_Origin, m_Particle[vert].m_Position ); 
	float t = IntersectRayWithPlane( ray, *pPlane ); 

	if (!bFarTest || (t <= 1.0f))
	{
		if ((t < m_Particle[vert].m_CollisionDist) && (t >= 0.0f))
		{
			m_Particle[vert].m_CollisionDist = t;
			m_Particle[vert].m_CollisionPlane = plane;
		}
	}
}



//-----------------------------------------------------------------------------
// Collision detect
//-----------------------------------------------------------------------------
void CSheetSimulator::InitPosition( int i )
{
	// Collision test...
	// Check a line that goes farther out than our current point...
	// This will let us check for resting contact
	trace_t tr;
	m_TraceHull(m_Origin, m_ControlPoints[i], m_FrustumBoxMin, m_FrustumBoxMax,
		MASK_SOLID_BRUSHONLY, m_CollisionGroup, &tr );
	if ( tr.fraction - 1.0 < 0 )
	{
		memcpy( &m_pCollisionPlanes[i], &tr.plane, sizeof(cplane_t) );
		m_pCollisionPlanes[i].dist += COLLISION_PLANE_OFFSET;

		// The trace endpos represents where the center of the box
		// ends up being. We actually want to choose a point which is on the 
		// collision plane
		Vector delta;
		VectorSubtract( m_ControlPoints[i], m_Origin, delta );
		int maxdist = VectorNormalize( delta ); 
		float dist = (m_pCollisionPlanes[i].dist - DotProduct( m_Origin, m_pCollisionPlanes[i].normal )) / 
			DotProduct( delta, m_pCollisionPlanes[i].normal );

		if (dist > maxdist)
			dist = maxdist;

		VectorMA( m_Origin, dist, delta, m_Particle[i].m_Position );
		
		m_pValidCollisionPlane[i] = true;
	}
	else if (tr.allsolid || tr.startsolid)
	{
		m_pValidCollisionPlane[i] = true;
		VectorSubtract( m_Origin, m_ControlPoints[i], m_pCollisionPlanes[i].normal );
		VectorNormalize( m_pCollisionPlanes[i].normal ); 
		m_pCollisionPlanes[i].dist = DotProduct( m_Origin, m_pCollisionPlanes[i].normal ) - COLLISION_PLANE_OFFSET;
		m_pCollisionPlanes[i].type = 3;
	}
	else
	{
		VectorCopy( m_ControlPoints[i], m_Particle[i].m_Position );
		m_pValidCollisionPlane[i] = false;
	}
}

//-----------------------------------------------------------------------------
// Collision detect
//-----------------------------------------------------------------------------
void CSheetSimulator::DetectCollision( int i, float flPlaneOffset )
{
	// Collision test...
	// Check a line that goes farther out than our current point...
	// This will let us check for resting contact
//	Vector endpt = m_Particle[i].m_Position;

	trace_t tr;
	m_TraceHull(m_Origin, m_ControlPoints[i], m_FrustumBoxMin, m_FrustumBoxMax,
		MASK_SOLID_BRUSHONLY, m_CollisionGroup, &tr );
	if ( tr.fraction - 1.0 < 0 )
	{
		m_pValidCollisionPlane[i] = true;
		memcpy( &m_pCollisionPlanes[i], &tr.plane, sizeof(cplane_t) );
		m_pCollisionPlanes[i].dist += flPlaneOffset;
	}
	else if (tr.allsolid || tr.startsolid)
	{
		m_pValidCollisionPlane[i] = true;
		VectorSubtract( m_Origin, m_ControlPoints[i], m_pCollisionPlanes[i].normal );
		VectorNormalize( m_pCollisionPlanes[i].normal ); 
		m_pCollisionPlanes[i].dist = DotProduct( m_Origin, m_pCollisionPlanes[i].normal ) - flPlaneOffset;
		m_pCollisionPlanes[i].type = 3;
	}
	else
	{
		m_pValidCollisionPlane[i] = false;
	}
}


//-----------------------------------------------------------------------------
// Collision plane fixup
//-----------------------------------------------------------------------------
void CSheetSimulator::DetermineBestCollisionPlane( bool bFarTest )
{
	// Check neighbors for violation of collision plane constraints
	for	( int i = 0; i < NumVertical(); ++i)
	{
		for ( int j = 0; j < NumHorizontal(); ++j)
		{
			// Here's the particle we're making springs for
			int idx = i * NumHorizontal() + j;

			// Now that we've seen all collisions, find the best collision plane
			// to use (look at myself and all neighbors). The best plane
			// is the one that comes closest to the origin.
			m_Particle[idx].m_CollisionDist = FLT_MAX;
			m_Particle[idx].m_CollisionPlane = -1;
			TestVertAgainstPlane( idx, idx, bFarTest );
			if (j > 0)
			{
				TestVertAgainstPlane( idx, idx-1, bFarTest );
			}
			if (j < NumHorizontal() - 1)
			{
				TestVertAgainstPlane( idx, idx+1, bFarTest );
			}
			if (i > 0)
				TestVertAgainstPlane( idx, idx-NumHorizontal(), bFarTest );
			if (i < NumVertical() - 1)
				TestVertAgainstPlane( idx, idx+NumHorizontal(), bFarTest );
		}
	}
}

//-----------------------------------------------------------------------------
// satify collision constraints
//-----------------------------------------------------------------------------

void CSheetSimulator::SatisfyCollisionConstraints()
{
	// Eliminate velocity perp to a collision plane
	for ( int i = 0; i < NumParticles(); ++i )
	{
		// The actual collision plane 
		if (m_Particle[i].m_CollisionPlane >= 0)
		{
			cplane_t* pPlane = &m_pCollisionPlanes[m_Particle[i].m_CollisionPlane];

			// Fix up position so it lies on the plane
			Vector delta = m_Particle[i].m_Position - m_Origin;
			m_Particle[i].m_Position = m_Origin + delta * m_Particle[i].m_CollisionDist;

			float perp = DotProduct( m_Particle[i].m_Velocity, pPlane->normal );
			if (perp < 0)
				m_Particle[i].m_Velocity -=	pPlane->normal * perp;
		}
	}
}

//-----------------------------------------------------------------------------
// integrator
//-----------------------------------------------------------------------------

void CSheetSimulator::EulerStep( float dt )
{
	ClearForces();
	ComputeForces();

	// Update positions and velocities
	for ( int i = 0; i < NumParticles(); ++i)
	{
		m_Particle[i].m_Position += m_Particle[i].m_Velocity * dt; 
		m_Particle[i].m_Velocity += m_Particle[i].m_Force * dt / m_Particle[i].m_Mass;

		assert( IsFinite( m_Particle[i].m_Velocity.x ) &&
			IsFinite( m_Particle[i].m_Velocity.y) &&
			IsFinite( m_Particle[i].m_Velocity.z) );

		// clamp for stability
		float lensq = m_Particle[i].m_Velocity.LengthSqr();
		if (lensq > 1e6)
		{
			m_Particle[i].m_Velocity *= 1e3 / sqrt(lensq);
		}
	}
	SatisfyCollisionConstraints();
}

//-----------------------------------------------------------------------------
// Update the shield position: 
//-----------------------------------------------------------------------------

void CSheetSimulator::Simulate( float dt )
{
	// Initialize positions if necessary
	EulerStep(dt);
}


void CSheetSimulator::Simulate( float dt, int steps )
{
	ComputeControlPoints();
	
	// Initialize positions if necessary
	dt /= steps;
	for (int i = 0; i < steps; ++i)
	{
		// Each step, we want to re-select the best collision planes to constrain
		// the movement by
		DetermineBestCollisionPlane();

		EulerStep(dt);
	}
}


#define CLAMP_DIST 6.0

void CSheetSimulator::ClampPointsToCollisionPlanes()
{
	// Find collision planes to clamp to
	DetermineBestCollisionPlane( false );

	// Eliminate velocity perp to a collision plane
	for ( int i = 0; i < NumParticles(); ++i )
	{
		// The actual collision plane 
		if (m_Particle[i].m_CollisionPlane >= 0)
		{
			cplane_t* pPlane = &m_pCollisionPlanes[m_Particle[i].m_CollisionPlane];

			// Make sure we have a close enough perpendicular distance to the plane...
			float flPerpDist = fabs ( DotProduct( m_Particle[i].m_Position, pPlane->normal ) - pPlane->dist );
			if (flPerpDist >= CLAMP_DIST)
				continue;

			// Drop it along the perp
			VectorMA( m_Particle[i].m_Position, -flPerpDist, pPlane->normal, m_Particle[i].m_Position );
		}
	}
}


//-----------------------------------------------------------------------------
// Class to help dealing with the iterative computation
//-----------------------------------------------------------------------------
CIterativeSheetSimulator::CIterativeSheetSimulator( TraceLineFunc_t traceline, TraceHullFunc_t traceHull ) :
	CSheetSimulator( traceline, traceHull ),
	m_SimulationSteps(0) 
{
}

void CIterativeSheetSimulator::BeginSimulation( float dt, int steps, int substeps, int collisionCount )
{
	m_CurrentCollisionPt = 0;
	m_TimeStep = dt;
	m_SimulationSteps = steps;
	m_TotalSteps = steps;
	m_SubSteps = substeps;
	m_InitialPass = true;
	m_CollisionCount = collisionCount;
}

bool CIterativeSheetSimulator::Think( )
{
	assert( m_SimulationSteps >= 0 );

	// Need to iteratively perform collision detection
	if (m_CurrentCollisionPt >= 0)
	{
		DetectCollisions();
		return false;
	}
	else
	{
		// Simulate it a bunch of times
		Simulate(m_TimeStep, m_SubSteps);

		// Reset the collision point for collision detect
		m_CurrentCollisionPt = 0;
		--m_SimulationSteps;

		if ( m_SimulationSteps == 0 )
		{
			ClampPointsToCollisionPlanes();
		}

		return true;
	}
}

// Iterative collision detection 
void CIterativeSheetSimulator::DetectCollisions( void )
{
	for ( int i = 0; i < m_CollisionCount; ++i )
	{
		if (m_InitialPass)
		{
			InitPosition( m_CurrentCollisionPt );
		}
		else
		{
			float flOffset = COLLISION_PLANE_OFFSET * ( (float)(m_SimulationSteps - 1) / (float)(m_TotalSteps - 1) );
			DetectCollision( m_CurrentCollisionPt, flOffset );
		}

		if (++m_CurrentCollisionPt >= NumParticles())
		{
			m_CurrentCollisionPt = -1;
			m_InitialPass = false;
			break;
		}
	}
}
