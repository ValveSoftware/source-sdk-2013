//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WEAPON_CSBASE_H
#define WEAPON_CSBASE_H
#ifdef _WIN32
#pragma once
#endif


#include "cs_playeranimstate.h"
#include "cs_weapon_parse.h"
#include "mom_player_shared.h"


#if defined( CLIENT_DLL )
	#define CWeaponCSBase C_WeaponCSBase
#endif

extern int  ClassnameToWeaponID( const char *classname );
extern int	AliasToWeaponID( const char *alias );
extern const char *WeaponIDToAlias( int id );
extern const char *GetTranslatedWeaponAlias( const char *alias);
extern bool	IsPrimaryWeapon( int id );
extern bool IsSecondaryWeapon( int id );
extern int GetShellForAmmoType( const char *ammoname );

#define SHIELD_VIEW_MODEL "models/weapons/v_shield.mdl"
#define SHIELD_WORLD_MODEL "models/weapons/w_shield.mdl"

#define CS_MUZZLEFLASH_NONE -1
#define CS_MUZZLEFLASH_NORM	0
#define CS_MUZZLEFLASH_X	1

// These are the names of the ammo types that go in the CAmmoDefs and that the 
// weapon script files reference.
#define BULLET_PLAYER_50AE		"BULLET_PLAYER_50AE"
#define BULLET_PLAYER_762MM		"BULLET_PLAYER_762MM"
#define BULLET_PLAYER_556MM		"BULLET_PLAYER_556MM"
#define BULLET_PLAYER_556MM_BOX	"BULLET_PLAYER_556MM_BOX"
#define BULLET_PLAYER_338MAG	"BULLET_PLAYER_338MAG"
#define BULLET_PLAYER_9MM		"BULLET_PLAYER_9MM"
#define BULLET_PLAYER_BUCKSHOT	"BULLET_PLAYER_BUCKSHOT"
#define BULLET_PLAYER_45ACP		"BULLET_PLAYER_45ACP"
#define BULLET_PLAYER_357SIG	"BULLET_PLAYER_357SIG"
#define BULLET_PLAYER_57MM		"BULLET_PLAYER_57MM"
#define AMMO_TYPE_HEGRENADE		"AMMO_TYPE_HEGRENADE"
#define AMMO_TYPE_FLASHBANG		"AMMO_TYPE_FLASHBANG"
#define AMMO_TYPE_SMOKEGRENADE	"AMMO_TYPE_SMOKEGRENADE"

#define CROSSHAIR_CONTRACT_PIXELS_PER_SECOND	7.0f

// Given an ammo type (like from a weapon's GetPrimaryAmmoType()), this compares it
// against the ammo name you specify.
// MIKETODO: this should use indexing instead of searching and strcmp()'ing all the time.
bool IsAmmoType( int iAmmoType, const char *pAmmoName );

typedef enum
{
	Primary_Mode = 0,
	Secondary_Mode,
} CSWeaponMode;

#if defined( CLIENT_DLL )

	//--------------------------------------------------------------------------------------------------------------
	/**
	*  Returns the client's ID_* value for the currently owned weapon, or ID_NONE if no weapon is owned
	*/
	CSWeaponID GetClientWeaponID( bool primary );

#endif

	//--------------------------------------------------------------------------------------------------------------
	CCSWeaponInfo * GetWeaponInfo( CSWeaponID weaponID );


class CWeaponCSBase : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponCSBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponCSBase();

	#ifdef GAME_DLL
		DECLARE_DATADESC();

		virtual void CheckRespawn();
		virtual CBaseEntity* Respawn();
		
		virtual const Vector& GetBulletSpread();
		virtual float	GetDefaultAnimSpeed();

		virtual void	BulletWasFired( const Vector &vecStart, const Vector &vecEnd );
		virtual bool	ShouldRemoveOnRoundRestart();
		virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );

		void SendReloadEvents();

		void Materialize();
		void AttemptToMaterialize();
		virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

		virtual bool IsRemoveable();
		
	#endif

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );
	// All predicted weapons need to implement and return true
	virtual bool	IsPredicted() const;

	// Pistols reset m_iShotsFired to 0 when the attack button is released.
	bool			IsPistol() const;

	// Is this an awp?
	virtual bool	IsAwp() const;

	CMomentumPlayer *GetPlayerOwner() const;

	virtual float GetMaxSpeed() const;	// What's the player's max speed while holding this weapon.

	// Get CS-specific weapon data.
	CCSWeaponInfo const	&GetCSWpnData() const;

	// Get specific CS weapon ID (ie: WEAPON_AK47, etc)
	virtual CSWeaponID GetWeaponID( void ) const		{ return WEAPON_NONE; }

	// return true if this weapon is an instance of the given weapon type (ie: "IsA" WEAPON_GLOCK)
	bool IsA( CSWeaponID id ) const						{ return GetWeaponID() == id; }

	// return true if this weapon is a kinf of the given weapon type (ie: "IsKindOf" WEAPONTYPE_RIFLE )
	bool IsKindOf( CSWeaponType type ) const			{ return GetCSWpnData().m_WeaponType == type; }

	// return true if this weapon has a silencer equipped
	virtual bool IsSilenced( void ) const				{ return false; }

	virtual void SetWeaponModelIndex( const char *pName );
	virtual void OnPickedUp( CBaseCombatCharacter *pNewOwner );

public:
	#if defined( CLIENT_DLL )

		virtual void	ProcessMuzzleFlashEvent();
		virtual bool	OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
		virtual bool	ShouldPredict();
		virtual void	DrawCrosshair();
		virtual void	OnDataChanged( DataUpdateType_t type );

		virtual int		GetMuzzleAttachment( void );
		virtual bool	HideViewModelWhenZoomed( void ) { return true; }

		float			m_flCrosshairDistance;
		int				m_iAmmoLastCheck;
		int				m_iAlpha;
		int				m_iScopeTextureID;
		int				m_iCrosshairTextureID; // for white additive texture

		virtual int GetMuzzleFlashStyle( void );
        bool m_bInReloadAnimation;
	#else

		virtual bool	Reload();
		virtual void	Spawn();
		virtual bool	KeyValue( const char *szKeyName, const char *szValue );

		virtual bool PhysicsSplash( const Vector &centerPoint, const Vector &normal, float rawSpeed, float scaledSpeed );

	#endif

	bool IsUseable();
	virtual bool	CanDeploy( void );
	virtual void	UpdateShieldState( void );
	virtual bool	SendWeaponAnim( int iActivity );
	virtual void	Precache( void );
	virtual bool	CanBeSelected( void );
	virtual Activity GetDeployActivity( void );
	virtual bool	DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	virtual void 	DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	virtual bool	DefaultPistolReload();

	virtual bool	Deploy();
	virtual void	Drop( const Vector &vecVelocity );
	bool PlayEmptySound();
	virtual void	ItemPostFrame();

	virtual const char		*GetViewModel( int viewmodelindex = 0 ) const;


	bool	m_bDelayFire;			// This variable is used to delay the time between subsequent button pressing.
	float	m_flAccuracy;
	
	void SetExtraAmmoCount( int count ) { m_iExtraPrimaryAmmo = count; }
	int GetExtraAmmoCount( void ) { return m_iExtraPrimaryAmmo; }

private:

	float	m_flDecreaseShotsFired;

	CWeaponCSBase( const CWeaponCSBase & );

	int		m_iExtraPrimaryAmmo;

	float	m_nextPrevOwnerTouchTime;
	CMomentumPlayer *m_prevOwner;

	int m_iDefaultExtraAmmo;
};


#endif // WEAPON_CSBASE_H
