//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_BASECSGRENADE_H
#define WEAPON_BASECSGRENADE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_csbase.h"


#ifdef CLIENT_DLL
	
	#define CBaseCSGrenade C_BaseCSGrenade

#endif


class CBaseCSGrenade : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CBaseCSGrenade, CWeaponCSBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseCSGrenade();

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
	bool IsBeingThrown() const { return m_fThrowTime > 0; }

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
	CBaseCSGrenade( const CBaseCSGrenade & ) {}
};


inline bool CBaseCSGrenade::IsPinPulled() const
{
	return m_bPinPulled;
}


#endif // WEAPON_BASECSGRENADE_H
