//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_WEAPON_BUILDER_H
#define TF_WEAPON_BUILDER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase.h"

class CBaseObject;

//=========================================================
// Builder Weapon
//=========================================================
class CTFWeaponBuilder : public CTFWeaponBase
{
	DECLARE_CLASS( CTFWeaponBuilder, CTFWeaponBase );
public:
	CTFWeaponBuilder();
	~CTFWeaponBuilder();

	DECLARE_SERVERCLASS();

	virtual void	SetSubType( int iSubType );
	virtual void	SetObjectMode( int iMode ) { m_iObjectMode = iMode; }
	virtual void	Precache( void );
	virtual bool	CanDeploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual bool	Deploy( void );	
	virtual Activity GetDrawActivity( void );
	virtual const char *GetViewModel( int iViewModel ) const;
	virtual const char *GetWorldModel( void ) const;

	virtual bool	AllowsAutoSwitchTo( void ) const;

	virtual int		GetType( void ) { return m_iObjectType; }

	virtual Activity TranslateViewmodelHandActivity( Activity actBase );

	void	SetCurrentState( int iState );
	void	SwitchOwnersWeaponToLast();

	// Placement
	void	StartPlacement( void );
	void	StopPlacement( void );
	void	UpdatePlacementState( void );		// do a check for valid placement
	bool	IsValidPlacement( void );			// is this a valid placement pos?


	// Building
	void	StartBuilding( void );

	// Special P Sapper (Wheatley) Item
	void	WheatleySapperIdle( CTFPlayer *pOwner );
	bool	IsWheatleySapper( void );
	void	WheatleyReset( bool bResetIntro = false );
	void	SetWheatleyState( int iNewState );
	float	WheatleyEmitSound( const char *snd , bool bEmitToAll = false, bool bNoRepeats = false );
	bool	IsWheatleyTalking( void );
	void	WheatleyDamage( void );
	int		GetWheatleyIdleWait();

	// Selection
	bool	HasAmmo( void );
	int		GetSlot( void ) const;
	int		GetPosition( void ) const;
	const char *GetPrintName( void ) const;
	bool	CanBuildObjectType( int iObjectType );
	void	SetObjectTypeAsBuildable( int iObjectType );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_BUILDER; }

	virtual void	WeaponReset( void );

	virtual float	InternalGetEffectBarRechargeTime( void ) { return 15.0; }
	virtual int		GetEffectBarAmmo( void ) { return TF_AMMO_GRENADES2; }

public:
	CNetworkVar( int, m_iBuildState );
	CNetworkVar( unsigned int, m_iObjectType );
	CNetworkVar( unsigned int, m_iObjectMode );
	CNetworkVar( bool, m_bRoboSapper );
	CNetworkVar( float, m_flWheatleyTalkingUntil );
	CNetworkArray( bool, m_aBuildableObjectTypes, OBJ_LAST );

	CNetworkHandle( CBaseObject, m_hObjectBeingBuilt );

	int m_iValidBuildPoseParam;

	float m_flNextDenySound;

	// P Sapper (Wheatley) 
	float		m_flNextVoicePakIdleStartTime;
	KeyValues	*m_pkvWavList;
	int			m_iSapState;
	
	float		m_flWheatleyLastDamage;
	float		m_flWheatleyLastDeploy;
	float		m_flWheatleyLastHolster;
	int			m_iWheatleyVOSequenceOffset;
	bool		m_bWheatleyIntroPlayed;
	EHANDLE     m_hLastSappedBuilding;
	Vector		m_vLastKnownSapPos;

private:
	bool		m_bAttack3Down;
};

// P Sapper (Wheatley) States
enum
{
	TF_PSAPSTATE_IDLE = 0,	
	TF_PSAPSTATE_WAITINGHACK,
	TF_PSAPSTATE_WAITINGHACKPW,
	TF_PSAPSTATE_WAITINGHACKED,
	TF_PSAPSTATE_WAITINGFOLLOWUP,
	TF_PSAPSTATE_SPECIALIDLE_KNIFE,
	TF_PSAPSTATE_SPECIALIDLE_HARMLESS,
	TF_PSAPSTATE_SPECIALIDLE_HACK,
	TF_PSAPSTATE_INTRO,
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFWeaponSapper : public CTFWeaponBuilder, public ITFChargeUpWeapon
{
public:
	DECLARE_CLASS( CTFWeaponSapper, CTFWeaponBuilder );
	DECLARE_SERVERCLASS();
	//DECLARE_PREDICTABLE();

	CTFWeaponSapper();

	virtual void		ItemPostFrame( void );

	// ITFChargeUpWeapon
	virtual bool CanCharge( void )				{ return GetChargeMaxTime() > 0; }
	virtual float GetChargeBeginTime( void )	{ return m_flChargeBeginTime; }
	virtual float GetChargeMaxTime( void )		{ float flChargeTime = 0; CALL_ATTRIB_HOOK_FLOAT( flChargeTime, sapper_deploy_time ); return flChargeTime; };

	virtual const char *GetViewModel( int iViewModel ) const;
	virtual const char *GetWorldModel( void ) const;

	virtual Activity TranslateViewmodelHandActivity( Activity actBase );

private:
	//float	m_flChargeBeginTime;
	bool	m_bAttackDown;

	CNetworkVar( float, m_flChargeBeginTime );
};

#define WHEATLEY_IDLE_WAIT_SECS_MIN 10.0
#define WHEATLEY_IDLE_WAIT_SECS_MAX 20.0


#endif // TF_WEAPON_BUILDER_H
