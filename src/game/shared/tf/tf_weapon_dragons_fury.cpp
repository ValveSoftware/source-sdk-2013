//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Flame Thrower
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_dragons_fury.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "tf_weapon_rocketpack.h"
#include "soundenvelope.h"

#if defined( CLIENT_DLL )
	#include "c_tf_gamestats.h"
	#include "prediction.h"
#else
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
#endif

extern ConVar tf_flamethrower_burstammo;


//=============================================================================
//
// FLAMEBALL BEGIN
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponFlameBall, DT_WeaponFlameBall )

BEGIN_NETWORK_TABLE( CTFWeaponFlameBall, DT_WeaponFlameBall )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flRechargeScale ) ),
#else
	SendPropFloat( SENDINFO( m_flRechargeScale ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponFlameBall )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_rocketlauncher_fireball, CTFWeaponFlameBall );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketlauncher_fireball );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFWeaponFlameBall )
END_DATADESC()
#endif
// FLAMEBALL END

#include "tf_flame.h"

#define DRAGONS_FURY_NEEDLE_POSEPARAM "charge_level"
#define DRAGONS_FURY_BARREL_RECOIL "reload"

ConVar tf_fireball_airblast_recharge_penalty( "tf_fireball_airblast_recharge_penalty", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_fireball_hit_recharge_boost( "tf_fireball_hit_recharge_boost", "1.5", FCVAR_REPLICATED | FCVAR_CHEAT );

CTFWeaponFlameBall::CTFWeaponFlameBall()
{
	m_flRechargeScale = 1.f;
	
#ifdef GAME_DLL
	m_pSndPressure = NULL;
#endif
}

#ifndef CLIENT_DLL
void CTFWeaponFlameBall::Precache()
{
	BaseClass::Precache();
	PrecacheScriptSound( "Weapon_DragonsFury.Single" );
	PrecacheScriptSound( "Weapon_DragonsFury.SingleCrit" );
	PrecacheScriptSound( "Weapon_DragonsFury.BonusDamage" );
	PrecacheScriptSound( "Weapon_DragonsFury.BonusDamagePain" );
	PrecacheScriptSound( "Weapon_DragonsFury.BonusDamageHit" );
	PrecacheScriptSound( "Weapon_DragonsFury.PressureBuild" );
	PrecacheScriptSound( "Weapon_DragonsFury.PressureBuildStop" );
}
#endif

void CTFWeaponFlameBall::PrimaryAttack( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Must have full-pressure (primary meter) to fire.
	if ( !HasFullCharge() )
		return;

	int iAmmo = pPlayer->GetAmmoCount( m_iPrimaryAmmoType );
	if ( iAmmo == 0 )
		return;

	if ( !CanAttack() )
		return;

#ifndef CLIENT_DLL
	if ( pPlayer->m_Shared.IsStealthed() && ShouldRemoveInvisibilityOnPrimaryAttack() )
	{
		pPlayer->RemoveInvisibility();
	}
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );

	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#else
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	CBaseEntity* pProj = FireProjectile( pPlayer );
	ModifyProjectile( pProj );

	if ( ShouldRemoveDisguiseOnPrimaryAttack() )
	{
		pPlayer->RemoveDisguise();
	}

	pPlayer->m_Shared.OnAttack();
	pPlayer->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_PRIMARY, 0.f );

#ifdef GAME_DLL
	StartPressureSound();
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

CBaseEntity* CTFWeaponFlameBall::FireProjectile( CTFPlayer *pPlayer )
{

	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;
	angle.x -= flPunchAngle;
	pPlayer->SetPunchAngle( angle );
	m_flLastFireTime = gpGlobals->curtime;

	RemoveProjectileAmmo( pPlayer );

#ifdef GAME_DLL
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	float fRight = 8.f;
	if ( IsViewModelFlipped() )
	{
		fRight *= -1;
	}
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	// Shoot from the right location
	vecSrc = vecSrc + (vecUp * -9.0f) + (vecRight * 7.0f) + (vecForward * 3.0f);

	QAngle angForward = pPlayer->EyeAngles();

	trace_t trace;	
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecEye, vecSrc, -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	CTFProjectile_Rocket *pRocket = static_cast<CTFProjectile_Rocket*>( CBaseEntity::CreateNoSpawn( "tf_projectile_balloffire", vecSrc, angForward, pPlayer ) );
	if ( pRocket )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

		DoFireEffects();

		pRocket->SetOwnerEntity( pPlayer );
		pRocket->SetLauncher( this ); 

		Vector vForward;
		AngleVectors( angForward, &vForward, NULL, NULL );

		pRocket->SetAbsVelocity( vForward * 600 );

		pRocket->SetDamage( 20 );
		pRocket->ChangeTeam( pPlayer->GetTeamNumber() );
		pRocket->SetCritical( pPlayer->m_Shared.IsCritBoosted() );

		DispatchSpawn( pRocket );

		EmitSound( pRocket->IsCritical() ? "Weapon_DragonsFury.SingleCrit" : "Weapon_DragonsFury.Single" );

		return pRocket;
	}

#else
	DoFireEffects();
#endif

	return NULL;
}

void CTFWeaponFlameBall::SecondaryAttack( void )
{
	// Dragon's Fury requires full-pressure (primary meter) to be able to fire
	if ( !HasFullCharge() )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	int iAmmo = pPlayer->GetAmmoCount( m_iPrimaryAmmoType );

	// charged airblast
	int iChargedAirblast = 0;
	CALL_ATTRIB_HOOK_INT( iChargedAirblast, set_charged_airblast );
	float flMultAmmoPerShot = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flMultAmmoPerShot, mult_airblast_cost );
	int iAmmoPerShot = tf_flamethrower_burstammo.GetInt() * flMultAmmoPerShot;


	if ( iAmmo < iAmmoPerShot )
		return;

	BaseClass::SecondaryAttack();

#ifdef CLIENT_DLL
	if ( prediction->InPrediction() && prediction->IsFirstTimePredicted() )
#endif
	{
		Assert( m_flRechargeScale == 1.f );
	}
	m_flRechargeScale = tf_fireball_airblast_recharge_penalty.GetFloat();
	pPlayer->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_PRIMARY, 0.f );
#ifdef GAME_DLL
	StartPressureSound();
	CSoundEnvelopeController::GetController().SoundChangePitch( m_pSndPressure, 80, 0.3f );
#endif
}

bool CTFWeaponFlameBall::HasFullCharge() const
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return false;

	return pOwner->m_Shared.GetItemChargeMeter( LOADOUT_POSITION_PRIMARY) >= 100.f;
}

void CTFWeaponFlameBall::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	bool bFired = false;

	// Secondary attack has priority
	if ( (pOwner->m_nButtons & IN_ATTACK2) && CanAttack() )
	{
		SecondaryAttack();
		bFired = true;
	}

	if ( !bFired && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		if ( pOwner->GetWaterLevel() == 3 )
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}

		PrimaryAttack();
		bFired = true;
	}

	if ( !bFired && !ReloadOrSwitchWeapons() )
	{
		WeaponIdle();
	}
}

void CTFWeaponFlameBall::OnResourceMeterFilled()
{
	m_flRechargeScale = 1.f;
#ifdef GAME_DLL
	StopPressureSound();
	EmitSound( "Weapon_DragonsFury.PressureBuildStop" );
#endif // GAME_DLL
}

float CTFWeaponFlameBall::GetMeterMultiplier() const
{
	return m_flRechargeScale;
}

#ifdef GAME_DLL

void CTFWeaponFlameBall::RefundAmmo( int nAmmo )
{
	if ( HasFullCharge() )
		return;

	Assert( m_flRechargeScale == 1.f );
	m_flRechargeScale = tf_fireball_hit_recharge_boost.GetFloat();

	// When we get a successful refund, we want to pitch-up the repressurization sound
	// so the user gets the idea that it's going faster (because it is)
	if ( m_pSndPressure )
	{
		CSoundEnvelopeController::GetController().SoundChangePitch( m_pSndPressure, 120.f, 0.2f );
	}
}

void CTFWeaponFlameBall::StartPressureSound()
{
	if ( m_pSndPressure )
		StopPressureSound();

	if ( !m_pSndPressure )
	{
		CPASAttenuationFilter filter( GetAbsOrigin() );
		// Create the repressurization sound
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pSndPressure = controller.SoundCreate( filter, entindex(), "Weapon_DragonsFury.PressureBuild" );

		controller.Play( m_pSndPressure, 1.0, 100 );
	}
}

void CTFWeaponFlameBall::StopPressureSound()
{
	if ( m_pSndPressure )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSndPressure );
		m_pSndPressure = NULL;
	}
}

#else

void CTFWeaponFlameBall::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	UpdatePoseParams();
}

bool CTFWeaponFlameBall::ShouldDrawMeter() const
{
	// There's a meter on the gun, so don't draw the meter
	return false;	
}

void CTFWeaponFlameBall::UpdatePoseParams()
{
	// Use the attachment if its there (1st person view)
	CBaseAnimating* pModelEnt = m_hViewmodelAttachment ? m_hViewmodelAttachment : this;

	if ( !pModelEnt || !pModelEnt->GetModelPtr() )
		return;

	// Get the param indices if we dont have them yet
	if ( m_nNeedlePoseParam == -1 )
	{
		m_nNeedlePoseParam = pModelEnt->LookupPoseParameter( pModelEnt->GetModelPtr(), DRAGONS_FURY_NEEDLE_POSEPARAM );
	}

	if ( m_nBarrelPoseParam == -1 )
	{
		m_nBarrelPoseParam = pModelEnt->LookupPoseParameter( pModelEnt->GetModelPtr(), DRAGONS_FURY_BARREL_RECOIL );
	}

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// Update the params based on the primary meter, which is what controls the refire rate
	if ( m_nNeedlePoseParam != -1 )
	{
		pModelEnt->SetPoseParameter( m_nNeedlePoseParam, pOwner->m_Shared.GetItemChargeMeter( LOADOUT_POSITION_PRIMARY ) / 100.f );
	}

	if ( m_nBarrelPoseParam != -1 )
	{
		pModelEnt->SetPoseParameter( m_nBarrelPoseParam, 1.f - ( pOwner->m_Shared.GetItemChargeMeter( LOADOUT_POSITION_PRIMARY ) / 100.f ) );
	}

}

void CTFWeaponFlameBall::GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] )
{
	if ( !pStudioHdr )
		return;

	UpdatePoseParams();

	BaseClass::GetPoseParameters( pStudioHdr, poseParameter );
}

#endif


