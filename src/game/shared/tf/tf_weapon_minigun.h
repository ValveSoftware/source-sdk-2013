//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_WEAPON_MINIGUN_H
#define TF_WEAPON_MINIGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#include "particles_new.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFMinigun C_TFMinigun
#endif

#ifdef GAME_DLL
class ITFProjectile;
#endif // GAME_DLL

enum MinigunState_t
{
	// Firing states.
	AC_STATE_IDLE = 0,
	AC_STATE_STARTFIRING,
	AC_STATE_FIRING,
	AC_STATE_SPINNING,
	AC_STATE_DRYFIRE
};

enum minigun_weapontypes_t
{
	MINIGUN_STANDARD = 0,
	MINIGUN_STUN,	// Natascha
};

//=============================================================================
//
// TF Weapon Minigun
//
class CTFMinigun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFMinigun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	CTFMinigun();
	~CTFMinigun();

	virtual void	Precache( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_MINIGUN; }
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	void			SharedAttack();
	virtual void	WeaponIdle();
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	HolsterOnDetach() { return true; }
	virtual bool	Lower( void );
	virtual void	HandleFireOnEmpty( void );
	virtual void	WeaponReset( void );
	virtual float	GetProjectileDamage( void );
	virtual bool	ShouldDoMuzzleFlash( void ) { return false; }
	virtual float	GetWeaponSpread( void );

	virtual void	FireGameEvent( IGameEvent *event );

#ifdef GAME_DLL
	virtual int		UpdateTransmitState( void );

	void			AttackEnemyProjectiles( void );
	void			ActivatePushBackAttackMode( void );
#endif

	void			RingOfFireAttack( int nDamage );

	virtual int		GetCustomDamageType() const { return TF_DMG_CUSTOM_MINIGUN; }
	int				GetMinigunType( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode ); return iMode; };
	bool			HasSpinSounds( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, minigun_no_spin_sounds ); return iMode!=1; };
	bool			CanHolsterWhileSpinning( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, mod_minigun_can_holster_while_spinning ); return iMode!=0; };

	float			GetFiringDuration( void ) { return ( m_flStartedFiringAt >= 0.f ) ? ( gpGlobals->curtime - m_flStartedFiringAt ) : 0.f; }
	float			GetWindUpDuration( void ) { return ( m_flStartedWindUpAt >= 0.f ) ? ( gpGlobals->curtime - m_flStartedWindUpAt ) : 0.f; }

	float			GetProgress( void );
	bool			IsRageFull( void ); // same as GetProgress() without the division by 100.0f
	const char*		GetEffectLabelText( void ) { return "#TF_Rage"; }
	bool			EffectMeterShouldFlash( void );

	virtual bool	CanInspect() const OVERRIDE;

#ifdef GAME_DLL
	virtual CDmgAccumulator	*GetDmgAccumulator( void ) { return &m_Accumulator; }
	virtual float GetInitialAfterburnDuration() const OVERRIDE;
#endif // GAME_DLL

#ifdef CLIENT_DLL
	float GetBarrelRotation();
#endif

private:
	
	CTFMinigun( const CTFMinigun & ) {}

	void WindUp( void );
	void WindDown( void );

#ifdef GAME_DLL
	CDmgAccumulator m_Accumulator;
#endif // GAME_DLL

#ifdef CLIENT_DLL
	// Barrel spinning
	virtual CStudioHdr *OnNewModel( void );
	virtual void		StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	
	virtual void		UpdateOnRemove( void );

	void				CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles );

	void				OnDataChanged( DataUpdateType_t type );
		
	virtual void	ItemPreFrame( void );
	
	// Firing sound
	void				WeaponSoundUpdate( void );
	void				PlayStopFiringSound();

	void				UpdateBarrelMovement( void );
	virtual void		SetDormant( bool bDormant );

	virtual void		ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
#endif

	virtual bool			CanReload( void ){ return false; }

private:
	virtual void PlayWeaponShootSound( void ) {}	// override base class call to play shoot sound; we handle that ourselves separately

	void SetWeaponState( MinigunState_t nState );

	CNetworkVar( MinigunState_t, m_iWeaponState );
	CNetworkVar( bool, m_bCritShot );

	float			m_flNextFiringSpeech;
	float			m_flStartedFiringAt;
	float			m_flStartedWindUpAt;
	float	m_flBarrelCurrentVelocity;
	float	m_flBarrelTargetVelocity;
	int		m_iBarrelBone;
	float	m_flBarrelAngle;
	CSoundPatch		*m_pSoundCur;				// the weapon sound currently being played
	int				m_iMinigunSoundCur;			// the enum value of the weapon sound currently being played
	float			m_flMinigunSoundCurrentPitch;

#ifdef GAME_DLL
	float	m_flAegisCheckTime;
#endif

	float	m_flNextRingOfFireAttackTime;
	float	m_flAccumulatedAmmoDrain;
	float	m_flLastAmmoDrainTime;

	bool	m_bAttack3Down;

#ifdef CLIENT_DLL
	void StartBrassEffect();
	void StopBrassEffect();
	void HandleBrassEffect();

	EHANDLE				m_hEjectBrassWeapon;
	CNewParticleEffect *m_pEjectBrassEffect;
	int					m_iEjectBrassAttachment;

	void StartMuzzleEffect();
	void StopMuzzleEffect();
	void HandleMuzzleEffect();

	EHANDLE				m_hMuzzleEffectWeapon;
	CNewParticleEffect *m_pMuzzleEffect;
	int					m_iMuzzleAttachment;

	int					m_nShotsFired;

	bool				m_bRageDraining;
	bool				m_bPrevRageDraining;

	MinigunState_t		m_iPrevMinigunState;
#endif
};

#endif // TF_WEAPON_MINIGUN_H
