//=============================================================================
//
// snowball :)
//
//=============================================================================

#ifndef TF_WEAPON_SNOWBALL_H
#define TF_WEAPON_SNOWBALL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_throwable.h"

#ifdef CLIENT_DLL
#include "econ_notifications.h"
#define CTFSnowball				C_TFSnowball
#define CTFProjectile_Snowball	C_TFProjectile_Snowball
#endif

#define TF_SNOWBALL_MODEL		"models/workshop/weapons/c_models/c_snowball/c_snowball.mdl"

// *************************************************************************************************************************
class CTFSnowball : public CTFThrowable
{
public:
	DECLARE_CLASS( CTFSnowball, CTFThrowable );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFSnowball();

	virtual void		Precache( void ) OVERRIDE;
	virtual bool		CanDeploy( void ) OVERRIDE;
	virtual bool		Deploy( void ) OVERRIDE;
	virtual void		ItemPostFrame( void ) OVERRIDE;
	virtual void		ItemBusyFrame( void ) OVERRIDE;
	virtual void		ItemHolsterFrame( void ) OVERRIDE;
	virtual void		PrimaryAttack( void ) OVERRIDE;
	virtual int			GetSlot( void ) const OVERRIDE	{ return 5; }
	virtual bool		CanInspect() const OVERRIDE		{ return false; }
	virtual bool		VisibleInWeaponSelection( void ) { return false; }
	virtual bool		CanBeSelected( void ) { return false; }

	virtual const char *GetEffectLabelText( void ) OVERRIDE			{ return "#TF_Snowball_Meter"; }
	virtual float		InternalGetEffectBarRechargeTime( void ) OVERRIDE;
	virtual void		GetProjectileEntityName( CAttribute_String *attrProjectileEntityName ) OVERRIDE;
	static const char  *GetHandEffect( CEconItemView *pItem );
#ifdef CLIENT_DLL
	EHANDLE			m_hHandEffectWeapon;
	HPARTICLEFFECT	m_hHandEffect;
#endif // CLIENT_DLL

private:
	bool				m_bThrown;
	float				m_flSwitchBackTime;
};

// *************************************************************************************************************************
class CTFProjectile_Snowball : public CTFProjectile_Throwable
{
public:
	DECLARE_CLASS( CTFProjectile_Snowball, CTFProjectile_Throwable );
	DECLARE_NETWORKCLASS();

	virtual void		SetCustomPipebombModel() OVERRIDE			{ SetModel( TF_SNOWBALL_MODEL ); }
	virtual bool		ExplodesOnHit() OVERRIDE					{ return true; }
	virtual const char *GetThrowSoundEffect() const OVERRIDE		{ return "Snowball.Throw"; }

#ifdef CLIENT_DLL
	virtual const char *GetTrailParticleName(void) OVERRIDE { return "snowball_trail"; }
#endif

#ifdef GAME_DLL
	virtual void		OnHit( CBaseEntity *pOther ) OVERRIDE;
	virtual float		GetDamage() OVERRIDE;
	virtual float		GetDamageRadius() const OVERRIDE			{ return 0.0f; }
	virtual const char *GetExplodeEffectParticle() const OVERRIDE { return "snowball_impact"; }
	virtual const char *GetExplodeEffectSound() const OVERRIDE		{ return "Snowball.Splat"; }
	virtual bool CanCollideWithTeammates() const OVERRIDE { return true; }
#endif // GAME_DLL
};

#ifdef CLIENT_DLL

class CEquipSnowballNotification : public CEconNotification
{
public:
	CEquipSnowballNotification() : CEconNotification()
	{
		m_bHasTriggered = false;
	}

	~CEquipSnowballNotification()
	{
		if ( !m_bHasTriggered )
		{
			m_bHasTriggered = true;
		}
	}

	virtual void MarkForDeletion()
	{
		m_bHasTriggered = true;
		CEconNotification::MarkForDeletion();
	}

	virtual bool BShowInGameElements() const { return true; }
	virtual EType NotificationType() { return eType_AcceptDecline; }

	virtual void Accept();
	virtual void Trigger() { Accept(); }
	virtual void Decline() { MarkForDeletion(); }
	virtual void UpdateTick();

	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast<CEquipSnowballNotification*>( pNotification ) != NULL; }

private:
	bool m_bHasTriggered;
};

#endif // CLIENT_DLL

#endif // TF_WEAPON_SNOWBALL_H
