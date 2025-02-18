//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEARABLE_H
#define TF_WEARABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_wearable.h"
#include "props_shared.h"
#include "GameEventListener.h"
#include "ihasgenericmeter.h"

#if defined( CLIENT_DLL )
#define CTFWearable C_TFWearable
#define CTFWearableVM C_TFWearableVM
#endif


#if defined( CLIENT_DLL )
class CTFWearable : public CEconWearable, public CGameEventListener, public IHasGenericMeter
#else
class CTFWearable : public CEconWearable, public IHasGenericMeter
#endif
{
	DECLARE_CLASS( CTFWearable, CEconWearable );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	CTFWearable();

	virtual void		Equip( CBasePlayer* pOwner );
	virtual void		UnEquip( CBasePlayer* pOwner );
	virtual bool		CanEquip( CBaseEntity *pOther );
	void				SetDisguiseWearable( bool bState ) { m_bDisguiseWearable = bState; }
	bool				IsDisguiseWearable( void ) const { return m_bDisguiseWearable; }
	void				SetWeaponAssociatedWith( CBaseEntity *pWeapon ) { m_hWeaponAssociatedWith = pWeapon; }
	CBaseEntity*		GetWeaponAssociatedWith( void ) const { return m_hWeaponAssociatedWith.Get(); }
	virtual bool		UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState );
	virtual void		ReapplyProvision( void );

#if defined( GAME_DLL )
	void				Break( void );
	virtual int			CalculateVisibleClassFor( CBaseCombatCharacter *pPlayer );
	virtual int			UpdateTransmitState();
	virtual	int			ShouldTransmit( const CCheckTransmitInfo *pInfo );

	int					GetKillStreak ( )			{ return m_iKillStreak; }
	void				SetKillStreak ( int value ) { m_iKillStreak = value; };
#endif

#if defined( CLIENT_DLL )
	virtual int			InternalDrawModel( int flags );
	virtual bool		ShouldDraw();
	virtual bool		ShouldDrawWhenPlayerIsDead() { return ( GetWeaponAssociatedWith() == NULL ); }
	virtual bool		ShouldDrawParticleSystems( void );		// can't be const because it potentially mutates m_eParticleSystemVisibility state
	virtual int			GetWorldModelIndex( void );
	virtual void		ValidateModelIndex( void );

	virtual void		OnDataChanged( DataUpdateType_t updateType );
	virtual void		FireGameEvent( IGameEvent *event );
#endif

	virtual int			GetSkin( void );

	void				AddHiddenBodyGroup( const char* bodygroup );

protected:
	virtual void		InternalSetPlayerDisplayModel( void );


private:
	CNetworkVar( bool, m_bDisguiseWearable );
	CNetworkHandle( CBaseEntity, m_hWeaponAssociatedWith );

	CUtlVector< const char* >	m_HiddenBodyGroups;

#ifdef GAME_DLL
	int				m_iKillStreak;
#endif // GAME_DLL

#if defined( CLIENT_DLL )
	enum eParticleSystemVisibility
	{
		kParticleSystemVisibility_Undetermined,
		kParticleSystemVisibility_Shown,
		kParticleSystemVisibility_Hidden,
	};
	eParticleSystemVisibility m_eParticleSystemVisibility;

	short		m_nWorldModelIndex;
#endif
};


class CTFWearableVM : public CTFWearable
{
	DECLARE_CLASS( CTFWearableVM, CTFWearable );
public:
	DECLARE_NETWORKCLASS();

	virtual bool IsViewModelWearable( void ) { return true; }

#if defined( CLIENT_DLL )
	virtual ShadowType_t ShadowCastType( void );
#endif
};

#endif // TF_WEARABLE_H
