//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef TF_POWERUP_H
#define TF_POWERUP_H

#ifdef _WIN32
#pragma once
#endif

#include "items.h"


#define TF_POWERUP_LIFETIME		30.0f		// normal powerup timeout


enum powerupsize_t
{
	POWERUP_SMALL,
	POWERUP_MEDIUM,
	POWERUP_FULL,

	POWERUP_SIZES,
};

extern float PackRatios[POWERUP_SIZES];

//=============================================================================
//
// CTF Powerup class.
//

class CTFPowerup : public CItem
{
public:
	DECLARE_CLASS( CTFPowerup, CItem );

	CTFPowerup();

	void			Spawn( void );
	CBaseEntity*	Respawn( void );
	virtual void	Precache();
	void			Materialize( void );
	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual bool	MyTouch( CBasePlayer *pPlayer );

	void			DropSingleInstance( Vector &vecLaunchVel, CBaseCombatCharacter *pThrower, float flThrowerTouchDelay, float flResetTime = 0.1f );

	bool			IsDisabled( void );
	void			SetDisabled( bool bDisabled );

	virtual float	GetRespawnDelay( void ) { return g_pGameRules->FlItemRespawnTime( this ); }

	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputToggle( inputdata_t &inputdata );

	virtual powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }

	virtual const char *GetPowerupModel( void );
	virtual const char *GetDefaultPowerupModel( void ) = 0;

	virtual bool	ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer );

	virtual float	GetLifeTime() { return TF_POWERUP_LIFETIME; }
protected:
	void			Materialize_Internal( void );

	bool			m_bDisabled;
	bool			m_bRespawning;
	bool			m_bThrownSingleInstance;
	bool			m_bAutoMaterialize;

	string_t		m_iszModel;

	float			m_flThrowerTouchTime;

	DECLARE_DATADESC();
};

#endif // TF_POWERUP_H


