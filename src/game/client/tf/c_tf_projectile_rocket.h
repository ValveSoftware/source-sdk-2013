//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_PROJECTILE_ROCKET_H
#define C_TF_PROJECTILE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

#define CTFProjectile_Rocket C_TFProjectile_Rocket

//-----------------------------------------------------------------------------
// Purpose: Rocket projectile.
//-----------------------------------------------------------------------------
class C_TFProjectile_Rocket : public C_TFBaseRocket
{
	DECLARE_CLASS( C_TFProjectile_Rocket, C_TFBaseRocket );
public:
	DECLARE_NETWORKCLASS();

	C_TFProjectile_Rocket();
	~C_TFProjectile_Rocket();

	virtual void	OnDataChanged(DataUpdateType_t updateType);

	virtual void	CreateTrails( void );
	virtual const char *GetTrailParticleName( void );
	bool			IsCritical() const { return m_bCritical; }

private:
	bool	m_bCritical;

	CNewParticleEffect	*pEffect;
};

#endif // C_TF_PROJECTILE_ROCKET_H
