//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#ifndef ENTITY_BONUSPACK_H
#define ENTITY_BONUSPACK_H

#ifdef _WIN32
#pragma once
#endif

#ifdef GAME_DLL
	#include "tf_powerup.h"
	#include "entity_currencypack.h"
#else
	#include "c_entity_currencypack.h"
#endif

#ifdef CLIENT_DLL
	#define CBonusPack C_BonusPack
#endif

DECLARE_AUTO_LIST( IBonusPackAutoList );

class CBonusPack
#ifdef GAME_DLL
	: public CCurrencyPack
#else
	: public C_BaseAnimating
#endif
	, public IBonusPackAutoList
{
public:
	DECLARE_DATADESC();
#ifdef GAME_DLL
	DECLARE_CLASS( CBonusPack, CCurrencyPack );
#else
	DECLARE_CLASS( CBonusPack, C_BaseAnimating );
#endif
	DECLARE_NETWORKCLASS();

	CBonusPack();

	virtual void	Spawn( void ) OVERRIDE;
	virtual void	Precache( void ) OVERRIDE;
#ifdef GAME_DLL
	virtual bool	AffectedByRadiusCollection() const OVERRIDE { return false; }
	virtual bool	MyTouch( CBasePlayer *pPlayer ) OVERRIDE;
	virtual bool	ValidTouch( CBasePlayer *pPlayer ) OVERRIDE;

	virtual const char *GetDefaultPowerupModel( void ) OVERRIDE
	{ 
		return "models/bots/bot_worker/bot_worker_powercore.mdl";
	}

private:

	void BlinkThink();
	
	int m_nBlinkCount;
	float m_flKillTime;
	float m_flCanPickupTime;
#endif
};

#endif // ENTITY_BONUSPACK_H


