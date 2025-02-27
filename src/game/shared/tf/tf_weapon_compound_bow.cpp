//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_compound_bow.h"
#include "tf_fx_shared.h"
#include "tf_gamerules.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_tf_gamestats.h"
#include "prediction.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_projectile_arrow.h"
#endif

#define COMPOUND_BOW_ATTACHMENT_POINT "muzzle"

//=============================================================================
//
// Weapon tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFCompoundBow, DT_WeaponCompoundBow )

BEGIN_NETWORK_TABLE( CTFCompoundBow, DT_WeaponCompoundBow )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bArrowAlight ) ),
	RecvPropBool( RECVINFO( m_bNoFire ) ),
#else
	SendPropBool( SENDINFO( m_bArrowAlight ) ),
	SendPropBool( SENDINFO( m_bNoFire ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCompoundBow )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargeBeginTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bNoFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_compound_bow, CTFCompoundBow );
PRECACHE_WEAPON_REGISTER( tf_weapon_compound_bow );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFCompoundBow )
END_DATADESC()
#endif

#define TF_ARROW_MAX_CHARGE_TIME 5.0f

//=============================================================================
//
// Weapon functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFCompoundBow::CTFCompoundBow()
{
	m_flLastDenySoundTime = 0.0f;
	m_bNoFire = false;
	m_bReloadsSingly = false;
}

void CTFCompoundBow::Precache( void )
{
	PrecacheScriptSound( "Weapon_CompoundBow.SinglePull" );
	PrecacheScriptSound( "ArrowLight" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::WeaponReset( void )
{
	LowerBow();

	BaseClass::WeaponReset();

//	GetInternalChargeBeginTime() = 0;	
	m_bArrowAlight = false;
	m_bNoAutoRelease = true;
	m_bNoFire = false;
}

#ifdef GAME_DLL


#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_bWantsToShoot = false;

#ifdef GAME_DLL
	CTFProjectile_Arrow *pMainArrow = assert_cast<CTFProjectile_Arrow*>( FireProjectile( pPlayer ) );
	if ( pMainArrow )
	{
		pMainArrow->SetArrowAlight( m_bArrowAlight );

	}

#else
	FireProjectile( pPlayer );
#endif

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif
#ifdef CLIENT_DLL
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	float flBaseFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	float flFireDelay = ApplyFireDelay( flBaseFireDelay );

	ApplyRefireSpeedModifications( flFireDelay );
	
	float flRateMultiplyer = flBaseFireDelay / flFireDelay;

	// Speed up the reload animation built in to firing
	if ( pPlayer->GetViewModel(0) )
	{
		pPlayer->GetViewModel(0)->SetPlaybackRate( flRateMultiplyer );
	}
	if ( pPlayer->GetViewModel(1) )
	{
		pPlayer->GetViewModel(1)->SetPlaybackRate( flRateMultiplyer );
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
	m_flLastDenySoundTime = gpGlobals->curtime;

	float flIdleDelay = 0.5f * flRateMultiplyer;
	SetWeaponIdleTime( m_flNextPrimaryAttack + flIdleDelay );

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

	SetInternalChargeBeginTime( 0 );
	m_bArrowAlight = false;

	// The bow doesn't actually reload, it instead uses the AE_WPN_INCREMENTAMMO anim event in the fire to reload the clip.
	// We need to reset this bool each time we fire so that anim event works.
	m_bReloadedThroughAnimEvent = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( m_bNoFire )
		return;

	if ( !CanAttack() )
	{
		SetInternalChargeBeginTime( 0 );
		return;
	}

	if ( GetInternalChargeBeginTime() <= 0 )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// save that we had the attack button down
		SetInternalChargeBeginTime( gpGlobals->curtime );

		SendWeaponAnim( ACT_VM_PULLBACK );

		float flRateMultiplyer = ApplyFireDelay( 1.0f );
		ApplyRefireSpeedModifications( flRateMultiplyer );
		if ( flRateMultiplyer > 0.0f )
		{
			flRateMultiplyer = 1.0f / flRateMultiplyer;
		}

		// Speed up the reload animation built in to firing
		if ( pPlayer->GetViewModel(0) )
		{
			pPlayer->GetViewModel(0)->SetPlaybackRate( flRateMultiplyer );
		}
		if ( pPlayer->GetViewModel(1) )
		{
			pPlayer->GetViewModel(1)->SetPlaybackRate( flRateMultiplyer );
		}

		bool bPlaySound = true;
#ifdef CLIENT_DLL
		bPlaySound = prediction->IsFirstTimePredicted();
#endif
		if ( bPlaySound )
		{
			// Increase the pitch of the pull sound when the fire rate is higher
			CSoundParameters params;
			if ( CBaseEntity::GetParametersForSound( "Weapon_CompoundBow.SinglePull", params, NULL ) )
			{
				CPASAttenuationFilter filter( pPlayer->GetAbsOrigin(), params.soundlevel );
#ifdef GAME_DLL
				filter.RemoveRecipient( pPlayer );
#endif
				EmitSound_t ep( params );
				ep.m_nPitch *= flRateMultiplyer;

				pPlayer->EmitSound( filter, pPlayer->entindex(), ep );
			}
		}

		// Slow down movement speed while the bow is pulled back.
		pPlayer->m_Shared.AddCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}
	else
	{
		float flTotalChargeTime = gpGlobals->curtime - GetInternalChargeBeginTime();

		if ( flTotalChargeTime >= GetChargeMaxTime() )
		{
			flTotalChargeTime = GetChargeMaxTime();
//			LaunchGrenade();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCompoundBow::GetChargeMaxTime( void )
{
	// It takes less time to charge if the fire rate is higher
	float flChargeMaxTime = ApplyFireDelay( 1.0f );
	ApplyRefireSpeedModifications( flChargeMaxTime );

	return flChargeMaxTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCompoundBow::GetCurrentCharge( void )
{
	if ( GetInternalChargeBeginTime() == 0 )
		return 0;
	else
		return MIN( gpGlobals->curtime - GetInternalChargeBeginTime(), 1.f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCompoundBow::GetProjectileDamage( void )
{
	float flDamage = BaseClass::GetProjectileDamage();
	float flBaseDamage = 50.f;
	CALL_ATTRIB_HOOK_FLOAT( flBaseDamage, mult_dmg );
	float flScale = Clamp( GetCurrentCharge() / GetChargeMaxTime(), 0.f, 1.f);
	float flScaleDamage = flDamage * flScale;

	return (flBaseDamage + flScaleDamage);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCompoundBow::GetProjectileSpeed( void )
{
	return RemapValClamped( GetCurrentCharge(), 0.0f, GetChargeMaxTime(), 1800, 2600 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCompoundBow::GetProjectileGravity( void )
{
	return RemapValClamped( GetCurrentCharge(), 0.0f, GetChargeMaxTime(), 0.5, 0.1 );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFCompoundBow::AddPipeBomb( CTFGrenadePipebombProjectile *pBomb )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFCompoundBow::SecondaryAttack( void )
{
	LowerBow();
}

//-----------------------------------------------------------------------------
// Purpose: Un-nocks a ready arrow.
//-----------------------------------------------------------------------------
void CTFCompoundBow::LowerBow( void )
{
	if ( GetCurrentCharge() == 0.f )
		return; // No arrow nocked.

	SetInternalChargeBeginTime( 0 );

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 1.f;

	m_bNoFire = true;
	m_bWantsToShoot = false;

	SendWeaponAnim( ACT_ITEM2_VM_DRYFIRE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFCompoundBow::DetonateRemotePipebombs( bool bFizzle )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFCompoundBow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
		pPlayer->TeamFortress_SetSpeed();
	}
	m_bNoFire = false;
	SetArrowAlight( false );
	SetInternalChargeBeginTime( 0 );

	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: Play animation appropriate to ball status.
//-----------------------------------------------------------------------------
bool CTFCompoundBow::SendWeaponAnim( int iActivity )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return BaseClass::SendWeaponAnim( iActivity );

	if ( iActivity == ACT_VM_PULLBACK )
	{
		iActivity = ACT_ITEM2_VM_CHARGE;
	}

	float flTotalChargeTime = gpGlobals->curtime - GetInternalChargeBeginTime();
	if ( GetCurrentCharge() > 0 )
	{
		switch ( iActivity )
		{
		case ACT_VM_IDLE:
			if ( flTotalChargeTime >= TF_ARROW_MAX_CHARGE_TIME )
			{
				int iAct = GetActivity();
				if ( iAct == ACT_ITEM2_VM_IDLE_3 || iAct == ACT_ITEM2_VM_CHARGE_IDLE_3 )
				{
					iActivity = ACT_ITEM2_VM_IDLE_3;
				}
				else
				{
					iActivity = ACT_ITEM2_VM_CHARGE_IDLE_3;
				}
			}
			else
			{
				iActivity = ACT_ITEM2_VM_IDLE_2;
			}
			break;
		default:
			break;
		}
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Play animation appropriate to ball status.
//-----------------------------------------------------------------------------
void CTFCompoundBow::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
		LowerBow();
	}

	// If we just fired, and we're past the point at which we tried to reload ourselves,
	// and we don't have any ammo in the clip, switch away to another weapon to stop us
	// from playing the "draw another arrow from the quiver" animation.
	if ( m_bReloadedThroughAnimEvent && m_iClip1 <= 0 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		g_pGameRules->SwitchToNextBestWeapon( pOwner, this );
		return;
	}

	BaseClass::ItemPostFrame();

	if ( !(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2) )
	{
		// Both buttons released. The player can draw the bow again.
		m_bNoFire = false;

		if ( GetActivity() == ACT_ITEM2_VM_PRIMARYATTACK && IsViewModelSequenceFinished() )
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}

	if ( GetCurrentCharge() == 1.f && IsViewModelSequenceFinished() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}

	if ( m_bNoFire )
	{
		WeaponIdle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Held the arrow drawn too long. Give up & play a fail animation.
//-----------------------------------------------------------------------------
void CTFCompoundBow::ForceLaunchGrenade( void ) 
{
	// LowerBow();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates, float flEndDist )
{
	BaseClass::GetProjectileFireSetup( pPlayer, vecOffset, vecSrc, angForward, bHitTeammates, flEndDist );

	float flTotalChargeTime = gpGlobals->curtime - GetInternalChargeBeginTime();
	if ( flTotalChargeTime >= TF_ARROW_MAX_CHARGE_TIME )
	{
		// We want to fire a really inaccurate shot.
		float frand = (float) rand() / VALVE_RAND_MAX;
		angForward->x += -6 + frand*12.f;
		frand = (float) rand() / VALVE_RAND_MAX;
		angForward->y += -6 + frand*12.f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFCompoundBow::ApplyRefireSpeedModifications( float &flBaseRef )
{
	CALL_ATTRIB_HOOK_FLOAT( flBaseRef, fast_reload );

	// Prototype hack
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		int iMaster = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iMaster, ability_master_sniper );
		if ( iMaster )
		{
			flBaseRef *= RemapValClamped( iMaster, 1, 2, 0.6f, 0.3f );
		}
		else if ( pPlayer->m_Shared.GetCarryingRuneType() == RUNE_HASTE )
		{
			flBaseRef *= 0.4f;
		}
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::StartBurningEffect( void )
{
	// clear any old effect before adding a new one
	if ( m_pBurningArrowEffect )
	{
		StopBurningEffect();
	}

	const char *pszEffect;
	m_hParticleEffectOwner = GetWeaponForEffect();
	if ( m_hParticleEffectOwner )
	{
		if ( m_hParticleEffectOwner != this )
		{
			// We're on the viewmodel
			pszEffect = "v_flaming_arrow";
		}
		else
		{
			pszEffect = "flaming_arrow";
		}

		m_pBurningArrowEffect = m_hParticleEffectOwner->ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, COMPOUND_BOW_ATTACHMENT_POINT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::StopBurningEffect( void )
{
	if ( m_pBurningArrowEffect )
	{
		if ( m_hParticleEffectOwner && m_hParticleEffectOwner->ParticleProp() )
		{
			m_hParticleEffectOwner->ParticleProp()->StopEmission( m_pBurningArrowEffect );
		}

		m_pBurningArrowEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::UpdateOnRemove( void )
{
	StopBurningEffect();
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	// Handle particle effect creation / destruction
	if ( m_bArrowAlight && !m_pBurningArrowEffect )
	{
		if ( GetBaseAnimating()->LookupAttachment( COMPOUND_BOW_ATTACHMENT_POINT ) != INVALID_PARTICLE_ATTACHMENT )
		{
			StartBurningEffect();
			EmitSound( "ArrowLight" );
		}
	}
	else if ( !m_bArrowAlight && m_pBurningArrowEffect )
	{
		StopBurningEffect();
	}
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCompoundBow::GetInitialAfterburnDuration() const
{
	// if the bow is lighting someone on fire it must have
	// been the arrow was lit before it was fired
	return 7.5f;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFCompoundBow::Reload( void )
{
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;
	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFCompoundBow::CalcIsAttackCriticalHelper()
{ 
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	// Crit boosted players fire all crits
	if ( pPlayer && pPlayer->m_Shared.IsCritBoosted() )
		return true;

	return false; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFCompoundBow::SetArrowAlight( bool bAlight ) 
{ 
	// Don't light arrows if we're still firing one.
	if (GetActivity() != ACT_ITEM2_VM_PRIMARYATTACK ) 
	{
		m_bArrowAlight = bAlight; 
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFCompoundBow::OwnerCanJump( void )
{
	return GetInternalChargeBeginTime() == 0.f;
}
