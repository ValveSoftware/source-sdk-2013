//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "basehlcombatweapon.h"
#include "soundenvelope.h"

#ifndef WEAPON_FLAREGUN_H
#define WEAPON_FLAREGUN_H
#ifdef _WIN32
#pragma once
#endif

#define	SF_FLARE_NO_DLIGHT	0x00000001
#define	SF_FLARE_NO_SMOKE	0x00000002
#define	SF_FLARE_INFINITE	0x00000004
#define	SF_FLARE_START_OFF	0x00000008

#define	FLARE_DURATION		30.0f
#define FLARE_DECAY_TIME	10.0f
#define FLARE_BLIND_TIME	6.0f

//---------------------
// Flare
//---------------------

class CFlare : public CBaseCombatCharacter
{
public:
	DECLARE_CLASS( CFlare, CBaseCombatCharacter );

	CFlare();
	~CFlare();

	static CFlare *	GetActiveFlares( void );
	CFlare *		GetNextFlare( void ) const { return m_pNextFlare; }

	static CFlare *Create( Vector vecOrigin, QAngle vecAngles, CBaseEntity *pOwner, float lifetime );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	void	Spawn( void );
	void	Precache( void );
	int		Restore( IRestore &restore );
	void	Activate( void );

	void	StartBurnSound( void );

	void	Start( float lifeTime );
	void	Die( float fadeTime );
	void	Launch( const Vector &direction, float speed );

	Class_T Classify( void );

	void	FlareTouch( CBaseEntity *pOther );
	void	FlareBurnTouch( CBaseEntity *pOther );
	void	FlareThink( void );

	void	InputStart( inputdata_t &inputdata );
	void	InputDie( inputdata_t &inputdata );
	void	InputLaunch( inputdata_t &inputdata );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	static CFlare *activeFlares;

	CBaseEntity *m_pOwner;
	int			m_nBounces;			// how many times has this flare bounced?
	CNetworkVar( float, m_flTimeBurnOut );	// when will the flare burn out?
	CNetworkVar( float, m_flScale );
	float		m_flDuration;
	float		m_flNextDamage;
	
	CSoundPatch	*m_pBurnSound;
	bool		m_bFading;
	CNetworkVar( bool, m_bLight );
	CNetworkVar( bool, m_bSmoke );
	CNetworkVar( bool, m_bPropFlare );

	bool		m_bInActiveList;
	CFlare *	m_pNextFlare;

	void		RemoveFromActiveFlares( void );
	void		AddToActiveFlares( void );
};

//---------------------
// Flaregun
//---------------------
class CFlaregun:public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CFlaregun, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	void Precache( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
};

#endif // WEAPON_FLAREGUN_H

