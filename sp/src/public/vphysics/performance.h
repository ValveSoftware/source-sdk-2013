//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PERFORMANCE_H
#define PERFORMANCE_H
#ifdef _WIN32
#pragma once
#endif

// Don't ever change these values, or face all kinds of subtle gameplay changes
const float k_flMaxVelocity = 2000.0f;
const float k_flMaxAngularVelocity = 360.0f * 10.0f;

const float DEFAULT_MIN_FRICTION_MASS = 10.0f;
const float DEFAULT_MAX_FRICTION_MASS = 2500.0f;
struct physics_performanceparams_t
{
	int		maxCollisionsPerObjectPerTimestep;		// object will be frozen after this many collisions (visual hitching vs. CPU cost)
	int		maxCollisionChecksPerTimestep;			// objects may penetrate after this many collision checks (can be extended in AdditionalCollisionChecksThisTick)
	float	maxVelocity;							// limit world space linear velocity to this (in / s)
	float	maxAngularVelocity;						// limit world space angular velocity to this (degrees / s)
	float	lookAheadTimeObjectsVsWorld;			// predict collisions this far (seconds) into the future
	float	lookAheadTimeObjectsVsObject;			// predict collisions this far (seconds) into the future
	float	minFrictionMass;						// min mass for friction solves (constrains dynamic range of mass to improve stability)
	float	maxFrictionMass;						// mas mass for friction solves

	void Defaults()
	{
		maxCollisionsPerObjectPerTimestep = 6;
		maxCollisionChecksPerTimestep = 250;
		maxVelocity = k_flMaxVelocity;
		maxAngularVelocity = k_flMaxAngularVelocity;
		lookAheadTimeObjectsVsWorld = 1.0f;
		lookAheadTimeObjectsVsObject = 0.5f;
		minFrictionMass = DEFAULT_MIN_FRICTION_MASS;
		maxFrictionMass = DEFAULT_MAX_FRICTION_MASS;
	}
};


#endif // PERFORMANCE_H
