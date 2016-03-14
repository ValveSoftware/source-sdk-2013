//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_csbasegun.h"
#include "fx_cs_shared.h"
#include "mom_player_shared.h"


IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCSBaseGun, DT_WeaponCSBaseGun )

BEGIN_NETWORK_TABLE( CWeaponCSBaseGun, DT_WeaponCSBaseGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCSBaseGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_csbase_gun, CWeaponCSBaseGun );



CWeaponCSBaseGun::CWeaponCSBaseGun()
{

}

void CWeaponCSBaseGun::Spawn()
{
	m_flAccuracy = 0.2;
	m_bDelayFire = false;
	m_zoomFullyActiveTime = -1.0f;

	BaseClass::Spawn();
}


bool CWeaponCSBaseGun::Deploy()
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return false;

	m_flAccuracy = 0.2;
	pPlayer->m_iShotsFired = 0;
	m_bDelayFire = false;
	m_zoomFullyActiveTime = -1.0f;

	return BaseClass::Deploy();
}

void CWeaponCSBaseGun::ItemPostFrame()
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();

	if ( !pPlayer )
		return;

	//GOOSEMAN : Return zoom level back to previous zoom level before we fired a shot. This is used only for the AWP.
	// And Scout.
	if ( (m_flNextPrimaryAttack <= gpGlobals->curtime) && (pPlayer->m_bResumeZoom == TRUE) )
	{
#ifndef CLIENT_DLL
		pPlayer->SetFOV( pPlayer, pPlayer->m_iLastZoom, 0.05f );
		m_zoomFullyActiveTime = gpGlobals->curtime + 0.05f;// Make sure we think that we are zooming on the server so we don't get instant acc bonus

		if ( pPlayer->GetFOV() == pPlayer->m_iLastZoom )
		{
			// return the fade level in zoom.
			pPlayer->m_bResumeZoom = false;
		}
#endif
	}

	BaseClass::ItemPostFrame();
}


void CWeaponCSBaseGun::PrimaryAttack()
{
	// Derived classes should implement this and call CSBaseGunFire.
	Assert( false );
}

bool CWeaponCSBaseGun::CSBaseGunFire( float flSpread, float flCycleTime, bool bPrimaryMode )
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return false;

	const CCSWeaponInfo &pCSInfo = GetCSWpnData();

	m_bDelayFire = true;
	pPlayer->m_iShotsFired++;
	
	// These modifications feed back into flSpread eventually.
	if ( pCSInfo.m_flAccuracyDivisor != -1 )
	{
		int iShotsFired = pPlayer->m_iShotsFired;

		if ( pCSInfo.m_bAccuracyQuadratic )
			iShotsFired = iShotsFired * iShotsFired;
		else
			iShotsFired = iShotsFired * iShotsFired * iShotsFired;

		m_flAccuracy = ( iShotsFired / pCSInfo.m_flAccuracyDivisor) + pCSInfo.m_flAccuracyOffset;
		
		if (m_flAccuracy > pCSInfo.m_flMaxInaccuracy)
			m_flAccuracy = pCSInfo.m_flMaxInaccuracy;
	}

	// Out of ammo?
	if ( m_iClip1 <= 0 )
	{
		if (m_bFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
		}

		return false;
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	m_iClip1--;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FX_FireBullets(
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
		pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
		GetWeaponID(),
		bPrimaryMode?Primary_Mode:Secondary_Mode,
		CBaseEntity::GetPredictionRandomSeed() & 255,
		flSpread );

	DoFireEffects();

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	SetWeaponIdleTime( gpGlobals->curtime + pCSInfo.m_flTimeToIdleAfterFire );
	return true;
}


void CWeaponCSBaseGun::DoFireEffects()
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();
	
	if ( pPlayer )
		 pPlayer->DoMuzzleFlash();
}


bool CWeaponCSBaseGun::Reload()
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return false;

	if (pPlayer->GetAmmoCount( GetPrimaryAmmoType() ) <= 0)
		return false;

	int iResult = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( !iResult )
		return false;

	pPlayer->SetAnimation( PLAYER_RELOAD );

#ifndef CLIENT_DLL
	if ((iResult) && (pPlayer->GetFOV() != pPlayer->GetDefaultFOV()))
	{
		pPlayer->SetFOV( pPlayer, pPlayer->GetDefaultFOV() );
	}
#endif

	m_flAccuracy = 0.2;
	pPlayer->m_iShotsFired = 0;
	m_bDelayFire = false;

	//pPlayer->SetShieldDrawnState( false );
	return true;
}

void CWeaponCSBaseGun::WeaponIdle()
{
	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	// only idle if the slid isn't back
	if ( m_iClip1 != 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + GetCSWpnData().m_flIdleInterval );
		SendWeaponAnim( ACT_VM_IDLE );
	}
}
