//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HEGRENADE_PROJECTILE_H
#define HEGRENADE_PROJECTILE_H
#ifdef _WIN32
#pragma once
#endif

#include "momentum/basecsgrenade_projectile.h"

class CHEGrenadeProjectile : public CBaseCSGrenadeProjectile
{
public:
	DECLARE_CLASS( CHEGrenadeProjectile, CBaseCSGrenadeProjectile );


// Overrides.
public:
	virtual void Spawn();
	virtual void Precache();
	virtual void BounceSound( void );
	virtual void Detonate();

// Grenade stuff.
public:

	static CHEGrenadeProjectile* Create( 
		const Vector &position, 
		const QAngle &angles, 
		const Vector &velocity, 
		const AngularImpulse &angVelocity, 
		CBaseCombatCharacter *pOwner, 
		float timer );

	void SetTimer( float timer );

private:
	float m_flDetonateTime;
};


#endif // HEGRENADE_PROJECTILE_H
