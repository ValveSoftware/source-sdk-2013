//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_throwable.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "basetypes.h"
#include "tf_weaponbase_gun.h"
#include "effect_dispatch_data.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "te_effect_dispatch.h"
#include "bone_setup.h"
#include "tf_target_dummy.h"
#endif


// Base
// Launcher
IMPLEMENT_NETWORKCLASS_ALIASED( TFThrowable, DT_TFWeaponThrowable )
BEGIN_NETWORK_TABLE( CTFThrowable, DT_TFWeaponThrowable )
#ifdef CLIENT_DLL
RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
#else
SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFThrowable )
END_PREDICTION_DATA()

//LINK_ENTITY_TO_CLASS( tf_weapon_throwable, CTFThrowable );
//PRECACHE_WEAPON_REGISTER( tf_weapon_throwable );


// Projectile
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Throwable, DT_TFProjectile_Throwable )
BEGIN_NETWORK_TABLE( CTFProjectile_Throwable, DT_TFProjectile_Throwable )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_throwable, CTFProjectile_Throwable );
PRECACHE_WEAPON_REGISTER( tf_projectile_throwable );

// Projectile Repel
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_ThrowableRepel, DT_TFProjectile_ThrowableRepel )
BEGIN_NETWORK_TABLE( CTFProjectile_ThrowableRepel, DT_TFProjectile_ThrowableRepel )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_throwable_repel, CTFProjectile_ThrowableRepel );
PRECACHE_WEAPON_REGISTER( tf_projectile_throwable_repel );

// Projectile Brick
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_ThrowableBrick, DT_TFProjectile_ThrowableBrick )
BEGIN_NETWORK_TABLE( CTFProjectile_ThrowableBrick, DT_TFProjectile_ThrowableBrick )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_throwable_brick, CTFProjectile_ThrowableBrick );
PRECACHE_WEAPON_REGISTER( tf_projectile_throwable_brick );

// Projectile Bread Monster
IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_ThrowableBreadMonster, DT_TFProjectile_ThrowableBreadMonster )
BEGIN_NETWORK_TABLE( CTFProjectile_ThrowableBreadMonster, DT_TFProjectile_ThrowableBreadMonster )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_throwable_breadmonster, CTFProjectile_ThrowableBreadMonster );
PRECACHE_WEAPON_REGISTER( tf_projectile_throwable_breadmonster );


#define TF_GRENADE_TIMER			"Weapon_Grenade.Timer"
#define TF_GRENADE_CHARGE			"Weapon_LooseCannon.Charge"

//****************************************************************************
// Throwable Weapon
//****************************************************************************
CTFThrowable::CTFThrowable( void )
{
	m_flChargeBeginTime = -1.0f;
}

//-----------------------------------------------------------------------------
void CTFThrowable::Precache()
{
	BaseClass::Precache();

	PrecacheModel( g_pszArrowModels[MODEL_BREAD_MONSTER] );
	PrecacheModel( g_pszArrowModels[MODEL_THROWING_KNIFE] );
	PrecacheScriptSound( TF_GRENADE_CHARGE );
	
	PrecacheScriptSound( "Weapon_bm_throwable.throw" );
	PrecacheScriptSound( "Weapon_bm_throwable.smash" );

	PrecacheParticleSystem( "grenade_smoke_cycle" );
	PrecacheParticleSystem( "blood_bread_biting" );
}

//-----------------------------------------------------------------------------
float CTFThrowable::InternalGetEffectBarRechargeTime( void )
{
	float flRechargeTime = 0;
	CALL_ATTRIB_HOOK_FLOAT( flRechargeTime, throwable_recharge_time );
	if ( flRechargeTime )
		return flRechargeTime;
	return 10.0f; // default
}

//-----------------------------------------------------------------------------
float CTFThrowable::GetDetonationTime()
{
	float flDetonationTime = 0;
	CALL_ATTRIB_HOOK_FLOAT( flDetonationTime, throwable_detonation_time );
	if ( flDetonationTime )
		return flDetonationTime;
	return 5.0f; // default 
}

//-----------------------------------------------------------------------------
void CTFThrowable::PrimaryAttack( void )
{
	if ( !CanCharge() )
	{
		// Fire
		BaseClass::PrimaryAttack();
		return;
	}

	if ( m_flChargeBeginTime > 0 )
		return;
	
	// Do all the Checks and start a charged (primed) attack
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Check for ammunition.
	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) < 1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( pPlayer->GetWaterLevel() == WL_Eyes )
		return;

	if ( !CanAttack() )
		return;

	if ( m_flChargeBeginTime <= 0 )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
		SendWeaponAnim( ACT_VM_PULLBACK );	// TODO : Anim!
#ifdef GAME_DLL
		// save that we had the attack button down
		m_flChargeBeginTime = gpGlobals->curtime;
#endif // GAME_LL
		
#ifdef CLIENT_DLL
		if ( pPlayer == C_BasePlayer::GetLocalPlayer() )
		{
			int iCanBeCharged = 0;
			CALL_ATTRIB_HOOK_INT( iCanBeCharged, is_throwable_chargeable );
			if ( iCanBeCharged )
			{
				EmitSound( TF_GRENADE_CHARGE );
			}
			else 
			{
				EmitSound( TF_GRENADE_TIMER );
			}
		}
#endif // CLIENT_DLL
	}
}

//-----------------------------------------------------------------------------
void CTFThrowable::ItemPostFrame( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( m_flChargeBeginTime > 0.f && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		bool bFiredWeapon = false;
		// If we're not holding down the attack button, launch our grenade
		if ( !( pPlayer->m_nButtons & IN_ATTACK ) )
		{
			FireProjectile( pPlayer );
			bFiredWeapon = true;
		}
		// Misfire
		else if ( m_flChargeBeginTime + GetDetonationTime() < gpGlobals->curtime )
		{
			CTFProjectile_Throwable * pThrowable = dynamic_cast<CTFProjectile_Throwable*>( FireProjectile( pPlayer ) );
			if ( pThrowable )
			{
#ifdef GAME_DLL
				pThrowable->Misfire();
#endif // GAME_DLL
			}

			bFiredWeapon = true;
		}

		if ( bFiredWeapon )
		{
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
			pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifdef GAME_DLL
			m_flChargeBeginTime = -1.0f; // reset
#endif // GAME_DLL
			// Set next attack times.
			float flFireDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
			m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
			SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
#ifdef CLIENT_DLL
			int iCanBeCharged = 0;
			CALL_ATTRIB_HOOK_INT( iCanBeCharged, is_throwable_chargeable );
			if ( iCanBeCharged )
			{
				StopSound( TF_GRENADE_CHARGE );
			}
#endif // CLIENT_DLL
		}
	}
	BaseClass::ItemPostFrame();
}

// ITFChargeUpWeapon
//-----------------------------------------------------------------------------
// Primable is for timed explosions
// Charagable is for things like distance or power increases
// Can't really have both but can have neither
bool CTFThrowable::CanCharge()
{
	int iCanBePrimed = 0;
	CALL_ATTRIB_HOOK_INT( iCanBePrimed, is_throwable_primable );

	int iCanBeCharged = 0;
	CALL_ATTRIB_HOOK_INT( iCanBeCharged, is_throwable_chargeable );

	return iCanBeCharged || iCanBePrimed ;
}

//-----------------------------------------------------------------------------
float CTFThrowable::GetChargeBeginTime( void )
{
	float flDetonateTimeLength = GetDetonationTime();
//	float flModDetonateTimeLength = 0;
	
	int iCanBePrimed = 0;
	CALL_ATTRIB_HOOK_INT( iCanBePrimed, is_throwable_primable );

	// Use reverse logic for primable grenades (Counts down to boom)
	// Full charge since we haven't fired
	if ( iCanBePrimed )
	{
		if ( m_flChargeBeginTime < 0 )
		{
			return gpGlobals->curtime - flDetonateTimeLength;
		}
		return gpGlobals->curtime - Clamp( m_flChargeBeginTime + flDetonateTimeLength - gpGlobals->curtime, 0.f, flDetonateTimeLength );
	}

	return m_flChargeBeginTime;
}

//-----------------------------------------------------------------------------
float CTFThrowable::GetChargeMaxTime( void )
{
	return GetDetonationTime();
}
//-----------------------------------------------------------------------------
CBaseEntity *CTFThrowable::FireJar( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	return FireProjectileInternal();
#endif
	return NULL;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
void CTFThrowable::TossJarThink( void )
{
	FireProjectileInternal();
}

//-----------------------------------------------------------------------------
CTFProjectile_Throwable *CTFThrowable::FireProjectileInternal( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return NULL;

	CAttribute_String attrProjectileEntityName;
	GetProjectileEntityName( &attrProjectileEntityName );
	if ( !attrProjectileEntityName.has_value() )
		return NULL;

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );

	float fRight = 8.f;
	if ( IsViewModelFlipped() )
	{
		fRight *= -1;
	}
	Vector vecSrc = pPlayer->Weapon_ShootPosition();

	// Make spell toss position at the hand
	vecSrc = vecSrc + ( vecUp * -9.0f ) + ( vecRight * 7.0f ) + ( vecForward * 3.0f );

	trace_t trace;
	Vector vecEye = pPlayer->EyePosition();
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceHull( vecEye, vecSrc, -Vector( 8, 8, 8 ), Vector( 8, 8, 8 ), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

	// If we started in solid, don't let them fire at all
	if ( trace.startsolid )
		return NULL;

	CalcIsAttackCritical();

	// Create the Grenade and Intialize it appropriately
	CTFProjectile_Throwable *pGrenade = static_cast<CTFProjectile_Throwable*>( CBaseEntity::CreateNoSpawn( attrProjectileEntityName.value().c_str(), trace.endpos, pPlayer->EyeAngles(), pPlayer ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly.
		pGrenade->SetPipebombMode();
		pGrenade->SetLauncher( this );
		pGrenade->SetCritical( IsCurrentAttackACrit() );

		DispatchSpawn( pGrenade );

		// Calculate a charge percentage
		// For now Charge just effects exit velocity
		int iCanBeCharged = 0;
		float flChargePercent = 0;
		float flDetonateTime = GetDetonationTime();
		CALL_ATTRIB_HOOK_INT( iCanBeCharged, is_throwable_chargeable );
		if ( iCanBeCharged )
		{
			flChargePercent = RemapVal( gpGlobals->curtime, m_flChargeBeginTime, m_flChargeBeginTime + flDetonateTime, 0.0f, 1.0f );
		}

		Vector vecVelocity = pGrenade->GetVelocityVector( vecForward, vecRight, vecUp, flChargePercent );
		AngularImpulse angVelocity = pGrenade->GetAngularImpulse();

		pGrenade->InitGrenade( vecVelocity, angVelocity, pPlayer, GetTFWpnData() );
		pGrenade->InitThrowable( flChargePercent );
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		
		if ( flDetonateTime > 0 )
		{
			// Check if this has been primed
			int iCanBePrimed = 0;
			CALL_ATTRIB_HOOK_INT( iCanBePrimed, is_throwable_primable );
			if ( m_flChargeBeginTime > 0 && iCanBePrimed > 0 )
			{
				flDetonateTime = ( m_flChargeBeginTime + flDetonateTime - gpGlobals->curtime );
			}
			pGrenade->SetDetonateTimerLength( flDetonateTime );
		}
		pGrenade->m_flFullDamage = 0;

		if ( pGrenade->GetThrowSoundEffect() )
		{
			pGrenade->EmitSound( pGrenade->GetThrowSoundEffect() );
		}
	}

	StartEffectBarRegen();

	return pGrenade;
}
#endif // GAME_DLL

//----------------------------------------------------------------------------------------------------------------------------------------------------------
// Throwable Projectile
//----------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef GAME_DLL
CTFProjectile_Throwable::CTFProjectile_Throwable( void )
{
	m_flChargePercent = 0;
	m_bHit = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
// Get Initial Velocity
Vector CTFProjectile_Throwable::GetVelocityVector( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, float flCharge )
{
	// Scale the projectile speed up to a maximum of 3000?
	float flSpeed = RemapVal( flCharge, 0, 1.0f, GetProjectileSpeed(), GetProjectileMaxSpeed() );

	return ( ( flSpeed * vecForward ) + 
		( ( random->RandomFloat( -10.0f, 10.0f ) + 200.0f ) * vecUp ) + 
		(   random->RandomFloat( -10.0f, 10.0f ) * vecRight ) );
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------
void CTFProjectile_Throwable::OnHit( CBaseEntity *pOther ) 
{ 
	if ( m_bHit )
		return;

	if ( ExplodesOnHit() )
	{
		Explode();
	}

	m_bHit = true;
}
//-----------------------------------------------------------------------------
void CTFProjectile_Throwable::Explode()
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!
	SetThink( NULL );
	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);
	Explode( &tr, GetDamageType() );
}

//-----------------------------------------------------------------------------
void CTFProjectile_Throwable::Explode( trace_t *pTrace, int bitsDamageType )
{
	if ( GetThrower() )
	{
		InitialExplodeEffects( NULL, pTrace );

		// Particle
		const char* pszExplodeEffect = GetExplodeEffectParticle();
		if ( pszExplodeEffect && pszExplodeEffect[0] != '\0' )
		{	
			CPVSFilter filter( GetAbsOrigin() );
			TE_TFParticleEffect( filter, 0.0, pszExplodeEffect, GetAbsOrigin(), vec3_angle );
		}

		// Sounds
		const char* pszSoundEffect = GetExplodeEffectSound();
		if ( pszSoundEffect && pszSoundEffect[0] != '\0' )
		{	
			EmitSound( pszSoundEffect );
		}
	}

	SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
}

//-----------------------------------------------------------------------------
// THROWABLE REPEL
//-----------------------------------------------------------------------------
void CTFProjectile_ThrowableRepel::OnHit( CBaseEntity *pOther ) 
{ 
	if ( m_bHit )
		return;

	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer*>( pOther );

	if ( pPlayer && !pPlayer->InSameTeam( GetThrower() ) )
	{
		if ( pPlayer->m_Shared.IsImmuneToPushback() )
			return;

		CTraceFilterIgnoreTeammates tracefilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		trace_t trace;
		UTIL_TraceLine( GetAbsOrigin(), pPlayer->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &tracefilter, &trace );

		// Apply AirBlast Force
		Vector vecToTarget;
		vecToTarget = pPlayer->GetAbsOrigin() - this->GetAbsOrigin();
		vecToTarget.z = 0;
		VectorNormalize( vecToTarget );

		float flForce = 300.0f * m_flChargePercent + 350.0f;
		pPlayer->ApplyGenericPushbackImpulse( vecToTarget * flForce + Vector( 0, 0, flForce ), ToTFPlayer( GetThrower() ) );
		pPlayer->ApplyPunchImpulseX( RandomInt( -50, -30 ) );

		// Apply Damage to Victim
		CTakeDamageInfo info;
		info.SetAttacker( GetThrower() );
		info.SetInflictor( this ); 
		info.SetWeapon( GetLauncher() );
		info.SetDamage( GetDamage() );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( this->GetAbsOrigin() );
		info.SetDamageType( DMG_CLUB | DMG_PREVENT_PHYSICS_FORCE );

		//Vector dir;
		//AngleVectors( GetAbsAngles(), &dir );

		pPlayer->DispatchTraceAttack( info, vecToTarget, &trace );
		ApplyMultiDamage();
	}

	BaseClass::OnHit( pOther );
}
//-----------------------------------------------------------------------------
// THROWABLE BRICK
//-----------------------------------------------------------------------------
void CTFProjectile_ThrowableBrick::OnHit( CBaseEntity *pOther ) 
{ 
	if ( m_bHit )
		return;

	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer*>( pOther );

	if ( pPlayer && !pPlayer->InSameTeam( GetThrower() ) )
	{
		CTraceFilterIgnoreTeammates tracefilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		trace_t trace;
		UTIL_TraceLine( GetAbsOrigin(), pPlayer->GetAbsOrigin(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &tracefilter, &trace );

		Vector vecToTarget;
		vecToTarget = pPlayer->WorldSpaceCenter() - this->WorldSpaceCenter();
		VectorNormalize( vecToTarget );

		// Apply Damage to Victim
		CTakeDamageInfo info;
		info.SetAttacker( GetThrower() );
		info.SetInflictor( this ); 
		info.SetWeapon( GetLauncher() );
		info.SetDamage( GetDamage() );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamageType( DMG_CLUB );

		pPlayer->DispatchTraceAttack( info, vecToTarget, &trace );
		pPlayer->ApplyPunchImpulseX( RandomInt( 15, 20 ) );
		ApplyMultiDamage();
	}

	BaseClass::OnHit( pOther );
}
//-----------------------------------------------------------------------------
// THROWABLE BREADMONSTER
//-----------------------------------------------------------------------------
void CTFProjectile_ThrowableBreadMonster::OnHit( CBaseEntity *pOther ) 
{ 
	if ( m_bHit )
		return;

	CTFPlayer *pVictim = dynamic_cast< CTFPlayer*>( pOther );
	CTFPlayer *pOwner = dynamic_cast< CTFPlayer*>( GetThrower() );

	if ( pVictim && pOwner && !pVictim->InSameTeam( pOwner ) )
	{
		m_bHit = true;
		CTraceFilterIgnoreTeammates tracefilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		trace_t trace;
		Vector vEndPos = pVictim->WorldSpaceCenter();
		vEndPos.z = WorldSpaceCenter().z + 1.0f;
		UTIL_TraceLine( WorldSpaceCenter(), vEndPos, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &tracefilter, &trace );

		Vector vecToTarget;
		vecToTarget = pVictim->WorldSpaceCenter() - this->WorldSpaceCenter();
		VectorNormalize( vecToTarget );

		// Apply Damage to Victim
		CTakeDamageInfo info;
		info.SetAttacker( GetThrower() );
		info.SetInflictor( this );
		info.SetWeapon( GetLauncher() );
		info.SetDamage( GetDamage() );
		info.SetDamageCustom( GetCustomDamageType() );
		info.SetDamagePosition( GetAbsOrigin() );
		
		int iDamageType = DMG_CLUB;
		if ( IsCritical() )
		{
			iDamageType |= DMG_CRITICAL;
		}
		info.SetDamageType( iDamageType );

		pVictim->DispatchTraceAttack( info, vecToTarget, &trace );
		pVictim->ApplyPunchImpulseX( RandomInt( 15, 20 ) );
		pVictim->m_Shared.MakeBleed( pOwner, dynamic_cast< CTFWeaponBase * >( GetLauncher() ), 5.0f, 1.0f );
		ApplyMultiDamage();

		// Bread Particle
		CPVSFilter filter( vEndPos );
		TE_TFParticleEffect( filter, 0.0, "blood_bread_biting", vEndPos, vec3_angle );

		// Attach Breadmonster to Victim
		CreateStickyAttachmentToTarget( pOwner, pVictim, &trace );

		BaseClass::Explode();
		return;
	}
	else // its a dud
	{
		BaseClass::Explode();
		return;
	}
	BaseClass::OnHit( pOther );
}

//-----------------------------------------------------------------------------
void CTFProjectile_ThrowableBreadMonster::Detonate() 
{
	SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
}

//-----------------------------------------------------------------------------
void CTFProjectile_ThrowableBreadMonster::Explode( trace_t *pTrace, int bitsDamageType )
{
	if ( !m_bHit )
	{
		// TODO, Spawn Debris / Flopping BreadInstead
		trace_t tr;
		Vector velDir = m_vCollisionVelocity;
		VectorNormalize( velDir );
		Vector vecSpot = GetAbsOrigin() - velDir * 32;
		UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );
		if ( tr.fraction < 1.0 && tr.surface.flags & SURF_SKY )
		{
			// We hit the skybox, go away soon.
			return;
		}

		// Create a breadmonster in the world
		CEffectData	data;
		data.m_vOrigin = tr.endpos;
		data.m_vNormal = velDir;
		data.m_nEntIndex = 0;
		data.m_nAttachmentIndex = 0;
		data.m_nMaterial = 0;
		data.m_fFlags = TF_PROJECTILE_BREAD_MONSTER;
		data.m_nColor = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

		DispatchEffect( "TFBoltImpact", data );
	}

	BaseClass::Explode( pTrace, bitsDamageType );
}

#endif // GAME_DLL
//
//#ifdef CLIENT_DLL
//
//static CUtlMap< const char*, CUtlString > s_TeamParticleMap;
//static bool s_TeamParticleMapInited = false;
//
////-----------------------------------------------------------------------------
//const char *CTFProjectile_Throwable::GetTrailParticleName( void )
//{
//	// Check for Particles
//	int iDynamicParticleEffect = 0;
//	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetLauncher(), iDynamicParticleEffect, set_attached_particle );
//	if ( iDynamicParticleEffect > 0 )
//	{
//		// Init Map Once
//		if ( !s_TeamParticleMapInited )
//		{
//			SetDefLessFunc( s_TeamParticleMap );
//			s_TeamParticleMapInited = true;
//		}
//
//		attachedparticlesystem_t *pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iDynamicParticleEffect );
//		if ( pParticleSystem )
//		{
//			// TF Team Color Particles
//			const char * pName = pParticleSystem->pszSystemName;
//			if ( GetTeamNumber() == TF_TEAM_BLUE && V_stristr( pName, "_teamcolor_red" ))
//			{
//				int index = s_TeamParticleMap.Find( pName );
//				if ( !s_TeamParticleMap.IsValidIndex( index ) )
//				{
//					char pBlue[256];
//					V_StrSubst( pName, "_teamcolor_red", "_teamcolor_blue", pBlue, 256 );
//					CUtlString pBlueString( pBlue );
//					index = s_TeamParticleMap.Insert( pName, pBlueString );
//				}
//				return s_TeamParticleMap[index].String();
//			}
//			else if ( GetTeamNumber() == TF_TEAM_RED && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_blue" ))
//			{
//				// Guard against accidentally giving out the blue team color (support tool)
//				int index = s_TeamParticleMap.Find( pName );
//				if ( !s_TeamParticleMap.IsValidIndex( index ) )
//				{
//					char pRed[256];
//					V_StrSubst( pName, "_teamcolor_blue", "_teamcolor_red", pRed, 256 );
//					CUtlString pRedString( pRed );
//					index = s_TeamParticleMap.Insert( pName, pRedString );
//				}
//				return s_TeamParticleMap[index].String();
//			}
//
//			return pName;
//		}
//	}
//
//	if ( GetTeamNumber() == TF_TEAM_BLUE )
//	{
//		return "trail_basic_blue";
//	}
//	else
//	{
//		return "trail_basic_red";
//	}
//}
//
//#endif // CLIENT_DLL

//----------------------------------------------------------------------------------------------------------------------------------------------------------
