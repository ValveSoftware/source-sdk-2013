//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef BASEPROJECTILE_H
#define BASEPROJECTILE_H
#ifdef _WIN32
#pragma once
#endif

// Creation.
struct baseprojectilecreate_t
{
	Vector vecOrigin;
	Vector vecVelocity;
	CBaseEntity *pOwner;
	string_t iszModel;
	float flDamage;
	int iDamageType;
	float flDamageScale;
};

//=============================================================================
//
// Generic projectile
//
class CBaseProjectile : public CBaseAnimating
{
	DECLARE_CLASS( CBaseProjectile, CBaseAnimating );
public:
	DECLARE_DATADESC();

	void	Spawn( void );
	void	Precache( void );

	static CBaseProjectile *Create( baseprojectilecreate_t &pCreate );

	void			SetDamage( float flDamage ) { m_flDamage = flDamage; }
	void			SetDamageScale( float &flScale ) { m_flDamageScale = flScale; }
	void			SetDamageType( int iType ) { m_iDamageType = iType; }

private:
	// Damage
	virtual float	GetDamage() { return m_flDamage; }
	virtual float	GetDamageScale( void ) { return m_flDamageScale; }
	virtual int		GetDamageType( void ) { return m_iDamageType; }

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	virtual void	ProjectileTouch( CBaseEntity *pOther );
	void			FlyThink( void );

protected:
	float			m_flDamage;
	int				m_iDamageType;
	float			m_flDamageScale;
};

#endif // BASEPROJECTILE_H
