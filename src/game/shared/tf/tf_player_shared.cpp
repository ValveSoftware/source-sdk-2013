//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_player_shared.h"
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "effect_dispatch_data.h"
#include "tf_item.h"
#include "entity_capture_flag.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_invis.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_shovel.h"
#include "tf_weapon_sword.h"
#include "tf_weapon_shotgun.h"
#include "in_buttons.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_flaregun.h"
#include "tf_weapon_wrench.h"
#include "econ_wearable.h"
#include "econ_item_system.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_syringegun.h"
#include "tf_weapon_flamethrower.h"
#include "econ_entity_creation.h"
#include "tf_mapinfo.h"
#include "tf_dropped_weapon.h"
#include "tf_weapon_passtime_gun.h"
#include "tf_weapon_rocketpack.h"
#include <functional>

// Client specific.
#ifdef CLIENT_DLL
#include "c_baseviewmodel.h"
#include "c_tf_player.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_fx.h"
#include "soundenvelope.h"
#include "c_tf_playerclass.h"
#include "iviewrender.h"
#include "prediction.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "achievements_tf.h"
#include "c_tf_weapon_builder.h"
#include "dt_utlvector_recv.h"
#include "recvproxy.h"
#include "c_tf_weapon_builder.h"
#include "c_func_capture_zone.h"
#include "tf_hud_target_id.h"
#include "tempent.h"
#include "cam_thirdperson.h"
#include "vgui/IInput.h"

#define CTFPlayerClass C_TFPlayerClass
#define CCaptureZone C_CaptureZone
#define CRecipientFilter C_RecipientFilter

#include "c_tf_objective_resource.h"
#include "tf_weapon_buff_item.h"
#include "c_tf_passtime_logic.h"

// Server specific.
#else
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "util.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_playerclass.h"
#include "SpriteTrail.h"
#include "tf_weapon_builder.h"
#include "nav_mesh/tf_nav_area.h"
#include "nav_pathfind.h"
#include "tf_obj_dispenser.h"
#include "dt_utlvector_send.h"
#include "tf_item_wearable.h"
#include "NextBotManager.h"
#include "tf_weapon_builder.h"
#include "func_capture_zone.h"
#include "hl2orange.spa.h"
#include "bot/tf_bot.h"
#include "tf_objective_resource.h"
#include "halloween/tf_weapon_spellbook.h"
#include "tf_weapon_buff_item.h"
#include "tf_passtime_logic.h"
#include "tf_weapon_passtime_gun.h"
#include "entity_healthkit.h"
#include "halloween/merasmus/merasmus.h"
#include "tf_weapon_grapplinghook.h"
#include "tf_wearable_levelable_item.h"
#include "tf_weapon_rocketpack.h"
#include "tf_obj_sentrygun.h"
#include "func_respawnroom.h"
#endif

#include "tf_wearable_weapons.h"
#include "tf_weapon_bonesaw.h"

static ConVar tf_demoman_charge_frametime_scaling( "tf_demoman_charge_frametime_scaling", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "When enabled, scale yaw limiting based on client performance (frametime)." );
static const float YAW_CAP_SCALE_MIN = 0.2f;
static const float YAW_CAP_SCALE_MAX = 2.f;

ConVar tf_halloween_kart_boost_recharge( "tf_halloween_kart_boost_recharge", "5.0f", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_halloween_kart_boost_duration( "tf_halloween_kart_boost_duration", "1.5f", FCVAR_REPLICATED | FCVAR_CHEAT );

ConVar tf_scout_air_dash_count( "tf_scout_air_dash_count", "1", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_spy_invis_time( "tf_spy_invis_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );
ConVar tf_spy_invis_unstealth_time( "tf_spy_invis_unstealth_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );

ConVar tf_spy_max_cloaked_speed( "tf_spy_max_cloaked_speed", "999", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );	// no cap
ConVar tf_whip_speed_increase( "tf_whip_speed_increase", "105", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
ConVar tf_max_health_boost( "tf_max_health_boost", "1.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max health factor that players can be boosted to by healers.", true, 1.0, false, 0 );
ConVar tf_invuln_time( "tf_invuln_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Time it takes for invulnerability to wear off." );

extern ConVar tf_player_movement_restart_freeze;
extern ConVar mp_tournament_readymode_countdown;
extern ConVar tf_max_charge_speed;

ConVar tf_always_loser( "tf_always_loser", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Force loserstate to true." );

ConVar tf_mvm_bot_flag_carrier_movement_penalty( "tf_mvm_bot_flag_carrier_movement_penalty", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );

//ConVar tf_scout_dodge_move_penalty_duration( "tf_scout_dodge_move_penalty_duration", "3.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );
//ConVar tf_scout_dodge_move_penalty( "tf_scout_dodge_move_penalty", "0.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );


#ifdef GAME_DLL
ConVar tf_boost_drain_time( "tf_boost_drain_time", "15.0", FCVAR_DEVELOPMENTONLY, "Time is takes for a full health boost to drain away from a player.", true, 0.1, false, 0 );
#ifdef _DEBUG
CON_COMMAND_F( tf_add_bombhead, "Add Merasmus Bomb Head Condition", 0 )
{
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, true );

	FOR_EACH_VEC ( playerVector, i )
	{
		float flBuffDuration = 7.0f;
		playerVector[i]->m_Shared.StunPlayer( flBuffDuration, 0.f, TF_STUN_LOSER_STATE );
		playerVector[i]->m_Shared.AddCond( TF_COND_HALLOWEEN_BOMB_HEAD, flBuffDuration );
		playerVector[i]->m_Shared.AddCond( TF_COND_SPEED_BOOST, flBuffDuration );
		//playerVector[i]->m_Shared.AddCond( TF_COND_HALLOWEEN_BOMB_HEAD, 7 );
	}
}

ConVar tf_debug_bullets( "tf_debug_bullets", "0", FCVAR_DEVELOPMENTONLY, "Visualize bullet traces." );
#endif // _DEBUG

ConVar tf_damage_events_track_for( "tf_damage_events_track_for", "30",  FCVAR_DEVELOPMENTONLY );

extern ConVar tf_halloween_giant_health_scale;

ConVar tf_allow_sliding_taunt( "tf_allow_sliding_taunt", "0", FCVAR_NONE, "1 - Allow player to slide for a bit after taunting" );

#endif // GAME_DLL


ConVar tf_useparticletracers( "tf_useparticletracers", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use particle tracers instead of old style ones." );
ConVar tf_spy_cloak_consume_rate( "tf_spy_cloak_consume_rate", "10.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to use per second while cloaked, from 100 max )" );	// 10 seconds of invis
ConVar tf_spy_cloak_regen_rate( "tf_spy_cloak_regen_rate", "3.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to regen per second, up to 100 max" );		// 30 seconds to full charge
ConVar tf_spy_cloak_no_attack_time( "tf_spy_cloak_no_attack_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "time after uncloaking that the spy is prohibited from attacking" );
ConVar tf_tournament_hide_domination_icons( "tf_tournament_hide_domination_icons", "0", FCVAR_REPLICATED, "Tournament mode server convar that forces clients to not display the domination icons above players dominating them." );
ConVar tf_damage_disablespread( "tf_damage_disablespread", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Toggles the random damage spread applied to all player damage." );

ConVar tf_scout_energydrink_regen_rate( "tf_scout_energydrink_regen_rate", "3.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "energy drink regen per second, up to 100 max" );
ConVar tf_scout_energydrink_consume_rate( "tf_scout_energydrink_consume_rate", "12.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "energy drink to use per second while boosted, from 100 max" );
ConVar tf_scout_energydrink_activation( "tf_scout_energydrink_activation", "0.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "how long it takes for the energy buff to become active" );

ConVar tf_demoman_charge_regen_rate( "tf_demoman_charge_regen_rate", "8.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "" );
ConVar tf_demoman_charge_drain_time( "tf_demoman_charge_drain_time", "1.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "" );

// STAGING_SPY
ConVar tf_feign_death_duration( "tf_feign_death_duration", "3.0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Time that feign death buffs last." );
ConVar tf_feign_death_speed_duration( "tf_feign_death_speed_duration", "3.0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Time that feign death speed boost last." );

ConVar tf_allow_taunt_switch( "tf_allow_taunt_switch", "0", FCVAR_REPLICATED, "0 - players are not allowed to switch weapons while taunting, 1 - players can switch weapons at the start of a taunt (old bug behavior), 2 - players can switch weapons at any time during a taunt." );

ConVar tf_allow_all_team_partner_taunt( "tf_allow_all_team_partner_taunt", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

// AFTERBURN
const float tf_afterburn_max_duration = 10.f;
const float tf_afterburn_duration_ratio_second_degree = 0.4f;
const float tf_afterburn_duration_ratio_third_degree = 0.8f;
const float tf_afterburn_mult_second_degree = 1.f;
const float tf_afterburn_mult_third_degree = 1.f;
#ifdef DEBUG
ConVar tf_afterburn_debug( "tf_afterburn_debug", "0", FCVAR_REPLICATED | FCVAR_CHEAT );
#endif // DEBUG


#ifdef CLIENT_DLL
ConVar tf_colorblindassist( "tf_colorblindassist", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Setting this to 1 turns on colorblind mode." );

extern ConVar cam_idealdist;
extern ConVar cam_idealdistright;

#endif // CLIENT_DLL

extern ConVar tf_flamethrower_flametime;
extern ConVar weapon_medigun_chargerelease_rate;
#if defined( _DEBUG ) || defined( STAGING_ONLY )
extern ConVar mp_developer;
#endif // _DEBUG || STAGING_ONLY

//ConVar tf_spy_stealth_blink_time( "tf_spy_stealth_blink_time", "0.3", FCVAR_DEVELOPMENTONLY, "time after being hit the spy blinks into view" );
//ConVar tf_spy_stealth_blink_scale( "tf_spy_stealth_blink_scale", "0.85", FCVAR_DEVELOPMENTONLY, "percentage visible scalar after being hit the spy blinks into view" );
#define TF_SPY_STEALTH_BLINKTIME   0.3f
#define TF_SPY_STEALTH_BLINKSCALE  0.85f

#define TF_BUILDING_PICKUP_RANGE 150
#define TF_BUILDING_RESCUE_MIN_RANGE_SQ 62500  //250 * 250
#define TF_BUILDING_RESCUE_MAX_RANGE 5500

#define TF_PLAYER_CONDITION_CONTEXT	"TFPlayerConditionContext"

#define TF_SCREEN_OVERLAY_MATERIAL_BURNING		"effects/imcookin" 
#define TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED	"effects/invuln_overlay_red" 
#define TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE	"effects/invuln_overlay_blue" 

#define TF_SCREEN_OVERLAY_MATERIAL_MILK				"effects/milk_screen" 
#define TF_SCREEN_OVERLAY_MATERIAL_URINE			"effects/jarate_overlay" 
#define TF_SCREEN_OVERLAY_MATERIAL_BLEED			"effects/bleed_overlay" 
#define TF_SCREEN_OVERLAY_MATERIAL_STEALTH			"effects/stealth_overlay"
#define TF_SCREEN_OVERLAY_MATERIAL_SWIMMING_CURSE	"effects/jarate_overlay" 
#define TF_SCREEN_OVERLAY_MATERIAL_GAS				"effects/gas_overlay" 

#define TF_SCREEN_OVERLAY_MATERIAL_PHASE	"effects/dodge_overlay"

#define MAX_DAMAGE_EVENTS		128

const char *g_pszBDayGibs[22] = 
{
	"models/effects/bday_gib01.mdl",
	"models/effects/bday_gib02.mdl",
	"models/effects/bday_gib03.mdl",
	"models/effects/bday_gib04.mdl",
	"models/player/gibs/gibs_balloon.mdl",
	"models/player/gibs/gibs_burger.mdl",
	"models/player/gibs/gibs_boot.mdl",
	"models/player/gibs/gibs_bolt.mdl",
	"models/player/gibs/gibs_can.mdl",
	"models/player/gibs/gibs_clock.mdl",
	"models/player/gibs/gibs_fish.mdl",
	"models/player/gibs/gibs_gear1.mdl",
	"models/player/gibs/gibs_gear2.mdl",
	"models/player/gibs/gibs_gear3.mdl",
	"models/player/gibs/gibs_gear4.mdl",
	"models/player/gibs/gibs_gear5.mdl",
	"models/player/gibs/gibs_hubcap.mdl",
	"models/player/gibs/gibs_licenseplate.mdl",
	"models/player/gibs/gibs_spring1.mdl",
	"models/player/gibs/gibs_spring2.mdl",
	"models/player/gibs/gibs_teeth.mdl",
	"models/player/gibs/gibs_tire.mdl"
};

ETFCond g_SoldierBuffAttributeIDToConditionMap[kSoldierBuffCount + 1] =
{
	TF_COND_LAST,				// dummy entry to deal with attribute value of "1" being the lowest value we store in the attribute itself
	TF_COND_OFFENSEBUFF,
	TF_COND_DEFENSEBUFF,
	TF_COND_REGENONDAMAGEBUFF,
	TF_COND_NOHEALINGDAMAGEBUFF,
	TF_COND_CRITBOOSTED_RAGE_BUFF,
	TF_COND_SNIPERCHARGE_RAGE_BUFF
};

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void RecvProxy_BuildablesListChanged( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	RecvProxy_Int32ToInt32( pData, pStruct, pOut );

	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || pLocalPlayer->entindex() != pData->m_ObjectID )
		return;

	int index = pData->m_pRecvProp->GetOffset() / sizeof(int);
	int object = pData->m_Value.m_Int;

	IGameEvent *event = gameeventmanager->CreateEvent( "update_status_item" );
	if ( event )
	{
		event->SetInt( "index", index );
		event->SetInt( "object", object );
		gameeventmanager->FireEventClientSide( event );
	}
}
#endif

//=============================================================================
//
// Tables.
//

// Client specific.

#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( localplayerscoring_t, DT_TFPlayerScoringDataExclusive )
	RecvPropInt( RECVINFO( m_iCaptures ) ),
	RecvPropInt( RECVINFO( m_iDefenses ) ),
	RecvPropInt( RECVINFO( m_iKills ) ),
	RecvPropInt( RECVINFO( m_iDeaths ) ),
	RecvPropInt( RECVINFO( m_iSuicides ) ),
	RecvPropInt( RECVINFO( m_iDominations ) ),
	RecvPropInt( RECVINFO( m_iRevenge ) ),
	RecvPropInt( RECVINFO( m_iBuildingsBuilt ) ),
	RecvPropInt( RECVINFO( m_iBuildingsDestroyed ) ),
	RecvPropInt( RECVINFO( m_iHeadshots ) ),
	RecvPropInt( RECVINFO( m_iBackstabs ) ),
	RecvPropInt( RECVINFO( m_iHealPoints ) ),
	RecvPropInt( RECVINFO( m_iInvulns ) ),
	RecvPropInt( RECVINFO( m_iTeleports ) ),
	RecvPropInt( RECVINFO( m_iResupplyPoints ) ),
	RecvPropInt( RECVINFO( m_iKillAssists ) ),
	RecvPropInt( RECVINFO( m_iPoints ) ),
	RecvPropInt( RECVINFO( m_iBonusPoints ) ),
	RecvPropInt( RECVINFO( m_iDamageDone ) ),
	RecvPropInt( RECVINFO( m_iCrits ) ),
END_RECV_TABLE()

EXTERN_RECV_TABLE(DT_TFPlayerConditionListExclusive);

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	RecvPropInt( RECVINFO( m_nDesiredDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDesiredDisguiseClass ) ),
	RecvPropTime( RECVINFO( m_flStealthNoAttackExpire ) ),
	RecvPropTime( RECVINFO( m_flStealthNextChangeTime ) ),
	RecvPropBool( RECVINFO( m_bLastDisguisedAsOwnTeam ) ),
	RecvPropFloat( RECVINFO( m_flRageMeter ) ),
	RecvPropBool( RECVINFO( m_bRageDraining ) ),
	RecvPropTime( RECVINFO( m_flNextRageEarnTime ) ),
	RecvPropBool( RECVINFO( m_bInUpgradeZone ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flItemChargeMeter ), RecvPropFloat( RECVINFO( m_flItemChargeMeter[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominated ), RecvPropBool( RECVINFO( m_bPlayerDominated[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominatingMe ), RecvPropBool( RECVINFO( m_bPlayerDominatingMe[0] ) ) ),
	RecvPropDataTable( RECVINFO_DT(m_ScoreData),0, &REFERENCE_RECV_TABLE(DT_TFPlayerScoringDataExclusive) ),
	RecvPropDataTable( RECVINFO_DT(m_RoundScoreData),0, &REFERENCE_RECV_TABLE(DT_TFPlayerScoringDataExclusive) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( condition_source_t, DT_TFPlayerConditionSource )
	//RecvPropInt( RECVINFO( m_nPreventedDamageFromCondition ) ),
	//RecvPropFloat( RECVINFO( m_flExpireTime ) ),
	RecvPropEHandle( RECVINFO( m_pProvider ) ),
	//RecvPropBool( RECVINFO( m_bPrevActive ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	RecvPropInt( RECVINFO( m_nPlayerCond ) ),
	RecvPropInt( RECVINFO( m_bJumping) ),
	RecvPropInt( RECVINFO( m_nNumHealers ) ),
	RecvPropInt( RECVINFO( m_iCritMult ) ),
	RecvPropInt( RECVINFO( m_iAirDash ) ),
	RecvPropInt( RECVINFO( m_nAirDucked ) ),
	RecvPropFloat( RECVINFO( m_flDuckTimer ) ),
	RecvPropInt( RECVINFO( m_nPlayerState ) ),
	RecvPropInt( RECVINFO( m_iDesiredPlayerClass ) ),
	RecvPropFloat( RECVINFO( m_flMovementStunTime ) ),
	RecvPropInt( RECVINFO( m_iMovementStunAmount ) ),
	RecvPropInt( RECVINFO( m_iMovementStunParity ) ),
	RecvPropEHandle( RECVINFO( m_hStunner ) ),
	RecvPropInt( RECVINFO( m_iStunFlags ) ),
	RecvPropInt( RECVINFO( m_nArenaNumChanges ) ),
	RecvPropBool( RECVINFO( m_bArenaFirstBloodBoost ) ),
	RecvPropInt( RECVINFO( m_iWeaponKnockbackID ) ),
	RecvPropBool( RECVINFO( m_bLoadoutUnavailable ) ),
	RecvPropInt( RECVINFO( m_iItemFindBonus ) ),
	RecvPropBool( RECVINFO( m_bShieldEquipped ) ),
	RecvPropBool( RECVINFO( m_bParachuteEquipped ) ),
	RecvPropInt( RECVINFO( m_iNextMeleeCrit ) ),
	RecvPropInt( RECVINFO( m_iDecapitations ) ),
	RecvPropInt( RECVINFO( m_iRevengeCrits ) ),
	RecvPropInt( RECVINFO( m_iDisguiseBody ) ),
	RecvPropEHandle( RECVINFO( m_hCarriedObject ) ),
	RecvPropBool( RECVINFO( m_bCarryingObject ) ),
	RecvPropFloat( RECVINFO( m_flNextNoiseMakerTime ) ),
	RecvPropInt( RECVINFO( m_iSpawnRoomTouchCount ) ),
	RecvPropInt( RECVINFO( m_iKillCountSinceLastDeploy ) ),
	RecvPropFloat( RECVINFO( m_flFirstPrimaryAttack ) ),

	//Scout
	RecvPropFloat( RECVINFO( m_flEnergyDrinkMeter) ),
	RecvPropFloat( RECVINFO( m_flHypeMeter) ),

	// Demoman
	RecvPropFloat( RECVINFO( m_flChargeMeter) ),

	// Spy.
	RecvPropTime( RECVINFO( m_flInvisChangeCompleteTime ) ),
	RecvPropInt( RECVINFO( m_nDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDisguiseClass ) ),
	RecvPropInt( RECVINFO( m_nDisguiseSkinOverride ) ),
	RecvPropInt( RECVINFO( m_nMaskClass ) ),
	RecvPropEHandle( RECVINFO ( m_hDisguiseTarget ) ),
	RecvPropInt( RECVINFO( m_iDisguiseHealth ) ),
	RecvPropBool( RECVINFO( m_bFeignDeathReady ) ),
	RecvPropEHandle( RECVINFO( m_hDisguiseWeapon ) ),
	RecvPropInt( RECVINFO( m_nTeamTeleporterUsed ) ),
	RecvPropFloat( RECVINFO( m_flCloakMeter ) ),
	RecvPropFloat( RECVINFO( m_flSpyTranqBuffDuration ) ),

	// Local Data.
	RecvPropDataTable( "tfsharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFPlayerSharedLocal) ),
	RecvPropDataTable( RECVINFO_DT(m_ConditionList),0, &REFERENCE_RECV_TABLE(DT_TFPlayerConditionListExclusive) ),

	RecvPropInt( RECVINFO( m_iTauntIndex ) ),
	RecvPropInt( RECVINFO( m_iTauntConcept ) ),

	RecvPropInt( RECVINFO( m_nPlayerCondEx ) ),
	RecvPropInt( RECVINFO( m_iStunIndex ) ),

	RecvPropInt( RECVINFO( m_nHalloweenBombHeadStage ) ),

	RecvPropInt( RECVINFO( m_nPlayerCondEx2 ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx3 ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nStreaks ), RecvPropInt( RECVINFO( m_nStreaks[0] ) ) ),
	RecvPropInt( RECVINFO( m_unTauntSourceItemID_Low ) ),
	RecvPropInt( RECVINFO( m_unTauntSourceItemID_High ) ),
	RecvPropFloat( RECVINFO( m_flRuneCharge ) ),
	RecvPropBool( RECVINFO( m_bHasPasstimeBall ) ),
	RecvPropBool( RECVINFO( m_bIsTargetedForPasstimePass ) ),
	RecvPropEHandle( RECVINFO( m_hPasstimePassTarget ) ),
	RecvPropFloat( RECVINFO( m_askForBallTime ) ),
	RecvPropBool( RECVINFO( m_bKingRuneBuffActive ) ),

	RecvPropUtlVectorDataTable( m_ConditionData, TF_COND_LAST, DT_TFPlayerConditionSource ),

	RecvPropInt( RECVINFO( m_nPlayerCondEx4 ) ),

	RecvPropFloat( RECVINFO( m_flHolsterAnimTime ) ),
	RecvPropEHandle( RECVINFO( m_hSwitchTo ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CTFPlayerShared )
	DEFINE_PRED_FIELD( m_nPlayerState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCond, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCloakMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flRageMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bRageDraining, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextRageEarnTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flEnergyDrinkMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flHypeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flChargeMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iAirDash, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nAirDucked, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flDuckTimer, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flInvisChangeCompleteTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nDisguiseTeam, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_nDisguiseClass, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_nDisguiseSkinOverride, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nMaskClass, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nDesiredDisguiseTeam, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_nDesiredDisguiseClass, FIELD_INTEGER, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_bLastDisguisedAsOwnTeam, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE  ),
	DEFINE_PRED_FIELD( m_bFeignDeathReady, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx3, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx4, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( m_hDisguiseWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_FIELD( m_flDisguiseCompleteTime, FIELD_FLOAT ),
	DEFINE_PRED_FIELD( m_bHasPasstimeBall, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIsTargetedForPasstimePass, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ), // does this belong here?
	DEFINE_PRED_FIELD( m_askForBallTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flHolsterAnimTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_flItemChargeMeter, FIELD_FLOAT, LAST_LOADOUT_SLOT_WITH_CHARGE_METER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iStunIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bScattergunJump, FIELD_BOOLEAN, 0 ),
END_PREDICTION_DATA()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( localplayerscoring_t, DT_TFPlayerScoringDataExclusive )
	SendPropInt( SENDINFO( m_iCaptures ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDefenses ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iKills ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDeaths ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iSuicides ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDominations ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iRevenge ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBuildingsBuilt ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBuildingsDestroyed ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iHeadshots ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBackstabs ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iHealPoints ), 20, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iInvulns ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iTeleports ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDamageDone ), 20, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iCrits ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iResupplyPoints ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iKillAssists ), 12, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iBonusPoints ), 10, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iPoints ), 10, SPROP_UNSIGNED ),
END_SEND_TABLE()

EXTERN_SEND_TABLE(DT_TFPlayerConditionListExclusive);
BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	SendPropInt( SENDINFO( m_nDesiredDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDesiredDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bLastDisguisedAsOwnTeam ) ),
	SendPropTime( SENDINFO( m_flStealthNoAttackExpire ) ),
	SendPropTime( SENDINFO( m_flStealthNextChangeTime ) ),
	SendPropFloat( SENDINFO( m_flRageMeter ), 0, SPROP_NOSCALE, 0.0, 100.0 ),
	SendPropBool( SENDINFO( m_bRageDraining ) ),
	SendPropTime( SENDINFO( m_flNextRageEarnTime ) ),
	SendPropBool( SENDINFO( m_bInUpgradeZone ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flItemChargeMeter ), SendPropFloat( SENDINFO_ARRAY( m_flItemChargeMeter ), -1, SPROP_NOSCALE ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominated ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominated ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominatingMe ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominatingMe ) ) ),
	SendPropDataTable( SENDINFO_DT(m_ScoreData), &REFERENCE_SEND_TABLE(DT_TFPlayerScoringDataExclusive) ),
	SendPropDataTable( SENDINFO_DT(m_RoundScoreData), &REFERENCE_SEND_TABLE(DT_TFPlayerScoringDataExclusive) ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( condition_source_t, DT_TFPlayerConditionSource )
	//SendPropInt( SENDINFO( m_nPreventedDamageFromCondition ) ),
	//SendPropFloat( SENDINFO( m_flExpireTime ) ),
	SendPropEHandle( SENDINFO( m_pProvider ) ),
	//SendPropBool( SENDINFO( m_bPrevActive ) ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	SendPropInt( SENDINFO( m_nPlayerCond ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bJumping ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nNumHealers ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iCritMult ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iAirDash ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nAirDucked ), 2, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flDuckTimer )  ),
	SendPropInt( SENDINFO( m_nPlayerState ), Q_log2( TF_STATE_COUNT )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDesiredPlayerClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flMovementStunTime )  ),
	SendPropInt( SENDINFO( m_iMovementStunAmount ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iMovementStunParity ), MOVEMENTSTUN_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hStunner ) ),
	SendPropInt( SENDINFO( m_iStunFlags  ), 12, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nArenaNumChanges ), 5, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bArenaFirstBloodBoost ) ),
	SendPropInt( SENDINFO( m_iWeaponKnockbackID ) ),
	SendPropBool( SENDINFO( m_bLoadoutUnavailable ) ),
	SendPropInt( SENDINFO( m_iItemFindBonus ) ),
	SendPropBool( SENDINFO( m_bShieldEquipped ) ),
	SendPropBool( SENDINFO( m_bParachuteEquipped ) ),
	SendPropInt( SENDINFO( m_iNextMeleeCrit ) ),
	SendPropInt( SENDINFO( m_iDecapitations ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iRevengeCrits ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDisguiseBody ) ),
	SendPropEHandle( SENDINFO( m_hCarriedObject ) ),
	SendPropBool( SENDINFO( m_bCarryingObject ) ),
	SendPropFloat( SENDINFO( m_flNextNoiseMakerTime ) ),
	SendPropInt( SENDINFO( m_iSpawnRoomTouchCount ) ),
	SendPropInt( SENDINFO( m_iKillCountSinceLastDeploy ) ),
	SendPropFloat( SENDINFO( m_flFirstPrimaryAttack ) ),

	//Scout
	SendPropFloat( SENDINFO( m_flEnergyDrinkMeter ), 0, SPROP_NOSCALE, 0.0, 100.0 ),
	SendPropFloat( SENDINFO( m_flHypeMeter ), 0, SPROP_NOSCALE, 0.0, 100.0 ),

	// Demoman
	SendPropFloat( SENDINFO( m_flChargeMeter ), 8, SPROP_NOSCALE, 0.0, 100.0 ),

	// Spy
	SendPropTime( SENDINFO( m_flInvisChangeCompleteTime ) ),
	SendPropInt( SENDINFO( m_nDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDisguiseSkinOverride ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMaskClass ), 4, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hDisguiseTarget ) ),
	SendPropInt( SENDINFO( m_iDisguiseHealth ), -1, SPROP_VARINT ),
	SendPropBool( SENDINFO( m_bFeignDeathReady ) ),
	SendPropEHandle( SENDINFO( m_hDisguiseWeapon ) ),
	SendPropInt( SENDINFO( m_nTeamTeleporterUsed ), 3, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flCloakMeter ), 16, SPROP_NOSCALE, 0.0, 100.0 ),
	SendPropFloat( SENDINFO( m_flSpyTranqBuffDuration ), 16, SPROP_NOSCALE, 0.0, 100.0 ),
	
	// Local Data.
	SendPropDataTable( "tfsharedlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFPlayerSharedLocal ), SendProxy_SendLocalDataTable ),	
	SendPropDataTable( SENDINFO_DT(m_ConditionList), &REFERENCE_SEND_TABLE(DT_TFPlayerConditionListExclusive) ),

	SendPropInt( SENDINFO( m_iTauntIndex ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iTauntConcept ), 8, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_nPlayerCondEx ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iStunIndex ), 8 ),
	
	SendPropInt( SENDINFO( m_nHalloweenBombHeadStage ), 2, SPROP_UNSIGNED ),

	SendPropInt( SENDINFO( m_nPlayerCondEx2 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nPlayerCondEx3 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),

	SendPropArray3( SENDINFO_ARRAY3( m_nStreaks ), SendPropInt( SENDINFO_ARRAY( m_nStreaks ) ) ),
	SendPropInt( SENDINFO( m_unTauntSourceItemID_Low ), -1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_unTauntSourceItemID_High ), -1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flRuneCharge ), 8, 0, 0.f, 100.f ),
	SendPropBool( SENDINFO( m_bHasPasstimeBall ) ),
	SendPropBool( SENDINFO( m_bIsTargetedForPasstimePass ) ),
	SendPropEHandle( SENDINFO( m_hPasstimePassTarget ) ),
	SendPropFloat( SENDINFO( m_askForBallTime ) ),
	SendPropBool( SENDINFO( m_bKingRuneBuffActive ) ),

	SendPropUtlVectorDataTable( m_ConditionData, TF_COND_LAST, DT_TFPlayerConditionSource ),

	SendPropInt( SENDINFO( m_nPlayerCondEx4 ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	
	SendPropFloat( SENDINFO( m_flHolsterAnimTime ) ),
	SendPropEHandle( SENDINFO( m_hSwitchTo ) ),
END_SEND_TABLE()

#endif

extern void HandleRageGain( CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale );

CTFWearableDemoShield* GetEquippedDemoShield( CTFPlayer * pPlayer )
{
	// Loop through our wearables in search of a shield
	for ( int i=0; i<pPlayer->GetNumWearables(); ++i )
	{
		CTFWearableDemoShield *pWearableShield = dynamic_cast<CTFWearableDemoShield*>( pPlayer->GetWearable( i ) );
		if ( pWearableShield )
		{
			return pWearableShield;
		}
	}

	return NULL;
}

CTFPlayer *GetRuneCarrier( RuneTypes_t type, int iTeam = TEAM_ANY )
{
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( !pPlayer )
			continue;

		if ( iTeam != TEAM_ANY && pPlayer->GetTeamNumber() != iTeam )
			continue;

		if ( pPlayer->m_Shared.GetCarryingRuneType() == type )
		{
			return pPlayer;
		}
	}

	return NULL;
}

// --------------------------------------------------------------------------------------------------- //
// Shared CTFPlayer implementation.
// --------------------------------------------------------------------------------------------------- //

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::HasCampaignMedal( int iMedal )
{ 
	return ( ( m_iCampaignMedals & iMedal ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAllowedToTaunt( void )
{
	if ( !IsAlive() )
		return false;

	// Check to see if we can taunt again!
	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return false;

	// Can't taunt while charging.
	if ( m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		return false;

	if ( m_Shared.InCond( TF_COND_COMPETITIVE_LOSER ) )
		return false;

	if ( IsLerpingFOV() )
		return false;

	// Check for things that prevent taunting
	if ( ShouldStopTaunting() )
		return false;

	// Check to see if we are on the ground.
	if ( GetGroundEntity() == NULL && !m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		if ( !pActiveWeapon->OwnerCanTaunt() )
			return false;

		// ignore taunt key if one of these if active weapon
		if ( pActiveWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_BUILD 
			 ||	pActiveWeapon->GetWeaponID() == TF_WEAPON_PDA_ENGINEER_DESTROY )
			return false;
	}

	// can't taunt while carrying an object
	if ( m_Shared.IsCarryingObject() )
		return false;

	// Can't taunt if hooked into a player
	if ( m_Shared.InCond( TF_COND_GRAPPLED_TO_PLAYER ) )
		return false;

	if ( IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_LUNCHBOX )
		{
			//Scouts can't drink while they're already phasing.
			if ( m_Shared.InCond( TF_COND_ENERGY_BUFF ) || m_Shared.InCond( TF_COND_PHASE ) )
				return false;
			
			// Or if their energy drink meter isn't refilled
			if ( m_Shared.GetScoutEnergyDrinkMeter() < 100 )
				return false;

			//They can't drink the default (phase) item while carrying a flag
			pActiveWeapon = m_Shared.GetActiveTFWeapon();
			if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_LUNCHBOX )
			{
				CTFLunchBox *pLunchbox = (CTFLunchBox*)pActiveWeapon;

				if ( ( pLunchbox->GetLunchboxType() == LUNCHBOX_STANDARD ) || ( pLunchbox->GetLunchboxType() == LUNCHBOX_STANDARD_ROBO ) )
				{
					if ( !TFGameRules()->IsMannVsMachineMode() && HasItem() )
						return false;
				}
			}
		}
	}

	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( m_Shared.IsStealthed() || m_Shared.InCond( TF_COND_STEALTHED_BLINK ) || 
			 m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}
	}

	return true;
}


// --------------------------------------------------------------------------------------------------- //
// CTFPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //

CTFPlayerShared::CTFPlayerShared()
{
	// If you hit this assert, CONGRATULATIONS!  You've added a condition that has gone 
	// beyond the amount of bits we network for conditions.  Take a look at the pattern
	// of m_nPlayerCond, m_nPlayerCondEx, m_nPlayerCondEx2, m_nPlayerCondEx3, and m_nPlayerCondEx4 to get more bits.
	// This pattern is as such to preserve replays.
	// Don't forget to add an m_nOldCond* and m_nForceCond*
	COMPILE_TIME_ASSERT( TF_COND_LAST < (32 + 32 + 32 + 32 + 32) );

	m_nPlayerState.Set( TF_STATE_WELCOME );
	m_bJumping = false;
	m_iAirDash = 0;
	m_nAirDucked = 0;
	m_flDuckTimer = 0.0f;
	m_flStealthNoAttackExpire = 0.0f;
	m_flStealthNextChangeTime = 0.0f;
	m_iCritMult = 0;
	m_flInvisibility = 0.0f;
	m_flPrevInvisibility = 0.f;
	m_flTmpDamageBonusAmount = 1.0f;

	m_bScattergunJump = false;

	m_bFeignDeathReady = false;

	m_fCloakConsumeRate = tf_spy_cloak_consume_rate.GetFloat();
	m_fCloakRegenRate = tf_spy_cloak_regen_rate.GetFloat();

	m_fEnergyDrinkConsumeRate = tf_scout_energydrink_consume_rate.GetFloat();
	m_fEnergyDrinkRegenRate = tf_scout_energydrink_regen_rate.GetFloat();

	m_bMotionCloak = false;

	m_hStunner = NULL;
	m_iStunFlags = 0;

	m_hAssist = NULL;

	m_bLastDisguisedAsOwnTeam = false;

	m_bRageDraining = false;
	m_bInUpgradeZone = false;
	m_bPhaseFXOn = false;
	ResetRageBuffs();
	
	m_iPhaseDamage = 0;

	Q_memset(m_pPhaseTrail, 0, sizeof(m_pPhaseTrail));
	
	m_iWeaponKnockbackID = -1;

	m_bLoadoutUnavailable = false;

	m_nMaskClass = 0;

	m_iItemFindBonus = 0;

	m_nTeamTeleporterUsed = TEAM_UNASSIGNED;

	m_bShieldEquipped = false;
	m_bPostShieldCharge = false;
	m_iNextMeleeCrit = 0;

	m_bParachuteEquipped = false;

	m_iDecapitations = m_iOldDecapitations = 0;
	m_iOldKillStreak = 0;
	m_iOldKillStreakWepSlot = 0;

	m_flNextNoiseMakerTime = 0;
	m_iSpawnRoomTouchCount = 0;

	m_iKillCountSinceLastDeploy = 0;
	m_flFirstPrimaryAttack = 0.0f;

#ifdef GAME_DLL
	m_flBestOverhealDecayMult = -1;
	m_hPeeAttacker = NULL;

	m_flHealedPerSecondTimer = -1000;
	m_bPulseRadiusHeal = false;

	m_flRadiusCurrencyCollectionTime = 0;
	m_flRadiusSpyScanTime = 0;

	m_flCloakStartTime = -1.0f;

	ListenForGameEvent( "player_disconnect" );
#else
	m_pWheelEffect = NULL;
	m_angVehicleMoveAngles = QAngle( 0.f, 0.f, 0.f );
	m_angVehicleMovePitchLast = 0.0f;
	m_hKartParachuteEntity = NULL;
#endif

	m_nForceConditions = 0;
	m_nForceConditionsEx = 0;
	m_nForceConditionsEx2 = 0;
	m_nForceConditionsEx3 = 0;
	m_nForceConditionsEx4 = 0;

	m_flChargeEndTime = -1000;
	m_flLastChargeTime = -1000;
	m_flLastNoChargeTime = 0;
	m_bChargeGlowing = false;

	m_bChargeOffSounded = false;

	m_bBiteEffectWasApplied = false;

	m_flLastMovementStunChange = 0;
	m_bStunNeedsFadeOut = false;

	m_flChargeMeter = 100;
	m_flEnergyDrinkMeter = 0;
	m_flHypeMeter = 0;

	m_bCarryingObject = false;
	m_hCarriedObject = NULL;

	m_iStunIndex = -1;
	m_flLastNoMovementTime = -1.f;
	m_flRuneCharge = 0.f;
	
	// generic meters
	for( int i=0; i < m_flItemChargeMeter.Count(); ++i )
	{
		SetItemChargeMeter( loadout_positions_t(i), 0.f );
	}

	m_iPasstimeThrowAnimState = PASSTIME_THROW_ANIM_NONE;
	m_bHasPasstimeBall = false;
	m_bIsTargetedForPasstimePass = false;
	m_askForBallTime = 0.0f;

	m_flHolsterAnimTime = 0.f;
	m_hSwitchTo = NULL;

	m_iCYOAPDAAnimState = CYOA_PDA_ANIM_NONE;

	// make sure we have all conditions in the list
	m_ConditionData.EnsureCount( TF_COND_LAST );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::Init( CTFPlayer *pPlayer )
{
	m_pOuter = pPlayer;

	m_flNextBurningSound = 0;

	m_bArenaFirstBloodBoost = false;
	m_iStunAnimState = STUN_ANIM_NONE;
	m_iPhaseDamage = 0;
	m_iWeaponKnockbackID = -1;
	m_hStunner = NULL;

	m_iPasstimeThrowAnimState = PASSTIME_THROW_ANIM_NONE;
	m_bHasPasstimeBall = false;
	m_bIsTargetedForPasstimePass = false;
	m_askForBallTime = 0.0f;

	m_iCYOAPDAAnimState = CYOA_PDA_ANIM_NONE;

	m_bMotionCloak = false;

	m_bShieldEquipped = false;
	m_bPostShieldCharge = false;
	m_iNextMeleeCrit = 0;

	m_bParachuteEquipped = false;

	m_iDecapitations = m_iOldDecapitations = 0;
	m_iOldKillStreak = 0;
	m_iOldKillStreakWepSlot = 0;

	SetJumping( false );
	SetAssist( NULL );

	m_flInvulnerabilityRemoveTime = -1;

	SetNextMeleeCrit( MELEE_NOCRIT );

	Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::ResetRageBuffs( void )
{
	for ( int i = 0; i < kBuffSlot_MAX; i++ )
	{
		m_RageBuffSlots[i].m_iBuffTypeActive = 0;
		m_RageBuffSlots[i].m_iBuffPulseCount = 0;
		m_RageBuffSlots[i].m_flNextBuffPulseTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::Spawn( void )
{
#ifdef GAME_DLL
	m_hPeeAttacker = NULL;

	if ( m_bCarryingObject )
	{
		CBaseObject* pObj = GetCarriedObject();
		if ( pObj )
		{
			pObj->DetonateObject();
		}
	}

	m_bCarryingObject = false;
	m_hCarriedObject = NULL;

	m_flRadiusHealCheckTime = 0;
	m_flKingRuneBuffCheckTime = 0.f;

	m_bBiteEffectWasApplied = false;

	m_iSpawnRoomTouchCount = 0;

	for( int i = FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; i <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++i )
	{
		CBaseEntity* pItem = m_pOuter->GetEntityForLoadoutSlot( i, true );
		if ( pItem )
		{
			int iDenyMeterResupply = 0;
			loadout_positions_t eSlot = loadout_positions_t(i);
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pItem, iDenyMeterResupply, item_meter_resupply_denied );
			SetItemChargeMeter( eSlot, ( iDenyMeterResupply > 0 ) ? GetItemChargeMeter( eSlot ) : pItem->GetDefaultItemChargeMeterValue() );
			m_flPrevItemChargeMeter[ eSlot ] = GetItemChargeMeter( eSlot );
		}
	}
	
	SetRevengeCrits( 0 );

	m_PlayerStuns.RemoveAll();

	m_iPasstimeThrowAnimState = PASSTIME_THROW_ANIM_NONE;
	m_bHasPasstimeBall = false;
	m_bIsTargetedForPasstimePass = false;
	m_askForBallTime = 0.0f;
#else
	m_bSyncingConditions = false;
#endif
	m_iStunIndex = -1;
	m_bKingRuneBuffActive = false;

	// Reset our assist here incase something happens before we get killed
	// again that checks this (getting slapped with a fish)
	SetAssist( NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template < typename tIntType >
class CConditionVars
{
public:
	CConditionVars( tIntType& nPlayerCond, tIntType& nPlayerCondEx, tIntType& nPlayerCondEx2, tIntType& nPlayerCondEx3, tIntType& nPlayerCondEx4, ETFCond eCond )
	{
		if ( eCond >= 128 )
		{
			Assert( eCond < 128 + 32 );
			m_pnCondVar = &nPlayerCondEx4;
			m_nCondBit = eCond - 128; 
		}
		else if ( eCond >= 96 )
		{
			Assert( eCond < 96 + 32 );
			m_pnCondVar = &nPlayerCondEx3;
			m_nCondBit = eCond - 96;
		}
		else if( eCond >= 64 )
		{
			Assert( eCond < (64 + 32) );
			m_pnCondVar = &nPlayerCondEx2;
			m_nCondBit = eCond - 64;
		}
		else if ( eCond >= 32 )
		{
			Assert( eCond < (32 + 32) );
			m_pnCondVar = &nPlayerCondEx;
			m_nCondBit = eCond - 32;
		}
		else
		{
			m_pnCondVar = &nPlayerCond;
			m_nCondBit = eCond;
		}
	}

	tIntType& CondVar() const
	{
		return *m_pnCondVar;
	}

	int CondBit() const
	{
		return 1 << m_nCondBit;
	}

private:
	tIntType *m_pnCondVar;
	int m_nCondBit;
};

//-----------------------------------------------------------------------------
// Purpose: Add a condition and duration
// duration of PERMANENT_CONDITION means infinite duration
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddCond( ETFCond eCond, float flDuration /* = PERMANENT_CONDITION */, CBaseEntity *pProvider /*= NULL */)
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	// If we're dead, don't take on any new conditions
	if( !m_pOuter || !m_pOuter->IsAlive() )
	{
		return;
	}

#ifdef CLEINT_DLL
	if ( m_pOuter->IsDormant() )
	{
		return;
	}
#endif

	// sanity check to prevent servers from adding these conditions when they shouldn't
	if ( ( eCond == TF_COND_COMPETITIVE_WINNER ) || ( eCond == TF_COND_COMPETITIVE_LOSER ) )
	{
		if ( TFGameRules() && !TFGameRules()->ShowMatchSummary() )
			return;
	}

	// Which bitfield are we tracking this condition variable in? Which bit within
	// that variable will we track it as?
	CConditionVars<int> cPlayerCond( m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond );

	// See if there is an object representation of the condition.
	bool bAddedToExternalConditionList = m_ConditionList.Add( eCond, flDuration, m_pOuter, pProvider );
	if ( !bAddedToExternalConditionList )
	{
		// Set the condition bit for this condition.
		cPlayerCond.CondVar() |= cPlayerCond.CondBit();

		// Flag for gamecode to query
		m_ConditionData[eCond].m_bPrevActive = ( m_ConditionData[eCond].m_flExpireTime != 0.f ) ? true : false;

		if ( flDuration != PERMANENT_CONDITION )
		{
			// if our current condition is permanent or we're trying to set a new
			// time that's less our current time remaining, use our current time instead
			if ( ( m_ConditionData[eCond].m_flExpireTime == PERMANENT_CONDITION ) || 
				 ( flDuration < m_ConditionData[eCond].m_flExpireTime ) )
			{
				flDuration = m_ConditionData[eCond].m_flExpireTime;
			}
		}

		m_ConditionData[eCond].m_flExpireTime = flDuration;
		m_ConditionData[eCond].m_pProvider = pProvider;
		m_ConditionData[eCond].m_nPreventedDamageFromCondition = 0;

		OnConditionAdded( eCond );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Forcibly remove a condition
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveCond( ETFCond eCond, bool ignore_duration )
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	if ( !InCond( eCond ) )
		return;

	CConditionVars<int> cPlayerCond( m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond );

	// If this variable is handled by the condition list, abort before doing the
	// work for the condition flags.
	if ( m_ConditionList.Remove( eCond, ignore_duration ) )
		return;

	cPlayerCond.CondVar() &= ~cPlayerCond.CondBit();
	OnConditionRemoved( eCond );

	if ( m_ConditionData[ eCond ].m_nPreventedDamageFromCondition )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "damage_prevented" );
		if ( pEvent )
		{
			pEvent->SetInt( "preventor", m_ConditionData[eCond].m_pProvider ? m_ConditionData[eCond].m_pProvider->entindex() : m_pOuter->entindex() );
			pEvent->SetInt( "victim", m_pOuter->entindex() );
			pEvent->SetInt( "amount", m_ConditionData[ eCond ].m_nPreventedDamageFromCondition );
			pEvent->SetInt( "condition", eCond );

			gameeventmanager->FireEvent( pEvent, true );
		}

		m_ConditionData[ eCond ].m_nPreventedDamageFromCondition = 0;
	}

	m_ConditionData[eCond].m_flExpireTime = 0;
	m_ConditionData[eCond].m_pProvider = NULL;
	m_ConditionData[eCond].m_bPrevActive = false;

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCond( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );

	// Old condition system, only used for the first 32 conditions
	if ( eCond < 32 && m_ConditionList.InCond( eCond ) )
		return true;

	CConditionVars<const int> cPlayerCond( m_nPlayerCond.m_Value, m_nPlayerCondEx.m_Value, m_nPlayerCondEx2.m_Value, m_nPlayerCondEx3.m_Value, m_nPlayerCondEx4.m_Value, eCond );
	return (cPlayerCond.CondVar() & cPlayerCond.CondBit()) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Return whether or not we were in this condition before.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::WasInCond( ETFCond eCond ) const
{
	// I don't know if this actually works for conditions < 32, because we definitely cannot peak into m_ConditionList (back in time).
	// But others think that m_ConditionList is propogated into m_nOldConditions, so just check if you hit the assert. (And then remove the 
	// assert. And this comment).
	Assert( eCond >= 32 && eCond < TF_COND_LAST );

	CConditionVars<const int> cPlayerCond( m_nOldConditions, m_nOldConditionsEx, m_nOldConditionsEx2, m_nOldConditionsEx3, m_nOldConditionsEx4, eCond );
	return (cPlayerCond.CondVar() & cPlayerCond.CondBit()) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Set a bit to force this condition off and then back on next time we sync bits from the server. 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ForceRecondNextSync( ETFCond eCond )
{
	// I don't know if this actually works for conditions < 32. We may need to set this bit in m_ConditionList, too.
	// Please check if you hit the assert. (And then remove the assert. And this comment).
	Assert(eCond >= 32 && eCond < TF_COND_LAST);

	CConditionVars<int> playerCond( m_nForceConditions, m_nForceConditionsEx, m_nForceConditionsEx2, m_nForceConditionsEx3, m_nForceConditionsEx4, eCond );
	playerCond.CondVar() |= playerCond.CondBit();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetConditionDuration( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	if ( InCond( eCond ) )
	{
		return m_ConditionData[eCond].m_flExpireTime;
	}
	
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity that provided the passed in condition
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetConditionProvider( ETFCond eCond ) const
{
	Assert( eCond >= 0 && eCond < TF_COND_LAST );
	Assert( eCond < m_ConditionData.Count() );

	CBaseEntity *pProvider = NULL;
	if ( InCond( eCond ) )
	{
		if ( eCond == TF_COND_CRITBOOSTED )
		{
			pProvider = m_ConditionList.GetProvider( eCond );
		}
		else
		{
			pProvider = m_ConditionData[eCond].m_pProvider;
		}
	}

	return pProvider;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity that applied this condition to us - for granting an assist when we die
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetConditionAssistFromVictim( void )
{
	// We only give an assist to one person.  That means this list is order
	// sensitive, so consider how "powerful" an effect is when adding it here.
	static const ETFCond nTrackedConditions[] = 
	{
		TF_COND_URINE,
		TF_COND_MAD_MILK,
		TF_COND_MARKEDFORDEATH,
		TF_COND_GAS,
	};

	CBaseEntity *pProvider = NULL;
	for ( int i = 0; i < ARRAYSIZE( nTrackedConditions ); i++ )
	{
		if ( InCond( nTrackedConditions[i] ) )
		{
			pProvider = GetConditionProvider( nTrackedConditions[i] );
			break;
		}
	}

	return pProvider;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the entity that applied this condition to us - for granting an assist when we kill someone
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetConditionAssistFromAttacker( void )
{
	// We only give an assist to one person.  That means this list is order
	// sensitive, so consider how "powerful" an effect is when adding it here.
	static const ETFCond nTrackedConditions[] = 
	{
		TF_COND_OFFENSEBUFF,			// Highest priority
		TF_COND_DEFENSEBUFF,
		TF_COND_REGENONDAMAGEBUFF,
		TF_COND_NOHEALINGDAMAGEBUFF,	// Lowest priority
	};

	CBaseEntity *pProvider = NULL;
	for ( int i = 0; i < ARRAYSIZE( nTrackedConditions ); i++ )
	{
		if ( InCond( nTrackedConditions[i] ) )
		{
			CBaseEntity* pPotentialProvider = GetConditionProvider( nTrackedConditions[i] );
			// Check to make sure we're not providing the condition to ourselves
			if( pPotentialProvider != m_pOuter )
			{
				pProvider = pPotentialProvider;
				break;
			}
		}
	}

	return pProvider;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::DebugPrintConditions( void )
{
#ifndef CLIENT_DLL
	const char *szDll = "Server";
#else
	const char *szDll = "Client";
#endif

	Msg( "( %s ) Conditions for player ( %d )\n", szDll, m_pOuter->entindex() );

	int i;
	int iNumFound = 0;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( InCond( (ETFCond)i ) )
		{
			if ( m_ConditionData[i].m_flExpireTime == PERMANENT_CONDITION )
			{
				Msg( "( %s ) Condition %d - ( permanent cond )\n", szDll, i );
			}
			else
			{
				Msg( "( %s ) Condition %d - ( %.1f left )\n", szDll, i, m_ConditionData[i].m_flExpireTime );
			}

			iNumFound++;
		}
	}

	if ( iNumFound == 0 )
	{
		Msg( "( %s ) No active conditions\n", szDll );
	}
}

void CTFPlayerShared::InstantlySniperUnzoom( void )
{
	// Unzoom if we are a sniper zoomed!
	if ( m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SNIPER )
	{
		CTFWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();

		if ( pWpn && WeaponID_IsSniperRifle( pWpn->GetWeaponID() ) )
		{
			CTFSniperRifle *pRifle = static_cast<CTFSniperRifle*>( pWpn );
			if ( pRifle->IsZoomed() )
			{
				// Let the rifle clean up conditions and state
				pRifle->ToggleZoom();
				// Slam the FOV right now
				m_pOuter->SetFOV( m_pOuter, 0, 0.0f );
			}
		}
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnPreDataChanged( void )
{
	m_ConditionList.OnPreDataChanged();

	m_nOldConditions = m_nPlayerCond;
	m_nOldConditionsEx = m_nPlayerCondEx;
	m_nOldConditionsEx2 = m_nPlayerCondEx2;
	m_nOldConditionsEx3 = m_nPlayerCondEx3;
	m_nOldConditionsEx4 = m_nPlayerCondEx4;
	m_nOldDisguiseClass = GetDisguiseClass();
	m_nOldDisguiseTeam = GetDisguiseTeam();
	m_iOldMovementStunParity = m_iMovementStunParity;

	SharedThink();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDataChanged( void )
{
	m_ConditionList.OnDataChanged( m_pOuter );

	if ( m_iOldMovementStunParity != m_iMovementStunParity )
	{
		m_flStunFade = gpGlobals->curtime + m_flMovementStunTime;
		m_flStunEnd  = m_flStunFade;
		if ( IsControlStunned() && (m_iStunAnimState == STUN_ANIM_NONE) )
		{
			m_flStunEnd += CONTROL_STUN_ANIM_TIME;
		}

		UpdateLegacyStunSystem();
	}

	// Update conditions from last network change
	SyncConditions( m_nOldConditions, m_nPlayerCond, m_nForceConditions, 0 );
	SyncConditions( m_nOldConditionsEx, m_nPlayerCondEx, m_nForceConditionsEx, 32 );
	SyncConditions( m_nOldConditionsEx2, m_nPlayerCondEx2, m_nForceConditionsEx2, 64 );
	SyncConditions( m_nOldConditionsEx3, m_nPlayerCondEx3, m_nForceConditionsEx3, 96 );
	SyncConditions( m_nOldConditionsEx4, m_nPlayerCondEx4, m_nForceConditionsEx4, 128 );

	// Make sure these items are present
	m_nPlayerCond		|= m_nForceConditions;
	m_nPlayerCondEx		|= m_nForceConditionsEx;
	m_nPlayerCondEx2	|= m_nForceConditionsEx2;
	m_nPlayerCondEx3	|= m_nForceConditionsEx3;
	m_nPlayerCondEx4	|= m_nForceConditionsEx4;

	// Clear our force bits now that we've used them.
	m_nForceConditions = 0;
	m_nForceConditionsEx = 0;
	m_nForceConditionsEx2 = 0;
	m_nForceConditionsEx3 = 0;
	m_nForceConditionsEx4 = 0;

	if ( m_nOldDisguiseClass != GetDisguiseClass() || m_nOldDisguiseTeam != GetDisguiseTeam() )
	{
		OnDisguiseChanged();
	}

	if ( m_hDisguiseWeapon )
	{
		m_hDisguiseWeapon->UpdateVisibility();
		m_hDisguiseWeapon->UpdateParticleSystems();
	}

	// XXX(JohnS): This is not the right place to do these things, SetWeaponVisible on the *client* is just stomping
	//             bugs elsewhere.  I'm not going to go re-fix all the spots doing this, but don't use this as a
	//             template!  Add a check in TFWeaponBase::ShouldDraw() and call TFWeapon->UpdateVisibility() on the
	//             edges.
	if ( ( IsLoser() || InCond( TF_COND_COMPETITIVE_LOSER ) ) && GetActiveTFWeapon() && !GetActiveTFWeapon()->IsEffectActive( EF_NODRAW ) )
	{
		GetActiveTFWeapon()->SetWeaponVisible( false );
	}

	InvisibilityThink();
}

//-----------------------------------------------------------------------------
// Purpose: check the newly networked conditions for changes
//-----------------------------------------------------------------------------
void CTFPlayerShared::SyncConditions( int nPreviousConditions, int nNewConditions, int nForceConditions, int nBaseCondBit )
{
	if ( nPreviousConditions == nNewConditions )
		return;

	int nCondChanged = nNewConditions ^ nPreviousConditions;
	int nCondAdded = nCondChanged & nNewConditions;
	int nCondRemoved = nCondChanged & nPreviousConditions;
	m_bSyncingConditions = true;

	for ( int i=0;i<32;i++ )
	{
		const int testBit = 1<<i;
		if ( nForceConditions & testBit )
		{
			if ( nPreviousConditions & testBit )
			{
				OnConditionRemoved((ETFCond)(nBaseCondBit + i));
			}
			OnConditionAdded((ETFCond)(nBaseCondBit + i));
		}
		else
		{
			if ( nCondAdded & testBit )
			{
				OnConditionAdded( (ETFCond)(nBaseCondBit + i) );
			}
			else if ( nCondRemoved & testBit )
			{
				OnConditionRemoved( (ETFCond)(nBaseCondBit + i) );
			}
		}
	}
	m_bSyncingConditions = false;
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Remove any conditions affecting players
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAllCond()
{
	m_ConditionList.RemoveAll();

	int i;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( InCond( (ETFCond)i ) )
		{
			RemoveCond( (ETFCond)i );
		}
	}

	// Now remove all the rest
	m_nPlayerCond = 0;
	m_nPlayerCondEx = 0;
	m_nPlayerCondEx2 = 0;
	m_nPlayerCondEx3 = 0;
	m_nPlayerCondEx4 = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we add the bit,
// and client when it receives the new cond bits and finds one added
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionAdded( ETFCond eCond )
{
	switch( eCond )
	{
	case TF_COND_ZOOMED:
		OnAddZoomed();
		break;
	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;

		m_flHealedPerSecondTimer = gpGlobals->curtime + 1.0f;
#endif
		break;
	
	case TF_COND_HEALTH_OVERHEALED:
		OnAddOverhealed();
		break;

	case TF_COND_FEIGN_DEATH:
		OnAddFeignDeath();
		break;
	
	case TF_COND_STEALTHED:
	case TF_COND_STEALTHED_USER_BUFF:
		OnAddStealthed();
		break;

	case TF_COND_INVULNERABLE:
	case TF_COND_INVULNERABLE_USER_BUFF:
	case TF_COND_INVULNERABLE_CARD_EFFECT:
		OnAddInvulnerable();
		break;

	case TF_COND_TELEPORTED:
		OnAddTeleported();
		break;

	case TF_COND_BURNING:
		OnAddBurning();
		break;

	case TF_COND_CRITBOOSTED:
		Assert( !"TF_COND_CRITBOOSTED should be handled by the condition list!" );
		break;

	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
		OnAddDemoCharge();
		break;

	// First blood falls through on purpose.
	case TF_COND_CRITBOOSTED_FIRST_BLOOD:
		SetFirstBloodBoosted( true );
	case TF_COND_CRITBOOSTED_PUMPKIN:
	case TF_COND_CRITBOOSTED_USER_BUFF:
	case TF_COND_CRITBOOSTED_BONUS_TIME:
	case TF_COND_CRITBOOSTED_CTF_CAPTURE:
	case TF_COND_CRITBOOSTED_ON_KILL:
	case TF_COND_CRITBOOSTED_RAGE_BUFF:
	case TF_COND_SNIPERCHARGE_RAGE_BUFF:
	case TF_COND_CRITBOOSTED_CARD_EFFECT:
	case TF_COND_CRITBOOSTED_RUNE_TEMP:
		OnAddCritBoost();
		break;

	case TF_COND_SODAPOPPER_HYPE:
		OnAddSodaPopperHype();
		break;

	case TF_COND_DISGUISING:
		OnAddDisguising();
		break;

	case TF_COND_DISGUISED:
		OnAddDisguised();
		break;

	case TF_COND_URINE:
		OnAddUrine();
		break;

	case TF_COND_MARKEDFORDEATH:
		OnAddMarkedForDeath();
		break;

	case TF_COND_BLEEDING:
		OnAddBleeding();
		break;

	case TF_COND_TAUNTING:
		OnAddTaunting();
		break;

	case TF_COND_STUNNED:
		OnAddStunned();
		break;

	case TF_COND_PHASE:
		OnAddPhase();
		break;

	case TF_COND_OFFENSEBUFF:
		OnAddOffenseBuff();
		break;

	case TF_COND_DEFENSEBUFF:
	case TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK:
	case TF_COND_DEFENSEBUFF_HIGH:
		OnAddDefenseBuff();
		break;

	case TF_COND_REGENONDAMAGEBUFF:
		OnAddOffenseHealthRegenBuff();
		break;

	case TF_COND_NOHEALINGDAMAGEBUFF:
		OnAddNoHealingDamageBuff();
		break;

	case TF_COND_SHIELD_CHARGE:
		OnAddShieldCharge();
		break;

	case TF_COND_DEMO_BUFF:
		OnAddDemoBuff();
		break;

	case TF_COND_ENERGY_BUFF:
		OnAddEnergyDrinkBuff();
		break;
		
	case TF_COND_RADIUSHEAL:
		OnAddRadiusHeal();
		break;

	case TF_COND_MEGAHEAL:
		OnAddMegaHeal();
		break;

	case TF_COND_MAD_MILK:
		OnAddMadMilk();
		break;

	case TF_COND_SPEED_BOOST:				OnAddSpeedBoost( false );		break;

	case TF_COND_SAPPED:
		OnAddSapped();
		break;

	case TF_COND_REPROGRAMMED:
		OnAddReprogrammed();
		break;

	case TF_COND_PASSTIME_PENALTY_DEBUFF:
	case TF_COND_MARKEDFORDEATH_SILENT:
		OnAddMarkedForDeathSilent();
		break;

	case TF_COND_DISGUISED_AS_DISPENSER:
		OnAddDisguisedAsDispenser();
		break;

	case TF_COND_HALLOWEEN_BOMB_HEAD:
		OnAddHalloweenBombHead();
		break;

	case TF_COND_HALLOWEEN_THRILLER:
		OnAddHalloweenThriller();
		break;

	case TF_COND_RADIUSHEAL_ON_DAMAGE:
		OnAddRadiusHealOnDamage();
		break;

	case TF_COND_MEDIGUN_UBER_BULLET_RESIST:
		OnAddMedEffectUberBulletResist();
		break;

	case TF_COND_MEDIGUN_UBER_BLAST_RESIST:
		OnAddMedEffectUberBlastResist();
		break;

	case TF_COND_MEDIGUN_UBER_FIRE_RESIST:
		OnAddMedEffectUberFireResist();
		break;

	case TF_COND_MEDIGUN_SMALL_BULLET_RESIST:
		OnAddMedEffectSmallBulletResist();
		break;

	case TF_COND_MEDIGUN_SMALL_BLAST_RESIST:
		OnAddMedEffectSmallBlastResist();
		break;

	case TF_COND_MEDIGUN_SMALL_FIRE_RESIST:
		OnAddMedEffectSmallFireResist();
		break;
	
	case TF_COND_STEALTHED_USER_BUFF_FADING:
		OnAddStealthedUserBuffFade();
		break;

	case TF_COND_BULLET_IMMUNE:
		OnAddBulletImmune();
		break;

	case TF_COND_BLAST_IMMUNE:
		OnAddBlastImmune();
		break;

	case TF_COND_FIRE_IMMUNE:
		OnAddFireImmune();
		break;

	case TF_COND_MVM_BOT_STUN_RADIOWAVE:
		OnAddMVMBotRadiowave();
		break;

	case TF_COND_HALLOWEEN_SPEED_BOOST:
		OnAddHalloweenSpeedBoost();
		break;
	
	case TF_COND_HALLOWEEN_QUICK_HEAL:
		OnAddHalloweenQuickHeal();
		break;

	case TF_COND_HALLOWEEN_GIANT:
		OnAddHalloweenGiant();
		break;

	case TF_COND_HALLOWEEN_TINY:
		OnAddHalloweenTiny();
		break;

	case TF_COND_HALLOWEEN_GHOST_MODE:
		OnAddHalloweenGhostMode();
		break;

	case TF_COND_PARACHUTE_ACTIVE:
		OnAddCondParachute();
		break;

	case TF_COND_HALLOWEEN_KART_DASH:
		OnAddHalloweenKartDash();
		break;
		
	case TF_COND_HALLOWEEN_KART:
		OnAddHalloweenKart();
		break;
		
	case TF_COND_BALLOON_HEAD:
		OnAddBalloonHead();
		break;

	case TF_COND_MELEE_ONLY:
		OnAddMeleeOnly();
		break;

	case TF_COND_SWIMMING_CURSE:
		OnAddSwimmingCurse();
		break;

	case TF_COND_HALLOWEEN_KART_CAGE:
		OnAddHalloweenKartCage();
		break;

	case TF_COND_RUNE_RESIST:
		OnAddRuneResist();
		break;

	case TF_COND_GRAPPLINGHOOK_LATCHED:
		OnAddGrapplingHookLatched();
		break;

	case TF_COND_PASSTIME_INTERCEPTION:
		OnAddPasstimeInterception();
		break;

	case TF_COND_RUNE_PLAGUE:
		OnAddRunePlague();
		break;

	case TF_COND_PLAGUE:
		OnAddPlague();
		break;

	case TF_COND_PURGATORY:
		OnAddInPurgatory();
		break;

	case TF_COND_COMPETITIVE_WINNER:
		OnAddCompetitiveWinner();
		break;

	case TF_COND_COMPETITIVE_LOSER:
		OnAddCompetitiveLoser();
		break;

	case TF_COND_GAS:
		OnAddCondGas();
		break;

	case TF_COND_ROCKETPACK:
		OnAddRocketPack();
		break;

	case TF_COND_HALLOWEEN_HELL_HEAL:
		OnAddHalloweenHellHeal();
		break;


	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we remove the bit,
// and client when it receives the new cond bits and finds one removed
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionRemoved( ETFCond eCond )
{
	switch( eCond )
	{
	case TF_COND_ZOOMED:
		OnRemoveZoomed();
		break;

	case TF_COND_BURNING:
		OnRemoveBurning();
		break;

	case TF_COND_CRITBOOSTED:
		Assert( !"TF_COND_CRITBOOSTED should be handled by the condition list!" );
		break;

	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
		OnRemoveDemoCharge();
		break;

	// First blood falls through on purpose.
	case TF_COND_CRITBOOSTED_FIRST_BLOOD:
		SetFirstBloodBoosted( false );
	case TF_COND_CRITBOOSTED_PUMPKIN:
	case TF_COND_CRITBOOSTED_USER_BUFF:
	case TF_COND_CRITBOOSTED_BONUS_TIME:
	case TF_COND_CRITBOOSTED_CTF_CAPTURE:
	case TF_COND_CRITBOOSTED_ON_KILL:
	case TF_COND_CRITBOOSTED_RAGE_BUFF:
	case TF_COND_SNIPERCHARGE_RAGE_BUFF:
	case TF_COND_CRITBOOSTED_CARD_EFFECT:
	case TF_COND_CRITBOOSTED_RUNE_TEMP:
		OnRemoveCritBoost();
		break;

	case TF_COND_SODAPOPPER_HYPE:
		OnRemoveSodaPopperHype();
		break;

	case TF_COND_TMPDAMAGEBONUS:
		OnRemoveTmpDamageBonus();
		break;

	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_HEALTH_OVERHEALED:
		OnRemoveOverhealed();
		break;

	case TF_COND_FEIGN_DEATH:
		OnRemoveFeignDeath();
		break;

	case TF_COND_STEALTHED:
	case TF_COND_STEALTHED_USER_BUFF:
		OnRemoveStealthed();
		break;

	case TF_COND_DISGUISED:
		OnRemoveDisguised();
		break;

	case TF_COND_DISGUISING:
		OnRemoveDisguising();
		break;

	case TF_COND_INVULNERABLE:
	case TF_COND_INVULNERABLE_USER_BUFF:
	case TF_COND_INVULNERABLE_CARD_EFFECT:
		OnRemoveInvulnerable();
		break;

	case TF_COND_TELEPORTED:
		OnRemoveTeleported();
		break;

	case TF_COND_STUNNED:
		OnRemoveStunned();
		break;

	case TF_COND_PHASE:
		OnRemovePhase();
		break;

	case TF_COND_URINE:
		OnRemoveUrine();
		break;

	case TF_COND_MARKEDFORDEATH:
		OnRemoveMarkedForDeath();
		break;

	case TF_COND_BLEEDING:
		OnRemoveBleeding();
		break;

	case TF_COND_INVULNERABLE_WEARINGOFF:
		OnRemoveInvulnerableWearingOff();
		break;	

	case TF_COND_OFFENSEBUFF:
		OnRemoveOffenseBuff();
		break;

	case TF_COND_DEFENSEBUFF:
	case TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK:
	case TF_COND_DEFENSEBUFF_HIGH:
		OnRemoveDefenseBuff();
		break;

	case TF_COND_REGENONDAMAGEBUFF:
		OnRemoveOffenseHealthRegenBuff();
		break;

	case TF_COND_NOHEALINGDAMAGEBUFF:
		OnRemoveNoHealingDamageBuff();
		break;

	case TF_COND_SHIELD_CHARGE:
		OnRemoveShieldCharge();
		break;

	case TF_COND_DEMO_BUFF:
		OnRemoveDemoBuff();
		break;

	case TF_COND_ENERGY_BUFF:
		OnRemoveEnergyDrinkBuff();
		break;
		
	case TF_COND_RADIUSHEAL:
		OnRemoveRadiusHeal();
		break;

	case TF_COND_MEGAHEAL:
		OnRemoveMegaHeal();
		break;

	case TF_COND_MAD_MILK:
		OnRemoveMadMilk();
		break;

	case TF_COND_TAUNTING:
		OnRemoveTaunting();
		break;

	case TF_COND_SPEED_BOOST:					OnRemoveSpeedBoost( false );		break;
		

	case TF_COND_SAPPED:
		OnRemoveSapped();
		break;

	case TF_COND_REPROGRAMMED:
		OnRemoveReprogrammed();
		break;

	case TF_COND_PASSTIME_PENALTY_DEBUFF:
	case TF_COND_MARKEDFORDEATH_SILENT:
		OnRemoveMarkedForDeathSilent();
		break;

	case TF_COND_DISGUISED_AS_DISPENSER:
		OnRemoveDisguisedAsDispenser();
		break;

	case TF_COND_HALLOWEEN_BOMB_HEAD:
		OnRemoveHalloweenBombHead();
		break;

	case TF_COND_HALLOWEEN_THRILLER:
		OnRemoveHalloweenThriller();
		break;

	case TF_COND_RADIUSHEAL_ON_DAMAGE:
		OnRemoveRadiusHealOnDamage();
		break;

	case TF_COND_MEDIGUN_UBER_BULLET_RESIST:
		OnRemoveMedEffectUberBulletResist();
		break;

	case TF_COND_MEDIGUN_UBER_BLAST_RESIST:
		OnRemoveMedEffectUberBlastResist();
		break;

	case TF_COND_MEDIGUN_UBER_FIRE_RESIST:
		OnRemoveMedEffectUberFireResist();
		break;

	case TF_COND_MEDIGUN_SMALL_BULLET_RESIST:
		OnRemoveMedEffectSmallBulletResist();
		break;

	case TF_COND_MEDIGUN_SMALL_BLAST_RESIST:
		OnRemoveMedEffectSmallBlastResist();
		break;

	case TF_COND_MEDIGUN_SMALL_FIRE_RESIST:
		OnRemoveMedEffectSmallFireResist();
		break;

	case TF_COND_STEALTHED_USER_BUFF_FADING:
		OnRemoveStealthedUserBuffFade();
		break;

	case TF_COND_BULLET_IMMUNE:
		OnRemoveBulletImmune();
		break;

	case TF_COND_BLAST_IMMUNE:
		OnRemoveBlastImmune();
		break;

	case TF_COND_FIRE_IMMUNE:
		OnRemoveFireImmune();
		break;

	case TF_COND_MVM_BOT_STUN_RADIOWAVE:
		OnRemoveMVMBotRadiowave();
		break;

	case TF_COND_HALLOWEEN_SPEED_BOOST:
		OnRemoveHalloweenSpeedBoost();
		break;

	case TF_COND_HALLOWEEN_QUICK_HEAL:
		OnRemoveHalloweenQuickHeal();
		break;

	case TF_COND_HALLOWEEN_GIANT:
		OnRemoveHalloweenGiant();
		break;

	case TF_COND_HALLOWEEN_TINY:
		OnRemoveHalloweenTiny();
		break;

	case TF_COND_HALLOWEEN_GHOST_MODE:
		OnRemoveHalloweenGhostMode();
		break;

	case TF_COND_PARACHUTE_ACTIVE:
		OnRemoveCondParachute();
		break;

	case TF_COND_HALLOWEEN_KART_DASH:
		OnRemoveHalloweenKartDash();
		break;
		
	case TF_COND_HALLOWEEN_KART:
		OnRemoveHalloweenKart();
		break;
		
	case TF_COND_BALLOON_HEAD:
		OnRemoveBalloonHead();
		break;

	case TF_COND_MELEE_ONLY:
		OnRemoveMeleeOnly();
		break;

	case TF_COND_SWIMMING_CURSE:
		OnRemoveSwimmingCurse();				

	case TF_COND_HALLOWEEN_KART_CAGE:
		OnRemoveHalloweenKartCage();
		break;

	case TF_COND_RUNE_RESIST:
		OnRemoveRuneResist();
		break;

	case TF_COND_GRAPPLINGHOOK_LATCHED:
		OnRemoveGrapplingHookLatched();
		break;

	case TF_COND_PASSTIME_INTERCEPTION:
		OnRemovePasstimeInterception();
		break;

	case TF_COND_RUNE_PLAGUE:
		OnRemoveRunePlague();
		break;

	case TF_COND_PLAGUE:
		OnRemovePlague();
		break;

	case TF_COND_PURGATORY:
		OnRemoveInPurgatory();
		break;

	case TF_COND_RUNE_KING:
		OnRemoveRuneKing();
		break;

	case TF_COND_KING_BUFFED:
		OnRemoveKingBuff();
		break;

	case TF_COND_RUNE_SUPERNOVA:
		OnRemoveRuneSupernova();
		break;

	case TF_COND_COMPETITIVE_WINNER:
		OnRemoveCompetitiveWinner();
		break;

	case TF_COND_COMPETITIVE_LOSER:
		OnRemoveCompetitiveLoser();
		break;

	case TF_COND_GAS:
		OnRemoveCondGas();
		break;

	case TF_COND_ROCKETPACK:
		OnRemoveRocketPack();
		break;

	case TF_COND_BURNING_PYRO:
		OnRemoveBurningPyro();
		break;

	case TF_COND_HALLOWEEN_HELL_HEAL:
		OnRemoveHalloweenHellHeal();
		break;


	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: The highest possible overheal this player is able to receive
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetMaxBuffedHealth( bool bIgnoreAttributes /*= false*/, bool bIgnoreHealthOverMax /*= false*/ )
{
	int nMaxHealthForBuffing = m_pOuter->GetMaxHealthForBuffing();
	float flBoostMax = nMaxHealthForBuffing * tf_max_health_boost.GetFloat();

#ifdef GAME_DLL
	// Look for any attributes that might change the answer
	if ( !bIgnoreAttributes )
	{
		// MvM can boost medics past the default max
		int nMaxMedicOverheal = nMaxHealthForBuffing * GetMaxOverhealMultiplier();
		if ( nMaxMedicOverheal > flBoostMax )
		{
			flBoostMax = nMaxMedicOverheal;
		}
	}
#endif

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	if ( !bIgnoreHealthOverMax )
	{
		// Don't allow overheal total to be less than the buffable + unbuffable max health or the current health
		int nBoostMin = Max( m_pOuter->GetMaxHealth(), m_pOuter->GetHealth() );
		if ( iRoundDown < nBoostMin )
		{
			iRoundDown = nBoostMin;
		}
	}

	return iRoundDown;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Based on active medics
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetMaxOverhealMultiplier( void )
{
	float flMax = 1.f;

	// Find out if any healer is providing more than the default max overheal
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		CTFPlayer *pTFMedic = ToTFPlayer( m_aHealers[i].pHealer );
		CWeaponMedigun *pMedigun = ( ( pTFMedic && pTFMedic->GetActiveTFWeapon() && pTFMedic->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MEDIGUN ) ? static_cast<CWeaponMedigun*>( pTFMedic->GetActiveTFWeapon() ) : NULL );
		float flOverHealBonus = ( ( pMedigun ) ? pMedigun->GetOverHealBonus( m_pOuter ) : m_aHealers[i].flOverhealBonus );
		if ( flOverHealBonus > flMax )
		{
			flMax = flOverHealBonus;
		}
	}

	return flMax;
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDisguiseMaxBuffedHealth( bool bIgnoreAttributes /*= false*/, bool bIgnoreHealthOverMax /*= false*/ )
{
	// Find the healer we have who's providing the most overheal
	float flBoostMax = GetDisguiseMaxHealth() * tf_max_health_boost.GetFloat();
#ifdef GAME_DLL
	if ( !bIgnoreAttributes )
	{
		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			float flOverheal = GetDisguiseMaxHealth() * m_aHealers[i].flOverhealBonus;
			if ( flOverheal > flBoostMax )
			{
				flBoostMax = flOverheal;
			}
		}
	}
#endif

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	if ( !bIgnoreHealthOverMax )
	{
		// Don't allow overheal total to be less than the buffable + unbuffable max health or the current health
		int nBoostMin = MAX(GetDisguiseMaxHealth(), GetDisguiseHealth() );
		if ( iRoundDown < nBoostMin )
		{
			iRoundDown = nBoostMin;
		}
	}

	return iRoundDown;
}

//-----------------------------------------------------------------------------
// Purpose: Runs SERVER SIDE only Condition Think
// If a player needs something to be updated no matter what do it here (invul, etc).
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionGameRulesThink( void )
{
#ifdef GAME_DLL

	m_ConditionList.ServerThink();

	if ( m_flNextCritUpdate < gpGlobals->curtime )
	{
		UpdateCritMult();
		m_flNextCritUpdate = gpGlobals->curtime + 0.5;
	}

	for ( int i=0; i < TF_COND_LAST; ++i )
	{
		// if we're in this condition and it's not already being handled by the condition list
		if ( InCond( (ETFCond)i ) && ((i >= 32) || !m_ConditionList.InCond( (ETFCond)i )) )
		{
			// Ignore permanent conditions
			if ( m_ConditionData[i].m_flExpireTime != PERMANENT_CONDITION )
			{
				float flReduction = gpGlobals->frametime;

				// If we're being healed, we reduce bad conditions faster
				if ( ConditionExpiresFast( (ETFCond)i) && m_aHealers.Count() > 0 )
				{
					if ( i == TF_COND_URINE )
					{
						flReduction += (m_aHealers.Count() * flReduction);
					}
					else
					{
						flReduction += (m_aHealers.Count() * flReduction * 4);
					}
				}

				m_ConditionData[i].m_flExpireTime = MAX( m_ConditionData[i].m_flExpireTime - flReduction, 0 );

				if ( m_ConditionData[i].m_flExpireTime == 0 )
				{
					RemoveCond( (ETFCond)i );
				}
			}
			else
			{
#if !defined( DEBUG )
				// Prevent hacked usercommand exploits
				if ( m_pOuter->GetTimeSinceLastUserCommand() > 5.f || m_pOuter->GetTimeSinceLastThink() > 5.f )
				{
					if ( GetCarryingRuneType() != RUNE_NONE )
					{
						m_pOuter->DropRune();
					}
				}
#endif
			}
		}
	}

	// Our health will only decay ( from being medic buffed ) if we are not being healed by a medic
	// Dispensers can give us the TF_COND_HEALTH_BUFF, but will not maintain or give us health above 100%s
	bool bDecayHealth = true;
	bool bDecayDisguiseHealth = true;

	// If we're being healed, heal ourselves
	if ( InCond( TF_COND_HEALTH_BUFF ) )
	{
		// Heal faster if we haven't been in combat for a while
		float flTimeSinceDamage = gpGlobals->curtime - m_pOuter->GetLastDamageReceivedTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10.f, 15.f, 1.f, 3.f );
		float flAttribModScale = 1.f;
		
		// Any attributes affecting heal rate?
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flAttribModScale, mult_health_fromhealers );
		CTFWeaponBase *pActiveWeapon = m_pOuter->GetActiveTFWeapon();
		if ( pActiveWeapon )
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pActiveWeapon, flAttribModScale, mult_health_fromhealers_penalty_active );
		}

		float flCurOverheal = (float)m_pOuter->GetHealth() / (float)m_pOuter->GetMaxHealth();
		if ( flCurOverheal > 1.0f )
		{
			// If they're over their max health the overheal calculation is relative to the max buffable amount scale
			float flMaxHealthForBuffing = m_pOuter->GetMaxHealthForBuffing();
			float flBuffableRangeHealth = m_pOuter->GetHealth() - ( m_pOuter->GetMaxHealth() - flMaxHealthForBuffing );
			flCurOverheal = flBuffableRangeHealth / flMaxHealthForBuffing;
		}

		float flCurDisguiseOverheal = ( GetDisguiseMaxHealth() != 0 ) ? ( (float)GetDisguiseHealth() / (float)GetDisguiseMaxHealth() ) : ( flCurOverheal );

		float fTotalHealAmount = 0.0f;
		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			Assert( m_aHealers[i].pHealer );

			float flPerHealerAttribModScale = 1.f;
			// Check if the healer has an attribute that modifies their overheal rate
			if( flCurOverheal > 1.f && !m_aHealers[i].bDispenserHeal )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_aHealers[i].pHealer, flPerHealerAttribModScale, overheal_fill_rate );
			}

			bool bHealDisguise = InCond( TF_COND_DISGUISED );
			bool bHealActual = true;

			// dispensers heal cloak
			if ( m_aHealers[i].bDispenserHeal )
			{
				AddToSpyCloakMeter( gpGlobals->frametime * m_aHealers[i].flAmount );	
			}

			// Don't heal over the healer's overheal bonus
			if ( flCurOverheal >= m_aHealers[i].flOverhealBonus )
			{
				bHealActual = false;
			}

			// Same overheal check, but for fake health
			if ( InCond( TF_COND_DISGUISED ) && flCurDisguiseOverheal >= m_aHealers[i].flOverhealBonus )
			{
				// Fake over-heal
				bHealDisguise = false;
			}

			CTFPlayer *pTFHealer = ToTFPlayer( m_aHealers[i].pHealer );
			if ( !bHealActual && !bHealDisguise )
			{
				if ( pTFHealer )
				{
					// Quick fix never lets health decay, even when they're at or above max overheal
					CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pTFHealer->GetActiveTFWeapon() );
					if ( pMedigun && pMedigun->GetMedigunType() == MEDIGUN_QUICKFIX )
					{
						bDecayHealth = false;
						bDecayDisguiseHealth = false;
					}
				}

				continue;
			}

			// Being healed by a medigun, don't decay our health
			if ( bHealActual )
			{
				bDecayHealth = false;
			}

			if ( bHealDisguise )
			{
				bDecayDisguiseHealth = false;
			}

			// What we multiply the heal amount by (can be changed by conditions or items).
			float flHealAmountMult = 1.0f;

			// Quick-Fix uber
			if ( InCond( TF_COND_MEGAHEAL ) )
			{
				flHealAmountMult = 3.0f;
			}

			flScale *= flHealAmountMult;

			// Dispensers heal at a constant rate
			if ( m_aHealers[i].bDispenserHeal )
			{
				// Dispensers heal at a slower rate, but ignore flScale
				if ( bHealActual )
				{
					float flDispenserFraction = gpGlobals->frametime * m_aHealers[i].flAmount * flAttribModScale;
					m_flHealFraction += flDispenserFraction;

					// track how much this healer has actually done so far
					m_aHealers[i].flHealAccum += clamp( flDispenserFraction, 0.f, (float) GetMaxBuffedHealth() - m_pOuter->GetHealth() );
				}
				if ( bHealDisguise )
				{
					m_flDisguiseHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flAttribModScale;
				}
			}
			else	// player heals are affected by the last damage time
			{
				if ( bHealActual )
				{
					// Scale this if needed
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flScale, mult_healing_from_medics );
					m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flScale * flAttribModScale * flPerHealerAttribModScale;
				}
				if ( bHealDisguise )
				{
					m_flDisguiseHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flScale * flAttribModScale * flPerHealerAttribModScale;
				}
			}

			fTotalHealAmount += m_aHealers[i].flAmount;

			// Keep our decay multiplier uptodate
			if ( m_flBestOverhealDecayMult == -1 || m_aHealers[i].flOverhealDecayMult < m_flBestOverhealDecayMult )
			{
				m_flBestOverhealDecayMult = m_aHealers[i].flOverhealDecayMult;
			}
		}

		if ( InCond( TF_COND_HEALING_DEBUFF ) )
		{
			m_flHealFraction *= ( 1.f - PYRO_AFTERBURN_HEALING_REDUCTION );
		}

		int nHealthToAdd = (int)m_flHealFraction;
		int nDisguiseHealthToAdd = (int)m_flDisguiseHealFraction;
		if ( nHealthToAdd > 0 || nDisguiseHealthToAdd > 0 )
		{
			if ( nHealthToAdd > 0 )
			{
				m_flHealFraction -= nHealthToAdd;
			}

			if ( nDisguiseHealthToAdd > 0 )
			{
				m_flDisguiseHealFraction -= nDisguiseHealthToAdd;
			}

			int iBoostMax = GetMaxBuffedHealth();

			if ( InCond( TF_COND_DISGUISED ) )
			{
				// Separate cap for disguised health
				int nFakeHealthToAdd = clamp( nDisguiseHealthToAdd, 0, GetDisguiseMaxBuffedHealth() - m_iDisguiseHealth );
				m_iDisguiseHealth += nFakeHealthToAdd;
			}

			// Track health prior to healing
			int nPrevHealth = m_pOuter->GetHealth();
			
			// Cap it to the max we'll boost a player's health
			nHealthToAdd = clamp( nHealthToAdd, 0, iBoostMax - m_pOuter->GetHealth() );
			
			m_pOuter->TakeHealth( nHealthToAdd, DMG_IGNORE_MAXHEALTH | DMG_IGNORE_DEBUFFS );
			
			m_pOuter->AdjustDrownDmg( -1.0 * nHealthToAdd ); // subtract this from the drowndmg in case they're drowning and being healed at the same time

			// split up total healing based on the amount each healer contributes
			if ( fTotalHealAmount > 0 )
			{
				for ( int i = 0; i < m_aHealers.Count(); i++ )
				{
					Assert( m_aHealers[i].pHealScorer );
					Assert( m_aHealers[i].pHealer );
					if ( m_aHealers[i].pHealScorer.IsValid() && m_aHealers[i].pHealer.IsValid() )
					{
						CBaseEntity *pHealer = m_aHealers[i].pHealer;
						float flHealAmount = nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount );

						if ( pHealer && IsAlly( pHealer ) )
						{
							CTFPlayer *pHealScorer = ToTFPlayer( m_aHealers[i].pHealScorer );
							if ( pHealScorer )
							{	
								// Don't report healing when we're close to the buff cap and haven't taken damage recently.
								// This avoids sending bogus heal stats while maintaining our max overheal.  Ideally we
								// wouldn't decay in this scenario, but that would be a risky change.
								if ( iBoostMax - nPrevHealth > 1 || gpGlobals->curtime - m_pOuter->GetLastDamageReceivedTime() <= 1.f )
								{
									CTF_GameStats.Event_PlayerHealedOther( pHealScorer, flHealAmount );
								}

								// Add this to the one-second-healing counter
								m_aHealers[i].flHealedLastSecond += flHealAmount;
					
								HandleRageGain( m_pOuter, kRageBuffFlag_OnMedicHealingReceived, flHealAmount / 2.f, 1.0f );
								
								float flRage = flHealAmount;
								if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && 
									 TFObjectiveResource() && TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
								{
									flRage = Max( flHealAmount, 10.f );
								}
								HandleRageGain( pHealScorer, kRageBuffFlag_OnHeal, flRage, 1.0f );

								// If it's been one second, or we know healing beyond this point will be overheal, generate an event
								if ( ( m_flHealedPerSecondTimer <= gpGlobals->curtime || m_pOuter->GetHealth() >= m_pOuter->GetMaxHealth() ) 
									   && m_aHealers[i].flHealedLastSecond > 1 )
								{
									// Make sure this isn't pure overheal
									if ( m_pOuter->GetHealth() - m_aHealers[i].flHealedLastSecond < m_pOuter->GetMaxHealth() )
									{
										float flOverHeal = m_pOuter->GetHealth() - m_pOuter->GetMaxHealth();
										if ( flOverHeal > 0 )
										{
											m_aHealers[i].flHealedLastSecond -= flOverHeal;
										}

										// TEST THIS
										// Give the medic some uber if it is from their (AoE heal) which has no overheal
										if ( m_aHealers[i].flOverhealBonus <= 1.0f )
										{
											// Give a litte bit of uber based on actual healing
											// Give them a little bit of Uber
											CWeaponMedigun *pMedigun = static_cast<CWeaponMedigun *>( pHealScorer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
											if ( pMedigun )
											{
												// On Mediguns, per frame, the amount of uber added is based on 
												// Default heal rate is 24per second, we scale based on that and frametime
												pMedigun->AddCharge( ( m_aHealers[i].flHealedLastSecond / 24.0f ) * gpGlobals->frametime * 0.33f );
											}
										}

										IGameEvent * event = gameeventmanager->CreateEvent( "player_healed" );
										if ( event )
										{
											// HLTV event priority, not transmitted
											event->SetInt( "priority", 1 );	

											// Healed by another player.
											event->SetInt( "patient", m_pOuter->GetUserID() );
											event->SetInt( "healer", pHealScorer->GetUserID() );
											event->SetInt( "amount", m_aHealers[i].flHealedLastSecond );
											gameeventmanager->FireEvent( event );
										}

										// Can we figure out which item is doing this healing?
										if ( pHealScorer )
										{
											// Can be Mediguns or anything that gives off 'heal' buff like amputator aoe heal
											EconEntity_OnOwnerKillEaterEvent_Batched( pHealScorer->GetActiveTFWeapon(), pHealScorer, m_pOuter, kKillEaterEvent_AllyHealingDone, m_aHealers[i].flHealedLastSecond );
										}
									}

									m_aHealers[i].flHealedLastSecond = 0;
									m_flHealedPerSecondTimer = gpGlobals->curtime + 1.0f;
								}
							}
						}
						else
						{
							CTF_GameStats.Event_PlayerLeachedHealth( m_pOuter, m_aHealers[i].bDispenserHeal, flHealAmount );
						}
					}
				}
			}
		}

		if ( InCond( TF_COND_BURNING ) )
		{
			// Reduce the duration of this burn 
			float flReduction = 2.f * gpGlobals->frametime;	 // ( flReduction + 1 ) x faster reduction
			m_flAfterburnDuration -= flReduction;
		}
		if ( InCond( TF_COND_BLEEDING ) )
		{
			// Reduce the duration of this bleeding 
			float flReduction = 2;	 // ( flReduction + 1 ) x faster reduction
			FOR_EACH_VEC( m_PlayerBleeds, i )
			{
				m_PlayerBleeds[i].flBleedingRemoveTime -= flReduction * gpGlobals->frametime;
			}
		}
	}

	if ( !InCond( TF_COND_HEALTH_OVERHEALED ) && m_pOuter->GetHealth() > ( m_pOuter->GetMaxHealth() - m_pOuter->GetRuneHealthBonus() ) )
	{
		AddCond( TF_COND_HEALTH_OVERHEALED, PERMANENT_CONDITION );
	}
	else if ( InCond( TF_COND_HEALTH_OVERHEALED ) && m_pOuter->GetHealth() <= ( m_pOuter->GetMaxHealth() - m_pOuter->GetRuneHealthBonus() ) )
	{
		RemoveCond( TF_COND_HEALTH_OVERHEALED );
	}

	if ( bDecayHealth )
	{
		float flOverheal = GetMaxBuffedHealth( false, true );

		// If we're not being buffed, our health drains back to our max
		if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
		{
			// Items exist that get us over max health, without ever being healed, in which case our m_flBestOverhealDecayMult will still be -1.
			float flDrainMult = ( m_flBestOverhealDecayMult == -1.f ) ? 1.f : m_flBestOverhealDecayMult;
			float flBoostMaxAmount = flOverheal - m_pOuter->GetMaxHealth();
			float flDrain = flBoostMaxAmount / (tf_boost_drain_time.GetFloat() * flDrainMult);
			m_flHealFraction += (gpGlobals->frametime * flDrain);

			// Vampires have generous overheal on damage, so we decay it quickly
			if ( GetCarryingRuneType() == RUNE_VAMPIRE ) 
			{
				m_flHealFraction *= 3.0;
			}

			int nHealthToDrain = (int)m_flHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flHealFraction -= nHealthToDrain;

				// Manually subtract the health so we don't generate pain sounds / etc
				m_pOuter->m_iHealth -= nHealthToDrain;
			}
		}
		else if ( m_flBestOverhealDecayMult != -1 )
		{
			m_flBestOverhealDecayMult = -1;
		}

	}

	if ( bDecayDisguiseHealth )
	{
		float flOverheal = GetDisguiseMaxBuffedHealth( false, true );

		if ( InCond( TF_COND_DISGUISED ) && (GetDisguiseHealth() > GetDisguiseMaxHealth()) )
		{
			// Items exist that get us over max health, without ever being healed, in which case our m_flBestOverhealDecayMult will still be -1.
			float flDrainMult = (m_flBestOverhealDecayMult == -1) ? 1.0 : m_flBestOverhealDecayMult;
			float flBoostMaxAmount = flOverheal - GetDisguiseMaxHealth();
			float flDrain = (flBoostMaxAmount / tf_boost_drain_time.GetFloat()) * flDrainMult;
			m_flDisguiseHealFraction += (gpGlobals->frametime * flDrain);

			int nHealthToDrain = (int)m_flDisguiseHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flDisguiseHealFraction -= nHealthToDrain;

				// Reduce our fake disguised health by roughly the same amount
				m_iDisguiseHealth -= nHealthToDrain;
			}
		}
	}

	// Taunt
	if ( InCond( TF_COND_TAUNTING ) )
	{
		if ( m_pOuter->IsAllowedToRemoveTaunt() && gpGlobals->curtime > m_pOuter->GetTauntRemoveTime() )
		{
			RemoveCond( TF_COND_TAUNTING );
		}
	}
	if ( InCond( TF_COND_BURNING ) )
	{
		if ( TFGameRules() && TFGameRules()->IsTruceActive() && m_hBurnAttacker && m_hBurnAttacker->IsTruceValidForEnt() )
		{
			RemoveCond( TF_COND_BURNING );
		}
		else if ( m_flAfterburnDuration <= 0.f || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			// If we're underwater, put the fire out
			if ( m_pOuter->GetWaterLevel() >= WL_Waist )
			{
				// General achievement for jumping into water while you're on fire
				m_pOuter->AwardAchievement( ACHIEVEMENT_TF_FIRE_WATERJUMP );

				// Pyro achievement for forcing players into water
				if ( m_hBurnAttacker )
				{
					m_hBurnAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_FORCE_WATERJUMP );
				}
			}

			RemoveCond( TF_COND_BURNING );

			if ( InCond( TF_COND_HEALTH_BUFF ) )
			{
				// one or more players is healing us, send a "player_extinguished" event. We
				// need to send one for each player who's healing us.
				for ( int i = 0; i < m_aHealers.Count(); i++ )
				{
					Assert( m_aHealers[i].pHealer );

					if ( m_aHealers[i].bDispenserHeal )
					{
						CObjectDispenser *pDispenser = dynamic_cast<CObjectDispenser*>( m_aHealers[i].pHealer.Get() );
						if ( pDispenser )
						{
							CTFPlayer *pTFPlayer = pDispenser->GetBuilder();
							if ( pTFPlayer )
							{
								UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"player_extinguished\" against \"%s<%i><%s><%s>\" with \"dispenser\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
									pTFPlayer->GetPlayerName(),
									pTFPlayer->GetUserID(),
									pTFPlayer->GetNetworkIDString(),
									pTFPlayer->GetTeam()->GetName(),
									m_pOuter->GetPlayerName(),
									m_pOuter->GetUserID(),
									m_pOuter->GetNetworkIDString(),
									m_pOuter->GetTeam()->GetName(),
									(int)m_aHealers[i].pHealer->GetAbsOrigin().x, 
									(int)m_aHealers[i].pHealer->GetAbsOrigin().y,
									(int)m_aHealers[i].pHealer->GetAbsOrigin().z,
									(int)m_pOuter->GetAbsOrigin().x, 
									(int)m_pOuter->GetAbsOrigin().y,
									(int)m_pOuter->GetAbsOrigin().z );
							}
						}

//						continue;
					}

					EHANDLE pHealer = m_aHealers[i].pHealer;
					if ( m_aHealers[i].bDispenserHeal || !pHealer || !pHealer->IsPlayer() )
						pHealer = m_aHealers[i].pHealScorer;

					if ( !pHealer )
						continue;

					CTFPlayer *pTFPlayer = ToTFPlayer( pHealer );
					if ( pTFPlayer && !m_aHealers[i].bDispenserHeal )
					{
						UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"player_extinguished\" against \"%s<%i><%s><%s>\" with \"%s\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
							pTFPlayer->GetPlayerName(),
							pTFPlayer->GetUserID(),
							pTFPlayer->GetNetworkIDString(),
							pTFPlayer->GetTeam()->GetName(),
							m_pOuter->GetPlayerName(),
							m_pOuter->GetUserID(),
							m_pOuter->GetNetworkIDString(),
							m_pOuter->GetTeam()->GetName(),
							( pTFPlayer->GetActiveTFWeapon() ) ? pTFPlayer->GetActiveTFWeapon()->GetName() : "tf_weapon_medigun",
							(int)pTFPlayer->GetAbsOrigin().x, 
							(int)pTFPlayer->GetAbsOrigin().y,
							(int)pTFPlayer->GetAbsOrigin().z,
							(int)m_pOuter->GetAbsOrigin().x, 
							(int)m_pOuter->GetAbsOrigin().y,
							(int)m_pOuter->GetAbsOrigin().z );
					}

					// Tell the clients involved 
					CRecipientFilter involved_filter;
					CBasePlayer *pBasePlayerHealer = ToBasePlayer( pHealer );
					if ( pBasePlayerHealer )
					{
						involved_filter.AddRecipient( pBasePlayerHealer );
					}
					involved_filter.AddRecipient( m_pOuter );
					UserMessageBegin( involved_filter, "PlayerExtinguished" );
						WRITE_BYTE( pHealer->entindex() );
						WRITE_BYTE( m_pOuter->entindex() );
					MessageEnd();

					IGameEvent *event = gameeventmanager->CreateEvent( "player_extinguished" );
					if ( event )
					{
						event->SetInt( "victim", m_pOuter->entindex() );
						event->SetInt( "healer", pHealer->entindex() );

						item_definition_index_t nDefIndex = INVALID_ITEM_DEF_INDEX;
						if ( pTFPlayer && pTFPlayer->GetActiveTFWeapon() && pTFPlayer->GetActiveTFWeapon()->GetAttributeContainer() && pTFPlayer->GetActiveTFWeapon()->GetAttributeContainer()->GetItem() )
						{
							nDefIndex = pTFPlayer->GetActiveTFWeapon()->GetAttributeContainer()->GetItem()->GetItemDefinition()->GetDefinitionIndex();
						}

						event->SetInt( "itemdefindex", nDefIndex );

						gameeventmanager->FireEvent( event, true );
					}
				}
			}
		}
		else if ( gpGlobals->curtime >= m_flFlameBurnTime )
		{
			// Burn the player (if not pyro, who does not take persistent burning damage)
			if ( ( TF_CLASS_PYRO != m_pOuter->GetPlayerClass()->GetClassIndex() ) || InCond( TF_COND_BURNING_PYRO ) )
			{
				float flBurnDamage = TF_BURNING_DMG;
				int nKillType = TF_DMG_CUSTOM_BURNING;

				if ( m_hBurnWeapon )
				{
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hBurnWeapon, flBurnDamage, mult_wpn_burndmg );

					if ( m_hBurnWeapon.Get()->GetWeaponID() == TF_WEAPON_FLAREGUN )
					{
						nKillType = TF_DMG_CUSTOM_BURNING_FLARE;
					}
					else if ( m_hBurnWeapon.Get()->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
					{
						nKillType = TF_DMG_CUSTOM_BURNING_ARROW;
					}
				}

#ifdef DEBUG
				if ( tf_afterburn_debug.GetBool() )
				{
					engine->Con_NPrintf( 0, "Tick afterburn duration = %f", m_flAfterburnDuration );
				}
#endif // DEBUG

				// which degree are we burning?
				if ( m_flAfterburnDuration >= tf_afterburn_duration_ratio_third_degree * tf_afterburn_max_duration )
				{
					flBurnDamage *= tf_afterburn_mult_third_degree;
				}
				else if ( m_flAfterburnDuration >= tf_afterburn_duration_ratio_second_degree * tf_afterburn_max_duration )
				{
					flBurnDamage *= tf_afterburn_mult_second_degree;
				}
		
				// Halloween Spell
				if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
				{
					int iHalloweenSpell = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hBurnWeapon, iHalloweenSpell, halloween_green_flames );
					if ( iHalloweenSpell > 0 )
					{
						const char *pEffectName = "halloween_burningplayer_flyingbits";
						// Extra Halloween Particles
						DispatchParticleEffect( pEffectName, PATTACH_ABSORIGIN_FOLLOW, m_pOuter, 0, false );
					}
				}
	
				CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, m_hBurnWeapon, flBurnDamage, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, nKillType );
				m_pOuter->TakeDamage( info );

				// Give health to attacker if they are carrying the Vampire Powerup.
				if ( TFGameRules() && TFGameRules()->IsPowerupMode() )  
				{
					CTFPlayer *pTFAttacker = ToTFPlayer( GetConditionProvider( TF_COND_BURNING ) );

					if ( pTFAttacker && pTFAttacker != m_pOuter )
					{
						if ( pTFAttacker->m_Shared.GetCarryingRuneType() == RUNE_VAMPIRE )
						{
							pTFAttacker->TakeHealth( flBurnDamage, DMG_IGNORE_MAXHEALTH );
						}
					}
				}
			}

			m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
			m_flAfterburnDuration -= TF_BURNING_FREQUENCY;
		}

		if ( m_flNextBurningSound < gpGlobals->curtime )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_ONFIRE );
			m_flNextBurningSound = gpGlobals->curtime + 2.5;
		}
	}


	// Stops the drain hack.
	if ( m_pOuter->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		CWeaponMedigun *pWeapon = ( CWeaponMedigun* )m_pOuter->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
		if ( pWeapon && pWeapon->IsReleasingCharge() )
		{
			pWeapon->DrainCharge();
		}
	}

	TestAndExpireChargeEffect( MEDIGUN_CHARGE_INVULN );
	TestAndExpireChargeEffect( MEDIGUN_CHARGE_CRITICALBOOST );
	TestAndExpireChargeEffect( MEDIGUN_CHARGE_MEGAHEAL );
	//TestAndExpireChargeEffect( MEDIGUN_CHARGE_BULLET_RESIST );
	//TestAndExpireChargeEffect( MEDIGUN_CHARGE_BLAST_RESIST );
	//TestAndExpireChargeEffect( MEDIGUN_CHARGE_FIRE_RESIST );

	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		float flBlinkTime = TF_SPY_STEALTH_BLINKTIME;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flBlinkTime, cloak_blink_time_penalty );
		if ( flBlinkTime < ( gpGlobals->curtime - m_flLastStealthExposeTime ) )
		{
			RemoveCond( TF_COND_STEALTHED_BLINK );
		}
	}

	if ( InCond( TF_COND_FEIGN_DEATH ) )
	{
		if ( m_flFeignDeathEnd < gpGlobals->curtime )
		{
			RemoveCond( TF_COND_FEIGN_DEATH );
		}
	}

	if ( m_pOuter->GetWaterLevel() >= WL_Waist )
	{
		if ( InCond( TF_COND_URINE ) )
		{
			// If we're underwater, wash off the urine.
			RemoveCond( TF_COND_URINE );
		}

		if ( InCond( TF_COND_MAD_MILK ) )
		{
			// If we're underwater, wash off the Mad Milk.
			RemoveCond( TF_COND_MAD_MILK );
		}

		if ( InCond( TF_COND_GAS ) )
		{
			// If we're underwater, wash off the Gas.
			RemoveCond( TF_COND_GAS );
		}
	}

	if ( !InCond( TF_COND_DISGUISED ) )
	{
		// Remove our disguise weapon if we are ever not disguised and we have one.
		RemoveDisguiseWeapon();

		// also clear the disguise weapon list
		m_pOuter->ClearDisguiseWeaponList();
	}

	if ( InCond( TF_COND_BLEEDING ) )
	{
		FOR_EACH_VEC_BACK( m_PlayerBleeds, i )
		{
			bleed_struct_t& bleed = m_PlayerBleeds[i];
			if ( TFGameRules() && TFGameRules()->IsTruceActive() && bleed.hBleedingAttacker && ( bleed.hBleedingAttacker != m_pOuter ) && bleed.hBleedingAttacker->IsTruceValidForEnt() )
			{
				m_PlayerBleeds.FastRemove( i );
			}
			else if ( gpGlobals->curtime >= bleed.flBleedingRemoveTime && !bleed.bPermanentBleeding )
			{
				m_PlayerBleeds.FastRemove( i );
			}
			else if ( ( gpGlobals->curtime >= bleed.flBleedingTime ) )
			{
				bleed.flBleedingTime = gpGlobals->curtime + TF_BLEEDING_FREQUENCY;

				CTakeDamageInfo info( bleed.hBleedingAttacker, bleed.hBleedingAttacker, bleed.hBleedingWeapon, bleed.nBleedDmg, DMG_SLASH, bleed.nDmgType );
				m_pOuter->TakeDamage( info );

				// It's very possible we died from the take damage, which clears all our conditions
				// and nukes m_PlayerBleeds.  If that happens, bust out of this loop.
				if( m_PlayerBleeds.Count() == 0 )
					break;
			}
		}

		if ( !m_PlayerBleeds.Count() )
		{
			RemoveCond( TF_COND_BLEEDING );
		}
	}

	{
		m_flSpyTranqBuffDuration = 0;
	}

	if ( TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsPlayingRobotDestructionMode() )
	{
		RadiusCurrencyCollectionCheck();
	}

	if ( TFGameRules()->IsMannVsMachineMode() && m_pOuter->IsPlayerClass( TF_CLASS_SPY) )
	{
		// In MvM, Spies reveal other spies in a radius around them
		RadiusSpyScan();
	}
	if ( GetCarryingRuneType() == RUNE_PLAGUE )
	{
		RadiusHealthkitCollectionCheck();
	}

#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: call all the shared think funcs here
//-----------------------------------------------------------------------------
void CTFPlayerShared::SharedThink( void )
{
	ConditionThink();
	InvisibilityThink();

	// keep trying to switch weapon if needed
	if ( m_flHolsterAnimTime > 0.f )
	{
		m_pOuter->Weapon_Switch( m_hSwitchTo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do CLIENT/SERVER SHARED condition thinks.
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionThink( void )
{
	// Client Only Updates Meters for Local Only
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
#endif
	{
		UpdateCloakMeter();
		UpdateRageBuffsAndRage();
		UpdateEnergyDrinkMeter();
		UpdateChargeMeter();
		DemoShieldChargeThink();
	}

	VehicleThink();

	if ( m_pOuter->GetFlags() & FL_ONGROUND )
	{
		// Airborne conditions end on ground contact
		RemoveCond( TF_COND_KNOCKED_INTO_AIR );
		RemoveCond( TF_COND_AIR_CURRENT );

		RemoveCond( TF_COND_PARACHUTE_ACTIVE );
		RemoveCond( TF_COND_PARACHUTE_DEPLOYED );

		if ( InCond( TF_COND_ROCKETPACK ) )
		{
			// Make sure we're still not dealing with launch, where it's possible
			// to hit your head and fall to the ground before the second stage.
			CTFWeaponBase *pRocketPack = m_pOuter->Weapon_OwnsThisID( TF_WEAPON_ROCKETPACK );
			if ( pRocketPack )
			{
				if ( gpGlobals->curtime > ( static_cast< CTFRocketPack* >( pRocketPack )->GetRefireTime() ) )
				{
#ifdef CLIENT_DLL
					if ( prediction->IsFirstTimePredicted() )
#endif
					{
						CPASAttenuationFilter filter( m_pOuter );
						filter.UsePredictionRules();
						m_pOuter->EmitSound( filter, m_pOuter->entindex(), "Weapon_RocketPack.BoostersShutdown" );
						m_pOuter->EmitSound( filter, m_pOuter->entindex(), "Weapon_RocketPack.Land" );
					}
					RemoveCond( TF_COND_ROCKETPACK );

#ifdef GAME_DLL
					IGameEvent *pEvent = gameeventmanager->CreateEvent( "rocketpack_landed" );
					if ( pEvent )
					{
						pEvent->SetInt( "userid", m_pOuter->GetUserID() );
						gameeventmanager->FireEvent( pEvent );
					}
#endif
				}
			}
		}
	}

	// See if we should be pulsing our radius heal
	PulseMedicRadiusHeal();
	PulseKingRuneBuff();

	m_ConditionList.Think();

	if ( InCond( TF_COND_STUNNED ) )
	{
#ifdef GAME_DLL
		if ( IsControlStunned() )
		{
			m_pOuter->SetAbsAngles( m_pOuter->m_angTauntCamera );
			m_pOuter->SetLocalAngles( m_pOuter->m_angTauntCamera );
		}
#endif
		if ( GetActiveStunInfo() && gpGlobals->curtime > GetActiveStunInfo()->flExpireTime )
		{
#ifdef GAME_DLL	
			m_PlayerStuns.Remove( m_iStunIndex );
			m_iStunIndex = -1;

			// Apply our next stun
			if ( m_PlayerStuns.Count() )
			{
				int iStrongestIdx = 0;
				for ( int i = 1; i < m_PlayerStuns.Count(); i++ )
				{
					if ( m_PlayerStuns[i].flStunAmount > m_PlayerStuns[iStrongestIdx].flStunAmount )
					{
						iStrongestIdx = i;
					}
				}
				m_iStunIndex = iStrongestIdx;

				AddCond( TF_COND_STUNNED, -1.f, m_PlayerStuns[m_iStunIndex].hPlayer );
				m_iMovementStunParity = ( m_iMovementStunParity + 1 ) & ( ( 1 << MOVEMENTSTUN_PARITY_BITS ) - 1 ); 

				Assert( GetActiveStunInfo() );
			}
			else
			{
				RemoveCond( TF_COND_STUNNED );
			}
#endif // GAME_DLL

			UpdateLegacyStunSystem();
		}
		else if ( IsControlStunned() && GetActiveStunInfo() && ( gpGlobals->curtime > GetActiveStunInfo()->flStartFadeTime ) )
		{
			// Control stuns have a final anim to play.
			ControlStunFading();
		}

#ifdef CLIENT_DLL
		// turn off stun effect that gets turned on when incomplete stun msg is received on the client
		if ( GetActiveStunInfo() && GetActiveStunInfo()->iStunFlags & TF_STUN_NO_EFFECTS )
		{
			if ( m_pOuter->m_pStunnedEffect )
			{
				// Remove stun stars if they are still around.
				// They might be if we died, etc.
				m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pStunnedEffect );
				m_pOuter->m_pStunnedEffect = NULL;
			}
		}
#endif
	}

	if ( InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
	{
#ifdef GAME_DLL
		static struct 
		{
			float flTimeLeft;
			int nStage;
		} s_vecBombStages[] = { { 8.0f, 0 }, { 3.0f, 1 }, { 0.0f, 2 } };

		for ( int i = 0; i < ARRAYSIZE( s_vecBombStages ); ++i )
		{
			if ( m_ConditionData[TF_COND_HALLOWEEN_BOMB_HEAD].m_flExpireTime >= s_vecBombStages[i].flTimeLeft )
			{
				m_nHalloweenBombHeadStage = s_vecBombStages[i].nStage;
				break;
			}
		}

		if ( TFGameRules() && TFGameRules()->GetActiveBoss() && ( TFGameRules()->GetActiveBoss()->GetBossType() == HALLOWEEN_BOSS_MERASMUS ) )
		{
			if ( m_pOuter->IsAlive() )
			{
				Vector vToBoss = m_pOuter->EyePosition() - TFGameRules()->GetActiveBoss()->WorldSpaceCenter();
				if ( vToBoss.IsLengthLessThan( 100.f ) )
				{
					CMerasmus* pMerasmus = assert_cast< CMerasmus* >( TFGameRules()->GetActiveBoss() );
					if ( pMerasmus )
					{
						pMerasmus->AddStun( m_pOuter );
					}
				}
			}
		}
#else
		m_pOuter->HalloweenBombHeadUpdate();
#endif 
	}
	else
	{
#ifdef GAME_DLL
		m_nHalloweenBombHeadStage = 0;
#endif
	}

#ifdef GAME_DLL
	if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_VIADUCT ) && InCond( TF_COND_PURGATORY ) )
	{
		// escalating injury multiplier while in purgatory
		if ( m_pOuter->m_purgatoryPainMultiplierTimer.IsElapsed() )
		{
			++m_pOuter->m_purgatoryPainMultiplier;

			// injury multiplies rapidly after initial period
			m_pOuter->m_purgatoryPainMultiplierTimer.Start( 10.0f );
		}
	}
#endif

	CheckDisguiseTimer();

#ifdef CLIENT_DLL
	if ( InCond( TF_COND_TAUNTING ) && m_flTauntParticleRefireTime > 0.0f && gpGlobals->curtime >= m_flTauntParticleRefireTime )
	{
		FireClientTauntParticleEffects();
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::CheckDisguiseTimer( void )
{
	if ( InCond( TF_COND_DISGUISING ) && GetDisguiseCompleteTime() > 0 )
	{
		if ( gpGlobals->curtime > GetDisguiseCompleteTime() )
		{
			CompleteDisguise();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddZoomed( void )
{
#ifdef CLIENT_DLL
	// hide cosmetic while zoom in thirdperson
	if ( m_pOuter == C_TFPlayer::GetLocalTFPlayer() && ::input->CAM_IsThirdPerson() )
	{
		m_pOuter->UpdateWearables();
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveZoomed( void )
{
#ifdef GAME_DLL
	m_pOuter->SetFOV( m_pOuter, 0, 0.1f );
#endif // GAME_DLL

#ifdef CLIENT_DLL
	// unhide cosmetic after zoom in thirdperson
	if ( m_pOuter == C_TFPlayer::GetLocalTFPlayer() && ::input->CAM_IsThirdPerson() )
	{
		m_pOuter->UpdateWearables();
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && ( !prediction->IsFirstTimePredicted() || m_bSyncingConditions ) )
		return;

	if ( m_pOuter->m_pDisguisingEffect )
	{
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
	}

	if ( !m_pOuter->IsLocalPlayer() && ( !IsStealthed() || !m_pOuter->IsEnemyPlayer() ) )
	{
		const char *pEffectName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "spy_start_disguise_red" : "spy_start_disguise_blue";
		m_pOuter->m_pDisguisingEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
		m_pOuter->m_flDisguiseEffectStartTime = gpGlobals->curtime;
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: set up effects for when player finished disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguised( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		// turn off disguising particles
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
	m_pOuter->m_flDisguiseEndEffectStartTime = gpGlobals->curtime;

	UpdateCritBoostEffect( kCritBoost_ForceRefresh );

	m_pOuter->UpdateSpyStateChange();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDemoCharge( void )
{
#ifdef CLIENT_DLL
	m_pOuter->StopSound( "DemoCharge.ChargeCritOn" );
	m_pOuter->EmitSound( "DemoCharge.ChargeCritOn" );
	UpdateCritBoostEffect();
#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Return the team that the spy is displayed as.
// Not disguised: His own team
// Disguised: The team he is disguised as
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDisplayedTeam( void ) const
{
	int iVisibleTeam = m_pOuter->GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
	if ( InCond( TF_COND_DISGUISED ) && m_pOuter->IsEnemyPlayer() )
	{
		iVisibleTeam = GetDisguiseTeam();
	}

	return iVisibleTeam;
}

//-----------------------------------------------------------------------------
// Purpose: start, end, and changing disguise classes
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDisguiseChanged( void )
{
	// recalc disguise model index
	//RecalcDisguiseWeapon( true );
	m_pOuter->UpdateSpyStateChange();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddInvulnerable( void )
{
#ifdef CLIENT_DLL

	if ( m_pOuter->IsLocalPlayer() )
	{
		const char *pEffectName = NULL;

		switch( m_pOuter->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
		default:
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE;
			break;
		case TF_TEAM_RED:
			pEffectName =  TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED;
			break;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}

		if ( m_pOuter->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_HEAVY_RECEIVE_UBER_GRIND );
		}
	}
#else
	// remove any persistent damaging conditions
	if ( InCond( TF_COND_BURNING ) )
	{
		RemoveCond( TF_COND_BURNING );
	}

	if ( InCond( TF_COND_URINE ) )
	{
		RemoveCond( TF_COND_URINE );
	}

	if ( InCond( TF_COND_BLEEDING ) )
	{
		RemoveCond( TF_COND_BLEEDING );
	}

	if ( InCond( TF_COND_MAD_MILK ) )
	{
		RemoveCond( TF_COND_MAD_MILK );
	}

	if ( InCond( TF_COND_GAS ) )
	{
		RemoveCond( TF_COND_GAS );
	}

	if ( InCond( TF_COND_PLAGUE ) )
	{
		RemoveCond( TF_COND_PLAGUE );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInvulnerable( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is an invuln material 
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial &&
				( FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE ) || 
				  FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED ) ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInvulnerableWearingOff( void )
{
#ifdef CLIENT_DLL
	m_flInvulnerabilityRemoveTime = gpGlobals->curtime;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddPhase( void )
{
	UpdatePhaseEffects();

#ifdef CLIENT_DLL

#else
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_DODGING, "started_dodging:1" );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemovePhase( void )
{
	RemovePhaseEffects();

#ifdef CLIENT_DLL

#else
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_DODGING, "started_dodging:0" );
	
	// Tell this player how much damage they dodged.
	CSingleUserRecipientFilter user( m_pOuter );
	UserMessageBegin( user, "DamageDodged" );
		WRITE_SHORT( clamp( m_iPhaseDamage, 0, 10000 ) );
	MessageEnd();

	if ( m_iPhaseDamage )
	{
		// Apply slow based on how much damage we took while active
		float flSlowDuration = 5.f; //RemapValClamped( m_iPhaseDamage, 10.f, 1000.f, 2.f, 6.f );
		float flSlowAmount = RemapValClamped( m_iPhaseDamage, 10.f, 200.f, 0.25f, 0.5f );
		StunPlayer( flSlowDuration, flSlowAmount, TF_STUN_MOVEMENT | TF_STUN_SOUND, m_pOuter );
	}

	m_iPhaseDamage = 0;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddUrine( void )
{
#ifdef CLIENT_DLL
	if ( tf_colorblindassist.GetBool() )
	{
		m_pOuter->AddOverheadEffect( "peejar_icon" );
	}
	
	if ( !m_pOuter->m_pUrineEffect )
	{
		m_pOuter->m_pUrineEffect = m_pOuter->ParticleProp()->Create( "peejar_drips", PATTACH_ABSORIGIN_FOLLOW ); // pEffect! Kek!
	}

	if ( m_pOuter->m_pUrineEffect )
	{
		m_pOuter->ParticleProp()->AddControlPoint( m_pOuter->m_pUrineEffect, 1, m_pOuter, PATTACH_ABSORIGIN_FOLLOW );
	}
	
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_URINE, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#else

//	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_DODGING, "urine_on" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveUrine( void )
{
	
#ifdef CLIENT_DLL
	m_pOuter->RemoveOverheadEffect( "peejar_icon", true );

	if ( m_pOuter->m_pUrineEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pUrineEffect );
		m_pOuter->m_pUrineEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is urine
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_URINE ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#else
//	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_DODGING, "urine_off" );

	if ( m_hPeeAttacker )
	{
		// Tell the clients involved in the jarate
		CRecipientFilter involved_filter;
		involved_filter.AddRecipient( m_pOuter );
		involved_filter.AddRecipient( m_hPeeAttacker );
		UserMessageBegin( involved_filter, "PlayerJaratedFade" );
			WRITE_BYTE( m_hPeeAttacker->entindex() );
			WRITE_BYTE( m_pOuter->entindex() );
		MessageEnd();
	}

	m_hPeeAttacker = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMarkedForDeath( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();

	if ( m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->EmitSound( "Weapon_Marked_for_Death.Indicator" );
	}
	else if ( !InCond( TF_COND_DISGUISED ) && !IsStealthed() )
	{
		m_pOuter->EmitSound( "Weapon_Marked_for_Death.Initial" );
	}

	/*
	// Do we want to have a screen overlay effect?
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_URINE, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
	*/
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMarkedForDeath( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();

	/*
	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is urine
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_URINE ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
	*/
#endif
}


//-----------------------------------------------------------------------------
// Purpose: PARACHUTE
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddCondParachute( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && ( prediction->IsFirstTimePredicted() && !m_bSyncingConditions ) )
	{
		m_pOuter->EmitSound( "Parachute_open" );
	}

	if ( InCond( TF_COND_HALLOWEEN_KART ) )
	{
		if ( !m_hKartParachuteEntity )
		{
			C_BaseAnimating* pBanner = new C_BaseAnimating;
			//if ( pBanner )
			//	return;

			pBanner->m_nSkin = 0;
			pBanner->InitializeAsClientEntity( "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_parachute.mdl", RENDER_GROUP_OPAQUE_ENTITY );
			pBanner->ForceClientSideAnimationOn();
			int iSpine = m_pOuter->LookupBone( "bip_spine_3" );
			Assert( iSpine != -1 );
			if ( iSpine != -1 )
			{
				pBanner->AttachEntityToBone( m_pOuter, iSpine );
			}

			int sequence = pBanner->SelectWeightedSequence( ACT_PARACHUTE_DEPLOY_IDLE );
			pBanner->ResetSequence( sequence );
			m_hKartParachuteEntity.Set( pBanner );
		}
	}
	else
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "parachute_deploy" );
		if ( event )
		{
			event->SetInt( "index", m_pOuter->entindex() );
			gameeventmanager->FireEventClientSide( event );
		}
	}
#endif
}
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveCondParachute( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && ( prediction->IsFirstTimePredicted() && !m_bSyncingConditions ) )
	{
		m_pOuter->EmitSound( "Parachute_close" );
	}

	if ( m_hKartParachuteEntity )
	{
		m_hKartParachuteEntity->Release();
		m_hKartParachuteEntity = NULL;
	}

	if ( !InCond( TF_COND_HALLOWEEN_KART ) )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "parachute_holster" );
		if ( event )
		{
			event->SetInt( "index", m_pOuter->entindex() );
			gameeventmanager->FireEventClientSide( event );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenHellHeal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenHellHeal( void )
{
#ifdef GAME_DLL
	StopHealing( m_pOuter );
#endif // SERVER_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMadMilk( void )
{
#ifdef GAME_DLL
	Assert( InCond( TF_COND_MAD_MILK ) );

	// Check for the attribute that extends duration on successive hits
	if ( m_ConditionData[TF_COND_MAD_MILK].m_bPrevActive )
	{
		CBaseEntity *pProvider = GetConditionProvider( TF_COND_MAD_MILK );
		if ( pProvider )
		{
			int iMadMilkSyringes = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pProvider, iMadMilkSyringes, mad_milk_syringes );
			if ( iMadMilkSyringes )
			{
				float flDuration = GetConditionDuration( TF_COND_MAD_MILK ) + 0.5f;
				SetConditionDuration( TF_COND_MAD_MILK, Min( flDuration , 4.f ) );
			}
		}
	}
#else
	if ( !m_pOuter->m_pMilkEffect )
	{
		m_pOuter->m_pMilkEffect = m_pOuter->ParticleProp()->Create( "peejar_drips_milk", PATTACH_ABSORIGIN_FOLLOW );
	}

	m_pOuter->ParticleProp()->AddControlPoint( m_pOuter->m_pMilkEffect, 1, m_pOuter, PATTACH_ABSORIGIN_FOLLOW );

// 	if ( m_pOuter->IsLocalPlayer() )
// 	{
// 		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_MILK, TEXTURE_GROUP_CLIENT_EFFECTS, false );
// 		if ( !IsErrorMaterial( pMaterial ) )
// 		{
// 			view->SetScreenOverlayMaterial( pMaterial );
// 		}
// 	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMadMilk( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pMilkEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pMilkEffect );
		m_pOuter->m_pMilkEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
//		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

//		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_MILK ) )
//		{
//			view->SetScreenOverlayMaterial( NULL );
//		}
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerShared::taunt_particle_state_t CTFPlayerShared::GetClientTauntParticleDesiredState() const
{
	const itemid_t unTauntSourceItemID = GetTauntSourceItemID();
	if ( unTauntSourceItemID != INVALID_ITEM_ID )
	{
		CSteamID steamIDForPlayer;
		m_pOuter->GetSteamID( &steamIDForPlayer );

		CPlayerInventory *pInventory = InventoryManager()->GetInventoryForAccount( steamIDForPlayer.GetAccountID() );
		CEconItemView *pTauntItem = pInventory ? pInventory->GetInventoryItemByItemID( unTauntSourceItemID ) : NULL;

		if ( pTauntItem )
		{
			static CSchemaAttributeDefHandle pAttrDef_TauntAttachParticleIndex( "taunt attach particle index" );
			uint32 unUnusualEffectIndex = 0;
			if ( pTauntItem->FindAttribute( pAttrDef_TauntAttachParticleIndex, &unUnusualEffectIndex ) && unUnusualEffectIndex > 0 )
			{
				const attachedparticlesystem_t *pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( unUnusualEffectIndex );
				if ( pParticleSystem )
				{
					// TF Team Color Particles
					if ( m_pOuter->GetTeamNumber() == TF_TEAM_BLUE && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_red" ) )
					{
						static char pBlue[256];
						V_StrSubst( pParticleSystem->pszSystemName, "_teamcolor_red", "_teamcolor_blue", pBlue, 256 );
						pParticleSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pBlue );
					}
					else if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_blue" ) )
					{
						// Guard against accidentally giving out the blue team color (support tool)
						static char pRed[256];
						V_StrSubst( pParticleSystem->pszSystemName, "_teamcolor_blue", "_teamcolor_red", pRed, 256 );
						pParticleSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pRed );
					}

					return taunt_particle_state_t( pParticleSystem->pszSystemName, pParticleSystem->fRefireTime );
				}
			}

			for ( int i=0; i<m_pOuter->GetNumWearables(); ++i )
			{
				C_EconWearable *pWearable = m_pOuter->GetWearable( i );
				CEconItemView *pItem = pWearable && pWearable->GetAttributeContainer() && pWearable->GetAttributeContainer()->GetItem() ? pWearable->GetAttributeContainer()->GetItem() : NULL;
				
				// check for Unusual Cap def index (1173)
				if ( pItem && pItem->GetItemDefIndex() == 1173 && pItem->FindAttribute( pAttrDef_TauntAttachParticleIndex, &unUnusualEffectIndex ) && unUnusualEffectIndex > 0 )
				{
					const attachedparticlesystem_t *pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( unUnusualEffectIndex );
					if ( pParticleSystem )
					{
						// TF Team Color Particles
						if ( m_pOuter->GetTeamNumber() == TF_TEAM_BLUE && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_red" ) )
						{
							static char pBlue[256];
							V_StrSubst( pParticleSystem->pszSystemName, "_teamcolor_red", "_teamcolor_blue", pBlue, 256 );
							pParticleSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pBlue );
						}
						else if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_blue" ) )
						{
							// Guard against accidentally giving out the blue team color (support tool)
							static char pRed[256];
							V_StrSubst( pParticleSystem->pszSystemName, "_teamcolor_blue", "_teamcolor_red", pRed, 256 );
							pParticleSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pRed );
						}

						return taunt_particle_state_t( pParticleSystem->pszSystemName, pParticleSystem->fRefireTime );
					}
				}
			}

			// do community_sparkle effect if this is a self-made or community item
			const int iQualityParticleType = pTauntItem->GetQualityParticleType();
			if ( iQualityParticleType > 0 )
			{
				const attachedparticlesystem_t *pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iQualityParticleType );
				if ( pParticleSystem )
				{
					return taunt_particle_state_t( pParticleSystem->pszSystemName, pParticleSystem->fRefireTime );
				}
			}
		}
	}

	return taunt_particle_state_t( NULL, 0.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::FireClientTauntParticleEffects()
{
	taunt_particle_state_t TauntParticleState = GetClientTauntParticleDesiredState();
	if ( TauntParticleState.first )
	{
		if ( m_pOuter->m_pTauntEffect )
		{
			m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pTauntEffect );
		}

		if ( !m_pOuter->GetTauntEconItemView() )
			return;

		if ( !m_pOuter->GetTauntEconItemView()->GetStaticData() )
			return;

		if ( !m_pOuter->GetTauntEconItemView()->GetStaticData()->GetTauntData() )
			return;

		const char *pszAttachment = m_pOuter->GetTauntEconItemView()->GetStaticData()->GetTauntData()->GetParticleAttachment();
		int iAttachment = pszAttachment ? m_pOuter->LookupAttachment( pszAttachment ) : INVALID_PARTICLE_ATTACHMENT;
		m_pOuter->m_pTauntEffect = m_pOuter->ParticleProp()->Create( TauntParticleState.first, iAttachment != INVALID_PARTICLE_ATTACHMENT ? PATTACH_POINT_FOLLOW : PATTACH_ABSORIGIN_FOLLOW, iAttachment, vec3_origin );

		if ( TauntParticleState.second > 0.0f )
		{
			m_flTauntParticleRefireTime = gpGlobals->curtime + TauntParticleState.second;
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTaunting( void )
{
	CTFWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();
	if ( pWpn )
	{
		// cancel any reload in progress.
		pWpn->AbortReload();

		// Check for taunt healing.
		if ( GetTauntIndex() == TAUNT_BASE_WEAPON )
		{
			int iAOEHeal = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWpn, iAOEHeal, enables_aoe_heal );
			if ( iAOEHeal == 1 )
			{
				Heal_Radius( true );
			}
		}
	}

	// Unzoom if we are a sniper zoomed!
	InstantlySniperUnzoom();

	if ( ( m_pOuter->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) || m_pOuter->IsPlayerClass( TF_CLASS_SCOUT ) ) && GetTauntIndex() == TAUNT_BASE_WEAPON )
	{
		CTFLunchBox *pLunchBox = dynamic_cast <CTFLunchBox *> ( pWpn );
		if ( pLunchBox )
		{
			pLunchBox->DrainAmmo();
		}
	}

#ifdef GAME_DLL
	m_pOuter->PlayWearableAnimsForPlaybackEvent( WAP_START_TAUNTING );
#else
	FireClientTauntParticleEffects();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTaunting( void )
{
#ifdef GAME_DLL

	m_pOuter->StopTaunt( false );

	if ( IsControlStunned() )
	{
		m_pOuter->SetAbsAngles( m_pOuter->m_angTauntCamera );
		m_pOuter->SetLocalAngles( m_pOuter->m_angTauntCamera );
	}
#endif // GAME_DLL

	// Stop aoe healing if it's active.
	Heal_Radius( false );

	// We're done taunting, our weapons are not being repurposed anymore
	for ( int i = 0; i < m_pOuter->WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *) m_pOuter->GetWeapon(i);
		if ( !pWpn )
			continue;
		
		pWpn->SetIsBeingRepurposedForTaunt( false );
	}

#ifdef GAME_DLL
	// Switch to our melee weapon, if we are at the end of a type 2 lunchbox taunt.
	if ( m_bBiteEffectWasApplied && InCond( TF_COND_CANNOT_SWITCH_FROM_MELEE ) )
	{
		CBaseCombatWeapon *pWpn = m_pOuter->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
		if ( pWpn )
		{
			m_pOuter->Weapon_Switch( pWpn );
		}
		else
		{
			// Safety net
			RemoveCond( TF_COND_ENERGY_BUFF );
			RemoveCond( TF_COND_CANNOT_SWITCH_FROM_MELEE );
		}
	}

	m_bBiteEffectWasApplied = false;

	if ( m_pOuter->m_hTauntItem != NULL )
	{
		// destroy the item we were showing off
		UTIL_Remove( m_pOuter->m_hTauntItem );
		m_pOuter->m_hTauntItem = NULL;
	}

	m_pOuter->ClearTauntAttack();

	m_pOuter->PlayWearableAnimsForPlaybackEvent( WAP_STOP_TAUNTING );

	m_pOuter->HandleWeaponSlotAfterTaunt();
#else
	CSteamID steamIDForPlayer;
	m_pOuter->GetSteamID( &steamIDForPlayer );

	int nMapDonationAmount = MapInfo_GetDonationAmount( steamIDForPlayer.GetAccountID(), engine->GetLevelName() );
	m_pOuter->SetFootStamps( nMapDonationAmount );

	if ( m_pOuter->m_pTauntEffect )
	{
		m_pOuter->ParticleProp()->StopEmissionAndDestroyImmediately( m_pOuter->m_pTauntEffect );
		m_pOuter->m_pTauntEffect = NULL;
	}

	m_flTauntParticleRefireTime = 0.0f;
#endif

	m_pOuter->m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );

	// when we stop taunting, make sure active weapon is visible
	if ( m_pOuter->GetActiveWeapon() )
	{
		m_pOuter->GetActiveWeapon()->SetWeaponVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBleeding( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_BLEED, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#else
	// We should have at least one bleed entry
	Assert( m_PlayerBleeds.Count() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBleeding( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_BLEED ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#else
	m_PlayerBleeds.RemoveAll();
#endif
}

const char* CTFPlayerShared::GetSoldierBuffEffectName( void )
{
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( m_pOuter->GetTeamNumber() == TF_TEAM_BLUE )
		{
			// MVM robot version has fewer particles. Helps keep the framerate up.
			return "soldierbuff_mvm";
		}
		else
		{
			return "soldierbuff_red_soldier";
		}
	}
	else
	{
		if ( m_pOuter->GetTeamNumber() == TF_TEAM_BLUE )
		{
			return "soldierbuff_blue_soldier";
		}
		else
		{
			return "soldierbuff_red_soldier";
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddSoldierOffensiveBuff( void )
{
#ifdef CLIENT_DLL
	const char* strBuffName = GetSoldierBuffEffectName();

	if ( !m_pOuter->m_pSoldierOffensiveBuffEffect )
	{
		m_pOuter->m_pSoldierOffensiveBuffEffect = m_pOuter->ParticleProp()->Create( strBuffName, PATTACH_ABSORIGIN_FOLLOW );
	}
#endif
}

void CTFPlayerShared::OnRemoveSoldierOffensiveBuff( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pSoldierOffensiveBuffEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pSoldierOffensiveBuffEffect );
		m_pOuter->m_pSoldierOffensiveBuffEffect = NULL;
	}
#endif
}

void CTFPlayerShared::OnAddSoldierDefensiveBuff( void )
{
#ifdef CLIENT_DLL
	const char* strBuffName = GetSoldierBuffEffectName();

	if ( !m_pOuter->m_pSoldierDefensiveBuffEffect )
	{
		m_pOuter->m_pSoldierDefensiveBuffEffect = m_pOuter->ParticleProp()->Create( strBuffName, PATTACH_ABSORIGIN_FOLLOW );
	}
#endif
}

void CTFPlayerShared::OnRemoveSoldierDefensiveBuff( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pSoldierDefensiveBuffEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pSoldierDefensiveBuffEffect );
		m_pOuter->m_pSoldierDefensiveBuffEffect = NULL;
	}
#endif
}

void CTFPlayerShared::OnAddSoldierOffensiveHealthRegenBuff( void )
{
#ifdef GAME_DLL
	AddCond( TF_COND_SPEED_BOOST );
#else
	const char* strBuffName = GetSoldierBuffEffectName();

	if ( !m_pOuter->m_pSoldierOffensiveHealthRegenBuffEffect )
	{
		m_pOuter->m_pSoldierOffensiveHealthRegenBuffEffect = m_pOuter->ParticleProp()->Create( strBuffName, PATTACH_ABSORIGIN_FOLLOW );
	}
#endif
}

void CTFPlayerShared::OnRemoveSoldierOffensiveHealthRegenBuff( void )
{
#ifdef GAME_DLL
	RemoveCond( TF_COND_SPEED_BOOST );
#else
	if ( m_pOuter->m_pSoldierOffensiveHealthRegenBuffEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pSoldierOffensiveHealthRegenBuffEffect );
		m_pOuter->m_pSoldierOffensiveHealthRegenBuffEffect = NULL;
	}
#endif
}

void CTFPlayerShared::OnAddSoldierNoHealingDamageBuff( void )
{
#ifdef CLIENT_DLL
	const char* strBuffName = GetSoldierBuffEffectName();

	if ( !m_pOuter->m_pSoldierNoHealingDamageBuffEffect )
	{
		m_pOuter->m_pSoldierNoHealingDamageBuffEffect = m_pOuter->ParticleProp()->Create( strBuffName, PATTACH_ABSORIGIN_FOLLOW );
	}
#endif
}

void CTFPlayerShared::OnRemoveSoldierNoHealingDamageBuff( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pSoldierNoHealingDamageBuffEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pSoldierNoHealingDamageBuffEffect );
		m_pOuter->m_pSoldierNoHealingDamageBuffEffect = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddOffenseBuff( void )
{
	OnAddSoldierOffensiveBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveOffenseBuff( void )
{
	OnRemoveSoldierOffensiveBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDefenseBuff( void )
{
	OnAddSoldierDefensiveBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDefenseBuff( void )
{
	OnRemoveSoldierDefensiveBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddOffenseHealthRegenBuff( void )
{
	OnAddSoldierOffensiveHealthRegenBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveOffenseHealthRegenBuff( void )
{
	OnRemoveSoldierOffensiveHealthRegenBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddNoHealingDamageBuff( void )
{
	OnAddSoldierNoHealingDamageBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveNoHealingDamageBuff( void )
{
	OnRemoveSoldierNoHealingDamageBuff();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddSpeedBoost( bool IsNonCombat )
{
#ifdef CLIENT_DLL
	const char* strBuffName = "speed_boost_trail";

	if ( !m_pOuter->m_pSpeedBoostEffect )
	{
		// No speedlines at all for stealth or feign death 
		if ( !InCond( TF_COND_STEALTHED ) && !InCond(TF_COND_FEIGN_DEATH) )
		{
			m_pOuter->m_pSpeedBoostEffect = m_pOuter->ParticleProp()->Create( strBuffName, PATTACH_ABSORIGIN_FOLLOW );
		}
	}
	
	// InCombat is played on the teleporter for all players to here
	// "Building_Speedpad.BoostStart"
	if ( !IsNonCombat && m_pOuter->IsLocalPlayer())
	{
		m_pOuter->EmitSound( "DisciplineDevice.PowerUp" );
	}
#else // !CLIENT_DLL
	m_pOuter->TeamFortress_SetSpeed();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveSpeedBoost( bool IsNonCombat )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pSpeedBoostEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pSpeedBoostEffect );
		m_pOuter->m_pSpeedBoostEffect = NULL;
	}

	if ( !IsNonCombat && m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->EmitSound( "DisciplineDevice.PowerDown" );
	}
	else
	{
		m_pOuter->EmitSound( "Building_Speedpad.BoostStop" );
	}
#else // !CLIENT_DLL
	m_pOuter->TeamFortress_SetSpeed();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Applied to bots
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddSapped( void )
{
#ifdef CLIENT_DLL
	if ( !m_pOuter->m_pSappedPlayerEffect )
	{
		const char* szParticle = "sapper_sentry1_fx";
		m_pOuter->m_pSappedPlayerEffect = m_pOuter->ParticleProp()->Create( szParticle, PATTACH_POINT_FOLLOW, "head" );
	}
#endif	// CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveSapped( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pSappedPlayerEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pSappedPlayerEffect );
		m_pOuter->m_pSappedPlayerEffect = NULL;
	}
#endif	// CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Applied to bots
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddReprogrammed( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveReprogrammed( void )
{
}

void CTFPlayerShared::OnAddDisguisedAsDispenser( void )
{
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenBombHead( void )
{
#ifdef CLIENT_DLL
	m_pOuter->HalloweenBombHeadUpdate();
	m_pOuter->CreateBombonomiconHint();
#else
	if ( InCond( TF_COND_HALLOWEEN_KART ) )
	{
		RemoveAttributeFromPlayer( "head scale" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenBombHead( void )
{
#ifdef CLIENT_DLL
	m_pOuter->HalloweenBombHeadUpdate();
	m_pOuter->DestroyBombonomiconHint();
#else
	if ( InCond( TF_COND_HALLOWEEN_KART ) )
	{
		ApplyAttributeToPlayer( "head scale", 3.f );
	}

	if ( m_pOuter->IsAlive() )
	{
		m_pOuter->MerasmusPlayerBombExplode( false );

		Vector vecOrigin = m_pOuter->GetAbsOrigin();
		// explode has a small force but we want to increase it
		if ( InCond ( TF_COND_HALLOWEEN_KART ) )
		{
			if ( !m_pOuter->GetKartBombHeadTarget() )
			{
				m_pOuter->AddHalloweenKartPushEvent( m_pOuter, NULL, NULL, Vector( 0, 0, 100 ), 50 );
			}
			m_pOuter->SetKartBombHeadTarget( NULL );
		}
		else if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) )
		{
			TFGameRules()->PushAllPlayersAway( vecOrigin, 150, 400, TEAM_ANY );
		}

		// Particle
		CPVSFilter filter( vecOrigin );
		TE_TFParticleEffect( filter, 0.0, "bombinomicon_burningdebris", vecOrigin, vec3_angle );
	}
#endif // GAME_DLL
}

void CTFPlayerShared::OnAddHalloweenThriller( void )
{
#ifdef CLIENT_DLL
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		if ( pLocalPlayer == m_pOuter )
		{
			m_pOuter->EmitSound( "Halloween.dance_howl" );
			m_pOuter->EmitSound( "Halloween.dance_loop" );	
		}
	}
#endif
}

void CTFPlayerShared::OnRemoveHalloweenThriller( void )
{
#ifdef CLIENT_DLL
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		if ( pLocalPlayer == m_pOuter )
		{
			m_pOuter->StopSound( "Halloween.dance_loop" );	
		}
	}
#else
	// If this is hightower, players will be healing themselves while dancing
	StopHealing( m_pOuter );
#endif
}

void CTFPlayerShared::OnAddRadiusHealOnDamage( void )
{
	Heal_Radius( true );
}

void CTFPlayerShared::OnRemoveRadiusHealOnDamage( void )
{
	Heal_Radius( false );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMarkedForDeathSilent( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();
#endif
}
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMarkedForDeathSilent( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatedMarkedForDeathEffect();
#endif
}

void CTFPlayerShared::OnRemoveDisguisedAsDispenser( void )
{
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddRocketPack( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveRocketPack( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBurningPyro( void )
{
#ifdef GAME_DLL
	if ( m_pOuter->IsPlayerClass( TF_CLASS_PYRO) && InCond( TF_COND_BURNING ) )
	{
		RemoveCond( TF_COND_BURNING );
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ApplyRocketPackStun( float flStunDuration )
{
	if ( flStunDuration < 0.1f )
		return;

#ifdef GAME_DLL
	int nStunFlags = TF_STUN_CONTROLS;
	float flStunAmount = 0.85f;
	const int nMaxEnts = 24;
	CBaseEntity	*pObjects[ nMaxEnts ];
	int nCount = UTIL_EntitiesInSphere( pObjects, nMaxEnts, m_pOuter->GetAbsOrigin(), 192.f, FL_CLIENT );
	for ( int i = 0; i < nCount; i++ )
	{
		if ( !pObjects[i] )
			continue;

		if ( !pObjects[i]->IsAlive() )
			continue;

		if ( m_pOuter->InSameTeam( pObjects[i] ) )
			continue;

		if ( !m_pOuter->FVisible( pObjects[i], MASK_OPAQUE ) )
			continue;

		CTFPlayer *pTFPlayer = ToTFPlayer( pObjects[i] );
		if ( !pTFPlayer )
			continue;

		if ( pTFPlayer->IsMiniBoss() )
		{
			nStunFlags = TF_STUN_MOVEMENT | TF_STUN_NO_EFFECTS;
		}

		pTFPlayer->m_Shared.StunPlayer( flStunDuration, flStunAmount, nStunFlags );
	}
#endif // GAME_DLL
}


//-----------------------------------------------------------------------------
// Purpose: Central place to handle specific effects/events when a player attacks
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAttack( void )
{
	if ( InCond( TF_COND_ENERGY_BUFF ) )
	{
		float flMarkedForDeathTime = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flMarkedForDeathTime, mod_mark_attacker_for_death );
		if ( flMarkedForDeathTime > 1.f )
		{
			AddCond( TF_COND_MARKEDFORDEATH_SILENT, flMarkedForDeathTime );
		}
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetDefaultItemChargeMeters( void )
{
	for ( int i = FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; i <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++i )
	{
		float flValue = 0.f;

		CBaseEntity *pItem = m_pOuter->GetEntityForLoadoutSlot( i, true );
		if ( pItem )
		{
			flValue = pItem->GetDefaultItemChargeMeterValue();
		}

		loadout_positions_t eSlot = loadout_positions_t( i );
		SetItemChargeMeter( eSlot, flValue );
		m_flPrevItemChargeMeter[eSlot] = GetItemChargeMeter( eSlot );
	}
}
#endif // GAME_DLL

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void AddUberScreenEffect( const CTFPlayer* pPlayer )
{
	// Add the uber effect onto the local player's screen
	if ( pPlayer && pPlayer->IsLocalPlayer() )
	{
		const char *pEffectName = NULL;
		if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
		{
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED;
		}
		else
		{
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void RemoveUberScreenEffect( const CTFPlayer* pPlayer )
{
	if ( pPlayer && pPlayer->IsLocalPlayer() )
	{
		// only remove the overlay if it is an invuln material 
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial &&
			( FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE ) || 
			FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED ) ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
}

static const char* s_pszRedResistOverheadEffectName[] =
{
	"vaccinator_red_buff1",
	"vaccinator_red_buff2",
	"vaccinator_red_buff3",
};
static const char* s_pszBlueResistOverheadEffectName[] =
{
	"vaccinator_blue_buff1",
	"vaccinator_blue_buff2",
	"vaccinator_blue_buff3",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszRedResistOverheadEffectName ) == MEDIGUN_NUM_RESISTS && ARRAYSIZE( s_pszBlueResistOverheadEffectName ) == MEDIGUN_NUM_RESISTS );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void AddResistParticle( CTFPlayer* pPlayer, medigun_resist_types_t nResistType, ETFCond eYeildToCond = TF_COND_LAST )
{
	// Don't spawn it over the local player's head
	if ( !pPlayer || pPlayer->IsLocalPlayer() )
		return;

	// do not add if stealthed
	if ( pPlayer->m_Shared.IsStealthed() )
		return;

	// Don't add this effect if the yield effect is passed in
	if( eYeildToCond != TF_COND_LAST && pPlayer->m_Shared.InCond( eYeildToCond ) )
		return;

	if ( pPlayer->m_Shared.GetDisplayedTeam() == TF_TEAM_RED )
	{
		pPlayer->AddOverheadEffect( s_pszRedResistOverheadEffectName[ nResistType ] );
	}
	else
	{
		pPlayer->AddOverheadEffect( s_pszBlueResistOverheadEffectName[ nResistType ] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void RemoveResistParticle( CTFPlayer* pPlayer, medigun_resist_types_t nResistType )
{
	if ( !pPlayer || pPlayer->IsLocalPlayer() )
		return;

	bool bKeep = false;
	switch ( nResistType )
	{
		case MEDIGUN_BULLET_RESIST:
			bKeep = pPlayer->m_Shared.InCond( TF_COND_MEDIGUN_UBER_BULLET_RESIST );
			break;
		case MEDIGUN_BLAST_RESIST:
			bKeep = pPlayer->m_Shared.InCond( TF_COND_MEDIGUN_UBER_BLAST_RESIST );
			break;
		case MEDIGUN_FIRE_RESIST:
			bKeep = pPlayer->m_Shared.InCond( TF_COND_MEDIGUN_UBER_FIRE_RESIST );
			break;
		default:
			AssertMsg( 0, "Invalid medigun resist type" );
			break;
	}

	// don't remove overhead effect if the uber's still active
	if ( bKeep )
		return;
	
	pPlayer->RemoveOverheadEffect( s_pszRedResistOverheadEffectName[ nResistType ], true );
	pPlayer->RemoveOverheadEffect( s_pszBlueResistOverheadEffectName[ nResistType ], true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int GetResistShieldSkinForResistType( ETFCond eCond )
{
	switch( eCond )
	{
		case TF_COND_MEDIGUN_UBER_BULLET_RESIST:
			return 2;

		case TF_COND_MEDIGUN_UBER_BLAST_RESIST:
			return 3;

		case TF_COND_MEDIGUN_UBER_FIRE_RESIST:
			return 4;

		default:
			AssertMsg( 0, "Invalid condition passed into AddResistShield" );
			return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void AddResistShield( C_LocalTempEntity** pShield, CTFPlayer* pPlayer, ETFCond eCond  )
{
	if( CBasePlayer::GetLocalPlayer() == pPlayer )
		return;

	// do not add if stealthed
	if ( pPlayer->m_Shared.IsStealthed() )
		return;

	// Don't create a new shield if we already have one
	if( *pShield )
		return;

	model_t *pModel = (model_t*) engine->LoadModel( "models/effects/resist_shield/resist_shield.mdl" );
	(*pShield) = tempents->SpawnTempModel( pModel, pPlayer->GetAbsOrigin(), pPlayer->GetAbsAngles(), Vector(0, 0, 0), 1, FTENT_NEVERDIE | FTENT_PLYRATTACHMENT );
	if ( *pShield )
	{
		(*pShield)->ChangeTeam( pPlayer->m_Shared.GetDisplayedTeam() );
		if( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			(*pShield)->m_nSkin = GetResistShieldSkinForResistType( eCond );
		}
		else
		{
			(*pShield)->m_nSkin = ( pPlayer->m_Shared.GetDisplayedTeam() == TF_TEAM_RED ) ? 0 : 1;
		}
		(*pShield)->clientIndex = pPlayer->entindex();
		(*pShield)->SetModelScale( pPlayer->GetModelScale() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void RemoveResistShield( C_LocalTempEntity** pShield, CTFPlayer* pPlayer )
{
	if ( *pShield )
	{
		ETFCond eCond = TF_COND_INVALID;
		// Check if we still have one of the other resist types on us
		eCond = pPlayer->m_Shared.InCond( TF_COND_MEDIGUN_UBER_BULLET_RESIST ) ? TF_COND_MEDIGUN_UBER_BULLET_RESIST : eCond;
		eCond = pPlayer->m_Shared.InCond( TF_COND_MEDIGUN_UBER_BLAST_RESIST ) ? TF_COND_MEDIGUN_UBER_BLAST_RESIST : eCond;
		eCond = pPlayer->m_Shared.InCond( TF_COND_MEDIGUN_UBER_FIRE_RESIST ) ? TF_COND_MEDIGUN_UBER_FIRE_RESIST : eCond;
		eCond = ( pPlayer->m_Shared.InCond( TF_COND_RUNE_RESIST ) && !pPlayer->m_Shared.IsStealthed() ) ? TF_COND_RUNE_RESIST : eCond;

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			eCond = ( pPlayer->IsEnemyPlayer() && pPlayer->m_Shared.InCond( TF_COND_RUNE_PLAGUE ) && pLocalPlayer->m_Shared.InCond( TF_COND_PLAGUE ) ) ? TF_COND_RUNE_PLAGUE : eCond;
		}

		// Still have one, don't remove the shield
		if( eCond != TF_COND_INVALID )
		{
			// If we're in MvM, and we're one of the bots, change the shield color
			if( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				(*pShield)->m_nSkin = GetResistShieldSkinForResistType( eCond );
			}

			return;
		}
		else	// No more bubble
		{
			(*pShield)->flags = FTENT_FADEOUT | FTENT_PLYRATTACHMENT;
			(*pShield)->die = gpGlobals->curtime;
			(*pShield)->fadeSpeed = 1.0f;
			(*pShield) = NULL;
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMedEffectUberBulletResist( void )
{
#ifdef CLIENT_DLL
	AddResistParticle( m_pOuter, MEDIGUN_BULLET_RESIST );
	AddUberScreenEffect( m_pOuter );
	AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_MEDIGUN_UBER_BULLET_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMedEffectUberBulletResist( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_BULLET_RESIST );
	RemoveUberScreenEffect( m_pOuter );
	RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
	OnAddMedEffectSmallBulletResist();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMedEffectUberBlastResist( void )
{
#ifdef CLIENT_DLL
	AddResistParticle( m_pOuter, MEDIGUN_BLAST_RESIST );
	AddUberScreenEffect( m_pOuter );
	AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_MEDIGUN_UBER_BLAST_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMedEffectUberBlastResist( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_BLAST_RESIST );
	RemoveUberScreenEffect( m_pOuter );
	RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
	OnAddMedEffectSmallBlastResist();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMedEffectUberFireResist( void )
{
#ifdef CLIENT_DLL
	AddResistParticle( m_pOuter, MEDIGUN_FIRE_RESIST );
	AddUberScreenEffect( m_pOuter );
	AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_MEDIGUN_UBER_FIRE_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMedEffectUberFireResist( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_FIRE_RESIST );
	RemoveUberScreenEffect( m_pOuter );
	RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
	OnAddMedEffectSmallFireResist();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMedEffectSmallBulletResist( void )
{
#ifdef CLIENT_DLL
	if( InCond( TF_COND_MEDIGUN_SMALL_BULLET_RESIST ) )
	{
		AddResistParticle( m_pOuter, MEDIGUN_BULLET_RESIST, TF_COND_MEDIGUN_UBER_BULLET_RESIST );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMedEffectSmallBulletResist( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_BULLET_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMedEffectSmallBlastResist( void )
{
#ifdef CLIENT_DLL
	if( InCond( TF_COND_MEDIGUN_SMALL_BLAST_RESIST ) )
	{
		AddResistParticle( m_pOuter, MEDIGUN_BLAST_RESIST, TF_COND_MEDIGUN_UBER_BLAST_RESIST );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMedEffectSmallBlastResist( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_BLAST_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMedEffectSmallFireResist( void )
{
#ifdef CLIENT_DLL
	if( InCond( TF_COND_MEDIGUN_SMALL_FIRE_RESIST ) )
	{
		AddResistParticle( m_pOuter, MEDIGUN_FIRE_RESIST, TF_COND_MEDIGUN_UBER_FIRE_RESIST );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMedEffectSmallFireResist( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_FIRE_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddRuneResist( void )
{
#ifdef CLIENT_DLL
	// Do use the condition bit here, it's passed along and is expected to be a cond.
	AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_RUNE_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveRuneResist( void )
{
#ifdef CLIENT_DLL
	RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveRuneKing( void )
{
#ifdef CLIENT_DLL
	EndKingBuffRadiusEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddGrapplingHookLatched( void )
{
	m_pOuter->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GRAPPLE_PULL_START );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveGrapplingHookLatched( void )
{
	// DO NOTHING
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBulletImmune( void )
{
#ifdef CLIENT_DLL
	AddResistParticle( m_pOuter, MEDIGUN_BULLET_RESIST );
	AddUberScreenEffect( m_pOuter );
	AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_MEDIGUN_UBER_BULLET_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBulletImmune( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_BULLET_RESIST );
	RemoveUberScreenEffect( m_pOuter );
	RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBlastImmune( void )
{
#ifdef CLIENT_DLL
	AddResistParticle( m_pOuter, MEDIGUN_BLAST_RESIST );
	AddUberScreenEffect( m_pOuter );
	AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_MEDIGUN_UBER_BLAST_RESIST );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBlastImmune( void )
{
#ifdef CLIENT_DLL
	RemoveResistParticle( m_pOuter, MEDIGUN_BLAST_RESIST );
	RemoveUberScreenEffect( m_pOuter );
	RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddFireImmune( void )
{
#ifdef CLIENT_DLL
	AddUberScreenEffect( m_pOuter );
	if ( !m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
	{
		AddResistParticle( m_pOuter, MEDIGUN_FIRE_RESIST );
		AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_MEDIGUN_UBER_FIRE_RESIST );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveFireImmune( void )
{
#ifdef CLIENT_DLL
	RemoveUberScreenEffect( m_pOuter );
	if ( !m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
	{
		RemoveResistParticle( m_pOuter, MEDIGUN_FIRE_RESIST );
		RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMVMBotRadiowave( void )
{
#ifdef CLIENT_DLL
	if ( !m_pOuter->IsABot() )
		return;

	if ( !m_pOuter->m_pMVMBotRadiowave )
	{
		m_pOuter->m_pMVMBotRadiowave = m_pOuter->ParticleProp()->Create( "bot_radio_waves", PATTACH_POINT_FOLLOW, "head" );
	}
#else
	if ( !m_pOuter->IsBot() )
		return;

	StunPlayer( GetConditionDuration( TF_COND_MVM_BOT_STUN_RADIOWAVE ), 1.0, TF_STUN_BOTH | TF_STUN_NO_EFFECTS );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMVMBotRadiowave( void )
{
#ifdef CLIENT_DLL
	if ( !m_pOuter->IsABot() )
		return;

	if ( m_pOuter->m_pMVMBotRadiowave )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pMVMBotRadiowave );
		m_pOuter->m_pMVMBotRadiowave = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenSpeedBoost( void )
{
#ifdef GAME_DLL
	AddCond( TF_COND_SPEED_BOOST );
	ApplyAttributeToPlayer( "halloween reload time decreased", 0.5f );
	ApplyAttributeToPlayer( "halloween fire rate bonus", 0.5f );
	ApplyAttributeToPlayer( "halloween increased jump height", 1.5f );
	//
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenSpeedBoost( void )
{
#ifdef GAME_DLL
	RemoveCond( TF_COND_SPEED_BOOST );
	RemoveAttributeFromPlayer( "halloween reload time decreased" );
	RemoveAttributeFromPlayer( "halloween fire rate bonus" );
	RemoveAttributeFromPlayer( "halloween increased jump height" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenQuickHeal( void )
{
#ifdef GAME_DLL
	AddCond( TF_COND_MEGAHEAL );
	Heal( m_pOuter, 30.0f, 2.0f, 1.0f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenQuickHeal( void )
{
#ifdef GAME_DLL
	RemoveCond( TF_COND_MEGAHEAL );
	StopHealing( m_pOuter );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenGiant( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 2.f );
	int nNewHP = tf_halloween_giant_health_scale.GetFloat() * m_pOuter->GetPlayerClass()->GetMaxHealth();
	m_pOuter->SetHealth( nNewHP );
	m_pOuter->SetMaxHealth( nNewHP );
#else
	cam_idealdist.SetValue( 300.f );
	cam_idealdistright.SetValue( 40.f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenGiant( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 1.f );
	int nNewHP = m_pOuter->GetPlayerClass()->GetMaxHealth();
	m_pOuter->SetHealth( nNewHP );
	m_pOuter->SetMaxHealth( nNewHP );
#else
	cam_idealdist.SetValue( cam_idealdist.GetDefault() );
	cam_idealdistright.SetValue( cam_idealdistright.GetDefault() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenTiny( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 0.5f );
	ApplyAttributeToPlayer( "voice pitch scale", 1.3f );
	ApplyAttributeToPlayer( "head scale", 3.f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenGhostMode( void )
{
	m_pOuter->SetGroundEntity( NULL );
	m_pOuter->SetSolid( SOLID_NONE );
	m_pOuter->SetSolidFlags( FSOLID_NOT_SOLID );
	m_pOuter->AddFlag( FL_NOTARGET );

#ifdef GAME_DLL

	CSingleUserRecipientFilter filter( m_pOuter );
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_HOW_TO_CONTROL_GHOST );
		}
		else
		{
			TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_HOW_TO_CONTROL_GHOST_NO_RESPAWN );
		}
	}

	// The game rules listens for this event
	IGameEvent *event = gameeventmanager->CreateEvent( "player_turned_to_ghost" );

	if ( event )
	{
		event->SetInt( "userid", m_pOuter->GetUserID() );
		gameeventmanager->FireEvent( event );
	}

	// Push them up a little bit
	Vector vecNewVel( 0, 0, 40 );
	m_pOuter->Teleport( NULL, NULL, &vecNewVel );

	if ( m_pOuter->GetActiveWeapon() )
	{
		m_pOuter->GetActiveWeapon()->SendViewModelAnim( ACT_IDLE );
		m_pOuter->GetActiveWeapon()->Holster();
	}
	m_pOuter->SetActiveWeapon( NULL );	

	CBaseObject * pCarriedObj = GetCarriedObject();
	if ( pCarriedObj )
	{
		pCarriedObj->DetonateObject();
	}

	CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( m_pOuter->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
	if ( pSpellBook )
	{
		pSpellBook->ClearSpell();
	}
#else
	// Go thirdperson
	SetAppropriateCamera( m_pOuter );

	Color color;
	m_pOuter->GetTeamColor( color );
	m_pOuter->SetRenderColor( color.r(), color.g(), color.b() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenKartDash()
{
	m_pOuter->SetFOV( m_pOuter, 110.f, 1.f, 0.f );
	m_pOuter->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_KART_ACTION_DASH );

#ifdef CLIENT_DLL

#else // CLIENT_DLL
	m_pOuter->EmitSound( "BumperCar.SpeedBoostStart" );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenKartDash()
{
	m_pOuter->SetFOV( m_pOuter, 0.f, 1.f, 0.f );
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pSpeedBoostEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pSpeedBoostEffect );
		m_pOuter->m_pSpeedBoostEffect = NULL;
	}
#else // CLIENT_DLL
	m_pOuter->EmitSound( "BumperCar.SpeedBoostStop" );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenTiny( void )
{
#ifdef GAME_DLL
	m_pOuter->SetModelScale( 1.f );

	RemoveAttributeFromPlayer( "voice pitch scale" );
	RemoveAttributeFromPlayer( "head scale" );

	const Vector& vOrigin = m_pOuter->GetAbsOrigin();
	const QAngle& qAngle = m_pOuter->GetAbsAngles();
	const Vector& vHullMins = m_pOuter->GetFlags() & FL_DUCKING ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN;
	const Vector& vHullMaxs = m_pOuter->GetFlags() & FL_DUCKING ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX;

	trace_t result;
	CTraceFilterIgnoreTeammates filter( m_pOuter, COLLISION_GROUP_NONE, m_pOuter->GetTeamNumber() );
	UTIL_TraceHull( vOrigin, vOrigin, vHullMins, vHullMaxs, MASK_PLAYERSOLID, &filter, &result );
	// am I stuck? try to resolve it
	if ( result.DidHit() )
	{
		float flPlayerHeight = vHullMaxs.z - vHullMins.z;
		float flExtraHeight = 10;
		static Vector vTest[] =
		{
			Vector( 32, 32, flExtraHeight ),
			Vector( -32, -32, flExtraHeight ),
			Vector( -32, 32, flExtraHeight ),
			Vector( 32, -32, flExtraHeight ),
			Vector( 0, 0, flPlayerHeight + flExtraHeight ),
			Vector( 0, 0, -flPlayerHeight - flExtraHeight )
		};
		for ( int i=0; i<ARRAYSIZE( vTest ); ++i )
		{
			Vector vTestPos = vOrigin + vTest[i];
			UTIL_TraceHull( vOrigin, vTestPos, vHullMins, vHullMaxs, MASK_PLAYERSOLID, &filter, &result );
			if ( !result.DidHit() )
			{
				//NDebugOverlay::Box( vTestPos, vHullMins, vHullMaxs, 0, 255, 0, 0, 5.f );
				m_pOuter->Teleport( &vTestPos, &qAngle, NULL );
				return;
			}
			else
			{
				//NDebugOverlay::Box( vTestPos, vHullMins, vHullMaxs, 255, 0, 0, 0, 5.f );
			}
		}

		// just kill the player if we can't resolve getting stuck
		m_pOuter->CommitSuicide( false, true );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenGhostMode( void )
{
#ifdef CLIENT_DLL
	m_pOuter->ParticleProp()->StopEmission();
	m_pOuter->SetRenderColor( 255, 255, 255 );
	m_pOuter->UpdateWearables();
#else

	// We don't do the rest if we're a spectator
	if ( m_pOuter->GetTeamNumber() == TEAM_SPECTATOR )
		return;

	m_pOuter->RemoveFlag( FL_NOTARGET );


	// Restore solid
	m_pOuter->SetSolid( SOLID_BBOX );
	m_pOuter->SetSolidFlags( FSOLID_NOT_STANDABLE );
	m_pOuter->SetCollisionGroup( COLLISION_GROUP_PLAYER );

	// Bring their gun back
	m_pOuter->SetActiveWeapon( m_pOuter->GetLastWeapon() );
	if ( m_pOuter->GetActiveWeapon() )
	{
		m_pOuter->GetActiveWeapon()->Deploy();
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenKart( void )
{
#ifdef GAME_DLL
	CSingleUserRecipientFilter filter( m_pOuter );
	TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_HOW_TO_CONTROL_KART );

	ApplyAttributeToPlayer( "head scale", 3.f );
	
	//ResetKartDamage
	m_pOuter->ResetKartDamage();
	
	CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( m_pOuter->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
	if ( pSpellBook )
	{
		pSpellBook->ClearSpell();
	}

	m_pOuter->m_flKartNextAvailableBoost = gpGlobals->curtime + 3.0f;

	// Switch to melee to make sure Spies and Engies don't have build menus open
	CTFWeaponBase *pMeleeWeapon = dynamic_cast<CTFWeaponBase*>( m_pOuter->GetEntityForLoadoutSlot( LOADOUT_POSITION_MELEE ) );
	Assert( pMeleeWeapon );
	if ( pMeleeWeapon )
	{
		m_pOuter->Weapon_Switch( pMeleeWeapon );
	}

	m_pOuter->m_flNextBonusDucksVOAllowedTime = gpGlobals->curtime + 17.f; // The longest Merasmus line + 1 second

	// Force client to update all view angles (including kart and taunt yaw)
	m_pOuter->ForcePlayerViewAngles( m_pOuter->GetAbsAngles() );
#else
	extern ConVar tf_halloween_kart_cam_dist;
	m_pOuter->SetTauntCameraTargets( tf_halloween_kart_cam_dist.GetFloat(), 0.0f );

	m_pOuter->CreateKart();
	// Set vehicle angles to be our current angles so we don't spin around
	// when we get in the car
	//$ This is handled in the ForcePlayerViewAngles user message.
	//$ m_angVehicleMoveAngles = m_pOuter->GetAbsAngles();

	// XXX(JohnS): This is not the right place to do these things, SetWeaponVisible on the *client* is just stomping
	//             bugs elsewhere.  I'm not going to go re-fix all the spots doing this, but don't use this as a
	//             template!  Add a check in TFWeaponBase::ShouldDraw() and call TFWeapon->UpdateVisibility() on the
	//             edges.
	if ( m_pOuter->GetActiveWeapon() )
	{
		m_pOuter->GetActiveWeapon()->SetWeaponVisible( false );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenKart( void )
{
#ifdef GAME_DLL
	RemoveAttributeFromPlayer( "head scale" );
	//ResetKartDamage
	m_pOuter->ResetKartDamage();

	CTFSpellBook *pSpellBook = dynamic_cast<CTFSpellBook*>( m_pOuter->GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
	if ( pSpellBook )
	{
		pSpellBook->ClearSpell();
	}
#else
	// When we have every taunt cam use this system, we should clean up after ourselves. But for now, this causes a bad interaction
	// with other systems.
	// m_pOuter->SetTauntCameraTargets( 0.0f, 0.0f );

	m_pOuter->RemoveKart();

	// Reset any tilting now that we're out of the karts
	if ( m_pOuter->m_PlayerAnimState )
	{
		QAngle renderAngles = m_pOuter->m_PlayerAnimState->GetRenderAngles();
		renderAngles[ ROLL ] = renderAngles[ PITCH ] = 0.f;
		m_pOuter->m_PlayerAnimState->SetRenderangles( renderAngles );
	}

	// XXX(JohnS): This is not the right place to do these things, SetWeaponVisible on the *client* is just stomping
	//             bugs elsewhere.  I'm not going to go re-fix all the spots doing this, but don't use this as a
	//             template!  Add a check in TFWeaponBase::ShouldDraw() and call TFWeapon->UpdateVisibility() on the
	//             edges.
	if ( m_pOuter->GetActiveWeapon() )
	{
		m_pOuter->GetActiveWeapon()->SetWeaponVisible( true );
	}

	if ( m_hKartParachuteEntity )
	{
		m_hKartParachuteEntity->Release();
		m_hKartParachuteEntity = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBalloonHead( void )
{
#ifdef GAME_DLL
	ApplyAttributeToPlayer( "voice pitch scale", 0.85f );
	ApplyAttributeToPlayer( "head scale", 4.f );
	ApplyAttributeToPlayer( "increased jump height", 0.8f );
	ApplyAttributeToPlayer( "increased air control", 0.2f );
#endif // GAME_DLL
	m_pOuter->SetGravity( 0.3f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBalloonHead( void )
{
#ifdef GAME_DLL
	RemoveAttributeFromPlayer( "voice pitch scale" );
	RemoveAttributeFromPlayer( "head scale" );
	RemoveAttributeFromPlayer( "increased jump height" );
	RemoveAttributeFromPlayer( "increased air control" );
#endif // GAME_DLL
	m_pOuter->SetGravity( 0.f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMeleeOnly( void )
{
#ifdef GAME_DLL
	CTFWeaponBase *pMeleeWeapon = dynamic_cast<CTFWeaponBase*>( m_pOuter->GetEntityForLoadoutSlot( LOADOUT_POSITION_MELEE ) );
	Assert( pMeleeWeapon );
	if ( pMeleeWeapon )
	{
		m_pOuter->Weapon_Switch( pMeleeWeapon );
	}

	ApplyAttributeToPlayer( "disable weapon switch", true );
	ApplyAttributeToPlayer( "hand scale", 3.f );
	AddCond( TF_COND_HALLOWEEN_TINY );
	AddCond( TF_COND_SPEED_BOOST );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMeleeOnly( void )
{
#ifdef GAME_DLL
	RemoveAttributeFromPlayer( "disable weapon switch" );
	RemoveAttributeFromPlayer( "hand scale" );
	RemoveCond( TF_COND_HALLOWEEN_TINY );
	RemoveCond( TF_COND_SPEED_BOOST );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddSwimmingCurse( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_SWIMMING_CURSE, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveSwimmingCurse( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is urine
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_SWIMMING_CURSE ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#else
	AddCond( TF_COND_URINE, 10.0f );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddHalloweenKartCage( void )
{
#ifdef CLIENT_DLL
	Assert( !m_pOuter->m_hHalloweenKartCage );
	if ( !m_pOuter->m_hHalloweenKartCage )
	{
		m_pOuter->m_hHalloweenKartCage = C_PlayerAttachedModel::Create( "models/props_halloween/bumpercar_cage.mdl", m_pOuter, 0, vec3_origin, PAM_PERMANENT, 0 );
		m_pOuter->m_hHalloweenKartCage->FollowEntity( m_pOuter, true );
	}
#else
	AddCond( TF_COND_FREEZE_INPUT );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveHalloweenKartCage( void )
{
#ifdef CLIENT_DLL
	Assert( m_pOuter->m_hHalloweenKartCage );
	if ( m_pOuter->m_hHalloweenKartCage )
	{
		m_pOuter->m_hHalloweenKartCage->StopFollowingEntity();
		m_pOuter->m_hHalloweenKartCage->Release();
	}
#else
	RemoveCond( TF_COND_FREEZE_INPUT );
	DispatchParticleEffect( "ghost_appearation", PATTACH_ABSORIGIN, m_pOuter );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddPasstimeInterception( void )
{
#ifdef CLIENT_DLL
	if ( !m_pOuter->m_pPhaseStandingEffect )
	{
		m_pOuter->m_pPhaseStandingEffect = m_pOuter->ParticleProp()->Create( "warp_version", PATTACH_ABSORIGIN_FOLLOW );
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_PHASE, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#else
	if ( !m_bPhaseFXOn )
	{
		AddPhaseEffects();
	}
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_DODGING, "started_dodging:1" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemovePasstimeInterception( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pPhaseStandingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pPhaseStandingEffect );
		m_pOuter->m_pPhaseStandingEffect = NULL;
	}		
		
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();
		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_PHASE ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#else
	if ( m_bPhaseFXOn )
	{
		RemovePhaseEffects();
	}
	m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_DODGING, "started_dodging:0" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddRunePlague( void )
{
#ifdef CLIENT_DLL
	
	m_pOuter->m_pRunePlagueEffect = m_pOuter->ParticleProp()->Create( "powerup_plague_carrier", PATTACH_ABSORIGIN_FOLLOW );

	// show resist effect on enemy player that has plague rune if local player is in plague cond
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer != m_pOuter && pLocalPlayer->m_Shared.InCond( TF_COND_PLAGUE ) && m_pOuter->IsEnemyPlayer() )
	{
		AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_RUNE_PLAGUE );
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveRunePlague( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pRunePlagueEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pRunePlagueEffect );
		m_pOuter->m_pRunePlagueEffect = NULL;
	}
	RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddPlague( void )
{
#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "Powerup.PickUpPlagueInfected" );

#endif
	CTFPlayer *pProvider = ToTFPlayer( m_ConditionData[TF_COND_PLAGUE].m_pProvider );

	//plague damage is a percentage of player health so everyone has the same life expectancy
	float flPlagueDmg = 0.05f * m_pOuter->GetMaxHealth();

	if ( pProvider )
	{
		MakeBleed( pProvider, NULL, 0.f, flPlagueDmg, true );
		CSingleUserRecipientFilter localFilter( pProvider );
		pProvider->EmitSound( localFilter, pProvider->entindex(), "Powerup.PickUpPlagueInfected" );
	}

	m_pOuter->EmitSound( "Powerup.PickUpPlagueInfectedLoop" );
	ClientPrint( m_pOuter, HUD_PRINTCENTER, "#TF_Powerup_Contract_Plague" );

#ifdef CLIENT_DLL
	// show resist effect on enemy player that has plague rune if local player is in plague cond
	if ( m_pOuter->IsLocalPlayer() && pProvider && pProvider->IsEnemyPlayer() )
	{
		AddResistShield( &pProvider->m_pTempShield, pProvider, TF_COND_RUNE_PLAGUE );
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemovePlague( void )
{
	m_pOuter->StopSound( "Powerup.PickUpPlagueInfectedLoop" );
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_BLEED ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}

		// remove shield from the current plague rune carrier
		int iEnemyTeam = m_pOuter->GetTeamNumber() == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED;
		CTFPlayer *pCurrentRuneCarrier = GetRuneCarrier( RUNE_PLAGUE, iEnemyTeam );
		if ( pCurrentRuneCarrier )
		{
			RemoveResistShield( &pCurrentRuneCarrier->m_pTempShield, pCurrentRuneCarrier );	
		}
	}
#endif

//	RemoveCond( TF_COND_BLEEDING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddInPurgatory( void )
{
#ifdef GAME_DLL
	// just entered
	m_pOuter->m_purgatoryPainMultiplierTimer.Start( 40.0f );
	m_pOuter->m_purgatoryPainMultiplier = 1;

	// Set our health to full 
	m_pOuter->SetHealth( m_pOuter->GetMaxHealth() );

	// Remove our projectiles
	m_pOuter->RemoveOwnedProjectiles();

	// Give us a brief period of invuln while we drop into purgatory
	AddCond( TF_COND_INVULNERABLE, 1.5f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInPurgatory( void )
{
#ifdef GAME_DLL
	if ( m_pOuter->IsAlive() )
	{
		// we escaped purgatory alive!
		const float buffDuration = 10.0f;
		AddCond( TF_COND_CRITBOOSTED_PUMPKIN, buffDuration );
		AddCond( TF_COND_SPEED_BOOST, buffDuration );
		AddCond( TF_COND_INVULNERABLE, buffDuration );

		m_pOuter->SetHealth( 2.0f * m_pOuter->GetMaxHealth() );

		m_pOuter->m_purgatoryBuffTimer.Start( buffDuration );

		TFGameRules()->BroadcastSound( 255, "Halloween.PlayerEscapedUnderworld" );

		// Remove our projectiles
		m_pOuter->RemoveOwnedProjectiles();

		CReliableBroadcastRecipientFilter filter;
		const char* pszEscapeMessage = "#TF_Halloween_Underworld";
		if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) )
		{
			pszEscapeMessage = "#TF_Halloween_Skull_Island_Escape";
		}

		UTIL_SayText2Filter( filter, m_pOuter, false, pszEscapeMessage, m_pOuter->GetPlayerName() );

		IGameEvent *pEvent = gameeventmanager->CreateEvent( "escaped_loot_island" );
		if ( pEvent )
		{
			pEvent->SetInt( "player", m_pOuter->GetUserID() );
			gameeventmanager->FireEvent( pEvent, true );
		}

		if ( m_pOuter->GetTeam() )
		{
			const char* pszLogMessage = "purgatory_escaped";
			if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) )
			{
				pszEscapeMessage = "skull_island_escaped";
			}

			UTIL_LogPrintf( "HALLOWEEN: \"%s<%i><%s><%s>\" %s\n",
							m_pOuter->GetPlayerName(),
							m_pOuter->GetUserID(),
							m_pOuter->GetNetworkIDString(),
							m_pOuter->GetTeam()->GetName(),
							pszLogMessage );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddCompetitiveWinner( void )
{
#ifdef GAME_DLL

#else
	if ( m_pOuter->IsLocalPlayer() )
	{
		gHUD.LockRenderGroup( gHUD.LookupRenderGroupIndexByName( "mid" ) );
		m_pOuter->UpdateVisibility();
		m_pOuter->UpdateWearables();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveCompetitiveWinner( void )
{
#ifdef GAME_DLL


#else


#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddCompetitiveLoser( void )
{
#ifdef GAME_DLL


#else
	if ( m_pOuter->IsLocalPlayer() )
	{
		gHUD.LockRenderGroup( gHUD.LookupRenderGroupIndexByName( "mid" ) );
		m_pOuter->UpdateVisibility();
		m_pOuter->UpdateWearables();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveCompetitiveLoser( void )
{
#ifdef GAME_DLL


#else


#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateChargeMeter( void )
{
	if ( !m_pOuter->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		return;

	if ( InCond( TF_COND_SHIELD_CHARGE ) )
	{
		// Drain the meter while we are charging.
		float flChargeDrainTime = tf_demoman_charge_drain_time.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flChargeDrainTime, mod_charge_time );
		float flChargeDrainMod = 100.f / flChargeDrainTime;
		m_flChargeMeter -= gpGlobals->frametime * flChargeDrainMod;
		if ( m_flChargeMeter <= 0 )
		{
			m_flChargeMeter = 0;
			RemoveCond( TF_COND_SHIELD_CHARGE );
		}

		m_flLastNoChargeTime = gpGlobals->curtime;
	}
	else if ( m_flChargeMeter < 100.f )
	{
		// Recharge the meter while we are not charging.
		float flChargeRegenMod = tf_demoman_charge_regen_rate.GetFloat();
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flChargeRegenMod, charge_recharge_rate );
		if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
		{
			if ( GetCarryingRuneType() != RUNE_NONE )
			{
				flChargeRegenMod *= 0.2f;
			}
			else
				flChargeRegenMod *= 0.4f;
		}
		
		flChargeRegenMod = Max( flChargeRegenMod, 1.f );
		m_flChargeMeter += gpGlobals->frametime * flChargeRegenMod;
		if ( m_flChargeMeter > 100.f )
		{
			m_flChargeMeter = 100.f;
		}

		// Used for the weapon glow cooldown.
		if ( !m_bChargeGlowing )
		{
			m_flLastNoChargeTime = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::EndCharge()
{
	if ( !InCond( TF_COND_SHIELD_CHARGE ) )
		return;

#ifdef GAME_DLL
	if ( GetDemomanChargeMeter() < 90 )
	{
		// Impacts drain the charge meter completely.
		float flMeterAtImpact = m_flChargeMeter;

		CTFWearableDemoShield *pWearableShield = GetEquippedDemoShield( m_pOuter );
		if ( pWearableShield )
		{
			pWearableShield->ShieldBash( m_pOuter, flMeterAtImpact );
		}

		CalcChargeCrit();

		// Removing the condition here would cause issues with prediction, so we set the
		// duration to zero so that it will be removed during the next condition think.
		SetConditionDuration( TF_COND_SHIELD_CHARGE, 0 );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::CalculateChargeCap( void ) const
{
	float flCap = 0.45f;

	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flCap, charge_turn_control );

	// Scale yaw cap based on frametime to prevent differences in turn effectiveness due to variable framerate (between clients mainly)
	if ( tf_demoman_charge_frametime_scaling.GetBool() )
	{
		// There's probably something better to use here as a baseline, instead of TICK_INTERVAL
		float flMod = RemapValClamped( gpGlobals->frametime, ( TICK_INTERVAL * YAW_CAP_SCALE_MIN ), ( TICK_INTERVAL * YAW_CAP_SCALE_MAX ), 0.25f, 2.f );
		flCap *= flMod;
	}

	return flCap;
}

bool CTFPlayerShared::HasDemoShieldEquipped() const
{
	return GetEquippedDemoShield( m_pOuter ) != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::CalcChargeCrit( bool bForceCrit )
{
#ifdef GAME_DLL
	// Keying on TideTurner
	int iDemoChargeDamagePenalty = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iDemoChargeDamagePenalty, lose_demo_charge_on_damage_when_charging );
	if ( iDemoChargeDamagePenalty && GetDemomanChargeMeter() <= 75 )
	{
		SetNextMeleeCrit( MELEE_MINICRIT );
	}
	else if ( GetDemomanChargeMeter() <= 40 || bForceCrit)
	{
		SetNextMeleeCrit( MELEE_CRIT );
	}
	else if ( GetDemomanChargeMeter() <= 75 )
	{
		SetNextMeleeCrit( MELEE_MINICRIT );
	}
	
	m_pOuter->SetContextThink( &CTFPlayer::RemoveMeleeCrit, gpGlobals->curtime + 0.3f, "RemoveMeleeCrit" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddShieldCharge( void )
{
	UpdatePhaseEffects();

	m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "DemoCharge.Charging" );
#else
	m_hPlayersVisibleAtChargeStart.Purge();

	// Remove debuffs
	for ( int i = 0; g_aDebuffConditions[i] != TF_COND_LAST; i++ )
	{
		RemoveCond( g_aDebuffConditions[i] );
	}

	// store the players we CAN see for the TF_DEMOMAN_KILL_PLAYER_YOU_DIDNT_SEE achievement
	CUtlVector<CTFPlayer *> vecPlayers;
	CollectPlayers( &vecPlayers, ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED, true );
	FOR_EACH_VEC( vecPlayers, i )
	{
		if ( !vecPlayers[i] )
			continue;

		if ( vecPlayers[i]->m_Shared.InCond( TF_COND_STEALTHED ) )
			continue;

		// can we see them?
		if ( m_pOuter->FVisible( vecPlayers[i], MASK_OPAQUE ) == false )
			continue;

		// are they in our field of view? (might be behind us)
		if ( m_pOuter->IsInFieldOfView( vecPlayers[i]) == false )
			continue;

		m_hPlayersVisibleAtChargeStart.AddToTail( vecPlayers[i] );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveShieldCharge( void )
{
	RemovePhaseEffects();

	m_pOuter->TeamFortress_SetSpeed();

	m_bPostShieldCharge = true;
	m_flChargeEndTime = gpGlobals->curtime;
	m_flChargeMeter = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::InterruptCharge( void )
{
	if ( !InCond( TF_COND_SHIELD_CHARGE ) )
		return;

	SetConditionDuration( TF_COND_SHIELD_CHARGE, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
void CTFPlayer::RemoveMeleeCrit( void )
{
	m_Shared.SetNextMeleeCrit( MELEE_NOCRIT );
	m_Shared.m_bPostShieldCharge = false;
	// Remove crit boost right away.  DemoShieldChargeThink depends on m_bPostShieldCharge being true
	// to attempt to remove crits (which we just cleared) so clear crits here as well.
	if ( m_Shared.InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
	{
		m_Shared.RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::DemoShieldChargeThink( void )
{
	if ( InCond( TF_COND_SHIELD_CHARGE ) || m_bPostShieldCharge )
	{
		if ( m_bPostShieldCharge && (gpGlobals->curtime - m_flChargeEndTime) >= 0.3f )
		{
			if ( InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
			{
				RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
			}
			m_bPostShieldCharge = false;
		}
		else if ( InCond( TF_COND_SHIELD_CHARGE ) && GetDemomanChargeMeter() < 75 )
		{
			if ( !InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
			{
				AddCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDemoBuff( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateDemomanEyeEffect( m_iDecapitations );
#else
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddEnergyDrinkBuff( void )
{
#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif

#ifdef GAME_DLL
	if ( m_pOuter->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) || m_pOuter->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		// Begin berzerker speed buff.
		m_pOuter->TeamFortress_SetSpeed();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveEnergyDrinkBuff( void )
{
#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif

#ifdef GAME_DLL
	if ( m_pOuter->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) || m_pOuter->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		// End berzerker speed buff.
		m_pOuter->TeamFortress_SetSpeed();
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::EndRadiusHealEffect( void )
{
	if ( m_pOuter->m_pRadiusHealEffect )
	{
		m_pOuter->m_pRadiusHealEffect->StopEmission();
		m_pOuter->m_pRadiusHealEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::EndKingBuffRadiusEffect( void )
{
	// For buffed player
	if ( m_pOuter->m_pKingBuffRadiusEffect )
	{
		m_pOuter->m_pKingBuffRadiusEffect->StopEmission();
		m_pOuter->m_pKingBuffRadiusEffect = NULL;
	}
	// For carrier of King Rune
	if ( m_pOuter->m_pKingRuneRadiusEffect )
	{
		m_pOuter->m_pKingRuneRadiusEffect->StopEmission();
		m_pOuter->m_pKingRuneRadiusEffect = NULL;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddRadiusHeal( void )
{
#ifdef CLIENT_DLL
	if ( InCond( TF_COND_RADIUSHEAL ) )
	{
		if ( IsStealthed() )
		{
			EndRadiusHealEffect();
			return;
		}

		const char *pszRadiusHealEffect;
		int nTeamNumber = m_pOuter->GetTeamNumber();
		if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) && InCond( TF_COND_DISGUISED ) && ( GetDisguiseTeam() == GetLocalPlayerTeam() ) )
		{
			nTeamNumber = GetLocalPlayerTeam();
		}

		if ( nTeamNumber == TF_TEAM_RED )
		{
			pszRadiusHealEffect = "medic_healradius_red_buffed";
		}
		else
		{
			pszRadiusHealEffect = "medic_healradius_blue_buffed";
		}

		if ( !m_pOuter->m_pRadiusHealEffect )
		{
			m_pOuter->m_pRadiusHealEffect = m_pOuter->ParticleProp()->Create( pszRadiusHealEffect, PATTACH_ABSORIGIN_FOLLOW );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveRadiusHeal( void )
{
#ifdef CLIENT_DLL
	EndRadiusHealEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddMegaHeal( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		const char *pEffectName = NULL;
		if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED )
		{
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED;
		}
		else
		{
			pEffectName = TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}

	if ( InCond( TF_COND_MEGAHEAL ) )
	{
		const char *pszMegaHealEffectName;
		if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED )
		{
			pszMegaHealEffectName = "medic_megaheal_red";
		}
		else
		{
			pszMegaHealEffectName = "medic_megaheal_blue";
		}

		if ( !m_pOuter->m_pMegaHealEffect )
		{
			m_pOuter->m_pMegaHealEffect = m_pOuter->ParticleProp()->Create( pszMegaHealEffectName, PATTACH_ABSORIGIN_FOLLOW );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveMegaHeal( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pMegaHealEffect )
	{
		m_pOuter->m_pMegaHealEffect->StopEmission();
		m_pOuter->m_pMegaHealEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is an invuln material 
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial &&
			( FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_BLUE ) || 
			FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_INVULN_RED ) ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddKingBuff( void )
{
#ifdef CLIENT_DLL
	if ( IsStealthed() )
	{
		EndKingBuffRadiusEffect();
		return;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveKingBuff( void )
{
#ifdef CLIENT_DLL
	EndKingBuffRadiusEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveRuneSupernova( void )
{
	SetRuneCharge( 0.f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDemoBuff( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateDemomanEyeEffect( 0 );
#else
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CTFPlayerShared::ClientDemoBuffThink( void )
{
	if ( m_iDecapitations > 0 )
	{
		if ( m_iDecapitations != m_iOldDecapitations )
		{
			m_iOldDecapitations = m_iDecapitations;
			m_pOuter->UpdateDemomanEyeEffect( m_iDecapitations );
		}
	}
}

//-----------------------------------------------------------------------------
void CTFPlayerShared::ClientKillStreakBuffThink( void )
{
	int nLoadoutSlot = m_pOuter->GetActiveTFWeapon() ? m_pOuter->GetActiveTFWeapon()->GetAttributeContainer()->GetItem()->GetStaticData()->GetLoadoutSlot( m_pOuter->GetPlayerClass()->GetClassIndex() ) : LOADOUT_POSITION_PRIMARY;
	int nKillStreak = GetStreak(kTFStreak_Kills);
	if ( nKillStreak != m_iOldKillStreak || m_iOldKillStreakWepSlot != nLoadoutSlot )
	{
		m_pOuter->UpdateKillStreakEffects( nKillStreak, m_iOldKillStreak < nKillStreak );
		m_iOldKillStreak = nKillStreak;
		m_iOldKillStreakWepSlot = nLoadoutSlot;
	}
	else if ( !m_pOuter->IsAlive() )
	{
		m_pOuter->UpdateKillStreakEffects( 0, false );
	}
	else
	{
		static bool bAlternate = false;
		Vector vColor = bAlternate ? m_pOuter->m_vEyeGlowColor1 : m_pOuter->m_vEyeGlowColor2;

		if ( m_pOuter->m_pEyeGlowEffect[0] )
		{
			m_pOuter->m_pEyeGlowEffect[0]->SetControlPoint( CUSTOM_COLOR_CP1, vColor );
		}
		if ( m_pOuter->m_pEyeGlowEffect[1] )
		{
			m_pOuter->m_pEyeGlowEffect[1]->SetControlPoint( CUSTOM_COLOR_CP1, vColor );
		}
		//
		bAlternate = !bAlternate;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
	m_flGotTeleEffectAt = gpGlobals->curtime;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::ShouldShowRecentlyTeleported( void )
{
	if ( IsStealthed() )
	{
		return false;
	}

	if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
	{
		// disguised as an enemy
		if ( InCond( TF_COND_DISGUISED ) && GetDisguiseTeam() != m_pOuter->GetTeamNumber() )
		{
			// was this my own team's teleporter?
			if ( GetTeamTeleporterUsed() == m_pOuter->GetTeamNumber() )
			{
				// don't show my trail
				return false;
			}
			else
			{
				// okay to show the local player the trail, but not his team (might confuse them)
				if ( !m_pOuter->IsLocalPlayer() && m_pOuter->GetTeamNumber() == GetLocalPlayerTeam() )
				{
					return false;
				}
			}
		}
		else
		{
			if ( GetTeamTeleporterUsed() != m_pOuter->GetTeamNumber() )
			{
				return false;
			}
		}
	}

	return InCond( TF_COND_TELEPORTED );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::Burn( CTFPlayer *pAttacker, CTFWeaponBase *pWeapon, float flBurningTime /*=-1*/ )
{
#ifdef GAME_DLL
	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	//Don't ignite if I'm in phase mode.
	if ( InCond( TF_COND_PHASE ) || InCond( TF_COND_PASSTIME_INTERCEPTION ) )
		return;

	// pyros don't burn persistently or take persistent burning damage, but we show brief burn effect so attacker can tell they hit
	bool bVictimIsImmunePyro = ( TF_CLASS_PYRO ==  m_pOuter->GetPlayerClass()->GetClassIndex() );
	if ( bVictimIsImmunePyro )
	{
		bVictimIsImmunePyro = !InCond( TF_COND_BURNING_PYRO );
	}

#ifdef DEBUG
	static float s_flStartAfterburnTime = 0.f;
	static float s_flReachMaxAfterburnTime = 0.f;
#endif // DEBUG

	if ( !InCond( TF_COND_BURNING ) )
	{
		// Start burning
		AddCond( TF_COND_BURNING, -1.f, pAttacker );
		m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
		m_flAfterburnDuration = pWeapon ? pWeapon->GetInitialAfterburnDuration() : 0.f;
		
		// Reduces direct healing effectiveness
		AddCond( TF_COND_HEALING_DEBUFF, m_flAfterburnDuration, pAttacker );

		// let the attacker know he burned me
		if ( pAttacker && !bVictimIsImmunePyro )
		{
			pAttacker->OnBurnOther( m_pOuter, pWeapon );

			m_hOriginalBurnAttacker = pAttacker;
		}

#ifdef DEBUG
		s_flStartAfterburnTime = gpGlobals->curtime;
		s_flReachMaxAfterburnTime = 0.f;
#endif // DEBUG
	}

	int bAfterburnImmunity = bVictimIsImmunePyro;

	// Check my weapon
	if ( !bAfterburnImmunity )
	{
		int nAfterburnImmunity = 0;
		CTFWeaponBase *pMyWeapon = GetActiveTFWeapon();
		if ( pMyWeapon )
		{
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pMyWeapon, nAfterburnImmunity, afterburn_immunity );
		}

		bAfterburnImmunity |= nAfterburnImmunity != 0;
	}

	// STAGING_SPY
	if ( InCond( TF_COND_AFTERBURN_IMMUNE ) )
	{
		bAfterburnImmunity = true;
	}

	// Check demo shield
	if ( !bAfterburnImmunity && IsShieldEquipped() )
	{
		int nAfterburnImmunity = 0;
		CTFWearableDemoShield *pWearableShield = GetEquippedDemoShield( m_pOuter );
		if ( pWearableShield && !pWearableShield->IsDisguiseWearable() )
		{
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWearableShield, nAfterburnImmunity, afterburn_immunity );
		}

		bAfterburnImmunity |= nAfterburnImmunity != 0;
	}
	
	// Check sniper shields (e.g. Darwin's)
	if ( !bAfterburnImmunity && m_pOuter->IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		for ( int i = 0; i < m_pOuter->GetNumWearables(); ++i )
		{
			CTFWearable *pWearableItem = dynamic_cast< CTFWearable* >( m_pOuter->GetWearable( i ) );
			if ( !pWearableItem )
				continue;

			int nAfterburnImmunity = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWearableItem, nAfterburnImmunity, afterburn_immunity );
			if ( nAfterburnImmunity )
			{
				bAfterburnImmunity = true;
				break;
			}
		}
	}

	// check afterburn duration
	float flFlameLife = pWeapon ? pWeapon->GetAfterburnRateOnHit() : 0.f;
	if ( bAfterburnImmunity )
	{
		flFlameLife = TF_BURNING_FLAME_LIFE_PYRO;
	}
	else if ( flBurningTime > 0 )
	{
		flFlameLife = flBurningTime;
	}
	
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flFlameLife, mult_wpn_burntime );

	// flame immunity will always have a fixed duration
	if ( bAfterburnImmunity )
	{
		m_flAfterburnDuration = flFlameLife;
	}
	// otherwise stack the duration
	else
	{
		m_flAfterburnDuration += flFlameLife;
	}

	m_flAfterburnDuration = Clamp( m_flAfterburnDuration, 0.f, tf_afterburn_max_duration );

#ifdef DEBUG
	if ( tf_afterburn_debug.GetBool() )
	{
		engine->Con_NPrintf( 1, "Added afterburn duration = %f", m_flAfterburnDuration );

		if ( s_flReachMaxAfterburnTime == 0.f && m_flAfterburnDuration == tf_afterburn_max_duration )
		{
			s_flReachMaxAfterburnTime = gpGlobals->curtime;
			DevMsg( "took %f seconds to reach max afterburn duration\n", s_flReachMaxAfterburnTime - s_flStartAfterburnTime );
		}
	}
#endif // DEBUG

	m_hBurnAttacker = pAttacker;
	m_hBurnWeapon = pWeapon;

#endif // GAME_DLL
}


//-----------------------------------------------------------------------------
// Purpose: A non-TF Player burned us
// Used for Bosses, they inflict self burn
//-----------------------------------------------------------------------------
void CTFPlayerShared::SelfBurn( float flBurningTime )
{
#ifdef GAME_DLL
	Burn( m_pOuter, NULL, flBurningTime );
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::MakeBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, float flBleedingTime, int nBleedDmg /* = TF_BLEEDING_DMG */, bool bPermanentBleeding /*= false*/, int nDmgType /*= TF_DMG_CUSTOM_BLEEDING*/ )
{
#ifdef CLIENT_DLL

#else
	// Don't bother if they are dead
	if ( !m_pOuter->IsAlive() )
		return;

	// Required for the CTakeDamageInfo we create later
	Assert( pPlayer && pWeapon );
	if ( !pPlayer && !pWeapon )
		return;

	float flExpireTime = gpGlobals->curtime + flBleedingTime;

	// See if this weapon has already applied a bleed and extend the time
	FOR_EACH_VEC( m_PlayerBleeds, i )
	{
		if ( m_PlayerBleeds[i].hBleedingAttacker && m_PlayerBleeds[i].hBleedingAttacker == pPlayer &&
			 m_PlayerBleeds[i].hBleedingWeapon && m_PlayerBleeds[i].hBleedingWeapon == pWeapon )
		{
			if ( flExpireTime > m_PlayerBleeds[i].flBleedingRemoveTime )
			{
				m_PlayerBleeds[i].flBleedingRemoveTime = flExpireTime;
				return;
			}
		}
	}

	// New bleed source
	bleed_struct_t bleedinfo =
	{
		pPlayer,			// hBleedingAttacker
		pWeapon,			// hBleedingWeapon
		flBleedingTime,		// flBleedingTime
		flExpireTime,		// flBleedingRemoveTime
		nBleedDmg,			// nBleedDmg
		bPermanentBleeding,
		nDmgType			// DmgType
	};
	m_PlayerBleeds.AddToTail( bleedinfo );

	if ( !InCond( TF_COND_BLEEDING ) )
	{
		AddCond( TF_COND_BLEEDING, -1.f, pPlayer );
	}
#endif
}


#ifdef GAME_DLL
void CTFPlayerShared::StopBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon )
{
	FOR_EACH_VEC_BACK( m_PlayerBleeds, i )
	{
		const bleed_struct_t& bleed = m_PlayerBleeds[i];
		if ( bleed.hBleedingAttacker == pPlayer && bleed.hBleedingWeapon == pWeapon )
		{
			m_PlayerBleeds.FastRemove( i );
		}
	}

	// remove condition right away when the list is empty
	if ( !m_PlayerBleeds.Count() )
	{
		RemoveCond( TF_COND_BLEEDING );
	}
}
#endif // GAME_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBurning( void )
{
#ifdef CLIENT_DLL
	m_pOuter->StopBurningSound();

	if ( m_pOuter->m_nOldWaterLevel > 0 )
	{	
		m_pOuter->ParticleProp()->Create( "water_burning_steam", PATTACH_ABSORIGIN );
	}

	if ( m_pOuter->m_pBurningEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pBurningEffect );
		m_pOuter->m_pBurningEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is the burning 
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();

		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_BURNING ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}

	m_pOuter->m_flBurnEffectStartTime = 0;
#else
	m_hBurnAttacker = NULL;
	m_hOriginalBurnAttacker = NULL;
	m_hBurnWeapon = NULL;

	m_pOuter->ClearBurnFromBehindAttackers();

	// If we were on fire and now we're not, and we're still alive, then give ourself some credit
	// for surviving this fire if we have any items that track it.
	if ( m_nPlayerState == TF_STATE_ACTIVE )
	{
		HatAndMiscEconEntities_OnOwnerKillEaterEventNoParter( m_pOuter, kKillEaterEvent_FiresSurvived );
	}

	if ( InCond( TF_COND_HEALING_DEBUFF ) )
	{
		RemoveCond( TF_COND_HEALING_DEBUFF );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveOverhealed( void )
{
#ifdef CLIENT_DLL
	if ( !m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->UpdateOverhealEffect();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDemoCharge( void )
{
#ifdef CLIENT_DLL
	m_pOuter->StopSound( "DemoCharge.ChargeCritOn" );
	m_pOuter->EmitSound( "DemoCharge.ChargeCritOff" );
	UpdateCritBoostEffect();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveCritBoost( void )
{
#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTmpDamageBonus( void )
{
	m_flTmpDamageBonusAmount = 1.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStealthed( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->GetPredictable() && ( !prediction->IsFirstTimePredicted() || m_bSyncingConditions ) )
		return;

	if ( !InCond( TF_COND_FEIGN_DEATH ) )
	{
		m_pOuter->EmitSound( "Player.Spy_Cloak" );
	}
	m_pOuter->RemoveAllDecals();
	UpdateCritBoostEffect();

	if ( m_pOuter->m_pTempShield && GetCarryingRuneType() == RUNE_RESIST )
	{
		RemoveResistShield( &m_pOuter->m_pTempShield, m_pOuter );
	}
#endif

	bool bSetInvisChangeTime = true;
#ifdef CLIENT_DLL
	if ( !m_pOuter->IsLocalPlayer() )
	{
		// We only clientside predict changetime for the local player
		bSetInvisChangeTime = false;
	}

	if ( InCond( TF_COND_STEALTHED_USER_BUFF ) && m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_STEALTH, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif

	if ( bSetInvisChangeTime )
	{
		if ( !InCond( TF_COND_FEIGN_DEATH ) && !InCond( TF_COND_STEALTHED_USER_BUFF ) )
		{
			float flInvisTime = tf_spy_invis_time.GetFloat();
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flInvisTime, mult_cloak_rate );
			m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisTime;
		}
		else
		{
			m_flInvisChangeCompleteTime = gpGlobals->curtime; // Stealth immediately if we are in feign death.
		}
	}

	// set our offhand weapon to be the invis weapon, but only for the spy's stealth
	if ( InCond( TF_COND_STEALTHED ) )
	{
		for (int i = 0; i < m_pOuter->WeaponCount(); i++) 
		{
			CTFWeaponInvis *pWpn = (CTFWeaponInvis *) m_pOuter->GetWeapon(i);
			if ( !pWpn )
				continue;

			if ( pWpn->GetWeaponID() != TF_WEAPON_INVIS )
				continue;

			// try to switch to this weapon
			m_pOuter->SetOffHandWeapon( pWpn );

			m_bMotionCloak = pWpn->HasMotionCloak();
			break;
		}
	}

	m_pOuter->TeamFortress_SetSpeed();

#ifdef CLIENT_DLL
	// Remove water balloon effect if it on player
	m_pOuter->ParticleProp()->StopParticlesNamed( "balloontoss_drip", true );

	m_pOuter->UpdateSpyStateChange();
	m_pOuter->UpdateKillStreakEffects( GetStreak( kTFStreak_Kills ) );
#endif

#ifdef GAME_DLL
	m_flCloakStartTime = gpGlobals->curtime;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStealthed( void )
{
#ifdef CLIENT_DLL
	if ( !m_bSyncingConditions )
		return;

	CTFWeaponInvis *pWpn = (CTFWeaponInvis *) m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS );

	int iReducedCloak = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iReducedCloak, set_quiet_unstealth );
	if ( iReducedCloak == 1 )
	{
		m_pOuter->EmitSound( "Player.Spy_UnCloakReduced" );
	}
	else if ( pWpn && pWpn->HasFeignDeath() )
	{
		m_pOuter->EmitSound( "Player.Spy_UnCloakFeignDeath" );
	}
	else
	{
		m_pOuter->EmitSound( "Player.Spy_UnCloak" );
	}
	UpdateCritBoostEffect( kCritBoost_ForceRefresh );

	if ( m_pOuter->IsLocalPlayer() && !InCond( TF_COND_STEALTHED_USER_BUFF_FADING ) )
	{
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();
		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_STEALTH ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}

	if ( !m_pOuter->m_pTempShield && GetCarryingRuneType() == RUNE_RESIST )
	{
		AddResistShield( &m_pOuter->m_pTempShield, m_pOuter, TF_COND_RUNE_RESIST );
	}
#else
	if ( m_flCloakStartTime > 0 )
	{
		// Calc a time and report every minute
		float flCloaktime = ( gpGlobals->curtime - m_flCloakStartTime );
		if ( flCloaktime > 0 )
		{
			EconEntity_OnOwnerKillEaterEventNoPartner( 
				dynamic_cast<CEconEntity *>( m_pOuter->GetEntityForLoadoutSlot( LOADOUT_POSITION_PDA2 ) ),
				m_pOuter,
				kKillEaterEvent_TimeCloaked,
				(int)flCloaktime
			);
		}
		m_flCloakStartTime = 0;
	}

#endif

	// End feign death if we leave stealth for some reason.
	if ( InCond( TF_COND_FEIGN_DEATH ) )
	{
		RemoveCond( TF_COND_FEIGN_DEATH );
	}

	m_pOuter->HolsterOffHandWeapon();

	m_pOuter->TeamFortress_SetSpeed();

	m_bMotionCloak = false;

#ifdef CLIENT_DLL
	m_pOuter->UpdateSpyStateChange();
	m_pOuter->UpdateKillStreakEffects( GetStreak( kTFStreak_Kills ) );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStealthedUserBuffFade( void )
{
#ifdef CLIENT_DLL
	// If a player is firing their weapon while radius stealth hits them, we never 
	// get a chance to apply the screenoverlay effect, so apply it here instead.
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_STEALTH, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStealthedUserBuffFade( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();
		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_STEALTH ) )
		{
			view->SetScreenOverlayMaterial( NULL );
			return;
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddFeignDeath( void )
{
#ifdef CLIENT_DLL
	// STAGING_SPY
	AddUberScreenEffect( m_pOuter );
#else
#endif
	// Go stealth w/o sound or fade out.
	if ( !IsStealthed() )
	{
		AddCond( TF_COND_STEALTHED, -1.f, m_pOuter );
	}

	// STAGING_SPY
	// Add a speed boost while feigned and afterburn immunity while running away
	AddCond( TF_COND_SPEED_BOOST, tf_feign_death_speed_duration.GetFloat() );
	AddCond( TF_COND_AFTERBURN_IMMUNE, tf_feign_death_speed_duration.GetFloat() );

	SetFeignDeathReady( false );

	m_flFeignDeathEnd = gpGlobals->curtime + tf_feign_death_speed_duration.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveFeignDeath( void )
{
#ifdef CLIENT_DLL
	// STAGING_SPY
	RemoveUberScreenEffect( m_pOuter );
#endif
	// Previous code removed cloak meter, this has been moved to on RemoveStealth checking for steath type
	// FeignDeath is the duration of cloak where speed, no shimmer and damage reduction take place
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
#else
	m_nDesiredDisguiseTeam = TF_SPY_UNDEFINED;

	// Do not reset this value, we use the last desired disguise class for the
	// 'lastdisguise' command

	//m_nDesiredDisguiseClass = TF_CLASS_UNDEFINED;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguised( void )
{
#ifdef CLIENT_DLL

	if ( m_pOuter->GetPredictable() && ( !prediction->IsFirstTimePredicted() || m_bSyncingConditions ) )
		return;

	// if local player is on the other team, reset the model of this player
	CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !m_pOuter->InSameTeam( pLocalPlayer ) )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( TF_CLASS_SPY );
		int iIndex = modelinfo->GetModelIndex( pData->GetModelName() );

		m_pOuter->SetModelIndex( iIndex );
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

	// They may have called for medic and created a visible medic bubble
	m_pOuter->StopSaveMeEffect( true );
	m_pOuter->StopTauntWithMeEffect();

	UpdateCritBoostEffect( kCritBoost_ForceRefresh );
	m_pOuter->UpdateSpyStateChange();

#else
	m_nDisguiseTeam  = TF_SPY_UNDEFINED;
	m_nDisguiseClass.Set( TF_CLASS_UNDEFINED );
	m_nDisguiseSkinOverride = 0;
	m_hDisguiseTarget.Set( NULL );
	m_iDisguiseHealth = 0;
	SetDisguiseBody( 0 );
	m_iDisguiseAmmo = 0;

	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->ClearExpression();

	m_pOuter->ClearDisguiseWeaponList();

	RemoveDisguiseWeapon();

	RemoveDisguiseWearables();

#endif

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBurning( void )
{
#ifdef CLIENT_DLL
	// Start the burning effect
	if ( !m_pOuter->m_pBurningEffect )
	{
		if ( !( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) && m_pOuter->IsLocalPlayer() ) )
		{
			const char *pEffectName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "burningplayer_red" : "burningplayer_blue";
			m_pOuter->m_pBurningEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
		}

		m_pOuter->m_flBurnEffectStartTime = gpGlobals->curtime;
	}
	// set the burning screen overlay
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_BURNING, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif

	// play a fire-starting sound
	m_pOuter->EmitSound( "Fire.Engulf" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddOverhealed( void )
{
#ifdef CLIENT_DLL
	// Start the Overheal effect
	 
	if ( !m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->UpdateOverhealEffect();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStunned( void )
{
	if ( IsControlStunned() || IsLoserStateStunned() )
	{
#ifdef CLIENT_DLL
		if ( GetActiveStunInfo() )
		{
			if ( !m_pOuter->m_pStunnedEffect && !( GetActiveStunInfo()->iStunFlags & TF_STUN_NO_EFFECTS ) )
			{
				if ( ( GetActiveStunInfo()->iStunFlags & TF_STUN_BY_TRIGGER ) || InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
				{
					const char* pEffectName = "yikes_fx";
					m_pOuter->m_pStunnedEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_POINT_FOLLOW, "head" );
				}
				else
				{
					const char* pEffectName = "conc_stars";
					m_pOuter->m_pStunnedEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_POINT_FOLLOW, "head" );
				}
			}
		}
#endif

		// Notify our weapon that we have been stunned.
		CTFWeaponBase* pWpn = m_pOuter->GetActiveTFWeapon();
		if ( pWpn )
		{
			pWpn->OnControlStunned();
		}

		if ( InCond( TF_COND_SHIELD_CHARGE ) )
		{
			SetDemomanChargeMeter( 0 );
			RemoveCond( TF_COND_SHIELD_CHARGE );
		}

		m_pOuter->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStunned( void )
{
	m_iStunFlags = 0;
	m_hStunner = NULL;

	m_iStunIndex = -1;

#ifdef CLIENT_DLL
	if ( m_pOuter->m_pStunnedEffect )
	{
		// Remove stun stars if they are still around.
		// They might be if we died, etc.
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pStunnedEffect );
		m_pOuter->m_pStunnedEffect = NULL;
	}
#else
	m_PlayerStuns.RemoveAll();
#endif

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ControlStunFading( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pStunnedEffect )
	{
		// Remove stun stars early...
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pStunnedEffect );
		m_pOuter->m_pStunnedEffect = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetStunExpireTime( float flTime )
{
#ifdef GAME_DLL
	if ( GetActiveStunInfo() ) 
	{
		GetActiveStunInfo()->flExpireTime = flTime;
	}
#else
	m_flStunEnd = flTime;
#endif

	UpdateLegacyStunSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Mirror stun info to the old system for networking
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateLegacyStunSystem( void )
{
	// What a mess.
#ifdef GAME_DLL
	stun_struct_t *pStun = GetActiveStunInfo();
	if ( pStun )
	{
		m_hStunner = pStun->hPlayer;
		m_flStunFade = gpGlobals->curtime + pStun->flDuration;
		m_flMovementStunTime = pStun->flDuration; 
		m_flStunEnd = pStun->flExpireTime;
		m_iMovementStunAmount = pStun->flStunAmount; 
		m_iStunFlags = pStun->iStunFlags;

		m_iMovementStunParity = ( m_iMovementStunParity + 1 ) & ( ( 1 << MOVEMENTSTUN_PARITY_BITS ) - 1 ); 
	}
#else
	m_ActiveStunInfo.hPlayer = m_hStunner;
	m_ActiveStunInfo.flDuration = m_flMovementStunTime;
	m_ActiveStunInfo.flExpireTime = m_flStunEnd;
	m_ActiveStunInfo.flStartFadeTime = m_flStunEnd;
	m_ActiveStunInfo.flStunAmount = m_iMovementStunAmount;
	m_ActiveStunInfo.iStunFlags = m_iStunFlags;
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
stun_struct_t *CTFPlayerShared::GetActiveStunInfo( void ) const
{
#ifdef GAME_DLL
	return ( m_PlayerStuns.IsValidIndex( m_iStunIndex ) ) ? const_cast<stun_struct_t*>( &m_PlayerStuns[m_iStunIndex] ) : NULL;
#else
	return ( m_iStunIndex >= 0 ) ? const_cast<stun_struct_t*>( &m_ActiveStunInfo ) : NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayerShared::GetStunner( void )
{ 
	return GetActiveStunInfo() ? GetActiveStunInfo()->hPlayer : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddCritBoost( void )
{
#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritBoostEffect( ECritBoostUpdateType eUpdateType )
{
	bool bShouldDisplayCritBoostEffect = IsCritBoosted()
									  || InCond( TF_COND_ENERGY_BUFF )
									  //|| IsHypeBuffed()
									  || InCond( TF_COND_SNIPERCHARGE_RAGE_BUFF );

	if ( m_pOuter->GetActiveTFWeapon() )
	{
		bShouldDisplayCritBoostEffect &= m_pOuter->GetActiveTFWeapon()->CanBeCritBoosted();
	}

	// Never show crit boost effects when stealthed
	bShouldDisplayCritBoostEffect &= !IsStealthed();

	// Never show crit boost effects when disguised unless we're the local player (so crits show on our viewmodel)
	if ( !m_pOuter->IsLocalPlayer() )
	{
		bShouldDisplayCritBoostEffect &= !InCond( TF_COND_DISGUISED );
	}

	// Remove our current crit-boosted effect if we're forcing a refresh (in which case we'll
	// regenerate an effect below) or if we aren't supposed to have an effect active.
	if ( eUpdateType == kCritBoost_ForceRefresh || !bShouldDisplayCritBoostEffect )
	{
		if ( m_pOuter->m_pCritBoostEffect )
		{
			Assert( m_pOuter->m_pCritBoostEffect->IsValid() );
			if ( m_pOuter->m_pCritBoostEffect->GetOwner() )
			{
				m_pOuter->m_pCritBoostEffect->GetOwner()->ParticleProp()->StopEmissionAndDestroyImmediately( m_pOuter->m_pCritBoostEffect );
			}
			else
			{
				m_pOuter->m_pCritBoostEffect->StopEmission();
			}

			m_pOuter->m_pCritBoostEffect = NULL;
		}

#ifdef CLIENT_DLL
		if ( m_pCritBoostSoundLoop )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pCritBoostSoundLoop );
			m_pCritBoostSoundLoop = NULL;
		}
#endif
	}

	// Should we have an active crit effect?
	if ( bShouldDisplayCritBoostEffect )
	{
		CBaseEntity *pWeapon = NULL;
		// Use GetRenderedWeaponModel() instead?
		if ( m_pOuter->IsLocalPlayer() )
		{
			pWeapon = m_pOuter->GetViewModel(0);
		}
		else
		{
			// is this player an enemy?
			if ( m_pOuter->GetTeamNumber() != GetLocalPlayerTeam() )
			{
				// are they a cloaked spy? or disguised as someone who almost assuredly isn't also critboosted?
				if ( IsStealthed() || InCond( TF_COND_STEALTHED_BLINK ) || InCond( TF_COND_DISGUISED ) )
					return;
			}

			pWeapon = m_pOuter->GetActiveWeapon();
		}

		if ( pWeapon )
		{
			if ( !m_pOuter->m_pCritBoostEffect )
			{
				if ( InCond( TF_COND_DISGUISED ) && !m_pOuter->IsLocalPlayer() && m_pOuter->GetTeamNumber() != GetLocalPlayerTeam() )
				{
					const char *pEffectName = ( GetDisguiseTeam() == TF_TEAM_RED ) ? "critgun_weaponmodel_red" : "critgun_weaponmodel_blu";
					m_pOuter->m_pCritBoostEffect = pWeapon->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
				}
				else
				{
					const char *pEffectName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "critgun_weaponmodel_red" : "critgun_weaponmodel_blu";
					m_pOuter->m_pCritBoostEffect = pWeapon->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
				}

				if ( m_pOuter->IsLocalPlayer() )
				{
					if ( m_pOuter->m_pCritBoostEffect )
					{
						ClientLeafSystem()->SetRenderGroup( m_pOuter->m_pCritBoostEffect->RenderHandle(), RENDER_GROUP_VIEW_MODEL_TRANSLUCENT );
					}
				}
			}
			else
			{
				m_pOuter->m_pCritBoostEffect->StartEmission();
			}

			Assert( m_pOuter->m_pCritBoostEffect->IsValid() );
		}

#ifdef CLIENT_DLL
		if ( m_pOuter->GetActiveTFWeapon() && !m_pCritBoostSoundLoop )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			CLocalPlayerFilter filter;
			m_pCritBoostSoundLoop = controller.SoundCreate( filter, m_pOuter->entindex(), "Weapon_General.CritPower" );	
			controller.Play( m_pCritBoostSoundLoop, 1.0, 100 );
		}
#endif
	}
}
#endif

//-----------------------------------------------------------------------------
// Soda Popper Condition
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddSodaPopperHype( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->EmitSound( "DisciplineDevice.PowerUp" );
	}
#endif // CLIENT_DLL
}
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveSodaPopperHype( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		m_pOuter->EmitSound( "DisciplineDevice.PowerDown" );
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Soda Popper Condition
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddCondGas( void )
{
#ifdef CLIENT_DLL
	// don't need to add the effect if they're already burning
	if ( !InCond( TF_COND_BURNING ) && !m_pOuter->m_pGasEffect )
	{
		// You'll only have the drip effect from the opposing team
		m_pOuter->m_pGasEffect = m_pOuter->ParticleProp()->Create( ( m_pOuter->GetTeamNumber() == TF_TEAM_BLUE ) ? "gas_can_drips_red" : "gas_can_drips_blue", PATTACH_ABSORIGIN_FOLLOW );
	}

	if ( m_pOuter->m_pGasEffect )
	{
		m_pOuter->ParticleProp()->AddControlPoint( m_pOuter->m_pGasEffect, 1, m_pOuter, PATTACH_ABSORIGIN_FOLLOW );
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( TF_SCREEN_OVERLAY_MATERIAL_GAS, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}

#endif // CLIENT_DLL
}
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveCondGas( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pGasEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pGasEffect );
		m_pOuter->m_pGasEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		// only remove the overlay if it is urine
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();
		if ( pMaterial && FStrEq( pMaterial->GetName(), TF_SCREEN_OVERLAY_MATERIAL_GAS ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetStealthNoAttackExpireTime( void )
{
	return m_flStealthNoAttackExpire;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominated.Set( iPlayerIndex, bDominated );
	pPlayer->m_Shared.SetPlayerDominatingMe( m_pOuter, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is being dominated by the other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominatingMe.Set( iPlayerIndex, bDominated );

#ifdef GAME_DLL
	if ( bDominated )
	{
		CTFPlayer *pDominatingPlayer = ToTFPlayer( pPlayer );
		if ( pDominatingPlayer && pDominatingPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			CBaseEntity *pHealedEntity = pPlayer->MedicGetHealTarget();
			CTFPlayer *pHealedPlayer = ToTFPlayer( pHealedEntity );

			if ( pHealedPlayer && pHealedPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				pHealedPlayer->AwardAchievement( ACHIEVEMENT_TF_HEAVY_EARN_MEDIC_DOMINATION );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominated( int iPlayerIndex )
{
#ifdef CLIENT_DLL
	// On the client, we only have data for the local player.
	// As a result, it's only valid to ask for dominations related to the local player
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	Assert( m_pOuter->IsLocalPlayer() || pLocalPlayer->entindex() == iPlayerIndex );

	if ( m_pOuter->IsLocalPlayer() )
		return m_bPlayerDominated.Get( iPlayerIndex );

	return pLocalPlayer->m_Shared.IsPlayerDominatingMe( m_pOuter->entindex() );
#else
	// Server has all the data.
	return m_bPlayerDominated.Get( iPlayerIndex );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominatingMe( int iPlayerIndex )
{
	return m_bPlayerDominatingMe.Get( iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: True if the given player is a spy disguised as our team.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsSpyDisguisedAsMyTeam( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) &&
		pPlayer->GetTeamNumber() != m_pOuter->GetTeamNumber() &&
		pPlayer->m_Shared.GetDisguiseTeam() == m_pOuter->GetTeamNumber() )
	{
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::NoteLastDamageTime( int nDamage )
{
	// we took damage
	if ( ( nDamage > 5 || InCond( TF_COND_BLEEDING ) ) && !InCond( TF_COND_FEIGN_DEATH ) && InCond( TF_COND_STEALTHED ) )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnSpyTouchedByEnemy( void )
{
	if ( !InCond( TF_COND_FEIGN_DEATH ) && InCond( TF_COND_STEALTHED ) )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsEnteringOrExitingFullyInvisible( void )
{
	return ( ( GetPercentInvisiblePrevious() != 1.f && GetPercentInvisible() == 1.f ) || 
			 ( GetPercentInvisiblePrevious() == 1.f && GetPercentInvisible() != 1.f ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanRuneCharge() const
{
	return InCond( TF_COND_RUNE_SUPERNOVA );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetItemChargeMeter( loadout_positions_t slot, float flValue )
{ 
	if ( ( slot >= FIRST_LOADOUT_SLOT_WITH_CHARGE_METER ) && ( slot <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER ) )
	{
		m_flPrevItemChargeMeter[ slot ] = m_flItemChargeMeter[ slot ];
		m_flItemChargeMeter.Set( slot, Clamp( flValue, 0.f, 100.f ) );

		if ( !m_pOuter )
			return;
		
		IHasGenericMeter *pGenericMeterUser = dynamic_cast< IHasGenericMeter* >( m_pOuter->GetEntityForLoadoutSlot( slot, true ) );
		if ( !pGenericMeterUser )
			return;

		const float flEndVal = pGenericMeterUser->GetMeterMultiplier() >= 0.f ? 100.f : 0.f;
		float flCurrentCharge = m_flItemChargeMeter[slot];
		float flPrevCharge = m_flPrevItemChargeMeter[slot];

		// no change
		if ( flCurrentCharge == flPrevCharge )
			return;

		// Check if we just filled an interval of this meter, let the item know in case it wants to do something special
		const float flChargeInterval = pGenericMeterUser->GetChargeInterval();
		const float flPrevChargeInterval = ceil( flPrevCharge / flChargeInterval ) * flChargeInterval;

		float flCurSign = Sign( flCurrentCharge - flPrevChargeInterval );
		float flPrevSign = Sign( flPrevCharge - flPrevChargeInterval );
		if ( ( flCurSign > 0 ) && ( flCurSign != flPrevSign ) )
		{
			pGenericMeterUser->OnResourceMeterFilled();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateItemChargeMeters()
{
	for( int i= FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; i <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++i )
	{
		int nMeterChargeType = 0;
		float flMeterChargeRate = 0.f;
		loadout_positions_t slot( (loadout_positions_t)i );
		CBaseEntity* pItem = m_pOuter->GetEntityForLoadoutSlot( i, true );
		if ( !pItem )
			continue;

		CALL_ATTRIB_HOOK_INT_ON_OTHER( pItem, nMeterChargeType, item_meter_charge_type );
		if ( ( nMeterChargeType == ATTRIBUTE_METER_TYPE_TIME ) || ( nMeterChargeType == ATTRIBUTE_METER_TYPE_COMBO ) )
		{
			// See how fast to charge
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pItem, flMeterChargeRate, item_meter_charge_rate );
			if ( flMeterChargeRate == 0.f )
				continue;

			// Modify it?
			float flMeterChargeRateMod = 1.f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pItem, flMeterChargeRateMod, mult_item_meter_charge_rate );
			flMeterChargeRate *= flMeterChargeRateMod;

			// Should we even charge?  The item can pause this if it wants
			IHasGenericMeter *pGenericMeterUser = dynamic_cast< IHasGenericMeter* >( m_pOuter->GetEntityForLoadoutSlot( slot, true ) );
			if ( !pGenericMeterUser || !pGenericMeterUser->ShouldUpdateMeter() )
				continue;
			
			// Fill up based on time
			const float flDeltaTime = ( gpGlobals->frametime ) * 100.f;
			SetItemChargeMeter( slot, GetItemChargeMeter( slot ) + pGenericMeterUser->GetMeterMultiplier() * ( flDeltaTime / flMeterChargeRate ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanFallStomp( void )
{
	if ( InCond( TF_COND_ROCKETPACK ) )
		return true;

	int iHeadStomp = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iHeadStomp, boots_falling_stomp );

	return iHeadStomp != 0;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::FadeInvis( float fAdditionalRateScale )
{
	ETFCond nExpiringCondition = TF_COND_LAST;
	if ( InCond( TF_COND_STEALTHED ) )
	{
		nExpiringCondition = TF_COND_STEALTHED;
		RemoveCond( TF_COND_STEALTHED );
	}
	else if ( InCond( TF_COND_STEALTHED_USER_BUFF ) )
	{
		nExpiringCondition = TF_COND_STEALTHED_USER_BUFF;
		RemoveCond( TF_COND_STEALTHED_USER_BUFF );
	}

#ifdef GAME_DLL
	// inform the bots
	CTFWeaponInvis *pInvis = dynamic_cast< CTFWeaponInvis * >( m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
	if ( pInvis )
	{
		TheNextBots().OnWeaponFired( m_pOuter, pInvis );
	}
#endif

	// If present, give our invisibility weapon a chance to override our decloak
	// rate scale.
	float flDecloakRateScale = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, flDecloakRateScale, mult_decloak_rate );

	// This comes from script, so sanity check the result.
	if ( flDecloakRateScale <= 0.0f )
	{
		flDecloakRateScale = 1.0f;
	}

	float flInvisFadeTime = fAdditionalRateScale
						  * (tf_spy_invis_unstealth_time.GetFloat() * flDecloakRateScale);

	if ( flInvisFadeTime < 0.15 )
	{
		// this was a force respawn, they can attack whenever
	}
	else if ( ( nExpiringCondition != TF_COND_STEALTHED_USER_BUFF ) && !InCond( TF_COND_STEALTHED_USER_BUFF_FADING ) )
	{
		// next attack in some time
		m_flStealthNoAttackExpire = gpGlobals->curtime + (tf_spy_cloak_no_attack_time.GetFloat() * flDecloakRateScale * fAdditionalRateScale);
	}

	m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisFadeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Approach our desired level of invisibility
//-----------------------------------------------------------------------------
void CTFPlayerShared::InvisibilityThink( void )
{
	if ( m_pOuter->GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY && InCond( TF_COND_STEALTHED ) )
	{
		// Shouldn't happen, but it's a safety net
		m_flInvisibility = 0.0f;
		if ( InCond( TF_COND_STEALTHED ) )
		{
			RemoveCond( TF_COND_STEALTHED );
		}
		return;
	}

	float flTargetInvis = 0.0f;
	float flTargetInvisScale = 1.0f;
	if ( InCond( TF_COND_STEALTHED_BLINK ) || InCond( TF_COND_URINE ) )
	{
		// We were bumped into or hit for some damage.
		flTargetInvisScale = TF_SPY_STEALTH_BLINKSCALE;
	}

	// Go invisible or appear.
	if ( m_flInvisChangeCompleteTime > gpGlobals->curtime )
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f - ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) );
		}
		else
		{
			flTargetInvis = ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) * 0.5f );
		}
	}
	else
	{
		if ( IsStealthed() )
		{
			flTargetInvis = 1.0f;
			m_flLastNoMovementTime = -1.f;

			if ( m_bMotionCloak )
			{
				if ( m_flCloakMeter == 0.f )
				{
					Vector vVel = m_pOuter->GetAbsVelocity();
					float fSpdSqr = vVel.LengthSqr();
					flTargetInvis = RemapVal( fSpdSqr, 0, m_pOuter->MaxSpeed()*m_pOuter->MaxSpeed(), 1.0f, 0.5f );
				}
				else
				{
					flTargetInvis = 1.f;
				}
			}
		}
		else
		{
			flTargetInvis = 0.0f;
		}
	}

	flTargetInvis *= flTargetInvisScale;
	m_flPrevInvisibility = m_flInvisibility;
	m_flInvisibility = clamp( flTargetInvis, 0.0f, 1.0f );

}


//-----------------------------------------------------------------------------
// Purpose: How invisible is the player [0..1]
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetPercentInvisible( void ) const
{

	return m_flInvisibility;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsCritBoosted( void ) const
{
	bool bAllWeaponCritActive = ( InCond( TF_COND_CRITBOOSTED ) || 
								  InCond( TF_COND_CRITBOOSTED_PUMPKIN ) || 
								  InCond( TF_COND_CRITBOOSTED_USER_BUFF ) || 
#ifdef CLIENT_DLL
								  InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) || 
#endif
								  InCond( TF_COND_CRITBOOSTED_FIRST_BLOOD ) || 
								  InCond( TF_COND_CRITBOOSTED_BONUS_TIME ) || 
								  InCond( TF_COND_CRITBOOSTED_CTF_CAPTURE ) || 
								  InCond( TF_COND_CRITBOOSTED_ON_KILL ) ||
								  InCond( TF_COND_CRITBOOSTED_CARD_EFFECT ) ||
								  InCond( TF_COND_CRITBOOSTED_RUNE_TEMP ) );

	if ( bAllWeaponCritActive )
		return true;


	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( m_pOuter->GetActiveWeapon() );
	if ( pWeapon )
	{
		if ( InCond( TF_COND_CRITBOOSTED_RAGE_BUFF ) && pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_PRIMARY )
		{
			// Only primary weapon can be crit boosted by pyro rage
			return true;
		}

		float flCritHealthPercent = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flCritHealthPercent, mult_crit_when_health_is_below_percent );

		if ( flCritHealthPercent < 1.0f && m_pOuter->HealthFraction() < flCritHealthPercent )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsInvulnerable( void ) const
{
	bool bInvuln = InCond( TF_COND_INVULNERABLE ) || 
				   InCond( TF_COND_INVULNERABLE_USER_BUFF ) || 
				   InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) ||
				   InCond( TF_COND_INVULNERABLE_CARD_EFFECT );

	return bInvuln;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsStealthed( void ) const
{

	return ( InCond( TF_COND_STEALTHED ) || InCond( TF_COND_STEALTHED_USER_BUFF ) || InCond( TF_COND_STEALTHED_USER_BUFF_FADING ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::CanBeDebuffed( void ) const
{
	if ( IsInvulnerable() )
		return false;

	if ( InCond( TF_COND_PHASE ) || InCond( TF_COND_PASSTIME_INTERCEPTION ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsImmuneToPushback( void ) const
{
	if ( InCond( TF_COND_MEGAHEAL ) )
		return true;

	if ( InCond( TF_COND_IMMUNE_TO_PUSHBACK ) )
		return true;

	if ( m_pOuter->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && InCond( TF_COND_AIMING ) )
	{
		int iImmune = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iImmune, spunup_push_force_immunity );
		if ( iImmune )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Start the process of disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::Disguise( int nTeam, int nClass, CTFPlayer* pDesiredTarget, bool bOnKill )
{
	int nRealTeam = m_pOuter->GetTeamNumber();
	int nRealClass = m_pOuter->GetPlayerClass()->GetClassIndex();

	Assert ( ( nClass >= TF_CLASS_SCOUT ) && ( nClass <= TF_CLASS_ENGINEER ) );

	// we're not a spy
	if ( nRealClass != TF_CLASS_SPY )
	{
		return;
	}

	if ( InCond( TF_COND_TAUNTING ) )
	{
		// not allowed to disguise while taunting
		return;
	}

	// we're not disguising as anything but ourselves (so reset everything)
	if ( nRealTeam == nTeam && nRealClass == nClass )
	{
		RemoveDisguise();
		return;
	}

	// Ignore disguise of the same type, unless we're using 'Your Eternal Reward'
	if ( nTeam == m_nDisguiseTeam && nClass == m_nDisguiseClass && !bOnKill )
	{
#ifdef GAME_DLL
		DetermineDisguiseWeapon( false );
#endif
		return;
	}

	// invalid team
	if ( nTeam <= TEAM_SPECTATOR || nTeam >= TF_TEAM_COUNT )
	{
		return;
	}

	// invalid class
	if ( nClass <= TF_CLASS_UNDEFINED || nClass >= TF_CLASS_COUNT )
	{
		return;
	}

	// are we already in the middle of disguising as this class? 
	// (the lastdisguise key might get pushed multiple times before the disguise is complete)
	if ( InCond( TF_COND_DISGUISING ) )
	{
		if ( nTeam == m_nDesiredDisguiseTeam && nClass == m_nDesiredDisguiseClass )
		{
			return;
		}
	}

	if ( !bOnKill )
	{
		int nDisguiseConsumesCloak = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER ( m_pOuter, nDisguiseConsumesCloak, mod_disguise_consumes_cloak );
		if ( nDisguiseConsumesCloak )
		{
			if ( m_flCloakMeter < 100.f )
			{
				CSingleUserRecipientFilter filter ( m_pOuter );
				m_pOuter->EmitSound ( filter, m_pOuter->entindex (), "Player.UseDeny", NULL, 0.f );
				return;
			}

#ifdef GAME_DLL
			if ( !PointInRespawnRoom( m_pOuter, m_pOuter->WorldSpaceCenter() ) )
			{
				m_flCloakMeter = 0.f;
			}
#endif // GAME_DLL
		}
	}

	m_hDesiredDisguiseTarget.Set( pDesiredTarget );
	m_nDesiredDisguiseClass = nClass;
	m_nDesiredDisguiseTeam = nTeam;

	m_bLastDisguisedAsOwnTeam = ( m_nDesiredDisguiseTeam == m_pOuter->GetTeamNumber() );

	AddCond( TF_COND_DISGUISING );

	// Start the think to complete our disguise
	float flTimeToDisguise = TF_TIME_TO_DISGUISE;
	//CALL_ATTRIB_HOOK_INT_ON_OTHER( m_pOuter, iTimeToDisguise, disguise_speed_penalty ); // Unused Attr

	// STAGING_SPY
	// Quick disguise if you already disguised
	if ( InCond( TF_COND_DISGUISED ) )
	{
		flTimeToDisguise = TF_TIME_TO_QUICK_DISGUISE;
	}

	if ( pDesiredTarget )
	{
		flTimeToDisguise = 0;
	}
	m_flDisguiseCompleteTime = gpGlobals->curtime + flTimeToDisguise;
}

//-----------------------------------------------------------------------------
// Purpose: Set our target with a player we've found to emulate
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
void CTFPlayerShared::FindDisguiseTarget( void )
{
	if ( m_hDesiredDisguiseTarget )
	{
		m_hDisguiseTarget = m_hDesiredDisguiseTarget;
		m_hDesiredDisguiseTarget = NULL;
	}
	else
	{
		m_hDisguiseTarget = m_pOuter->TeamFortress_GetDisguiseTarget( m_nDisguiseTeam, m_nDisguiseClass );
	}

	m_pOuter->CreateDisguiseWeaponList( m_hDisguiseTarget );
}

#endif

int CTFPlayerShared::GetDisguiseTeam( void ) const
{
	return InCond( TF_COND_DISGUISED_AS_DISPENSER ) ? (int)( ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED ) : m_nDisguiseTeam;
}

//-----------------------------------------------------------------------------
// Purpose: Complete our disguise
//-----------------------------------------------------------------------------
void CTFPlayerShared::CompleteDisguise( void )
{
	AddCond( TF_COND_DISGUISED );

	m_nDisguiseClass = m_nDesiredDisguiseClass;
	m_nDisguiseTeam = m_nDesiredDisguiseTeam;

	if ( m_nDisguiseClass == TF_CLASS_SPY )
	{
		m_nMaskClass = rand()%9+1;
	}

	RemoveCond( TF_COND_DISGUISING );

#ifdef GAME_DLL
	// Update the player model and skin.
	m_pOuter->UpdateModel();
	m_pOuter->ClearExpression();

	FindDisguiseTarget();

	if ( GetDisguiseTarget() )
	{
		m_iDisguiseHealth = GetDisguiseTarget()->GetHealth();
		if ( m_iDisguiseHealth <= 0 || !GetDisguiseTarget()->IsAlive() )
		{
			// If we disguised as an enemy who is currently dead, just set us to full health.
			m_iDisguiseHealth = GetDisguiseMaxHealth();
		}
	}
	else
	{
		int iMaxHealth = m_pOuter->GetMaxHealth();
		m_iDisguiseHealth = (int)random->RandomInt( iMaxHealth / 2, iMaxHealth );
	}

	// In Medieval mode, don't force primary weapon because most classes just have melee weapons
	DetermineDisguiseWeapon( !TFGameRules()->IsInMedievalMode() );
	DetermineDisguiseWearables();
#endif

	m_pOuter->TeamFortress_SetSpeed();

	m_flDisguiseCompleteTime = 0.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetDisguiseHealth( int iDisguiseHealth )
{
	m_iDisguiseHealth = iDisguiseHealth;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDisguiseMaxHealth( void )
{
	TFPlayerClassData_t *pClass = g_pTFPlayerClassDataMgr->Get( GetDisguiseClass() );
	if ( pClass )
	{
		return pClass->m_nMaxHealth;
	}
	else
	{
		return m_pOuter->GetMaxHealth();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveDisguise( void )
{
	if ( GetDisguiseTeam() != m_pOuter->GetTeamNumber() )
	{
		if ( InCond( TF_COND_TELEPORTED ) )
		{
			RemoveCond( TF_COND_TELEPORTED );
		}
	}

	RemoveCond( TF_COND_DISGUISED );
	RemoveCond( TF_COND_DISGUISING );

	AddCond( TF_COND_DISGUISE_WEARINGOFF, 0.5f );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::DetermineDisguiseWeapon( bool bForcePrimary )
{
	Assert( m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY );

	const char* strDisguiseWeapon = NULL;

	CTFPlayer *pDisguiseTarget = ToTFPlayer( m_hDisguiseTarget.Get() );
	TFPlayerClassData_t *pData = GetPlayerClassData( m_nDisguiseClass );
	if ( pDisguiseTarget && (pDisguiseTarget->GetPlayerClass()->GetClassIndex() != m_nDisguiseClass) )
	{
		pDisguiseTarget = NULL;
	}

	// Determine which slot we have active.
	int iCurrentSlot = 0;
	if ( m_pOuter->GetActiveTFWeapon() && !bForcePrimary )
	{
		iCurrentSlot = m_pOuter->GetActiveTFWeapon()->GetSlot();
		if ( (iCurrentSlot == 3) && // Cig Case, so they are using the menu not a key bind to disguise.
			m_pOuter->GetLastWeapon() )
		{
			iCurrentSlot = m_pOuter->GetLastWeapon()->GetSlot();
		}
	}

	CTFWeaponBase *pItemWeapon = NULL;
	if ( pDisguiseTarget )
	{
		CTFWeaponBase *pLastDisguiseWeapon = m_hDisguiseWeapon;
		CTFWeaponBase *pFirstValidWeapon = NULL;
		// Cycle through the target's weapons and see if we have a match.
		// Note that it's possible the disguise target doesn't have a weapon in the slot we want,
		// for example if they have replaced it with an unlockable that isn't a weapon (wearable).
		for ( int i=0; i<m_pOuter->m_hDisguiseWeaponList.Count(); ++i )
		{
			CTFWeaponBase *pWeapon = m_pOuter->m_hDisguiseWeaponList[i];

			if ( !pWeapon )
				continue;

			if ( !pFirstValidWeapon )
			{
				pFirstValidWeapon = pWeapon;
			}

			// skip passtime gun
			if ( pWeapon->GetWeaponID() == TF_WEAPON_PASSTIME_GUN )
			{
				continue;
			}

			if ( pWeapon->GetSlot() == iCurrentSlot )
			{
				pItemWeapon = pWeapon;
				break;
			}
		}

		if ( !pItemWeapon )
		{
			if ( pLastDisguiseWeapon )
			{
				pItemWeapon = pLastDisguiseWeapon;
			}
			else if ( pFirstValidWeapon )
			{
				pItemWeapon = pFirstValidWeapon;
			}
		}

		if ( pItemWeapon )
		{
			strDisguiseWeapon = pItemWeapon->GetClassname();
		}
	}

	if ( !pItemWeapon && pData )
	{
		// We have not found our item yet, so cycle through the class's default weapons
		// to find a match.
		for ( int i=0; i<TF_PLAYER_WEAPON_COUNT; ++i )
		{
			if ( pData->m_aWeapons[i] == TF_WEAPON_NONE )
				continue;
			const char *pWpnName = WeaponIdToAlias( pData->m_aWeapons[i] );
			pWpnName = TranslateWeaponEntForClass( pWpnName, m_nDisguiseClass );
			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWpnName );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
			if ( pWeaponInfo->iSlot == iCurrentSlot )
			{
				strDisguiseWeapon = pWeaponInfo->szClassName;
			}
		}
	}

	if ( strDisguiseWeapon )
	{
		// Remove the old disguise weapon, if any.
		RemoveDisguiseWeapon();

		CEconItemView *pItem = NULL;
		if ( pItemWeapon )
		{
			// We are copying a generated, non-base item.
			CAttributeContainer *pContainer = pItemWeapon->GetAttributeContainer();
			if ( pContainer )
			{
				pItem = pContainer->GetItem();
			}
		}

		// We may need a sub-type if we're a builder. Otherwise we'll always appear as a engineer's workbox.
		int iSubType = 0;
		if ( Q_strcmp( strDisguiseWeapon, "tf_weapon_builder" ) == 0 )
		{
			return; // Temporary.
		}

		m_hDisguiseWeapon.Set( dynamic_cast<CTFWeaponBase*>(m_pOuter->GiveNamedItem( strDisguiseWeapon, iSubType, pItem, true )) );
		if ( m_hDisguiseWeapon )
		{
			m_hDisguiseWeapon->SetSolid( SOLID_NONE );
			m_hDisguiseWeapon->SetSolidFlags( FSOLID_NOT_SOLID );
			m_hDisguiseWeapon->SetTouch( NULL );// no touch
			m_hDisguiseWeapon->SetOwner( dynamic_cast<CBaseCombatCharacter*>(m_pOuter) );
			m_hDisguiseWeapon->SetOwnerEntity( m_pOuter );
			m_hDisguiseWeapon->SetParent( m_pOuter );
			m_hDisguiseWeapon->FollowEntity( m_pOuter, true );
			m_hDisguiseWeapon->m_iState = WEAPON_IS_ACTIVE;
			m_hDisguiseWeapon->m_bDisguiseWeapon = true;
			m_hDisguiseWeapon->SetContextThink( &CTFWeaponBase::DisguiseWeaponThink, gpGlobals->curtime + 0.5, "DisguiseWeaponThink" );


			// Ammo/clip state is displayed to attached medics
			m_iDisguiseAmmo = 0;
			if ( !m_hDisguiseWeapon->IsMeleeWeapon() )
			{
				// Use the player we're disguised as if possible
				if ( pDisguiseTarget )
				{
					CTFWeaponBase *pWeapon = pDisguiseTarget->GetActiveTFWeapon();
					if ( pWeapon && pWeapon->GetWeaponID() == m_hDisguiseWeapon->GetWeaponID() )
					{
						m_iDisguiseAmmo = pWeapon->UsesClipsForAmmo1() ? 
										  pWeapon->Clip1() : 
										  pDisguiseTarget->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
					}
				}

				// Otherwise display a faked ammo count
				if ( !m_iDisguiseAmmo )
				{
					int nMaxCount = m_hDisguiseWeapon->UsesClipsForAmmo1() ? 
									m_hDisguiseWeapon->GetMaxClip1() : 
									m_pOuter->GetMaxAmmo( m_hDisguiseWeapon->GetPrimaryAmmoType(), m_nDisguiseClass );
				
					m_iDisguiseAmmo = (int)random->RandomInt( 1, nMaxCount );
				}
			}
		}
	}
}

void CTFPlayerShared::DetermineDisguiseWearables()
{
	CTFPlayer *pDisguiseTarget = ToTFPlayer( m_hDisguiseTarget.Get() );
	if ( !pDisguiseTarget )
		return;

	// Remove any existing disguise wearables.
	RemoveDisguiseWearables();

	if ( GetDisguiseClass() != pDisguiseTarget->GetPlayerClass()->GetClassIndex() )
		return;

	// Equip us with copies of our disguise target's wearables.
	int iPlayerSkinOverride = 0;
	for ( int i=0; i<pDisguiseTarget->GetNumWearables(); ++i )
	{
		CTFWearable *pWearable = dynamic_cast<CTFWearable*>( pDisguiseTarget->GetWearable( i ) );
		if ( pWearable )
		{
			if ( pWearable->IsDisguiseWearable() )
				continue; // Never copy a target's disguise wearables.
			CEconItemView *pScriptItem = pWearable->GetAttributeContainer()->GetItem();
			// Never copy target's action slot items
			if ( pScriptItem && pScriptItem->IsValid() && pScriptItem->GetStaticData()->GetItemClass() && ( pScriptItem->GetStaticData()->GetLoadoutSlot( GetDisguiseClass() ) != LOADOUT_POSITION_ACTION ) )
			{
				CEconEntity *pNewItem = dynamic_cast<CEconEntity*>( m_pOuter->GiveNamedItem( pScriptItem->GetStaticData()->GetItemClass(), 0, pScriptItem ) );
				CTFWearable *pNewWearable = dynamic_cast<CTFWearable*>( pNewItem );
				Assert( pNewWearable );
				if ( pNewWearable )
				{
					pNewWearable->SetDisguiseWearable( true );
					pNewWearable->GiveTo( m_pOuter );

					// copy over the level for levelable items
					CTFWearableLevelableItem *pLevelableItem = dynamic_cast<CTFWearableLevelableItem*>( pWearable );
					CTFWearableLevelableItem *pNewLevelableItem = dynamic_cast<CTFWearableLevelableItem*>( pNewWearable );
					if ( pLevelableItem && pNewLevelableItem )
					{
						int nBodyGroup = pNewLevelableItem->FindBodygroupByName( LEVELABLE_ITEM_BODYGROUP_NAME );
						if ( nBodyGroup != -1 )
						{
							pNewLevelableItem->SetBodygroup( nBodyGroup, pLevelableItem->GetLevel() );
						}
					}

					// find the first skin override item
					if ( iPlayerSkinOverride == 0 )
					{
						CALL_ATTRIB_HOOK_INT_ON_OTHER( pNewWearable, iPlayerSkinOverride, player_skin_override );
					}
				}
			}
		}
	}

	m_nDisguiseSkinOverride = iPlayerSkinOverride;
}

void CTFPlayerShared::RemoveDisguiseWearables()
{
	bool bFoundDisguiseWearable = true;
	while ( bFoundDisguiseWearable )
	{
		int i = 0;
		for ( ; i<m_pOuter->GetNumWearables(); ++i )
		{
			CTFWearable *pWearable = dynamic_cast<CTFWearable*>( m_pOuter->GetWearable( i ) );
			if ( pWearable && pWearable->IsDisguiseWearable() )
			{
				// Every time we do this the list changes, so we have to loop through again.
				pWearable->RemoveFrom( m_pOuter );
				break;
			}
		}
		if ( i == m_pOuter->GetNumWearables() )
		{
			bFoundDisguiseWearable = false;
		}
	}

}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ProcessDisguiseImpulse( CTFPlayer *pPlayer )
{
	// Get the player owning the weapon.
	if ( !pPlayer )
		return;

	if ( pPlayer->GetImpulse() > 200 )
	{ 
		char szImpulse[6];
		Q_snprintf( szImpulse, sizeof( szImpulse ), "%d", pPlayer->GetImpulse() );

		char szTeam[3];
		Q_snprintf( szTeam, sizeof( szTeam ), "%c", szImpulse[1] );

		char szClass[3];
		Q_snprintf( szClass, sizeof( szClass ), "%c", szImpulse[2] );

		// 'Your Eternal Reward' handling
		bool bSwitchWeaponOnly = false;
		if ( pPlayer->CanDisguise_OnKill() && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			// Only trying to change the disguise weapon via 'lastdisguise'
			if ( Q_atoi( szClass ) == pPlayer->m_Shared.GetDisguiseClass() && Q_atoi( szTeam ) == pPlayer->m_Shared.GetDisguiseTeam() )
			{
				bSwitchWeaponOnly = true;
			}
		}

		if ( pPlayer->CanDisguise() || bSwitchWeaponOnly )
		{
			// intercepting the team value and reassigning what gets passed into Disguise()
			// because the team numbers in the client menu don't match the #define values for the teams
			pPlayer->m_Shared.Disguise( Q_atoi( szTeam ), Q_atoi( szClass ) );

			// Switch from the PDA to our previous weapon
			if ( GetActiveTFWeapon() && GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_PDA_SPY )
			{
				pPlayer->SelectLastItem();
			}
		}
	}
}

bool CTFPlayerShared::CanRecieveMedigunChargeEffect( medigun_charge_types eType ) const
{
	bool bCanRecieve = true;

	const CTFItem *pItem = m_pOuter->GetItem();
	if ( pItem && pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		bCanRecieve = false;

		// The "flag" in Player Destruction doesn't block uber
		const CCaptureFlag* pFlag = static_cast< const CCaptureFlag* >( pItem );
		if ( pFlag->GetType() == TF_FLAGTYPE_PLAYER_DESTRUCTION )
		{
			bCanRecieve = true;
		}

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			// allow bot flag carriers to be ubered
			bCanRecieve = true;
		}

		if ( ( eType == MEDIGUN_CHARGE_MEGAHEAL ) 
		  || ( eType == MEDIGUN_CHARGE_BULLET_RESIST )
		  || ( eType == MEDIGUN_CHARGE_BLAST_RESIST )
		  || ( eType == MEDIGUN_CHARGE_FIRE_RESIST ) )
		{
			bCanRecieve = true;
		}
	}

	
	if( TFGameRules() && TFGameRules()->IsPasstimeMode() )
	{
		bCanRecieve &= ! HasPasstimeBall();
	}

	return bCanRecieve;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::Heal( CBaseEntity *pHealer, float flAmount, float flOverhealBonus, float flOverhealDecayMult, bool bDispenserHeal /* = false */, CTFPlayer *pHealScorer /* = NULL */ )
{
	// If already healing, stop healing
	float flHealAccum = 0;
	if ( FindHealerIndex(pHealer) != m_aHealers.InvalidIndex() )
	{
		flHealAccum = StopHealing( pHealer );
	}

	healers_t newHealer;
	newHealer.pHealer = pHealer;
	newHealer.flAmount = flAmount;
	newHealer.flHealAccum = flHealAccum;
	newHealer.iKillsWhileBeingHealed = 0;
	newHealer.flOverhealBonus = flOverhealBonus;
	newHealer.flOverhealDecayMult = flOverhealDecayMult;
	newHealer.bDispenserHeal = bDispenserHeal;
	newHealer.flHealedLastSecond = 0;

	if ( pHealScorer )
	{
		newHealer.pHealScorer = pHealScorer;
	}
	else
	{
		//Assert( pHealer->IsPlayer() );
		newHealer.pHealScorer = pHealer;
	}

	m_aHealers.AddToTail( newHealer );

	AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION, pHealer );

	RecalculateChargeEffects();

	m_nNumHealers = m_aHealers.Count();

	if ( pHealer && pHealer->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pHealer );
		Assert(pPlayer);
		pPlayer->m_AchievementData.AddTargetToHistory( m_pOuter );
		pPlayer->TeamFortress_SetSpeed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
float CTFPlayerShared::StopHealing( CBaseEntity *pHealer )
{
	int iIndex = FindHealerIndex(pHealer);
	if ( iIndex == m_aHealers.InvalidIndex() )
		return 0;

	float flHealingDone = 0.f;

	if ( iIndex != m_aHealers.InvalidIndex() )
	{
		flHealingDone = m_aHealers[iIndex].flHealAccum;
		m_aHealers.Remove( iIndex );
	}

	if ( !m_aHealers.Count() )
	{
		RemoveCond( TF_COND_HEALTH_BUFF );
	}

	RecalculateChargeEffects();

	m_nNumHealers = m_aHealers.Count();

	return flHealingDone;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculateChargeEffects( bool bInstantRemove )
{
	struct medic_charges_t
	{
		bool bActive;
		CTFPlayer *pProvider;
	};

	medic_charges_t aCharges[MEDIGUN_NUM_CHARGE_TYPES];

	for ( int i = 0; i < ARRAYSIZE( aCharges ); i++ )
	{
		aCharges[ i ].bActive = false;
		aCharges[i].pProvider = NULL;
	}

	medigun_charge_types iMyCharge = m_pOuter->GetChargeEffectBeingProvided();

	if ( iMyCharge != MEDIGUN_CHARGE_INVALID )
	{
		Assert( iMyCharge >= 0 && iMyCharge < MEDIGUN_NUM_CHARGE_TYPES );
		aCharges[iMyCharge].bActive = true;
		aCharges[iMyCharge].pProvider = m_pOuter;
	}

	// Loop through our medics and get all their charges
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		if ( !m_aHealers[i].pHealer )
			continue;

		CTFPlayer *pPlayer = ToTFPlayer( m_aHealers[i].pHealer );
		if ( !pPlayer )
			continue;

		medigun_charge_types iCharge = pPlayer->GetChargeEffectBeingProvided();

		if ( iCharge != MEDIGUN_CHARGE_INVALID )
		{
			Assert( iCharge >= 0 && iCharge < MEDIGUN_NUM_CHARGE_TYPES );
			aCharges[iCharge].bActive = true;
			aCharges[iCharge].pProvider = pPlayer;
		}
	}

	if ( !CanRecieveMedigunChargeEffect( iMyCharge ) )
	{
		aCharges[MEDIGUN_CHARGE_INVULN].bActive = false;
	}

	SetChargeEffect( MEDIGUN_CHARGE_INVULN,			aCharges[MEDIGUN_CHARGE_INVULN].bActive,		bInstantRemove, g_MedigunEffects[ MEDIGUN_CHARGE_INVULN ],			tf_invuln_time.GetFloat(),	aCharges[MEDIGUN_CHARGE_INVULN].pProvider );
	SetChargeEffect( MEDIGUN_CHARGE_CRITICALBOOST,	aCharges[MEDIGUN_CHARGE_CRITICALBOOST].bActive, bInstantRemove, g_MedigunEffects[ MEDIGUN_CHARGE_CRITICALBOOST ],	0.0f,						aCharges[MEDIGUN_CHARGE_CRITICALBOOST].pProvider );
	SetChargeEffect( MEDIGUN_CHARGE_MEGAHEAL,		aCharges[MEDIGUN_CHARGE_MEGAHEAL].bActive,		bInstantRemove, g_MedigunEffects[ MEDIGUN_CHARGE_MEGAHEAL ],		0.0f,						aCharges[MEDIGUN_CHARGE_MEGAHEAL].pProvider );
	SetChargeEffect( MEDIGUN_CHARGE_BULLET_RESIST,	aCharges[MEDIGUN_CHARGE_BULLET_RESIST].bActive,	bInstantRemove, g_MedigunEffects[ MEDIGUN_CHARGE_BULLET_RESIST ],	0.0f,						aCharges[MEDIGUN_CHARGE_BULLET_RESIST].pProvider );
	SetChargeEffect( MEDIGUN_CHARGE_BLAST_RESIST,	aCharges[MEDIGUN_CHARGE_BLAST_RESIST].bActive,	bInstantRemove, g_MedigunEffects[ MEDIGUN_CHARGE_BLAST_RESIST ],	0.0f,						aCharges[MEDIGUN_CHARGE_BLAST_RESIST].pProvider );
	SetChargeEffect( MEDIGUN_CHARGE_FIRE_RESIST,	aCharges[MEDIGUN_CHARGE_FIRE_RESIST].bActive,	bInstantRemove, g_MedigunEffects[ MEDIGUN_CHARGE_FIRE_RESIST ],		0.0f,						aCharges[MEDIGUN_CHARGE_FIRE_RESIST].pProvider );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::TestAndExpireChargeEffect( medigun_charge_types iCharge )
{
	const MedigunEffects_t& effects = g_MedigunEffects[iCharge];

	if ( InCond( effects.eCondition )  )
	{
		bool bRemoveEffect = false;
		bool bGameInWinState = TFGameRules()->State_Get() == GR_STATE_TEAM_WIN;
		bool bPlayerOnWinningTeam = TFGameRules()->GetWinningTeam() == m_pOuter->GetTeamNumber();

		// Lose all charge effects in post-win state if we're the losing team
		if ( bGameInWinState && !bPlayerOnWinningTeam )
		{
			bRemoveEffect = true;
		}

		if ( m_flChargeEffectOffTime[iCharge] )
		{
			if ( gpGlobals->curtime > m_flChargeEffectOffTime[iCharge] )
			{
				bRemoveEffect = true;
			}
			if (iCharge == MEDIGUN_CHARGE_CRITICALBOOST && ( bGameInWinState && bPlayerOnWinningTeam ) )
			{
				bRemoveEffect = false;
				m_flChargeEffectOffTime[iCharge] = 0;
			}
			if ( GetRevengeCrits() > 0 && effects.eCondition == TF_COND_CRITBOOSTED )
			{
				// Don't remove while we have a weapon deployed that can consume revenge crits
				CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
				if ( pWeapon && pWeapon->CanHaveRevengeCrits() )
					bRemoveEffect = false;
			}
		}

		// Check healers for possible usercommand invuln exploit
		FOR_EACH_VEC( m_aHealers, i )
		{
			CTFPlayer *pTFHealer = ToTFPlayer( m_aHealers[i].pHealer );
			if ( !pTFHealer )
				continue;

			CTFPlayer *pTFProvider = ToTFPlayer( GetConditionProvider( effects.eCondition ) );
			if ( !pTFProvider )
				continue;

			if ( pTFProvider == pTFHealer && pTFHealer->GetTimeSinceLastUserCommand() > weapon_medigun_chargerelease_rate.GetFloat() + 1.f )
			{
				// Force remove uber and detach the medigun
				bRemoveEffect = true;
				pTFHealer->Weapon_Switch( pTFHealer->Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
			}
		}

		if ( bRemoveEffect )
		{
			m_flChargeEffectOffTime[iCharge] = 0;
			RemoveCond( effects.eCondition );
			if ( effects.eWearingOffCondition != TF_COND_LAST )
			{
				RemoveCond( effects.eWearingOffCondition );
			}
		}
	}
	else if ( m_bChargeSoundEffectsOn[iCharge] )
	{
		if ( effects.pszChargeOnSound[0] )
		{
			m_pOuter->StopSound( effects.pszChargeOnSound );
		}
		m_bChargeSoundEffectsOn[iCharge] = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: We've started a new charge effect
//-----------------------------------------------------------------------------
void CTFPlayerShared::SendNewInvulnGameEvent( void )
{
	// for each medic healing me
	for ( int i=0;i<m_aHealers.Count();i++ )
	{
		CTFPlayer *pMedic = ToTFPlayer( GetHealerByIndex(i) );
		if ( !pMedic )
			continue;

		// ACHIEVEMENT_TF_MEDIC_CHARGE_FRIENDS
		IGameEvent *event = gameeventmanager->CreateEvent( "player_invulned" );

		if ( event )
		{
			event->SetInt( "userid", m_pOuter->GetUserID() );
			event->SetInt( "medic_userid", pMedic->GetUserID() );
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetChargeEffect( medigun_charge_types iCharge, bool bState, bool bInstant, const MedigunEffects_t& effects, float flWearOffTime, CTFPlayer *pProvider /*= NULL*/ )
{
	if ( effects.eCondition == TF_COND_CRITBOOSTED )
	{
		// Don't remove while we have a weapon deployed that can consume revenge crits
		CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
		if ( pWeapon )
		{
			if ( pWeapon->CanHaveRevengeCrits() && GetRevengeCrits() > 0 )
			{
				return;
			}

			if ( pWeapon->HasLastShotCritical() )
			{
				return;
			}
		}
	}

	bool bCurrentState = InCond( effects.eCondition );
	if ( bCurrentState == bState )
	{
		if ( bState && m_flChargeEffectOffTime[iCharge] )
		{
			m_flChargeEffectOffTime[iCharge] = 0;
			if ( effects.eWearingOffCondition != TF_COND_LAST )
			{
				RemoveCond( effects.eWearingOffCondition );
			}

			SendNewInvulnGameEvent();
		}
		return;
	}

	float flNonBotMaxDuration = weapon_medigun_chargerelease_rate.GetFloat();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pProvider, flNonBotMaxDuration, add_uber_time );
	flNonBotMaxDuration += 1.0f;

	// Avoid infinite duration, because... the internet.
	float flMaxDuration = ( pProvider && pProvider->IsBot() ) ? PERMANENT_CONDITION : flNonBotMaxDuration;

	if ( bState )
	{
		if ( m_flChargeEffectOffTime[iCharge] )
		{
			m_pOuter->StopSound( effects.pszChargeOffSound );

			m_flChargeEffectOffTime[iCharge] = 0;
			if ( effects.eWearingOffCondition != TF_COND_LAST )
			{
				RemoveCond( effects.eWearingOffCondition );
			}
		}

		// Invulnerable turning on
		AddCond( effects.eCondition, flMaxDuration, pProvider );

		SendNewInvulnGameEvent();

		CSingleUserRecipientFilter filter( m_pOuter );
		m_pOuter->EmitSound( filter, m_pOuter->entindex(), effects.pszChargeOnSound );
		m_bChargeOffSounded = false;
		m_bChargeSoundEffectsOn[iCharge] = true;
	}
	else
	{
		if ( m_bChargeSoundEffectsOn[iCharge] )
		{
			m_pOuter->StopSound( effects.pszChargeOnSound );
			m_bChargeSoundEffectsOn[iCharge] = false;
		}

		if ( !m_flChargeEffectOffTime[iCharge] && !m_bChargeOffSounded )
		{
			// Make sure we don't have duplicate Off sounds playing
			m_pOuter->StopSound( effects.pszChargeOffSound );

			CSingleUserRecipientFilter filter( m_pOuter );
			m_pOuter->EmitSound( filter, m_pOuter->entindex(), effects.pszChargeOffSound );
			m_bChargeOffSounded = true;
		}

		if ( bInstant )
		{
			m_flChargeEffectOffTime[iCharge] = 0;
			RemoveCond( effects.eCondition );
			if ( effects.eWearingOffCondition != TF_COND_LAST )
			{
				RemoveCond( effects.eWearingOffCondition );
			}
		}
		else
		{
			// We're already in the process of turning it off
			if ( m_flChargeEffectOffTime[iCharge] )
				return;

			if ( effects.eWearingOffCondition != TF_COND_LAST )
			{
				AddCond( effects.eWearingOffCondition, PERMANENT_CONDITION, pProvider );
			}
			m_flChargeEffectOffTime[iCharge] = gpGlobals->curtime + flWearOffTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Collect currency packs in a radius around the scout
//-----------------------------------------------------------------------------
void CTFPlayerShared::RadiusCurrencyCollectionCheck( void )
{
	if ( m_pOuter->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS && TFGameRules()->IsMannVsMachineMode() )
		return;

	if ( !m_pOuter->IsAlive() )
		return;

	if ( m_flRadiusCurrencyCollectionTime > gpGlobals->curtime )
		return;
	
	bool bScout = m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT;
	const int nRadiusSqr = bScout ? 288 * 288 : 72 * 72;
	Vector vecPos = m_pOuter->GetAbsOrigin();

	// NDebugOverlay::Sphere( vecPos, nRadius, 0, 255, 0, 40, 5 );

	for ( int i = 0; i < ICurrencyPackAutoList::AutoList().Count(); ++i )
	{
		CCurrencyPack *pCurrencyPack = static_cast< CCurrencyPack* >( ICurrencyPackAutoList::AutoList()[i] );
		if ( !pCurrencyPack )
			continue;

		if ( !pCurrencyPack->AffectedByRadiusCollection() )
			continue;

		if ( ( vecPos - pCurrencyPack->GetAbsOrigin() ).LengthSqr() > nRadiusSqr )
			continue;

		if ( pCurrencyPack->IsClaimed() )
			continue;

		if ( m_pOuter->FVisible( pCurrencyPack, MASK_OPAQUE ) == false )
			continue;

		if ( !pCurrencyPack->ValidTouch( m_pOuter ) )
			continue;

		// Currencypack's seek classes with a large collection radius
		if ( bScout )
		{
			bool bFound = false;
			FOR_EACH_VEC( m_CurrencyPacks, i )
			{
				pulledcurrencypacks_t packinfo = m_CurrencyPacks[i];
				if ( packinfo.hPack == pCurrencyPack )
					bFound = true;
			}

			if ( !bFound )
			{
				// Mark as claimed to prevent other players from grabbing
				pCurrencyPack->SetClaimed();
				pulledcurrencypacks_t packinfo;
				packinfo.hPack = pCurrencyPack;
				packinfo.flTime = gpGlobals->curtime + 1.f;
				m_CurrencyPacks.AddToTail( packinfo );
			}
		}
		else
		{
			pCurrencyPack->Touch( m_pOuter );
		}
	}

	FOR_EACH_VEC_BACK( m_CurrencyPacks, i )
	{
		if ( m_CurrencyPacks[i].hPack )
		{
			// If the timeout hits, force a touch
			if ( m_CurrencyPacks[i].flTime <= gpGlobals->curtime )
			{
				m_CurrencyPacks[i].hPack->Touch( m_pOuter );
			}
			else
			{
				// Seek the player
				const float flForce = 550.0f;

				Vector vToPlayer = m_pOuter->GetAbsOrigin() - m_CurrencyPacks[i].hPack->GetAbsOrigin();

				vToPlayer.z = 0.0f;
				vToPlayer.NormalizeInPlace();
				vToPlayer.z = 0.25f;

				Vector vPush = flForce * vToPlayer;

				m_CurrencyPacks[i].hPack->RemoveFlag( FL_ONGROUND );
				m_CurrencyPacks[i].hPack->ApplyAbsVelocityImpulse( vPush );
			}
		}
		else
		{
			// Automatic clean-up
			m_CurrencyPacks.Remove( i );
		}
	}

	m_flRadiusCurrencyCollectionTime = bScout ? gpGlobals->curtime + 0.15f : gpGlobals->curtime + 0.25f;
}

//-----------------------------------------------------------------------------
// Purpose: Collect objects in a radius around the player
//-----------------------------------------------------------------------------
void CTFPlayerShared::RadiusHealthkitCollectionCheck( void )
{
	if ( GetCarryingRuneType() != RUNE_PLAGUE )
		return;

	if ( !m_pOuter->IsAlive() )
		return;

	if ( m_flRadiusCurrencyCollectionTime > gpGlobals->curtime )
		return;

	const int nRadiusSqr = 600 * 600;
	const Vector& vecPos = m_pOuter->WorldSpaceCenter();

//	NDebugOverlay::Sphere( vecPos, 600, 0, 255, 0, false, 2.f );

	for ( int i = 0; i < IHealthKitAutoList::AutoList().Count(); ++i )
	{
		CHealthKit *pHealthKit = static_cast<CHealthKit*>( IHealthKitAutoList::AutoList()[i] );
		if ( !pHealthKit )
			continue;

		if ( ( vecPos - pHealthKit->GetAbsOrigin() ).LengthSqr() > nRadiusSqr )
			continue;

		if ( !pHealthKit->ValidTouch( m_pOuter ) )
			continue;

		if ( pHealthKit->IsEffectActive( EF_NODRAW ) )
			continue;

		pHealthKit->ItemTouch( m_pOuter );
	}

	m_flRadiusCurrencyCollectionTime = gpGlobals->curtime + 0.15f;
}

//-----------------------------------------------------------------------------
// Purpose: Scan for and reveal spies in a radius around the player
//-----------------------------------------------------------------------------
void CTFPlayerShared::RadiusSpyScan( void )
{
	if ( m_pOuter->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS )
		return;

	if ( !m_pOuter->IsAlive() )
		return;

	if ( m_flRadiusSpyScanTime <= gpGlobals->curtime )
	{
//		bool bRevealed = false;
		const int iRange = 750;

		CUtlVector<CTFPlayer *> vecPlayers;
		CollectPlayers( &vecPlayers, TF_TEAM_PVE_INVADERS, true );
		FOR_EACH_VEC( vecPlayers, i )
		{

			if ( !vecPlayers[i] )
				continue;

			if ( vecPlayers[i]->GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY )
				continue;
			
			if ( !vecPlayers[i]->m_Shared.InCond( TF_COND_STEALTHED ) )
				continue;

			if ( m_pOuter->FVisible( vecPlayers[i], MASK_OPAQUE ) == false )
				continue;

			Vector vDist = vecPlayers[i]->GetAbsOrigin() - m_pOuter->GetAbsOrigin();
			if ( vDist.LengthSqr() <= iRange * iRange )
			{
				vecPlayers[i]->m_Shared.OnSpyTouchedByEnemy();
//				bRevealed = true;
			}
		}

// 		if ( bRevealed )
// 		{
// 			bRevealed = false;
// 			CSingleUserRecipientFilter filter( m_pOuter );
// 			m_pOuter->EmitSound( filter, m_pOuter->entindex(), "Recon.Ping" );
// 		}

		m_flRadiusSpyScanTime = gpGlobals->curtime + 0.3f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ApplyAttributeToPlayer( const char* pszAttribName, float flValue )
{
	const CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttribName );
	if ( !pDef )
		return;

	m_pOuter->GetAttributeList()->SetRuntimeAttributeValue( pDef, flValue );
	m_pOuter->TeamFortress_SetSpeed();
	m_pOuter->GetAttributeManager()->OnAttributeValuesChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAttributeFromPlayer( const char* pszAttribName )
{
	const CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pszAttribName );
	if ( !pDef )
		return;

	m_pOuter->GetAttributeList()->RemoveAttribute( pDef );
	m_pOuter->TeamFortress_SetSpeed();
	m_pOuter->GetAttributeManager()->OnAttributeValuesChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddTmpDamageBonus( float flBonus, float flExpiration )
{
	AddCond( TF_COND_TMPDAMAGEBONUS, flExpiration );
	m_flTmpDamageBonusAmount += flBonus;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::FindHealerIndex( CBaseEntity *pHealer )
{
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		if ( m_aHealers[i].pHealer == pHealer )
			return i;
	}

	return m_aHealers.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayerShared::GetHealerByIndex( int index )
{
	int iNumHealers = m_aHealers.Count();

	if ( index < 0 || index >= iNumHealers )
		return NULL;

	return m_aHealers[index].pHealer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::HealerIsDispenser( int index )
{
	int iNumHealers = m_aHealers.Count();

	if ( index < 0 || index >= iNumHealers )
		return false;

	return m_aHealers[index].bDispenserHeal;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first healer in the healer array.  Note that this
//		is an arbitrary healer.
//-----------------------------------------------------------------------------
EHANDLE CTFPlayerShared::GetFirstHealer()
{
	if ( m_aHealers.Count() > 0 )
		return m_aHealers.Head().pHealer;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: External code has decided that the trigger event for an achievement
//			has occurred. Go through our data and give it to the right people.
//-----------------------------------------------------------------------------
void CTFPlayerShared::CheckForAchievement( int iAchievement )
{
	if ( iAchievement == ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE || 
		(iAchievement == ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER && InCond( TF_COND_INVULNERABLE ) ) )
	{
		// ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE : We were just saved from death by invuln. See if any medics deployed
		// their charge on us recently, and if so, give them the achievement.

		// ACHIEVEMENT_TF_MEDIC_CHARGE_BLOCKER: We just blocked a capture, and we're invuln. Whoever's invulning us gets the achievement.

		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( m_aHealers[i].pHealer );
			if ( !pPlayer )
				continue;

			if ( !pPlayer->IsPlayerClass(TF_CLASS_MEDIC) )
				continue;

			CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
			if ( !pWpn )
				continue;

			CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>(pWpn);
			if ( pMedigun && pMedigun->IsReleasingCharge() )
			{
				// Save teammate requires us to have deployed the charge within the last second
				if ( iAchievement != ACHIEVEMENT_TF_MEDIC_SAVE_TEAMMATE || (gpGlobals->curtime - pMedigun->GetReleaseStartedAt()) < 1.0 )
				{
					pPlayer->AwardAchievement( iAchievement );
				}
			}
		}
	}
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Get all of our conditions in a nice CBitVec
//-----------------------------------------------------------------------------
void CTFPlayerShared::GetConditionsBits( CBitVec< TF_COND_LAST >& vbConditions ) const
{
	vbConditions.Set( 0u, (uint32)m_nPlayerCond );
	vbConditions.Set( 1u, (uint32)m_nPlayerCondEx );
	vbConditions.Set( 2u, (uint32)m_nPlayerCondEx2 );
	vbConditions.Set( 3u, (uint32)m_nPlayerCondEx3 );
	vbConditions.Set( 4u, (uint32)m_nPlayerCondEx4 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayerShared::GetActiveTFWeapon() const
{
	return m_pOuter->GetActiveTFWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Team check.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsAlly( CBaseEntity *pEntity )
{
	return ( pEntity->GetTeamNumber() == m_pOuter->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDesiredPlayerClassIndex( void )
{
	return m_iDesiredPlayerClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}

void CTFPlayerShared::SetAirDash( int iAirDash )
{
	m_iAirDash = iAirDash;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetCritMult( void )
{
	float flRemapCritMul = RemapValClamped( m_iCritMult, 0, 255, 1.0, 4.0 );
/*#ifdef CLIENT_DLL
	Msg("CLIENT: Crit mult %.2f - %d\n",flRemapCritMul, m_iCritMult);
#else
	Msg("SERVER: Crit mult %.2f - %d\n", flRemapCritMul, m_iCritMult );
#endif*/

	return flRemapCritMul;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritMult( void )
{
	const float flMinMult = 1.0;
	const float flMaxMult = TF_DAMAGE_CRITMOD_MAXMULT;

	if ( m_DamageEvents.Count() == 0 )
	{
		m_iCritMult = RemapValClamped( flMinMult, 1.0, 4.0, 0, 255 );
		return;
	}

	//Msg( "Crit mult update for %s\n", m_pOuter->GetPlayerName() );
	//Msg( "   Entries: %d\n", m_DamageEvents.Count() );

	// Go through the damage multipliers and remove expired ones, while summing damage of the others
	float flTotalDamage = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta > tf_damage_events_track_for.GetFloat() )
		{
			//Msg( "      Discarded (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			m_DamageEvents.Remove(i);
			continue;
		}

		// Ignore damage we've just done. We do this so that we have time to get those damage events
		// to the client in time for using them in prediction in this code.
		if ( flDelta < TF_DAMAGE_CRITMOD_MINTIME )
		{
			//Msg( "      Ignored (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			continue;
		}

		if ( flDelta > TF_DAMAGE_CRITMOD_MAXTIME )
			continue;

		//Msg( "      Added %.2f (%d: time %.2f, now %.2f)\n", m_DamageEvents[i].flDamage, i, m_DamageEvents[i].flTime, gpGlobals->curtime );

		flTotalDamage += m_DamageEvents[i].flDamage * m_DamageEvents[i].flDamageCritScaleMultiplier;
	}

	float flMult = RemapValClamped( flTotalDamage, 0, TF_DAMAGE_CRITMOD_DAMAGE, flMinMult, flMaxMult );

//	Msg( "   TotalDamage: %.2f   -> Mult %.2f\n", flTotalDamage, flMult );

	m_iCritMult = (int)RemapValClamped( flMult, flMinMult, flMaxMult, 0, 255 );
}

#define CRIT_DAMAGE_TIME		0.1f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecordDamageEvent( const CTakeDamageInfo &info, bool bKill, int nVictimPrevHealth  )
{
	if ( m_DamageEvents.Count() >= MAX_DAMAGE_EVENTS )
	{
		// Remove the oldest event
		m_DamageEvents.Remove( m_DamageEvents.Count()-1 );
	}

	// Don't count critical damage toward the critical multiplier.
	float flDamage = info.GetDamage() - info.GetDamageBonus();

	float flDamageCriticalScale = info.GetDamageType() & DMG_DONT_COUNT_DAMAGE_TOWARDS_CRIT_RATE
								? 0.0f
								: 1.0f;

	// cap the damage at our current health amount since it's going to kill us
	if ( bKill && flDamage > nVictimPrevHealth )
	{
		flDamage = nVictimPrevHealth;
	}

	// Don't allow explosions to stack up damage toward the critical modifier.
	bool bOverride = false;
	if ( info.GetDamageType() & DMG_BLAST )
	{
		int nDamageCount = m_DamageEvents.Count();
		for ( int iDamage = 0; iDamage < nDamageCount; ++iDamage )
		{
			// Was the older event I am checking against an explosion as well?
			if ( m_DamageEvents[iDamage].nDamageType & DMG_BLAST )
			{
				// Did it happen very recently?
				if ( ( gpGlobals->curtime - m_DamageEvents[iDamage].flTime ) < CRIT_DAMAGE_TIME )
				{
					if ( bKill )
					{
						m_DamageEvents[iDamage].nKills++;

						if ( m_pOuter->IsPlayerClass( TF_CLASS_DEMOMAN ) )
						{
							// Make sure the previous & the current are stickybombs, and go with it.
							if ( m_DamageEvents[iDamage].nDamageType == info.GetDamageType() &&
								m_DamageEvents[iDamage].nDamageType == g_aWeaponDamageTypes[TF_WEAPON_PIPEBOMBLAUNCHER] )
							{
								if ( TFGameRules()->IsMannVsMachineMode() && m_DamageEvents[iDamage].nKills >= 10 )
								{
									m_pOuter->AwardAchievement( ACHIEVEMENT_TF_MVM_DEMO_GROUP_KILL );
								}
								else if ( m_DamageEvents[iDamage].nKills >= 3 )
								{
									m_pOuter->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL3_WITH_DETONATION );
								}
							}
						}
					}

					// Take the max damage done in the time frame.
					if ( flDamage > m_DamageEvents[iDamage].flDamage )
					{
						m_DamageEvents[iDamage].flDamage = flDamage;
						m_DamageEvents[iDamage].flDamageCritScaleMultiplier = flDamageCriticalScale;
						m_DamageEvents[iDamage].flTime = gpGlobals->curtime;
						m_DamageEvents[iDamage].nDamageType = info.GetDamageType();

//						Msg( "Update Damage Event: D:%f, T:%f\n", m_DamageEvents[iDamage].flDamage, m_DamageEvents[iDamage].flTime );
					}

					bOverride = true;
				}
			}
		}
	}

	// We overrode a value, don't add this to the list.
	if ( bOverride )
		return;

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = flDamage;
	m_DamageEvents[iIndex].flDamageCritScaleMultiplier = flDamageCriticalScale;
	m_DamageEvents[iIndex].nDamageType = info.GetDamageType();
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].nKills = bKill;

//	Msg( "Damage Event: D:%f, T:%f\n", m_DamageEvents[iIndex].flDamage, m_DamageEvents[iIndex].flTime );

	if ( TFGameRules()->IsMannVsMachineMode() && m_pOuter->IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		int nKillCount = 0;
		int nDamageCount = m_DamageEvents.Count();
		for ( int iDamage = 0; iDamage < nDamageCount; ++iDamage )
		{
			// Did it happen very recently?
			if ( ( gpGlobals->curtime - m_DamageEvents[iDamage].flTime ) < CRIT_DAMAGE_TIME )
			{
				nKillCount += m_DamageEvents[iDamage].nKills;
			}
		}

		if ( nKillCount >= 4 )
		{
			m_pOuter->AwardAchievement( ACHIEVEMENT_TF_MVM_SNIPER_KILL_GROUP );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddTempCritBonus( float flAmount )
{
	if ( m_DamageEvents.Count() >= MAX_DAMAGE_EVENTS )
	{
		// Remove the oldest event
		m_DamageEvents.Remove( m_DamageEvents.Count()-1 );
	}

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = RemapValClamped( flAmount, 0, 1, 0, TF_DAMAGE_CRITMOD_DAMAGE ) / (TF_DAMAGE_CRITMOD_MAXMULT - 1.0);
	m_DamageEvents[iIndex].flDamageCritScaleMultiplier = 1.0f;
	m_DamageEvents[iIndex].nDamageType = DMG_GENERIC;
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].nKills = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::GetNumKillsInTime( float flTime )
{
	if ( tf_damage_events_track_for.GetFloat() < flTime )
	{
		Warning("Player asking for damage events for time %.0f, but tf_damage_events_track_for is only tracking events for %.0f\n", flTime, tf_damage_events_track_for.GetFloat() );
	}

	int iKills = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta < flTime )
		{
			iKills += m_DamageEvents[i].nKills;
		}
	}

	return iKills;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::AddToSpyCloakMeter( float val, bool bForce )
{
	CTFWeaponInvis *pWpn = (CTFWeaponInvis *) m_pOuter->Weapon_OwnsThisID( TF_WEAPON_INVIS );
	if ( !pWpn )
		return false;

	if ( !bForce )
	{
		int iNoItemRegen = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWpn, iNoItemRegen, mod_cloak_no_regen_from_items );
		if ( iNoItemRegen )
			return false;

		// STAGING_SPY
		// Special cloaks only get cloak if not active and receive a smaller portion
		int iNoCloakedPickup = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWpn, iNoCloakedPickup, NoCloakWhenCloaked );

		if ( InCond( TF_COND_STEALTHED ) && iNoCloakedPickup )
		{
			return false;
		}
		else
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, val, ReducedCloakFromAmmo );
		}
	}

	bool bResult = ( val > 0 && m_flCloakMeter < 100.0f );

	m_flCloakMeter = clamp( m_flCloakMeter + val, 0.0f, 100.0f );

	return bResult;
}


#endif

//-----------------------------------------------------------------------------
// Purpose: Stun & Snare Application
//-----------------------------------------------------------------------------
void CTFPlayerShared::StunPlayer( float flTime, float flReductionAmount, int iStunFlags, CTFPlayer* pAttacker )
{
#ifdef GAME_DLL
	// Insanity prevention
	if ( ( m_PlayerStuns.Count() + 1 ) >= 250 )
		return;
#endif

	if ( InCond( TF_COND_PHASE ) || InCond( TF_COND_PASSTIME_INTERCEPTION ) )
		return;

	if ( InCond( TF_COND_MEGAHEAL ) )
		return;

	if ( InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) && !InCond( TF_COND_MVM_BOT_STUN_RADIOWAVE ) )
		return;

#ifdef GAME_DLL
	if ( pAttacker && TFGameRules() && TFGameRules()->IsTruceActive() && pAttacker->IsTruceValidForEnt() )
	{
		if ( ( pAttacker->GetTeamNumber() == TF_TEAM_RED ) || ( pAttacker->GetTeamNumber() == TF_TEAM_BLUE ) )
			return;
	}
#endif

	float flRemapAmount = RemapValClamped( flReductionAmount, 0.0, 1.0, 0, 255 );

#ifdef GAME_DLL
	int iOldStunFlags = GetStunFlags();
#endif

	// Already stunned
	bool bStomp = false;
	if ( InCond( TF_COND_STUNNED ) )
	{
		if ( GetActiveStunInfo() )
		{
			// Is it stronger than the active?
			if ( flRemapAmount > GetActiveStunInfo()->flStunAmount || iStunFlags & TF_STUN_CONTROLS || iStunFlags & TF_STUN_LOSER_STATE )
			{
				bStomp = true;
			}
			// It's weaker.  Would it expire before the active?
			else if ( gpGlobals->curtime + flTime < GetActiveStunInfo()->flExpireTime )
			{
				// Ignore
				return;
			}
		}
	}
	else if ( GetActiveStunInfo() )
	{
#ifdef GAME_DLL
		// Something yanked our TF_COND_STUNNED in an unexpected way
		if ( !HushAsserts() )
			Assert( !"Something yanked out TF_COND_STUNNED." );
		m_PlayerStuns.RemoveAll();
#endif
		m_iStunIndex = -1;
		return;
	}

	// Add it to the stack
	stun_struct_t stunEvent = 
	{
		pAttacker,						// hPlayer
		flTime,							// flDuration
		gpGlobals->curtime + flTime,	// flExpireTime
		gpGlobals->curtime + flTime,	// flStartFadeTime
		flRemapAmount,					// flStunAmount
		iStunFlags						// iStunFlags
	};

	// Should this become the active stun?
	if ( bStomp || !GetActiveStunInfo() )
	{
		// If stomping, see if the stun we're replacing has a stronger slow.
		// This can happen when stuns use TF_STUN_CONTROLS or TF_STUN_LOSER_STATE.
		float flOldStun = GetActiveStunInfo() ? GetActiveStunInfo()->flStunAmount : 0.f;

#ifdef GAME_DLL
		m_iStunIndex = m_PlayerStuns.AddToTail( stunEvent );
#else
		m_iStunIndex = 0;

		if ( prediction->IsFirstTimePredicted() )
		{
			m_ActiveStunInfo = stunEvent;
		}
#endif

		if ( flOldStun > flRemapAmount )
		{
			GetActiveStunInfo()->flStunAmount = flOldStun;
		}
	}
	else
	{
		// Done for now
#ifdef GAME_DLL
		m_PlayerStuns.AddToTail( stunEvent );
#else
		if ( prediction->IsFirstTimePredicted() )
		{
			m_ActiveStunInfo = stunEvent;
		}
#endif
		return;
	}

#ifdef GAME_DLL
	// Add in extra time when TF_STUN_CONTROLS
	if ( GetActiveStunInfo()->iStunFlags & TF_STUN_CONTROLS )
	{
		if ( !InCond( TF_COND_HALLOWEEN_KART ) )
		{
			GetActiveStunInfo()->flExpireTime += CONTROL_STUN_ANIM_TIME;
		}
	}

	GetActiveStunInfo()->flStartFadeTime = gpGlobals->curtime + GetActiveStunInfo()->flDuration;
	
	// Update old system for networking
	UpdateLegacyStunSystem();
	
	if ( GetActiveStunInfo()->iStunFlags & TF_STUN_CONTROLS || GetActiveStunInfo()->iStunFlags & TF_STUN_LOSER_STATE )
	{
		m_pOuter->m_angTauntCamera = m_pOuter->EyeAngles();
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_STUNNED );
		if ( pAttacker )
		{
			pAttacker->SpeakConceptIfAllowed( MP_CONCEPT_STUNNED_TARGET );
		}
	}

	if ( ( GetActiveStunInfo()->iStunFlags & TF_STUN_SOUND ) ||
		 ( GetActiveStunInfo()->iStunFlags & TF_STUN_SPECIAL_SOUND ) ||
		 ( GetActiveStunInfo()->iStunFlags & TF_STUN_CONTROLS ) ||
		 ( GetActiveStunInfo()->iStunFlags & TF_STUN_LOSER_STATE ) )
	{
		m_pOuter->StunSound( pAttacker, GetActiveStunInfo()->iStunFlags, iOldStunFlags );
	}

	// Event for achievements.
	IGameEvent *event = gameeventmanager->CreateEvent( "player_stunned" );
	if ( event )
	{
		if ( pAttacker )
		{
			event->SetInt( "stunner", pAttacker->GetUserID() );
		}
		event->SetInt( "victim", m_pOuter->GetUserID() );
		event->SetBool( "victim_capping", m_pOuter->IsCapturingPoint() );
		event->SetBool( "big_stun", ( GetActiveStunInfo()->iStunFlags & TF_STUN_SPECIAL_SOUND ) != 0 );
		gameeventmanager->FireEvent( event );
	}

	// Clear off all taunts, expressions, and scenes.
	if ( ( GetActiveStunInfo()->iStunFlags & TF_STUN_CONTROLS) == TF_STUN_CONTROLS || ( GetActiveStunInfo()->iStunFlags & TF_STUN_LOSER_STATE) == TF_STUN_LOSER_STATE )
	{
		m_pOuter->StopTaunt();
		m_pOuter->ClearExpression();
		m_pOuter->ClearWeaponFireScene();
	}
#endif

	AddCond( TF_COND_STUNNED, -1.f, pAttacker );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the intensity of the current stun effect, if we have the type of stun indicated.
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetAmountStunned( int iStunFlags )
{
	if ( GetActiveStunInfo() )
	{
		if ( InCond( TF_COND_STUNNED ) && ( iStunFlags & GetActiveStunInfo()->iStunFlags ) && ( GetActiveStunInfo()->flExpireTime > gpGlobals->curtime ) )
			return MIN( MAX( GetActiveStunInfo()->flStunAmount, 0 ), 255 ) * ( 1.f/255.f );
	}

	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that our controls are stunned.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsControlStunned( void )
{
	if ( GetActiveStunInfo() )
	{
		if ( InCond( TF_COND_STUNNED ) && ( m_iStunFlags & TF_STUN_CONTROLS ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that our controls are stunned.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsLoserStateStunned( void ) const
{
	if ( GetActiveStunInfo() )
	{
		if ( InCond( TF_COND_STUNNED ) && ( m_iStunFlags & TF_STUN_LOSER_STATE ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Indicates that our movement is slowed, but our controls are still free.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsSnared( void )
{
	if ( InCond( TF_COND_STUNNED ) && !IsControlStunned() )
		return true;
	else
		return false;
}

//=============================================================================
//
// Shared player code that isn't CTFPlayerShared
//
//-----------------------------------------------------------------------------
struct penetrated_target_list
{
	CBaseEntity *pTarget;
	float flDistanceFraction;
};

//-----------------------------------------------------------------------------
class CBulletPenetrateEnum : public IEntityEnumerator
{
public:
	CBulletPenetrateEnum( Vector vecStart, Vector vecEnd, CBaseEntity *pShooter, int nCustomDamageType, bool bIgnoreTeammates = true )
	{
		m_vecStart = vecStart;
		m_vecEnd = vecEnd;
		m_pShooter = pShooter;
		m_nCustomDamageType = nCustomDamageType;
		m_bIgnoreTeammates = bIgnoreTeammates;
	}

	// We need to sort the penetrated targets into order, with the closest target first
	class PenetratedTargetLess
	{
	public:
		bool Less( const penetrated_target_list &src1, const penetrated_target_list &src2, void *pCtx )
		{
			return src1.flDistanceFraction < src2.flDistanceFraction;
		}
	};

	virtual bool EnumEntity( IHandleEntity *pHandleEntity )
	{
		trace_t tr;

		CBaseEntity *pEnt = static_cast<CBaseEntity*>(pHandleEntity);

		// Ignore collisions with the shooter
		if ( pEnt == m_pShooter )
			return true;

		if ( pEnt->IsCombatCharacter() || pEnt->IsBaseObject() )
		{
			if ( m_bIgnoreTeammates && pEnt->GetTeam() == m_pShooter->GetTeam() )
				return true;

			Ray_t ray;
			ray.Init( m_vecStart, m_vecEnd );
			enginetrace->ClipRayToEntity( ray, MASK_SOLID | CONTENTS_HITBOX, pHandleEntity, &tr );

			if (tr.fraction < 1.0f)
			{
				penetrated_target_list newEntry;
				newEntry.pTarget = pEnt;
				newEntry.flDistanceFraction = tr.fraction;
				m_Targets.Insert( newEntry );
				return true;
			}
		}

		return true;
	}

public:
	Vector		 m_vecStart;
	Vector		 m_vecEnd;
	int			 m_nCustomDamageType;
	CBaseEntity	*m_pShooter;
	bool		 m_bIgnoreTeammates;
	CUtlSortVector<penetrated_target_list, PenetratedTargetLess> m_Targets;
};


CTargetOnlyFilter::CTargetOnlyFilter( CBaseEntity *pShooter, CBaseEntity *pTarget ) 
	: CTraceFilterSimple( pShooter, COLLISION_GROUP_NONE )
{
	m_pShooter = pShooter;
	m_pTarget = pTarget;
}

bool CTargetOnlyFilter::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEnt = static_cast<CBaseEntity*>(pHandleEntity);

	if ( pEnt && pEnt == m_pTarget ) 
		return true;
	else if ( !pEnt || pEnt != m_pTarget )
	{
		// If we hit a solid piece of the world, we're done.
		if ( pEnt->IsBSPModel() && pEnt->IsSolid() )
			return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
		return false;
	}
	else
		return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Purpose:
//   Input: info
//          bDoEffects - effects (blood, etc.) should only happen client-side.
//-----------------------------------------------------------------------------
void CTFPlayer::MaybeDrawRailgunBeam( IRecipientFilter *pFilter, CTFWeaponBase *pWeapon, const Vector& vStartPos, const Vector& vEndPos )
{
#ifdef GAME_DLL
	Assert( pFilter );
#else // !GAME_DLL
	Assert( !pFilter );
#endif
	Assert( pWeapon );

	int iShouldFireTracer = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iShouldFireTracer, sniper_fires_tracer );

	if ( !iShouldFireTracer )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iShouldFireTracer, sniper_fires_tracer_HIDDEN );
	}

	// Check for heatmaker
	if ( !iShouldFireTracer )
	{
		iShouldFireTracer = m_Shared.InCond( TF_COND_SNIPERCHARGE_RAGE_BUFF ) && pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() );
	}

	if ( iShouldFireTracer )
	{
		const char *pParticleSystemName = pWeapon->GetTeamNumber() == TF_TEAM_BLUE ? "dxhr_sniper_rail_blue" : "dxhr_sniper_rail_red";
		CTFSniperRifle *pRifle = dynamic_cast< CTFSniperRifle* >( pWeapon );
		if ( pRifle && ( pRifle->GetRifleType() == RIFLE_CLASSIC ) )
		{
			pParticleSystemName = "tfc_sniper_distortion_trail";
		}

#ifdef GAME_DLL
		te_tf_particle_effects_control_point_t controlPoint = { PATTACH_WORLDORIGIN, vEndPos };

		TE_TFParticleEffectComplex( *pFilter, 0.0f, pParticleSystemName, vStartPos, QAngle( 0, 0, 0 ), NULL, &controlPoint, pWeapon, PATTACH_CUSTOMORIGIN );
#else // !GAME_DLL
		CSmartPtr<CNewParticleEffect> pEffect = pWeapon->ParticleProp()->Create( pParticleSystemName, PATTACH_CUSTOMORIGIN, 0 );
		if ( pEffect.IsValid() && pEffect->IsValid() )
		{
			pEffect->SetSortOrigin( vStartPos );
			pEffect->SetControlPoint( 0, vStartPos );
			pEffect->SetControlPoint( 1, vEndPos );
		}
#endif // GAME_DLL
	}
}

void CTFPlayer::GetHorriblyHackedRailgunPosition( const Vector& vStart, Vector *out_pvStartPos )
{
	Assert( out_pvStartPos != NULL );

	// DO NOT LOOK BEHIND THE MAGIC CURTAIN
	Vector vForward, vRight, vUp;
	AngleVectors( EyeAngles(), &vForward, &vRight, &vUp );

#ifdef CLIENT_DLL
	// Flips the horizontal position.
	if ( TeamFortress_ShouldFlipClientViewModel() )
	{
		vRight *= -1;
	}
#endif // CLIENT_DLL

	*out_pvStartPos = vStart
					+ (vForward * 60.9f)
					+ (vRight * 13.1f)
					+ (vUp * -15.1f);
}

static bool OnOpposingTFTeams( int iTeam0, int iTeam1 )
{
	// This logic is weird because we want to make sure that we're actually shooting someone on the
	// other team, not just someone on a different team. This prevents weirdness where we count shooting
	// the BSP as an enemy because they aren't on our team.

	if ( iTeam0 == TF_TEAM_BLUE	)				// if we're on the blue team...
		return iTeam1 == TF_TEAM_RED;			// ...and we shot someone on the red team, then we're opposing.

	if ( iTeam0 == TF_TEAM_RED	)				// if we're on the blue team...
		return iTeam1 == TF_TEAM_BLUE;			// ...and we shot someone on the red team, then we're opposing.

	return iTeam0 != iTeam1;					// if we're neither red nor blue, then anyone different from us is opposing
}

#ifdef GAME_DLL
extern void ExtinguishPlayer( CEconEntity *pExtinguisher, CTFPlayer *pOwner, CTFPlayer *pTarget, const char *pExtinguisherName );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ModifyDamageInfo( CTakeDamageInfo *pInfo, const CBaseEntity *pTarget )
{
	if ( pInfo && pTarget )
	{
		// Increased damage vs sentry's target?
		if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			float flDamageMod = 1.f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetActiveWeapon(), flDamageMod, mult_dmg_bullet_vs_sentry_target );
			if ( flDamageMod > 1.f )
			{
				CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun*>( GetObjectOfType( OBJ_SENTRYGUN ) );
				if ( pSentry && ( pSentry->GetTarget() == pTarget ) )
				{
					pInfo->SetDamage( pInfo->GetDamage() * flDamageMod );
				}
			}
		}
	}
}
#endif

// Josh: Go on, pick a better name!
class CTraceFilterIgnoreAllPlayersExceptThisOne : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreAllPlayersExceptThisOne, CTraceFilterSimple );

	CTraceFilterIgnoreAllPlayersExceptThisOne( const IHandleEntity* passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity* pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		const CBaseEntity *pPassEntity = EntityFromEntityHandle( GetPassEntity() );

		if ( pEntity->IsPlayer() )
			return pEntity == pPassEntity;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

// Josh:
// This function exists to handle hitting hitboxes outside of the player's
// bbox in a way that works for TF2.
// 
// One of the major problems it that if a player gets up against a thin wall or
// door, you can shoot them through it if their hitbox is outside of their bbox
//
// This fixes that problem in a simple way by tracing from the hitbox hit point
// to the player's origin at the hitbox hit height and checking if it hit the
// player or not.
static void UTIL_PlayerBulletTrace( const Vector& vecStart, const Vector& vecEnd, const Vector &vecDir, unsigned int mask, ITraceFilter* pFilter, trace_t* trace )
{
	UTIL_TraceLine( vecStart, vecEnd, mask | CONTENTS_HITBOX, pFilter, trace );

	if ( !trace->startsolid && !trace->DidHitNonWorldEntity() )
	{
		// Josh: Extend the ray length by ~ size of the bbox if we are clipping
		// to the player to keep the ray length consistent with the normal bbox path.
		const float rayExtension = 40.0f;

		trace_t playerClipTrace;
		memcpy( &playerClipTrace, trace, sizeof( trace_t ) );

		UTIL_ClipTraceToPlayers( vecStart, vecEnd + vecDir * rayExtension, mask | CONTENTS_HITBOX, pFilter, &playerClipTrace );
		if ( playerClipTrace.m_pEnt )
		{
			Vector entOrigin = playerClipTrace.m_pEnt->GetAbsOrigin();

			CTraceFilterIgnoreAllPlayersExceptThisOne playerClipValidationFilter( playerClipTrace.m_pEnt, COLLISION_GROUP_NONE );
			// Josh:
			// Trace from the hitbox to the player origin at hitbox hit height to see if they are obstructed by the world.
			// Don't use hitboxes on this trace.
			//
			// It's unfair if a player is poking through a thin wall and gets shot! :(
			trace_t playerClipValidationTrace;
			UTIL_TraceLine( playerClipTrace.endpos, Vector( entOrigin.x, entOrigin.y, playerClipTrace.endpos.z ), mask, &playerClipValidationFilter, &playerClipValidationTrace );

			if ( playerClipValidationTrace.m_pEnt == playerClipTrace.m_pEnt )
				memcpy( trace, &playerClipTrace, sizeof( trace_t ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FireBullet( CTFWeaponBase *pWpn, const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType /*= TF_DMG_CUSTOM_NONE*/ )
{
	// Fire a bullet (ignoring the shooter).
	Vector vecStart = info.m_vecSrc;
	Vector vecEnd = vecStart + info.m_vecDirShooting * info.m_flDistance;
	trace_t trace;

	ETFDmgCustom ePenetrateType = pWpn ? pWpn->GetPenetrateType() : TF_DMG_CUSTOM_NONE;
	if ( ePenetrateType == TF_DMG_CUSTOM_NONE )
	{
		ePenetrateType = (ETFDmgCustom)nCustomDamageType;
	}

	// Ignore teammates and their (physical) upgrade items when shooting in MvM
	if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
	{
		CTraceFilterIgnoreFriendlyCombatItems traceFilter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		UTIL_PlayerBulletTrace( vecStart, vecEnd, info.m_vecDirShooting, MASK_SOLID, &traceFilter, &trace );
	}
	else
	{
		CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
		UTIL_PlayerBulletTrace( vecStart, vecEnd, info.m_vecDirShooting, MASK_SOLID, &traceFilter, &trace );
	}

#ifndef CLIENT_DLL
	CUtlVector<CBaseEntity *> vecTracedEntities;
	bool bPenetratingShot = ( (ePenetrateType == TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS) || (ePenetrateType == TF_DMG_CUSTOM_PENETRATE_MY_TEAM) || (ePenetrateType == TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE) );
	if ( bPenetratingShot && trace.m_pEnt )
	{
		if ( trace.m_pEnt->IsCombatCharacter() || trace.m_pEnt->IsBaseObject() )
		{
			const float penetrationHullExtension = 40.0f;
			// Josh: EnumerateEntities only collides with bboxes, extend the ray to a larger hull, then we clip to it.
			Ray_t ray;
			ray.Init( vecStart, vecEnd,
				Vector( -penetrationHullExtension, -penetrationHullExtension, -penetrationHullExtension ),
				Vector(  penetrationHullExtension,  penetrationHullExtension,  penetrationHullExtension ) );

			// Penetrating shot: Strikes everything along the bullet's path.
			CBulletPenetrateEnum bulletpenetrate( vecStart, vecEnd, this, ePenetrateType, ePenetrateType == TF_DMG_CUSTOM_PENETRATE_MY_TEAM );
			enginetrace->EnumerateEntities( ray, false, &bulletpenetrate );
			
			FOR_EACH_VEC( bulletpenetrate.m_Targets, i )
			{
				vecTracedEntities.AddToTail( bulletpenetrate.m_Targets[i].pTarget );
			}
		}
		else
		{
			// We traced into something we don't understand (sticky bomb? pumpkin bomb?) -- just apply our
			// hit logic to whatever we traced first.
			vecTracedEntities.AddToTail( trace.m_pEnt );
		}
	}
	else
#endif
	{
		ePenetrateType = TF_DMG_CUSTOM_NONE;
	}

#ifndef CLIENT_DLL
	CTakeDamageInfo dmgInfo( this, info.m_pAttacker, info.m_flDamage, nDamageType );
	dmgInfo.SetWeapon( GetActiveWeapon() );
	dmgInfo.SetDamageCustom( nCustomDamageType );

	int iPenetratedPlayerCount = 0;

	int iEnemyPlayersHit = 0;
	if ( bPenetratingShot )
	{
		int iChargedPenetration = 0;
		CALL_ATTRIB_HOOK_INT( iChargedPenetration, sniper_penetrate_players_when_charged );
		int iPenetrationLimit = 0;
		CALL_ATTRIB_HOOK_INT( iPenetrationLimit, projectile_penetration );

		// Damage every enemy player struck by the bullet along its path.
		trace_t pen_trace;
		FOR_EACH_VEC( vecTracedEntities, i )
		{
			// Limit the number of pen targets in MvM if we're not charge-based
			if ( TFGameRules()->IsMannVsMachineMode() && iChargedPenetration == 0 )
			{
				// For sniper class, treat iPenetrationLimit as a bool
				bool bIsSniper = IsPlayerClass( TF_CLASS_SNIPER );
				if ( bIsSniper && iPenetrationLimit == 0 && iPenetratedPlayerCount > 0 )
					break;

				if ( !bIsSniper && iPenetratedPlayerCount > iPenetrationLimit )
					break;
			}

			CBaseEntity *pTarget = vecTracedEntities[i];

			if ( !pTarget )
				continue;

			trace_t *pTraceToUse = &pen_trace;
			
			if ( ePenetrateType == TF_DMG_CUSTOM_PENETRATE_MY_TEAM )
			{
				// Skip friendlies if we're looking for the first enemy
				if ( GetTeamNumber() == pTarget->GetTeamNumber() )
					continue;
				
				pTraceToUse = &trace;
			}
			else if ( ePenetrateType == TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE )
			{
				if ( GetTeamNumber() == pTarget->GetTeamNumber() )
				{
					if ( pTarget->IsPlayer() )
					{
						// skip friendlies that are not on burning
						CTFPlayer *pTeammate = ToTFPlayer( pTarget );
						if ( !pTeammate->m_Shared.InCond( TF_COND_BURNING ) )
							continue;
					}
				}

				pTraceToUse = &trace;
			}

			CTargetOnlyFilter penetrateFilter( this, pTarget );
			UTIL_PlayerBulletTrace( vecStart, vecEnd, info.m_vecDirShooting, MASK_SOLID, &penetrateFilter, pTraceToUse );

			if ( pTraceToUse->m_pEnt == pTarget )
			{
				CTFPlayer *pTargetPlayer = NULL;
				if ( pTarget->IsPlayer() )
					pTargetPlayer = ToTFPlayer( pTarget );

				// put out fire for burning teammate
				if ( nCustomDamageType == TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE )
				{
					if ( pTargetPlayer && GetTeamNumber() == pTargetPlayer->GetTeamNumber() && pTargetPlayer->m_Shared.InCond( TF_COND_BURNING ) )
					{
						ExtinguishPlayer( GetActiveWeapon(), ToTFPlayer( GetActiveWeapon()->GetOwner() ), pTargetPlayer, GetActiveWeapon()->GetName() );
					}
				}

				ModifyDamageInfo( &dmgInfo, pTarget );
				CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, info.m_vecDirShooting, pTraceToUse->endpos, 1.0 );
				dmgInfo.SetPlayerPenetrationCount( iPenetratedPlayerCount );
				pTarget->DispatchTraceAttack( dmgInfo, info.m_vecDirShooting, pTraceToUse, GetActiveWeapon() ? GetActiveWeapon()->GetDmgAccumulator() : NULL );

				const bool bIsPenetratingPlayer = pTargetPlayer != NULL;
				if ( bIsPenetratingPlayer )
				{
					iPenetratedPlayerCount++;
					float flPenetrationPenalty = 1.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flPenetrationPenalty, penetration_damage_penalty );
					dmgInfo.SetDamage( dmgInfo.GetDamage() * flPenetrationPenalty );
				}

				// If we're only supposed to penetrate players and this thing isn't a player, stop here.
				if ( !bIsPenetratingPlayer && (ePenetrateType == TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS) )
					break;
			}
			else
			{
				// We hit something solid that said we should stop tracing.
				break;
			}

			if( pTarget->IsPlayer() && OnOpposingTFTeams( GetTeamNumber(), pTarget->GetTeamNumber() ) )
			{
				iEnemyPlayersHit++;
			}

			// If we're penetrating team mates, but we've just hit an enemy, we're done.
			if ( ePenetrateType == TF_DMG_CUSTOM_PENETRATE_MY_TEAM )
				break;

			// just hit an enemy or a burning teammate
			if ( ePenetrateType == TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE )
				break;
		}
	}
	else
	{
		// Damage only the first entity encountered on the bullet's path.
		if ( trace.m_pEnt )
		{
			ModifyDamageInfo( &dmgInfo, trace.m_pEnt );
			CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, info.m_vecDirShooting, trace.endpos, 1.0 );
			trace.m_pEnt->DispatchTraceAttack( dmgInfo, info.m_vecDirShooting, &trace );
			if ( trace.m_pEnt->IsPlayer() && OnOpposingTFTeams( GetTeamNumber(), trace.m_pEnt->GetTeamNumber() ) )
			{
				iEnemyPlayersHit++;
			}
		}
	}
	if ( pWpn )
	{
		pWpn->OnBulletFire( iEnemyPlayersHit );

		if ( iEnemyPlayersHit )
		{	// Guarantee that the bullet that hit an enemy trumps the player viewangles
			// that are locked in for the duration of the server simulation ticks
			m_iLockViewanglesTickNumber = gpGlobals->tickcount;
			m_qangLockViewangles = pl.v_angle;
		}
	}
#endif

#ifdef GAME_DLL
#ifdef _DEBUG
	if ( tf_debug_bullets.GetBool() )
	{
		NDebugOverlay::Line( vecStart, trace.endpos, 0,255,0, true, 30 );
	}
#endif // _DEBUG
#endif

	if ( trace.fraction < 1.0 )
	{
		// Verify we have an entity at the point of impact.
		Assert( trace.m_pEnt );

#ifdef GAME_DLL
		// We intentionally do this logic here outside our client-side "should we do effects?" logic. We send this
		// to everyone except our local owner (ourself) as we'll do our own fire effects below.
		Vector vMuzzleOrigin;
		if ( pWpn )
		{
			Vector vStartPos;
			GetHorriblyHackedRailgunPosition( trace.startpos, &vStartPos );

			CBroadcastNonOwnerRecipientFilter filter( this );
			MaybeDrawRailgunBeam( &filter, pWpn, vStartPos, trace.endpos );
		}
#endif // GAME_DLL

		if ( bDoEffects )
		{
			// If shot starts out of water and ends in water
			if ( !( enginetrace->GetPointContents( trace.startpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) &&
				( enginetrace->GetPointContents( trace.endpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) )
			{	
				// Water impact effects.
				ImpactWaterTrace( trace, vecStart );
			}
			else
			{
				// Regular impact effects.

				// don't decal your teammates or objects on your team
				if ( trace.m_pEnt && trace.m_pEnt->GetTeamNumber() != GetTeamNumber() )
				{
					UTIL_ImpactTrace( &trace, nDamageType );
				}
			}

#ifdef CLIENT_DLL
			if ( pWpn )
			{
				Vector vStartPos;
				GetHorriblyHackedRailgunPosition( trace.startpos, &vStartPos );

				MaybeDrawRailgunBeam( NULL, pWpn, vStartPos, trace.endpos );
			}

			static int	tracerCount;
			if ( ( ( info.m_iTracerFreq != 0 ) && ( tracerCount++ % info.m_iTracerFreq ) == 0 ) || (ePenetrateType == TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS) )
			{
				// if this is a local player, start at attachment on view model
				// else start on attachment on weapon model
				int iUseAttachment = TRACER_DONT_USE_ATTACHMENT;
				int iAttachment = 1;

				{
					C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

					if ( pWeapon )
					{
						iAttachment = pWeapon->LookupAttachment( "muzzle" );
					}
				}


				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

				bool bInToolRecordingMode = clienttools->IsInRecordingMode();

				// If we're using a viewmodel, override vecStart with the muzzle of that - just for the visual effect, not gameplay.
				if ( ( pLocalPlayer != NULL ) && !pLocalPlayer->ShouldDrawThisPlayer() && !bInToolRecordingMode && pWpn )
				{
					C_BaseAnimating *pAttachEnt = pWpn->GetAppropriateWorldOrViewModel();
					if ( pAttachEnt != NULL )
					{
						pAttachEnt->GetAttachment( iAttachment, vecStart );
					}
				}
				else if ( !IsDormant() )
				{
					// fill in with third person weapon model index
					C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

					if( pWeapon )
					{
						int nModelIndex = pWeapon->GetModelIndex();
						int nWorldModelIndex = pWeapon->GetWorldModelIndex();
						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nWorldModelIndex );
						}

						pWeapon->GetAttachment( iAttachment, vecStart );

						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nModelIndex );
						}
					}
				}

				if ( tf_useparticletracers.GetBool() )
				{
					const char *pszTracerEffect = GetTracerType();
					if ( pszTracerEffect && pszTracerEffect[0] )
					{
						char szTracerEffect[128];
						if ( nDamageType & DMG_CRITICAL )
						{
							Q_snprintf( szTracerEffect, sizeof(szTracerEffect), "%s_crit", pszTracerEffect );
							pszTracerEffect = szTracerEffect;
						}

						UTIL_ParticleTracer( pszTracerEffect, vecStart, trace.endpos, entindex(), iUseAttachment, true );
					}
				}
				else
				{
					UTIL_Tracer( vecStart, trace.endpos, entindex(), iUseAttachment, 5000, true, GetTracerType() );
				}
			}
#endif
		}
	}
}

#ifdef CLIENT_DLL
static ConVar tf_impactwatertimeenable( "tf_impactwatertimeenable", "0", FCVAR_CHEAT, "Draw impact debris effects." );
static ConVar tf_impactwatertime( "tf_impactwatertime", "1.0f", FCVAR_CHEAT, "Draw impact debris effects." );
#endif

//-----------------------------------------------------------------------------
// Purpose: Trace from the shooter to the point of impact (another player,
//          world, etc.), but this time take into account water/slime surfaces.
//   Input: trace - initial trace from player to point of impact
//          vecStart - starting point of the trace 
//-----------------------------------------------------------------------------
void CTFPlayer::ImpactWaterTrace( trace_t &trace, const Vector &vecStart )
{
#ifdef CLIENT_DLL
	if ( tf_impactwatertimeenable.GetBool() )
	{
		if ( m_flWaterImpactTime > gpGlobals->curtime )
			return;
	}
#endif 

	trace_t traceWater;
	UTIL_TraceLine( vecStart, trace.endpos, ( MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME ), 
		this, COLLISION_GROUP_NONE, &traceWater );
	if( traceWater.fraction < 1.0f )
	{
		CEffectData	data;
		data.m_vOrigin = traceWater.endpos;
		data.m_vNormal = traceWater.plane.normal;
		data.m_flScale = random->RandomFloat( 8, 12 );
		if ( traceWater.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		const char *pszEffectName = "tf_gunshotsplash";
		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon && ( TF_WEAPON_MINIGUN == pWeapon->GetWeaponID() ) )
		{
			// for the minigun, use a different, cheaper splash effect because it can create so many of them
			pszEffectName = "tf_gunshotsplash_minigun";
		}		
		DispatchEffect( pszEffectName, data );

#ifdef CLIENT_DLL
		if ( tf_impactwatertimeenable.GetBool() )
		{
			m_flWaterImpactTime = gpGlobals->curtime + tf_impactwatertime.GetFloat();
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayer::GetActiveTFWeapon( void ) const
{
	CBaseCombatWeapon *pRet = GetActiveWeapon();
	if ( pRet )
	{
		Assert( dynamic_cast< CTFWeaponBase* >( pRet ) != NULL );
		return static_cast< CTFWeaponBase * >( pRet );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetPassiveWeapons( CUtlVector<CTFWeaponBase*>& vecOut )
{
	Assert( vecOut.IsEmpty() );

	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase * )GetWeapon( i );
		if ( !pWpn )
			continue;

		if ( !pWpn->IsPassiveWeapon() )
			continue;

		vecOut.AddToTail( pWpn );
	}

	return vecOut.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we are currently wielding a weapon that
// matches the given item def handle.
//-----------------------------------------------------------------------------
bool CTFPlayer::IsActiveTFWeapon( CEconItemDefinition *weaponHandle ) const
{
	return ( GetActiveTFWeapon() &&
			 GetActiveTFWeapon()->GetAttributeContainer() &&
			 GetActiveTFWeapon()->GetAttributeContainer()->GetItem() &&
			 GetActiveTFWeapon()->GetAttributeContainer()->GetItem()->GetItemDefinition() == weaponHandle );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we are currently wielding a weapon that
// matches the given item def handle.
bool CTFPlayer::IsActiveTFWeapon( const CSchemaItemDefHandle &weaponHandle ) const
{
	return ( GetActiveTFWeapon() &&
			 GetActiveTFWeapon()->GetAttributeContainer() &&
			 GetActiveTFWeapon()->GetAttributeContainer()->GetItem() &&
			 GetActiveTFWeapon()->GetAttributeContainer()->GetItem()->GetItemDefinition() == weaponHandle );
}

//-----------------------------------------------------------------------------
// Purpose: How much build resource ( metal ) does this player have
//-----------------------------------------------------------------------------
int CTFPlayer::GetBuildResources( void )
{
	return GetAmmoCount( TF_AMMO_METAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template < typename T >
class CScopedFlag
{
public:
	CScopedFlag( T& ref_ )
		: ref( ref_ )
	{
		Assert( !ref );
		ref = true;
	}

	~CScopedFlag()
	{
		Assert( ref );
		ref = false;
	}

private:
	T& ref;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetMovementForwardPull( void ) const
{
	CTFWeaponBase *pWpn = GetActiveTFWeapon();
	if ( pWpn && pWpn->IsFiring() )
	{
		float flFiringForwardPull = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flFiringForwardPull, firing_forward_pull );

		return flFiringForwardPull;
	}

	return 0.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanPlayerMove() const
{
	if ( GetMoveType() == MOVETYPE_OBSERVER )
		return true;

	if ( GetMoveType() == MOVETYPE_NOCLIP )
		return true;

	// No one can move when in a final countdown transition.
	if ( TFGameRules() && TFGameRules()->BInMatchStartCountdown() )
		return false;

	if ( IsViewingCYOAPDA() )
		return false;

	bool bFreezeOnRestart = tf_player_movement_restart_freeze.GetBool();
	if ( bFreezeOnRestart )
	{
#if defined( _DEBUG ) || defined( STAGING_ONLY )
		if ( mp_developer.GetBool() )
			bFreezeOnRestart = false;
#endif // _DEBUG || STAGING_ONLY

	if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() && ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS ) )
		bFreezeOnRestart = false;
	}

	bool bInRoundRestart = TFGameRules() && TFGameRules()->InRoundRestart();
	if ( bInRoundRestart && TFGameRules()->IsCompetitiveMode() )
	{
		if ( TFGameRules()->GetRoundsPlayed() > 0 )
		{
			if ( gpGlobals->curtime < TFGameRules()->GetPreroundCountdownTime() )
			{
				bFreezeOnRestart = true;
			}
		}
		else
		{
			bFreezeOnRestart = false;
		}
	}

	bool bNoMovement = bInRoundRestart && bFreezeOnRestart;

	return !bNoMovement;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::TeamFortress_CalculateMaxSpeed( bool bIgnoreSpecialAbility /*= false*/ ) const
{
	if ( !GameRules() )
		return 0.0f;

	int playerclass = GetPlayerClass()->GetClassIndex();

	// Spectators can move while in Classic Observer mode
	if ( IsObserver() )
	{
		if ( GetObserverMode() == OBS_MODE_ROAMING )
			return GetPlayerClassData( TF_CLASS_SCOUT )->m_flMaxSpeed;

		return 0.0f;
	}

	// Check for any reason why they can't move at all
	if ( playerclass == TF_CLASS_UNDEFINED || !CanPlayerMove() )
		return 1.0f;					// this can't return 0 because other parts of the code interpret that as "use default speed" during setup

	// First, get their max class speed
	float default_speed = GetPlayerClassData( playerclass )->m_flMaxSpeed;

	// Avoid re-entering and calculating our velocity while we're calculating our velocity.
	// This can happen if we have two characters trying to match each other's velocity, for
	// example if you have two medics with Quick-Fixes healing each other.
	//
	// In the case where we run into this, we end the recursion with someone running default
	// speed.
	if ( m_bIsCalculatingMaximumSpeed )
		return default_speed;

	CScopedFlag<char> flagAvoidReentrancy( m_bIsCalculatingMaximumSpeed );

	// Slow us down if we're disguised as a slower class
	// unless we're cloaked..
	float maxfbspeed = default_speed;

	bool bAllowSlowing = m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) ? false : true;

	if ( m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) && !m_Shared.IsStealthed() )
	{
		maxfbspeed = 0.0f;
	}
	else if ( m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.IsStealthed() )
	{
		float flMaxDisguiseSpeed = GetPlayerClassData( m_Shared.GetDisguiseClass() )->m_flMaxSpeed;
		maxfbspeed = MIN( flMaxDisguiseSpeed, maxfbspeed );
	}

	if ( !TFGameRules()->IsMannVsMachineMode() || !IsMiniBoss() ) // No aiming slowdown penalties for MiniBoss players in MVM
	{
		// if they're a sniper, and they're aiming, their speed must be 80 or less
		if ( m_Shared.InCond( TF_COND_AIMING ) )
		{
			float flAimMax = 0;

			// Heavies are allowed to move slightly faster than a sniper when spun-up
			if ( playerclass == TF_CLASS_HEAVYWEAPONS )
			{
				{
					flAimMax = 110;
				}
			}
			else
			{
				if ( GetActiveTFWeapon() && (GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_COMPOUND_BOW) )
				{
					flAimMax = 160;
				}
				else
				{
					flAimMax = 80;
				}
			}
			
			CALL_ATTRIB_HOOK_FLOAT( flAimMax, mult_player_aiming_movespeed );
			maxfbspeed = MIN( maxfbspeed, flAimMax );
		}
	}

#ifdef GAME_DLL
	if ( m_Shared.InCond( TF_COND_SPEED_BOOST ) )
	{
		// We only allow our speed boost to apply if we have a base speed to work with. If we're supposed
		// to be stationary for whatever reason we don't allow a speed to allow us to move.
		if ( maxfbspeed > 0.0f )
		{
			maxfbspeed += MIN( maxfbspeed * 0.4f, tf_whip_speed_increase.GetFloat() );
		}
	}
#endif

	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if (maxfbspeed > tf_spy_max_cloaked_speed.GetFloat() )
		{
			maxfbspeed = tf_spy_max_cloaked_speed.GetFloat();
		}
	}

	// if we're in bonus time because a team has won, give the winners 110% speed and the losers 90% speed
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		
		if ( iWinner != TEAM_UNASSIGNED )
		{
			if ( iWinner == GetTeamNumber() )
			{
				maxfbspeed *= 1.1f;
			}
			else
			{
				maxfbspeed *= 0.9f;
			}
		}
	}

	CTFWeaponBase* pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		maxfbspeed *= pWeapon->GetSpeedMod();
	}

	if ( playerclass == TF_CLASS_DEMOMAN )
	{
		CTFSword *pSword = dynamic_cast<CTFSword*>(Weapon_OwnsThisID( TF_WEAPON_SWORD ));
		if ( pSword )
		{
			maxfbspeed *= pSword->GetSwordSpeedMod();
		}

		if ( !bIgnoreSpecialAbility && m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			maxfbspeed = tf_max_charge_speed.GetFloat();
		}
	}

	bool bCarryPenalty = true;

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		bCarryPenalty = false;
	}

	if ( m_Shared.IsCarryingObject() && bCarryPenalty && bAllowSlowing )
	{
		// STAGING_ENGY
		maxfbspeed *= 0.90f;
	}

	if ( m_Shared.IsLoserStateStunned() && bAllowSlowing )
	{
		// Yikes is not as slow, terrible gotcha
		if ( m_Shared.GetActiveStunInfo()->iStunFlags & TF_STUN_BY_TRIGGER ) 
		{
			maxfbspeed *= 0.75f;
		}
		else
		{
			maxfbspeed *= 0.5f;
		}
	}

	// If we have an item with a move speed modification, apply it to the final speed.
	CALL_ATTRIB_HOOK_FLOAT( maxfbspeed, mult_player_movespeed );

	if ( m_Shared.IsShieldEquipped() )
	{
		CALL_ATTRIB_HOOK_FLOAT( maxfbspeed, mult_player_movespeed_shieldrequired );
	}

	if ( playerclass == TF_CLASS_MEDIC )
	{
		if ( pWeapon )
		{
			CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pWeapon );
			if ( pMedigun )
			{
				// Medics match faster classes when healing them
				CTFPlayer *pHealTarget = ToTFPlayer( pMedigun->GetHealTarget() );
				if ( pHealTarget )
				{
					// The Quick-Fix attaches to charging demos
					bool bCharge = ( pMedigun->GetMedigunType() == MEDIGUN_QUICKFIX && pHealTarget->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) );

					const float flHealTargetMaxSpeed = ( bCharge ) ? tf_max_charge_speed.GetFloat() : pHealTarget->TeamFortress_CalculateMaxSpeed( true );
					maxfbspeed = Max( maxfbspeed, flHealTargetMaxSpeed );
				}
			}
		}

		// Special bone saw
		int iTakeHeads = 0;
		CALL_ATTRIB_HOOK_INT( iTakeHeads, add_head_on_hit );
		if ( iTakeHeads )
		{
			CTFBonesaw *pSaw = dynamic_cast<CTFBonesaw*>(Weapon_OwnsThisID( TF_WEAPON_HARVESTER_SAW ));
			if ( pSaw )
			{
				maxfbspeed *= pSaw->GetBoneSawSpeedMod();
			}
		}
	}

	float flClassResourceLevelMod = 1.f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flClassResourceLevelMod, mult_player_movespeed_resource_level );
	if ( flClassResourceLevelMod != 1.f )
	{
		// Medic Uber
		if ( playerclass == TF_CLASS_MEDIC )
		{
			CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
			if ( pMedigun )
			{
				maxfbspeed *= RemapValClamped( pMedigun->GetChargeLevel(), 0.f, 1.f, 1.f, flClassResourceLevelMod );
			}
		}
	}

	float flItemSpeedMod = 1.f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flItemSpeedMod, mult_player_movespeed_active );
	if ( flItemSpeedMod != 1.f )
	{
		maxfbspeed *= flItemSpeedMod;
	}

	// If we're a heavy with berzerker mode...
	if ( playerclass == TF_CLASS_HEAVYWEAPONS )
	{
		float heavy_max_speed = default_speed * 1.35f;
		if ( m_Shared.InCond( TF_COND_ENERGY_BUFF ) )
		{
			maxfbspeed *= 1.3f;
			if ( maxfbspeed > heavy_max_speed )
			{
				// Prevent other speed modifiers like GRU from making berzerker mode too fast.
				maxfbspeed = heavy_max_speed;
			}
		}
	}

	if ( playerclass == TF_CLASS_SCOUT )
	{
		if ( Weapon_OwnsThisID( TF_WEAPON_PEP_BRAWLER_BLASTER ) )
		{
			// Make this change based on attrs, hardcode right now
			maxfbspeed *= RemapValClamped( m_Shared.GetScoutHypeMeter(), 0.0f, 100.0f, 1.0f, 1.45f );
		}
		// Atomic Punch gives a move bonus while active
// 		if ( m_Shared.InCond( TF_COND_PHASE ) )
// 		{
// 			maxfbspeed *= 1.25f;
// 		}
	}

	// Mann Vs Machine mode has a speed penalty for carrying the flag
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			if ( HasTheFlag() && !IsMiniBoss() )
			{
				maxfbspeed *= tf_mvm_bot_flag_carrier_movement_penalty.GetFloat();
			}
		}
	}

	
	if ( m_Shared.GetCarryingRuneType() == RUNE_HASTE )
	{
		maxfbspeed *= 1.3f;
	}
	if ( m_Shared.GetCarryingRuneType() == RUNE_AGILITY )
	{
		// light classes get more benefit due to movement speed cap of 520 
		switch ( GetPlayerClass()->GetClassIndex() )
		{
		case TF_CLASS_DEMOMAN:
		case TF_CLASS_SOLDIER:
		case TF_CLASS_HEAVYWEAPONS:
			maxfbspeed *= 1.4f;
			break;
		default:
			maxfbspeed *= 1.5f;
			break;
		}
	}

	return maxfbspeed;
}

void CTFPlayer::TeamFortress_SetSpeed()
{
#ifdef GAME_DLL
	if ( TFGameRules() && TFGameRules()->IsPasstimeMode() && g_pPasstimeLogic )
	{
		float flPackSpeed = g_pPasstimeLogic->GetPackSpeed( this );
		if ( flPackSpeed > 0 ) 
		{
			SetMaxSpeed( flPackSpeed );
			return;
		}
	}
#endif

	const float fMaxSpeed = TeamFortress_CalculateMaxSpeed();

	// Set the speed
	SetMaxSpeed( fMaxSpeed );

	if ( fMaxSpeed <= 0.0f )
	{
		SetAbsVelocity( vec3_origin );
	}

#ifdef GAME_DLL
	// Anyone that's watching our speed should know that our speed changed so they can
	// update their own speed.
	//
	// We guard against re-entrancy here as well to avoid the case where two medics are
	// healing each other with Quick-Fixes.
	// 
	// This can also happen when a quickfix medic is healing a player that gets a speed
	// boost. And it doesn't work because the recursive call will just return the healed
	// character's default speed instead of current speed.
	// TODO fix this. why not just set the medic's speed directly at this point?
	//
	if ( !m_bIsCalculatingMaximumSpeed )
	{
		CScopedFlag<char> flagAvoidReentrancy( m_bIsCalculatingMaximumSpeed );

		CUtlVector<CTFPlayer *> vecSpeedWatchers;
		m_Shared.GetSpeedWatchersList( &vecSpeedWatchers );
		FOR_EACH_VEC( vecSpeedWatchers, i )
		{
			vecSpeedWatchers[i]->TeamFortress_SetSpeed();
		}
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::HasItem( void ) const
{
	return ( m_hItem != NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::SetItem( CTFItem *pItem )
{
	m_hItem = pItem;

#ifndef CLIENT_DLL
	if ( pItem )
	{
		AddGlowEffect();
	}
	else
	{
		RemoveGlowEffect();
	}

	if ( pItem && pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		RemoveInvisibility();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItem	*CTFPlayer::GetItem( void ) const
{
	return m_hItem;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player carrying the flag?
//-----------------------------------------------------------------------------
bool CTFPlayer::HasTheFlag( ETFFlagType exceptionTypes[], int nNumExceptions ) const
{
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		CCaptureFlag* pFlag = static_cast< CCaptureFlag* >( GetItem() );

		for( int i=0; i < nNumExceptions; ++i )
		{
			if ( exceptionTypes[ i ] == pFlag->GetType() )
				return false;
		}

		return true;
	}

	return false;
}

bool CTFPlayer::IsAllowedToPickUpFlag( void ) const
{
	int iCannotPickUpIntelligence = 0;
	CALL_ATTRIB_HOOK_INT( iCannotPickUpIntelligence, cannot_pick_up_intelligence );
	if ( iCannotPickUpIntelligence )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureZone *CTFPlayer::GetCaptureZoneStandingOn( void )
{
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch && pTouch->IsSolidFlagSet( FSOLID_TRIGGER ) && pTouch->IsBSPModel() )
			{
				CCaptureZone *pAreaTrigger = dynamic_cast< CCaptureZone* >(pTouch);
				if ( pAreaTrigger )
				{
					return pAreaTrigger;
				}
			}
		}
	}

	return NULL;
}

CCaptureZone *CTFPlayer::GetClosestCaptureZone( void )
{
	CCaptureZone *pCaptureZone = NULL;
	float flClosestDistance = FLT_MAX;

	for ( int i=0; i<ICaptureZoneAutoList::AutoList().Count(); ++i )
	{
		CCaptureZone *pTempCaptureZone = static_cast< CCaptureZone* >( ICaptureZoneAutoList::AutoList()[i] );
		if ( !pTempCaptureZone->IsDisabled() && pTempCaptureZone->GetTeamNumber() == GetTeamNumber() )
		{
			float fCurrentDistance = GetAbsOrigin().DistTo( pTempCaptureZone->WorldSpaceCenter() );
			if ( flClosestDistance > fCurrentDistance )
			{
				pCaptureZone = pTempCaptureZone;
				flClosestDistance = fCurrentDistance;
			}
		}
	}

	return pCaptureZone;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player's allowed to build another one of the specified object
//-----------------------------------------------------------------------------
int CTFPlayer::CanBuild( int iObjectType, int iObjectMode )
{
	if ( iObjectType < 0 || iObjectType >= OBJ_LAST )
		return CB_UNKNOWN_OBJECT;

	const CObjectInfo *pInfo = GetObjectInfo( iObjectType );
	if ( pInfo && ((iObjectMode > pInfo->m_iNumAltModes) || (iObjectMode < 0)) )
		return CB_CANNOT_BUILD;

	// Does this type require a specific builder?
	bool bHasSubType = false;
	CTFWeaponBuilder *pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, iObjectType );
	if ( pBuilder )
	{
		bHasSubType = true;
	}

	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsTruceActive() && ( iObjectType == OBJ_ATTACHMENT_SAPPER ) )
			return CB_CANNOT_BUILD;

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			// If a human is placing a sapper
			if ( !IsBot() && iObjectType == OBJ_ATTACHMENT_SAPPER )
			{
				// Only allow one Sapper of any kind in MvM
				if ( GetNumObjects( iObjectType, BUILDING_MODE_ANY ) )
					return CB_LIMIT_REACHED;

				return ( ( GetAmmoCount( TF_AMMO_GRENADES2 ) > 0 ) ? CB_CAN_BUILD : CB_CANNOT_BUILD );
			}
		}
	}

#ifndef CLIENT_DLL
	CTFPlayerClass *pCls = GetPlayerClass();

	if ( !bHasSubType && pCls && pCls->CanBuildObject( iObjectType ) == false )
	{
		return CB_CANNOT_BUILD;
	}
#endif

	// We can redeploy the object if we are carrying it.
	CBaseObject* pObjType = GetObjectOfType( iObjectType, iObjectMode );
	if ( pObjType && pObjType->IsCarried() )
	{
		return CB_CAN_BUILD;
	}

	// Special handling of "disposable" sentries
	if ( TFGameRules()->GameModeUsesUpgrades() && iObjectType == OBJ_SENTRYGUN )
	{
		// If we have our main sentry, see if we're allowed to build disposables
		if ( GetNumObjects( iObjectType, iObjectMode ) )
		{
			bool bHasPrimary = false;
			int nDisposableCount = 0;
			int nMaxDisposableCount = 0;
			CALL_ATTRIB_HOOK_INT( nMaxDisposableCount, engy_disposable_sentries );
			if ( nMaxDisposableCount )
			{

				for ( int i = GetObjectCount()-1; i >= 0; i-- )
				{
					CBaseObject *pObj = GetObject( i );
					if ( pObj )
					{
						if ( !pObj->IsDisposableBuilding() )
						{
							bHasPrimary = true;
						}
						else
						{
							nDisposableCount++;
						}
					}
				}

				if ( bHasPrimary )
				{
					if ( nDisposableCount < nMaxDisposableCount )
					{
						return CB_CAN_BUILD;
					}
					else
					{
						return CB_LIMIT_REACHED;
					}
				}
			}
		}
	}

	// Allow MVM engineer bots to have multiple sentries.  Currently they only need this so
	// they can appear to be carrying a new building when advancing their nest rather than
	// transporting an existing building.
	if( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && IsBot() )
	{
		return CB_CAN_BUILD;
	}

	// Make sure we haven't hit maximum number
	int iObjectCount = GetNumObjects( iObjectType, iObjectMode );
	if ( iObjectCount >= GetObjectInfo( iObjectType )->m_nMaxObjects && GetObjectInfo( iObjectType )->m_nMaxObjects != -1)
	{
		return CB_LIMIT_REACHED;
	}

	// Find out how much the object should cost
	int iCost = m_Shared.CalculateObjectCost( this, iObjectType );

	// Make sure we have enough resources
	if ( GetBuildResources() < iCost )
	{
		return CB_NEED_RESOURCES;
	}

	return CB_CAN_BUILD;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsAiming( void )
{
	if ( !m_pOuter )
		return false;

	bool bAiming = InCond( TF_COND_AIMING ) && !m_pOuter->IsPlayerClass( TF_CLASS_SOLDIER );
	if ( m_pOuter->IsPlayerClass( TF_CLASS_SNIPER ) && m_pOuter->GetActiveTFWeapon() && ( m_pOuter->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_SNIPERRIFLE_CLASSIC ) )
	{
		bAiming = InCond( TF_COND_ZOOMED );
	}

	return bAiming;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::ShouldSuppressPrediction( void )
{
	// c_tf_player will decide to do this, bits in gamerules will allow it. Sorry.
	return InCond( TF_COND_HALLOWEEN_KART ) && !InCond( TF_COND_HALLOWEEN_GHOST_MODE );
}

//-----------------------------------------------------------------------------
// Purpose: Set what type of rune we are carrying--or that we are not carrying any.
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetCarryingRuneType( RuneTypes_t rt )
{
#ifdef GAME_DLL
	// Stat Tracking
	if ( rt != RUNE_NONE )
	{
		// if getting a rune, start timer
		m_flRuneAcquireTime = gpGlobals->curtime;
	}
	else if ( IsCarryingRune() )
	{
		// if setting to none (death or drop) and I have a power up, report and set timer to -1
		float duration = gpGlobals->curtime - m_flRuneAcquireTime;
		m_flRuneAcquireTime = -1;

		CTF_GameStats.Event_PowerUpRuneDuration( m_pOuter, (int)duration, GetCarryingRuneType()	);
	}

	// clear rune charge
	SetRuneCharge( 0.f );
#endif

	// Not 100% sure AddCond does what I want to do if we already have that cond, so
	// let's assert so we can debug it if it ever comes up.
	Assert( rt != GetCarryingRuneType() );

	// We are only ever allowed to carry one rune type at a time, this logic ensures that.
	for ( int i = 0; i < RUNE_TYPES_MAX; ++i )
	{
		if ( i == rt )
		{
			AddCond( GetConditionFromRuneType( (RuneTypes_t) i ) );
		}
		else
		{
			RemoveCond( GetConditionFromRuneType( (RuneTypes_t) i ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the currently carried rune type, or RUNE_NONE if we are not carrying one.
//-----------------------------------------------------------------------------
RuneTypes_t CTFPlayerShared::GetCarryingRuneType( void ) const
{
	RuneTypes_t retVal = RUNE_NONE;
	for ( int i = 0; i < RUNE_TYPES_MAX; ++i )
	{
		if ( InCond( GetConditionFromRuneType( (RuneTypes_t) i ) ) )
		{
			// You are only allowed to have one rune type, if this hits we somehow erroneously 
			// have two condition bits set for different types of runes. 
			Assert( retVal == RUNE_NONE );
			retVal = (RuneTypes_t)i;
			break;
		}
	}

	return retVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::CalculateObjectCost( CTFPlayer* pBuilder, int iObjectType )
{
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( TFGameRules()->InSetup() || TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
		{
			return 0;
		}
	}

	int nCost = InternalCalculateObjectCost( iObjectType );

	// Mini sentires are 30 metal cheaper
	CTFWrench* pWrench = dynamic_cast<CTFWrench*>( pBuilder->Weapon_OwnsThisID( TF_WEAPON_WRENCH ) );
	if ( pWrench && pWrench->IsPDQ() && ( iObjectType == OBJ_SENTRYGUN ) )
	{
		nCost -= 30;
	}
	

	if ( iObjectType == OBJ_TELEPORTER )
	{
		float flCostMod = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pBuilder, flCostMod, mod_teleporter_cost );
		if ( flCostMod != 1.f )
		{
			nCost *= flCostMod;
		}
	}

	CALL_ATTRIB_HOOK_INT_ON_OTHER( pBuilder, nCost, building_cost_reduction );
	
	return nCost;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::HealthKitPickupEffects( int iHealthGiven /*= 0*/ )
{
	// Healthkits also contain a fire blanket.
	if ( InCond( TF_COND_BURNING ) )
	{
		RemoveCond( TF_COND_BURNING );		
	}
	// and sutures
	if ( InCond( TF_COND_BLEEDING ) )
	{
		RemoveCond( TF_COND_BLEEDING );
	}
	// and cures plague
	if ( InCond( TF_COND_PLAGUE ) )
	{
		RemoveCond( TF_COND_PLAGUE );
	}

	// Spawns a number on the player's health bar in the HUD, and also
	// spawns a "+" particle over their head for enemies to see
	if ( iHealthGiven && !IsStealthed() && m_pOuter )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iHealthGiven );
			event->SetInt( "entindex", m_pOuter->entindex() );
			event->SetInt( "weapon_def_index", INVALID_ITEM_DEF_INDEX );
			gameeventmanager->FireEvent( event ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of objects of the specified type that this player has
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumObjects( int iObjectType, int iObjectMode /*= 0*/ )
{
	int iCount = 0;
	for (int i = 0; i < GetObjectCount(); i++)
	{
		if ( !GetObject(i) )
			continue;

		if ( GetObject(i)->IsDisposableBuilding() )
			continue;

		if ( GetObject(i)->GetType() == iObjectType && 
			( GetObject(i)->GetObjectMode() == iObjectMode || iObjectMode == BUILDING_MODE_ANY ) )
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ItemPostFrame()
{
	m_Shared.UpdateItemChargeMeters();

	// cache buttons because some weapons' ItemPostFrame could change m_nButtons against other weapons
	int nButtons = m_nButtons;

	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		if ( gpGlobals->curtime < m_flNextAttack )
		{
			m_hOffHandWeapon->ItemBusyFrame();
		}
		else
		{
#if defined( CLIENT_DLL )
			// Not predicting this weapon
			if ( m_hOffHandWeapon->IsPredicted() )
#endif
			{
				m_hOffHandWeapon->ItemPostFrame( );
			}
		}
	}

	CUtlVector< CTFWeaponBase* > vecPassiveWeapons;
	if ( GetPassiveWeapons( vecPassiveWeapons ) )
	{
		if ( gpGlobals->curtime < m_flNextAttack )
		{
			FOR_EACH_VEC( vecPassiveWeapons, i )
			{
				vecPassiveWeapons[i]->ItemBusyFrame();
			}
		}
		else
		{
			FOR_EACH_VEC( vecPassiveWeapons, i )
			{

#if defined( CLIENT_DLL )
				// Not predicting this weapon
				if ( vecPassiveWeapons[i]->IsPredicted() )
#endif
				{
					vecPassiveWeapons[i]->ItemPostFrame();
				}
			}
		}
	}

#ifdef GAME_DLL
	CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		pActiveWeapon->HandleInspect();
	}
#endif // GAME_DLL

	BaseClass::ItemPostFrame();

	// restore m_nButtons so all the afButtons are in correct state
	m_nButtons = nButtons;
}

void CTFPlayer::SetOffHandWeapon( CTFWeaponBase *pWeapon )
{
	m_hOffHandWeapon = pWeapon;
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Deploy();
	}
}

// Set to NULL at the end of the holster?
void CTFPlayer::HolsterOffHandWeapon( void )
{
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Holster();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should record our last weapon when switching between the two specified weapons
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	// if the weapon doesn't want to be auto-switched to, don't!
	CTFWeaponBase *pTFOldWeapon = dynamic_cast< CTFWeaponBase * >( pOldWeapon );
	if ( pTFOldWeapon )
	{
		if ( pTFOldWeapon->AllowsAutoSwitchTo() == false )
		{
			return false;
		}
	}

	return BaseClass::Weapon_ShouldSetLast( pOldWeapon, pNewWeapon );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_PoseParamOverride( CTFWeaponBase *pOldWeapon, CTFWeaponBase *pNewWeapon )
{
	auto lambdaUpdatePoseParam = []( CBaseAnimating *pAnim, CTFWeaponBase *pWeapon, bool bReset, bool bPlayer )
	{
		if ( pWeapon )
		{
			int iCount = 0;
			const poseparamtable_t *pPoseParamList = bPlayer ? pWeapon->GetPlayerPoseParamList( iCount ) : pWeapon->GetItemPoseParamList( iCount );
			if ( pPoseParamList )
			{
				for ( int i=0; i<iCount; ++i )
				{
					const char *pszPoseParamName = pPoseParamList[i].strName;
					float flNewVal = bReset ? 0 : pPoseParamList[i].flValue;
					pAnim->SetPoseParameter( pszPoseParamName, flNewVal );
				}
			}
		}
	};

	// reset poseparam on the old weapon
	lambdaUpdatePoseParam( this, pOldWeapon, true, true );

	// set new one on the new weapon
	lambdaUpdatePoseParam( this, pNewWeapon, false, true );

	// update poseparam on weapon if necessary
	lambdaUpdatePoseParam( pNewWeapon, pNewWeapon, false, false );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::SelectItem( const char *pstr, int iSubType /*= 0*/ )
{
	// This is basically a copy from the base class with addition of Weapon_CanSwitchTo
	// We're not calling BaseClass::SelectItem on purpose to prevent breaking other games
	// that might rely on not calling Weapon_CanSwitchTo

	if (!pstr)
		return;

	CBaseCombatWeapon *pItem = Weapon_OwnsThisType( pstr, iSubType );

	if (!pItem)
		return;

	if( GetObserverMode() != OBS_MODE_NONE )
		return;// Observers can't select things.

	if ( !Weapon_ShouldSelectItem( pItem ) )
		return;

	// FIX, this needs to queue them up and delay
	// Make sure the current weapon can be holstered
	if ( !Weapon_CanSwitchTo( pItem ) )
		return;

	ResetAutoaim();

	Weapon_Switch( pItem );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	// Ghosts cant switch weapons!
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		return false;
	}

	if ( m_Shared.m_flHolsterAnimTime == 0.f
#ifdef GAME_DLL
		&& !m_bSwitchedClass && !m_bRespawning && !m_bRegenerating
#endif // GAME_DLL
		)
	{
		CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
		if ( pActiveWeapon && pActiveWeapon->CanHolster() && pWeapon != pActiveWeapon )
		{
			float flHolsterAnimTime = 0.f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pActiveWeapon, flHolsterAnimTime, holster_anim_time );
			if ( flHolsterAnimTime > 0.f )
			{
				// this actually calls Holster() to start the anim. Then we do another Holster call when we actually switch weapon.
				pActiveWeapon->StartHolsterAnim();
				m_Shared.m_flHolsterAnimTime = gpGlobals->curtime + flHolsterAnimTime;
				m_Shared.m_hSwitchTo = pWeapon;
				return false;
			}
		}
	}
	else if ( m_Shared.m_flHolsterAnimTime > 0.f && gpGlobals->curtime < m_Shared.m_flHolsterAnimTime )
	{
		return false;
	}
	else
	{
		m_Shared.m_flHolsterAnimTime = 0.f;
	}

	// set last weapon before we switch to a new weapon to make sure that we can get the correct last weapon in Deploy/Holster
	// This should be done in CBasePlayer::Weapon_Switch, but we don't want to break other games
	CBaseCombatWeapon *pPreviousLastWeapon = GetLastWeapon();
	CBaseCombatWeapon *pPreviousActiveWeapon = GetActiveWeapon();

	// always set last for Weapon_Switch code to get attribute from the correct last item
	Weapon_SetLast( GetActiveWeapon() );

	bool bSwitched = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
	if ( bSwitched )
	{
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );

		// valid last weapon
		if ( Weapon_ShouldSetLast( pPreviousActiveWeapon, pWeapon ) )
		{
			Weapon_SetLast( pPreviousActiveWeapon );
			SetSecondaryLastWeapon( pPreviousLastWeapon );
		}
		// previous active weapon is not valid to be last weapon, but the new active weapon is
		else if ( Weapon_ShouldSetLast( pWeapon, pPreviousLastWeapon ) )
		{
			// this will skip the logic to ignore first time block and allow weapon to check for honorbound attribute right away
			CTFWeaponBase *pTFWeapon = assert_cast< CTFWeaponBase * >( pWeapon );
			if ( pTFWeapon && pTFWeapon->IsHonorBound() )
			{
				m_Shared.m_flFirstPrimaryAttack = gpGlobals->curtime;
			}
			
			if ( pWeapon != GetSecondaryLastWeapon() )
			{
				Weapon_SetLast( GetSecondaryLastWeapon() );
				SetSecondaryLastWeapon( pPreviousLastWeapon );
			}
			else
			{
				// new active weapon is the same as the secondary last weapon, leave the last weapon alone
				Weapon_SetLast( pPreviousLastWeapon );
			}
		}
		// both previous and new active weapons are not not valid for last weapon
		else
		{
			Weapon_SetLast( pPreviousLastWeapon );
		}

		// override pose params
		Weapon_PoseParamOverride( assert_cast< CTFWeaponBase * >( pPreviousActiveWeapon ), assert_cast< CTFWeaponBase * >( pWeapon ) );
	}
	else
	{
		// restore to the previous last weapon if we failed to switch to a new weapon
		Weapon_SetLast( pPreviousLastWeapon );
	}

#ifdef GAME_DLL
	if ( bSwitched && TFGameRules() && TFGameRules()->IsInTraining() && TFGameRules()->GetTrainingModeLogic() && IsFakeClient() == false)
	{
		TFGameRules()->GetTrainingModeLogic()->OnPlayerSwitchedWeapons( this );
	}
#endif

#ifdef CLIENT_DLL
	m_Shared.UpdateCritBoostEffect();
#endif

	return bSwitched;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWearable *CTFPlayer::GetEquippedWearableForLoadoutSlot( int iLoadoutSlot )
{
	int iClass = GetPlayerClass()->GetClassIndex();

	for ( int i = 0; i < GetNumWearables(); ++i )
	{
		CTFWearable *pWearableItem = dynamic_cast< CTFWearable * >( GetWearable( i ) );
		if ( !pWearableItem )
			continue;

		if ( !pWearableItem->GetAttributeContainer() )
			continue;

		CEconItemView *pEconItemView = pWearableItem->GetAttributeContainer()->GetItem();
		if ( !pEconItemView )
			continue;

		CTFItemDefinition *pItemDef = pEconItemView->GetStaticData();
		if ( !pItemDef )
			continue;

		if ( pItemDef->GetLoadoutSlot(iClass) == iLoadoutSlot )
			return pWearableItem;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::GetEntityForLoadoutSlot( int iLoadoutSlot, bool bForceCheckWearable /*= false*/ )
{
	CBaseEntity *pEntity = NULL;
	if ( IsWearableSlot( iLoadoutSlot ) || bForceCheckWearable )
	{
		// Search Wearables first otherwise search Weapons as a fall back
		pEntity = GetEquippedWearableForLoadoutSlot( iLoadoutSlot );
		if ( pEntity )
		{
			return pEntity;
		}
	}

	int iClass = GetPlayerClass()->GetClassIndex();
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( GetWeapon(i) )
		{
			CEconItemView *pEconItemView = GetWeapon(i)->GetAttributeContainer()->GetItem();
			if ( !pEconItemView )
				continue;

			CTFItemDefinition *pItemDef = pEconItemView->GetStaticData();
			if ( !pItemDef )
				continue;

			if ( pItemDef->GetLoadoutSlot( iClass ) == iLoadoutSlot )
				return GetWeapon(i);
		}
	}
	return NULL;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StopViewModelParticles( C_BaseEntity *pParticleEnt )
{
	pParticleEnt->ParticleProp()->StopParticlesInvolving( pParticleEnt );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::GetStepSoundVelocities( float *velwalk, float *velrun )
{
	float flMaxSpeed = MaxSpeed();

	if ( ( GetFlags() & FL_DUCKING ) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		if ( m_Shared.IsLoser() )
		{
			*velwalk = 0;
			*velrun = 0;		
		}
		else
		{
			*velwalk = flMaxSpeed * 0.25;
			*velrun = flMaxSpeed * 0.3;		
		}
	}
	else
	{
		*velwalk = flMaxSpeed * 0.3;
		*velrun = flMaxSpeed * 0.8;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking )
{
	float flMaxSpeed = MaxSpeed();

	switch ( iStepSoundTime )
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 400, 200 );
		if ( bWalking )
		{
			m_flStepSoundTime += 100;
		}
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 600, 400 );
		break;

	default:
		Assert(0);
		break;
	}

	if ( ( GetFlags() & FL_DUCKING) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		m_flStepSoundTime += 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayer::GetOverrideStepSound( const char *pszBaseStepSoundName )
{

	if( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS && !IsMiniBoss() && !m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		return "MVM.BotStep";
	}

	Assert( pszBaseStepSoundName );

	struct override_sound_entry_t { int iOverrideIndex; const char *pszBaseSoundName; const char *pszNewSoundName; };

	enum
	{
		kFootstepSoundSet_Default = 0,
		kFootstepSoundSet_SoccerCleats = 1,
		kFootstepSoundSet_HeavyGiant = 2,
		kFootstepSoundSet_SoldierGiant = 3,
		kFootstepSoundSet_DemoGiant = 4,
		kFootstepSoundSet_ScoutGiant = 5,
		kFootstepSoundSet_PyroGiant = 6,
		kFootstepSoundSet_SentryBuster = 7,
		kFootstepSoundSet_TreasureChest = 8,
		kFootstepSoundSet_Octopus = 9,
	};

	int iOverrideFootstepSoundSet = kFootstepSoundSet_Default;
	CALL_ATTRIB_HOOK_INT( iOverrideFootstepSoundSet, override_footstep_sound_set );

	if ( iOverrideFootstepSoundSet != kFootstepSoundSet_Default )
	{
		static const override_sound_entry_t s_ReplacementSounds[] =
		{
			{ kFootstepSoundSet_SoccerCleats,	"Default.StepLeft",		"cleats_conc.StepLeft" },
			{ kFootstepSoundSet_SoccerCleats,	"Default.StepRight",	"cleats_conc.StepRight" },
			{ kFootstepSoundSet_SoccerCleats,	"Dirt.StepLeft",		"cleats_dirt.StepLeft" },
			{ kFootstepSoundSet_SoccerCleats,	"Dirt.StepRight",		"cleats_dirt.StepRight" },
			{ kFootstepSoundSet_SoccerCleats,	"Concrete.StepLeft",	"cleats_conc.StepLeft" },
			{ kFootstepSoundSet_SoccerCleats,	"Concrete.StepRight",	"cleats_conc.StepRight" },

			//
			{ kFootstepSoundSet_Octopus,	"Default.StepLeft",		"Octopus.StepCommon" },
			{ kFootstepSoundSet_Octopus,	"Default.StepRight",	"Octopus.StepCommon" },
			{ kFootstepSoundSet_Octopus,	"Dirt.StepLeft",		"Octopus.StepCommon" },
			{ kFootstepSoundSet_Octopus,	"Dirt.StepRight",		"Octopus.StepCommon" },
			{ kFootstepSoundSet_Octopus,	"Concrete.StepLeft",	"Octopus.StepCommon" },
			{ kFootstepSoundSet_Octopus,	"Concrete.StepRight",	"Octopus.StepCommon" },

			//
			{ kFootstepSoundSet_HeavyGiant,		"",		"MVM.GiantHeavyStep" },

			//
			{ kFootstepSoundSet_SoldierGiant,	"",		"MVM.GiantSoldierStep" },

			//
			{ kFootstepSoundSet_DemoGiant,		"",		"MVM.GiantDemomanStep" },

			//
			{ kFootstepSoundSet_ScoutGiant,		"",		"MVM.GiantScoutStep" },

			//
			{ kFootstepSoundSet_PyroGiant,		"",		"MVM.GiantPyroStep" },

			//
			{ kFootstepSoundSet_SentryBuster,	"",		"MVM.SentryBusterStep" },

			//
			{ kFootstepSoundSet_TreasureChest,	"",		"Chest.Step" },
		};

		for ( int i = 0; i < ARRAYSIZE( s_ReplacementSounds ); i++ )
		{
			if ( iOverrideFootstepSoundSet == s_ReplacementSounds[i].iOverrideIndex )
			{
				if ( !s_ReplacementSounds[i].pszBaseSoundName[0] ||
					 !Q_stricmp( pszBaseStepSoundName, s_ReplacementSounds[i].pszBaseSoundName ) )
					return s_ReplacementSounds[i].pszNewSoundName;
			}
		}
	}

	// Fallback.
	return BaseClass::GetOverrideStepSound( pszBaseStepSoundName );
}

void CTFPlayer::OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume )
{
	// play jingles in addition to normal footstep sounds, 
	// and play them quietly to the local player so they don't go insane
	int iJingle = 0;
	CALL_ATTRIB_HOOK_INT( iJingle, add_jingle_to_footsteps );
	if ( ( iJingle > 0 ) && !m_Shared.IsStealthed() && !m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		CRecipientFilter filter;
		filter.AddRecipientsByPAS( vecOrigin );

#ifndef CLIENT_DLL
		// in MP, server removes all players in the vecOrigin's PVS, these players generate the footsteps client side
		if ( gpGlobals->maxClients > 1 )
		{
			filter.RemoveRecipientsByPVS( vecOrigin );
		}
#endif

		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = ( iJingle == 1 ) ? "xmas.jingle" : "xmas.jingle_higher";
#ifdef CLIENT_DLL
		ep.m_flVolume = IsLocalPlayer() ? 0.3f * fVolume : fVolume;	// quieter for local player
#else
		ep.m_flVolume = fVolume;
#endif
		ep.m_SoundLevel = params.soundlevel;
		ep.m_nFlags = SND_CHANGE_VOL;
		ep.m_nPitch = params.pitch;
		ep.m_pOrigin = &vecOrigin;

		EmitSound( filter, entindex(), ep );
	}

#ifdef CLIENT_DLL
	// Halloween-specific bonus footsteps for viewmodel-rendering only. Real model footsteps will happen in the real
	// footstep code in response to animation events. THIS IS A HACK!
	if ( !ShouldDrawThisPlayer() && !m_Shared.IsStealthed() && !m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		int iHalloweenFootstepType = 0;
		if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
		{
			CALL_ATTRIB_HOOK_INT( iHalloweenFootstepType, halloween_footstep_type );
		}

		if ( m_nFootStamps > 0 )
		{
			// White color!
			iHalloweenFootstepType = 0xFFFFFFFF;
		}

		if ( iHalloweenFootstepType != 0 )
		{
			CNewParticleEffect *pEffect = SpawnHalloweenSpellFootsteps( PATTACH_CUSTOMORIGIN, iHalloweenFootstepType );
			if ( pEffect )
			{
				pEffect->SetControlPoint( 0, GetAbsOrigin() );
			}
		}

		if ( m_nFootStamps > 0 )
		{
			m_nFootStamps--;
		}
	}
#endif
}

void CTFPlayer::ModifyEmitSoundParams( EmitSound_t &params )
{
	BaseClass::ModifyEmitSoundParams( params );

	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		pWeapon->ModifyEmitSoundParams( params );
	}

#ifdef CLIENT_DLL
	ClientAdjustStartSoundParams( params );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAttack( int iCanAttackFlags )
{
	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( IsViewingCYOAPDA() )
		return false;

	if ( m_Shared.HasPasstimeBall() ) 
	{
		// Always allow throwing the ball.
		return true;
	}

	if ( ( m_Shared.GetStealthNoAttackExpireTime() > gpGlobals->curtime && !m_Shared.InCond( TF_COND_STEALTHED_USER_BUFF ) ) || m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if ( !( iCanAttackFlags & TF_CAN_ATTACK_FLAG_GRAPPLINGHOOK ) )
		{
#ifdef CLIENT_DLL
			HintMessage( HINT_CANNOT_ATTACK_WHILE_CLOAKED, true, true );
#endif
			return false;
		}
	}

	if ( m_Shared.IsFeignDeathReady() )
	{
#ifdef CLIENT_DLL
		HintMessage( HINT_CANNOT_ATTACK_WHILE_FEIGN_ARMED, true, true );
#endif

		return false;
	}

	if ( IsTaunting() )
		return false;

	if ( m_Shared.InCond( TF_COND_PHASE ) == true )
		return false;

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		return false;
	}


	int iNoAttack = 0;
	CALL_ATTRIB_HOOK_INT( iNoAttack, no_attack );
	if ( iNoAttack )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanJump() const
{
	// Cannot jump while taunting
	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		if ( !pActiveWeapon->OwnerCanJump() )
			return false;
	}

	int iNoJump = 0;
	CALL_ATTRIB_HOOK_INT( iNoJump, no_jump );

	return iNoJump == 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanDuck() const
{
	int iNoDuck = 0;
	CALL_ATTRIB_HOOK_INT( iNoDuck, no_duck );

	return iNoDuck == 0;
}


//-----------------------------------------------------------------------------
// Purpose: Weapons can call this on secondary attack and it will link to the class
// ability
//-----------------------------------------------------------------------------
bool CTFPlayer::DoClassSpecialSkill( void )
{
	if ( !IsAlive() )
		return false;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	bool bDoSkill = false;

	// powerup charge activation has higher priority than any class special skill
	if ( m_Shared.IsRuneCharged() && GetActiveTFWeapon() && GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_GRAPPLINGHOOK )
	{
#ifdef GAME_DLL
		CTFGrapplingHook *pHook = static_cast<CTFGrapplingHook*>( GetActiveTFWeapon() );
		if ( pHook )
		{
			pHook->ActivateRune();
		}
#endif // GAME_DLL
		return true;
	}

	switch( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_SPY:
		{
			if ( !m_Shared.InCond( TF_COND_TAUNTING ) )
			{
				if ( m_Shared.m_flStealthNextChangeTime <= gpGlobals->curtime )
				{
					// Feign death if we have the right equipment mod.
					CTFWeaponInvis* pInvisWatch = static_cast<CTFWeaponInvis*>( Weapon_OwnsThisID( TF_WEAPON_INVIS ) );
					if ( pInvisWatch )
					{
						pInvisWatch->ActivateInvisibilityWatch();
					}
				}
			}
		}
		break;

	case TF_CLASS_DEMOMAN:
		if ( !m_Shared.HasPasstimeBall() )
		{
			CTFPipebombLauncher *pPipebombLauncher = static_cast<CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );
			if ( pPipebombLauncher )
			{
				pPipebombLauncher->SecondaryAttack();
			}
			else
			{
				CTFWearableDemoShield *pWearableShield = GetEquippedDemoShield( this );
				if ( pWearableShield )
				{
					pWearableShield->DoSpecialAction( this );
					break;
				}
			}
		}
		bDoSkill = true;
		break;
	case TF_CLASS_ENGINEER:
		if ( !m_Shared.HasPasstimeBall() )
		{
			bDoSkill = TryToPickupBuilding();
		}
		break;
	default:
		break;
	}

	return bDoSkill;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::EndClassSpecialSkill( void )
{
	if ( !IsAlive() )
		return false;

	switch( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_DEMOMAN:
		{
			CTFPipebombLauncher *pPipebombLauncher = static_cast<CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );
			if ( !pPipebombLauncher )
			{
				CTFWearableDemoShield *pWearableShield = GetEquippedDemoShield( this );
				if ( pWearableShield )
				{
					pWearableShield->EndSpecialAction( this );
					break;
				}
			}
		}
		break;

	default:
		break;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanPickupBuilding( CBaseObject *pPickupObject )
{
	if ( !pPickupObject )
		return false;

	if ( pPickupObject->IsBuilding() )
		return false;

	if ( pPickupObject->IsUpgrading() )
		return false;

	if ( pPickupObject->HasSapper() )
		return false;

	if ( pPickupObject->IsPlasmaDisabled() )
		return false;

	// If we were recently carried & placed we may still be upgrading up to our old level.
	if ( pPickupObject->GetUpgradeLevel() != pPickupObject->GetHighestUpgradeLevel() )
		return false;

	if ( !IsAlive() )
		return false;

	if ( m_Shared.IsCarryingObject() )
		return false;

	if ( m_Shared.IsLoserStateStunned() || m_Shared.IsControlStunned() )
		return false;

	if ( m_Shared.IsLoser() )
		return false;

	if ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING && TFGameRules()->State_Get() != GR_STATE_STALEMATE && TFGameRules()->State_Get() != GR_STATE_BETWEEN_RNDS )
		return false;

	// don't allow to pick up building while grappling hook
	if ( m_Shared.InCond( TF_COND_GRAPPLINGHOOK ) )
		return false;

	// There's ammo in the clip... no switching away!
	if ( GetActiveTFWeapon() && GetActiveTFWeapon()->AutoFiresFullClip() && GetActiveTFWeapon()->Clip1() > 0 )
		return false;

 	// Knockout powerup restricts user to melee only, so cannot equip other items such as building pickups
	if ( m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
//		ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_Pickup_Deny" );
		{
		ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_No_Building_Pickup" );
			return false;
		}


	// Check it's within range
	int nPickUpRangeSq = TF_BUILDING_PICKUP_RANGE * TF_BUILDING_PICKUP_RANGE;
	int iIncreasedRangeCost = 0;
	int nSqrDist = (EyePosition() - pPickupObject->GetAbsOrigin()).LengthSqr();

	// Extra range only works with primary weapon
	CTFWeaponBase * pWeapon = GetActiveTFWeapon();
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iIncreasedRangeCost, building_teleporting_pickup );
	if ( iIncreasedRangeCost != 0 )
	{
		// False on deadzone
		if ( nSqrDist > nPickUpRangeSq && nSqrDist < TF_BUILDING_RESCUE_MIN_RANGE_SQ )
			return false;
		if ( nSqrDist >= TF_BUILDING_RESCUE_MIN_RANGE_SQ && GetAmmoCount( TF_AMMO_METAL ) < iIncreasedRangeCost )
			return false;
		return true;
	}
	else if ( nSqrDist > nPickUpRangeSq )
		return false;

	if ( TFGameRules()->IsInTraining() )
	{
		ConVarRef training_can_pickup_sentry( "training_can_pickup_sentry" );
		ConVarRef training_can_pickup_dispenser( "training_can_pickup_dispenser" );
		ConVarRef training_can_pickup_tele_entrance( "training_can_pickup_tele_entrance" );
		ConVarRef training_can_pickup_tele_exit( "training_can_pickup_tele_exit" );
		switch ( pPickupObject->GetType() )
		{
		case OBJ_DISPENSER:
			return training_can_pickup_dispenser.GetBool();
		case OBJ_TELEPORTER:
			return pPickupObject->GetObjectMode() == MODE_TELEPORTER_ENTRANCE ? training_can_pickup_tele_entrance.GetBool() : training_can_pickup_tele_exit.GetBool();
		case OBJ_SENTRYGUN:
			return training_can_pickup_sentry.GetBool();
		} // switch
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::TryToPickupBuilding()
{
	if ( m_Shared.IsCarryingObject() )
		return false;

	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) )
		return false;

	if ( m_Shared.InCond( TF_COND_MELEE_ONLY ) )
		return false;

	if ( m_Shared.InCond( TF_COND_SWIMMING_CURSE ) )
		return false;

#ifdef GAME_DLL
	if ( m_bIsTeleportingUsingEurekaEffect )
		return false;

	int iCannotPickUpBuildings = 0;
	CALL_ATTRIB_HOOK_INT( iCannotPickUpBuildings, cannot_pick_up_buildings );
	if ( iCannotPickUpBuildings )
	{
		return false;
	}
#endif

	// Check to see if a building we own is in front of us.
	Vector vecForward;
	AngleVectors( EyeAngles(), &vecForward, NULL, NULL );

	int iPickUpRange = TF_BUILDING_PICKUP_RANGE;
	int iIncreasedRangeCost = 0;
	CTFWeaponBase * pWeapon = GetActiveTFWeapon();
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iIncreasedRangeCost, building_teleporting_pickup );
	if ( iIncreasedRangeCost != 0 )
	{
		iPickUpRange = TF_BUILDING_RESCUE_MAX_RANGE;
	}
	
	const Vector vecStart = EyePosition();
	const Vector vecEnd   = vecStart + vecForward * iPickUpRange;

	// Create a ray a see if any of my objects touch it
	Ray_t ray;
	ray.Init( vecStart, vecEnd );

	CBulletPenetrateEnum ePickupPenetrate( vecStart, vecEnd, this, TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS, false );
	enginetrace->EnumerateEntities( ray, false, &ePickupPenetrate );

	CBaseObject *pPickupObject = NULL;
	float flCurrDistanceSq = iPickUpRange * iPickUpRange;

	for ( int i=0; i<GetObjectCount(); i++ )
	{
		CBaseObject	*pObj = GetObject(i);
		if ( !pObj )
			continue;

		float flDistToObjSq = ( pObj->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
		if ( flDistToObjSq > flCurrDistanceSq )
			continue;		

		FOR_EACH_VEC( ePickupPenetrate.m_Targets, iTarget )
		{
			if ( ePickupPenetrate.m_Targets[iTarget].pTarget == pObj )
			{
				CTargetOnlyFilter penetrateFilter( this, pObj );
				trace_t pTraceToUse;
				UTIL_TraceLine( EyePosition(), EyePosition() + vecForward * iPickUpRange, ( MASK_SOLID | CONTENTS_HITBOX ), &penetrateFilter, &pTraceToUse );
				if ( pTraceToUse.m_pEnt == pObj )
				{
					pPickupObject = pObj;
					flCurrDistanceSq = flDistToObjSq;
					break;
				}
			}
			if ( ePickupPenetrate.m_Targets[iTarget].pTarget->IsWorld() )
			{
				break;
			}
		}
	}

	if ( !CanPickupBuilding(pPickupObject) )
	{
		if ( pPickupObject )
		{
			CSingleUserRecipientFilter filter( this );
			EmitSound( filter, entindex(), "Player.UseDeny", NULL, 0.0f );
		}

		return false;
	}

#ifdef CLIENT_DLL

	return (bool) pPickupObject;

#elif GAME_DLL

	if ( pPickupObject )
	{
		// remove rage for long range
		if ( iIncreasedRangeCost )
		{
			int nSqrDist = (EyePosition() - pPickupObject->GetAbsOrigin()).LengthSqr();
			if ( nSqrDist > TF_BUILDING_RESCUE_MIN_RANGE_SQ )
			{
				RemoveAmmo( iIncreasedRangeCost, TF_AMMO_METAL );

				// Particles
				// Spawn a railgun
				Vector origin = pPickupObject->GetAbsOrigin();
				CPVSFilter filter( origin );

				const char *pRailParticleName = GetTeamNumber() == TF_TEAM_BLUE ? "dxhr_sniper_rail_blue" : "dxhr_sniper_rail_red";
				const char *pTeleParticleName = GetTeamNumber() == TF_TEAM_BLUE ? "teleported_blue" : "teleported_red";

				TE_TFParticleEffect( filter, 0.0, pTeleParticleName, origin, vec3_angle );

				te_tf_particle_effects_control_point_t controlPoint = { PATTACH_WORLDORIGIN, pPickupObject->GetAbsOrigin() + Vector(0,0,32) };
				TE_TFParticleEffectComplex( filter, 0.0f, pRailParticleName, GetAbsOrigin() + Vector(0,0,32), QAngle( 0, 0, 0 ), NULL, &controlPoint );

				// Play Sounds
				pPickupObject->EmitSound( "Building_Teleporter.Send" );
				EmitSound( "Building_Teleporter.Receive" );
			}
		}

		pPickupObject->MakeCarriedObject( this );

		CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder*>(Weapon_OwnsThisID( TF_WEAPON_BUILDER ));
		if ( pBuilder )
		{
			if ( GetActiveTFWeapon() == pBuilder )
				SetActiveWeapon( NULL );

			Weapon_Switch( pBuilder );
			pBuilder->m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		}

		SpeakConceptIfAllowed( MP_CONCEPT_PICKUP_BUILDING, pPickupObject->GetResponseRulesModifier() );

		m_flCommentOnCarrying = gpGlobals->curtime + random->RandomFloat( 6.f, 12.f );
		return true;
	}
	else
	{
		return false;
	}


#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanGoInvisible( bool bAllowWhileCarryingFlag )
{
	// The "flag" in Player Destruction doesn't block cloak
	ETFFlagType ignoreTypes[] = { TF_FLAGTYPE_PLAYER_DESTRUCTION };
	if ( !bAllowWhileCarryingFlag && ( HasTheFlag( ignoreTypes, ARRAYSIZE( ignoreTypes ) ) || m_Shared.HasPasstimeBall() ) )
	{
		HintMessage( HINT_CANNOT_CLOAK_WITH_FLAG );
		return false;
	}

	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanStartPhase( void )
{
	if ( HasTheFlag() || m_Shared.HasPasstimeBall() )
	{
		HintMessage( HINT_CANNOT_PHASE_WITH_FLAG );
		return false;
	}

	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//ConVar testclassviewheight( "testclassviewheight", "0", FCVAR_DEVELOPMENTONLY );
//Vector vecTestViewHeight(0,0,0);

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
Vector CTFPlayer::GetClassEyeHeight( void )
{
	CTFPlayerClass *pClass = GetPlayerClass();

	if ( !pClass )
		return VEC_VIEW_SCALED( this );

	//if ( testclassviewheight.GetFloat() > 0 )
	//{
	//	vecTestViewHeight.z = test.GetFloat();
	//	return vecTestViewHeight;
	//}

	int iClassIndex = pClass->GetClassIndex();

	if ( iClassIndex < TF_FIRST_NORMAL_CLASS || iClassIndex > TF_LAST_NORMAL_CLASS )
		return VEC_VIEW_SCALED( this );

	return g_TFClassViewVectors[pClass->GetClassIndex()] * GetModelScale();
}


CTFWeaponBase *CTFPlayer::Weapon_OwnsThisID( int iWeaponID ) const
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		if ( pWpn->GetWeaponID() == iWeaponID )
		{
			return pWpn;
		}
	}

	return NULL;
}

CTFWeaponBase *CTFPlayer::Weapon_GetWeaponByType( int iType )
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		int iWeaponRole = pWpn->GetTFWpnData().m_iWeaponType;

		if ( iWeaponRole == iType )
		{
			return pWpn;
		}
	}

	return NULL;
}

bool CTFPlayer::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	bool bCanSwitch = BaseClass::Weapon_CanSwitchTo( pWeapon );

	if ( bCanSwitch )
	{
		if ( GetActiveTFWeapon() )
		{
			// There's ammo in the clip while auto firing... no switching away!
			if ( GetActiveTFWeapon()->AutoFiresFullClip() && GetActiveTFWeapon()->Clip1() > 0 )
				return false;
		}

		if ( m_Shared.IsCarryingObject() && (GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER) )
		{
			CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( pWeapon );
			if ( pTFWeapon && (pTFWeapon->GetWeaponID() != TF_WEAPON_BUILDER) )
			{
				return false;
			}
		}

		// prevents script exploits, like switching to the minigun while eating a sandvich
		if ( IsTaunting() && tf_allow_taunt_switch.GetInt() == 0 )
		{
			return false;
		}

		int iDisableWeaponSwitch = 0;
		CALL_ATTRIB_HOOK_INT( iDisableWeaponSwitch, disable_weapon_switch );
		if ( iDisableWeaponSwitch != 0 )
			return false;
	}

	return bCanSwitch;
}

void CTFPlayer::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
#ifdef CLIENT_DLL
	// Don't make predicted footstep sounds in third person, animevents will take care of that.
	if ( prediction->InPrediction() && C_BasePlayer::ShouldDrawLocalPlayer() )
		return;
#endif

	BaseClass::PlayStepSound( vecOrigin, psurface, fvol, force );
}

//-----------------------------------------------------------------------------
// Purpose: Gives the player an opportunity to abort a double jump.
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAirDash( void ) const
{	
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_SPEED_BOOST ) )
		return true;

	bool bScout = GetPlayerClass()->IsClass( TF_CLASS_SCOUT );
	if ( !bScout )
		return false;

	if ( m_Shared.InCond( TF_COND_SODAPOPPER_HYPE ) )
	{
		if ( m_Shared.GetAirDash() < 5 )
			return true;
		else
 			return false;
	}

	CTFWeaponBase *pTFActiveWeapon = GetActiveTFWeapon();
	int iDashCount = tf_scout_air_dash_count.GetInt();
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFActiveWeapon, iDashCount, air_dash_count );

	if ( m_Shared.GetAirDash() >= iDashCount )
		return false;

	if ( pTFActiveWeapon )
	{
		// TODO(driller): Hack fix to restrict this to The Atomzier (currently the only item that uses this attribute) on what would be the third jump
		float flTimeSinceDeploy = gpGlobals->curtime - pTFActiveWeapon->GetLastDeployTime();
		if ( iDashCount >= 2 && m_Shared.GetAirDash() == 1 && flTimeSinceDeploy < 0.7f )
			return false;
	}

	int iNoAirDash = 0;
	CALL_ATTRIB_HOOK_INT( iNoAirDash, set_scout_doublejump_disabled );
	if ( 1 == iNoAirDash )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Should be immune to Jarate and Mad Milk?
//-----------------------------------------------------------------------------
bool CTFPlayer::CanGetWet( void ) const
{
	int iWetImmune = 0;
	CALL_ATTRIB_HOOK_INT( iWetImmune, wet_immunity );
	
	return iWetImmune ? false : true;
}


//-----------------------------------------------------------------------------
// Purpose: Remove disguise
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveDisguise( void )
{
	// remove quickly
	if ( m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		m_Shared.RemoveDisguise();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveDisguiseWeapon( void )
{
#ifdef GAME_DLL
	if ( m_hDisguiseWeapon )
	{
		m_hDisguiseWeapon->Drop( Vector(0,0,0) );
		m_hDisguiseWeapon = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanDisguise( void )
{
	if ( !IsAlive() )
		return false;

	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY )
		return false;

	bool bHasFlag = false;

	ETFFlagType ignoreTypes[] = { TF_FLAGTYPE_PLAYER_DESTRUCTION };
	if ( HasTheFlag( ignoreTypes, ARRAYSIZE( ignoreTypes ) ) )
	{
		bHasFlag = true;
	}

	if ( bHasFlag || m_Shared.HasPasstimeBall() )
	{
		HintMessage( HINT_CANNOT_DISGUISE_WITH_FLAG );
		return false;
	}

	if ( !Weapon_GetWeaponByType( TF_WPN_TYPE_PDA ) )
		return false;

	int iCannotDisguise = 0;
	CALL_ATTRIB_HOOK_INT( iCannotDisguise, set_cannot_disguise );
	if ( iCannotDisguise == 1 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 'Your Eternal Reward' handling for Disguise testing
//-----------------------------------------------------------------------------
bool CTFPlayer::CanDisguise_OnKill( void )
{
	if ( !IsAlive() )
		return false;

	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY )
		return false;

	CTFKnife *pKnife = dynamic_cast<CTFKnife *>( Weapon_GetWeaponByType( TF_WPN_TYPE_MELEE ) );
	if ( pKnife && pKnife->GetKnifeType() != KNIFE_DISGUISE_ONKILL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayer::GetMaxAmmo( int iAmmoIndex, int iClassIndex /*= -1*/ )
{
	int iMax = ( iClassIndex == -1 ) ? m_PlayerClass.GetData()->m_aAmmoMax[iAmmoIndex] : GetPlayerClassData( iClassIndex )->m_aAmmoMax[iAmmoIndex];
	if ( iAmmoIndex == TF_AMMO_PRIMARY )
	{
		CALL_ATTRIB_HOOK_INT( iMax, mult_maxammo_primary );
	}
	else if ( iAmmoIndex == TF_AMMO_SECONDARY )
	{
		CALL_ATTRIB_HOOK_INT( iMax, mult_maxammo_secondary );
	}
	else if ( iAmmoIndex == TF_AMMO_METAL )
	{
		CALL_ATTRIB_HOOK_INT( iMax, mult_maxammo_metal );
	}
	else if ( iAmmoIndex == TF_AMMO_GRENADES1 )
	{
		CALL_ATTRIB_HOOK_INT( iMax, mult_maxammo_grenades1 );
	}
	else if ( iAmmoIndex == TF_AMMO_GRENADES3 )
	{
		// All classes by default can carry a max of 1 "Grenade3" which is being used as ACTIONSLOT Throwables
		iMax = 1;
	}

	// Haste Powerup Rune adds multiplier to Max Ammo
	if ( m_Shared.GetCarryingRuneType() == RUNE_HASTE )
	{
		iMax *= 2.0f;
	}

	return iMax;
}

bool CTFPlayer::IsMiniBoss( void ) const
{
	return m_bIsMiniBoss;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::MedicGetHealTarget( void )
{
	if ( IsPlayerClass(TF_CLASS_MEDIC) )
	{
		CWeaponMedigun *pWeapon = dynamic_cast <CWeaponMedigun*>( GetActiveWeapon() );

		if ( pWeapon )
			return pWeapon->GetHealTarget();
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::MedicGetChargeLevel( CTFWeaponBase **pRetMedigun )
{
	if ( IsPlayerClass(TF_CLASS_MEDIC) )
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

		if ( pWpn == NULL )
			return 0;

		CWeaponMedigun *pMedigun = dynamic_cast <CWeaponMedigun*>( pWpn );

		if ( pRetMedigun )
		{
			*pRetMedigun = pMedigun;
		}

		if ( pMedigun )
			return pMedigun->GetChargeLevel();
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumActivePipebombs( void )
{
	if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		CTFPipebombLauncher *pWeapon = dynamic_cast < CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );

		if ( pWeapon )
		{
			return pWeapon->GetPipeBombCount();
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Fills out the vector with the sets that are currently active on this player
//-----------------------------------------------------------------------------
void CTFPlayer::GetActiveSets( CUtlVector<const CEconItemSetDefinition *> *pItemSets )
{
	pItemSets->Purge();

	CSteamID steamIDForPlayer;
	GetSteamID( &steamIDForPlayer );

	TFInventoryManager()->GetActiveSets( pItemSets, steamIDForPlayer, GetPlayerClass()->GetClassIndex() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanMoveDuringTaunt()
{

	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
	{
		if ( ( TFGameRules()->GetRoundRestartTime() > -1.f ) && ( (int)( TFGameRules()->GetRoundRestartTime() - gpGlobals->curtime ) <= mp_tournament_readymode_countdown.GetInt() ) )
			return false;

		if ( TFGameRules()->PlayersAreOnMatchSummaryStage() )
			return false;
	}

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return true;

	if ( m_Shared.InCond( TF_COND_TAUNTING ) || m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
	{
#ifdef GAME_DLL
		if ( tf_allow_sliding_taunt.GetBool() )
		{
			return true;
		}
#endif // GAME_DLL


		if ( m_bAllowMoveDuringTaunt )
		{
			return true;
		}

		if ( IsReadyToTauntWithPartner() || CTFPlayerSharedUtils::ConceptIsPartnerTaunt( m_Shared.m_iTauntConcept ) )
		{
			return false;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldStopTaunting()
{
	// stop taunt if we're under water
	if ( GetWaterLevel() > WL_Waist )
		return true;

	if ( IsViewingCYOAPDA() )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ParseSharedTauntDataFromEconItemView( const CEconItemView *pEconItemView )
{
	static CSchemaAttributeDefHandle pAttrDef_TauntForceMoveForward( "taunt force move forward" );
	attrib_value_t attrTauntForceMoveForward = 0;
	pEconItemView->FindAttribute( pAttrDef_TauntForceMoveForward, &attrTauntForceMoveForward );
	m_bTauntForceMoveForward = attrTauntForceMoveForward != 0; 

	static CSchemaAttributeDefHandle pAttrDef_TauntMoveSpeed( "taunt move speed" );
	attrib_value_t attrTauntMoveSpeed = 0;
	pEconItemView->FindAttribute( pAttrDef_TauntMoveSpeed, &attrTauntMoveSpeed );
	m_flTauntForceMoveForwardSpeed = (float&)attrTauntMoveSpeed;

	static CSchemaAttributeDefHandle pAttrDef_TauntMoveAccelerationTime( "taunt move acceleration time" );
	attrib_value_t attrTauntMoveAccelerationTime = 0;
	pEconItemView->FindAttribute( pAttrDef_TauntMoveAccelerationTime, &attrTauntMoveAccelerationTime );
	m_flTauntMoveAccelerationTime = (float&)attrTauntMoveAccelerationTime;

	static CSchemaAttributeDefHandle pAttrDef_TauntTurnSpeed( "taunt turn speed" );
	attrib_value_t attrTauntTurnSpeed = 0;
	pEconItemView->FindAttribute( pAttrDef_TauntTurnSpeed, &attrTauntTurnSpeed );
	m_flTauntTurnSpeed = (float&)attrTauntTurnSpeed;

	static CSchemaAttributeDefHandle pAttrDef_TauntTurnAccelerationTime( "taunt turn acceleration time" );
	attrib_value_t attrTauntTurnAccelerationTime = 0;
	pEconItemView->FindAttribute( pAttrDef_TauntTurnAccelerationTime, &attrTauntTurnAccelerationTime );
	m_flTauntTurnAccelerationTime = (float&)attrTauntTurnAccelerationTime;

#ifdef CLIENT_DLL
	CTFTauntInfo *pTauntInfo = pEconItemView->GetStaticData()->GetTauntData();
	if ( pTauntInfo )
	{
		if ( pTauntInfo->GetCameraDist() != 0 )
			m_flTauntCamTargetDist = pTauntInfo->GetCameraDist();

		if ( pTauntInfo->GetCameraDistUp() != 0 )
			m_flTauntCamTargetDistUp = pTauntInfo->GetCameraDistUp();

	}
#endif // CLIENT_DLL
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetTauntYaw( float flTauntYaw )
{
	m_flPrevTauntYaw = m_flTauntYaw;
	m_flTauntYaw = flTauntYaw;

	QAngle angle = GetLocalAngles();
	angle.y = flTauntYaw;
	SetLocalAngles( angle );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StartBuildingObjectOfType( int iType, int iMode )
{
	// early out if we can't build this type of object
	if ( CanBuild( iType, iMode ) != CB_CAN_BUILD )
		return;

	// Does this type require a specific builder?
	CTFWeaponBuilder *pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, iType );
	if ( pBuilder )
	{
#ifdef GAME_DLL
		pBuilder->SetSubType( iType );
		pBuilder->SetObjectMode( iMode );


		if ( GetActiveTFWeapon() == pBuilder )
		{
			SetActiveWeapon( NULL );
		}	
#endif

		// try to switch to this weapon
		Weapon_Switch( pBuilder );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdatePhaseEffects( void )
{

	bool bRunning;
	float flSpeed = m_pOuter->MaxSpeed();
	flSpeed *= flSpeed;

	CTFPlayer *pPlayer = ToTFPlayer( m_pOuter );
	if ( InCond( TF_COND_SHIELD_CHARGE ) || (pPlayer->GetAbsVelocity().LengthSqr() >= (flSpeed* 0.1f)) )
	{
		bRunning = true;
	}
	else
	{
		bRunning = false;
	}

#ifdef CLIENT_DLL
	if ( m_pOuter )
	{
		if ( !bRunning && !m_pOuter->m_pPhaseStandingEffect && m_flEnergyDrinkMeter < 100.0f )
		{
			m_pOuter->m_pPhaseStandingEffect = m_pOuter->ParticleProp()->Create( "warp_version", PATTACH_ABSORIGIN_FOLLOW );
		}
		else if ( bRunning && m_pOuter->m_pPhaseStandingEffect )
		{
			m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pPhaseStandingEffect );
			m_pOuter->m_pPhaseStandingEffect = NULL;
		}
	}
#else

// #ifdef STAGING_ONLY
// 	if ( !InCond( TF_COND_PHASE ) && !InCond( TF_COND_SHIELD_CHARGE ) && !InCond( TF_COND_SELF_CONC ) )
// #else
	if ( !InCond( TF_COND_PHASE ) && !InCond( TF_COND_SHIELD_CHARGE ) )
//#endif // STAGING_ONLY
		return;

	if ( bRunning )
	{
		if ( !m_bPhaseFXOn )
		{
			AddPhaseEffects();
		}
		else
		{
			if ( m_flEnergyDrinkMeter <= 10.0f )
			{
				float fAlpha = ( m_flEnergyDrinkMeter / 10 ) * 255;
				for ( int i = 0; i < TF_SCOUT_NUMBEROFPHASEATTACHMENTS; ++i )
				{
					CSpriteTrail *pTempTrail = dynamic_cast< CSpriteTrail*>( m_pPhaseTrail[i].Get() );

					if ( pTempTrail )
					{
						pTempTrail->SetBrightness( int(fAlpha) );
					}
				}
			}
// #ifdef STAGING_ONLY
// 			else if ( InCond( TF_COND_SHIELD_CHARGE ) || InCond( TF_COND_SELF_CONC ) )
// #else
			else if ( InCond( TF_COND_SHIELD_CHARGE ) )
//#endif // STAGING_ONLY
			{
				for ( int i = 0; i < TF_SCOUT_NUMBEROFPHASEATTACHMENTS; ++i )
				{
					CSpriteTrail *pTempTrail = dynamic_cast< CSpriteTrail*>( m_pPhaseTrail[i].Get() );

					if ( pTempTrail )
					{
						pTempTrail->SetBrightness( 0 );
					}
				}
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddPhaseEffects( void )
{
#ifdef CLIENT_DLL

#else

	const char *pTrailTeamName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "effects/beam001_red.vmt" : "effects/beam001_blu.vmt";

	CSpriteTrail *pTempTrail = NULL;

	pTempTrail = CSpriteTrail::SpriteTrailCreate( pTrailTeamName, m_pOuter->GetAbsOrigin(), true );
	pTempTrail->FollowEntity( m_pOuter );
	pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
	pTempTrail->SetStartWidth( 12 );
	pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
	pTempTrail->SetLifeTime( 1 );
	pTempTrail->TurnOn();
	pTempTrail->SetAttachment( m_pOuter, m_pOuter->LookupAttachment( "back_upper" ) );

	m_pPhaseTrail[0] = pTempTrail;

	pTempTrail = CSpriteTrail::SpriteTrailCreate( pTrailTeamName, m_pOuter->GetAbsOrigin(), true );
	pTempTrail->FollowEntity( m_pOuter );
	pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
	pTempTrail->SetStartWidth( 16 );
	pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
	pTempTrail->SetLifeTime( 1 );
	pTempTrail->TurnOn();
	pTempTrail->SetAttachment( m_pOuter, m_pOuter->LookupAttachment( "back_lower" ) );

	m_pPhaseTrail[1] = pTempTrail;

	pTempTrail = CSpriteTrail::SpriteTrailCreate( "effects/beam001_white.vmt", m_pOuter->GetAbsOrigin(), true );
	pTempTrail->FollowEntity( m_pOuter );
	pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
	pTempTrail->SetStartWidth( 8 );
	pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
	pTempTrail->SetLifeTime( 0.5f );
	pTempTrail->TurnOn();
	pTempTrail->SetAttachment( m_pOuter, m_pOuter->LookupAttachment( "foot_R" ) );

	m_pPhaseTrail[2] = pTempTrail;

	pTempTrail = CSpriteTrail::SpriteTrailCreate( "effects/beam001_white.vmt", m_pOuter->GetAbsOrigin(), true );
	pTempTrail->FollowEntity( m_pOuter );
	pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
	pTempTrail->SetStartWidth( 8 );
	pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
	pTempTrail->SetLifeTime( 0.5f );
	pTempTrail->TurnOn();
	pTempTrail->SetAttachment( m_pOuter, m_pOuter->LookupAttachment( "foot_L" ) );

	m_pPhaseTrail[3] = pTempTrail;

	pTempTrail = CSpriteTrail::SpriteTrailCreate( pTrailTeamName, m_pOuter->GetAbsOrigin(), true );
	pTempTrail->FollowEntity( m_pOuter );
	pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
	pTempTrail->SetStartWidth( 8 );
	pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
	pTempTrail->SetLifeTime( 0.5f );
	pTempTrail->TurnOn();
	pTempTrail->SetAttachment( m_pOuter, m_pOuter->LookupAttachment( "hand_L" ) );

	m_pPhaseTrail[4] = pTempTrail;


	m_bPhaseFXOn = true;
	
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemovePhaseEffects( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pPhaseStandingEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pPhaseStandingEffect );
		m_pOuter->m_pPhaseStandingEffect = NULL;
	}

#else
	for ( int i = 0; i < TF_SCOUT_NUMBEROFPHASEATTACHMENTS; ++i )
	{
		UTIL_Remove(m_pPhaseTrail[i]);
	}
	m_bPhaseFXOn = false;
	
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetSequenceForDeath( CBaseAnimating* pRagdoll, bool bBurning, int nCustomDeath )
{
	if ( !pRagdoll )
		return -1;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( m_pOuter && ( m_pOuter->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
			return -1;
	}

	int iDeathSeq = -1;
// 	if ( bBurning )
// 	{
// 		iDeathSeq = pRagdoll->LookupSequence( "primary_death_burning" );
// 	}

	switch ( nCustomDeath )
	{
	case TF_DMG_CUSTOM_HEADSHOT_DECAPITATION:
	case TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING:
	case TF_DMG_CUSTOM_DECAPITATION:
	case TF_DMG_CUSTOM_HEADSHOT:
		iDeathSeq = pRagdoll->LookupSequence( "primary_death_headshot" );
		break;
	case TF_DMG_CUSTOM_BACKSTAB:
		iDeathSeq = pRagdoll->LookupSequence( "primary_death_backstab" );
		break;
	}

	return iDeathSeq;
}

extern ConVar tf_halloween_kart_dash_speed;
ConVar tf_halloween_kart_slow_turn_accel_speed( "tf_halloween_kart_slow_turn_accel_speed", "200", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_fast_turn_accel_speed( "tf_halloween_kart_fast_turn_accel_speed", "400", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_return_turn_accell( "tf_halloween_kart_return_turn_accell", "200", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_slow_turn_speed( "tf_halloween_kart_slow_turn_speed", "100", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_fast_turn_speed( "tf_halloween_kart_fast_turn_speed", "60", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_turning_curve_peak_position( "tf_halloween_kart_turning_curve_peak_position", "0.5", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_stationary_turn_speed( "tf_halloween_kart_stationary_turn_speed", "50", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_reverse_turn_speed( "tf_halloween_kart_reverse_turn_speed", "50", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_halloween_kart_air_turn_scale( "tf_halloween_kart_air_turn_scale", "1.2f", FCVAR_CHEAT | FCVAR_REPLICATED );

#ifdef CLIENT_DLL
ConVar tf_halloween_kart_pitch( "tf_halloween_kart_pitch", "10", FCVAR_ARCHIVE );
ConVar tf_halloween_kart_pitch_slow_follow_rate( "tf_halloween_kart_pitch_slow_follow_rate", "0.5", FCVAR_ARCHIVE );
ConVar tf_halloween_kart_pitch_fast_follow_rate( "tf_halloween_kart_pitch_fast_follow_rate", "2", FCVAR_ARCHIVE );
#endif // CLIENT_DLL

void CTFPlayerShared::CreateVehicleMove( float flInputSampleTime, CUserCmd *pCmd )
{
	const float flSign = pCmd->sidemove == 0.f ? 0.f : Sign( pCmd->sidemove );

	// Compute target turn speed
	const float flVel = m_pOuter->GetAbsVelocity().Length2D();
	const float flNormalizedSpeed = Clamp( flVel / tf_halloween_kart_dash_speed.GetFloat(), 0.0f, 1.0f );
	float flTargetTurnSpeed;
	if ( flNormalizedSpeed == 0.f )
	{
		flTargetTurnSpeed = flSign * tf_halloween_kart_stationary_turn_speed.GetFloat();
	}
	else if ( Sign( m_pOuter->GetCurrentTauntMoveSpeed() ) < 0 )
	{
		flTargetTurnSpeed = Sign( m_pOuter->GetCurrentTauntMoveSpeed() ) * flSign * tf_halloween_kart_reverse_turn_speed.GetFloat();
	}
	else
	{
		const float flSmoothCurveVal = SmoothCurve_Tweak( flNormalizedSpeed, tf_halloween_kart_turning_curve_peak_position.GetFloat() );
		flTargetTurnSpeed = Sign( m_pOuter->GetCurrentTauntMoveSpeed() ) * flSign * RemapValClamped( flSmoothCurveVal, 0.f, 1.f, tf_halloween_kart_slow_turn_speed.GetFloat(), tf_halloween_kart_fast_turn_speed.GetFloat() );
	}

	float flTurnAccel = 0.f;
	// Compute turn accelleration
	if ( flSign == Sign( m_flCurrentTauntTurnSpeed ) )
	{
		flTurnAccel = RemapValClamped( flNormalizedSpeed, 0.f, 1.f, tf_halloween_kart_slow_turn_accel_speed.GetFloat(), tf_halloween_kart_fast_turn_accel_speed.GetFloat() );
	}
	else
	{	// When not trying to turn, or turning the opposite way you're already turning
		// accelerate much faster
		flTurnAccel = tf_halloween_kart_return_turn_accell.GetFloat();
	}

	// Turn faster in the air
	if ( !(m_pOuter->GetFlags() & FL_ONGROUND) )
	{
		flTurnAccel *= tf_halloween_kart_air_turn_scale.GetFloat();
	}

	// Get actual turn speed
	m_flCurrentTauntTurnSpeed = Approach( flTargetTurnSpeed, m_flCurrentTauntTurnSpeed, flTurnAccel * flInputSampleTime );

	const float flMaxPossibleTurnSpeed = Max( tf_halloween_kart_slow_turn_speed.GetFloat(), tf_halloween_kart_fast_turn_speed.GetFloat() );
	m_flCurrentTauntTurnSpeed = clamp( m_flCurrentTauntTurnSpeed, -flMaxPossibleTurnSpeed, flMaxPossibleTurnSpeed );

#ifdef DEBUG
	#ifdef CLIENT_DLL
		engine->Con_NPrintf( 4, "Turn: %3.2f", m_flCurrentTauntTurnSpeed );
		engine->Con_NPrintf( 5, "TargetTurn: %3.2f", flTargetTurnSpeed );
		engine->Con_NPrintf( 6, "TurnAccell: %3.2f", flTurnAccel );
	#else
		engine->Con_NPrintf( 4+3, "Turn: %3.2f", m_flCurrentTauntTurnSpeed );
		engine->Con_NPrintf( 5+3, "TargetTurn: %3.2f", flTargetTurnSpeed );
		engine->Con_NPrintf( 6+3, "TurnAccell: %3.2f", flTurnAccel );
	#endif
#endif

#ifdef CLIENT_DLL
	// Turn!
	m_angVehicleMoveAngles -= QAngle( 0.f, m_flCurrentTauntTurnSpeed * flInputSampleTime, 0.f );

	// We want our pitch to slowly catch up to the pitch of the player's model
	const float flTargetPitch = tf_halloween_kart_pitch.GetFloat() + m_pOuter->m_PlayerAnimState->GetRenderAngles()[PITCH];
	const float flStepSpeed = fabs( flTargetPitch - tf_halloween_kart_pitch.GetFloat() ) < fabs( m_angVehicleMovePitchLast - tf_halloween_kart_pitch.GetFloat() ) ? tf_halloween_kart_pitch_fast_follow_rate.GetFloat() : tf_halloween_kart_pitch_slow_follow_rate.GetFloat();
	const float flPitchDiff = fabs( flTargetPitch - m_angVehicleMovePitchLast );
	const float flPitchStep = flPitchDiff * flStepSpeed;

	m_angVehicleMovePitchLast = Approach( flTargetPitch, m_angVehicleMovePitchLast, flPitchStep * gpGlobals->frametime );

	m_angVehicleMoveAngles[PITCH] = m_angVehicleMovePitchLast;
	pCmd->weaponselect = 0;
	pCmd->buttons &= IN_MOVELEFT | IN_MOVERIGHT | IN_ATTACK | IN_ATTACK2 | IN_FORWARD | IN_BACK | IN_JUMP;
	VectorCopy( m_angVehicleMoveAngles, pCmd->viewangles );

	m_pOuter->SetLocalAngles( m_angVehicleMoveAngles );

	// Fill out our kart state for the local client
	m_pOuter->m_iKartState = 0;

	// Hitting the gas
	if ( pCmd->buttons & IN_FORWARD )
	{
		m_pOuter->m_iKartState |= CTFPlayerShared::kKartState_Driving;
	}
	else if ( pCmd->buttons & IN_BACK )	// Hitting the brakes
	{
		// slowing down
		if ( m_pOuter->GetCurrentTauntMoveSpeed() > 0 )
		{
			m_pOuter->m_iKartState |= CTFPlayerShared::kKartState_Braking;
		}
		// if we are already stopped, look for new input to start going backwards
		else 
		{
			// check for new input, else do nothing
			if ( ( pCmd->buttons & IN_BACK )
				|| m_pOuter->GetCurrentTauntMoveSpeed() < 0
				|| m_pOuter->GetVehicleReverseTime() < gpGlobals->curtime
			) 
			{

				m_pOuter->m_iKartState |= CTFPlayerShared::kKartState_Reversing;
			}
			else
			{
				m_pOuter->m_iKartState |= CTFPlayerShared::kKartState_Stopped;
			}
		}
	}
#endif
}

void CTFPlayerShared::VehicleThink( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateKartSounds();

	// Ordered list of effects.  Lower on the list has higher prescedence
	static WheelEffect_t wheelEffects[] =
	{
		WheelEffect_t( 20.f, "kart_dust_trail_red", "kart_dust_trail_blue" )
	};

	const float flCurrentSpeed = m_pOuter->GetCurrentTauntMoveSpeed();
	const WheelEffect_t* pDesiredEffect = NULL;

	if ( InCond( TF_COND_HALLOWEEN_KART ) )
	{
		// Go through the effects, and figure out which effect to use
		for( int i=0; i < ARRAYSIZE(wheelEffects); ++ i )
		{
			const WheelEffect_t& effect = wheelEffects[ i ];
			if ( effect.m_flMinTriggerSpeed <= flCurrentSpeed )
			{
				pDesiredEffect = &effect;
			}
		}
	}


	// Start/stop effects if the desired effect is different
	if ( pDesiredEffect != m_pWheelEffect )
	{
		C_BaseAnimating * pKart = m_pOuter->GetKart();
		if ( !pKart )
			return;

		m_pWheelEffect = pDesiredEffect;

		// New effect
		if ( pDesiredEffect )
		{
			const char *pszEffectName =  pDesiredEffect->m_pszParticleName[ m_pOuter->GetTeamNumber() ];
			m_pOuter->CreateKartEffect( pszEffectName );
		}
		else // Turn off current effect
		{
			m_pOuter->StopKartEffect();
		}
	}
#endif
}

//-----------------------------------------------------------------------------
float CTFPlayer::GetKartSpeedBoost( void )
{
	if ( m_flKartNextAvailableBoost < gpGlobals->curtime )
		return 1.0f;

	if ( m_flKartNextAvailableBoost > gpGlobals->curtime + tf_halloween_kart_boost_recharge.GetFloat() )
		return 0.0f;

	// Calculate time
	return RemapValClamped( gpGlobals->curtime, m_flKartNextAvailableBoost - tf_halloween_kart_boost_recharge.GetFloat(), m_flKartNextAvailableBoost, 0.0f, 1.0f );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsLoser( void )
{
	if ( tf_always_loser.GetBool() )
		return true;

	if ( !TFGameRules() )
		return false;

	// No loser mode in competitive
	if ( TFGameRules()->IsMatchTypeCompetitive() )
		return false;

	if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
	{
		if ( IsLoserStateStunned() )
			return true;
		else
			return false; 
	}

	bool bLoser = TFGameRules()->GetWinningTeam() != m_pOuter->GetTeamNumber();

	int iClass = m_pOuter->GetPlayerClass()->GetClassIndex();

	// don't reveal disguised spies
	if ( bLoser && iClass == TF_CLASS_SPY )
	{
		if ( InCond( TF_COND_DISGUISED ) && GetDisguiseTeam() == TFGameRules()->GetWinningTeam() )
		{
			bLoser = false;
		}
	}

	return bLoser;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculatePlayerBodygroups( void )
{
	// We have to clear the m_nBody bitfield.
	// Leaving bits on from previous player classes can have weird effects
	// like if we switch to a class that uses those bits for other things.
	m_pOuter->m_nBody = 0;

	// Update our weapon bodygroups that change state purely based on whether they're
	// equipped or not.
	CTFWeaponBase::UpdateWeaponBodyGroups( m_pOuter, false );

	// Update our wearable bodygroups.
	CEconWearable::UpdateWearableBodyGroups( m_pOuter );

	// Update our weapon bodygroups for weapons that only change state when active.
	CTFWeaponBase::UpdateWeaponBodyGroups( m_pOuter, true );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::GetSpeedWatchersList( CUtlVector<CTFPlayer *> *out_pVecSpeedWatchers ) const
{
	Assert( out_pVecSpeedWatchers != NULL );

	// Are any medics healing us with the Quick-Fix?
	FOR_EACH_VEC( m_aHealers, i )
	{
		CTFPlayer *pTFHealer = ToTFPlayer( m_aHealers[i].pHealer );
		if ( !pTFHealer )
			continue;

		if ( !pTFHealer->GetActiveTFWeapon() || pTFHealer->GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_MEDIGUN )
			continue;

		// QuickFix medics heal themselves when deploying an Uber
		if ( m_aHealers[i].pHealer == m_pOuter )
			continue;

		out_pVecSpeedWatchers->AddToTail( pTFHealer );
	}
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetupRageBuffTimer( int iBuffType, int iPulseCount, ERageBuffSlot eBuffSlot )
{
	m_RageBuffSlots[eBuffSlot].m_iBuffTypeActive = iBuffType;
	m_RageBuffSlots[eBuffSlot].m_iBuffPulseCount = iPulseCount;
	m_RageBuffSlots[eBuffSlot].m_flNextBuffPulseTime = gpGlobals->curtime + 1.0f;

	PulseRageBuff( eBuffSlot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ActivateRageBuff( CBaseEntity *pBuffItem, int iBuffType )
{
	// Sniper Focus can be activated at all times
	if ( GetRageMeter() < 100.f && iBuffType != 6 )
		return;

	Assert( iBuffType > 0 && iBuffType < ARRAYSIZE( g_RageBuffTypes ) );	// 0 is valid in the array, but an invalid buff
	if ( iBuffType < 0 || iBuffType >= ARRAYSIZE( g_RageBuffTypes ) )
	{
		DevMsg( "Invalid rage buff type %i for entindex %i\n", iBuffType, m_pOuter->entindex() );
		ResetRageSystem();
		return;
	}

	int nBuffPulses = g_RageBuffTypes[iBuffType].m_nMaxPulses;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, nBuffPulses, mod_buff_duration );

#ifdef GAME_DLL
	switch ( iBuffType )
	{
	case 1:
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
		break;
	case 2:
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_INCOMING );
		break;
	case 3:
		// FIXME: new sound file for samurai buff?
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
	case 5:
		// Pyro Rage
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
		break;
	case 6 :
		// Sniper Focus
		m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
		nBuffPulses *= (m_flRageMeter / 100);
		break;
	}
#endif
	
	m_bRageDraining = true;
	SetupRageBuffTimer( iBuffType, nBuffPulses, kBuffSlot_Rage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateRageBuffsAndRage( void )
{
	// We allow this for all classes to allow item creators and plugin authors to give rage to any class.

	// If we're dead, reset both our rage and our active buffs.
	if ( !m_pOuter->IsAlive() )
	{
		ResetRageSystem();
		return;
	}

	// Find out whether we've run out of rage.
	if ( m_bRageDraining )
	{
		int nBuffType = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, nBuffType, set_buff_type );

		Assert( nBuffType >= 0 && nBuffType < ARRAYSIZE( g_RageBuffTypes ) );	// 0 is valid in the array, but an invalid buff
		if ( nBuffType < 0 || nBuffType >= ARRAYSIZE( g_RageBuffTypes ) )
		{
			DevMsg( "Invalid rage buff type %i for entindex %i\n", nBuffType, m_pOuter->entindex() );
			ResetRageSystem();
			return;
		}

		int nBuffPulses = g_RageBuffTypes[nBuffType].m_nMaxPulses;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, nBuffPulses, mod_buff_duration );
		if ( nBuffPulses > 0 )
		{
			m_flRageMeter -= gpGlobals->frametime * ( 100.f / (float)nBuffPulses );
			if ( m_flRageMeter <= 0.0f )
			{
				m_flRageMeter = 0.0f;
				m_bRageDraining = false;

				if ( g_SoldierBuffAttributeIDToConditionMap[ nBuffType ] == TF_COND_CRITBOOSTED_RAGE_BUFF )
				{
					// Pyro rage needs a cooldown so that the final crit flames
					// don't significantly fill up his next rage meter
					m_flNextRageEarnTime = gpGlobals->curtime + tf_flamethrower_flametime.GetFloat() + 0.1f;
				}
			}
		}
		else
		{
			ResetRageSystem();
		}
	}
	

	// Handle pulsing all of our active rage buffs.
	for ( int i = 0; i < ARRAYSIZE( m_RageBuffSlots ); i++ )
	{
		RageBuff& rageBuff = m_RageBuffSlots[i];

		if ( gpGlobals->curtime > rageBuff.m_flNextBuffPulseTime && rageBuff.m_iBuffPulseCount > 0 )
		{
			rageBuff.m_flNextBuffPulseTime += 1.0f;
			--rageBuff.m_iBuffPulseCount;
			PulseRageBuff( (ERageBuffSlot)i );
		}
	}
}

static const int k_RageBuffType_Sniper = 6;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetRageMeter( float val )
{
	// Allow Sniper to gain rage on kills even when buffed
	if ( !InCond( TF_COND_SNIPERCHARGE_RAGE_BUFF ) && !m_pOuter->IsPlayerClass( TF_CLASS_SPY ) ) 
	{
		if ( IsRageDraining() )
			return;

		// Can't earn rage until the time is past this delay
		if ( val > m_flRageMeter && gpGlobals->curtime < m_flNextRageEarnTime )
			return;
	}

	m_flRageMeter = MIN( val, 100.0f );

	if ( InCond( TF_COND_SNIPERCHARGE_RAGE_BUFF ) )
	{	
		Assert( k_RageBuffType_Sniper > 0 && k_RageBuffType_Sniper < ARRAYSIZE( g_RageBuffTypes ) );	// 0 is valid in the array, but an invalid buff
		if ( k_RageBuffType_Sniper < 0 || k_RageBuffType_Sniper >= ARRAYSIZE( g_RageBuffTypes ) )
			return;

		int nBuffPulses = g_RageBuffTypes[k_RageBuffType_Sniper].m_nMaxPulses;
		m_bRageDraining = true;

		nBuffPulses *= (m_flRageMeter / 100);

		m_RageBuffSlots[kBuffSlot_Rage].m_iBuffPulseCount = nBuffPulses;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ModifyRage( float fDelta )
{
	SetRageMeter( GetRageMeter() + fDelta );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ResetRageMeter( void )
{
	m_flRageMeter = 0.0f;
	m_flNextRageEarnTime = 0.0f;

	ResetRageBuffs();
	UpdateRageBuffsAndRage();
}

//-----------------------------------------------------------------------------
// Purpose: Apply the buff effect to everyone within a radius around the player.
//-----------------------------------------------------------------------------
void CTFPlayerShared::PulseRageBuff( ERageBuffSlot eBuffSlot )
{
	Assert( m_RageBuffSlots[eBuffSlot].m_iBuffTypeActive != 0 );

#ifdef CLIENT_DLL
	// if this is not the local player, we don't want to do anything
	if ( !m_pOuter->IsLocalPlayer() )
		return;

	int nBuffedFriends = 0;
#else
	int nBuffedPlayers = 0;
#endif

	int iSoldierBuffType = m_RageBuffSlots[eBuffSlot].m_iBuffTypeActive;
	ETFCond eBuffCond = TF_COND_LAST;
	if ( iSoldierBuffType > 0 && iSoldierBuffType <= kSoldierBuffCount )
	{
		eBuffCond = g_SoldierBuffAttributeIDToConditionMap[iSoldierBuffType];
	}

	float fMaxRadius = 450.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_pOuter, fMaxRadius, mod_soldier_buff_range );
	const float fMaxRadiusSq = fMaxRadius * fMaxRadius;

	for( int iPlayerIndex=1; iPlayerIndex<=MAX_PLAYERS; ++iPlayerIndex )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( !pTFPlayer || !pTFPlayer->IsAlive() )
			continue;

		if ( pTFPlayer->GetTeamNumber() != m_pOuter->GetTeamNumber() )
			continue;

		if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && (pTFPlayer->m_Shared.GetDisguiseTeam() != m_pOuter->GetTeamNumber()) )
			continue; // For now don't give the buff to spies on our team who are disguised as enemies.

		if ( pTFPlayer->m_Shared.IsStealthed() )
			continue; // Don't give the buff to cloaked spies

		Vector vDist = pTFPlayer->GetAbsOrigin() - m_pOuter->GetAbsOrigin();
		if ( vDist.LengthSqr() > fMaxRadiusSq )
			continue;

#ifdef CLIENT_DLL
		if ( pTFPlayer != m_pOuter )
		{
			if ( eBuffCond == TF_COND_CRITBOOSTED_RAGE_BUFF || eBuffCond == TF_COND_SNIPERCHARGE_RAGE_BUFF )
			{
				// Pyro and sniper only buffs themselves
				continue;
			}

			// this is not the localplayer, are they a friend?
			if ( !steamapicontext->SteamFriends() || !steamapicontext->SteamUtils() )
				return;

			player_info_t pi;
			if ( !engine->GetPlayerInfo( pTFPlayer->entindex(), &pi ) )
				return;

			if ( !pi.friendsID )
				return;

			// check and see if they're on the local player's friends list
			CSteamID steamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
			if ( steamapicontext->SteamFriends()->HasFriend( steamID, k_EFriendFlagImmediate ) )
			{
				nBuffedFriends++;
			}
		}
#else
		if ( pTFPlayer != m_pOuter )
		{
			if ( eBuffCond == TF_COND_CRITBOOSTED_RAGE_BUFF || eBuffCond == TF_COND_SNIPERCHARGE_RAGE_BUFF )
			{
				// Pyro and sniper only buffs themselves
				continue;
			}
		}

		if ( eBuffCond != TF_COND_LAST )
		{
			pTFPlayer->m_Shared.AddCond( eBuffCond, 1.2f, m_pOuter );

			nBuffedPlayers++;

			IGameEvent* event = gameeventmanager->CreateEvent( "player_buff" );
			if ( event )
			{
				event->SetInt( "userid", pTFPlayer->GetUserID() );
				event->SetInt( "buff_owner", m_pOuter->GetUserID() );
				event->SetInt( "buff_type", iSoldierBuffType );
				gameeventmanager->FireEvent( event );
			}
		}
#endif
	}

#ifdef CLIENT_DLL
	if ( nBuffedFriends >= 5 )
	{
		g_AchievementMgrTF.OnAchievementEvent( ACHIEVEMENT_TF_SOLDIER_BUFF_FRIENDS );
	}
#else
	// ACHIEVEMENT_TF_MVM_SOLDIER_BUFF_TEAM
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( ( m_pOuter->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS ) && m_pOuter->IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			if ( nBuffedPlayers >= 5 )
			{
				m_pOuter->AwardAchievement( ACHIEVEMENT_TF_MVM_SOLDIER_BUFF_TEAM );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::ResetRageSystem( void )
{
	m_flRageMeter = 0.f;
	m_bRageDraining = false;
	m_flNextRageEarnTime = 0.f;

	ResetRageBuffs();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateEnergyDrinkMeter( void )
{
	if ( !m_pOuter->IsPlayerClass( TF_CLASS_SCOUT ) )
		return;

	bool bIsLocalPlayer = false;
#ifdef CLIENT_DLL
	bIsLocalPlayer = m_pOuter->IsLocalPlayer();
#else
	bIsLocalPlayer = true;
#endif

	if ( bIsLocalPlayer )
	{
		if ( IsHypeBuffed() )
		{
			m_flHypeMeter -= gpGlobals->frametime * (m_fEnergyDrinkConsumeRate*0.75f);
			if ( m_flHypeMeter <= 0.0f )
			{
				RemoveCond( TF_COND_SODAPOPPER_HYPE );
			}
		}

		if ( InCond( TF_COND_PHASE ) || InCond( TF_COND_ENERGY_BUFF ) )
		{
			// Drain the meter
			m_flEnergyDrinkMeter -= gpGlobals->frametime * m_fEnergyDrinkConsumeRate;

			// If we've drained the meter, remove the condition
			if ( m_flEnergyDrinkMeter <= 0.f )
			{
				RemoveCond( TF_COND_PHASE );
				RemoveCond( TF_COND_ENERGY_BUFF );

#ifdef GAME_DLL
				m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_TIRED );
#endif
			}
			// Update the effect on phasing only
			else if ( InCond( TF_COND_PHASE ) )
			{
				UpdatePhaseEffects();
			}
		} 
		else if ( m_flEnergyDrinkMeter < 100.0f )
		{
			// Regen the meter
			m_flEnergyDrinkMeter += gpGlobals->frametime * m_fEnergyDrinkRegenRate;

			CTFLunchBox_Drink *pDrink  = static_cast< CTFLunchBox_Drink* >( m_pOuter->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX ) );

			if ( pDrink )
			{
				// This is here in case something replenishes grenades
				if ( m_flEnergyDrinkMeter < 100.0f && m_pOuter->GetAmmoCount( TF_AMMO_GRENADES2 ) == m_pOuter->GetMaxAmmo( TF_AMMO_GRENADES2 ) )
				{
					m_flEnergyDrinkMeter = 100.0f;
				}
			}
			else if ( m_flEnergyDrinkMeter >= 100.0f )
			{
				m_flEnergyDrinkMeter = 100.0f;
			}
		}
	}
}

void CTFPlayerShared::SetScoutHypeMeter( float val )
{
	if ( IsHypeBuffed() )
		return;

	m_flHypeMeter = Clamp(val, 0.0f, 100.0f);
	//if ( m_flHypeMeter >= 100.f )
	//{
	//	if ( m_pOuter->IsPlayerClass( TF_CLASS_SCOUT ) )
	//	{
	//		CTFWeaponBase* pWeapon = m_pOuter->GetActiveTFWeapon();
	//		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_SODA_POPPER )
	//		{
	//			AddCond( TF_COND_CRITBOOSTED_HYPE );
	//		}
	//	}
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCloakMeter( void )
{
	if ( !m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
		return;

	if ( InCond( TF_COND_STEALTHED ) )
	{
		if ( m_bMotionCloak )
		{
			// Motion cloak: drain based on our movement rate.
			Vector vVel = m_pOuter->GetAbsVelocity();
			float fSpdSqr = vVel.LengthSqr();
			if ( fSpdSqr == 0.f )
			{
				if ( gpGlobals->curtime - m_flLastStealthExposeTime > 1.f )
				{
					m_flCloakMeter += gpGlobals->frametime * m_fCloakRegenRate;
					if ( m_flCloakMeter >= 100.0f )
					{
						m_flCloakMeter = 100.0f;
					}
				}
			}
			else
			{
				float fFactor = RemapVal( fSpdSqr, 0, m_pOuter->MaxSpeed()*m_pOuter->MaxSpeed(), 0.f, 1.f );
				if ( fFactor > 1.f )
				{
					fFactor = 1.f;
				}
				m_flCloakMeter -= gpGlobals->frametime * m_fCloakConsumeRate * fFactor * 1.5f;
				if ( m_flCloakMeter < 0.f )
				{
					m_flCloakMeter = 0.f;
				}
			}
		}
		else
		{
			// Classic cloak: drain at a fixed rate.
			m_flCloakMeter -= gpGlobals->frametime * m_fCloakConsumeRate;
		}

		if ( m_flCloakMeter <= 0.0f && !m_bMotionCloak)	
		{
			FadeInvis( 1.0f );
		}

		// Update Debuffs
		// Decrease duration if cloaked
#ifdef GAME_DLL
		// staging_spy
		float flReduction = gpGlobals->frametime * 0.75f;
		for ( int i = 0; g_aDebuffConditions[i] != TF_COND_LAST; i++ )
		{
			if ( InCond( g_aDebuffConditions[i] ) )
			{
				if ( m_ConditionData[g_aDebuffConditions[i]].m_flExpireTime != PERMANENT_CONDITION )
				{			
					m_ConditionData[g_aDebuffConditions[i]].m_flExpireTime = MAX( m_ConditionData[g_aDebuffConditions[i]].m_flExpireTime - flReduction, 0 );
				}
				// Burning and Bleeding and extra timers
				if ( g_aDebuffConditions[i] == TF_COND_BURNING )
				{
					// Reduce the duration of this burn
					m_flAfterburnDuration -= flReduction;
				}
				else if ( g_aDebuffConditions[i] == TF_COND_BLEEDING )
				{
					// Reduce the duration of this bleeding 
					FOR_EACH_VEC( m_PlayerBleeds, i )
					{
						m_PlayerBleeds[i].flBleedingRemoveTime -= flReduction;
					}
				}
			}
		}
#endif
	} 
	else
	{
		m_flCloakMeter += gpGlobals->frametime * m_fCloakRegenRate;

		if ( m_flCloakMeter >= 100.0f )
		{
			m_flCloakMeter = 100.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Whether we should be doing a radius heal (does not stack with a medigun)
//-----------------------------------------------------------------------------
void CTFPlayerShared::Heal_Radius( bool bActive )
{
	if ( bActive )
	{
		m_bPulseRadiusHeal = true;
	}
	else
	{
		m_bPulseRadiusHeal = false;

#ifdef GAME_DLL
		// Stop any Radius healing
		if ( m_iRadiusHealTargets.Count() > 0 )
		{
			for ( int iIndex = 0; iIndex < m_iRadiusHealTargets.Count(); iIndex++ )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( m_iRadiusHealTargets[iIndex] ) );
				if ( !pTFPlayer )
					continue;

				pTFPlayer->m_Shared.StopHealing( m_pOuter );
			}
			m_iRadiusHealTargets.RemoveAll();
		}
#endif	// GAME_DLL

#ifdef CLIENT_DLL
		if ( m_pOuter && m_pOuter->m_pRadiusHealEffect )
		{
			m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pRadiusHealEffect );
		}
		m_pOuter->m_pRadiusHealEffect = NULL;
#endif	// CLIENT_DLL
	}
}

//-----------------------------------------------------------------------------
// Purpose: Emits an area-of-effect heal around the medic
//-----------------------------------------------------------------------------
void CTFPlayerShared::PulseMedicRadiusHeal( void )
{
	if ( !m_bPulseRadiusHeal )
	{
#ifdef GAME_DLL
		Assert( m_iRadiusHealTargets.Count() == 0 );
		if ( m_iRadiusHealTargets.Count() > 0 )
		{
			// We shouldn't have any heal targets if we aren't pulsing.
			Heal_Radius( false );
		}
#endif
		return;
	}

	// If we're set to heal, make sure it's still valid
	if ( !m_pOuter->IsAlive() || ( !m_pOuter->IsPlayerClass( TF_CLASS_MEDIC ) && !InCond( TF_COND_RADIUSHEAL_ON_DAMAGE ) ) )
	{
		Heal_Radius( false );
		return;
	}

#ifdef GAME_DLL
	if ( gpGlobals->curtime >= m_flRadiusHealCheckTime )
	{
		for( int iPlayerIndex = 1; iPlayerIndex <= MAX_PLAYERS; ++iPlayerIndex )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsAlive() )
				continue;

			// Don't heal ourselves, unless this is due to radius heal on damage proc
			if ( pTFPlayer == m_pOuter && !InCond( TF_COND_RADIUSHEAL_ON_DAMAGE ) )
				continue;

			if ( !pTFPlayer->InSameTeam( m_pOuter ) )
			{
				if ( !pTFPlayer->m_Shared.IsStealthed() && !pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					continue;

				if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( pTFPlayer->m_Shared.GetDisguiseTeam() != m_pOuter->GetTeamNumber() ) )
					continue;
			}

			// Don't heal players with weapon_blocks_healing
			CTFWeaponBase *pTFWeapon = pTFPlayer->GetActiveTFWeapon();
			if ( pTFWeapon )
			{
				int iBlockHealing = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFWeapon, iBlockHealing, weapon_blocks_healing );
				if ( iBlockHealing )
					continue;
			}

			Vector vDist = pTFPlayer->GetAbsOrigin() - m_pOuter->GetAbsOrigin();
			if ( vDist.LengthSqr() <= 450 * 450 )
			{
				// Ignore players we can't see
				trace_t	trace;
				UTIL_TraceLine( pTFPlayer->WorldSpaceCenter(), m_pOuter->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trace );

				if ( trace.fraction < 1.0f )
					continue;

				// Refresh this condition, which we use to give players a particle effect
				pTFPlayer->m_Shared.AddCond( TF_COND_RADIUSHEAL, 1.2f );

				// Make sure we're not already healing them
				if ( m_iRadiusHealTargets.Find( iPlayerIndex ) == m_iRadiusHealTargets.InvalidIndex() )
				{
					m_iRadiusHealTargets.AddToTail( iPlayerIndex );
					pTFPlayer->m_Shared.Heal( m_pOuter, 25.f, 1.f, 1.f );
				}
			}
			else
			{
				if ( m_iRadiusHealTargets.Find( iPlayerIndex ) != m_iRadiusHealTargets.InvalidIndex() )
				{
					m_iRadiusHealTargets.FindAndRemove( iPlayerIndex );
					pTFPlayer->m_Shared.StopHealing( m_pOuter );
				}
			}
		}

		m_flRadiusHealCheckTime = gpGlobals->curtime + 1.0f;
	}
#endif	// GAME_DLL

#ifdef CLIENT_DLL
	// Radius healer gets an effect to broadcast to others what they're doing
	if ( !m_pOuter->m_pRadiusHealEffect )
	{
		const char *pszRadiusHealEffect;
		if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED )
		{
			pszRadiusHealEffect = "medic_healradius_red_buffed";
		}
		else
		{
			pszRadiusHealEffect = "medic_healradius_blue_buffed";
		}
		m_pOuter->m_pRadiusHealEffect = m_pOuter->ParticleProp()->Create( pszRadiusHealEffect, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 0 ) );
	}
#endif	// CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Emits an area-of-effect buff around the King Rune carrier
//-----------------------------------------------------------------------------
void CTFPlayerShared::PulseKingRuneBuff( void )
{
	// Make sure we have the King Powerup and are not invisible
	if ( !m_pOuter->IsAlive() || IsStealthed() || GetCarryingRuneType() != RUNE_KING )
	{
		return;
	}

#ifdef GAME_DLL
	if ( gpGlobals->curtime >= m_flKingRuneBuffCheckTime )
	{
		m_bKingRuneBuffActive = false;
		
		// Plague blocks king team buff
 		if ( !InCond( TF_COND_PLAGUE ) )
 		{
			for ( int iPlayerIndex = 1; iPlayerIndex <= MAX_PLAYERS; ++iPlayerIndex )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
				if ( !pTFPlayer || !pTFPlayer->IsAlive() )
					continue;

				// Ignore players outside of the buff radius
				Vector vDist = pTFPlayer->GetAbsOrigin() - m_pOuter->GetAbsOrigin();
				if ( vDist.LengthSqr() >= 768 * 768 )
					continue;

				// If King is the only player, there's no effect
				if ( pTFPlayer == m_pOuter )
					continue;

				// Spies who are invisible or disguised as the King's enemy team are ignored
				if ( pTFPlayer->m_Shared.IsStealthed() || ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pTFPlayer->m_Shared.GetDisguiseTeam() != m_pOuter->GetTeamNumber() ) )
					continue;

				// Enemies - ignore unless they are disguised as the King's team
				if ( !pTFPlayer->InSameTeam( m_pOuter ) && !pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					continue;

				pTFPlayer->m_Shared.AddCond( TF_COND_KING_BUFFED, 1.f );
				m_bKingRuneBuffActive = true;
			}
 		}

		m_flKingRuneBuffCheckTime = gpGlobals->curtime + 0.5f;
	}
#endif	// GAME_DLL

#ifdef CLIENT_DLL
	// King Rune carrier gets an effect to show that he's buffing someone
	if ( m_bKingRuneBuffActive && !InCond( TF_COND_PLAGUE ) )
	{
		if ( !m_pOuter->m_pKingRuneRadiusEffect )
		{
			const char *pszRadiusEffect;
			if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED )
			{
				pszRadiusEffect = "powerup_king_red";
			}
			else
			{
				pszRadiusEffect = "powerup_king_blue";
			}
			m_pOuter->m_pKingRuneRadiusEffect = m_pOuter->ParticleProp()->Create( pszRadiusEffect, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 0 ) );
		}
	}
	else 
	{
		EndKingBuffRadiusEffect();
	}
#endif	// CLIENT_DLL
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::IncrementRevengeCrits( void )
{
	SetRevengeCrits( m_iRevengeCrits + 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetRevengeCrits( int iVal )
{	
	m_iRevengeCrits = clamp( iVal, 0, 35 );

	CTFWeaponBase *pWeapon = m_pOuter->GetActiveTFWeapon();
	if ( ( pWeapon && pWeapon->CanHaveRevengeCrits() ) )
	{
		if ( m_iRevengeCrits > 0 && !InCond( TF_COND_CRITBOOSTED ) )
		{
			AddCond( TF_COND_CRITBOOSTED );
		}
		else if ( m_iRevengeCrits == 0 && InCond( TF_COND_CRITBOOSTED ) )
		{
			RemoveCond( TF_COND_CRITBOOSTED );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::FireGameEvent( IGameEvent *event )
{
#ifdef GAME_DLL
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "player_disconnect" ) )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
		if ( pPlayer )
		{
			int iIndex = m_iRadiusHealTargets.Find( pPlayer->entindex() );
			if ( iIndex != m_iRadiusHealTargets.InvalidIndex() )
			{
				m_iRadiusHealTargets.FastRemove( iIndex );
			}
		}
	}
#endif //GAME_DLL
}

//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPasstimePassTarget( CTFPlayer *pEnt ) 
{ 
	if ( CBaseEntity *pTarget = m_hPasstimePassTarget )
	{
		CTFPlayer *pPlayerTarget = ToTFPlayer( pTarget );
		if ( pPlayerTarget )
			pPlayerTarget->m_Shared.m_bIsTargetedForPasstimePass = false;
	}

	Assert( pEnt != m_pOuter );
	m_hPasstimePassTarget = pEnt; 

	if ( CBaseEntity *pTarget = m_hPasstimePassTarget )
	{
		CTFPlayer *pPlayerTarget = ToTFPlayer( pTarget );
		if ( pPlayerTarget )
			pPlayerTarget->m_Shared.m_bIsTargetedForPasstimePass = true;
	}
}

//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayerShared::GetPasstimePassTarget() const { return m_hPasstimePassTarget.Get(); }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterIgnoreTeammatesAndTeamObjects::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

	if ( pEntity->GetTeamNumber() == m_iIgnoreTeam )
	{
		return false;
	}

	CTFPlayer *pPlayer = dynamic_cast<CTFPlayer*>( pEntity );
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == m_iIgnoreTeam )
			return false;

		if ( pPlayer->m_Shared.IsStealthed() )
			return false;
	}

	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}

void CTFPlayerShared::SetCarriedObject( CBaseObject* pObj )
{
	m_bCarryingObject = (pObj != NULL);
	m_hCarriedObject.Set( pObj ); 
#ifdef GAME_DLL
	if ( m_pOuter )
		m_pOuter->TeamFortress_SetSpeed(); 
#endif
}

void localplayerscoring_t::UpdateStats( RoundStats_t& roundStats, CTFPlayer *pPlayer, bool bIsRoundData )
{
	m_iCaptures = roundStats.m_iStat[TFSTAT_CAPTURES];
	m_iDefenses = roundStats.m_iStat[TFSTAT_DEFENSES];

	m_iKills = roundStats.m_iStat[TFSTAT_KILLS];
	m_iDeaths = roundStats.m_iStat[TFSTAT_DEATHS];
	m_iSuicides = roundStats.m_iStat[TFSTAT_SUICIDES];
	m_iKillAssists = roundStats.m_iStat[TFSTAT_KILLASSISTS];

	m_iBuildingsBuilt = roundStats.m_iStat[TFSTAT_BUILDINGSBUILT];
	m_iBuildingsDestroyed = roundStats.m_iStat[TFSTAT_BUILDINGSDESTROYED];

	m_iHeadshots = roundStats.m_iStat[TFSTAT_HEADSHOTS];
	m_iDominations = roundStats.m_iStat[TFSTAT_DOMINATIONS];
	m_iRevenge = roundStats.m_iStat[TFSTAT_REVENGE];
	m_iInvulns = roundStats.m_iStat[TFSTAT_INVULNS];
	m_iTeleports = roundStats.m_iStat[TFSTAT_TELEPORTS];

	m_iDamageDone = roundStats.m_iStat[TFSTAT_DAMAGE];
	m_iCrits = roundStats.m_iStat[TFSTAT_CRITS];

	m_iBackstabs = roundStats.m_iStat[TFSTAT_BACKSTABS];

	int iHealthPointsHealed = (int) roundStats.m_iStat[TFSTAT_HEALING];
	// send updated healing data every 10 health points, and round off what we send to nearest 10 points
	int iHealPointsDelta = abs( iHealthPointsHealed - m_iHealPoints );
	if ( iHealPointsDelta > 10 )
	{
		m_iHealPoints = ( iHealthPointsHealed / 10 ) * 10;
	}
	m_iBonusPoints = roundStats.m_iStat[TFSTAT_BONUS_POINTS] / TF_SCORE_BONUS_POINT_DIVISOR;
	const int nPoints = TFGameRules()->CalcPlayerScore( &roundStats, pPlayer );
	const int nDelta = nPoints - m_iPoints;
	m_iPoints = nPoints;
	
	if ( nDelta > 0 && !bIsRoundData )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_score_changed" );
		if ( event )
		{
			event->SetInt( "player", pPlayer->entindex() );
			event->SetInt( "delta", nDelta );
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( CTFPlayer *pTFPlayer, int iSlot, CEconEntity **pEntity )
{
	int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();

	// See if it's a weapon first
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)pTFPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		CEconItemView *pEconItemView = pWeapon->GetAttributeContainer()->GetItem();
		if ( !pEconItemView )
			continue;

		int iLoadoutSlot = pEconItemView->GetStaticData()->GetLoadoutSlot( iClass );
		if ( iLoadoutSlot == iSlot )
		{
			if ( pEntity )
			{
				*pEntity = pWeapon;
			}
			return pEconItemView;
		}
	}

	// Go through each of the actual items we have equipped right now...
	for ( int i = 0; i < pTFPlayer->GetNumWearables(); ++i )
	{
		CTFWearable *pWearableItem = dynamic_cast<CTFWearable *>( pTFPlayer->GetWearable( i ) );
		if ( !pWearableItem )
			continue;

		if ( !pWearableItem->GetAttributeContainer() )
			continue;

		CEconItemView *pEconItemView = pWearableItem->GetAttributeContainer()->GetItem();
		if ( !pEconItemView )
			continue;

		CTFItemDefinition *pItemDef = pEconItemView->GetStaticData();
		if ( !pItemDef )
			continue;

		if ( pItemDef->GetLoadoutSlot(iClass) != iSlot )
			continue;

		// Yay!
		if ( pEntity )
		{
			*pEntity = pWearableItem;
		}
		return pEconItemView;
	}

	// Nothing we currently have equipped claims to be in this slot.
	if ( pEntity )
	{
		*pEntity = NULL;
	}
	return NULL;
}

bool CTFPlayerSharedUtils::ConceptIsPartnerTaunt( int iConcept )
{
	return iConcept == MP_CONCEPT_HIGHFIVE_SUCCESS_FULL || iConcept == MP_CONCEPT_HIGHFIVE_SUCCESS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder *CTFPlayerSharedUtils::GetBuilderForObjectType( CTFPlayer *pTFPlayer, int iObjectType )
{
	const int OBJ_ANY = -1;

	if ( !pTFPlayer )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBuilder *pBuilder = dynamic_cast< CTFWeaponBuilder* >( pTFPlayer->GetWeapon( i ) );
		if ( !pBuilder )
			continue;

		// Any builder will do - return first
		if ( iObjectType == OBJ_ANY )
			return pBuilder;

		// Requires a specific builder for this type
		if ( pBuilder->CanBuildObjectType( iObjectType ) )
			return pBuilder;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Can player pick up this weapon?
//-----------------------------------------------------------------------------
bool CTFPlayer::CanPickupDroppedWeapon( const CTFDroppedWeapon *pWeapon )
{
	if ( !pWeapon->GetItem()->IsValid() )
		return false;

	int iClass = GetPlayerClass()->GetClassIndex();
	if ( iClass == TF_CLASS_SPY && ( m_Shared.InCond( TF_COND_DISGUISED ) || m_Shared.GetPercentInvisible() > 0 ) )
		return false;

 	if ( IsTaunting() )
 		return false;

	if ( !IsAlive() )
		return false;

	// There's a rare case that the player doesn't have an active weapon. This shouldn't happen. 
	// If you hit this assert, figure out and fix WHY the player doesn't have a weapon.
	Assert( GetActiveTFWeapon() );
	if ( !GetActiveTFWeapon() || !GetActiveTFWeapon()->CanPickupOtherWeapon() )
		return false;

	int iItemSlot = pWeapon->GetItem()->GetStaticData()->GetLoadoutSlot( iClass );
	CBaseEntity *pOwnedWeaponToDrop = GetEntityForLoadoutSlot( iItemSlot );

	return pOwnedWeaponToDrop && pWeapon->GetItem()->GetStaticData()->CanBeUsedByClass( iClass ) && IsValidPickupWeaponSlot( iItemSlot );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if player is in range to pick up this weapon
//-----------------------------------------------------------------------------
CTFDroppedWeapon* CTFPlayer::GetDroppedWeaponInRange()
{
	// Check to see if a building we own is in front of us.
	Vector vecForward;
	AngleVectors( EyeAngles(), &vecForward );

	trace_t tr;
	UTIL_TraceLine( EyePosition(), EyePosition() + vecForward * TF_WEAPON_PICKUP_RANGE, MASK_SOLID | CONTENTS_DEBRIS, this, COLLISION_GROUP_NONE, &tr );

	CTFDroppedWeapon *pDroppedWeapon = dynamic_cast< CTFDroppedWeapon * >( tr.m_pEnt );
	if ( !pDroppedWeapon )
		return NULL;

	if ( !CanPickupDroppedWeapon( pDroppedWeapon ) )
		return NULL;

	// too far?
	if ( EyePosition().DistToSqr( pDroppedWeapon->GetAbsOrigin() ) > Square( TF_WEAPON_PICKUP_RANGE ) )
		return NULL;

	return pDroppedWeapon;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if player is inspecting
//-----------------------------------------------------------------------------
bool CTFPlayer::IsInspecting() const
{
	return m_flInspectTime != 0.f && gpGlobals->curtime - m_flInspectTime > 0.2f;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if helpme button is pressed
//-----------------------------------------------------------------------------
bool CTFPlayer::IsHelpmeButtonPressed() const
{
	return m_flHelpmeButtonPressTime != 0.f;
}

