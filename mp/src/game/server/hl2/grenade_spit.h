//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot by bullsquid 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADESPIT_H
#define	GRENADESPIT_H

#include "basegrenade_shared.h"

class CParticleSystem;

enum SpitSize_e
{
	SPIT_SMALL,
	SPIT_MEDIUM,
	SPIT_LARGE,
};

#define SPIT_GRAVITY 600

class CGrenadeSpit : public CBaseGrenade
{
	DECLARE_CLASS( CGrenadeSpit, CBaseGrenade );

public:
						CGrenadeSpit( void );

	virtual void		Spawn( void );
	virtual void		Precache( void );
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	virtual	unsigned int	PhysicsSolidMaskForEntity( void ) const { return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_WATER ); }

	void 				GrenadeSpitTouch( CBaseEntity *pOther );
	void				SetSpitSize( int nSize );
	void				Detonate( void );
	void				Think( void );

private:
	DECLARE_DATADESC();
	
	void	InitHissSound( void );
	
	CHandle< CParticleSystem >	m_hSpitEffect;
	CSoundPatch		*m_pHissSound;
	bool			m_bPlaySound;
};

#endif	//GRENADESPIT_H
