//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_PROJECTILE_ARROW_H
#define C_TF_PROJECTILE_ARROW_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"

#define CTFProjectile_Arrow C_TFProjectile_Arrow

//-----------------------------------------------------------------------------
// Purpose: Arrow projectile.
//-----------------------------------------------------------------------------
class C_TFProjectile_Arrow : public C_TFBaseRocket
{
	DECLARE_CLASS( C_TFProjectile_Arrow, C_TFBaseRocket );

public:

	DECLARE_NETWORKCLASS();

	C_TFProjectile_Arrow();
	~C_TFProjectile_Arrow();
	
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	NotifyBoneAttached( C_BaseAnimating* attachTarget );
	virtual void	ClientThink( void );

	void			CheckNearMiss( void );

	void			CreateCritTrail( void );

	void			SetLifeTime( float flLifetime ) { m_flLifeTime = flLifetime; }

private:

	float			m_fAttachTime;
	float			m_nextNearMissCheck;
	bool			m_bNearMiss;
	bool			m_bArrowAlight;
	bool			m_bCritical;
	CNewParticleEffect	*m_pCritEffect;
	int				m_iCachedDeflect;

	float			m_flLifeTime;

	CNetworkVar( int, m_iProjectileType );
};

//-----------------------------------------------------------------------------
// Purpose: Arrow projectile.
//-----------------------------------------------------------------------------
class C_TFProjectile_HealingBolt : public C_TFProjectile_Arrow
{
	DECLARE_CLASS( C_TFProjectile_HealingBolt, C_TFProjectile_Arrow );

	virtual void	OnDataChanged( DataUpdateType_t updateType );

public:

	DECLARE_NETWORKCLASS();
};

//-----------------------------------------------------------------------------
// Purpose: Grappling Hook projectile.
//-----------------------------------------------------------------------------
class C_TFProjectile_GrapplingHook : public C_TFProjectile_Arrow
{
	DECLARE_CLASS( C_TFProjectile_GrapplingHook, C_TFProjectile_Arrow );
public:
	DECLARE_NETWORKCLASS();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	UpdateOnRemove();
	virtual void	ClientThink();
private:
	void UpdateRope();
	void RemoveRope();

	CHandle< C_RopeKeyframe > m_hRope;
};

#endif // C_TF_PROJECTILE_ARROW_H
