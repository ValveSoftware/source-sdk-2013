//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_PROJECTILE_FLARE_H
#define C_TF_PROJECTILE_FLARE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

#define CTFProjectile_Flare C_TFProjectile_Flare

//-----------------------------------------------------------------------------
// Purpose: Flare projectile.
//-----------------------------------------------------------------------------
class C_TFProjectile_Flare : public C_TFBaseRocket
{
	DECLARE_CLASS( C_TFProjectile_Flare, C_TFBaseRocket );

public:

	DECLARE_NETWORKCLASS();

	C_TFProjectile_Flare();
	~C_TFProjectile_Flare();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	
	void			CreateTrails( void );

private:

	bool	m_bCritical;
	CNewParticleEffect	*pEffect;
};

#endif // C_TF_PROJECTILE_FLARE_H
