//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_revolver.h"
#include "tf_fx_shared.h"
#include "datamap.h"
#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Revolver tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRevolver, DT_WeaponRevolver )

BEGIN_NETWORK_TABLE( CTFRevolver, DT_WeaponRevolver )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFRevolver )
DEFINE_PRED_FIELD( m_flLastAccuracyCheck, FIELD_FLOAT, 0 ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_revolver, CTFRevolver );
PRECACHE_WEAPON_REGISTER( tf_weapon_revolver );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRevolver )
END_DATADESC()
#endif


//=============================================================================
//
// Weapon Revolver functions.
//

CTFRevolver::CTFRevolver()
{
	m_flLastAccuracyCheck = 0.f;
	m_flAccuracyCheckTime = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRevolver::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	// The the owning local player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			return false;
		}
	}

	if ( pPlayer->m_Shared.IsFeignDeathReady() )
		return false; // Can't reload if our feign death arm is up.

	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFRevolver::GetDamageType( void ) const
{
	if ( CanHeadshot() && (gpGlobals->curtime - m_flLastAccuracyCheck > 1.f) )
	{
		int iDamageType = BaseClass::GetDamageType() | DMG_USE_HITLOCATIONS;
		return iDamageType;
	}

	return BaseClass::GetDamageType();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRevolver::CanFireCriticalShot( bool bIsHeadshot, CBaseEntity *pTarget /*= NULL*/ )
{
	if ( !BaseClass::CanFireCriticalShot( bIsHeadshot, pTarget ) )
		return false;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer && pPlayer->m_Shared.IsCritBoosted() )
		return true;

	// Magic.
	if ( pTarget && ( pPlayer->GetAbsOrigin() - pTarget->GetAbsOrigin() ).Length2DSqr() > Square( 1200.f ) )
		return false;

	// can only fire a crit shot if this is a headshot, unless we're critboosted
	if ( !bIsHeadshot )
	{
		// Base revolver still randomly crits. Ambassador doesn't.
		return !CanHeadshot();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRevolver::PrimaryAttack( void )
{
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	if ( HasLastShotCritical() )
	{
		pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED );
	}
	else
	{
		int iAttr = 0;
		CALL_ATTRIB_HOOK_INT( iAttr, last_shot_crits );
		if ( iAttr )
		{
			pPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
		}
	}

	m_flLastAccuracyCheck = gpGlobals->curtime;

	if ( SapperKillsCollectCrits() )
	{
		// Do this after the attack, so that we know if we are doing custom damage
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( pOwner )
		{
			int iRevengeCrits = pOwner->m_Shared.GetRevengeCrits();
			if ( iRevengeCrits > 0 )
			{
				pOwner->m_Shared.SetRevengeCrits( iRevengeCrits-1 );
			}
		}
	}
#ifdef GAME_DLL
	// Lower bonus for each attack
	int iExtraDamageOnHitPenalty = 0;
	CALL_ATTRIB_HOOK_INT( iExtraDamageOnHitPenalty, extra_damage_on_hit_penalty );
	if ( iExtraDamageOnHitPenalty )
	{
		int iDecaps = pPlayer->m_Shared.GetDecapitations();
		pPlayer->m_Shared.SetDecapitations( Max( 0, iDecaps - iExtraDamageOnHitPenalty ) );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRevolver::GetWeaponSpread( void )
{
	float fSpread = BaseClass::GetWeaponSpread();

	if ( CanHeadshot() )
	{
		// We are highly accurate for our first shot.
		float flTimeSinceCheck = gpGlobals->curtime - m_flLastAccuracyCheck;
		fSpread = RemapValClamped( flTimeSinceCheck, 1.0f, 0.5f, 0.f, fSpread );
	}

	//DevMsg( "Spread: base %3.5f mod: %3.5f\n", BaseClass::GetWeaponSpread(), fSpread );

	return fSpread;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRevolver::GetWeaponCrosshairScale( float &flScale )
{
	C_TFPlayer* pTFPlayer = ToTFPlayer( GetOwner() );
	if ( !pTFPlayer )
		return;

	if ( CanHeadshot() )
	{
		float curtime = pTFPlayer->GetFinalPredictedTime() + ( gpGlobals->interpolation_amount * TICK_INTERVAL );
		float flTimeSinceCheck = curtime - m_flLastAccuracyCheck;
		flScale = RemapValClamped( flTimeSinceCheck, 1.0f, 0.5f, 0.75f, 2.5f );
	}
	else
	{
		BaseClass::GetWeaponCrosshairScale( flScale );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFRevolver::GetCount( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return 0;

	if ( SapperKillsCollectCrits() )
	{
		return pOwner->m_Shared.GetRevengeCrits();
	}

	int iExtraDamageOnHit = 0;
	CALL_ATTRIB_HOOK_INT( iExtraDamageOnHit, extra_damage_on_hit );
	if ( iExtraDamageOnHit )
	{
		return Min( 200, pOwner->m_Shared.GetDecapitations() );
	}

	return 0;
}

//-----------------------------------------------------------------------------
const char* CTFRevolver::GetEffectLabelText( void )
{
	int iExtraDamageOnHit = 0;
	CALL_ATTRIB_HOOK_INT( iExtraDamageOnHit, extra_damage_on_hit );
	if ( iExtraDamageOnHit )
	{
		return "#TF_BONUS";
	}
	return "#TF_CRITS";
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRevolver::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		if ( SapperKillsCollectCrits() )
		{	
			if ( pOwner->m_Shared.GetRevengeCrits() )
			{
				pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
			}
		}

		if ( HasLastShotCritical() )
		{
			pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
		}
	}
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRevolver::Deploy( void )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		if ( SapperKillsCollectCrits() )
		{
			if ( pOwner->m_Shared.GetRevengeCrits() )
			{
				pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED );
			}
		}

		if ( HasLastShotCritical() )
		{
			pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED );
		}
	}
#endif

	return BaseClass::Deploy();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Reset revenge crits when the revolver is changed
//-----------------------------------------------------------------------------
void CTFRevolver::Detach( void )
{
	if ( SapperKillsCollectCrits() )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->m_Shared.SetRevengeCrits( 0 );
			pPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
		}
	}

	BaseClass::Detach();
}

//-----------------------------------------------------------------------------
float CTFRevolver::GetProjectileDamage( void )
{
	float flDamageMod = 1.0f;
	int iExtraDamageOnHit = 0;
	CALL_ATTRIB_HOOK_INT( iExtraDamageOnHit, extra_damage_on_hit );
	if ( iExtraDamageOnHit )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if ( pOwner )
		{
			flDamageMod = 1.0f + ( Min( 200, pOwner->m_Shared.GetDecapitations() ) * 0.01f );
		}
	}

	return BaseClass::GetProjectileDamage() * flDamageMod;
}
#endif
