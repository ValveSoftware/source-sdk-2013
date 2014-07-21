//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot by mortar synth
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEENERGY_H
#define	GRENADEENERGY_H

#include "basegrenade_shared.h"

class CGrenadeEnergy : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeEnergy, CBaseGrenade );

	static void Shoot( CBaseEntity* pOwner, const Vector &vStart, Vector vVelocity );

public:
	void		Spawn( void );
	void		Precache( void );
	void		Animate( void );
	void 		GrenadeEnergyTouch( CBaseEntity *pOther );
	void		Event_Killed( const CTakeDamageInfo &info );

	int			m_flMaxFrame;
	int			m_nEnergySprite;
	float		m_flLaunchTime;		// When was this thing launched

	void EXPORT				Detonate(void);

	DECLARE_DATADESC();
};

#endif	//GRENADEENERGY_H
