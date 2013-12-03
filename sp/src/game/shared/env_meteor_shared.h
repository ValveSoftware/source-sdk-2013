//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENV_METEOR_SHARED_H
#define ENV_METEOR_SHARED_H
#pragma once

#include "vstdlib/random.h"
#include "mathlib/vector.h"
#include "utlvector.h"

//=============================================================================
//
// Shared Meteor Class
//
#define METEOR_INVALID_TIME		-9999.9f
#define METEOR_PASSIVE_TIME		0.0f
#define METEOR_MAX_LIFETIME		60.0f
#define METEOR_MIN_SIZE			Vector( -100, -100, -100 )
#define METEOR_MAX_SIZE			Vector(  100,  100,  100 )

#define METEOR_LOCATION_INVALID		-1
#define METEOR_LOCATION_WORLD		0
#define METEOR_LOCATION_SKYBOX		1

class CEnvMeteorShared
{
public:

	//-------------------------------------------------------------------------
	// Initialization.
	//-------------------------------------------------------------------------
	CEnvMeteorShared();
	void Init( int nID, float flStartTime, float flPassiveTime,
		       const Vector &vecStartPosition, 
		       const Vector &vecDirection, float flSpeed, float flDamageRadius,
			   const Vector &vecTriggerMins, const Vector &vecTriggerMaxs );

	//-------------------------------------------------------------------------
	// Returns the position of the object at a given time.
	//-------------------------------------------------------------------------
	void GetPositionAtTime( float flTime, Vector &vecPosition );

	//-------------------------------------------------------------------------
	// Changes an objects paramters from "skybox space" to "world space."
	//-------------------------------------------------------------------------
	void ConvertFromSkyboxToWorld( void );

	//-------------------------------------------------------------------------
	// Changes an objects paramters from "world space" to "skybox space."
	//-------------------------------------------------------------------------
	void ConvertFromWorldToSkybox( void );

	//-------------------------------------------------------------------------
	// Returns whether or not the object is the the skybox given the time.
	//-------------------------------------------------------------------------
	bool IsInSkybox( float flTime );

	//-------------------------------------------------------------------------
	// Returns whether or not the object is moving in the skybox (or passive).
	//-------------------------------------------------------------------------
	bool IsPassive( float flTime );

	//-------------------------------------------------------------------------
	// Returns whether or not the object will ever transition from skybox to world.
	//-------------------------------------------------------------------------
	bool WillTransition( void );

	//-------------------------------------------------------------------------
	// Returns the splash damage radius of the object.
	//-------------------------------------------------------------------------
	float GetDamageRadius( void );

public:

	int		m_nID;					// unique identifier

	// The objects initial parametric conditions.
	Vector	m_vecStartPosition;
	Vector	m_vecDirection;			
	float	m_flSpeed;				// (units/sec), unit = 1 inch
	float	m_flStartTime;

	// NOTE: All times are absolute - ie m_flStartTime has been added in.

	// The time after the starting time in which it object starts to "move."
	float	m_flPassiveTime;

	// The enter and exit times define the times at which the object enters and
	// exits the world.  In other words, m_flEnterTime is the time at which the
	// object leaves the skybox and enters the world. m_flExitTime is the opposite.
	float	m_flWorldEnterTime;
	float	m_flWorldExitTime;

	float	m_flPosTime;				// Timer used to find the position of the meteor.
	Vector	m_vecPos;

	// 
	int		m_nLocation;				// 0 = Skybox, 1 = World

	float	m_flDamageRadius;			// 

private:

	// Calculate the enter/exit times. (called from Init)
	void CalcEnterAndExitTimes( const Vector &vecTriggerMins, const Vector &vecTriggerMaxs );			
};

//=============================================================================
//
// Meteor Factory Interface
//
abstract_class IMeteorFactory
{
public:

	virtual void CreateMeteor( int nID, int iType, 
		                       const Vector &vecPosition, const Vector &vecDirection, 
		                       float flSpeed, float flStartTime, float flDamageRadius,
							   const Vector &vecTriggerMins, const Vector &vecTriggerMaxs ) = 0;
};

//=============================================================================
//
// Shared Meteor Spawner Class
//
class CEnvMeteorSpawnerShared
{
public:
	DECLARE_CLASS_NOBASE( CEnvMeteorSpawnerShared );
	DECLARE_EMBEDDED_NETWORKVAR();

	//-------------------------------------------------------------------------
	// Initialization.
	//-------------------------------------------------------------------------
	CEnvMeteorSpawnerShared();
	void	Init( IMeteorFactory *pFactory, int nRandomSeed, float flTime,
				  const Vector &vecMinBounds, const Vector &vecMaxBounds,
				  const Vector &vecTriggerMins, const Vector &vecTriggerMaxs );

	//-------------------------------------------------------------------------
	// Method to generate meteors. 
	// Time passed in here is global time, not delta time.
	// The function returns the time at which it must be called again.
	//-------------------------------------------------------------------------
	float	MeteorThink( float flTime );

	//-------------------------------------------------------------------------
	// Add meteor target data, used to determine meteor travel direction.
	//-------------------------------------------------------------------------
	void	AddToTargetList( const Vector &vecPosition, float flRadius );

	// Debugging!
	int		GetRandomInt( int nMin, int nMax );
	float	GetRandomFloat( float flMin, float flMax );

public:

	// Factory.
	IMeteorFactory					*m_pFactory;			// Meteor creation factory.

	int								m_nMeteorCount;			// Number of meteors created - used as IDs

	// Initial spawner data.
	CNetworkVar( float, m_flStartTime );			// Start time.
	CNetworkVar( int, m_nRandomSeed );			// The random number stream seed.

	CNetworkVar( int, m_iMeteorType );			// Type of meteor.
	float							m_flMeteorDamageRadius;	// Meteor damage radius.
	CNetworkVar( bool, m_bSkybox );				// Is the spawner in the skybox?

	CNetworkVar( float, m_flMinSpawnTime );		// Spawn time - Min
	CNetworkVar( float, m_flMaxSpawnTime );		//              Max
	CNetworkVar( int, m_nMinSpawnCount );		// Number of meteors to spawn - Min
	CNetworkVar( int, m_nMaxSpawnCount );		//							    Max
	CNetworkVector( m_vecMinBounds );			// Spawner volume (space) - Min
	CNetworkVector( m_vecMaxBounds );			//                          Max
	CNetworkVar( float, m_flMinSpeed );			// Meteor speed - Min
	CNetworkVar( float, m_flMaxSpeed );			//                Max
	CNetworkVector( m_vecTriggerMins );		// World Bounds (Trigger) in 3D Skybox - Min
	CNetworkVector( m_vecTriggerMaxs );		//                                       Max
	Vector							m_vecTriggerCenter;

	// Generated data.
	int								m_nRandomCallCount;		// Debug! Keep track of number steam calls.
	float							m_flNextSpawnTime;		// Next meteor spawn time (random).
	CUniformRandomStream			m_NumberStream;			// Used to generate random numbers.	

	// Use "Targets" to determine meteor direction(s).
	struct meteortarget_t
	{
		Vector		m_vecPosition;
		float		m_flRadius;
	};
	CUtlVector<meteortarget_t>	m_aTargets;
};

#endif // ENV_METEOR_SHARED_H
