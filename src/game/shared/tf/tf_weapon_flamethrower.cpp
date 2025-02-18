//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Flame Thrower
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flamethrower.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "tf_weapon_rocketpack.h"
#include "debugoverlay_shared.h"
#include "soundenvelope.h"

#if defined( CLIENT_DLL )

	#include "c_tf_player.h"
	#include "vstdlib/random.h"
	#include "engine/IEngineSound.h"
	#include "prediction.h"
	#include "haptics/ihaptics.h"
	#include "c_tf_gamestats.h"
#else

	#include "explode.h"
	#include "tf_player.h"
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
	#include "collisionutils.h"
	#include "tf_team.h"
	#include "tf_obj.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "particle_parse.h"
	#include "tf_weaponbase_grenadeproj.h"
	#include "tf_weapon_compound_bow.h"
	#include "tf_projectile_arrow.h"
	#include "NextBot/NextBotManager.h"
	#include "halloween/merasmus/merasmus_trick_or_treat_prop.h"
	#include "tf_logic_robot_destruction.h"
	#include "tf_passtime_logic.h"

	ConVar  tf_flamethrower_velocity( "tf_flamethrower_velocity", "2300.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Initial velocity of flame damage entities." );
	ConVar	tf_flamethrower_drag("tf_flamethrower_drag", "0.87", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Air drag of flame damage entities." );
	ConVar	tf_flamethrower_float("tf_flamethrower_float", "50.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Upward float velocity of flame damage entities." );
	ConVar  tf_flamethrower_vecrand("tf_flamethrower_vecrand", "0.05", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Random vector added to initial velocity of flame damage entities." );

	ConVar  tf_flamethrower_maxdamagedist("tf_flamethrower_maxdamagedist", "350.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Maximum damage distance for flamethrower." );
	ConVar  tf_flamethrower_shortrangedamagemultiplier("tf_flamethrower_shortrangedamagemultiplier", "1.2", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Damage multiplier for close-in flamethrower damage." );
	ConVar  tf_flamethrower_velocityfadestart("tf_flamethrower_velocityfadestart", ".3", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time at which attacker's velocity contribution starts to fade." );
	ConVar  tf_flamethrower_velocityfadeend("tf_flamethrower_velocityfadeend", ".5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time at which attacker's velocity contribution finishes fading." );
	ConVar	tf_flamethrower_burst_zvelocity( "tf_flamethrower_burst_zvelocity", "350", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
	const float	tf_flamethrower_burn_frequency = 0.075f;
	const float	tf_flamethrower_afterburn_rate = 0.4f;

	static const char *s_pszFlameThrowerHitTargetThink = "FlameThrowerHitTargetThink";
#endif

ConVar	tf_debug_flamethrower("tf_debug_flamethrower", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Visualize the flamethrower damage." );
ConVar  tf_flamethrower_boxsize("tf_flamethrower_boxsize", "12.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Size of flame damage entities.", true, 1.f, true, 24.f );
ConVar  tf_flamethrower_new_flame_offset( "tf_flamethrower_new_flame_offset", "40 5 0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Starting position relative to the flamethrower." );
const float	tf_flamethrower_initial_afterburn_duration = 3.f;
const float	tf_flamethrower_airblast_cone_angle = 35.0f;


#include "tf_pumpkin_bomb.h"

const float	tf_flamethrower_new_flame_fire_delay = 0.02f;
const float	tf_flamethrower_damage_per_tick = 13.f;
ConVar  tf_flamethrower_burstammo("tf_flamethrower_burstammo", "20", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "How much ammo does the air burst use per shot." );
ConVar  tf_flamethrower_flametime("tf_flamethrower_flametime", "0.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Time to live of flame damage entities." );


// If we're shipping this it needs to be better hooked with flame manager -- right now we just spawn 5 managers for
// prototyping
#ifdef WATERFALL_FLAMETHROWER_TEST
// TODO:  ConVars for easier for easier testing, but should be attributes for shipping?
// (TODO: Add con commands to modify stock attributes on the fly)
ConVar	tf_flamethrower_waterfall_damage_per_tick( "tf_flamethrower_waterfall_damage_per_tick", "7", FCVAR_REPLICATED );
#endif // WATERFALL_FLAMETHROWER_TEST

#if defined( GAME_DLL )
// TODO These should be cheat upon shipping probably
ConVar tf_airblast_cray( "tf_airblast_cray", "1", FCVAR_CHEAT,
                         "Use alternate cray airblast logic globally." );
ConVar tf_airblast_cray_debug( "tf_airblast_cray_debug", "0", FCVAR_CHEAT,
                               "Enable debugging overlays & output for cray airblast.  "
                               "Value is length of time to show debug overlays in seconds." );
ConVar tf_airblast_cray_power( "tf_airblast_cray_power", "600", FCVAR_CHEAT,
                               "Amount of force cray airblast should apply unconditionally. "
                               "Set to 0 to only perform player momentum reflection.");
ConVar tf_airblast_cray_power_relative( "tf_airblast_cray_power_relative", "0", FCVAR_CHEAT,
                                        "If set, the blast power power also inherits from the blast's forward momentum." );
ConVar tf_airblast_cray_reflect_coeff( "tf_airblast_cray_reflect_coeff", "2", FCVAR_CHEAT,
                                       "The coefficient of reflective power cray airblast employs.\n"
                                       " 0   - No reflective powers\n"
                                       " 0-1 - Cancel out some/all incoming velocity\n"
                                       " 1-2 - Reflect some/all incoming velocity outwards\n"
                                       " 2+  - Reflect incoming velocity outwards and then some\n" );
ConVar tf_airblast_cray_reflect_cost_coeff( "tf_airblast_cray_reflect_cost_coeff", "0.5", FCVAR_CHEAT,
                                            "What portion of power used for reflection is removed from the push effect. "
                                            "Note that reflecting incoming momentum requires 2x the momentum - "
                                            "to first neutralize and then reverse it.  Setting this to 1 means that a "
                                            "target running towards the blast at more than 50% blast-speed would have "
                                            "a net pushback half that of a stationary target, since half the power was "
                                            "used to negate their incoming momentum. A value of 0.5 would mean that "
                                            "running towards the blast would not be beneficial vs being still, while "
                                            "values >.5 would make it beneficial to do so, and <.5 detrimental." );
ConVar tf_airblast_cray_reflect_relative( "tf_airblast_cray_reflect_relative", "0", FCVAR_CHEAT,
                                          "If set, the relative, rather than absolute, target velocity is considered "
                                          "for reflection." );
ConVar tf_airblast_cray_ground_reflect( "tf_airblast_cray_ground_reflect", "1", FCVAR_CHEAT,
                                        "If set, cray airblast reflects any airblast power directed into the ground "
                                        "off of it, to prevent ground-stuck and provide a bit more control over "
                                        "up-vs-forward vectoring" );
ConVar tf_airblast_cray_ground_minz( "tf_airblast_cray_ground_minz", "100", FCVAR_CHEAT,
                                     "If set, cray airblast ensures the target has this minimum Z velocity after "
                                     "reflections and impulse have been applied. "
                                     "Set to 268.3281572999747 for exact old airblast Z behavior." );
ConVar tf_airblast_cray_lose_footing_duration( "tf_airblast_cray_lose_footing_duration", "0.5", FCVAR_CHEAT,
                                               "How long the player should be unable to regain their footing after "
                                               "being airblast, separate from air-control stun." );
ConVar tf_airblast_cray_stun_duration( "tf_airblast_cray_stun_duration", "0", FCVAR_CHEAT,
                                       "If set, apply this duration of stun when initially hit by an airblast.  "
                                       "Does not apply to repeated airblasts.", true, 0.0f, true, 1.0f );
ConVar tf_airblast_cray_stun_amount( "tf_airblast_cray_stun_amount", "0", FCVAR_CHEAT,
                                     "Amount of control loss to apply if stun_duration is set.",
                                     true, 0.0f, true, 1.0f );
ConVar tf_airblast_cray_pitch_control( "tf_airblast_cray_pitch_control", "0", FCVAR_CHEAT,
                                       "If set, allow controlling the pitch of the airblast, in addition to the yaw." );
#endif // defined( GAME_DLL )

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// position of end of muzzle relative to shoot position
#define TF_FLAMETHROWER_MUZZLEPOS_FORWARD		70.0f
#define TF_FLAMETHROWER_MUZZLEPOS_RIGHT			12.0f
#define TF_FLAMETHROWER_MUZZLEPOS_UP			-12.0f

#define TF_FLAMETHROWER_AMMO_PER_SECOND_PRIMARY_ATTACK		14.0f

#define TF_FLAMETHROWER_HITACCURACY_MED			40.0f
#define TF_FLAMETHROWER_HITACCURACY_HIGH		60.0f

//-----------------------------------------------------------------------------

#define TF_WEAPON_BUBBLE_WAND_MODEL		"models/player/items/pyro/mtp_bubble_wand.mdl"

//-----------------------------------------------------------------------------

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Only send to local player
//-----------------------------------------------------------------------------
void* SendProxy_SendLocalFlameThrowerDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendLocalFlameThrowerDataTable );
#endif	// CLIENT_DLL

IMPLEMENT_NETWORKCLASS_ALIASED( TFFlameThrower, DT_WeaponFlameThrower )

//-----------------------------------------------------------------------------
// Purpose: Only sent to the local player
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CTFFlameThrower, DT_LocalFlameThrower )
	#if defined( CLIENT_DLL )
		RecvPropInt( RECVINFO( m_iActiveFlames ) ),
		RecvPropInt( RECVINFO( m_iDamagingFlames ) ),
		RecvPropEHandle( RECVINFO( m_hFlameManager ) ),
		RecvPropBool( RECVINFO( m_bHasHalloweenSpell ) ),
	#else
		SendPropInt( SENDINFO( m_iActiveFlames ), 5, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
		SendPropInt( SENDINFO( m_iDamagingFlames ), 10, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
		SendPropEHandle( SENDINFO( m_hFlameManager ) ),
		SendPropBool( SENDINFO( m_bHasHalloweenSpell ) ),
	#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE( CTFFlameThrower, DT_WeaponFlameThrower )
	#if defined( CLIENT_DLL )
		RecvPropInt( RECVINFO( m_iWeaponState ) ),
		RecvPropBool( RECVINFO( m_bCritFire ) ),
		RecvPropBool( RECVINFO( m_bHitTarget ) ),
		RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
		RecvPropDataTable("LocalFlameThrowerData", 0, 0, &REFERENCE_RECV_TABLE( DT_LocalFlameThrower ) ),
	#else
		SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
		SendPropBool( SENDINFO( m_bCritFire ) ),
		SendPropBool( SENDINFO( m_bHitTarget ) ),
		SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
		SendPropDataTable("LocalFlameThrowerData", 0, &REFERENCE_SEND_TABLE( DT_LocalFlameThrower ), SendProxy_SendLocalFlameThrowerDataTable ),
	#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFFlameThrower )
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCritFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD(  m_flChargeBeginTime, FIELD_FLOAT ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_flamethrower, CTFFlameThrower );
PRECACHE_WEAPON_REGISTER( tf_weapon_flamethrower );

BEGIN_DATADESC( CTFFlameThrower )
END_DATADESC()

// ------------------------------------------------------------------------------------------------ //
// CTFFlameThrower implementation.
// ------------------------------------------------------------------------------------------------ //
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlameThrower::CTFFlameThrower()
#if defined( CLIENT_DLL )
: 	m_FlameEffects( this )
,	m_MmmmphEffect( this )
#endif
{
	WeaponReset();

#if defined( CLIENT_DLL )
	m_pFiringStartSound = NULL;
	m_pFiringLoop = NULL;
	m_pFiringAccuracyLoop = NULL;
	m_pFiringHitLoop = NULL;
	m_bFiringLoopCritical = false;
	m_pPilotLightSound = NULL;
	m_pSpinUpSound = NULL;
	m_szAccuracySound = NULL;
	m_bEffectsThinking = false;
	m_bFullRageEffect = false;
#else
	m_flTimeToStopHitSound = 0;
#endif

	m_flSecondaryAnimTime = 0.f;
	m_bHasHalloweenSpell.Set( false );
	m_flMinPrimaryAttackBurstTime = 0.f;

	m_szParticleEffectBlue[0] = '\0';
	m_szParticleEffectRed[0] = '\0';	
	m_szParticleEffectBlueCrit[0] = '\0';
	m_szParticleEffectRedCrit[0] = '\0';

	ListenForGameEvent( "recalculate_holidays" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlameThrower::~CTFFlameThrower()
{
	DestroySounds();
#if defined( CLIENT_DLL )
	StopFullCritEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFlameThrower::GetNewFlameEffectInternal( int nTeam, bool bCrit )
{
	static CSchemaAttributeDefHandle pAttrDef_FireParticleBlue( "fire particle blue" );
	static CSchemaAttributeDefHandle pAttrDef_FireParticleRed( "fire particle red" );
	static CSchemaAttributeDefHandle pAttrDef_FireParticleBlueCrit( "fire particle blue crit" );
	static CSchemaAttributeDefHandle pAttrDef_FireParticleRedCrit( "fire particle red crit" );

	char *pszParticleEffect = ( nTeam == TF_TEAM_BLUE ) ? ( bCrit ? m_szParticleEffectBlueCrit : m_szParticleEffectBlue ) : ( bCrit ? m_szParticleEffectRedCrit : m_szParticleEffectRed );
	if ( !pszParticleEffect[0] )
	{
		CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem )
		{
			CAttribute_String attrModule;
			const CEconItemAttributeDefinition *pAttrDef = ( nTeam == TF_TEAM_BLUE ) ? ( bCrit ? pAttrDef_FireParticleBlueCrit : pAttrDef_FireParticleBlue ) : ( bCrit ? pAttrDef_FireParticleRedCrit : pAttrDef_FireParticleRed );
			if ( !pItem->FindAttribute( pAttrDef, &attrModule ) || !attrModule.has_value() )
			{
				V_strncpy( pszParticleEffect, bCrit ? ( ( nTeam == TF_TEAM_BLUE ) ? "new_flame_crit_blue" : "new_flame_crit_red" ) : "new_flame", MAX_PARTICLE_EFFECT_NAME_LENGTH );
			}
			else
			{
				V_strncpy( pszParticleEffect, attrModule.value().c_str(), MAX_PARTICLE_EFFECT_NAME_LENGTH );
			}
		}
	}

	return ( pszParticleEffect[0] ? pszParticleEffect : "new_flame" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::Precache( void )
{
	BaseClass::Precache();

	int iModelIndex = PrecacheModel( TF_WEAPON_BUBBLE_WAND_MODEL );
	PrecacheGibsForModel( iModelIndex );

	PrecacheParticleSystem( "pyro_blast" );
	PrecacheParticleSystem( "flamethrower_rope" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttack" );
	PrecacheScriptSound( "TFPlayer.AirBlastImpact" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	PrecacheParticleSystem( "deflect_fx" );
	PrecacheParticleSystem( "drg_bison_idle" );
	PrecacheParticleSystem( "medicgun_invulnstatus_fullcharge_blue" );
	PrecacheParticleSystem( "medicgun_invulnstatus_fullcharge_red" );
	PrecacheParticleSystem( "halloween_burningplayer_flyingbits" );
	PrecacheParticleSystem( "torch_player_burn" );
	PrecacheParticleSystem( "torch_red_core_1" );

	// for airblast projectile turn into ammopack
	PrecacheModel( "models/items/ammopack_small.mdl" );


#ifdef GAME_DLL
	// only do this for the server here, since the client
	// isn't ready for this yet when it calls into Precache()
	PrecacheParticleSystem( GetNewFlameEffectInternal( TF_TEAM_BLUE, false ) );
	PrecacheParticleSystem( GetNewFlameEffectInternal( TF_TEAM_BLUE, true ) );
	PrecacheParticleSystem( GetNewFlameEffectInternal( TF_TEAM_RED, false ) );
	PrecacheParticleSystem( GetNewFlameEffectInternal( TF_TEAM_RED, true ) );

#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::CanAirBlast() const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	int iAirblastDisabled = 0;
	CALL_ATTRIB_HOOK_INT( iAirblastDisabled, airblast_disabled );

	return ( iAirblastDisabled == 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::CanAirBlastPushPlayer() const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( !CanAirBlast() )
		return false;

	int iNoPushPlayer = 0;
	CALL_ATTRIB_HOOK_INT( iNoPushPlayer, airblast_pushback_disabled );

	return ( iNoPushPlayer == 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::CanAirBlastDeflectProjectile() const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( !CanAirBlast() )
		return false;

	int iDeflectProjectilesDisabled = 0;
	CALL_ATTRIB_HOOK_INT( iDeflectProjectilesDisabled, airblast_deflect_projectiles_disabled );

	return ( iDeflectProjectilesDisabled == 0 );
}

bool CTFFlameThrower::CanAirBlastPutOutTeammate() const
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return false;

	if ( !CanAirBlast() )
		return false;

	int iPutOutTeammateDisabled = 0;
	CALL_ATTRIB_HOOK_INT( iPutOutTeammateDisabled, airblast_put_out_teammate_disabled );

	return ( iPutOutTeammateDisabled == 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::DestroySounds( void )
{
#if defined( CLIENT_DLL )
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pFiringStartSound )
	{
		controller.SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}
	if ( m_pFiringLoop )
	{
		controller.SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}
	if ( m_pPilotLightSound )
	{
		controller.SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
	if ( m_pSpinUpSound )
	{
		controller.SoundDestroy( m_pSpinUpSound );
		m_pSpinUpSound = NULL;
	}
	if ( m_pFiringAccuracyLoop )
	{
		controller.SoundDestroy( m_pFiringAccuracyLoop );
		m_pFiringAccuracyLoop = NULL;
	}

	StopHitSound();
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::WeaponReset( void )
{
	BaseClass::WeaponReset();

	SetWeaponState( FT_STATE_IDLE );
	m_bCritFire = false;
	m_bHitTarget = false;
	m_flStartFiringTime = 0.f;
	m_flMinPrimaryAttackBurstTime = 0.f;
	m_flAmmoUseRemainder = 0.f;
	m_flChargeBeginTime = 0.f;
	m_flSpinupBeginTime = 0.f;
	ResetFlameHitCount();
	DestroySounds();

#if defined( CLIENT_DLL )
	StopFullCritEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::WeaponIdle( void )
{
	BaseClass::WeaponIdle();

	SetWeaponState( FT_STATE_IDLE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_FLAMETHROWER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::UpdateOnRemove( void )
{
#ifdef CLIENT_DLL
	m_FlameEffects.StopEffects();
	m_MmmmphEffect.StopEffects();
	StopPilotLight();
	StopFullCritEffect();
	m_bEffectsThinking = false;
#endif // CLIENT_DLL

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	SetWeaponState( FT_STATE_IDLE );
	m_bCritFire = false;
	m_bHitTarget = false;
	m_flChargeBeginTime = 0;

#if defined ( CLIENT_DLL )
	StopFlame();
	StopPilotLight();
	StopFullCritEffect();

	m_bEffectsThinking = false;
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::ItemPostFrame()
{
	if ( m_bLowered )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

#ifdef CLIENT_DLL
	if ( !m_bEffectsThinking )
	{
		m_bEffectsThinking = true;
		SetContextThink( &CTFFlameThrower::ClientEffectsThink, gpGlobals->curtime, "EFFECTS_THINK" );
	}
#endif

	int iAmmo = pOwner->GetAmmoCount( m_iPrimaryAmmoType );

	m_bFiredSecondary = false;
	if ( pOwner->IsAlive() && ( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		SecondaryAttack();
	}

	// Fixes an exploit where the airblast effect repeats while +attack is active
	if ( m_bFiredBothAttacks )
	{
		if ( pOwner->m_nButtons & IN_ATTACK && !( pOwner->m_nButtons & IN_ATTACK2 ) )
		{
			pOwner->m_nButtons &= ~IN_ATTACK;
		}
		m_bFiredBothAttacks = false;
	}

	if ( !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		// We were forced to fire, but time's up
		if ( m_flMinPrimaryAttackBurstTime > 0.f && gpGlobals->curtime > m_flMinPrimaryAttackBurstTime )
		{
			m_flMinPrimaryAttackBurstTime = 0.f;
#ifdef GAME_DLL
			if ( m_hFlameManager )
				{ m_hFlameManager->StopFiring(); }
#endif // GAME_DLL
			//DevMsg( "Stop Firing\n" );
		}
	}

	if ( pOwner->m_nButtons & IN_ATTACK && pOwner->m_nButtons & IN_ATTACK2 )
	{
		m_bFiredBothAttacks = true;
	}

	// Force a min window of emission to prevent a case where
	// tap-spamming +attack can create invisible flame points.
	bool bForceFire = ( m_flMinPrimaryAttackBurstTime > 0.f && gpGlobals->curtime < m_flMinPrimaryAttackBurstTime );

	if ( !m_bFiredSecondary )
	{
		bool bSpinDown = m_flSpinupBeginTime > 0.0f;

		if ( pOwner->IsAlive() && ( ( pOwner->m_nButtons & IN_ATTACK ) || bForceFire ) && iAmmo > 0 )
		{
			PrimaryAttack();
			bSpinDown = false;
		}
		else if ( m_iWeaponState > FT_STATE_IDLE && m_iWeaponState != FT_STATE_SECONDARY )
		{
			SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
			SetWeaponState( FT_STATE_IDLE );
			m_bCritFire = false;
			m_bHitTarget = false;
		}

		if ( bSpinDown )
		{
			m_flSpinupBeginTime = 0.0f;

#if defined( CLIENT_DLL )
			if ( m_pSpinUpSound )
			{
				float flSpinUpTime = GetSpinUpTime();

				CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
				controller.SoundChangePitch( m_pSpinUpSound, 40, flSpinUpTime * 0.5f );
				controller.SoundChangeVolume( m_pSpinUpSound, 0.0f, flSpinUpTime * 2.0f );
			}
#endif
		}
	}

	if ( !( ( pOwner->m_nButtons & IN_ATTACK ) || ( pOwner->m_nButtons & IN_RELOAD ) || ( pOwner->m_nButtons & IN_ATTACK2 ) || m_bFiredSecondary ) && !bForceFire )
	{
		// no fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) && m_flSecondaryAnimTime < gpGlobals->curtime )
		{
			WeaponIdle();
		}
	}

	// charged airblast
	int iChargedAirblast = 0;
	CALL_ATTRIB_HOOK_INT( iChargedAirblast, set_charged_airblast );
	if ( iChargedAirblast != 0 )
	{
		if ( m_flChargeBeginTime > 0 )
		{
			// If we're not holding down the attack button, launch the flame rocket
			if ( !(pOwner->m_nButtons & IN_ATTACK2) )
			{
				//FireProjectile( pOwner );
				float flMultAmmoPerShot = 1.0f;
				CALL_ATTRIB_HOOK_FLOAT( flMultAmmoPerShot, mult_airblast_cost );
				int iAmmoPerShot = tf_flamethrower_burstammo.GetInt() * flMultAmmoPerShot;
				FireAirBlast( iAmmoPerShot );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::PrimaryAttack()
{
	float flSpinUpTime = GetSpinUpTime();

	if ( flSpinUpTime > 0.0f )
	{
		if ( m_flSpinupBeginTime > 0.0f )
		{
			if ( gpGlobals->curtime - m_flSpinupBeginTime < flSpinUpTime )
			{
				return;
			}
		}
		else
		{
			m_flSpinupBeginTime = gpGlobals->curtime;

#if defined( CLIENT_DLL )
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			if ( !m_pSpinUpSound )
			{
				// Create the looping pilot light sound
				const char *pchSpinUpSound = GetShootSound( RELOAD );
				CLocalPlayerFilter filter;
				m_pSpinUpSound = controller.SoundCreate( filter, entindex(), pchSpinUpSound );

				controller.Play( m_pSpinUpSound, 0.0f, 40 );
			}

			if ( m_pSpinUpSound )
			{
				controller.SoundChangePitch( m_pSpinUpSound, 100, flSpinUpTime );
				controller.SoundChangeVolume( m_pSpinUpSound, 1.0f, flSpinUpTime * 0.1f );
			}
#endif
			return;
		}
	}

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
#if defined ( CLIENT_DLL )
		StopFlame();
#endif
		SetWeaponState( FT_STATE_IDLE );
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	CalcIsAttackCritical();

	// Because the muzzle is so long, it can stick through a wall if the player is right up against it.
	// Make sure the weapon can't fire in this condition by tracing a line between the eye point and the end of the muzzle.
	trace_t trace;	
	Vector vecEye = pOwner->EyePosition();
	Vector vecMuzzlePos = GetVisualMuzzlePos();
	CTraceFilterIgnoreObjects traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecMuzzlePos, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction < 1.0 && ( !trace.m_pEnt || trace.m_pEnt->m_takedamage == DAMAGE_NO ) )
	{
		// there is something between the eye and the end of the muzzle, most likely a wall, don't fire, and stop firing if we already are
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
#if defined ( CLIENT_DLL )
			StopFlame();
#endif
			SetWeaponState( FT_STATE_IDLE );
		}
		return;
	}

	switch ( m_iWeaponState )
	{
	case FT_STATE_IDLE:
		{
			// Just started, play PRE and start looping view model anim

			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );

			SendWeaponAnim( ACT_VM_PRIMARYATTACK );

			m_flStartFiringTime = gpGlobals->curtime + 0.16;	// 5 frames at 30 fps
			
			// Force a min window of emission to prevent a case where
			// tap-spamming +attack can create invisible flame points.
			if ( m_flMinPrimaryAttackBurstTime == 0.f )
			{
				m_flMinPrimaryAttackBurstTime = gpGlobals->curtime + 0.2f;
			}

			SetWeaponState( FT_STATE_STARTFIRING );
		}
		break;
	case FT_STATE_STARTFIRING:
		{
			// if some time has elapsed, start playing the looping third person anim
			if ( gpGlobals->curtime > m_flStartFiringTime )
			{
				SetWeaponState( FT_STATE_FIRING );
				m_flNextPrimaryAttackAnim = gpGlobals->curtime;
			}
		}
		break;
	case FT_STATE_FIRING:
		{
			if ( gpGlobals->curtime >= m_flNextPrimaryAttackAnim )
			{
				pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flNextPrimaryAttackAnim = gpGlobals->curtime + 1.4;		// fewer than 45 frames!
			}
		}
		break;

	default:
		break;
	}

#ifdef CLIENT_DLL
	// Restart our particle effect if we've transitioned across water boundaries
	if ( m_iParticleWaterLevel != -1 && pOwner->GetWaterLevel() != m_iParticleWaterLevel )
	{
		if ( m_iParticleWaterLevel == WL_Eyes || pOwner->GetWaterLevel() == WL_Eyes )
		{
			RestartParticleEffect();
		}
	}
#endif

#if !defined (CLIENT_DLL)
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, m_bCritFire );

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );

	// PASSTIME custom lag compensation for the ball; see also tf_fx_shared.cpp
	// it would be better if all entities could opt-in to this, or a way for lagcompensation to handle non-players automatically
	if ( g_pPasstimeLogic && g_pPasstimeLogic->GetBall() )
	{
		g_pPasstimeLogic->GetBall()->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );
	}

#endif
#ifdef CLIENT_DLL
	C_CTF_GameStats.Event_PlayerFiredWeapon( pOwner, IsCurrentAttackACrit() );
#endif

	float flFiringInterval = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	{
		flFiringInterval = tf_flamethrower_new_flame_fire_delay;
	}

	// Don't attack if we're underwater
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
		// Find eligible entities in a cone in front of us.
		// Vector vOrigin = pOwner->Weapon_ShootPosition();
		Vector vForward, vRight, vUp;
		QAngle vAngles = pOwner->EyeAngles() + pOwner->GetPunchAngle();
		AngleVectors( vAngles, &vForward, &vRight, &vUp );

		#define NUM_TEST_VECTORS	30

#ifdef CLIENT_DLL
		bool bWasCritical = m_bCritFire;
#endif

		// Burn & Ignite 'em
		int iDmgType = g_aWeaponDamageTypes[ GetWeaponID() ];
		m_bCritFire = IsCurrentAttackACrit();
		if ( m_bCritFire )
		{
			iDmgType |= DMG_CRITICAL;
		}

#ifdef CLIENT_DLL
		if ( bWasCritical != m_bCritFire )
		{
			RestartParticleEffect();
		}
#endif


#ifdef GAME_DLL
		// create the flame entity
		int iDamagePerSec = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
		float flDamage = (float)iDamagePerSec * flFiringInterval;
		{
			flDamage = tf_flamethrower_damage_per_tick;
		}
#ifdef WATERFALL_FLAMETHROWER_TEST
		int iWaterfallMode = 0;
		CALL_ATTRIB_HOOK_INT( iWaterfallMode, flame_waterfall );
		if ( iWaterfallMode )
		{
			flDamage = tf_flamethrower_waterfall_damage_per_tick.GetFloat();
		}
#endif
		CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );

		int iCritFromBehind = 0;
		CALL_ATTRIB_HOOK_INT( iCritFromBehind, set_flamethrower_back_crit );

		{
			if ( !m_hFlameManager )
			{
				m_hFlameManager = CTFFlameManager::Create( this );
				// This is a hack(?).  Right now, the flame manager goes outside of the shooter's
				// own PVS when they get very close to a wall (or just looks down), so we end
				// up creating flame managers repeatedly for the same burst of flames.  This
				// call ensures that the new manager will create particle effects.
				//
				// The *real* fix is to figure out how to get the flame manager to not go out
				// of the shooter's PVS ever.
				m_hFlameManager->StartFiring();
			}

			if ( m_hFlameManager )
			{
				// update damage state
				m_hFlameManager->UpdateDamage( iDmgType, flDamage, tf_flamethrower_burn_frequency, iCritFromBehind == 1 );
				m_hFlameManager->AddPoint( TIME_TO_TICKS( gpGlobals->curtime ) );
			}
		}

		// Pyros can become invis in some game modes.  Hitting fire normally handles this,
		// but in the case of flamethrowers it's likely that stealth will be applied while
		// the fire button is down, so we have to call into RemoveInvisibility here, too.
		if ( pOwner->m_Shared.IsStealthed() )
		{
			pOwner->RemoveInvisibility();
		}
#endif
	}

#ifdef GAME_DLL
	// Figure how much ammo we're using per shot and add it to our remainder to subtract.  (We may be using less than 1.0 ammo units
	// per frame, depending on how constants are tuned, so keep an accumulator so we can expend fractional amounts of ammo per shot.)
	// Note we do this only on server and network it to client.  If we predict it on client, it can get slightly out of sync w/server
	// and cause ammo pickup indicators to appear
	float flAmmoPerSecond = TF_FLAMETHROWER_AMMO_PER_SECOND_PRIMARY_ATTACK;
	CALL_ATTRIB_HOOK_FLOAT( flAmmoPerSecond, mult_flame_ammopersec );
	m_flAmmoUseRemainder += flAmmoPerSecond * flFiringInterval;
	// take the integer portion of the ammo use accumulator and subtract it from player's ammo count; any fractional amount of ammo use
	// remains and will get used in the next shot
	int iAmmoToSubtract = (int) m_flAmmoUseRemainder;
	if ( iAmmoToSubtract > 0 )
	{
		pOwner->RemoveAmmo( iAmmoToSubtract, m_iPrimaryAmmoType );
		m_flAmmoUseRemainder -= iAmmoToSubtract;
		// round to 2 digits of precision
		m_flAmmoUseRemainder = (float) ( (int) (m_flAmmoUseRemainder * 100) ) / 100.0f;
	}
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + flFiringInterval;
	m_flTimeWeaponIdle = gpGlobals->curtime + flFiringInterval;

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pOwner );

	// PASSTIME custom lag compensation for the ball; see also tf_fx_shared.cpp
	// it would be better if all entities could opt-in to this, or a way for lagcompensation to handle non-players automatically
	if ( g_pPasstimeLogic && g_pPasstimeLogic->GetBall() )
	{
		g_pPasstimeLogic->GetBall()->FinishLagCompensation( pOwner );
	}
#endif

	pOwner->m_Shared.OnAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float AirBurstDamageForce( const Vector &size, float damage, float scale )
{ 
	float force = damage * ((48 * 48 * 82.0) / (size.x * size.y * size.z)) * scale;

	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::FireAirBlast( int iAmmoPerShot )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	m_bFiredSecondary = true;

#ifdef CLIENT_DLL
	// Stop the flame if we're currently firing
	StopFlame( false );
#endif

	SetWeaponState( FT_STATE_SECONDARY );

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
	m_flSecondaryAnimTime = gpGlobals->curtime + SequenceDuration( GetSequence() );
	
#ifdef GAME_DLL
	int nDash = 0;
	CALL_ATTRIB_HOOK_INT( nDash, airblast_dashes );

	if ( !nDash )
	{
		DeflectProjectiles();
	}
	else
	{
		Vector vDashDir = pOwner->GetAbsVelocity();
		if ( !pOwner->GetGroundEntity() || vDashDir.Length() == 0.0f )
		{
			AngleVectors( pOwner->EyeAngles(), &vDashDir );
		}
		vDashDir.z = 0.0f;
		VectorNormalize( vDashDir );

		DeflectPlayer( pOwner, pOwner, vDashDir );
	}

	// for charged airblast
	int iChargedAirblast = 0;
	CALL_ATTRIB_HOOK_INT( iChargedAirblast, set_charged_airblast );
	if ( iChargedAirblast != 0 )
	{
		m_flChargeBeginTime = 0;
	}

	// compression blast doesn't go through the normal "weapon fired" code path
	TheNextBots().OnWeaponFired( pOwner, this );
#endif

#ifdef CLIENT_DLL
	if ( prediction->IsFirstTimePredicted() == true )
	{
		StartFlame();
	}
#endif

	float fAirblastRefireTimeScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( fAirblastRefireTimeScale, mult_airblast_refire_time );
	if ( fAirblastRefireTimeScale <= 0.0f  )
	{
		fAirblastRefireTimeScale = 1.0f;
	}

	float fAirblastPrimaryRefireTimeScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( fAirblastPrimaryRefireTimeScale, mult_airblast_primary_refire_time );
	if ( fAirblastPrimaryRefireTimeScale <= 0.0f )
	{
		fAirblastPrimaryRefireTimeScale = 1.0f;
	}

	// Haste Powerup Rune adds multiplier to fire delay time
	if ( pOwner->m_Shared.GetCarryingRuneType() == RUNE_HASTE )
	{
		fAirblastRefireTimeScale *= 0.5f;
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + (0.75f * fAirblastRefireTimeScale);	
	m_flNextPrimaryAttack = gpGlobals->curtime + (1.0f * fAirblastRefireTimeScale * fAirblastPrimaryRefireTimeScale);
	m_flResetBurstEffect = gpGlobals->curtime + 0.05f;

	pOwner->RemoveAmmo( iAmmoPerShot, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlameThrower::GetSpinUpTime( void ) const
{
	float flSpinUpTime = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT( flSpinUpTime, mod_flamethrower_spinup_time );

	return flSpinUpTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SetWeaponState( int nWeaponState )
{
	if ( m_iWeaponState == nWeaponState )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();

	switch ( nWeaponState )
	{
	case FT_STATE_IDLE:
		if ( pOwner )
		{
			float flFiringForwardPull = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT( flFiringForwardPull, firing_forward_pull );
			if ( flFiringForwardPull )
			{
				pOwner->m_Shared.RemoveCond( TF_COND_SPEED_BOOST );
			}
			m_flMinPrimaryAttackBurstTime = 0.f;
		}

		break;

	case FT_STATE_STARTFIRING:
		if ( pOwner )
		{
			float flFiringForwardPull = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT( flFiringForwardPull, firing_forward_pull );
			if ( flFiringForwardPull )
			{
				pOwner->m_Shared.AddCond( TF_COND_SPEED_BOOST );
			}
		}

		break;
	}

	if ( m_hFlameManager )
	{
		if ( nWeaponState == FT_STATE_IDLE )
		{
			m_hFlameManager->StopFiring();
		}
		else
		{
			m_hFlameManager->StartFiring();
		}
	}

	m_iWeaponState = nWeaponState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::UseRage( void )
{
	if ( !IsRageFull() )
		return;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAllowedToTaunt() )
		return;

	float flNextAttack = m_flNextSecondaryAttack;

#if GAME_DLL
	// Do a taunt so everyone has a chance to run
	pPlayer->Taunt( TAUNT_BASE_WEAPON );
	if ( pPlayer->m_Shared.IsRageDraining() )
	{
		// taunt succeeded
		flNextAttack = gpGlobals->curtime + 1.0f;
	}
#else
	flNextAttack = gpGlobals->curtime + 1.0f;
#endif

	m_flNextSecondaryAttack = flNextAttack;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SecondaryAttack()
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( m_flChargeBeginTime > 0 )
	{
		m_bFiredSecondary = true;
		return;
	}

	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
	{
#ifndef CLIENT_DLL
		if ( m_flResetBurstEffect <= gpGlobals->curtime )
		{
			SetWeaponState( FT_STATE_IDLE );
		}
#endif
		return;
	}

	if ( pOwner->GetWaterLevel() == WL_Eyes )
		return;

	if ( !CanAttack() )
	{
		SetWeaponState( FT_STATE_IDLE );
		return;
	}

	
	int iAmmo = pOwner->GetAmmoCount( m_iPrimaryAmmoType );

	// charged airblast
	int iChargedAirblast = 0;
	CALL_ATTRIB_HOOK_INT( iChargedAirblast, set_charged_airblast );
	int iBuffType = 0;
	CALL_ATTRIB_HOOK_INT( iBuffType, set_buff_type );
	float flMultAmmoPerShot = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flMultAmmoPerShot, mult_airblast_cost );
	int iAmmoPerShot = tf_flamethrower_burstammo.GetInt() * flMultAmmoPerShot;

	if ( iBuffType != 0 )
	{
		UseRage();
		return;
	}

	if ( iAmmo < iAmmoPerShot )
		return;

	// normal air blast?
	if ( iChargedAirblast == 0 && CanAirBlast() )
	{
		FireAirBlast( iAmmoPerShot );
		return;
	}

	SetWeaponState( FT_STATE_SECONDARY );

#ifdef CLIENT_DLL
	// Stop the flame if we're currently firing
	StopFlame( false );
#else
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	m_flChargeBeginTime = gpGlobals->curtime;
	SendWeaponAnim( ACT_VM_PULLBACK );
	// @todo replace with the correct one
	WeaponSound( SINGLE );
#endif
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlameThrower::GetDeflectionRadius() const
{
	float fMultiplier = 1.0f;

	// int iChargedAirblast = 0;
	// CALL_ATTRIB_HOOK_INT( iChargedAirblast, set_charged_airblast );
	// if ( iChargedAirblast != 0 )
	// {
	//	 fMultiplier *= RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
	// 										  0.0f,
	// 										  GetChargeMaxTime(),
	// 										  AIRBLAST_CHARGE_MULT_MIN,
	// 										  AIRBLAST_CHARGE_MULT_MAX );
	// }

	// Allow custom attributes to scale the deflection size.
	CALL_ATTRIB_HOOK_FLOAT( fMultiplier, deflection_size_multiplier );

	return fMultiplier * BaseClass::GetDeflectionRadius();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef _DEBUG
ConVar tf_pushbackscalescale( "tf_pushbackscalescale", "1.0" );
ConVar tf_pushbackscalescale_vertical( "tf_pushbackscalescale_vertical", "1.0" );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ExtinguishPlayer( CEconEntity *pExtinguisher, CTFPlayer *pOwner, CTFPlayer *pTarget, const char *pExtinguisherName )
{
	pTarget->EmitSound( "TFPlayer.FlameOut" );

	pTarget->m_Shared.RemoveCond( TF_COND_BURNING );

	// we're going to limit the number of times you can be awarded bonus points to prevent exploits
	if ( pOwner->ShouldGetBonusPointsForExtinguishEvent( pTarget->GetUserID() ) )
	{
		CTF_GameStats.Event_PlayerAwardBonusPoints( pOwner, pTarget, 10 );
	}

	CRecipientFilter involved_filter;
	involved_filter.AddRecipient( pOwner );
	involved_filter.AddRecipient( pTarget );

	UserMessageBegin( involved_filter, "PlayerExtinguished" );
	WRITE_BYTE( pOwner->entindex() );
	WRITE_BYTE( pTarget->entindex() );
	MessageEnd();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_extinguished" );
	if ( event )
	{
		event->SetInt( "victim", pTarget->entindex() );
		event->SetInt( "healer", pOwner->entindex() );

		gameeventmanager->FireEvent( event, true );
	}

	// stats
	EconEntity_OnOwnerKillEaterEvent( pExtinguisher, pOwner, pTarget, kKillEaterEvent_BurningAllyExtinguished );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"player_extinguished\" against \"%s<%i><%s><%s>\" with \"%s\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
				pOwner->GetPlayerName(), pOwner->GetUserID(), pOwner->GetNetworkIDString(), pOwner->GetTeam()->GetName(),
				pTarget->GetPlayerName(), pTarget->GetUserID(), pTarget->GetNetworkIDString(), pTarget->GetTeam()->GetName(),
				pExtinguisherName, (int)pOwner->GetAbsOrigin().x, (int)pOwner->GetAbsOrigin().y, (int)pOwner->GetAbsOrigin().z,
				(int)pTarget->GetAbsOrigin().x, (int)pTarget->GetAbsOrigin().y, (int)pTarget->GetAbsOrigin().z );
}

//-----------------------------------------------------------------------------
// Purpose: Computes the push vector to apply for the Cray-Airblast logic.
//-----------------------------------------------------------------------------
void CTFFlameThrower::ComputeCrayAirBlastForce( CTFPlayer *pTarget, CTFPlayer *pPlayer, Vector &vecForward, Vector &vecOutForce )
{
	// Setup
	const float flDebugOverlayDuration = tf_airblast_cray_debug.GetFloat();
	const bool  bDebug                 = flDebugOverlayDuration > 0.f;
	const float flDebugOverlayScale    = .5f;
	const float flDebugOverlayWidth    = 2.f;
	const bool  bGroundReflect         = tf_airblast_cray_ground_reflect.GetBool();
	const bool  bRelativeReflect       = tf_airblast_cray_reflect_relative.GetBool();
	const float flGroundMinimumUpSpeed = tf_airblast_cray_ground_minz.GetFloat();
	const bool  bRelativePower         = tf_airblast_cray_power_relative.GetBool();
	const float flReflectCoeff         = tf_airblast_cray_reflect_coeff.GetFloat();
	const float flReflectCostCoeff     = tf_airblast_cray_reflect_cost_coeff.GetFloat();

	float flAirblastBasePower = tf_airblast_cray_power.GetFloat();
	float flAirblastVerticalMultiplier = 1.f;

	// Attributes.  Pushback scale is on the player, vulnerability multiplier on the victim.
	CALL_ATTRIB_HOOK_FLOAT( flAirblastBasePower, airblast_pushback_scale );
	CALL_ATTRIB_HOOK_FLOAT( flAirblastVerticalMultiplier, airblast_vertical_pushback_scale );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTarget, flAirblastBasePower, airblast_vulnerability_multiplier );
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTarget, flAirblastVerticalMultiplier, airblast_vertical_vulnerability_multiplier );

	// Debug convars
#ifdef _DEBUG
	flAirblastBasePower          *= tf_pushbackscalescale.GetFloat();
	flAirblastVerticalMultiplier *= tf_pushbackscalescale_vertical.GetFloat();
#endif

	Vector vecTargetOrigin = pTarget->GetAbsOrigin();
	Vector vecTargetVel = pTarget->GetAbsVelocity();
	// Vector vecAbsPlayer = pPlayer->WorldSpaceCenter();
	// Vector vecToTarget = vecAbsTarget - vecAbsPlayer;
	Vector &vecAim = vecForward;

	Vector vecResult( 0, 0, 0 );

	// Only compute this if we're using one of the relative switches
	float flForwardAimMomentum = -1.f;
	if ( bRelativeReflect || bRelativePower )
	{
		Vector vecPlayerVel = pPlayer->GetAbsVelocity();
		flForwardAimMomentum = DotProduct( vecPlayerVel, vecAim );
	}

	if ( bDebug )
	{
		Vector vecDebugMuzzle = GetVisualMuzzlePos() + Vector( 0, 0, 20 );
		NDebugOverlay::Text( vecDebugMuzzle, "Airblast Aim", true, flDebugOverlayDuration );
		NDebugOverlay::HorzArrow( vecDebugMuzzle, vecDebugMuzzle + vecAim * 5.f,
		                          flDebugOverlayWidth, 100, 0, 0, 255, true, flDebugOverlayDuration );
	}

	//
	// 1 - Reflect incoming velocity away from the pyro
	//

	// Aim is normalized, so this is the magnitude of the pyro-aim-axis momentum, negative if the target is coming at
	// the pyro
	float flMomentumAlongPyroAim = DotProduct( vecTargetVel, vecAim );
	if ( bRelativeReflect )
	{
		// Negative means towards-us here, so everything else has a base of -1 * our forward-aim velocity in relative
		// mode.
		flMomentumAlongPyroAim -= flForwardAimMomentum;
	}
	if ( flMomentumAlongPyroAim < 0.f )
	{
		// If it is negative, reflect by applying 2x this force straight forward. This should perserve their
		// non-aim-vector velocity but mirror it away from the pyro at that strength.
		float flReflectForce = -1.f * flReflectCoeff * flMomentumAlongPyroAim;
		Vector vecReflect = vecAim * flReflectForce;
		vecResult += vecReflect;

		if ( bDebug )
		{
			// This set of arrows will be from the target
			Vector vecTargetCenter = pTarget->WorldSpaceCenter();
			Vector vecIncomingVel = vecAim * flMomentumAlongPyroAim;
			Vector vecOutgoingVel = vecIncomingVel + ( vecAim * flReflectForce );

			// Visualize velocity towards pyro aim
			NDebugOverlay::HorzArrow( vecTargetCenter,
			                          vecTargetCenter + vecIncomingVel * flDebugOverlayScale,
			                          flDebugOverlayWidth, 200, 0, 0, 255,
			                          false, flDebugOverlayDuration );
			// Visualize mirror'd velocity away from pyro aim
			NDebugOverlay::HorzArrow( vecTargetCenter,
			                          vecTargetCenter + vecOutgoingVel * flDebugOverlayScale,
			                          flDebugOverlayWidth, 0, 200, 0, 255, true, flDebugOverlayDuration );
			// Visualize push
			NDebugOverlay::HorzArrow( vecTargetCenter,
			                          vecTargetCenter + vecReflect * flDebugOverlayScale,
			                          flDebugOverlayWidth, 150, 150, 150, 255, true, flDebugOverlayDuration );
			CFmtStr strDebug( "Reflected player off blast ( into-blast momentum %f )", -1 * flMomentumAlongPyroAim );
			NDebugOverlay::Text( vecTargetCenter, strDebug, true, flDebugOverlayDuration );
		}
	}

	//
	// 2 - If we have not yet applied minimum airblast force, add more
	//

	// This applies the remaining force for an enemy not moving fast enough to need it all for reflection.
	//
	// This generally results in:
	// - Standing still will apply 100% airblast power full away from the pyro
	//
	// - Approaching the pyro at (0,.5] of airblast power causing a dampened pushback (some of the power went to
	//   reflecting your incoming velocity, subtracted from the power then used to accelerate you).
	//
	// - Approaching at exactly 50% of airblast's power will be the minimum-strength pushback - we'll need 100% power to
	//   reflect that momentum away.
	//
	// - Approaching at over 50% airblast's power will still result in a full reflection, which will then be a push in
	// - excess of the normal full-force of airblast. (But no additional power beyond the reflect will be allowed below)
	//
	// Consider: If airblast force is 400.f, and ...
	//           ... A player is standing still, we'll apply 0 reflective force above, and thus 400.f force here.
	//
	//           ... A player is running at a pyro at 200.f, it will take 400.f force to reflect that momentum
	//           above. We'll then apply 0 additional here.  So they'll be pushed less forcefully than standing still.
	//
	//           ... A player is running at a pyro at 400.f.  Reflect will push them away at 400.f, requiring 800.f of
	//           power, and apply no additional here.
	float flPreampPower = vecResult.Length();
	float flAirblastPower = flAirblastBasePower + ( bRelativePower ? flForwardAimMomentum : 0.f );
	float flRemainingPushPower = flAirblastPower - flPreampPower * flReflectCostCoeff;
	if ( flRemainingPushPower > 0.f )
	{
		Vector vecAdditionalPush = vecAim * flRemainingPushPower;
		vecResult += vecAdditionalPush;

		if ( bDebug )
		{
			Vector vecDebugMuzzle = GetVisualMuzzlePos();
			Vector vecDebugBasePush = vecDebugMuzzle;
			Vector vecEndArrow = vecDebugBasePush + ( vecAdditionalPush * flDebugOverlayScale );
			NDebugOverlay::HorzArrow( vecDebugBasePush, vecEndArrow,
			                          flDebugOverlayWidth, 0, 0, 200, 255, true, flDebugOverlayDuration );
			CFmtStr strDebug("Remaining power after reflection ( %f power - %f reflection * %f cost coeff )",
			                  flAirblastPower, flPreampPower, flReflectCostCoeff );
			// Put at end of arrow since we're also drawing the final-impulse text at this origin
			NDebugOverlay::Text( vecEndArrow, strDebug, false, flDebugOverlayDuration );
		}
	}

	//
	// 3 - If this puts the player's momentum vector into the ground, reflect it off of the ground
	//

	// This replaces the older logic of "always positive Z" with instead determining if this is pushing the target into
	// their ground normal, and instead doing an elastic-mirroring upward.  This lets the pyro intentionally aim down to
	// get a more upward bounce, trading away push momentum.
	//
	// A different version of this might be giving the player a bounce-attribute for some duration, rather than only
	// doing reflects to entities currently-on-the-ground.  But this would mean you couldn't purposefully direct a
	// flying enemy into the ground.

	bool bTargetOnGround = pTarget->GetGroundEntity() != NULL;
	if ( bGroundReflect && bTargetOnGround )
	{
		// Find ground normal
		//
		// TODO This duplicates tracing done in CGameMovement::CategorizePosition -- we should cache this off for
		//      players on movement
		Ray_t ray;
		trace_t tr;
		ray.Init( vecTargetOrigin, vecTargetOrigin + Vector( 0, 0, -5 ), pTarget->GetPlayerMins(), pTarget->GetPlayerMaxs() );
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, pTarget, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

		bool bHit = tr.DidHit();
		AssertMsg( bHit, "Airblast: Player on ground entity doesn't trace to it" );
		if ( bHit )
		{
			Vector vecHypotheticalTargetVelocity = vecTargetVel + vecResult;
			float flFromGroundForce = DotProduct( vecHypotheticalTargetVelocity, tr.plane.normal );
			if ( flFromGroundForce < 0.f )
			{
				Vector vecReflectVector = tr.plane.normal * -2.f * flFromGroundForce;
				vecResult += vecReflectVector;
				if ( bDebug )
				{
					Vector vecDebugGround = vecTargetOrigin;

					// Draw an arrow showing into-ground velocity
					NDebugOverlay::HorzArrow( vecDebugGround,
					                          vecDebugGround + vecHypotheticalTargetVelocity * flDebugOverlayScale,
					                          flDebugOverlayWidth, 200, 0, 200, 255,
					                          true, flDebugOverlayDuration );
					// ... and post-reflect
					NDebugOverlay::HorzArrow( vecDebugGround,
					                          vecDebugGround + (vecHypotheticalTargetVelocity + vecReflectVector) * flDebugOverlayScale,
					                          flDebugOverlayWidth, 200, 200, 0, 255, true, flDebugOverlayDuration );
					CFmtStr strDebug( "Reflected player off ground ( into-ground momentum %f )", -1 * flFromGroundForce );
					NDebugOverlay::Text( vecDebugGround, strDebug, false, flDebugOverlayDuration );
				}
			}
		}
	}

	//
	// 4 - Ensure the player has an absolute minimum away-from-ground velocity to prevent sliding them along it
	//     just-right to cause pseudo-ground-stuck.
	//

	//
	if ( bTargetOnGround && flGroundMinimumUpSpeed > 0.f )
	{
		float flGroundVelZ = ( vecTargetVel + vecResult ).z;
		float flAddZ = flGroundMinimumUpSpeed - flGroundVelZ;
		if ( flAddZ > 0.f )
		{
			// Steal necessary velocity from XY components, rather than adding any magical velocity, so we don't reward
			// near-horizontal airblasts with more power than usual.
			Vector vecXY( vecResult.x, vecResult.y, 0 );
			float flInitialXY = vecXY.Length();
			float flFinalXY = Max( flInitialXY - flAddZ, 0.f );
			vecXY *= flInitialXY > 0.f ? ( flFinalXY / flInitialXY ) : 0.f;
			// If we couldn't steal enough power for Z, reduce
			flAddZ = Min( flAddZ, flInitialXY );

			// If its total magnitude is less than z+flAddZ we'll just redirect to straight-up with that magnitude
			// (maximum deflect up)
			Vector vecUpwardCorrected;
			float flCurrentMagnitudeSqr = vecResult.LengthSqr();
			if ( flCurrentMagnitudeSqr < flAddZ * flAddZ )
			{
				// Just redirect all available power upwards
				vecUpwardCorrected = Vector( 0, 0, FastSqrt( flCurrentMagnitudeSqr ) );
			}
			else
			{
				// Otherwise, to redirect the vector to a (z+flAddZ) Z component without changing its magnitude or xy
				// projection, we want to solve for N in:
				//   Vec(x,y,z).Length() = Vec(N*x, N*y, z + flAddZ).Length()
				// Which is:
				//   N = sqrt( -flAddZ^2 - 2(flAddZ)*z + x^2 + y^2 ) / sqrt( x^2 + y^2 )
				//
				// (Doing this the trigonometric way might be faster)
				float flResultXYSqr = Vector( vecResult.x, vecResult.y, 0 ).LengthSqr();
				float flXYScaleBase = -(flAddZ*flAddZ) - 2*flAddZ*vecResult.z + flResultXYSqr;
				float flXYScale = FastSqrt( Max( flXYScaleBase, 0.f ) ) / FastSqrt( flResultXYSqr );
				vecUpwardCorrected = Vector( vecResult.x * flXYScale, vecResult.y * flXYScale, vecResult.z + flAddZ );
			}

			if ( bDebug )
			{
				// Arrow from their feet showing Z redirect
				Vector vecArrowEnd = vecTargetOrigin + ( vecUpwardCorrected - vecResult ) * flDebugOverlayScale;
				NDebugOverlay::HorzArrow( vecTargetOrigin, vecArrowEnd,
				                          flDebugOverlayWidth, 50, 50, 50, 255, true, flDebugOverlayDuration );
				CFmtStr strDebug( "Applied redirect to maintain minimum Z ( %f -> %f )",
				                  vecResult.z, vecUpwardCorrected.z );
				NDebugOverlay::Text( vecArrowEnd, strDebug, false, flDebugOverlayDuration );
			}

			vecResult = vecUpwardCorrected;
		}
	}

	// Other random thought:
	// If the pyro is airborne he splits some portion of vecResult with himself. Eh? Eh?

	//
	// 5 - Apply flAirblastVerticalMultiplier if we have it (usually from attributes -- e.g. MvM bots have a additional
	//     resistance to being pushed up, separate from their general resistance)
	//
	if ( flAirblastVerticalMultiplier != 1.f )
	{
		float flOldZ = vecResult.z;
		vecResult.z *= flAirblastVerticalMultiplier;
		if ( bDebug )
		{
			// Arrow from their feet showing bonus Z, offset to not conflict with flGroundMinimumUpSpeed arrow
			Vector vecOffset = Vector( 20, 0, 0 );
			Vector vecArrowEnd = vecTargetOrigin + Vector( 0, 0, vecResult.z - flOldZ );
			NDebugOverlay::HorzArrow( vecTargetOrigin + vecOffset, vecArrowEnd + vecOffset,
			                          flDebugOverlayWidth, 50, 90, 90, 255, true, flDebugOverlayDuration );
			CFmtStr strDebug( "Airblast vertical push multiplier from attributes/vulnerabilities ( %f -> %f )",
			                  flOldZ, vecResult.z );
			NDebugOverlay::Text( vecArrowEnd + vecOffset, strDebug, false, flDebugOverlayDuration );
		}
	}

	vecOutForce = vecResult;
	if ( bDebug )
	{
		Vector vecDebugMuzzle = GetVisualMuzzlePos();
		// Show applied push vector, transparent because it should be superceding the component arrows
		NDebugOverlay::HorzArrow( vecDebugMuzzle, vecDebugMuzzle + vecResult * flDebugOverlayScale,
		                          flDebugOverlayWidth + 1.f, 255, 255, 255, 150,
		                          true, flDebugOverlayDuration );
		CFmtStr strDebug( "Applied Impulse: %f", vecResult.Length() );
		NDebugOverlay::Text( vecDebugMuzzle + Vector( 0, 0, -2 ), strDebug, true, flDebugOverlayDuration );

		// Show overall outgoing vector
		vecDebugMuzzle += Vector( 0, -20, 0 ); // Next to it
		NDebugOverlay::HorzArrow( vecDebugMuzzle,
		                          vecDebugMuzzle + ( vecTargetVel + vecResult ) * flDebugOverlayScale,
		                          flDebugOverlayWidth + 1.f, 50, 50, 50, 200,
		                          true, flDebugOverlayDuration );
		CFmtStr strDebugFinal( "Expected result speed: %f", (vecTargetVel + vecResult).Length() );
		NDebugOverlay::Text( vecDebugMuzzle + Vector( 0, 0, -2 ), strDebugFinal, true, flDebugOverlayDuration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::DeflectPlayer( CTFPlayer *pTarget, CTFPlayer *pOwner, Vector &vecForward )
{
	if ( pTarget->GetTeamNumber() == pOwner->GetTeamNumber() && pTarget != pOwner )
	{
		if ( pTarget->m_Shared.InCond( TF_COND_BURNING ) && CanAirBlastPutOutTeammate() )
		{
			ExtinguishPlayer( this, pOwner, pTarget, "tf_weapon_flamethrower" );

			// Return health to the Pyro. 
			// We may want to cap the amount of health per extinguish but for now lets test this
			int iRestoreHealthOnExtinguish = 0;
			CALL_ATTRIB_HOOK_INT( iRestoreHealthOnExtinguish, extinguish_restores_health );
			if ( iRestoreHealthOnExtinguish > 0 )
			{
				pOwner->TakeHealth( iRestoreHealthOnExtinguish, DMG_GENERIC );
				IGameEvent *healevent = gameeventmanager->CreateEvent( "player_healonhit" );
				if ( healevent )
				{
					healevent->SetInt( "amount", iRestoreHealthOnExtinguish );
					healevent->SetInt( "entindex", pOwner->entindex() );
					item_definition_index_t healingItemDef = INVALID_ITEM_DEF_INDEX;
					if ( GetAttributeContainer() && GetAttributeContainer()->GetItem() )
					{
						healingItemDef = GetAttributeContainer()->GetItem()->GetItemDefIndex();
					}
					healevent->SetInt( "weapon_def_index", healingItemDef );

					gameeventmanager->FireEvent( healevent ); 
				}
			}
		}

		float flGiveTeammateSpeedBoost = 0;
		CALL_ATTRIB_HOOK_FLOAT( flGiveTeammateSpeedBoost, airblast_give_teammate_speed_boost );
		if ( flGiveTeammateSpeedBoost > 0.f )
		{
			pTarget->m_Shared.AddCond( TF_COND_SPEED_BOOST, flGiveTeammateSpeedBoost );
			// give the owner extra time to catch up with faster class
			pOwner->m_Shared.AddCond( TF_COND_SPEED_BOOST, flGiveTeammateSpeedBoost + 1.f );
		}

		return false;
	}
	
	if ( CanAirBlastPushPlayer() )
	{
		if ( pTarget->m_Shared.IsImmuneToPushback() )
			return false;

		int iReverseBlast = 0;
		CALL_ATTRIB_HOOK_INT( iReverseBlast, reverse_airblast );

		// Against players, let's force the pyro to be actually looking at them.
		// We'll be a bit more laxed when it comes to aiming at rockets and grenades.
		Vector vecToTarget;

		if ( pTarget == pOwner )
		{
			vecToTarget = vecForward;
		}
		else
		{
			vecToTarget = pTarget->WorldSpaceCenter() - pOwner->WorldSpaceCenter();
			VectorNormalize( vecToTarget );
		}

		float flAirblastConeScale = 1.f;
		CALL_ATTRIB_HOOK_FLOAT( flAirblastConeScale, mult_airblast_cone_scale );

		truncatedcone_t testCone;
		testCone.origin	= pOwner->EyePosition();
		testCone.normal	= vecForward;
		testCone.h		= 2.f * GetDeflectionRadius(); // diameter of enum sphere
		testCone.theta	= flAirblastConeScale * tf_flamethrower_airblast_cone_angle;


		Vector vTargetAbsMins = pTarget->GetAbsOrigin() + pTarget->WorldAlignMins();
		Vector vTargetAbsMaxs = pTarget->GetAbsOrigin() + pTarget->WorldAlignMaxs();

		// Require our target be in a cone in front of us
		if ( !physcollision->IsBoxIntersectingCone( vTargetAbsMins, vTargetAbsMaxs, testCone ) )
		{
			return false;
		}


		if ( pTarget != pOwner )
		{
			int nNoViewpunch = 0;
			CALL_ATTRIB_HOOK_INT( nNoViewpunch, airblast_pushback_no_viewpunch );
			if ( nNoViewpunch == 0 )
			{
				pTarget->ApplyPunchImpulseX( RandomInt( 10, 15 ) );
			}
		}

		pTarget->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:0,victim:1" );

		//
		// Apply force - Old & new modes
		//

		int nOldAirblast = 0;
		if ( !nOldAirblast && tf_airblast_cray.GetBool() )
		{
			// TODO This is not honoring some of the attributes of old airblast
			Vector vecPushDirection;
			Vector vecForce;
			if ( !tf_airblast_cray_pitch_control.GetBool() )
			{
				QAngle angForward;
				QAngle angToTarget;
				VectorAngles( vecForward, angForward );
				VectorAngles( vecToTarget, angToTarget );
				angForward[YAW] = angToTarget[YAW];
				AngleVectors( angForward, &vecPushDirection, nullptr, nullptr );
			}
			else
			{
				vecPushDirection = vecForward;
			}

			ComputeCrayAirBlastForce( pTarget, pOwner, vecPushDirection, /* out */ vecForce );

			// This is bypassing ApplyGenericPushbackImpulse because it implements its own pushback logic.
			pTarget->RemoveFlag( FL_ONGROUND );
			pTarget->SetGroundEntity( NULL ); // We'll restick if necessary, but we want to bypass the gamemovement
											  // requirements for un-sticking.
			// Only apply stun if we're about to apply knocked into air initially
			if ( !pTarget->m_Shared.InCond( TF_COND_KNOCKED_INTO_AIR ) )
			{
				int nNoStun = 0;
				CALL_ATTRIB_HOOK_INT( nNoStun, airblast_pushback_no_stun );
				if ( !nNoStun )
				{
					float flStunDuration = tf_airblast_cray_stun_duration.GetFloat();
					float flStunAmount = tf_airblast_cray_stun_amount.GetFloat();
					if ( flStunDuration > 0.f && flStunAmount > 0.f )
						{ pTarget->m_Shared.StunPlayer( flStunDuration, flStunAmount, TF_STUN_MOVEMENT, pOwner ); }
				}
			}
			float flLoseFooting = tf_airblast_cray_lose_footing_duration.GetFloat();
			if ( flLoseFooting )
				{ pTarget->m_Shared.AddCond( TF_COND_LOST_FOOTING, flLoseFooting ); }
			pTarget->m_Shared.AddCond( TF_COND_AIR_CURRENT );
			pTarget->m_Shared.AddCond( TF_COND_KNOCKED_INTO_AIR );
			pTarget->ApplyAbsVelocityImpulse( vecForce );
		}
		else
		{

			//
			// Old airblast, most of this logic is in ApplyGenericPushbackImpulse, which is used by other things, so
			// keeping it working is fairly easy for now.
			//

			// Apply stun, unless they are already in the air (only when leaving ground for the first time)
			int nNoStun = 0;
			CALL_ATTRIB_HOOK_INT( nNoStun, airblast_pushback_no_stun );
			if ( nNoStun == 0 )
			{
				if ( !pTarget->m_Shared.InCond( TF_COND_KNOCKED_INTO_AIR ) )
				{
					// Old airblast stun values
					pTarget->m_Shared.StunPlayer( 0.5f, 1.f, TF_STUN_MOVEMENT, pOwner );
				}
			}

			float flForce = AirBurstDamageForce( pTarget->WorldAlignSize(), 60, 6.f );

			CALL_ATTRIB_HOOK_FLOAT( flForce, airblast_pushback_scale );

#ifdef _DEBUG
			Vector vecForce = vecToTarget * flForce * tf_pushbackscalescale.GetFloat();
#else
			Vector vecForce = vecToTarget * flForce;	
#endif

			if ( iReverseBlast )
			{
				vecForce = -vecForce;
			}

			float flVerticalPushbackScale = tf_flamethrower_burst_zvelocity.GetFloat();
			if ( iReverseBlast )
			{
				// Don't give quite so big a vertical kick if we're sucking rather than blowing...
				flVerticalPushbackScale *= 0.75f;
			}

			{
				CALL_ATTRIB_HOOK_FLOAT( flVerticalPushbackScale, airblast_vertical_pushback_scale );
			}

#ifdef _DEBUG
			vecForce.z += flVerticalPushbackScale * tf_pushbackscalescale_vertical.GetFloat();

			/*
			// Kyle says: this will force players off the ground for at least one frame.
			//			  This is disabled on purpose right now to match previous flamethrower functionality.
			if ( pTarget->GetFlags() & FL_ONGROUND )
			{
			vecForce.z += 268.3281572999747f;
			}
			*/
#else
			vecForce.z += flVerticalPushbackScale;
#endif

			// Old airblast only - stomp velocity before applying
			pTarget->SetAbsVelocity( vec3_origin );

			// Apply GenericPushback
			pTarget->ApplyGenericPushbackImpulse( vecForce, pOwner );
		}


		// Make sure we get credit for the airblast if the target falls to its death
		pTarget->m_AchievementData.AddDamagerToHistory( pOwner );

		SendObjectDeflectedEvent( pOwner, pTarget, TF_WEAPON_NONE, pTarget ); // TF_WEAPON_NONE means the player got pushed

		// If the target is charging, stop the charge and keep the charge meter where it is.
		pTarget->m_Shared.InterruptCharge();

		// Track for achievements
		pTarget->m_AchievementData.AddPusherToHistory( pOwner );

		// Give bonus points whenever a pyro pushes high-value targets back
		if ( TFGameRules() && ( pTarget->IsMiniBoss() || pTarget->m_Shared.IsInvulnerable() ) )
		{
			int nAmount = pTarget->IsMiniBoss() ? 10 : 5;
			CTF_GameStats.Event_PlayerAwardBonusPoints( pOwner, pTarget, nAmount );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::PlayDeflectionSound( bool bPlayer )
{
	if ( bPlayer )
	{
		EmitSound( "TFPlayer.AirBlastImpact" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlameThrower::GetInitialAfterburnDuration() const
{
	return tf_flamethrower_initial_afterburn_duration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlameThrower::GetAfterburnRateOnHit() const
{
	float flAfterburnDurationScale = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flAfterburnDurationScale, afterburn_duration_mult );

	return flAfterburnDurationScale * tf_flamethrower_afterburn_rate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::DeflectEntity( CBaseEntity *pTarget, CTFPlayer *pOwner, Vector &vecForward )
{
	// pTarget shouldn't be player. Call DeflectPlayer instead
	Assert( pTarget && !pTarget->IsPlayer() );
	Assert( pOwner );

	int iAirblastDestroyProjectile = 0;
	CALL_ATTRIB_HOOK_INT( iAirblastDestroyProjectile, airblast_destroy_projectile );
	if ( iAirblastDestroyProjectile )
	{
		CBaseProjectile *pProjectile = dynamic_cast< CBaseProjectile* >( pTarget );
		if ( !pProjectile || !pProjectile->IsDestroyable() )
			return false;

		pProjectile->Destroy( false, true );
		EmitSound( "Halloween.HeadlessBossAxeHitWorld" );	// Todo: need a sound for stopping a projectile
		CTF_GameStats.Event_PlayerAwardBonusPoints( pOwner, NULL, 2 );

		return true;
	}

	if ( !CanAirBlastDeflectProjectile() )
		return false;

	// can't deflect things on our own team
	// except the passtime ball when in passtime mode
	if ( (pTarget->GetTeamNumber() == pOwner->GetTeamNumber()) 
		&& !(g_pPasstimeLogic && (g_pPasstimeLogic->GetBall() == pTarget)) )
	{
		return false;
	}

	// Grab the owner of the projectile *before* we reflect it.
	CTFPlayer *pTFPlayerVictim = dynamic_cast< CTFPlayer * >( pTarget->GetOwnerEntity() );
	if ( !pTFPlayerVictim )
	{
		// We can't use OwnerEntity for grenades, because then the owner can't shoot them with his hitscan weapons (due to collide rules)
		// Thrower is used to store the person who threw the grenade, for damage purposes.
		CBaseGrenade *pBaseGrenade = dynamic_cast< CBaseGrenade* >( pTarget );
		if ( pBaseGrenade )
		{
			pTFPlayerVictim = dynamic_cast< CTFPlayer * >( pBaseGrenade->GetThrower() );
		}
	}
	if ( !pTFPlayerVictim )
	{
		// Is the OwnerEntity() a base object, like a sentry gun shooting rockets at us?
		if ( pTarget->GetOwnerEntity() && pTarget->GetOwnerEntity()->IsBaseObject() )
		{
			CBaseObject *pObj = dynamic_cast< CBaseObject * >( pTarget->GetOwnerEntity() );
			if ( pObj )
			{
				pTFPlayerVictim = dynamic_cast< CTFPlayer * >( pObj->GetOwner() );
			}
		}
	}

	bool bDeflected = BaseClass::DeflectEntity( pTarget, pOwner, vecForward );
	if ( bDeflected )
	{
		int iAirblastTurnProjectileToAmmo = 0;
		CALL_ATTRIB_HOOK_INT( iAirblastTurnProjectileToAmmo, airblast_turn_projectile_to_ammo );
		if ( iAirblastTurnProjectileToAmmo )
		{
			pOwner->DropAmmoPackFromProjectile( pTarget );
		}
		else
		{
			pTarget->EmitSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
			EconEntity_OnOwnerKillEaterEvent( this, pOwner, pTFPlayerVictim, kKillEaterEvent_ProjectileReflect );
		}
	}
	return bDeflected;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		// If we were firing, stop
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
			SetWeaponState( FT_STATE_IDLE );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle at it appears visually
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetVisualMuzzlePos()
{
	return GetMuzzlePosHelper( true );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position at which to spawn flame damage entities
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetFlameOriginPos()
{
	return GetMuzzlePosHelper( false );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlameThrower::GetFlameHitRatio( void )
{
	// Safety net to avoid divide by zero
	if ( m_iActiveFlames == 0 )
		return 0.1f;

	float flRatio = ( ( (float)m_iDamagingFlames ) / ( (float)m_iActiveFlames ) );
	//Msg( "Act:  %d  Dmg:  %d\n", m_iActiveFlames, m_iDamagingFlames );

	return flRatio;
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameThrower::IncrementFlameDamageCount( void )
{
	m_iDamagingFlames++;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameThrower::DecrementFlameDamageCount( void )
{
	if ( m_iDamagingFlames <= 0 )
		return;

	m_iDamagingFlames--;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameThrower::IncrementActiveFlameCount( void )
{
	m_iActiveFlames++;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameThrower::DecrementActiveFlameCount( void )
{
	if ( m_iActiveFlames <= 0 )
		return;

	m_iActiveFlames--;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameThrower::ResetFlameHitCount( void )
{
	m_iDamagingFlames = 0;
	m_iActiveFlames = 0;
}


//-----------------------------------------------------------------------------
// Purpose: UI Progress
//-----------------------------------------------------------------------------
float CTFFlameThrower::GetProgress( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return 0.f;

	return pPlayer->m_Shared.GetRageMeter() / 100.0f;
}


//-----------------------------------------------------------------------------
// Purpose: UI Progress (same as GetProgress() without the division by 100.0f)
//-----------------------------------------------------------------------------
bool CTFFlameThrower::IsRageFull( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	return ( pPlayer->m_Shared.GetRageMeter() >= 100.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlameThrower::EffectMeterShouldFlash( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	if ( pPlayer && (IsRageFull() || pPlayer->m_Shared.IsRageDraining()) )
		return true;
	else
		return false;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetMuzzlePosHelper( bool bVisualPos )
{
	Vector vecMuzzlePos;
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner ) 
	{
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pOwner->GetNetworkEyeAngles(), &vecForward, &vecRight, &vecUp );
		{
			Vector vecOffset;
			UTIL_StringToVector( vecOffset.Base(), tf_flamethrower_new_flame_offset.GetString() );

			vecOffset *= pOwner->GetModelScale();
			vecMuzzlePos = pOwner->EyePosition() + vecOffset.x * vecForward + vecOffset.y * vecRight + vecOffset.z * vecUp;
		}
	}
	return vecMuzzlePos;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::CalculateHalloweenSpell( void )
{
	m_bHasHalloweenSpell.Set( false );
	if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
	{
		int iHalloweenSpell = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( this, iHalloweenSpell, halloween_green_flames );
		m_bHasHalloweenSpell.Set( iHalloweenSpell > 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::Deploy( void )
{
#if defined( CLIENT_DLL )
	StartPilotLight();
	m_flFlameHitRatio = 0;
	m_flPrevFlameHitRatio = -1;
	m_flChargeBeginTime = 0;

	m_bEffectsThinking = true;
	SetContextThink( &CTFFlameThrower::ClientEffectsThink, gpGlobals->curtime, "EFFECTS_THINK" );

	StopFullCritEffect();
#endif // CLIENT_DLL

	CalculateHalloweenSpell();

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlameThrower::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "recalculate_holidays" ) )
	{
		CalculateHalloweenSpell();
	}
}

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	C_TFPlayer *pPlayerOwner = GetTFPlayerOwner();

	//
	bool bLocalPlayerAmmo = true;

	if ( pPlayerOwner == pLocalPlayer )
	{
		bLocalPlayerAmmo = GetPlayerOwner()->GetAmmoCount( m_iPrimaryAmmoType ) > 0;
	}

	if ( IsCarrierAlive() && ( WeaponState() == WEAPON_IS_ACTIVE ) && bLocalPlayerAmmo == true )
	{
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			if ( ( m_iWeaponState == FT_STATE_SECONDARY && GetPlayerOwner() != C_BasePlayer::GetLocalPlayer() ) || m_iWeaponState != FT_STATE_SECONDARY )
			{
				StartFlame();
			}
		}
		else
		{
			StartPilotLight();
		}
	}
	else 
	{
		StopFlame();
		StopPilotLight();
		StopFullCritEffect();
		m_bEffectsThinking = false;
	}

	if ( pPlayerOwner == pLocalPlayer )
	{
		if ( m_pFiringLoop )
		{
 			m_flFlameHitRatio = GetFlameHitRatio();
 			m_flFlameHitRatio = RemapValClamped( m_flFlameHitRatio, 0.0f, 1.0f, 1.0f, 100.f );

			// Msg ( "%f\n", m_flFlameHitRatio );

			if ( m_flFlameHitRatio != m_flPrevFlameHitRatio )
			{
				m_flPrevFlameHitRatio = m_flFlameHitRatio;

				CLocalPlayerFilter filter;
				CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

				// We play accent sounds based on accuracy
				if ( m_flFlameHitRatio >= TF_FLAMETHROWER_HITACCURACY_HIGH )
				{
					controller.SoundChangePitch( m_pFiringLoop, 140, 0.1 );	
					m_szAccuracySound = "Weapon_FlameThrower.FireHitHard";
				}
				else
				{
					controller.SoundChangePitch( m_pFiringLoop, 100, 0.1 );	

					// If our accuracy is too low
					if ( m_pFiringAccuracyLoop )
					{
						controller.SoundDestroy( m_pFiringAccuracyLoop );
						m_pFiringAccuracyLoop = NULL;					
					}

					return;
				}

				// Only start a new sound if there's been a change
				if ( !m_pFiringAccuracyLoop )
				{
					m_pFiringAccuracyLoop = controller.SoundCreate( filter, entindex(), m_szAccuracySound );
					controller.Play( m_pFiringAccuracyLoop, 1.0, 100 );
				}

			}
		}
		else if ( m_pFiringAccuracyLoop )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringAccuracyLoop );
			m_pFiringAccuracyLoop = NULL;
		}
	}

	if ( GetBuffType() > 0 )
	{
		if ( !m_bFullRageEffect && pPlayerOwner && pPlayerOwner->m_Shared.GetRageMeter() >= 100.0f )
		{
			m_bFullRageEffect = true;
			m_MmmmphEffect.StartEffects( pPlayerOwner, FullCritChargedEffectName() );
		}
		else if ( m_bFullRageEffect && pPlayerOwner && pPlayerOwner->m_Shared.GetRageMeter() < 100.0f )
		{
			StopFullCritEffect();
			m_MmmmphEffect.StopEffects();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		if ( !IsDormant() && bDormant )
		{
			StopFlame();
			StopPilotLight();
			StopFullCritEffect();
			m_bEffectsThinking = false;
		}
	}

	// Deliberately skip base combat weapon to avoid being holstered
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFFlameThrower::GetWorldModelIndex( void )
{
	// Pyro bubble wand support.
	if ( GetFlameThrowerMode() == TF_FLAMETHROWER_MODE_RAINBOW )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) && pPlayer->m_Shared.GetTauntIndex() == TAUNT_BASE_WEAPON )
		{
			// While we are taunting, replace our normal world model with the bubble wand.
			m_iWorldModelIndex = modelinfo->GetModelIndex( TF_WEAPON_BUBBLE_WAND_MODEL );
			return m_iWorldModelIndex;
		}
	}

	return BaseClass::GetWorldModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StartFlame()
{
	if ( m_iWeaponState == FT_STATE_SECONDARY )
	{
		GetAppropriateWorldOrViewModel()->ParticleProp()->Create( "pyro_blast", PATTACH_POINT_FOLLOW, "muzzle" );
		CLocalPlayerFilter filter;
		const char *shootsound = GetShootSound( WPN_DOUBLE );
		EmitSound( filter, entindex(), shootsound );

		return;
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// normally, crossfade between start sound & firing loop in 3.5 sec
	float flCrossfadeTime = 3.5;

	if ( m_pFiringLoop && ( m_bCritFire != m_bFiringLoopCritical ) )
	{
		// If we're firing and changing between critical & noncritical, just need to change the firing loop.
		// Set crossfade time to zero so we skip the start sound and go to the loop immediately.

		flCrossfadeTime = 0;
		StopFlame( true );
	}

	StopPilotLight();

	if ( !m_pFiringStartSound && !m_pFiringLoop )
	{
		// NVNT if the local player is owning this weapon, process the start event
		if ( C_BasePlayer::GetLocalPlayer() == GetOwner() && haptics )
			haptics->ProcessHapticEvent(2,"Weapons","flamer_start");

		RestartParticleEffect();
		CLocalPlayerFilter filter;

		// Play the fire start sound
		const char *shootsound = GetShootSound( SINGLE );
		if ( flCrossfadeTime > 0.0 )
		{
			// play the firing start sound and fade it out
			m_pFiringStartSound = controller.SoundCreate( filter, entindex(), shootsound );		
			controller.Play( m_pFiringStartSound, 1.0, 100 );
			controller.SoundChangeVolume( m_pFiringStartSound, 0.0, flCrossfadeTime );
		}

		// Start the fire sound loop and fade it in
		if ( m_bCritFire )
		{
			shootsound = GetShootSound( BURST );
		}
		else
		{
			shootsound = GetShootSound( SPECIAL1 );
		}
		m_pFiringLoop = controller.SoundCreate( filter, entindex(), shootsound );
		m_bFiringLoopCritical = m_bCritFire;

		// play the firing loop sound and fade it in
		if ( flCrossfadeTime > 0.0 )
		{
			controller.Play( m_pFiringLoop, 0.0, 100 );
			controller.SoundChangeVolume( m_pFiringLoop, 1.0, flCrossfadeTime );
		}
		else
		{
			controller.Play( m_pFiringLoop, 1.0, 100 );
		}
	}

	// check our "hit" sound
	if ( m_bHitTarget != m_bFiringHitTarget )
	{
		if ( !m_bHitTarget )
		{
			StopHitSound();
		}
		else
		{
			char *pchFireHitSound = "Weapon_FlameThrower.FireHit";

			if ( GetFlameThrowerMode() == TF_FLAMETHROWER_MODE_RAINBOW )
			{
				pchFireHitSound = "Weapon_Rainblower.FireHit";
			}

			CLocalPlayerFilter filter;
			m_pFiringHitLoop = controller.SoundCreate( filter, entindex(), pchFireHitSound );	
			controller.Play( m_pFiringHitLoop, 1.0, 100 );
		}

		m_bFiringHitTarget = m_bHitTarget;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopHitSound()
{
	if ( m_pFiringHitLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringHitLoop );
		m_pFiringHitLoop = NULL;
	}

	m_bHitTarget = m_bFiringHitTarget = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopFlame( bool bAbrupt /* = false */ )
{
	if ( ( m_pFiringLoop || m_pFiringStartSound ) && !bAbrupt )
	{
		// play a quick wind-down poof when the flame stops
		CLocalPlayerFilter filter;
		const char *shootsound = GetShootSound( SPECIAL3 );
		EmitSound( filter, entindex(), shootsound );
	}

	if ( m_pFiringLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}

	if ( m_pFiringStartSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}

	if ( m_FlameEffects.StopEffects() )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer && pLocalPlayer == GetOwner() )
		{
			// NVNT local player is finished firing. send the stop event.
			if ( haptics )
				haptics->ProcessHapticEvent(2,"Weapons","flamer_stop");
		}
	}

	if ( !bAbrupt )
	{
		StopHitSound();
	}

	m_iParticleWaterLevel = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StartPilotLight()
{
	if ( !m_pPilotLightSound )
	{
		StopFlame();

		// Create the looping pilot light sound
		const char *pilotlightsound = GetShootSound( SPECIAL2 );
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pPilotLightSound = controller.SoundCreate( filter, entindex(), pilotlightsound );

		controller.Play( m_pPilotLightSound, 1.0, 100 );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopPilotLight()
{
	if ( m_pPilotLightSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopFullCritEffect()
{
	m_bFullRageEffect = false;

	m_MmmmphEffect.StopEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::RestartParticleEffect( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( m_iWeaponState != FT_STATE_FIRING && m_iWeaponState != FT_STATE_STARTFIRING )
	{
		return;
	}

	m_iParticleWaterLevel = pOwner->GetWaterLevel();

	m_FlameEffects.StartEffects( pOwner, GetParticleEffectName() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFFlameThrower::FlameEffectName( bool bIsFirstPersonView )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return NULL;

	// Halloween Spell
	if ( m_bHasHalloweenSpell )
	{
		return "flamethrower_halloween_new_flame";
	}

	switch ( GetFlameThrowerMode() )
	{
	case TF_FLAMETHROWER_MODE_PHLOG:	return "drg_phlo_stream_new_flame";
	case TF_FLAMETHROWER_MODE_GIANT:	return "flamethrower_giant_mvm_new_flame";
	case TF_FLAMETHROWER_MODE_RAINBOW:	return "flamethrower_rainbow_new_flame";
	default:							
		{
			return GetNewFlameEffectInternal( pOwner->GetTeamNumber(), false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFFlameThrower::FlameCritEffectName( bool bIsFirstPersonView )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return NULL;

	// Halloween Spell
	if ( m_bHasHalloweenSpell )
	{
		return ( pOwner->GetTeamNumber() == TF_TEAM_BLUE ? "flamethrower_halloween_crit_blue_new_flame" : "flamethrower_halloween_crit_red_new_flame" );
	}

	switch ( GetFlameThrowerMode() )
	{
	case TF_FLAMETHROWER_MODE_PHLOG:	return "drg_phlo_stream_crit_new_flame";
	case TF_FLAMETHROWER_MODE_GIANT:	return "flamethrower_crit_giant_mvm_new_flame";
	case TF_FLAMETHROWER_MODE_RAINBOW:	return "flamethrower_rainbow_new_flame";
	default:
		{
			return GetNewFlameEffectInternal( pOwner->GetTeamNumber(), true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFFlameThrower::FullCritChargedEffectName( void )
{
	switch( GetTeamNumber() )
	{
	case TF_TEAM_BLUE:	return "medicgun_invulnstatus_fullcharge_blue";
	case TF_TEAM_RED:	return "medicgun_invulnstatus_fullcharge_red";
	default:			return "";
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFFlameThrower::GetParticleEffectName( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return NULL;

	const char *pszParticleEffect = NULL;

	bool bIsFirstPersonView = IsFirstPersonView();

	// Start the appropriate particle effect
	if ( pOwner->GetWaterLevel() == WL_Eyes )
	{
		pszParticleEffect = "flamethrower_underwater";
	}
	else
	{
		if ( m_bCritFire )
		{
			pszParticleEffect = FlameCritEffectName( bIsFirstPersonView );
		}
		else 
		{
			pszParticleEffect = FlameEffectName( bIsFirstPersonView );
		}
	}

	return pszParticleEffect;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::ClientEffectsThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( !m_bEffectsThinking )
		return;

	float flRageInverse = 1.f;

	if ( GetBuffType() > 0 )
	{
		flRageInverse = 1.0f - ( pPlayer->m_Shared.GetRageMeter() / 100.0f );
		if ( flRageInverse < 1.0f )
		{
			// We have some rage, let's spark!
			ParticleProp()->Init( this );
			CNewParticleEffect* pEffect = ParticleProp()->Create( "drg_bison_idle", PATTACH_POINT_FOLLOW, "muzzle" );
			if ( pEffect )
			{
				pEffect->SetControlPoint( CUSTOM_COLOR_CP1, GetParticleColor( 1 ) );
				pEffect->SetControlPoint( CUSTOM_COLOR_CP2, GetParticleColor( 2 ) );
			}
		}
	}

	SetContextThink( &CTFFlameThrower::ClientEffectsThink, gpGlobals->curtime + 0.1f + RandomFloat( 1.0f, 5.0f ) * flRageInverse, "EFFECTS_THINK" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::FlameEffect_t::StartEffects( CTFPlayer *pTFOwner, const char *pszEffectName )
{
	if ( !pTFOwner )
		return;

	if ( !pszEffectName )
		return;

	m_pOwner = pTFOwner;

	// Stop any old flame effects
	StopEffects();

	// Figure out which weapon this flame effect is to be attached to.  Store this for
	// later so we know which weapon to deactivate the effect on
	m_hEffectWeapon = m_pFlamethrower->GetWeaponForEffect();

	if ( m_hEffectWeapon )
	{
		CParticleProperty* pParticleProp = m_hEffectWeapon->ParticleProp();
		if ( pParticleProp )
		{
			// Flame on
			m_pFlameEffect = pParticleProp->Create( pszEffectName, PATTACH_POINT_FOLLOW, "muzzle" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::FlameEffect_t::StopEffects()
{
	bool bStopped = false;
	// Stop any old flame effects
	if ( m_pFlameEffect && m_hEffectWeapon )
	{
		m_hEffectWeapon->ParticleProp()->StopEmission( m_pFlameEffect );
		bStopped = true;
	}

	m_pFlameEffect = NULL;
	m_hEffectWeapon = NULL;

	return bStopped;
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::HitTargetThink( void )
{
	if ( ( m_flTimeToStopHitSound > 0 ) && ( m_flTimeToStopHitSound < gpGlobals->curtime ) )
	{
		m_bHitTarget = false;
		m_flTimeToStopHitSound = 0;
		SetContextThink( NULL, 0, s_pszFlameThrowerHitTargetThink );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1f, s_pszFlameThrowerHitTargetThink );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SetHitTarget( void )
{
	if ( m_iWeaponState > FT_STATE_IDLE )
	{
		m_bHitTarget = true;
		m_flTimeToStopHitSound = gpGlobals->curtime + 0.2;

		// Start the hit target thinking
		SetContextThink( &CTFFlameThrower::HitTargetThink, gpGlobals->curtime + 0.1f, s_pszFlameThrowerHitTargetThink );
	}
}

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFFlameRocket, DT_TFFlameRocket )
BEGIN_NETWORK_TABLE( CTFFlameRocket, DT_TFFlameRocket )
END_NETWORK_TABLE()

#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( tf_flame, CTFFlameEntity );
IMPLEMENT_AUTO_LIST( ITFFlameEntityAutoList );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlameEntity::CTFFlameEntity()
{}

//-----------------------------------------------------------------------------
// Purpose: Spawns this entity
//-----------------------------------------------------------------------------
void CTFFlameEntity::Spawn( void )
{
	BaseClass::Spawn();

	// don't collide with anything, we do our own collision detection in our think method
	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	// move noclip: update position from velocity, that's it
	SetMoveType( MOVETYPE_NOCLIP, MOVECOLLIDE_DEFAULT );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );

	float iBoxSize = tf_flamethrower_boxsize.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwnerEntity(), iBoxSize, mult_flame_size );
	UTIL_SetSize( this, -Vector( iBoxSize, iBoxSize, iBoxSize ), Vector( iBoxSize, iBoxSize, iBoxSize ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	m_vecInitialPos = GetAbsOrigin();
	m_vecPrevPos = m_vecInitialPos;

	// Track total active flame entities
	m_hFlameThrower = dynamic_cast< CTFFlameThrower* >( GetOwnerEntity() );
	if ( m_hFlameThrower )
	{
		m_hFlameThrower->IncrementActiveFlameCount();
		m_bBurnedEnemy = false;

		float flFlameLife = tf_flamethrower_flametime.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetOwnerEntity(), flFlameLife, mult_flame_life );
		m_flTimeRemove = gpGlobals->curtime + ( flFlameLife * random->RandomFloat( 0.9f, 1.1f ) );
	}
	else
	{
		m_flTimeRemove = gpGlobals->curtime + 3.f;
	}

	// Setup the think function.
	SetThink( &CTFFlameEntity::FlameThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Creates an instance of this entity
//-----------------------------------------------------------------------------
CTFFlameEntity *CTFFlameEntity::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, float flSpeed, int iDmgType, float flDmgAmount, bool bAlwaysCritFromBehind, bool bRandomize )
{
	CTFFlameEntity *pFlame = static_cast<CTFFlameEntity*>( CBaseEntity::Create( "tf_flame", vecOrigin, vecAngles, pOwner ) );
	if ( !pFlame )
		return NULL;

	// Initialize the owner.
	pFlame->SetOwnerEntity( pOwner );
	if ( pOwner->GetOwnerEntity() )
		pFlame->m_hAttacker = pOwner->GetOwnerEntity();
	else
		pFlame->m_hAttacker = pOwner;

	// Set team.
	pFlame->ChangeTeam( pOwner->GetTeamNumber() );
	pFlame->m_iDmgType = iDmgType;
	pFlame->m_flDmgAmount = flDmgAmount;

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float flFlameLifeMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pFlame->m_hAttacker, flFlameLifeMult, mult_flame_life );
	float velocity = flFlameLifeMult * flSpeed;
	pFlame->m_vecBaseVelocity = vecForward * velocity;
	float iFlameSizeMult = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pFlame->m_hAttacker, iFlameSizeMult, mult_flame_size );
	if ( bRandomize )
	{
		pFlame->m_vecBaseVelocity += RandomVector( -velocity * iFlameSizeMult * tf_flamethrower_vecrand.GetFloat(), velocity * iFlameSizeMult * tf_flamethrower_vecrand.GetFloat() );
	}
	if ( pOwner->GetOwnerEntity() )
	{
		pFlame->m_vecAttackerVelocity = pOwner->GetOwnerEntity()->GetAbsVelocity();
	}
	pFlame->SetAbsVelocity( pFlame->m_vecBaseVelocity );	
	// Setup the initial angles.
	pFlame->SetAbsAngles( vecAngles );
	pFlame->SetCritFromBehind( bAlwaysCritFromBehind );

	return pFlame;
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
void CTFFlameEntity::FlameThink( void )
{
	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 )
	// if we've expired, remove ourselves
	if ( gpGlobals->curtime >= m_flTimeRemove )
	{
		RemoveFlame();
		return;
	}
	else
	{
		// Always think, if we haven't died due to our timeout.
		SetNextThink( gpGlobals->curtime );
	}

	// Did we move? should we check collision?
	if ( GetAbsOrigin() != m_vecPrevPos )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s Collision", __FUNCTION__ );
		CTFPlayer *pAttacker = dynamic_cast<CTFPlayer *>( (CBaseEntity *) m_hAttacker );
		if ( !pAttacker )
			return;

		// Create a ray for flame entity to trace
		Ray_t rayWorld;
		rayWorld.Init( m_vecInitialPos, GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs() );

		// check against world first
		// if we collide with world, just destroy the flame
		trace_t trWorld;
		UTIL_TraceRay( rayWorld, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );

		bool bHitWorld = trWorld.startsolid || trWorld.fraction < 1.f;

		// update the ray
		Ray_t rayEnt;
		rayEnt.Init( m_vecPrevPos, GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs() );

		// burn all entities that we should collide with
		CFlameEntityEnum eFlameEnum( pAttacker );
		enginetrace->EnumerateEntities( rayEnt, false, &eFlameEnum );

		bool bHitSomething = false;
		FOR_EACH_VEC( eFlameEnum.GetTargets(), i )
		{
			CBaseEntity *pEnt = eFlameEnum.GetTargets()[i];

			// skip ent that's already burnt by this flame
			int iIndex = m_hEntitiesBurnt.Find( pEnt );
			if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
				continue;

			// if we're removing the flame this frame from hitting world, check if we hit this ent before hitting the world
			if ( bHitWorld )
			{
				trace_t trEnt;
				enginetrace->ClipRayToEntity( rayWorld, MASK_SOLID | CONTENTS_HITBOX, pEnt, &trEnt );
				// hit world before this ent, skip it
				if ( trEnt.fraction >= trWorld.fraction )
					continue;
			}

			// burn them all!
			if ( pEnt->IsPlayer() && pEnt->InSameTeam( pAttacker ) )
			{
				OnCollideWithTeammate( ToTFPlayer( pEnt ) );
			}
			else
			{
				OnCollide( pEnt );
			}

			bHitSomething = true;
		}

		// now, let's see if the flame visual could have actually hit this player.  Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything.
		if ( bHitSomething && tf_debug_flamethrower.GetBool() )
		{
			NDebugOverlay::SweptBox( m_vecPrevPos, GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), vec3_angle, 255, 255, 0, 100, 5.0 );
			NDebugOverlay::EntityBounds( this, 255, 255, 0, 100, 5.0 );
		}

		// remove the flame if it hits the world
		if ( bHitWorld )
		{
			if ( tf_debug_flamethrower.GetInt() )
			{
				NDebugOverlay::SweptBox( m_vecInitialPos, GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), vec3_angle, 255, 0, 0, 100, 3.0 );
			}

			RemoveFlame();
		}
	}

	// Reduce our base velocity by the air drag constant
	m_vecBaseVelocity *= GetFlameDrag();

	// Add our float upward velocity
	Vector vecVelocity = m_vecBaseVelocity + Vector( 0, 0, GetFlameFloat() ) + m_vecAttackerVelocity;

	// Update our velocity
	SetAbsVelocity( vecVelocity );
	
	// Render debug visualization if convar on
	if ( tf_debug_flamethrower.GetInt() )
	{
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			int val = ( (int) ( gpGlobals->curtime * 10 ) ) % 255;
			NDebugOverlay::EntityBounds(this, val, 255, val, 0 ,0 );
		} 
		else 
		{
			NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;
		}
	}

	m_vecPrevPos = GetAbsOrigin();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameEntity::SetHitTarget( void )
{
	if ( !m_hFlameThrower )
		return;

	m_hFlameThrower->SetHitTarget();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameEntity::RemoveFlame()
{
	UpdateFlameThrowerHitRatio();

	UTIL_Remove( this );
}


//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity
//-----------------------------------------------------------------------------
void CTFFlameEntity::OnCollide( CBaseEntity *pOther )
{
	int nContents = UTIL_PointContents( GetAbsOrigin() );
	if ( (nContents & MASK_WATER) )
	{
		RemoveFlame();
		return;
	}

	// remember that we've burnt this player
	m_hEntitiesBurnt.AddToTail( pOther );
	
	float flDistance = GetAbsOrigin().DistTo( m_vecInitialPos );
	float flDamage = m_flDmgAmount * RemapValClamped( flDistance, tf_flamethrower_maxdamagedist.GetFloat()/2, tf_flamethrower_maxdamagedist.GetFloat(), 1.0f, 0.70f );

	flDamage = MAX( flDamage, 1.0 );
	if ( tf_debug_flamethrower.GetInt() )
	{
		Msg( "Flame touch dmg: %.1f\n", flDamage );
	}

	CBaseEntity *pAttacker = m_hAttacker;
	if ( !pAttacker )
		return;

	SetHitTarget();

	int iDamageType = m_iDmgType;

	if ( pOther && pOther->IsPlayer() )
	{
		CTFPlayer *pVictim = ToTFPlayer( pOther );
		if ( IsBehindTarget( pOther ) )
		{
			if ( m_bCritFromBehind == true )
			{
				iDamageType |= DMG_CRITICAL;
			}

			if ( pVictim )
			{
				pVictim->HandleAchievement_Pyro_BurnFromBehind( ToTFPlayer( pAttacker ) );
			}
		}

		// Pyro-specific
		if ( pAttacker->IsPlayer() && pVictim )
		{
			CTFPlayer *pPlayerAttacker = ToTFPlayer( pAttacker );
			if ( pPlayerAttacker && pPlayerAttacker->IsPlayerClass( TF_CLASS_PYRO ) )
			{
				// burn the victim while taunting?
				if ( pVictim->m_Shared.InCond( TF_COND_TAUNTING ) )
				{
					static CSchemaItemDefHandle flipTaunt( "Flippin' Awesome Taunt" );
					// if I'm the one being flipped, and getting lit on fire
					if ( !pVictim->IsTauntInitiator() && pVictim->GetTauntEconItemView() && pVictim->GetTauntEconItemView()->GetItemDefinition() == flipTaunt )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_IGNITE_PLAYER_BEING_FLIPPED );
					}
				}
			}
		}
	}

	CTakeDamageInfo info( GetOwnerEntity(), pAttacker, GetOwnerEntity(), flDamage, iDamageType, TF_DMG_CUSTOM_BURNING );
	info.SetReportedPosition( pAttacker->GetAbsOrigin() );

	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		info.SetCritType( CTakeDamageInfo::CRIT_FULL );
	}

	// terrible hack for flames hitting the Merasmus props to get the particle effect in the correct position
	if ( TFGameRules() && TFGameRules()->GetActiveBoss() && ( TFGameRules()->GetActiveBoss()->GetBossType() == HALLOWEEN_BOSS_MERASMUS ) )
	{
		info.SetDamagePosition( GetAbsOrigin() );
	}

	// Track hits for the Flamethrower, which is used to change the weapon sound based on hit ratio
	if ( m_hFlameThrower )
	{
		m_bBurnedEnemy = true;
		m_hFlameThrower->IncrementFlameDamageCount();
	}

	// We collided with pOther, so try to find a place on their surface to show blood
	trace_t pTrace;
	UTIL_TraceLine( WorldSpaceCenter(), pOther->WorldSpaceCenter(), MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &pTrace );

	pOther->DispatchTraceAttack( info, GetAbsVelocity(), &pTrace );
	ApplyMultiDamage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameEntity::OnCollideWithTeammate( CTFPlayer *pPlayer )
{
	// Only care about Snipers
	if ( !pPlayer->IsPlayerClass(TF_CLASS_SNIPER) )
		return;

	int iIndex = m_hEntitiesBurnt.Find( pPlayer );
	if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
		return;

	m_hEntitiesBurnt.AddToTail( pPlayer );

	// Does he have the bow?
	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
	{
		CTFCompoundBow *pBow = static_cast<CTFCompoundBow*>( pWpn );
		pBow->SetArrowAlight( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameEntity::IsBehindTarget( CBaseEntity *pTarget )
{
	return ( DotProductToTarget( pTarget ) > 0.8 );
}

//-----------------------------------------------------------------------------
// Purpose: Utility to calculate dot product between facing angles of flame and target
//-----------------------------------------------------------------------------
float CTFFlameEntity::DotProductToTarget( CBaseEntity *pTarget )
{
	Assert( pTarget );

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;
	AngleVectors( pTarget->EyeAngles(), &vecVictimForward, NULL, NULL );
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	Vector vecTraveling = m_vecBaseVelocity;
	vecTraveling.z = 0.0f;
	vecTraveling.NormalizeInPlace();

	return DotProduct( vecVictimForward, vecTraveling );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameEntity::UpdateFlameThrowerHitRatio( void )
{
	if ( !m_hFlameThrower )
		return;
	
	if ( m_bBurnedEnemy )
	{
		m_hFlameThrower->DecrementFlameDamageCount();
	}

	m_hFlameThrower->DecrementActiveFlameCount();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlameEntity::GetFlameFloat( void )
{
	return tf_flamethrower_float.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFlameEntity::GetFlameDrag( void )
{
	return tf_flamethrower_drag.GetFloat();
}

#endif // GAME_DLL
