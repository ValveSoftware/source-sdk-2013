//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_lunchbox.h"
#include "tf_fx_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "prediction.h"
// Server specific.
#else
#include "tf_player.h"
#include "entity_healthkit.h"
#include "econ_item_view.h"
#include "econ_item_system.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Lunchbox tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFLunchBox, DT_WeaponLunchBox )

BEGIN_NETWORK_TABLE( CTFLunchBox, DT_WeaponLunchBox )
#if defined( CLIENT_DLL )
RecvPropBool( RECVINFO( m_bBroken ), 0, CTFLunchBox::RecvProxy_Broken )
#else
SendPropBool( SENDINFO( m_bBroken ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFLunchBox )
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD( m_bBroken, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_INSENDTABLE )
#endif // CLIENT_DLL
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_lunchbox, CTFLunchBox );
PRECACHE_WEAPON_REGISTER( tf_weapon_lunchbox );

//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFLunchBox_Drink, DT_TFLunchBox_Drink )

BEGIN_NETWORK_TABLE( CTFLunchBox_Drink, DT_TFLunchBox_Drink )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFLunchBox_Drink )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_lunchbox_drink, CTFLunchBox_Drink );
PRECACHE_WEAPON_REGISTER( tf_weapon_lunchbox_drink );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFLunchBox )
END_DATADESC()
#endif

#define LUNCHBOX_DROP_MODEL  "models/items/plate.mdl"
#define LUNCHBOX_STEAK_DROP_MODEL  "models/workshop/weapons/c_models/c_buffalo_steak/plate_buffalo_steak.mdl"
#define LUNCHBOX_ROBOT_DROP_MODEL  "models/items/plate_robo_sandwich.mdl"
#define LUNCHBOX_FESTIVE_DROP_MODEL  "models/items/plate_sandwich_xmas.mdl"
#define LUNCHBOX_CHOCOLATE_BAR_DROP_MODEL		"models/workshop/weapons/c_models/c_chocolate/plate_chocolate.mdl"
#define LUNCHBOX_BANANA_DROP_MODEL  "models/items/banana/plate_banana.mdl"
#define LUNCHBOX_FISHCAKE_DROP_MODEL	"models/workshop/weapons/c_models/c_fishcake/plate_fishcake.mdl"

#define LUNCHBOX_DROPPED_MINS	Vector( -17, -17, -10 )
#define LUNCHBOX_DROPPED_MAXS	Vector( 17, 17, 10 )

#define LUNCHBOX_BREAK_BODYGROUP 0
// Absolute body number of broken/not-broken since the server can't figure them out from the studiohdr.  Would only
// matter if we had other body groups going on anyway
#define LUNCHBOX_BODY_NOTBROKEN 0
#define LUNCHBOX_BODY_BROKEN 1

static const char *s_pszLunchboxMaxHealThink = "LunchboxMaxHealThink";

//=============================================================================
//
// Weapon Lunchbox functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFLunchBox::CTFLunchBox()
{
	m_bBroken = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::UpdateOnRemove( void )
{
#ifndef CLIENT_DLL
	// If we're removed, we remove any dropped powerups. This prevents an exploit
	// where they switch classes away & back to get another lunchbox to drop with.
	if ( m_hThrownPowerup )
	{
		UTIL_Remove( m_hThrownPowerup );
	}
#endif

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::Precache( void )
{
	if ( DropAllowed() )
	{
		PrecacheModel( "models/items/medkit_medium.mdl" );
		PrecacheModel( "models/items/medkit_medium_bday.mdl" );
		PrecacheModel( LUNCHBOX_DROP_MODEL );
		PrecacheModel( LUNCHBOX_STEAK_DROP_MODEL );
		PrecacheModel( LUNCHBOX_ROBOT_DROP_MODEL );
		PrecacheModel( LUNCHBOX_FESTIVE_DROP_MODEL );
		PrecacheModel( LUNCHBOX_CHOCOLATE_BAR_DROP_MODEL );
		PrecacheModel( LUNCHBOX_BANANA_DROP_MODEL );
		PrecacheModel( LUNCHBOX_FISHCAKE_DROP_MODEL );
	}

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::WeaponReset( void )
{
	BaseClass::WeaponReset();

	if ( !GetOwner() || !GetOwner()->IsAlive() )
	{
		m_bBroken = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLunchBox::UsesPrimaryAmmo( void )
{
	return CBaseCombatWeapon::UsesPrimaryAmmo();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLunchBox::DropAllowed( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		if ( pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !HasAmmo() )
		return;

#if GAME_DLL
	pOwner->Taunt();
	m_flNextPrimaryAttack = pOwner->GetTauntRemoveTime() + 0.1f;
#else
	m_flNextPrimaryAttack = gpGlobals->curtime + 2.0f; // this will be corrected by the game server
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::SecondaryAttack( void )
{
	if ( !DropAllowed() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !HasAmmo() )
		return;

#ifndef CLIENT_DLL

	if ( m_hThrownPowerup )
	{
		UTIL_Remove( m_hThrownPowerup );
	}

	// Throw out the medikit
	Vector vecSrc = pPlayer->EyePosition() + Vector(0,0,-8);
	QAngle angForward = pPlayer->EyeAngles() + QAngle(-10,0,0);

	int nLunchBoxType = GetLunchboxType();

	const char *pszHealthKit;	
	switch ( nLunchBoxType )
	{
	case LUNCHBOX_CHOCOLATE_BAR:
	case LUNCHBOX_BANANA:
	case LUNCHBOX_FISHCAKE:
		pszHealthKit = "item_healthkit_small";
		break;

	case LUNCHBOX_ADDS_AMMO:
		pszHealthKit = "item_healthammokit";
		break;

	default:
		pszHealthKit = "item_healthkit_medium";
	}

	CHealthKit *pMedKit = assert_cast<CHealthKit*>( CBaseEntity::Create( pszHealthKit, vecSrc, angForward, pPlayer ) );

	if ( pMedKit )
	{
		Vector vecForward, vecRight, vecUp;
		AngleVectors( angForward, &vecForward, &vecRight, &vecUp );
		Vector vecVelocity = vecForward * 500.0;
		
		if ( nLunchBoxType == LUNCHBOX_ADDS_MINICRITS )
		{
			pMedKit->SetModel( LUNCHBOX_STEAK_DROP_MODEL );
		}
		else if ( nLunchBoxType == LUNCHBOX_STANDARD_ROBO )
		{
			pMedKit->SetModel( LUNCHBOX_ROBOT_DROP_MODEL );
			pMedKit->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
		}
		else if ( nLunchBoxType == LUNCHBOX_STANDARD_FESTIVE )
		{
			pMedKit->SetModel( LUNCHBOX_FESTIVE_DROP_MODEL );
			pMedKit->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
		}
		else if ( nLunchBoxType == LUNCHBOX_CHOCOLATE_BAR )
		{
			pMedKit->SetModel( LUNCHBOX_CHOCOLATE_BAR_DROP_MODEL );
			pMedKit->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
		}
		else if ( nLunchBoxType == LUNCHBOX_BANANA )
		{
			pMedKit->SetModel( LUNCHBOX_BANANA_DROP_MODEL );
		}
		else if ( nLunchBoxType == LUNCHBOX_FISHCAKE )
		{
			pMedKit->SetModel( LUNCHBOX_FISHCAKE_DROP_MODEL );
			pMedKit->m_nSkin = ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
		}
		else
		{
			pMedKit->SetModel( LUNCHBOX_DROP_MODEL );
		}

		// clear out the overrides so the thrown sandvich/steak look correct in either vision mode
		pMedKit->ClearModelIndexOverrides();

		pMedKit->SetAbsAngles( vec3_angle );
		pMedKit->SetSize( LUNCHBOX_DROPPED_MINS, LUNCHBOX_DROPPED_MAXS );

		// the thrower has to wait 0.3 to pickup the powerup (so he can throw it while running forward)
		pMedKit->DropSingleInstance( vecVelocity, pPlayer, 0.3 );
	}

	m_hThrownPowerup = pMedKit;
#endif

	pPlayer->RemoveAmmo( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iPrimaryAmmoType );
	g_pGameRules->SwitchToNextBestWeapon( pPlayer, this );

	pPlayer->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, 0.f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::DrainAmmo( bool bForceCooldown )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

#ifdef GAME_DLL

	int iLunchboxType = GetLunchboxType();

	// If we're damaged while eating/taunting, bForceCooldown will be true
	if ( pOwner->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		if ( pOwner->GetHealth() < pOwner->GetMaxHealth() || GetLunchboxType() == LUNCHBOX_ADDS_MINICRITS || iLunchboxType == LUNCHBOX_CHOCOLATE_BAR || iLunchboxType == LUNCHBOX_FISHCAKE || bForceCooldown )
		{
			pOwner->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, 0.f );
		}
		else	// Full health regular sandwhich, I can eat forever
		{	
			return;
		}
	}
	else if ( pOwner->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		StartEffectBarRegen();
	}

	// Strange Tracking.  Only go through if we have ammo at this point.
	if ( !pOwner->IsBot() && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		EconEntity_OnOwnerKillEaterEventNoPartner( dynamic_cast<CEconEntity *>( this ), pOwner, kKillEaterEvent_FoodEaten );
	}

	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
#else
	
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	if ( pOwner->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		StartEffectBarRegen();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::Detach( void )
{
#ifdef GAME_DLL
	// Terrible - but for now, we're the only place that adds this (custom) attribute
	if ( GetLunchboxType() == LUNCHBOX_CHOCOLATE_BAR || GetLunchboxType() == LUNCHBOX_FISHCAKE )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( pOwner )
		{
			// Prevents use-then-switch-class exploit (heavy->scout)
			// Not a big deal in pubs, but it can mess with competitive
			pOwner->RemoveCustomAttribute( "hidden maxhealth non buffed" );
		}
	}
#endif

	BaseClass::Detach();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLunchBox::Holster( CBaseCombatWeapon *pSwitchingTo /* = NULL */ )
{ 
//	SetBroken( false );

	return BaseClass::Holster( pSwitchingTo );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFLunchBox::ApplyBiteEffects( CTFPlayer *pPlayer )
{
	SetBroken( true );

	int nLunchBoxType = GetLunchboxType();

	const float DALOKOHS_MAXHEALTH_BUFF = 50.f;

	if ( nLunchBoxType == LUNCHBOX_CHOCOLATE_BAR || nLunchBoxType == LUNCHBOX_FISHCAKE )
	{
		// add 50 health to player for 30 seconds
		pPlayer->AddCustomAttribute( "hidden maxhealth non buffed", DALOKOHS_MAXHEALTH_BUFF, 30.f );
	}
	else if ( nLunchBoxType == LUNCHBOX_ADDS_MINICRITS )
	{
		static const float s_fSteakSandwichDuration = 16.0f;

		// Steak sandvich.
		pPlayer->m_Shared.AddCond( TF_COND_ENERGY_BUFF, s_fSteakSandwichDuration );
		pPlayer->m_Shared.AddCond( TF_COND_CANNOT_SWITCH_FROM_MELEE, s_fSteakSandwichDuration );
		pPlayer->m_Shared.SetBiteEffectWasApplied();

		return;
	}
	
	// Then heal the player
	int iHeal = ( nLunchBoxType == LUNCHBOX_CHOCOLATE_BAR || nLunchBoxType == LUNCHBOX_FISHCAKE ) ? 25 : 75;
	int iHealType = DMG_GENERIC;
	if ( ( nLunchBoxType == LUNCHBOX_CHOCOLATE_BAR || nLunchBoxType == LUNCHBOX_FISHCAKE ) && pPlayer->GetHealth() < ( 300.f + DALOKOHS_MAXHEALTH_BUFF ) )
	{
		iHealType = DMG_IGNORE_MAXHEALTH;
		iHeal = Min( 25, 350 - pPlayer->GetHealth() );
	}

	float flHealScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flHealScale, lunchbox_healing_scale );
	iHeal = iHeal * flHealScale;

	int iHealed = pPlayer->TakeHealth( iHeal, iHealType );

	if ( iHealed > 0 )
	{
		CTF_GameStats.Event_PlayerHealedOther( pPlayer, iHealed );
	}

	// Restore ammo if applicable
	if ( nLunchBoxType == LUNCHBOX_ADDS_AMMO )
	{
		int maxPrimary = pPlayer->GetMaxAmmo( TF_AMMO_PRIMARY );
		pPlayer->GiveAmmo( maxPrimary * 0.25, TF_AMMO_PRIMARY, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox::OnResourceMeterFilled()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	pOwner->GiveAmmo( 1, m_iPrimaryAmmoType, false, kAmmoSource_ResourceMeter );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CTFLunchBox::SwitchBodyGroups( void )
{
	int iState = 0;

	if ( m_bBroken )
	{
		iState = 1;
	}

#ifdef CLIENT_DLL
	// We'll successfully predict m_nBody along with m_bBroken, but this can be called outside prediction, in which case
	// we want to use the networked m_nBody value -- but still fixup our viewmodel which is clientside only.
	if ( prediction->InPrediction() )
	{
		SetBodygroup( LUNCHBOX_BREAK_BODYGROUP, iState );
	}

	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
	{
		C_BaseAnimating *pViewWpn = GetAppropriateWorldOrViewModel();
		if ( pViewWpn != this )
		{
			pViewWpn->SetBodygroup( LUNCHBOX_BREAK_BODYGROUP, iState );
		}
	}
#else // CLIENT_DLL
	m_nBody = iState ? LUNCHBOX_BODY_BROKEN : LUNCHBOX_BODY_NOTBROKEN;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
bool CTFLunchBox::UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState )
{
	SwitchBodyGroups();

	return BaseClass::UpdateBodygroups( pOwner, iState );
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CTFLunchBox::SetBroken( bool bBroken )
{
	m_bBroken = bBroken;
	SwitchBodyGroups();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
/* static */ void CTFLunchBox::RecvProxy_Broken( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CTFLunchBox* pLunchBox = ( CTFLunchBox* ) pStruct;

	if ( !!pData->m_Value.m_Int != pLunchBox->m_bBroken )
	{
		pLunchBox->m_bBroken = !!pData->m_Value.m_Int;
		pLunchBox->SwitchBodyGroups();
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:  Energy Drink
//-----------------------------------------------------------------------------
CTFLunchBox_Drink::CTFLunchBox_Drink()
{
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFLunchBox_Drink::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner && pOwner->IsLocalPlayer() )
	{
		C_BaseEntity *pParticleEnt = pOwner->GetViewModel(0);
		if ( pParticleEnt )
		{
			pOwner->StopViewModelParticles( pParticleEnt );
		}
	}

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFLunchBox_Drink::ModifyEventParticles( const char* token )
{
	if ( GetLunchboxType() == LUNCHBOX_ADDS_MINICRITS )
	{
		if ( FStrEq( token, "energydrink_splash") )
		{
			CEconItemView *pItem = m_AttributeManager.GetItem();
			int iSystems = pItem->GetStaticData()->GetNumAttachedParticles( GetTeamNumber() );
			for ( int i = 0; i < iSystems; i++ )
			{
				attachedparticlesystem_t *pSystem = pItem->GetStaticData()->GetAttachedParticleData( GetTeamNumber(),i );
				if ( pSystem->iCustomType == 1 )
				{
					return pSystem->pszSystemName;
				}
			}
		}
	}

	return BaseClass::ModifyEventParticles( token );
}

#endif // CLIENT_DLL
