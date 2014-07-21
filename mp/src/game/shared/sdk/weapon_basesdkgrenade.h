//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_BASESDKGRENADE_H
#define WEAPON_BASESDKGRENADE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_sdkbase.h"


#ifdef CLIENT_DLL
	
	#define CBaseSDKGrenade C_BaseSDKGrenade

#endif


class CBaseSDKGrenade : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CBaseSDKGrenade, CWeaponSDKBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseSDKGrenade();

	virtual void	Precache();

	bool			Deploy();
	bool			Holster( CBaseCombatWeapon *pSwitchingTo );

	void			PrimaryAttack();
	void			SecondaryAttack();

	bool			Reload();

	virtual void	ItemPostFrame();
	
	void			DecrementAmmo( CBaseCombatCharacter *pOwner );
	virtual void	StartGrenadeThrow();
	virtual void	ThrowGrenade();
	virtual void	DropGrenade();

	bool IsPinPulled() const;

#ifndef CLIENT_DLL
	DECLARE_DATADESC();

	virtual bool AllowsAutoSwitchFrom( void ) const;

	int		CapabilitiesGet();
	
	// Each derived grenade class implements this.
	virtual void EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer );
#endif

protected:
	CNetworkVar( bool, m_bRedraw );	// Draw the weapon again after throwing a grenade
	CNetworkVar( bool, m_bPinPulled );	// Set to true when the pin has been pulled but the grenade hasn't been thrown yet.
	CNetworkVar( float, m_fThrowTime ); // the time at which the grenade will be thrown.  If this value is 0 then the time hasn't been set yet.

private:
	CBaseSDKGrenade( const CBaseSDKGrenade & ) {}
};


inline bool CBaseSDKGrenade::IsPinPulled() const
{
	return m_bPinPulled;
}


#endif // WEAPON_BASESDKGRENADE_H
