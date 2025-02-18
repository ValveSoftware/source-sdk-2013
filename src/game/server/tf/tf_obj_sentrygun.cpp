//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Sentrygun OMG
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_sentrygun.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "world.h"
#include "tf_projectile_rocket.h"
#include "te_effect_dispatch.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "tf_weapon_wrench.h"
#include "tf_weapon_laser_pointer.h"
#include "tf_weapon_shotgun.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"
#include "bot/tf_bot.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "nav_pathfind.h"
#include "tf_weapon_knife.h"
#include "tf_logic_robot_destruction.h"
#include "tf_target_dummy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern bool IsInCommentaryMode();

extern ConVar tf_nav_in_combat_range;

// Ground placed version
#define SENTRY_MODEL_PLACEMENT			"models/buildables/sentry1_blueprint.mdl"
#define SENTRY_MODEL_LEVEL_1			"models/buildables/sentry1.mdl"
#define SENTRY_MODEL_LEVEL_1_UPGRADE	"models/buildables/sentry1_heavy.mdl"
#define SENTRY_MODEL_LEVEL_2			"models/buildables/sentry2.mdl"
#define SENTRY_MODEL_LEVEL_2_UPGRADE	"models/buildables/sentry2_heavy.mdl"
#define SENTRY_MODEL_LEVEL_3			"models/buildables/sentry3.mdl"
#define SENTRY_MODEL_LEVEL_3_UPGRADE	"models/buildables/sentry3_heavy.mdl"

#define SENTRY_ROCKET_MODEL "models/buildables/sentry3_rockets.mdl"

#define SENTRYGUN_MINS			Vector(-20, -20, 0)
#define SENTRYGUN_MAXS			Vector( 20,  20, 66)

#define SENTRYGUN_ADD_SHELLS	40
#define SENTRYGUN_ADD_ROCKETS	8

#define SENTRY_THINK_DELAY	0.05

#define	SENTRYGUN_CONTEXT	"SentrygunContext"

#define SENTRYGUN_RECENTLY_ATTACKED_TIME 2.0

#define SENTRYGUN_MINIGUN_RESIST_LVL_1		0.0
#define SENTRYGUN_MINIGUN_RESIST_LVL_2		0.15
#define SENTRYGUN_MINIGUN_RESIST_LVL_3		0.20

#define SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER	0.66f

#define SENTRYGUN_MAX_LEVEL_MINI			1
#define MINI_SENTRY_SCALE			0.75f
#define DISPOSABLE_SCALE			0.65f
#define SMALL_SENTRY_SCALE			0.80f

#define WRANGLER_DISABLE_TIME		3.0f

enum
{	
	SENTRYGUN_ATTACHMENT_MUZZLE = 0,
	SENTRYGUN_ATTACHMENT_MUZZLE_ALT,
	SENTRYGUN_ATTACHMENT_ROCKET,
};

enum target_ranges
{
	RANGE_MELEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR,
};

#define VECTOR_CONE_TF_SENTRY		Vector( 0.1, 0.1, 0 )

//-----------------------------------------------------------------------------
// Purpose: Only send the LocalWeaponData to the player carrying the weapon
//-----------------------------------------------------------------------------
void* SendProxy_SendLocalObjectDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseObject *pObject = (CBaseObject*)pVarData;
	if ( pObject )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pObject->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendLocalObjectDataTable );

BEGIN_NETWORK_TABLE_NOBASE( CObjectSentrygun, DT_SentrygunLocalData )
	SendPropInt( SENDINFO(m_iKills), -1, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO(m_iAssists), -1, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST( CObjectSentrygun, DT_ObjectSentrygun )
	SendPropInt( SENDINFO(m_iAmmoShells), -1, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO(m_iAmmoRockets), -1, SPROP_VARINT | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO(m_iState), Q_log2( SENTRY_NUM_STATES ) + 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bPlayerControlled ) ),
	SendPropInt( SENDINFO( m_nShieldLevel ), 4, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hEnemy ) ),
	SendPropEHandle( SENDINFO( m_hAutoAimTarget ) ),
	SendPropDataTable( "SentrygunLocalData", 0, &REFERENCE_SEND_TABLE( DT_SentrygunLocalData ), SendProxy_SendLocalObjectDataTable ),
END_SEND_TABLE()

BEGIN_DATADESC( CObjectSentrygun )
END_DATADESC()

LINK_ENTITY_TO_CLASS(obj_sentrygun, CObjectSentrygun);
PRECACHE_REGISTER(obj_sentrygun);

ConVar tf_sentrygun_damage( "tf_sentrygun_damage", "16", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_sentrygun_mini_damage( "tf_sentrygun_mini_damage", "8", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_sentrygun_ammocheat( "tf_sentrygun_ammocheat", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
extern ConVar tf_obj_upgrade_per_hit;
ConVar tf_sentrygun_newtarget_dist( "tf_sentrygun_newtarget_dist", "200", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_sentrygun_metal_per_shell( "tf_sentrygun_metal_per_shell", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_sentrygun_metal_per_rocket( "tf_sentrygun_metal_per_rocket", "2", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_sentrygun_notarget( "tf_sentrygun_notarget", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_sentrygun_max_absorbed_damage_while_controlled_for_achievement( "tf_sentrygun_max_absorbed_damage_while_controlled_for_achievement", "500", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_sentrygun_kill_after_redeploy_time_achievement( "tf_sentrygun_kill_after_redeploy_time_achievement", "10", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
extern ConVar tf_cheapobjects;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectSentrygun::CObjectSentrygun()
{
	// Don't bother with health modifying attributes here, because we don't have an owner yet, and it'll be stomped in FirstSpawn()
	int iHealth = GetMaxHealthForCurrentLevel();
	SetMaxHealth( iHealth );
	SetHealth( iHealth );
	SetType( OBJ_SENTRYGUN );

	m_bFireNextFrame = false;
	m_bFireRocketNextFrame = false;
	m_flAutoAimStartTime = 0.f;
	m_bPlayerControlled = false;
	m_iLifetimeShieldedDamage = 0;
	m_flFireRate = 1.f;
	m_flSentryRange = SENTRY_MAX_RANGE;
	m_nShieldLevel.Set( SHIELD_NONE );

	m_lastTeammateWrenchHit = NULL;
	m_lastTeammateWrenchHitTimer.Invalidate();

	m_flScaledSentry = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::Spawn()
{
	m_iPitchPoseParameter = -1;
	m_iYawPoseParameter = -1;

	SetModel( SENTRY_MODEL_PLACEMENT );

	// Rotate Details
	m_iRightBound = 45;
	m_iLeftBound = 315;
	m_iBaseTurnRate = 6;
	m_flFieldOfView = VIEW_FIELD_NARROW;

	// Give the Gun some ammo
	m_iAmmoShells = 0;
	m_iAmmoRockets = 0;

	float flMaxAmmoMult = 1.f;
	if ( GetOwner() )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwner(), flMaxAmmoMult, mvm_sentry_ammo );
	}

	m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1 * flMaxAmmoMult;
	m_iMaxAmmoRockets = SENTRYGUN_MAX_ROCKETS * flMaxAmmoMult;

	m_iAmmoType = GetAmmoDef()->Index( "TF_AMMO_PRIMARY" );

	// Start searching for enemies
	m_hEnemy = NULL;

	m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_1;

	m_lastTeammateWrenchHit = NULL;
	m_lastTeammateWrenchHitTimer.Invalidate();

	BaseClass::Spawn();

	SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_1 );

	SetBuildingSize();

	m_iState.Set( SENTRY_STATE_INACTIVE );

	SetContextThink( &CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::FirstSpawn()
{
	m_flLastAttackedTime = 0;

	int iHealth = GetMaxHealthForCurrentLevel();

	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	BaseClass::FirstSpawn();
}

Vector CObjectSentrygun::GetEnemyAimPosition( CBaseEntity* pEnemy ) const
{
	// Default to pointing to the origin
	Vector vecPos = pEnemy->WorldSpaceCenter();

	CTFPlayer* pTFEnemy = ToTFPlayer( pEnemy );

	// This is expensive, so only do it if our target is in a state that requires it
	if ( pTFEnemy )
	{
		bool bShouldUseAccurateMethod = false; 

		int playerFlags = pTFEnemy->GetFlags();
		// Crouch jumping makes your box weird
		bShouldUseAccurateMethod |= !( playerFlags & FL_ONGROUND ) && ( playerFlags & FL_DUCKING );
		// Taunting can make your box weird
		bShouldUseAccurateMethod |= pTFEnemy->m_Shared.InCond( TF_COND_TAUNTING );

		if ( bShouldUseAccurateMethod )
		{
			// Use this bone as the the aim target
			int iSpineBone = pTFEnemy->LookupBone( "bip_spine_2" );
			if ( iSpineBone != -1 )
			{
				QAngle angles;
				pTFEnemy->GetBonePosition( iSpineBone, vecPos, angles );
			}
		}
	}

	return vecPos;
}

void CObjectSentrygun::SentryThink( void )
{
	m_flSentryRange = SENTRY_MAX_RANGE;
	if ( !IsDisposableBuilding() )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwner(), m_flSentryRange, mult_sentry_range );
	}

	switch( m_iState )
	{
	case SENTRY_STATE_INACTIVE:
	case SENTRY_STATE_UPGRADING:		// Base class handles this
		break;

	case SENTRY_STATE_SEARCHING:
		SentryRotate();
		break;

	case SENTRY_STATE_ATTACKING:
		Attack();
		break;

	default:
		Assert( 0 );
		break;
	}

	SetContextThink( &CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT );

	if ( m_nShieldLevel > 0 && (gpGlobals->curtime > m_flShieldFadeTime) )
	{
		m_nShieldLevel.Set( SHIELD_NONE );
		m_vecGoalAngles.x = 0;
	}

	// infinite ammo for enemy team in MvM mode
	if ( TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		m_iAmmoRockets = SENTRYGUN_MAX_ROCKETS;
		m_iMaxAmmoRockets = SENTRYGUN_MAX_ROCKETS;
		m_iAmmoShells = SENTRYGUN_MAX_SHELLS_3;
		m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_3;
	}
}

void CObjectSentrygun::StartPlacement( CTFPlayer *pPlayer )
{
	BaseClass::StartPlacement( pPlayer );

	// Set my build size
	m_vecBuildMins = SENTRYGUN_MINS;
	m_vecBuildMaxs = SENTRYGUN_MAXS;
	m_vecBuildMins -= Vector( 4,4,0 );
	m_vecBuildMaxs += Vector( 4,4,0 );

	MakeMiniBuilding( pPlayer );
	MakeDisposableBuilding( pPlayer );
	MakeScaledBuilding( GetBuilder() );
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectSentrygun::StartBuilding( CBaseEntity *pBuilder )
{
	SetStartBuildingModel();

	// Have to re-call this in case the player changed their weapon
	// between StartPlacement and StartBuilding.
	MakeMiniBuilding( GetBuilder() );
	MakeDisposableBuilding( GetBuilder() );
	MakeScaledBuilding( GetBuilder() );

	if ( IsMiniBuilding() )
	{
		SetBodygroup( FindBodygroupByName( "mini_sentry_light" ), 1 );
	}

	CreateBuildPoints();

	SetPoseParameter( m_iPitchPoseParameter, 0.0 );
	SetPoseParameter( m_iYawPoseParameter, 0.0 );

	SetObjectMode( IsDisposableBuilding() ? MODE_SENTRYGUN_DISPOSABLE : MODE_SENTRYGUN_NORMAL );

	return BaseClass::StartBuilding( pBuilder );
}

void CObjectSentrygun::SetStartBuildingModel( void )
{
	SetModel( SENTRY_MODEL_LEVEL_1_UPGRADE );
	m_iState.Set( SENTRY_STATE_INACTIVE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::MakeMiniBuilding( CTFPlayer* pPlayer )
{
	if ( !ShouldBeMiniBuilding( pPlayer ) || IsMiniBuilding() )
		return;

	BaseClass::MakeMiniBuilding( pPlayer );
	SetModelScale( MINI_SENTRY_SCALE );

	int iHealth = GetMaxHealthForCurrentLevel();

	SetMaxHealth( iHealth );
	SetHealth( iHealth / 2.0f );
	SetBuildingSize();
}

//-----------------------------------------------------------------------------
int CObjectSentrygun::GetMaxUpgradeLevel( )
{ 
	if ( IsDisposableBuilding() || IsMiniBuilding() )
		return SENTRYGUN_MAX_LEVEL_MINI;

	return BaseClass::GetMaxUpgradeLevel(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnGoActive( void )
{
	SetModel( SENTRY_MODEL_LEVEL_1 );

	if ( IsMiniBuilding() )
	{
		SetBodygroup( FindBodygroupByName( "mini_sentry_light" ), 1 );
	}

	m_iState.Set( SENTRY_STATE_SEARCHING );

	// Orient it
	QAngle angles = GetAbsAngles();

	m_vecCurAngles.y = UTIL_AngleMod( angles.y );
	m_iRightBound = UTIL_AngleMod( (int)angles.y - 50 );
	m_iLeftBound = UTIL_AngleMod( (int)angles.y + 50 );
	if ( m_iRightBound > m_iLeftBound )
	{
		m_iRightBound = m_iLeftBound;
		m_iLeftBound = UTIL_AngleMod( (int)angles.y - 50);
	}

	// Start it rotating
	m_vecGoalAngles.y = m_iRightBound;
	m_vecGoalAngles.x = m_vecCurAngles.x = 0;
	m_bTurningRight = true;

	EmitSound( "Building_Sentrygun.Built" );

	// if our eye pos is underwater, we're waterlevel 3, else 0
	bool bUnderwater = ( UTIL_PointContents( EyePosition() ) & MASK_WATER ) ? true : false;
	SetWaterLevel( ( bUnderwater ) ? 3 : 0 );	

	if ( m_bCarryDeploy )
	{
		m_iAmmoShells = m_iOldAmmoShells;
		m_iAmmoRockets = m_iOldAmmoRockets;
	}
	else
	{
		m_iAmmoShells = m_iMaxAmmoShells;
		m_iAmmoRockets = m_iMaxAmmoRockets;
	}

	// Init attachments for level 1 sentry gun
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = LookupAttachment( "muzzle" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = 0;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET] = 0;

	BaseClass::OnGoActive();

	IGameEvent * event = gameeventmanager->CreateEvent( "sentry_on_go_active" );
	if ( event )
	{
		event->SetInt( "index", entindex() );	// object entity index

		gameeventmanager->FireEvent( event, true );	// don't send to clients
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::Precache()
{
	BaseClass::Precache();

	int iModelIndex;

	// Models
	PrecacheModel( SENTRY_MODEL_PLACEMENT );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_1 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_1_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_2 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_2_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_3 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_3_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	PrecacheModel( SENTRY_ROCKET_MODEL );
	PrecacheModel( "models/effects/sentry1_muzzle/sentry1_muzzle.mdl" );

	PrecacheModel( "models/buildables/sentry_shield.mdl" );

	// Sounds
	PrecacheScriptSound( "Building_Sentrygun.Fire" );
	PrecacheScriptSound( "Building_Sentrygun.Fire2" );	// level 2 sentry
	PrecacheScriptSound( "Building_Sentrygun.Fire3" );	// level 3 sentry
	PrecacheScriptSound( "Building_Sentrygun.FireRocket" );
	PrecacheScriptSound( "Building_Sentrygun.Alert" );
	PrecacheScriptSound( "Building_Sentrygun.AlertTarget" );
	PrecacheScriptSound( "Building_Sentrygun.Idle" );
	PrecacheScriptSound( "Building_Sentrygun.Idle2" );	// level 2 sentry
	PrecacheScriptSound( "Building_Sentrygun.Idle3" );	// level 3 sentry
	PrecacheScriptSound( "Building_Sentrygun.Built" );
	PrecacheScriptSound( "Building_Sentrygun.Empty" );
	PrecacheScriptSound( "Building_Sentrygun.ShaftFire" );
	PrecacheScriptSound( "Building_Sentrygun.ShaftFire2" );
	PrecacheScriptSound( "Building_Sentrygun.ShaftFire3" );
	PrecacheScriptSound( "Building_Sentrygun.ShaftLaserPass" );
	PrecacheScriptSound( "Building_MiniSentrygun.Fire" );

	PrecacheParticleSystem( "sentrydamage_1" );
	PrecacheParticleSystem( "sentrydamage_2" );
	PrecacheParticleSystem( "sentrydamage_3" );
	PrecacheParticleSystem( "sentrydamage_4" );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CObjectSentrygun::CanBeUpgraded( CTFPlayer *pPlayer )
{
	if ( m_bWasMapPlaced && !HasSpawnFlags(SF_SENTRY_UPGRADEABLE) )
	{
		return false;
	}

	return BaseClass::CanBeUpgraded( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Raises the Sentrygun one level
//-----------------------------------------------------------------------------
void CObjectSentrygun::StartUpgrading( void )
{
	BaseClass::StartUpgrading();

	float flMaxAmmoMult = 1.f;
	if ( GetOwner() )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwner(), flMaxAmmoMult, mvm_sentry_ammo );
	}

	switch( m_iUpgradeLevel )
	{
	case 2:
		SetModel( SENTRY_MODEL_LEVEL_2_UPGRADE );
		m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_2;
		SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_2 );
		m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_2 * flMaxAmmoMult;
		break;
	case 3:
		SetModel( SENTRY_MODEL_LEVEL_3_UPGRADE );
		if ( !m_bCarryDeploy )
		{
			m_iAmmoRockets = SENTRYGUN_MAX_ROCKETS;
		}
		m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_3;
		SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_3 );
		m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_3 * flMaxAmmoMult;
		break;
	default:
		Assert(0);
		break;
	}

	// more ammo capability
	if ( !m_bCarryDeploy )
	{
		m_iAmmoShells = m_iMaxAmmoShells;
	}

	m_iState.Set( SENTRY_STATE_UPGRADING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::FinishUpgrading( void )
{
	BaseClass::FinishUpgrading();

	m_iState.Set( SENTRY_STATE_SEARCHING );
	m_hEnemy = NULL;

	switch( m_iUpgradeLevel )
	{
	case 1:
		// This can happen when a saper downgrades a sentry
		// No need to do anything here
		break;
	case 2:
		SetModel( SENTRY_MODEL_LEVEL_2 );
		break;
	case 3:
		SetModel( SENTRY_MODEL_LEVEL_3 );
		break;
	default:
		Assert(0);
		break;
	}

	// Look up the new attachments
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = LookupAttachment( "muzzle_l" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = LookupAttachment( "muzzle_r" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET] = LookupAttachment( "rocket_l" );
}

//-----------------------------------------------------------------------------
// Hit by a friendly engineer's wrench
//-----------------------------------------------------------------------------
bool CObjectSentrygun::OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc )
{
	if ( IsDisposableBuilding() )
		return false;

	bool bDidWork = false;

	// If the player repairs it at all, we're done
	if ( GetHealth() < GetMaxHealth() )
	{
		// STAGING_ENGY
		// Mod repair value by shield value
		float flRepairAmount = pWrench->GetRepairAmount();
		if ( m_nShieldLevel == SHIELD_NORMAL )
		{
			flRepairAmount *= SHIELD_NORMAL_VALUE;
		}
		
		if ( Command_Repair( pPlayer, flRepairAmount, 1.f ) )
		{
			DoWrenchHitEffect( hitLoc, true, false );
			bDidWork = true;
		}
	}

	// Don't put in upgrade metal until the sentry is fully healed
	if ( !bDidWork )
	{
		if ( CheckUpgradeOnHit( pPlayer ) )
		{
			DoWrenchHitEffect( hitLoc, false, true );
			bDidWork = true;
		}
	}

	if ( !IsUpgrading() )
	{
		// player ammo into rockets
		//	1 ammo = 1 shell
		//  2 ammo = 1 rocket
		//  only fill rockets if we have extra shells

		int iPlayerMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );

		// If the sentry has less that 100% ammo, put some ammo in it
		if ( m_iAmmoShells < m_iMaxAmmoShells && iPlayerMetal > 0 )
		{
			int iMaxShellsPlayerCanAfford = (int)( (float)iPlayerMetal / tf_sentrygun_metal_per_shell.GetFloat() );

			// cap the amount we can add
			int iAmountToAdd = MIN( SENTRYGUN_ADD_SHELLS, iMaxShellsPlayerCanAfford );
			iAmountToAdd = MIN( ( m_iMaxAmmoShells - m_iAmmoShells ), iAmountToAdd );

			// STAGING_ENGY
			// Mod Ammo if shielded
			if ( m_nShieldLevel == SHIELD_NORMAL )
			{
				iAmountToAdd *= SHIELD_NORMAL_VALUE;
			}

			pPlayer->RemoveAmmo( iAmountToAdd * tf_sentrygun_metal_per_shell.GetInt(), TF_AMMO_METAL );
			m_iAmmoShells += iAmountToAdd;

			if ( iAmountToAdd > 0 )
			{
				bDidWork = true;
			}
		}

		// One rocket per two ammo
		iPlayerMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );

		if ( m_iAmmoRockets < m_iMaxAmmoRockets && m_iUpgradeLevel == 3 && iPlayerMetal > 0  )
		{
			int iMaxRocketsPlayerCanAfford = (int)( (float)iPlayerMetal / tf_sentrygun_metal_per_rocket.GetFloat() );

			int iAmountToAdd = MIN( ( SENTRYGUN_ADD_ROCKETS ), iMaxRocketsPlayerCanAfford );
			iAmountToAdd = MIN( ( m_iMaxAmmoRockets - m_iAmmoRockets ), iAmountToAdd );

			// STAGING_ENGY
			// Mod Ammo if shielded
			if ( m_nShieldLevel == SHIELD_NORMAL )
			{
				iAmountToAdd *= SHIELD_NORMAL_VALUE;
			}

			pPlayer->RemoveAmmo( iAmountToAdd * tf_sentrygun_metal_per_rocket.GetFloat(), TF_AMMO_METAL );
			m_iAmmoRockets += iAmountToAdd;

			if ( iAmountToAdd > 0 )
			{
				bDidWork = true;
			}
		}
	}

	if ( GetOwner() != pPlayer )
	{
		if ( bDidWork && m_bPlayerControlled )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_HELP_MANUAL_SENTRY, 1 );
		}

		// keep track of who last hit us with a wrench for kill assists
		m_lastTeammateWrenchHit = pPlayer;
		m_lastTeammateWrenchHitTimer.Start();
	}

	return bDidWork;
}

//-----------------------------------------------------------------------------
// Debug infos
//-----------------------------------------------------------------------------
int CObjectSentrygun::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf( tempstr, sizeof( tempstr ), "Level: %d", m_iUpgradeLevel.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf( tempstr, sizeof( tempstr ), "Shells: %d / %d", m_iAmmoShells.Get(), m_iMaxAmmoShells.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if ( m_iUpgradeLevel == 3 )
		{
			Q_snprintf( tempstr, sizeof( tempstr ), "Rockets: %d / %d", m_iAmmoRockets.Get(), m_iMaxAmmoRockets.Get() );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		Q_snprintf( tempstr, sizeof( tempstr ), "Upgrade metal %d", m_iUpgradeMetal.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Vector vecSrc = EyePosition();
		Vector forward;

		// m_vecCurAngles
		AngleVectors( m_vecCurAngles, &forward );
		NDebugOverlay::Line( vecSrc, vecSrc + forward * 200, 0, 255, 0, false, 0.1 );

		// m_vecGoalAngles
		AngleVectors( m_vecGoalAngles, &forward );
		NDebugOverlay::Line( vecSrc, vecSrc + forward * 200, 0, 0, 255, false, 0.1 );
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Returns the sentry targeting range the target is in
//-----------------------------------------------------------------------------
int CObjectSentrygun::Range( CBaseEntity *pTarget )
{
	Vector vecOrg = EyePosition();
	Vector vecTargetOrg = pTarget->EyePosition();

	int iDist = ( vecTargetOrg - vecOrg ).Length();

	if (iDist < 132)
		return RANGE_MELEE;
	if (iDist < 550)
		return RANGE_NEAR;
	if (iDist < m_flSentryRange)
		return RANGE_MID;
	return RANGE_FAR;
}

//-----------------------------------------------------------------------------
// Look for a target
//-----------------------------------------------------------------------------
bool CObjectSentrygun::FindTarget()
{
	if ( m_bPlayerControlled )
	{
		m_flShieldFadeTime = gpGlobals->curtime + WRANGLER_DISABLE_TIME;
	}
	m_bPlayerControlled = false;

	// Disable the sentry guns for ifm.
	if ( tf_sentrygun_notarget.GetBool() )
		return false;

	if ( IsInCommentaryMode() )
		return false;

	// Sapper, etc.
	if ( IsDisabled() )
		return false;

	// Loop through players within SENTRY_MAX_RANGE units (sentry range).
	Vector vecSentryOrigin = EyePosition();

	// find the enemy team
	int iEnemyTeam = ( GetTeamNumber() == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( iEnemyTeam );
	if ( !pTeam )
		return false;

	// If we have an enemy get his minimum distance to check against.
	Vector vecSegment;
	Vector vecTargetCenter;
	float flMinDist2 = m_flSentryRange * m_flSentryRange;
	CBaseEntity *pTargetCurrent = NULL;
	CBaseEntity *pTargetOld = m_hEnemy.Get();
	float flOldTargetDist2 = FLT_MAX;
	bool bDummyTarget = false;

	// Sentry Decoy
	// If we already have a sentry decoy target, keep shooting at it
	// Otherwise look for a sentry decoy's first
	if ( pTargetCurrent == NULL )
	{
		CTFTargetDummy *pDummy = dynamic_cast<CTFTargetDummy*>( pTargetOld );
		if ( pDummy )
		{
			pTargetCurrent = pDummy;
			bDummyTarget = true;
		}
		else
		{
			// Search through all dummies and find one in range
			for ( int i = 0; i < ITFTargetDummy::AutoList().Count(); ++i )
			{
				pDummy = static_cast<CTFTargetDummy*>( ITFTargetDummy::AutoList()[i] );
				if ( InSameTeam( pDummy ) )
					continue;

				vecTargetCenter = pDummy->GetAbsOrigin();
				vecTargetCenter += pDummy->GetViewOffset();
				VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
				float flDist2 = vecSegment.LengthSqr();

				// Check to see if the target is closer than the already validated target.
				if ( flDist2 > flMinDist2 )
					continue;

				// Ray trace!!!
				if ( FVisible( pDummy, MASK_SHOT | CONTENTS_GRATE ) )
				{
					pTargetCurrent = pDummy;
					bDummyTarget = true;
				}
			}
		}
	}

	// If our builder has an active laser pointer we don't seek targets.
	CTFPlayer* pBuilder = GetBuilder();
	if ( pBuilder )
	{
		// CTFLaserPointer* pPointer = static_cast<CTFLaserPointer*>( pBuilder->Weapon_OwnsThisID( TF_WEAPON_LASER_POINTER ) );
		// FIX ME:  Temp fix until we find out why the pointer thinks its deployed after spawn
		CTFLaserPointer* pPointer = dynamic_cast<CTFLaserPointer*>( pBuilder->GetActiveWeapon() );
		if ( pPointer && pPointer->HasLaserDot() && !IsDisposableBuilding() )
		{
			m_bPlayerControlled = true;
			m_nShieldLevel.Set( SHIELD_NORMAL );
			m_flShieldFadeTime = gpGlobals->curtime + WRANGLER_DISABLE_TIME;

			// If not target dummy, use laserdot, otherwise targetdummy overrides
			if ( !bDummyTarget || !pTargetCurrent )
			{
				pTargetCurrent = pPointer->GetLaserDot();

				// Are we in our brief auto aim period?
				float flAutoAimTime = gpGlobals->curtime - m_flAutoAimStartTime;
				if ( m_hAutoAimTarget && (flAutoAimTime < 0.2f) )
				{
					// Only use the auto aim target if we can actually range to him.
					Vector vecSrc;
					QAngle vecAng;
					GetAttachment( GetFireAttachment(), vecSrc, vecAng );
					Vector vecEnemy = GetEnemyAimPosition( m_hAutoAimTarget );
					trace_t	trace;
					CTraceFilterIgnoreTeammatesAndTeamObjects filter( pBuilder, COLLISION_GROUP_NONE, pBuilder->GetTeamNumber() );
					UTIL_TraceLine( vecSrc, vecEnemy, MASK_SOLID, &filter, &trace );
					if ( trace.m_pEnt == m_hAutoAimTarget )
					{
						pTargetCurrent = m_hAutoAimTarget;
					}
					else
					{
						m_hAutoAimTarget = NULL;
					}
				}
				else
				{
					m_hAutoAimTarget = NULL;
				}
			}

			if ( pTargetCurrent->GetAbsOrigin().DistTo( vecSentryOrigin ) > 30.f )
			{
				if ( pTargetCurrent != pTargetOld )
				{
					FoundTarget( pTargetCurrent, vecSentryOrigin, true );
				}
				return true;
			}
			else
			{
				pTargetCurrent = NULL;
			}
		}
	}

	// Don't auto track to targets while under the effects of the player shield.
	// The shield fades 3 seconds after we disengage from player control.
	if ( m_nShieldLevel == SHIELD_NORMAL )
		return false;

	// is there an active truce?
	bool bTruceActive = TFGameRules() && TFGameRules()->IsTruceActive();
		
	if ( ( pTargetCurrent == NULL ) && !bTruceActive )
	{
		// Sentries will try to target players first, then objects.  However, if the enemy held was an object it will continue
		// to try and attack it first.
		int nTeamCount = pTeam->GetNumPlayers();
		for ( int iPlayer = 0; iPlayer < nTeamCount; ++iPlayer )
		{
			CTFPlayer *pTargetPlayer = static_cast<CTFPlayer*>( pTeam->GetPlayer( iPlayer ) );
			if ( pTargetPlayer == NULL )
				continue;

			// Make sure the player is alive.
			if ( !pTargetPlayer->IsAlive() )
				continue;

			if ( pTargetPlayer->GetFlags() & FL_NOTARGET )
				continue;

			vecTargetCenter = pTargetPlayer->GetAbsOrigin();
			vecTargetCenter += pTargetPlayer->GetViewOffset();
			VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
			float flDist2 = vecSegment.LengthSqr();

			// Check to see if the target is closer than the already validated target.
			if ( flDist2 > flMinDist2 )
				continue;

			// It is closer, check to see if the target is valid.
			if ( ValidTargetPlayer( pTargetPlayer, vecSentryOrigin, vecTargetCenter ) )
			{
				flMinDist2 = flDist2;
				pTargetCurrent = pTargetPlayer;

				// Store the current target distance if we come across it
				if ( pTargetPlayer == pTargetOld )
				{
					flOldTargetDist2 = flDist2;
				}
			}
		}
	}

	// If we already have a target, don't check objects.
	if ( pTargetCurrent == NULL )
	{
		// target non-player bots
		CUtlVector< INextBot * > botVector;
		TheNextBots().CollectAllBots( &botVector );

		float closeBotRangeSq = m_flSentryRange * m_flSentryRange;

		for( int b=0; b<botVector.Count(); ++b )
		{
			CBaseCombatCharacter *bot = botVector[b]->GetEntity();

			Vector vecBotTarget = GetEnemyAimPosition( bot );
			float rangeSq = ( vecBotTarget - vecSentryOrigin ).LengthSqr();

			if ( rangeSq < closeBotRangeSq )
			{
				if ( ValidTargetBot( bot, vecSentryOrigin, vecBotTarget ) )
				{
					closeBotRangeSq = rangeSq;
					pTargetCurrent = bot;
				}
			}
		}

		if ( ( pTargetCurrent == NULL ) && !bTruceActive )
		{
			// target objects
			int nTeamObjectCount = pTeam->GetNumObjects();
			for ( int iObject = 0; iObject < nTeamObjectCount; ++iObject )
			{
				CBaseObject *pTargetObject = pTeam->GetObject( iObject );
				if ( !pTargetObject )
					continue;

				vecTargetCenter = pTargetObject->GetAbsOrigin();
				vecTargetCenter += pTargetObject->GetViewOffset();
				VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
				float flDist2 = vecSegment.LengthSqr();

				// Store the current target distance if we come across it
				if ( pTargetObject == pTargetOld )
				{
					flOldTargetDist2 = flDist2;
				}

				// Check to see if the target is closer than the already validated target.
				if ( flDist2 > flMinDist2 )
					continue;

				// It is closer, check to see if the target is valid.
				if ( ValidTargetObject( pTargetObject, vecSentryOrigin, vecTargetCenter ) )
				{
					flMinDist2 = flDist2;
					pTargetCurrent = pTargetObject;
				}
			}
		}
	}

	// We have a target.
	if ( pTargetCurrent )
	{
		if ( pTargetCurrent != pTargetOld )
		{
			// Always target dummies
			// flMinDist2 is the new target's distance
			// flOldTargetDist2 is the old target's distance
			// Don't switch unless the new target is closer by some percentage
			if ( bDummyTarget || flMinDist2 < ( flOldTargetDist2 * 0.75f ) )
			{
				FoundTarget( pTargetCurrent, vecSentryOrigin );
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd )
{
	// Keep shooting at spies that go invisible after we acquire them as a target.
	if ( pPlayer->m_Shared.GetPercentInvisible() > 0.5 )
		return false;

	// Keep shooting at spies that disguise after we acquire them as at a target.
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber() && pPlayer != m_hEnemy )
		return false;

	// Don't shoot spys that are pretending to be a dispenser
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) )
		return false;

	// Don't target spies after they OnKill disguise with 'Your Eternal Reward'
	if ( ( pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) || pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		&& pPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber() )
	{
		CTFKnife *pKnife = dynamic_cast<CTFKnife *>( pPlayer->GetActiveTFWeapon() );
		if ( pKnife && pKnife->GetKnifeType() == KNIFE_DISGUISE_ONKILL )
			return false;
	}

	// Not across water boundary.
	if ( ( GetWaterLevel() == 0 && pPlayer->GetWaterLevel() >= 3 ) || ( GetWaterLevel() == 3 && pPlayer->GetWaterLevel() <= 0 ) )
		return false;

	// Ray trace!!!
	return FVisible( pPlayer, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd )
{
	// Ignore objects being placed, they are not real objects yet.
	if ( pObject->IsPlacing() )
		return false;

	// Ignore sappers.
	if ( pObject->MustBeBuiltOnAttachmentPoint() )
		return false;

	// Not across water boundary.
	if ( ( GetWaterLevel() == 0 && pObject->GetWaterLevel() >= 3 ) || ( GetWaterLevel() == 3 && pObject->GetWaterLevel() <= 0 ) )
		return false;

	if ( pObject->GetObjectFlags() & OF_DOESNT_HAVE_A_MODEL )
		return false;

	// Ray trace.
	return FVisible( pObject, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetBot( CBaseCombatCharacter *pBot, const Vector &vecStart, const Vector &vecEnd )
{
	// Already collected all of the players in FindTarget()
	if ( pBot->IsPlayer() )
		return false;

	// Don't want to shoot bots that are dead, on the same team, or aren't solid (they won't take damage anyway)
	if  ( !pBot->IsAlive() || pBot->InSameTeam( this ) || pBot->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return false;

	// Not across water boundary.
	if ( ( GetWaterLevel() == 0 && pBot->GetWaterLevel() >= 3 ) || ( GetWaterLevel() == 3 && pBot->GetWaterLevel() <= 0 ) )
		return false;

	if ( TFGameRules() && TFGameRules()->IsPlayingRobotDestructionMode() )
	{
		CTFRobotDestruction_Robot *pRobot = dynamic_cast< CTFRobotDestruction_Robot* >( pBot );
		if ( pRobot && pRobot->GetShieldedState() )
			return false;
	}

	// Ray trace.
	CBaseEntity *pBlocker;
	bool bVisible = FVisible( pBot, MASK_SHOT | CONTENTS_GRATE, &pBlocker );

	if ( bVisible )
		return true;

	// Also valid if it's parented to the blocker
	if ( pBlocker == pBot->GetParent() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Found a Target
//-----------------------------------------------------------------------------
void CObjectSentrygun::FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter, bool bNoSound )
{
	m_hEnemy = pTarget;

	if ( ( m_iAmmoShells > 0 ) || ( m_iAmmoRockets > 0 && m_iUpgradeLevel == 3 ) )
	{
		// Play one sound to everyone but the target.
		CPASFilter filter( vecSoundCenter );

		if ( pTarget->IsPlayer() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pTarget );

			// Play a specific sound just to the target and remove it from the general recipient list.
			if ( !bNoSound )
			{
				CSingleUserRecipientFilter singleFilter( pPlayer );
				EmitSentrySound( singleFilter, entindex(), "Building_Sentrygun.AlertTarget" );
				filter.RemoveRecipient( pPlayer );

				// if the target is a bot, alert it
				CTFBot *bot = ToTFBot( pPlayer );
				if ( bot )
				{
					bot->GetVisionInterface()->AddKnownEntity( this );
					bot->RememberEnemySentry( this, bot->GetAbsOrigin() );
				}
			}
		}

		if ( !bNoSound )
		{
			EmitSentrySound( filter, entindex(), "Building_Sentrygun.Alert" );
		}
	}

	// Update timers, we are attacking now!
	m_iState.Set( SENTRY_STATE_ATTACKING );
	m_flNextAttack = gpGlobals->curtime + SENTRY_THINK_DELAY;
	if ( m_flNextRocketAttack < gpGlobals->curtime )
	{
		m_flNextRocketAttack = gpGlobals->curtime;// + 0.5;
	}
}

//-----------------------------------------------------------------------------
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::FInViewCone ( CBaseEntity *pEntity )
{
	Vector forward;
	AngleVectors( m_vecCurAngles, &forward );

	Vector2D vec2LOS = ( pEntity->GetAbsOrigin() - GetAbsOrigin() ).AsVector2D();
	vec2LOS.NormalizeInPlace();

	float flDot = vec2LOS.Dot( forward.AsVector2D() );

	if ( flDot > m_flFieldOfView )
	{
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Make sure our target is still valid, and if so, fire at it
//-----------------------------------------------------------------------------
void CObjectSentrygun::Attack()
{
	StudioFrameAdvance( );

	if ( IsUsingReverseBuild() || !FindTarget() )
	{
		m_iState.Set( SENTRY_STATE_SEARCHING );
		m_hEnemy = NULL;
		return;
	}

	// Track enemy
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = GetEnemyAimPosition( m_hEnemy );
	Vector vecDirToEnemy = vecMidEnemy - vecMid;

	QAngle angToTarget;
	VectorAngles( vecDirToEnemy, angToTarget );

	angToTarget.y = UTIL_AngleMod( angToTarget.y );
	if (angToTarget.x < -180)
		angToTarget.x += 360;
	if (angToTarget.x > 180)
		angToTarget.x -= 360;

	// now all numbers should be in [1...360]
	// pin to turret limitations to [-50...50]
	if (angToTarget.x > 50)
		angToTarget.x = 50;
	else if (angToTarget.x < -50)
		angToTarget.x = -50;
	m_vecGoalAngles.y = angToTarget.y;
	m_vecGoalAngles.x = angToTarget.x;

	MoveTurret();

	// Fire on the target if it's within 10 units of being aimed right at it
	if ( m_flNextAttack <= gpGlobals->curtime && (m_vecGoalAngles - m_vecCurAngles).Length() <= 10 )
	{
		if ( !m_bPlayerControlled || m_bFireNextFrame )
		{
			m_bFireNextFrame = false;
			Fire();
		}

		m_flFireRate = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwner(), m_flFireRate, mult_sentry_firerate );

		if ( m_bPlayerControlled )
		{
			m_flFireRate *= 0.5f;
		}
			
		if ( IsMiniBuilding() && !IsDisposableBuilding() )
		{
			m_flFireRate *= 0.75f;
		}

		if ( GetBuilder() && GetBuilder()->m_Shared.InCond( TF_COND_CRITBOOSTED_USER_BUFF ) )
		{
			m_flFireRate *= 0.4f;
		}

		if ( m_iUpgradeLevel == 1 )
		{
			// Level 1 sentries fire slower
			m_flNextAttack = gpGlobals->curtime + (0.2*m_flFireRate);
		}
		else
		{
			m_flNextAttack = gpGlobals->curtime + (0.1*m_flFireRate);
		}
	}
	else
	{
		// SetSentryAnim( TFTURRET_ANIM_SPIN );
	}

	if ( m_bPlayerControlled && m_bFireRocketNextFrame )
	{
		m_bFireRocketNextFrame = false;
		FireRocket();
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::FireRocket()
{
	if ( m_flNextRocketAttack >= gpGlobals->curtime || m_iAmmoRockets <= 0 )
		return false;

	if ( m_hEnemy.Get() == NULL )
		return false;

	Vector vecAimDir;

	Vector vecSrc;
	QAngle vecAng;

	GetAttachment( m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET], vecSrc, vecAng );

	Vector vecEnemyPos = GetEnemyAimPosition( m_hEnemy );
	vecAimDir = vecEnemyPos - vecSrc;
	vecAimDir.NormalizeInPlace();

	// If we cannot see their WorldSpaceCenter ( possible, as we do our target finding based
	// on the eye position of the target ) then fire at the eye position
	trace_t tr;

	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	ITraceFilter *pFilterChain = NULL;

	CTraceFilterIgnoreFriendlyCombatItems traceFilterCombatItem( this, COLLISION_GROUP_NONE, GetTeamNumber() );
	if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
	{
		// Ignore teammates and their (physical) upgrade items in MvM
		pFilterChain = &traceFilterCombatItem;
	}

	CTraceFilterChain traceFilterChain( &traceFilter, pFilterChain );
	UTIL_TraceLine( vecSrc, vecEnemyPos, MASK_SOLID, &traceFilterChain, &tr);

	if ( m_bPlayerControlled || (tr.m_pEnt && !tr.m_pEnt->IsWorld()) )
	{
		// NOTE: vecAng is not actually set by GetAttachment!!!
		QAngle angDir;
		VectorAngles( vecAimDir, angDir );

		EmitSentrySound( "Building_Sentrygun.FireRocket" );

		QAngle angAimDir;
		VectorAngles( vecAimDir, angAimDir );
		CTFProjectile_SentryRocket *pProjectile = CTFProjectile_SentryRocket::Create( vecSrc, angAimDir, this, GetBuilder() );
		if ( pProjectile )
		{
			int iDamage = 100;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOwner(), iDamage, mult_engy_sentry_damage );
			pProjectile->SetDamage( iDamage );
		}

		// Setup next rocket shot
		if ( m_bPlayerControlled )
		{
			m_flNextRocketAttack = gpGlobals->curtime + 2.25;
		}
		else
		{
			AddGesture( ACT_RANGE_ATTACK2 );
			m_flNextRocketAttack = gpGlobals->curtime + 3;
		}

		if ( !tf_sentrygun_ammocheat.GetBool() && !HasSpawnFlags( SF_SENTRY_INFINITE_AMMO ) )
		{
			m_iAmmoRockets--;
		}
	}

	m_timeSinceLastFired.Start();

	return true;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CObjectSentrygun::GetFireAttachment()
{
	int iAttachment;

	if ( m_iUpgradeLevel > 1 && m_iLastMuzzleAttachmentFired == m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] )
	{
		// level 2 and 3 turrets alternate muzzles each time they fizzy fizzy fire.
		iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT];
	}
	else
	{
		iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
	}
	m_iLastMuzzleAttachmentFired = iAttachment;

	return iAttachment;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnKilledEnemy(CBasePlayer* pVictim)
{
	if ( !pVictim )
		return;

	CTFPlayer *pOwner = GetOwner();
	if ( !pOwner )
		return;

	if ( m_bPlayerControlled && pVictim->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) > ( m_flSentryRange * m_flSentryRange ) )
	{
		pOwner->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_MANUAL_SENTRY_KILLS_BEYOND_RANGE );
	}

	CTFPlayer *pCTFVictim = static_cast<CTFPlayer *>( pVictim );
	if ( pCTFVictim->GetControlPointStandingOn() != NULL )
	{
		pOwner->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_SENTRY_KILL_CAPS, 1 );
	}

	if ( (gpGlobals->curtime - GetCarryDeployTime() < tf_sentrygun_kill_after_redeploy_time_achievement.GetInt()) &&
		 GetUpgradeLevel() == 3 )
	{
		pOwner->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_MOVE_SENTRY_GET_KILL );
	}
}

//-----------------------------------------------------------------------------
// Fire on our target
//-----------------------------------------------------------------------------
bool CObjectSentrygun::Fire()
{
	//NDebugOverlay::Cross3D( m_hEnemy->WorldSpaceCenter(), 10, 255, 0, 0, false, 0.1 );

	Vector vecAimDir;

	// Level 3 Turrets fire rockets every 3 seconds
	if ( m_iUpgradeLevel == 3 &&
		 m_iAmmoRockets > 0 &&
		 m_flNextRocketAttack < gpGlobals->curtime &&
		 !m_bPlayerControlled )
	{
		FireRocket();
	}

	// All turrets fire shells
	if ( m_iAmmoShells > 0 )
	{
		if ( !IsPlayingGesture( ACT_RANGE_ATTACK1 ) )
		{
			RemoveGesture( ACT_RANGE_ATTACK1_LOW );
			AddGesture( ACT_RANGE_ATTACK1 );
		}

		if ( m_hEnemy.Get() == NULL )
			return false;

		Vector vecSrc;
		QAngle vecAng;

		int iAttachment = GetFireAttachment();
		GetAttachment( iAttachment, vecSrc, vecAng );

		// Because the muzzle is so long, it can stick through a wall if the sentry is right up against it.
		// Make sure the sentry can't fire in this condition by tracing a line between the center of the gun and the end of the muzzle.
		trace_t trace;
		UTIL_TraceLine( WorldSpaceCenter(), vecSrc, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
		if ( ( trace.fraction < 1.0f ) && ( !trace.m_pEnt || trace.m_pEnt->m_takedamage == DAMAGE_NO ) )
		{
			// there is something between the center and the end of the muzzle, most likely a wall, so don't fire
			return false;
		}

		Vector vecMidEnemy = GetEnemyAimPosition( m_hEnemy );

		// If we cannot see their WorldSpaceCenter ( possible, as we do our target finding based
		// on the eye position of the target ) then fire at the eye position
		trace_t tr;
		CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
		ITraceFilter *pFilterChain = NULL;

		CTraceFilterIgnoreFriendlyCombatItems traceFilterCombatItem( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
		{
			// Ignore teammates and their (physical) upgrade items in MvM
			pFilterChain = &traceFilterCombatItem;
		}

		CTraceFilterChain traceFilterChain( &traceFilter, pFilterChain );
		UTIL_TraceLine( vecSrc, vecMidEnemy, MASK_SOLID, &traceFilterChain, &tr);

		if ( !tr.m_pEnt || tr.m_pEnt->IsWorld() )
		{
			// Hack it lower a little bit..
			// The eye position is not always within the hitboxes for a standing TF Player
			vecMidEnemy = m_hEnemy->EyePosition() + Vector(0,0,-5);
		}

		vecAimDir = vecMidEnemy - vecSrc;

		float flDistToTarget = vecAimDir.Length();

		vecAimDir.NormalizeInPlace();

		//NDebugOverlay::Cross3D( vecSrc, 10, 255, 0, 0, false, 0.1 );

		FireBulletsInfo_t info;

		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecAimDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = GetBuilder();
		if ( info.m_pAttacker == NULL )
		{
			info.m_pAttacker = this;
		}
		if ( m_bPlayerControlled )
		{
			info.m_vecSpread = VECTOR_CONE_3DEGREES;
		}
		else
		{
			info.m_vecSpread = vec3_origin;
		}
		info.m_flDistance = flDistToTarget + 100;
		info.m_iAmmoType = m_iAmmoType;

		if ( IsMiniBuilding() )
		{
			info.m_flDamage = tf_sentrygun_mini_damage.GetFloat();
			info.m_flDamageForceScale = 0.0f;
		}
		else
		{
			info.m_flDamage = tf_sentrygun_damage.GetFloat();
		}

		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwner(), info.m_flDamage, mult_engy_sentry_damage );

		FireBullets( info );

		// sentry gun fire 'heats up' the nav mesh around it
		UpdateNavMeshCombatStatus();


		//NDebugOverlay::Line( vecSrc, vecSrc + vecAimDir * 1000, 255, 0, 0, false, 0.1 );

		CEffectData data;
		data.m_nEntIndex = entindex();
		data.m_nAttachmentIndex = iAttachment;
		data.m_fFlags = m_iUpgradeLevel;
		data.m_vOrigin = vecSrc;
		DispatchEffect( "TF_3rdPersonMuzzleFlash_SentryGun", data );

		if ( IsMiniBuilding() )
		{
			EmitSound_t params;
			params.m_pSoundName = "Building_MiniSentrygun.Fire";
			params.m_flSoundTime = 0;
			params.m_pflSoundDuration = 0;
			params.m_bWarnOnDirectWaveReference = true;
			CPASAttenuationFilter filter( this, "Building_MiniSentrygun.Fire" );
			EmitSound( filter, entindex(), params );
		}
		else
		{
			if ( !m_bPlayerControlled )
			{
				switch( m_iUpgradeLevel )
				{
				case 1:
				default:
					EmitSentrySound( "Building_Sentrygun.Fire" );
					break;
				case 2:
					EmitSentrySound( "Building_Sentrygun.Fire2" );
					break;
				case 3:
					EmitSentrySound( "Building_Sentrygun.Fire3" );
					break;
				}
			}
			else
			{
				switch ( m_iUpgradeLevel )
				{
				case 1:
					EmitSentrySound( "Building_Sentrygun.ShaftFire" );
					break;
				case 2:
					EmitSentrySound( "Building_Sentrygun.ShaftFire2" );
					break;
				case 3:
					EmitSentrySound( "Building_Sentrygun.ShaftFire3" );
					break;
				}
			}
		}

		if ( !tf_sentrygun_ammocheat.GetBool() && !HasSpawnFlags( SF_SENTRY_INFINITE_AMMO ) )
		{
			m_iAmmoShells--;
		}
	}
	else
	{
		if ( m_iUpgradeLevel > 1 )
		{
			if ( !IsPlayingGesture( ACT_RANGE_ATTACK1_LOW ) )
			{
				RemoveGesture( ACT_RANGE_ATTACK1 );
				AddGesture( ACT_RANGE_ATTACK1_LOW );
			}
		}

		// Out of ammo, play a click
		EmitSound( "Building_Sentrygun.Empty" );

		// Disposable sentries blow up when their ammo runs out
		if ( IsDisposableBuilding() )
		{
			DetonateObject();
		}

		m_flNextAttack = gpGlobals->curtime + 0.2;
	}

	// note when we last fired at en enemy (or tried to)
	m_timeSinceLastFired.Start();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::ModifyFireBulletsDamage( CTakeDamageInfo* dmgInfo )
{
	if ( m_bPlayerControlled && dmgInfo )
	{
		dmgInfo->SetDamageCustom( TF_DMG_CUSTOM_PLAYER_SENTRY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CObjectSentrygun::GetPushMultiplier()
{
	if ( IsMiniBuilding() )
		return 8.f;
	else
		return 16.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	trace_t tmptrace;
	tmptrace.endpos = tr.endpos + RandomVector(-10,10);

	// Sentryguns are perfectly accurate, but this doesn't look good for tracers.
	// Add a little noise to them, but not enough so that it looks like they're missing.
	BaseClass::MakeTracer( vecTracerSrc, tmptrace, iTracerType );
}

//-----------------------------------------------------------------------------
// Purpose: MakeTracer asks back for the attachment index
//-----------------------------------------------------------------------------
int	CObjectSentrygun::GetTracerAttachment( void )
{
	return m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
}

//-----------------------------------------------------------------------------
// Rotate and scan for targets
//-----------------------------------------------------------------------------
void CObjectSentrygun::SentryRotate( void )
{
	if ( GetReversesBuildingConstructionSpeed() )
	{
		m_iState.Set( SENTRY_STATE_INACTIVE );
		return;
	}

	// if we're playing a fire gesture, stop it
	if ( IsPlayingGesture( ACT_RANGE_ATTACK1 ) )
	{
		RemoveGesture( ACT_RANGE_ATTACK1 );
	}

	if ( IsPlayingGesture( ACT_RANGE_ATTACK1_LOW ) )
	{
		RemoveGesture( ACT_RANGE_ATTACK1_LOW );
	}

	// animate
	StudioFrameAdvance();

	// Look for a target
	if ( FindTarget() )
		return;
	
	// Rotate
	if ( !MoveTurret() )
	{
		// Change direction

		if ( IsDisabled() || m_nShieldLevel == SHIELD_NORMAL )
		{
			EmitSound( "Building_Sentrygun.Disabled" );
			m_vecGoalAngles.x = 30;
		}
		else
		{
			switch( m_iUpgradeLevel )
			{
			case 1:
			default:
				EmitSentrySound( "Building_Sentrygun.Idle" );
				break;
			case 2:
				EmitSound( "Building_Sentrygun.Idle2" );
				break;
			case 3:
				EmitSound( "Building_Sentrygun.Idle3" );
				break;
			}

			// Switch rotation direction
			if ( m_bTurningRight )
			{
				m_bTurningRight = false;
				m_vecGoalAngles.y = m_iLeftBound;
			}
			else
			{
				m_bTurningRight = true;
				m_vecGoalAngles.y = m_iRightBound;
			}

			// Randomly look up and down a bit
			if (random->RandomFloat(0, 1) < 0.3)
			{
				m_vecGoalAngles.x = (int)random->RandomFloat(-10,10);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add the EMP effect
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnStartDisabled( void )
{
	// stay at current rotation, angle down
	m_vecGoalAngles.x = m_vecCurAngles.x;
	m_vecGoalAngles.y = m_vecCurAngles.y;

	// target = nULL

	BaseClass::OnStartDisabled();
}

//-----------------------------------------------------------------------------
// Purpose: Remove the EMP effect
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnEndDisabled( void )
{
	// return to normal rotations
	if ( m_bTurningRight )
	{
		m_bTurningRight = false;
		m_vecGoalAngles.y = m_iLeftBound;
	}
	else
	{
		m_bTurningRight = true;
		m_vecGoalAngles.y = m_iRightBound;
	}

	m_vecGoalAngles.x = 0;

	BaseClass::OnEndDisabled();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CObjectSentrygun::GetBaseTurnRate( void )
{
	if ( m_bPlayerControlled )
	{
		return m_iBaseTurnRate * 100;
	}
	else
	{
		return m_iBaseTurnRate;
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::MoveTurret( void )
{
	bool bMoved = false;

	int iBaseTurnRate = GetBaseTurnRate();
	
	if ( IsMiniBuilding() )
	{
		iBaseTurnRate *= 1.35f;
	}

	// any x movement?
	if ( m_vecCurAngles.x != m_vecGoalAngles.x )
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += SENTRY_THINK_DELAY * ( iBaseTurnRate * 5 ) * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if ( flDir == 1 )
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		SetPoseParameter( m_iPitchPoseParameter, -m_vecCurAngles.x );

		bMoved = true;
	}

	if ( m_vecCurAngles.y != m_vecGoalAngles.y )
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs( m_vecGoalAngles.y - m_vecCurAngles.y );
		bool bReversed = false;

		if ( flDist > 180 )
		{
			flDist = 360 - flDist;
			flDir = -flDir;
			bReversed = true;
		}

		if ( m_hEnemy.Get() == NULL )
		{
			if ( flDist > 30 )
			{
				if ( m_flTurnRate < iBaseTurnRate * 10 )
				{
					m_flTurnRate += iBaseTurnRate;
				}
			}
			else
			{
				// Slow down
				if ( m_flTurnRate > (iBaseTurnRate * 5) )
					m_flTurnRate -= iBaseTurnRate;
			}
		}
		else
		{
			// When tracking enemies, move faster and don't slow
			if ( flDist > 30 )
			{
				if (m_flTurnRate < iBaseTurnRate * 30)
				{
					m_flTurnRate += iBaseTurnRate * 3;
				}
			}
		}

		m_vecCurAngles.y += SENTRY_THINK_DELAY * m_flTurnRate * flDir;

		// if we passed over the goal, peg right to it now
		if (flDir == -1)
		{
			if ( (bReversed == false && m_vecGoalAngles.y > m_vecCurAngles.y) ||
				(bReversed == true && m_vecGoalAngles.y < m_vecCurAngles.y) )
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		} 
		else
		{
			if ( (bReversed == false && m_vecGoalAngles.y < m_vecCurAngles.y) ||
                (bReversed == true && m_vecGoalAngles.y > m_vecCurAngles.y) )
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		}

		if ( m_vecCurAngles.y < 0 )
		{
			m_vecCurAngles.y += 360;
		}
		else if ( m_vecCurAngles.y >= 360 )
		{
			m_vecCurAngles.y -= 360;
		}

		if ( flDist < ( SENTRY_THINK_DELAY * 0.5 * iBaseTurnRate ) )
		{
			m_vecCurAngles.y = m_vecGoalAngles.y;
		}

		QAngle angles = GetAbsAngles();

		float flYaw = m_vecCurAngles.y - angles.y;

		SetPoseParameter( m_iYawPoseParameter, -flYaw );

		InvalidatePhysicsRecursive( ANIMATION_CHANGED );

		bMoved = true;
	}

	if ( !bMoved || m_flTurnRate <= 0 )
	{
		m_flTurnRate = iBaseTurnRate * 5;
	}

	return bMoved;
}

//-----------------------------------------------------------------------------
// Purpose: Note our last attacked time
//-----------------------------------------------------------------------------
int CObjectSentrygun::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;

	// As we increase in level, we get more resistant to minigun bullets, to compensate for
	// our increased surface area taking more minigun hits.
	if ( ( info.GetDamageType() & DMG_BULLET ) && ( info.GetDamageCustom() == TF_DMG_CUSTOM_MINIGUN ) )
	{
		float flDamage = newInfo.GetDamage();
		flDamage *= ( 1.0 - m_flHeavyBulletResist );
		newInfo.SetDamage( flDamage );
	}
	
	int iAttackIgnoresResists = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iAttackIgnoresResists, mod_pierce_resists_absorbs );

	// If we are shielded due to player control, we take less damage.
	bool bFullyShielded = ( m_nShieldLevel > 0 && !iAttackIgnoresResists ) && !HasSapper() && !IsPlasmaDisabled();
	if ( bFullyShielded )
	{
		float flDamage = newInfo.GetDamage();
		flDamage *= ( m_nShieldLevel == SHIELD_NORMAL ) ? SHIELD_NORMAL_VALUE : SHIELD_MAX_VALUE;
		newInfo.SetDamage( flDamage );
	}

	// Check to see if we are being sapped.
	if ( HasSapper() )
	{
		// Get the sapper owner.
		CBaseObject *pSapper = GetObjectOfTypeOnMe( OBJ_ATTACHMENT_SAPPER );

		// Take less damage if the owner is causing additional damage.
		if ( pSapper && ( info.GetAttacker() == pSapper->GetOwner() ) )
		{
			float flDamage = newInfo.GetDamage() * SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER;
			newInfo.SetDamage( flDamage );
		}
	}

	int iDamageTaken = BaseClass::OnTakeDamage( newInfo );

	if ( iDamageTaken > 0 )
	{
		m_flLastAttackedTime = gpGlobals->curtime;

		// check for achievement
		if ( bFullyShielded )
		{
			int iPrevLifetimeShieldedDamage = m_iLifetimeShieldedDamage;
			m_iLifetimeShieldedDamage += iDamageTaken;
			const int kMaxDamageForAchievement = tf_sentrygun_max_absorbed_damage_while_controlled_for_achievement.GetInt();
			if ( iPrevLifetimeShieldedDamage <= kMaxDamageForAchievement && m_iLifetimeShieldedDamage > kMaxDamageForAchievement )
			{
				CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
				if ( pOwner && pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_MANUAL_SENTRY_ABSORB_DMG );
				}
			}
		}
	}

	return iDamageTaken;
}

//-----------------------------------------------------------------------------
// Purpose: Called when this object is destroyed
//-----------------------------------------------------------------------------
void CObjectSentrygun::Killed( const CTakeDamageInfo &info )
{
	CTFPlayer *pTFKiller = ToTFPlayer( info.GetAttacker() );
	if ( pTFKiller && pTFKiller->IsPlayerClass( TF_CLASS_SOLDIER ) )
	{
		if ( pTFKiller->GetAbsOrigin().DistTo( GetAbsOrigin() ) > SENTRY_MAX_RANGE )
		{
			pTFKiller->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_DESTROY_SENTRY_OUT_OF_RANGE );
		}
		//If we are in the corridor map, then we check for the achievement for it.
		else if ( m_hEnemy && !( pTFKiller->GetFlags() & FL_ONGROUND ) )
		{
			CBaseEntity *pDamager = GetBuilder();
			
			if ( NULL == pDamager )
			{
				pDamager = this;
			}

			static const float DAMAGE_INTERVAL = 2.0f;
			if ( pTFKiller->m_AchievementData.IsDamagerInHistory( pDamager, DAMAGE_INTERVAL ) )
			{
				//Check the map.
				if ( 0 == Q_stricmp( "tra_sol_corridor", STRING( gpGlobals->mapname ) ) )
				{
#ifdef TF_SOLDIER_TRAINING_ACHIEVEMENTS
					//If the attacker was in the air when this sentry died, give him an achievement.
					pTFKiller->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_TRAINING_COR_SENTRY_FROM_AIR );
#endif // TF_SOLDIER_TRAINING_ACHIEVEMENTS
				}
			}
		}
	}

	// Tell our owner's shotgun the sentry died.
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner )
	{
		CTFShotgun_Revenge* pShotgun = dynamic_cast<CTFShotgun_Revenge*>( pOwner->Weapon_OwnsThisID( TF_WEAPON_SENTRY_REVENGE ) );
		if ( pShotgun )
		{
			pShotgun->SentryKilled( GetKills() * 2 + GetAssists() );
		}
	}

	// find nearby sentry hint
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		CTFBotHintSentrygun *sentryHint;
		for( sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( NULL, "bot_hint_sentrygun" ) );
			sentryHint;
			sentryHint = static_cast< CTFBotHintSentrygun * >( gEntList.FindEntityByClassname( sentryHint, "bot_hint_sentrygun" ) ) )
		{
			if ( sentryHint->IsEnabled() && sentryHint->InSameTeam( this ) )
			{
				Vector toMe = GetAbsOrigin() - sentryHint->GetAbsOrigin();
				float dist2 = toMe.LengthSqr();
				if ( dist2 < 1.0f )
				{
					sentryHint->OnSentryGunDestroyed( this );
					sentryHint->DecrementUseCount();
					break;
				}
			}
		}
	}

	// Engineers destroying their own sentry don't escape the buster.
	// Destroying disposable sentries doesn't reset the buster.
	if ( info.GetAttacker() != this && !IsDisposableBuilding() )
	{
		// Sentry Buster mission accomplished
		if ( pOwner )
		{
			pOwner->ResetAccumulatedSentryGunDamageDealt();
			pOwner->ResetAccumulatedSentryGunKillCount();
		}
	}

	// do normal handling
	BaseClass::Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::SetModel( const char *pModel )
{
	float flPoseParam0 = 0.0;
	float flPoseParam1 = 0.0;

	// Save pose parameters across model change
	if ( m_iPitchPoseParameter >= 0 )
	{
		flPoseParam0 = GetPoseParameter( m_iPitchPoseParameter );
	}

	if ( m_iYawPoseParameter >= 0 )
	{
		flPoseParam1 = GetPoseParameter( m_iYawPoseParameter );
	}

	BaseClass::SetModel( pModel );

	// Reset this after model change
	SetBuildingSize();
	SetSolid( SOLID_BBOX );

	// Restore pose parameters
	m_iPitchPoseParameter = LookupPoseParameter( "aim_pitch" );
	m_iYawPoseParameter = LookupPoseParameter( "aim_yaw" );

	SetPoseParameter( m_iPitchPoseParameter, flPoseParam0 );
	SetPoseParameter( m_iYawPoseParameter, flPoseParam1 );

	CreateBuildPoints();

	ReattachChildren();

	ResetSequenceInfo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::SetBuildingSize()
{
	// Mini's do NOT need to have their size set here, SetModelScale already handles scaling for hulls (change from MvM)
	UTIL_SetSize( this, SENTRYGUN_MINS, SENTRYGUN_MAXS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::MakeCarriedObject( CTFPlayer *pCarrier )
{
	BaseClass::MakeCarriedObject( pCarrier );

	m_iOldAmmoShells = m_iAmmoShells;
	m_iOldAmmoRockets = m_iAmmoRockets;

	m_nShieldLevel.Set( SHIELD_NONE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectSentrygun::MakeDisposableBuilding( CTFPlayer* pPlayer )
{
	// We don't have our main gun
	if ( !( pPlayer->GetNumObjects( OBJ_SENTRYGUN ) && pPlayer->CanBuild( OBJ_SENTRYGUN ) == CB_CAN_BUILD ) )
		return;

	// We're carrying our main gun
	if ( pPlayer->m_Shared.IsCarryingObject() && pPlayer->m_Shared.GetCarriedObject() && !pPlayer->m_Shared.GetCarriedObject()->IsDisposableBuilding() )
		return;

	if ( IsDisposableBuilding() )
		return;

	SetMaxHealth( SENTRYGUN_MINI_MAX_HEALTH );
	SetHealth( SENTRYGUN_MINI_MAX_HEALTH );

	SetModelScale( DISPOSABLE_SCALE );
	
	BaseClass::MakeDisposableBuilding( pPlayer );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::RemoveAllAmmo()
{
	m_iOldAmmoShells = m_iAmmoShells;
	m_iOldAmmoRockets = m_iAmmoRockets;

	m_iAmmoShells = 0;
	m_iAmmoRockets = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::EmitSentrySound( IRecipientFilter& filter, int iEntIndex, const char *soundname )
{
	EmitSound_t params;
	params.m_pSoundName = soundname;
	params.m_flSoundTime = 0;
	params.m_pflSoundDuration = 0;
	params.m_bWarnOnDirectWaveReference = true;
	
	if ( IsMiniBuilding() )
	{
		StopSound( soundname );
		params.m_nPitch = PITCH_HIGH;
		params.m_nFlags = SND_CHANGE_PITCH;
	}

	EmitSound( filter, entindex(), params );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::EmitSentrySound( const char* soundname )
{
	CPASAttenuationFilter filter( this, soundname );

	EmitSound_t params;
	params.m_pSoundName = soundname;
	params.m_flSoundTime = 0;
	params.m_pflSoundDuration = 0;
	params.m_bWarnOnDirectWaveReference = true;
	
	if ( IsMiniBuilding() || m_flFireRate != 1.f )
	{
		StopSound( soundname );
		params.m_nPitch = IsMiniBuilding() ? PITCH_HIGH : RemapValClamped( m_flFireRate, 1.0f, 0.5f, 100.f, 120.f );
		params.m_nFlags = SND_CHANGE_PITCH;
	}

	EmitSound( filter, entindex(), params );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CObjectSentrygun::GetAssistingTeammate( float maxAssistDuration ) const
{
	if ( maxAssistDuration > 0.0f && ( !m_lastTeammateWrenchHitTimer.HasStarted() || m_lastTeammateWrenchHitTimer.IsGreaterThen( maxAssistDuration ) ) )
		return NULL;

	return m_lastTeammateWrenchHit;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::SetAutoAimTarget( CTFPlayer* pPlayer )
{
	if ( !pPlayer )
		return;

	// No auto aim target if a dummy is found
	CBaseEntity *pTargetOld = m_hEnemy.Get();
	if ( pTargetOld )
	{
		CTFTargetDummy *pDummy = dynamic_cast<CTFTargetDummy*>( pTargetOld );
		if ( pDummy )
		{
			m_hAutoAimTarget = NULL;
			return;
		}
	}

	m_hAutoAimTarget = pPlayer;
	m_flAutoAimStartTime = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
void CObjectSentrygun::UpdateNavMeshCombatStatus( void )
{
	// mark region as 'in combat'
	if ( m_inCombatThrottleTimer.IsElapsed() )
	{
		// important to keep this at one second, so rate cvars make sense (units/sec)
		m_inCombatThrottleTimer.Start( 1.0f );

		UpdateLastKnownArea();

		// only search up/down StepHeight as a cheap substitute for line of sight
		CUtlVector< CNavArea * > nearbyAreaVector;
		CollectSurroundingAreas( &nearbyAreaVector, GetLastKnownArea(), tf_nav_in_combat_range.GetFloat(), StepHeight, StepHeight );

		for( int i=0; i<nearbyAreaVector.Count(); ++i )
		{
			CTFNavArea *area = static_cast< CTFNavArea * >( nearbyAreaVector[i] );

			// hacky - we want sentry gunfire to immediately heat the area since it is so dangerous
			area->OnCombat();
			area->OnCombat();
			area->OnCombat();
			area->OnCombat();
			area->OnCombat();
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------------
int CObjectSentrygun::GetUpgradeMetalRequired()
{
	int iMetal = BaseClass::GetUpgradeMetalRequired();
	int iSmallSentry = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwner(), iSmallSentry, build_small_sentries );
	if ( iSmallSentry )
	{
		iMetal *= 0.75f;
	}
	
	return iMetal;
}

//-------------------------------------------------------------------------------------------------------------------------------
int CObjectSentrygun::GetMaxHealthForCurrentLevel( void )
{
	int iHealth = BaseClass::GetMaxHealthForCurrentLevel();
	if ( IsScaledSentry() )
	{
		iHealth *= 0.66f;
	}
	return iHealth;
}
//-------------------------------------------------------------------------------------------------------------------------------
void CObjectSentrygun::MakeScaledBuilding( CTFPlayer *pPlayer )
{
	int iSmallSentry = 0;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwner(), iSmallSentry, build_small_sentries );
	if ( iSmallSentry )
	{
		m_flScaledSentry = iSmallSentry ? SMALL_SENTRY_SCALE : 1.0f;

		SetModelScale( m_flScaledSentry );

		int iHealth = GetMaxHealthForCurrentLevel();

		SetMaxHealth( iHealth );
		SetHealth( iHealth );
		SetBuildingSize();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_projectile_sentryrocket, CTFProjectile_SentryRocket );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )

BEGIN_NETWORK_TABLE( CTFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Creation
//-----------------------------------------------------------------------------
CTFProjectile_SentryRocket *CTFProjectile_SentryRocket::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_SentryRocket *pRocket = static_cast<CTFProjectile_SentryRocket*>( CTFBaseRocket::Create( NULL, "tf_projectile_sentryrocket", vecOrigin, vecAngles, pOwner ) );

	if ( pRocket )
	{
		pRocket->SetScorer( pScorer );
	}

	return pRocket;
}

CTFProjectile_SentryRocket::CTFProjectile_SentryRocket()
{
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_SentryRocket::Spawn()
{
	BaseClass::Spawn();

	SetModel( SENTRY_ROCKET_MODEL );

	UTIL_SetSize( this, vec3_origin, vec3_origin );

	ResetSequence( LookupSequence("idle") );
}

