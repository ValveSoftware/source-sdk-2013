//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_wrench.h"
#include "decals.h"
#include "baseobject_shared.h"
#include "tf_viewmodel.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "in_buttons.h"
	#include "tf_hud_menu_eureka_teleport.h"
	// NVNT haptics system interface
	#include "haptics/ihaptics.h"
// Server specific.
#else
	#include "tf_player.h"
	#include "variant_t.h"
	#include "tf_gamerules.h"
	#include "particle_parse.h"
	#include "tf_fx.h"
	#include "tf_obj_sentrygun.h"
	#include "ilagcompensationmanager.h"
#endif

// Maximum time between robo arm hits to maintain the three-hit-combo
#define ROBOARM_COMBO_TIMEOUT 1.0f

//=============================================================================
//
// Weapon Wrench tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWrench, DT_TFWeaponWrench )

BEGIN_NETWORK_TABLE( CTFWrench, DT_TFWeaponWrench )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWrench )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_wrench, CTFWrench );
PRECACHE_WEAPON_REGISTER( tf_weapon_wrench );

//=============================================================================
//
// Robot Arm tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRobotArm, DT_TFWeaponRobotArm )

BEGIN_NETWORK_TABLE( CTFRobotArm, DT_TFWeaponRobotArm )
#ifdef GAME_DLL
SendPropEHandle(SENDINFO(m_hRobotArm)),
#else
RecvPropEHandle(RECVINFO(m_hRobotArm)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFRobotArm )
// DEFINE_PRED_FIELD( name, fieldtype, flags )
DEFINE_PRED_FIELD( m_iComboCount, FIELD_INTEGER, 0 ),
DEFINE_PRED_FIELD( m_flLastComboHit, FIELD_FLOAT, 0 ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_robot_arm, CTFRobotArm );
PRECACHE_WEAPON_REGISTER( tf_weapon_robot_arm );

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableRobotArm, DT_TFWearableRobotArm )

BEGIN_NETWORK_TABLE( CTFWearableRobotArm, DT_TFWearableRobotArm )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable_robot_arm, CTFWearableRobotArm );

//=============================================================================
//
// Weapon Wrench functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWrench::CTFWrench()
	: m_bReloadDown( false )
{}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWrench::Spawn()
{
	BaseClass::Spawn();
}


#ifdef GAME_DLL
void CTFWrench::OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector hitLoc )
{
	bool bHelpTeammateBuildStructure = pObject->IsBuilding() && pObject->GetOwner() != GetOwner();

	// Did this object hit do any work? repair or upgrade?
	bool bUsefulHit = pObject->InputWrenchHit( pPlayer, this, hitLoc );

	// award achievement if we helped a teammate build a structure
	if ( bUsefulHit && bHelpTeammateBuildStructure )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if ( pOwner && pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			pOwner->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_HELP_BUILD_STRUCTURE );
		}
	}

	CDisablePredictionFiltering disabler;

	if ( pObject->IsDisposableBuilding() )
	{
		CSingleUserRecipientFilter singleFilter( pPlayer );
		EmitSound( singleFilter, pObject->entindex(), "Player.UseDeny" );
	}
	else
	{
		if ( bUsefulHit )
		{
			// play success sound
			WeaponSound( SPECIAL1 );
		}
		else
		{
			// play failure sound
			WeaponSound( SPECIAL2 );
		}
	}
}
#endif

void CTFWrench::Smack( void )
{
	// see if we can hit an object with a higher range

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;

	// only trace against objects

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt &&
		 trace.m_pEnt->IsBaseObject() &&
		 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
#ifdef GAME_DLL
		OnFriendlyBuildingHit( dynamic_cast< CBaseObject * >( trace.m_pEnt ), pPlayer, trace.endpos );
#else
		// NVNT if the local player is the owner of this wrench 
		//   Notify the haptics system we just repaired something.
		if(pPlayer==C_TFPlayer::GetLocalTFPlayer() && haptics)
			haptics->ProcessHapticEvent(2,"Weapons","tf_weapon_wrench_fix");
#endif
	}
	else
	{
		// if we cannot, Smack as usual for player hits
		BaseClass::Smack();
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWrench::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if ( !CanAttack() )
	{
		return;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
	{
		return;
	}

	// Just pressed reload?
	if ( pOwner->m_nButtons & IN_RELOAD && !m_bReloadDown )
	{
		m_bReloadDown = true;
		int iAltFireTeleportToSpawn = 0;
		CALL_ATTRIB_HOOK_INT( iAltFireTeleportToSpawn, alt_fire_teleport_to_spawn );
		if ( iAltFireTeleportToSpawn )
		{
			// Tell the teleport menu to show
			CHudEurekaEffectTeleportMenu *pTeleportMenu = ( CHudEurekaEffectTeleportMenu * )GET_HUDELEMENT( CHudEurekaEffectTeleportMenu );
			if ( pTeleportMenu )
			{
				pTeleportMenu->WantsToTeleport();
			}
		}
	}
	else if ( !(pOwner->m_nButtons & IN_RELOAD) && m_bReloadDown )
	{
		m_bReloadDown = false;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Kill all buildings when wrench is changed.
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
void CTFWrench::Equip( CBaseCombatCharacter *pOwner )
{
	// STAGING_ENGY
	CTFPlayer *pPlayer = ToTFPlayer( pOwner );
	if ( pPlayer )
	{
		// if switching too gunslinger, blow up other sentry
		int iMiniSentry = 0;
		CALL_ATTRIB_HOOK_INT( iMiniSentry, wrench_builds_minisentry );
		if ( iMiniSentry )
		{
			// Just detonate Sentries
			CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pPlayer->GetObjectOfType( OBJ_SENTRYGUN ) );
			if ( pSentry )
			{
				pSentry->DetonateObject();
			}
		}
	}

	BaseClass::Equip( pOwner );
}
//-----------------------------------------------------------------------------
// Purpose: Kill all buildings when wrench is changed.
//-----------------------------------------------------------------------------
void CTFWrench::Detach( void )
{
	// STAGING_ENGY
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		bool bDetonateObjects = true;

		// In MvM mode, leave engineer's buildings after he dies
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( pPlayer->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS )
			{
				bDetonateObjects = false;
			}
		}

		// Only detonate if we are unequipping gunslinger
		if ( bDetonateObjects )
		{
			// if switching off of gunslinger detonate
			int iMiniSentry = 0;
			CALL_ATTRIB_HOOK_INT( iMiniSentry, wrench_builds_minisentry );
			if ( iMiniSentry )
			{
				// Just detonate Sentries
				CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( pPlayer->GetObjectOfType( OBJ_SENTRYGUN ) );
				if ( pSentry )
				{
					pSentry->DetonateObject();
				}
			}
		}
	}

	BaseClass::Detach();
}


//-----------------------------------------------------------------------------
// Purpose: Apply health upgrade to our existing buildings
//-----------------------------------------------------------------------------
void CTFWrench::ApplyBuildingHealthUpgrade( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	for ( int i = pPlayer->GetObjectCount()-1; i >= 0; i-- )
	{
		CBaseObject *pObj = pPlayer->GetObject(i);
		if ( pObj )
		{
			pObj->ApplyHealthUpgrade();
		}		
	}
}

#endif

// STAGING_ENGY
ConVar tf_construction_build_rate_multiplier( "tf_construction_build_rate_multiplier", "1.5f", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
float CTFWrench::GetConstructionValue( void )
{
	float flValue = tf_construction_build_rate_multiplier.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT( flValue, mult_construction_value );
	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFWrench::GetRepairAmount( void )
{
	float flRepairAmount = 100.f;

	float flMod = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flMod, mult_repair_value );

#ifdef GAME_DLL
	if ( GetOwner() )
	{
		CBaseCombatWeapon* pWpn = GetOwner()->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
		if ( pWpn )
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flMod, mult_repair_value );
		}
	}
#endif

	return flRepairAmount * flMod;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWrench::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFRobotArm::CTFRobotArm()
{
	m_iComboCount = 0;
	m_flLastComboHit = 0.f;
	m_bBigIdle = false;
	m_bBigHit = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRobotArm::Precache()
{
	BaseClass::Precache();

	extern const char *g_HACK_GunslingerEngineerArmsOverride;
	PrecacheModel( g_HACK_GunslingerEngineerArmsOverride );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
void CTFRobotArm::Equip( CBaseCombatCharacter* pOwner )
{
	BaseClass::Equip( pOwner );

	if ( !IsPDQ() )
		return;

	CTFWearable* pArmItem = dynamic_cast<CTFWearable*>( CreateEntityByName( "tf_wearable_robot_arm" ) );
	if ( pArmItem )
	{
		pArmItem->AddSpawnFlags( SF_NORESPAWN );
		pArmItem->SetAlwaysAllow( true );
		DispatchSpawn( pArmItem );
		pArmItem->GiveTo( pOwner );
		pArmItem->AddHiddenBodyGroup( "rightarm" );
		pArmItem->SetOwnerEntity( pOwner );
		m_hRobotArm.Set( pArmItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRobotArm::Drop( const Vector &vecVelocity )
{
	RemoveRobotArm();

	BaseClass::Drop( vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::UpdateOnRemove( void )
{
	RemoveRobotArm();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::RemoveRobotArm( void )
{
	if ( m_hRobotArm )
	{
		m_hRobotArm->RemoveFrom( GetOwnerEntity() );
		m_hRobotArm = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::OnActiveStateChanged( int iOldState )
{
	if ( m_iState == WEAPON_NOT_CARRIED )
	{
		RemoveRobotArm();
	}
}

#endif

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFRobotArm::PrimaryAttack()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( gpGlobals->curtime - m_flLastComboHit > ROBOARM_COMBO_TIMEOUT )
	{
		m_iComboCount = 0;
	}

	if ( m_iComboCount == 2 && CanAttack() )
	{
		pPlayer->m_Shared.SetNextMeleeCrit( MELEE_CRIT );
	}

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::Smack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

#if !defined (CLIENT_DLL)
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	trace_t trace;
	bool btrace = DoSwingTrace( trace );
	if ( btrace && trace.DidHitNonWorldEntity() && trace.m_pEnt && trace.m_pEnt->IsPlayer() &&
		 trace.m_pEnt->GetTeamNumber() != pPlayer->GetTeamNumber() )
	{
		m_iComboCount++;
		m_flLastComboHit = gpGlobals->curtime;

		if ( m_iComboCount == 3 )
		{
			m_iComboCount = 0;
			m_bBigIdle = true;
			m_bBigHit = true;
		}
	}
	else
	{
		m_iComboCount = 0;
	}

#if !defined (CLIENT_DLL)	
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	BaseClass::Smack();

	m_bBigHit = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::DoViewModelAnimation( void )
{
	if ( m_iComboCount == 2 )
	{
		SendWeaponAnim( ACT_ITEM2_VM_SWINGHARD );
	}
	else
	{
		SendWeaponAnim( ACT_ITEM2_VM_HITCENTER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
int CTFRobotArm::GetDamageCustom()
{
	if ( m_bBigHit )
	{
		return TF_DMG_CUSTOM_COMBO_PUNCH;
	}
	else
	{
		return BaseClass::GetDamageCustom();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRobotArm::GetForceScale( void )
{
	if ( m_bBigHit )
	{
		return 500.f;
	}
	else
	{
		return BaseClass::GetForceScale();
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotArm::WeaponIdle( void )
{
#ifdef GAME_DLL
	if ( m_bBigIdle )
	{
		m_bBigIdle = false;
		SendWeaponAnim( ACT_ITEM2_VM_IDLE_2 );
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
		return;
	}
#endif

	BaseClass::WeaponIdle();
}
