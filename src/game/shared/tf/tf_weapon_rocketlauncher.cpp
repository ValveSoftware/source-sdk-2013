//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Rocket Launcher
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_rocketlauncher.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase_rocket.h"
#include "in_buttons.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "soundenvelope.h"
#include "particle_property.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "tf_projectile_arrow.h"

#endif

#define BOMBARDMENT_ROCKET_MODEL "models/buildables/sentry3_rockets.mdl"

//=============================================================================
//
// Weapon Rocket Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketLauncher, DT_WeaponRocketLauncher )

BEGIN_NETWORK_TABLE( CTFRocketLauncher, DT_WeaponRocketLauncher )
#ifndef CLIENT_DLL
//	SendPropInt( SENDINFO( m_iSecondaryShotsFired ) ),
#else
//	RecvPropInt( RECVINFO( m_iSecondaryShotsFired ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRocketLauncher )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_rocketlauncher, CTFRocketLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketlauncher );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRocketLauncher )
END_DATADESC()
#endif

//=============================================================================
//
// Direct Hit tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketLauncher_DirectHit, DT_WeaponRocketLauncher_DirectHit )

BEGIN_NETWORK_TABLE( CTFRocketLauncher_DirectHit, DT_WeaponRocketLauncher_DirectHit )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRocketLauncher_DirectHit )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_rocketlauncher_directhit, CTFRocketLauncher_DirectHit );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketlauncher_directhit );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRocketLauncher_DirectHit )
END_DATADESC()
#endif

//=============================================================================
//
// AIRSTRIKE BEGIN
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketLauncher_AirStrike, DT_WeaponRocketLauncher_AirStrike )

BEGIN_NETWORK_TABLE( CTFRocketLauncher_AirStrike, DT_WeaponRocketLauncher_AirStrike )
#ifndef CLIENT_DLL
//	SendPropInt( SENDINFO( m_iRocketKills ) ),
#else
//	RecvPropInt( RECVINFO( m_iRocketKills ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRocketLauncher_AirStrike )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_rocketlauncher_airstrike, CTFRocketLauncher_AirStrike );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketlauncher_airstrike );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRocketLauncher_AirStrike )
END_DATADESC()
#endif
// AIRSTRIKE END

//CREATE_SIMPLE_WEAPON_TABLE( TFRocketLauncher_AirStrike, tf_weapon_rocketlauncher_airstrike )
//CREATE_SIMPLE_WEAPON_TABLE( TFRocketLauncher_Mortar, tf_weapon_rocketlauncher_mortar )
//=============================================================================
//
// Mortar tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketLauncher_Mortar, DT_WeaponRocketLauncher_Mortar )

BEGIN_NETWORK_TABLE( CTFRocketLauncher_Mortar, DT_WeaponRocketLauncher_Mortar )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRocketLauncher_Mortar )
END_PREDICTION_DATA()


// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFRocketLauncher_Mortar )
END_DATADESC()
#endif

//=============================================================================
//
// Crossbow tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFCrossbow, DT_Crossbow )

BEGIN_NETWORK_TABLE( CTFCrossbow, DT_Crossbow )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flRegenerateDuration ) ),
	RecvPropFloat( RECVINFO( m_flLastUsedTimestamp ) ),
#else
	SendPropFloat( SENDINFO( m_flRegenerateDuration ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flLastUsedTimestamp ), 0, SPROP_NOSCALE ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCrossbow )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_crossbow, CTFCrossbow );
PRECACHE_WEAPON_REGISTER( tf_weapon_crossbow );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFCrossbow )
END_DATADESC()
#endif


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFRocketLauncher::CTFRocketLauncher()
{
	m_bReloadsSingly = true;
	m_nReloadPitchStep = 0;

#ifdef GAME_DLL
	m_bIsOverloading = false;
#endif //GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFRocketLauncher::~CTFRocketLauncher()
{
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::Precache()
{
	BaseClass::Precache();
	PrecacheParticleSystem( "rocketbackblast" );

	// FIXME: DO WE STILL NEED THESE??
	PrecacheScriptSound( "MVM.GiantSoldierRocketShoot" );
	PrecacheScriptSound( "MVM.GiantSoldierRocketShootCrit" );
	PrecacheScriptSound( "MVM.GiantSoldierRocketExplode" );

	PrecacheScriptSound( "Weapon_Airstrike.AltFire" );
	PrecacheScriptSound( "Weapon_Airstrike.Fail" );
	//Building_Sentrygun.FireRocket
}
#endif

void CTFRocketLauncher::ModifyEmitSoundParams( EmitSound_t &params )
{
	bool bBaseReloadSound = V_strcmp( params.m_pSoundName, "Weapon_RPG.Reload" ) == 0;
	if ( AutoFiresFullClip() && ( bBaseReloadSound || V_strcmp( params.m_pSoundName, "Weapon_DumpsterRocket.Reload" ) == 0 ) )
	{
		float fMaxAmmoInClip = GetMaxClip1();
		float fAmmoPercentage = static_cast< float >( m_nReloadPitchStep ) / fMaxAmmoInClip;

		// Play a sound that gets higher pitched as more ammo is added
		if ( bBaseReloadSound )
		{
			params.m_pSoundName = "Weapon_DumpsterRocket.Reload_FP";
		}
		else
		{
			params.m_pSoundName = "Weapon_DumpsterRocket.Reload";
		}

		params.m_nPitch *= RemapVal( fAmmoPercentage, 0.0f, ( fMaxAmmoInClip - 1.0f ) / fMaxAmmoInClip, 0.79f, 1.19f );
		params.m_nFlags |= SND_CHANGE_PITCH;

		m_nReloadPitchStep = MIN( GetMaxClip1() - 1, m_nReloadPitchStep + 1 );

		// The last rocket goes in right when this sound happens so that you can launch it before a misfire
		IncrementAmmo();
		m_bReloadedThroughAnimEvent = true;
	}
	else if ( UsesCenterFireProjectile() && ( bBaseReloadSound || V_strcmp( params.m_pSoundName, "Weapon_QuakeRPG.Reload" ) == 0 ) )
	{
		params.m_pSoundName = "Weapon_QuakeRPG.Reload";
	}
}

void CTFRocketLauncher::Misfire( void )
{
	BaseClass::Misfire();

#ifdef GAME_DLL
	if ( CanOverload() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer )
			return;

		CTFBaseRocket *pRocket = dynamic_cast< CTFBaseRocket* >( BaseClass::FireProjectile( pPlayer ) );
		if ( pRocket )
		{
			trace_t tr;
			UTIL_TraceLine( pRocket->GetAbsOrigin(), pPlayer->EyePosition(), MASK_SOLID, pRocket, COLLISION_GROUP_NONE, &tr );
			pRocket->Explode( &tr, pPlayer );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
bool CTFRocketLauncher::CheckReloadMisfire( void )
{
	if ( !CanOverload() )
		return false;

#ifdef GAME_DLL
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( m_bIsOverloading )
	{
		if ( Clip1() > 0 )
		{
			Misfire();
			return true;
		}
		else
		{
			m_bIsOverloading = false;
		}
	}
	else if ( Clip1() >= GetMaxClip1() || ( Clip1() > 0 && pPlayer && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) == 0 ) )
	{
		Misfire();
		m_bIsOverloading = true;
		return true;
	}
#endif // GAME_DLL
	return false;
}


//-----------------------------------------------------------------------------
bool CTFRocketLauncher::ShouldBlockPrimaryFire()
{
	return !AutoFiresFullClip();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRocketLauncher::CanInspect() const
{
	if ( AutoFiresFullClip() && ( m_iClip1 > 0 ) )
		return false;

	return BaseClass::CanInspect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFRocketLauncher::FireProjectile( CTFPlayer *pPlayer )
{
	m_flShowReloadHintAt = gpGlobals->curtime + 30;
	CBaseEntity *pRocket = BaseClass::FireProjectile( pPlayer );

	m_nReloadPitchStep = MAX( 0, m_nReloadPitchStep - 1 );

#ifdef GAME_DLL
	int iProjectile = 0;
	CALL_ATTRIB_HOOK_INT( iProjectile, override_projectile_type );
	if ( iProjectile == 0 )
	{
		iProjectile = GetWeaponProjectileType();
	}
	if ( pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) && IsCurrentAttackARandomCrit() && ( iProjectile == TF_PROJECTILE_ROCKET ) )
	{
		// Track consecutive crit shots for achievements
		m_iConsecutiveCrits++;
		if ( m_iConsecutiveCrits == 2 )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_SHOOT_MULT_CRITS );
		}
	}
	else
	{
		m_iConsecutiveCrits = 0;
	}
	m_bIsOverloading = false;
#endif

	if ( TFGameRules()->GameModeUsesUpgrades() )
	{
		PlayUpgradedShootSound( "Weapon_Upgrade.DamageBonus" );
	}


	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	BaseClass::ItemPostFrame();

#ifdef GAME_DLL

	if ( m_flShowReloadHintAt && m_flShowReloadHintAt < gpGlobals->curtime )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			pOwner->HintMessage( HINT_SOLDIER_RPG_RELOAD );
		}
		m_flShowReloadHintAt = 0;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketLauncher::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	m_flShowReloadHintAt = 0;
	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketLauncher::CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex )
{
	BaseClass::CreateMuzzleFlashEffects( pAttachEnt, nIndex );

	// Don't do backblast effects in first person
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner->IsLocalPlayer() )
		return;

	ParticleProp()->Init( this );
	ParticleProp()->Create( "rocketbackblast", PATTACH_POINT_FOLLOW, "backblast" );
}
#endif


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CTFRocketLauncher::GetWeaponProjectileType( void ) const
{
	return BaseClass::GetWeaponProjectileType();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// CTFRocketLauncher_AirStrike BEGIN
//----------------------------------------------------------------------------------------------------------------------------------------------------------
CTFRocketLauncher_AirStrike::CTFRocketLauncher_AirStrike()
{
	//m_iSecondaryShotsFired = 0;
}

#ifdef GAME_DLL
//----------------------------------------------------------------------------------------------------------------------------------------------------------
void CTFRocketLauncher_AirStrike::OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::OnPlayerKill( pVictim, info );

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	int iDecap = pOwner->m_Shared.GetDecapitations() + 1;
	if ( pVictim )
	{
		iDecap += pVictim->m_Shared.GetDecapitations();
	}
	pOwner->m_Shared.SetDecapitations( iDecap );	

	int iClipSizeOnKills = 0;
	CALL_ATTRIB_HOOK_INT( iClipSizeOnKills, clipsize_increase_on_kill );
	if ( iClipSizeOnKills && ( iDecap >= iClipSizeOnKills ) )
	{
		pOwner->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_AIRSTRIKE_MAX_CLIP );
	}
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
#endif

//-----------------------------------------------------------------------------
int CTFRocketLauncher_AirStrike::GetCount( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return 0;

	return pOwner->m_Shared.GetDecapitations();
}

////----------------------------------------------------------------------------------------------------------------------------------------------------------
//void CTFRocketLauncher_AirStrike::PrimaryAttack( void )
//{
//	CTFPlayer *pPlayer = GetTFPlayerOwner();
//	if ( !pPlayer )
//		return;
//
//	// If the player is blast jumping and hasn't fired a shot yet, we can initiate
//	if ( pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) && m_iSecondaryShotsFired == 0 )
//	{
//		FireSecondaryRockets();
//	}
//	else
//	{
//		BaseClass::PrimaryAttack();
//	}
//}
////----------------------------------------------------------------------------------------------------------------------------------------------------------
//bool CTFRocketLauncher_AirStrike::CanHolster( void )
//{
//	if ( m_iSecondaryShotsFired > 0 )
//		return false;
//
//	return BaseClass::CanHolster();
//}
////-----------------------------------------------------------------------------
//void CTFRocketLauncher_AirStrike::ItemPostFrame( void )
//{
//	// If allowed
//	FireSecondaryRockets();
//	BaseClass::ItemPostFrame();
//}
////-----------------------------------------------------------------------------
//void CTFRocketLauncher_AirStrike::ItemBusyFrame( void )
//{
//	// If allowed
//	FireSecondaryRockets();
//	BaseClass::ItemBusyFrame();
//}
//
////-----------------------------------------------------------------------------
//void CTFRocketLauncher_AirStrike::FireSecondaryRockets()
//{
//#ifdef STAGING_ONLY
//	if ( m_flNextPrimaryAttack >= gpGlobals->curtime )
//		return;
//
//	CTFPlayer *pPlayer = GetTFPlayerOwner();
//	if ( !pPlayer )
//		return;
//
//	if ( !( pPlayer->m_nButtons & IN_ATTACK ) && m_iSecondaryShotsFired == 0 )
//		return;
//
//	int iAirBombardment = 0;
//	CALL_ATTRIB_HOOK_INT( iAirBombardment, rj_air_bombardment );
//	if ( !iAirBombardment )
//		return;
//
//	// This function and its checks are only on the server
//	if ( !pPlayer->m_Shared.InCond( TF_COND_BLASTJUMPING ) )
//	{
//#ifdef CLIENT_DLL
//		// play fail sound locally
//		//pPlayer->EmitSound( "Weapon_Airstrike.Fail" );
//#endif
//		m_iSecondaryShotsFired = 0;
//		return;
//	}
//
//	if ( m_iClip1 <= 0 && m_iSecondaryShotsFired == 0 )
//		return;
//
//	if ( m_bReloadsSingly )
//	{
//		m_iReloadMode.Set( TF_RELOAD_START );
//	}
//
//	float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
//	flFireDelay += GetFireDelay();
//	CALL_ATTRIB_HOOK_FLOAT( flFireDelay, mult_postfiredelay );
//	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flFireDelay, hwn_mult_postfiredelay );
//
//	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
//	pPlayer->SetAnimation( PLAYER_ATTACK1 );
//	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
//	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay / 2.0f;
//
//	// Want a different sound
//	pPlayer->EmitSound( "Weapon_Airstrike.AltFire" );
//
//#ifdef GAME_DLL
//	// Server only - create the rocket.
//	Vector vecSrc;
//	QAngle angForward;
//	Vector vecOffset( 23.5f, 12.0f, -3.0f );
//	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );
//
//	CTFProjectile_SentryRocket *pProjectile = CTFProjectile_SentryRocket::Create( vecSrc, angForward, this, pPlayer );
//
//	if ( pProjectile )
//	{
//		pProjectile->SetCritical( IsCurrentAttackACrit() );
//		pProjectile->SetDamage( GetProjectileDamage() * tf_airstrike_dmg_scale.GetFloat() );
//		pProjectile->SetDamageForceScale( tf_airstrike_dmg_scale.GetFloat() );
//	}
//
//	if ( m_iSecondaryShotsFired == 0 )
//	{
//		RemoveProjectileAmmo( pPlayer );
//	}
//
//	m_iSecondaryShotsFired++;
//	if ( m_iSecondaryShotsFired >= 3 )
//	{
//		// Decrement ammo and reset
//		m_iSecondaryShotsFired = 0;
//		// Give normal delay between shots here
//		m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
//	}
//#endif
//
//#endif
//}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// CTFRocketLauncher_Mortar BEGIN
//----------------------------------------------------------------------------------------------------------------------------------------------------------
//CTFRocketLauncher_Mortar::CTFRocketLauncher_Mortar()
//{
//	
//}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
CBaseEntity *CTFRocketLauncher_Mortar::FireProjectile( CTFPlayer *pPlayer )
{
	// Fire the rocket
	CBaseEntity* pRocket = BaseClass::FireProjectile( pPlayer );
	// Add it to my list
#ifdef GAME_DLL
	m_vecRockets.AddToTail( pRocket );
#endif

	return pRocket;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
void CTFRocketLauncher_Mortar::SecondaryAttack( void )
{
	RedirectRockets();
}
//-----------------------------------------------------------------------------
void CTFRocketLauncher_Mortar::ItemPostFrame( void )
{
#ifdef GAME_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->m_nButtons & IN_ATTACK2 )
	{
		// If allowed
		RedirectRockets();
	}
#endif
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
void CTFRocketLauncher_Mortar::ItemBusyFrame( void )
{
#ifdef GAME_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->m_nButtons & IN_ATTACK2 )
	{
		// If allowed
		RedirectRockets();
	}
#endif
	BaseClass::ItemBusyFrame();
}


//-----------------------------------------------------------------------------
void CTFRocketLauncher_Mortar::RedirectRockets( void )
{
#ifdef GAME_DLL
	if ( m_vecRockets.Count() <= 0 )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	Vector vecEye = pOwner->EyePosition();
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pOwner->EyeAngles(), &vecForward, &vecRight, &vecUp );

	trace_t tr;
	UTIL_TraceLine( vecEye, vecEye + vecForward * MAX_TRACE_LENGTH, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
	float flVel = 1100.0f;

	FOR_EACH_VEC_BACK( m_vecRockets, i )
	{
		CBaseEntity* pRocket = m_vecRockets[i].Get();
		// Remove targets that have disappeared
		if ( !pRocket || pRocket->GetOwnerEntity() != GetOwnerEntity() )
		{
			m_vecRockets.Remove( i );
			continue;
		}

		// Give the rocket a new target
		Vector vecDir = pRocket->WorldSpaceCenter() - tr.endpos;
		VectorNormalize( vecDir );

		Vector vecVel = pRocket->GetAbsVelocity();
		vecVel = -flVel * vecDir;
		pRocket->SetAbsVelocity( vecVel );

		QAngle newAngles;
		VectorAngles( -vecDir, newAngles );
		pRocket->SetAbsAngles( newAngles );

		m_vecRockets.Remove( i );
	}
#endif
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// CROSSBOW BEGIN
//----------------------------------------------------------------------------------------------------------------------------------------------------------
bool CTFCrossbow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// Allow Crossbow to silently reload like the flaregun
	if ( m_iClip1 == 0 )
	{
		// These Values need to match the anim times since all this stuff is actually driven by animation sequence time in the base code
		float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );

		float flReloadTime = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReload;
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time );
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, mult_reload_time_hidden );
		CALL_ATTRIB_HOOK_FLOAT( flReloadTime, fast_reload );

		float flIdleTime = GetLastPrimaryAttackTime() + flFireDelay + flReloadTime;
		if ( GetWeaponIdleTime() < flIdleTime )
		{
			SetWeaponIdleTime( flIdleTime );
			m_flNextPrimaryAttack = flIdleTime;
		}

		IncrementAmmo();
	}

	return BaseClass::Holster( pSwitchingTo );
}
//-----------------------------------------------------------------------------
void CTFCrossbow::SecondaryAttack( void )
{
	// If this is the jarate bolt crossbow, make sure we are allowed to do it
	int iMilkBolt = 0;
	CALL_ATTRIB_HOOK_INT( iMilkBolt, fires_milk_bolt );
	if ( iMilkBolt )
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( !pPlayer )
			return;

		if ( !CanAttack() )
			return;

		if ( m_flNextPrimaryAttack > gpGlobals->curtime )
			return;

		// Can we attack
		if ( GetProgress() >= 1.0f )
		{
			// Call Primary Attack and modify the projectile
			m_bMilkNextAttack = true;
			PrimaryAttack();
			m_flRegenerateDuration = iMilkBolt;
			m_flLastUsedTimestamp = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
void CTFCrossbow::ModifyProjectile( CBaseEntity* pProj )
{
#ifdef GAME_DLL
	if ( m_bMilkNextAttack )
	{
		CTFProjectile_Arrow* pMainArrow = assert_cast<CTFProjectile_Arrow*>( pProj );
		if ( pMainArrow )
		{
			pMainArrow->SetApplyMilkOnHit();
		}
	}
#endif

	m_bMilkNextAttack = false;
}
//-----------------------------------------------------------------------------
void CTFCrossbow::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();
	m_bMilkNextAttack = false;
}
//-----------------------------------------------------------------------------
float CTFCrossbow::GetProjectileSpeed( void )
{
	return RemapValClamped( 0.75f, 0.0f, 1.f, 1800, 2600 ); // Temp, if we want to ramp.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFCrossbow::GetProjectileGravity( void )
{
	return RemapValClamped( 0.75f, 0.0f, 1.f, 0.5, 0.1 ); // Temp, if we want to ramp.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFCrossbow::IsViewModelFlipped( void )
{
	return !BaseClass::IsViewModelFlipped(); // Invert because arrows are backwards by default.
}
//-----------------------------------------------------------------------------
void CTFCrossbow::WeaponRegenerate( void )
{
	BaseClass::WeaponRegenerate();
	m_flLastUsedTimestamp = 0;
}
//-----------------------------------------------------------------------------
inline float CTFCrossbow::GetProgress( void )
{
	int iMilkBolt = 0;
	CALL_ATTRIB_HOOK_INT( iMilkBolt, fires_milk_bolt );
	if ( iMilkBolt == 0 )
		return 0;

	float meltedTime = gpGlobals->curtime - m_flLastUsedTimestamp;
	return meltedTime / m_flRegenerateDuration;
}

