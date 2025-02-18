//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:			The Medic's Medikit weapon
//					
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"
#include "tf_item.h"
#include "entity_capture_flag.h"

#if defined( CLIENT_DLL )
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "particles_simple.h"
#include "c_tf_player.h"
#include "soundenvelope.h"
#include "tf_hud_mediccallers.h"
#include "c_tf_playerresource.h"
#include "prediction.h"
#else
#include "ndebugoverlay.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#include "tf_obj.h"
#include "inetchannel.h"
#include "IEffects.h"
#include "baseprojectile.h"
#include "soundenvelope.h"
#include "effect_dispatch_data.h"
#include "func_respawnroom.h"
#endif

#include "tf_revive.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_shovel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


MedigunEffects_t g_MedigunEffects[MEDIGUN_NUM_CHARGE_TYPES] =
{
	{ TF_COND_INVULNERABLE,					TF_COND_INVULNERABLE_WEARINGOFF, "TFPlayer.InvulnerableOn",						"TFPlayer.InvulnerableOff" },	// MEDIGUN_CHARGE_INVULN = 0,
	{ TF_COND_CRITBOOSTED,					TF_COND_LAST,					 "TFPlayer.CritBoostOn",						"TFPlayer.CritBoostOff" },		// MEDIGUN_CHARGE_CRITICALBOOST,
	{ TF_COND_MEGAHEAL,						TF_COND_LAST,					 "TFPlayer.QuickFixInvulnerableOn",				"TFPlayer.MegaHealOff" },		// MEDIGUN_CHARGE_MEGAHEAL,
	{ TF_COND_MEDIGUN_UBER_BULLET_RESIST,	TF_COND_LAST,					 "WeaponMedigun_Vaccinator.InvulnerableOn",		"WeaponMedigun_Vaccinator.InvulnerableOff" },		// TF_COND_MEDIGUN_UBER_BULLET_RESIST,
	{ TF_COND_MEDIGUN_UBER_BLAST_RESIST,	TF_COND_LAST,					 "WeaponMedigun_Vaccinator.InvulnerableOn",		"WeaponMedigun_Vaccinator.InvulnerableOff" },		// TF_COND_MEDIGUN_UBER_BLAST_RESIST,
	{ TF_COND_MEDIGUN_UBER_FIRE_RESIST,		TF_COND_LAST,					 "WeaponMedigun_Vaccinator.InvulnerableOn",		"WeaponMedigun_Vaccinator.InvulnerableOff" },		// TF_COND_MEDIGUN_UBER_FIRE_RESIST,
};

struct MedigunResistConditions_t
{
	medigun_resist_types_t eResistType;
	ETFCond passiveCond;
	ETFCond uberCond;
};

MedigunResistConditions_t g_MedigunResistConditions[MEDIGUN_NUM_RESISTS] = 
{
	{ MEDIGUN_BULLET_RESIST,	TF_COND_MEDIGUN_SMALL_BULLET_RESIST,	TF_COND_MEDIGUN_UBER_BULLET_RESIST },
	{ MEDIGUN_BLAST_RESIST,		TF_COND_MEDIGUN_SMALL_BLAST_RESIST,		TF_COND_MEDIGUN_UBER_BLAST_RESIST },
	{ MEDIGUN_FIRE_RESIST,		TF_COND_MEDIGUN_SMALL_FIRE_RESIST,		TF_COND_MEDIGUN_UBER_FIRE_RESIST }
};

// Buff ranges
ConVar weapon_medigun_damage_modifier( "weapon_medigun_damage_modifier", "1.5", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Scales the damage a player does while being healed with the medigun." );
ConVar weapon_medigun_construction_rate( "weapon_medigun_construction_rate", "10", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Constructing object health healed per second by the medigun." );
ConVar weapon_medigun_charge_rate( "weapon_medigun_charge_rate", "40", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time healing it takes to fully charge the medigun." );
ConVar weapon_medigun_chargerelease_rate( "weapon_medigun_chargerelease_rate", "8", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time it takes the a full charge of the medigun to be released." );
ConVar weapon_medigun_resist_num_chunks( "weapon_medigun_resist_num_chunks", "4", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "How many uber bar chunks the vaccinator has." );
ConVar tf_vaccinator_uber_charge_rate_modifier( "tf_vaccinator_uber_charge_rate_modifier", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY , "Vaccinator uber charge rate." );

#if defined (CLIENT_DLL)
ConVar tf_medigun_autoheal( "tf_medigun_autoheal", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Setting this to 1 will cause the Medigun's primary attack to be a toggle instead of needing to be held down." );
#endif

#if !defined (CLIENT_DLL)
ConVar tf_medigun_lagcomp(  "tf_medigun_lagcomp", "1", FCVAR_DEVELOPMENTONLY );
#endif

ConVar weapon_vaccinator_resist_duration( "weapon_vaccinator_resist_duration", "3", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time it takes the a full charge of the vaccinator to be released." );

static const char *s_pszMedigunHealTargetThink = "MedigunHealTargetThink";

extern ConVar tf_invuln_time;

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RecvProxy_HealingTarget( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CWeaponMedigun *pMedigun = ((CWeaponMedigun*)(pStruct));
	if ( pMedigun != NULL )
	{
		pMedigun->ForceHealingTargetUpdate();
	}

	RecvProxy_IntToEHandle( pData, pStruct, pOut );
}
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_medigun, CWeaponMedigun );
PRECACHE_WEAPON_REGISTER( tf_weapon_medigun );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMedigun, DT_WeaponMedigun )

#ifdef GAME_DLL
void* SendProxy_SendActiveLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
void* SendProxy_SendNonLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
#endif

//-----------------------------------------------------------------------------
// Purpose: Only sent when a player's holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CWeaponMedigun, DT_LocalTFWeaponMedigunData )
#if defined( CLIENT_DLL )
RecvPropFloat( RECVINFO(m_flChargeLevel) ),
#else
SendPropFloat( SENDINFO(m_flChargeLevel), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Variables sent at low precision to non-holding observers.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CWeaponMedigun, DT_TFWeaponMedigunDataNonLocal )
#if defined( CLIENT_DLL )
RecvPropFloat( RECVINFO(m_flChargeLevel) ),
#else
SendPropFloat( SENDINFO(m_flChargeLevel), 12, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0, 100.0f ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Variables always sent
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE( CWeaponMedigun, DT_WeaponMedigun )
#if !defined( CLIENT_DLL )
//	SendPropFloat( SENDINFO(m_flChargeLevel), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropEHandle( SENDINFO( m_hHealingTarget ) ),
	SendPropBool( SENDINFO( m_bHealing ) ),
	SendPropBool( SENDINFO( m_bAttacking ) ),
	SendPropBool( SENDINFO( m_bChargeRelease ) ),
	SendPropBool( SENDINFO( m_bHolstered ) ),
	SendPropInt( SENDINFO( m_nChargeResistType ) ),
	SendPropEHandle( SENDINFO( m_hLastHealingTarget ) ),
	SendPropDataTable("LocalTFWeaponMedigunData", 0, &REFERENCE_SEND_TABLE(DT_LocalTFWeaponMedigunData), SendProxy_SendLocalWeaponDataTable ),
	SendPropDataTable("NonLocalTFWeaponMedigunData", 0, &REFERENCE_SEND_TABLE(DT_TFWeaponMedigunDataNonLocal), SendProxy_SendNonLocalWeaponDataTable ),
#else
//	RecvPropFloat( RECVINFO(m_flChargeLevel) ),
	RecvPropEHandle( RECVINFO( m_hHealingTarget ), RecvProxy_HealingTarget ),
	RecvPropBool( RECVINFO( m_bHealing ) ),
	RecvPropBool( RECVINFO( m_bAttacking ) ),
	RecvPropBool( RECVINFO( m_bChargeRelease ) ),
	RecvPropBool( RECVINFO( m_bHolstered ) ),
	RecvPropInt( RECVINFO( m_nChargeResistType ) ),
	RecvPropEHandle( RECVINFO( m_hLastHealingTarget ) ),
	RecvPropDataTable("LocalTFWeaponMedigunData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalTFWeaponMedigunData)),
	RecvPropDataTable("NonLocalTFWeaponMedigunData", 0, 0, &REFERENCE_RECV_TABLE(DT_TFWeaponMedigunDataNonLocal)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponMedigun  )

	DEFINE_PRED_FIELD( m_bHealing, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAttacking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bHolstered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hHealingTarget, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),

	DEFINE_FIELD( m_bCanChangeTarget, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flHealEffectLifetime, FIELD_FLOAT ),

	DEFINE_PRED_FIELD( m_flChargeLevel, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bChargeRelease, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

//	DEFINE_PRED_FIELD( m_bPlayingSound, FIELD_BOOLEAN ),
//	DEFINE_PRED_FIELD( m_bUpdateHealingTargets, FIELD_BOOLEAN ),

END_PREDICTION_DATA()
#endif

#define PARTICLE_PATH_VEL				140.0
#define NUM_PATH_PARTICLES_PER_SEC		300.0f
#define NUM_MEDIGUN_PATH_POINTS		8


extern ConVar tf_max_health_boost;

//-----------------------------------------------------------------------------
// Purpose: For HUD auto medic callers
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
ConVar hud_medicautocallers( "hud_medicautocallers", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_medicautocallersthreshold( "hud_medicautocallersthreshold", "75", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
ConVar hud_medichealtargetmarker ( "hud_medichealtargetmarker", "0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );
#endif

const char *g_pszMedigunHealSounds[] =
{
	"WeaponMedigun.HealingWorld",		// MEDIGUN_CHARGE_INVULN = 0,
	"WeaponMedigun.HealingWorld",		// MEDIGUN_CHARGE_CRITICALBOOST,
	"Weapon_Quick_Fix.Healing",			// MEDIGUN_CHARGE_MEGAHEAL,
	"WeaponMedigun_Vaccinator.Healing",	// MEDIGUN_CHARGE_BULLET_RESIST,
	"WeaponMedigun_Vaccinator.Healing",	// MEDIGUN_CHARGE_BLAST_RESIST,
	"WeaponMedigun_Vaccinator.Healing",	// MEDIGUN_CHARGE_FIRE_RESIST,
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszMedigunHealSounds ) == MEDIGUN_NUM_CHARGE_TYPES );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMedigun::CWeaponMedigun( void )
{
	WeaponReset();

	SetPredictionEligible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponMedigun::~CWeaponMedigun()
{
#ifdef CLIENT_DLL
	StopChargeEffect( true );

	if ( m_pChargedSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
	}

	if ( m_pDisruptSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pDisruptSound );
	}

	if ( m_pHealSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pHealSound );
	}

	if ( m_pDetachSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pDetachSound );
	}

	m_flAutoCallerCheckTime = 0.0f;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( m_bHealing && pOwner && pOwner->m_Shared.InState( TF_STATE_DYING ) )
	{
		m_bWasHealingBeforeDeath = true;
	}
	else
	{
		m_bWasHealingBeforeDeath = false;
	}

	m_flHealEffectLifetime = 0;

	m_bHealing = false;
	m_bAttacking = false;
	m_bChargeRelease = false;
	m_DetachedTargets.Purge();
	m_flEndResistCharge = 0.f;

	m_bCanChangeTarget = true;

	m_flNextBuzzTime = 0;
	m_flReleaseStartedAt = 0;

	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		// This is determined via an attribute in SetStoredChargeLevel()
		m_flChargeLevel = Min( (float)m_flChargeLevel, m_flChargeLevelToPreserve );
	}
	else
	{
		m_flChargeLevel = 0.f;
	}

	RemoveHealingTarget( true );

	m_bAttack2Down	= false;
	m_bAttack3Down	= false;
	m_bReloadDown	= false;

	m_nChargeResistType = 0;

#if defined( GAME_DLL )
	StopHealingOwner();
	m_hLastHealingTarget = NULL;
	RecalcEffectOnTarget( ToTFPlayer( GetOwnerEntity() ), true );
	m_nHealTargetClass = 0;
	m_nChargesReleased = 0;
#endif

#if defined( CLIENT_DLL )
	m_nOldChargeResistType = 0;
	m_bPlayingSound = false;
	m_bUpdateHealingTargets = false;
	m_bOldChargeRelease = false;

	UpdateEffects();
	StopChargeEffect( true );
	StopHealSound();

	m_pChargeEffectOwner = NULL;
	m_pChargeEffect = NULL;
	m_pChargedSound = NULL;
	m_pDisruptSound = NULL;
	m_flDenySecondary = 0.f;
	m_pHealSound = NULL;
	m_pDetachSound = NULL;
#endif

	HookAttributes();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::Precache()
{
	BaseClass::Precache();
	PrecacheModel( "models/weapons/c_models/c_proto_backpack/c_proto_backpack.mdl" );
	PrecacheScriptSound( "WeaponMedigun.NoTarget" );
	PrecacheScriptSound( "Weapon_Quick_Fix.Healing" );
	PrecacheScriptSound( "WeaponMedigun.Charged" );
	PrecacheParticleSystem( "medicgun_invulnstatus_fullcharge_blue" );
	PrecacheParticleSystem( "medicgun_invulnstatus_fullcharge_red" );
	PrecacheParticleSystem( "medicgun_beam_red_invun" );
	PrecacheParticleSystem( "medicgun_beam_red" );
	PrecacheParticleSystem( "medicgun_beam_red_targeted" );
	PrecacheParticleSystem( "medicgun_beam_blue_invun" );
	PrecacheParticleSystem( "medicgun_beam_blue" );
	PrecacheParticleSystem( "medicgun_beam_blue_targeted" );
	PrecacheParticleSystem( "vaccinator_red_buff1" );
	PrecacheParticleSystem( "vaccinator_red_buff2" );
	PrecacheParticleSystem( "vaccinator_red_buff3" );
	PrecacheParticleSystem( "vaccinator_blue_buff1" );
	PrecacheParticleSystem( "vaccinator_blue_buff2" );
	PrecacheParticleSystem( "vaccinator_blue_buff3" );
	PrecacheParticleSystem( "drain_effect" );
	PrecacheScriptSound( "WeaponMedigun_Vaccinator.Charged_tier_01");
	PrecacheScriptSound( "WeaponMedigun_Vaccinator.Charged_tier_02");
	PrecacheScriptSound( "WeaponMedigun_Vaccinator.Charged_tier_03");
	PrecacheScriptSound( "WeaponMedigun_Vaccinator.Charged_tier_04");
	PrecacheScriptSound( "WeaponMedigun.HealingDisrupt" );
	PrecacheScriptSound( "WeaponMedigun.HealingDetachHealer" );
	PrecacheScriptSound( "WeaponMedigun.HealingDetachTarget" );
	PrecacheScriptSound( "WeaponMedigun.HealingDetachWorld" );
	PrecacheScriptSound( "WeaponMedigun.HealingHealer" );
	PrecacheScriptSound( "WeaponMedigun.HealingTarget" );
	PrecacheScriptSound( "WeaponMedigun.HealingWorld" );
	// PrecacheParticleSystem( "medicgun_beam_machinery" );

	for( int i=0; i<ARRAYSIZE(g_MedigunEffects); ++i )
	{
		if( g_MedigunEffects[i].pszChargeOnSound[0] )
			PrecacheScriptSound( g_MedigunEffects[i].pszChargeOnSound );

		if( g_MedigunEffects[i].pszChargeOffSound[0] )
			PrecacheScriptSound( g_MedigunEffects[i].pszChargeOffSound );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bHolstered = false;

		m_bWasHealingBeforeDeath = false;

#ifdef GAME_DLL
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( m_bChargeRelease )
		{
			RecalcEffectOnTarget( pOwner );
		}

		if ( pOwner && pOwner->m_Shared.IsRageDraining() )
		{
			CreateMedigunShield();
		}

		// Resume healing self for Quick-Fix if we're still ubering and switch back to the Quick-Fix.
		if ( ( GetMedigunType() == MEDIGUN_QUICKFIX ) && m_bChargeRelease && !m_bHealingSelf )
		{
			StartHealingTarget( pOwner );
			m_bHealingSelf = true;
		}
#endif

#ifdef CLIENT_DLL
		ManageChargeEffect();
#endif

		m_flNextTargetCheckTime = gpGlobals->curtime;

		HookAttributes();

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	RemoveHealingTarget( true );
	m_bAttacking = false;
	m_bHolstered = true;

#ifdef GAME_DLL
	RecalcEffectOnTarget( ToTFPlayer( GetOwnerEntity() ), true );
	StopHealingOwner();
#endif

	RemoveMedigunShield();

#ifdef CLIENT_DLL
	UpdateEffects();
	ManageChargeEffect();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::UpdateOnRemove( void )
{
	RemoveHealingTarget( true );
	m_bAttacking = false;
	m_bChargeRelease = false;

#ifdef GAME_DLL
	RecalcEffectOnTarget( ToTFPlayer( GetOwnerEntity() ), true );
	StopHealingOwner();
#endif

#ifdef CLIENT_DLL
	if ( m_bPlayingSound )
	{
		m_bPlayingSound = false;
		StopHealSound();
	}

	UpdateEffects();
#endif

	RemoveMedigunShield();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetTargetRange( void )
{
	return (float)m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flRange;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetStickRange( void )
{
	return (GetTargetRange() * 1.2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetHealRate( void )
{
	float flHealRate = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwnerEntity(), flHealRate, mult_medigun_healrate );

	// This attribute represents a bucket of attributes.
	int iHealingMastery = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOwnerEntity(), iHealingMastery, healing_mastery );
	if ( iHealingMastery )
	{
		float flPerc = RemapValClamped( (float)iHealingMastery, 1.f, 4.f, 1.25f, 2.f );
		flHealRate *= flPerc;
	}

	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		CTFPlayer *pTFHealingTarget = ToTFPlayer( m_hHealingTarget );

		//medics or heal targets with powerups are too powerful. We reduce heal and charge gain rate to offset the advantage powerups give them
		//because of the nature of the Haste and Agility powerups, we don't add a heal or charge rate penalty if the Medic is carrying either of them 
		if ( pOwner && pTFHealingTarget )
		{
			bool bMedicIsPoweredUp = pOwner->m_Shared.GetCarryingRuneType() != RUNE_NONE && pOwner->m_Shared.GetCarryingRuneType() != RUNE_HASTE && pOwner->m_Shared.GetCarryingRuneType() != RUNE_AGILITY;
			if ( bMedicIsPoweredUp && pTFHealingTarget->m_Shared.GetCarryingRuneType() == RUNE_NONE ) //medic is powered up but target isn't
			{
				flHealRate *= 0.75f;
			}
			else if ( ( !bMedicIsPoweredUp && pTFHealingTarget->m_Shared.GetCarryingRuneType() != RUNE_NONE ) ) //target is powered up but medic isn't
			{
				flHealRate *= 0.50f;
			}
			else if ( bMedicIsPoweredUp && pTFHealingTarget->m_Shared.GetCarryingRuneType() != RUNE_NONE ) //both players are powered up
			{
				flHealRate *= 0.25f;
			}
			else
				return flHealRate;
		}
	}

	return flHealRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::HealingTarget( CBaseEntity *pTarget )
{
	if ( pTarget == m_hHealingTarget )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::AllowedToHealTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

	if ( !pTarget )
		return false;

	if ( pTarget->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );
		if ( !pTFPlayer )
			return false;

		if ( !pTFPlayer->IsAlive() )
			return false;

		// We cannot heal teammates who are using the Equalizer.
		CTFWeaponBase *pTFWeapon = pTFPlayer->GetActiveTFWeapon();
		int iWeaponBlocksHealing = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFWeapon, iWeaponBlocksHealing, weapon_blocks_healing );
		if ( iWeaponBlocksHealing == 1 )
		{
			return false;
		}

		bool bStealthed = pTFPlayer->m_Shared.IsStealthed() && !pOwner->m_Shared.IsStealthed(); // Allow stealthed medics to heal stealthed targets
		bool bDisguised = pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED );

		// We can heal teammates and enemies that are disguised as teammates
		if ( !bStealthed && 
			( pTFPlayer->InSameTeam( pOwner ) || 
			( bDisguised && pTFPlayer->m_Shared.GetDisguiseTeam() == pOwner->GetTeamNumber() ) ) )
		{
			return true;
		}
	}
	else
	{
		if ( !pTarget->InSameTeam( pOwner ) )
			return false;

		if ( pTarget->IsBaseObject() )
			return false;

		CTFReviveMarker *pReviveMarker = dynamic_cast< CTFReviveMarker* >( pTarget );
		if ( pReviveMarker )
		{
			m_hReviveMarker = pReviveMarker;	// Store last marker we touched
			return true;
		}
	}

	return false;
}

// Now make sure there isn't something other than team players in the way.
class CMedigunFilter : public CTraceFilterSimple
{
public:
	CMedigunFilter( CBaseEntity *pShooter ) : CTraceFilterSimple( pShooter, COLLISION_GROUP_WEAPON )
	{
		m_pShooter = pShooter;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		// If it hit an edict that isn't the target and is on our team, then the ray is blocked.
		CBaseEntity *pEnt = static_cast<CBaseEntity*>(pHandleEntity);

		// Ignore collisions with the shooter
		if ( pEnt == m_pShooter )
			return false;
		
		if ( pEnt->GetTeam() == m_pShooter->GetTeam() )
			return false;

		return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
	}

	CBaseEntity	*m_pShooter;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::MaintainTargetInSlot()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	CBaseEntity *pTarget = m_hHealingTarget;
	Assert( pTarget );

	// Make sure the guy didn't go out of range.
	bool bLostTarget = true;
	Vector vecSrc = pOwner->Weapon_ShootPosition( );
	Vector vecTargetPoint = pTarget->WorldSpaceCenter();
	Vector vecPoint;

	// If it's brush built, use absmins/absmaxs
	pTarget->CollisionProp()->CalcNearestPoint( vecSrc, &vecPoint );

	float flDistance = (vecPoint - vecSrc).Length();
	if ( flDistance < GetStickRange() )
	{
		if ( m_flNextTargetCheckTime > gpGlobals->curtime )
			return;

		m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;

		CheckAchievementsOnHealTarget();

		trace_t tr;
		CMedigunFilter drainFilter( pOwner );

		Vector vecAiming;
		pOwner->EyeVectors( &vecAiming );

		Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
		UTIL_TraceLine( vecSrc, vecEnd, (MASK_SHOT & ~CONTENTS_HITBOX), pOwner, DMG_GENERIC, &tr );

		// Still visible?
		if ( tr.m_pEnt == pTarget )
			return;

		UTIL_TraceLine( vecSrc, vecTargetPoint, MASK_SHOT, &drainFilter, &tr );

		// Still visible?
		if (( tr.fraction == 1.0f) || (tr.m_pEnt == pTarget))
			return;

		// If we failed, try the target's eye point as well
		UTIL_TraceLine( vecSrc, pTarget->EyePosition(), MASK_SHOT, &drainFilter, &tr );
		if (( tr.fraction == 1.0f) || (tr.m_pEnt == pTarget))
			return;
	}

	// We've lost this guy
	if ( bLostTarget )
	{
		RemoveHealingTarget();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::FindNewTargetForSlot()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	Vector vecSrc = pOwner->Weapon_ShootPosition( );
	if ( m_hHealingTarget )
	{
		RemoveHealingTarget();
	}

	// In Normal mode, we heal players under our crosshair
	Vector vecAiming;
	pOwner->EyeVectors( &vecAiming );

	// Find a player in range of this player, and make sure they're healable.
	Vector vecEnd = vecSrc + vecAiming * GetTargetRange();
	trace_t tr;

	UTIL_TraceLine( vecSrc, vecEnd, (MASK_SHOT & ~CONTENTS_HITBOX), pOwner, DMG_GENERIC, &tr );
	if ( tr.fraction != 1.0 && tr.m_pEnt )
	{
		if ( !HealingTarget( tr.m_pEnt ) && AllowedToHealTarget( tr.m_pEnt ) )
		{
#ifdef GAME_DLL
			pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STARTEDHEALING );
			if ( tr.m_pEnt->IsPlayer() )
			{
				CTFPlayer *pTarget = ToTFPlayer( tr.m_pEnt );
				pTarget->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STARTEDHEALING );
			}

			// Start the heal target thinking.
			SetContextThink( &CWeaponMedigun::HealTargetThink, gpGlobals->curtime, s_pszMedigunHealTargetThink );
#endif
			m_hHealingTarget.Set( tr.m_pEnt );
			m_flNextTargetCheckTime = gpGlobals->curtime + 1.0f;
		}			
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponMedigun::IsReleasingCharge( void ) const
{ 
	return (m_bChargeRelease && !m_bHolstered);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CWeaponMedigun::GetMedigunType( void ) const 
{ 
	int iMode = 0;
	CALL_ATTRIB_HOOK_INT( iMode, set_weapon_mode );
	return iMode;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::SetChargeLevelToPreserve( float flAmount )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	float flPreserveUber = 0.f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flPreserveUber, ubercharge_preserved_on_spawn_max );
	m_flChargeLevelToPreserve = Min( flAmount, flPreserveUber );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetMinChargeAmount( void ) const
{
	if( GetMedigunType() == MEDIGUN_RESIST )
	{
		return 1.f / weapon_medigun_resist_num_chunks.GetInt();
	}
	else
	{
		return 1.f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
medigun_charge_types CWeaponMedigun::GetChargeType( void ) const
{ 
	int iTmp = MEDIGUN_CHARGE_INVULN;
	CALL_ATTRIB_HOOK_INT( iTmp, set_charge_type );

	if( GetMedigunType() == MEDIGUN_RESIST )
	{
		// If this is a resist-medigun, then the charge type needs to be within the resist types
		Assert( iTmp >= MEDIGUN_CHARGE_BULLET_RESIST && iTmp <= MEDIGUN_CHARGE_FIRE_RESIST );
		Assert( m_nChargeResistType < MEDIGUN_NUM_RESISTS );

		iTmp += m_nChargeResistType;
	}

	return (medigun_charge_types)iTmp;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::CycleResistType()
{
	// Resist medigun only!
	if( GetMedigunType() != MEDIGUN_RESIST )
		return;

	if( IsReleasingCharge() )
		return;

#ifdef GAME_DLL
	// When cycling resist we have to remove the current resist, then add the new resist.
	CTFPlayer *pTFHealingTarget = ToTFPlayer( m_hHealingTarget );
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	ETFCond cond = g_MedigunResistConditions[ GetResistType() ].passiveCond;
	// Remove from out healing target
	if( pTFHealingTarget)
	{
		CBaseEntity* pProvider = pTFHealingTarget->m_Shared.GetConditionProvider( cond );

		// Remove from our healing target if we're the provider
		if( pProvider == pOwner || pProvider == NULL )
		{
			pTFHealingTarget->m_Shared.RemoveCond( cond );
		}
	}
	// Remove from ourselves
	if( pOwner )
	{
		// Remove from ourselves if we're the provider
		CBaseEntity* pProvider = pOwner->m_Shared.GetConditionProvider( cond );
		if( pProvider == pOwner || pProvider == NULL )
		{
			pOwner->m_Shared.RemoveCond( cond );
		}
	}
	
#endif

	m_nChargeResistType += 1;
	m_nChargeResistType = m_nChargeResistType % MEDIGUN_NUM_RESISTS;


#ifdef GAME_DLL
	CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );

	if( pTFOwner )
	{
		RecalcEffectOnTarget( pTFOwner );
	}


	if( pTFHealingTarget )
	{
		// Now add the new resist
		RecalcEffectOnTarget( pTFHealingTarget );
		pTFHealingTarget->m_Shared.AddCond( g_MedigunResistConditions[ GetResistType() ].passiveCond, PERMANENT_CONDITION, pTFOwner );
		pTFOwner->m_Shared.AddCond( g_MedigunResistConditions[ GetResistType() ].passiveCond, PERMANENT_CONDITION, pTFOwner );
	}
#else
	// Updates our particles
	ForceHealingTargetUpdate();

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
medigun_resist_types_t CWeaponMedigun::GetResistType() const
{
	Assert( GetMedigunType() == MEDIGUN_RESIST );

	int nCurrentActiveResist = ( GetChargeType() - MEDIGUN_CHARGE_BULLET_RESIST );
	Assert( nCurrentActiveResist >= 0 && nCurrentActiveResist < MEDIGUN_NUM_RESISTS );
	nCurrentActiveResist = nCurrentActiveResist % MEDIGUN_NUM_RESISTS;

	return medigun_resist_types_t(nCurrentActiveResist);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponMedigun::IsAllowedToTargetBuildings( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponMedigun::IsAttachedToBuilding( void )
{
	if ( !m_hHealingTarget )
		return false;

	return m_hHealingTarget->IsBaseObject();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::HealTargetThink( void )
{	
	// Verify that we still have a valid heal target.
	CBaseEntity *pTarget = m_hHealingTarget;
	if ( !pTarget || !pTarget->IsAlive() )
	{
		SetContextThink( NULL, 0, s_pszMedigunHealTargetThink );
		return;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	float flTime = gpGlobals->curtime - pOwner->GetTimeBase();
	if ( flTime > 5.0f || !AllowedToHealTarget(pTarget) )
	{
		RemoveHealingTarget( true );
	}

	// Make sure our heal target hasn't changed classes while being healed
	CTFPlayer *pTFTarget = ToTFPlayer( pTarget );
	if ( pTFTarget )
	{
		int nPrevClass = m_nHealTargetClass;
		m_nHealTargetClass = pTFTarget->GetPlayerClass()->GetClassIndex();
		if ( m_nHealTargetClass != nPrevClass )
		{
			pOwner->TeamFortress_SetSpeed();

			// Do this so the medic has to re-attach, which causes a re-evaluation of any attributes that may have changed on the target
			if ( m_hHealingTarget.Get() == m_hLastHealingTarget.Get() )
			{
				Lower();
			}
		}
	}

	if ( !pTarget->IsPlayer() )
	{

		CTFReviveMarker *pReviveMarker = dynamic_cast< CTFReviveMarker* >( pTarget );
		if ( pReviveMarker )
		{
			CTFPlayer *pDeadPlayer = pReviveMarker->GetOwner();
			if ( pDeadPlayer )
			{
				pReviveMarker->SetReviver( pOwner );

				// Instantly revive players when deploying uber
				float flHealRate = GetHealRate();
				float flReviveRate = m_bChargeRelease ? flHealRate / 2.f : flHealRate / 8.f;
				pReviveMarker->AddMarkerHealth( flReviveRate );

				// Set observer target to reviver so they know they're being revived
				if ( pDeadPlayer->GetObserverMode() > OBS_MODE_FREEZECAM )
				{
					if ( pReviveMarker->GetReviver() && pDeadPlayer->GetObserverTarget() != pReviveMarker->GetReviver() )
					{
						pDeadPlayer->SetObserverTarget( pReviveMarker->GetReviver() );
					}
				}

				if ( !pReviveMarker->HasOwnerBeenPrompted() )
				{
					// This will give them a messagebox that has a Cancel button
					pReviveMarker->PromptOwner();
				}
			}
		}
	}

	SetNextThink( gpGlobals->curtime + 0.2f, s_pszMedigunHealTargetThink );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::StartHealingTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pTFTarget = ToTFPlayer( pTarget );
	if ( !pTFTarget )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	float flOverhealBonus = GetOverHealBonus( pTFTarget );
	float flOverhealDecayMult = GetOverHealDecayMult( pTFTarget );
	pTFTarget->m_Shared.Heal( pOwner, GetHealRate(), flOverhealBonus, flOverhealDecayMult );

	// Add on the small passive resist when we attach onto a target
	if( GetMedigunType() == MEDIGUN_RESIST )
	{
		pTFTarget->m_Shared.AddCond( g_MedigunResistConditions[ GetResistType() ].passiveCond, PERMANENT_CONDITION, pOwner );
		pOwner->m_Shared.AddCond( g_MedigunResistConditions[ GetResistType() ].passiveCond, PERMANENT_CONDITION, pOwner );
	}
}

//-----------------------------------------------------------------------------
// Purpose: QuickFix uber heals the target and medic
//-----------------------------------------------------------------------------
void CWeaponMedigun::StopHealingOwner( void )
{
	if ( !m_bHealingSelf ) 
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;
		
	pOwner->m_Shared.StopHealing( pOwner );
	m_bHealingSelf = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::AddCharge( float flPercentage )
{
	m_flChargeLevel = MIN( m_flChargeLevel + flPercentage, 1.0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::SubtractCharge( float flPercentage )
{
	float flSubtractAmount = Max( flPercentage, 0.0f );
	SubtractChargeAndUpdateDeployState( flSubtractAmount, true );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::RecalcEffectOnTarget( CTFPlayer *pPlayer, bool bInstantRemove )
{
	if ( !pPlayer )
		return;

	pPlayer->m_Shared.RecalculateChargeEffects( bInstantRemove );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::UberchargeChunkDeployed()
{
	m_nChargesReleased++;
	if( m_nChargesReleased % weapon_medigun_resist_num_chunks.GetInt() == 0 )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pOwner )
			return;

		CTF_GameStats.Event_PlayerInvulnerable( pOwner );
		EconEntity_OnOwnerKillEaterEvent( this, pOwner, ToTFPlayer( m_hHealingTarget ), kKillEaterEvent_UberActivated );
	}
}
#endif // GAME_DLL

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CWeaponMedigun::GetHealSound( void ) const
{
	int iMedigunType = GetMedigunType();
	const char *pszRetVal = g_pszMedigunHealSounds[iMedigunType];
	if ( ( iMedigunType == MEDIGUN_CHARGE_INVULN ) || ( iMedigunType == MEDIGUN_CHARGE_CRITICALBOOST ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
			if ( pFiringPlayer && m_hHealingTarget.Get() )
			{
				if ( pLocalPlayer == pFiringPlayer )
				{
					pszRetVal = "WeaponMedigun.HealingHealer";
				}
				else if ( pLocalPlayer == m_hHealingTarget.Get() )
				{
					pszRetVal = "WeaponMedigun.HealingTarget";
				}
			}
		}
	}

	return pszRetVal;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CWeaponMedigun::GetDetachSound( void ) const
{
	int iMedigunType = GetMedigunType();
	const char *pszRetVal = "WeaponMedigun.HealingDetachWorld";
	if ( ( iMedigunType == MEDIGUN_CHARGE_INVULN ) || ( iMedigunType == MEDIGUN_CHARGE_CRITICALBOOST ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
			if ( pFiringPlayer && ( m_hHealingTarget.Get() || m_hLastHealingTarget.Get() ) )
			{
				if ( pLocalPlayer == pFiringPlayer )
				{
					pszRetVal = "WeaponMedigun.HealingDetachHealer";
				}
				else if ( ( pLocalPlayer == m_hHealingTarget.Get() ) || ( pLocalPlayer == m_hLastHealingTarget.Get() ) )
				{
					pszRetVal = "WeaponMedigun.HealingDetachTarget";
				}
			}
		}
	}

	return pszRetVal;
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::CreateMedigunShield( void )
{
#ifdef GAME_DLL
	if ( m_hMedigunShield )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	{
		// check if we can activate the shield
		if ( ( pOwner->m_Shared.GetRageMeter() < 100.f ) && !pOwner->m_Shared.IsRageDraining() )
			return;
	}

	pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_HEAL_SHIELD );
	m_hMedigunShield = CTFMedigunShield::Create( pOwner );
	if ( m_hMedigunShield )
	{
		pOwner->m_Shared.StartRageDrain();
		
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedigun::RemoveMedigunShield( void )
{
#ifdef GAME_DLL
	if ( m_hMedigunShield )
	{
		m_hMedigunShield->RemoveShield();
		m_hMedigunShield.Set( NULL );
	}
#endif // GAME_DLL
}




//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to a healable target
//-----------------------------------------------------------------------------
bool CWeaponMedigun::FindAndHealTargets( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return false;

#ifdef GAME_DLL
	if ( !pOwner->IsBot() )
	{
		INetChannelInfo *pNetChanInfo = engine->GetPlayerNetInfo( pOwner->entindex() );
		if ( !pNetChanInfo || pNetChanInfo->IsTimingOut() )
			return false;
	}
#endif // GAME_DLL

	bool bFound = false;

	// Maintaining beam to existing target?
	CBaseEntity *pTarget = m_hHealingTarget;
	if ( pTarget && pTarget->IsAlive() )
	{
		MaintainTargetInSlot();
	}
	else
	{	
		FindNewTargetForSlot();
	}

	CBaseEntity *pNewTarget = m_hHealingTarget;
	if ( pNewTarget && pNewTarget->IsAlive() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pNewTarget );

#ifdef GAME_DLL
		// HACK: For now, just deal with players
		if ( pTFPlayer )
		{
			if ( pTarget != pNewTarget )
			{
				StartHealingTarget( pNewTarget );
			}

			RecalcEffectOnTarget( pTFPlayer );
		}
#endif
	
		bFound = true;

		// Charge up our power if we're not releasing it, and our target
		// isn't receiving any benefit from our healing.
		if ( !m_bChargeRelease )
		{
			float flChargeRate = weapon_medigun_charge_rate.GetFloat();
			float flChargeAmount = gpGlobals->frametime / flChargeRate;

			if ( pTFPlayer && weapon_medigun_charge_rate.GetFloat() )
			{
#ifdef GAME_DLL
				int iBoostMax = floor( pTFPlayer->m_Shared.GetMaxBuffedHealth() * 0.95);
				float flChargeModifier = 1.f;

				bool bTargetOverhealBlocked = false;
				float flMod = 1.f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFPlayer, flMod, mult_patient_overheal_penalty );
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFPlayer->GetActiveTFWeapon(), flMod, mult_patient_overheal_penalty_active );
				if ( flMod <= 0.f )
				{
					if ( pTFPlayer->GetHealth() >= pTFPlayer->GetMaxHealth() )
					{
						bTargetOverhealBlocked = true;
					}
				}

				// Reduced charge for healing fully healed guys
				if ( ( bTargetOverhealBlocked || ( pNewTarget->GetHealth() >= iBoostMax ) ) && ( TFGameRules() && !(TFGameRules()->InSetup() && TFGameRules()->GetActiveRoundTimer() ) ) )
				{
					flChargeModifier *= 0.5f;
				}

				int iTotalHealers = pTFPlayer->m_Shared.GetNumHealers();
				if ( iTotalHealers > 1 )
				{
					flChargeModifier /= (float)iTotalHealers;
				}

				// The resist medigun has a uber charge rate
				flChargeAmount *= flChargeModifier;

				if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
				{
					bool bMedicIsPoweredUp = pOwner->m_Shared.GetCarryingRuneType() != RUNE_NONE && pOwner->m_Shared.GetCarryingRuneType() != RUNE_HASTE && pOwner->m_Shared.GetCarryingRuneType() != RUNE_AGILITY;
					if ( bMedicIsPoweredUp && pTFPlayer->m_Shared.GetCarryingRuneType() == RUNE_NONE ) //medic is powered up but target isn't
					{
						flChargeAmount *= 0.75f;
					}
					else if ( !bMedicIsPoweredUp && pTFPlayer->m_Shared.GetCarryingRuneType() != RUNE_NONE ) //target is powered up but medic isn't
					{
						flChargeAmount *= 0.50f;
					}
					else if ( bMedicIsPoweredUp && pTFPlayer->m_Shared.GetCarryingRuneType() != RUNE_NONE ) //both players are powered up
					{
						flChargeAmount *= 0.25f;
					}
					else
						flChargeAmount *= 1.0f;
				}

				if ( pNewTarget->GetHealth() >= pNewTarget->GetMaxHealth() && ( TFGameRules() && !(TFGameRules()->InSetup() && TFGameRules()->GetActiveRoundTimer() ) ) )
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_overheal_uberchargerate );
				}

				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flChargeAmount, mult_medigun_uberchargerate );


				// Apply any bonus our target gives us.
				if ( pTarget )
				{
					bool bInRespawnRoom = 
						PointInRespawnRoom( pTarget, WorldSpaceCenter() ) ||
						PointInRespawnRoom( pOwner, WorldSpaceCenter() );

					if ( !bInRespawnRoom )
					{
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTarget, flChargeAmount, mult_uberchargerate_for_healer );
					}
				}
				if ( TFGameRules() )
				{
					if ( TFGameRules()->IsQuickBuildTime() )
					{
						flChargeAmount *= 4.f;
					}
					else if ( TFGameRules()->InSetup() && TFGameRules()->GetActiveRoundTimer() )
					{
						flChargeAmount *= 3.f;
					}
				}
#endif

				float flNewLevel = MIN( m_flChargeLevel + flChargeAmount, 1.0 );

				float flMinChargeAmount = GetMinChargeAmount();

				if ( flNewLevel >= flMinChargeAmount && m_flChargeLevel < flMinChargeAmount )
				{
#ifdef GAME_DLL
					pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEREADY );
					pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEREADY );
#else
					// send a message that we've got charge
					// if you change this from being a client-side only event, you have to 
					// fix ACHIEVEMENT_TF_MEDIC_KILL_WHILE_CHARGED to check the medic userid.
					IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_chargeready" );
					if ( event )
					{
						gameeventmanager->FireEventClientSide( event );
					}
#endif
				}

#ifdef CLIENT_DLL
					if ( GetMedigunType() == MEDIGUN_RESIST )
					{
						// Play a sound when we tick over to a new charge level
						int nChargeLevel = int(floor(flNewLevel/flMinChargeAmount));
						float flNextChargeLevelAmount = nChargeLevel * flMinChargeAmount;
						if( flNewLevel >= flNextChargeLevelAmount && m_flChargeLevel < flNextChargeLevelAmount )
						{
							const char* pzsSoundName =  CFmtStr( "WeaponMedigun_Vaccinator.Charged_tier_0%d", nChargeLevel );

							if (  nChargeLevel == 1 )
							{
								if ( m_pChargedSound != NULL )
								{
									CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
									m_pChargedSound = NULL;
								}

								CLocalPlayerFilter filter;

								CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

								m_pChargedSound = controller.SoundCreate( filter, entindex(), pzsSoundName );
								controller.Play( m_pChargedSound, 1.0, 100 );
							}
							else
							{
								pOwner->EmitSound( pzsSoundName );
							}
						}
					}
#endif
					SetChargeLevel( flNewLevel );
			}
			else if ( IsAttachedToBuilding() )
			{
				m_flChargeLevel = MIN( m_flChargeLevel + flChargeAmount, 1.0 );
			}
		}
	}

	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	DrainCharge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::DrainCharge( void )
{
	// If we're in charge release mode, drain our charge
	if ( m_bChargeRelease )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pOwner )
			return;

		int flUberTime = weapon_medigun_chargerelease_rate.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flUberTime, add_uber_time );

		float flChargeAmount = gpGlobals->frametime / flUberTime;
		float flExtraPlayerCost = flChargeAmount * 0.5;

		// Drain faster the more targets we're applying to. Extra targets count for 50% drain to still reward juggling somewhat.
		for ( int i = m_DetachedTargets.Count()-1; i >= 0; i-- )
		{
			if ( m_DetachedTargets[i].hTarget == NULL || m_DetachedTargets[i].hTarget.Get() == m_hHealingTarget.Get() || 
				!m_DetachedTargets[i].hTarget->IsAlive() || m_DetachedTargets[i].flTime < (gpGlobals->curtime - tf_invuln_time.GetFloat()) )
			{
				m_DetachedTargets.Remove(i);
			}
			else
			{
				flChargeAmount += flExtraPlayerCost;
			}
		}

		SubtractChargeAndUpdateDeployState( flChargeAmount, false );
	}
}


void CWeaponMedigun::SubtractChargeAndUpdateDeployState( float flSubtractAmount, bool bForceDrain )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	float flNewCharge = Max( m_flChargeLevel - flSubtractAmount, 0.0f );

	if( GetMedigunType() == MEDIGUN_RESIST )
	{
		if( flNewCharge <= m_flEndResistCharge )
		{
			// If the player is holding down ATTACK2 and they have a bar of Uber left,
			// let them burn straight into the next bar
			if( (m_flEndResistCharge > 0) && m_bAttack2Down )
			{
				float flChunkSize = GetMinChargeAmount();
				int nCurrentChunk = floor(m_flChargeLevel / flChunkSize);
				m_flEndResistCharge = flChunkSize * Max( 0, (nCurrentChunk - 1) );
#ifdef GAME_DLL
				UberchargeChunkDeployed();
#endif
			}
			else
			{

				// Make sure we don't cross over too far if this is a natural drain
				if( !bForceDrain )
				{
					flNewCharge = m_flEndResistCharge;
				}
				m_flEndResistCharge = 0.f;
				// Stop deploying
				m_bChargeRelease = false;
				m_flReleaseStartedAt = 0;
				m_DetachedTargets.Purge();

#ifdef GAME_DLL
				pOwner->ClearPunchVictims();
				RecalcEffectOnTarget( pOwner );
#endif
			}
		}
	}

	m_flChargeLevel = flNewCharge;

	if ( !m_flChargeLevel )
	{
		m_bChargeRelease = false;
		m_flReleaseStartedAt = 0;
		m_DetachedTargets.Purge();

#ifdef GAME_DLL
		pOwner->ClearPunchVictims();
		RecalcEffectOnTarget( pOwner );
		StopHealingOwner(); // QuickFix uber heals the target and medic
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: Overloaded to handle the hold-down healing
//-----------------------------------------------------------------------------
void CWeaponMedigun::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	// If we're lowered, we're not allowed to fire
	if ( CanAttack() == false )
	{
		RemoveHealingTarget( true );
		return;
	}

#if !defined( CLIENT_DLL )
	if ( AppliesModifier() )
	{
		m_DamageModifier.SetModifier( weapon_medigun_damage_modifier.GetFloat() );
	}
#endif

	// Try to start healing
	m_bAttacking = false;
	if ( pOwner->GetMedigunAutoHeal() )
	{
		if ( pOwner->m_nButtons & IN_ATTACK )
		{
			if ( m_bCanChangeTarget )
			{
				RemoveHealingTarget();
#if defined( CLIENT_DLL )
				if (prediction->IsFirstTimePredicted() ) {
					m_bPlayingSound = false;
					StopHealSound( true, true, false );
				}
#endif
				// can't change again until we release the attack button
				m_bCanChangeTarget = false;
			}
		}
		else
		{
			m_bCanChangeTarget = true;
		}

		if ( m_bHealing && ( m_iState != WEAPON_IS_ACTIVE || pOwner->IsTaunting() ) )
		{
			RemoveHealingTarget();
		}
		else if ( m_bHealing || ( pOwner->m_nButtons & IN_ATTACK ) )
		{
			PrimaryAttack();
			m_bAttacking = true;
		}
	}
	else
	{
		if ( /*m_bChargeRelease || */ pOwner->m_nButtons & IN_ATTACK )
		{
			PrimaryAttack();
			m_bAttacking = true;
		}
 		else if ( m_bHealing )
 		{
 			// Detach from the player if they release the attack button.
 			RemoveHealingTarget();
 		}
	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		SecondaryAttack();
	}
	else
	{
		m_bAttack2Down = false;
	}

	if( (pOwner->m_nButtons & IN_ATTACK3) && !m_bAttack3Down )
	{
		CreateMedigunShield();
		m_bAttack3Down = true;
	}
	else if( !(pOwner->m_nButtons & IN_ATTACK3) && m_bAttack3Down )
	{
		m_bAttack3Down = false;
	}

	if ( pOwner->m_nButtons & IN_RELOAD && !m_bReloadDown )
	{
#ifdef GAME_DLL
		CycleResistType();
#endif
		m_bReloadDown = true;
	}
	else if ( !(pOwner->m_nButtons & IN_RELOAD) && m_bReloadDown )
	{
		m_bReloadDown = false;
	}

	WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponMedigun::Lower( void )
{
	// Stop healing if we are
	if ( m_bHealing )
	{
		RemoveHealingTarget( true );
		m_bAttacking = false;

#ifdef CLIENT_DLL
		UpdateEffects();
#endif
	}

	return BaseClass::Lower();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::RemoveHealingTarget( bool bStopHealingSelf )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;


	// If this guy is already in our detached target list, update the time. Otherwise, add him.
	if ( m_bChargeRelease )
	{
		int i = 0;
		for ( i = 0; i < m_DetachedTargets.Count(); i++ )
		{
			if ( m_DetachedTargets[i].hTarget == m_hHealingTarget )
			{
				m_DetachedTargets[i].flTime = gpGlobals->curtime;
				break;
			}
		}
		if ( i == m_DetachedTargets.Count() )
		{
			int iIdx = m_DetachedTargets.AddToTail();
			m_DetachedTargets[iIdx].hTarget = m_hHealingTarget;
			m_DetachedTargets[iIdx].flTime = gpGlobals->curtime;
		}
	}

#ifdef GAME_DLL
	int nMedigunType = GetMedigunType();

	if ( m_hHealingTarget )
	{
		// HACK: For now, just deal with players
		if ( m_hHealingTarget->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
			CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
			if ( pTFPlayer && pOwner )
			{
				pTFPlayer->m_Shared.StopHealing( pOwner );

				pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_STOPPEDHEALING, pTFPlayer->IsAlive() ? "healtarget:alive" : "healtarget:dead" );
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_STOPPEDHEALING );

				// Remove our passive resist
				if( nMedigunType == MEDIGUN_RESIST )
				{
					ETFCond cond = g_MedigunResistConditions[ GetResistType() ].passiveCond;
					CBaseEntity* pProvider = pTFPlayer->m_Shared.GetConditionProvider( cond );

					// Remove from our healing target if we're the provider
					if( pProvider == pOwner || pProvider == NULL )
					{
						pTFPlayer->m_Shared.RemoveCond( cond );
					}

					// Remove from ourselves if we're the provider
					pProvider = pOwner->m_Shared.GetConditionProvider( cond );
					if( pProvider == pOwner || pProvider == NULL )
					{
						pOwner->m_Shared.RemoveCond( cond );
					}
				}
			}
		}
	}

	// Stop thinking - we no longer have a heal target.
	SetContextThink( NULL, 0, s_pszMedigunHealTargetThink );
#endif

	m_hLastHealingTarget.Set( m_hHealingTarget );
	m_hHealingTarget.Set( NULL );

#ifdef GAME_DLL
	// See if we have The QuickFix, which adjusts our move speed based on heal target
	pOwner->TeamFortress_SetSpeed();

#endif

	// Stop the welding animation
	if ( m_bHealing )
	{
		SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
		pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
	}

#ifndef CLIENT_DLL
	m_DamageModifier.RemoveModifier();
#endif
	m_bHealing = false;
}


//-----------------------------------------------------------------------------
// Purpose: Attempt to heal any player within range of the medikit
//-----------------------------------------------------------------------------
void CWeaponMedigun::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;


	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	/*
	// Start boosting ourself if we're not
	if ( m_bChargeRelease && !m_bHealingSelf )
	{
		pOwner->m_Shared.Heal( pOwner, GetHealRate() * 2 );
		m_bHealingSelf = true;
	}
	*/
#endif

#if !defined (CLIENT_DLL)
	if ( tf_medigun_lagcomp.GetBool() )
		lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );
#endif

	if ( FindAndHealTargets() )
	{
		// Start the animation
		if ( !m_bHealing )
		{
#ifdef GAME_DLL
			pOwner->SpeakWeaponFire();
#endif

			SendWeaponAnim( ACT_MP_ATTACK_STAND_PREFIRE );
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );
		}

		m_bHealing = true;
	}
	else
	{
		RemoveHealingTarget();
	}
	
#if !defined (CLIENT_DLL)
	if ( tf_medigun_lagcomp.GetBool() )
		lagcompensation->FinishLagCompensation( pOwner );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Burn charge level to generate invulnerability
//-----------------------------------------------------------------------------
void CWeaponMedigun::SecondaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	CTFPlayer *pTFPlayerPatient = NULL;
	if ( m_hHealingTarget && m_hHealingTarget->IsPlayer() )
	{
		pTFPlayerPatient = ToTFPlayer( m_hHealingTarget );
	}

	// STAGING_MEDIC
	// Resist gun early outs if the patient and medic both have the condition (or medic with no patient has condition)
	if ( GetMedigunType() == MEDIGUN_RESIST )
	{
		ETFCond uberCond = g_MedigunResistConditions[GetResistType()].uberCond;
		if ( pOwner->m_Shared.InCond( uberCond ) ) 
		{
			if ( !pTFPlayerPatient || pTFPlayerPatient->m_Shared.InCond( uberCond ) )
			{
				return;
			}
		}
	}

	m_bAttack2Down = true;

	// If using standard-uber-model-medigun, ensure they have a full charge and are not already in charge release mode
	bool bDenyUse = GetMedigunType() != MEDIGUN_RESIST && (m_flChargeLevel < 1.0);
	// If using the resist-medigun, they can shoot sooner
	float flChunkSize = GetMinChargeAmount();
	bDenyUse |= GetMedigunType() == MEDIGUN_RESIST && m_flChargeLevel < flChunkSize;

	if ( bDenyUse || m_bChargeRelease )
	{
#ifdef CLIENT_DLL
		// Deny, flash
		if ( !m_bChargeRelease && gpGlobals->curtime >= m_flDenySecondary )
		{
			m_flDenySecondary = gpGlobals->curtime + 0.5f;
			pOwner->EmitSound( "Player.DenyWeaponSelection" );
		}
#endif
		return;
	}

	if ( !pOwner->m_Shared.CanRecieveMedigunChargeEffect( GetChargeType() ) )
	{
		if ( pOwner->m_afButtonPressed & IN_ATTACK2 
#ifdef CLIENT_DLL
			&& prediction->IsFirstTimePredicted()
#endif
			)
		{
#ifdef GAME_DLL
			CSingleUserRecipientFilter filter( pOwner );
			TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_NO_INVULN_WITH_FLAG );
#else
			pOwner->EmitSound( "Player.DenyWeaponSelection" );
#endif
		}
		return;
	}


	// Toggle super charge state
	m_bChargeRelease = true;
	m_flReleaseStartedAt = gpGlobals->curtime;

#ifdef GAME_DLL
	if( GetMedigunType() == MEDIGUN_RESIST )
	{
		// We dont want to give the user a point every time they deploy an uber with the resist medigun.
		// Instead we give them a point for every 4 deploys
		UberchargeChunkDeployed();

		int nCurrentChunk = floor( m_flChargeLevel / flChunkSize );
		Assert( nCurrentChunk >= 1 );

		CPVSFilter filter( pOwner->WorldSpaceCenter() );
		pOwner->EmitSound( filter, pOwner->entindex(), CFmtStr( "WeaponMedigun_Vaccinator.Charged_tier_0%d", nCurrentChunk ) );
		pOwner->EmitSound( filter, pOwner->entindex(), g_MedigunEffects[MEDIGUN_CHARGE_BULLET_RESIST].pszChargeOnSound );	
	}
	else
	{
		// Award assist point
		CTF_GameStats.Event_PlayerInvulnerable( pOwner );
		// Award strange assist score
		EconEntity_OnOwnerKillEaterEvent( this, pOwner, ToTFPlayer( m_hHealingTarget ), kKillEaterEvent_UberActivated );
	}
	
	// STAGING_MEDIC
	if ( GetMedigunType() != MEDIGUN_RESIST )
	{
		RecalcEffectOnTarget( pOwner );
	}
	pOwner->SpeakConceptIfAllowed( MP_CONCEPT_MEDIC_CHARGEDEPLOYED );

	if ( pTFPlayerPatient )
	{
		// STAGING_MEDIC
		if ( GetMedigunType() != MEDIGUN_RESIST )
		{
			RecalcEffectOnTarget( pTFPlayerPatient );
		}

		pTFPlayerPatient->SpeakConceptIfAllowed( MP_CONCEPT_HEALTARGET_CHARGEDEPLOYED );
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "player_chargedeployed" );
	if ( event )
	{
		event->SetInt( "userid", pOwner->GetUserID() );
		if ( m_hHealingTarget && m_hHealingTarget->IsPlayer() )
		{
			event->SetInt( "targetid", ToTFPlayer(m_hHealingTarget)->GetUserID() );
		}
		gameeventmanager->FireEvent( event );
	}

	// Check for achievements
	// Simultaneous uber charge with teammates.
	CTeam *pTeam = pOwner->GetTeam();
	if ( pTeam )
	{
		CUtlVector<CTFPlayer*> aChargingMedics;
		aChargingMedics.AddToTail( pOwner );
		for ( int i = 0; i < pTeam->GetNumPlayers(); i++ )
		{
			CTFPlayer *pTeamPlayer = ToTFPlayer( pTeam->GetPlayer(i) );
			if ( pTeamPlayer && pTeamPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pTeamPlayer != pOwner )
			{
				CWeaponMedigun *pWeapon = dynamic_cast <CWeaponMedigun*>( pTeamPlayer->GetActiveWeapon() );
				if ( pWeapon && pWeapon->IsReleasingCharge() )
				{
					aChargingMedics.AddToTail( pTeamPlayer );
				}
			}
		}

		if ( aChargingMedics.Count() >= 3 )
		{
			// Give the achievement to all the Medics
			for ( int i = 0; i < aChargingMedics.Count(); i++ )
			{
				if ( aChargingMedics[i] )
				{
					aChargingMedics[i]->AwardAchievement( ACHIEVEMENT_TF_MEDIC_SIMUL_CHARGE );
				}
			}
		}
	}

	// reset this count
	pOwner->HandleAchievement_Medic_AssistHeavy( NULL );

	// If using the QuickFix, heal the medic
	if ( GetMedigunType() == MEDIGUN_QUICKFIX )
	{
		if ( IsReleasingCharge() && !m_bHealingSelf )
		{
			StartHealingTarget( pOwner );
			m_bHealingSelf = true;
		}
	}
#endif // GAME_DLL

	// STAGING_MEDIC
	if ( GetMedigunType() == MEDIGUN_RESIST )
	{
		// Remove charge immediately and just give target and yourself the conditions
		m_bChargeRelease = false;
#ifdef GAME_DLL
		float flResistDuration = weapon_vaccinator_resist_duration.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, flResistDuration, add_uber_time );
		pOwner->m_Shared.AddCond( g_MedigunResistConditions[GetResistType()].uberCond, flResistDuration, pOwner );
		m_flChargeLevel -= flChunkSize;
		if ( pTFPlayerPatient )
		{
			pTFPlayerPatient->m_Shared.AddCond( g_MedigunResistConditions[GetResistType()].uberCond, flResistDuration, pOwner );
		}
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			CBaseEntity *pTarget = m_hHealingTarget;
			CTFReviveMarker *pReviveMarker = dynamic_cast<CTFReviveMarker*>( pTarget );
			if ( pReviveMarker )
			{
				CTFPlayer *pDeadPlayer = pReviveMarker->GetOwner();
				if ( pDeadPlayer )
				{
					pReviveMarker->SetReviver( pOwner );
					// fill almost to max, give a small time period so patient has time to see notifications from regular revive code
					pReviveMarker->AddMarkerHealth( pReviveMarker->GetMaxHealth() * 0.9f ); 
				}
			}
		}

#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Idle tests to see if we're facing a valid target for the medikit
//			If so, move into the "heal-able" animation. 
//			Otherwise, move into the "not-heal-able" animation.
//-----------------------------------------------------------------------------
void CWeaponMedigun::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		// Loop the welding animation
		if ( m_bHealing )
		{
			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
			return;
		}

		return BaseClass::WeaponIdle();
	}
}

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::StopHealSound( bool bStopHealingSound, bool bStopNoTargetSound, bool bStopDetachSound )
{
	if ( bStopHealingSound && m_pHealSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pHealSound );
		m_pHealSound = NULL;

		if ( !bStopDetachSound )
		{
			CLocalPlayerFilter filter;
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			C_BaseEntity *pHealingTargetEnt = m_hHealingTarget;
			if ( !pHealingTargetEnt )
			{
				pHealingTargetEnt = m_hLastHealingTarget;
			}
			int iIndex = entindex();
			if ( ( GetMedigunType() == MEDIGUN_CHARGE_INVULN ) || ( GetMedigunType() == MEDIGUN_CHARGE_CRITICALBOOST ) )
			{
				if ( pHealingTargetEnt && pHealingTargetEnt->IsPlayer() && ( pHealingTargetEnt == CBasePlayer::GetLocalPlayer() ) )
				{
					iIndex = pHealingTargetEnt->entindex();
				}
			}
			m_pDetachSound = controller.SoundCreate( filter, iIndex, GetDetachSound() );
			controller.Play( m_pDetachSound, 1.f, 100.f );
		}
	}

	if ( bStopDetachSound && m_pDetachSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pDetachSound );
		m_pDetachSound = NULL;
	}

	if ( bStopNoTargetSound )
	{
		StopSound( "WeaponMedigun.NoTarget" );
	}

	if ( m_pDisruptSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pDisruptSound );
		m_pDisruptSound = NULL;
	}
}

void CWeaponMedigun::StopChargeEffect( bool bImmediately )
{
	// Either these should both be NULL or neither NULL
	Assert( ( m_pChargeEffect != NULL && m_pChargeEffectOwner != NULL ) || ( m_pChargeEffect == NULL && m_pChargeEffectOwner == NULL ) );

	if ( m_pChargeEffect != NULL && m_pChargeEffectOwner != NULL )
	{
		if( bImmediately )
		{
			m_pChargeEffectOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_pChargeEffect );
		}
		else
		{
			m_pChargeEffectOwner->ParticleProp()->StopEmission( m_pChargeEffect );
		}
		m_pChargeEffect = NULL;
		m_pChargeEffectOwner = NULL;
	}

	if ( m_pChargedSound != NULL )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pChargedSound );
		m_pChargedSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ManageChargeEffect( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pEffectOwner = this;

	if ( pLocalPlayer == NULL )
		return;

	if ( pLocalPlayer == GetTFPlayerOwner() )
	{
		pEffectOwner = pLocalPlayer->GetRenderedWeaponModel();
		if ( !pEffectOwner )
		{
			return;
		}
	}

	bool bOwnerTaunting = false;

	if ( GetTFPlayerOwner() && GetTFPlayerOwner()->m_Shared.InCond( TF_COND_TAUNTING ) == true )
	{
		bOwnerTaunting = true;
	}

	float flMinChargeToDeploy = GetMinChargeAmount();

	if ( GetTFPlayerOwner() && bOwnerTaunting == false && m_bHolstered == false && ( m_flChargeLevel >= flMinChargeToDeploy || m_bChargeRelease == true ) )
	{
		// Did we switch from 1st to 3rd or 3rd to 1st?  Taunting does this.
		if( pEffectOwner != m_pChargeEffectOwner )
		{
			// Stop the current effect so we can make a new one
			StopChargeEffect( m_bHolstered );
		}

		if ( m_pChargeEffect == NULL )
		{
			const char *pszEffectName = NULL;

			switch( GetTFPlayerOwner()->GetTeamNumber() )
			{
			case TF_TEAM_BLUE:
				pszEffectName = "medicgun_invulnstatus_fullcharge_blue";
				break;
			case TF_TEAM_RED:
				pszEffectName = "medicgun_invulnstatus_fullcharge_red";
				break;
			default:
				pszEffectName = "";
				break;
			}

			m_pChargeEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
			m_pChargeEffectOwner = pEffectOwner;
		}

		if ( m_pChargedSound == NULL )
		{
			CLocalPlayerFilter filter;

			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

			m_pChargedSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun.Charged" );
			controller.Play( m_pChargedSound, 1.0, 100 );
		}
	}
	else
	{
		StopChargeEffect( m_bHolstered );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CWeaponMedigun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bUpdateHealingTargets )
	{
		UpdateEffects();
		m_bUpdateHealingTargets = false;
	}

	if ( m_nOldChargeResistType != m_nChargeResistType )
	{
		m_nOldChargeResistType = m_nChargeResistType;
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( GetOwner() == pLocalPlayer && pLocalPlayer )
		{
			// Sound effect
			pLocalPlayer->EmitSound( "WeaponMedigun_Vaccinator.Toggle" );	
		}
	}

	if ( m_bHealing )
	{
		CTFPlayer *pTarget = ToTFPlayer( m_hHealingTarget );
		if ( !m_pDisruptSound && pTarget && pTarget->m_Shared.InCond( TF_COND_HEALING_DEBUFF ) )
		{
			CLocalPlayerFilter filter;
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			m_pDisruptSound = controller.SoundCreate( filter, entindex(), "WeaponMedigun.HealingDisrupt" );
			controller.Play( m_pDisruptSound, 1.f, 100.f );
		}
		else if ( m_pDisruptSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pDisruptSound );
			m_pDisruptSound = NULL;
		}
	}
	else
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );
		m_bPlayingSound = false;
		StopHealSound( true, false, false );

		// Are they holding the attack button but not healing anyone? Give feedback.
		if ( IsActiveByLocalPlayer() && GetOwner() && GetOwner()->IsAlive() && m_bAttacking && GetOwner() == C_BasePlayer::GetLocalPlayer() && CanAttack() == true )
		{
			if ( gpGlobals->curtime >= m_flNextBuzzTime )
			{
				CLocalPlayerFilter filter;
				EmitSound( filter, entindex(), "WeaponMedigun.NoTarget" );
				m_flNextBuzzTime = gpGlobals->curtime + 0.5f;	// only buzz every so often.
			}
		}
		else
		{
			StopHealSound( false, true, false );	// Stop the "no target" sound.
		}
	}

	// Think?
	if ( m_bHealing || IsCarriedByLocalPlayer() )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );
	}

	ManageChargeEffect();

	// Find teammates that need healing
	if ( IsCarriedByLocalPlayer() )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer || !pLocalPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			return;
		}

		if ( pLocalPlayer == GetOwner() && hud_medicautocallers.GetBool() )
		{
			UpdateMedicAutoCallers();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::ClientThink()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		return;
	}

	// Don't show it while the player is dead. Ideally, we'd respond to m_bHealing in OnDataChanged,
	// but it stops sending the weapon when it's holstered, and it gets holstered when the player dies.
	CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pFiringPlayer || pFiringPlayer->IsPlayerDead() || pFiringPlayer->IsDormant() )
	{
		ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_NEVER );
		m_bPlayingSound = false;
		StopHealSound();
		return;
	}

	// If the local player is the guy getting healed, let him know 
	// who's healing him, and their charge level.
	if( m_hHealingTarget != NULL )
	{
		if ( pLocalPlayer == m_hHealingTarget )
		{
			pLocalPlayer->SetHealer( pFiringPlayer, m_flChargeLevel );
		}

		// Setup whether we were last healed by the local player or by someone else (used by replay system)
		// since GetHealer() gets cleared out every frame before player_death events get fired.  See tf_replay.cpp.
		C_BaseEntity *pHealingTargetEnt = m_hHealingTarget;
		if ( pHealingTargetEnt && pHealingTargetEnt->IsPlayer() )
		{
			C_TFPlayer *pHealingTargetPlayer = ToTFPlayer( pHealingTargetEnt );
			pHealingTargetPlayer->SetWasHealedByLocalPlayer( pFiringPlayer == pLocalPlayer );
		}

		if ( !m_bPlayingSound )
		{
			m_bPlayingSound = true;
			CLocalPlayerFilter filter;
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			int iIndex = entindex();
			if ( ( GetMedigunType() == MEDIGUN_CHARGE_INVULN ) || ( GetMedigunType() == MEDIGUN_CHARGE_CRITICALBOOST ) )
			{
				if ( pHealingTargetEnt && pHealingTargetEnt->IsPlayer() && ( pHealingTargetEnt == CBasePlayer::GetLocalPlayer() ) )
				{
					iIndex = pHealingTargetEnt->entindex();
				}
			}

			StopHealSound( false, false, true );
			m_pHealSound = controller.SoundCreate( filter, iIndex, GetHealSound() );
			controller.Play( m_pHealSound, 1.f, 100.f );
		}
	}

	if ( m_bOldChargeRelease != m_bChargeRelease )
	{
		m_bOldChargeRelease = m_bChargeRelease;
		ForceHealingTargetUpdate();
	}

	// If the rendered weapon has changed, we need to update our particles
	if ( m_hHealingTargetEffect.pOwner && pFiringPlayer->GetRenderedWeaponModel() != m_hHealingTargetEffect.pOwner )
	{
		ForceHealingTargetUpdate();
	}

	if ( pFiringPlayer->m_Shared.IsEnteringOrExitingFullyInvisible() )
	{
		UpdateEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::UpdateEffects( void )
{
	CTFPlayer *pFiringPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pFiringPlayer )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseEntity *pEffectOwner = this;
	if ( pLocalPlayer == pFiringPlayer )
	{
		pEffectOwner = pLocalPlayer->GetRenderedWeaponModel();
	}

	// If we're still healing and our owner changed, then we did something
	// like changed 
	bool bImmediate = pEffectOwner != m_hHealingTargetEffect.pOwner && m_bHealing;

	// Remove all the effects
	if ( m_hHealingTargetEffect.pOwner )
	{
		if ( m_hHealingTargetEffect.pEffect )
		{
			bImmediate ? m_hHealingTargetEffect.pOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_hHealingTargetEffect.pEffect )
					   : m_hHealingTargetEffect.pOwner->ParticleProp()->StopEmission( m_hHealingTargetEffect.pEffect );
		}
		if ( m_hHealingTargetEffect.pCustomEffect )
		{
			bImmediate ? m_hHealingTargetEffect.pOwner->ParticleProp()->StopEmissionAndDestroyImmediately( m_hHealingTargetEffect.pCustomEffect )
					   : m_hHealingTargetEffect.pOwner->ParticleProp()->StopEmission( m_hHealingTargetEffect.pCustomEffect );
		}
	}
	else
	{
		if ( m_hHealingTargetEffect.pEffect )
		{
			m_hHealingTargetEffect.pEffect->StopEmission();
		}
		if ( m_hHealingTargetEffect.pCustomEffect )
		{
			m_hHealingTargetEffect.pCustomEffect->StopEmission();
		}
	}
	m_hHealingTargetEffect.pOwner			= NULL;
	m_hHealingTargetEffect.pTarget			= NULL;
	m_hHealingTargetEffect.pEffect			= NULL;
	m_hHealingTargetEffect.pCustomEffect	= NULL;

	// Don't add targets if the medic is dead
	if ( !pEffectOwner || pFiringPlayer->IsPlayerDead() || !pFiringPlayer->IsPlayerClass( TF_CLASS_MEDIC ) || pFiringPlayer->m_Shared.IsFullyInvisible() )
		return;

	// Add our targets
	// Loops through the healing targets, and make sure we have an effect for each of them

	if ( m_hHealingTarget )
	{
		if ( m_hHealingTargetEffect.pTarget == m_hHealingTarget )
			return;

		bool bReviveMarker = m_hReviveMarker && m_hReviveMarker == m_hHealingTarget;	// Hack to avoid another dynamic_cast here
		bool bHealTargetMarker = hud_medichealtargetmarker.GetBool() && !bReviveMarker;

		const char *pszEffectName;
		if ( IsAttachedToBuilding() )
		{
			pszEffectName = "medicgun_beam_machinery";
		}
		else if ( pFiringPlayer->GetTeamNumber() == TF_TEAM_RED )
		{
			if ( m_bChargeRelease )
			{
				pszEffectName = "medicgun_beam_red_invun";
			}
			else
			{
				if ( bHealTargetMarker && pFiringPlayer == pLocalPlayer )
				{
					pszEffectName = "medicgun_beam_red_targeted";
				}
				else
				{
					pszEffectName = "medicgun_beam_red";
				}
			}
		}
		else
		{
			if ( m_bChargeRelease )
			{
				pszEffectName = "medicgun_beam_blue_invun";
			}
			else
			{
				if ( bHealTargetMarker && pFiringPlayer == pLocalPlayer )
				{
					pszEffectName = "medicgun_beam_blue_targeted";
				}
				else
				{
					pszEffectName = "medicgun_beam_blue";
				}
			}
		}

		// Attach differently if targeting a revive marker
		float flVecHeightOffset = bReviveMarker ? 0.f : 50.f;
		ParticleAttachment_t attachType = bReviveMarker ? PATTACH_POINT_FOLLOW : PATTACH_ABSORIGIN_FOLLOW;
		const char *pszAttachName = bReviveMarker ? "healbeam" : NULL;

		CNewParticleEffect *pEffect = pEffectOwner->ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
		pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTarget, attachType, pszAttachName, Vector(0.f,0.f,flVecHeightOffset) );

		m_hHealingTargetEffect.pTarget = m_hHealingTarget;
		m_hHealingTargetEffect.pEffect = pEffect;
		m_hHealingTargetEffect.pOwner  = pEffectOwner;

		// See if we have a custom particle effect that wants to add to the beam
		CEconItemView *pItem = m_AttributeManager.GetItem();
		int iSystems = pItem->GetStaticData()->GetNumAttachedParticles( GetTeamNumber() );
		for ( int i = 0; i < iSystems; i++ )
		{
			attachedparticlesystem_t *pSystem = pItem->GetStaticData()->GetAttachedParticleData( GetTeamNumber(),i );
			if ( pSystem->iCustomType == 1 )
			{
				pEffect = pEffectOwner->ParticleProp()->Create( pSystem->pszSystemName, PATTACH_POINT_FOLLOW, "muzzle" );
				pEffectOwner->ParticleProp()->AddControlPoint( pEffect, 1, m_hHealingTarget, attachType, pszAttachName, Vector(0.f,0.f,flVecHeightOffset) );
				m_hHealingTargetEffect.pCustomEffect = pEffect;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look for teammates that need healing
//-----------------------------------------------------------------------------
void CWeaponMedigun::UpdateMedicAutoCallers( void )
{
	// Find teammates that need healing
	if ( gpGlobals->curtime > m_flAutoCallerCheckTime )
	{
		if ( !g_TF_PR )
		{
			return;
		}

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		for( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
		{
			C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( playerIndex ) );
			if ( pPlayer )
			{
				// Don't do this for the local player
				if ( pPlayer == pLocalPlayer )
					continue;

				if ( ( pPlayer->GetTeamNumber() == GetLocalPlayerTeam() ) ||
					 ( pPlayer->GetPlayerClass() && ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY ) && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( pPlayer->m_Shared.GetDisguiseTeam() == GetLocalPlayerTeam() ) ) )
				{
					if ( m_hHealingTarget != NULL )
					{
						// Don't do this for players the medic is healing
						if ( pPlayer == m_hHealingTarget )
							continue;
					}

					if ( pPlayer->IsAlive() )
					{
						int iHealth = float( pPlayer->GetHealth() ) / float( pPlayer->GetMaxHealth() ) * 100;
						int iHealthThreshold = hud_medicautocallersthreshold.GetInt();

						// If it's a healthy teammate....
						if ( iHealth > iHealthThreshold )
						{
							// Make sure we don't have them in our list if previously hurt
							if ( m_iAutoCallers.Find( playerIndex ) != m_iAutoCallers.InvalidIndex() )
							{
								m_iAutoCallers.FindAndRemove( playerIndex );
								continue;
							}
						}

						// If it's a hurt teammate....
						if ( iHealth <= iHealthThreshold )
						{

							// Make sure we're not already tracking this
							if ( m_iAutoCallers.Find( playerIndex ) != m_iAutoCallers.InvalidIndex() )
								continue;

							// Distance check
							float flDistSq = pPlayer->GetAbsOrigin().DistToSqr( pLocalPlayer->GetAbsOrigin() );
							if ( flDistSq >= 1000000 )
							{
								continue;
							}

							// Now add auto-caller
							pPlayer->CreateSaveMeEffect( CALLER_TYPE_AUTO );

							// And track the player so we don't re-add them
							m_iAutoCallers.AddToTail( playerIndex );
						}
					}
				}
			}
		}

		// Throttle this check
		m_flAutoCallerCheckTime = gpGlobals->curtime + 0.25f;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMedigun::CheckAchievementsOnHealTarget( void )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( m_hHealingTarget );
	if ( !pTFPlayer )
		return;

#ifdef GAME_DLL

	// Check for "target under fire" achievement
	if ( pTFPlayer->m_AchievementData.CountDamagersWithinTime(3.0) >= 4 )
	{
		if ( GetTFPlayerOwner() )
		{
			GetTFPlayerOwner()->AwardAchievement( ACHIEVEMENT_TF_MEDIC_HEAL_UNDER_FIRE );
		}
	}

	// Check for "Engineer repairing sentrygun" achievement
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		// Has Engineer worked on his sentrygun recently?
		CBaseObject	*pSentry = pTFPlayer->GetObjectOfType( OBJ_SENTRYGUN );
		if ( pSentry && pTFPlayer->m_AchievementData.IsTargetInHistory( pSentry, 4.0 ) )
		{
			if ( pSentry->m_AchievementData.CountDamagersWithinTime(3.0) > 0 )
			{
				CTFPlayer *pOwner = GetTFPlayerOwner();
				if ( pOwner )
				{
					// give to medic
					pOwner->AwardAchievement( ACHIEVEMENT_TF_MEDIC_HEAL_ENGINEER );

					// give to the engineer!
					pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_REPAIR_SENTRY_W_MEDIC );
				}
			}
		}
	}
#else

	// check for ACHIEVEMENT_TF_MEDIC_HEAL_CALLERS
	if ( pTFPlayer->m_flSaveMeExpireTime > gpGlobals->curtime )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healedmediccall" );
		if ( event )
		{
			event->SetInt( "userid", pTFPlayer->GetUserID() );
			gameeventmanager->FireEventClientSide( event );
		}
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: Our owner has become stunned.
//-----------------------------------------------------------------------------
void CWeaponMedigun::OnControlStunned( void )
{
	BaseClass::OnControlStunned();

	// Interrupt auto healing.
	RemoveHealingTarget( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponMedigun::EffectMeterShouldFlash( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	if ( pPlayer && ( pPlayer->m_Shared.GetRageMeter() >= 100.0f || pPlayer->m_Shared.IsRageDraining() ) )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: UI Progress
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0.f;

	return pPlayer->m_Shared.GetRageMeter() / 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetOverHealBonus( CTFPlayer *pTFTarget )
{
	// Handle bonuses as additive, penalties as percentage...
	float flOverhealBonus = tf_max_health_boost.GetFloat() - 1.0f;
	float flMod = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flMod, mult_medigun_overheal_amount );
	// Anything on the patient?
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFTarget, flMod, mult_patient_overheal_penalty );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFTarget->GetActiveTFWeapon(), flMod, mult_patient_overheal_penalty_active );
	if ( flMod >= 1.0f )
	{
		flOverhealBonus += flMod;
	}
	else if ( flMod < 1.0f && flOverhealBonus > 0.0f )
	{
		flOverhealBonus *= flMod;
		flOverhealBonus += 1.0f;
	}

	// Safety net
	if ( flOverhealBonus < 1.0f )
	{
		flOverhealBonus = 1.0f;
	}

	// Upgrades?
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
	{
		flOverhealBonus = Max( flOverhealBonus, flOverhealBonus + ( m_flOverHealExpert / 4 ) );
	}

	//Powered up heal targets don't get overheal
	if ( pTFTarget && pTFTarget->m_Shared.GetCarryingRuneType() != RUNE_NONE )
	{
		flOverhealBonus = 1.0f;
	}
	
	return flOverhealBonus;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CWeaponMedigun::GetOverHealDecayMult( CTFPlayer *pTFTarget )
{
	float flOverhealDecayMult = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flOverhealDecayMult, mult_medigun_overheal_decay );
	flOverhealDecayMult = Max( flOverhealDecayMult, flOverhealDecayMult + ( m_flOverHealExpert / 2 ) );
	return flOverhealDecayMult;
}

//-----------------------------------------------------------------------------
// Purpose: The future is now
//-----------------------------------------------------------------------------
void CWeaponMedigun::HookAttributes( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	m_flOverHealExpert = 0.f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOwner, m_flOverHealExpert, overheal_expert );
}


//-----------------------------------------------------------------------------
// entity_medigun_shield
//-----------------------------------------------------------------------------
#define MEDIGUNSHIELD_THINK_CONTEXT		"CTFMedigunShield_ShieldThink"

LINK_ENTITY_TO_CLASS( entity_medigun_shield, CTFMedigunShield );
PRECACHE_WEAPON_REGISTER( entity_medigun_shield );

// Network
IMPLEMENT_NETWORKCLASS_ALIASED( TFMedigunShield, DT_TFMedigunShield )

BEGIN_NETWORK_TABLE( CTFMedigunShield, DT_TFMedigunShield )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFMedigunShield )
END_PREDICTION_DATA()

// Data
BEGIN_DATADESC( CTFMedigunShield )
#ifdef GAME_DLL
DEFINE_FUNCTION( ShieldTouch ),
DEFINE_THINKFUNC( ShieldThink ),
#endif // GAME_DLL
END_DATADESC()

#ifdef GAME_DLL
static const float PROJECTILE_SHIELD_ALPHA_BASE = 150.f;
static const float PROJECTILE_SHIELD_ENERGY_REFILL_PULSE = 10.f;
static const float PROJECTILE_SHIELD_ENERGY_MAX = 1000.f;
#endif // GAME_DLL

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CTFMedigunShield::CTFMedigunShield()
{
	m_nBlinkCount = 0;
#ifdef GAME_DLL
	m_pTouchLoop = NULL;


#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CTFMedigunShield::~CTFMedigunShield()
{
#ifdef GAME_DLL
	if ( m_pTouchLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pTouchLoop );
		m_pTouchLoop = NULL;
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMedigunShield::Spawn()
{
	BaseClass::Spawn();

	SetModel( "models/props_mvm/mvm_player_shield.mdl" );
	SetSolid( SOLID_VPHYSICS );
	SetSolidFlags( FSOLID_TRIGGER );
	SetCollisionGroup( TFCOLLISION_GROUP_COMBATOBJECT );
	SetBlocksLOS( false );
	AddEffects( EF_NOSHADOW );

	m_takedamage = DAMAGE_EVENTS_ONLY;

#ifdef GAME_DLL
	m_flShieldEnergyLevel = PROJECTILE_SHIELD_ENERGY_MAX;

	SetTouch( &CTFMedigunShield::ShieldTouch );
	SetContextThink( &CTFMedigunShield::ShieldThink, gpGlobals->curtime, MEDIGUNSHIELD_THINK_CONTEXT );
#else
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMedigunShield::Precache()
{
	PrecacheScriptSound( "WeaponMedi_Shield.Deploy" );
	PrecacheScriptSound( "WeaponMedi_Shield.Protection" );
	PrecacheScriptSound( "WeaponMedi_Shield.Retract" );
	PrecacheScriptSound( "WeaponMedi_Shield.Burn" );
	PrecacheScriptSound( "WeaponMedi_Shield.Burn_lp" );

	PrecacheModel( "models/props_mvm/mvm_player_shield.mdl" );
	PrecacheModel( "models/props_mvm/mvm_player_shield2.mdl" );

	BaseClass::Precache();
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMedigunShield::ClientThink()
{
	UpdateShieldPosition();
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMedigunShield::UpdateShieldPosition( void )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( !pOwner )
		return;

	// Positioning logic
	Vector vecForward;
	AngleVectors( pOwner->EyeAngles(), &vecForward );
	vecForward.z = 0.f;
	Vector vecShieldOrigin = pOwner->GetAbsOrigin() + vecForward * 145.f;
	SetAbsOrigin( vecShieldOrigin );
	SetAbsAngles( QAngle( 0.f, pOwner->EyeAngles().y, 0.f ) );	// Ignore pitch
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMedigunShield *CTFMedigunShield::Create( CTFPlayer *pOwner )
{
	if ( pOwner )
	{
		CTFMedigunShield *pShield = static_cast< CTFMedigunShield* >( CBaseEntity::Create( "entity_medigun_shield", pOwner->GetAbsOrigin() + Vector( 0, 0, 60 ), pOwner->GetAbsAngles() ) );
		if ( pShield )
		{
			pShield->SetOwnerEntity( pOwner );
			pShield->ChangeTeam( pOwner->GetTeamNumber() );

			int nShieldLevel = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, nShieldLevel, generate_rage_on_heal );
			if ( nShieldLevel > 1 )
			{
				pShield->SetModel( "models/props_mvm/mvm_player_shield2.mdl" );
			}

			pShield->m_nSkin = pShield->GetTeamNumber() == TF_TEAM_RED ? 0 : 1;

			CPVSFilter filter( pOwner->WorldSpaceCenter() );
			pOwner->EmitSound( filter, pOwner->entindex(), "WeaponMedi_Shield.Deploy" );

			return pShield;
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
// bool CTFMedigunShield::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
// {
// 	switch( GetTeamNumber() )
// 	{
// 	case TF_TEAM_RED:
// 		if ( ( mask & CONTENTS_REDTEAM ) )
// 			return false;
// 		break;
// 
// 	case TF_TEAM_BLUE:
// 		if ( ( mask & CONTENTS_BLUETEAM ) )
// 			return false;
// 		break;
// 	}
// 
// 	vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
// 
// 	UTIL_ClearTrace( trace );
// 
// 	physcollision->TraceBox( ray, pCollide->solids[0], GetAbsOrigin(), GetAbsAngles(), &trace );
// 
// 	if ( trace.fraction >= 1 )
// 		return false;
// 
// 	// return owner as trace hit
// 	trace.m_pEnt = this;
// 	return true;
// }

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMedigunShield::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PROJECTILE || 
		 collisionGroup == TFCOLLISION_GROUP_ROCKETS || 
		 collisionGroup == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( ( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;

		case TF_TEAM_BLUE:
			if ( ( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;
		}
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFMedigunShield::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	CBaseEntity *pOwner = GetOwnerEntity();
	if ( !pOwner )
		return;

	if ( !InSameTeam( pOther ) )
	{
		CPVSFilter filter( WorldSpaceCenter() );
		pOwner->EmitSound( filter, entindex(), "WeaponMedi_Shield.Burn" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMedigunShield::ShieldTouch( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	if ( !InSameTeam( pOther ) )
	{
		// Drip pan for unknown projectiles
		if ( !pOther->IsDeflectable() && pOther->GetCollisionGroup() == COLLISION_GROUP_PROJECTILE )
		{
			UTIL_Remove( pOther );
			return;
		}

		CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pTFOwner )
			return;

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && ( pTFOwner->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
			return;

		int nShieldLevel = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFOwner, nShieldLevel, generate_rage_on_heal );

		// Damage it
		float flDamage = ( nShieldLevel > 1 ) ? 2.f : 1.f;
		CTakeDamageInfo info;
		info.SetAttacker( pTFOwner );
		info.SetInflictor( this ); 
		info.SetWeapon( pTFOwner->GetActiveTFWeapon() );
		info.SetDamage( flDamage );
		info.SetDamageType( DMG_ENERGYBEAM );
		info.SetDamageCustom( TF_DMG_CUSTOM_PLASMA );
		info.SetDamagePosition( pOther->EyePosition() );
		pOther->TakeDamage( info );

		// Slow enemy players
		if ( pOther->IsPlayer() )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pOther );
			if ( !pTFVictim )
				return;

			float flStun = ( nShieldLevel > 1 ) ? 0.5f : 0.3f;
			pTFVictim->m_Shared.StunPlayer( 0.5f, flStun, TF_STUN_MOVEMENT, pTFOwner );
		}

		// Sound
		if ( !m_pTouchLoop )
		{
			CPVSFilter filter( GetAbsOrigin() );
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			m_pTouchLoop = controller.SoundCreate( filter, entindex(), "WeaponMedi_Shield.Burn_lp" );
			controller.Play( m_pTouchLoop, 1.0, 100 );
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFMedigunShield::EndTouch( CBaseEntity *pOther )
{
	BaseClass::EndTouch( pOther );

	if ( m_pTouchLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pTouchLoop );
		m_pTouchLoop = NULL;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFMedigunShield::ShieldThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner || !pOwner->m_Shared.IsRageDraining() )
	{
		RemoveShield();
		return;
	}

	UpdateShieldPosition();
	
	// Regen shield
	if ( m_flShieldEnergyLevel < PROJECTILE_SHIELD_ENERGY_MAX )
	{
		m_flShieldEnergyLevel += PROJECTILE_SHIELD_ENERGY_REFILL_PULSE;
		m_flShieldEnergyLevel = Min( m_flShieldEnergyLevel, PROJECTILE_SHIELD_ENERGY_MAX );
	}

	// Visuals
	SetRenderMode( kRenderTransAlpha );
	int nShieldOpacity = PROJECTILE_SHIELD_ALPHA_BASE;

	// Determine alpha
	float flFadePoint = PROJECTILE_SHIELD_ENERGY_MAX / 2.f;
	if ( m_flShieldEnergyLevel <= flFadePoint )
	{
		nShieldOpacity = (int)RemapValClamped( m_flShieldEnergyLevel, 0.f, flFadePoint, 255.f, PROJECTILE_SHIELD_ALPHA_BASE );
	}

	// Blink when we're about to expire
	if ( pOwner->m_Shared.GetRageMeter() <= 25.f )
	{
		++m_nBlinkCount;

		if ( m_nBlinkCount & 0x2 )
		{
			SetRenderColorA( PROJECTILE_SHIELD_ALPHA_BASE - 100 );
		}
		else
		{
			SetRenderColorA( nShieldOpacity );
		}

		// Play a retract sound
		if ( m_nBlinkCount == 1 )
		{
			CPVSFilter filter( pOwner->WorldSpaceCenter() );
			pOwner->EmitSound( filter, entindex(), "WeaponMedi_Shield.Retract" );
		}
	}
	else
	{
		SetRenderColorA( nShieldOpacity );
	}

	SetContextThink( &CTFMedigunShield::ShieldThink, gpGlobals->curtime + 0.1f, MEDIGUNSHIELD_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFMedigunShield::RemoveShield( void )
{
	SetTouch( NULL );
	AddEffects( EF_NODRAW );

	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CTFMedigunShield::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( !InSameTeam( info.GetAttacker() ) )
	{
		CEffectData	data;
		data.m_vOrigin = ptr->endpos - (vecDir * 8);
		data.m_vNormal = -vecDir;
		data.m_flMagnitude = RemapValClamped( info.GetDamage(), 1.f, 50.f, 0.5f, 2.f );

		CPVSFilter filter( GetAbsOrigin() );
		te->DispatchEffect( filter, 0.f, data.m_vOrigin, "EnergyShieldImpact", data );

		// g_pEffects->EnergySplash( ptr->endpos - (vecDir * 8), -vecDir, false );
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int	CTFMedigunShield::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !InSameTeam( info.GetAttacker() ) )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( pOwner )
		{
			float flDamage = info.GetDamage();
			m_flShieldEnergyLevel -= flDamage;
			m_flShieldEnergyLevel = Max( m_flShieldEnergyLevel, 0.f );

			// Add to blocked damage stat
			CTF_GameStats.Event_PlayerBlockedDamage( pOwner, flDamage );

// 			CPVSFilter filter( pOwner->WorldSpaceCenter() );
// 			pOwner->EmitSound( filter, entindex(), "WeaponMedi_Shield.Protection" );

			pOwner->PlayDamageResistSound( 100.f, RandomFloat( 0.85f, 1.f ) );

			IGameEvent *event = gameeventmanager->CreateEvent( "medigun_shield_blocked_damage" );
			if ( event )
			{
				event->SetInt( "userid", pOwner->GetUserID() );
				event->SetFloat( "damage", flDamage );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	return 0;
}
#endif // GAME_DLL


