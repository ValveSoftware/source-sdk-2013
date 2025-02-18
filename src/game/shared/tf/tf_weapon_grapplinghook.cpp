//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Grappling Hook
//
//=============================================================================
#include "cbase.h"
#include "in_buttons.h"
#include "tf_weapon_grapplinghook.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "gc_clientsystem.h"
#include "prediction.h"
#include "soundenvelope.h"
// Server specific.
#else
#include "tf_player.h"
#include "entity_rune.h"
#include "effect_dispatch_data.h"
#include "tf_fx.h"
#include "func_respawnroom.h"
#endif

//=============================================================================
//
// Grappling hook tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrapplingHook, DT_GrapplingHook )

BEGIN_NETWORK_TABLE( CTFGrapplingHook, DT_GrapplingHook )
#ifdef GAME_DLL
	SendPropEHandle( SENDINFO( m_hProjectile ) ),
#else // GAME_DLL
	RecvPropEHandle( RECVINFO( m_hProjectile ) ),
#endif // CLIENT_DLL
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrapplingHook )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grapplinghook, CTFGrapplingHook );
PRECACHE_WEAPON_REGISTER( tf_weapon_grapplinghook );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFGrapplingHook )
END_DATADESC()
#endif // CLIENT_DLL

// This is basically a copy of s_acttableMeleeAllclass table except primary fire to use grappling hook specific
acttable_t s_grapplinghook_normal_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_MELEE_ALLCLASS,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_MELEE_ALLCLASS,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_MELEE_ALLCLASS,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_MELEE_ALLCLASS,				false },
	{ ACT_MP_AIRWALK,			ACT_GRAPPLE_PULL_IDLE,					false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_MELEE_ALLCLASS,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_MELEE_ALLCLASS,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_MELEE_ALLCLASS,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_GRAPPLE_FIRE_START,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_GRAPPLE_FIRE_START,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_GRAPPLE_FIRE_START,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_GRAPPLE_FIRE_START,		false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },
};

// This is basically a copy of s_acttableSecondary table except primary fire to use grappling hook specific
acttable_t s_grapplinghook_engineer_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_SECONDARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_SECONDARY,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_SECONDARY,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_SECONDARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_SECONDARY,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_SECONDARY,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_SECONDARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_SECONDARY,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_SECONDARY,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_SECONDARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_SECONDARY,				false },
	{ ACT_MP_DOUBLEJUMP_CROUCH,	ACT_MP_DOUBLEJUMP_CROUCH_SECONDARY,	false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_GRAPPLE_FIRE_START,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_GRAPPLE_FIRE_START,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_GRAPPLE_FIRE_START,	false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_GRAPPLE_FIRE_START,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_SECONDARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_SECONDARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP,false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_SECONDARY,		false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_SECONDARY,	false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_SECONDARY_END,false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_SECONDARY, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_SECONDARY_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_SECONDARY_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_SECONDARY_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_SECONDARY_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_SECONDARY_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_SECONDARY_GRENADE2_ATTACK,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_SECONDARY,	false },
};

acttable_t *CTFGrapplingHook::ActivityList( int &iActivityCount )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		if ( pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			iActivityCount = ARRAYSIZE( s_grapplinghook_engineer_acttable );
			return s_grapplinghook_engineer_acttable;
		}
		else
		{
			iActivityCount = ARRAYSIZE( s_grapplinghook_normal_acttable );
			return s_grapplinghook_normal_acttable;
		}
	}

	return BaseClass::ActivityList( iActivityCount );
}


poseparamtable_t s_grapplinghook_normal_poseparamtable[] =
{
	{ "R_hand_grip", 14 },
	{ "R_arm", 2 },
};

poseparamtable_t s_grapplinghook_engineer_poseparamtable[] =
{
	{ "r_handposes_engineer", 1 },
};

poseparamtable_t *CTFGrapplingHook::GetPlayerPoseParamList( int &iPoseParamCount )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		if ( pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			iPoseParamCount = ARRAYSIZE( s_grapplinghook_engineer_poseparamtable );
			return s_grapplinghook_engineer_poseparamtable;
		}
		else
		{
			iPoseParamCount = ARRAYSIZE( s_grapplinghook_normal_poseparamtable );
			return s_grapplinghook_normal_poseparamtable;
		}
	}

	return BaseClass::GetPlayerPoseParamList( iPoseParamCount );
}


ConVar tf_grapplinghook_projectile_speed( "tf_grapplinghook_projectile_speed", "1500", FCVAR_REPLICATED | FCVAR_CHEAT, "How fast does the grappliing hook projectile travel" );
ConVar tf_grapplinghook_max_distance( "tf_grapplinghook_max_distance", "2000", FCVAR_REPLICATED | FCVAR_CHEAT, "Valid distance for grappling hook to travel" );
ConVar tf_grapplinghook_fire_delay( "tf_grapplinghook_fire_delay", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );

float m_flNextSupernovaDenyWarning = 0.f;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGrapplingHook::CTFGrapplingHook()
{
#ifdef GAME_DLL
	m_bReleasedAfterLatched = false;
#endif // GAME_DLL

#ifdef CLIENT_DLL
	m_pHookSound = NULL;
	m_bLatched = false;
#endif // CLIENT_DLL
}


void CTFGrapplingHook::Precache()
{
	BaseClass::Precache();

#ifdef GAME_DLL
	PrecacheScriptSound( "WeaponGrapplingHook.Shoot" );
#endif // GAME_DLL
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFGrapplingHook::FireProjectile( CTFPlayer *pPlayer )
{
#ifdef GAME_DLL
	Assert( m_hProjectile == NULL );
	m_hProjectile = BaseClass::FireProjectile( pPlayer );

	return m_hProjectile;
#else
	return BaseClass::FireProjectile( pPlayer );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::ItemPostFrame( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if (!pOwner)
		return;

#ifdef CLIENT_DLL
	static bool bFired = false;
#endif // CLIENT_DLL

	CBaseEntity *pHookTarget = pOwner->GetGrapplingHookTarget();
	bool bJump = ( pOwner->m_nButtons & IN_JUMP ) > 0;
	bool bForceReleaseHook = pHookTarget != NULL && bJump;

	if ( !bForceReleaseHook && ( pOwner->m_nButtons & IN_ATTACK || pOwner->IsUsingActionSlot() ) )
	{
#ifdef CLIENT_DLL
		if ( !bFired )
		{
			PrimaryAttack();
			bFired = true;
		}
#else
		PrimaryAttack();
#endif // CLIENT_DLL

		if ( pOwner->GetGrapplingHookTarget() )
		{
			SendWeaponAnim( ACT_GRAPPLE_PULL_START );
		}
		else if ( m_hProjectile )
		{
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
		}
		else
		{
			SendWeaponAnim( ACT_GRAPPLE_IDLE );
		}
	}
	else
	{
#ifdef CLIENT_DLL
		bFired = false;
#endif // CLIENT_DLL

		OnHookReleased( bForceReleaseHook );
	}

	if ( pOwner->GetViewModel(0) )
	{
		pOwner->GetViewModel(0)->SetPlaybackRate( 1.f );
	}
	if ( pOwner->GetViewModel(1) )
	{
		pOwner->GetViewModel(1)->SetPlaybackRate( 1.f );
	}

	BaseClass::ItemPostFrame();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGrapplingHook::CanAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer->m_Shared.IsFeignDeathReady() )
		return false;

	return BaseClass::CanAttack();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::PrimaryAttack( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();

#ifdef GAME_DLL
	// make sure to unlatch from the current target and remove the old projectile before we fire a new one
	if ( m_bReleasedAfterLatched )
	{
		RemoveHookProjectile( true );
	}
#endif // GAME_DLL

	if ( m_hProjectile )
		return;

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( pOwner && pOwner->m_Shared.IsControlStunned() )
		return;

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, -8.0f, -3.0f ); // copied from CTFWeaponBaseGun::FireArrow
	GetProjectileFireSetup( pOwner, vecOffset, &vecSrc, &angForward, false );
	Vector vecForward;
	AngleVectors( angForward, &vecForward );

	// check if aiming at skybox
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc + tf_grapplinghook_max_distance.GetFloat() * vecForward, MASK_SOLID, pOwner, COLLISION_GROUP_DEBRIS, &tr );
	if ( !tr.DidHit() || ( tr.fraction < 1.0 && tr.surface.flags & SURF_SKY ) )
	{
#ifdef CLIENT_DLL
		// play fail sound on client here
		if ( pOwner && prediction->IsFirstTimePredicted() )
		{
			pOwner->EmitSound( "Player.DenyWeaponSelection" );
		}
#endif // CLIENT_DLL
		return;
	}

	BaseClass::PrimaryAttack();

	m_flNextPrimaryAttack = gpGlobals->curtime + tf_grapplinghook_fire_delay.GetFloat();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGrapplingHook::Deploy( void )
{
#ifdef GAME_DLL
	RemoveHookProjectile( true );
	m_bReleasedAfterLatched = IsLatchedToTargetPlayer();
#endif // GAME_DLL
	return BaseClass::Deploy();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGrapplingHook::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef GAME_DLL
	RemoveHookProjectile();
	m_bReleasedAfterLatched = IsLatchedToTargetPlayer();
#endif // GAME_DLL

	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /*= true*/, float flEndDist /*= 2000.f*/ )
{
	BaseClass::GetProjectileFireSetup( pPlayer, vecOffset, vecSrc, angForward, bHitTeammates, flEndDist );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFGrapplingHook::GetProjectileSpeed()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	
	if ( pOwner && pOwner->m_Shared.GetCarryingRuneType() == RUNE_AGILITY )
	{
		switch ( pOwner->GetPlayerClass()->GetClassIndex() )
		{
		case TF_CLASS_SOLDIER:
		case TF_CLASS_HEAVYWEAPONS:
			return 2600.f;
		default:
			return 3000.f;
		}
	}

	return tf_grapplinghook_projectile_speed.GetFloat();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGrapplingHook::SendWeaponAnim( int actBase )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return BaseClass::SendWeaponAnim( actBase );

	if ( actBase == ACT_VM_DRAW )
	{
		actBase = ACT_GRAPPLE_DRAW;
	}
	else if ( pPlayer->GetGrapplingHookTarget() )
	{
		if ( GetActivity() != ACT_GRAPPLE_PULL_START && GetActivity() != ACT_GRAPPLE_PULL_IDLE )
		{
			bool bResult = BaseClass::SendWeaponAnim( ACT_GRAPPLE_PULL_START );
			//DevMsg("pull start %f\n", gpGlobals->curtime );

			m_startPullingTimer.Start( SequenceDuration() );

			return bResult;
		}
		else
		{
			if ( GetActivity() == ACT_GRAPPLE_PULL_IDLE )
				return true;

			if ( GetActivity() == ACT_GRAPPLE_PULL_START && m_startPullingTimer.HasStarted() && !m_startPullingTimer.IsElapsed() )
				return true;

			actBase = ACT_GRAPPLE_PULL_IDLE;
			//DevMsg("pull idle %f\n", gpGlobals->curtime );

			m_startPullingTimer.Invalidate();
		}
	}
	else if ( actBase == ACT_VM_PRIMARYATTACK )
	{
		if ( GetActivity() != ACT_GRAPPLE_FIRE_START && GetActivity() != ACT_GRAPPLE_FIRE_IDLE )
		{
			bool bResult = BaseClass::SendWeaponAnim( ACT_GRAPPLE_FIRE_START );
			//DevMsg("fire start %f\n", gpGlobals->curtime );

			m_startFiringTimer.Start( SequenceDuration() );

			return bResult;
		}
		else
		{
			if ( GetActivity() == ACT_GRAPPLE_FIRE_IDLE )
				return true;

			if ( GetActivity() == ACT_GRAPPLE_FIRE_START && m_startFiringTimer.HasStarted() && !m_startFiringTimer.IsElapsed() )
				return true;

			actBase = ACT_GRAPPLE_FIRE_IDLE;
			//DevMsg("fire idle %f\n", gpGlobals->curtime );

			m_startFiringTimer.Invalidate();
		}
	}
	else
	{
		if ( GetActivity() == ACT_GRAPPLE_PULL_IDLE )
		{
			actBase = ACT_GRAPPLE_PULL_END;
			//DevMsg("pull end %f\n", gpGlobals->curtime );
		}
		else
		{
			if ( GetActivity() == ACT_GRAPPLE_IDLE )
				return true;

			actBase = ACT_GRAPPLE_IDLE;
			//DevMsg("grapple idle %f\n", gpGlobals->curtime );
		}
	}

	return BaseClass::SendWeaponAnim( actBase );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::PlayWeaponShootSound( void )
{
#ifdef GAME_DLL
	EmitSound( "WeaponGrapplingHook.Shoot" );
#endif // GAME_DLL
}



#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::ActivateRune()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->m_Shared.IsRuneCharged() )
	{
		RuneTypes_t type = pOwner->m_Shared.GetCarryingRuneType();
		if ( type == RUNE_SUPERNOVA )
		{
			// don't allow stealthed player to activate the power
			if ( pOwner->m_Shared.IsStealthed() )
				return;

			int nEnemyTeam = GetEnemyTeam( pOwner->GetTeamNumber() );

			// apply super nova effect to all enemies
			bool bHitAnyTarget = false;
			CUtlVector< CTFPlayer* > vecPlayers;
			CUtlVector< CTFPlayer* > vecVictims;
			CollectPlayers( &vecPlayers, nEnemyTeam, COLLECT_ONLY_LIVING_PLAYERS );
			const float flEffectRadiusSqr = Sqr( 1500.f );
			const float flMinPushForce = 200.f;
			const float flMaxPushForce = 500.f;
			const float flMinStunDuration = 2.f;
			const float flMaxStunDuration = 4.f;
			for ( int i = 0; i < vecPlayers.Count(); ++i )
			{
				CTFPlayer *pOther = vecPlayers[i];
				Vector toPlayer = pOther->WorldSpaceCenter() - pOwner->WorldSpaceCenter();
				float flDistSqr = toPlayer.LengthSqr();

				// Collect valid enemies
				if ( flDistSqr <= flEffectRadiusSqr && pOwner->IsLineOfSightClear( pOther, CBaseCombatCharacter::IGNORE_ACTORS ) && !PointInRespawnRoom( pOther, pOther->WorldSpaceCenter() ) )
				{
					vecVictims.AddToTail( pOther );
				}
			}

			// if there is more than one victim, the stun duration increases
			float flStunDuration = MIN( flMinStunDuration + ( ( vecVictims.Count() - 1 ) * 0.5 ), flMaxStunDuration );

			for ( int i = 0; i < vecVictims.Count(); ++i )
			{
				// force enemy to drop rune, stun, and push them
				CTFPlayer *pOther = vecVictims[i];
				const char *pszEffect = pOwner->GetTeamNumber() == TF_TEAM_RED ? "powerup_supernova_strike_red" : "powerup_supernova_strike_blue";

				CPVSFilter filter( WorldSpaceCenter() );
				Vector vStart = pOwner->WorldSpaceCenter();
				Vector vEnd = pOther->GetAbsOrigin() + Vector( 0, 0, 56 );
				te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
				TE_TFParticleEffectComplex( filter, 0.f, pszEffect, vStart, QAngle( 0.f, 0.f, 0.f ), NULL, &controlPoint, pOther, PATTACH_CUSTOMORIGIN );

					
				pOther->DropRune( false, pOwner->GetTeamNumber() );
				pOther->DropFlag();

				pOther->m_Shared.StunPlayer( flStunDuration, 1.f, TF_STUN_MOVEMENT | TF_STUN_CONTROLS, pOwner );
	
				// send the player flying
				// make sure we push players up and away
				Vector toPlayer = pOther->WorldSpaceCenter() - pOwner->WorldSpaceCenter();
				toPlayer.z = 0.0f;
				toPlayer.NormalizeInPlace();
				toPlayer.z = 1.0f;

				// scale push force based on distance from the supernova origin
				float flDistSqr = toPlayer.LengthSqr();
				float flPushForce = RemapValClamped( flDistSqr, 0.f, flEffectRadiusSqr, flMaxPushForce, flMinPushForce );
				Vector vPush = flPushForce * toPlayer;
				pOther->ApplyAbsVelocityImpulse( vPush );

				bHitAnyTarget = true;
			}

			// don't deploy with no target
			if ( bHitAnyTarget )
			{
				// play effect
				const char *pszEffect = pOwner->GetTeamNumber() == TF_TEAM_RED ? "powerup_supernova_explode_red" : "powerup_supernova_explode_blue";
				CEffectData	data;
				data.m_nHitBox = GetParticleSystemIndex( pszEffect );
				data.m_vOrigin = pOwner->GetAbsOrigin();
				data.m_vAngles = vec3_angle;

				CPASFilter filter( data.m_vOrigin );
				filter.SetIgnorePredictionCull( true );

				te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );
				pOwner->EmitSound( "Powerup.PickUpSupernovaActivate" );

				// remove the power and reposition instantly
				pOwner->m_Shared.SetCarryingRuneType( RUNE_NONE );
				CTFRune::RepositionRune( type, TEAM_ANY );
			}
			else
			{
				if ( gpGlobals->curtime > m_flNextSupernovaDenyWarning )
				{
					m_flNextSupernovaDenyWarning = gpGlobals->curtime + 0.5f;

					CSingleUserRecipientFilter singleFilter( pOwner );
					EmitSound( singleFilter, pOwner->entindex(), "Player.UseDeny" );
					ClientPrint( pOwner, HUD_PRINTCENTER, "#TF_Powerup_Supernova_Deny" );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::RemoveHookProjectile( bool bForce /*= false*/ )
{
	if ( !bForce && IsLatchedToTargetPlayer() )
	{
		// don't remove the projectile until we unlatched from the target (by hooking again)
		return;
	}

	if ( m_hProjectile )
	{
		UTIL_Remove( m_hProjectile );
		m_hProjectile = NULL;
	}
}

bool CTFGrapplingHook::IsLatchedToTargetPlayer() const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	return pOwner && pOwner->GetGrapplingHookTarget() && pOwner->GetGrapplingHookTarget()->IsPlayer();
}

#endif // GAME_DLL

void CTFGrapplingHook::OnHookReleased( bool bForce )
{
#ifdef GAME_DLL
	RemoveHookProjectile( bForce );
	m_bReleasedAfterLatched = IsLatchedToTargetPlayer();
#endif // GAME_DLL

	if ( GetActivity() != ACT_GRAPPLE_DRAW && GetActivity() != ACT_GRAPPLE_IDLE && GetActivity() != ACT_GRAPPLE_PULL_END )
		SendWeaponAnim( ACT_GRAPPLE_PULL_END );

	if ( bForce )
		m_flNextPrimaryAttack = gpGlobals->curtime + tf_grapplinghook_fire_delay.GetFloat();
}

#ifdef CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::UpdateOnRemove()
{
	StopHookSound();

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	UpdateHookSound();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::StartHookSound()
{
	StopHookSound();

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CLocalPlayerFilter filter;
	m_pHookSound = controller.SoundCreate( filter, entindex(), "WeaponGrapplingHook.ReelStart" );
	controller.Play( m_pHookSound, 1.0, 100 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::StopHookSound()
{
	if ( m_pHookSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pHookSound );
		m_pHookSound = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrapplingHook::UpdateHookSound()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		bool bLatched = pOwner->GetGrapplingHookTarget() != NULL && m_hProjectile != NULL;
		if ( m_bLatched != bLatched )
		{
			if ( !m_bLatched )
			{
				StartHookSound();
			}
			else
			{
				StopHookSound();

				CLocalPlayerFilter filter;
				EmitSound( filter, entindex(), "WeaponGrapplingHook.ReelStop" );
			}

			m_bLatched = bLatched;
		}
	}
}


//-----------------------------------------------------------------------------
// CEquipGrapplingHookNotification
//-----------------------------------------------------------------------------
void CEquipGrapplingHookNotification::Accept()
{
	m_bHasTriggered = true;
		
	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( !pLocalInv )
	{
		MarkForDeletion();
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		MarkForDeletion();
		return;
	}

	// try to equip non-stock-grapplinghook first
	/*static CSchemaItemDefHandle pItemDef_GrapplingHook( "TF_WEAPON_GRAPPLINGHOOK" );

	Assert( pItemDef_GrapplingHook );

	CEconItemView *pGrapplingHook = NULL;

	if ( pItemDef_GrapplingHook )
	{
		for ( int i = 0 ; i < pLocalInv->GetItemCount() ; ++i )
		{
			CEconItemView *pItem = pLocalInv->GetItem( i );
			Assert( pItem );
			if ( pItem->GetItemDefinition() == pItemDef_GrapplingHook )
			{
				pGrapplingHook = pItem;
				break;
			}
		}
	}*/

	// Default item becomes a grappling hook in this mode
	itemid_t iItemId = INVALID_ITEM_ID;
	/*if ( pGrapplingHook )
	{
		iItemId = pGrapplingHook->GetItemID();
	}*/
	
	if ( iItemId == INVALID_ITEM_ID )
	{
		iItemId = 0;

		static CSchemaItemDefHandle pItemDef_Grapple( "TF_WEAPON_GRAPPLINGHOOK" );
		CEconItemView *pDefaultGrapple = TFInventoryManager()->GetBaseItemForClass( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION );
		if ( pDefaultGrapple )
		{
			if ( pDefaultGrapple->GetItemDefinition() == pItemDef_Grapple )
			{
				iItemId = pDefaultGrapple->GetItemID();
			}
		}
	}

	TFInventoryManager()->EquipItemInLoadout( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION, iItemId );
	
	// Tell the GC to tell server that we should respawn if we're in a respawn room

	MarkForDeletion();
}

//===========================================================================================
void CEquipGrapplingHookNotification::UpdateTick()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		CTFGrapplingHook *pGrapplingHook = dynamic_cast<CTFGrapplingHook*>( pLocalPlayer->Weapon_OwnsThisID( TF_WEAPON_GRAPPLINGHOOK ) );
		if ( pGrapplingHook )
		{
			MarkForDeletion();
		}
	}
}

#endif // CLIENT_DLL
