//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_RAYGUN_H
#define TF_WEAPON_RAYGUN_H
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

class CTFRaygun : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFRaygun, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRaygun();

	virtual void		Precache();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_RAYGUN; }
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

		return 5.f; 
	}
	virtual float		Energy_GetRechargeCost( void ) const { return 5.f; }

#ifdef CLIENT_DLL
	virtual void		DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	void				ClientEffectsThink( void );
	virtual bool		ShouldPlayClientReloadSound() { return true; }
	virtual const char *GetIdleParticleEffect( void ) { return "drg_bison_idle"; }
#endif

	bool				UseNewProjectileCode() const { return m_bUseNewProjectileCode; }

private:

	float				m_flIrradiateTime;
	bool				m_bEffectsThinking;

	CNetworkVar( bool, m_bUseNewProjectileCode );
};


//---------------------------------------------------------
class CTFDRGPomson : public CTFRaygun
{
public:
	DECLARE_CLASS( CTFDRGPomson, CTFRaygun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void		Precache();

	virtual int			GetWeaponID( void ) const				{ return TF_WEAPON_DRG_POMSON; }
	const char*			GetEffectLabelText( void )				{ return "#TF_POMSON_HUD"; }

	virtual const char *GetMuzzleFlashParticleEffect( void )	{ return "drg_pomson_muzzleflash"; }
	virtual const char *GetIdleParticleEffect( void )			{ return "drg_pomson_idle"; }

	virtual void GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, float flEndDist = 2000.f );
};



#endif // TF_WEAPON_RAYGUN_H
