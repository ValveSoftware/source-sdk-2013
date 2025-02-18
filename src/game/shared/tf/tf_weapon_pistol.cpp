//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_pistol.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_tf_gamestats.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

//=============================================================================
//
// Weapon Pistol tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol, DT_WeaponPistol )

BEGIN_NETWORK_TABLE( CTFPistol, DT_WeaponPistol )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol, CTFPistol );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFPistol )
END_DATADESC()
#endif

//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_Scout, DT_WeaponPistol_Scout )

BEGIN_NETWORK_TABLE( CTFPistol_Scout, DT_WeaponPistol_Scout )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_Scout )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol_scout, CTFPistol_Scout );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol_scout );

//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_ScoutPrimary, DT_WeaponPistol_ScoutPrimary )

BEGIN_NETWORK_TABLE( CTFPistol_ScoutPrimary, DT_WeaponPistol_ScoutPrimary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_ScoutPrimary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_handgun_scout_primary, CTFPistol_ScoutPrimary );
PRECACHE_WEAPON_REGISTER( tf_weapon_handgun_scout_primary );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPistol_ScoutPrimary::CTFPistol_ScoutPrimary()
{
	m_flPushTime = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPistol_ScoutPrimary::PlayWeaponShootSound( void )
{
	BaseClass::PlayWeaponShootSound();

	if ( TFGameRules()->GameModeUsesUpgrades() )
	{
		PlayUpgradedShootSound( "Weapon_Upgrade.DamageBonus" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPistol_ScoutPrimary::SecondaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;

	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	SendWeaponAnim( ACT_SECONDARY_VM_ALTATTACK );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.6f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.5f;
	m_flPushTime = gpGlobals->curtime + 0.2f;	// Anim delay

	EmitSound( "Weapon_Hands.Push" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPistol_ScoutPrimary::Push( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

#ifdef GAME_DLL
	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );

	CUtlVector< CTFPlayer* > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( pOwner->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	for ( int i = 0; i < enemyVector.Count(); ++i )
	{
		CTFPlayer *pVictim = enemyVector[i];

		if ( !pVictim->IsAlive() )
			continue;

		if ( pVictim == pOwner )
			continue;

		if ( pVictim->InSameTeam( pOwner ) )
			continue;

		if ( TFGameRules() && TFGameRules()->IsTruceActive() && pOwner->IsTruceValidForEnt() )
			continue;

		if ( ( pOwner->GetAbsOrigin()- pVictim->GetAbsOrigin() ).LengthSqr() > ( 128.f * 128.f ) )
			continue;

		if ( !pOwner->FVisible( pVictim, MASK_SOLID ) )
			continue;

		Vector vecEyes = pOwner->EyePosition();
		Vector vecForward;
		AngleVectors( pOwner->EyeAngles(), &vecForward );
		CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
		const Vector vHull = Vector( 16.f, 16.f, 16.f );
		trace_t trace;

		float flDist = 50.f;
		UTIL_TraceHull( vecEyes, vecEyes + vecForward * flDist,  -vHull, vHull, MASK_SOLID, &traceFilter, &trace );
		
		bool bDebug = false;
		if ( bDebug )
		{
			NDebugOverlay::SweptBox( vecEyes, vecEyes + vecForward * flDist, -vHull, vHull, pOwner->EyeAngles(), 255, 0, 0, 40, 5 );
		}

		if ( trace.m_pEnt && trace.m_pEnt == pVictim && trace.fraction < 1.f )
		{
			Vector vecToVictim = pVictim->GetAbsOrigin() - pOwner->GetAbsOrigin();
			VectorNormalize( vecToVictim );
			pVictim->ApplyGenericPushbackImpulse( vecToVictim * 400.f, pOwner );
			float flDamage = 1.f;
			CTakeDamageInfo info( pVictim, pOwner, this, flDamage, DMG_MELEE | DMG_NEVERGIB | DMG_CLUB, TF_DMG_CUSTOM_NONE );
			CalculateMeleeDamageForce( &info, vecForward, GetAbsOrigin() + vecForward * flDist, 1.f / flDamage * 80.f );
			pVictim->DispatchTraceAttack( info, vecForward, &trace );
			ApplyMultiDamage();

			CPVSFilter filter( vecToVictim );
			EmitSound( "Weapon_Hands.PushImpact" );

			// Make sure we get credit for the push if the target falls to its death
			pVictim->m_AchievementData.AddDamagerToHistory( pOwner );

			break;			
		}
	}

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, IsCurrentAttackACrit() );

	lagcompensation->FinishLagCompensation( pOwner );
#else
	C_CTF_GameStats.Event_PlayerFiredWeapon( pOwner, IsCurrentAttackACrit() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFPistol_ScoutPrimary::ItemPostFrame()
{
	// Check for smack.
	if ( m_flPushTime > -1.f && gpGlobals->curtime > m_flPushTime )
	{
		Push();
		m_flPushTime = -1.f;
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPistol_ScoutPrimary::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flPushTime = -1.f;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPistol_ScoutPrimary::Precache( void )
{
	PrecacheScriptSound( "Weapon_Hands.Push" );
	PrecacheScriptSound( "Weapon_Hands.PushImpact" );
	
	BaseClass::Precache();
}

//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_ScoutSecondary, DT_WeaponPistol_ScoutSecondary )

BEGIN_NETWORK_TABLE( CTFPistol_ScoutSecondary, DT_WeaponPistol_ScoutSecondary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_ScoutSecondary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_handgun_scout_secondary, CTFPistol_ScoutSecondary );
PRECACHE_WEAPON_REGISTER( tf_weapon_handgun_scout_secondary );

//-----------------------------------------------------------------------------
int	CTFPistol_ScoutSecondary::GetDamageType( void ) const
{
	int iBackheadshot = 0;
	CALL_ATTRIB_HOOK_INT( iBackheadshot, back_headshot );
	if ( iBackheadshot )
	{
		return BaseClass::GetDamageType() | DMG_USE_HITLOCATIONS;	
	}
	return BaseClass::GetDamageType();
}
