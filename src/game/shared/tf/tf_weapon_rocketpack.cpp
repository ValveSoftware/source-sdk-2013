//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_rocketpack.h"
#include "tf_gamerules.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_rope.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_ammo_pack.h"
#include "ilagcompensationmanager.h"
#include "tf_gamestats.h"
#include "tf_fx.h"
#include "tf_weapon_medigun.h"
#endif

//#define ROCKET_PACK_LAUNCH_EFFECT "rocketpack_exhaust_fire"
#define ROCKET_PACK_LAUNCH_EFFECT "rocketpack_exhaust_launch"
#define ROCKET_PACK_LAUNCH_EFFECT_TRAIL "rocketpack_exhaust"

// TFRocketPack --
IMPLEMENT_NETWORKCLASS_ALIASED( TFRocketPack, DT_TFWeaponRocketPack )

BEGIN_NETWORK_TABLE( CTFRocketPack, DT_TFWeaponRocketPack )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flInitLaunchTime ) ),
	RecvPropFloat( RECVINFO( m_flLaunchTime ) ),
	RecvPropFloat( RECVINFO( m_flToggleEndTime ) ),
	RecvPropBool( RECVINFO( m_bEnabled ) ),	
#else
	SendPropFloat( SENDINFO( m_flInitLaunchTime ) ),
	SendPropFloat( SENDINFO( m_flLaunchTime ) ),
	SendPropFloat( SENDINFO( m_flToggleEndTime ) ),
	SendPropBool( SENDINFO( m_bEnabled ) ),	
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFRocketPack )
	DEFINE_PRED_FIELD( m_flInitLaunchTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flToggleEndTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif // CLIENT_DLL

LINK_ENTITY_TO_CLASS( tf_weapon_rocketpack, CTFRocketPack );
PRECACHE_WEAPON_REGISTER( tf_weapon_rocketpack );

// Todo: Mark as cheat prior to shipping
ConVar tf_rocketpack_cost( "tf_rocketpack_cost", "50", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_launch_delay( "tf_rocketpack_launch_delay", "0.65", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_refire_delay( "tf_rocketpack_refire_delay", "1.2", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_toggle_duration( "tf_rocketpack_toggle_duration", "1.0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_delay_launch( "tf_rocketpack_delay_launch", "0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_launch_absvelocity_preserved( "tf_rocketpack_launch_absvelocity_preserved", "0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_airborne_launch_absvelocity_preserved( "tf_rocketpack_airborne_launch_absvelocity_preserved", "0", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_impact_push_min( "tf_rocketpack_impact_push_min", "100", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_impact_push_max( "tf_rocketpack_impact_push_max", "300", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );
ConVar tf_rocketpack_launch_push( "tf_rocketpack_launch_push", "250", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_HIDDEN );

#ifdef GAME_DLL
#define TF_ROCKETPACK_PASSENGER_DELAY_LAUNCH 0.2f
#endif // GAME_DLL


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFRocketPack::CTFRocketPack()
{
	m_flRefireTime = 0.f;
	m_flInitLaunchTime = 0.f;
	m_flLaunchTime = 0.f;
	m_flToggleEndTime = -1.f;

	m_bEnabled = false;
	m_bLaunchedFromGround = false;

#ifdef GAME_DLL

#else
	ListenForGameEvent( "rocketpack_landed" );

	m_flOldInitLaunchTime = 0.f;
	m_bWasEnabled = false;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::WeaponReset( void )
{
	BaseClass::WeaponReset();

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( m_flInitLaunchTime > 0.f )
	{
		pOwner->StopSound( "Weapon_LooseCannon.Charge" );
	}
	
	m_flRefireTime = 0.f;
	m_flInitLaunchTime = 0.f;
	
	ResetTransition();

#ifdef GAME_DLL
	SetEnabled( false );
#endif // GAME_DLL

#ifdef CLIENT_DLL
	m_flOldInitLaunchTime = 0.f;
	m_bWasEnabled = false;
	CleanupParticles();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_RocketPack.BoostersExtend" );
	PrecacheScriptSound( "Weapon_RocketPack.BoostersReady" );	// ( after initial extend or heat bar returns to yellow )
	PrecacheScriptSound( "Weapon_RocketPack.BoostersCharge" );	// ( mouse click but yet to take off )
	PrecacheScriptSound( "Weapon_RocketPack.BoostersFire" );	// ( take off and fly, has a loop, we'll see how this sounds)
	PrecacheScriptSound( "Weapon_RocketPack.BoostersShutdown" );// ( can play this when you land for now, will cut off the loop )
	PrecacheScriptSound( "Weapon_RocketPack.Land" );			// ( also plays when you land, on a different channel )
	PrecacheScriptSound( "Weapon_RocketPack.BoostersNotReady" );// ( attempt to use but heat bar is in the red )
	PrecacheScriptSound( "Weapon_RocketPack.BoostersRetract" );

	PrecacheParticleSystem( ROCKET_PACK_LAUNCH_EFFECT );
	PrecacheParticleSystem( ROCKET_PACK_LAUNCH_EFFECT_TRAIL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::CanFire( void ) const
{
	if ( !m_bEnabled )
		return false;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;
	
	if ( !pOwner->CanPlayerMove() )
		return false;
	
	if ( !TFGameRules() )
		return false;

	if ( TFGameRules()->State_Get() == GR_STATE_PREROUND )
		return false;

	if ( pOwner->m_Shared.IsLoser() )
		return false;

	/*if ( pOwner->m_Shared.InCond( TF_COND_STUNNED ) )
		return false;*/

	if ( pOwner->IsTaunting() )
		return false;

	if ( m_flRefireTime > gpGlobals->curtime )
		return false;

	if ( !pOwner->m_Shared.IsRocketPackReady() )
		return false;

	// Transitioning
	if ( IsInTransition() && !IsTransitionCompleted() )
		return false;

	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return false;

	if ( pOwner->m_Shared.InCond( TF_COND_ROCKETPACK ) )
	{
		int iAirLaunch = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, iAirLaunch, thermal_thruster_air_launch );
		if ( !iAirLaunch )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::InitiateLaunch( void )
{
	if ( !CanFire() )
		return false;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner->m_Shared.IsRocketPackReady() )
	{
#ifdef GAME_DLL
		CPVSFilter filter( WorldSpaceCenter() );
		pOwner->EmitSound( filter, entindex(), "Weapon_RocketPack.BoostersNotReady" );
#endif // GAME_DLL
		return false;
	}

	if ( m_flInitLaunchTime > 0 )
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
		return false;
	}

#ifdef GAME_DLL
	if ( pOwner->m_Shared.IsLoser() )
	{
		pOwner->EmitSound( "Weapon_RocketPack.BoostersNotReady" );
	}
	else
	{
		pOwner->EmitSound( "Weapon_RocketPack.BoostersCharge" );
	}
#endif // GAME_DLL

	m_flInitLaunchTime = gpGlobals->curtime;

	return true;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bWasEnabled = m_bEnabled;

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_flOldInitLaunchTime != m_flInitLaunchTime )
	{
		CleanupParticles();

		CTFWearable *pWearable = m_hExtraWearable.Get();
		if ( pWearable )
		{
			int iAttachL = pWearable->LookupAttachment( "charge_LA" );
			int iAttachR = pWearable->LookupAttachment( "charge_RA" );
			Assert( iAttachL != INVALID_PARTICLE_ATTACHMENT && iAttachR != INVALID_PARTICLE_ATTACHMENT );
			m_hLeftTrail = pWearable->ParticleProp()->Create( ROCKET_PACK_LAUNCH_EFFECT_TRAIL, PATTACH_POINT_FOLLOW, iAttachL );
			m_hRightTrail = pWearable->ParticleProp()->Create( ROCKET_PACK_LAUNCH_EFFECT_TRAIL, PATTACH_POINT_FOLLOW, iAttachR );
		}
	}

	if ( m_bEnabled != m_bWasEnabled )
	{
		CTFWearable *pWearable = m_hExtraWearable.Get();
		if ( pWearable )
		{
			if (m_bEnabled)
			{
				pWearable->ResetSequence( pWearable->LookupSequence( "deploy" ) );
			}
			else
			{
				pWearable->ResetSequence( pWearable->LookupSequence( "undeploy" ) );
			}
			pWearable->SetCycle( 0.f );
		}
	}


	m_flOldInitLaunchTime = m_flInitLaunchTime;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	CleanupParticles();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::CleanupParticles( void )
{
	CTFWearable *pWearable = m_hExtraWearable.Get();
	if ( pWearable )
	{
		if ( m_hLeftBlast )
		{
			pWearable->ParticleProp()->StopEmission( m_hLeftBlast );
		}
		if ( m_hRightBlast )
		{
			pWearable->ParticleProp()->StopEmission( m_hRightBlast );
		}
		if ( m_hLeftTrail )
		{
			pWearable->ParticleProp()->StopEmission( m_hLeftTrail );
		}
		if ( m_hRightTrail )
		{
			pWearable->ParticleProp()->StopEmission( m_hRightTrail );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRocketPack::FireGameEvent( IGameEvent *event )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( FStrEq( event->GetName(), "rocketpack_landed" ) )
	{
		const int iUserID = event->GetInt( "userid" );
		if ( pOwner->GetUserID() == iUserID )
		{
			CleanupParticles();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRocketPack::ShouldDraw()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && pOwner->IsTaunting() )
		return BaseClass::ShouldDraw();

	return false;
}

#endif // CLIENT_DLL

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFRocketPack::CalcRocketForceFromPlayer( CTFPlayer *pPlayer )
{
	bool bOnGround = ( m_bLaunchedFromGround && abs( pPlayer->EyeAngles().x ) <= 15.f );

	Vector vecDir;
	Vector vecForce;
	float flForce = 450.f;
	const float flPushScale = 1.8f;
	const float flVertPushScale = ( bOnGround ) ? 0.7f : 0.25f;	// Less vertical force while airborne

	Vector vecForward, vecRight;
	QAngle angAim = ( bOnGround ) ? pPlayer->GetAbsAngles() : pPlayer->EyeAngles();
	AngleVectors( angAim, &vecForward, &vecRight, NULL );
	bool bNone = !( pPlayer->m_nButtons & IN_FORWARD ) &&
		!( pPlayer->m_nButtons & IN_BACK ) /* &&
		!( pPlayer->m_nButtons & IN_MOVELEFT ) &&
		!( pPlayer->m_nButtons & IN_MOVERIGHT ) */;

	vecDir = vecForward;

	if ( bNone || ( pPlayer->m_nButtons & IN_FORWARD ) )
	{
		//vecDir = vecForward;
	}
	else if ( pPlayer->m_nButtons & IN_BACK )
	{
		vecForward.Negate();
		vecDir = vecForward;
	}

// 	if ( pPlayer->m_nButtons & IN_MOVELEFT )
// 	{
// 		vecRight.Negate();
// 		vecDir += vecRight;
// 	}
// 	else if ( pPlayer->m_nButtons & IN_MOVERIGHT )
// 	{
// 		vecDir += vecRight;
// 	}

	// Remove down if we're on the ground and aiming down
	if ( bOnGround && vecDir.z < 0.f )
	{
		vecDir.z = 0.f;
	}

	vecForce = vecDir.Normalized() * flForce * flPushScale;
	vecForce.z += 1.f * flForce * flVertPushScale;

	// v1.0
	// 	pPlayer->EyeVectors( &vecDir );
	// 	Vector vecFlightDir = -vecDir;
	// 	VectorNormalize( vecFlightDir );
	//
	// 	// Remove down if we're on the ground
	// 	if ( bOnGround && vecDir.z < 0.f )
	// 	{
	// 		vecDir.z = 0.f;
	// 	}
	//
	// 	vecForce = vecFlightDir * -flForce * flPushScale;
	// 	vecForce.z += 1.f * flForce * flVertPushScale;

	return vecForce;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::RocketLaunchPlayer( CTFPlayer *pPlayer, const Vector& vecForce, bool bIsPassenger )
{
	if ( !pPlayer->m_Shared.InCond( TF_COND_ROCKETPACK ) )
	{
		pPlayer->m_Shared.AddCond( TF_COND_ROCKETPACK );
		if ( bIsPassenger )
		{
		}
		pPlayer->m_Shared.StunPlayer( 0.5f, 1.0f, TF_STUN_MOVEMENT );
		UTIL_ScreenShake( pPlayer->GetAbsOrigin(), 100.f, 150, 0.5f, 128.f, SHAKE_START, true );
	}

	if ( pPlayer->GetFlags() & FL_ONGROUND )
	{
		if ( !tf_rocketpack_launch_absvelocity_preserved.GetBool() )
		{
			pPlayer->SetAbsVelocity( vec3_origin );
		}
	}
	else if ( !tf_rocketpack_airborne_launch_absvelocity_preserved.GetBool() )
	{
		pPlayer->SetAbsVelocity( vec3_origin );
	}

	pPlayer->ApplyAbsVelocityImpulse( vecForce );
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::PreLaunch( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	pOwner->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, ACT_MP_ATTACK_STAND_PRIMARYFIRE );
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

#ifdef GAME_DLL
	// Negate any fall
	Vector vecVel = pOwner->GetAbsVelocity();
	if ( vecVel.z < 0.f )
	{
		vecVel.z = 0.f;
		pOwner->SetAbsVelocity( vecVel );
	}

	// Pop them off the ground
	Vector vForward( 0.f, 0.f, 350.f );
	pOwner->ApplyAbsVelocityImpulse( vForward );
	pOwner->m_Shared.AddCond( TF_COND_PARACHUTE_ACTIVE );

	const Vector &vecOrigin = pOwner->GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	TE_TFParticleEffect( filter, 0.f, "heavy_ring_of_fire", vecOrigin, vec3_angle );
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pOwner, "foot_L" );
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, pOwner, "foot_R" );

	m_flLaunchTime = gpGlobals->curtime + tf_rocketpack_launch_delay.GetFloat();
#endif // GAME_DLL

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::Launch( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	pOwner->StopSound( "Weapon_LooseCannon.Charge" );

#ifdef GAME_DLL
	m_flLaunchTime = 0.f;
	pOwner->m_Shared.RemoveCond( TF_COND_PARACHUTE_ACTIVE );

	// Launch
	m_vecLaunchDir = CalcRocketForceFromPlayer( pOwner );
	RocketLaunchPlayer( pOwner, m_vecLaunchDir, false );

	
	SetContextThink( &CTFRocketPack::PassengerDelayLaunchThink, gpGlobals->curtime + TF_ROCKETPACK_PASSENGER_DELAY_LAUNCH, "PassengerDelayLaunchThink" );
	
	m_flRefireTime = gpGlobals->curtime + 0.5f;

	{
		pOwner->m_Shared.SetRocketPackCharge( pOwner->m_Shared.GetRocketPackCharge() - tf_rocketpack_cost.GetFloat() );
	}

	// Knock-back nearby enemies
	float flRadius = 150.f;
	CUtlVector< CTFPlayer* > vecPushedPlayers;
	TFGameRules()->PushAllPlayersAway( GetAbsOrigin(), flRadius, tf_rocketpack_launch_push.GetFloat(), GetEnemyTeam( GetTeamNumber() ), &vecPushedPlayers );
	FOR_EACH_VEC( vecPushedPlayers, i )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_rocketpack_pushed" );
		if ( event )
		{
			event->SetInt( "pusher", pOwner->GetUserID() );
			event->SetInt( "pushed", vecPushedPlayers[ i ]->GetUserID() );

			gameeventmanager->FireEvent( event, true );
		}
	}

	// Extinguish teammates
	CUtlVector< CTFPlayer * > vecPlayers;
	CollectPlayers( &vecPlayers, pOwner->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );
	FOR_EACH_VEC( vecPlayers, i )
	{
		CTFPlayer *pPlayer = vecPlayers[i];
		if ( !pPlayer )
			continue;

		if ( !pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
			continue;

		if ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() > ( flRadius * flRadius ) )
			continue;

		if ( !pOwner->FVisible( pPlayer, MASK_OPAQUE ) )
			continue;

		pPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
		pPlayer->EmitSound( "TFPlayer.FlameOut" );
		CTF_GameStats.Event_PlayerAwardBonusPoints( pOwner, pPlayer, 10 );

		// Event of this happening
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_extinguished" );
		if ( pEvent )
		{
			pEvent->SetInt( "victim", pPlayer->entindex() );
			pEvent->SetInt( "healer", pOwner->entindex() );

			item_definition_index_t nDefIndex = INVALID_ITEM_DEF_INDEX;
			if ( GetAttributeContainer() && GetAttributeContainer()->GetItem() )
			{
				nDefIndex = GetAttributeContainer()->GetItem()->GetItemDefinition()->GetDefinitionIndex();
			}

			pEvent->SetInt( "itemdefindex", nDefIndex );

			gameeventmanager->FireEvent( pEvent, true );
		}
	}

	CPASAttenuationFilter filter( pOwner );
	pOwner->EmitSound( filter, pOwner->entindex(), "Weapon_RocketPack.BoostersFire" );

	IGameEvent *pEvent = gameeventmanager->CreateEvent( "rocketpack_launch" );
	if ( pEvent )
	{
		pEvent->SetInt( "userid", pOwner->GetUserID() );
		pEvent->SetBool( "playsound", true );
		gameeventmanager->FireEvent( pEvent );
	}

	CTFWearable *pWearable = m_hExtraWearable.Get();
	if ( pWearable )
	{
		DispatchParticleEffect( ROCKET_PACK_LAUNCH_EFFECT, PATTACH_POINT_FOLLOW, pWearable, "charge_LA" );
		DispatchParticleEffect( ROCKET_PACK_LAUNCH_EFFECT, PATTACH_POINT_FOLLOW, pWearable, "charge_RA" );
	}
#endif // GAME_DLL

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::ResetTransition( void )
{
	m_flToggleEndTime = -1.f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::StartTransition( void )
{
	m_flToggleEndTime = gpGlobals->curtime + tf_rocketpack_toggle_duration.GetFloat();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::IsInTransition( void ) const
{
	return m_flToggleEndTime >= 0.f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRocketPack::IsTransitionCompleted( void ) const
{
	return gpGlobals->curtime >= m_flToggleEndTime;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::WaitToLaunch( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

#ifdef GAME_DLL

	float flStunDutation = 0.1f;

	// prevent the owner and passenger from moving for a bit while waiting to launch
	pOwner->m_Shared.StunPlayer( flStunDutation, 1.f, TF_STUN_MOVEMENT, pOwner );
#endif // GAME_DLL
}


#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::SetEnabled( bool bEnabled )
{
	if ( m_bEnabled == bEnabled )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	StartTransition();

	m_bEnabled = bEnabled;

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::PassengerDelayLaunchThink()
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	// Are we being healed by any QuickFix medics?
	for ( int i = 0; i < pOwner->m_Shared.GetNumHealers(); i++ )
	{
		CTFPlayer *pMedic = ToTFPlayer( pOwner->m_Shared.GetHealerByIndex( i ) );
		if ( !pMedic )
			continue;

		CTFWeaponBase *pWeapon = pMedic->GetActiveTFWeapon();
		if ( !pWeapon )
			continue;

		if ( pWeapon->GetWeaponID() != TF_WEAPON_MEDIGUN )
			continue;

		// Share blast jump with them
		CWeaponMedigun *pMedigun = assert_cast< CWeaponMedigun* >( pWeapon );
		if ( pMedigun && pMedigun->GetMedigunType() == MEDIGUN_QUICKFIX )
		{
			RocketLaunchPlayer( pMedic, m_vecLaunchDir, true );
		}
	}

}

#endif // GAME_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRocketPack::ItemPostFrame( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	// Transitioning
	if ( IsInTransition() )
	{
		m_flLaunchTime = 0.f;

		// Completed
		if ( IsTransitionCompleted() )
		{
			ResetTransition();
#ifdef CLIENT_DLL
			if ( pOwner == C_TFPlayer::GetLocalTFPlayer() )
			{
				pOwner->EmitSound( "Weapon_RocketPack.BoostersReady" );
			}
#endif // CLIENT_DLL
		}
		else if ( pOwner->m_afButtonPressed & IN_ATTACK2 )
		{
#ifdef CLIENT_DLL
			if ( pOwner == C_TFPlayer::GetLocalTFPlayer() )
			{
				pOwner->EmitSound( "Player.DenyWeaponSelection" );
			}
#endif // CLIENT_DLL
		}

		// disabled secondary attack while transitioning
		pOwner->m_nButtons &= ~IN_ATTACK2;
	}

	// Toggle
// #ifdef GAME_DLL
// 	if ( ( pOwner->m_nButtons & IN_RELOAD ) && !IsInTransition() )
// 	{
// 		if ( m_bEnabled )
// 		{
// 			EmitSound( "Weapon_RocketPack.BoostersRetract" );
// 		}
// 		else
// 		{
// 			EmitSound( "Weapon_RocketPack.BoostersExtend" );
// 		}
// 
// 		SetEnabled( !m_bEnabled );
// 	}
// #endif // GAME_DLL

	if ( m_flInitLaunchTime > 0.f )
	{
		if ( !m_bEnabled || pOwner->m_afButtonPressed & IN_JUMP )
		{
			// rocketpack was disabled while waiting for player, just turn it off
			m_flInitLaunchTime = 0.f;
			m_flNextSecondaryAttack = gpGlobals->curtime + tf_rocketpack_launch_delay.GetFloat();
			pOwner->m_Shared.RemoveCond( TF_COND_PARACHUTE_ACTIVE );
		}
		else
		{
			m_bLaunchedFromGround = ( pOwner->GetFlags() & FL_ONGROUND ) > 0;
			PreLaunch();
			m_flInitLaunchTime = 0.f;
			m_flNextSecondaryAttack = gpGlobals->curtime + tf_rocketpack_refire_delay.GetFloat();
		}
	}
	else if ( m_flLaunchTime > 0.f && m_flLaunchTime <= gpGlobals->curtime )
	{
		Launch();
	}
	else if ( ( pOwner->m_afButtonPressed & IN_ATTACK ) || ( pOwner->m_afButtonPressed & IN_ATTACK2 ) )
	{
		InitiateLaunch();
	}
	else
	{
		WeaponIdle();
	}

	// while enabled, disabled secondary attack so active weapon cannot use this
	if ( m_bEnabled )
	{
		pOwner->m_nButtons &= ~IN_ATTACK2;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const CEconItemView *CTFRocketPack::GetTauntItem() const
{
	if ( m_bEnabled )
	{
		return BaseClass::GetTauntItem();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFRocketPack::Deploy( void )
{
#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		EmitSound( "Weapon_RocketPack.BoostersExtend" );
		SetEnabled( true );
	}
#endif // GAME_DLL

	return BaseClass::Deploy();
}


// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFRocketPack::StartHolsterAnim( void )
{
	BaseClass::StartHolsterAnim();

#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner )
	{
		EmitSound( "Weapon_RocketPack.BoostersRetract" );
		SetEnabled( false );
	}
#endif // GAME_DLL
}


// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFRocketPack::CanHolster( void ) const
{
	if ( m_flLaunchTime > 0.f )
		return false;

	return BaseClass::CanHolster();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFRocketPack::OnResourceMeterFilled()
{
#ifdef CLIENT_DLL
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer->IsLocalPlayer() )
	{
		pPlayer->EmitSound( "TFPlayer.ReCharged" );
	}
#endif // CLIENT_DLL
}
