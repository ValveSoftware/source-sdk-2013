//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_shotgun.h"
#include "decals.h"
#include "tf_fx_shared.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"

// Client specific.
#if defined( CLIENT_DLL )
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "ilagcompensationmanager.h"
#include "collisionutils.h"
#include "in_buttons.h"
#endif

//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFShotgun, tf_weapon_shotgun_primary )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Soldier, tf_weapon_shotgun_soldier )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_HWG, tf_weapon_shotgun_hwg )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Pyro, tf_weapon_shotgun_pyro )
CREATE_SIMPLE_WEAPON_TABLE( TFScatterGun, tf_weapon_scattergun )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgun_Revenge, tf_weapon_sentry_revenge )
CREATE_SIMPLE_WEAPON_TABLE( TFSodaPopper, tf_weapon_soda_popper )
CREATE_SIMPLE_WEAPON_TABLE( TFPEPBrawlerBlaster, tf_weapon_pep_brawler_blaster )
CREATE_SIMPLE_WEAPON_TABLE( TFShotgunBuildingRescue, tf_weapon_shotgun_building_rescue )

#define SCATTERGUN_KNOCKBACK_MIN_DMG		30.0f
#define SCATTERGUN_KNOCKBACK_MIN_RANGE_SQ	160000.0f //400x400
//=============================================================================
//
// Weapon Shotgun functions.
//
bool CanScatterGunKnockBack( CTFWeaponBase *pWeapon, float flDamage, float flDistanceSq )
{
	int nBulletKnockBack = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, nBulletKnockBack, set_scattergun_has_knockback );
	if ( nBulletKnockBack != 0 )
	{
		if (flDamage > SCATTERGUN_KNOCKBACK_MIN_DMG && flDistanceSq < SCATTERGUN_KNOCKBACK_MIN_RANGE_SQ )
			return true;

		float flKnockbackMult = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flKnockbackMult, scattergun_knockback_mult );
		if ( flKnockbackMult > 1.0f )
			return true;
	}

	return false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFShotgun::CTFShotgun()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFShotgun::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;
	angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
	pPlayer->SetPunchAngle( angle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun::PlayWeaponShootSound( void )
{
	BaseClass::PlayWeaponShootSound();

	if ( TFGameRules()->GameModeUsesUpgrades() )
	{
		PlayUpgradedShootSound( "Weapon_Upgrade.DamageBonus" );
	}
}

//-----------------------------------------------------------------------------
// CTFShotgun_Revenge
//-----------------------------------------------------------------------------
CTFShotgun_Revenge::CTFShotgun_Revenge()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::Precache()
{
	int iModelIndex = PrecacheModel( TF_WEAPON_TAUNT_FRONTIER_JUSTICE_GUITAR_MODEL );
	PrecacheGibsForModel( iModelIndex );
	PrecacheParticleSystem( "blood_impact_backscatter" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	// Do this after the attack, so that we know if we are doing custom damage
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		int iRevengeCrits = pOwner->m_Shared.GetRevengeCrits();
		pOwner->m_Shared.SetRevengeCrits( iRevengeCrits-1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::SentryKilled( int iCrits )
{
	int val = 0;
	CALL_ATTRIB_HOOK_INT( val, sentry_killed_revenge );
	if ( val == 1 )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( pOwner )
		{
			pOwner->m_Shared.SetRevengeCrits( pOwner->m_Shared.GetRevengeCrits() + iCrits );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFShotgun_Revenge::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner && pOwner->m_Shared.GetRevengeCrits() )
	{
		pOwner->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
	}
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFShotgun_Revenge::Deploy( void )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner && pOwner->m_Shared.GetRevengeCrits() )
	{
		pOwner->m_Shared.AddCond( TF_COND_CRITBOOSTED );
	}
#endif

	return BaseClass::Deploy();
}												

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFShotgun_Revenge::GetCustomDamageType() const
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		int iRevengeCrits = pOwner->m_Shared.GetRevengeCrits();
		return iRevengeCrits > 0 ? TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT : TF_DMG_CUSTOM_NONE;
	}
	return TF_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFShotgun_Revenge::GetCount( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner )
	{
		return pOwner->m_Shared.GetRevengeCrits();
	}

	return 0;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFShotgun_Revenge::SetWeaponVisible( bool visible )
{
	if ( !visible )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER && pPlayer->m_Shared.GetTauntIndex() == TAUNT_BASE_WEAPON )
		{
			int nModelIndex = modelinfo->GetModelIndex( TF_WEAPON_TAUNT_FRONTIER_JUSTICE_GUITAR_MODEL );
			CUtlVector<breakmodel_t> guitarGibs;
			BuildGibList( guitarGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
			if ( guitarGibs.Count() > 0 )
			{
				Vector vForward, vRight, vUp;
				AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );

				Vector vecBreakVelocity = Vector(0,0,200);
				AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
				Vector vecOrigin = GetAbsOrigin() + vForward*70 + vUp*10;
				QAngle vecAngle = GetAbsAngles();
				breakablepropparams_t breakParams( vecOrigin, vecAngle, vecBreakVelocity, angularImpulse );
				breakParams.impactEnergyScale = 1.0f;

				CreateGibsFromList( guitarGibs, nModelIndex, NULL, breakParams, NULL, -1 , false, true );
			}
		}
	}

	BaseClass::SetWeaponVisible( visible );
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
int CTFShotgun_Revenge::GetWorldModelIndex( void )
{
	// Engineer guitar support.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer && pPlayer->GetPlayerClass() && ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER ) && 
			( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) ) && ( pPlayer->m_Shared.GetTauntIndex() == TAUNT_BASE_WEAPON ) )
	{
		// While we are taunting, replace our normal world model with the guitar.
		m_iWorldModelIndex = modelinfo->GetModelIndex( TF_WEAPON_TAUNT_FRONTIER_JUSTICE_GUITAR_MODEL );
		return m_iWorldModelIndex;
	}

	return BaseClass::GetWorldModelIndex();
}
#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Reset revenge crits when the shotgun is changed
//-----------------------------------------------------------------------------
void CTFShotgun_Revenge::Detach( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		pPlayer->m_Shared.SetRevengeCrits( 0 );
		pPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED );
	}

	BaseClass::Detach();
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFScatterGun::Reload( void )
{
	int iWeaponMod = 0;
	CALL_ATTRIB_HOOK_INT( iWeaponMod, set_scattergun_no_reload_single );
	if ( iWeaponMod == 1 )
	{
		m_bReloadsSingly = false;
	}

	return BaseClass::Reload();
}

#define JUMP_SPEED	268.3281572999747f
extern float AirBurstDamageForce( const Vector &size, float damage, float scale );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFScatterGun::FireBullet( CTFPlayer *pPlayer )
{
#ifndef CLIENT_DLL
	if ( HasKnockback() )
	{
		// Perform some knock back.
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
		if ( !pOwner )
			return;

		// No knockback during pre-round freeze.
		if ( TFGameRules() && (TFGameRules()->State_Get() == GR_STATE_PREROUND) )
			return;

		// Knock the firer back!
		if ( !(pOwner->GetFlags() & FL_ONGROUND) && !pPlayer->m_bScattergunJump )
		{
			pPlayer->m_bScattergunJump = true;

			pOwner->m_Shared.StunPlayer( 0.3f, 1.f, TF_STUN_MOVEMENT | TF_STUN_MOVEMENT_FORWARD_ONLY );

			float flForce = AirBurstDamageForce( pOwner->WorldAlignSize(), 60, 6.f );

			Vector vecForward;
			AngleVectors( pOwner->EyeAngles(), &vecForward );
			Vector vecForce = vecForward * -flForce;

			EntityMatrix mtxPlayer;
			mtxPlayer.InitFromEntity( pOwner );
			Vector vecAbsVelocity = pOwner->GetAbsVelocity();
			Vector vecAbsVelocityAsPoint = vecAbsVelocity + pOwner->GetAbsOrigin();
			Vector vecLocalVelocity = mtxPlayer.WorldToLocal( vecAbsVelocityAsPoint );

			vecLocalVelocity.x = -300;

			vecAbsVelocityAsPoint = mtxPlayer.LocalToWorld( vecLocalVelocity );
			vecAbsVelocity = vecAbsVelocityAsPoint - pOwner->GetAbsOrigin();
			pOwner->SetAbsVelocity( vecAbsVelocity );

			// Impulse an additional bit of Z push.
			pOwner->ApplyAbsVelocityImpulse( Vector(0,0,50.f) );

			// Slow player movement for a brief period of time.
			pOwner->RemoveFlag( FL_ONGROUND );
		}
	}
#endif

	BaseClass::FireBullet( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFScatterGun::ApplyPostHitEffects( const CTakeDamageInfo &inputInfo, CTFPlayer *pPlayer )
{
#ifndef CLIENT_DLL
	if ( !HasKnockback() )
		return;

	CTFPlayer *pAttacker = ToTFPlayer( inputInfo.GetAttacker() );
	if ( !pAttacker )
		return;

	CTFPlayer *pTarget = pPlayer;
	if ( !pTarget )
		return;

	if ( pTarget->m_Shared.GetWeaponKnockbackID() > -1 )
		return;

	if ( pTarget->m_Shared.IsImmuneToPushback() )
		return;

	float flDam = inputInfo.GetDamage();
	Vector vecDir = pAttacker->WorldSpaceCenter() - pTarget->WorldSpaceCenter();
	if ( !CanScatterGunKnockBack( this, flDam, vecDir.LengthSqr() ) )
		return;
	
	VectorNormalize( vecDir );

	float flKnockbackMult = 3.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( this, flKnockbackMult, scattergun_knockback_mult );	

	float flForce = AirBurstDamageForce( pTarget->WorldAlignSize(), flDam, flKnockbackMult );
	Vector vecForce = vecDir * -flForce;
	vecForce.z += JUMP_SPEED;

	pTarget->ApplyGenericPushbackImpulse( vecForce, pAttacker );

	pTarget->m_Shared.StunPlayer( 0.3f, 1.f, TF_STUN_MOVEMENT | TF_STUN_MOVEMENT_FORWARD_ONLY, pAttacker );
	pTarget->m_Shared.SetWeaponKnockbackID( pAttacker->GetUserID() );

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFScatterGun::FinishReload( void )
{
	CTFPlayer* pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( UsesClipsForAmmo1() && !m_bReloadsSingly )
	{
		int primary	= MIN( GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));	
		m_iClip1 += primary;

		// Takes a whole clip worth of ammo to reload, causing us to lose whatever was chambered.
		pOwner->RemoveAmmo( GetMaxClip1(), m_iPrimaryAmmoType);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFScatterGun::HasKnockback( void )
{
	int iWeaponMod = 0;
	CALL_ATTRIB_HOOK_INT( iWeaponMod, set_scattergun_has_knockback );
	if ( iWeaponMod == 1 )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: Play animation appropriate to ball status.
//-----------------------------------------------------------------------------
bool CTFScatterGun::SendWeaponAnim( int iActivity )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return BaseClass::SendWeaponAnim( iActivity );

	if ( HasKnockback() )
	{
		// Knockback version uses a different model and animation set.
		switch ( iActivity )
		{
		case ACT_VM_DRAW:
			iActivity = ACT_ITEM2_VM_DRAW;
			break;
		case ACT_VM_HOLSTER:
			iActivity = ACT_ITEM2_VM_HOLSTER;
			break;
		case ACT_VM_IDLE:
			iActivity = ACT_ITEM2_VM_IDLE;
			break;
		case ACT_VM_PULLBACK:
			iActivity = ACT_ITEM2_VM_PULLBACK;
			break;
		case ACT_VM_PRIMARYATTACK:
			iActivity = ACT_ITEM2_VM_PRIMARYATTACK;
			break;
		case ACT_VM_SECONDARYATTACK:
			iActivity = ACT_ITEM2_VM_SECONDARYATTACK;
			break;
		case ACT_VM_RELOAD:
			iActivity = ACT_ITEM2_VM_RELOAD;
			break;
		case ACT_VM_DRYFIRE:
			iActivity = ACT_ITEM2_VM_DRYFIRE;
			break;
		case ACT_VM_IDLE_TO_LOWERED:
			iActivity = ACT_ITEM2_VM_IDLE_TO_LOWERED;
			break;
		case ACT_VM_IDLE_LOWERED:
			iActivity = ACT_ITEM2_VM_IDLE_LOWERED;
			break;
		case ACT_VM_LOWERED_TO_IDLE:
			iActivity = ACT_ITEM2_VM_LOWERED_TO_IDLE;
			break;
		default:
			break;
		}
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
void CTFScatterGun::Equip( CBaseCombatCharacter *pOwner )
{
	CTFPlayer *pPlayer = dynamic_cast<CTFPlayer*>( pOwner );
	if ( pPlayer )
	{
		pPlayer->m_Shared.SetScoutHypeMeter( 0.0f );
	}

	BaseClass::Equip( pOwner );
}
#endif // GAME_DLL
//-----------------------------------------------------------------------------
// CTFSodaPopper
//-----------------------------------------------------------------------------
float CTFSodaPopper::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0.f;

	return pPlayer->m_Shared.GetScoutHypeMeter() * 0.01f;
}

//-----------------------------------------------------------------------------
void CTFSodaPopper::ItemBusyFrame( void )
{
#ifdef GAME_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->m_nButtons & IN_ATTACK2 )
	{
		// Check here so we can always activate buff when we want (similar to stickies)
		SecondaryAttack();
	}
#endif

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
void CTFSodaPopper::SecondaryAttack()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner( );
	if ( !pPlayer || pPlayer->m_Shared.IsHypeBuffed() )
		return;

	if ( pPlayer->m_Shared.GetScoutHypeMeter() >= 100.f )
	{
		pPlayer->m_Shared.AddCond( TF_COND_SODAPOPPER_HYPE );
	}
}

//-----------------------------------------------------------------------------
float CTFPEPBrawlerBlaster::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0.f;

	return pPlayer->m_Shared.GetScoutHypeMeter() * 0.01f;
}

//-----------------------------------------------------------------------------
float CTFShotgunBuildingRescue::GetProjectileSpeed( void )
{
	return RemapValClamped( 0.75f, 0.0f, 1.f, 1800, 2600 ); // Temp, if we want to ramp.
}

//-----------------------------------------------------------------------------
float CTFShotgunBuildingRescue::GetProjectileGravity( void )
{
	return RemapValClamped( 0.75f, 0.0f, 1.f, 0.5f, 0.1f ); // Temp, if we want to ramp.
}

//-----------------------------------------------------------------------------
bool CTFShotgunBuildingRescue::IsViewModelFlipped( void )
{
	return !BaseClass::IsViewModelFlipped(); // Invert because arrows are backwards by default.
}
