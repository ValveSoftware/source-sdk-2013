//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_SMG_H
#define TF_WEAPON_SMG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFSMG C_TFSMG
#define CTFChargedSMG C_TFChargedSMG
#endif

//=============================================================================
//
// TF Weapon Sub-machine gun.
//
class CTFSMG : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSMG, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFSMG() {}
	~CTFSMG() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SMG; }

	virtual int		GetDamageType( void ) const;
	virtual bool	CanFireCriticalShot( bool bIsHeadshot, CBaseEntity *pTarget = NULL ) OVERRIDE;

	bool			CanHeadshot( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return (iMode == 1); };

private:

	CTFSMG( const CTFSMG & ) {}
};

//=============================================================================
//
// TF Weapon Charged Sub-machine gun.
//
class CTFChargedSMG : public CTFSMG
{
public:
	DECLARE_CLASS( CTFChargedSMG, CTFSMG );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFChargedSMG() {}
	~CTFChargedSMG() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_CHARGED_SMG; }

	const char*		GetEffectLabelText( void ) { return "#TF_SmgCharge"; }
	float			GetProgress( void );
	bool			ShouldFlashChargeBar();
	void			SecondaryAttack() OVERRIDE;
	bool			CanPerformSecondaryAttack() const OVERRIDE;
	void			WeaponReset() OVERRIDE;

#ifdef GAME_DLL
	void	ApplyOnHitAttributes( CBaseEntity *pVictimBaseEntity, CTFPlayer *pAttacker, const CTakeDamageInfo &info ) OVERRIDE;
#endif

protected:
	CNetworkVar( float, m_flMinicritCharge );

	float m_flMinicritStartTime;

private:
	CTFChargedSMG( const CTFChargedSMG & ) {}
};


#endif // TF_WEAPON_SMG_H
