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


class CFlashbangProjectile : public CBaseCSGrenadeProjectile
{
public:
	DECLARE_CLASS( CFlashbangProjectile, CBaseCSGrenadeProjectile );

// Overrides.
public:
	virtual void Spawn();
	virtual void Precache();
	virtual void BounceSound( void );
	virtual void Detonate();

// Grenade stuff.
	static CFlashbangProjectile* Create( 
		const Vector &position, 
		const QAngle &angles, 
		const Vector &velocity, 
		const AngularImpulse &angVelocity, 
		CBaseCombatCharacter *pOwner );	
};


#endif // HEGRENADE_PROJECTILE_H
