//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BUFF_ITEM_H
#define TF_WEAPON_BUFF_ITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"
#include "tf_item_wearable.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
#include "c_tf_buff_banner.h"
#define CTFBuffItem C_TFBuffItem
class C_TFBuffBanner;
#endif

//=============================================================================
//
// Buff item weapon class.
//
enum EBuffItemTypes
{
	EBuffBanner = 1,
	EBattalion,
	EConcheror,
	EParachute,

	NUM_BUFF_ITEM_TYPES = EParachute
};

enum EParachuteStates
{
	EParachuteDeployed,
	EParachuteDeployed_Idle,
	EParachuteRetracted,
	EParachuteRetracted_Idle,
};

class CTFBuffItem : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBuffItem, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBuffItem();
	~CTFBuffItem();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_BUFF_ITEM; }

	virtual void	Precache();

	virtual void	PrimaryAttack();

	virtual void	Equip( CBaseCombatCharacter *pOwner ) OVERRIDE;
	virtual void	Detach( void ) OVERRIDE;

	void			FireGameEvent( IGameEvent* event );
	virtual void	CreateBanner();

	virtual Activity TranslateViewmodelHandActivityInternal( Activity actBase ) OVERRIDE;

	void			BlowHorn( void );
	void			RaiseFlag( void );

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	virtual bool	CanReload( void );

	virtual int		GetBuffType() { int iBuffType = 0; CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type ); return iBuffType; }

#ifdef CLIENT_DLL
	void			SetBanner( C_TFBuffBanner* pNewBanner ) { m_hBannerEntity.Set( pNewBanner ); }
	virtual void	NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );
#endif

	virtual void	WeaponReset( void );

	float			GetProgress( void );
	bool			IsFull( void ); // same as GetProgress() without the division by 100.0f
	const char*		GetEffectLabelText( void ) { return "#TF_RAGE"; }
	bool			EffectMeterShouldFlash( void );

protected:
#ifdef CLIENT_DLL
	CHandle<C_TFBuffBanner>		m_hBannerEntity;
	int							m_iBuffType;
#endif // CLIENT_DLL

private:

	CTFBuffItem( const CTFBuffItem & ) {}

	bool						m_bPlayingHorn;
};

#endif // TF_WEAPON_BUFF_ITEM_H
