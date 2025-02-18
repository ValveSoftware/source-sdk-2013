//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Base Grenade.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_GRENADE_H
#define TF_WEAPONBASE_GRENADE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "npcevent.h"

#ifdef CLIENT_DLL
#define CTFWeaponBaseGrenade C_TFWeaponBaseGrenade
#endif

//=============================================================================//
//
// TF Base Grenade
//
class CTFWeaponBaseGrenade : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseGrenade, CTFWeaponBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFWeaponBaseGrenade();

	virtual void			Spawn();
	virtual void			Precache();

	bool					Deploy( void );
	bool					IsPrimed( void );
	void					Prime( void );

	void					Throw( void );

	bool					ShouldDetonate( void );

	virtual void			ItemPostFrame();

	virtual void			HideThink( void )	{ SetWeaponVisible( false ); }

	bool					ShouldLowerMainWeapon( void );

// Client specific.
#ifdef CLIENT_DLL

	bool					ShouldDraw( void );
// Server specific.
#else

	DECLARE_DATADESC();

	virtual void			HandleAnimEvent( animevent_t *pEvent );

	// Each derived grenade class implements this.
	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

protected:

	CNetworkVar( bool, m_bPrimed );			// Set to true when the pin has been pulled but the grenade hasn't been thrown yet.
	CNetworkVar( float, m_flThrowTime );	// the time at which the grenade will be thrown.  If this value is 0 then the time hasn't been set yet.
	CNetworkVar( bool, m_bThrow );			// True when the player is throwing the grenade

private:

	CTFWeaponBaseGrenade( const CTFWeaponBaseGrenade & ) {}
};

#endif // TF_WEAPONBASE_GRENADE_H
