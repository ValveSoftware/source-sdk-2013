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

// for constraints
#include "vphysics/constraints.h"

class CTripmineGrenade : public CBaseGrenade
{
public:
	DECLARE_CLASS( CTripmineGrenade, CBaseGrenade );

	CTripmineGrenade();
	~CTripmineGrenade();

	void Spawn( void );
	void Precache( void );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	void WarningThink( void );
	void PowerupThink( void );
	void BeamBreakThink( void );
	void DelayDeathThink( void );
	void Event_Killed( const CTakeDamageInfo &info );

	void MakeBeam( void );
	void KillBeam( void );

	// Added to create a constraint
	void AttachToEntity( CBaseEntity *pOther );
	bool MakeConstraint( CBaseEntity *pOther );

public:
	EHANDLE		m_hOwner;
	// Added for following
	EHANDLE		m_hAttachEntity;

private:
	float		m_flPowerUp;
	Vector		m_vecDir;
	Vector		m_vecEnd;
	float		m_flBeamLength;

	CBeam		*m_pBeam;
	Vector		m_posOwner;
	Vector		m_angleOwner;

	// signifies if we're attached to something, and need to update slightly differently.
	bool		m_bAttached;
	IPhysicsConstraint	*m_pConstraint;
	Vector		m_vAttachedPosition;	// if the attached position changes, we need to detonate

	DECLARE_DATADESC();
};

#endif // GRENADE_TRIPMINE_H
