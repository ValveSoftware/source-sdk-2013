//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef BASECSGRENADE_PROJECTILE_H
#define BASECSGRENADE_PROJECTILE_H
#ifdef _WIN32
#pragma once
#endif


#include "basegrenade_shared.h"


#ifdef CLIENT_DLL
	#define CBaseCSGrenadeProjectile C_BaseCSGrenadeProjectile
#endif


class CBaseCSGrenadeProjectile : public CBaseGrenade
{
public:
	DECLARE_CLASS( CBaseCSGrenadeProjectile, CBaseGrenade );
	DECLARE_NETWORKCLASS(); 

	virtual void Spawn();


public:

	// This gets sent to the client and placed in the client's interpolation history
	// so the projectile starts out moving right off the bat.
	CNetworkVector( m_vInitialVelocity );


#ifdef CLIENT_DLL
	CBaseCSGrenadeProjectile() {}
	CBaseCSGrenadeProjectile( const CBaseCSGrenadeProjectile& ) {}
	virtual int DrawModel( int flags );
	virtual void PostDataUpdate( DataUpdateType_t type );
	
	float m_flSpawnTime;
#else
	DECLARE_DATADESC();

	virtual void PostConstructor( const char *className );
	virtual ~CBaseCSGrenadeProjectile();

	//Constants for all CS Grenades
	static inline float GetGrenadeGravity() { return 0.4f; }
	static inline const float GetGrenadeFriction() { return 0.2f; }
	static inline const float GetGrenadeElasticity() { return 0.45f; }

	//Think function to emit danger sounds for the AI
	void DangerSoundThink( void );
	
	virtual float GetShakeAmplitude( void ) { return 0.0f; }
	virtual void Splash();

	// Specify what velocity we want the grenade to have on the client immediately.
	// Without this, the entity wouldn't have an interpolation history initially, so it would
	// sit still until it had gotten a few updates from the server.
	void SetupInitialTransmittedGrenadeVelocity( const Vector &velocity );

protected:

	//Set the time to detonate ( now + timer )
	void SetDetonateTimerLength( float timer );

private:	
	
	//Custom collision to allow for constant elasticity on hit surfaces
	virtual void ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );
	
	float m_flDetonateTime;
#endif
};


#endif // BASECSGRENADE_PROJECTILE_H
