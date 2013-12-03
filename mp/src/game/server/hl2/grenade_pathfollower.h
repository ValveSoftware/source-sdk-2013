//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot by wasteland scanner 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEPATHFOLLOWER_H
#define	GRENADEPATHFOLLOWER_H

#include "basegrenade_shared.h"

class RocketTrail;

class CGrenadePathfollower : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadePathfollower, CBaseGrenade );

	static CGrenadePathfollower* CreateGrenadePathfollower( string_t sModelName, string_t sFlySound, const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner );

	CHandle<RocketTrail>	m_hRocketTrail;
	CBaseEntity*	m_pPathTarget;				// path corner we are heading towards
	float			m_flFlySpeed;
	string_t		m_sFlySound;
	float			m_flNextFlySoundTime;

	Class_T			Classify( void);
	void			Spawn( void );
	void			AimThink( void );
	void 			GrenadeTouch( CBaseEntity *pOther );
	void			Event_Killed( const CTakeDamageInfo &info );
	void			Launch( float flLaunchSpeed, string_t sPathCornerName);
	void			PlayFlySound(void);

	void EXPORT		Detonate(void);

	CGrenadePathfollower(void);
	~CGrenadePathfollower(void);

	virtual void Precache();

	DECLARE_DATADESC();
};

#endif	//GRENADEPATHFOLLOWER_H
