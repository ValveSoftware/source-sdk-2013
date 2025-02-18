//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon Knife Class
//
//=============================================================================
#ifndef TF_WEAPON_KNIFE_H
#define TF_WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFKnife C_TFKnife
#endif

// Knives use the "set_weapon_mode" attribute to define which type of knife they are
// Keep this enum in sync with the values used for set_weapon_mode.
enum knife_weapontypes_t
{
	KNIFE_STANDARD = 0,
	KNIFE_DISGUISE_ONKILL = 1,
	KNIFE_MOVEMENT_CLOAK = 2,		// The Cloak and Dagger
	KNIFE_ICICLE = 3,
};

//=============================================================================
//
// Knife class.
//
class CTFKnife : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFKnife, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFKnife();
	virtual void		PrimaryAttack( void ) OVERRIDE;
	virtual int			GetWeaponID( void ) const OVERRIDE			{ return TF_WEAPON_KNIFE; }
	int					GetKnifeType( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; };

	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage ) OVERRIDE;

	virtual void		SendPlayerAnimEvent( CTFPlayer *pPlayer ) OVERRIDE;

	virtual bool		CanDeploy( void ) OVERRIDE;
	virtual bool		Deploy( void ) OVERRIDE;
	void				BackstabVMThink( void );

	bool				SendWeaponAnim( int iActivity );

	bool				CanPerformBackstabAgainstTarget( CTFPlayer *pTarget );		// "backstab" sometimes means "frontstab"
	bool				IsBehindAndFacingTarget( CTFPlayer *pTarget );
	bool				IsBackstab( void ) { return (m_hBackstabVictim.Get() != NULL); }
	void				BackstabBlocked( void );
	bool				ShouldDisguiseOnBackstab( void );
	void				DisguiseOnKill();
	void				ProcessDisguiseImpulse();

	virtual bool		CanHolster( void ) const OVERRIDE;

	virtual void		ItemPostFrame( void ) OVERRIDE;
	virtual void		ItemPreFrame( void ) OVERRIDE;
	virtual void		ItemBusyFrame( void ) OVERRIDE;
	virtual void		ItemHolsterFrame( void ) OVERRIDE;

	virtual bool		CalcIsAttackCriticalHelper( void ) OVERRIDE;
	virtual bool		CalcIsAttackCriticalHelperNoCrits( void ) OVERRIDE;
	virtual bool		DoSwingTrace( trace_t &trace ) OVERRIDE;

	virtual void		WeaponRegenerate( void ) OVERRIDE;
	virtual void		WeaponReset( void ) OVERRIDE; 

#ifdef GAME_DLL
	virtual void		ApplyOnInjuredAttributes( CTFPlayer *pVictim, CTFPlayer *pAttacker, const CTakeDamageInfo &info ) OVERRIDE;	// when owner of this weapon is hit
	bool				DecreaseRegenerationTime( float value, bool bForce );
#endif

	float				GetProgress( void );
	const char*			GetEffectLabelText( void ) { return "#TF_KNIFE"; }
	virtual void		SecondaryAttack( void ) OVERRIDE;

private:
	void 				ResetVars( void );

private:
	float				m_flBlockedTime;
	bool				m_bAllowHolsterBecauseForced;
	CHandle<CTFPlayer>	m_hBackstabVictim;
	CNetworkVar( bool,	m_bReadyToBackstab );
	CNetworkVar( bool,	m_bKnifeExists );
	CNetworkVar( float, m_flKnifeRegenerateDuration );
	CNetworkVar( float, m_flKnifeMeltTimestamp );

	bool m_bWasTaunting;

	CTFKnife( const CTFKnife & ) {}
};

inline float CTFKnife::GetProgress( void )
{
	if ( m_bKnifeExists )
	{
		return 1.0f;
	}

	float meltedTime = gpGlobals->curtime - m_flKnifeMeltTimestamp;

	return meltedTime / m_flKnifeRegenerateDuration;
}


#endif // TF_WEAPON_KNIFE_H
