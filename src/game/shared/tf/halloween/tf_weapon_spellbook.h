//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SPELLBOOK_H
#define TF_WEAPON_SPELLBOOK_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "tf_weapon_jar.h"
#include "tf_weapon_throwable.h"
#include "tf_shareddefs.h"
#include "tf_viewmodel.h"
#include "econ_item_view.h"

#ifdef CLIENT_DLL
	#include <vgui_controls/EditablePanel.h>
	#include "hudelement.h"
	#include "econ_controls.h"
	#include "c_tf_projectile_rocket.h"
	#include "econ_notifications.h"
	#include "vgui_controls/ImagePanel.h"

	#define CTFSpellBook							C_TFSpellBook
	#define CTFProjectile_SpellFireball				C_TFProjectile_SpellFireball
	#define CTFProjectile_SpellBats					C_TFProjectile_SpellBats
	#define CTFProjectile_SpellSpawnZombie			C_TFProjectile_SpellSpawnZombie
	#define CTFProjectile_SpellSpawnHorde			C_TFProjectile_SpellSpawnHorde
	#define CTFProjectile_SpellMirv					C_TFProjectile_SpellMirv
	#define CTFProjectile_SpellPumpkin				C_TFProjectile_SpellPumpkin

	#define CTFProjectile_SpellSpawnBoss			C_TFProjectile_SpellSpawnBoss
	#define CTFProjectile_SpellMeteorShower			C_TFProjectile_SpellMeteorShower
	#define CTFProjectile_SpellTransposeTeleport	C_TFProjectile_SpellTransposeTeleport
	#define CTFProjectile_SpellLightningOrb			C_TFProjectile_SpellLightningOrb
	#define CTFProjectile_SpellVortex				C_TFProjectile_SpellVortex
	
	#define CTFProjectile_SpellKartOrb				C_TFProjectile_SpellKartOrb
	#define CTFProjectile_SpellKartBats				C_TFProjectile_SpellKartBats
	#define CTFProjectile_SpellKartMirv				C_TFProjectile_SpellKartMirv
	#define CTFProjectile_SpellKartPumpkin			C_TFProjectile_SpellKartPumpkin

	#define CTFProjectile_BallOfFire			C_TFProjectile_BallOfFire	
#else
	#include "tf_projectile_rocket.h"
#endif

#ifdef CLIENT_DLL

// For testing, hijack this basic menu but replace it later with TF specific UI
class CHudSpellMenu : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudSpellMenu, EditablePanel );
public:
	CHudSpellMenu( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void OnTick( void ) OVERRIDE;

	void UpdateSpellText( int iSpellIndex, int iCharges );

private:
	vgui::ImagePanel	*m_pSpellIcon;
	CExLabel			*m_pKeyBinding;

	int		m_iPrevSelectedSpell;
	float	m_iNextRollTime;
	float	m_flRollTickGap;
	bool	m_bTickSoundA;

	bool	m_bKillstreakMeterDrawing;
};

//=============================================================================
class CEquipSpellbookNotification : public CEconNotification
{
public:
	CEquipSpellbookNotification() : CEconNotification()
	{
		m_bHasTriggered = false;
	}

	~CEquipSpellbookNotification()
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

	virtual EType NotificationType() { return eType_AcceptDecline; }
	virtual bool BShowInGameElements() const { return true; }

	virtual void Accept();
	virtual void Trigger() { Accept(); }
	virtual void Decline() { MarkForDeletion(); }
	virtual void UpdateTick();

	static bool IsNotificationType( CEconNotification *pNotification ) { return dynamic_cast< CEquipSpellbookNotification *>( pNotification ) != NULL; }

private:
	bool m_bHasTriggered;
};
#endif // CLIENT_DLL

#ifdef GAME_DLL
void RemoveAll2013HalloweenTeleportSpellsInMidFlight( void );
#endif

//=============================================================================
//
// CTFSpellBook class.
//
class CTFSpellBook : public CTFThrowable
{
public:
	DECLARE_CLASS( CTFSpellBook, CTFThrowable );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSpellBook();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SPELLBOOK; }
	virtual const char*	GetEffectLabelText( void )			{ return "#TF_KART"; }
	virtual void		Precache( void );
	
	virtual void		PrimaryAttack();
	virtual void		ItemPostFrame( void );

	virtual void		ItemBusyFrame( void );		
	virtual void		ItemHolsterFrame( void );

	virtual bool		ShowHudElement ()								{ return false; }
	virtual bool		VisibleInWeaponSelection( void )				{ return false; }
	virtual bool		CanBeSelected( void )							{ return false; }

	bool				HasASpellWithCharges();

	virtual CBaseEntity *FireJar( CTFPlayer *pPlayer ) OVERRIDE;

	bool			CanCastSpell( CTFPlayer *pPlayer );
	void			PaySpellCost( CTFPlayer *pPlayer );
	void			ClearSpell();

	// Hack for infinite ammo
	virtual bool	IsEnergyWeapon( void ) const		{ return true; }
	float			Energy_GetMaxEnergy( void ) const	{ return 500; }
	float			Energy_GetEnergy( void ) const		{ return 500; }
	bool			Energy_FullyCharged( void ) const	{ return true; }
	bool			Energy_HasEnergy( void )			{ return true; }



#ifdef GAME_DLL
	void			SaveLastWeapon( CBaseCombatWeapon *pWpn ) { m_pStoredLastWpn = pWpn; }

	// Projectile Creation
	virtual void	TossJarThink( void );
	virtual void	CreateSpellRocket( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
	virtual void	CreateSpellJar( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	// Spell Helpers
	// Think
	void			RollNewSpell( int iTier, bool bForceReroll = false );
	void			SetSelectedSpell( int index );
	void			SpeakSpellConceptIfAllowed();
	
	// Spells
	void			CastKartSpell();
	bool			CastSpell( CTFPlayer *pPlayer, int iSpellIndex );

	CHandle<CBaseCombatWeapon> m_pStoredLastWpn;

	void	RollNewSpellFinish( void );
	int		m_iNextSpell;
	int		m_iPreviouslyCastSpell;

#endif

	virtual bool CanThrowUnderWater( void ){ return true; }

#ifdef CLIENT_DLL
	float			m_flTimeNextErrorSound;
	EHANDLE			m_hHandEffectWeapon;
	HPARTICLEFFECT	m_hHandEffect;

#endif // CLIENT_DLL

	// Self Cast Spells
	static bool		CastSelfHeal( CTFPlayer *pPlayer );
	static bool		CastRocketJump( CTFPlayer *pPlayer );
	static bool		CastSelfSpeedBoost( CTFPlayer *pPlayer );
	static bool		CastSelfStealth( CTFPlayer *pPlayer );
	
	static bool		CastKartRocketJump( CTFPlayer *pPlayer );
	static bool		CastKartUber( CTFPlayer *pPlayer );
	static bool		CastKartBombHead( CTFPlayer *pPlayer );

	static const char* GetHandEffect( CEconItemView *pItem, int iTier );

	CNetworkVar( float, m_flTimeNextSpell );
	CNetworkVar( int, m_iSelectedSpellIndex );
	CNetworkVar( int, m_iSpellCharges );

	CNetworkVar( bool, m_bFiredAttack );
};


#ifdef GAME_DLL
CBaseEntity* CreateSpellSpawnZombie( CBaseCombatCharacter *pCaster, const Vector& vSpawnPosition, int nSkeletonType );
#endif


#endif // TF_WEAPON_SPELLBOOK_H
