//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Grappling Hook
//
//=============================================================================
#ifndef TF_WEAPON_GRAPPLINGHOOK_H
#define TF_WEAPON_GRAPPLINGHOOK_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_rocketlauncher.h"

#ifdef CLIENT_DLL
#include "econ_notifications.h"

#define CTFGrapplingHook C_TFGrapplingHook
#endif // CLIENT_DLL

// ------------------------------------------------------------------------------------------------------------------------
class CTFGrapplingHook : public CTFRocketLauncher
{
public:
	DECLARE_CLASS( CTFGrapplingHook, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif // GAME_DLL

	CTFGrapplingHook();

	virtual void	Precache() OVERRIDE;
	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer ) OVERRIDE;
	virtual void	ItemPostFrame( void ) OVERRIDE;
	virtual bool	CanAttack( void ) OVERRIDE;
	virtual void	PrimaryAttack( void ) OVERRIDE;
	virtual bool	Deploy( void ) OVERRIDE;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo ) OVERRIDE;
	virtual void	GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, float flEndDist = 2000.f ) OVERRIDE;

	virtual int		GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_GRAPPLINGHOOK; }
	virtual float	GetProjectileSpeed( void ) OVERRIDE;
	virtual float	GetProjectileGravity( void ) OVERRIDE { return 0.f; }
	virtual int		GetWeaponProjectileType( void ) const OVERRIDE { return TF_PROJECTILE_GRAPPLINGHOOK; }
	virtual bool	ShouldRemoveDisguiseOnPrimaryAttack() const OVERRIDE { return false; }
	virtual bool	ShouldRemoveInvisibilityOnPrimaryAttack() const OVERRIDE { return false; }
	virtual int		GetCanAttackFlags() const OVERRIDE { return TF_CAN_ATTACK_FLAG_GRAPPLINGHOOK; }

	virtual bool	SendWeaponAnim( int iActivity );

	virtual void	PlayWeaponShootSound( void ) OVERRIDE;

#ifdef CLIENT_DLL
	virtual void	UpdateOnRemove() OVERRIDE;
	virtual void	OnDataChanged( DataUpdateType_t type ) OVERRIDE;
#endif // CLIENT_DLL


	// acttable override
	virtual acttable_t *ActivityList( int &iActivityCount ) OVERRIDE;

	// poseparam override
	virtual poseparamtable_t *GetPlayerPoseParamList( int &iPoseParamCount ) OVERRIDE;

#ifdef GAME_DLL
	void			ActivateRune();
#endif // GAME_DLL

private:
#ifdef GAME_DLL
	void			RemoveHookProjectile( bool bForce = false );
	bool			IsLatchedToTargetPlayer() const;
	bool			m_bReleasedAfterLatched;
#endif // GAME_DLL

#ifdef CLIENT_DLL
	void			StartHookSound();
	void			StopHookSound();
	void			UpdateHookSound();
	CSoundPatch		*m_pHookSound;
	bool			m_bLatched;
	float			m_flNextSupernovaDenyWarning;
#endif // CLIENT_DLL

	void			OnHookReleased( bool bForce );

	CNetworkHandle( CBaseEntity, m_hProjectile );
	CountdownTimer m_startFiringTimer;
	CountdownTimer m_startPullingTimer;
};

#ifdef CLIENT_DLL

class CEquipGrapplingHookNotification : public CEconNotification
{
public:
	CEquipGrapplingHookNotification() : CEconNotification()
	{
		m_bHasTriggered = false;
	}

	~CEquipGrapplingHookNotification()
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

	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast< CEquipGrapplingHookNotification *>( pNotification ) != NULL; }

private:
	bool m_bHasTriggered;
};

#endif // CLIENT_DLL

#endif // TF_WEAPON_GRAPPLINGHOOK_H
