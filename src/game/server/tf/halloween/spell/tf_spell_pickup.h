//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Spell.
//
//=============================================================================//
#ifndef TF_SPELL_PICKUP_H
#define TF_SPELL_PICKUP_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
class CSpellPickup : public CTFPowerup
{
	DECLARE_CLASS( CSpellPickup, CTFPowerup )
	DECLARE_DATADESC();

public:
	CSpellPickup();

	virtual void	Spawn( void ) OVERRIDE;
	virtual void	Precache() OVERRIDE;

	virtual bool	MyTouch( CBasePlayer *pPlayer ) OVERRIDE;
	virtual const char *GetPowerupModel( void ) OVERRIDE;
	virtual const char *GetDefaultPowerupModel( void ) OVERRIDE { return "models/props_halloween/hwn_spellbook_upright.mdl"; }
	virtual bool	ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer ) OVERRIDE;

	void SetTier( int nTier ) { m_nTier = nTier; }

private:
	
	int m_nTier;
};

#endif // TF_SPELL_PICKUP_H
