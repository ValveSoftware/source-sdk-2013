//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF CurrencyPack.
//
//=============================================================================//
#ifndef ENTITY_CURRENCYPACK_H
#define ENTITY_CURRENCYPACK_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
#include "player.h"
#include "tf_shareddefs.h"


//=============================================================================
//
// CTF CurrencyPack class.
//

DECLARE_AUTO_LIST( ICurrencyPackAutoList );

class CCurrencyPack : public CTFPowerup, public ICurrencyPackAutoList
{
public:
	DECLARE_CLASS( CCurrencyPack, CTFPowerup );
	DECLARE_SERVERCLASS();

	CCurrencyPack();
	~CCurrencyPack();

	void	Spawn( void );
	void	Precache( void );
	void	UpdateOnRemove( void );
	virtual int UpdateTransmitState() OVERRIDE;
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;
	bool	MyTouch( CBasePlayer *pPlayer );
	virtual bool AffectedByRadiusCollection() const { return true; }

	void	SetAmount( float flAmount );
	void	SetClaimed( void ) { m_bClaimed = true; }	// Radius collection code "steers" packs toward the player
	bool	IsClaimed( void ) { return m_bClaimed; }	// So don't allow other players to interfere
	void	DistributedBy( CBasePlayer* pMoneyMaker );
	
	virtual CurrencyRewards_t	GetPackSize( void ) { return TF_CURRENCY_PACK_LARGE; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_large.mdl"; }

protected:
	virtual void ComeToRest( void );

	void BlinkThink( void );

	int		m_nAmount;
	uint32  m_nWaveNumber;
	int m_blinkCount;
	CountdownTimer m_blinkTimer;
	bool	m_bTouched;
	bool	m_bClaimed;
	CNetworkVar( bool, m_bDistributed );
};

class CCurrencyPackMedium : public CCurrencyPack
{
public:
	DECLARE_CLASS( CCurrencyPackMedium, CCurrencyPack );

	virtual CurrencyRewards_t	GetPackSize( void ) { return TF_CURRENCY_PACK_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_medium.mdl"; }
};

class CCurrencyPackSmall : public CCurrencyPack
{
public:
	DECLARE_CLASS( CCurrencyPackSmall, CCurrencyPack );

	virtual CurrencyRewards_t	GetPackSize( void ) { return TF_CURRENCY_PACK_SMALL; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_small.mdl"; }
};

class CCurrencyPackCustom : public CCurrencyPack
{
public:
	DECLARE_CLASS( CCurrencyPackCustom, CCurrencyPack );

	virtual CurrencyRewards_t	GetPackSize( void ) { return TF_CURRENCY_PACK_CUSTOM; }
	virtual const char *GetDefaultPowerupModel( void );
};


#endif // ENTITY_CURRENCYPACK_H


