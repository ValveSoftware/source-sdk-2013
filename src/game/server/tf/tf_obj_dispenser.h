//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Dispenser
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_DISPENSER_H
#define TF_OBJ_DISPENSER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"
#include "triggers.h"

class CTFPlayer;

#define DISPENSER_MAX_HEALTH	150

#define DISPENSER_MINI_MAX_HEALTH	100
#define DISPENSER_MINI_MAX_LEVEL	1
#define DISPENSER_MINI_HEAL_RATE	10.0 
#define DISPENSER_MINI_AMMO_RATE	0.2 
#define DISPENSER_MINI_AMMO_THINK	0.5

#define SF_DISPENSER_IGNORE_LOS					(SF_BASEOBJ_INVULN<<1)
#define SF_DISPENSER_DONT_HEAL_DISGUISED_SPIES	(SF_BASEOBJ_INVULN<<2)	

// ------------------------------------------------------------------------ //
// Repair Trigger
// ------------------------------------------------------------------------ //
class CDispenserTouchTrigger : public CBaseTrigger
{
	DECLARE_CLASS( CDispenserTouchTrigger, CBaseTrigger );

public:
	CDispenserTouchTrigger() {}

	void Spawn( void )
	{
		BaseClass::Spawn();
		AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
		InitTrigger();
	}

	virtual void StartTouch( CBaseEntity *pEntity )
	{
		if ( PassesTriggerFilters( pEntity ) )
		{
			CBaseEntity *pParent = GetOwnerEntity();

			if ( pParent )
			{
				pParent->StartTouch( pEntity );
			}
		}
	}

	virtual void EndTouch( CBaseEntity *pEntity )
	{
		if ( PassesTriggerFilters( pEntity ) )
		{
			CBaseEntity *pParent = GetOwnerEntity();

			if ( pParent )
			{
				pParent->EndTouch( pEntity );
			}
		}
	}
};

// Ground placed version
#define DISPENSER_MODEL_PLACEMENT		"models/buildables/dispenser_blueprint.mdl"
#define DISPENSER_MODEL_BUILDING		"models/buildables/dispenser.mdl"
#define DISPENSER_MODEL					"models/buildables/dispenser_light.mdl"
#define DISPENSER_MODEL_BUILDING_LVL2	"models/buildables/dispenser_lvl2.mdl"
#define DISPENSER_MODEL_LVL2			"models/buildables/dispenser_lvl2_light.mdl"
#define DISPENSER_MODEL_BUILDING_LVL3	"models/buildables/dispenser_lvl3.mdl"
#define DISPENSER_MODEL_LVL3			"models/buildables/dispenser_lvl3_light.mdl"


// ------------------------------------------------------------------------ //
// Resupply object that's built by the player
// ------------------------------------------------------------------------ //
class CObjectDispenser : public CBaseObject
{
	DECLARE_CLASS( CObjectDispenser, CBaseObject );

public:
	DECLARE_SERVERCLASS();

	CObjectDispenser();
	~CObjectDispenser();

	static CObjectDispenser* Create(const Vector &vOrigin, const QAngle &vAngles);

	virtual void	Spawn() OVERRIDE;
	virtual void	FirstSpawn( void ) OVERRIDE;
	virtual void	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName ) OVERRIDE;
	virtual void	Precache() OVERRIDE;

	virtual void	DetonateObject( void ) OVERRIDE;
	virtual void	DestroyObject( void ) OVERRIDE;		// Silent cleanup

	virtual void	OnGoActive( void );	
	virtual void	StartPlacement( CTFPlayer *pPlayer ) OVERRIDE;
	virtual bool	StartBuilding( CBaseEntity *pBuilder ) OVERRIDE;
	virtual void	SetStartBuildingModel( void ) OVERRIDE;
	virtual int		DrawDebugTextOverlays(void) OVERRIDE;
	virtual void	SetModel( const char *pModel ) OVERRIDE;
	virtual void	InitializeMapPlacedObject( void ) OVERRIDE;
	virtual bool	ShouldBeMiniBuilding( CTFPlayer* pPlayer ) OVERRIDE;

	virtual bool	IsUpgrading( void ) const OVERRIDE { return ( m_iState == DISPENSER_STATE_UPGRADING ); }
	virtual void	StartUpgrading( void ) OVERRIDE;
	virtual void	FinishUpgrading( void ) OVERRIDE;

	virtual int		DispenseMetal( CTFPlayer *pPlayer );
	virtual int		GetAvailableMetal( void ) const;

	virtual void RefillThink( void );
	virtual void DispenseThink( void );

	virtual void StartTouch( CBaseEntity *pOther ) OVERRIDE;
	virtual void Touch( CBaseEntity *pOther ) OVERRIDE;
	virtual void EndTouch( CBaseEntity *pOther ) OVERRIDE;

	virtual const char* GetBuildingModel( int iLevel );
	virtual const char* GetFinishedModel( int iLevel );
	virtual const char* GetPlacementModel();

	virtual int	ObjectCaps( void ) OVERRIDE { return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE); }

	virtual bool DispenseAmmo( CTFPlayer *pPlayer );

	virtual void DropSpellPickup() { /* DO NOTHING */ }
	virtual void DropDuckPickup() { /* DO NOTHING */ }
	virtual void DispenseSouls() { /* Do nothing */}

	virtual float GetHealRate() const;
	virtual void StartHealing( CBaseEntity *pOther );
	void StopHealing( CBaseEntity *pOther );

	void AddHealingTarget( CBaseEntity *pOther );
	bool RemoveHealingTarget( CBaseEntity *pOther );
	bool IsHealingTarget( CBaseEntity *pTarget );

	bool CouldHealTarget( CBaseEntity *pTarget );
	virtual float GetDispenserRadius( void );


	Vector GetHealOrigin( void );

	CUtlVector< EHANDLE >	m_hHealingTargets;

	virtual void	MakeMiniBuilding( CTFPlayer* pPlayer ) OVERRIDE;
	virtual void	MakeCarriedObject( CTFPlayer *pCarrier );

	virtual int		GetBaseHealth( void ) { return DISPENSER_MAX_HEALTH; }

	virtual int		GetMaxUpgradeLevel( void ) OVERRIDE;

	virtual int		GetMiniBuildingStartingHealth( void ) OVERRIDE { return DISPENSER_MINI_MAX_HEALTH; }

	CBaseEntity		*GetTouchTrigger() const { return m_hTouchTrigger; }
	void			DisableAmmoPickupSound() { m_bPlayAmmoPickupSound = false; }
	void			DisableGenerateMetalSound() { m_bUseGenerateMetalSound = false; }

private:
	virtual void PlayActiveSound();
	void ResetHealingTargets( void );

protected:

	// The regular and mini dispenser can be repaired
	virtual bool CanBeRepaired() const OVERRIDE { return true; }

	CNetworkVar( int, m_iState );
	CNetworkVar( int, m_iAmmoMetal );
	CNetworkVar( int, m_iMiniBombCounter );

	bool m_bUseGenerateMetalSound;

	// Entities currently being touched by this trigger
	CUtlVector< EHANDLE >	m_hTouchingEntities;

	float m_flNextAmmoDispense;

	bool m_bThrown;

	string_t m_iszCustomTouchTrigger;
	EHANDLE m_hTouchTrigger;

	DECLARE_DATADESC();

private:

	CountdownTimer m_spellTimer;
	CountdownTimer m_duckTimer;
	CountdownTimer m_soulTimer;

	float m_flPrevRadius;
	bool m_bPlayAmmoPickupSound;
};

inline int CObjectDispenser::GetAvailableMetal( void ) const
{
	return m_iAmmoMetal;
}


//------------------------------------------------------------------------------
class CObjectCartDispenser : public CObjectDispenser
{
	DECLARE_CLASS( CObjectCartDispenser, CObjectDispenser );

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CObjectCartDispenser();

	virtual void Spawn( void );
	virtual void OnGoActive( void );
	virtual void GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );

	virtual int	DispenseMetal( CTFPlayer *pPlayer );
	virtual void DropSpellPickup();
	virtual void DropDuckPickup();
	virtual void DispenseSouls() OVERRIDE;

	virtual bool	CanBeUpgraded( CTFPlayer *pPlayer ){ return false; }
	virtual void	SetModel( const char *pModel );

	void InputFireHalloweenBonus( inputdata_t &inputdata );
	void InputSetDispenserLevel( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
};

#endif // TF_OBJ_DISPENSER_H
