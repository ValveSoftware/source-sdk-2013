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


class CSmokeGrenadeProjectile : public CBaseCSGrenadeProjectile
{
public:
	DECLARE_CLASS( CSmokeGrenadeProjectile, CBaseCSGrenadeProjectile );
	DECLARE_DATADESC();

// Overrides.
public:
	
	virtual void Spawn();
	virtual void Precache();
	virtual void Detonate();
	virtual void BounceSound( void );

	void Think_Detonate();
	void Think_Fade();
	void Think_Remove();


// Grenade stuff.
public:

	static CSmokeGrenadeProjectile* Create( 
		const Vector &position, 
		const QAngle &angles, 
		const Vector &velocity, 
		const AngularImpulse &angVelocity, 
		CBaseCombatCharacter *pOwner );

	void SetTimer( float timer );

	EHANDLE m_hSmokeEffect;
	bool m_bDidSmokeEffect;
};


#endif // HEGRENADE_PROJECTILE_H
