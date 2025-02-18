//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_PROJECTILE_ENERGY_BALL_H
#define C_TF_PROJECTILE_ENERGY_BALL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

#define CTFProjectile_EnergyBall C_TFProjectile_EnergyBall

//-----------------------------------------------------------------------------
// Purpose: EnergyBall projectile.
//-----------------------------------------------------------------------------
class C_TFProjectile_EnergyBall : public C_TFBaseRocket
{
	DECLARE_CLASS( C_TFProjectile_EnergyBall, C_TFBaseRocket );

public:

	DECLARE_NETWORKCLASS();

	C_TFProjectile_EnergyBall();
	~C_TFProjectile_EnergyBall();

	virtual void	CreateTrails( void );
	virtual const char *GetTrailParticleName( void );

private:
	CNewParticleEffect	*pEffect;
	bool m_bChargedShot;

	Vector m_vColor1;
	Vector m_vColor2;
};

#endif // C_TF_PROJECTILE_ENERGY_BALL_H
