//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_TRIPMINE_H
#define GRENADE_TRIPMINE_H
#ifdef _WIN32
#pragma once
#endif

#include "basegrenade_shared.h"

class CBeam;

#ifdef MAPBASE
#define SF_TRIPMINE_START_INACTIVE (1 << 0)
#endif

class CTripmineGrenade : public CBaseGrenade
{
public:
	DECLARE_CLASS( CTripmineGrenade, CBaseGrenade );

	CTripmineGrenade();
	void Spawn( void );
	void Precache( void );

#if 0 // FIXME: OnTakeDamage_Alive() is no longer called now that base grenade derives from CBaseAnimating
	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
#endif	
	void WarningThink( void );
	void PowerupThink( void );
	void BeamBreakThink( void );
	void DelayDeathThink( void );
	void Event_Killed( const CTakeDamageInfo &info );

	void MakeBeam( void );
	void KillBeam( void );

#ifdef MAPBASE
	void PowerUp();

	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );
	void InputSetOwner( inputdata_t &inputdata ) { m_hOwner = inputdata.value.Entity(); }

	COutputEvent m_OnExplode;
#endif

public:
	EHANDLE		m_hOwner;

#ifdef MAPBASE
	float		m_flPowerUpTime;
	EHANDLE		m_hAttacker;
#endif

private:
	float		m_flPowerUp;
	Vector		m_vecDir;
	Vector		m_vecEnd;
	float		m_flBeamLength;

	CBeam		*m_pBeam;
	Vector		m_posOwner;
	Vector		m_angleOwner;

	DECLARE_DATADESC();
};

#endif // GRENADE_TRIPMINE_H
