//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENV_HEADCRABCANISTER_SHARED_H
#define ENV_HEADCRABCANISTER_SHARED_H

#ifdef _WIN32
#pragma once
#endif

#include "vstdlib/random.h"
#include "mathlib/vector.h"
#include "utlvector.h"
#include "networkvar.h"

//=============================================================================
//
// Shared HeadcrabCanister Class
//
class CEnvHeadcrabCanisterShared
{
	DECLARE_CLASS_NOBASE( CEnvHeadcrabCanisterShared );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_SIMPLE_DATADESC();

public:
	CEnvHeadcrabCanisterShared();

	// Initialization.
	void InitInWorld( float flLaunchTime, const Vector &vecStartPosition, const QAngle &vecStartAngles, const Vector &vecDirection, const Vector &vecImpactPosition, bool bLaunchedFromWithinWorld = false );
	void InitInSkybox( float flLaunchTime, const Vector &vecStartPosition, const QAngle &vecStartAngles, const Vector &vecDirection, const Vector &vecImpactPosition, const Vector &vecSkyboxOrigin, float flSkyboxScale );

	// Returns the position of the object at a given time.
	void GetPositionAtTime( float flTime, Vector &vecPosition, QAngle &vecAngles );

	// Returns whether or not the object is the the skybox
	bool IsInSkybox( );

	// Returns the time at which it enters the world
	float GetEnterWorldTime() const;

	// Convert from skybox to world
	void ConvertFromSkyboxToWorld();

	// Did we impact?
	bool DidImpact( float flTime ) const;

public:
	// The objects initial parametric conditions.
	CNetworkVector(	m_vecStartPosition );
	CNetworkVector(	m_vecEnterWorldPosition );
	CNetworkVector(	m_vecDirection );	
	CNetworkQAngle( m_vecStartAngles );

	CNetworkVar( float,	m_flFlightTime );
	CNetworkVar( float,	m_flFlightSpeed );
	CNetworkVar( float,	m_flLaunchTime );

	CNetworkVar( float, m_flInitialZSpeed );
	CNetworkVar( float, m_flZAcceleration );
	CNetworkVar( float, m_flHorizSpeed );

	CNetworkVar( bool, m_bLaunchedFromWithinWorld );

	CNetworkVector( m_vecParabolaDirection );

	// The time at which the canister enters the skybox
	CNetworkVar( float,	m_flWorldEnterTime );

	// Skybox data
	CNetworkVector(	m_vecSkyboxOrigin );
	CNetworkVar( float, m_flSkyboxScale );
	CNetworkVar( bool, m_bInSkybox );

private:
	float	m_flLaunchHeight;

	// Calculate the enter time. (called from Init)
	void CalcEnterTime( const Vector &vecTriggerMins, const Vector &vecTriggerMaxs );			

	friend class CEnvHeadcrabCanister;
	friend class C_EnvHeadcrabCanister;
};

/*
//=============================================================================
//
// HeadcrabCanister Factory Interface
//
abstract_class IHeadcrabCanisterFactory
{
public:

	virtual void CreateHeadcrabCanister( int nID, int iType, 
		                       const Vector &vecPosition, const Vector &vecDirection, 
		                       float flSpeed, float flStartTime, float flDamageRadius,
							   const Vector &vecTriggerMins, const Vector &vecTriggerMaxs ) = 0;
};

//=============================================================================
//
// Shared HeadcrabCanister Spawner Class
//
class CEnvHeadcrabCanisterSpawnerShared
{
public:
	DECLARE_CLASS_NOBASE( CEnvHeadcrabCanisterSpawnerShared );
	DECLARE_EMBEDDED_NETWORKVAR();

	//-------------------------------------------------------------------------
	// Initialization.
	//-------------------------------------------------------------------------
	CEnvHeadcrabCanisterSpawnerShared();
	void	Init( IHeadcrabCanisterFactory *pFactory, int nRandomSeed, float flTime,
				  const Vector &vecMinBounds, const Vector &vecMaxBounds,
				  const Vector &vecTriggerMins, const Vector &vecTriggerMaxs );

	//-------------------------------------------------------------------------
	// Method to generate HeadcrabCanisters. 
	// Time passed in here is global time, not delta time.
	// The function returns the time at which it must be called again.
	//-------------------------------------------------------------------------
	float	HeadcrabCanisterThink( float flTime );

	//-------------------------------------------------------------------------
	// Add HeadcrabCanister target data, used to determine HeadcrabCanister travel direction.
	//-------------------------------------------------------------------------
	void	AddToTargetList( const Vector &vecPosition, float flRadius );

	// Debugging!
	int		GetRandomInt( int nMin, int nMax );
	float	GetRandomFloat( float flMin, float flMax );

public:

	// Factory.
	IHeadcrabCanisterFactory					*m_pFactory;			// HeadcrabCanister creation factory.

	int								m_nHeadcrabCanisterCount;			// Number of HeadcrabCanisters created - used as IDs

	// Initial spawner data.
	CNetworkVar( float, m_flStartTime );			// Start time.
	CNetworkVar( int, m_nRandomSeed );			// The random number stream seed.

	CNetworkVar( int, m_iHeadcrabCanisterType );			// Type of HeadcrabCanister.
	float							m_flHeadcrabCanisterDamageRadius;	// HeadcrabCanister damage radius.
	CNetworkVar( bool, m_bSkybox );				// Is the spawner in the skybox?

	CNetworkVar( float, m_flMinSpawnTime );		// Spawn time - Min
	CNetworkVar( float, m_flMaxSpawnTime );		//              Max
	CNetworkVar( int, m_nMinSpawnCount );		// Number of HeadcrabCanisters to spawn - Min
	CNetworkVar( int, m_nMaxSpawnCount );		//							    Max
	CNetworkVector( m_vecMinBounds );			// Spawner volume (space) - Min
	CNetworkVector( m_vecMaxBounds );			//                          Max
	CNetworkVar( float, m_flMinSpeed );			// HeadcrabCanister speed - Min
	CNetworkVar( float, m_flMaxSpeed );			//                Max
	CNetworkVector( m_vecTriggerMins );		// World Bounds (Trigger) in 3D Skybox - Min
	CNetworkVector( m_vecTriggerMaxs );		//                                       Max
	Vector							m_vecTriggerCenter;

	// Generated data.
	int								m_nRandomCallCount;		// Debug! Keep track of number steam calls.
	float							m_flNextSpawnTime;		// Next HeadcrabCanister spawn time (random).
	CUniformRandomStream			m_NumberStream;			// Used to generate random numbers.	

	// Use "Targets" to determine HeadcrabCanister direction(s).
	struct HeadcrabCanistertarget_t
	{
		Vector		m_vecPosition;
		float		m_flRadius;
	};
	CUtlVector<HeadcrabCanistertarget_t>	m_aTargets;
};
*/

#endif // ENV_HEADCRAB_CANISTER_SHARED_H
