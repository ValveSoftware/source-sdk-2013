//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_fx_shared.h"
#include "tf_weapon_sniperrifle.h"
#include "in_buttons.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"
#include "client_virtualreality.h"
#include "sourcevr/isourcevirtualreality.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );
#else
#include "tf_gamerules.h"
#include "tf_fx.h"
#endif

#define TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC	50.0
#define TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC	75.0
#define	TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN		50
#define TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX		150
#define TF_WEAPON_SNIPERRIFLE_RELOAD_TIME		1.5f
#define TF_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

#define TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME	0.2f

#define SNIPER_DOT_SPRITE_RED		"effects/sniperdot_red.vmt"
#define SNIPER_DOT_SPRITE_BLUE		"effects/sniperdot_blue.vmt"
#define SNIPER_CHARGE_BEAM_RED		"tfc_sniper_charge_red"
#define SNIPER_CHARGE_BEAM_BLUE		"tfc_sniper_charge_blue"

#ifdef CLIENT_DLL
ConVar tf_sniper_fullcharge_bell( "tf_sniper_fullcharge_bell", "0", FCVAR_ARCHIVE );
#endif

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifle, DT_TFSniperRifle )

BEGIN_NETWORK_TABLE_NOBASE( CTFSniperRifle, DT_SniperRifleLocalData )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO(m_flChargedDamage), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO(m_flChargedDamage) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFSniperRifle, DT_TFSniperRifle )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "SniperRifleLocalData", 0, &REFERENCE_SEND_TABLE( DT_SniperRifleLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "SniperRifleLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SniperRifleLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flUnzoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flRezoomTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_bRezoomAfterShot, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_flChargedDamage, FIELD_FLOAT, 0 ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle, CTFSniperRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle );

BEGIN_DATADESC( CTFSniperRifle )
DEFINE_FIELD( m_flUnzoomTime, FIELD_FLOAT ),
DEFINE_FIELD( m_flRezoomTime, FIELD_FLOAT ),
DEFINE_FIELD( m_bRezoomAfterShot, FIELD_BOOLEAN ),
DEFINE_FIELD( m_flChargedDamage, FIELD_FLOAT ),
END_DATADESC()

//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifleDecap, DT_TFSniperRifleDecap )

BEGIN_NETWORK_TABLE( CTFSniperRifleDecap, DT_TFSniperRifleDecap )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifleDecap )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle_decap, CTFSniperRifleDecap );
PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle_decap );

//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifleClassic, DT_TFSniperRifleClassic )

BEGIN_NETWORK_TABLE( CTFSniperRifleClassic, DT_TFSniperRifleClassic )
#if !defined( CLIENT_DLL )
	SendPropBool( SENDINFO(m_bCharging) ),
#else
	RecvPropBool( RECVINFO(m_bCharging) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifleClassic )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bCharging, FIELD_BOOLEAN, 0 ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle_classic, CTFSniperRifleClassic );
PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle_classic );
//=============================================================================


//=============================================================================
//
// Weapon Sniper Rifles functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	m_hSniperDot = NULL;
#else
	m_bPlayedBell = false;
#endif

	m_bCurrentShotIsHeadshot = false;
	m_flChargedDamage = 0.0f;
	m_flChargePerSec = TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;

	m_bWasAimedAtEnemy = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::~CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	DestroySniperDot();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::Spawn()
{
	m_iAltFireHint = HINT_ALTFIRE_SNIPERRIFLE;
	BaseClass::Spawn();

	ResetTimers();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::Precache()
{
	BaseClass::Precache();
	PrecacheModel( SNIPER_DOT_SPRITE_RED );
	PrecacheModel( SNIPER_DOT_SPRITE_BLUE );

	PrecacheScriptSound( "doomsday.warhead" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ResetTimers( void )
{
	SetInternalUnzoomTime( -1 );
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Reload( void )
{
	// We currently don't reload.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::CanHolster( void ) const
{
 	CTFPlayer *pPlayer = GetTFPlayerOwner();
 	if ( pPlayer )
	{
		// TF_COND_MELEE_ONLY need to be able to immediately holster and switch to melee weapon
		if ( pPlayer->m_Shared.InCond( TF_COND_MELEE_ONLY ) )
			return true;

		// don't allow us to holster this weapon if we're in the process of zooming and 
		// we've just fired the weapon (next primary attack is only 1.5 seconds after firing)
		if ( ( pPlayer->GetFOV() < pPlayer->GetDefaultFOV() ) && ( m_flNextPrimaryAttack > gpGlobals->curtime ) )
			return false;
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		ZoomOut();
	}

	m_flChargedDamage = 0.0f;
#ifdef CLIENT_DLL
	m_bPlayedBell = false;
#endif
	ResetTimers();

	return BaseClass::Holster( pSwitchingTo );
}

void CTFSniperRifle::WeaponReset( void )
{
	BaseClass::WeaponReset();

	ZoomOut();

	m_bCurrentShotIsHeadshot = false;
	m_flChargePerSec = TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::HandleZooms( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Handle the zoom when taunting.
	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			ToggleZoom();
		}

		//Don't rezoom in the middle of a taunt.
		ResetTimers();
	}

	if ( m_flUnzoomTime > 0 && gpGlobals->curtime > m_flUnzoomTime )
	{
		if ( m_bRezoomAfterShot )
		{
			ZoomOutIn();
			m_bRezoomAfterShot = false;
		}
		else
		{
			ZoomOut();
		}

		SetInternalUnzoomTime( -1 );
	}

	if ( m_flRezoomTime > 0 )
	{
		if ( gpGlobals->curtime > m_flRezoomTime )
		{
            ZoomIn();
			m_flRezoomTime = -1;
		}
	}

	if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		// If we're in the process of rezooming, just cancel it
		if ( m_flRezoomTime > 0 || m_flUnzoomTime > 0 )
		{
			// Prevent them from rezooming in less time than they would have
			m_flNextSecondaryAttack = m_flRezoomTime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
			m_flRezoomTime = -1;
		}
		else
		{
			Zoom();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::ItemPostFrame( void )
{
	// If we're lowered, we're not allowed to fire
	if ( m_bLowered )
		return;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	// Update the sniper dot position if we have one
	if ( m_hSniperDot )
	{
		UpdateSniperDot();
	}
#endif

	// Start charging when we're zoomed in, and allowed to fire
	if ( pPlayer->m_Shared.IsJumping() )
	{
		// Unzoom if we're jumping
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		m_flChargedDamage = 0.0f;
		m_bRezoomAfterShot = false;
		m_flRezoomTime = -1.f;
	}
	else if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		// Don't start charging in the time just after a shot before we unzoom to play rack anim.
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && !m_bRezoomAfterShot )
		{
			float fSniperRifleChargePerSec = m_flChargePerSec;
			ApplyChargeSpeedModifications( fSniperRifleChargePerSec );
			fSniperRifleChargePerSec += SniperRifleChargeRateMod();

			// we don't want sniper charge rate to go too high.
			fSniperRifleChargePerSec = clamp( fSniperRifleChargePerSec, 0, 2.f * TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC );

			m_flChargedDamage = MIN( m_flChargedDamage + gpGlobals->frametime * fSniperRifleChargePerSec, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX );

#ifdef CLIENT_DLL
			// play the recharged bell if we're fully charged
			if ( IsFullyCharged() && !m_bPlayedBell )
			{
				m_bPlayedBell = true;
				if ( tf_sniper_fullcharge_bell.GetBool() )
				{
					C_TFPlayer::GetLocalTFPlayer()->EmitSound( "TFPlayer.ReCharged" );
				}
			}
#endif
		}
		else
		{
			m_flChargedDamage = MAX( 0, m_flChargedDamage - gpGlobals->frametime * TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC );
		}
	}

	// Fire.
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		Fire( pPlayer );
	}

	// Idle.
	if ( !( ( pPlayer->m_nButtons & IN_ATTACK) || ( pPlayer->m_nButtons & IN_ATTACK2 ) ) )
	{
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}

	// Sniper Rage (Hitman's heatmaker)
	// Activate on 'R'
	// no longer need full charge
	if ( (pPlayer->m_nButtons & IN_RELOAD ) && pPlayer->m_Shared.GetRageMeter() > 1.0f )
	{
		int iBuffType = 0;
		CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );
		if ( iBuffType > 0 )
		{
			pPlayer->m_Shared.ActivateRageBuff( pPlayer, iBuffType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::PlayWeaponShootSound( void )
{
	if ( TFGameRules()->GameModeUsesUpgrades() )
	{
		PlayUpgradedShootSound( "Weapon_Upgrade.DamageBonus" );
	}

	if ( !IsFullyCharged() )
	{
		float flDamageBonus = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flDamageBonus, sniper_full_charge_damage_bonus );
		if ( flDamageBonus > 1.0f )
		{
			WeaponSound( SPECIAL3 );
			return;
		}
	}

	BaseClass::PlayWeaponShootSound();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFSniperRifle::Zoom( void )
{
	// Don't allow the player to zoom in while jumping
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsJumping() )
	{
		if ( pPlayer->GetFOV() >= 75 )
			return;
	}

	ToggleZoom();

	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextPrimaryAttack = MAX( m_flNextPrimaryAttack, gpGlobals->curtime + 0.1 );
	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ZoomOutIn( void )
{
	ZoomOut();

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->ShouldAutoRezoom() )
	{
		float flRezoomDelay = 0.9f;
		if ( !UsesClipsForAmmo1() )
		{
			// Since sniper rifles don't actually use clips the fast reload hook also affects unzoom and zoom delays
			ApplyScopeSpeedModifications( flRezoomDelay );
		}
		m_flRezoomTime = gpGlobals->curtime + flRezoomDelay;
	}
	else
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ZoomIn( void )
{
	// Start aiming.
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	BaseClass::ZoomIn();

	pPlayer->m_Shared.AddCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	// Create the sniper dot.
	CreateSniperDot();
	pPlayer->ClearExpression();
#endif
}


//-----------------------------------------------------------------------------
bool CTFSniperRifle::IsZoomed( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TF_COND_ZOOMED );
	}

	return false;
}


//-----------------------------------------------------------------------------
//
// Have we been zoomed in long enough for our shot to do max damage
//
bool CTFSniperRifle::IsFullyCharged( void ) const
{
	return m_flChargedDamage >= TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ZoomOut( void )
{
	BaseClass::ZoomOut();

	// Stop aiming
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
	pPlayer->ClearExpression();
#endif

	// if we are thinking about zooming, cancel it
	SetInternalUnzoomTime( -1 );
	m_flRezoomTime = -1;
	m_bRezoomAfterShot = false;
	m_flChargedDamage = 0.0f;

#ifdef CLIENT_DLL
	m_bPlayedBell = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::ApplyScopeSpeedModifications( float &flBaseRef )
{
	CALL_ATTRIB_HOOK_FLOAT( flBaseRef, fast_reload );

	// Prototype hack
	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_HASTE || pPlayer->m_Shared.GetCarryingRuneType() == RUNE_PRECISION )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) )
			{
				flBaseRef *= 0.75f;
			}
			else
			{
				flBaseRef *= 0.5f;
			}
		}
		else if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KING || pPlayer->m_Shared.InCond( TF_COND_KING_BUFFED ) )
		{
			flBaseRef *= 0.75f;
		}
		
		int iMaster = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iMaster, ability_master_sniper );
		if ( iMaster )
		{
			flBaseRef *= RemapValClamped( iMaster, 1, 2, 0.6f, 0.3f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::ApplyChargeSpeedModifications( float &flBaseRef )
{
	CALL_ATTRIB_HOOK_FLOAT( flBaseRef, mult_sniper_charge_per_sec );

	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer )
	{
		Vector vForward;
		AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vForward );

		Vector vShootPos = pPlayer->Weapon_ShootPosition();
		trace_t tr;
		UTIL_TraceLine( vShootPos, vShootPos + vForward * m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flRange, MASK_BLOCKLOS_AND_NPCS, pPlayer, COLLISION_GROUP_NONE, &tr );
		
		CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
		if ( pTarget && pTarget->IsAlive() && pTarget->GetTeamNumber() != pPlayer->GetTeamNumber() && 
			 !( pTarget->m_Shared.IsStealthed() && !pTarget->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ) )
		{
			CALL_ATTRIB_HOOK_FLOAT( flBaseRef, mult_sniper_charge_per_sec_with_enemy_under_crosshair );

			int nBeep = 0;
			CALL_ATTRIB_HOOK_FLOAT( nBeep, sniper_beep_with_enemy_under_crosshair );

			if ( nBeep > 0 && !m_bWasAimedAtEnemy )
			{
				pPlayer->EmitSound( "doomsday.warhead" );
			}

			m_bWasAimedAtEnemy = true;
		}
		else
		{
			m_bWasAimedAtEnemy = false;
		}

		if ( pPlayer && ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_PRECISION || pPlayer->m_Shared.GetCarryingRuneType() == RUNE_HASTE ) )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) )
			{
				flBaseRef *= 2.0f;
			}
			else
			{
				flBaseRef *= 3.0f;
			}
		}
		else if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_KING || pPlayer->m_Shared.InCond( TF_COND_KING_BUFFED ) )
		{
			flBaseRef *= 1.5f;
		}


		// Prototype hack
		int iMaster = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iMaster, ability_master_sniper );
		if ( iMaster )
		{
			flBaseRef *= RemapValClamped( iMaster, 1, 2, 1.5f, 3.f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::MustBeZoomedToFire( void )
{
	int iModOnlyFireZoomed = 0;
	CALL_ATTRIB_HOOK_INT( iModOnlyFireZoomed, sniper_only_fire_zoomed );
	return ( iModOnlyFireZoomed != 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::HandleNoScopeFireDeny( void )
{ 
	if ( m_flNextEmptySoundTime < gpGlobals->curtime )
	{
		WeaponSound( SPECIAL2 );

#ifdef CLIENT_DLL
		ParticleProp()->Init( this );
		ParticleProp()->Create( "dxhr_sniper_fizzle", PATTACH_POINT_FOLLOW, "muzzle" );
#endif

		m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::SetInternalUnzoomTime( float flUnzoomTime )
{
#ifdef GAME_DLL
	if ( m_flUnzoomTime == flUnzoomTime )
		return;

	if ( flUnzoomTime > gpGlobals->curtime )
	{
		DisableJump();
	}
	else
	{
		EnableJump();
	}
#endif // GAME_DLL

	m_flUnzoomTime = flUnzoomTime;
}
  
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ETFDmgCustom CTFSniperRifle::GetPenetrateType() const
{
	if ( IsFullyCharged() )
	{
		int iPenetrate = 0;
		CALL_ATTRIB_HOOK_INT( iPenetrate, sniper_penetrate_players_when_charged ); 
		if ( iPenetrate > 0 )
			return TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS;
	}

	return BaseClass::GetPenetrateType();
}

#ifdef SIXENSE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetRezoomTime() const
{
	return m_flRezoomTime;
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::Fire( CTFPlayer *pPlayer )
{
	// Check the ammo.  We don't use clip ammo, check the primary ammo type.
	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		HandleFireOnEmpty();
		return;
	}

	// Some weapons can only fire while zoomed
	if ( MustBeZoomedToFire() )
	{
		if ( !IsZoomed() )
		{
			HandleNoScopeFireDeny();
			return;
		}
	}

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Fire the sniper shot.
	PrimaryAttack();

	if ( IsZoomed() )
	{
		// If we have more bullets, zoom out, play the bolt animation and zoom back in
		if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
		{
			// do not zoom out if we're under rage or about to enter it
			if ( !( pPlayer->m_Shared.InCond( TF_COND_SNIPERCHARGE_RAGE_BUFF ) ) )
			{
				float flUnzoomDelay = 0.5f;
				if ( !UsesClipsForAmmo1() )
				{
					// Since sniper rifles don't actually use clips the fast reload hook also affects unzoom and zoom delays
					ApplyScopeSpeedModifications( flUnzoomDelay );
				}
				SetRezoom( true, flUnzoomDelay );	// zoom out in 0.5 seconds, then rezoom
#ifdef GAME_DLL
				SetContextThink( &CTFSniperRifleClassic::EnableJump, gpGlobals->curtime + flUnzoomDelay, "RenableJump" );
#endif
			}
		}
		else	
		{
			//just zoom out
			SetRezoom( false, 0.5f );	// just zoom out in 0.5 seconds
		}
	}
	else
	{
		float flZoomDelay = SequenceDuration();

		// Since sniper rifles don't actually use clips the fast reload hook also affects zoom delays
		ApplyScopeSpeedModifications( flZoomDelay );

		// Prevent primary fire preventing zooms
		m_flNextSecondaryAttack = gpGlobals->curtime + flZoomDelay;
	}

	m_flChargedDamage = 0.0f;

#ifdef GAME_DLL
	if ( m_hSniperDot )
	{
		m_hSniperDot->ResetChargeTime();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::SetRezoom( bool bRezoom, float flDelay )
{
	SetInternalUnzoomTime( gpGlobals->curtime + flDelay );

	m_bRezoomAfterShot = bRezoom;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetProjectileDamage( void )
{
	float flDamage = MAX( m_flChargedDamage, TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN );

	float flDamageMod = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flDamageMod, mult_dmg );
	flDamage *= flDamageMod;

	
	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() ); 

		if ( pPlayer && pPlayer->m_Shared.GetCarryingRuneType() == RUNE_PRECISION )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) )
			{
				flDamage *= 1.5f;
			}
			else
			{
				flDamage *= 2.f;
			}
		}
	}

	if ( IsFullyCharged() )
	{
		CALL_ATTRIB_HOOK_FLOAT( flDamage, sniper_full_charge_damage_bonus );
	}

	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFSniperRifle::GetDamageType( void ) const
{
	// Only do hit location damage if we're zoomed
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		return BaseClass::GetDamageType();

	int nDamageType = BaseClass::GetDamageType() & ~DMG_USE_HITLOCATIONS;

	return nDamageType;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::CreateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Check to see if we have already been created?
	if ( m_hSniperDot )
		return;

	// Get the owning player (make sure we have one).
	CBaseCombatCharacter *pPlayer = GetOwner();
	if ( !pPlayer )
		return;

	// Create the sniper dot, but do not make it visible yet.
	m_hSniperDot = CSniperDot::Create( GetAbsOrigin(), pPlayer, true );
	m_hSniperDot->ChangeTeam( pPlayer->GetTeamNumber() );

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::DestroySniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Destroy the sniper dot.
	if ( m_hSniperDot )
	{
		UTIL_Remove( m_hSniperDot );
		m_hSniperDot = NULL;
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::UpdateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Get the start and endpoints.
	Vector vecMuzzlePos = pPlayer->Weapon_ShootPosition();
	Vector forward;
	pPlayer->EyeVectors( &forward );
	Vector vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );

	trace_t	trace;
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, ( MASK_SHOT & ~CONTENTS_WINDOW ), GetOwner(), COLLISION_GROUP_NONE, &trace );

	// Update the sniper dot.
	if ( m_hSniperDot )
	{
		CBaseEntity *pEntity = NULL;
		if ( trace.DidHitNonWorldEntity() )
		{
			pEntity = trace.m_pEnt;
			if ( !pEntity || !pEntity->m_takedamage )
			{
				pEntity = NULL;
			}
		}

		m_hSniperDot->Update( pEntity, trace.endpos, trace.plane.normal );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::CanFireCriticalShot( bool bIsHeadshot, CBaseEntity *pTarget /*= NULL*/ )
{
	m_bCurrentAttackIsCrit = false;
	m_bCurrentShotIsHeadshot = false;

	if ( !BaseClass::CanFireCriticalShot( bIsHeadshot, pTarget ) )
		return false;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsCritBoosted() )
	{
		m_bCurrentShotIsHeadshot = bIsHeadshot;
		return true;
	}

	// If we don't auto crit on a headshot, use standard criteria to determine other crits.
	if ( GetRifleType() == RIFLE_JARATE )
	{
		return false; // Never auto crit on headshot.
	}

	// can only fire a crit shot if this is a headshot, unless we're critboosted
	if ( !bIsHeadshot )
		return false;

	int iFullChargeHeadShotPenalty = 0;
	CALL_ATTRIB_HOOK_INT( iFullChargeHeadShotPenalty, sniper_no_headshot_without_full_charge );
	if ( iFullChargeHeadShotPenalty != 0 )
	{
		if ( !IsFullyCharged() )
			return false;
	}

	int iCanCritNoScope = 0;
	CALL_ATTRIB_HOOK_INT( iCanCritNoScope, sniper_crit_no_scope );
	if ( iCanCritNoScope == 0 )
	{
		if ( pPlayer )
		{
			// no crits if they're not zoomed
			if ( pPlayer->GetFOV() >= pPlayer->GetDefaultFOV() )
			{
				return false;
			}

			// no crits for 0.2 seconds after starting to zoom
			if ( ( gpGlobals->curtime - pPlayer->GetFOVTime() ) < TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME )
			{
				return false;
			}
		}
	}

	m_bCurrentAttackIsCrit = true;
	m_bCurrentShotIsHeadshot = bIsHeadshot;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Our owner was stunned.
//-----------------------------------------------------------------------------
void CTFSniperRifle::OnControlStunned( void )
{
	BaseClass::OnControlStunned();

	ZoomOut();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFSniperRifle::GetCustomDamageType() const
{
	if ( IsJarateRifle() )
	{
		return TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE;
	}

	return TF_DMG_CUSTOM_PENETRATE_MY_TEAM;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::Detach( void )
{
	if ( IsZoomed() )
	{
		ToggleZoom();
	}

	BaseClass::Detach();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::OnPlayerKill( pVictim, info );

	if ( m_iConsecutiveKills == 3 )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pPlayer )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_SNIPER_RIFLE_NO_MISSING );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::OnBulletFire( int iEnemyPlayersHit )
{
	BaseClass::OnBulletFire( iEnemyPlayersHit );

	// Did we completely miss?
	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if( iEnemyPlayersHit == 0 && pPlayer && pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		EconEntity_OnOwnerKillEaterEventNoPartner( assert_cast<CEconEntity *>( this ), pPlayer, kKillEaterEvent_NEGATIVE_SniperShotsMissed );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ExplosiveHeadShot( CTFPlayer *pAttacker, CTFPlayer *pVictim )
{
	if ( !pAttacker )
		return;

	if ( !pVictim )
		return;

	int iExplosiveShot = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER ( pAttacker, iExplosiveShot, explosive_sniper_shot );

	// Stun the source
	float flStunDuration = 1.f + ( ( (float)iExplosiveShot - 1.f ) * 0.5f );
	float flStunAmt = pVictim->IsMiniBoss() ? 0.5f : RemapValClamped( iExplosiveShot, 1, 3, 0.5f, 0.8f );
	pVictim->m_Shared.StunPlayer( flStunDuration, flStunAmt, TF_STUN_MOVEMENT, pAttacker );

	// Generate an explosion and look for nearby bots
	float flDmgRange = 125.f + iExplosiveShot * 25.f;
	float flDmg = 130.f + iExplosiveShot * 20.f;

	CBaseEntity	*pObjects[MAX_PLAYERS_ARRAY_SAFE ];
	int nCount = UTIL_EntitiesInSphere( pObjects, ARRAYSIZE( pObjects ), pVictim->GetAbsOrigin(), flDmgRange, FL_CLIENT );
	for ( int i = 0; i < nCount; i++ )
	{
		if ( !pObjects[i] )
			continue;

		if ( !pObjects[i]->IsAlive() )
			continue;

		if ( pObjects[i] == pVictim )
			continue;

		if ( pAttacker->InSameTeam( pObjects[i] ) )
			continue;

		if ( !pVictim->FVisible( pObjects[i], MASK_OPAQUE ) )
			continue;

		CTFPlayer *pTFPlayer = static_cast<CTFPlayer *>( pObjects[i] );
		if ( !pTFPlayer )
			continue;

		if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) || pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
			continue;

		if ( pTFPlayer->m_Shared.IsInvulnerable() )
			continue;

		// Stun			
		flStunAmt = pTFPlayer->IsMiniBoss() ? 0.5f : RemapValClamped( iExplosiveShot, 1, 3, 0.5f, 0.8f );
		pTFPlayer->m_Shared.StunPlayer( flStunDuration, flStunAmt, TF_STUN_MOVEMENT, pAttacker );

		// DoT
		pTFPlayer->m_Shared.MakeBleed( pAttacker, this, 0.1f, flDmg );

		// Shoot a beam at them
		CPVSFilter filter( pTFPlayer->WorldSpaceCenter() );
		Vector vStart = pVictim->EyePosition();
		Vector vEnd = pTFPlayer->EyePosition();
		te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
		TE_TFParticleEffectComplex( filter, 0.0f, "dxhr_arm_muzzleflash", vStart, QAngle( 0, 0, 0 ), NULL, &controlPoint, pTFPlayer, PATTACH_CUSTOMORIGIN );

		pTFPlayer->EmitSound( "Weapon_Upgrade.ExplosiveHeadshot" );
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::IsJarateRifle( void ) const
{
	return GetJarateTimeInternal() > 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetJarateTime( void ) const
{
	if ( m_flChargedDamage > 0.f )
	{
		return GetJarateTimeInternal();
	}
	else
		return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetJarateTimeInternal( void ) const
{
	float flMaxJarateTime = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flMaxJarateTime, jarate_duration );
	if ( flMaxJarateTime > 0 )
	{
		const float flMinJarateTime = 2.f;
		float flDuration = RemapValClamped( m_flChargedDamage, TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX, flMinJarateTime, flMaxJarateTime );
		return flDuration;
	}

	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: UI Progress (same as GetProgress() without the division by 100.0f)
//-----------------------------------------------------------------------------
bool CTFSniperRifle::IsRageFull( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer ) 
	{
		return false;
	}

	return ( pPlayer->m_Shared.GetRageMeter() >= 100.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::EffectMeterShouldFlash( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
	{
		return false;
	}

	if ( pPlayer && ( IsRageFull() || pPlayer->m_Shared.IsRageDraining() ) )
	{
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: UI Progress
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer ) 
	{
		return 0.f;
	}

	return pPlayer->m_Shared.GetRageMeter() / 100.0f;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::ShouldEjectBrass()
{
	if ( GetJarateTimeInternal() > 0.f )
		return false;
	else
		return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetHUDDamagePerc( void )
{
	return (m_flChargedDamage / TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX);
}

//-----------------------------------------------------------------------------
// Returns the sniper chargeup from 0 to 1
//-----------------------------------------------------------------------------
class CProxySniperRifleCharge : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CProxySniperRifleCharge::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( GetSpectatorTarget() != 0 && GetSpectatorMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = (C_TFPlayer *)UTIL_PlayerByIndex( GetSpectatorTarget() );
	}

	if ( pPlayer )
	{
		CTFSniperRifle *pWeapon = assert_cast<CTFSniperRifle*>(pPlayer->GetActiveTFWeapon());
		if ( pWeapon )
		{
			float flChargeValue = ( ( 1.0 - pWeapon->GetHUDDamagePerc() ) * 0.8 ) + 0.6;

			VMatrix mat, temp;

			Vector2D center( 0.5, 0.5 );
			MatrixBuildTranslation( mat, -center.x, -center.y, 0.0f );

			// scale
			{
				Vector2D scale( 1.0f, 0.25f );
				MatrixBuildScale( temp, scale.x, scale.y, 1.0f );
				MatrixMultiply( temp, mat, mat );
			}

			MatrixBuildTranslation( temp, center.x, center.y, 0.0f );
			MatrixMultiply( temp, mat, mat );

			// translation
			{
				Vector2D translation( 0.0f, flChargeValue );
				MatrixBuildTranslation( temp, translation.x, translation.y, 0.0f );
				MatrixMultiply( temp, mat, mat );
			}

			m_pResult->SetMatrixValue( mat );
		}
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CProxySniperRifleCharge, IMaterialProxy, "SniperRifleCharge" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

//=============================================================================
//
// Laser Dot functions.
//

IMPLEMENT_NETWORKCLASS_ALIASED( SniperDot, DT_SniperDot )

BEGIN_NETWORK_TABLE( CSniperDot, DT_SniperDot )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeStartTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeStartTime ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( env_sniperdot, CSniperDot );

BEGIN_DATADESC( CSniperDot )
DEFINE_FIELD( m_vecSurfaceNormal,	FIELD_VECTOR ),
DEFINE_FIELD( m_hTargetEnt,			FIELD_EHANDLE ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CSniperDot::CSniperDot( void )
{
	m_vecSurfaceNormal.Init();
	m_hTargetEnt = NULL;

#ifdef CLIENT_DLL
	m_hSpriteMaterial = NULL;
	m_laserBeamEffect = NULL;
#endif


	ResetChargeTime();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CSniperDot::~CSniperDot( void )
{
#ifdef CLIENT_DLL
	if ( m_laserBeamEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_laserBeamEffect );

		m_laserBeamEffect = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CSniperDot
//-----------------------------------------------------------------------------
CSniperDot *CSniperDot::Create( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
// Client specific.
#ifdef CLIENT_DLL

	return NULL;

// Server specific.
#else

	// Create the sniper dot entity.
	CSniperDot *pDot = static_cast<CSniperDot*>( CBaseEntity::Create( "env_sniperdot", origin, QAngle( 0.0f, 0.0f, 0.0f ) ) );
	if ( !pDot )
		return NULL;

	//Create the graphic
	pDot->SetMoveType( MOVETYPE_NONE );
	pDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pDot->AddEffects( EF_NOSHADOW );
	UTIL_SetSize( pDot, -Vector( 4.0f, 4.0f, 4.0f ), Vector( 4.0f, 4.0f, 4.0f ) );

	// Set owner.
	pDot->SetOwnerEntity( pOwner );

	// Force updates even though we don't have a model.
	pDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );


	return pDot;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal )
{
	SetAbsOrigin( vecOrigin );
	m_vecSurfaceNormal = vecNormal;
	m_hTargetEnt = pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CSniperDot::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

bool CSniperDot::GetRenderingPositions( C_TFPlayer *pPlayer, Vector &vecAttachment, Vector &vecEndPos, float &flSize )
{
	if ( !pPlayer )
		return false;

	// Get the sprite rendering position.
	flSize = 6.0;
	bool bScaleSizeByDistance = false;

	const float c_fMaxSizeDistVR = 384.0f;
	const float	c_flMaxSizeDistUnzoomed = 200.0f;

	if ( !pPlayer->IsDormant() )
	{
		Vector vecDir;
		QAngle angles;

		float flDist = MAX_TRACE_LENGTH;

		// Always draw the dot in front of our faces when in first-person.
		if ( pPlayer->IsLocalPlayer() )
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();

			// Clamp the forward distance for the sniper's firstperson
			flDist = c_fMaxSizeDistVR;
			flSize = 2.0;

			// Make the dot bigger when charging and not zoomed in (The Classic)
			if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && !pPlayer->m_Shared.InCond( TF_COND_ZOOMED ))
			{
				flSize = 4.0f;
				bScaleSizeByDistance = true;
			}

			if ( UseVR() )
			{
				// The view direction is not exactly the same as the weapon direction because of stereo, calibration, etc.
				g_ClientVirtualReality.OverrideWeaponHudAimVectors ( &vecAttachment, &vecDir );

				// No clamping, thanks - we need the distance to be correct so that
				// vergence works properly, and we'll scale the size up accordingly.
				flDist = MAX_TRACE_LENGTH;
				bScaleSizeByDistance = true;
			}
		}
		else
		{
			// Take the owning player eye position and direction.
			vecAttachment = pPlayer->EyePosition();
			QAngle anglesEye = pPlayer->EyeAngles();
			AngleVectors( anglesEye, &vecDir );
		}

		trace_t tr;
		CTraceFilterIgnoreFriendlyCombatItems filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * flDist ), MASK_SHOT, &filter, &tr );

		// Backup off the hit plane, towards the source
		vecEndPos = tr.endpos + vecDir * -4;
		if ( UseVR() )
		{
			float fDist = ( vecEndPos - vecAttachment ).Length();
			if ( fDist > c_fMaxSizeDistVR )
			{
				// Scale the dot up so it's still visible in first person.
				flSize *= ( fDist * ( 1.0f / c_fMaxSizeDistVR ) );
			}
		}
		else if ( bScaleSizeByDistance )
		{
			float fDist = ( vecEndPos - vecAttachment ).Length();
			if ( fDist > c_flMaxSizeDistUnzoomed )
			{
				// Scale the dot up so it's still visible in first person.
				flSize *= ( fDist * ( 1.0f / c_flMaxSizeDistUnzoomed ) );
			}
		}
	}
	else
	{
		// Just use our position if we can't predict it otherwise.
		vecAttachment = GetAbsOrigin();
		vecEndPos = GetAbsOrigin();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// TFTODO: Make the sniper dot get brighter the more damage it will do.
//-----------------------------------------------------------------------------
int CSniperDot::DrawModel( int flags )
{
	// Get the owning player.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return -1;

	Vector vecAttachement;
	Vector vecEndPos;
	float flSize;

	if ( !GetRenderingPositions( pPlayer, vecAttachement, vecEndPos, flSize ) )
	{
		return -1;
	}

	// Draw our laser dot in space.
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );

	CTFWeaponBase *pBaseWeapon = pPlayer->GetActiveTFWeapon();
	CTFSniperRifle *pWeapon = dynamic_cast< CTFSniperRifle* >( pBaseWeapon );

	float flLifeTime = gpGlobals->curtime - m_flChargeStartTime;
	float flChargePerSec = TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;
	if ( pWeapon )
	{
		pWeapon->ApplyChargeSpeedModifications( flChargePerSec );
	}

	// Sniper Rage
	if ( pPlayer->m_Shared.InCond( TF_COND_SNIPERCHARGE_RAGE_BUFF ) ) 
	{
		flChargePerSec *= 1.25f;
	}

	float flStrength;

	if ( pWeapon )
	{
		flStrength = pWeapon->GetHUDDamagePerc();

		// FIXME: We should find out what's causing this and fix it.
		AssertMsg1( flStrength >= ( 0.0f - FLT_EPSILON ) && flStrength <= ( 1.0f + FLT_EPSILON ), "GetHUDDamagePerc returned out of range value: %f", flStrength );
		flStrength = clamp( flStrength, 0.1f, 1.0f );
	}
	else
	{
		flStrength = RemapValClamped( flLifeTime, 0.0, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX / flChargePerSec, 0.1f, 1.0f );
	}

	color32 innercolor = { 255, 255, 255, 255 };
	color32 outercolor = { 255, 255, 255, 128 };

	DrawSprite( vecEndPos, flSize, flSize, outercolor );
	DrawSprite( vecEndPos, flSize * flStrength, flSize * flStrength, innercolor );

	// Successful.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSniperDot::ShouldDraw( void )			
{
	if ( IsEffectActive( EF_NODRAW ) )
		return false;

	// Don't draw the sniper dot when in thirdperson.
	if ( ::input->CAM_IsThirdPerson() )
		return false;

	return true;
}

void CSniperDot::ClientThink( void )
{
	// snipers have laser sights in PvE mode
	if ( TFGameRules()->IsPVEModeActive() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pPlayer )
		{
			if ( !m_laserBeamEffect )
			{
				m_laserBeamEffect = ParticleProp()->Create( "laser_sight_beam", PATTACH_ABSORIGIN_FOLLOW );
			}

			if ( m_laserBeamEffect )
			{
				m_laserBeamEffect->SetSortOrigin( m_laserBeamEffect->GetRenderOrigin() );
				m_laserBeamEffect->SetControlPoint( 2, Vector( 0, 0, 255 ) );

				Vector vecAttachment;
				Vector vecEndPos;
				float flSize;

				if ( pPlayer->GetAttachment( "eye_1", vecAttachment ) )
				{
					m_laserBeamEffect->SetControlPoint( 1, vecAttachment );
				}
				else if ( GetRenderingPositions( pPlayer, vecAttachment, vecEndPos, flSize ) )
				{
					m_laserBeamEffect->SetControlPoint( 1, vecAttachment );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_BLUE, TEXTURE_GROUP_CLIENT_EFFECTS );
		}
		else
		{
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_RED, TEXTURE_GROUP_CLIENT_EFFECTS );
		}

		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

#endif // CLIENT_DLL

#ifdef GAME_DLL

void CTFSniperRifleDecap::OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::OnPlayerKill( pVictim, info );

	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer && IsHeadshot( info.GetDamageCustom() ) )
	{
		// If we got a headshot kill, increment our number of decapitations.
		int iDecaps = pPlayer->m_Shared.GetDecapitations() + 1;
		pPlayer->m_Shared.SetDecapitations( iDecaps );
	}
}

#endif // GAME_DLL

static const int MAX_HEAD_BONUS = 6;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifleDecap::SniperRifleChargeRateMod()
{
	return ( .25f * ( MIN( GetCount(), MAX_HEAD_BONUS ) - 2 ) ) * TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFSniperRifleDecap::GetCount( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer )
	{
		return pPlayer->m_Shared.GetDecapitations();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifleClassic::CTFSniperRifleClassic()
{
	m_bCharging = false;
#ifdef CLIENT_DLL
	m_pChargedEffect = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifleClassic::~CTFSniperRifleClassic()
{
#ifdef CLIENT_DLL
	if ( m_pChargedEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_pChargedEffect );
		m_pChargedEffect = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::Precache()
{
	BaseClass::Precache();
	PrecacheParticleSystem( SNIPER_CHARGE_BEAM_RED );
	PrecacheParticleSystem( SNIPER_CHARGE_BEAM_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::ZoomOut( void )
{
	CTFWeaponBaseGun::ZoomOut(); // intentionally skipping CTFSniperRifle::ZoomOut()
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::ZoomIn( void )
{
	// Start aiming.
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	CTFWeaponBaseGun::ZoomIn(); // intentionally skipping CTFSniperRifle::ZoomIn()
}

//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::Zoom( void )
{
	ToggleZoom();

	// at least 0.1 seconds from now, but don't stomp a previous value
	m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::HandleZooms( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Handle the zoom when taunting.
	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
			return;
		}
	}

	if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		Zoom();
	}
}

#ifdef CLIENT_DLL
void CTFSniperRifleClassic::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	ManageChargeBeam();
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::ItemPostFrame( void )
{
	// If we're lowered, we're not allowed to fire
	if ( m_bLowered )
		return;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}
		WeaponReset();
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	// Update the sniper dot position if we have one
	if ( m_hSniperDot )
	{
		UpdateSniperDot();
	}
#endif

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		WeaponReset();
		return;
	}

	if ( ( pPlayer->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) )
	{
		if ( !m_bCharging )
		{
			pPlayer->m_Shared.AddCond( TF_COND_AIMING );
			pPlayer->TeamFortress_SetSpeed();

			m_bCharging = true;
#ifdef GAME_DLL
			// Create the sniper dot.
			CreateSniperDot();
			pPlayer->ClearExpression();	
#endif
		}

		float fSniperRifleChargePerSec = m_flChargePerSec;
		ApplyChargeSpeedModifications( fSniperRifleChargePerSec );
		fSniperRifleChargePerSec += SniperRifleChargeRateMod();

		// we don't want sniper charge rate to go too high.
		fSniperRifleChargePerSec = clamp( fSniperRifleChargePerSec, 0, 2.f * TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC );

		m_flChargedDamage = MIN( m_flChargedDamage + gpGlobals->frametime * fSniperRifleChargePerSec, TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX );

#ifdef CLIENT_DLL
		// play the recharged bell if we're fully charged
		if ( IsFullyCharged() && !m_bPlayedBell )
		{
			m_bPlayedBell = true;
			if ( tf_sniper_fullcharge_bell.GetBool() )
			{
				C_TFPlayer::GetLocalTFPlayer()->EmitSound( "TFPlayer.ReCharged" );
			}
		}
#endif
	}
	else if ( m_bCharging )
	{
		if ( pPlayer->GetGroundEntity() )
		{
			Fire( pPlayer );
		}
		else
		{
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
		}

		WeaponReset();
	}
	else
	{
		// Idle.
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFSniperRifleClassic::GetDamageType( void ) const
{
	return CTFWeaponBaseGun::GetDamageType(); // intentionally skipping CTFSniperRifle::GetDamageType()
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifleClassic::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	WeaponReset();

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifleClassic::Deploy( void )
{
	WeaponReset();

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::WeaponReset( void )
{
	m_flChargedDamage = 0.0f;
	m_bCharging = false;
#ifdef CLIENT_DLL
	ManageChargeBeam();
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
	if ( pPlayer )
	{
		pPlayer->ClearExpression();
	}
#else
	m_bPlayedBell = false;
#endif

	m_bCurrentShotIsHeadshot = false;
	m_flChargePerSec = TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC; 
	
	CTFWeaponBase::WeaponReset(); // intentionally skipping CTFSniperRifle::WeaponReset()
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifleClassic::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		WeaponReset();
		return true;
	}

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::ManageChargeBeam( void )
{
	if ( m_bCharging )
	{
		if ( !m_pChargedEffect )
		{
			m_pChargedEffect = ParticleProp()->Create( ( GetTeamNumber() == TF_TEAM_RED ) ? SNIPER_CHARGE_BEAM_RED : SNIPER_CHARGE_BEAM_BLUE, PATTACH_POINT_FOLLOW, "laser" );
		}
	}
	else
	{
		if ( m_pChargedEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pChargedEffect );
			m_pChargedEffect = NULL;
		}
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifleClassic::Detach( void )
{
	WeaponReset();
	BaseClass::Detach();
}

