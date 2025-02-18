//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_RAYGUN_REVENGE_H
#define TF_WEAPON_RAYGUN_REVENGE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_shotgun.h"
#include "tf_weapon_rocketlauncher.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFRaygun C_TFRaygun
#define CTFDRGPomson C_TFDRGPomson
#endif

#define RAYGUN_ENERGY_PER_SHOT		20
#define RAYGUN_ENERGY_PER_PUMP		100

class CTFRaygun_Revenge : public CTFRaygun
{
public:
	DECLARE_CLASS( CTFRaygun, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRaygun();

	virtual void		Precache();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_RAYGUN_REVENGE; }
	virtual float		GetProjectileSpeed( void );
	virtual float		GetProjectileGravity( void );
	virtual bool		IsViewModelFlipped( void );

	const char*			GetEffectLabelText( void )			{ return "#TF_BISON"; }
	float				GetProgress( void );

	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool		Deploy( void );
	virtual void		ItemPostFrame( void );

	virtual void		PrimaryAttack( void );
	virtual void		ModifyProjectile( CBaseEntity* pProj );

	virtual const char*	GetMuzzleFlashParticleEffect( void );

	virtual float		GetDamage( void ) { return 20.f; }

	virtual bool		IsEnergyWeapon( void ) const { return true; }
	virtual float		Energy_GetShotCost( void ) const
	{
		int iNoDrain = 0;
		CALL_ATTRIB_HOOK_INT( iNoDrain, energy_weapon_no_drain );
		if ( iNoDrain > 0 )
		{
			return 0.0f;
		}

		return 25.f; 
	}
	virtual float		Energy_GetRechargeCost( void ) const { return 25.f; }

#ifdef CLIENT_DLL
	virtual void		DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	void				ClientEffectsThink( void );
	virtual bool		ShouldPlayClientReloadSound() { return true; }
#endif

private:
	float				m_flIrradiateTime;
	bool				m_bEffectsThinking;
};


//---------------------------------------------------------
class CTFDRGPomson : public CTFRaygun
{
public:
	DECLARE_CLASS( CTFDRGPomson, CTFRaygun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_DRG_POMSON; }
	const char*			GetEffectLabelText( void )			{ return "#TF_POMSON_HUD"; }
};



#endif // TF_WEAPON_RAYGUN_REVENGE_H
