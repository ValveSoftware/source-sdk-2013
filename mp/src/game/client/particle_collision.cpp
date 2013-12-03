//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Local fast collision system for particles
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "particle_collision.h"
#include "engine/ivdebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef _XBOX
#define	__DEBUG_PARTICLE_COLLISION_RETEST			0
#else
#define	__DEBUG_PARTICLE_COLLISION_RETEST			1
#endif // _XBOX

#define	__DEBUG_PARTICLE_COLLISION_OVERLAY			0
#define	__DEBUG_PARTICLE_COLLISION_OVERLAY_LIFETIME	0.1f

#define	NUM_DISCREET_STEPS		8.0f
#define	NUM_SIMULATION_SECONDS	2.0f

#define	COLLISION_EPSILON		0.01f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseSimpleCollision::CBaseSimpleCollision( void )
{
	ClearActivePlanes();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			radius - 
//-----------------------------------------------------------------------------
void CBaseSimpleCollision::Setup( const Vector &origin, float speed, float gravity )
{
	TestForPlane( origin, Vector(  1,  0,  0 ), speed, gravity );
	TestForPlane( origin, Vector( -1,  0,  0 ), speed, gravity );
	TestForPlane( origin, Vector(  0,  1,  0 ), speed, gravity );
	TestForPlane( origin, Vector(  0, -1,  0 ), speed, gravity );
	TestForPlane( origin, Vector(  0,  0,  1 ), speed, gravity );
	TestForPlane( origin, Vector(  0,  0, -1 ), speed, gravity );
}

//-----------------------------------------------------------------------------
// Purpose: Trace line for super-simplified traces
// Input  : &start - start position
//			&end - end position
//			*pTrace - trace structure to fill
//			coarse - tests again with a real trace unless coarse is set
//-----------------------------------------------------------------------------
void CBaseSimpleCollision::TraceLine( const Vector &start, const Vector &end, trace_t *pTrace, bool coarse )
{
	//Iterate over all active planes
	for ( int i = 0; i < m_nActivePlanes; i++ )
	{
		//Must be a valid plane
		if ( m_collisionPlanes[i].m_Dist == -1.0f )
			continue;

		//Get our information about the relation to this plane
		float dot1 = m_collisionPlanes[i].DistTo(start);
		float dot2 = m_collisionPlanes[i].DistTo(end);

		//Don't consider particles on the backside of planes
		if ( dot1 < -COLLISION_EPSILON )
			continue;

		//Must be crossing the plane's boundary
		if ( ( dot1 > COLLISION_EPSILON ) == ( dot2 > COLLISION_EPSILON ) )
			continue;

		//Find the intersection point
		float	t = dot1 / (dot1 - dot2);
		Vector	vIntersection = start + (end - start) * t;

		//Fake the collision info
		pTrace->endpos			= vIntersection;
		pTrace->fraction		= t	- COLLISION_EPSILON;
		pTrace->plane.normal	= m_collisionPlanes[i].m_Normal;
		pTrace->plane.dist		= m_collisionPlanes[i].m_Dist;
		
		//If we need an exact trace, test again on a successful hit
		if ( ( coarse == false ) && ( pTrace->fraction < 1.0f ) )
		{
			UTIL_TraceLine( start, end, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, pTrace );
		}

		#if	__DEBUG_PARTICLE_COLLISION_OVERLAY
		debugoverlay->AddBoxOverlay( vIntersection, Vector(-1,-1,-1), Vector(1,1,1), QAngle(0,0,0), 0, 255, 0, 16, __DEBUG_PARTICLE_COLLISION_OVERLAY_LIFETIME );
		#endif	//__DEBUG_PARTICLE_COLLISION_OVERLAY
		
		//Done
		return;
	}

	//Fell through, so clear all the fields
	pTrace->plane.normal[0] = 0.0f;
	pTrace->plane.normal[1] = 0.0f;
	pTrace->plane.normal[2] = 0.0f;
	pTrace->plane.dist		= 0.0f;
	pTrace->fraction		= 1.0f;
	pTrace->allsolid		= false;
	pTrace->startsolid		= false;
	pTrace->m_pEnt			= NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Tests the planes against all others for validity
// Input  : *plane - plane to test
//-----------------------------------------------------------------------------
void CBaseSimpleCollision::ConsiderPlane( cplane_t *plane )
{
	//Test against all other active planes
	for ( int i = 0; i < m_nActivePlanes; i++ )
	{
		if ( m_collisionPlanes[i].m_Dist != -1.0f )
		{
			//Test for coplanar
			if ( ( m_collisionPlanes[i].m_Normal == plane->normal ) && ( m_collisionPlanes[i].m_Dist == plane->dist ) )
				return;
		}
	}

	//Don't overrun
	if ( m_nActivePlanes >= MAX_COLLISION_PLANES )
		return;

	//Take it
	m_collisionPlanes[m_nActivePlanes].m_Dist	= plane->dist;
	m_collisionPlanes[m_nActivePlanes].m_Normal	= plane->normal;
	m_nActivePlanes++;
}

//-----------------------------------------------------------------------------
// Purpose: Runs a simulation of the average particle's movement, looking for collisions along the way
// Input  : &start - start of the simulation
//			&dir - direction of travel
//			speed - speed of the particle
//			gravity - gravity being used
//-----------------------------------------------------------------------------
void CBaseSimpleCollision::TestForPlane( const Vector &start, const Vector &dir, float speed, float gravity )
{
	trace_t	tr;
	Vector	testStart, testEnd;

	testStart = start;

	//Setup our step increments
	float	dStepTime = (NUM_SIMULATION_SECONDS/NUM_DISCREET_STEPS);
	Vector	vStepIncr = dir * ( speed * dStepTime );
	float	flGravIncr = gravity*dStepTime;

	//Simulate collsions in discreet steps
	for ( int i = 1; i <= NUM_DISCREET_STEPS; i++ )
	{
		testEnd		= testStart + vStepIncr;
		testEnd[2] -= flGravIncr * (0.5f*(dStepTime*i)*(dStepTime*i) );

		//Trace the line
		UTIL_TraceLine( testStart, testEnd, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

		//See if we found one
		if ( tr.fraction != 1.0f )
		{
			#if	__DEBUG_PARTICLE_COLLISION_OVERLAY
			debugoverlay->AddLineOverlay( testStart, tr.endpos, 255, 0, 0, true, __DEBUG_PARTICLE_COLLISION_OVERLAY_LIFETIME );
			
			QAngle angles;

			VectorAngles( tr.plane.normal,angles );
			angles[PITCH] += 90;

			debugoverlay->AddBoxOverlay( tr.endpos, Vector(-64,-64,0), Vector(64,64,0), angles, 255, 0, 0, 16, __DEBUG_PARTICLE_COLLISION_OVERLAY_LIFETIME );
			#endif	//__DEBUG_PARTICLE_COLLISION_OVERLAY
			
			//Test the plane against a set of criteria
			ConsiderPlane( &tr.plane );

			return;
		}

		//We missed
		#if	__DEBUG_PARTICLE_COLLISION_OVERLAY
		debugoverlay->AddLineOverlay( testStart, tr.endpos, 0, 128.0f+(128.0f*((float)i/(float)NUM_DISCREET_STEPS)), 0, true, __DEBUG_PARTICLE_COLLISION_OVERLAY_LIFETIME );
		#endif	//__DEBUG_PARTICLE_COLLISION_OVERLAY
		
		//Save that position for the next round
		testStart = testEnd;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseSimpleCollision::ClearActivePlanes( void )
{ 
	for ( int i = 0; i < MAX_COLLISION_PLANES; i++ )
	{
		m_collisionPlanes[i].m_Dist	= -1.0f;
		m_collisionPlanes[i].m_Normal.Init();
	}

	m_nActivePlanes = 0; 
}

//
// CParticleCollision
//

//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
CParticleCollision::CParticleCollision( void )
{
	m_flGravity					= 800.0f;
	m_flCollisionDampen			= 0.5f;
	m_flAngularCollisionDampen	= 0.25f;

	ClearActivePlanes();
}

//-----------------------------------------------------------------------------
// Purpose: Test for surrounding collision surfaces for quick collision testing for the particle system
// Input  : &origin - starting position
//			*dir - direction of movement (if NULL, will do a point emission test in four directions)
//			angularSpread - looseness of the spread
//			minSpeed - minimum speed
//			maxSpeed - maximum speed
//			gravity - particle gravity for the sytem
//			dampen - dampening amount on collisions
//-----------------------------------------------------------------------------
void CParticleCollision::Setup( const Vector &origin, const Vector *dir, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen )
{
	//Take the information for this simulation
	m_flGravity				= gravity;
	m_flCollisionDampen		= dampen;
	m_nActivePlanes			= 0;

	//We take a rough estimation of the spray
	float	speedAvg	= (minSpeed+maxSpeed)*0.5f;

	//Point or directed?
	if ( dir == NULL )
	{
		//Test all around
		TestForPlane( origin, Vector(  1,  0,  0 ), speedAvg, gravity );
		TestForPlane( origin, Vector( -1,  0,  0 ), speedAvg, gravity );
		TestForPlane( origin, Vector(  0,  1,  0 ), speedAvg, gravity );
		TestForPlane( origin, Vector(  0, -1,  0 ), speedAvg, gravity );
		TestForPlane( origin, Vector(  0,  0,  1 ), speedAvg, gravity );
		TestForPlane( origin, Vector(  0,  0, -1 ), speedAvg, gravity );
	}
	else
	{
		Vector	vSkewDir, vRight;
		QAngle  vAngles;

		//FIXME: Quicker conversion?
		//FIXME: We need to factor in the angular spread instead
		VectorAngles( *dir, vAngles );
		AngleVectors( vAngles, NULL, &vRight, NULL );

		//Test straight
		TestForPlane( origin, *dir, speedAvg, gravity );

		vSkewDir = vRight;

		//Test right
		TestForPlane( origin, vSkewDir, speedAvg, gravity );

		vSkewDir *= -1.0f;

		//Test left
		TestForPlane( origin, vSkewDir, speedAvg, gravity );

		#if	__DEBUG_PARTICLE_COLLISION_OVERLAY
		DevMsg( 1, "CParticleCollision: Found %d active plane(s)\n", m_nActivePlanes );
		#endif	//__DEBUG_PARTICLE_COLLISION_OVERLAY
	}
}

//-----------------------------------------------------------------------------
// Purpose: Simulate movement, with collision
// Input  : &origin - position of the particle
//			&velocity - velocity of the particle
//			&rollDelta - roll delta of the particle
//			timeDelta - time step
//-----------------------------------------------------------------------------
bool CParticleCollision::MoveParticle( Vector &origin, Vector &velocity, float *rollDelta, float timeDelta, trace_t *pTrace )
{	
	//Don't bother with non-moving particles
	if ( velocity == vec3_origin )
		return false;

	//Factor in gravity
	velocity[2] -= m_flGravity * timeDelta;

	//Move
	Vector testPosition = ( origin + ( velocity * timeDelta ) );	

	//Only collide if we have active planes
	if ( m_nActivePlanes > 0 )
	{
		//Collide
		TraceLine( origin, testPosition, pTrace );

		//See if we hit something
		if ( pTrace->fraction != 1.0f )
		{
			#if	__DEBUG_PARTICLE_COLLISION_RETEST
			//Retest the collision with a true trace line to avoid errant collisions
			UTIL_TraceLine( origin, testPosition, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, pTrace );
			#endif	//__DEBUG_RETEST_COLLISION

			//Did we hit anything?
			if ( pTrace->fraction != 1.0f )
			{
				//See if we've settled
				if ( ( pTrace->plane.normal[2] >= 0.5f ) && ( fabs( velocity[2] ) <= 48.0f ) )
				{		
					//Leave the particle at the collision point
					origin += velocity * ( (pTrace->fraction-COLLISION_EPSILON) * timeDelta );
					
					//Stop the particle
					velocity	= vec3_origin;
					
					if ( rollDelta != NULL )
					{
						*rollDelta	= 0.0f;
					}
					
					return false;
				}
				else
				{
					//Move the particle to the collision point
					origin += velocity * ( (pTrace->fraction-COLLISION_EPSILON) * timeDelta );
					
					//Find the reflection vector
					float proj = velocity.Dot( pTrace->plane.normal );
					velocity += pTrace->plane.normal * (-proj*2.0f);
					
					//Apply dampening
					velocity *= random->RandomFloat( (m_flCollisionDampen-0.1f), (m_flCollisionDampen+0.1f) );

					//Dampen the roll of the particles
					if ( rollDelta != NULL )
					{
						(*rollDelta) *= -0.25f;
					}

					return true;
				}
			}
			else
			{
				#if	__DEBUG_PARTICLE_COLLISION_OVERLAY
				//Display a false hit
				debugoverlay->AddBoxOverlay( pTrace->endpos, Vector(-1,-1,-1), Vector(1,1,1), QAngle(0,0,0), 255, 0, 0, 16, __DEBUG_PARTICLE_COLLISION_OVERLAY_LIFETIME );
				#endif	//__DEBUG_PARTICLE_COLLISION_OVERLAY
			}
		}
	}

	//Simple move, no collision
	origin = testPosition;
	
	return false;
}
