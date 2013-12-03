//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef STATS_H
#define STATS_H
#ifdef _WIN32
#pragma once
#endif

// internal counters to measure cost of physics simulation
struct physics_stats_t
{
	float	maxRescueSpeed;
	float	maxSpeedGain;
	
	int		impactSysNum;
	int		impactCounter;
	int		impactSumSys;
	int		impactHardRescueCount;
	int		impactRescueAfterCount;
	int		impactDelayedCount;
	int		impactCollisionChecks;
	int		impactStaticCount;

	double	totalEnergyDestroyed;
	int		collisionPairsTotal;
	int		collisionPairsCreated;
	int		collisionPairsDestroyed;

	int		potentialCollisionsObjectVsObject;
	int		potentialCollisionsObjectVsWorld;

	int		frictionEventsProcessed;
};


#endif // STATS_H
