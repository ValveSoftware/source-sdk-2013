//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL1.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_gamestats.h"
#include "KeyValues.h"
#include "viewport_panel_names.h"
#include "client.h"
#include "team.h"
#include "tf_weaponbase.h"
#include "tf_client.h"
#include "tf_team.h"
#include "tf_viewmodel.h"
#include "tf_item.h"
#include "in_buttons.h"
#include "entity_capture_flag.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "game.h"
#include "tf_weapon_builder.h"
#include "tf_obj.h"
#include "tf_ammo_pack.h"
#include "datacache/imdlcache.h"
#include "particle_parse.h"
#include "props_shared.h"
#include "filesystem.h"
#include "toolframework_server.h"
#include "IEffects.h"
#include "func_respawnroom.h"
#include "networkstringtable_gamedll.h"
#include "team_control_point_master.h"
#include "tf_weapon_pda.h"
#include "sceneentity.h"
#include "fmtstr.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_minigun.h"
#include "tf_weapon_fists.h"
#include "tf_weapon_shotgun.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_bottle.h"
#include "tf_weapon_sword.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_buff_item.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_laser_pointer.h"
#include "tf_projectile_flare.h"
#include "trigger_area_capture.h"
#include "triggers.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_invis.h"
#include "hl2orange.spa.h"
#include "te_tfblood.h"
#include "activitylist.h"
#include "cdll_int.h"
#include "econ_entity_creation.h"
#include "tf_weaponbase_gun.h"
#include "team_train_watcher.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"
#include "serverbenchmark_base.h"
#include "trains.h"
#include "tf_fx.h"
#include "recipientfilter.h"
#include "ilagcompensationmanager.h"
#include "dt_utlvector_send.h"
#include "tf_item_wearable.h"
#include "tf_item_powerup_bottle.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tier0/vprof.h"
#include "econ_gcmessages.h"
#include "tf_gcmessages.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_shovel.h"
#include "bot/tf_bot.h"
#include "bot/tf_bot_manager.h"
#include "NextBotUtil.h"
#include "tf_wearable_weapons.h"
#include "tier0/icommandline.h"
#include "entity_healthkit.h"
#include "choreoevent.h"
#include "minigames/tf_duel.h"
#include "tf_bot_temp.h"
#include "tf_objective_resource.h"
#include "tf_weapon_pipebomblauncher.h"
#include "func_achievement.h"
#include "halloween/merasmus/merasmus.h"
#include "inetchannel.h"
#include "tf_wearable_levelable_item.h"
#include "tf_weapon_jar.h"
#include "halloween/tf_weapon_spellbook.h"
#include "soundenvelope.h"
#include "tf_triggers.h"
#include "collisionutils.h"
#include "tf_taunt_prop.h"
#include "eventlist.h"
#include "entity_rune.h"
#include "entity_halloween_pickup.h"
#include "tf_gc_server.h"
#include "tf_logic_halloween_2014.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_grapplinghook.h"
#include "tf_dropped_weapon.h"
#include "tf_passtime_logic.h"
#include "tf_weapon_passtime_gun.h"
#include "player_resource.h"
#include "tf_player_resource.h"
#include "gcsdk/gcclient_sharedobjectcache.h"
#include "tf_party.h"

#ifdef TF_RAID_MODE
#include "bot_npc/bot_npc_decoy.h"
#include "raid/tf_raid_logic.h"
#endif

#include "entity_currencypack.h"
#include "tf_mann_vs_machine_stats.h"
#include "player_vs_environment/tf_upgrades.h"
#include "player_vs_environment/tf_population_manager.h"
#include "tf_revive.h"
#include "tf_logic_halloween_2014.h"
#include "tf_logic_player_destruction.h"
#include "tf_weapon_slap.h"
#include "func_croc.h"
#include "tf_weapon_bonesaw.h"
#include "pointhurt.h"
#include "info_camera_link.h"

// NVNT haptic utils
#include "haptics/haptic_utils.h"

#include "gc_clientsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma warning( disable: 4355 ) // disables ' 'this' : used in base member initializer list'

ConVar sv_motd_unload_on_dismissal( "sv_motd_unload_on_dismissal", "0", 0, "If enabled, the MOTD contents will be unloaded when the player closes the MOTD." );

#define DAMAGE_FORCE_SCALE_SELF				9
#define SCOUT_ADD_BIRD_ON_GIB_CHANCE		5
#define MEDIC_RELEASE_DOVE_COUNT			10

#define JUMP_MIN_SPEED	268.3281572999747f		

extern bool IsInCommentaryMode( void );
extern void SpawnClientsideFlyingBird( Vector &vecSpawn );

extern ConVar	sk_player_head;
extern ConVar	sk_player_chest;
extern ConVar	sk_player_stomach;
extern ConVar	sk_player_arm;
extern ConVar	sk_player_leg;

extern ConVar	tf_spy_invis_time;
extern ConVar	tf_spy_invis_unstealth_time;
extern ConVar	tf_stalematechangeclasstime;
extern ConVar	tf_gravetalk;

extern ConVar	tf_bot_quota_mode;
extern ConVar	tf_bot_quota;
extern ConVar	halloween_starting_souls;

extern ConVar tf_powerup_mode_killcount_timer_length;

float GetCurrentGravity( void );

float			m_flNextReflectZap = 0.f;

static CTFPlayer *gs_pRecursivePlayerCheck = NULL;

bool CTFPlayer::m_bTFPlayerNeedsPrecache = true;

static const char g_pszIdleKickString[] = "#TF_Idle_kicked";

EHANDLE g_pLastSpawnPoints[TF_TEAM_COUNT];

EHANDLE	g_hTestSub;

ConVar tf_playerstatetransitions( "tf_playerstatetransitions", "-2", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "tf_playerstatetransitions <ent index or -1 for all>. Show player state transitions." );
ConVar tf_playergib( "tf_playergib", "1", FCVAR_NOTIFY, "Allow player gibbing. 0: never, 1: normal, 2: always", true, 0, true, 2 );

ConVar tf_damageforcescale_other( "tf_damageforcescale_other", "6.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damageforcescale_self_soldier_rj( "tf_damageforcescale_self_soldier_rj", "10.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damageforcescale_self_soldier_badrj( "tf_damageforcescale_self_soldier_badrj", "5.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damageforcescale_pyro_jump( "tf_damageforcescale_pyro_jump", "8.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_damagescale_self_soldier( "tf_damagescale_self_soldier", "0.60", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );


ConVar tf_damage_range( "tf_damage_range", "0.5", FCVAR_DEVELOPMENTONLY );
ConVar tf_damage_multiplier_blue( "tf_damage_multiplier_blue", "1.0", FCVAR_CHEAT, "All incoming damage to a blue player is multiplied by this value" );
ConVar tf_damage_multiplier_red( "tf_damage_multiplier_red", "1.0", FCVAR_CHEAT, "All incoming damage to a red player is multiplied by this value" );


ConVar tf_max_voice_speak_delay( "tf_max_voice_speak_delay", "1.5", FCVAR_DEVELOPMENTONLY, "Max time after a voice command until player can do another one", true, 0.1f, false, 0.f );

ConVar tf_allow_player_use( "tf_allow_player_use", "0", FCVAR_NOTIFY, "Allow players to execute +use while playing." );

ConVar tf_deploying_bomb_time( "tf_deploying_bomb_time", "1.90", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to deploy bomb before the point of no return." );
ConVar tf_deploying_bomb_delay_time( "tf_deploying_bomb_delay_time", "0.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to delay before deploying bomb." );

#ifdef TF_RAID_MODE
ConVar tf_raid_team_size( "tf_raid_team_size", "5", FCVAR_NOTIFY, "Max number of Raiders" );
ConVar tf_raid_respawn_safety_time( "tf_raid_respawn_safety_time", "1.5", FCVAR_NOTIFY, "Number of seconds of invulnerability after respawning" );
ConVar tf_raid_allow_class_change( "tf_raid_allow_class_change", "1", FCVAR_NOTIFY, "If nonzero, allow invaders to change their class after leaving the safe room" );
ConVar tf_raid_use_rescue_closets( "tf_raid_use_rescue_closets", "1", FCVAR_NOTIFY );
ConVar tf_raid_drop_healthkit_chance( "tf_raid_drop_healthkit_chance", "50" ); // , FCVAR_CHEAT );

ConVar tf_boss_battle_team_size( "tf_boss_battle_team_size", "5", FCVAR_NOTIFY, "Max number of players in Boss Battle mode" );
ConVar tf_boss_battle_respawn_safety_time( "tf_boss_battle_respawn_safety_time", "3", FCVAR_NOTIFY, "Number of seconds of invulnerability after respawning" );
ConVar tf_boss_battle_respawn_on_friends( "tf_boss_battle_respawn_on_friends", "1", FCVAR_NOTIFY );
#endif

ConVar tf_mvm_death_penalty( "tf_mvm_death_penalty", "0", FCVAR_NOTIFY | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "How much currency players lose when dying" );
extern ConVar tf_populator_damage_multiplier;
extern ConVar tf_mvm_skill;

ConVar tf_highfive_separation_forward( "tf_highfive_separation_forward", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Forward distance between high five partners" );
ConVar tf_highfive_separation_right( "tf_highfive_separation_right", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Right distance between high five partners" );

ConVar tf_highfive_max_range( "tf_highfive_max_range", "150", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "The farthest away a high five partner can be" );
ConVar tf_highfive_height_tolerance( "tf_highfive_height_tolerance", "12", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "The maximum height difference allowed for two high-fivers." );
ConVar tf_highfive_debug( "tf_highfive_debug", "0", FCVAR_NONE, "Turns on some console spew for debugging high five issues." );

ConVar tf_test_teleport_home_fx( "tf_test_teleport_home_fx", "0", FCVAR_CHEAT );

ConVar tf_halloween_giant_health_scale( "tf_halloween_giant_health_scale", "10", FCVAR_CHEAT );

ConVar tf_grapplinghook_los_force_detach_time( "tf_grapplinghook_los_force_detach_time", "1", FCVAR_CHEAT );
ConVar tf_powerup_max_charge_time( "tf_powerup_max_charge_time", "30", FCVAR_CHEAT );

extern ConVar tf_powerup_mode;
extern ConVar tf_mvm_buybacks_method;
extern ConVar tf_mvm_buybacks_per_wave;

#define TF_CANNONBALL_FORCE_SCALE	80.f
#define TF_CANNONBALL_FORCE_UPWARD	300.f


ConVar tf_halloween_unlimited_spells( "tf_halloween_unlimited_spells", "0", FCVAR_CHEAT );
extern ConVar tf_halloween_kart_boost_recharge;
extern ConVar tf_halloween_kart_boost_duration;

ConVar tf_halloween_kart_impact_force( "tf_halloween_kart_impact_force", "0.75f", FCVAR_CHEAT, "Impact force scaler" );
ConVar tf_halloween_kart_impact_damage( "tf_halloween_kart_impact_damage", "1.0f", FCVAR_CHEAT, "Impact damage scaler" );
ConVar tf_halloween_kart_impact_rate( "tf_halloween_kart_impact_rate", "0.5f", FCVAR_CHEAT, "rate of allowing impact damage" );
ConVar tf_halloween_kart_boost_impact_force( "tf_halloween_kart_boost_impact_force", "0.75f", FCVAR_CHEAT, "Impact force scaler on boosts" );
ConVar tf_halloween_kart_impact_bounds_scale( "tf_halloween_kart_impact_bounds_scale", "1.0f", FCVAR_CHEAT );
ConVar tf_halloween_kart_impact_feedback( "tf_halloween_kart_impact_feedback", "0.25f", FCVAR_CHEAT );
ConVar tf_halloween_kart_impact_lookahead( "tf_halloween_kart_impact_lookahead", "12.0f", FCVAR_CHEAT );
ConVar tf_halloween_kart_bomb_head_damage_scale( "tf_halloween_kart_bomb_head_damage_scale", "2", FCVAR_CHEAT );
ConVar tf_halloween_kart_bomb_head_impulse_scale( "tf_halloween_kart_bomb_head_impulse_scale", "2", FCVAR_CHEAT );
ConVar tf_halloween_kart_impact_air_scale( "tf_halloween_kart_impact_air_scale", "0.75f", FCVAR_CHEAT );
ConVar tf_halloween_kart_damage_to_force( "tf_halloween_kart_damage_to_force", "300.0f", FCVAR_CHEAT );
ConVar tf_halloween_kart_stun_duration_scale( "tf_halloween_kart_stun_duration_scale", "0.70f", FCVAR_CHEAT );
ConVar tf_halloween_kart_stun_amount( "tf_halloween_kart_stun_amount", "1.0f", FCVAR_CHEAT );
ConVar tf_halloween_kart_stun_enabled( "tf_halloween_kart_stun_enabled", "1", FCVAR_CHEAT );

ConVar tf_tauntcam_fov_override( "tf_tauntcam_fov_override", "0", FCVAR_CHEAT );

ConVar tf_nav_in_combat_range( "tf_nav_in_combat_range", "1000", FCVAR_CHEAT );

ConVar tf_halloween_kart_punting_ghost_force_scale( "tf_halloween_kart_punting_ghost_force_scale", "4", FCVAR_CHEAT );
ConVar tf_halloween_allow_ghost_hit_by_kart_delay( "tf_halloween_allow_ghost_hit_by_kart_delay", "0.5", FCVAR_CHEAT );

ConVar tf_maxhealth_drain_hp_min( "tf_maxhealth_drain_hp_min", "100", FCVAR_DEVELOPMENTONLY );
ConVar tf_maxhealth_drain_deploy_cost( "tf_maxhealth_drain_deploy_cost", "20", FCVAR_DEVELOPMENTONLY );

extern ConVar sv_vote_allow_spectators;
ConVar sv_vote_late_join_time( "sv_vote_late_join_time", "90", FCVAR_NONE, "Grace period after the match starts before players who join the match receive a vote-creation cooldown" );
ConVar sv_vote_late_join_cooldown( "sv_vote_late_join_cooldown", "300", FCVAR_NONE, "Length of the vote-creation cooldown when joining the server after the grace period has expired" );

extern ConVar tf_voice_command_suspension_mode;
extern ConVar tf_feign_death_duration;
extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;
extern ConVar sv_maxunlag;
extern ConVar tf_allow_taunt_switch;
extern ConVar weapon_medigun_chargerelease_rate;
extern ConVar tf_scout_energydrink_consume_rate;
extern ConVar tf_mm_trusted;
extern ConVar mp_spectators_restricted;
extern ConVar mp_teams_unbalance_limit;
extern ConVar tf_tournament_classchange_allowed;
extern ConVar tf_tournament_classchange_ready_allowed;
extern ConVar tf_rocketpack_impact_push_min;
extern ConVar tf_rocketpack_impact_push_max;
#if defined( _DEBUG ) || defined( STAGING_ONLY )
extern ConVar mp_developer;
extern ConVar bot_mimic;
#endif // _DEBUG || STAGING_ONLY 

extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
extern bool CanScatterGunKnockBack( CTFWeaponBase *pWeapon, float flDamage, float flDistanceSq );
extern bool IsCustomGameMode();

static const char *s_pszTauntRPSParticleNames[] =
{
	"rps_rock_red",
	"rps_paper_red",
	"rps_scissors_red",
	"rps_rock_red_win",
	"rps_paper_red_win",
	"rps_scissors_red_win",
	"rps_rock_blue",
	"rps_paper_blue",
	"rps_scissors_blue",
	"rps_rock_blue_win",
	"rps_paper_blue_win",
	"rps_scissors_blue_win"
};

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent( const char *name )
		: CBaseTempEntity( name )
	{
	}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	// BUGBUG:  ywb  we assume this is either 0 or an animation sequence #, but it could also be an activity, which should fit within this limit, but we're not guaranteed.
	SendPropInt( SENDINFO( m_nData ), ANIMATION_SEQUENCE_BITS ),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
    Vector vecEyePos = pPlayer->EyePosition();
	CPVSFilter filter( vecEyePos );
	if ( !IsCustomPlayerAnimEvent( event ) && ( event != PLAYERANIMEVENT_SNAP_YAW ) && ( event != PLAYERANIMEVENT_VOICE_COMMAND_GESTURE ) )
	{
		// if prediction is off, alway send jump
		if ( !( ( event == PLAYERANIMEVENT_JUMP ) && ( FStrEq(engine->GetClientConVarValue( pPlayer->entindex(), "cl_predict" ), "0" ) ) ) )
		{
			filter.RemoveRecipient( pPlayer );
		}
	}

	Assert( pPlayer->entindex() >= 1 && pPlayer->entindex() <= MAX_PLAYERS );
	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	Assert( nData < (1<<ANIMATION_SEQUENCE_BITS) );
	Assert( (1<<ANIMATION_SEQUENCE_BITS) >= ActivityList_HighestIndex() );
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//=================================================================================
//
// Ragdoll Entity
//
class CTFRagdoll : public CBaseAnimatingOverlay
{
public:

	DECLARE_CLASS( CTFRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	CTFRagdoll()
	{
		m_bGib = false;
		m_bBurning = false;
		m_bElectrocuted = false;
		m_bFeignDeath = false;
		m_bWasDisguised = false;
		m_bBecomeAsh = false;
		m_bOnGround = false;
		m_bCloaked = false;
		m_iDamageCustom = 0;
		m_bCritOnHardHit = false;
		m_vecRagdollOrigin.Init();
		m_vecRagdollVelocity.Init();
	}

	~CTFRagdoll()
	{
		// Destroy all of our attached wearables.
		for ( int i=0; i<m_hRagWearables.Count(); ++i )
		{
			if ( m_hRagWearables[i] )
			{
				m_hRagWearables[i]->Remove();
			}
		}
		m_hRagWearables.Purge();
	}

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		UseClientSideAnimation();
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
	CNetworkVar( bool, m_bGib );
	CNetworkVar( bool, m_bBurning );
	CNetworkVar( bool, m_bElectrocuted );
	CNetworkVar( bool, m_bFeignDeath );
	CNetworkVar( bool, m_bWasDisguised );
	CNetworkVar( bool, m_bBecomeAsh );
	CNetworkVar( bool, m_bOnGround );
	CNetworkVar( bool, m_bCloaked );
	CNetworkVar( int, m_iDamageCustom );
	CNetworkVar( int, m_iTeam );
	CNetworkVar( int, m_iClass );
	CNetworkVar( bool, m_bGoldRagdoll );
	CNetworkVar( bool, m_bIceRagdoll );
	CNetworkVar( bool, m_bCritOnHardHit );
	CNetworkVar( float, m_flHeadScale );
	CNetworkVar( float, m_flTorsoScale );
	CNetworkVar( float, m_flHandScale );
	CUtlVector<CHandle<CEconWearable > > m_hRagWearables;
};

LINK_ENTITY_TO_CLASS( tf_ragdoll, CTFRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTFRagdoll, DT_TFRagdoll )
	SendPropVector( SENDINFO( m_vecRagdollOrigin ), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO ( m_hPlayer) ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ), 13, SPROP_ROUNDDOWN, -2048.0f, 2048.0f ),
	SendPropInt( SENDINFO( m_nForceBone ) ),
	SendPropBool( SENDINFO( m_bGib ) ),
	SendPropBool( SENDINFO( m_bBurning ) ),
	SendPropBool( SENDINFO( m_bElectrocuted ) ),
	SendPropBool( SENDINFO( m_bFeignDeath ) ),
	SendPropBool( SENDINFO( m_bWasDisguised ) ),
	SendPropBool( SENDINFO( m_bBecomeAsh ) ),
	SendPropBool( SENDINFO( m_bOnGround ) ),
	SendPropBool( SENDINFO( m_bCloaked ) ),
	SendPropInt( SENDINFO( m_iDamageCustom ) ),
	SendPropInt( SENDINFO( m_iTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iClass ), 4, SPROP_UNSIGNED ),			
	SendPropUtlVector( SENDINFO_UTLVECTOR( m_hRagWearables ), 8, SendPropEHandle( NULL, 0 ) ),
	SendPropBool( SENDINFO( m_bGoldRagdoll ) ),
	SendPropBool( SENDINFO( m_bIceRagdoll ) ),
	SendPropBool( SENDINFO( m_bCritOnHardHit ) ),
	SendPropFloat( SENDINFO( m_flHeadScale ) ),
	SendPropFloat( SENDINFO( m_flTorsoScale ) ),
	SendPropFloat( SENDINFO( m_flHandScale ) ),
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //
// Tables.
// -------------------------------------------------------------------------------- //

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the UtlVector list of objects to entindexes, where it's reassembled on the client
//-----------------------------------------------------------------------------
void SendProxy_PlayerObjectList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;

	// If this fails, then SendProxyArrayLength_PlayerObjects didn't work.
	Assert( iElement < pPlayer->GetObjectCount() );

	CBaseObject *pObject = pPlayer->GetObject(iElement);

	EHANDLE hObject;
	hObject = pObject;

	SendProxy_EHandleToInt( pProp, pStruct, &hObject, pOut, iElement, objectID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int SendProxyArrayLength_PlayerObjects( const void *pStruct, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;
	int iObjects = pPlayer->GetObjectCount();
	Assert( iObjects <= MAX_OBJECTS_PER_PLAYER );
	return iObjects;
}

//-----------------------------------------------------------------------------
// Purpose: Send to attached medics
//-----------------------------------------------------------------------------
void* SendProxy_SendHealersDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTFPlayer *pPlayer = (CTFPlayer*)pStruct;
	if ( pPlayer )
	{
		// Add attached medics
		for ( int i = 0; i < pPlayer->m_Shared.GetNumHealers(); i++ )
		{
			CTFPlayer *pMedic = ToTFPlayer( pPlayer->m_Shared.GetHealerByIndex( i ) );
			if ( !pMedic )
				continue;

			pRecipients->SetRecipient( pMedic->GetClientIndex() );
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendHealersDataTable );

BEGIN_DATADESC( CTFPlayer )
	DEFINE_INPUTFUNC( FIELD_VOID, "IgnitePlayer", InputIgnitePlayer ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCustomModel", InputSetCustomModel ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCustomModelWithClassAnimations", InputSetCustomModelWithClassAnimations ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetCustomModelOffset", InputSetCustomModelOffset ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetCustomModelRotation", InputSetCustomModelRotation ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearCustomModelRotation", InputClearCustomModelRotation ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCustomModelRotates", InputSetCustomModelRotates ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCustomModelVisibleToSelf", InputSetCustomModelVisibleToSelf ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetForcedTauntCam", InputSetForcedTauntCam ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ExtinguishPlayer", InputExtinguishPlayer ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "BleedPlayer", InputBleedPlayer ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TriggerLootIslandAchievement", InputTriggerLootIslandAchievement ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TriggerLootIslandAchievement2", InputTriggerLootIslandAchievement2 ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SpeakResponseConcept",	InputSpeakResponseConcept ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RollRareSpell", InputRollRareSpell ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
END_DATADESC()

BEGIN_ENT_SCRIPTDESC( CTFPlayer, CBaseMultiplayerPlayer , "Team Fortress 2 Player" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetActiveWeapon, "GetActiveWeapon", "Get the player's current weapon" )

	DEFINE_SCRIPTFUNC( ForceRespawn, "Force respawns the player" )
	DEFINE_SCRIPTFUNC( ForceRegenerateAndRespawn, "Force regenerates and respawns the player" )
	DEFINE_SCRIPTFUNC( Regenerate, "Resupplies a player. If regen health/ammo is set, clears negative conds, gives back player health/ammo" )

	DEFINE_SCRIPTFUNC( HasItem, "Currently holding an item? Eg. capture flag" )
	DEFINE_SCRIPTFUNC( GetNextRegenTime, "Get next health regen time." )
	DEFINE_SCRIPTFUNC( SetNextRegenTime, "Set next health regen time." )
	DEFINE_SCRIPTFUNC( GetNextChangeClassTime, "Get next change class time." )
	DEFINE_SCRIPTFUNC( SetNextChangeClassTime, "Set next change class time." )
	DEFINE_SCRIPTFUNC( GetNextChangeTeamTime, "Get next change team time." )
	DEFINE_SCRIPTFUNC( SetNextChangeTeamTime, "Set next change team time." )
	DEFINE_SCRIPTFUNC( DropFlag, "Force player to drop the flag." )
	DEFINE_SCRIPTFUNC( DropRune, "Force player to drop the rune." )
	DEFINE_SCRIPTFUNC( ForceChangeTeam, "Force player to change their team." )
	DEFINE_SCRIPTFUNC( IsMiniBoss, "Is this player an MvM mini-boss?" )
	DEFINE_SCRIPTFUNC( SetIsMiniBoss, "Make this player an MvM mini-boss." )
	DEFINE_SCRIPTFUNC( CanJump, "Can the player jump?" )
	DEFINE_SCRIPTFUNC( CanDuck, "Can the player duck?" )
	DEFINE_SCRIPTFUNC( CanPlayerMove, "Can the player move?" )
	DEFINE_SCRIPTFUNC( RemoveAllObjects, "Remove all player objects. Eg. dispensers/sentries." )
	DEFINE_SCRIPTFUNC( IsPlacingSapper, "Returns true if we placed a sapper in the last few moments" )
	DEFINE_SCRIPTFUNC( IsSapping, "Returns true if we are currently sapping" )
	DEFINE_SCRIPTFUNC( RemoveInvisibility, "Un-invisible a spy." )
	DEFINE_SCRIPTFUNC( RemoveDisguise, "Undisguise a spy." )
	DEFINE_SCRIPTFUNC( TryToPickupBuilding, "Make the player attempt to pick up a building in front of them" )
	DEFINE_SCRIPTFUNC( IsCallingForMedic, "Is this player calling for medic?" )
	DEFINE_SCRIPTFUNC( GetTimeSinceCalledForMedic, "When did the player last call medic" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHealTarget, "GetHealTarget", "Who is the medic healing?" )
	DEFINE_SCRIPTFUNC( GetClassEyeHeight, "Gets the eye height of the player" )
	DEFINE_SCRIPTFUNC( FiringTalk, "Makes eg. a heavy go AAAAAAAAAAaAaa like they are firing their minigun." )
	DEFINE_SCRIPTFUNC( CanAirDash, "" )
	DEFINE_SCRIPTFUNC( CanBreatheUnderwater, "" )
	DEFINE_SCRIPTFUNC( CanGetWet, "" )
	DEFINE_SCRIPTFUNC( InAirDueToExplosion, "" )
	DEFINE_SCRIPTFUNC( InAirDueToKnockback, "" )
	DEFINE_SCRIPTFUNC( ApplyAbsVelocityImpulse, "" )
	DEFINE_SCRIPTFUNC( ApplyPunchImpulseX, "" )
	DEFINE_SCRIPTFUNC( SetUseBossHealthBar, "" )
	DEFINE_SCRIPTFUNC( IsFireproof, "" )
	DEFINE_SCRIPTFUNC( IsAllowedToTaunt, "" )
	DEFINE_SCRIPTFUNC( IsViewingCYOAPDA, "" )
	DEFINE_SCRIPTFUNC( IsRegenerating, "" )
	DEFINE_SCRIPTFUNC( GetCurrentTauntMoveSpeed, "" )
	DEFINE_SCRIPTFUNC( SetCurrentTauntMoveSpeed, "" )
	DEFINE_SCRIPTFUNC( IsUsingActionSlot, "" )
	DEFINE_SCRIPTFUNC( IsInspecting, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetGrapplingHookTarget, "GetGrapplingHookTarget", "What entity is the player grappling?" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetGrapplingHookTarget, "SetGrapplingHookTarget", "Set the player's target grapple entity" )
	DEFINE_SCRIPTFUNC( AddCustomAttribute, "Add a custom attribute to the player" )
	DEFINE_SCRIPTFUNC( RemoveCustomAttribute, "Remove a custom attribute to the player" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptAddCond, "AddCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptAddCondEx, "AddCondEx", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveCond, "RemoveCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveCondEx, "RemoveCondEx", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptInCond, "InCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptWasInCond, "WasInCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveAllCond,"RemoveAllCond", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCondDuration, "GetCondDuration", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetCondDuration, "SetCondDuration", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseTarget, "GetDisguiseTarget", "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsCarryingRune, "IsCarryingRune", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsCritBoosted, "IsCritBoosted", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsInvulnerable, "IsInvulnerable", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsStealthed, "IsStealthed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptCanBeDebuffed, "CanBeDebuffed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsImmuneToPushback, "IsImmuneToPushback", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseAmmoCount, "GetDisguiseAmmoCount", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetDisguiseAmmoCount, "SetDisguiseAmmoCount", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDisguiseTeam, "GetDisguiseTeam", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsFullyInvisible, "IsFullyInvisible", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetSpyCloakMeter, "GetSpyCloakMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetSpyCloakMeter, "SetSpyCloakMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsRageDraining, "IsRageDraining", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetRageMeter, "GetRageMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetRageMeter, "SetRageMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetScoutHypeMeter, "GetScoutHypeMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetScoutHypeMeter, "SetScoutHypeMeter", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsHypeBuffed, "IsHypeBuffed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsJumping, "IsJumping", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsAirDashing, "IsAirDashing", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsControlStunned, "IsControlStunned", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsSnared, "IsSnared", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCaptures, "GetCaptures", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDefenses, "GetDefenses", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetDominations, "GetDominations", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetRevenge, "GetRevenge", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBuildingsDestroyed, "GetBuildingsDestroyed", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHeadshots, "GetHeadshots", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBackstabs, "GetBackstabs", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetHealPoints, "GetHealPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetInvulns, "GetInvulns", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetTeleports, "GetTeleports", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetResupplyPoints, "GetResupplyPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKillAssists, "GetKillAssists", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBonusPoints, "GetBonusPoints", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptResetScores, "ResetScores", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsParachuteEquipped, "IsParachuteEquipped", "" )

	DEFINE_SCRIPTFUNC( GetCurrency, "Get player's cash for game modes with upgrades, ie. MvM" )
	DEFINE_SCRIPTFUNC( SetCurrency, "Set player's cash for game modes with upgrades, ie. MvM" )
	DEFINE_SCRIPTFUNC( AddCurrency, "Kaching! Give the player some cash for game modes with upgrades, ie. MvM" )
	DEFINE_SCRIPTFUNC( RemoveCurrency, "Take away money from a player for reasons such as ie. spending." )

	DEFINE_SCRIPTFUNC( IgnitePlayer, "" )
	DEFINE_SCRIPTFUNC( SetCustomModel, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelWithClassAnimations, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelOffset, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelRotation, "" )
	DEFINE_SCRIPTFUNC( ClearCustomModelRotation, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelRotates, "" )
	DEFINE_SCRIPTFUNC( SetCustomModelVisibleToSelf, "" )
	DEFINE_SCRIPTFUNC( SetForcedTauntCam, "" )
	DEFINE_SCRIPTFUNC( ExtinguishPlayerBurning, "" )
	DEFINE_SCRIPTFUNC( BleedPlayer, "" )
	DEFINE_SCRIPTFUNC( BleedPlayerEx, "" )
	DEFINE_SCRIPTFUNC( RollRareSpell, "" )
	DEFINE_SCRIPTFUNC( ClearSpells, "" )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetPlayerClass, "GetPlayerClass", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetPlayerClass, "SetPlayerClass", "" )

	DEFINE_SCRIPTFUNC( RemoveTeleportEffect, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveAllItems, "RemoveAllItems", "" )

	DEFINE_SCRIPTFUNC( UpdateSkin, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_ShootPosition, "" ) // Needs this slim wrapper or the world falls apart on MSVC.
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_CanUse, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_Equip, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_Drop, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_DropEx, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_Switch, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Weapon_SetLast, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetLastWeapon, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( EquipWearableViewModel, "" )

	DEFINE_SCRIPTFUNC_WRAPPED( IsFakeClient, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( GetBotType, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( IsBotOfType, "" )

	DEFINE_SCRIPTFUNC( AddHudHideFlags, "Hides a hud element based on Constants.FHideHUD." )
	DEFINE_SCRIPTFUNC( RemoveHudHideFlags, "Unhides a hud element based on Constants.FHideHUD." )
	DEFINE_SCRIPTFUNC( SetHudHideFlags, "Force hud hide flags to a value" )
	DEFINE_SCRIPTFUNC( GetHudHideFlags, "Gets current hidden hud elements" )

	DEFINE_SCRIPTFUNC( IsTaunting, "" )
	DEFINE_SCRIPTFUNC( DoTauntAttack, "" )
	DEFINE_SCRIPTFUNC( CancelTaunt, "" )
	DEFINE_SCRIPTFUNC( StopTaunt, "" )
	DEFINE_SCRIPTFUNC( EndLongTaunt, "" )
	DEFINE_SCRIPTFUNC( GetTauntRemoveTime, "" )
	DEFINE_SCRIPTFUNC( IsAllowedToRemoveTaunt, "" )
	DEFINE_SCRIPTFUNC( HandleTauntCommand, "" )
	DEFINE_SCRIPTFUNC( ClearTauntAttack, "" )
	DEFINE_SCRIPTFUNC( GetTauntAttackTime, "" )
	DEFINE_SCRIPTFUNC( SetRPSResult, "" )
	DEFINE_SCRIPTFUNC( GetVehicleReverseTime, "" )
	DEFINE_SCRIPTFUNC( SetVehicleReverseTime, "" )
	DEFINE_SCRIPTFUNC_WRAPPED( Taunt, "" )

	DEFINE_SCRIPTFUNC( GrantOrRemoveAllUpgrades, "Grants or removes all upgrades the player has purchased." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCustomAttribute, "GetCustomAttribute", "Get a custom attribute float from the player" )

	DEFINE_SCRIPTFUNC_WRAPPED( StunPlayer, "" )
END_SCRIPTDESC();


EXTERN_SEND_TABLE( DT_ScriptCreatedItem );

// specific to the local player
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFLocalPlayerExclusive )
	// send a hi-res origin to the local player for use in prediction
	SendPropVectorXY(SENDINFO(m_vecOrigin),               -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginXY ),
	SendPropFloat   (SENDINFO_VECTORELEM(m_vecOrigin, 2), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginZ ),
	SendPropArray2( 
		SendProxyArrayLength_PlayerObjects,
		SendPropInt("player_object_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_PlayerObjectList), 
		MAX_OBJECTS_PER_PLAYER, 
		0, 
		"player_object_array"
		),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

	SendPropBool( SENDINFO( m_bIsCoaching ) ),
	SendPropEHandle( SENDINFO( m_hCoach ) ),
	SendPropEHandle( SENDINFO( m_hStudent ) ),

	SendPropInt( SENDINFO( m_nCurrency ), -1, SPROP_VARINT ),
	SendPropInt( SENDINFO( m_nExperienceLevel ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nExperienceLevelProgress ), 7, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bMatchSafeToLeave ) ),

END_SEND_TABLE()

// all players except the local player
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFNonLocalPlayerExclusive )
	// send a lo-res origin to other players
	SendPropVectorXY(SENDINFO(m_vecOrigin),               -1, SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginXY ),
	SendPropFloat   (SENDINFO_VECTORELEM(m_vecOrigin, 2), -1, SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_OriginZ ),

	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),

END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Sent to attached medics
//-----------------------------------------------------------------------------
BEGIN_SEND_TABLE_NOBASE( CTFPlayer, DT_TFSendHealersDataTable )
	SendPropInt( SENDINFO( m_nActiveWpnClip ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
END_SEND_TABLE()

//============

LINK_ENTITY_TO_CLASS( player, CTFPlayer );
PRECACHE_REGISTER(player);

IMPLEMENT_SERVERCLASS_ST( CTFPlayer, DT_TFPlayer )
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimating", "m_nBody" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_BaseEntity", "m_nModelIndex" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),

	// cs_playeranimstate and clientside animation takes care of these on the client
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),

	SendPropExclude( "DT_BaseFlex", "m_flexWeight" ),
	SendPropExclude( "DT_BaseFlex", "m_blinktoggle" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

	SendPropBool(SENDINFO(m_bSaveMeParity)),
	SendPropBool(SENDINFO(m_bIsMiniBoss)),
	SendPropBool(SENDINFO(m_bIsABot)),
	SendPropInt( SENDINFO(m_nBotSkill), 3, SPROP_UNSIGNED ),

	// This will create a race condition will the local player, but the data will be the same so.....
	SendPropInt( SENDINFO( m_nWaterLevel ), 2, SPROP_UNSIGNED ),

	// Ragdoll.
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropDataTable( SENDINFO_DT( m_PlayerClass ), &REFERENCE_SEND_TABLE( DT_TFPlayerClassShared ) ),
	SendPropDataTable( SENDINFO_DT( m_Shared ), &REFERENCE_SEND_TABLE( DT_TFPlayerShared ) ),
	SendPropEHandle(SENDINFO(m_hItem)),

	// Data that only gets sent to the local player
	SendPropDataTable( "tflocaldata", 0, &REFERENCE_SEND_TABLE(DT_TFLocalPlayerExclusive), SendProxy_SendLocalDataTable ),

	// Data that gets sent to all other players
	SendPropDataTable( "tfnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_TFNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable ),

	SendPropBool( SENDINFO( m_bAllowMoveDuringTaunt ) ),
	SendPropBool( SENDINFO( m_bIsReadyToHighFive ) ),
	SendPropEHandle( SENDINFO( m_hHighFivePartner ) ),
	SendPropInt( SENDINFO( m_nForceTauntCam ), 2, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flTauntYaw ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_nActiveTauntSlot ) ),
	SendPropInt( SENDINFO( m_iTauntItemDefIndex ) ),
	SendPropFloat( SENDINFO( m_flCurrentTauntMoveSpeed ) ),
	SendPropFloat( SENDINFO( m_flVehicleReverseTime ) ),

	SendPropFloat( SENDINFO( m_flMvMLastDamageTime ), 16, SPROP_ROUNDUP ),
	SendPropInt( SENDINFO( m_iSpawnCounter ) ),
	SendPropBool( SENDINFO( m_bFlipViewModels ) ),
	SendPropBool( SENDINFO( m_bArenaSpectator ) ),
	SendPropFloat( SENDINFO( m_flHeadScale ) ),
	SendPropFloat( SENDINFO( m_flTorsoScale ) ),
	SendPropFloat( SENDINFO( m_flHandScale ) ),

	SendPropBool( SENDINFO( m_bUseBossHealthBar ) ),

	SendPropBool( SENDINFO( m_bUsingVRHeadset ) ),

	SendPropBool( SENDINFO( m_bForcedSkin ) ),
	SendPropInt( SENDINFO( m_nForcedSkin ), ANIMATION_SKIN_BITS ),

	SendPropDataTable( SENDINFO_DT( m_AttributeManager ), &REFERENCE_SEND_TABLE(DT_AttributeManager) ),

	SendPropDataTable( "TFSendHealersDataTable", 0, &REFERENCE_SEND_TABLE( DT_TFSendHealersDataTable ), SendProxy_SendHealersDataTable ),

	SendPropFloat( SENDINFO( m_flKartNextAvailableBoost ) ),
	SendPropInt( SENDINFO( m_iKartHealth ) ),
	SendPropInt( SENDINFO( m_iKartState ) ),
	SendPropEHandle( SENDINFO( m_hGrapplingHookTarget ) ),
	SendPropEHandle( SENDINFO( m_hSecondaryLastWeapon ) ),
	SendPropBool( SENDINFO( m_bUsingActionSlot ) ),
	SendPropFloat( SENDINFO( m_flInspectTime ) ),
	SendPropFloat( SENDINFO( m_flHelpmeButtonPressTime ) ),
	SendPropInt( SENDINFO( m_iCampaignMedals ) ),
	SendPropInt( SENDINFO( m_iPlayerSkinOverride ) ),
	SendPropBool( SENDINFO( m_bViewingCYOAPDA ) ),
	SendPropBool( SENDINFO( m_bRegenerating ) ),
END_SEND_TABLE()

// -------------------------------------------------------------------------------- //

void cc_CreatePredictionError_f()
{
	CBaseEntity *pEnt = CBaseEntity::Instance( 1 );
	pEnt->SetAbsOrigin( pEnt->GetAbsOrigin() + Vector( 63, 0, 0 ) );
}
ConCommand cc_CreatePredictionError( "CreatePredictionError", cc_CreatePredictionError_f, "Create a prediction error", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

// -------------------------------------------------------------------------------- //

enum eCoachCommand
{
	kCoachCommand_Look = 1, // slot1
	kCoachCommand_Go,		// slot2
	kCoachCommand_Attack,
	kCoachCommand_Defend,
	kNumCoachCommands,
};

/**
 * Handles a command from the coach
 */
static void HandleCoachCommand( CTFPlayer *pPlayer, eCoachCommand command )
{
	if ( pPlayer && pPlayer->IsCoaching() && pPlayer->GetStudent() && command < kNumCoachCommands )
	{
		const float kMaxRateCoachCommands = 1.0f;
		float flLastCoachCommandDelta = gpGlobals->curtime - pPlayer->m_flLastCoachCommand;
		if ( flLastCoachCommandDelta < kMaxRateCoachCommands && flLastCoachCommandDelta > 0.0f )
		{
			return;
		}
		pPlayer->m_flLastCoachCommand = gpGlobals->curtime;
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "show_annotation" );
		if ( pEvent )
		{
			Vector vForward;
 			AngleVectors( pPlayer->EyeAngles(), &vForward );

			trace_t	trace;
			CTraceFilterSimple filter( pPlayer->GetStudent(), COLLISION_GROUP_NONE );
			UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + vForward * MAX_TRACE_LENGTH, MASK_SOLID, &filter, &trace );

			CBaseEntity *pHitEntity = trace.m_pEnt && trace.m_pEnt->IsWorld() == false && trace.m_pEnt != pPlayer->GetStudent() ? trace.m_pEnt : NULL;
			pEvent->SetInt( "id", pPlayer->entindex() );
			pEvent->SetFloat( "worldPosX", trace.endpos.x );
			pEvent->SetFloat( "worldPosY", trace.endpos.y );
			pEvent->SetFloat( "worldPosZ", trace.endpos.z );
			pEvent->SetFloat( "worldNormalX", trace.plane.normal.x );
			pEvent->SetFloat( "worldNormalY", trace.plane.normal.y );
			pEvent->SetFloat( "worldNormalZ", trace.plane.normal.z );
			pEvent->SetFloat( "lifetime", 10.0f );
			if ( pHitEntity )
			{
				pEvent->SetInt( "follow_entindex", pHitEntity->entindex() );
			}
			pEvent->SetInt( "visibilityBitfield", ( 1 << pPlayer->entindex() | 1 << pPlayer->GetStudent()->entindex() ) );
			pEvent->SetBool( "show_distance", true );
			pEvent->SetBool( "show_effect", true );

			switch ( command )
			{
			case kCoachCommand_Attack:	
				pEvent->SetString( "text", pHitEntity ? "#TF_Coach_AttackThis" : "#TF_Coach_AttackHere" ); 
				pEvent->SetString( "play_sound", "coach/coach_attack_here.wav" );
				break;
			case kCoachCommand_Defend:	
				pEvent->SetString( "text", pHitEntity ? "#TF_Coach_DefendThis" : "#TF_Coach_DefendHere" ); 
				pEvent->SetString( "play_sound", "coach/coach_defend_here.wav" );
				break;
			case kCoachCommand_Look:	
				pEvent->SetString( "text", pHitEntity ? "#TF_Coach_LookAt" : "#TF_Coach_LookHere" ); 
				pEvent->SetString( "play_sound", "coach/coach_look_here.wav" );
				break;
			case kCoachCommand_Go:
				pEvent->SetString( "text", pHitEntity ? "#TF_Coach_GoToThis" : "#TF_Coach_GoHere" ); 
				pEvent->SetString( "play_sound", "coach/coach_go_here.wav" );
				break;
			}
			gameeventmanager->FireEvent( pEvent );
		}

	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer::CTFPlayer() 
{
	m_pAttributes = this;

	m_PlayerAnimState = CreateTFPlayerAnimState( this );

	SetArmorValue( 10 );

	m_hItem = NULL;
	m_hTauntScene = NULL;
	m_hTauntProp = NULL;

	m_flTauntNextStartTime = -1.f;

	UseClientSideAnimation();
	m_angEyeAngles.Init();
	m_pStateInfo = NULL;
	m_lifeState = LIFE_DEAD; // Start "dead".
	m_iMaxSentryKills = 0;
	m_flLastCoachCommand = 0;

	m_flNextTimeCheck = gpGlobals->curtime;
	m_flSpawnTime = 0;

	m_flWaterExitTime = 0;

	SetViewOffset( TF_PLAYER_VIEW_OFFSET );

	m_Shared.Init( this );

	m_iLastSkin = -1;

	m_bHudClassAutoKill = false;
	m_bMedigunAutoHeal = false;

	m_vecLastDeathPosition = Vector( FLT_MAX, FLT_MAX, FLT_MAX );

	SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );

	m_flLastAction = gpGlobals->curtime;
	m_flTimeInSpawn = 0;

	m_bInitTaunt = false;

	m_bSpeakingConceptAsDisguisedSpy = false;

	m_nMannpowerKills = 0;
	m_nMannpowerDeaths = 0;
	m_bMannpowerHereForFullInterval = false;

	m_iPreviousteam = TEAM_UNASSIGNED;
	m_bArenaSpectator = false;

	m_bArenaIsAFK = false;
	m_bIsAFK = false;

	m_nDeployingBombState = TF_BOMB_DEPLOYING_NONE;

	m_flNextChangeClassTime = 0.0f;
	m_flNextChangeTeamTime = 0.0f;

	m_iOldStunFlags = 0;
	m_iLastWeaponSlot = 1;
	m_iNumberofDominations = 0;
	m_bFlipViewModels = false;
	m_iBlastJumpState = 0;
	m_flBlastJumpLandTime = 0;
	m_fMaxHealthTime = -1;
	m_iHealthBefore = 0;

	m_iTeamChanges = 0;
	m_iClassChanges = 0;

	m_hReviveMarker = NULL;

	// Bounty Mode
	m_nExperienceLevel = 1;
	m_nExperiencePoints = 0;
	m_nExperienceLevelProgress = 0;

	SetDefLessFunc( m_Cappers );		// Tracks victims for demo achievement

 	//=============================================================================
	// HPE_BEGIN:
	// [msmith]	Added a player type so we can distinguish between bots and humans.
	//============================================================================= 
	m_playerType = HUMAN_PLAYER;
	//=============================================================================
	// HPE_END
	//=============================================================================

	m_bIsTargetDummy = false;
	
	m_bCollideWithSentry = false;

	m_flCommentOnCarrying = 0;

	m_bIsReadyToHighFive = false;
	m_hHighFivePartner = NULL;
	m_nForceTauntCam = 0;
	m_bAllowMoveDuringTaunt = false;
	m_bTauntForceMoveForward = false;
	m_flTauntForceMoveForwardSpeed = 0.f;
	m_flTauntMoveAccelerationTime = 0.f;
	m_flTauntTurnSpeed = 0.f;
	m_flTauntTurnAccelerationTime = 0.f;
	m_bTauntMimic = false;
	m_bIsTauntInitiator = false;
	m_TauntEconItemView.Invalidate();
	m_iPreTauntWeaponSlot = -1;

	m_bIsCalculatingMaximumSpeed = false;

	m_flLastThinkTime = -1.f;

	m_nCurrency = 0;
	m_pWaveSpawnPopulator = NULL;
	m_flLastReadySoundTime = 0.f;

	m_damageRateArray = new int[ DPS_Period ];
	ResetDamagePerSecond();

	m_nActiveWpnClip.Set( 0 );
	m_nActiveWpnClipPrev = 0;
	m_flNextClipSendTime = 0;

	m_nCanPurchaseUpgradesCount = 0;

	m_flHeadScale = 1.f;
	m_flTorsoScale = 1.f;
	m_flHandScale = 1.f;

	m_bPendingMerasmusPlayerBombExplode = false;
	m_fLastBombHeadTimestamp = 0.0f;

	m_bIsSapping = false;
	m_iSappingEvent = TF_SAPEVENT_NONE;
	m_flSapStartTime = 0.00;

	m_bIsMiniBoss = false;

	m_bUseBossHealthBar = false;

	m_bUsingVRHeadset = false;

	m_bForcedSkin = false;
	m_nForcedSkin = 0;

	SetRespawnOverride( -1.f, NULL_STRING );

	m_qPreviousChargeEyeAngle.Init();

	m_vHalloweenKartPush.Zero();
	m_flHalloweenKartPushEventTime = 0.f;
	m_bCheckKartCollision = false;
	m_flHHHKartAttackTime = 0.f;
	m_flNextBonusDucksVOAllowedTime = 0.f;

	m_flGhostLastHitByKartTime = 0.f;

	m_flVehicleReverseTime = FLT_MAX;
	m_iCampaignMedals = 0;

	m_bPasstimeBallSlippery = false;
	m_flNextScorePointForPD = -1;

	m_iPlayerSkinOverride = 0;

	m_nPrevRoundTeamNum = TEAM_UNASSIGNED;
	m_flLastDamageResistSoundTime = -1.f;
	m_hLastDamageDoneEntity = NULL;
	
	m_mapCustomAttributes.SetLessFunc( UtlStringCaseInsensitiveLessFunc );

	SetDefLessFunc( m_PlayersExtinguished );

	m_flLastAutobalanceTime = 0.f;

	m_bViewingCYOAPDA = false;

	ResetMaxHealthDrain();

	m_bRegenerating = false;
	m_bRespawning = false;

	m_bAlreadyUsedExtendFreezeThisDeath = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ForcePlayerViewAngles( const QAngle& qTeleportAngles )
{
	CSingleUserRecipientFilter filter( this );

	UserMessageBegin( filter, "ForcePlayerViewAngles" );
	WRITE_BYTE( 0x01 ); // Reserved space for flags.
	WRITE_BYTE( entindex() );
	WRITE_ANGLES( qTeleportAngles );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetGrapplingHookTarget( CBaseEntity *pTarget, bool bShouldBleed /*= false*/ )
{
	if ( pTarget )
	{
		// prevent fall damage after a successful hook
		m_Shared.AddCond( TF_COND_GRAPPLINGHOOK_SAFEFALL );
		m_Shared.AddCond( TF_COND_GRAPPLINGHOOK_LATCHED );
	}
	else
	{
		m_Shared.RemoveCond( TF_COND_GRAPPLINGHOOK_LATCHED );
	}

	CBaseEntity *pPreviousTarget = m_hGrapplingHookTarget;
	m_hGrapplingHookTarget = pTarget;
	
	if ( pTarget )
	{
		if ( pTarget->IsPlayer() )
		{
			CTFPlayer *pTargetPlayer = ToTFPlayer( pTarget );

			m_Shared.AddCond( TF_COND_GRAPPLED_TO_PLAYER );

			// make player bleed
			if ( bShouldBleed )
			{
				CTFGrapplingHook *pGrapplingHook = dynamic_cast< CTFGrapplingHook* >( GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
				if ( pGrapplingHook )
					pTargetPlayer->m_Shared.MakeBleed( this, pGrapplingHook, 0, TF_BLEEDING_DMG, true );

				pTargetPlayer->m_nHookAttachedPlayers++;
			}

			if ( !pTargetPlayer->m_Shared.InCond( TF_COND_GRAPPLINGHOOK_BLEEDING ) && pTargetPlayer->m_nHookAttachedPlayers > 0 )
			{
				pTargetPlayer->m_Shared.AddCond( TF_COND_GRAPPLINGHOOK_BLEEDING );
			}
			if ( !pTargetPlayer->m_Shared.InCond( TF_COND_GRAPPLED_BY_PLAYER ) && pTargetPlayer->m_nHookAttachedPlayers > 0 )
			{
				pTargetPlayer->m_Shared.AddCond( TF_COND_GRAPPLED_BY_PLAYER );
			}
		}

		m_flLastSeenHookTarget = gpGlobals->curtime;
	}
	else
	{
		if ( pPreviousTarget && pPreviousTarget->IsPlayer() )
		{
			CTFPlayer *pPreviousTargetPlayer = ToTFPlayer( pPreviousTarget );

			m_Shared.RemoveCond( TF_COND_GRAPPLED_TO_PLAYER );
			
			// try to remove bleeding from hook if there's one
			if ( pPreviousTargetPlayer->m_Shared.InCond( TF_COND_BLEEDING ) )
			{
				CTFGrapplingHook *pGrapplingHook = dynamic_cast< CTFGrapplingHook* >( GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
				if ( pGrapplingHook )
					pPreviousTargetPlayer->m_Shared.StopBleed( this, pGrapplingHook );
			}

			pPreviousTargetPlayer->m_nHookAttachedPlayers--;
			Assert( pPreviousTargetPlayer->m_nHookAttachedPlayers >= 0 );
			if ( pPreviousTargetPlayer->m_nHookAttachedPlayers == 0 )
			{
				pPreviousTargetPlayer->m_Shared.RemoveCond( TF_COND_GRAPPLINGHOOK_BLEEDING );
				pPreviousTargetPlayer->m_Shared.RemoveCond( TF_COND_GRAPPLED_BY_PLAYER );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanBeForcedToLaugh( void )
{
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && IsBot() && ( GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TFPlayerThink()
{
	if ( m_pStateInfo && m_pStateInfo->pfnThink )
	{
		(this->*m_pStateInfo->pfnThink)();
	}

	if ( m_flSendPickupWeaponMessageTime != -1.f && gpGlobals->curtime >= m_flSendPickupWeaponMessageTime )
	{
		CSingleUserRecipientFilter filter( this );
		filter.MakeReliable();
		UserMessageBegin( filter, "PlayerPickupWeapon" );
		MessageEnd();

		m_flSendPickupWeaponMessageTime = -1.f;
	}

	// In doomsday event, kart can run over ghost to do stuff
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TEAM_ANY, true );
		CUtlVector< CTFPlayer * > ghostVector;
		for ( int i=0; i<playerVector.Count(); ++i )
		{
			if ( playerVector[i] == this )
				continue;

			// touching ghost player?
			// we just check for radius of 100 and assume that we touch to avoid custom collision for ghost
			if ( playerVector[i]->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			{
				if ( ( playerVector[i]->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() < Square( 100 ) )
				{
					ghostVector.AddToTail( playerVector[i] );
				}
			}
		}

		for ( int i=0; i<ghostVector.Count(); ++i )
		{
			CTFPlayer *pGhost = ghostVector[i];
			
			// revive ghost on the same team
			if ( pGhost->GetTeamNumber() == GetTeamNumber() )
			{
				// Trace the ghosts bbox right where they are to see if they collide with enemy players
				trace_t trace;
				Ray_t ray;
				ray.Init( pGhost->GetAbsOrigin(), pGhost->GetAbsOrigin(), pGhost->GetPlayerMins(), pGhost->GetPlayerMaxs() );
				UTIL_TraceRay( ray, PlayerSolidMask(), pGhost, COLLISION_GROUP_PLAYER, &trace );

				// If our trace is clear, spawn that ghost
				if ( trace.fraction == 1.0f )
				{
					// Force the players kart angles to line up with our current ghost angles.
					// This should put us in the kart at the same direction we are currently looking.
					pGhost->ForcePlayerViewAngles( pGhost->GetAbsAngles() );

					pGhost->m_Shared.RemoveCond( TF_COND_HALLOWEEN_GHOST_MODE );
					pGhost->m_Shared.AddCond( TF_COND_HALLOWEEN_KART );
					pGhost->m_Shared.AddCond( TF_COND_HALLOWEEN_IN_HELL ); // keep you in hell to be able to respawn as ghost
					pGhost->m_Shared.AddCond( TF_COND_HALLOWEEN_QUICK_HEAL, 3, this );
					pGhost->EmitSound( "BumperCar.SpawnFromLava" );
					DispatchParticleEffect( "ghost_appearation", PATTACH_ABSORIGIN, pGhost );

					if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
					{
						// achievement for me!
						AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_RESPAWN_TEAMMATES );

						IGameEvent *pEvent = gameeventmanager->CreateEvent( "respawn_ghost" );
						if ( pEvent )
						{
							pEvent->SetInt( "reviver", GetUserID() );
							pEvent->SetInt( "ghost", pGhost->GetUserID() );
							gameeventmanager->FireEvent( pEvent, true );
						}
					}
				}
			}
			else if ( tf_halloween_allow_ghost_hit_by_kart_delay.GetFloat() > 0 && gpGlobals->curtime - pGhost->m_flGhostLastHitByKartTime > tf_halloween_allow_ghost_hit_by_kart_delay.GetFloat() )
			{
				// punt off other team ghost
				float flImpactForce = GetLocalVelocity().Length();
				flImpactForce = MAX( 100.f, flImpactForce ); // add min force
				Vector vOffset = pGhost->WorldSpaceCenter() - WorldSpaceCenter();
				vOffset.z = 0;
				Vector vPuntDir = ( vOffset ).Normalized();
				vPuntDir.z = 0.5f;
				pGhost->ApplyGenericPushbackImpulse( tf_halloween_kart_punting_ghost_force_scale.GetFloat() * flImpactForce * vPuntDir, nullptr );
				pGhost->EmitSound( "BumperCar.HitGhost" );
				pGhost->m_flGhostLastHitByKartTime = gpGlobals->curtime;
			}
		}
	}

	if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() )
	{
		if ( IsUsingActionSlot() && GetActiveTFWeapon() && GetActiveTFWeapon()->GetWeaponID() != TF_WEAPON_GRAPPLINGHOOK )
		{
			CTFGrapplingHook *pGrapplingHook = dynamic_cast< CTFGrapplingHook* >( GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
			if ( pGrapplingHook )
			{
				Weapon_Switch( pGrapplingHook );
			}
		}

		CBaseEntity *pHookTarget = GetGrapplingHookTarget();
		if ( pHookTarget )
		{
			// detatch hook if the object's picked up
			if ( pHookTarget->IsBaseObject() )
			{
				CBaseObject *pObj = assert_cast< CBaseObject* >( pHookTarget );
				if ( pObj->IsCarried() )
				{
					SetGrapplingHookTarget( NULL );
					pHookTarget = NULL;
				}
			}

			// check if something is blocking the player from traveling to the hook target
			if ( pHookTarget )
			{
				trace_t tr;
				CTraceFilterLOS filter( this, COLLISION_GROUP_PLAYER_MOVEMENT, pHookTarget );
				UTIL_TraceLine( WorldSpaceCenter(), pHookTarget->WorldSpaceCenter(), MASK_SOLID, &filter, &tr );
				if ( !tr.DidHit() )
				{
					m_flLastSeenHookTarget = gpGlobals->curtime;
				}
				else if ( gpGlobals->curtime - m_flLastSeenHookTarget > tf_grapplinghook_los_force_detach_time.GetFloat() )
				{
					// force to detach if the hooker lost sight of the target for sometime
					SetGrapplingHookTarget( NULL );
				}
			}
		}
	}

	UpdateCustomAttributes();

	// Time to finish the current random expression? Or time to pick a new one?
	if ( IsAlive() && !IsReadyToTauntWithPartner() && ( m_flNextSpeakWeaponFire < gpGlobals->curtime ) && m_flNextRandomExpressionTime >= 0 && gpGlobals->curtime > m_flNextRandomExpressionTime )
	{
		// Random expressions need to be cleared, because they don't loop. So if we
		// pick the same one again, we want to restart it.
		ClearExpression();
		m_iszExpressionScene = NULL_STRING;
		UpdateExpression();
	}

	if ( IsTaunting() )
	{
		if ( !m_strTauntSoundName.IsEmpty() && m_flTauntSoundTime > 0 && m_flTauntSoundTime <= gpGlobals->curtime )
		{
			EmitSound( m_strTauntSoundName.String() );
			m_flTauntSoundTime = 0.f;
		}

		if ( !m_strTauntSoundLoopName.IsEmpty() && m_flTauntSoundLoopTime > 0 && m_flTauntSoundLoopTime <= gpGlobals->curtime )
		{
			CReliableBroadcastRecipientFilter filter;
			UserMessageBegin( filter, "PlayerTauntSoundLoopStart" );
				WRITE_BYTE( entindex() );
				WRITE_STRING( m_strTauntSoundLoopName.String() );
			MessageEnd();

			m_flTauntSoundLoopTime = 0.f;
		}
		
		// play taunt outro
		if ( m_flTauntOutroTime > 0.f && m_flTauntOutroTime <= gpGlobals->curtime )
		{
			m_bAllowedToRemoveTaunt = true;
			float flDuration = PlayTauntOutroScene();
			m_flTauntRemoveTime = gpGlobals->curtime + flDuration;
			m_flTauntOutroTime = 0.f;
		}
	}

	// Halloween Hacks
	// Spell Casting on Attack1
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		// Check if this is the spellbook so we can save off info to preserve weapon switching
		CTFSpellBook *pSpellBook = dynamic_cast<CTFSpellBook*>( GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
		if ( pSpellBook )
		{
			// cast Spell
			if ( m_nButtons & IN_ATTACK )
			{
				if ( pSpellBook )
				{
					pSpellBook->PrimaryAttack();
				}
			}
		}

		// Speed Boost
		if ( m_nButtons & IN_ATTACK2 )
		{
			if ( GetKartSpeedBoost() >= 1.0f )
			{
				m_flKartNextAvailableBoost = gpGlobals->curtime + tf_halloween_kart_boost_recharge.GetFloat();
				m_Shared.AddCond( TF_COND_HALLOWEEN_KART_DASH, tf_halloween_kart_boost_duration.GetFloat() );
			}
		}
	}

	CBaseEntity *pGroundEntity = GetGroundEntity();

	// We consider players "in air" if they have no ground entity and they're not in water.
	if ( pGroundEntity == NULL && GetWaterLevel() == WL_NotInWater )
	{
		if ( m_iLeftGroundHealth < 0 )
		{
			m_iLeftGroundHealth = GetHealth();
		}
	}
	else
	{
		m_iLeftGroundHealth = -1;

		if ( m_iBlastJumpState )
		{
			const char *pszEvent = NULL;

			if ( StickyJumped() )
			{
				pszEvent = "sticky_jump_landed";
			}
			else if ( RocketJumped() )
			{
				pszEvent = "rocket_jump_landed";
			}

			ClearBlastJumpState();

			if ( pszEvent )
			{
				IGameEvent * event = gameeventmanager->CreateEvent( pszEvent );
				if ( event )
				{
					event->SetInt( "userid", GetUserID() );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}

	if( IsTaunting() )
	{
		bool bStopTaunt = false;
		// if I'm not supposed to move during taunt
		// stop taunting if I lost my ground entity or was moved at all
		if ( !CanMoveDuringTaunt() )
		{
			bStopTaunt |= pGroundEntity == NULL;
			
			if ( m_TauntEconItemView.IsValid() && m_TauntEconItemView.GetStaticData()->GetTauntData()->ShouldStopTauntIfMoved() )
				bStopTaunt |= m_vecTauntStartPosition.DistToSqr( GetAbsOrigin() ) > 0.1f;
		}

		if ( !bStopTaunt  )
		{
			bStopTaunt |= ShouldStopTaunting();
		}

		if ( bStopTaunt )
		{
			CancelTaunt();
		}
	}

	if ( ( RocketJumped() || StickyJumped() ) && IsAlive() && m_bCreatedRocketJumpParticles == false )
	{
		const char *pEffectName = "rocketjump_smoke";
		DispatchParticleEffect( pEffectName, PATTACH_POINT_FOLLOW, this, "foot_L" );
		DispatchParticleEffect( pEffectName, PATTACH_POINT_FOLLOW, this, "foot_R" );
		m_bCreatedRocketJumpParticles = true;
	}

	if ( !m_bCollideWithSentry )
	{
		if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			CBaseObject	*pSentry = GetObjectOfType( OBJ_SENTRYGUN );
			if ( !pSentry )
			{
				m_bCollideWithSentry = true;
			}
			else
			{
				if ( ( pSentry->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() > 2500 )
				{
					m_bCollideWithSentry = true;
				}
			}
		}
		else
		{
			m_bCollideWithSentry = true;
		}
	}

	if ( gpGlobals->curtime > m_flCommentOnCarrying && (m_flCommentOnCarrying != 0.f) )
	{
		m_flCommentOnCarrying = 0.f;

		CBaseObject* pObj = m_Shared.GetCarriedObject();
		if ( pObj )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_CARRYING_BUILDING, pObj->GetResponseRulesModifier() );
		}
	}

#ifdef TF_RAID_MODE
	CTFNavArea *area = (CTFNavArea *)GetLastKnownArea();
	if ( area && area->HasAttributeTF( TF_NAV_RESCUE_CLOSET ) )
	{
		// we're standing in a rescue closet and need a friend to let us out - call for help!
		SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_HELP );
	}
#endif

	// Wrenchmotron taunt effect
	if ( m_bIsTeleportingUsingEurekaEffect )
	{
		if ( m_teleportHomeFlashTimer.HasStarted() && m_teleportHomeFlashTimer.IsElapsed() )
		{
			m_teleportHomeFlashTimer.Invalidate();

			if ( !tf_test_teleport_home_fx.GetBool() )
			{
				// cover up the end of the taunt with a flash
				color32 colorHit = { 255, 255, 255, 255 };
				UTIL_ScreenFade( this, colorHit, 0.25f, 0.25f, FFADE_IN );
			}

			Vector origin = GetAbsOrigin();
			CPVSFilter filter( origin );

			UserMessageBegin( filter, "PlayerTeleportHomeEffect" );
				WRITE_BYTE( entindex() );
			MessageEnd();

			// DispatchParticleEffect( "drg_wrenchmotron_teleport", PATTACH_ABSORIGIN );

			switch( GetTeamNumber() )
			{
			case TF_TEAM_RED:
				TE_TFParticleEffect( filter, 0.0, "teleported_red", origin, vec3_angle );
				TE_TFParticleEffect( filter, 0.0, "player_sparkles_red", origin, vec3_angle, this, PATTACH_POINT );
				break;
			case TF_TEAM_BLUE:
				TE_TFParticleEffect( filter, 0.0, "teleported_blue", origin, vec3_angle );
				TE_TFParticleEffect( filter, 0.0, "player_sparkles_blue", origin, vec3_angle, this, PATTACH_POINT );
				break;
			default:
				break;
			}
		}

		// teleport home when taunt finishes
		if ( !IsTaunting() )
		{
			// drop the intel and any powerup we are carrying
			DropFlag();
			DropRune();

			EmitSound( "Building_Teleporter.Send" );
			m_bIsTeleportingUsingEurekaEffect = false;

			CObjectTeleporter* pTeleExit = assert_cast< CObjectTeleporter* >( GetObjectOfType( OBJ_TELEPORTER, MODE_TELEPORTER_EXIT ) );
			 
			// Check if they wanted to go to their teleporter AND their teleporter can accept them
			if ( m_eEurekaTeleportTarget == EUREKA_TELEPORT_TELEPORTER_EXIT && pTeleExit && ( pTeleExit->GetState() != TELEPORTER_STATE_BUILDING ) )
			{
				pTeleExit->RecieveTeleportingPlayer( this );
			}
			else
			{
				// Default to the spawn
				TFGameRules()->GetPlayerSpawnSpot( this );
			}
		}
	}

	// Send active weapon's clip state to attached medics
	bool bSendClipInfo = gpGlobals->curtime > m_flNextClipSendTime &&
						 m_Shared.GetNumHealers() &&
						 IsAlive();
	if ( bSendClipInfo )
	{
		CTFWeaponBase *pTFWeapon = GetActiveTFWeapon();
		if ( pTFWeapon )
		{
			int nClip = 0;

			if ( m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				nClip = m_Shared.GetDisguiseAmmoCount();
			}
			else
			{
				nClip = pTFWeapon->UsesClipsForAmmo1() ? pTFWeapon->Clip1() : GetAmmoCount( pTFWeapon->GetPrimaryAmmoType() );
			}

			if ( nClip >= 0 && nClip != m_nActiveWpnClipPrev )
			{
				if ( nClip > 500 )
				{
					Warning( "Heal Target: ClipSize Data Limit Exceeded: %d (max 500)\n", nClip );
					nClip = MIN( nClip, 500 );
				}
				m_nActiveWpnClip.Set( nClip );
				m_nActiveWpnClipPrev = m_nActiveWpnClip;
				m_flNextClipSendTime = gpGlobals->curtime + 0.25f;
			}
		}
	}

	if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY && ( GetFlags() & FL_DUCKING ) && ( pGroundEntity != NULL ) )
	{
		int nDisguiseAsDispenserOnCrouch = 0;
		CALL_ATTRIB_HOOK_FLOAT( nDisguiseAsDispenserOnCrouch, disguise_as_dispenser_on_crouch );
		if ( nDisguiseAsDispenserOnCrouch != 0 )
		{
			m_Shared.AddCond( TF_COND_DISGUISED_AS_DISPENSER, 0.5f );
		}
	}

	// rune charge over time
	if ( m_Shared.CanRuneCharge() && !m_Shared.IsRuneCharged() )
	{
		float dt = gpGlobals->curtime - m_flLastRuneChargeUpdate;
		float flAdd = dt * 100.f / tf_powerup_max_charge_time.GetFloat();
		m_Shared.SetRuneCharge( m_Shared.GetRuneCharge() + flAdd );

		if (m_Shared.GetCarryingRuneType() == RUNE_SUPERNOVA && m_Shared.IsRuneCharged() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_Supernova_Deploy" );
		}
	}
	m_flLastRuneChargeUpdate = gpGlobals->curtime;

	// Mannpower dominant clean up and checks
	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		if ( m_bIsInMannpowerDominantCondition )
		{
			// Clear Mannpower dominant condition when the time is up
			if ( ( gpGlobals->curtime >= m_flRemoveDominantConditionTime ) )
			{
				EndPowerupModeDominant();
			}
			// other events can clear the marked for death condition while we're still in the dominant state so we need to add it back
			else if ( m_Shared.GetCarryingRuneType() != RUNE_NONE && !m_Shared.InCond( TF_COND_MARKEDFORDEATH ) )
			{
				m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, 5.0f );
			}
		}
	}

	// You can't touch a hooked target, so transmit plague when you get as close as you can
	if ( GetGrapplingHookTarget() && GetGrapplingHookTarget()->IsPlayer() && m_Shared.GetCarryingRuneType() == RUNE_PLAGUE )
	{
		CTFPlayer *pHookedPlayer = ToTFPlayer( GetGrapplingHookTarget() );

		float flDistSqrToTarget = GetAbsOrigin().DistToSqr( pHookedPlayer->GetAbsOrigin() );
		if ( flDistSqrToTarget < 8100 && !pHookedPlayer->m_Shared.InCond( TF_COND_PLAGUE ) &&
			!m_Shared.IsAlly( pHookedPlayer ) &&
			!pHookedPlayer->m_Shared.IsInvulnerable() && 
			pHookedPlayer->m_Shared.GetCarryingRuneType() != RUNE_RESIST )
		{
			pHookedPlayer->m_Shared.AddCond( TF_COND_PLAGUE, PERMANENT_CONDITION, this );
		}
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		// prevents player from standing on bot's head to block its movement.
		if ( pGroundEntity && pGroundEntity->IsPlayer() )
		{
			Vector vPush = GetAbsOrigin() - pGroundEntity->GetAbsOrigin();
			vPush.z = 0.f;
			vPush.NormalizeInPlace();
			vPush.z = 1.f;
			vPush *= 100.f;

			ApplyAbsVelocityImpulse( vPush );
		}
	}

	// Scale our head
	m_flHeadScale = Approach( GetDesiredHeadScale(), m_flHeadScale, GetHeadScaleSpeed() );

	// scale our torso
	m_flTorsoScale = Approach( GetDesiredTorsoScale(), m_flTorsoScale, GetTorsoScaleSpeed() );

	// scale our torso
	m_flHandScale = Approach( GetDesiredHandScale(), m_flHandScale, GetHandScaleSpeed() );

/*
#ifdef STAGING_ONLY
	if ( m_Shared.InCond( TF_COND_SPACE_GRAVITY ) )
	{
		// JetPack testing
		if ( m_nButtons & IN_JUMP && !( GetFlags() & FL_ONGROUND ) && m_Shared.GetSpaceJumpChargeMeter() > tf_space_thrust_use_rate.GetFloat() )
		{
			//mv->m_vecVelocity[2] += 10.0f;
			Vector vThrust = Vector(0,0,0);
			switch( GetPlayerClass()->GetClassIndex() )
			{
			case TF_CLASS_SCOUT :				vThrust.z = tf_space_thrust_scout.GetFloat();				break;
			case TF_CLASS_SNIPER :				vThrust.z = tf_space_thrust_sniper.GetFloat();				break;
			case TF_CLASS_SOLDIER :				vThrust.z = tf_space_thrust_soldier.GetFloat();				break;
			case TF_CLASS_DEMOMAN :				vThrust.z = tf_space_thrust_demo.GetFloat();				break;
			case TF_CLASS_MEDIC :				vThrust.z = tf_space_thrust_medic.GetFloat();				break;
			case TF_CLASS_HEAVYWEAPONS :		vThrust.z = tf_space_thrust_heavy.GetFloat();				break;
			case TF_CLASS_PYRO :				vThrust.z = tf_space_thrust_pyro.GetFloat();				break;
			case TF_CLASS_SPY :					vThrust.z = tf_space_thrust_spy.GetFloat();				break;
			case TF_CLASS_ENGINEER :			vThrust.z = tf_space_thrust_engy.GetFloat();				break;
			}

			ApplyAbsVelocityImpulse( vThrust );

			m_Shared.SetSpaceJumpChargeMeter( m_Shared.GetSpaceJumpChargeMeter() - tf_space_thrust_use_rate.GetFloat() );
		}
		else
		{
			if (  GetFlags() & FL_ONGROUND )
			{
				m_Shared.SetSpaceJumpChargeMeter( m_Shared.GetSpaceJumpChargeMeter() + tf_space_thrust_recharge_rate.GetFloat() );
			}
		}
	}
#endif
*/

	SetContextThink( &CTFPlayer::TFPlayerThink, gpGlobals->curtime, "TFPlayerThink" );
	m_flLastThinkTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a portion of health every think.
//-----------------------------------------------------------------------------
void CTFPlayer::RegenThink( void )
{
	if ( !IsAlive() )
		return;

	// Queue the next think
	SetContextThink( &CTFPlayer::RegenThink, gpGlobals->curtime + TF_REGEN_TIME, "RegenThink" );

	// if we're going in to this too often, quit out.
	if ( m_flLastHealthRegenAt + TF_REGEN_TIME > gpGlobals->curtime )
		return;

	bool bShowRegen = true;

	// Medic has a base regen amount
	if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
	{
		// Heal faster if we haven't been in combat for a while.
		float flTimeSinceDamage = gpGlobals->curtime - GetLastDamageReceivedTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 5.0f, 10.0f, 1.0f, 2.0f );
		float flRegenAmt = TF_REGEN_AMOUNT;

		// If you are healing a hurt patient, increase your base regen
		CTFPlayer *pPatient = ToTFPlayer( MedicGetHealTarget() );
		if ( pPatient && pPatient->GetHealth() < pPatient->GetMaxHealth() )
		{
			// Double regen amount
			flRegenAmt += TF_REGEN_AMOUNT;
		}

		flRegenAmt *= flScale;

		// If the medic has this attribute, increase their regen.
		if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
		{
			int iHealingMastery = 0;
			CALL_ATTRIB_HOOK_INT( iHealingMastery, healing_mastery );
			if ( iHealingMastery )
			{
				float flPerc = RemapValClamped( (float)iHealingMastery, 1.f, 4.f, 1.25f, 2.f );
				flRegenAmt *= flPerc;
			}
		}

		m_flAccumulatedHealthRegen += flRegenAmt;

		bShowRegen = false;
	}

	// Other classes can be regenerated by items
	float flRegenAmount = 0;
	CALL_ATTRIB_HOOK_FLOAT( flRegenAmount, add_health_regen );
	if ( flRegenAmount )
	{
		float flTimeSinceDamage = gpGlobals->curtime - GetLastDamageReceivedTime();
		float flScale = 1.0f;
		// Ignore Scale for MvM, always give full regen
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			flScale = 1.0f;
		}
		else if ( flTimeSinceDamage < 5.0f )
		{
			flScale = 0.25f;
		}
		else
		{
			flScale = RemapValClamped( flTimeSinceDamage, 5.0f, 10.0f, 0.5f, 1.0f );
		}
		
		flRegenAmount *= flScale;
	}
	m_flAccumulatedHealthRegen += flRegenAmount;

// 	if ( m_Shared.InCond( TF_COND_HEALING_DEBUFF ) )
// 	{
// 		m_flAccumulatedHealthRegen *= ( 1.f - PYRO_AFTERBURN_HEALING_REDUCTION );
// 	}

	int nHealAmount = 0;
	if ( m_flAccumulatedHealthRegen >= 1.f )
	{
		nHealAmount = floor( m_flAccumulatedHealthRegen );
		if ( GetHealth() < GetMaxHealth() )
		{
			int nHealedAmount = TakeHealth( nHealAmount, DMG_GENERIC | DMG_IGNORE_DEBUFFS );
			if ( nHealedAmount > 0 )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
				if ( event )
				{
					event->SetInt( "priority", 1 );	// HLTV event priority
					event->SetInt( "patient", GetUserID() );
					event->SetInt( "healer", GetUserID() );
					event->SetInt( "amount", nHealedAmount );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
	else if ( m_flAccumulatedHealthRegen < -1.f )
	{
		nHealAmount = ceil( m_flAccumulatedHealthRegen );
		TakeDamage( CTakeDamageInfo( this, this, NULL, vec3_origin, WorldSpaceCenter(), nHealAmount * -1, DMG_GENERIC ) );
	}

	if ( GetHealth() < GetMaxHealth() && nHealAmount != 0 && bShowRegen )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", nHealAmount );
			event->SetInt( "entindex", entindex() );
			event->SetInt( "weapon_def_index", INVALID_ITEM_DEF_INDEX );
			gameeventmanager->FireEvent( event ); 
		}
	}

	m_flAccumulatedHealthRegen -= nHealAmount;
	m_flLastHealthRegenAt = gpGlobals->curtime;

	// Regenerate ammo 
	if ( m_flNextAmmoRegenAt < gpGlobals->curtime )
	{
		// We regen ammo every 5 seconds
		m_flNextAmmoRegenAt = gpGlobals->curtime + 5.0;

		flRegenAmount = 0;
		CALL_ATTRIB_HOOK_FLOAT( flRegenAmount, addperc_ammo_regen );

		if ( flRegenAmount )
		{
			RegenAmmoInternal( TF_AMMO_PRIMARY, flRegenAmount );
			RegenAmmoInternal( TF_AMMO_SECONDARY, flRegenAmount );
		}

		// Regenerate metal
		int iMetal = 0;
		CALL_ATTRIB_HOOK_INT( iMetal, add_metal_regen );

		if ( iMetal )
		{
			GiveAmmo( iMetal, TF_AMMO_METAL, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns a portion of health every think.
//-----------------------------------------------------------------------------
void CTFPlayer::RuneRegenThink( void )
{
	if ( !IsAlive() )
		return;

	// Queue the next think
	SetContextThink( &CTFPlayer::RuneRegenThink, gpGlobals->curtime + TF_REGEN_TIME_RUNE, "RuneRegenThink" );

	// if we're going in to this too often, quit out.
	if ( m_flLastRuneHealthRegenAt + TF_REGEN_TIME_RUNE > gpGlobals->curtime )
		return;

	int nRuneType = m_Shared.GetCarryingRuneType();
	if ( nRuneType == RUNE_NONE && !HasTheFlag() )
		return;

	// Regenerate health 
	float flAmount = 0.f;
	switch ( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_SCOUT:
	case TF_CLASS_SPY:
		flAmount = 16;
		break;
	case TF_CLASS_SNIPER:
	case TF_CLASS_ENGINEER:
		flAmount = 14;
		break;
	case TF_CLASS_MEDIC:
	case TF_CLASS_DEMOMAN:
	case TF_CLASS_PYRO:
		flAmount = 12;
		break;
	case TF_CLASS_SOLDIER:
		flAmount = 10;
		break;
	case TF_CLASS_HEAVYWEAPONS:
		flAmount = 8;
		break;
	}
	if ( nRuneType == RUNE_REGEN )
	{
		if ( m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) )
		{
			flAmount *= 0.7f;
		}
	
		m_flAccumulatedRuneHealthRegen += flAmount;
	}
	// King and buffed team mates get some health regeneration unless they have the plague
	else if ( ( nRuneType == RUNE_KING || m_Shared.InCond( TF_COND_KING_BUFFED ) ) && !m_Shared.InCond( TF_COND_PLAGUE ) )
	{
		flAmount *= 0.3;
		m_flAccumulatedRuneHealthRegen += flAmount;
	}
	// non powered up flag carriers get a small health regeneration
	else if ( HasTheFlag() && nRuneType == RUNE_NONE )
	{
		flAmount *= 0.1;
		m_flAccumulatedRuneHealthRegen += flAmount;
	}

	int nHealAmount = 0;
	if ( m_flAccumulatedRuneHealthRegen >= 1.0 )
	{
		nHealAmount = floor( m_flAccumulatedRuneHealthRegen );
		if ( GetHealth() < GetMaxHealth() )
		{
			TakeHealth( nHealAmount, DMG_GENERIC );
			int nHealedAmount = TakeHealth( nHealAmount, DMG_GENERIC );
			if ( nHealedAmount > 0 )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
				if ( event )
				{
					event->SetInt("priority", 1);	// HLTV event priority
					event->SetInt("patient", GetUserID());
					event->SetInt( "healer", GetUserID() );
					event->SetInt( "amount", nHealedAmount );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
	else if ( m_flAccumulatedRuneHealthRegen < -1.0 )
	{
		nHealAmount = ceil( m_flAccumulatedRuneHealthRegen );
		TakeDamage( CTakeDamageInfo( this, this, NULL, vec3_origin, WorldSpaceCenter(), nHealAmount * -1, DMG_GENERIC ) );
	}

	if ( GetHealth() < GetMaxHealth() && nHealAmount != 0 )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", nHealAmount );
			event->SetInt( "entindex", entindex() );
			event->SetInt( "weapon_def_index", INVALID_ITEM_DEF_INDEX );
			gameeventmanager->FireEvent( event );
		}
	}

	m_flAccumulatedRuneHealthRegen -= nHealAmount;
	m_flLastRuneHealthRegenAt = gpGlobals->curtime;

	// Regenerate ammo and metal
	if ( m_flNextRuneAmmoRegenAt < gpGlobals->curtime )
	{
		m_flNextRuneAmmoRegenAt = gpGlobals->curtime + 5;

		if ( nRuneType == RUNE_REGEN )
		{
			RegenAmmoInternal( TF_AMMO_PRIMARY, 0.5f );
			RegenAmmoInternal( TF_AMMO_SECONDARY, 0.5f );
			GiveAmmo( 200, TF_AMMO_METAL, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RegenAmmoInternal( int iIndex, float flRegen )
{
	m_flAccumulatedAmmoRegens[iIndex] += flRegen;
	
	// As soon as we have enough accumulated to regen a single unit of ammo, do it.
	int iMaxAmmo = GetMaxAmmo(iIndex);
	int iAmmo = m_flAccumulatedAmmoRegens[iIndex] * iMaxAmmo;
	if ( iAmmo >= 1 )
	{
		GiveAmmo( iAmmo, iIndex, true );
		m_flAccumulatedAmmoRegens[iIndex] -= ((float)iAmmo / (float)iMaxAmmo);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer::~CTFPlayer()
{
	delete [] m_damageRateArray;

	DestroyRagdoll();
	m_PlayerAnimState->Release();

	FOR_EACH_VEC( m_ItemsToTest, i )
	{
		int iDef = TESTITEM_DEFINITIONS_BEGIN_AT + m_ItemsToTest[i].scriptItem.GetItemDefIndex();
		ItemSystem()->GetItemSchema()->ItemTesting_DiscardTestDefinition( iDef );

		m_ItemsToTest[i].pKV->deleteThis();
		m_ItemsToTest[i].pKV = NULL;
	}

	if ( m_hReviveMarker )
	{
		UTIL_Remove( m_hReviveMarker );
		m_hReviveMarker = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayer::CreatePlayer( const char *className, edict_t *ed )
{
	CTFPlayer::s_PlayerEdict = ed;
	return (CTFPlayer*)CreateEntityByName( className );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateTimers( void )
{
	m_Shared.SharedThink();
}

//-----------------------------------------------------------------------------
// Purpose: One-time, 100ms post spawn
//-----------------------------------------------------------------------------
void CTFPlayer::PostSpawnThink( void )
{
	if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>( Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		if ( pMedigun )
		{
				pMedigun->SetChargeLevelToPreserve( 0.f );
		}
	}
}

//-----------------------------------------------------------------------------
// Estimate where a projectile fired from the given weapon will initially hit (it may bounce on from there).
// NOTE: We should be able to directly compute this knowing initial velocity, angle, gravity, etc, 
// but I have been unable to find a formula that reproduces what our physics actually
// do.
//-----------------------------------------------------------------------------
Vector CTFPlayer::EstimateProjectileImpactPosition( CTFWeaponBaseGun *weapon )
{
	if ( !weapon )
	{
		return GetAbsOrigin();
	}

	const QAngle &angles = EyeAngles();

	float initVel = weapon->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) ? 900.0f : weapon->GetProjectileSpeed();  
	CALL_ATTRIB_HOOK_FLOAT( initVel, mult_projectile_range );

	return EstimateProjectileImpactPosition( angles.x, angles.y, initVel );
}

//-----------------------------------------------------------------------------
// Estimate where a stickybomb projectile will hit,
// using given pitch, yaw, and weapon charge (0-1)
//-----------------------------------------------------------------------------
Vector CTFPlayer::EstimateStickybombProjectileImpactPosition( float pitch, float yaw, float charge )
{
	// estimate impact spot
	float initVel = charge * ( TF_PIPEBOMB_MAX_CHARGE_VEL - TF_PIPEBOMB_MIN_CHARGE_VEL ) + TF_PIPEBOMB_MIN_CHARGE_VEL;
	CALL_ATTRIB_HOOK_FLOAT( initVel, mult_projectile_range );

	return EstimateProjectileImpactPosition( pitch, yaw, initVel );
}

//-----------------------------------------------------------------------------
// Estimate where a projectile fired will initially hit (it may bounce on from there),
// using given pitch, yaw, and initial velocity.
//-----------------------------------------------------------------------------
Vector CTFPlayer::EstimateProjectileImpactPosition( float pitch, float yaw, float initVel )
{
	// copied from CTFWeaponBaseGun::FirePipeBomb()
	Vector vecForward, vecRight, vecUp;
	QAngle angles( pitch, yaw, 0.0f );
	AngleVectors( angles, &vecForward, &vecRight, &vecUp );

	// we will assume bots never flip viewmodels
	float fRight = 8.f;
	Vector vecSrc = Weapon_ShootPosition();
	vecSrc += vecForward * 16.0f + vecRight * fRight + vecUp * -6.0f;

	const float initVelScale = 0.9f;
	Vector vecVelocity = initVelScale * ( ( vecForward * initVel ) + ( vecUp * 200.0f ) );

	const float timeStep = 0.01f;
	const float maxTime = 5.0f;
								 
	Vector pos = vecSrc;
	Vector lastPos = pos;
	const float g = GetCurrentGravity();


	// compute forward facing unit vector in horiz plane
	Vector alongDir = vecForward;
	alongDir.z = 0.0f;
	alongDir.NormalizeInPlace();

	float alongVel = FastSqrt( vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y );

	trace_t trace;	
	trace.endpos = vec3_origin; // misyl: Making the compiler happy :)
	NextBotTraceFilterIgnoreActors traceFilter( NULL, COLLISION_GROUP_NONE );

	float t;
	for( t = 0.0f; t < maxTime; t += timeStep )
	{
		float along = alongVel * t;
		float height = vecVelocity.z * t - 0.5f * g * t * t;

		pos.x = vecSrc.x + alongDir.x * along;
		pos.y = vecSrc.y + alongDir.y * along;
		pos.z = vecSrc.z + height;

		UTIL_TraceHull( lastPos, pos, -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

		if ( trace.DidHit() )
		{
			break;
		}


		lastPos = pos;
	}

	return trace.endpos;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	// TF Players only process scene events on the server while running taunts
	if ( !IsTaunting() )
		return false;

	// Only process sequences
	if ( event->GetType() != CChoreoEvent::SEQUENCE )
		return false;

	return BaseClass::ProcessSceneEvent( info, scene, event );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PreThink()
{
	// Update timers.
	UpdateTimers();

	// Pass through to the base class think.
	BaseClass::PreThink();

	// Reset bullet force accumulator, only lasts one frame, for ragdoll forces from multiple shots.
	m_vecTotalBulletForce = vec3_origin;

	CheckForIdle();

	ProcessSceneEvents();

	if ( TFGameRules()->IsInArenaMode() == true )
	{
		if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
		{
			if ( GetTeamNumber() == TEAM_SPECTATOR )
			{
				m_Local.m_iHideHUD &= ~HIDEHUD_MISCSTATUS;
			}
		}
	}

	// Hype Decreases over time
	if ( IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		float flHypeDecays = 0;
		CALL_ATTRIB_HOOK_FLOAT( flHypeDecays, hype_decays_over_time );

		if ( flHypeDecays != 0 )
		{
			// Loose hype over time
			float flHype = m_Shared.GetScoutHypeMeter();
			flHype = flHype - flHypeDecays;
			m_Shared.SetScoutHypeMeter( flHype );
			TeamFortress_SetSpeed();
		}
	}

}

ConVar mp_idledealmethod( "mp_idledealmethod", "1", FCVAR_GAMEDLL, "Deals with Idle Players. 1 = Sends them into Spectator mode then kicks them if they're still idle, 2 = Kicks them out of the game;" );
ConVar mp_idlemaxtime( "mp_idlemaxtime", "3", FCVAR_GAMEDLL, "Maximum time a player is allowed to be idle (in minutes)" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CheckForIdle( void )
{
	if ( m_afButtonLast != m_nButtons )
		m_flLastAction = gpGlobals->curtime;

	if ( mp_idledealmethod.GetInt() )
	{
		if ( IsHLTV() || IsReplay() )
			return;

		if ( IsFakeClient() )
			return;

		if ( IsCoaching() && GetStudent() != NULL )
			return;

		if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
			return;

		if ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS )
			return;

		//Don't mess with the host on a listen server (probably one of us debugging something)
		if ( engine->IsDedicatedServer() == false && entindex() == 1 )
			return;

		if ( IsAutoKickDisabled() )
			return;

		const bool cbMoving = ( m_nButtons & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) != 0;

		m_bIsAFK = false;

		if ( !cbMoving && PointInRespawnRoom( this, WorldSpaceCenter() ) )
		{
			m_flTimeInSpawn += TICK_INTERVAL;
		}
		else
			m_flTimeInSpawn = 0;

		if ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() == true )
		{
			if ( GetTeamNumber() == TEAM_SPECTATOR )
				return;

			if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() != GetTeamNumber() )
			{
				if ( m_bArenaIsAFK )
				{
					m_bIsAFK = true;
					m_bArenaIsAFK = false;
				}
			}
		}
		else
		{
			// Cannot possibly get out of the spawn room in 0 seconds--so if the ConVar says 0, let's assume 30 seconds.
			float flIdleTime = Max( mp_idlemaxtime.GetFloat() * 60, 30.0f );

			if ( TFGameRules()->InStalemate() )
			{
				flIdleTime = mp_stalemate_timelimit.GetInt() * 0.5f;
			}

			m_bIsAFK = ( gpGlobals->curtime - m_flLastAction ) > flIdleTime
			        || ( m_flTimeInSpawn > flIdleTime ); 
		}
		
		if ( m_bIsAFK == true )
		{
			bool bKickPlayer = false;

			ConVarRef mp_allowspectators( "mp_allowspectators" );
			if ( ( mp_allowspectators.IsValid() && mp_allowspectators.GetBool() == false ) || ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() ) )
			{
				// just kick the player if this server doesn't allow spectators
				bKickPlayer = true;
			}
			else if ( mp_idledealmethod.GetInt() == 1 )
			{
				if ( GetTeamNumber() < FIRST_GAME_TEAM )
				{
					bKickPlayer = true;
				}
				else
				{
					//First send them into spectator mode then kick him.
					ForceChangeTeam( TEAM_SPECTATOR );
					m_flLastAction = gpGlobals->curtime;
					m_flTimeInSpawn = 0;
					return;
				}
			}
			else if ( mp_idledealmethod.GetInt() == 2 )
			{
				bKickPlayer = true;
			}

			if ( bKickPlayer == true )
			{
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#game_idle_kick", GetPlayerName() );
				engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", GetUserID(), g_pszIdleKickString ) );
				m_flLastAction = gpGlobals->curtime;
				m_flTimeInSpawn = 0;
			}
		}
	}
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FlashlightTurnOff( void )
{
	if( IsEffectActive(EF_DIMLIGHT) )
	{
		RemoveEffects( EF_DIMLIGHT );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Update Halloween scenario effects on players.
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateHalloween( void )
{
	// This is a push force
	if ( !m_vHalloweenKartPush.IsZero() )
	{
		if ( !m_Shared.InCond( TF_COND_INVULNERABLE_USER_BUFF ) )
		{		
			CPVSFilter filter( GetAbsOrigin() );
			TE_TFParticleEffect( filter, 0.0, "kart_impact_sparks", GetAbsOrigin(), vec3_angle, this, PATTACH_ABSORIGIN );

			float flStunDuration = m_vHalloweenKartPush.Length() / 1000.0f;
			if ( m_vHalloweenKartPush.LengthSqr() > 1000 * 1000 )
			{
				TE_TFParticleEffect( filter, 0.0, "kartimpacttrail", GetAbsOrigin(), vec3_angle, this, PATTACH_ABSORIGIN_FOLLOW );
				EmitSound( "BumperCar.BumpIntoAir" );
				EmitSound( "BumperCar.BumpHard" );
			}
			else
			{
				EmitSound( "BumperCar.Bump" );
			}

			if ( tf_halloween_kart_stun_enabled.GetBool() )
			{
				m_Shared.StunPlayer( flStunDuration * tf_halloween_kart_stun_duration_scale.GetFloat(), tf_halloween_kart_stun_amount.GetFloat(), TF_STUN_BOTH | TF_STUN_NO_EFFECTS );
			}

			if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART_DASH ) )
			{
				m_Shared.RemoveCond( TF_COND_HALLOWEEN_KART_DASH );
			}

			ApplyGenericPushbackImpulse( m_vHalloweenKartPush, nullptr );
		}
		
		m_vHalloweenKartPush.Zero();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AddHalloweenKartPushEvent( CTFPlayer *pOther, CBaseEntity *pInflictor, CBaseEntity *pWeapon, Vector vForce, int iDamage, int iDamageType /* = 0 */ )
{
	// Create a damage event so they can get credit for the kill
	//m_vHalloweenKartPushEventTime + 0.2 > gpGlobals->curtime &&
	if ( m_Shared.InCond( TF_COND_INVULNERABLE_USER_BUFF ) )
		return;

	// Ignore small forces
	float flForce = vForce.LengthSqr();
	if ( flForce < 100.0f )
		return;

	float flExtraMultiplier = 1.f;
	if ( pOther && pOther != this && pOther->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
	{
		iDamage *= tf_halloween_kart_bomb_head_damage_scale.GetFloat();
		flExtraMultiplier = tf_halloween_kart_bomb_head_impulse_scale.GetFloat();
		pOther->SetKartBombHeadTarget( pOther );
		pOther->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );
	}

	const float flCurrentKartKnockbackMultiplier =  GetKartKnockbackMultiplier( flExtraMultiplier );

	if ( pOther )
	{
		// Fake Damage
		IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
		if ( event )
		{
			int iKartDamageType = DMG_CLUB | DMG_PREVENT_PHYSICS_FORCE;
			if ( pOther != this && flCurrentKartKnockbackMultiplier * vForce.LengthSqr() > 1000 * 1000 )
			{
				iKartDamageType |= DMG_CRITICAL;
			}

			event->SetInt( "userid", GetUserID() );
			event->SetInt( "health", MAX( 0, m_iHealth ) );

			// HLTV event priority, not transmitted
			event->SetInt( "priority", 5 );
			event->SetInt( "damageamount", iDamage );

			// Hurt by another player.
			event->SetInt( "attacker", pOther->GetUserID() );
			event->SetInt( "custom", TF_DMG_CUSTOM_SUICIDE );
			event->SetBool( "crit", ( iKartDamageType & DMG_CRITICAL ) != 0 );
			event->SetBool( "allseecrit", ( iKartDamageType & DMG_CRITICAL ) != 0 );
			event->SetInt( "bonuseffect", (int)kBonusEffect_None );
			//
			gameeventmanager->FireEvent( event );
		}
		
		m_AchievementData.AddDamagerToHistory( pOther );
		m_AchievementData.AddPusherToHistory( pOther );
		//m_Shared.SetAssist( pOther );
	}

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		m_iKartHealth += iDamage;
	}

	// HHH
	if ( TFGameRules()->IsIT( pOther ) )
	{
		// Tag! You're IT!
		TFGameRules()->SetIT( this );
	}

	if ( iDamageType == TF_DMG_CUSTOM_DECAPITATION_BOSS )
	{
		m_flHHHKartAttackTime = gpGlobals->curtime;
	}

	//m_vHalloweenKartPushEventTime = gpGlobals->curtime;
	float flImpulseScale = 1.0f;

	// m_flKartHealth might change by this point, calculate new knock back multiplier
	const float flNewKartKnockbackMultiplier = GetKartKnockbackMultiplier( flExtraMultiplier );

	// push other
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		// If applying damage increase the force, otherwise its likely just a wall collision
		if ( iDamage > 0 )
		{
			vForce *= flImpulseScale * flNewKartKnockbackMultiplier;
			vForce.z *= ( ( flNewKartKnockbackMultiplier - 1.0f ) * 0.20f ) + 1.0f;

			// Decrease all forces if in the air
			if (!(GetFlags() & FL_ONGROUND) )
			{
				vForce *= tf_halloween_kart_impact_air_scale.GetFloat();
			}
		}
	}
	else
	{
		// Make non-karters take damage!
		vForce *= ( flImpulseScale * 2.0f );

		//ghostinfo.SetDamageCustom( TF_DMG_CUSTOM_KART );	
		// Create a damage event based on Speed
		float flDamage = vForce.Length() / 50.0f + RandomFloat( 3.0f, 7.0f );
		CTakeDamageInfo info;
		info.SetAttacker( pOther );
		info.SetInflictor( pOther );
		info.SetDamage( flDamage );
		info.SetDamageCustom( TF_DMG_CUSTOM_KART );
		info.SetDamagePosition( GetAbsOrigin() );
		int iKartDamageType = DMG_CLUB;
		if ( flDamage > 20 )
		{
			iKartDamageType |= DMG_CRITICAL;
		}
		info.SetDamageType( iKartDamageType );
		info.SetDamageForce( vForce );
		TakeDamage( info );
		return;
	}

	m_vHalloweenKartPush += vForce;

	// Dropped collection game tokens if hit hard enough and we're in the collection minigame
	if ( pOther && iDamage > 10 && CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() && 
		CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame()->GetMinigameType() == CTFMiniGame::EMinigameType::MINIGAME_HALLOWEEN2014_COLLECTION )
	{
		CUtlVector< CTFPlayer* > vecEveryone;
		CollectPlayers( &vecEveryone );
		int nNumPlayers = vecEveryone.Count();

		// Drop tokens
		uint32 nNumToSpawn = ( iDamage / 5 ) + 1;
		nNumToSpawn = RemapValClamped( nNumPlayers, 8, 16, nNumToSpawn, 1 );

		if ( gpGlobals->curtime > m_flNextBonusDucksVOAllowedTime )
		{
			// Tell this user to play a "Bonus Ducks!" line. 
			CSingleUserRecipientFilter filter( pOther );
			UserMessageBegin( filter, "BonusDucks" );
				WRITE_BYTE( entindex() );
				WRITE_BYTE( false );
			MessageEnd();
		}

		while( nNumToSpawn-- )
		{
			CHalloweenPickup *pPickup = dynamic_cast< CHalloweenPickup * >( CreateEntityByName( "tf_halloween_pickup" ) );
			if (pPickup)
			{
				pPickup->m_nSkin = 2; // Golden skin
				pPickup->Precache();
				DispatchSpawn(pPickup);
				Vector vecRandom = RandomVector( -200.f, 200.f );
				vecRandom.z = RandomFloat( 300.f, 400.f );
				Vector vecDropVector = vecRandom + vForce * 0.2f;
				pPickup->DropSingleInstance( vecDropVector, this, 1.f, 1.f );

				pPickup->SetAbsOrigin( GetAbsOrigin() + Vector( 0.f, 0.f, 40.f ) );

				pPickup->Activate();
			}
		}
	}
	
	//DevMsg( "Kart Impact %fx,%fy,%fz - %f Base. %f Multiplayer, %f TotalForce, %d Damage, %i Class \n", 
	//	vForce.x, vForce.y, vForce.z, vForce.Length(), flNewKartKnockbackMultiplier, vForce.Length() * flNewKartKnockbackMultiplier, iDamage, GetPlayerClass()->GetClassIndex() );
	
	vForce.z = 0;
	vForce.NormalizeInPlace();
	DispatchParticleEffect( "kart_impact_sparks", GetAbsOrigin() + vForce * 24, GetAbsAngles() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetKartKnockbackMultiplier( float flExtraMultiplier /*= 1.f*/ ) const
{
	return flExtraMultiplier * ( 1.0f + (float)m_iKartHealth / tf_halloween_kart_damage_to_force.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ResetKartDamage() 
{ 
	m_iKartHealth = 0; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CancelEurekaTeleport()
{
	m_bIsTeleportingUsingEurekaEffect = false;
	m_teleportHomeFlashTimer.Invalidate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PostThink()
{
	BaseClass::PostThink();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
	
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

    m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	if ( m_flTauntAttackTime && m_flTauntAttackTime < gpGlobals->curtime )
	{
		m_flTauntAttackTime = 0;
		DoTauntAttack();
	}

	// if we are coaching, then capture events for adding annotations
	if ( m_bIsCoaching && m_hStudent )
	{
		if ( ( m_afButtonPressed & ( IN_ATTACK | IN_ATTACK2 ) ) != 0 )
		{
			if ( m_afButtonPressed & IN_ATTACK )
			{
				HandleCoachCommand( this, kCoachCommand_Attack );
			}
			else if ( m_afButtonPressed & IN_ATTACK2 )
			{
				HandleCoachCommand( this, kCoachCommand_Defend );
			}
		}
		if ( m_hStudent->GetTeamNumber() != TEAM_SPECTATOR )
		{
			// tether coach to student--if the coach gets too far, move them toward the student
			Vector vecTarget = m_hStudent->GetAbsOrigin();
			Vector vecDelta = GetAbsOrigin() - vecTarget;
			float flDistance = vecDelta.Length();
			const float kInchesToMeters = 0.0254f;
			const float kMetersToInches = 1.0f / kInchesToMeters;
			const float kMaxDistanceToStudent = 30;
			int distance = RoundFloatToInt( flDistance * kInchesToMeters );
			if ( distance > kMaxDistanceToStudent )
			{
				VectorNormalize( vecDelta );
				SetAbsOrigin( vecTarget + vecDelta * ( kMaxDistanceToStudent * kMetersToInches ) );
			}
		}
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// metal is free during setup time
		if ( TFGameRules()->IsQuickBuildTime() )
		{
			GiveAmmo( 1000, TF_AMMO_METAL, true );
		}

		// clamp maximum velocity to avoid sending mini-bosses into the stratosphere
		if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			Vector ahead = GetAbsVelocity();
			float speed = ahead.NormalizeInPlace();

			const float velocityLimit = 1000.0f;
			if ( speed > velocityLimit )
			{
				speed = velocityLimit;
			}

			SetAbsVelocity( speed * ahead );
		}
	}

	UpdateHalloween();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PrecacheMvM()
{
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
	{
		COMPILE_TIME_ASSERT( ARRAYSIZE( g_szBotModels ) == TF_LAST_NORMAL_CLASS );
		int iModelIndex = PrecacheModel( g_szBotModels[ i ] );
		PrecacheGibsForModel( iModelIndex );

		COMPILE_TIME_ASSERT( ARRAYSIZE( g_szBotBossModels ) == TF_LAST_NORMAL_CLASS );
		iModelIndex = PrecacheModel( g_szBotBossModels[ i ] );
		PrecacheGibsForModel( iModelIndex );
	}

	int iModelIndex = PrecacheModel( g_szBotBossSentryBusterModel );
	PrecacheGibsForModel( iModelIndex );

	PrecacheModel( "models/items/currencypack_small.mdl" );
	PrecacheModel( "models/items/currencypack_medium.mdl" );
	PrecacheModel( "models/items/currencypack_large.mdl" );

	PrecacheModel( "models/bots/tw2/boss_bot/twcarrier_addon.mdl" );

	PrecacheParticleSystem( "bot_impact_light" );
	PrecacheParticleSystem( "bot_impact_heavy" );
	PrecacheParticleSystem( "bot_death" );
	PrecacheParticleSystem( "bot_radio_waves" );

	PrecacheScriptSound( "MVM.BotStep" );
	PrecacheScriptSound( "MVM.GiantHeavyStep" );
	PrecacheScriptSound( "MVM.GiantSoldierStep" );
	PrecacheScriptSound( "MVM.GiantDemomanStep" );
	PrecacheScriptSound( "MVM.GiantScoutStep" );
	PrecacheScriptSound( "MVM.GiantPyroStep" );
	PrecacheScriptSound( "MVM.GiantHeavyLoop" );
	PrecacheScriptSound( "MVM.GiantSoldierLoop" );
	PrecacheScriptSound( "MVM.GiantDemomanLoop" );
	PrecacheScriptSound( "MVM.GiantScoutLoop" );
	PrecacheScriptSound( "MVM.GiantPyroLoop" );
	PrecacheScriptSound( "MVM.GiantHeavyExplodes" );
	PrecacheScriptSound( "MVM.GiantCommonExplodes" );
	PrecacheScriptSound( "MVM.SentryBusterExplode" );
	PrecacheScriptSound( "MVM.SentryBusterLoop" );
	PrecacheScriptSound( "MVM.SentryBusterIntro" );
	PrecacheScriptSound( "MVM.SentryBusterStep" );
	PrecacheScriptSound( "MVM.SentryBusterSpin" );
	PrecacheScriptSound( "MVM.DeployBombSmall" );
	PrecacheScriptSound( "MVM.DeployBombGiant" );
	PrecacheScriptSound( "Weapon_Upgrade.ExplosiveHeadshot" );
	PrecacheScriptSound( "Spy.MVM_Chuckle" );
	PrecacheScriptSound( "Spy.MVM_TeaseVictim" );
	PrecacheScriptSound( "MVM.Robot_Engineer_Spawn" );
	PrecacheScriptSound( "MVM.Robot_Teleporter_Deliver" );
	PrecacheScriptSound( "MVM.MoneyPickup" );

	PrecacheMaterial( "effects/circle_nocull" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PrecacheKart()
{
	PrecacheModel( "models/player/items/taunts/bumpercar/parts/bumpercar.mdl" );
	PrecacheModel( "models/props_halloween/bumpercar_cage.mdl" );

	PrecacheScriptSound( "BumperCar.Spawn" );
	PrecacheScriptSound( "BumperCar.SpawnFromLava" );
	PrecacheScriptSound( "BumperCar.GoLoop" );
	PrecacheScriptSound( "BumperCar.Screech" );
	PrecacheScriptSound( "BumperCar.HitGhost" );
	PrecacheScriptSound( "BumperCar.Bump" );
	PrecacheScriptSound( "BumperCar.BumpHard" );
	PrecacheScriptSound( "BumperCar.BumpIntoAir" );
	PrecacheScriptSound( "BumperCar.SpeedBoostStart" );
	PrecacheScriptSound( "BumperCar.SpeedBoostStop" );
	PrecacheScriptSound( "BumperCar.Jump" );
	PrecacheScriptSound( "BumperCar.JumpLand" );
	PrecacheScriptSound( "sf14.Merasmus.DuckHunt.BonusDucks" );

	PrecacheParticleSystem( "kartimpacttrail" );
	PrecacheParticleSystem( "kart_dust_trail_red" );
	PrecacheParticleSystem( "kart_dust_trail_blue" );

	PrecacheParticleSystem( "kartdamage_4");
}

//-----------------------------------------------------------------------------
// Purpose: Precache the player models and player model gibs.
//-----------------------------------------------------------------------------
void CTFPlayer::PrecachePlayerModels( void )
{
	int i;
	for ( i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		const char *pszModel = GetPlayerClassData( i )->m_szModelName;
		if ( pszModel && pszModel[0] )
		{
			int iModel = PrecacheModel( pszModel );
			PrecacheGibsForModel( iModel );
		}

		pszModel = GetPlayerClassData( i )->m_szHandModelName;
		if ( pszModel && pszModel[0] )
		{
			PrecacheModel( pszModel );
		}

/*
		if ( !IsX360() )
		{
			// Precache the hardware facial morphed models as well.
			const char *pszHWMModel = GetPlayerClassData( i )->m_szHWMModelName;
			if ( pszHWMModel && pszHWMModel[0] )
			{
				PrecacheModel( pszHWMModel );
			}
		}
*/
	}
	
	// Always precache the silly gibs.
	for ( i = 4; i < ARRAYSIZE( g_pszBDayGibs ); ++i )
	{
		PrecacheModel( g_pszBDayGibs[i] );
	}

	if ( TFGameRules() && TFGameRules()->IsBirthday() )
	{
		for ( i = 0; i < 4/*ARRAYSIZE(g_pszBDayGibs)*/; i++ )
		{
			PrecacheModel( g_pszBDayGibs[i] );
		}
		PrecacheModel( "models/effects/bday_hat.mdl" );
	}
	if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
	{
		PrecacheModel( "models/props_halloween/halloween_gift.mdl" );
		PrecacheModel( "models/props_halloween/ghost_no_hat.mdl" );
		PrecacheModel( "models/props_halloween/ghost_no_hat_red.mdl" );
	}

	// Precache player class sounds
	for ( i = TF_FIRST_NORMAL_CLASS; i < TF_CLASS_COUNT_ALL; ++i )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( i );

		for ( int i = 0; i < ARRAYSIZE( pData->m_szDeathSound ); ++i )
		{
			PrecacheScriptSound( pData->m_szDeathSound[ i ] ); 
		}
	}


	COMPILE_TIME_ASSERT( TF_CALLING_CARD_MODEL_COUNT == ARRAYSIZE( g_pszDeathCallingCardModels ) );
	// Precache, Deliberatly skipping zero
	for ( i = 1; i < TF_CALLING_CARD_MODEL_COUNT; i++ )		
	{
		PrecacheModel( g_pszDeathCallingCardModels[i] );
	}

	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PrecacheTFPlayer()
{
	VPROF_BUDGET( "CTFPlayer::PrecacheTFPlayer", VPROF_BUDGETGROUP_PLAYER );
	if( !m_bTFPlayerNeedsPrecache )
		return;

	m_bTFPlayerNeedsPrecache = false;

	// Precache the player models and gibs.
	PrecachePlayerModels();

	// Precache the player sounds.
	PrecacheScriptSound( "Player.Spawn" );
	PrecacheScriptSound( "TFPlayer.Pain" );
	PrecacheScriptSound( "TFPlayer.CritHit" );
	PrecacheScriptSound( "TFPlayer.CritHitMini" );
	PrecacheScriptSound( "TFPlayer.DoubleDonk" );
	PrecacheScriptSound( "TFPlayer.CritPain" );
	PrecacheScriptSound( "TFPlayer.CritDeath" );
	PrecacheScriptSound( "TFPlayer.FreezeCam" );
	PrecacheScriptSound( "TFPlayer.Drown" );
	PrecacheScriptSound( "TFPlayer.AttackerPain" );
	PrecacheScriptSound( "TFPlayer.SaveMe" );
	PrecacheScriptSound( "TFPlayer.CritBoostOn" );
	PrecacheScriptSound( "TFPlayer.CritBoostOff" );
	PrecacheScriptSound( "TFPlayer.Decapitated" );
	PrecacheScriptSound( "TFPlayer.ReCharged" );
	PrecacheScriptSound( "Camera.SnapShot" );
	PrecacheScriptSound( "TFPlayer.Dissolve" );

	PrecacheScriptSound( "Saxxy.TurnGold" );

	PrecacheScriptSound( "Icicle.TurnToIce" );
	PrecacheScriptSound( "Icicle.HitWorld" );
	PrecacheScriptSound( "Icicle.Melt" );

	PrecacheScriptSound( "DemoCharge.ChargeCritOn" );
	PrecacheScriptSound( "DemoCharge.ChargeCritOff" );
	PrecacheScriptSound( "DemoCharge.Charging" );

	PrecacheScriptSound( "TFPlayer.StunImpactRange" );
	PrecacheScriptSound( "TFPlayer.StunImpact" );
	PrecacheScriptSound( "Halloween.PlayerScream" );
	PrecacheScriptSound( "Halloween.PlayerEscapedUnderworld" );

	PrecacheScriptSound( "Game.YourTeamLost" );
	PrecacheScriptSound( "Game.YourTeamWon" );
	PrecacheScriptSound( "Game.SuddenDeath" );
	PrecacheScriptSound( "Game.Stalemate" );
	PrecacheScriptSound( "TV.Tune" );

	//This will be moved out once we do the announcer pass.
	PrecacheScriptSound( "Announcer.AM_FirstBloodRandom" );
	PrecacheScriptSound( "Announcer.AM_CapEnabledRandom" );
	PrecacheScriptSound( "Announcer.AM_RoundStartRandom" );
	PrecacheScriptSound( "Announcer.AM_FirstBloodFast" );
	PrecacheScriptSound( "Announcer.AM_FirstBloodFinally" );
	PrecacheScriptSound( "Announcer.AM_FlawlessVictoryRandom" );
	PrecacheScriptSound( "Announcer.AM_FlawlessDefeatRandom" );
	PrecacheScriptSound( "Announcer.AM_FlawlessVictory01" );
	PrecacheScriptSound( "Announcer.AM_TeamScrambleRandom" );
	PrecacheScriptSound( "Taunt.MedicHeroic" );
	PrecacheScriptSound( "Taunt.GuitarRiff" );

	// Dmg absorb sound
	PrecacheScriptSound( "Powerup.ReducedDamage" );

	// Tourney UI
	PrecacheScriptSound( "Tournament.PlayerReady" );

	PrecacheScriptSound( "Medic.AutoCallerAnnounce" );

	PrecacheScriptSound( "Player.FallDamageIndicator" );

	PrecacheScriptSound( "Player.FallDamageDealt" );


	// Precache particle systems
	PrecacheParticleSystem( "crit_text" );
	PrecacheParticleSystem( "miss_text" );
	PrecacheParticleSystem( "cig_smoke" );
	PrecacheParticleSystem( "speech_mediccall" );
	PrecacheParticleSystem( "speech_mediccall_auto" );
	PrecacheParticleSystem( "speech_taunt_all" );
	PrecacheParticleSystem( "speech_taunt_red" );
	PrecacheParticleSystem( "speech_taunt_blue" );
	PrecacheParticleSystem( "player_recent_teleport_blue" );
	PrecacheParticleSystem( "player_recent_teleport_red" );
	PrecacheParticleSystem( "particle_nemesis_red" );
	PrecacheParticleSystem( "particle_nemesis_blue" );
	PrecacheParticleSystem( "spy_start_disguise_red" );
	PrecacheParticleSystem( "spy_start_disguise_blue" );
	PrecacheParticleSystem( "burningplayer_red" );
	PrecacheParticleSystem( "burningplayer_blue" );
	PrecacheParticleSystem( "burningplayer_rainbow" );
	PrecacheParticleSystem( "blood_spray_red_01" );
	PrecacheParticleSystem( "blood_spray_red_01_far" );
	PrecacheParticleSystem( "pyrovision_blood" );

	PrecacheParticleSystem( "water_blood_impact_red_01" );
	PrecacheParticleSystem( "blood_impact_red_01" );
	PrecacheParticleSystem( "water_playerdive" );
	PrecacheParticleSystem( "water_playeremerge" );
	PrecacheParticleSystem( "healthgained_red" );
	PrecacheParticleSystem( "healthgained_blu" );
	PrecacheParticleSystem( "healthgained_red_large" );
	PrecacheParticleSystem( "healthgained_blu_large" );
	PrecacheParticleSystem( "healthgained_red_giant" );
	PrecacheParticleSystem( "healthgained_blu_giant" );
	PrecacheParticleSystem( "critgun_weaponmodel_red" );
	PrecacheParticleSystem( "critgun_weaponmodel_blu" );
	PrecacheParticleSystem( "overhealedplayer_red_pluses" );
	PrecacheParticleSystem( "overhealedplayer_blue_pluses" );
	PrecacheParticleSystem( "highfive_red" );
	PrecacheParticleSystem( "highfive_blue" );
	PrecacheParticleSystem( "god_rays" );
	PrecacheParticleSystem( "bl_killtaunt" );
	PrecacheParticleSystem( "birthday_player_circling" );
	PrecacheParticleSystem( "drg_fiery_death" );
	PrecacheParticleSystem( "drg_wrenchmotron_teleport" );
	PrecacheParticleSystem( "taunt_flip_land_red" );
	PrecacheParticleSystem( "taunt_flip_land_blue" );
	PrecacheParticleSystem( "tfc_sniper_mist" );
	PrecacheParticleSystem( "dxhr_sniper_rail_blue" );
	PrecacheParticleSystem( "dxhr_sniper_rail_red" );
	PrecacheParticleSystem( "tfc_sniper_distortion_trail" );

	for ( int i=0; i<ARRAYSIZE( s_pszTauntRPSParticleNames ); ++i )
	{
		PrecacheParticleSystem( s_pszTauntRPSParticleNames[i] );
	}

	PrecacheParticleSystem( "blood_decap" );

	PrecacheParticleSystem( "xms_icicle_idle" );
	PrecacheParticleSystem( "xms_icicle_impact" );
	PrecacheParticleSystem( "xms_icicle_impact_dryice" );
	PrecacheParticleSystem( "xms_icicle_melt" );
	PrecacheParticleSystem( "xms_ornament_glitter" );
	PrecacheParticleSystem( "xms_ornament_smash_blue" );
	PrecacheParticleSystem( "xms_ornament_smash_red" );

	PrecacheParticleSystem( "drg_pomson_muzzleflash" );
	PrecacheParticleSystem( "drg_pomson_impact" );
	PrecacheParticleSystem( "drg_pomson_impact_drain" );
	PrecacheParticleSystem( "dragons_fury_effect" );

	PrecacheParticleSystem( "dxhr_arm_muzzleflash" );

	PrecacheModel( "effects/beam001_red.vmt" );
	PrecacheModel( "effects/beam001_blu.vmt" );
	PrecacheModel( "effects/beam001_white.vmt" );
	PrecacheModel( "models/player/gibs/random_organ.mdl" );

	PrecacheScriptSound( "Weapon_Mantreads.Impact" );

	// Precache footstep override sounds.
	PrecacheScriptSound( "cleats_conc.StepLeft" );
	PrecacheScriptSound( "cleats_conc.StepRight" );
	PrecacheScriptSound( "cleats_dirt.StepLeft" );
	PrecacheScriptSound( "cleats_dirt.StepRight" );

	PrecacheScriptSound( "xmas.jingle" );
	PrecacheScriptSound( "xmas.jingle_higher" );

	PrecacheScriptSound( "PegLeg.StepRight" );

	// Halloween
	// Bombinomicon deaths
	PrecacheParticleSystem( "bombinomicon_burningdebris" );
	PrecacheParticleSystem( "bombinomicon_burningdebris_halloween" );

	PrecacheParticleSystem( "halloween_player_death_blue" );
	PrecacheParticleSystem( "halloween_player_death" );

	PrecacheScriptSound( "Bombinomicon.Explode" );

	PrecacheScriptSound( "Weapon_DRG_Wrench.Teleport" );
	PrecacheScriptSound( "Weapon_Pomson.Single" );
	PrecacheScriptSound( "Weapon_Pomson.SingleCrit" );
	PrecacheScriptSound( "Weapon_Pomson.Reload" );
	PrecacheScriptSound( "Weapon_Pomson.DrainedVictim" );

	PrecacheScriptSound( "BlastJump.Whistle" );

	PrecacheScriptSound( "Spy.TeaseVictim" );
	PrecacheScriptSound( "Demoman.CritDeath" );
	PrecacheScriptSound( "Heavy.Battlecry03" );

	PrecacheModel( "models/effects/resist_shield/resist_shield.mdl" );

	PrecacheModel( "models/props_mvm/mvm_revive_tombstone.mdl" );

	PrecacheScriptSound( "General.banana_slip" ); // Used for SodaPopper Hype Jumps


	PrecacheScriptSound( "Parachute_open" );
	PrecacheScriptSound( "Parachute_close" );


	// precache the EOTL bomb cart replacements
	PrecacheModel( "models/props_trainyard/bomb_eotl_blue.mdl" );
	PrecacheModel( "models/props_trainyard/bomb_eotl_red.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Precache()
{
	VPROF_BUDGET( "CTFPlayer::Precache", VPROF_BUDGETGROUP_PLAYER );
	
	/*
	Note: All TFPlayer specific must go inside PrecacheTFPlayer()
		  This assumes that we're loading all resources for any TF player class
		  The reason is to safe performance because tons of string compares is very EXPENSIVE!!!
		  The most offending function is PrecacheGibsForModel which re-parsing through KeyValues every time it's called
		  If you have any question, come talk to me (Bank)
	*/
	PrecacheTFPlayer();

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Allow pre-frame adjustments on the player
//-----------------------------------------------------------------------------
ConVar sv_runcmds( "sv_runcmds", "1" );
void CTFPlayer::PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper )
{
	static bool bSeenSyncError = false;
	VPROF( "CTFPlayer::PlayerRunCommand" );

	if ( !sv_runcmds.GetInt() )
		return;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		m_Shared.CreateVehicleMove( gpGlobals->frametime, ucmd );
	}
	else if ( IsTaunting() || m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
	{
		// For some taunts, it is critical that the player not move once they start
		if ( !CanMoveDuringTaunt() )
		{
			ucmd->forwardmove = 0;
			ucmd->upmove = 0;
			ucmd->sidemove = 0;
			ucmd->viewangles = pl.v_angle;
		}

		if ( tf_allow_taunt_switch.GetInt() == 0 && ucmd->weaponselect != 0 )
		{
			ucmd->weaponselect = 0;

			// FIXME: The client will have predicted the weapon switch and have
			// called Holster/Deploy which will make the wielded weapon
			// invisible on their end.
		}
	}

	BaseClass::PlayerRunCommand( ucmd, moveHelper );

	// try to play taunt remap on input after updating user command
	if ( IsTaunting() && m_flNextAllowTauntRemapInputTime >= 0.f && m_flNextAllowTauntRemapInputTime <= gpGlobals->curtime )
	{
		float flSceneDuration = PlayTauntRemapInputScene();
		if ( flSceneDuration > 0.f )
		{
			m_flNextAllowTauntRemapInputTime = gpGlobals->curtime + flSceneDuration;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsReadyToPlay( void )
{
	bool bRetVal = false;

	if ( GetTeamNumber() == TEAM_SPECTATOR && m_bArenaSpectator == true )
		return false;
		
	//=============================================================================
	// HPE_BEGIN:
	// [msmith]	We don't want to say that the player is ready if they're still
	//			a training video.
	//=============================================================================
	if ( TFGameRules() && TFGameRules()->IsInTraining() && tf_training_client_message.GetInt() == TRAINING_CLIENT_MESSAGE_WATCHING_INTRO_MOVIE )
		return false;
	//=============================================================================
	// HPE_END
	//=============================================================================

	if ( GetTeamNumber() > LAST_SHARED_TEAM )
	{
		if ( GetDesiredPlayerClassIndex() > TF_CLASS_UNDEFINED )
		{
			bRetVal = true;
		}
		else 
		{
			if ( TFGameRules() && TFGameRules()->IsInArenaMode() && tf_arena_force_class.GetBool() == true && tf_arena_use_queue.GetBool() == true )
			{
				bRetVal = true;
			}
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsReadyToSpawn( void )
{
	if ( IsClassMenuOpen() )
	{
		return false;
	}

#ifdef TF_RAID_MODE
	if ( GetTeamNumber() == TF_TEAM_RED )
	{
		if ( TFGameRules()->IsRaidMode() || TFGameRules()->IsBossBattleMode() )
		{
			// enemy bots never respawn - they are spawned by the population system
			return false;
		}
	}
#endif // TF_RAID_MODE

	// Medic attached to marker - delay
	if ( m_hReviveMarker && m_hReviveMarker->IsReviveInProgress() && ( StateGet() != TF_STATE_DYING ) )
	{
		return false;
	}

	// Map-makers can force players to have custom respawn times
	if ( GetRespawnTimeOverride() != -1.f && gpGlobals->curtime < GetDeathTime() + GetRespawnTimeOverride() )
		return false;

	return ( StateGet() != TF_STATE_DYING );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player should be allowed to instantly spawn
//			when they next finish picking a class.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGainInstantSpawn( void )
{
	return ( GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED || IsClassMenuOpen() ) && !TFGameRules()->IsInArenaMode();
}

//-----------------------------------------------------------------------------
// Purpose: Resets player scores
//-----------------------------------------------------------------------------
void CTFPlayer::ResetScores( void )
{
	m_Shared.ResetScores();
	CTF_GameStats.ResetPlayerStats( this );
	RemoveNemesisRelationships();
	MannVsMachineStats_ResetPlayerEvents( this );

	BaseClass::ResetScores();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	m_AttributeManager.InitializeAttributes( this );
	m_AttributeManager.SetPlayer( this );
	m_AttributeList.SetManager( &m_AttributeManager );

	SetWeaponBuilder( NULL );

	ResetScores();
	StateEnter( TF_STATE_WELCOME );
	UpdateInventory( true );

	ResetAccumulatedSentryGunDamageDealt();
	ResetAccumulatedSentryGunKillCount();
	ResetDamagePerSecond();

	IGameEvent * event = gameeventmanager->CreateEvent( "player_initial_spawn" );
	if ( event )
	{
		event->SetInt( "index", entindex() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Request this player's inventories from the steam backend
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

#if !defined(NO_STEAM)
	m_Inventory.RemoveListener( this );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Override Base ApplyAbsVelocityImpulse (BaseEntity) to apply potential item attributes
//-----------------------------------------------------------------------------
void CTFPlayer::ApplyAbsVelocityImpulse( const Vector &vecImpulse ) 
{
	// Check for Attributes (mult_aiming_knockback_resistance)
	Vector vecForce = vecImpulse;
	float flImpulseScale = 1.0f;
	if ( IsPlayerClass( TF_CLASS_SNIPER ) && m_Shared.InCond( TF_COND_AIMING ) )
	{		
		CALL_ATTRIB_HOOK_FLOAT( flImpulseScale, mult_aiming_knockback_resistance );
	}

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) && !m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		flImpulseScale *= 2.f;
	}

	// take extra force if you have a parachute deployed in x-y directions
	if ( m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
	{
		// don't allow parachute robot to get push in MvM
		float flHorizontalScale = TFGameRules()->IsMannVsMachineMode() && IsBot() ? 0.f : 1.5f;
		vecForce.x *= flHorizontalScale;
		vecForce.y *= flHorizontalScale;
	}

	CBaseMultiplayerPlayer::ApplyAbsVelocityImpulse( vecForce * flImpulseScale );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ApplyGenericPushbackImpulse( const Vector &vecImpulse, CTFPlayer *pAttacker )
{
	// Knockout powerup carriers are immune to airblast
	if ( m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT || m_Shared.IsImmuneToPushback() )
		return;

	if ( pAttacker && TFGameRules() && TFGameRules()->IsTruceActive() && pAttacker->IsTruceValidForEnt() )
	{
		if ( ( pAttacker->GetTeamNumber() == TF_TEAM_RED ) || ( pAttacker->GetTeamNumber() == TF_TEAM_BLUE ) )
			return;
	}
	
	Vector vForce = vecImpulse;

	float flScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flScale, airblast_vulnerability_multiplier );
	vForce *= flScale;

	// if on the ground, require min force to boost you off it
	if ( ( GetFlags() & FL_ONGROUND ) && ( vForce.z < JUMP_MIN_SPEED ) )
	{
		// Minimum value of vecForce.z
		vForce.z = JUMP_MIN_SPEED;
	}
	
	CALL_ATTRIB_HOOK_FLOAT( vForce.z, airblast_vertical_vulnerability_multiplier );

	RemoveFlag( FL_ONGROUND );
	m_Shared.AddCond( TF_COND_KNOCKED_INTO_AIR );

	ApplyAbsVelocityImpulse( vForce );
}

//-----------------------------------------------------------------------------
// Purpose: Go between for Setting Local Punch Impulses. Checks item attributes
// Use this instead of directly calling m_Local.m_vecPunchAngle.SetX( value );
//-----------------------------------------------------------------------------
bool CTFPlayer::ApplyPunchImpulseX ( float flImpulse ) 
{
	// Check for No Aim Flinch
	bool bFlinch = true;
	if ( IsPlayerClass( TF_CLASS_SNIPER ) && m_Shared.InCond( TF_COND_AIMING ) )
	{	
		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
		{
			CTFSniperRifle *pRifle = static_cast< CTFSniperRifle* >( pWeapon );
			if ( pRifle->IsFullyCharged() )
			{
				int iAimingNoFlinch = 0;
				CALL_ATTRIB_HOOK_INT( iAimingNoFlinch, aiming_no_flinch );
				if ( iAimingNoFlinch > 0 )
				{
					bFlinch = false;
				}
			}
		}
	}
	
	if ( bFlinch )
	{
		m_Local.m_vecPunchAngle.SetX( flImpulse );
	}

	return bFlinch;
}

//-----------------------------------------------------------------------------
// Purpose: Request this player's inventories from the steam backend
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateInventory( bool bInit )
{
#if !defined(NO_STEAM)
	if ( IsFakeClient() )
		return;

	if ( bInit || !m_Inventory.GetSOC() )
	{
		if ( steamgameserverapicontext->SteamGameServer() )
		{
			CSteamID steamIDForPlayer;
			if ( GetSteamID( &steamIDForPlayer ) )
			{
				TFInventoryManager()->SteamRequestInventory( &m_Inventory, steamIDForPlayer, this );
			}
		}
	}

	// If we have an SOCache, we've got a connection to the GC
	bool bInvalid = true;
	if ( m_Inventory.GetSOC() )
	{
		bInvalid = m_Inventory.GetSOC()->BIsInitialized() == false;
	}
	m_Shared.SetLoadoutUnavailable( bInvalid );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Requests that the GC confirm that this player is supposed to have 
//			an SO cache on this gameserver and send it again if so.
//-----------------------------------------------------------------------------
void CTFPlayer::VerifySOCache()
{
#if !defined(NO_STEAM)
	if ( IsFakeClient() || IsHLTV() || IsReplay() )
		return;

	CSteamID steamIDForPlayer;
	GetSteamID( &steamIDForPlayer );

	if( steamIDForPlayer.BIndividualAccount() )
	{
		// if we didn't find an inventory ask the GC to refresh us
		GCSDK::CGCMsg<MsgGCVerifyCacheSubscription_t> msgVerifyCache( k_EMsgGCVerifyCacheSubscription );
		msgVerifyCache.Body().m_ulSteamID = steamIDForPlayer.ConvertToUint64();
		GCClientSystem()->BSendMessage( msgVerifyCache );
	}
	else
	{
		Msg( "Cannot verify load for invalid steam ID %s\n", steamIDForPlayer.Render() );
	}
#endif
}

#ifdef DEBUG
CON_COMMAND_F( verifyloadout, "Cause the server to verify the player's items on the server.", FCVAR_NONE )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	pPlayer->VerifySOCache();
}
#endif // DEBUG

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// always send information to student or client
	if ( pInfo->m_pClientEnt )
	{
		if ( m_hStudent && m_hStudent == CBaseEntity::Instance( pInfo->m_pClientEnt ) )
		{
			return FL_EDICT_ALWAYS;
		}
		else if ( m_hCoach && m_hCoach == CBaseEntity::Instance( pInfo->m_pClientEnt ) )
		{
			return FL_EDICT_ALWAYS;
		}
		else if ( TFGameRules() && TFGameRules()->IsPasstimeMode() )
		{
			// TODO it should be possible to restrict this further based on
			// the values of tf_passtime_player_reticles_friends/enemies
			return FL_EDICT_ALWAYS;
		}

		CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
		if ( pRecipientEntity && pRecipientEntity->ShouldForceTransmitsForTeam( GetTeamNumber() ) )
			return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	// coach can only "see" what the student "sees"
	if ( m_bIsCoaching && m_hStudent )
	{
		Vector org;
		org = m_hStudent->EyePosition();

		engine->AddOriginToPVS( org );
	}
	else
	{
		BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );
	}

	int area = pViewEntity ? pViewEntity->NetworkProp()->AreaNum() : NetworkProp()->AreaNum();
	PointCameraSetupVisibility( this, area, pvs, pvssize );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Spawn()
{
	VPROF_BUDGET( "CTFPlayer::Spawn", VPROF_BUDGETGROUP_PLAYER );
	MDLCACHE_CRITICAL_SECTION();

	m_bIsABot = IsBot();

	if ( m_bIsABot && IsBotOfType( TF_BOT_TYPE ) )
	{
		m_nBotSkill = ToTFBot( this )->GetDifficulty();
	}
	else
	{
		m_nBotSkill = 0;
	}

	m_flSpawnTime = gpGlobals->curtime;

	SetModelScale( 1.0f );
	UpdateModel();

	SetMoveType( MOVETYPE_WALK );
	BaseClass::Spawn();

	// We have to clear this early, so that the sword knows its max health in ManageRegularWeapons below
	m_Shared.SetDecapitations( 0 );

	// Check the make sure we have our inventory each time we spawn
	UpdateInventory( false );

#ifndef NO_STEAM
	if( m_Shared.IsLoadoutUnavailable() )
	{
		VerifySOCache();
	}
#endif

	// Create our off hand viewmodel if necessary
	CreateViewModel( 1 );
	// Make sure it has no model set, in case it had one before
	GetViewModel(1)->SetModel( "" );

	// Kind of lame, but CBasePlayer::Spawn resets a lot of the state that we initially want on.
	// So if we're in the welcome state, call its enter function to reset 
	if ( m_Shared.InState( TF_STATE_WELCOME ) )
	{
		StateEnterWELCOME();
	}

	// If they were dead, then they're respawning. Put them in the active state.
	if ( m_Shared.InState( TF_STATE_DYING ) )
	{
		StateTransition( TF_STATE_ACTIVE );
	}

	// If they're spawning into the world as fresh meat, give them items and stuff.
	bool bMatchSummary = TFGameRules() && TFGameRules()->ShowMatchSummary();
	if ( m_Shared.InState( TF_STATE_ACTIVE ) || bMatchSummary )
	{
		// remove our disguise each time we spawn
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			m_Shared.RemoveDisguise();
		}

		if ( !bMatchSummary )
		{
			EmitSound( "Player.Spawn" );
		}
		m_bRespawning = true;
		InitClass();
		m_bRespawning = false;
		m_Shared.RemoveAllCond(); // Remove conc'd, burning, rotting, hallucinating, etc.

		// add team glows for a period of time after we respawn
		m_Shared.AddCond( TF_COND_TEAM_GLOWS, tf_spawn_glows_duration.GetInt() );

		UpdateSkin( GetTeamNumber() );

		// Prevent firing for a second so players don't blow their faces off
		SetNextAttack( gpGlobals->curtime + 1.0 );

		DoAnimationEvent( PLAYERANIMEVENT_SPAWN );

		// Force a taunt off, if we are still taunting, the condition should have been cleared above.
		StopTaunt();

		// turn on separation so players don't get stuck in each other when spawned
		m_Shared.SetSeparation( true );
		m_Shared.SetSeparationVelocity( vec3_origin );

		RemoveTeleportEffect();
	
		//If this is true it means I respawned without dying (changing class inside the spawn room) but doesn't necessarily mean that my healers have stopped healing me
		//This means that medics can still be linked to me but my health would not be affected since this condition is not set.
		//So instead of going and forcing every healer on me to stop healing we just set this condition back on. 
		//If the game decides I shouldn't be healed by someone (LOS, Distance, etc) they will break the link themselves like usual.
		if ( m_Shared.GetNumHealers() > 0 )
		{
			m_Shared.AddCond( TF_COND_HEALTH_BUFF );
		}

		if ( !m_bSeenRoundInfo )
		{
			if ( TFGameRules() && TFGameRules()->IsPasstimeMode() )
			{
				CSingleUserRecipientFilter filter( this );
				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_PASSTIME_HOWTO );
			}

			TFGameRules()->ShowRoundInfoPanel( this );
			m_bSeenRoundInfo = true;
		}

		if ( IsInCommentaryMode() && !IsFakeClient() )
		{
			// Player is spawning in commentary mode. Tell the commentary system.
			CBaseEntity *pEnt = NULL;
			variant_t emptyVariant;
			while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "commentary_auto" )) != NULL )
			{
				pEnt->AcceptInput( "MultiplayerSpawned", this, this, emptyVariant, 0 );
			}
		}

		bool bRemoveRestriction = ( GetTeamNumber() == TEAM_SPECTATOR ) || ( ( GetTeamNumber() > TEAM_SPECTATOR ) && !IsPlayerClass( TF_CLASS_UNDEFINED ) );
		if ( !m_bFirstSpawnAndCanCallVote && bRemoveRestriction )
		{
			m_bFirstSpawnAndCanCallVote = true;

			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[ 0 ] : NULL;
			bool bFirstMiniRound = ( !pMaster || !pMaster->PlayingMiniRounds() || ( pMaster->GetCurrentRoundIndex() == 0 ) );
			float flRoundStart = TFGameRules()->GetRoundStart();
			bool bShouldHaveCooldown = ( TFGameRules()->GetRoundsPlayed() > 0 ) || !bFirstMiniRound || ( ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) && ( flRoundStart > 0.f ) && ( gpGlobals->curtime - flRoundStart > sv_vote_late_join_time.GetFloat() ) );
			if ( bShouldHaveCooldown )
			{
				// add a cooldown for our first vote
				if ( g_voteControllerGlobal )
					g_voteControllerGlobal->TrackVoteCaller( this, sv_vote_late_join_cooldown.GetFloat() );

				if ( GetTeamVoteController() )
					GetTeamVoteController()->TrackVoteCaller( this, sv_vote_late_join_cooldown.GetFloat() );
			}
		}
	}

	CTF_GameStats.Event_PlayerSpawned( this );

	m_iSpawnCounter = !m_iSpawnCounter;
	m_bAllowInstantSpawn = false;

	m_Shared.SetSpyCloakMeter( 100.0f );
	m_Shared.SetScoutEnergyDrinkMeter( 100.0f );
	m_Shared.SetScoutHypeMeter( 0.0f );
	m_Shared.StopScoutHypeDrain();
	m_Shared.SetRageMeter( 0.0f );
	m_Shared.SetDemomanChargeMeter( 100.0f );

	m_Shared.ClearDamageEvents();
	m_AchievementData.ClearHistories();

	m_flLastDamageTime = 0.f;
	m_flMvMLastDamageTime = 0.f;
	m_flLastDamageDoneTime = 0.f;
	m_iMaxSentryKills = 0;

	m_flNextVoiceCommandTime = gpGlobals->curtime;
	m_iVoiceSpamCounter = 0;

	ClearZoomOwner();
	SetFOV( this , 0 );

	SetViewOffset( GetClassEyeHeight() );

	RemoveAllScenesInvolvingActor( this );
	ClearExpression();
	m_flNextSpeakWeaponFire = gpGlobals->curtime;
	// This makes the surrounding box always the same size as the standing collision box
	// helps with parts of the hitboxes that extend out of the crouching hitbox, eg with the
	// heavyweapons guy
	Vector mins = VEC_HULL_MIN;
	Vector maxs = VEC_HULL_MAX;
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

	m_iLeftGroundHealth = -1;
	m_iBlastJumpState = 0;
	m_bGoingFeignDeath = false;
	m_bTakenBlastDamageSinceLastMovement = false;

	ClearTauntAttack();
	m_hTauntItem = NULL;

	m_bArenaIsAFK = false;

	m_Shared.SetFeignDeathReady( false );

	m_iOldStunFlags = 0;

	m_flAccumulatedHealthRegen = 0;
	memset( m_flAccumulatedAmmoRegens, 0, sizeof(m_flAccumulatedAmmoRegens) );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_spawn" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "class", GetPlayerClass()->GetClassIndex() );

		gameeventmanager->FireEvent( event );
	}

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() && GetTeamNumber() == TF_TEAM_BLUE )
	{
		// raiders respawn invulnerable for a short time
		m_Shared.AddCond( TF_COND_INVULNERABLE, tf_raid_respawn_safety_time.GetFloat() );

		// friends glow
		AddGlowEffect();
	}

	if ( TFGameRules()->IsBossBattleMode() && GetTeamNumber() == TF_TEAM_BLUE )
	{
		// respawn invulnerable for a short time
		m_Shared.AddCond( TF_COND_INVULNERABLE, tf_boss_battle_respawn_safety_time.GetFloat() );
	}
#endif // TF_RAID_MODE

	m_bIsMissionEnemy = false;
	m_bIsSupportEnemy = false;
	m_bIsLimitedSupportEnemy = false;

	m_Shared.Spawn();

	m_bCollideWithSentry = false;
	m_calledForMedicTimer.Invalidate();
	m_placedSapperTimer.Invalidate();

	m_bIsReadyToHighFive = false;

	m_nForceTauntCam = 0;
	m_bAllowedToRemoveTaunt = true;

	m_purgatoryPainMultiplier = 1;
	m_purgatoryPainMultiplierTimer.Invalidate();

	m_bIsTeleportingUsingEurekaEffect = false;

	m_playerMovementStuckTimer.Invalidate();

	m_bIsMiniBoss = false;
	m_bUseBossHealthBar = false;

	m_hGrapplingHookTarget = NULL;
	m_nHookAttachedPlayers = 0;
	m_bUsingActionSlot = false;

	m_flInspectTime = 0.f;

	m_flHelpmeButtonPressTime = 0.f;

	m_flSendPickupWeaponMessageTime = -1.f;

	m_bAlreadyUsedExtendFreezeThisDeath = false;

	SetRespawnOverride( -1.f, NULL_STRING );

	// Remove all powerups and add temporary invuln on spawn
	if ( TFGameRules()  && TFGameRules()->IsPowerupMode() )
	{
		m_Shared.AddCond( TF_COND_INVULNERABLE_USER_BUFF, 8.f );

		if ( !m_bIsInMannpowerDominantCondition )
		{
			if ( ( GetTeamNumber() == TF_TEAM_BLUE ) || ( GetTeamNumber() == TF_TEAM_RED ) )
			{
				CSteamID steamIDForPlayer;
				if ( GetSteamID( &steamIDForPlayer ) )
				{
					float flRemoveDominantConditionTime = TFGameRules()->CheckPowerupModeDominantDisconnect( steamIDForPlayer );
					if ( flRemoveDominantConditionTime > 0 )
					{
						ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_Dominant_Continue" );
						ClientPrint( this, HUD_PRINTTALK, "#TF_Powerup_Dominant_Continue" );

						m_Shared.AddCond( TF_COND_POWERUPMODE_DOMINANT, PERMANENT_CONDITION );

						m_flRemoveDominantConditionTime = flRemoveDominantConditionTime;
						m_bIsInMannpowerDominantCondition = true;

						int nEnemyTeam = ( GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;
						for ( int i = 0; i < MAX_PLAYERS; ++i )
						{
							CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
							if ( pTFPlayer && ( pTFPlayer->GetTeamNumber() == nEnemyTeam ) )
							{
								ClientPrint( pTFPlayer, HUD_PRINTCENTER, "#TF_Powerup_Dominant_Other_Team" );
								ClientPrint( pTFPlayer, HUD_PRINTTALK, "#TF_Powerup_Dominant_Other_Team" );
							}
						}
					}
				}
			}
		}
		else
		{
			m_Shared.AddCond( TF_COND_POWERUPMODE_DOMINANT );
			ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_Dominant_StillIn" );
		}
	}

	if ( TFGameRules() )
	{
		// It's halloween, and it's hell time.  Force this player to spawn in hell.
		if ( TFGameRules()->ArePlayersInHell() )
		{
			const char *pSpawnEntName = NULL;
			if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
			{
				pSpawnEntName = "hell_ghost_spawn";
			}
			else if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) && CTFMinigameLogic::GetMinigameLogic() )
			{
				CTFMiniGame *pActiveMinigame = CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame();
				if ( pActiveMinigame )
				{
					pSpawnEntName = pActiveMinigame->GetTeamSpawnPointName( GetTeamNumber() );
				}
			}

			if ( pSpawnEntName )
			{
				TFGameRules()->SpawnPlayerInHell( this, pSpawnEntName );
			}
		}
		TFGameRules()->OnPlayerSpawned( this );
	}

	if ( m_hReviveMarker )
	{
		UTIL_Remove( m_hReviveMarker );
		m_hReviveMarker = NULL;
	}

	// make sure we clear custom attributes that we added
	RemoveAllCustomAttributes();


	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
	if ( pResource )
	{
		pResource->SetPlayerClassWhenKilled( entindex(), TF_CLASS_UNDEFINED );
	}

	if ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS )
	{
		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		if ( pMatchDesc && pMatchDesc->BUsesAutoReady() )
		{
			TFGameRules()->PlayerReadyStatus_UpdatePlayerState( this, true );
		}
	}

	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch )
	{
		CSteamID steamID;
		GetSteamID( &steamID );

		// This client entered a running match
		CMatchInfo::PlayerMatchData_t *pMatchPlayer = pMatch->GetMatchDataForPlayer( steamID );
		if ( pMatchPlayer && TFGameRules() && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
		{
			pMatchPlayer->bPlayed = true;
		}
	}

	if ( m_nMaxHealthDrainBucket )
	{
		ResetMaxHealthDrain();
		SetHealth( GetMaxHealth() );
	}

	SetContextThink( &CTFPlayer::PostSpawnThink, gpGlobals->curtime + 0.1f, "PostSpawnThink" );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all nemesis relationships between this player and others
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveNemesisRelationships()
{
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != this )
		{
			bool bRemove = false;

			if ( TFGameRules()->IsInArenaMode() == true )
			{
				if ( GetTeamNumber() != TEAM_SPECTATOR )
				{
					if ( InSameTeam( pTemp ) == true )
					{
						bRemove = true;
					}
				}

				if ( IsDisconnecting() == true )
				{
					bRemove = true;
				}
			}
			else
			{
				bRemove = true;
			}
			
			if ( bRemove == true )
			{
				// set this player to be not dominating anyone else
				m_Shared.SetPlayerDominated( pTemp, false );
				m_iNumberofDominations = 0;

				// set no one else to be dominating this player	
				bool bThisPlayerIsDominatingMe = m_Shared.IsPlayerDominatingMe( i );
				pTemp->m_Shared.SetPlayerDominated( this, false );
				if ( bThisPlayerIsDominatingMe )
				{
					int iDoms = pTemp->GetNumberofDominations();
					pTemp->SetNumberofDominations( iDoms - 1);
				}
			}
		}
	}	

	if ( TFGameRules()->IsInArenaMode() == false || IsDisconnecting() == true )
	{
		// reset the matrix of who has killed whom with respect to this player
		CTF_GameStats.ResetKillHistory( this );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "remove_nemesis_relationships" );
	if ( event )
	{
		event->SetInt( "player", entindex() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::Regenerate( bool bRefillHealthAndAmmo /*= true*/ )
{
	// We may have been boosted over our max health. If we have, 
	// restore it after we reset out class values.
	int nOldMaxHealth = GetMaxHealth();
	int nOldHealth = GetHealth();
	bool bBoosted = ( nOldHealth > nOldMaxHealth || !bRefillHealthAndAmmo ) && ( nOldMaxHealth > 0 );

	int nAmmo[ TF_AMMO_COUNT ];
	if ( !bRefillHealthAndAmmo )
	{
		for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			nAmmo[ iAmmo ] = GetAmmoCount( iAmmo );
		}
	}

	ResetMaxHealthDrain();

	m_bRegenerating.Set( true );
	// This recomputes MaxHealth
	InitClass();
	m_bRegenerating.Set( false );

	if ( bBoosted )
	{
		SetHealth( MAX( nOldHealth, GetMaxHealth() ) );
	}

	if ( bRefillHealthAndAmmo )
	{
		if ( m_Shared.InCond( TF_COND_BURNING ) )
		{
			m_Shared.RemoveCond( TF_COND_BURNING );
		}

		if ( m_Shared.InCond( TF_COND_URINE ) )
		{
			m_Shared.RemoveCond( TF_COND_URINE );
		}

		if ( m_Shared.InCond( TF_COND_MAD_MILK ) )
		{
			m_Shared.RemoveCond( TF_COND_MAD_MILK );
		}

		if ( m_Shared.InCond( TF_COND_GAS ) )
		{
			m_Shared.RemoveCond( TF_COND_GAS );
		}

		if ( m_Shared.InCond( TF_COND_BLEEDING ) )
		{
			m_Shared.RemoveCond( TF_COND_BLEEDING );
		}

		if ( m_Shared.InCond( TF_COND_ENERGY_BUFF ) )
		{
			m_Shared.RemoveCond( TF_COND_ENERGY_BUFF );

			if ( m_Shared.InCond( TF_COND_CANNOT_SWITCH_FROM_MELEE ) )
			{
				m_Shared.RemoveCond( TF_COND_CANNOT_SWITCH_FROM_MELEE );
			}
		}

		if ( m_Shared.InCond( TF_COND_PHASE ) )
		{
			m_Shared.RemoveCond( TF_COND_PHASE );
		}

		if ( m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
		{
			m_Shared.RemoveCond( TF_COND_PARACHUTE_ACTIVE );
		}

		if ( m_Shared.InCond( TF_COND_PLAGUE ) )
		{
			m_Shared.RemoveCond( TF_COND_PLAGUE );
		}


		m_Shared.SetSpyCloakMeter( 100.0f );
		m_Shared.SetScoutEnergyDrinkMeter( 100.0f );
		m_Shared.SetDemomanChargeMeter( 100.0f );

		// Selectively refill the item effect meters if they're allowed
		for( int i = FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; i <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++i )
		{
			int iDenyResupply = 0;
			CBaseEntity* pItem = GetEntityForLoadoutSlot( i, true );
			if ( !pItem )
				continue;

			CALL_ATTRIB_HOOK_INT_ON_OTHER( pItem, iDenyResupply, item_meter_resupply_denied );
			if ( iDenyResupply )
				continue;

			m_Shared.SetItemChargeMeter( loadout_positions_t(i), pItem->GetDefaultItemChargeMeterValue() );
		}
	}

	// Reset our first allowed fire time. This allows honorbound weapons to be switched away
	// from for a bit.
	m_Shared.m_flFirstPrimaryAttack = MAX( m_Shared.m_flFirstPrimaryAttack, gpGlobals->curtime + 1.0f );

	if ( bRefillHealthAndAmmo )
	{
		for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			if ( GetAmmoCount( iAmmo ) > GetMaxAmmo( iAmmo ) )
			{
				SetAmmoCount( GetMaxAmmo( iAmmo ), iAmmo );
			}
		}
	}
	else
	{
		for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			SetAmmoCount( nAmmo[ iAmmo ], iAmmo );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_regenerate" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::InitClass( void )
{
	SetArmorValue( GetPlayerClass()->GetMaxArmor() );

	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	// Give default items for class.
	GiveDefaultItems();

	// Set initial health and armor based on class.
	// Do it after items have been delivered, so items can modify it
	SetMaxHealth( GetMaxHealth() );
	SetHealth( GetMaxHealth() );

	TeamFortress_SetSpeed();

}

//-----------------------------------------------------------------------------
// Purpose: Check if a player can use chat commands at the moment
//-----------------------------------------------------------------------------
bool CTFPlayer::CanPlayerTalk()
{
	// If this player is known to the match, and joined as chat-suspended, they can not. 
	if ( BHaveChatSuspensionInCurrentMatch() )
		return false;

	return BaseClass::CanPlayerTalk();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CreateViewModel( int iViewModel )
{
	Assert( iViewModel >= 0 && iViewModel < MAX_VIEWMODELS );

	if ( GetViewModel( iViewModel ) )
		return;

	CTFViewModel *pViewModel = ( CTFViewModel * )CreateEntityByName( "tf_viewmodel" );
	if ( pViewModel )
	{
		pViewModel->SetAbsOrigin( GetAbsOrigin() );
		pViewModel->SetOwner( this );
		pViewModel->SetIndex( iViewModel );
		DispatchSpawn( pViewModel );
		pViewModel->FollowEntity( this, false );
		m_hViewModel.Set( iViewModel, pViewModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the view model for the player's off hand
//-----------------------------------------------------------------------------
CBaseViewModel *CTFPlayer::GetOffHandViewModel()
{
	// off hand model is slot 1
	return GetViewModel( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: Sends the specified animation activity to the off hand view model
//-----------------------------------------------------------------------------
void CTFPlayer::SendOffHandViewModelActivity( Activity activity )
{
	CBaseViewModel *pViewModel = GetOffHandViewModel();
	if ( pViewModel )
	{
		int sequence = pViewModel->SelectWeightedSequence( activity );
		pViewModel->SendViewModelMatchingSequence( sequence );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the player up with the default weapons, ammo, etc.
//-----------------------------------------------------------------------------
void CTFPlayer::GiveDefaultItems()
{
	// Get the player class data.
	TFPlayerClassData_t *pData = m_PlayerClass.GetData();
	if ( GetTeamNumber() == TEAM_SPECTATOR )
	{
		RemoveAllWeapons();
		return;
	}
	
	// Give weapons.
	ManageRegularWeapons( pData );

	if ( !TFGameRules() || !TFGameRules()->IsInMedievalMode() )
	{
		// Give a builder weapon for each object the playerclass is allowed to build
		ManageBuilderWeapons( pData );
	}

	// Weapons that added greater ammo than base require us to now fill the player up to max ammo
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		GiveAmmo( GetMaxAmmo(iAmmo), iAmmo, true, kAmmoSource_Resupply );
	}

	// Clear the player's banner buffs.
	m_Shared.RemoveCond( TF_COND_OFFENSEBUFF );
	m_Shared.RemoveCond( TF_COND_DEFENSEBUFF );
	m_Shared.RemoveCond( TF_COND_REGENONDAMAGEBUFF );
	m_Shared.RemoveCond( TF_COND_NOHEALINGDAMAGEBUFF );
	m_Shared.RemoveCond( TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK );
	m_Shared.RemoveCond( TF_COND_DEFENSEBUFF_HIGH );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageBuilderWeapons( TFPlayerClassData_t *pData )
{
	// Collect all builders and validate them against the list of objects (below)
	CUtlVector< CTFWeaponBuilder* > vecBuilderDestroyList;
	for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		CTFWeaponBuilder *pBuilder = dynamic_cast< CTFWeaponBuilder* >( GetWeapon( i ) );
		if ( !pBuilder )
			continue;

		vecBuilderDestroyList.AddToTail( pBuilder );
	}

	CEconItemView *pLoadoutBuilderItemView = NULL;

	// Go through each object and see if we need to create or remove builders
	for ( int i = 0; i < OBJ_LAST; ++i )
	{
		if ( !GetPlayerClass()->CanBuildObject( i ) )
			continue;

		// TODO:  Need to add support for "n" builders, rather hard-wired for two.
		// Currently, the only class that uses more than one is the spy:
		// - BUILDER is OBJ_ATTACHMENT_SAPPER, which is invoked via weapon selection (see objects.txt).
		// - BUILDER2 is OBJ_SPY_TRAP, which is invoked via a build command from PDA3 (spy-specific).
		int nLoadoutPos = LOADOUT_POSITION_BUILDING;
		pLoadoutBuilderItemView = GetLoadoutItem( GetPlayerClass()->GetClassIndex(), nLoadoutPos, true );

		// Do we have a specific builder for this object?
		CTFWeaponBuilder *pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, i );
		if ( pBuilder )
		{
			// We may have a different builder back-end item now.  If so, destroy and make a new one below.
			CEconItemView *pCurrentBuilderItemView = pBuilder->GetAttributeContainer()->GetItem();
			if ( pCurrentBuilderItemView == NULL || pLoadoutBuilderItemView == NULL || !ItemsMatch( pData, pCurrentBuilderItemView, pLoadoutBuilderItemView, pBuilder ) )
			{
				// Manually nuke the item from the weapon list here so that we don't find it 
				vecBuilderDestroyList.FindAndRemove( pBuilder );
				Weapon_Detach( pBuilder );
				UTIL_Remove( pBuilder );

				// Wrong builder item, so pretend we didn't find one
				pBuilder = NULL;
			}
		}
		else if ( !GetObjectInfo( i )->m_bRequiresOwnBuilder )
		{
			// Do we have a default builder, and an object that doesn't require a specific builder?
			pBuilder = CTFPlayerSharedUtils::GetBuilderForObjectType( this, -1 );
			if ( pBuilder )
			{
				// Flag it as supported by this builder (ugly, but necessary for legacy system)
				pBuilder->SetObjectTypeAsBuildable( i );
			}
		}
				
		// Is a new builder required?
		if ( !pBuilder || ( GetObjectInfo( i )->m_bRequiresOwnBuilder && !( CTFPlayerSharedUtils::GetBuilderForObjectType( this, i ) ) ) )
		{
			pBuilder = dynamic_cast< CTFWeaponBuilder* >( GiveNamedItem( "tf_weapon_builder", i, pLoadoutBuilderItemView ) );
			if ( pBuilder )
			{
				pBuilder->DefaultTouch( this );
			}
		}

		// Builder settings
		if ( pBuilder )
		{
			if ( m_bRegenerating == false )
			{
				pBuilder->WeaponReset();
			}

			pBuilder->GiveDefaultAmmo();
			pBuilder->ChangeTeam( GetTeamNumber() );
			pBuilder->SetObjectTypeAsBuildable( i );
			pBuilder->m_nSkin = GetTeamNumber() - 2;	// color the w_model to the team

			// Pull it out of the "destroy" list
			vecBuilderDestroyList.FindAndRemove( pBuilder );
		}
	}

	// Anything left should be destroyed
	FOR_EACH_VEC( vecBuilderDestroyList, i )
	{
		Assert( vecBuilderDestroyList[i] );

		Weapon_Detach( vecBuilderDestroyList[i] );
		UTIL_Remove( vecBuilderDestroyList[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ItemsMatch( TFPlayerClassData_t *pData, CEconItemView *pCurWeaponItem, CEconItemView *pNewWeaponItem, CTFWeaponBase *pWpnEntity )
{
	if ( !pNewWeaponItem || !pNewWeaponItem->IsValid() )
		return false;

	// If we already have a weapon in this slot but is not the same type, nuke it (changed classes)
	// We don't need to do this for non-base items because they've already been verified above.
	bool bHasNonBaseWeapon = pNewWeaponItem ? pNewWeaponItem->GetItemQuality() != AE_NORMAL : false;
	if ( bHasNonBaseWeapon )
	{
		// If the item isn't the one we're supposed to have, nuke it
		if ( pCurWeaponItem->GetItemID() != pNewWeaponItem->GetItemID() )
		{
			/*
			Msg("Removing %s because its global index (%d) doesn't match the loadout's (%d)\n", pWeapon->GetDebugName(), 
				pCurWeaponItem->GetItemID(),
				pNewWeaponItem->GetItemID() );
			*/
			return false;
		}

		// Some items create different entities when wielded by different classes. If so, we need to say
		// the items don't match so the item gets recreated as the right entity.
		if ( pWpnEntity )
		{
			const char *pszCurWeaponClass	   = pWpnEntity->GetClassname(),
					   *pszNewWeaponTransClass = TranslateWeaponEntForClass( pNewWeaponItem->GetStaticData()->GetItemClass(), GetPlayerClass()->GetClassIndex() );

			if ( !pszCurWeaponClass || !pszNewWeaponTransClass || Q_stricmp( pszCurWeaponClass, pszNewWeaponTransClass ) )
				return false;
		}
	}
	else
	{
		if ( pCurWeaponItem->GetItemQuality() != AE_NORMAL || (pCurWeaponItem->GetItemDefIndex() != pNewWeaponItem->GetItemDefIndex()) )
		{
			//Msg("Removing %s because it's not the right type for the class.\n", pWeapon->GetDebugName() );
			return false;
		}

		CSteamID ownerSteamID;
		GetSteamID( &ownerSteamID );

		// If the owner is not the same, then they're different as well. This catches
		// cases of stock items comparing
		if ( pCurWeaponItem->GetAccountID() != ownerSteamID.GetAccountID() )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ItemIsAllowed( CEconItemView *pItem )
{
	if ( !pItem || !pItem->GetStaticData() )
		return false;

	int iClass = GetPlayerClass()->GetClassIndex();
	int iSlot = pItem->GetStaticData()->GetLoadoutSlot(iClass);

	// Passtime hack to allow passtime gun
	if ( V_stristr( pItem->GetItemDefinition()->GetDefinitionName(), "passtime" ) )
	{
		return TFGameRules() && TFGameRules()->IsPasstimeMode();
	}

	// Holiday Restriction
	CEconItemDefinition* pData = pItem->GetStaticData();
	if ( TFGameRules() && pData && pData->GetHolidayRestriction() )
	{
		int iHolidayRestriction = UTIL_GetHolidayForString( pData->GetHolidayRestriction() );
		if ( iHolidayRestriction != kHoliday_None && !TFGameRules()->IsHolidayActive( iHolidayRestriction ) )
			return false;
	}

	if ( TFGameRules()->InStalemate() && mp_stalemate_meleeonly.GetBool() )
	{
		bool bMeleeOnlyAllowed = (iSlot == LOADOUT_POSITION_MELEE)
							  || (iClass == TF_CLASS_SPY && (iSlot == LOADOUT_POSITION_PDA || iSlot == LOADOUT_POSITION_PDA2));

		if ( !bMeleeOnlyAllowed )
			return false;
	}

	if ( TFGameRules()->IsInMedievalMode() )
	{
		bool bMedievalModeAllowed = false;

		// Allow all melee-class weapons, non-weapons, and the spy equipment.
		switch ( iSlot )
		{
		case LOADOUT_POSITION_MELEE:
		case LOADOUT_POSITION_HEAD:
		case LOADOUT_POSITION_MISC:
		case LOADOUT_POSITION_MISC2:
		case LOADOUT_POSITION_ACTION:
		case LOADOUT_POSITION_TAUNT:
		case LOADOUT_POSITION_TAUNT2:
		case LOADOUT_POSITION_TAUNT3:
		case LOADOUT_POSITION_TAUNT4:
		case LOADOUT_POSITION_TAUNT5:
		case LOADOUT_POSITION_TAUNT6:
		case LOADOUT_POSITION_TAUNT7:
		case LOADOUT_POSITION_TAUNT8:
			bMedievalModeAllowed = true;
			break;

		case LOADOUT_POSITION_PDA:
		case LOADOUT_POSITION_PDA2:
			if ( iClass == TF_CLASS_SPY )
				bMedievalModeAllowed = true;
			break;
		}
		
		if ( !bMedievalModeAllowed )
		{
			static CSchemaAttributeDefHandle pAttrib_AllowedInMedievalMode( "allowed in medieval mode" );
			if ( !pItem->FindAttribute( pAttrib_AllowedInMedievalMode ) )
			{
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageRegularWeapons( TFPlayerClassData_t *pData )
{	
	// Reset ammo.
	RemoveAllAmmo();

	// Remove our disguise weapon.
	m_Shared.RemoveDisguiseWeapon();

	CUtlVector<const char *> precacheStrings;

	CBaseCombatWeapon* pCurrentWeapon = m_hActiveWeapon;

	// Give ammo. Must be done before weapons, so weapons know the player has ammo for them.
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		// This screws up weapons/items that shouldn't start with grenades
		if ( iAmmo >= TF_AMMO_GRENADES1 && iAmmo <= TF_AMMO_GRENADES3 )
			continue;

		GiveAmmo( GetMaxAmmo(iAmmo), iAmmo, true, kAmmoSource_Resupply );
	}

	if ( IsX360() )
	{
		ManageRegularWeaponsLegacy( pData );
	}
	else
	{
		// Loop through our current wearables and ensure we're supposed to have them.
		ValidateWearables( pData );

		// Loop through all our current weapons, and ensure we're supposed to have them.
		ValidateWeapons( pData, true );

		// Create a copy of currently equipped items, if we equip something new report player loadout
		bool bItemsChanged = false;

		// Now Loop through our inventory for the current class, and give us any items we don't have.
		int iClass = GetPlayerClass()->GetClassIndex();
		if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_CLASS_COUNT )
		{
			CSteamID ownerSteamID;
			GetSteamID( &ownerSteamID );

			for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
			{
				// bots don't need the action slot item for MvM (canteen)
 				if ( ( i == LOADOUT_POSITION_ACTION ) && IsBot() && TFGameRules() && TFGameRules()->IsMannVsMachineMode() && ( GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
 					continue;

				m_EquippedLoadoutItemIndices[i] = LOADOUT_SLOT_USE_BASE_ITEM;

				// use base items in training mode
				CEconItemView *pItem = GetLoadoutItem( iClass, i, true );
				if ( !pItem || !pItem->IsValid() )
					continue;

				if ( !ItemIsAllowed( pItem ) )
					continue;

				// Only do this for taunts, because other items will be caught by the dynamic model loading system. 
				if ( IsTauntSlot( i ) )
				{
					tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - Precaching taunts, etc", __FUNCTION__ );
					// This has to be done before the continue for "no_entity", because we're trying to precache taunts which
					// explicitly bail out there.
					precacheStrings.RemoveAll();
					pItem->GetItemDefinition()->GeneratePrecacheModelStrings( false, &precacheStrings );
					FOR_EACH_VEC( precacheStrings, iModel )
					{
						if ( precacheStrings[iModel] && ( *precacheStrings[iModel] ) )
						{
							PrecacheModel( precacheStrings[iModel], false );
						}
					}
				}

				m_EquippedLoadoutItemIndices[i] = pItem->GetItemID();

				Assert( pItem->GetStaticData()->GetItemClass() );
				if ( pItem->GetStaticData()->GetItemClass() && FStrEq( pItem->GetStaticData()->GetItemClass(), "no_entity" ) )
					continue;

				CTFWeaponBase *pCurrentWeaponOfType = NULL;
				bool bAlreadyHave = false;
				// Don't need to check weapons if it's a wearable-only slot
				if ( !IsWearableSlot(i) || pItem->GetItemDefinition()->IsActingAsAWeapon() )
				{
					// Weapon slot. Check out weapons to see if we have it.
					for ( int wpn = 0; wpn < MAX_WEAPONS; wpn++ )
					{
						CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(wpn);
						if ( !pWeapon )
							continue;

						if ( ItemsMatch( pData, pWeapon->GetAttributeContainer()->GetItem(), pItem, pWeapon ) )
						{
							pCurrentWeaponOfType = pWeapon;
							bAlreadyHave = true;
							break;
						}
					}
				}

				CEconWearable *pWearable = NULL;
				if ( !bAlreadyHave )
				{
					// We couldn't find a matching weapon. See if we have a matching wearable.
					for ( int wbl = 0; wbl < m_hMyWearables.Count(); wbl++ )
					{
						pWearable = m_hMyWearables[wbl];
						if ( !pWearable )
							continue;

						CEconItemView *pWearableView = pWearable->GetAttributeContainer()->GetItem();
						if ( ItemsMatch( pData, pWearableView, pItem ) )
						{
							bAlreadyHave = true;
							break;
						}
					}
				}

				if ( !bAlreadyHave && pItem->GetStaticData()->GetItemClass() )
				{
					CEconEntity *pNewItem = dynamic_cast<CEconEntity*>(GiveNamedItem( pItem->GetStaticData()->GetItemClass(), 0, pItem ));
					Assert( pNewItem );
					if ( pNewItem )
					{
						pNewItem->GetAttributeContainer()->GetItem()->SetOverrideAccountID( ownerSteamID.GetAccountID() );

						CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder*>( (CBaseEntity*)pNewItem );
						if ( pBuilder )
						{
							pBuilder->SetSubType( pData->m_aBuildable[0] );
						}

						CBaseCombatWeapon* pWeapon = dynamic_cast< CBaseCombatWeapon* >( pNewItem );
						if ( pWeapon )
						{
							pWeapon->SetSoundsEnabled( false );
						}
						
						pNewItem->GiveTo( this );

						// set the default item charge meter value for this new weapon
						m_Shared.SetItemChargeMeter( loadout_positions_t( i ), pNewItem->GetDefaultItemChargeMeterValue() );

						if ( pWeapon )
						{
							pWeapon->SetSoundsEnabled( true );
						}
					}
				}
				else
				{
					if ( pCurrentWeaponOfType )
					{
						pCurrentWeaponOfType->UpdateExtraWearables();

						// We need to ensure all hands pointers are updated for all weapons.
						// Otherwise we could end up using animation sequences from the wrong class hands.
						pCurrentWeaponOfType->UpdateHands();
					}
				}

				bItemsChanged |= !bAlreadyHave;
			} // For each item in load out
		}

		if ( bItemsChanged )
		{
			CTF_GameStats.Event_PlayerLoadoutChanged( this, false );			
		}
		// We may have added weapons that make others invalid. Recheck.
		ValidateWeapons( pData, false );

		if ( m_hActiveWeapon.Get() != pCurrentWeapon && m_hActiveWeapon )
		{
			m_hActiveWeapon->WeaponSound( DEPLOY );
		}

		CSingleUserRecipientFilter filter( this );
		UserMessageBegin( filter, "PlayerLoadoutUpdated" );
			WRITE_BYTE( entindex() );
		MessageEnd();
	}


	// On equip, legacy source code will autoswitch to new weapons.
	// Instead of refactoring, we check here to see if we are allowed to have certain weapons switched to

	// TF2: Not allowed to have a actionslot item as last or active on regenerate / respawn
	// HACK Don't allow the parachute to be an active weapon
	CTFWeaponBase *pCurr = GetActiveTFWeapon();
	CTFWeaponBase *pPrev = dynamic_cast<CTFWeaponBase*>( GetLastWeapon() );
	if ( ( pCurr && pCurr->GetAttributeContainer()->GetItem()->GetEquippedPositionForClass( GetPlayerClass()->GetClassIndex() ) == LOADOUT_POSITION_ACTION )
	  || ( pPrev && pPrev->GetAttributeContainer()->GetItem()->GetEquippedPositionForClass( GetPlayerClass()->GetClassIndex() ) == LOADOUT_POSITION_ACTION )
	  || ( pCurr && pCurr->GetWeaponID() == TF_WEAPON_PARACHUTE )
	) {
		m_bRegenerating.Set( false );
		m_iLastWeaponSlot = 0;
	}

	if ( m_bRegenerating == false )
	{
		bool bWepSwitched = false;
		if ( m_bRememberActiveWeapon && m_iActiveWeaponTypePriorToDeath )
		{
			CTFWeaponBase *pWeapon = Weapon_OwnsThisID( m_iActiveWeaponTypePriorToDeath );
			if ( pWeapon && pWeapon->GetAttributeContainer()->GetItem()->GetEquippedPositionForClass( GetPlayerClass()->GetClassIndex() ) != LOADOUT_POSITION_ACTION )
			{
				bWepSwitched = Weapon_Switch( pWeapon );
			}
		}
		
		if ( !bWepSwitched )
		{
			SetActiveWeapon( NULL );

			// Find a weapon to switch to, starting with primary.
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>( GetEntityForLoadoutSlot( LOADOUT_POSITION_PRIMARY ) );
			if ( !pWeapon || !pWeapon->CanBeSelected() || !Weapon_Switch( pWeapon ) )
			{
				pWeapon = dynamic_cast<CTFWeaponBase*>( GetEntityForLoadoutSlot( LOADOUT_POSITION_SECONDARY ) );
				if ( !pWeapon || pWeapon->CanBeSelected() || !Weapon_Switch( pWeapon ) )
				{
					pWeapon = dynamic_cast<CTFWeaponBase*>( GetEntityForLoadoutSlot( LOADOUT_POSITION_MELEE ) );
					Weapon_Switch( pWeapon );
				}
			}
		}

		if ( (m_iLastWeaponSlot == 0 || !m_bRememberLastWeapon) && !m_bRememberActiveWeapon )  
		{
			m_iLastWeaponSlot = 1;
		}

		if ( !Weapon_GetSlot( m_iLastWeaponSlot ) )
		{
			Weapon_SetLast( Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
		}
		else
		{
			Weapon_SetLast( Weapon_GetSlot( m_iLastWeaponSlot ) );
		}		
	}

	// Now make sure we don't have too much ammo. This can happen if an item has reduced our max ammo.
	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		int iMax = GetMaxAmmo(iAmmo);
		if ( iMax < GetAmmoCount(iAmmo) )
		{
			RemoveAmmo( GetAmmoCount(iAmmo) - iMax, iAmmo );
		}
	}

	// If our max health dropped below current due to item changes, drop our current health.
	// If we're not being buffed, clamp it to max. Otherwise, clamp it to the max buffed health
	int iMaxHealth = m_Shared.InCond( TF_COND_HEALTH_BUFF ) ? m_Shared.GetMaxBuffedHealth() : GetMaxHealth();
	if ( m_iHealth > iMaxHealth )
	{
		// Modify health manually to prevent showing all the "you got hurt" UI. 
		m_iHealth = iMaxHealth;
	}

	if ( TFGameRules()->InStalemate() && mp_stalemate_meleeonly.GetBool() )
	{
		CBaseCombatWeapon *meleeWeapon = Weapon_GetSlot( TF_WPN_TYPE_MELEE );
		if ( meleeWeapon )
		{
			Weapon_Switch( meleeWeapon );
		}
	}

	// In testing mode, switch bots to the weapon being tested
	if ( TFGameRules()->IsInItemTestingMode() && IsFakeClient() )
	{
		// Our first player should be the human tester
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( 1 ) );
		if ( pPlayer && !pPlayer->IsFakeClient() )
		{
			// Loop through all the items we're testing
			FOR_EACH_VEC( pPlayer->m_ItemsToTest, i )
			{
				CEconItemView *pItem = &pPlayer->m_ItemsToTest[i].scriptItem;
				if ( !pItem )
					continue;

				int iSlot = pItem->GetStaticData()->GetLoadoutSlot( GetPlayerClass()->GetClassIndex() );
				if ( IsWearableSlot( iSlot ) )
					continue;

				CBaseCombatWeapon *pWeapon = Weapon_GetSlot( iSlot );
				if ( pWeapon )
				{
					Weapon_Switch( pWeapon );
					break;
				}
			}
		}
	}

	if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() && !IsBot() ) 
	{
		if (  m_Inventory.ClassLoadoutHasChanged( GetPlayerClass()->GetClassIndex() ) 
		   || ( m_bSwitchedClass )
		   || ( g_pPopulationManager && g_pPopulationManager->IsRestoringCheckpoint() ) )
		{
			ReapplyPlayerUpgrades();
		}

		// Calculate how much money is being used on active class / items
		int nSpending = 0;
		int iClass = GetPlayerClass()->GetClassIndex();
		CUtlVector< CUpgradeInfo > *upgrades = GetPlayerUpgradeHistory();
		if ( upgrades )
		{
			for( int u = 0; u < upgrades->Count(); ++u )
			{
				// Class Match, Check to see if we have this item equipped
				if ( iClass == upgrades->Element(u).m_iPlayerClass) 
				{
					// Player upgrade
					if ( upgrades->Element( u ).m_itemDefIndex == INVALID_ITEM_DEF_INDEX )
					{
						nSpending += upgrades->Element(u).m_nCost;
						continue;
					}

					// Item upgrade, look at equipment only not miscs or bottle
					for ( int itemIndex = 0; itemIndex <= LOADOUT_POSITION_PDA2; itemIndex++ )
					{
						CEconItemView *pItem = GetLoadoutItem( iClass, itemIndex, true );
						if ( upgrades->Element(u).m_itemDefIndex == pItem->GetItemDefIndex() )
						{
							nSpending += upgrades->Element(u).m_nCost;
							break;
						}
					}
				}
			}
		}

		CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
		if ( pStats )
		{
			pStats->NotifyPlayerActiveUpgradeCosts( this, nSpending );
		}
	}



	// Check if we should give a "grenade"
	for( int i = FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; i <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++i )
	{
		CBaseEntity* pItem = GetEntityForLoadoutSlot( i, true );
		if ( !pItem )
			continue;

		attrib_value_t chargeType = ATTRIBUTE_METER_TYPE_NONE;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pItem, chargeType, item_meter_charge_type );

		if ( chargeType == ATTRIBUTE_METER_TYPE_NONE )
			continue;

		loadout_positions_t eLoadoutPosition( (loadout_positions_t)i );
		if ( m_Shared.GetItemChargeMeter( eLoadoutPosition ) < 100.f )
			continue;
	
		CBaseCombatWeapon *pWeapon = Weapon_GetSlot( eLoadoutPosition );
		if ( !pWeapon )
			continue;
		
		int nAmmoType = pWeapon->m_iPrimaryAmmoType;
		if ( ( nAmmoType < TF_AMMO_GRENADES1 ) || ( nAmmoType > TF_AMMO_GRENADES3 ) )
			continue;

		if ( GetAmmoCount( nAmmoType ) == 0 )
		{
			GiveAmmo( 1, nAmmoType, true, kAmmoSource_ResourceMeter );
		}
	}

	PostInventoryApplication();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayer::GetLoadoutItem( int iClass, int iSlot, bool bReportWhitelistFails )
{
	if ( TFGameRules()->IsInItemTestingMode() )
	{
		CEconItemView *pItem = ItemTesting_GetTestItem( iClass, iSlot );
		if ( pItem )
			return pItem;
	}

	if ( TFGameRules()->IsInTraining() || TFGameRules()->IsInItemTestingMode() )
	{
		CTFInventoryManager *pInventoryManager = TFInventoryManager();
		return pInventoryManager->GetBaseItemForClass( iClass, iSlot );
	}

	CEconItemView *pItem = m_Inventory.GetItemInLoadout( iClass, iSlot );

	// Check to see if this item passes the tournament rules (in whitelist/or normal quality).
	// If it doesn't, we fall back to the base item for the loadout slot.
	if ( (pItem && pItem->IsValid()) && (pItem->GetItemQuality() != AE_NORMAL) && !pItem->GetStaticData()->IsAllowedInMatch() && TFGameRules()->IsInTournamentMode() )
	{
		if ( bReportWhitelistFails )
		{
			ClientPrint( this, HUD_PRINTNOTIFY, "#Item_BlacklistedInMatch", pItem->GetStaticData()->GetItemBaseName() );
		}

		pItem = TFInventoryManager()->GetBaseItemForClass( iClass, iSlot );
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Handles pressing the use action slot item key.
//-----------------------------------------------------------------------------
void CTFPlayer::UseActionSlotItemPressed( void )
{
	m_bUsingActionSlot = true;

	if ( TryToPickupDroppedWeapon() )
		return;

	int iNoiseMaker = 0;
	CALL_ATTRIB_HOOK_INT( iNoiseMaker, enable_misc2_noisemaker );
	if ( iNoiseMaker )
	{
		DoNoiseMaker();
		return;
	}

	CBaseEntity *pActionSlotEntity = GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION );
	if ( !pActionSlotEntity )
		return;

	// get the equipped item and see what it is
	CTFPowerupBottle *pPowerupBottle = dynamic_cast< CTFPowerupBottle* >( pActionSlotEntity );
	if ( pPowerupBottle )
	{
		// @todo send event to clients so that they know what's going on
		pPowerupBottle->Use();
		return;
	}

	// is it a throwable?
	CTFThrowable *pThrowable = dynamic_cast< CTFThrowable* >( pActionSlotEntity );
	if ( pThrowable )
	{
		if ( !Weapon_ShouldSelectItem( pThrowable ) )
			return;

		if ( GetActiveWeapon() )
		{
			if ( !GetActiveWeapon()->CanHolster() )
				return;

			ResetAutoaim( );
		}

		// Check if this is the spellbook so we can save off info to preserve weapon switching
		CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( pThrowable );
		if ( pSpellBook )
		{
			if ( !pSpellBook->CanCastSpell( this ) )
			{
				// if no spell force a roll if cheat is active
				if ( tf_halloween_unlimited_spells.GetBool() && !pSpellBook->HasASpellWithCharges() )
				{
					pSpellBook->RollNewSpell( 0 );
				}
				else if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
				{
					// if I'm in a halloween Vehicle, cast the spell immediately
					//pSpellBook->CastKartSpell();
					pSpellBook->PrimaryAttack();
				}
				else
				{
					EmitSound_t params;
					params.m_flSoundTime = 0;
					params.m_pflSoundDuration = 0;
					params.m_pSoundName = "Player.DenyWeaponSelection";

					CSingleUserRecipientFilter filter( this );
					EmitSound( filter, entindex(), params );
				}
				return;
			}
			// Notify the spellbook of the current last used weapon
			pSpellBook->SaveLastWeapon( GetLastWeapon() );
		}
		// Equip it
		Weapon_Switch( pThrowable );
		return;
	}

	if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() )
	{
		CTFGrapplingHook *pGrapplingHook = dynamic_cast< CTFGrapplingHook* >( pActionSlotEntity );
		if ( pGrapplingHook )
		{
			Weapon_Switch( pGrapplingHook );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles releasing the use action slot item key.
//-----------------------------------------------------------------------------
void CTFPlayer::UseActionSlotItemReleased( void )
{
	m_bUsingActionSlot = false;

	if ( TFGameRules() && TFGameRules()->IsUsingGrapplingHook() )
	{
		// if we're using the hook, switch back to the last weapon
		if ( GetActiveTFWeapon() && GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_GRAPPLINGHOOK )
		{
			CBaseCombatWeapon *pLastWeapon = GetLastWeapon();
			if ( pLastWeapon && Weapon_CanSwitchTo( pLastWeapon ) )
			{
				Weapon_Switch( pLastWeapon );
			}
			else
			{
				// in case we failed to switch back to last weapon for some reason, just find the next best
				SwitchToNextBestWeapon( pLastWeapon );
			}

			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles pressing the inspect key.
//-----------------------------------------------------------------------------
void CTFPlayer::InspectButtonPressed()
{
	m_flInspectTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Handles releasing the inspect key.
//-----------------------------------------------------------------------------
void CTFPlayer::InspectButtonReleased()
{
	m_flInspectTime = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: Handles helpme trace. returns true if we do something with the trace
//-----------------------------------------------------------------------------
ConVar tf_helpme_range( "tf_helpme_range", "150", FCVAR_DEVELOPMENTONLY );
bool CTFPlayer::HandleHelpmeTrace()
{

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Handles pressing the helpme key.
//-----------------------------------------------------------------------------
void CTFPlayer::HelpmeButtonPressed()
{
	m_flHelpmeButtonPressTime = gpGlobals->curtime;

	if ( !HandleHelpmeTrace() )
	{
		// default to calling for medic
		engine->ClientCommand( edict(), "voicemenu 0 0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles releasing the helpme key.
//-----------------------------------------------------------------------------
void CTFPlayer::HelpmeButtonReleased()
{
	m_flHelpmeButtonPressTime = 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::AddToSpyKnife( float value, bool force )
{
	CTFKnife *pWpn = (CTFKnife *)Weapon_OwnsThisID( TF_WEAPON_KNIFE );
	if ( !pWpn )
		return false;

	return pWpn->DecreaseRegenerationTime( value, force );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllItems()
{
	// Nuke items.
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(i);
		if ( !pWeapon )
			continue;

		Weapon_Detach( pWeapon );
		UTIL_Remove( pWeapon );
	}

	// Nuke wearables.
	for ( int wbl = m_hMyWearables.Count()-1; wbl >= 0; wbl-- )
	{
		CEconWearable *pWearable = m_hMyWearables[wbl];
		Assert( pWearable );
		if ( !pWearable )
			continue;

		RemoveWearable( pWearable );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ValidateWeapons( TFPlayerClassData_t *pData, bool bResetWeapons )
{
	CSteamID steamIDForPlayer;
	GetSteamID( &steamIDForPlayer );

	bool bFoundBuffItem = false;

	bool bOverrideRemoval = false;
	if ( bResetWeapons && m_bForceItemRemovalOnRespawn )
	{
		bOverrideRemoval = true;
		m_bForceItemRemovalOnRespawn = false;
	}

	// Disable sounds for all weapons.  We're about to switch weapons MANY times,
	// and we don't want the deploy sounds to play for any of them, since none
	// of the deploys are actually visible to the player
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(i);
		if ( !pWeapon )
			continue;

		pWeapon->SetSoundsEnabled( false );
	}

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(i);
		if ( !pWeapon )
			continue;

		int iLoadoutSlot = pWeapon->GetAttributeContainer()->GetItem()->GetStaticData()->GetLoadoutSlot( GetPlayerClass()->GetClassIndex() );
		CEconItemView *pItem = GetLoadoutItem( GetPlayerClass()->GetClassIndex(), iLoadoutSlot );

		// See if gamerules says this item isn't allowed right now
		bool bForceRemoved = bOverrideRemoval || !ItemIsAllowed( pItem );

		if ( bForceRemoved || !ItemsMatch( pData, pWeapon->GetAttributeContainer()->GetItem(), pItem, pWeapon ) )
		{
			// we can't hold this weapon anymore, switch to the next best weapon before removing it
			if ( GetActiveTFWeapon() == pWeapon )
			{
				SwitchToNextBestWeapon( pWeapon );
			}

			// drop weapon that belongs to other player, unless we're not regenerating
			// which happens at round restart
			if ( !bForceRemoved && m_bRegenerating )
			{
				CEconItemView *pDroppedItem = pWeapon->GetAttributeContainer()->GetItem();
				CSteamID steamID;
				GetSteamID( &steamID );
				if ( pDroppedItem->GetAccountID() != steamID.GetAccountID() )
				{
					// Find the position and angle of the weapons so the "ammo box" matches.
					Vector vecPackOrigin;
					QAngle vecPackAngles;
					if( !CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles ) )
						return;

					CTFDroppedWeapon *pDroppedWeapon = CTFDroppedWeapon::Create( this, vecPackOrigin, vecPackAngles, pWeapon->GetWorldModel(), pDroppedItem );
					if ( pDroppedWeapon )
					{
						pDroppedWeapon->InitDroppedWeapon( this, pWeapon, false );
					}
				}
			}

			// We shouldn't have this weapon. Remove it.
			Weapon_Detach( pWeapon );
			UTIL_Remove( pWeapon );
		}
		else if ( bResetWeapons )
		{
			// We should have this weapon. Reset it.
			pWeapon->ChangeTeam( GetTeamNumber() );
			pWeapon->GiveDefaultAmmo();
			pWeapon->ClearKillComboCount();

			if ( m_bRegenerating == false )
			{
				pWeapon->WeaponReset();
			}
			else
			{
				pWeapon->WeaponRegenerate();
			}
		}

		int nBuffType = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, nBuffType, set_buff_type );

		if ( pWeapon->GetWeaponID() == TF_WEAPON_BUFF_ITEM || nBuffType )
		{
			bFoundBuffItem = true;
		}
	}

	// Reenable sounds for all weapons
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(i);
		if ( !pWeapon )
			continue;

		pWeapon->SetSoundsEnabled( true );
	}

	// Prevent a rage exploit with changing items outside of a spawn room
	if ( ( IsPlayerClass( TF_CLASS_SOLDIER ) || IsPlayerClass( TF_CLASS_PYRO ) || IsPlayerClass( TF_CLASS_SNIPER ) ) && !bFoundBuffItem )
	{
		m_Shared.SetRageMeter( 0.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ValidateWearables( TFPlayerClassData_t *pData )
{
	CSteamID steamIDForPlayer;
	GetSteamID( &steamIDForPlayer );

	bool bIsDisguisedSpy = IsPlayerClass( TF_CLASS_SPY ) && m_Shared.InCond( TF_COND_DISGUISED );

	// Need to move backwards because we'll be removing them as we find them.
	for ( int wbl = m_hMyWearables.Count()-1; wbl >= 0; wbl-- )
	{
		CEconWearable *pWearable = m_hMyWearables[wbl];
		Assert( pWearable );
		if ( !pWearable )
		{
			// Integrity is failing, remove NULLs
			m_hMyWearables.Remove( wbl );
			continue;
		}
			
		CTFWearable *pTFWearable = assert_cast< CTFWearable* >( pWearable );
		if ( bIsDisguisedSpy && pTFWearable->IsDisguiseWearable() )
			continue;

		bool itemMatch = false;		

		// If you are an extra wearable, just make sure your associated weapon is valid instead
		CBaseEntity *pEntity = pTFWearable->GetWeaponAssociatedWith();
		if ( pEntity )
		{
			CTFWeaponBase *pWeapon = assert_cast< CTFWeaponBase* >( pTFWearable->GetWeaponAssociatedWith() );

			int iLoadoutSlot = pWeapon->GetAttributeContainer()->GetItem()->GetStaticData()->GetLoadoutSlot( GetPlayerClass()->GetClassIndex() );
			if (iLoadoutSlot >= 0 )
			{
				CEconItemView *pItem = TFInventoryManager()->GetItemInLoadoutForClass( GetPlayerClass()->GetClassIndex(), iLoadoutSlot, &steamIDForPlayer );
				itemMatch |= ItemsMatch( pData, pWeapon->GetAttributeContainer()->GetItem(), pItem );
			}
		}
		else
		{
			// Regular Wearable
			int iLoadoutSlot = pWearable->GetAttributeContainer()->GetItem()->GetStaticData()->GetLoadoutSlot( GetPlayerClass()->GetClassIndex() );
			if ( iLoadoutSlot >= 0 )
			{
				CEconItemView *pItem = TFInventoryManager()->GetItemInLoadoutForClass( GetPlayerClass()->GetClassIndex(), iLoadoutSlot, &steamIDForPlayer );
				itemMatch |= ItemsMatch( pData, pWearable->GetAttributeContainer()->GetItem(), pItem );

				// Item says what slot it wants to be in, but Misc's and Taunts can be in multiple places, check against all
				bool bLoadoutMisc = iLoadoutSlot == LOADOUT_POSITION_MISC;
				bool bLoadoutTaunt = iLoadoutSlot == LOADOUT_POSITION_TAUNT;
				if ( bLoadoutMisc || bLoadoutTaunt ) 
				{
					for ( int i = LOADOUT_POSITION_INVALID + 1; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
					{
						if ( ( bLoadoutMisc && IsMiscSlot( i ) ) || ( bLoadoutTaunt && IsTauntSlot( i ) ) )
						{
							pItem = TFInventoryManager()->GetItemInLoadoutForClass( GetPlayerClass()->GetClassIndex(), i, &steamIDForPlayer );
							itemMatch |= ItemsMatch( pData, pWearable->GetAttributeContainer()->GetItem(), pItem );
						}
					}
				}
			}
		}

		if ( !itemMatch || pWearable->GetTeamNumber() != GetTeamNumber() || m_bForceItemRemovalOnRespawn || m_bSwitchedClass )
		{
			if ( !pWearable->AlwaysAllow() )
			{
				// We shouldn't have this wearable. Remove it.
				RemoveWearable( pWearable );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PostInventoryApplication( void )
{
	m_Shared.RecalculatePlayerBodygroups();

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// Using weapons lockers destroys our disguise weapon, so we might need a new one.
		m_Shared.DetermineDisguiseWeapon( false );
	}

	// Apply set bonuses.
	ApplySetBonuses();

	// Remove our disguise if we can't disguise.
	if ( !CanDisguise() )
	{
		RemoveDisguise();
	}

	// Notify the client.
	IGameEvent *event = gameeventmanager->CreateEvent( "post_inventory_application" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		gameeventmanager->FireEvent( event ); 
	}

	// Iterate over all of our wearables
	int iPlayerSkinOverride = 0;
	for ( int i=0; i< GetNumWearables(); ++i )
	{
		CTFWearable *pWearable = dynamic_cast<CTFWearable *>( GetWearable( i ) );
		if ( pWearable == NULL || pWearable->IsDisguiseWearable() )
			continue;

		// Check if we have an item that activates the skin override we want
		// find first skin override
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWearable, iPlayerSkinOverride, player_skin_override );
		if ( iPlayerSkinOverride != 0 ) // Zombie
		{
			break;
		}
	}
	m_iPlayerSkinOverride = iPlayerSkinOverride;

	m_Inventory.ClearClassLoadoutChangeTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ManageRegularWeaponsLegacy( TFPlayerClassData_t *pData )
{
	CSteamID steamIDForPlayer;
	GetSteamID( &steamIDForPlayer );

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		if ( pData->m_aWeapons[iWeapon] != TF_WEAPON_NONE )
		{
			int iWeaponID = pData->m_aWeapons[iWeapon];
			const char *pszWeaponName = WeaponIdToAlias( iWeaponID );

			CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );

			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponName );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
			int iLoadoutSlot = pWeaponInfo->m_iWeaponType;

			// HACK: Convert engineer's second PDA to using the second pda slot
			if ( iWeaponID == TF_WEAPON_PDA_ENGINEER_DESTROY || iWeaponID == TF_WEAPON_INVIS )
			{
				iLoadoutSlot = LOADOUT_POSITION_PDA2;
			}


			// Do we have a custom weapon in this slot?
			CEconItemView *pItem = TFInventoryManager()->GetItemInLoadoutForClass( GetPlayerClass()->GetClassIndex(), iLoadoutSlot, &steamIDForPlayer );
			bool bHasNonBaseWeapon = pItem ? pItem->GetItemQuality() != AE_NORMAL : false;

			if ( pWeapon )
			{
				bool bShouldRemove = false;

				if ( pItem )
				{
					// If the item isn't the one we're supposed to have, nuke it
					if ( pWeapon->GetAttributeContainer()->GetItem()->GetItemID() != pItem->GetItemID() )
					{
						bShouldRemove = true;

						/*
						Msg("Removing %s because its global index (%d) doesn't match the loadout's (%d)\n", pWeapon->GetDebugName(), 
						pWeapon->GetAttributeContainer()->GetItem()->GetItemID(),
						pItem->GetItemID() );
						*/
					}
				}
				else
				{
					// We should have a base item in our loadout.
					if ( pWeapon->GetAttributeContainer()->GetItem()->GetItemQuality() != AE_NORMAL )
					{
						bShouldRemove = true;
						//Msg("Removing %s because it's a non-base item, and the loadout specifies a base item.\n", pWeapon->GetDebugName() );
					}
				}

				// If we already have a weapon in this slot but is not the same type, nuke it (changed classes)
				// We don't do this if the weapon in this slot isn't a base item, because items like the flaregun
				// don't have matching weaponIDs, yet they shouldn't be removed. The inventory system has already
				// ensured that the weapon is valid in this slot.
				if ( !bShouldRemove && pWeapon->GetWeaponID() != iWeaponID && !bHasNonBaseWeapon )
				{
					bShouldRemove = true;
					//Msg("Removing %s because it's not the right type for the class.\n", pWeapon->GetDebugName() );
				}

				if ( bShouldRemove )
				{
					Weapon_Detach( pWeapon );
					UTIL_Remove( pWeapon );
					pWeapon = NULL;
				}
			}

			if ( !bHasNonBaseWeapon )
			{
				pWeapon = dynamic_cast<CTFWeaponBase*>(Weapon_OwnsThisID( iWeaponID ));
			}

			if ( pWeapon )
			{
				Assert( pWeapon->GetAttributeContainer()->GetItem()->GetItemID() == ( pItem ? pItem->GetItemID() : INVALID_ITEM_ID ) );

				pWeapon->ChangeTeam( GetTeamNumber() );
				pWeapon->GiveDefaultAmmo();

				if ( m_bRegenerating == false )
				{
					pWeapon->WeaponReset();
				}

				//char tempstr[1024];
				//g_pVGuiLocalize->ConvertUnicodeToANSI( pWeapon->GetAttributeContainer()->GetItem()->GetItemName(), tempstr, sizeof(tempstr) );
				//Msg("Updated %s for %s\n", tempstr, GetPlayerName() );
			}
			else
			{
				CEconEntity* pNewItem = dynamic_cast<CEconEntity*>(GiveNamedItem( pszWeaponName, 0, pItem ));
				Assert( pNewItem );
				if ( pNewItem )
				{
					//char tempstr[1024];
					//g_pVGuiLocalize->ConvertUnicodeToANSI( pWeapon->GetAttributeContainer()->GetItem()->GetItemName(), tempstr, sizeof(tempstr) );
					//Msg("Created %s for %s\n", tempstr, GetPlayerName() );
					//pWeapon->DebugDescribe();

					pNewItem->GiveTo( this );
				}
			}
		}
		else
		{
			//I shouldn't have any weapons in this slot, so get rid of it
			CTFWeaponBase *pCarriedWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );

			//Don't nuke builders since they will be nuked if we don't need them later.
			if ( pCarriedWeapon && pCarriedWeapon->GetWeaponID() != TF_WEAPON_BUILDER )
			{
				Weapon_Detach( pCarriedWeapon );
				UTIL_Remove( pCarriedWeapon );
			}
		}
	}

	// If we lack a primary or secondary weapon, start with our melee weapon ready.
	// This is for supporting new unlockables that take up weapon slots and leave our character with nothing to wield.
	int iMainWeaponCount = 0;
	CTFWeaponBase* pMeleeWeapon = NULL;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase*) GetWeapon(i);

		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_PRIMARY ||
			 pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_SECONDARY )
		{
			++iMainWeaponCount;
		}
		else if ( pWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_MELEE )
		{
			pMeleeWeapon = pWeapon;
		}
	}
	if ( pMeleeWeapon && (iMainWeaponCount==0) )
	{
		Weapon_Switch( pMeleeWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create and give the named item to the player. Then return it.
//-----------------------------------------------------------------------------
CBaseEntity	*CTFPlayer::GiveNamedItem( const char *pszName, int iSubType, const CEconItemView *pScriptItem, bool bForce )
{
	// We need to support players putting any shotgun into a shotgun slot, pistol into a pistol slot, etc.
	// For legacy reasons, different classes actually spawn different entities for their shotguns/pistols/etc.
	// To deal with this, we translate entities into the right one for the class we're playing.
	if ( !bForce )
	{
		// We don't do this if force is set, since a spy might be disguising as this character, etc.
		pszName = TranslateWeaponEntForClass( pszName, GetPlayerClass()->GetClassIndex() );
	}

	if ( !pszName )
		return NULL;

	// If I already own this type don't create one
	if ( Weapon_OwnsThisType(pszName, iSubType) && !bForce)
	{
		Assert(0);
		return NULL;
	}

	CBaseEntity *pItem = NULL;

	if ( pScriptItem )
	{
		// Generate a weapon directly from that item	
		pItem = ItemGeneration()->GenerateItemFromScriptData( pScriptItem, GetLocalOrigin(), vec3_angle, pszName );
	}
	else
	{
		// Generate a base item of the specified type
		CItemSelectionCriteria criteria;
		criteria.SetQuality( AE_NORMAL );
		criteria.BAddCondition( "name", k_EOperator_String_EQ, pszName, true );
		pItem = ItemGeneration()->GenerateRandomItem( &criteria, GetAbsOrigin(), vec3_angle, pszName );
	}

	if ( pItem == NULL )
	{
		Msg( "Failed to generate base item: %s\n", pszName );
		return NULL;
	}

	pItem->AddSpawnFlags( SF_NORESPAWN );

	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( (CBaseEntity*)pItem );
	if ( pWeapon )
	{
		pWeapon->SetSubType( iSubType );
	}

	DispatchSpawn( pItem );

	if ( pItem != NULL && !(pItem->IsMarkedForDeletion()) ) 
	{
		pItem->Touch( this );
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy all attributes on this player that match the bSetBonuses flag
//-----------------------------------------------------------------------------
void CTFPlayer::RemovePlayerAttributes( bool bSetBonuses )
{
	const int iAttribs = m_AttributeList.GetNumAttributes();
	for ( int i = iAttribs-1; i >= 0; i-- )
	{
		const CEconItemAttribute *pAttrib = m_AttributeList.GetAttribute(i);
		const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( pAttrib->GetAttribIndex() );
		if ( !pAttrDef || (pAttrDef->BIsSetBonusAttribute() == bSetBonuses) )
		{
			m_AttributeList.RemoveAttributeByIndex( i );
		}
	}
	GetAttributeManager()->OnAttributeValuesChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ApplySetBonuses( void )
{
	RemovePlayerAttributes( true );

	CUtlVector<const CEconItemSetDefinition *> pActiveSets;
	GetActiveSets( &pActiveSets );

	FOR_EACH_VEC( pActiveSets, set )
	{
		for ( int i = 0; i < pActiveSets[set]->m_iAttributes.Count(); i++ )
		{
			const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinition( pActiveSets[set]->m_iAttributes[i].m_iAttribDefIndex );	
			if ( pAttrDef )
			{
				Assert( pAttrDef->GetAttributeType() );
				Assert( pAttrDef->GetAttributeType()->BSupportsGameplayModificationAndNetworking() );		// is an assert instead of a check because we're in client code here -- this means someone set up a set with bad data
				Assert( pAttrDef->BIsSetBonusAttribute() );

				float flAttrValue = pActiveSets[set]->m_iAttributes[i].m_flValue;
				GetAttributeList()->SetRuntimeAttributeValue( pAttrDef, flAttrValue );
			}
		}
	}
}

#ifdef TF_RAID_MODE
//-----------------------------------------------------------------------------
// Return true if the given entity can be used by a dead Raider
// as a respawn point in Raid mode.
bool IsValidRaidRespawnTarget( CBaseEntity *entity )
{
	if ( !entity->IsPlayer() )
	{
		CObjectSentrygun *pSentry = dynamic_cast< CObjectSentrygun* >( entity );
		if ( pSentry && pSentry->GetTeamNumber() == TF_TEAM_BLUE )
		{
			if ( pSentry->GetOwner() && !pSentry->GetOwner()->IsBot() )
			{
				return true;
			}
		}

		return false;
	}

	if ( entity->GetTeamNumber() != TF_TEAM_BLUE )
		return false;

	CTFPlayer *player = ToTFPlayer( entity );
	CTFBot *bot = ToTFBot( player );
	return !bot || !bot->HasAttribute( CTFBot::IS_NPC );
}
#endif // TF_RAID_MODE

//-----------------------------------------------------------------------------
// Purpose: Find a spawn point for the player.
//-----------------------------------------------------------------------------
CBaseEntity* CTFPlayer::EntSelectSpawnPoint()
{
	CBaseEntity *pSpot = g_pLastSpawnPoints[ GetTeamNumber() ];
	const char *pSpawnPointName = "";

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			// only spawn next to friends if the round is not restarting
			if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
			{
				if ( tf_raid_use_rescue_closets.GetBool() )
				{
					// find a valid rescue closet to spawn into
					CBaseEntity *rescueSpawn = g_pRaidLogic->GetRescueRespawn();

					if ( rescueSpawn )
					{
						return rescueSpawn;
					}
				}
				else if ( tf_boss_battle_respawn_on_friends.GetBool() )
				{
					// the raiders are in the wild - respawn next to them
					float timeSinceInjured = -1.0f;
					CBaseEntity *respawnEntity = NULL;

					// if we are observing a friend, spawn into them
					CBaseEntity *watchEntity = GetObserverTarget();
					if ( watchEntity && IsValidRaidRespawnTarget( watchEntity ) )
					{
						respawnEntity = watchEntity;
					}
					else
					{
						// spawn on the least recently damaged friend
						CTeam *raidingTeam = GetGlobalTeam( TF_TEAM_BLUE );
						for( int i=0; i<raidingTeam->GetNumPlayers(); ++i )
						{
							CTFPlayer *buddy = (CTFPlayer *)raidingTeam->GetPlayer(i);

							// we can't use IsAlive(), because that has already been reset since
							// this code is mid-spawn.  Use m_Shared state instead.
							if ( buddy != this && buddy->m_Shared.InState( TF_STATE_ACTIVE ) && IsValidRaidRespawnTarget( buddy ) )
							{
								// pick the friend who has been hurt least recently
								if ( buddy->GetTimeSinceLastInjury( TF_TEAM_RED ) > timeSinceInjured )
								{
									timeSinceInjured = buddy->GetTimeSinceLastInjury( TF_TEAM_RED );
									respawnEntity = buddy;
								}
							}
						}
					}

					if ( respawnEntity )
					{
						CPVSFilter filter( respawnEntity->GetAbsOrigin() );
						TE_TFParticleEffect( filter, 0.0, "teleported_blue", respawnEntity->GetAbsOrigin(), vec3_angle );
						TE_TFParticleEffect( filter, 0.0, "player_sparkles_blue", respawnEntity->GetAbsOrigin(), vec3_angle, this, PATTACH_POINT );
						return respawnEntity;
					}
				}
			}
		}
	}
#endif // TF_RAID_MODE

	bool bMatchSummary = TFGameRules() && TFGameRules()->ShowMatchSummary();

	// See if the map is asking to force this player to spawn at a specific location
	if ( GetRespawnLocationOverride() && !bMatchSummary )
	{
		if ( SelectSpawnSpotByName( GetRespawnLocationOverride(), pSpot ) )
		{
			m_pSpawnPoint = dynamic_cast< CTFTeamSpawn* >( pSpot );		// Is this even used anymore?
			return pSpot;
		}
		
		// If the entity doesn't exist - or isn't valid - let the regular system handle it
	}

	switch( GetTeamNumber() )
	{
	case TF_TEAM_RED:
	case TF_TEAM_BLUE:
		{
			pSpawnPointName = "info_player_teamspawn";
			if ( SelectSpawnSpotByType( pSpawnPointName, pSpot ) )
			{
				g_pLastSpawnPoints[ GetTeamNumber() ] = pSpot;
			}
			else if ( pSpot )
			{
				int iClass = GetPlayerClass()->GetClassIndex();
				if ( iClass >= 0 && iClass < ARRAYSIZE( g_aPlayerClassNames ) )
				{
					Warning( "EntSelectSpawnPoint(): No valid spawns for class %s on team %i found, even though at least one spawn entity exists.\n", g_aPlayerClassNames[iClass], GetTeamNumber() );
				}
			}

			// need to save this for later so we can apply and modifiers to the armor and grenades...after the call to InitClass()
			m_pSpawnPoint = dynamic_cast<CTFTeamSpawn*>( pSpot );
			break;
		}
	case TEAM_SPECTATOR:
	case TEAM_UNASSIGNED:
	default:
		{
			pSpot = CBaseEntity::Instance( INDEXENT(0) );
			break;		
		}
	}

	if ( !pSpot )
	{
		Warning( "PutClientInServer: no %s on level\n", pSpawnPointName );
		return CBaseEntity::Instance( INDEXENT(0) );
	}

	return pSpot;
} 

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::SelectSpawnSpotByType( const char *pEntClassName, CBaseEntity* &pSpot )
{
	bool bMatchSummary = TFGameRules()->ShowMatchSummary();
	CBaseEntity *pMatchSummaryFallback = NULL;

	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	if ( !pSpot )
	{
		// Sometimes the first spot can be NULL????
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}

	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawn point that's clear except for another players. We
	// don't collide with our team members, so we should be fine.
	bool bIgnorePlayers = false;
	// When dealing with a standard spawn ent, try to obey any class spawn flags
	bool bRestrictByClass = !V_strcmp( pEntClassName, "info_player_teamspawn" );

	CBaseEntity *pFirstSpot = pSpot;
	do 
	{
		if ( pSpot )
		{
			// Check to see if this is a valid team spawn (player is on this team, etc.).
			if ( TFGameRules()->IsSpawnPointValid( pSpot, this, bIgnorePlayers ) )
			{
				// Check for a bad spawn entity.
				if ( pSpot->GetAbsOrigin() == Vector( 0, 0, 0 ) )
				{
					goto next_spawn_point;
				}
				// SpawnFlags were only recently added to the .fgd (Feb 2016), which means older maps won't have any flags at all (they default to on).
				// So this means we only look for restrictions when we find flags, which a map compiled after this change would/should have.
				else if ( bRestrictByClass && pSpot->GetSpawnFlags() )
				{
					int nClass = GetPlayerClass()->GetClassIndex() - 1;
					if ( !pSpot->HasSpawnFlags( ( 1 << nClass ) ) )
					{
						goto next_spawn_point;
					}
				}

				// Found a valid spawn point.
				return true;
			}
		}

	next_spawn_point:;

		// Let's save off a fallback spot for competitive mode
		if ( bMatchSummary && !pMatchSummaryFallback )
		{
			CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
			if ( pCTFSpawn )
			{
				if ( ( pCTFSpawn->GetTeamNumber() == pCTFSpawn->GetTeamNumber() ) && ( pCTFSpawn->GetMatchSummaryType() == PlayerTeamSpawn_MatchSummary_None ) )
				{
					pMatchSummaryFallback = pCTFSpawn;
				}
			}
		}

		// Get the next spawning point to check.
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );

		// Exhausted the list
		if ( pSpot == pFirstSpot )
		{
			// Loop through again, ignoring class restrictions (but check against players)
			if ( bRestrictByClass )
			{
				bRestrictByClass = false;
				pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
			}
			// Loop through again, ignoring players and classes
			else if ( !bRestrictByClass && !bIgnorePlayers )
			{
				bIgnorePlayers = true;
				pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
			}
		}
	} 
	// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot );

	// Return a fallback spot for competitive mode
	if ( bMatchSummary && pMatchSummaryFallback )
	{
		pSpot = pMatchSummaryFallback;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: We're being asked to use a spawn with a specific name
//-----------------------------------------------------------------------------
bool CTFPlayer::SelectSpawnSpotByName( const char *pEntName, CBaseEntity* &pSpot )
{
	if ( pEntName && pEntName[0] )
	{
		pSpot = gEntList.FindEntityByName( pSpot, pEntName );

		while ( pSpot )
		{
			if ( TFGameRules()->IsSpawnPointValid( pSpot, this, true, PlayerTeamSpawnMode_Triggered ) )
				return true;

			pSpot = gEntList.FindEntityByName( pSpot, pEntName );
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	MDLCACHE_CRITICAL_SECTION();

	m_PlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_TAUNT_ENABLE_MOVE )
	{
		m_bAllowMoveDuringTaunt = true;
	}
	else if ( pEvent->event == AE_TAUNT_DISABLE_MOVE )
	{
		m_bAllowMoveDuringTaunt = false;
	}
	else if ( pEvent->event == AE_WPN_HIDE )
	{
		// does nothing for now.
	}
	else if ( pEvent->event == AE_TAUNT_ADD_ATTRIBUTE )
	{
		char szAttrName[128];
		float flVal;
		float flDuration;
		if ( sscanf( pEvent->options, "%127s %f %f", szAttrName, &flVal, &flDuration ) == 3 )
		{
			szAttrName[ ARRAYSIZE( szAttrName ) - 1 ] = '\0';
			Assert( flDuration > 0.f );
			AddCustomAttribute( szAttrName, flVal, flDuration );
		}
		else
		{
			Warning( "Usage: AE_TAUNT_ADD_ATTRIBUTE <frame> <attr_name> <attr_value> <duration>\n" );
		}
	}
	else if ( pEvent->event == AE_SV_EXCLUDE_PLAYER_SOUND )
	{
		CBroadcastNonOwnerRecipientFilter filter( this );
		EmitSound( filter, entindex(), pEvent->options );
	}
	else if ( pEvent->event == AE_RAGDOLL && IsAlive() )
	{
		// Eat this.  Players have found an exploit where they can run this binding:
		// bind "q" "+attack; wait 10; join_class heavyweapons; -attack
		// while they are Engineer, while holding the wrench, while in a respawn room, that will cause them
		// to ragdoll as the Heavy, but be invisible and able to kill everyone.
		//
		// The event seems to be tied to the special headshot-kill anim on the Heavy, but this doesn't break that.
	}
	else
	{
		BaseClass::HandleAnimEvent( pEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PhysObjectSleep()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Sleep();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PhysObjectWake()
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj )
		pObj->Wake();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetAutoTeam( int nPreferedTeam /*= TF_TEAM_AUTOASSIGN*/ )
{
	int iTeam = TEAM_SPECTATOR;

	CTFTeam *pBlue = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	CTFTeam *pRed  = TFTeamMgr()->GetTeam( TF_TEAM_RED );

	if ( pBlue && pRed )
	{
		if ( TFGameRules() )
		{
			if ( TFGameRules()->IsInHighlanderMode() )
			{
				if ( ( pBlue->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 ) &&
					 ( pRed->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 ) )
				{
					// teams are full....join team Spectator for now
					return TEAM_SPECTATOR;
				}
			}

			bool bReturnDefenders = false;

#ifdef TF_RAID_MODE
			if ( TFGameRules()->IsBossBattleMode() )
			{
				bReturnDefenders = true;
			}
#endif // TF_RAID_MODE

			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				bReturnDefenders = true;
			}

			if ( bReturnDefenders )
			{
				// If joining a MVM game that's in-progress, give us the max per-player collected value
				if ( TFGameRules()->IsMannVsMachineMode() && g_pPopulationManager )
				{
					int nRoundCurrency = MannVsMachineStats_GetAcquiredCredits();
					nRoundCurrency += g_pPopulationManager->GetStartingCurrency();

					// Check to see if this player has an upgrade history and apply it to them
					// deduct any cash that has already been spent
					int spentCurrency = g_pPopulationManager->GetPlayerCurrencySpent( this );

					if ( m_nCurrency < nRoundCurrency )
					{
						SetCurrency( nRoundCurrency - spentCurrency );
					}

					if ( g_pPopulationManager )
					{
						// See if the team's earned any respec credits
						if ( TFGameRules()->IsMannVsMachineRespecEnabled() && !g_pPopulationManager->GetNumRespecsAvailableForPlayer( this ) )
						{
							uint16 nRespecs = g_pPopulationManager->GetNumRespecsEarned();
							if ( nRespecs )
							{
								g_pPopulationManager->SetNumRespecsForPlayer( this, nRespecs );
							}
						}

						// Set buyback credits - if they aren't reconnecting
						if ( !g_pPopulationManager->IsPlayerBeingTrackedForBuybacks( this ) )
						{
							g_pPopulationManager->SetBuybackCreditsForPlayer( this, tf_mvm_buybacks_per_wave.GetInt() );
						}
					}
				}

				return TFGameRules()->GetTeamAssignmentOverride( this, TF_TEAM_PVE_DEFENDERS );
			}
		}

		CTFBot *pPlayerBot = dynamic_cast<CTFBot*>( this );
		if ( FStrEq( tf_bot_quota_mode.GetString(), "fill" ) && ( tf_bot_quota.GetInt() > 0 ) && !( pPlayerBot && pPlayerBot->HasAttribute( CTFBot::QUOTA_MANANGED ) ) )
		{
			// We're using 'tf_bot_quota_mode fill' to keep the teams even so balance based on the human players on each team
			int nPlayerCountRed = 0;
			int nPlayerCountBlue = 0;

			for ( int i = 1; i <= gpGlobals->maxClients; ++i )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer == NULL )
					continue;

				if ( FNullEnt( pPlayer->edict() ) )
					continue;

				if ( !pPlayer->IsConnected() )
					continue;

				if ( !pPlayer->IsPlayer() )
					continue;

				CTFBot* pBot = dynamic_cast<CTFBot*>( pPlayer );
				if ( pBot && pBot->HasAttribute( CTFBot::QUOTA_MANANGED ) )
					continue;

				if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
				{
					nPlayerCountRed++;
				}
				else if( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
				{
					nPlayerCountBlue++;
				}
			}

			if ( nPlayerCountRed < nPlayerCountBlue )
			{
				iTeam = TF_TEAM_RED;
			}
			else if ( nPlayerCountBlue < nPlayerCountRed )
			{
				iTeam = TF_TEAM_BLUE;
			}
			else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT || pRed->GetRole() == TEAM_ROLE_DEFENDERS )
			{
				// AutoTeam should give new players to the attackers on A/D maps if the teams are even
				iTeam = TF_TEAM_BLUE;
			}
			else
			{
				// teams have an even number of human players, pick a random team
				iTeam = RandomInt( 0, 1 ) ? TF_TEAM_RED : TF_TEAM_BLUE;
			}

			bool bKick = false;
			// Now we have a team we want to join to balance the human players, can we join it?
			if ( iTeam == TF_TEAM_RED )
			{
				if ( pBlue->GetNumPlayers() < pRed->GetNumPlayers() )
				{
					bKick = true;
				}
			}
			else
			{
				if ( pRed->GetNumPlayers() < pBlue->GetNumPlayers() )
				{
					bKick = true;
				}
			}

			if ( !bKick || TheTFBots().RemoveBotFromTeamAndKick( iTeam ) )
			{
				return iTeam;
			}

			// If kick needed but failed, fall through to default logic
		}

		if ( pBlue->GetNumPlayers() < pRed->GetNumPlayers() )
		{
			iTeam = TF_TEAM_BLUE;
		}
		else if ( pRed->GetNumPlayers() < pBlue->GetNumPlayers() )
		{
			iTeam = TF_TEAM_RED;
		}
		else if ( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT || pRed->GetRole() == TEAM_ROLE_DEFENDERS )
		{
			// AutoTeam should give new players to the attackers on A/D maps if the teams are even
			iTeam = TF_TEAM_BLUE;
		}
		else
		{
			if ( nPreferedTeam == TF_TEAM_AUTOASSIGN )
			{
				iTeam = RandomInt( 0, 1 ) ? TF_TEAM_RED : TF_TEAM_BLUE;
			}
			else
			{
				Assert( nPreferedTeam >= FIRST_GAME_TEAM );
				iTeam = nPreferedTeam;
			}
		}
	}

	return iTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldForceAutoTeam( void )
{
	if ( mp_forceautoteam.GetBool() )
		return true;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		return true;

	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return true;

	bool bForce = false;

	// On official servers, and in normal game modes, see if we should re-assign returning players
	if ( TFGameRules() && TFGameRules()->IsDefaultGameMode() )
	{
		int nTimeSinceLast = TFGameRules()->PlayerHistory_GetTimeSinceLastSeen( this );
		bForce = ( tf_mm_trusted.GetBool() && nTimeSinceLast > 0 && nTimeSinceLast < 60 );
	}

	return bForce;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam( const char *pTeamName )
{
	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
		return;

	if ( GetTeamNumber() == TF_TEAM_RED || GetTeamNumber() == TF_TEAM_BLUE )
	{
		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		if ( pMatchDesc && !pMatchDesc->BAllowTeamChange() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_Ladder_NoTeamChange" );
			return;
		}
		else if ( TFGameRules()->ArePlayersInHell() || TFGameRules()->IsPowerupMode() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_CantChangeTeamNow" );
			return;
		}
	}

	bool bAutoTeamed = false;
	bool bArenaSpectator = false;

	int iTeam = TF_TEAM_RED;

	if ( stricmp( pTeamName, "auto" ) == 0 )
	{	
		iTeam = GetAutoTeam();
		bAutoTeamed = true;
	}
	else if ( stricmp( pTeamName, "spectate" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;
	}
	else if ( stricmp( pTeamName, "spectatearena" ) == 0 )
	{
		iTeam = TEAM_SPECTATOR;

		if ( mp_allowspectators.GetBool() == true )
		{
			bArenaSpectator = true;
		}
	}
	else
	{
		for ( int i = 0; i < TF_TEAM_COUNT; ++i )
		{
			COMPILE_TIME_ASSERT( TF_TEAM_COUNT == ARRAYSIZE( g_aTeamNames ) );
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	// now check if we're limited in our team selection (unless we want to be on the spectator team)
	if ( !IsBot() && iTeam != TEAM_SPECTATOR )
	{		
		int iHumanTeam = TFGameRules()->GetAssignedHumanTeam();
		if ( iHumanTeam != TEAM_ANY )
		{
			iTeam = iHumanTeam;
			bAutoTeamed = true;
		}
	}
	
	// invalid team selection
	if ( iTeam < TEAM_SPECTATOR )
	{
		return;
	}

	if ( IsCoaching() && ( iTeam != TEAM_SPECTATOR ) )
		return;

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( !IsBot() && iTeam != TEAM_SPECTATOR )
		{
			// human raiders can only be on the blue team
			CTeam *raidingTeam = GetGlobalTeam( TF_TEAM_BLUE );
			int humanCount = 0;
			for( int i=0; i<raidingTeam->GetNumPlayers(); ++i )
			{
				if ( raidingTeam->GetPlayer(i)->IsBot() )
					continue;

				++humanCount;
			}

			if ( humanCount < tf_raid_team_size.GetInt() )
			{
				iTeam = TF_TEAM_BLUE;
			}
			else
			{
				// no room
				iTeam = TEAM_SPECTATOR;
			}
		}
	}

	if ( TFGameRules()->IsBossBattleMode() )
	{
		if ( !IsBot() && iTeam != TEAM_SPECTATOR )
		{
			// players can only be on the blue team
			if ( GetGlobalTeam( TF_TEAM_BLUE )->GetNumPlayers() < tf_boss_battle_team_size.GetInt() )
			{
				iTeam = TF_TEAM_BLUE;
			}
			else
			{
				// no room
				iTeam = TEAM_SPECTATOR;
			}
		}

		DuelMiniGame_NotifyPlayerChangedTeam( this, iTeam, true );
		ChangeTeam( iTeam, true );

		return;
	}
#endif // TF_RAID_MODE	

	// Some game modes will overrule our player-based logic
	iTeam = TFGameRules()->GetTeamAssignmentOverride( this, iTeam );

	if ( iTeam == TEAM_SPECTATOR || ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() && GetTeamNumber() <= LAST_SHARED_TEAM ) )
	{
		// Prevent this is the cvar is set
		if ( ( mp_allowspectators.GetBool() == false ) && !IsHLTV() && !IsReplay() && TFGameRules()->IsInArenaMode() == false )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return;
		}

		// Deny spectator access if it would unbalance the teams
		if ( ( mp_spectators_restricted.GetBool() || tf_mm_trusted.GetBool() ) && TFGameRules() && !TFGameRules()->IsMannVsMachineMode() )
		{
			if ( GetTeamNumber() == TF_TEAM_RED || GetTeamNumber() == TF_TEAM_BLUE )
			{
				CTeam *pRedTeam = GetGlobalTeam( TF_TEAM_RED );
				CTeam *pBlueTeam = GetGlobalTeam( TF_TEAM_BLUE );
				if ( pRedTeam && pBlueTeam )
				{
					int nRedCount = pRedTeam->GetNumPlayers();
					int nBlueCount = pBlueTeam->GetNumPlayers();
					int nGap = GetTeamNumber() == TF_TEAM_RED ? ( nBlueCount - nRedCount ) : ( nRedCount - nBlueCount );
					if ( nGap >= mp_teams_unbalance_limit.GetInt() )
					{
						ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator_Unbalance" );
						return;
					}
				}
			}
		}

		if ( GetTeamNumber() != TEAM_UNASSIGNED && !IsDead() )
		{
			CommitSuicide( false, true );
		}

		m_bArenaSpectator = bArenaSpectator;
		DuelMiniGame_NotifyPlayerChangedTeam( this, TEAM_SPECTATOR, true );
		ChangeTeam( TEAM_SPECTATOR );

		if ( m_bArenaSpectator == true )
		{
			SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );
			TFGameRules()->Arena_ClientDisconnect( GetPlayerName() );
			TFGameRules()->RemovePlayerFromQueue( this );
		}

		// do we have fadetoblack on? (need to fade their screen back in)
		if ( mp_fadetoblack.GetBool() )
		{
			color32_s clr = { 0,0,0,255 };
			UTIL_ScreenFade( this, clr, 0, 0, FFADE_IN | FFADE_PURGE );
		}

		if ( TFGameRules()->IsInArenaMode() == true && m_bArenaSpectator == false )
		{
			ShowViewPortPanel( PANEL_CLASS_BLUE );
		}
	}
	else
	{
		if ( iTeam == GetTeamNumber() )
		{
			return;	// we wouldn't change the team
		}

		if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() )
		{
			CTFTeam *pTeam = TFTeamMgr()->GetTeam( iTeam );
			if ( pTeam )
			{
				if ( pTeam->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 )
				{
					// if this join would put too many players on the team in Highlander mode, refuse
					// come up with a better way to tell the player they tried to join a full team!
					ShowViewPortPanel( PANEL_TEAM );
					return;
				}
			}
		}

		// if this join would unbalance the teams, refuse
		// come up with a better way to tell the player they tried to join a full team!
		if ( TFGameRules()->WouldChangeUnbalanceTeams( iTeam, GetTeamNumber() ) )
		{
			ShowViewPortPanel( PANEL_TEAM );
			return;
		}

		DuelMiniGame_NotifyPlayerChangedTeam( this, iTeam, true );
		bool bSilent = TFGameRules() && TFGameRules()->IsPVEModeActive() && IsBot();

#ifndef _DEBUG
		TFGameRules()->SetPlayerReadyState( entindex(), false );
		TFGameRules()->SetTeamReadyState( false, GetTeamNumber() );
#endif // _DEBUG

		ChangeTeam( iTeam, bAutoTeamed, bSilent );

		if ( tf_arena_force_class.GetBool() == false )
		{
			ShowViewPortPanel( ( iTeam == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Join a team without using the game menus
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinTeam_NoMenus( const char *pTeamName )
{
	Assert( IsX360() );

	Msg( "Client command HandleCommand_JoinTeam_NoMenus: %s\n", pTeamName );

	// Only expected to be used on the 360 when players leave the lobby to start a new game
	if ( !IsInCommentaryMode() )
	{
		Assert( GetTeamNumber() == TEAM_UNASSIGNED );
		Assert( IsX360() );
	}

	int iTeam = TEAM_SPECTATOR;
	if ( Q_stricmp( pTeamName, "spectate" ) )
	{
		for ( int i = 0; i < TF_TEAM_COUNT; ++i )
		{
			COMPILE_TIME_ASSERT( TF_TEAM_COUNT == ARRAYSIZE( g_aTeamNames ) );
			if ( stricmp( pTeamName, g_aTeamNames[i] ) == 0 )
			{
				iTeam = i;
				break;
			}
		}
	}

	ForceChangeTeam( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: Player has been forcefully changed to another team
//-----------------------------------------------------------------------------
void CTFPlayer::ForceChangeTeam( int iTeamNum, bool bFullTeamSwitch )
{
	int iNewTeam = iTeamNum;

	if ( iNewTeam == TF_TEAM_AUTOASSIGN )
	{
		iNewTeam = GetAutoTeam();
	}

	if ( !GetGlobalTeam( iNewTeam ) )
	{
		Warning( "CTFPlayer::ForceChangeTeam( %d ) - invalid team index.\n", iNewTeam );
		return;
	}

	// Some game modes will overrule our player-based logic
	if ( !TFGameRules()->IsCommunityGameMode() )
		iNewTeam = TFGameRules()->GetTeamAssignmentOverride( this, iNewTeam );

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iNewTeam == iOldTeam )
		return;

	// can't change teams if in a duel
	if ( DuelMiniGame_IsInDuel( this ) )
	{
		if ( !m_bIsCoaching )
			return;

		DuelMiniGame_NotifyPlayerChangedTeam( this, iTeamNum, true );
	}

	// can't change teams if coaching
	if ( m_bIsCoaching && m_hStudent != NULL && iTeamNum != TEAM_SPECTATOR )
		return;

	RemoveAllOwnedEntitiesFromWorld( true );
	
	m_iPreviousteam = iOldTeam;
	
	BaseClass::ChangeTeam( iNewTeam, false, true );

	if ( !bFullTeamSwitch )
	{
		RemoveNemesisRelationships();

		if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() )
		{
			if ( IsAlive() )
			{
				CommitSuicide( false, true );
			}

			ResetPlayerClass();
		}
	}
	
	if ( iNewTeam == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else if ( iNewTeam == TEAM_SPECTATOR )
	{
		StateTransition( TF_STATE_OBSERVER );

		RemoveAllWeapons();
		DestroyViewModels();

		if ( TFGameRules()->IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
		{
			TFGameRules()->AddPlayerToQueueHead( this );
		}
	}

	DropFlag();

	// Don't modify living players in any way
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleFadeToBlack( void )
{
	if ( mp_fadetoblack.GetBool() )
	{
		SetObserverMode( OBS_MODE_CHASE );

		color32_s clr = { 0,0,0,255 };
		UTIL_ScreenFade( this, clr, 0.75, 0, FFADE_OUT | FFADE_STAYOUT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent, bool bAutoBalance /*= false*/ )
{
	if ( !GetGlobalTeam( iTeamNum ) )
	{
		Warning( "CTFPlayer::ChangeTeam( %d ) - invalid team index.\n", iTeamNum );
		return;
	}

	// game rules don't allow to change team
	if ( TFGameRules() && !TFGameRules()->CanChangeTeam( GetTeamNumber() ) )
	{
		return;
	}

	// Not allowed to change teams when a ghost
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		return;
	}

	// Not allowed to change teams in bumper kart
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		return;
	}

	// can only be on TEAM_SPECTATOR when coaching
	if ( IsCoaching() && ( iTeamNum >= FIRST_GAME_TEAM ) )
	{
		return;
	}

	// Some game modes will overrule our player-based logic
	iTeamNum = TFGameRules()->GetTeamAssignmentOverride( this, iTeamNum, bAutoBalance );

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if ( iTeamNum == iOldTeam )
		return;

	RemoveAllOwnedEntitiesFromWorld( true );

	bool bNoTeam = GetTeamNumber() == TEAM_UNASSIGNED;

	m_iPreviousteam = iOldTeam;

	CTF_GameStats.Event_TeamChange( this, iOldTeam, iTeamNum );

	m_iTeamChanges++;

	// If joining the underdog team, make next spawn instant (autobalance, paladins)
	if ( TFGameRules() && TFGameRules()->IsDefaultGameMode() && GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		int nStackedTeam, nWeakTeam;
		if ( TFGameRules()->AreTeamsUnbalanced( nStackedTeam, nWeakTeam ) )
		{
			if ( iTeamNum == nWeakTeam )
			{
				AllowInstantSpawn();
			}
		}
	}

	BaseClass::ChangeTeam( iTeamNum, bAutoTeam, bSilent, bAutoBalance );

	if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() )
	{
		if ( IsAlive() )
		{
			CommitSuicide( false, true );
		}

		ResetPlayerClass();
	}

	RemoveNemesisRelationships();

	if ( iTeamNum == TEAM_UNASSIGNED )
	{
		StateTransition( TF_STATE_OBSERVER );
	}
	else if ( iTeamNum == TEAM_SPECTATOR )
	{
		StateTransition( TF_STATE_OBSERVER );

		RemoveAllWeapons();
		DestroyViewModels();

		if ( TFGameRules()->IsInArenaMode() == true && bNoTeam == false && tf_arena_use_queue.GetBool() == true )
		{
			TFGameRules()->AddPlayerToQueue( this );
		}
	}
	else // active player
	{
		bool bKill = true;


		if ( bKill && !IsDead() && (iOldTeam == TF_TEAM_RED || iOldTeam == TF_TEAM_BLUE) )
		{
			// Kill player if switching teams while alive
			CommitSuicide( false, true );
		}
		else if ( IsDead() && iOldTeam < FIRST_GAME_TEAM )
		{
			HandleFadeToBlack();
		}

		// let any spies disguising as me know that I've changed teams
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( pTemp && pTemp != this )
			{
				if ( ( pTemp->m_Shared.GetDisguiseTarget() == this ) || // they were disguising as me and I've changed teams
 					 ( !pTemp->m_Shared.GetDisguiseTarget() && pTemp->m_Shared.GetDisguiseTeam() == iTeamNum ) ) // they don't have a disguise and I'm joining the team they're disguising as
				{
					// choose someone else...
					pTemp->m_Shared.FindDisguiseTarget();
				}
			}
		}
	}
	
	m_Shared.RemoveAllCond();
	DuelMiniGame_NotifyPlayerChangedTeam( this, iTeamNum, false );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ResetPlayerClass( void )
{
	if ( GetPlayerClass() )
	{
		GetPlayerClass()->Reset();
	}

	SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleCommand_JoinClass( const char *pClassName, bool bAllowSpawn /* = true */ )
{
	VPROF_BUDGET( "CTFPlayer::HandleCommand_JoinClass", VPROF_BUDGETGROUP_PLAYER );
	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
	{
		return;
	}

// 	if ( TFGameRules()->ArePlayersInHell() && ( m_Shared.m_iDesiredPlayerClass > TF_CLASS_UNDEFINED ) )
// 	{
// 		ClientPrint( this, HUD_PRINTCENTER, "#TF_CantChangeClassNow" );
// 		return;
// 	}

	if ( TFGameRules()->IsCompetitiveMode() )
	{
		if ( !tf_tournament_classchange_allowed.GetBool() && 
			 TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_Ladder_NoClassChangeRound" );
			return;
		}

		if ( !tf_tournament_classchange_ready_allowed.GetBool() && 
			 TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS && 
			 TFGameRules()->IsPlayerReady( entindex() ) )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_Ladder_NoClassChangeReady" );
			return;
		}
	}

	if ( IsCoaching() )
		return;

	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return;

	// can only join a class after you join a valid team
	if ( GetTeamNumber() <= LAST_SHARED_TEAM && TFGameRules()->IsInArenaMode() == false )
		return;

	// In case we don't get the class menu message before the spawn timer
	// comes up, fake that we've closed the menu.
	SetClassMenuOpen( false );

	if ( TFGameRules()->InStalemate() && TFGameRules()->IsInArenaMode() == false )
	{
		if ( IsAlive() && !TFGameRules()->CanChangeClassInStalemate() )
		{
			char szTime[6];
			Q_snprintf( szTime, sizeof( szTime ), "%d", tf_stalematechangeclasstime.GetInt() );
	
			ClientPrint( this, HUD_PRINTTALK, "#game_stalemate_cant_change_class", szTime );
			return;
		}
	}

	if ( TFGameRules()->IsInArenaMode() == true && IsAlive() == true )
	{
		if ( GetTeamNumber() > LAST_SHARED_TEAM && TFGameRules()->InStalemate() == true ) 
		{
			ClientPrint( this, HUD_PRINTTALK, "#TF_Arena_NoClassChange" );
			return;
		}
	}

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() && GetTeamNumber() == TF_TEAM_BLUE && !tf_raid_allow_class_change.GetBool() )
	{
		CTFNavArea *area = (CTFNavArea *)GetLastKnownArea();

		if ( area && !area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
		{
			ClientPrint( this, HUD_PRINTTALK, "No class changes after leaving the safe room" );
			return;
		}
	}
#endif // TF_RAID_MODE

	// this was moved outside of the block below for the community game mode that allows upgrades
	if ( m_nCanPurchaseUpgradesCount > 0 )
	{
		ClientPrint( this, HUD_PRINTCENTER, "#TF_MVM_NoClassUpgradeUI" );
		return;
	}

	if ( TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_DEFENDERS )
	{
		if ( IsReadyToPlay() && !TFGameRules()->InSetup() && g_pPopulationManager && !g_pPopulationManager->IsInEndlessWaves() )
		{
			ClientPrint( this, HUD_PRINTTALK, "#TF_MVM_NoClassChangeAfterSetup" );
			return;
		}
	}

	int iClass = TF_CLASS_UNDEFINED;
	bool bShouldNotRespawn = false;

	if ( !bAllowSpawn || ( ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && ( TFGameRules()->GetWinningTeam() != GetTeamNumber() ) ) )
	{
		m_bAllowInstantSpawn = false;
		bShouldNotRespawn = true;
	}

	if ( stricmp( pClassName, "random" ) != 0 && stricmp( pClassName, "auto" ) != 0 )
	{
		int i = 0;

		for ( i = TF_CLASS_SCOUT ; i < TF_CLASS_COUNT_ALL ; i++ )
		{
			if ( stricmp( pClassName, GetPlayerClassData( i )->m_szClassName ) == 0 )
			{
				iClass = i;
				break;
			}
		}
		 
		bool bCivilianOkay = false;

		if ( !bCivilianOkay && ( i >= TF_LAST_NORMAL_CLASS ) )
		{
			Warning( "HandleCommand_JoinClass( %s ) - invalid class name.\n", pClassName );
			return;
		}

		// Check class limits
		if ( !TFGameRules()->CanPlayerChooseClass(this, iClass) )
		{
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			return;
		}
	}
	else
	{
		int iChoices = 0;
		int iClasses[ TF_LAST_NORMAL_CLASS - 1 ] = {}; // -1 to remove the civilian from the randomness
		int iCurrentClass = GetPlayerClass()->GetClassIndex();

		for ( iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
		{
			if ( iClass != iCurrentClass && TFGameRules()->CanPlayerChooseClass( this, iClass ) )
			{
				iClasses[ iChoices++ ] = iClass;
			}
		}

		if ( !iChoices )
		{
			if ( TFGameRules()->CanPlayerChooseClass( this, iCurrentClass ) )
				return;

			// We failed to find a random class. Bring up the class menu again.
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			return;
		}

		iClass = iClasses[ random->RandomInt( 0, iChoices - 1 ) ];
	}

	if ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		// Bit field of classes played during the game
		CSteamID steamID;
		GetSteamID( &steamID );

		CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
		if ( pMatch )
		{
			CMatchInfo::PlayerMatchData_t *pMatchPlayer = pMatch->GetMatchDataForPlayer( steamID );
			if ( pMatchPlayer )
			{
				pMatchPlayer->UpdateClassesPlayed( GetPlayerClass()->GetClassIndex() );
			}
		}
	}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() && !IsBot() )
	{
		Vector vPos = GetAbsOrigin();
		QAngle qAngle = GetAbsAngles();
		SetDesiredPlayerClassIndex( iClass );
		ForceRespawn();
		Teleport( &vPos, &qAngle, &vec3_origin );
		return;
	}
#endif // _DEBUG || STAGING_ONLY

	// joining the same class?
	if ( iClass != TF_CLASS_RANDOM && iClass == GetDesiredPlayerClassIndex() )
	{
		// If we're dead, and we have instant spawn, respawn us immediately. Catches the case
		// where a player misses respawn wave because they're at the class menu, and then changes
		// their mind and reselects their current class.
		if ( m_bAllowInstantSpawn && !IsAlive() )
		{
			ForceRespawn();
		}
		return;
	}

	if ( TFGameRules()->IsInArenaMode() && tf_arena_use_queue.GetBool() == true && GetTeamNumber() <= LAST_SHARED_TEAM )
	{
		TFGameRules()->AddPlayerToQueue( this );
	}

	// @note Tom Bui: we need to restrict the UI somehow
	// if there's a class restriction on duels...
	int iDuelClass = DuelMiniGame_GetRequiredPlayerClass( this );
	if ( iDuelClass >= TF_FIRST_NORMAL_CLASS && iDuelClass < TF_LAST_NORMAL_CLASS )
	{
		iClass = iDuelClass;
	}

	SetDesiredPlayerClassIndex( iClass );
	IGameEvent * event = gameeventmanager->CreateEvent( "player_changeclass" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "class", iClass );

		gameeventmanager->FireEvent( event );
	}

	// are they TF_CLASS_RANDOM and trying to select the class they're currently playing as (so they can stay this class)?
	if ( iClass == GetPlayerClass()->GetClassIndex() )
	{
		// If we're dead, and we have instant spawn, respawn us immediately. Catches the case
		// were a player misses respawn wave because they're at the class menu, and then changes
		// their mind and reselects their current class.
		if ( m_bAllowInstantSpawn && !IsAlive() )
		{
			ForceRespawn();
		}
		return;
	}

	// We can respawn instantly if:
	//	- We're dead, and we're past the required post-death time
	//	- We're inside a respawn room of our own team
	//	- We're in the stalemate grace period
	bool bInRespawnRoom = PointInRespawnRoom( this, WorldSpaceCenter(), true );
	if ( bInRespawnRoom && !IsAlive() )
	{
		// If we're not spectating ourselves, ignore respawn rooms. Otherwise we'll get instant spawns
		// by spectating someone inside a respawn room.
		bInRespawnRoom = (GetObserverTarget() == this);
	}
	bool bDeadInstantSpawn = !IsAlive();
	if ( bDeadInstantSpawn && m_flDeathTime )
	{
		// In death mode, don't allow class changes to force respawns ahead of respawn waves
		float flWaveTime = TFGameRules()->GetNextRespawnWave( GetTeamNumber(), this );
		bDeadInstantSpawn = (gpGlobals->curtime > flWaveTime);
	}
	bool bInStalemateClassChangeTime = false;
	if ( TFGameRules()->InStalemate() && TFGameRules()->IsInWaitingForPlayers() == false )
	{
		// Stalemate overrides respawn rules. Only allow spawning if we're in the class change time.
		bInStalemateClassChangeTime = TFGameRules()->CanChangeClassInStalemate();
		bDeadInstantSpawn = false;
		bInRespawnRoom = false;
	}

	if ( TFGameRules()->IsInArenaMode() == true )
	{
		if ( TFGameRules()->IsInWaitingForPlayers() == false )
		{
			bDeadInstantSpawn = false;

			if ( TFGameRules()->InStalemate() == false && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN  )
			{
				bInRespawnRoom = true;
				bShouldNotRespawn = false;
			}
			else
			{
				bShouldNotRespawn = true;
				
				 if ( tf_arena_use_queue.GetBool() == false )
					return;
			}
		}
		else if ( tf_arena_use_queue.GetBool() == false )
		{
			return;
		}
	}

	if ( TFGameRules()->IsMannVsMachineMode() && TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS )
		m_bAllowInstantSpawn = true;

	if ( bShouldNotRespawn == false && ( m_bAllowInstantSpawn || bDeadInstantSpawn || bInRespawnRoom || bInStalemateClassChangeTime ) )
	{
		ForceRespawn();


		return;
	}

	if( iClass == TF_CLASS_RANDOM )
	{
		if( IsAlive() )
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_respawn_asrandom" );
		}
		else
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_spawn_asrandom" );
		}
	}
	else
	{
		if( IsAlive() )
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_respawn_as", GetPlayerClassData( iClass )->m_szLocalizableName );
		}
		else
		{
			ClientPrint(this, HUD_PRINTTALK, "#game_spawn_as", GetPlayerClassData( iClass )->m_szLocalizableName );
		}
	}

	if ( IsAlive() && ( GetHudClassAutoKill() == true ) && bShouldNotRespawn == false )
	{
		CommitSuicide( false, true );
	}

}

//-----------------------------------------------------------------------------
// Purpose: The GC has told us this player wants to respawn now that their loadout has changed.
//-----------------------------------------------------------------------------
void CTFPlayer::CheckInstantLoadoutRespawn( void )
{
	// Must be alive
	if ( !IsAlive() )
		return;

	// In a respawn room of your own team
	if ( !PointInRespawnRoom( this, WorldSpaceCenter(), true ) )
		return;
	
	// Not in stalemate (beyond the change class period)
	if ( TFGameRules()->InStalemate() && !TFGameRules()->CanChangeClassInStalemate() )
		return;

	// Not in Arena mode
	if ( TFGameRules()->IsInArenaMode() == true )
		return;

	// Not if we're on the losing team
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() != GetTeamNumber() ) 
		return;

	// Not if our current class's loadout hasn't changed
	int iClass = GetPlayerClass() ? GetPlayerClass()->GetClassIndex() : TF_CLASS_UNDEFINED;
	if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS )
	{
		if ( m_Inventory.ClassLoadoutHasChanged( iClass ) )
		{
			if ( m_Shared.InCond( TF_COND_AIMING ) )
			{
				// If we are in condition TF_COND_AIMING it will be removed during the ForceRespawn() so we need to reset the weapon
				// (which is normally skipped while regenerating)...this only affects the Minigun and the Sniper Rifle.
				CTFWeaponBase *pWeapon = GetActiveTFWeapon();
				if ( pWeapon )
				{
					if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) || WeaponID_IsSniperRifleOrBow( pWeapon->GetWeaponID() ) )
					{
						pWeapon->WeaponReset();
					}
				}
			}

			if ( IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( GetActiveTFWeapon() );
				if ( pMedigun )
				{
					pMedigun->Lower();
				}
			}

			// Force healers to disconnect, so they re-evaluate any healing mods/attributes from the patient
			for ( int i = 0; i < m_Shared.GetNumHealers(); i++ )
			{
				CTFPlayer *pMedic = ToTFPlayer( m_Shared.GetHealerByIndex( i ) );
				if ( pMedic && pMedic->IsPlayerClass( TF_CLASS_MEDIC ) && pMedic->GetActiveTFWeapon() )
				{
					pMedic->GetActiveTFWeapon()->Lower();
				}
			}

			// We want to use ForceRespawn() here so the player is physically moved back
			// into the spawn room and not just regenerated instantly in the doorway
			ForceRegenerateAndRespawn();
		}
	}
}

class CGC_RespawnPostLoadoutChange : public GCSDK::CGCClientJob
{
public:
	CGC_RespawnPostLoadoutChange( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCRespawnPostLoadoutChange_t> msg( pNetPacket );
		CSteamID steamID = msg.Body().m_ulInitiatorSteamID;

		// Find the player with this steamID
		CSteamID tmpID;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;
			if ( !pPlayer->GetSteamID( &tmpID ) )
				continue;

			if ( tmpID == steamID )
			{
				pPlayer->CheckInstantLoadoutRespawn();
				break;
			}
		}

		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGC_RespawnPostLoadoutChange, "CGC_RespawnPostLoadoutChange", k_EMsgGCRespawnPostLoadoutChange, GCSDK::k_EServerTypeGCClient );

#if defined (_DEBUG)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void DebugEconItemView( const char *pszDescStr, CEconItemView *pEconItemView )
{
	if ( !pEconItemView )
		return;

	const GameItemDefinition_t *pItemDef = pEconItemView->GetItemDefinition();
	Assert( pItemDef );

	Warning("%s: \"%s\"\n", pszDescStr, pItemDef->GetDefinitionName() );
}
#endif

bool CTFPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];
	
	m_flLastAction = gpGlobals->curtime;

	if ( FStrEq( pcmd, "addcond" ) )
	{
		if ( sv_cheats->GetBool() && args.ArgC() >= 2 )
		{
			CTFPlayer *pTargetPlayer = this;
			if ( args.ArgC() >= 4 )
			{
				// Find the matching netname
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(i) );
					if ( pPlayer )
					{
						if ( Q_strstr( pPlayer->GetPlayerName(), args[3] ) )
						{
							pTargetPlayer = ToTFPlayer(pPlayer);
							break;
						}
					}
				}
			}

			int iCond = atoi( args[1] );
			if ( args[1][0] != '0' && iCond == 0 )
			{
				iCond = GetTFConditionFromName( args[1] );
			}

			ETFCond eCond = TF_COND_INVALID;
			if ( iCond >= 0 && iCond < TF_COND_LAST )
			{
				eCond = ( ETFCond )iCond;
			}
			else
			{
				Warning( "Failed to addcond %s to player %s\n", args[1], pTargetPlayer->GetPlayerName() );
				return true;
			}

			if ( args.ArgC() >= 3 )
			{
				float flDuration = atof( args[2] );
				pTargetPlayer->m_Shared.AddCond( eCond, flDuration );
			}
			else
			{
				pTargetPlayer->m_Shared.AddCond( eCond );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "removecond" ) )
	{
		if ( sv_cheats->GetBool() && args.ArgC() >= 2 )
		{
			CTFPlayer *pTargetPlayer = this;
			if ( args.ArgC() >= 3 )
			{
				// Find the matching netname
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(i) );
					if ( pPlayer )
					{
						if ( Q_strstr( pPlayer->GetPlayerName(), args[2] ) )
						{
							pTargetPlayer = ToTFPlayer(pPlayer);
							break;
						}
					}
				}
			}

			if ( FStrEq( args[1], "all" ) )
			{
				pTargetPlayer->m_Shared.RemoveAllCond();
			}
			else
			{
				int iCond = atoi( args[1] );
				if ( args[1][0] != '0' && iCond == 0 )
				{
					iCond = GetTFConditionFromName( args[1] );
				}

				if ( iCond >= 0 && iCond < TF_COND_LAST )
				{
					ETFCond eCond = (ETFCond)iCond;
					pTargetPlayer->m_Shared.RemoveCond( eCond );
				}
				else
				{
					Warning( "Failed to removecond %s from player %s\n", args[1], pTargetPlayer->GetPlayerName() );
				}
			}
		}
		return true;
	}
#ifdef _DEBUG
	else if ( FStrEq( pcmd, "burn" ) ) 
	{
		m_Shared.Burn( this, GetActiveTFWeapon() );
		return true;
	}
	else if ( FStrEq( pcmd, "bleed" ) )
	{
		m_Shared.MakeBleed( this, GetActiveTFWeapon(), 10.0f );
		return true;
	}
	else if ( FStrEq( pcmd, "dump_damagers" ) )
	{
		m_AchievementData.DumpDamagers();
		return true;
	}
	else if ( FStrEq( pcmd, "stun" ) )
	{
		if ( args.ArgC() >= 4 )
		{
			m_Shared.StunPlayer( atof(args[1]), atof(args[2]), atof(args[3]) );
		}
		return true;
	}
// 	else if ( FStrEq( pcmd, "decoy" ) )
// 	{
// 		CBotNPCDecoy *decoy = (CBotNPCDecoy *)CreateEntityByName( "bot_npc_decoy" );
// 		if ( decoy )
// 		{
// 			decoy->SetOwnerEntity( this );
// 			DispatchSpawn( decoy );
// 		}
// 		return true;
// 	}
	else if ( FStrEq( pcmd, "tada" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			Taunt( TAUNT_SHOW_ITEM );
		}
		return true;
	}
//	else if ( FStrEq( pcmd, "player_disguise" ) )
//	{
//		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByName( args[1] ) );
//		pPlayer->m_Shared.Disguise( Q_atoi( args[2] ), Q_atoi( args[3] ) );
//		return true;
//	}
	else
#endif

	if ( FStrEq( pcmd, "jointeam" ) )
	{
		// don't let them spam the server with changes
		if ( ( GetNextChangeTeamTime() > gpGlobals->curtime ) && ( GetTeamNumber() != TEAM_UNASSIGNED ) )
			return true;

		SetNextChangeTeamTime( gpGlobals->curtime + 2.0f );  // limit to one change every 2 secs

		if ( args.ArgC() >= 2 )
		{
			HandleCommand_JoinTeam( args[1] );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "jointeam_nomenus" ) )
	{
		if ( IsX360() )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinTeam_NoMenus( args[1] );
			}
			return true;
		}
		return false;
	}
	else if ( FStrEq( pcmd, "closedwelcomemenu" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( GetTeamNumber() == TEAM_UNASSIGNED )
			{
				if ( ShouldForceAutoTeam() )
				{
					ChangeTeam( GetAutoTeam(), true, false );
					ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_BLUE ) ? PANEL_CLASS_BLUE : PANEL_CLASS_RED );
				}
				else
				{
					ShowViewPortPanel( PANEL_TEAM, true );
				}
			}
			else if ( IsPlayerClass( TF_CLASS_UNDEFINED ) )
			{
				if ( tf_arena_force_class.GetBool() == false )
				{
					switch( GetTeamNumber() )
					{
					case TF_TEAM_RED:
						ShowViewPortPanel( PANEL_CLASS_RED, true );
						break;

					case TF_TEAM_BLUE:
						ShowViewPortPanel( PANEL_CLASS_BLUE, true );
						break;

					default:
						break;
					}
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "joinclass" ) ) 
	{
		// don't let them spam the server with changes
		if ( GetNextChangeClassTime() > gpGlobals->curtime )
			return true;

		SetNextChangeClassTime( gpGlobals->curtime + 0.5 );  // limit to one change every 0.5 secs

		if ( tf_arena_force_class.GetBool() == false )
		{
			if ( args.ArgC() >= 2 )
			{
				HandleCommand_JoinClass( args[1] );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "resetclass" ) )
	{
		if ( TFGameRules() && TFGameRules()->IsInHighlanderMode() && ( GetTeamNumber() > LAST_SHARED_TEAM ) )
		{
			if ( IsAlive() )
			{
				CommitSuicide( false, true );
			}

			ResetPlayerClass();
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "mp_playgesture" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() == 1 )
			{
				Warning( "mp_playgesture: Gesture activity or sequence must be specified!\n" );
				return true;
			}

			if ( sv_cheats->GetBool() )
			{
				if ( !PlayGesture( args[1] ) )
				{
					Warning( "mp_playgesture: unknown sequence or activity name \"%s\"\n", args[1] );
					return true;
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "mp_playanimation" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() == 1 )
			{
				Warning( "mp_playanimation: Activity or sequence must be specified!\n" );
				return true;
			}

			if ( sv_cheats->GetBool() )
			{
				if ( !PlaySpecificSequence( args[1] ) )
				{
					Warning( "mp_playanimation: Unknown sequence or activity name \"%s\"\n", args[1] );
					return true;
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "menuopen" ) )
	{
		SetClassMenuOpen( true );
		return true;
	}
	else if ( FStrEq( pcmd, "menuclosed" ) )
	{
		SetClassMenuOpen( false );
		return true;
	}
	else if ( FStrEq( pcmd, "pda_click" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// player clicked on the PDA, play attack animation
			CTFWeaponBase *pWpn = GetActiveTFWeapon();
			CTFWeaponPDA *pPDA = dynamic_cast<CTFWeaponPDA *>( pWpn );

			if ( pPDA && !m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "weapon_taunt" ) || FStrEq( pcmd, "taunt" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( m_flTauntNextStartTime < gpGlobals->curtime )
			{
				int iTauntSlot = args.ArgC() == 2 ? atoi( args[1] ) : 0;
				HandleTauntCommand( iTauntSlot );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "stop_taunt" ) )
	{
		if( m_Shared.GetTauntIndex() == TAUNT_LONG && !m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			EndLongTaunt();
		}

		return true;
	}
	else if ( FStrEq( pcmd, "-taunt" ) )
	{
		// DO NOTHING
		// We changed taunt key to be press to toggle instead of press and hold to do long taunt
		return true;
	}
	else if ( FStrEq( pcmd, "td_buyback" ) )
	{
		if ( TFGameRules() && TFGameRules()->IsPVEModeActive() && IsObserver() && ( GetTeamNumber() > TEAM_SPECTATOR ) && IsValidTFPlayerClass( GetPlayerClass()->GetClassIndex() ) )
		{
			// Make sure we're not still in freezecam
			int iObsMode = GetObserverMode();
			if ( iObsMode == OBS_MODE_FREEZECAM || iObsMode == OBS_MODE_DEATHCAM )
				return true;

			float flWaveTime = TFGameRules()->GetNextRespawnWave( GetTeamNumber(), this );

			bool bSuccess = false;
			int iRespawnWait = (flWaveTime - gpGlobals->curtime);
			int iCost = iRespawnWait * MVM_BUYBACK_COST_PER_SEC;

			if ( iCost <= 0 )
				return true;

			// New system (finite buybacks per-wave, not currency-based)
			if ( tf_mvm_buybacks_method.GetBool() )
			{
				if ( g_pPopulationManager->GetNumBuybackCreditsForPlayer( this ) )
				{
					bSuccess = true;
					iCost = 1;
					g_pPopulationManager->RemoveBuybackCreditFromPlayer( this );
				}
			}
			// Old system (currency-based)
			else
			{
				if ( GetCurrency() >= iCost )
				{
					bSuccess = true;
					RemoveCurrency( iCost );
					MannVsMachineStats_PlayerEvent_BoughtInstantRespawn( this, iCost );
				}
			}

			if ( bSuccess )
			{
				ForceRespawn();
				IGameEvent *event = gameeventmanager->CreateEvent( "player_buyback" );
				if ( event )
				{
					event->SetInt( "player", entindex() );
					event->SetInt( "cost", iCost );
					gameeventmanager->FireEvent( event );
				}
			}
			else
			{
				CSingleUserRecipientFilter filter( this );
				EmitSound_t params;
				params.m_pSoundName = "Player.DenyWeaponSelection";
				EmitSound( filter, entindex(), params );
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "build" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( TFGameRules()->InStalemate() && mp_stalemate_meleeonly.GetBool() )
				return true;

			// can't issue a build command while carrying an object
			if ( m_Shared.IsCarryingObject() )
				return true;

			if ( IsTaunting() )
				return true;

			int iBuilding = 0;
			int iMode = 0;
			bool bArgsChecked = false;

			// Fixup old binds.
			if ( args.ArgC() == 2 )
			{
				iBuilding = atoi( args[ 1 ] );
				if ( iBuilding == 3 ) // Teleport exit is now a mode.
				{
					iBuilding = 1;
					iMode = 1;
				}
				bArgsChecked = true;
			}
			else if ( args.ArgC() == 3 )
			{
				iBuilding = atoi( args[ 1 ] );
				iMode = atoi( args[ 2 ] );
				bArgsChecked = true;
			}

			if ( bArgsChecked )
			{
				StartBuildingObjectOfType( iBuilding, iMode );
			}
			else
			{
				Warning( "Usage: build <building> <mode>\n" );
			}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
			if ( bot_mimic.GetBool() )
			{
				CUtlVector< CTFPlayer* > vecPlayers;
				CollectPlayers( &vecPlayers, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );
				FOR_EACH_VEC( vecPlayers, i )
				{
					CTFPlayer *pTFBot = ToTFPlayer( vecPlayers[i] );
					if ( !pTFBot )
						continue;

					if ( !pTFBot->IsPlayerClass( TF_CLASS_ENGINEER ) )
						continue;

					if ( pTFBot->GetPlayerType() != CTFPlayer::TEMP_BOT )
						continue;

					// Mimic it
					pTFBot->ClientCommand( args );
				}
			}
#endif // STAGING_ONLY, _DEBUG
		}

		return true;
	}
	else if ( FStrEq( pcmd, "destroy" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( IsPlayerClass( TF_CLASS_ENGINEER ) ) // Spies can't destroy buildings (sappers)
			{
				int iBuilding = 0;
				int iMode = 0;
				bool bArgsChecked = false;

				// Fixup old binds.
				if ( args.ArgC() == 2 )
				{
					iBuilding = atoi( args[ 1 ] );
					if ( iBuilding == 3 ) // Teleport exit is now a mode.
					{
						iBuilding = 1;
						iMode = 1;
					}
					bArgsChecked = true;
				}
				else if ( args.ArgC() == 3 )
				{
					iBuilding = atoi( args[ 1 ] );
					iMode = atoi( args[ 2 ] );
					bArgsChecked = true;
				}

				if ( bArgsChecked )
				{
					DetonateObjectOfType( iBuilding, iMode );
				}
				else
				{
					Warning( "Usage: destroy <building> <mode>\n" );
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "eureka_teleport" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			CTFWeaponBase* pWeapon = GetActiveTFWeapon();
			if ( !pWeapon )
				return true;

			if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
				return true;

			if ( TFGameRules() && TFGameRules()->ArePlayersInHell() )
				return true;

			int iAltFireTeleportToSpawn = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iAltFireTeleportToSpawn, alt_fire_teleport_to_spawn );

			if ( IsPlayerClass( TF_CLASS_ENGINEER ) && iAltFireTeleportToSpawn )
			{
				if ( args.ArgC() == 2 )
				{
					m_eEurekaTeleportTarget = (eEurekaTeleportTargets)atoi( args[1] );
				}
				else
				{
					m_eEurekaTeleportTarget = EUREKA_TELEPORT_HOME;
				}

				// Do the Eureka Effect teleport taunt
				Taunt( TAUNT_SPECIAL, MP_CONCEPT_TAUNT_EUREKA_EFFECT_TELEPORT );
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "arena_changeclass" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( TFGameRules() && TFGameRules()->IsInArenaMode() && ( tf_arena_force_class.GetBool() == true ) )
			{
				if ( TFGameRules()->State_Get() == GR_STATE_PREROUND )
				{
					if ( m_Shared.GetArenaNumChanges() < tf_arena_change_limit.GetInt() )
					{
						CommitSuicide( true, false );
						m_Shared.IncrementArenaNumChanges();
					}
				}
			}
		}
		return true;
	}
	else if ( FStrEq( pcmd, "extendfreeze" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) && IsDead() && !m_bAlreadyUsedExtendFreezeThisDeath )
		{
			m_bAlreadyUsedExtendFreezeThisDeath = true;
			m_flDeathTime += 2.0f;
		}
		return true;
	}
	else if ( FStrEq( pcmd, "show_motd" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( ShouldForceAutoTeam() )
			{
				int nPreferedTeam = TF_TEAM_AUTOASSIGN;
				PlayerHistoryInfo_t *pPlayerInfo = ( TFGameRules() ) ? TFGameRules()->PlayerHistory_GetPlayerInfo( this ) : NULL;
				if ( pPlayerInfo && pPlayerInfo->nTeam >= FIRST_GAME_TEAM )
				{
					nPreferedTeam = pPlayerInfo->nTeam;
				}

				int iTeam = GetAutoTeam( nPreferedTeam );
				ChangeTeam( iTeam, true, false );
				ShowViewPortPanel( ( iTeam == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			}
#ifdef TF_RAID_MODE
			else if ( TFGameRules()->IsBossBattleMode() )
			{
				int iTeam = GetAutoTeam();
				ChangeTeam( iTeam, true );
				ShowViewPortPanel( ( iTeam == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
			}
#endif
			else
			{
				ShowViewPortPanel( PANEL_TEAM, false );
			}
			ShowViewPortPanel( PANEL_ARENA_TEAM, false );

			char pszWelcome[128];
			Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome" );
			if ( UTIL_GetActiveHolidayString() )
			{
				Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome_%s", UTIL_GetActiveHolidayString() );
			}

			KeyValues *data = new KeyValues( "data" );
			data->SetString( "title", pszWelcome );		// info panel title
			data->SetString( "type", "1" );				// show userdata from stringtable entry
			data->SetString( "msg",	"motd" );			// use this stringtable entry
			data->SetString( "msg_fallback",	"motd_text" );			// use this stringtable entry if the base is HTML, and client has disabled HTML motds
			data->SetBool( "unload", sv_motd_unload_on_dismissal.GetBool() );

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
		return true;
	}
	else if ( FStrEq( pcmd, "show_htmlpage" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			if ( args.ArgC() != 2 )
			{
				Warning( "Usage: show_htmlpage <url>\n" );
				return true;
			}

			KeyValues *data = new KeyValues( "data" );
			data->SetString( "title", "#TF_Welcome" );	// info panel title
			data->SetString( "type", "2" );				// show url
			data->SetString( "msg",	args[1] );
			data->SetString( "msg_fallback", "motd_text" );			// use this stringtable entry if the base is HTML, and client has disabled HTML motds
			data->SetInt( "cmd", TEXTWINDOW_CMD_CLOSED_HTMLPAGE );		// exec this command if panel closed
			data->SetString( "customsvr", "1" );
			data->SetBool( "unload", false );

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
		return true;
	}
	else if ( FStrEq( pcmd, "closed_htmlpage" ) )
	{
		// Does nothing, it's for server plugins to hook.
		return true;
	}
	else if ( FStrEq( pcmd, "condump_on" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
				Msg("Console dumping on.\n");
				return true;
		}
	}
	else if ( FStrEq( pcmd, "condump_off" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
				Msg("Console dumping off.\n");
				return true;
		}
	}
	else if ( FStrEq( pcmd, "spec_next" ) ) // chase next player
	{
		if ( m_bIsCoaching )
		{
			return true;
		}
// 		if ( !ShouldRunRateLimitedCommand( args ) )
// 			return true;

		// intentionally falling through to the bottom so the baseclass version is called
		m_bArenaIsAFK = false;
	}
	else if ( FStrEq( pcmd, "spec_prev" ) ) // chase prev player
	{
		if ( m_bIsCoaching )
		{
			return true;
		}
// 		if ( !ShouldRunRateLimitedCommand( args ) )
// 			return true;

		// intentionally falling through to the bottom so the baseclass version is called
		m_bArenaIsAFK = false;
	}
	else if ( FStrEq( pcmd, "spec_mode" ) ) // set obs mode
	{
// 		if ( !ShouldRunRateLimitedCommand( args ) )
// 			return true;

		// intentionally falling through to the bottom so the baseclass version is called
		m_bArenaIsAFK = false;
	}
	else if ( FStrEq( pcmd, "showroundinfo" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// don't let the player open the round info menu until they're a spectator or they're on a regular team and have picked a class
			if ( ( GetTeamNumber() == TEAM_SPECTATOR ) || ( ( GetTeamNumber() != TEAM_UNASSIGNED ) && ( GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) ) )
			{
				if ( TFGameRules() )
				{
					TFGameRules()->ShowRoundInfoPanel( this );
				}
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "autoteam" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
			if ( !pMatchDesc || pMatchDesc->BAllowTeamChange() )
			{
				bool bPreventCustomGameModeChange = ( IsCustomGameMode() && ( GetTeamNumber() >= FIRST_GAME_TEAM ) );
				if ( !IsCoaching() && !bPreventCustomGameModeChange )
				{
					int iTeam = GetAutoTeam();
					ChangeTeam( iTeam, true, false );

					if ( iTeam > LAST_SHARED_TEAM )
					{
						ShowViewPortPanel( ( iTeam == TF_TEAM_RED ) ? PANEL_CLASS_RED : PANEL_CLASS_BLUE );
					}
				}
			}
		}

		return true;
	}
	else if ( FStrEq( pcmd, "coach_command" ) )
	{
		if ( m_bIsCoaching && m_hStudent && args.ArgC() > 1 )
		{
			eCoachCommand command = (eCoachCommand)atoi( args[1] );
			HandleCoachCommand( this, command );
			return true;
		}
	}
	else if ( FStrEq( pcmd, "boo" ) && m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		if ( m_booTimer.IsElapsed() )
		{
			m_booTimer.Start( 1.f );
			EmitSound( "Halloween.GhostBoo" );
		}

		return true;
	}
	else if ( FStrEq( pcmd, "loot_response" ) )
	{
		// Only allowed to speak these during post-game MvM
		if (   !TFGameRules() 
			|| !TFGameRules()->IsMannVsMachineMode()
			|| !( TFGameRules()->State_Get() == GR_STATE_GAME_OVER ) )
		{
			return true;
		}

		if ( FStrEq( args[1], "common" ) )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_COMMON );
			return true;
		}
		else if ( FStrEq( args[1], "rare" ) )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_RARE );
			return true;
		}
		else if ( FStrEq( args[1], "ultra_rare" ) )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_ULTRARARE );
			return true;
		}
	}
	else if ( FStrEq( pcmd, "done_viewing_loot" ) )
	{
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && g_pPopulationManager )
		{
			g_pPopulationManager->PlayerDoneViewingLoot( this );
		}
		return true;
	}
	else if ( FStrEq( pcmd, "spectate" ) )
	{
		HandleCommand_JoinTeam( "spectate" );
		return true;
	}
	else if ( FStrEq( pcmd, "team_ui_setup" ) )
	{
		bool bAutoTeam = ShouldForceAutoTeam();
#ifdef TF_RAID_MODE
		bAutoTeam |= TFGameRules()->IsBossBattleMode();
#endif
		
		// For autoteam, display the appropriate team's CLASS selection ui
		if ( bAutoTeam )
		{
			ChangeTeam( GetAutoTeam(), true, false );
			ShowViewPortPanel( ( GetTeamNumber() == TF_TEAM_BLUE ) ? PANEL_CLASS_BLUE : PANEL_CLASS_RED );
		}
		// Otherwise, show TEAM selection ui
		else
		{
			ShowViewPortPanel( PANEL_TEAM );
		}

		return true;
	}
	else if ( FStrEq( "next_map_vote", pcmd ) )
	{
		CTFGameRules::EUserNextMapVote eVoteState = (CTFGameRules::EUserNextMapVote)atoi( args[1] );
		switch( eVoteState )
		{
		case CTFGameRules::USER_NEXT_MAP_VOTE_MAP_0:
		case CTFGameRules::USER_NEXT_MAP_VOTE_MAP_1:
		case CTFGameRules::USER_NEXT_MAP_VOTE_MAP_2:
			// Valid
			break;
		default:
			// Invalid
			Assert( false );
			return true;
		}

		// No flip flop!
		if ( TFGameRules()->PlayerNextMapVoteState( entindex() ) != CTFGameRules::USER_NEXT_MAP_VOTE_UNDECIDED )
			return true;

		// Needs to do next-map voting
		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		if ( !pMatchDesc || !pMatchDesc->BUsesMapVoteAfterMatchEnds() )
			return true;

		if ( TFGameRules()->State_Get() != GR_STATE_GAME_OVER )
			return true;

		CMatchInfo* pMatch = GTFGCClientSystem()->GetMatch();
		if ( !pMatch )
			return true;

		TFGameRules()->SetPlayerNextMapVote( entindex(), eVoteState );
		DevMsg( "Settings player %d to rematch vote state %d.\n", entindex(), eVoteState );
		
		return true;
	}
	else if ( FStrEq( "cyoa_pda_open", pcmd ) )
	{
		bool bOpen = atoi( args[1] ) != 0;

		if ( bOpen && IsTaunting() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#TF_CYOA_PDA_Taunting" );
		}
		else
		{
			m_bViewingCYOAPDA.Set( bOpen );
			TeamFortress_SetSpeed();
		}
		return true;
	}

	return BaseClass::ClientCommand( args );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetClassMenuOpen( bool bOpen )
{
	m_bIsClassMenuOpen = bOpen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsClassMenuOpen( void )
{
	return m_bIsClassMenuOpen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::MerasmusPlayerBombExplode( bool bExcludeMe /*= true */ )
{
	float flDamage = 40.0f;
	// bomb head damage is 100 only for fighting Merasmus, lower for all other scenarios
	if ( TFGameRules() && TFGameRules()->GetActiveBoss() && ( TFGameRules()->GetActiveBoss()->GetBossType() == HALLOWEEN_BOSS_MERASMUS ) )
	{
		flDamage = 100.0f;
	}

	// explode!
	Vector vecExplosion = EyePosition();

	CPVSFilter filter( vecExplosion );
	TE_TFExplosion( filter, 0.0f, vecExplosion, Vector(0,0,1), NULL, entindex() );
	int iDmgType = DMG_BLAST | DMG_USEDISTANCEMOD;
	CTakeDamageInfo info( this, this, NULL, vecExplosion, vecExplosion, flDamage, iDmgType, TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB, &vecExplosion );

	CBaseEntity *pIgnoreEnt = NULL;
	if ( bExcludeMe )
	{
		pIgnoreEnt = this;
	}

	CTFRadiusDamageInfo radiusinfo( &info, vecExplosion, 100.f, pIgnoreEnt );
	TFGameRules()->RadiusDamage( radiusinfo );

	UTIL_ScreenShake( vecExplosion, 15.0f, 5.0f, 2.f, 750.f, SHAKE_START, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropDeathCallingCard( CTFPlayer* pTFAttacker, CTFPlayer* pTFVictim )
{
	int iCallingCard = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iCallingCard, calling_card_on_kill );
	if ( iCallingCard )
	{
		CEffectData	data;

		data.m_vOrigin = pTFVictim->GetAbsOrigin();
		data.m_vAngles = pTFVictim->GetAbsAngles();
		data.m_nAttachmentIndex = pTFVictim->entindex();	// Victim
		data.m_nHitBox = entindex();						// iShooter
		data.m_fFlags = iCallingCard;						// Index to the Calling card

		DispatchEffect( "TFDeathCallingCard", data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayGesture( const char *pGestureName )
{
	Activity nActivity = (Activity)LookupActivity( pGestureName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pGestureName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlaySpecificSequence( const char *pAnimationName )
{
	Activity nActivity = (Activity)LookupActivity( pAnimationName );
	if ( nActivity != ACT_INVALID )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, nActivity );
		return true;
	}

	int nSequence = LookupSequence( pAnimationName );
	if ( nSequence != -1 )
	{
		DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, nSequence );
		return true;
	} 

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DetonateObjectOfType( int iType, int iMode, bool bIgnoreSapperState )
{
	CBaseObject *pObj = GetObjectOfType( iType, iMode );
	if( !pObj )
		return;

	if( !bIgnoreSapperState && ( pObj->HasSapper() || pObj->IsPlasmaDisabled() ) )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
	if ( event )
	{
		event->SetInt( "userid", GetUserID() ); // user ID of the object owner
		event->SetInt( "objecttype", iType ); // type of object removed
		event->SetInt( "index", pObj->entindex() ); // index of the object removed
		gameeventmanager->FireEvent( event );
	}

	if ( TFGameRules() && TFGameRules()->GetTrainingModeLogic() && IsFakeClient() == false )
	{
		TFGameRules()->GetTrainingModeLogic()->OnPlayerDetonateBuilding( this, pObj );
	}

	SpeakConceptIfAllowed( MP_CONCEPT_DETONATED_OBJECT, pObj->GetResponseRulesModifier() );
	pObj->DetonateObject();

	const CObjectInfo *pInfo = GetObjectInfo( iType );

	if ( pInfo )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"killedobject\" (object \"%s\") (weapon \"%s\") (objectowner \"%s<%i><%s><%s>\") (attacker_position \"%d %d %d\")\n",   
			GetPlayerName(),
			GetUserID(),
			GetNetworkIDString(),
			GetTeam()->GetName(),
			pInfo->m_pObjectName,
			"pda_engineer",
			GetPlayerName(),
			GetUserID(),
			GetNetworkIDString(),
			GetTeam()->GetName(),
			(int)GetAbsOrigin().x, 
			(int)GetAbsOrigin().y,
			(int)GetAbsOrigin().z );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetObjectBuildSpeedMultiplier( int iObjectType, bool bIsRedeploy ) const
{
	float flBuildRate = 1.f; // need a base value for mult

	switch( iObjectType )
	{
	case OBJ_SENTRYGUN:
		CALL_ATTRIB_HOOK_FLOAT( flBuildRate, sentry_build_rate_multiplier );
		flBuildRate += bIsRedeploy ? 2.0 : 0.0f;
		break;

	case OBJ_TELEPORTER:
		CALL_ATTRIB_HOOK_FLOAT( flBuildRate, teleporter_build_rate_multiplier );
		flBuildRate += bIsRedeploy ? 3.0 : 0.0f;
		break;

	case OBJ_DISPENSER:
		CALL_ATTRIB_HOOK_FLOAT( flBuildRate, teleporter_build_rate_multiplier );
		flBuildRate += bIsRedeploy ? 3.0 : 0.0f;
		break;
	}

	return flBuildRate - 1.0f; // sub out the initial 1 so the final result is added
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( m_takedamage != DAMAGE_YES )
		return;

	CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
	if ( pAttacker )
	{
		// Weapons that use uber ammo can transfer that uber into other medics
		if ( pAttacker->IsPlayerClass( TF_CLASS_MEDIC ) && IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			CTFWeaponBase *pWep = pAttacker->GetActiveTFWeapon();
			if ( pWep )
			{
				float flUberTransfer = pWep->UberChargeAmmoPerShot();
				if ( flUberTransfer > 0.0f )
				{
					float flTransferPercent = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWep, flTransferPercent, ubercharge_transfer );

					if ( flTransferPercent )
					{
						flUberTransfer *= ( flTransferPercent * 0.01f );

						CWeaponMedigun *pMedigun = static_cast< CWeaponMedigun * >( Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
						if ( pMedigun )
						{
							pMedigun->AddCharge( flUberTransfer );
						}
					}
				}
			}
		}

		// Prevent team damage here so blood doesn't appear
		if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker, info ) )
		{
			return;
		}
	}

	// Save this bone for the ragdoll.
	m_nForceBone = ptr->physicsbone;

	SetLastHitGroup( ptr->hitgroup );

	// Ignore hitboxes for all weapons except the sniper rifle
	CTakeDamageInfo info_modified = info;
	bool bIsHeadshot = false;

	if ( info_modified.GetDamageType() & DMG_USE_HITLOCATIONS )
	{
		if ( !m_Shared.InCond( TF_COND_INVULNERABLE ) && ptr->hitgroup == HITGROUP_HEAD )
		{
			CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();
			bool bCritical = true;
			bIsHeadshot = true;

			if ( pWpn && !pWpn->CanFireCriticalShot( true, this ) )
			{
				bCritical = false;
			}

			int iBackheadshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetInflictor(), iBackheadshot, back_headshot );
			if ( iBackheadshot )
			{
				// only allow if hit in the back of the head
				Vector entForward;
				AngleVectors( EyeAngles(), &entForward );

				Vector toEnt = GetAbsOrigin() - pAttacker->GetAbsOrigin();
				toEnt.NormalizeInPlace();

				// did not backshot
				//if ( DotProduct( toEnt, entForward ) <= 0.7071f ) // 0.7 os 45 degress from center
				if ( DotProduct( toEnt, entForward ) < 0.5f )	// 60 degrees from center (total of 120)
				{
					bCritical = false;
					bIsHeadshot = false;
				}
			}

			// Check for headshot damage modifiers
			float flHeadshotModifier = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( pAttacker, flHeadshotModifier, headshot_damage_modify);
			info_modified.ScaleDamage(flHeadshotModifier);

			if ( bCritical )
			{
				info_modified.AddDamageType( DMG_CRITICAL );

				int iDecapType = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER ( pAttacker, iDecapType, decapitate_type);
				if ( iDecapType > 0 ) 
				{
					info_modified.SetDamageCustom( TF_DMG_CUSTOM_HEADSHOT_DECAPITATION );
				}
				else 
				{
					info_modified.SetDamageCustom( TF_DMG_CUSTOM_HEADSHOT );
				}

				// play the critical shot sound to the shooter	
				if ( pWpn )
				{
					pWpn->WeaponSound( BURST );
				}
			}
		}
	}

	if ( !bIsHeadshot && pAttacker )
	{	
		// Check for bodyshot damage modifiers
		CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();
		float flBodyshotModifier = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER ( pWpn, flBodyshotModifier, bodyshot_damage_modify);
		info_modified.ScaleDamage( flBodyshotModifier );
	}

	if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		info_modified.SetDamage( info_modified.GetDamage() * tf_damage_multiplier_blue.GetFloat() );
	}
	else if ( GetTeamNumber() == TF_TEAM_RED )
	{
		info_modified.SetDamage( info_modified.GetDamage() * tf_damage_multiplier_red.GetFloat() );
	}

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// no impact effects
	}
	else if ( m_Shared.IsInvulnerable() )
	{ 
		// Make bullet impacts
		g_pEffects->Ricochet( ptr->endpos - (vecDir * 8), -vecDir );
	}
	else
	{	
		// Since this code only runs on the server, make sure it shows the tempents it creates.
		CDisablePredictionFiltering disabler;

		// This does smaller splotches on the guy and splats blood on the world.
		TraceBleed( info_modified.GetDamage(), vecDir, ptr, info_modified.GetDamageType() );
	}

	AddMultiDamage( info_modified, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::TakeHealth( float flHealth, int bitsDamageType )
{
	if ( m_Shared.InCond( TF_COND_NOHEALINGDAMAGEBUFF ) )
	{
		return 0; // No healing while in this state!
	}

	int nResult = 0;

	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		float flHealingBonus = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flHealingBonus, mult_healing_received );
		flHealth *= flHealingBonus;
	}

	// Medigun healing and player/class regen use an accumulator, so they've already factored in debuffs.
// 	if ( m_Shared.InCond( TF_COND_HEALING_DEBUFF ) && !( bitsDamageType & DMG_IGNORE_DEBUFFS ) )
// 	{
// 		flHealth *= ( 1.f - PYRO_AFTERBURN_HEALING_REDUCTION );
// 	}

	// If the bit's set, add over the max health
	if ( bitsDamageType & DMG_IGNORE_MAXHEALTH )
	{
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= ~(bitsDamageType & ~iTimeBasedDamage);
		m_iHealth += flHealth;
		nResult = flHealth;
	}
	else
	{
		float flHealthToAdd = flHealth;
		float flMaxHealth = GetMaxHealth();
		
		// don't want to add more than we're allowed to have
		if ( flHealthToAdd > flMaxHealth - m_iHealth )
		{
			flHealthToAdd = flMaxHealth - m_iHealth;
		}

		if ( flHealthToAdd <= 0 )
		{
			nResult = 0;
		}
		else
		{
			nResult = BaseClass::TakeHealth( flHealthToAdd, bitsDamageType );
		}
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TFWeaponRemove( int iWeaponID )
{
	// find the weapon that matches the id and remove it
	int i;
	for (i = 0; i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWeapon = ( CTFWeaponBase *)GetWeapon( i );
		if ( !pWeapon )
			continue;

		if ( pWeapon->GetWeaponID() != iWeaponID )
			continue;

		RemovePlayerItem( pWeapon );
		UTIL_Remove( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		UTIL_Remove( pWeapon );
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if ( !pWeapon->FVisible( this, MASK_SOLID ) )
		return false;

	// ----------------------------------------
	// If I already have it just take the ammo
	// ----------------------------------------
	if (Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType())) 
	{
		UTIL_Remove( pWeapon );
		return true;
	}
	else 
	{
		// -------------------------
		// Otherwise take the weapon
		// -------------------------
		pWeapon->CheckRespawn();

		pWeapon->AddSolidFlags( FSOLID_NOT_SOLID );
		pWeapon->AddEffects( EF_NODRAW );

		Weapon_Equip( pWeapon );
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::DropCurrentWeapon( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropFlag( bool bSilent /* = false */ )
{
	if ( HasItem() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( GetItem() );
		if ( pFlag )
		{
			int nFlagTeamNumber = pFlag->GetTeamNumber();
			pFlag->Drop( this, true, true, !bSilent );
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( event )
			{
				event->SetInt( "player", entindex() );
				event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
				event->SetInt( "priority", 8 );
				event->SetInt( "team", nFlagTeamNumber );

				gameeventmanager->FireEvent( event );
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: Players can drop Powerup Runes
//-----------------------------------------------------------------------------
void CTFPlayer::DropRune( bool bApplyForce /* = true */, int nTeam /* = TEAM_ANY */ )
{
	if ( m_Shared.IsCarryingRune() )
	{
		Vector forward;
		EyeVectors( &forward );

		RuneTypes_t nRuneType = m_Shared.GetCarryingRuneType();
		// We expect that we are actually are carrying here, so assert that we are.
		Assert( nRuneType >= 0 && nRuneType < RUNE_TYPES_MAX );

		m_Shared.SetCarryingRuneType( RUNE_NONE );

		bool bShouldRemoveMeleeOnly = !( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && m_Shared.InCond( TF_COND_ENERGY_BUFF ) );
		if ( bShouldRemoveMeleeOnly )
		{
			m_Shared.RemoveCond( TF_COND_CANNOT_SWITCH_FROM_MELEE );	// Knockout powerup sets this to on
		}
		TeamFortress_SetSpeed();									// Need to call this or speed bonus isn't removed immediately
		CTFRune::CreateRune( GetAbsOrigin(), nRuneType, nTeam, true, bApplyForce, forward ); // Manually dropped powerups are always neutral
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayer::TeamFortress_GetDisguiseTarget( int nTeam, int nClass )
{
	if ( /*nTeam == GetTeamNumber() ||*/ nTeam == TF_SPY_UNDEFINED )
	{
		// we're not disguised as the enemy team
		return NULL;
	}

	CUtlVector<int> potentialTargets;

	CBaseEntity *pLastTarget = m_Shared.GetDisguiseTarget(); // don't redisguise self as this person
	
	// Find a player on the team the spy is disguised as to pretend to be
	CTFPlayer *pPlayer = NULL;

	// Loop through players and attempt to find a player as the team/class we're disguising as
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && ( pPlayer != pLastTarget ) )
		{
			// First, try to find a player with the same color AND skin
			if ( ( pPlayer->GetTeamNumber() == nTeam ) && ( pPlayer->GetPlayerClass()->GetClassIndex() == nClass ) )
			{
				potentialTargets.AddToHead( i );
			}
		}
	}

	// do we have any potential targets in the list?
	if ( potentialTargets.Count() > 0 )
	{
		int iIndex = random->RandomInt( 0, potentialTargets.Count() - 1 );
		return ToTFPlayer( UTIL_PlayerByIndex( potentialTargets[iIndex] ) );
	}

	// we didn't find someone with the class, so just find someone with the same team color
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && ( pPlayer->GetTeamNumber() == nTeam ) )
		{
			potentialTargets.AddToHead( i );
		}
	}

	if ( potentialTargets.Count() > 0 )
	{
		int iIndex = random->RandomInt( 0, potentialTargets.Count() - 1 );
		return ToTFPlayer( UTIL_PlayerByIndex( potentialTargets[iIndex] ) );
	}

	// we didn't find anyone
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float DamageForce( const Vector &size, float damage, float scale )
{ 
	float force = damage * ((48 * 48 * 82.0) / (size.x * size.y * size.z)) * scale;
	
	if ( force > 1000.0 )
	{
		force = 1000.0;
	}

	return force;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetBlastJumpState( int iState, bool bPlaySound /*= false*/ )
{
	m_iBlastJumpState |= iState;

	const char *pszEvent = NULL;
	if ( iState == TF_PLAYER_STICKY_JUMPED )
	{
		pszEvent = "sticky_jump";
	}
	else if ( iState == TF_PLAYER_ROCKET_JUMPED )
	{
		pszEvent = "rocket_jump";
	}

	if ( pszEvent )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( pszEvent );
		if ( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetBool( "playsound", bPlaySound );
			gameeventmanager->FireEvent( event );
		}
	}

	m_Shared.AddCond( TF_COND_BLASTJUMPING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearBlastJumpState( void )
{
	m_bCreatedRocketJumpParticles = false;
	m_iBlastJumpState = 0;
	m_flBlastJumpLandTime = gpGlobals->curtime;
	m_Shared.RemoveCond( TF_COND_BLASTJUMPING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HandleRageGain( CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale )
{
	if ( !pPlayer )
		return;

	if ( pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) )
	{
		CTFBuffItem *pBuffItem = dynamic_cast<CTFBuffItem*>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_BUFF_ITEM ) );
		unsigned int iBuffId = pBuffItem ? pBuffItem->GetBuffType() : 0;
		if ( iBuffId < ARRAYSIZE( g_RageBuffTypes ) )
		{
			// In Mannpower, the passive 20hp benefit of the Battalion's Backup makes it superior to the Buff Banner, so we reduce the rage build for it to compensate
			if ( TFGameRules() && TFGameRules()->IsPowerupMode() && iBuffId == 2 )
			{
				pPlayer->m_Shared.ModifyRage( g_RageBuffTypes[iBuffId].m_fRageScale * ( ( flDamage / 2.5 ) / fInverseRageGainScale ) );
			}
			else
			{
				if ( g_RageBuffTypes[iBuffId].m_iBuffFlags & iRequiredBuffFlags )
				{
					pPlayer->m_Shared.ModifyRage( g_RageBuffTypes[iBuffId].m_fRageScale * ( flDamage / fInverseRageGainScale ) );
				}
			}
		}
	}
	else if ( pPlayer->IsPlayerClass( TF_CLASS_PYRO ) )
	{
		CTFFlameThrower *pFlameThrower = dynamic_cast<CTFFlameThrower*>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_FLAMETHROWER ) );
		unsigned int iBuffId = pFlameThrower ? pFlameThrower->GetBuffType() : 0;
		if ( iBuffId < ARRAYSIZE( g_RageBuffTypes ) )
		{
			if ( g_RageBuffTypes[iBuffId].m_iBuffFlags & iRequiredBuffFlags )
			{
				if ( TFGameRules() && TFGameRules()->IsPowerupMode() && pPlayer->m_Shared.GetCarryingRuneType() != RUNE_NONE )
				{
					pPlayer->m_Shared.ModifyRage(g_RageBuffTypes[iBuffId].m_fRageScale * ( ( flDamage / 10 ) / fInverseRageGainScale) );
				}
				else
				{
					pPlayer->m_Shared.ModifyRage( g_RageBuffTypes[iBuffId].m_fRageScale * ( flDamage / fInverseRageGainScale ) );
				}
			}
		}
	}

	// General
	int iRage = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iRage, generate_rage_on_dmg );
	if ( iRage )
	{
		if ( pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) && ( kRageBuffFlag_OnDamageDealt & iRequiredBuffFlags ) )
		{
			pPlayer->m_Shared.ModifyRage( flDamage );
		}
		else if ( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && ( kRageBuffFlag_OnDamageDealt & iRequiredBuffFlags ) )
		{
			pPlayer->m_Shared.ModifyRage( 0.22f * ( flDamage / fInverseRageGainScale ) );	
		}
	}

	int iHealRage = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iHealRage, generate_rage_on_heal );		// ...lol
	if ( iHealRage )
	{
		if ( pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && ( kRageBuffFlag_OnHeal & iRequiredBuffFlags ) )
		{
			pPlayer->m_Shared.ModifyRage( 0.25f * flDamage );	
		}
	}

}

// we want to ship this...do not remove
ConVar tf_debug_damage( "tf_debug_damage", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	bool bIsObject = info.GetInflictor() && info.GetInflictor()->IsBaseObject(); 

// need to check this now, before dying
	bool bHadBallBeforeDamage = false;
	if ( TFGameRules() && TFGameRules()->IsPasstimeMode() )
	{
		bHadBallBeforeDamage = m_Shared.HasPasstimeBall();
	}

	// damage may not come from a weapon (ie: Bosses, etc)
	// The existing code below already checked for NULL pWeapon, anyways
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( inputInfo.GetWeapon() );

	if ( GetFlags() & FL_GODMODE )
		return 0;

	if ( IsInCommentaryMode() )
		return 0;

	bool bBuddha = ( m_debugOverlays & OVERLAY_BUDDHA_MODE ) ? true : false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetInt() > 1 && !IsBot() )
		bBuddha = true;
#endif // _DEBUG || STAGING_ONLY

	if ( bBuddha )
	{
		if ( ( m_iHealth - info.GetDamage() ) <= 0 )
		{
			m_iHealth = 1;
			return 0;
		}
	}

	if ( !IsAlive() )
		return 0;

	// Early out if there's no damage
	if ( !info.GetDamage() )
		return 0;

	// Ghosts dont take damage
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return 0;

	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pAttacker = info.GetAttacker();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );

	bool bDebug = tf_debug_damage.GetBool();

	// If attacker has Strength Powerup Rune, apply damage multiplier, but not if you're a building or a crit
	bool bCrit = ( info.GetDamageType() & DMG_CRITICAL ) > 0;
	if ( !bIsObject && pTFAttacker && pTFAttacker->m_Shared.GetCarryingRuneType() == RUNE_STRENGTH && !bCrit )
	{
		if ( pTFAttacker->m_Shared.InCond( TF_COND_POWERUPMODE_DOMINANT ) ) 
		{
			info.ScaleDamage( 1.4f );
		}
		else
		{
			info.ScaleDamage( 2.f );
		}
	}

	// Make sure the player can take damage from the attacking entity
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker, info ) )
	{
		if ( bDebug )
		{
			Warning( "    ABORTED: Player can't take damage from that attacker.\n" );
		}

		return 0;
	}

	if ( IsBot() )
	{
		// Don't let Sentry Busters die until they've done their spin-up
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			CTFBot *bot = ToTFBot( this );
			if ( bot )
			{
				if ( bot->HasMission( CTFBot::MISSION_DESTROY_SENTRIES ) )
				{
					if ( ( m_iHealth - info.GetDamage() ) <= 0 )
					{
						m_iHealth = 1;
						return 0;
					}
				}

				// Sentry Busters hurt teammates when they explode.
				// Force damage value when the victim is a giant.
				if ( pTFAttacker && pTFAttacker->IsBot() )
				{
					CTFBot *pTFAttackerBot = ToTFBot( pTFAttacker );
					if ( pTFAttackerBot && 
						 ( pTFAttackerBot != this ) && 
						 pTFAttackerBot->GetPrevMission() == CTFBot::MISSION_DESTROY_SENTRIES &&
						 info.IsForceFriendlyFire() && 
						 InSameTeam( pTFAttackerBot ) &&
						 IsMiniBoss() )
					{
						info.SetDamage( 600.f );
					}
				}
			}
		}
	}

	// Halloween 2011
	if ( IsInPurgatory() )
	{
		info.SetDamage( m_purgatoryPainMultiplier * info.GetDamage() );
	}

	m_iHealthBefore = GetHealth();

	bool bIsSoldierRocketJumping = ( IsPlayerClass( TF_CLASS_SOLDIER ) && (pAttacker == this) && !(GetFlags() & FL_ONGROUND) && !(GetFlags() & FL_INWATER)) && (inputInfo.GetDamageType() & DMG_BLAST);
	bool bIsDemomanPipeJumping = ( IsPlayerClass( TF_CLASS_DEMOMAN) && (pAttacker == this) && !(GetFlags() & FL_ONGROUND) && !(GetFlags() & FL_INWATER)) && (inputInfo.GetDamageType() & DMG_BLAST);
	
	if ( bDebug )
	{
		Warning( "%s taking damage from %s, via %s. Damage: %.2f\n", GetDebugName(), info.GetInflictor() ? info.GetInflictor()->GetDebugName() : "Unknown Inflictor", pAttacker ? pAttacker->GetDebugName() : "Unknown Attacker", info.GetDamage() );
	}

	if ( pTFAttacker )
	{
		pTFAttacker->SetLastEntityDamagedTime( gpGlobals->curtime );
		pTFAttacker->SetLastEntityDamaged( this );

		CTFWeaponBase *myWeapon = GetActiveTFWeapon();
		CTFWeaponBase *attackerWeapon = pTFAttacker->GetActiveTFWeapon();
		
		if ( myWeapon && attackerWeapon )
		{
			int iStunEnemyWithSameWeapon = 0;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( attackerWeapon, iStunEnemyWithSameWeapon, stun_enemies_wielding_same_weapon );
			if ( iStunEnemyWithSameWeapon )
			{
				CEconItemView *myItem = myWeapon->GetAttributeContainer()->GetItem();
				CEconItemView *attackerItem = attackerWeapon->GetAttributeContainer()->GetItem();

				if ( myItem && attackerItem && myItem->GetItemDefIndex() == attackerItem->GetItemDefIndex() )
				{
					// we're both wielding the same weapon - stun!
					m_Shared.StunPlayer( 1.0f, 1.0, TF_STUN_BOTH | TF_STUN_NO_EFFECTS );
				}
			}
		}
	}

	if ( ( info.GetDamageType() & DMG_FALL ) )
	{
		float flOriginalVelocity = m_Local.m_flFallVelocity;

		bool bHitEnemy = false;

		// Are we transferring falling damage to someone else?
		if ( GetGroundEntity() && GetGroundEntity()->IsPlayer() && m_Shared.CanFallStomp() )
		{
			// Did we land on a guy from the enemy team?
			CTFPlayer *pOther = ToTFPlayer( GetGroundEntity() );
			if ( pOther && pOther->GetTeamNumber() != GetTeamNumber() )
			{
				float flStompDamage = 10.0f + info.GetDamage() * 3.f;

				CTakeDamageInfo infoInner( this, this, GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_SECONDARY ), flStompDamage, DMG_FALL, TF_DMG_CUSTOM_BOOTS_STOMP );
				pOther->TakeDamage( infoInner );
				m_Local.m_flFallVelocity = 0;
				info.SetDamage( 0.0f );
				EmitSound( "Weapon_Mantreads.Impact" );
				EmitSound( "Player.FallDamageDealt" );
				UTIL_ScreenShake( pOther->WorldSpaceCenter(), 15.0, 150.0, 1.0, 500, SHAKE_START );

				bHitEnemy = true;
			}
		}

/*
		// no fall damage in space
		if (  m_Shared.InCond( TF_COND_SPACE_GRAVITY ) )
		{
			info.SetDamage( 0.0f );
		}
*/

		// Apply an impact effect (intensity determined by velocity)
		if ( m_Shared.InCond( TF_COND_ROCKETPACK ) )
		{
			int iImpactPushback = 0;
			CALL_ATTRIB_HOOK_INT( iImpactPushback, falling_impact_radius_pushback );
			if ( iImpactPushback )
			{
				float flPushAmount = RemapValClamped( flOriginalVelocity, 100.f, 1000.f, tf_rocketpack_impact_push_min.GetFloat(), tf_rocketpack_impact_push_max.GetFloat() );
				float flPushRadius = RemapValClamped( flOriginalVelocity, 100.f, 1000.f, 150.f, 220.f );
			
				// Stun, too?
				int iImpactStun = 0;
				CALL_ATTRIB_HOOK_INT( iImpactStun, falling_impact_radius_stun );
				if ( iImpactStun && flOriginalVelocity >= 100.f )
				{
					float flStunTime = RemapValClamped( flOriginalVelocity, 100.f, 1000.f, 1.5f, 3.f );
					m_Shared.ApplyRocketPackStun( ( bHitEnemy ) ? 5.f : flStunTime );
				}
				
				TFGameRules()->PushAllPlayersAway( GetAbsOrigin(), flPushRadius, flPushAmount, GetEnemyTeam( GetTeamNumber() ) );

				m_Local.m_flFallVelocity = 0.f;

				// Extinguish teammates
				CUtlVector< CTFPlayer * > vecPlayers;
				CollectPlayers( &vecPlayers, GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );
				FOR_EACH_VEC( vecPlayers, i )
				{
					CTFPlayer *pPlayer = vecPlayers[i];
					if ( !pPlayer )
						continue;

					if ( !pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
						continue;

					if ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() > ( flPushRadius * flPushRadius ) )
						continue;

					if ( !FVisible( pPlayer, MASK_OPAQUE ) )
						continue;

					pPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
					pPlayer->EmitSound( "TFPlayer.FlameOut" );
					CTF_GameStats.Event_PlayerAwardBonusPoints( this, pPlayer, 10 );
				}
			}

			info.SetDamage( Max( info.GetDamage() * 0.25f, 1.f ) );
		}
	}

	// Ignore damagers on our team, to prevent capturing rocket jumping, etc.
	if ( pAttacker && pAttacker->GetTeam() != GetTeam() )
	{
		m_AchievementData.AddDamagerToHistory( pAttacker );
		if ( pAttacker->IsPlayer() )
		{
			ToTFPlayer( pAttacker )->m_AchievementData.AddTargetToHistory( this );

			// add to list of damagers via sentry so that later we can check for achievement: ACHIEVEMENT_TF_ENGINEER_SHOTGUN_KILL_PREV_SENTRY_TARGET
			CBaseEntity *pInflictor = info.GetInflictor();
			CObjectSentrygun *pSentry = dynamic_cast< CObjectSentrygun * >( pInflictor );
			if ( pSentry )
			{
				m_AchievementData.AddSentryDamager( pAttacker, pInflictor );
			}
		}
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = info.GetDamage();
	m_LastDamageType = info.GetDamageType();

	if ( m_LastDamageType & DMG_FALL )
	{
		if ( ( m_lastDamageAmount > m_iLeftGroundHealth ) && ( m_lastDamageAmount < GetHealth() ) )
		{
			// we gained health in the air, and it saved us from death.
			// if any medics are healing us, they get an achievement
			int iNumHealers = m_Shared.GetNumHealers();
			for ( int i=0;i<iNumHealers;i++ )
			{
				CTFPlayer *pMedic = ToTFPlayer( m_Shared.GetHealerByIndex(i) );

				// if its a medic healing us
				if ( pMedic && pMedic->IsPlayerClass( TF_CLASS_MEDIC ) )
				{
					pMedic->AwardAchievement( ACHIEVEMENT_TF_MEDIC_SAVE_FALLING_TEAMMATE );
				}
			}
		}
	}

	// Check for Demo Achievement:
	// Kill a Heavy from full health with one detonation
	if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER )
			{
				// We're at full health
				if ( m_iHealthBefore >= GetMaxHealth() )
				{
					// Record the time
					m_fMaxHealthTime = gpGlobals->curtime;
				}

				// If we're still being hit in the same time window
				if ( m_fMaxHealthTime == gpGlobals->curtime )
				{
					// Check if the damage is fatal
					int iDamage = info.GetDamage();
					if ( m_iHealth - iDamage <= 0 )
					{
						pTFAttacker->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_HEAVIES_FULLHP_ONEDET );
					}
				}
			}
		}
	}
	
	if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_MEDIC ) && pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_BONESAW )
	{
		CTFBonesaw *pBoneSaw = static_cast< CTFBonesaw* >( pWeapon );
		if ( pBoneSaw->GetBonesawType() == BONESAW_UBER_SAVEDONDEATH )
		{
			// Spawn their spleen
			CPhysicsProp *pRandomInternalOrgan = dynamic_cast< CPhysicsProp* >( CreateEntityByName( "prop_physics_override" ) );
			if ( pRandomInternalOrgan )
			{
				pRandomInternalOrgan->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
				pRandomInternalOrgan->AddFlag( FL_GRENADE );
				char buf[512];
				Q_snprintf( buf, sizeof( buf ), "%.10f %.10f %.10f", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
				pRandomInternalOrgan->KeyValue( "origin", buf );
				Q_snprintf( buf, sizeof( buf ), "%.10f %.10f %.10f", GetAbsAngles().x, GetAbsAngles().y, GetAbsAngles().z );
				pRandomInternalOrgan->KeyValue( "angles", buf );
				pRandomInternalOrgan->KeyValue( "model", "models/player/gibs/random_organ.mdl" );
				pRandomInternalOrgan->KeyValue( "fademindist", "-1" );
				pRandomInternalOrgan->KeyValue( "fademaxdist", "0" );
				pRandomInternalOrgan->KeyValue( "fadescale", "1" );
				pRandomInternalOrgan->KeyValue( "inertiaScale", "1.0" );
				pRandomInternalOrgan->KeyValue( "physdamagescale", "0.1" );
				DispatchSpawn( pRandomInternalOrgan );
				pRandomInternalOrgan->m_takedamage = DAMAGE_YES;	// Take damage, otherwise this can block trains
				pRandomInternalOrgan->SetHealth( 100 );
				pRandomInternalOrgan->Activate();

				Vector vecImpulse = RandomVector( -1.f, 1.f );
				vecImpulse.z = 1.f;
				VectorNormalize( vecImpulse );
				Vector vecVelocity = vecImpulse * 250.0;
				pRandomInternalOrgan->ApplyAbsVelocityImpulse( vecVelocity );

				pRandomInternalOrgan->ThinkSet( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 5.f, "DieContext" );
			}
		}
	}

	if ( bIsSoldierRocketJumping || bIsDemomanPipeJumping )
	{
		int nJumpType = 0;

		// If this is our own rocket, scale down the damage if we're rocket jumping
		if ( bIsSoldierRocketJumping ) 
		{
			float flDamage = info.GetDamage() * tf_damagescale_self_soldier.GetFloat();
			info.SetDamage( flDamage );

			if ( m_iHealthBefore - flDamage > 0 )
			{
				nJumpType = TF_PLAYER_ROCKET_JUMPED;
			}
		}
		else if ( bIsDemomanPipeJumping )
		{
			nJumpType = TF_PLAYER_STICKY_JUMPED;
		}

		if ( nJumpType )
		{
			bool bPlaySound = false;
			if ( pWeapon )
			{
				int iNoBlastDamage = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iNoBlastDamage, no_self_blast_dmg )
					bPlaySound = iNoBlastDamage ? true : false;
			}

			SetBlastJumpState( nJumpType, bPlaySound );
		}
	}

	if ( TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		// can only bounce invaders when they are on the ground
		if ( GetGroundEntity() == NULL )
		{
			info.SetDamageForce( vec3_origin );
		}
	}

	// Save damage force for ragdolls.
	m_vecTotalBulletForce = info.GetDamageForce();
	m_vecTotalBulletForce.x = clamp( m_vecTotalBulletForce.x, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.y = clamp( m_vecTotalBulletForce.y, -15000.0f, 15000.0f );
	m_vecTotalBulletForce.z = clamp( m_vecTotalBulletForce.z, -15000.0f, 15000.0f );

	int bTookDamage = 0;
 	int bitsDamage = inputInfo.GetDamageType();

	bool bAllowDamage = false;

	CPointHurt *pPointHurt = dynamic_cast<CPointHurt *>( pInflictor );
	if ( pPointHurt )
	{
		// check to see if our attacker is a point_hurt entity (and allow it to kill us even if we're invuln with the flag)
		if ( pPointHurt->HasSpawnFlags( SF_PHURT_HURT_UBER ) )
		{
			bAllowDamage = true;
			info.SetDamageCustom( TF_DMG_CUSTOM_TRIGGER_HURT );
		}
	}
	else if ( pInflictor && pInflictor->IsSolidFlagSet( FSOLID_TRIGGER ) )
	{
		// check to see if our attacker is a trigger_hurt entity (and allow it to kill us even if we're invuln)
		CTriggerHurt *pTrigger = dynamic_cast<CTriggerHurt *>( pInflictor );
		if ( pTrigger )
		{
			bAllowDamage = true;
			info.SetDamageCustom( TF_DMG_CUSTOM_TRIGGER_HURT );
		}
		else
		{
			CFuncCroc *pFuncCroc = dynamic_cast< CFuncCroc * >( pInflictor );
			if ( pFuncCroc )
			{
				bAllowDamage = true;
				info.SetDamageCustom( TF_DMG_CUSTOM_CROC );
			}
		}
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TELEFRAG )
	{
		bAllowDamage = true;
	}

	if ( !TFGameRules()->ApplyOnDamageModifyRules( info, this, bAllowDamage ) )
	{
		return 0;
	}

	// If player has Reflect Powerup, reflect damage to attacker. 
	// We do this here, after damage modify rules to ensure distance falloff calculations have already been made before we pass that damage back to the attacker
	if ( pTFAttacker && m_Shared.GetCarryingRuneType() == RUNE_REFLECT && pTFAttacker != this && !pTFAttacker->m_Shared.IsInvulnerable() && pTFAttacker->IsAlive() )
	{
		CTakeDamageInfo dmg = info;
		CTFProjectile_SentryRocket *sentryRocket = dynamic_cast<CTFProjectile_SentryRocket *>( info.GetInflictor() );

		if ( gpGlobals->curtime > m_flNextReflectZap ) // don't spam the effect for fast weapons like flamethrower and minigun
		{
			m_flNextReflectZap = gpGlobals->curtime + 0.5f;

			CPVSFilter filter( WorldSpaceCenter() );
			Vector vEnd = pTFAttacker->WorldSpaceCenter();
			Vector vStart = WorldSpaceCenter();

			if ( bIsObject || sentryRocket )
			{
				CBaseEntity *pInflictor = info.GetInflictor();
				vEnd = pInflictor->WorldSpaceCenter();
			}
			else
			{
				// Push the attacker away from the Reflect powerup holder
				Vector toPlayer = vEnd - vStart;
				toPlayer.z = 0.0f;
				toPlayer.NormalizeInPlace();
				toPlayer.z = 1.0f;
				float flDamage = dmg.GetDamage();
				if ( dmg.GetDamageCustom() != TF_DMG_CUSTOM_BURNING )
				{
					float flPushForce = RemapValClamped( flDamage, 0.1f, 150.f, 300.f, 500.f );		// Scale the push force according to damage
					Vector vPush = flPushForce * toPlayer;
					pTFAttacker->ApplyAbsVelocityImpulse( vPush );
				}

				// Play a sound and reduce the volume if damage is low
				CSoundParameters params;
				if ( CBaseEntity::GetParametersForSound( "Powerup.Reflect.Reflect", params, NULL ) )
				{
					CPASAttenuationFilter soundFilter( pTFAttacker->GetAbsOrigin(), params.soundlevel );
					EmitSound_t ep( params );

					if ( flDamage < 10.f )
					{
						ep.m_flVolume *= 0.75f;
					}

					pTFAttacker->EmitSound( soundFilter, entindex(), ep );
					pTFAttacker->PainSound( dmg );
				}
			}

			te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
			TE_TFParticleEffectComplex( filter, 0.f, "dxhr_arm_muzzleflash", vStart, QAngle( 0.f, 0.f, 0.f ), NULL, &controlPoint, pTFAttacker, PATTACH_CUSTOMORIGIN );
		}

		dmg.SetDamageCustom( TF_DMG_CUSTOM_RUNE_REFLECT );
		dmg.SetDamageType( DMG_SHOCK );
		dmg.SetAttacker( this );

		if ( bIsObject )
		{
			CBaseEntity *pInflictor = info.GetInflictor();
			dmg.SetDamage( info.GetDamage() );
			pInflictor->TakeDamage( dmg );
		}
		// Sentry rockets are not included in bIsobject so we deal with them separately
		else
		{
			if ( sentryRocket )
			{
				dmg.SetDamage( info.GetDamage() );
				info.GetInflictor()->GetOwnerEntity()->TakeDamage( dmg );
			}
			else
			{
				// Take damage unless you have Resist or Vampire (they are immune to reflect damage)
				if ( pTFAttacker->m_Shared.GetCarryingRuneType() != RUNE_RESIST && pTFAttacker->m_Shared.GetCarryingRuneType() != RUNE_VAMPIRE )
				{
					dmg.SetDamage( info.GetDamage() * ( m_bIsInMannpowerDominantCondition ? 0.5f : 0.8f ) );
					pTFAttacker->TakeDamage( dmg );
				}
			}
		}
	}

	//Don't take damage while I'm phasing.
	if ( ( m_Shared.InCond( TF_COND_PHASE ) || m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) ) && bAllowDamage == false )
	{
		SpeakConceptIfAllowed( MP_CONCEPT_DODGE_SHOT );

		if ( pAttacker && pAttacker->IsPlayer() )
		{
			CEffectData	data;
			data.m_nHitBox = GetParticleSystemIndex( "miss_text" );
			data.m_vOrigin = WorldSpaceCenter() + Vector(0,0,32);
			data.m_vAngles = vec3_angle;
			data.m_nEntIndex = 0;

			CSingleUserRecipientFilter filter( (CBasePlayer*)pAttacker );
			te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );
		}

		Vector vecDir = vec3_origin;
		if ( info.GetInflictor() )
		{
			vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0.0f, 0.0f, 10.0f ) - WorldSpaceCenter();
			VectorNormalize( vecDir );
		}

		ApplyPushFromDamage( info, vecDir );

		if ( m_Shared.InCond( TF_COND_PHASE ) )
		{
			m_Shared.m_ConditionData[ TF_COND_PHASE ].m_nPreventedDamageFromCondition += info.GetDamage();
			m_Shared.m_iPhaseDamage += info.GetDamage();
		}

		bTookDamage = false;
	}
	else
	{
		bool bFatal = ( m_iHealth - info.GetDamage() ) <= 0;
		bool bIsBot = 
#ifdef _DEBUG
		false;
#else
		( pTFAttacker && pTFAttacker->IsBot() ) || IsBot();
#endif
		bool bTrackEvent = pTFAttacker && pTFAttacker != this && !bIsBot;
		if ( bTrackEvent )
		{
			float flHealthRemoved = bFatal ? m_iHealth : info.GetDamage();
			if ( info.GetDamageBonus() && info.GetDamageBonusProvider() )
			{
				// Don't deal with raw damage numbers, only health removed.
				// Example based on a crit rocket to a player with 120 hp:
				// Actual damage is 120, but potential damage is 300, where
				// 100 is the base, and 200 is the bonus.  Apply this ratio
				// to actual (so, attacker did 40, and provider added 80).
				float flBonusMult = info.GetDamage() / abs( info.GetDamageBonus() - info.GetDamage() );
				float flBonus = flHealthRemoved - ( flHealthRemoved / flBonusMult );
				m_AchievementData.AddDamageEventToHistory( info.GetDamageBonusProvider(), flBonus );
				flHealthRemoved -= flBonus;
			}
			m_AchievementData.AddDamageEventToHistory( pAttacker, flHealthRemoved );
		}

		// This should kill us
		if ( bFatal )
		{

			// Damage could have been modified since we started
			// Try to prevent death with buddha one more time
			if ( bBuddha )
			{
				m_iHealth = 1;
				return 0;
			}

			// Check to see if we have the cheat death attribute that makes
			// us teleport to base rather than die
			float flCheatDeathChance = 0.f;
			CALL_ATTRIB_HOOK_FLOAT( flCheatDeathChance, teleport_instead_of_die );
			if( RandomFloat() < flCheatDeathChance )
			{
				// Send back to base
				ForceRespawn();

				m_iHealth = 1;
				return 0;
			}

			// Avoid one death
			if ( m_Shared.InCond( TF_COND_PREVENT_DEATH ) )
			{
				m_Shared.RemoveCond( TF_COND_PREVENT_DEATH );
				m_iHealth = 1;
				return 0;
			}

			// Powerup-sourced reflected damage should not kill player
			if ( info.GetDamageCustom() == TF_DMG_CUSTOM_RUNE_REFLECT )
			{
				m_iHealth = 1;
				return 0;
			}
		}

		// NOTE: Deliberately skip base player OnTakeDamage, because we don't want all the stuff it does re: suit voice
		bTookDamage = CBaseCombatCharacter::OnTakeDamage( info );

		// Early out if the base class took no damage
		if ( !bTookDamage )
		{
			if ( bDebug )
			{
				Warning( "    ABORTED: Player failed to take the damage.\n" );
			}
			return 0;
		}

		// Check to see if we need to pass along the damage to other players
		if ( pWeapon && ( gs_pRecursivePlayerCheck == NULL ) )
		{
			int iDamageAllConnected = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iDamageAllConnected, damage_all_connected );

			if ( iDamageAllConnected > 0 )
			{
				// Am I healing someone or being healed?
				CUtlVector<CTFPlayer*> pTempPlayerQueue;
				AddConnectedPlayers( pTempPlayerQueue, this );

				gs_pRecursivePlayerCheck = this;
				for ( int iCount = 0 ; iCount < pTempPlayerQueue.Count() ; iCount++ )
				{
					CTFPlayer *pTFPlayer = pTempPlayerQueue[iCount];
					if ( pTFPlayer && ( pTFPlayer != this ) )
					{
						pTFPlayer->TakeDamage( inputInfo );
					}
				}
				gs_pRecursivePlayerCheck = NULL;
			}
		}
	}

	if ( bTookDamage == false )
		return 0;

	if ( bDebug )
	{
		Warning( "    DEALT: Player took %.2f damage.\n", info.GetDamage() );
		Warning( "    HEALTH LEFT: %d\n", GetHealth() );
	}

	// Some weapons have the ability to impart extra moment just because they feel like it. Let their attributes
	// do so if they're in the mood.
	if ( pWeapon != NULL )
	{
		float flZScale = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flZScale, apply_z_velocity_on_damage );
		if ( flZScale != 0.0f )
		{
			ApplyAbsVelocityImpulse( Vector( 0.0f, 0.0f, flZScale ) );
		}

		float flDirScale = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flDirScale, apply_look_velocity_on_damage );
		if ( flDirScale != 0.0f && pAttacker != NULL )
		{
			Vector vecForward;
			AngleVectors( pAttacker->EyeAngles(), &vecForward );

			Vector vecForwardNoDownward = Vector( vecForward.x, vecForward.y, MIN( 0.0f, vecForward.z ) ).Normalized();
			ApplyAbsVelocityImpulse( vecForwardNoDownward * flDirScale );
		}
	}

	// let weapons react to their owner being injured
	CTFWeaponBase *pMyWeapon = GetActiveTFWeapon();
	if ( pMyWeapon )
	{
		pMyWeapon->ApplyOnInjuredAttributes( this, pTFAttacker, info );
	}

	// Send the damage message to the client for the hud damage indicator
	// Try and figure out where the damage is coming from
	Vector vecDamageOrigin = info.GetReportedPosition();

	// If we didn't get an origin to use, try using the attacker's origin
	if ( vecDamageOrigin == vec3_origin && info.GetInflictor() )
	{
		vecDamageOrigin = info.GetInflictor()->GetAbsOrigin();
	}

	// Tell the player's client that he's been hurt.
	if ( m_iHealthBefore != GetHealth() )
	{
 		CSingleUserRecipientFilter user( this );
		UserMessageBegin( user, "Damage" );
			WRITE_SHORT( clamp( (int)info.GetDamage(), 0, 32000 ) );
			WRITE_LONG( info.GetDamageType() );
			// Tell the client whether they should show it in the indicator
			if ( bitsDamage != DMG_GENERIC && !(bitsDamage & (DMG_DROWN | DMG_FALL | DMG_BURN) ) )
			{
				WRITE_BOOL( true );
				WRITE_VEC3COORD( vecDamageOrigin );
			}
			else
			{
				WRITE_BOOL( false );
			}
		MessageEnd();
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if ( info.GetInflictor() && info.GetInflictor()->edict() )
	{
		m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();
	}

	m_DmgTake += (int)info.GetDamage();

	// Reset damage time countdown for each type of time based damage player just sustained
	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		// Make sure the damage type is really time-based.
		// This is kind of hacky but necessary until we setup DamageType as an enum.
		int iDamage = ( DMG_PARALYZE << i );
		if ( ( info.GetDamageType() & iDamage ) && g_pGameRules->Damage_IsTimeBased( iDamage ) )
		{
			m_rgbTimeBasedDamage[i] = 0;
		}
	}

	const char* pzsMedigunResistEffect = NULL;
	const char* pzsTeam = GetTeamNumber() == TF_TEAM_RED ? "red" : "blue";

	// If we have one of the medigun resist buffs and get hit with the matching damage type then
	// spawn a particle above our head to let enemies know their damage is being resisted, and tell
	// the medic he's doing the right thing.

	bool bMedicBulletResist		= m_Shared.InCond( TF_COND_MEDIGUN_UBER_BULLET_RESIST ) || m_Shared.InCond( TF_COND_MEDIGUN_SMALL_BULLET_RESIST );
	bool bMedicExplosiveResist	= m_Shared.InCond( TF_COND_MEDIGUN_UBER_BLAST_RESIST )	|| m_Shared.InCond( TF_COND_MEDIGUN_SMALL_BLAST_RESIST );
	bool bMedicFireResist		= m_Shared.InCond( TF_COND_MEDIGUN_UBER_FIRE_RESIST )	|| m_Shared.InCond( TF_COND_MEDIGUN_SMALL_FIRE_RESIST );

	if( ( bMedicBulletResist && ( bitsDamage & DMG_BULLET	) ) )
	{
		pzsMedigunResistEffect = CFmtStr( "vaccinator_%s_buff1_burst", pzsTeam );
	}
	else if( bMedicExplosiveResist	&& ( bitsDamage & DMG_BLAST	) )
	{
		pzsMedigunResistEffect = CFmtStr( "vaccinator_%s_buff2_burst", pzsTeam );
	}
	else if( bMedicFireResist && ( bitsDamage & DMG_BURN ) )
	{
		pzsMedigunResistEffect = CFmtStr( "vaccinator_%s_buff3_burst", pzsTeam );
	}

	if( pzsMedigunResistEffect != NULL )
	{
		const Vector& vecOrigin = GetAbsOrigin();
		CPVSFilter filter( vecOrigin );
		TE_TFParticleEffect( filter, 0, pzsMedigunResistEffect, vecOrigin, vec3_angle );
	}

	// Display any effect associate with this damage type
	DamageEffect( info.GetDamage(),bitsDamage );

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get reset

	// Flinch
	bool bFlinch = true;
	if ( bitsDamage != DMG_GENERIC )
	{
		if ( IsPlayerClass( TF_CLASS_SNIPER ) && m_Shared.InCond( TF_COND_AIMING ) )
		{
			if ( pTFAttacker && pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
			{
				float flDistSqr = ( pTFAttacker->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
				if ( flDistSqr > 750 * 750 )
				{
					bFlinch = false;
				}
			}
		}

		if ( bFlinch )
		{
			if ( ApplyPunchImpulseX( -2 ) ) 
			{
				PlayFlinch( info );
			}
		}

		// PASSTIME intense flinch to make it hard to throw straight while taking damage
		extern ConVar tf_passtime_flinch_boost;
		if( TFGameRules() && TFGameRules()->IsPasstimeMode() && (tf_passtime_flinch_boost.GetInt() > 0) )
		{
			int iFlinch = tf_passtime_flinch_boost.GetInt();
			CTFWeaponBase *pMyWeapon = GetActiveTFWeapon();
			if( pMyWeapon && pMyWeapon->GetWeaponID() == TF_WEAPON_PASSTIME_GUN )
			{
				QAngle punch;
				punch.Random( -iFlinch, iFlinch );
				SetPunchAngle( punch );
			}
		}
	}

	// Do special explosion damage effect
	if ( bitsDamage & DMG_BLAST )
	{
		OnDamagedByExplosion( info );
	}

	if ( m_iHealthBefore != GetHealth() )
	{
		PainSound( info );
	}

	// Detect drops below 25% health and restart expression, so that characters look worried.
	int iHealthBoundary = (GetMaxHealth() * 0.25);
	if ( GetHealth() <= iHealthBoundary && m_iHealthBefore > iHealthBoundary )
	{
		ClearExpression();
	}

#ifdef _DEBUG
	// Report damage from the info in debug so damage against targetdummies goes
	// through the system, as m_iHealthBefore - GetHealth() will always be 0.
	CTF_GameStats.Event_PlayerDamage( this, info, info.GetDamage() );
#else
	CTF_GameStats.Event_PlayerDamage( this, info, m_iHealthBefore - GetHealth() );
#endif // _DEBUG

	// if we take damage after we leave the ground, update the health if its less
	if ( bTookDamage && m_iLeftGroundHealth > 0 )
	{
		if ( GetHealth() < m_iLeftGroundHealth )
		{
			m_iLeftGroundHealth = GetHealth();
		}
	}
	
	if ( IsPlayerClass( TF_CLASS_SPY ) && ( inputInfo.GetDamageCustom() != TF_DMG_CUSTOM_TELEFRAG ) && ( inputInfo.GetDamageCustom() != TF_DMG_CUSTOM_CROC ) )
	{
		// Trigger feign death if the player has it prepped...
		if ( m_Shared.IsFeignDeathReady() )
		{
			m_Shared.SetFeignDeathReady( false );
			if ( !m_Shared.InCond( TF_COND_TAUNTING ) )
			{
				SpyDeadRingerDeath( info );

				if ( pTFAttacker )
				{
					pTFAttacker->IncrementKillCountSinceLastDeploy( info );
				}
			}
		}
		else if ( !( info.GetDamageType() & DMG_FALL ) )
		{
			m_Shared.NoteLastDamageTime( m_lastDamageAmount );
		}
	}

	if ( pWeapon ) 
	{
		pWeapon->ApplyPostHitEffects( inputInfo, this );
	}

	if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		// Reduce charge if damage is taken
		int iDemoChargeDamagePenalty = 0;
		CALL_ATTRIB_HOOK_INT( iDemoChargeDamagePenalty, lose_demo_charge_on_damage_when_charging );
		// Does not apply to self or fall damage
		if ( iDemoChargeDamagePenalty && m_Shared.InCond( TF_COND_SHIELD_CHARGE ) && !( info.GetDamageType() & DMG_FALL ) && (pAttacker != this) )
		{
			iDemoChargeDamagePenalty *= info.GetDamage();
			m_Shared.SetDemomanChargeMeter( Max( m_Shared.GetDemomanChargeMeter() - (float)iDemoChargeDamagePenalty, 0.0f ) );
		}
	}


	float flRageScale = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flRageScale, rage_giving_scale );

	// Give the soldier/pyro some rage points for dealing/taking damage.
	if ( bTookDamage && pTFAttacker != this )
	{
		// Buff flag 1: we get rage when we deal damage. Here, that means the soldier that attacked
		// gets rage when we take damage.
		HandleRageGain( pTFAttacker, kRageBuffFlag_OnDamageDealt, info.GetDamage() * flRageScale, 6.0f );

		// Buff flag 2: we get rage when we take damage.
		if (  !( info.GetDamageType() & DMG_FALL ) )
		{
			HandleRageGain( this, kRageBuffFlag_OnDamageReceived, info.GetDamage() * flRageScale, 3.5f );
		}

		// Buff 5: our pyro attacker get rage when we're damaged by fire
		if ( ( info.GetDamageType() & DMG_BURN ) != 0 || ( info.GetDamageType() & DMG_PLASMA ) != 0 )
		{
			float flInverseRageGainScale = TFGameRules()->IsMannVsMachineMode() ? 12.f : 3.f;
			HandleRageGain( pTFAttacker, kRageBuffFlag_OnBurnDamageDealt, info.GetDamage() * flRageScale, flInverseRageGainScale );
		}
	}

	if ( pWeapon && ( ( pWeapon->GetWeaponID() == TF_WEAPON_BAT_FISH ) || ( pWeapon->GetWeaponID() == TF_WEAPON_SLAP ) ) )
	{
		bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED ) && pTFAttacker && ( m_Shared.GetDisguiseTeam() == pTFAttacker->GetTeamNumber() );
		bool bFish = ( pWeapon->GetWeaponID() == TF_WEAPON_BAT_FISH );

		if ( m_iHealth <= 0 )
		{
			info.SetDamageCustom( bFish ? TF_DMG_CUSTOM_FISH_KILL : TF_DMG_CUSTOM_SLAP_KILL );
		}

		if ( m_iHealth <= 0 || !bDisguised )
		{
			// Do you ever find yourself typing "fish damage override" into a million-lines-of-code project and
			// wondering about the world? Because I do.
			int iFishDamageOverride = 0;
			if ( bFish )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iFishDamageOverride, fish_damage_override );
			}

			TFGameRules()->DeathNotice( this, info, bFish ? ( iFishDamageOverride ? "fish_notice__arm" : "fish_notice" ) : "slap_notice" );
		}
	}

	if ( IsPlayerClass( TF_CLASS_SCOUT) )
	{
		// Lose hype on take damage
		int iHypeResetsOnTakeDamage = 0;
		CALL_ATTRIB_HOOK_INT( iHypeResetsOnTakeDamage, lose_hype_on_take_damage );
		if ( iHypeResetsOnTakeDamage != 0 )
		{
			// Loose x hype on jump
			float flHype = m_Shared.GetScoutHypeMeter();
			m_Shared.SetScoutHypeMeter( flHype - iHypeResetsOnTakeDamage * info.GetDamage() );
			TeamFortress_SetSpeed();
		}
	}

	// Add humilation Obituary here for throwable hits
	//if ( info.GetDamageCustom() == TF_DMG_CUSTOM_THROWABLE )
	//{
	//	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED ) && (m_Shared.GetDisguiseTeam() == pTFAttacker->GetTeamNumber());

	//	if( m_iHealth <= 0 )
	//	{
	//		info.SetDamageCustom( TF_DMG_CUSTOM_THROWABLE_KILL );
	//	}

	//	if ( m_iHealth <= 0 || !bDisguised )
	//	{
	//		TFGameRules()->DeathNotice( this, info, "throwable_hit" );
	//	}
	//}

	// Let attacker react to the damage they dealt
	if ( pTFAttacker )
	{
		pTFAttacker->OnDealtDamage( this, info );
	}

	bool bIsPyroDetonateJumping = ( IsPlayerClass( TF_CLASS_PYRO ) && pAttacker == this && !(GetFlags() & FL_ONGROUND) && !(GetFlags() & FL_INWATER));
	if ( bIsDemomanPipeJumping || bIsSoldierRocketJumping || bIsPyroDetonateJumping )
	{
		// Are we being healed by any QuickFix medics?
		for ( int i = 0; i < m_Shared.m_nNumHealers; i++ )
		{
			CTFPlayer *pMedic = ToTFPlayer( m_Shared.m_aHealers[i].pHealer );
			if ( !pMedic )
				continue;

			// Share blast jump with them
			CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pMedic->GetActiveTFWeapon() );
			if ( pMedigun && pMedigun->GetMedigunType() == MEDIGUN_QUICKFIX )
			{
// 				Vector vecDir = vec3_origin;
// 				if ( info.GetInflictor() )
// 				{
// 					vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0.0f, 0.0f, 10.0f ) - WorldSpaceCenter();
// 					info.GetInflictor()->AdjustDamageDirection( info, vecDir, this );
// 					VectorNormalize( vecDir );
// 				}
// 				pMedic->RemoveFlag( FL_ONGROUND );
// 				pMedic->ApplyPushFromDamage( info, vecDir );

				float flForce = GetAbsVelocity().Length();
				flForce = MIN( flForce, 900.f );
				Vector vecNewVelocity = GetAbsVelocity();
				VectorNormalize( vecNewVelocity );
				pMedic->RemoveFlag( FL_ONGROUND );
				pMedic->ApplyAbsVelocityImpulse( vecNewVelocity * flForce );
			}
		}
	}

	if ( pTFAttacker && pTFAttacker->IsPlayerClass( TF_CLASS_SOLDIER ) )
	{
		if ( info.GetDamageType() & DMG_BLAST )
		{
			// Send an event whenever a soldier hits another player directly with a stun rocket
			CTFBaseRocket *pRocket = dynamic_cast< CTFBaseRocket* >( info.GetInflictor() );
			if ( pRocket && pRocket->GetStunLevel() && pRocket->GetEnemy() && pRocket->GetEnemy() == this )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_directhit_stun" );
				if ( event )
				{
					event->SetInt( "attacker", pTFAttacker->entindex() );
					event->SetInt( "victim", entindex() );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}


	CTFWeaponBase *pTFWeapon = GetKilleaterWeaponFromDamageInfo( &info );
	if ( !pTFWeapon )
	{
		// Check Wearable instead like demoshields or manntreads
		CTFWearable *pWearable = dynamic_cast< CTFWearable* >( info.GetWeapon() );
		if ( pWearable )
		{
			EconEntity_OnOwnerKillEaterEvent_Batched( pWearable, pTFAttacker, this, kKillEaterEvent_DamageDealt, info.GetDamage() );
			EconEntity_OnOwnerKillEaterEvent_Batched( pWearable, pTFAttacker, this, kKillEaterEvent_PlayersHit, 1 );
		}
	}
	else
	{
		EconEntity_OnOwnerKillEaterEvent_Batched( pTFWeapon, pTFAttacker, this, kKillEaterEvent_DamageDealt, info.GetDamage() );
		EconEntity_OnOwnerKillEaterEvent_Batched( pTFWeapon, pTFAttacker, this, kKillEaterEvent_PlayersHit, 1 );
	}

	if ( bTookDamage && m_Shared.InCond( TF_COND_GAS ) )
	{
		CTFPlayer *pTFGasTosser = dynamic_cast< CTFPlayer* >( m_Shared.GetConditionProvider( TF_COND_GAS ) );
		if ( pTFGasTosser )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "gas_doused_player_ignited" );
			if ( event )
			{
				event->SetInt( "igniter", pAttacker ? pAttacker->entindex() : 0 );
				event->SetInt( "douser", pTFGasTosser->entindex() );
				event->SetInt( "victim", entindex() );
				gameeventmanager->FireEvent( event );
			}
		}

		if ( IsPlayerClass( TF_CLASS_PYRO ) )
		{
			m_Shared.AddCond( TF_COND_BURNING_PYRO, tf_afterburn_max_duration );
		}

		CTFWeaponBase *pGasCan = nullptr;
		if ( pTFGasTosser )
		{
			pGasCan = dynamic_cast<CTFWeaponBase*>( pTFGasTosser->GetEntityForLoadoutSlot( LOADOUT_POSITION_SECONDARY ) );
		}

		m_Shared.Burn( pTFGasTosser ? pTFGasTosser : this, ( pGasCan && pGasCan->GetWeaponID() == TF_WEAPON_JAR_GAS ) ? pGasCan : NULL, tf_afterburn_max_duration );
		m_Shared.RemoveCond( TF_COND_GAS );

		// Explode?
		if ( pTFGasTosser && pGasCan )
		{
			int iExplodeOnIgnite = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pGasCan, iExplodeOnIgnite, explode_on_ignite );
			if ( iExplodeOnIgnite )
			{
				bool bExploded = false;
				float flRadius = 200.f;

				CBaseEntity	*pObjects[32];
				int nCount = UTIL_EntitiesInSphere( pObjects, ARRAYSIZE( pObjects ), GetAbsOrigin(), flRadius, FL_CLIENT );
				for ( int i = 0; i < nCount; i++ )
				{
					if ( !pObjects[i] )
						continue;

					if ( !pObjects[i]->IsAlive() )
						continue;

// 						if ( pObjects[i] == this )
// 							continue;

					if ( pAttacker->InSameTeam( pObjects[i] ) )
						continue;

					CTFPlayer *pTFBlastVictim = ToTFPlayer( pObjects[i] );

					if ( pTFBlastVictim->m_Shared.InCond( TF_COND_PHASE ) || pTFBlastVictim->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
						continue;

					if ( pTFBlastVictim->m_Shared.IsInvulnerable() )
						continue;

					if ( pTFBlastVictim->m_Shared.InCond( TF_COND_BLEEDING ) )
					{
						if ( pTFBlastVictim->m_Shared.GetConditionProvider( TF_COND_BLEEDING ) == pTFGasTosser )
							continue;
					}
							
					if ( !FVisible( pTFBlastVictim, MASK_OPAQUE ) )
						continue;

					pTFBlastVictim->m_Shared.MakeBleed( pTFGasTosser, pGasCan, 0.1f, 350.f, false, TF_DMG_CUSTOM_BURNING );
					DispatchParticleEffect( "dragons_fury_effect", pTFBlastVictim->GetAbsOrigin(), vec3_angle );
					bExploded = true;
				}

				if ( bExploded )
				{
					EmitSound( "Weapon_Grenade_Pipebomb.Explode" );
				}
			}
		}
	}

	// bHadBallBeforeDamage will always be false in non-passtime modes
	if ( bTookDamage && bHadBallBeforeDamage )
	{
		g_pPasstimeLogic->OnBallCarrierDamaged( this, pTFAttacker, info );
	}

	return info.GetDamage();
}



//-----------------------------------------------------------------------------
// Purpose: Invoked when we deal damage to another victim
//-----------------------------------------------------------------------------
void CTFPlayer::OnDealtDamage( CBaseCombatCharacter *pVictim, const CTakeDamageInfo &info )
{
	float flDamage = info.GetDamage();

	if ( pVictim )
	{
		// which second of the window are we in
		int i = (int)gpGlobals->curtime;
		i %= DPS_Period;

		if ( i != m_lastDamageRateIndex )
		{
			// a second has ticked over, start a new accumulation
			m_damageRateArray[ i ] = flDamage;
			m_lastDamageRateIndex = i;

			// track peak DPS for this player
			m_peakDamagePerSecond = 0;
			for( i=0; i<DPS_Period; ++i )
			{
				if ( m_damageRateArray[i] > m_peakDamagePerSecond )
				{
					m_peakDamagePerSecond = m_damageRateArray[i];
				}
			}
		}
		else
		{
			m_damageRateArray[ i ] += flDamage;
		}
	}

	// Some item charge meters fill up on damage
	for( int i= FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; i <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++i )
	{
		loadout_positions_t eLoadoutPosition( (loadout_positions_t)i );
		CBaseEntity* pItem = GetEntityForLoadoutSlot( eLoadoutPosition, true );
		if ( !pItem )
			continue;

		attrib_value_t chargetype = ATTRIBUTE_METER_TYPE_NONE;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pItem, chargetype, item_meter_charge_type );

		// Only do this if we charge on damage
		if ( chargetype != ATTRIBUTE_METER_TYPE_DAMAGE && chargetype != ATTRIBUTE_METER_TYPE_COMBO )
			continue;

		// Don't allow "explode on ignite" damage to refill the gas can's meter
		if ( IsPlayerClass( TF_CLASS_PYRO ) && eLoadoutPosition == LOADOUT_POSITION_SECONDARY && TFGameRules()->GameModeUsesUpgrades() )
		{
			CTFWeaponBase *pTFWeapon = dynamic_cast< CTFWeaponBase* >( info.GetWeapon() );
			if ( pTFWeapon && pTFWeapon->GetWeaponID() == TF_WEAPON_JAR_GAS && info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
				continue;
		}

		IHasGenericMeter *pGenericMeterUser = dynamic_cast< IHasGenericMeter* >( GetEntityForLoadoutSlot( eLoadoutPosition, true ) );
		if ( !pGenericMeterUser || !pGenericMeterUser->ShouldUpdateMeter() )
			continue;

		float flDamageToFullAttr = 0.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pItem, flDamageToFullAttr, item_meter_damage_for_full_charge );
		if ( flDamageToFullAttr <= 0.f )
			continue;

		// Modify it?
		float flMeterChargeRateMod = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pItem, flMeterChargeRateMod, mult_item_meter_charge_rate );
		flDamageToFullAttr *= flMeterChargeRateMod;
		
		m_Shared.SetItemChargeMeter( eLoadoutPosition, m_Shared.GetItemChargeMeter( eLoadoutPosition ) + ( flDamage / flDamageToFullAttr ) * 100.f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AddConnectedPlayers( CUtlVector<CTFPlayer*> &vecPlayers, CTFPlayer *pPlayerToConsider )
{
	if ( !pPlayerToConsider )
		return;

	if ( vecPlayers.Find( pPlayerToConsider ) != vecPlayers.InvalidIndex() )
		return; // already in the list

	vecPlayers.AddToTail( pPlayerToConsider );
  
	if ( pPlayerToConsider->MedicGetHealTarget() )
	{
		AddConnectedPlayers( vecPlayers, ToTFPlayer( pPlayerToConsider->MedicGetHealTarget() ) );
	}

	for ( int i = 0 ; i < pPlayerToConsider->m_Shared.GetNumHealers() ; i++ )
	{
		CTFPlayer *pMedic = ToTFPlayer( pPlayerToConsider->m_Shared.GetHealerByIndex( i ) );
		if ( pMedic )
		{
			AddConnectedPlayers( vecPlayers, pMedic );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reduces backstab damage if we have a back shield.
//-----------------------------------------------------------------------------
bool CTFPlayer::CheckBlockBackstab( CTFPlayer *pTFAttacker )
{
	// Resistance blocks backstabs before any items are checked
	if ( m_Shared.GetCarryingRuneType() == RUNE_RESIST )
	{
		return true;
	}

	// Check all items for the attribute that blocks a backstab.
	// Destroy the first item that intercepts the backstab.
	CUtlVector<CBaseEntity*> itemList;
	int iBackStabShield = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER_WITH_ITEMS( this, iBackStabShield, &itemList, set_blockbackstab_once );
	if ( itemList.Count() )
	{
		CBaseEntity *pEntity = itemList.Element( 0 );
		if ( pEntity && !pEntity->IsEffectActive( EF_NODRAW ) )
		{
			if ( pEntity->IsWearable() )
			{
				// Yay stats.
				EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pEntity ), this, pTFAttacker, kKillEaterEvent_BackstabAbsorbed );

				// Unequip.
				CTFWearable *pItem = dynamic_cast<CTFWearable *>( pEntity );
				pItem->Break();
				pItem->AddEffects( EF_NODRAW );

				// reset the charge.
				m_Shared.SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, 0.f );
			}

			// tell the bot his Razorback just got broken
			CTFBot *me = ToTFBot( this );
			if ( me )
			{
				me->DelayedThreatNotice( pTFAttacker, 0.5f );
			}

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DamageEffect(float flDamage, int fDamageType)
{
	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );

	if (fDamageType & DMG_CRUSH)
	{
		//Red damage indicator
		color32 red = {128,0,0,128};
		UTIL_ScreenFade( this, red, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_DROWN)
	{
		//Red damage indicator
		color32 blue = {0,0,128,128};
		UTIL_ScreenFade( this, blue, 1.0f, 0.1f, FFADE_IN );
	}
	else if (fDamageType & DMG_SLASH)
	{
		if ( !bDisguised )
		{
			// If slash damage shoot some blood
			SpawnBlood(EyePosition(), g_vecAttackDir, BloodColor(), flDamage);
		}
	}
	else if ( fDamageType & DMG_BULLET )
	{
		if ( !bDisguised )
		{
			EmitSound( "Flesh.BulletImpact" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( ( ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ) && tf_avoidteammates.GetBool() ) ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS || collisionGroup == TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;
		}
	}
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

//---------------------------------------
// Is the player the passed player class?
//---------------------------------------
bool CTFPlayer::IsPlayerClass( int iClass ) const
{
	const CTFPlayerClass *pClass = &m_PlayerClass;

	if ( !pClass )
		return false;

	return ( pClass->IsClass( iClass ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CommitSuicide( bool bExplode /* = false */, bool bForce /*= false*/ )
{
	// Don't suicide if we haven't picked a class for the first time, or we're not in active state
	if ( IsPlayerClass( TF_CLASS_UNDEFINED ) || !m_Shared.InState( TF_STATE_ACTIVE ) )
		return;

	// Don't suicide during the "bonus time" if we're not on the winning team
	if ( !bForce && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
		 GetTeamNumber() != TFGameRules()->GetWinningTeam() )
	{
		return;
	}

	if ( TFGameRules()->ShowMatchSummary() )
		return;

	// No suicide while a ghost!
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return;
	
	// No suicide while a kart
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return;

	m_bSuicideExplode = bExplode;
	m_iSuicideCustomKillFlags = TF_DMG_CUSTOM_SUICIDE;

	BaseClass::CommitSuicide( bExplode, bForce );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : int
//-----------------------------------------------------------------------------
ConVar tf_preround_push_from_damage_enable( "tf_preround_push_from_damage_enable", "0", FCVAR_NONE, "If enabled, this will allow players using certain type of damage to move during pre-round freeze time." );
void CTFPlayer::ApplyPushFromDamage( const CTakeDamageInfo &info, Vector vecDir )
{
	// check if player can be moved
	if ( !tf_preround_push_from_damage_enable.GetBool() && !CanPlayerMove() )
		return;

	if ( m_bIsTargetDummy )
		return;

	Vector vecForce;
	vecForce.Init();
	if ( info.GetAttacker() == this )
	{
		Vector vecSize = WorldAlignSize();
		Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;

		if ( vecSize == hullSizeCrouch )
		{
			// Use the original hull for damage force calculation to ensure our RJ height doesn't change due to crouch hull increase
			// ^^ Comment above is an ancient lie, Ducking actually increases blast force, this value increases it even more 82 standing, 62 ducking, 55 modified
			vecSize.z = 55;
		}

		float flDamageForForce = info.GetDamageForForceCalc() ? info.GetDamageForForceCalc() : info.GetDamage();

		float flSelfPushMult = 1.0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetWeapon(), flSelfPushMult, mult_dmgself_push_force );

		
		if ( IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			// Rocket Jump
			if ( (info.GetDamageType() & DMG_BLAST) )
			{
				if ( GetFlags() & FL_ONGROUND )
				{
					vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, tf_damageforcescale_self_soldier_badrj.GetFloat() ) * flSelfPushMult;
				}
				else
				{
					vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, tf_damageforcescale_self_soldier_rj.GetFloat() ) * flSelfPushMult;
				}

				SetBlastJumpState( TF_PLAYER_ROCKET_JUMPED );

				// Reset duck in air on self rocket impulse.
				m_Shared.SetAirDucked( 0 );
			}
			else
			{
				// Self Damage no force
				vecForce.Zero();
			}
			
		}
		else
		{
			// Detonator blast jump modifier
			if ( IsPlayerClass( TF_CLASS_PYRO ) && info.GetDamageCustom() == TF_DMG_CUSTOM_FLARE_EXPLOSION )
			{
				vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, tf_damageforcescale_pyro_jump.GetFloat() ) * flSelfPushMult;
			}
			else
			{
				// Other Jumps (Stickies)
				vecForce = vecDir * -DamageForce( vecSize, flDamageForForce, DAMAGE_FORCE_SCALE_SELF ) * flSelfPushMult;
			}

			// Reset duck in air on self grenade impulse.
			m_Shared.SetAirDucked( 0 );
		}
		// Precision removes self damage so we don't want push force from damage
		if ( m_Shared.GetCarryingRuneType() == RUNE_PRECISION )
		{
			vecForce.Zero();
		}
	}
	else
	{
		// Don't let bot get pushed while they're in spawn area
		if ( m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) || m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
		{
			return;
		}

		// Sentryguns push a lot harder
		if ( (info.GetDamageType() & DMG_BULLET) && info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			float flSentryPushMultiplier = 16.f;
			CObjectSentrygun* pSentry = dynamic_cast<CObjectSentrygun*>( info.GetInflictor() );
			if ( pSentry )
			{
				flSentryPushMultiplier = pSentry->GetPushMultiplier();

				// Scale the force based on Distance, Wrangled Sentries should not push so hard at distance
				// get the distance between sentry and victim and lower push force if outside of attack range (wrangled)
				float flDistSqr = (pSentry->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
				if ( flDistSqr > SENTRY_MAX_RANGE_SQRD )
				{
					flSentryPushMultiplier *= 0.5f;
				}
			}
			vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), flSentryPushMultiplier );
		}
		else
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>(info.GetWeapon());
			if ( pWeapon && (pWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW) )
			{
				vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), tf_damageforcescale_other.GetFloat() );
				vecForce.z = 0;
			}
			else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_PLASMA_CHARGED )
			{
				vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), tf_damageforcescale_other.GetFloat() ) * 1.25f;
			}
			else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_FLARE_PELLET)
			{
				float flTimeAlive = 0.0f;
				CTFProjectile_Flare *pFlare = dynamic_cast< CTFProjectile_Flare* >( info.GetInflictor() );
				if ( pFlare )
				{
					flTimeAlive = pFlare->GetTimeAlive();
				}
				vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), TF_FLARE_PELLET_FORCE * RemapValClamped( flTimeAlive, 0.1f, 1.0f, 1.0f, TF_FLARE_PELLET_FORCE_DISTANCE_SCALE ) );
				vecForce.z = ( ( GetPlayerClass()->GetClassIndex() == TF_CLASS_HEAVYWEAPONS ) ? ( TF_FLARE_PELLET_FORCE_UPWARD_HEAVY ) : ( TF_FLARE_PELLET_FORCE_UPWARD ) );
			}
			else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_KART )
			{
				vecForce = info.GetDamageForce();
			}
			else
			{
				vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), tf_damageforcescale_other.GetFloat() );
			}

			if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				// Heavies take less push from non sentryguns
				vecForce *= 0.5f;
			}

			CBaseEntity* pInflictor = info.GetInflictor();
			if ( pInflictor && CanScatterGunKnockBack(pWeapon, info.GetDamage(), (WorldSpaceCenter() - pInflictor->WorldSpaceCenter()).LengthSqr() ) )
			{
				// Remove all Z force from these shots if they are close enough and doing enough damage
				if ( vecForce.z < 0 )
				{
					vecForce.z = 0;
				}
			}

			int iAirBlast = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iAirBlast, damage_causes_airblast );
			if ( iAirBlast )
			{
				float force = -DamageForce( WorldAlignSize(), 100, 6 );
				ApplyGenericPushbackImpulse( force * vecDir, ToTFPlayer( info.GetAttacker() ) );
				vecForce.Zero();
			}
		}

		bool bBigKnockback = false;

		CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
		if ( pAttacker && pAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS )  && pAttacker->m_Shared.IsRageDraining() )
		{
			// Generic Rage attribute
			int iRage = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pAttacker, iRage, generate_rage_on_dmg );
			if ( iRage )
			{
				// In MvM, Heavies can purchase a knockback+stun effect
				float flPushMultiplier = ( iRage + 1 ) * 24.f;
				vecForce = vecDir * -DamageForce( WorldAlignSize(), info.GetDamage(), flPushMultiplier );
				bBigKnockback = true;

				// Track for achievements
				m_AchievementData.AddPusherToHistory( pAttacker );
			}
		}

		// Airblast effect for general attacks.  Scaled by range.
		float flImpactBlastForce = 1.f;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), flImpactBlastForce, damage_blast_push );
		if ( flImpactBlastForce != 1.f )
		{
			CBaseEntity *pInflictor = info.GetInflictor();
			if ( pInflictor )
			{
				const float flMaxPushBackDistSqr = 700.f * 700.f;
				float flDistSqr = ( WorldSpaceCenter() - pInflictor->WorldSpaceCenter() ).LengthSqr();
				if ( flDistSqr <= flMaxPushBackDistSqr )
				{
					if ( vecForce.z < 0 )
					{
						vecForce.z = 0;
					}

					m_Shared.StunPlayer( 0.3f, 1.f, TF_STUN_MOVEMENT | TF_STUN_MOVEMENT_FORWARD_ONLY, pAttacker );
					flImpactBlastForce = RemapValClamped( flDistSqr, 1000.f, flMaxPushBackDistSqr, flImpactBlastForce, ( flImpactBlastForce * 0.5f ) );
					float flForce = -DamageForce( WorldAlignSize(), info.GetDamage() * 2, flImpactBlastForce );
					ApplyGenericPushbackImpulse( flForce * vecDir, pAttacker );
				}
			}
		}

		if ( TFGameRules()->GameModeUsesUpgrades() )
		{
			if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
			{
				// invading bots can't be pushed by sentry guns
				if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
				{
					return;
				}
			}

			if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS && !bBigKnockback )
			{
				if ( IsMiniBoss() )
				{
					// Minibosses can't be pushed by anything except heavy rage and airblast (airblast is suppressed when deploying in deploy ai code)
					return;
				}
				else if ( m_nDeployingBombState != TF_BOMB_DEPLOYING_NONE && ( info.GetDamageType() & DMG_BLAST ) == 0 )
				{
					// Regular robots only get pushed by blast damage when deploying the bomb
					return;
				}
			}
		}

		float flDamageForceReduction = 1.f;
		CALL_ATTRIB_HOOK_FLOAT( flDamageForceReduction, damage_force_reduction );
		vecForce *= flDamageForceReduction;
	}

	ApplyAbsVelocityImpulse( vecForce );

	// If we were pushed by an enemy explosion, we're now marked as being blasted by an enemy.
	// If we stay on the ground, next frame our player think will remove this flag.
	if ( info.GetAttacker() != this && info.GetDamageType() & DMG_BLAST )
	{
		m_bTakenBlastDamageSinceLastMovement = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayDamageResistSound( float flStartDamage, float flModifiedDamage )
{
	if ( flStartDamage <= 0.f )
		return;

	// Spam control
	if ( gpGlobals->curtime - m_flLastDamageResistSoundTime <= 0.1f )
		return;

	// Play an absorb sound based on the percentage the damage has been reduced to
	float flDamagePercent = flModifiedDamage / flStartDamage;
	if ( flDamagePercent > 0.f && flDamagePercent < 1.f )
	{
		const char *pszSoundName = ( flDamagePercent >= 0.75f ) ? "Player.ResistanceLight" :
								   ( flDamagePercent <= 0.25f ) ? "Player.ResistanceHeavy" : "Player.ResistanceMedium";

		CSoundParameters params;
		if ( CBaseEntity::GetParametersForSound( pszSoundName, params, NULL ) )
		{
			CPASAttenuationFilter filter( GetAbsOrigin(), params.soundlevel );
			EmitSound_t ep( params );
			ep.m_flVolume *= RemapValClamped( flStartDamage, 1.f, 70.f, 0.7f, 1.f );
			EmitSound( filter, entindex(), ep );
			m_flLastDamageResistSoundTime = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : int
//-----------------------------------------------------------------------------
int CTFPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( TFGameRules()->IsInItemTestingMode() && !IsFakeClient() )
		return 0;

	bool bUsingUpgrades = TFGameRules()->GameModeUsesUpgrades();

	// Always NULL check this below
	CTFPlayer *pTFAttacker = ToTFPlayer( info.GetAttacker() );

	CTFGameRules::DamageModifyExtras_t outParams;
	outParams.bIgniting = false;
	outParams.bSelfBlastDmg = false;
	outParams.bSendPreFeignDamage = false;
	outParams.bPlayDamageReductionSound = false;
	float realDamage = info.GetDamage();
	int iPreFeignDamage = realDamage;
	if ( TFGameRules() )
	{
		realDamage = TFGameRules()->ApplyOnDamageAliveModifyRules( info, this, outParams );

		if ( realDamage == -1 )
		{
			// Hard out requested from ApplyOnDamageModifyRules 
			return 0;
		}
	}

	if ( outParams.bPlayDamageReductionSound )
	{
		PlayDamageResistSound( info.GetDamage(), realDamage );
	}

	// Grab the vector of the incoming attack. 
	// (Pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	if ( info.GetInflictor() )
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector ( 0.0f, 0.0f, 10.0f ) - WorldSpaceCenter();
		info.GetInflictor()->AdjustDamageDirection( info, vecDir, this );
		VectorNormalize( vecDir );
	}
	g_vecAttackDir = vecDir;

	// Do the damage.
	m_bitsDamageType |= info.GetDamageType();

	// Check to see if the Wheatley sapper item is equipped and should react
	if ( m_bitsDamageType & DMG_BULLET &&  IsPlayerClass( TF_CLASS_SPY ) )
	{
		CBaseCombatWeapon *pRet = GetActiveWeapon();
		CTFWeaponSapper *pSap = dynamic_cast< CTFWeaponSapper* >( pRet );
		if ( pSap != NULL )
		{
			if (pSap->IsWheatleySapper())
			{
				pSap->WheatleyDamage();
			}
		}
	}

	float flBleedingTime = 0.0f;
	int iPrevHealth = m_iHealth;

	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		if ( info.GetDamageCustom() != TF_DMG_CUSTOM_BLEEDING && !outParams.bSelfBlastDmg )
		{
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( info.GetWeapon(), flBleedingTime, bleeding_duration );
		}

		// Take damage - round to the nearest integer.
		int iOldHealth = m_iHealth;
		m_iHealth -= ( realDamage + 0.5f );

		if ( IsHeadshot( info.GetDamageCustom() ) && (m_iHealth <= 0) && (iOldHealth != 1) )
		{
			int iNoDeathFromHeadshots = 0;
			CALL_ATTRIB_HOOK_INT( iNoDeathFromHeadshots, no_death_from_headshots );
			if ( iNoDeathFromHeadshots == 1 )
			{
				m_iHealth = 1;
			}
		}

		// For lifeleech, calculate how much damage we actually inflicted.
		if ( pTFAttacker && pTFAttacker->GetActiveWeapon() )
		{
			float fLifeleechOnDamage = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFAttacker->GetActiveWeapon(), fLifeleechOnDamage, lifeleech_on_damage );
			if ( fLifeleechOnDamage > 0.0f )
			{
				const float fActualDamageDealt = iOldHealth - m_iHealth;
				const float fHealAmount = fActualDamageDealt * fLifeleechOnDamage;

				if ( fHealAmount >= 0.5f )
				{
					const int iHealthToAdd = MIN( (int)(fHealAmount + 0.5f), pTFAttacker->m_Shared.GetMaxBuffedHealth() - pTFAttacker->GetHealth() );
					pTFAttacker->TakeHealth( iHealthToAdd, DMG_GENERIC );
				}
			}
		}

		// track accumulated sentry gun damage dealt by players
		if ( pTFAttacker )
		{
			// track amount of damage dealt by defender's sentry guns
			CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );
			CTFProjectile_SentryRocket *sentryRocket = dynamic_cast< CTFProjectile_SentryRocket * >( info.GetInflictor() );

			if ( ( sentry && !sentry->IsDisposableBuilding() ) || sentryRocket )
			{
				int flooredHealth = clamp( m_iHealth, 0, m_iHealth );

				pTFAttacker->AccumulateSentryGunDamageDealt( iOldHealth - flooredHealth );
			}
		}
	}

	m_flLastDamageTime = gpGlobals->curtime; // not networked
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// We only need damage time networked while in MvM
		m_flMvMLastDamageTime = gpGlobals->curtime;
	}

	// Apply a damage force.
	CBaseEntity *pAttacker = info.GetAttacker();
	if ( !pAttacker )
		return 0;

	if ( ( info.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE ) == 0 )
	{
		if ( info.GetInflictor() && ( GetMoveType() == MOVETYPE_WALK ) && 
		   ( !pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) ) && 
		   ( !m_Shared.InCond( TF_COND_DISGUISED ) ) )	
		{
			if ( !m_Shared.IsImmuneToPushback() || outParams.bSelfBlastDmg )
			{
				ApplyPushFromDamage( info, vecDir );
			}
		}
	}

	if ( outParams.bIgniting && pTFAttacker )
	{
		m_Shared.Burn( pTFAttacker, dynamic_cast< CTFWeaponBase * >( info.GetWeapon() ) );
	}

	if ( flBleedingTime > 0 && pTFAttacker )
	{
		m_Shared.MakeBleed( pTFAttacker, dynamic_cast< CTFWeaponBase * >( info.GetWeapon() ), flBleedingTime );
	}

	// Don't recieve reflected damage if you are carrying Reflect (prevents a loop in a game with two Reflect players)
	if ( ( info.GetDamageType() & TF_DMG_CUSTOM_RUNE_REFLECT ) && m_Shared.GetCarryingRuneType() == RUNE_REFLECT )
	{
		return 0;
	}

	CTFWeaponBase *pTFWeapon = dynamic_cast< CTFWeaponBase * >( info.GetWeapon() );
	if ( pTFWeapon && WeaponID_IsSniperRifle( pTFWeapon->GetWeaponID() ) )
	{
		CTFSniperRifle *pSniper = dynamic_cast<CTFSniperRifle*>( pTFWeapon );
		if ( pSniper && ( pSniper->IsZoomed() || ( pSniper->GetWeaponID() == TF_WEAPON_SNIPERRIFLE_CLASSIC ) ) )
		{
			float flJarateTime = pSniper->GetJarateTime();
			if ( flJarateTime >= 1.f )
			{
				if ( !m_Shared.IsInvulnerable() && !m_Shared.InCond( TF_COND_PHASE ) && !m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
				{
					Vector vecOrigin = info.GetDamagePosition();
					CPVSFilter filter( vecOrigin );
					TE_TFParticleEffect( filter, 0.0, "peejar_impact_small", vecOrigin, vec3_angle );
					m_Shared.AddCond( TF_COND_URINE, flJarateTime );

					if ( pTFAttacker )
					{
						UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\" against \"%s<%i><%s><%s>\" with \"%s\" (attacker_position \"%d %d %d\") (victim_position \"%d %d %d\")\n",    
							pTFAttacker->GetPlayerName(),
							pTFAttacker->GetUserID(),
							pTFAttacker->GetNetworkIDString(),
							pTFAttacker->GetTeam()->GetName(),
							"jarate_attack",
							GetPlayerName(),
							GetUserID(),
							GetNetworkIDString(),
							GetTeam()->GetName(),
							"sniperrifle",
							(int)pTFAttacker->GetAbsOrigin().x, 
							(int)pTFAttacker->GetAbsOrigin().y,
							(int)pTFAttacker->GetAbsOrigin().z,
							(int)GetAbsOrigin().x, 
							(int)GetAbsOrigin().y,
							(int)GetAbsOrigin().z );

						if ( IsHeadshot( info.GetDamageCustom() ) || LastHitGroup() == HITGROUP_HEAD )
						{
							auto pWeaponBaseSecondary = dynamic_cast< CTFWeaponBase* >( pTFAttacker->GetEntityForLoadoutSlot( LOADOUT_POSITION_SECONDARY ) );
							if ( pWeaponBaseSecondary && pWeaponBaseSecondary->HasEffectBarRegeneration() )
							{
								float flProgress = pWeaponBaseSecondary->GetEffectBarProgress();
								if ( flProgress < 1.f )
								{
									pWeaponBaseSecondary->DecrementBarRegenTime( 1.f );
								}
							}

// 							// Do an AE when it's a headshot
// 							JarExplode( entindex(), pTFAttacker, pTFWeapon, pTFWeapon, info.GetDamagePosition(), pTFAttacker->GetTeamNumber(), tf_space_thrust_scout.GetFloat(), TF_COND_URINE, flJarateTime, "peejar_impact", TF_WEAPON_PEEJAR_EXPLODE_SOUND );
// 							NDebugOverlay::Sphere( info.GetDamagePosition(), tf_space_thrust_scout.GetFloat(), 255, 20, 20, true, 5.f );
						}
					}
				}
			}

			if ( bUsingUpgrades && pTFAttacker && ( IsHeadshot( info.GetDamageCustom() ) || ( flJarateTime && LastHitGroup() == HITGROUP_HEAD ) ) )
			{
				int iExplosiveShot = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iExplosiveShot, explosive_sniper_shot );
				if ( iExplosiveShot )
				{
					pSniper->ExplosiveHeadShot( pTFAttacker, this );
				}
			}
		}
	}

	// Prevents a sandwich ignore-ammo-while-taking-damage-and-eating alias exploit
	if ( m_Shared.InCond( TF_COND_TAUNTING ) && m_Shared.GetTauntIndex() == TAUNT_BASE_WEAPON )
	{
		if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			CTFLunchBox *pLunchBox = dynamic_cast <CTFLunchBox *> ( m_Shared.GetActiveTFWeapon() );
			if ( pLunchBox )
			{
				if ( ( pLunchBox->GetLunchboxType() != LUNCHBOX_CHOCOLATE_BAR ) && ( pLunchBox->GetLunchboxType() != LUNCHBOX_FISHCAKE ) )
				{
					pLunchBox->DrainAmmo( true );
				}
			}
		}
	}

	// Fire a global game event - "player_hurt"
	IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetInt( "health", MAX( 0, m_iHealth ) );

		// HLTV event priority, not transmitted
		event->SetInt( "priority", 5 );	

		int iDamageAmount = ( iPrevHealth - m_iHealth );
		event->SetInt( "damageamount", outParams.bSendPreFeignDamage ? iPreFeignDamage : iDamageAmount );

		// Hurt by another player.
		if ( pAttacker->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( pAttacker );
			event->SetInt( "attacker", pPlayer->GetUserID() );
			
			event->SetInt( "custom", info.GetDamageCustom() );
			event->SetBool( "showdisguisedcrit", m_bShowDisguisedCrit );
			event->SetBool( "crit", (info.GetDamageType() & DMG_CRITICAL) != 0 );
			event->SetBool( "minicrit", m_bMiniCrit );
			event->SetBool( "allseecrit", m_bAllSeeCrit );
			Assert( (int)m_eBonusAttackEffect < 256 );
			event->SetInt( "bonuseffect", (int)m_eBonusAttackEffect );

			if ( pTFAttacker && pTFAttacker->GetActiveTFWeapon() )
			{
				event->SetInt( "weaponid", pTFAttacker->GetActiveTFWeapon()->GetWeaponID() );
			}
		}
		// Hurt by world.
		else
		{
			event->SetInt( "attacker", 0 );
		}

        gameeventmanager->FireEvent( event );
	}
	
	if ( pTFAttacker && pTFAttacker != this )
	{
		pTFAttacker->RecordDamageEvent( info, (m_iHealth <= 0), iPrevHealth );
	}

	//No bleeding while invul or disguised.
	bool bBleed = ( ( m_Shared.InCond( TF_COND_DISGUISED ) == false || m_Shared.GetDisguiseTeam() != pAttacker->GetTeamNumber() )
					&& !m_Shared.IsInvulnerable() );

	// No bleed effects for DMG_GENERIC
	if ( info.GetDamageType() == 0 )
	{
		bBleed = false;
	}
										   
	// Except if we are really bleeding!
	bBleed |= m_Shared.InCond( TF_COND_BLEEDING );
	
	if ( bBleed && pTFAttacker )
	{
		CTFWeaponBase *pWeapon = pTFAttacker->GetActiveTFWeapon();
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
		{
			bBleed = false;
		}
	}

	if ( bBleed && ( realDamage > 0.f ) )
	{
		Vector vDamagePos = info.GetDamagePosition();

		if ( vDamagePos == vec3_origin )
		{
			vDamagePos = WorldSpaceCenter();
		}

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			if ( ( IsMiniBoss() && static_cast< float >( GetHealth() ) / GetMaxHealth() > 0.3f ) || realDamage < 50 )
			{
				DispatchParticleEffect( "bot_impact_light", GetAbsOrigin(), vec3_angle );
			}
			else
			{
				DispatchParticleEffect( "bot_impact_heavy", GetAbsOrigin(), vec3_angle );
			}
		}
		else
		{
			CPVSFilter filter( vDamagePos );
			TE_TFBlood( filter, 0.0, vDamagePos, -vecDir, entindex() );
		}
	}

	if ( m_bIsTargetDummy )
	{
		// In the case of a targetdummy bot, restore any damage so it can never die
		TakeHealth( ( iPrevHealth - m_iHealth ), DMG_GENERIC );
	}

	m_vecFeignDeathVelocity = GetAbsVelocity();

	if ( pTFAttacker )
	{
		// If we're invuln, give whomever provided it rewards/credit
		if ( m_Shared.IsInvulnerable() && realDamage > 0.f )
		{
			// Medigun?
			CBaseEntity *pProvider = m_Shared.GetConditionProvider( TF_COND_INVULNERABLE );
			if ( !pProvider && bUsingUpgrades )
			{
				// Bottle?
				pProvider = m_Shared.GetConditionProvider( TF_COND_INVULNERABLE_USER_BUFF );
			}

			if ( pProvider )
			{
				CTFPlayer *pTFProvider = ToTFPlayer( pProvider );
				if ( pTFProvider )
				{
					if ( pTFProvider != pTFAttacker && bUsingUpgrades )
					{
						HandleRageGain( pTFProvider, kRageBuffFlag_OnHeal, ( realDamage / 2.f ), 1.f );
					}

					CTF_GameStats.Event_PlayerBlockedDamage( pTFProvider, realDamage );
				}
			}
		}

		// Give the attacker's medic Energy based on damage done
		CBaseEntity *pProvider = pTFAttacker->m_Shared.GetConditionProvider( TF_COND_HEALTH_BUFF );
		if ( pProvider )
		{
			CTFPlayer *pTFProvider = ToTFPlayer( pProvider );
			if ( pTFProvider && pTFProvider->IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				// Cap to prevent insane values coming from headshots and backstabs
				float flAmount = Min( realDamage, 250.f ) / 10.f;
				HandleRageGain( ToTFPlayer( pProvider ), kRageBuffFlag_OnHeal, flAmount, 1.f );
			}
		}
	}

	// Done.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGib( const CTakeDamageInfo &info )
{
	// Check to see if we should allow players to gib.
	if ( tf_playergib.GetInt() != 1 )
	{
		if ( tf_playergib.GetInt() < 1 )
			return false;
		else
			return true;
	}

	// normal players/bots don't gib in MvM
	if ( TFGameRules()->IsMannVsMachineMode() )
		return false;

	// Suicide explode always gibs.
	if ( m_bSuicideExplode )
	{
		m_bSuicideExplode = false;
		return true;
	}

	// Are we set up to gib always on critical hits?
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		int iAlwaysGibOnCrit = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iAlwaysGibOnCrit, crit_kill_will_gib );
		if ( iAlwaysGibOnCrit )
			return true;
	}

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_CROC )
		return true;

	int iCritOnHardHit = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iCritOnHardHit, crit_on_hard_hit );
	if ( iCritOnHardHit == 0 )
	{
		// Only blast & half falloff damage can gib.
		if ( ( (info.GetDamageType() & DMG_BLAST) == 0 ) &&
			( (info.GetDamageType() & DMG_HALF_FALLOFF) == 0 ) )
			return false;
	}

	// Explosive crits always gib.
	if ( info.GetDamageType() & DMG_CRITICAL )
		return true;

	// Hard hits also gib.
	if ( GetHealth() <= -10 )
		return true;
	
	if ( m_bGoingFeignDeath )
	{
		// The player won't actually have negative health,
		// but spies often gib from explosive damage so we should make that likely here.
		float frand = (float) rand() / VALVE_RAND_MAX;
		return (frand>0.15f) ? true : false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::HasBombinomiconEffectOnDeath( void )
{
	int iBombinomicomEffectOnDeath = 0;
	CALL_ATTRIB_HOOK_INT( iBombinomicomEffectOnDeath, bombinomicon_effect_on_death );

	return ( iBombinomicomEffectOnDeath != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Figures out if there is a special assist responsible for our death.
// Must be called before conditions are cleared druing death.
//-----------------------------------------------------------------------------
void CTFPlayer::DetermineAssistForKill( const CTakeDamageInfo &info )
{
	CTFPlayer *pPlayerAttacker = ToTFPlayer( info.GetAttacker() );
	if ( !pPlayerAttacker )
		return;

	CTFPlayer *pPlayerAssist = NULL;

	if ( m_Shared.GetConditionAssistFromVictim() )
	{
		// If we are covered in urine, mad milk, etc, then give the provider an assist.
		pPlayerAssist = ToTFPlayer( m_Shared.GetConditionAssistFromVictim() );
	}

	if ( m_Shared.IsControlStunned() )
	{
		// If we've been stunned, the stunner gets credit for the assist.
		pPlayerAssist = m_Shared.GetStunner();
	}

	// Can't assist ourself.
	if ( pPlayerAttacker && (pPlayerAttacker != pPlayerAssist) )
	{
		m_Shared.SetAssist( pPlayerAssist );
	}
	else
	{
		m_Shared.SetAssist( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );

	if ( pVictim->IsPlayer() )
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( pTFVictim && pTFVictim->IsBot() && ( pTFVictim->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
			{
				if ( pTFVictim->GetDeployingBombState() > TF_BOMB_DEPLOYING_NONE )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "mvm_kill_robot_delivering_bomb" );
					if ( event )
					{
						event->SetInt( "player", entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}

		// Custom death handlers
		// TODO: Need a system here!  This conditional is getting pretty big.
		const char *pszCustomDeath = "customdeath:none";
		if ( info.GetAttacker() && info.GetAttacker()->IsBaseObject() )
		{
			pszCustomDeath = "customdeath:sentrygun";
		}
		else if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			CBaseObject* pObj = dynamic_cast<CBaseObject*>( info.GetInflictor() );
			if ( pObj->IsMiniBuilding() )
			{
				pszCustomDeath = "customdeath:minisentrygun";
			}
			else
			{
				pszCustomDeath = "customdeath:sentrygun";
			}
		}
		else if ( IsHeadshot( info.GetDamageCustom() ) )
		{				
			pszCustomDeath = "customdeath:headshot";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			pszCustomDeath = "customdeath:backstab";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
		{
			pszCustomDeath = "customdeath:burning";
		}
		else if ( IsTauntDmg( info.GetDamageCustom() ) )
		{
			pszCustomDeath = "customdeath:taunt";
		}
		else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING_FLARE )
		{
			pszCustomDeath = "customdeath:flareburn";
		}

		// Revenge handler
		const char *pszDomination = "domination:none";
		if ( pTFVictim->GetDeathFlags() & (TF_DEATH_REVENGE|TF_DEATH_ASSISTER_REVENGE) )
		{
			pszDomination = "domination:revenge";
		}
		else if ( pTFVictim->GetDeathFlags() & TF_DEATH_DOMINATION )
		{
			pszDomination = "domination:dominated";
		}

		const char *pszVictimStunned = "victimstunned:0";
		if ( pTFVictim->m_Shared.InCond( TF_COND_STUNNED ) )
		{
			pszVictimStunned = "victimstunned:1";
		}

		const char *pszVictimDoubleJumping = "victimdoublejumping:0";
		if ( pTFVictim->m_Shared.GetAirDash() > 0 )
		{
			pszVictimDoubleJumping = "victimdoublejumping:1";
		}

		CFmtStrN<128> modifiers( "%s,%s,%s,%s,victimclass:%s", pszCustomDeath, pszDomination, pszVictimStunned, pszVictimDoubleJumping, g_aPlayerClassNames_NonLocalized[ pTFVictim->GetPlayerClass()->GetClassIndex() ] );

		bool bPlayspeech = true;

		// Don't play speech if this kill disguises the spy
		if ( IsPlayerClass( TF_CLASS_SPY ) )
		{
			if ( !Q_stricmp( "customdeath:backstab", pszCustomDeath ) )
			{
				CTFKnife *pKnife = dynamic_cast<CTFKnife *>( GetActiveTFWeapon() );
				if ( pKnife && pKnife->GetKnifeType() == KNIFE_DISGUISE_ONKILL )
				{
					bPlayspeech = false;
				}
			}
		}

		if ( bPlayspeech )
		{
			SpeakConceptIfAllowed( MP_CONCEPT_KILLED_PLAYER, modifiers );
		}

		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());
		if ( pWeapon )
		{
			pWeapon->OnPlayerKill( pTFVictim, info );

			int iCritBoost = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iCritBoost, add_onkill_critboost_time );
			if ( iCritBoost )
			{
				// Perceptually, people seem to think the effect is shorter than the stated time, so we cheat by adding a tad more for that
				m_Shared.AddCond( TF_COND_CRITBOOSTED_ON_KILL, iCritBoost+1 );
			}

			int iMiniCritBoost = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iMiniCritBoost, add_onkill_minicritboost_time );
			if ( iMiniCritBoost )
			{
				// Perceptually, people seem to think the effect is shorter than the stated time, so we cheat by adding a tad more for that
				m_Shared.AddCond( TF_COND_ENERGY_BUFF, iMiniCritBoost + 1 );
			}
		}

		// Check for CP_Foundry achievements
		if ( FStrEq( "cp_foundry", STRING( gpGlobals->mapname ) ) )
		{
			if ( pTFVictim && ( pTFVictim->GetTeamNumber() != GetTeamNumber() ) )
			{
				if ( pTFVictim->IsCapturingPoint() )
				{
					if ( info.GetDamageType() & DMG_CRITICAL )
					{
						AwardAchievement( ACHIEVEMENT_TF_MAPS_FOUNDRY_KILL_CAPPING_ENEMY );
					}
				}

				if ( InAchievementZone( pTFVictim ) )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "player_killed_achievement_zone" );
					if ( event )
					{
						event->SetInt( "attacker", entindex() );
						event->SetInt( "victim", pTFVictim->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}
		
		// Check for SD_Doomsday achievements		
		if ( FStrEq( "sd_doomsday", STRING( gpGlobals->mapname ) ) )
		{
			if ( pTFVictim && ( pTFVictim->GetTeamNumber() != GetTeamNumber() ) )
			{
				// find the flag in the map
				CCaptureFlag *pFlag = NULL;
				for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
				{
					pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
					if ( !pFlag->IsDisabled() )
					{
						break;
					}
				}

				// was the victim in an achievement zone?
				CAchievementZone *pZone = InAchievementZone( pTFVictim );
				if ( pZone )
				{
					int iZoneID = pZone->GetZoneID();
					if ( iZoneID == 0 )
					{
						if ( pFlag && pFlag->IsHome() )
						{
							AwardAchievement( ACHIEVEMENT_TF_MAPS_DOOMSDAY_DENY_NEUTRAL_PICKUP );
						}
					}
					else
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "player_killed_achievement_zone" );
						if ( event )
						{
							event->SetInt( "attacker", entindex() );
							event->SetInt( "victim", pTFVictim->entindex() );
							event->SetInt( "zone_id", iZoneID );
							gameeventmanager->FireEvent( event );
						}
					}
				}

				// check the flag carrier to see if the victim has recently damaged them
				if ( pFlag && pFlag->IsStolen() )
				{
					CTFPlayer *pFlagCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
					if ( pFlagCarrier && ( pFlagCarrier->GetTeamNumber() == GetTeamNumber() ) )
					{
						// has the victim damaged the flag carrier in the last 3 seconds?
						if ( pFlagCarrier->m_AchievementData.IsDamagerInHistory( pTFVictim, 3.0 ) )
						{
							AwardAchievement( ACHIEVEMENT_TF_MAPS_DOOMSDAY_DEFEND_CARRIER );
						}
					}
				}
			}
		}

		// Check for CP_Snakewater achievement
		if ( FStrEq( "cp_snakewater_final1", STRING( gpGlobals->mapname ) ) )
		{
			if ( pTFVictim && ( pTFVictim->GetTeamNumber() != GetTeamNumber() ) )
			{
				if ( InAchievementZone( pTFVictim ) )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "player_killed_achievement_zone" );
					if ( event )
					{
						event->SetInt( "attacker", entindex() );
						event->SetInt( "victim", pTFVictim->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}

		if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			if ( pVictim->GetTeamNumber() != GetTeamNumber() )
			{
				// Check if this kill should refill the charge meter
				CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());

				float flRefill = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT( flRefill, kill_refills_meter );
				if ( m_Shared.GetCarryingRuneType() != RUNE_NONE ) // Powerups restricts charge 
				{
					flRefill *= 0.2;
				}

				if ( flRefill > 0 && ((info.GetDamageType() & DMG_MELEE) || ( info.GetDamageCustom() == TF_DMG_CUSTOM_CHARGE_IMPACT ) ) )
				{
					m_Shared.SetDemomanChargeMeter( m_Shared.GetDemomanChargeMeter() + flRefill * 100.0f );
				}

				if ( ( pWeapon && pWeapon->IsCurrentAttackDuringDemoCharge() ) || ( info.GetDamageCustom() == TF_DMG_CUSTOM_CHARGE_IMPACT ) )
				{	
					if ( flRefill > 0 )
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "kill_refills_meter" );
						if ( event )
						{
							event->SetInt( "index", entindex() );
							gameeventmanager->FireEvent( event );
						}
					}

					if ( pTFVictim )
					{
						// could the attacker see this player when the charge started?
						if ( m_Shared.m_hPlayersVisibleAtChargeStart.Find( pTFVictim ) == m_Shared.m_hPlayersVisibleAtChargeStart.InvalidIndex() )
						{
							AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_PLAYER_YOU_DIDNT_SEE );
						}
					}
				}

				// Demoman achievement:  Kill at least 3 players capping or pushing the cart with the same detonation
				CTriggerAreaCapture *pAreaTrigger = pTFVictim->GetControlPointStandingOn();
				if ( pAreaTrigger )
				{
					CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
					if ( pCP )
					{
						if ( pCP->GetOwner() == GetTeamNumber() )
						{
							if ( GetActiveTFWeapon() && ( GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER ) )
							{
								// Add victim to our list
								int iIndex = m_Cappers.Find( pTFVictim->GetUserID() );
								if ( iIndex != m_Cappers.InvalidIndex() )
								{
									// they're already in our list
									m_Cappers[iIndex] = gpGlobals->curtime;
								}
								else
								{
									// we need to add them
									m_Cappers.Insert( pTFVictim->GetUserID(), gpGlobals->curtime );
								}
								// Did we get three?
								if ( m_Cappers.Count() >= 3 )
								{
									// Traverse the list, comparing the recorded time to curtime
									int iHitCount = 0;
									FOR_EACH_MAP_FAST ( m_Cappers, cIndex )
									{
										// For each match, increment counter
										if ( gpGlobals->curtime <= m_Cappers[cIndex] + 0.1f )
										{
											iHitCount++;
										}
										else
										{
											m_Cappers.Remove( cIndex );
										}
										
										// If we hit 3, award and purge the group
										if ( iHitCount >= 3 )
										{
											AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_CAPPING_ONEDET );
											m_Cappers.RemoveAll();
										}					
									}
								}
							}
						}
						// Kill players defending "x" times
						else
						{
							// If we're able to cap the point...
							if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
								TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
							{
								AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_DEFENDING );
							}
						}
					}
				}
			}
		}

		// Sniper Kill Rage
		if ( IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			// Item attribute
			// Add Sniper Rage On Kills
			float flRageGain = 0;
			CALL_ATTRIB_HOOK_FLOAT( flRageGain, rage_on_kill );
			if (flRageGain != 0)
			{
				m_Shared.ModifyRage(flRageGain);
			}
			
		}

		for ( int i=0; i<m_Shared.m_nNumHealers; i++ )
		{
			m_Shared.m_aHealers[i].iKillsWhileBeingHealed++;
			if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				if ( m_Shared.m_aHealers[i].iKillsWhileBeingHealed >= 5 && m_Shared.m_aHealers[i].bDispenserHeal )
				{
					// We got five kills while being healed by this dispenser. Reward the engineer with an achievement!
					CTFPlayer *pHealScorer = ToTFPlayer( m_Shared.m_aHealers[i].pHealScorer );
					if ( pHealScorer && pHealScorer->IsPlayerClass( TF_CLASS_ENGINEER ) )
					{
						pHealScorer->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_HEAVY_ASSIST );
					}
				}
			}
		}

		OnKilledOther_Effects( pVictim, info );

		// track accumulated sentry gun kills on owning player for Sentry Busters in MvM (so they can't clear this by rebuilding their sentry)
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );
		CTFProjectile_SentryRocket *sentryRocket = dynamic_cast< CTFProjectile_SentryRocket * >( info.GetInflictor() );

		if ( ( sentry && !sentry->IsDisposableBuilding() ) || sentryRocket )
		{
			IncrementSentryGunKillCount();
		}

		// Check for Halloween Death Ghosts
		CheckSpellHalloweenDeathGhosts( info, pTFVictim );

		DropDeathCallingCard( this, pTFVictim );

		if ( pTFVictim != this )
		{
			for ( int i=0; i<GetNumWearables(); ++i )
			{
				CTFWearableLevelableItem *pItem = dynamic_cast< CTFWearableLevelableItem* >( GetWearable(i) );
				if ( pItem )
				{
					pItem->IncrementLevel();
				}
			}
		}

		if ( pTFVictim )
		{
			// was the victim on a control point (includes payload carts)
			CTriggerAreaCapture *pAreaTrigger = pTFVictim->GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP && ( pCP->GetOwner() != pTFVictim->GetTeamNumber() ) )
				{
					if ( TeamplayGameRules()->TeamMayCapturePoint( pTFVictim->GetTeamNumber(), pCP->GetPointIndex() ) &&
						TeamplayGameRules()->PlayerMayCapturePoint( pTFVictim, pCP->GetPointIndex() ) )
					{
						CTFPlayer *pTFAssister = NULL;
						if ( TFGameRules() ) 
						{
							pTFAssister = ToTFPlayer( TFGameRules()->GetAssister( pTFVictim, this, info.GetInflictor() ) );
						}

						IGameEvent *event = gameeventmanager->CreateEvent( "killed_capping_player" );
						if ( event )
						{
							event->SetInt( "cp", pCP->GetPointIndex() );
							event->SetInt( "killer", entindex() );
							event->SetInt( "victim", pTFVictim->entindex() );
							event->SetInt( "assister", pTFAssister ? pTFAssister->entindex() : -1 );
							event->SetInt( "priority", 9 );

							gameeventmanager->FireEvent( event );
						}
					}
				}
			}
		}
	}
	else
	{
		if ( pVictim->IsBaseObject() )
		{
			CBaseObject *pObject = dynamic_cast<CBaseObject *>( pVictim );
			SpeakConceptIfAllowed( MP_CONCEPT_KILLED_OBJECT, pObject->GetResponseRulesModifier() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CheckSpellHalloweenDeathGhosts( const CTakeDamageInfo &info, CTFPlayer *pTFVictim )
{
	if ( !pTFVictim )
		return;

	// Check the weapon I used to kill with this player and if it has my desired attribute
	if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
	{
		int iHalloweenDeathGhosts = 0;
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );

		// was this a wrangler kill?
		if ( info.GetDamageCustom() == TF_DMG_CUSTOM_PLAYER_SENTRY )
		{
			CTFLaserPointer* pLaserPointer = dynamic_cast<CTFLaserPointer *>( GetEntityForLoadoutSlot( LOADOUT_POSITION_SECONDARY ) );
			if ( pLaserPointer )
			{
				pWeapon = pLaserPointer;
			}
		}

		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iHalloweenDeathGhosts, halloween_death_ghosts );
		if ( iHalloweenDeathGhosts > 0 )
		{
			if ( pTFVictim->GetTeam()->GetTeamNumber() == TF_TEAM_BLUE )
			{
				DispatchParticleEffect( "halloween_player_death_blue", pTFVictim->GetAbsOrigin() + Vector( 0, 0, 32 ), vec3_angle );
			}
			else if ( pTFVictim->GetTeam()->GetTeamNumber() == TF_TEAM_RED )
			{
				DispatchParticleEffect( "halloween_player_death", pTFVictim->GetAbsOrigin() + Vector( 0, 0, 32 ), vec3_angle );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on kill for primary and second-highest damage dealer
//-----------------------------------------------------------------------------
void CTFPlayer::OnKilledOther_Effects( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	int iHealOnKill = 0;

	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		int iCloakOnKill = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( GetActiveWeapon(), iCloakOnKill, add_cloak_on_kill );
		if ( iCloakOnKill > 0 )
		{
			m_Shared.AddToSpyCloakMeter( iCloakOnKill, true );
		}
	}

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
	if ( !pWeapon )
		return;

	int iRestoreHealthToPercentageOnKill = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iRestoreHealthToPercentageOnKill, restore_health_on_kill );

	if ( iRestoreHealthToPercentageOnKill > 0 )
	{
		// This attribute should ignore runes
		int iRestoreMax = GetMaxHealth() - GetRuneHealthBonus();
		// We add one here to deal with a bizarre problem that comes up leaving you one health short sometimes
		// due to bizarre floating point rounding or something equally silly.
		int iTargetHealth = ( int )( ( ( float )iRestoreHealthToPercentageOnKill / 100.0f ) * ( float )iRestoreMax ) + 1;

		int iBaseMaxHealth = GetMaxHealth() * 1.5,
			iNewHealth = Min( GetHealth() + iTargetHealth, iBaseMaxHealth ),
			iDeltaHealth = Max(iNewHealth - GetHealth(), 0);

		TakeHealth( iDeltaHealth, DMG_IGNORE_MAXHEALTH );
	}

	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iHealOnKill, heal_on_kill );
	if ( iHealOnKill != 0 )
	{
		int iHealthToAdd = MIN( iHealOnKill, m_Shared.GetMaxBuffedHealth() - m_iHealth );
		TakeHealth( iHealthToAdd, DMG_GENERIC );
		//m_iHealth += iHealthToAdd;

		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iHealthToAdd );
			event->SetInt( "entindex", entindex() );
			item_definition_index_t healingItemDef = INVALID_ITEM_DEF_INDEX;
			if ( pWeapon->GetAttributeContainer() && pWeapon->GetAttributeContainer()->GetItem() )
			{
				healingItemDef = pWeapon->GetAttributeContainer()->GetItem()->GetItemDefIndex();
			}
			event->SetInt( "weapon_def_index", healingItemDef );
			gameeventmanager->FireEvent( event ); 
		}
	}

	int iSpeedBoostOnKill = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iSpeedBoostOnKill, speed_boost_on_kill );
	if ( iSpeedBoostOnKill )
	{
		m_Shared.AddCond( TF_COND_SPEED_BOOST, iSpeedBoostOnKill );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	CTFPlayer *pPlayerAttacker = NULL;
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		pPlayerAttacker = ToTFPlayer( info.GetAttacker() );
	}

	CTFWeaponBase *pKillerWeapon = NULL;
	if ( pPlayerAttacker )
	{
		pKillerWeapon = dynamic_cast < CTFWeaponBase * > ( info.GetWeapon() );
	}

	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		static CSchemaItemDefHandle dosidoTaunt( "Square Dance Taunt" );
		static CSchemaItemDefHandle congaTaunt( "Conga Taunt" );
		if ( GetTauntEconItemView() )
		{
			if ( GetTauntEconItemView()->GetItemDefinition() == dosidoTaunt )
			{
				if ( pKillerWeapon && ( pKillerWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_MELEE ) )
				{
					if ( pPlayerAttacker )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_TAUNT_DOSIDO_MELLE_KILL );
					}
				}
			}
			else if ( GetTauntEconItemView()->GetItemDefinition() == congaTaunt )
			{
				if ( pPlayerAttacker )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "conga_kill" );
					if ( event )
					{
						event->SetInt( "index", pPlayerAttacker->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}

		StopTaunt();
	}

	// Cheat this death!
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_IN_HELL ) )
	{
		// Turn into a ghost
		m_Shared.RemoveAllCond();
		m_Shared.AddCond( TF_COND_HALLOWEEN_GHOST_MODE );

		// Create a puff right where we died to mask the ghost spawning in
		DispatchParticleEffect( "ghost_appearation", PATTACH_ABSORIGIN, this );

		// Check for achievement
		if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TRIGGER_HURT )
		{
			if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
			{
				CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 5.0 );
				if ( pRecentDamager )
				{
					pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_ENVIRONMENTAL_KILLS );
				}
			}
		}

		CTFPlayer *pRecentDamager = ( pPlayerAttacker == this ) ? TFGameRules()->GetRecentDamager( this, 0, 5.0 ) : pPlayerAttacker;
		CTakeDamageInfo ghostinfo = info;

		// If we were killed by "the world", then give credit to the next damager in the list
		if ( ( info.GetAttacker() == info.GetInflictor() && info.GetAttacker() && info.GetAttacker()->IsBSPModel() ) ||
			 ( info.GetDamageCustom() == TF_DMG_CUSTOM_TRIGGER_HURT ) ||
			 ( info.GetDamageType() & DMG_VEHICLE ) ||
			 ( info.GetDamageCustom() == TF_DMG_CUSTOM_CROC ) )
		{
			pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 10.0 );

			if ( pRecentDamager )
			{
				if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
				{
					ghostinfo.SetDamageCustom( TF_DMG_CUSTOM_KART );
					pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_KILL_KARTS );
					HatAndMiscEconEntities_OnOwnerKillEaterEvent( pRecentDamager, this, kKillEaterEvent_Halloween_UnderworldKills );
				}
			}
			// if no recent damager, check for HHH
			else if ( m_flHHHKartAttackTime > gpGlobals->curtime - 15.0f )
			{
				ghostinfo.SetDamageCustom( TF_DMG_CUSTOM_DECAPITATION_BOSS );	
			}
		}

		if ( pRecentDamager )
		{
			ghostinfo.SetAttacker( pRecentDamager );

			IGameEvent *pEvent = gameeventmanager->CreateEvent( "kill_in_hell" );
			if ( pEvent )
			{
				pEvent->SetInt( "killer", pRecentDamager->GetUserID() );
				pEvent->SetInt( "victim", GetUserID() );
				gameeventmanager->FireEvent( pEvent, true );
			}
		}

		gamestats->Event_PlayerKilled( this, ghostinfo );
		g_pGameRules->PlayerKilled( this, ghostinfo );

		if ( pRecentDamager )
		{
			pRecentDamager->Event_KilledOther( this, ghostinfo );
		}

		FeignDeath( ghostinfo, false );

		// Have 1 HP
		m_iHealth = 1;
		return;
	}

	SpeakConceptIfAllowed( MP_CONCEPT_DIED );

	StateTransition( TF_STATE_DYING );	// Transition into the dying state.

	if ( pPlayerAttacker )
	{
		if ( TFGameRules()->IsIT( this ) )
		{
			// I was IT - transfer to my killer
			TFGameRules()->SetIT( pPlayerAttacker );
		}

		if ( pPlayerAttacker != this )
		{
			if ( CTFPlayerDestructionLogic::GetRobotDestructionLogic() && ( CTFPlayerDestructionLogic::GetRobotDestructionLogic()->GetType() == CTFPlayerDestructionLogic::TYPE_PLAYER_DESTRUCTION ) )
			{
				// was this the team leader?
				if ( CTFPlayerDestructionLogic::GetRobotDestructionLogic()->GetTeamLeader( GetTeamNumber() ) == this )
				{
					IGameEvent * event = gameeventmanager->CreateEvent( "team_leader_killed" );
					if ( event )
					{
						event->SetInt( "killer", pPlayerAttacker->entindex() );
						event->SetInt( "victim", entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}
	}

	m_bIsTeleportingUsingEurekaEffect = false;

	for ( int i=0; i<GetNumWearables(); ++i )
	{
		CTFWearableLevelableItem *pItem = dynamic_cast< CTFWearableLevelableItem* >( GetWearable(i) );
		if ( pItem )
		{
			pItem->ResetLevel();
		}
	}

/*
	// We're going to save this for a future date
	if ( pPlayerAttacker )
	{
		if ( pPlayerAttacker != this )
		{
			// Killed by another player
			if ( ( TFGameRules()->GetBirthdayPlayer() == this ) || ( TFGameRules()->GetBirthdayPlayer() == NULL ) )
			{
				// I was the birthday player (or we don't have one) - transfer to my killer
				TFGameRules()->SetBirthdayPlayer( pPlayerAttacker );
			}
		}
		else
		{
			// Suicide
			if ( TFGameRules()->GetBirthdayPlayer() == this )
			{
				// I was the birthday player - reset for suicide
				TFGameRules()->SetBirthdayPlayer( NULL );
			}
		}
	}
*/
	bool bOnGround = GetFlags() & FL_ONGROUND;
	bool bElectrocuted = false;
	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
	// we want the rag doll to burn if the player was burning and was not a pyro (who only burns momentarily)
	bool bBurning = m_Shared.InCond( TF_COND_BURNING ) && ( TF_CLASS_PYRO != GetPlayerClass()->GetClassIndex() );
	CTFPlayer *pOriginalBurner = m_Shared.GetOriginalBurnAttacker();
	CTFPlayer *pLastBurner = m_Shared.GetBurnAttacker();

	if ( m_aBurnFromBackAttackers.Count() > 0 )
	{
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER )
		{
			for ( int i = 0; i < m_aBurnFromBackAttackers.Count(); i++ )
			{
				CTFPlayer *pBurner = ToTFPlayer( m_aBurnFromBackAttackers[i].Get() );

				if ( pBurner )
				{
					pBurner->AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_FROM_BEHIND );
				}
			}
		}

		ClearBurnFromBehindAttackers();
	}

	if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		CWeaponMedigun* pMedigun = assert_cast<CWeaponMedigun*>( Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		float flChargeLevel = pMedigun ? pMedigun->GetChargeLevel() : 0.f;
		float flMinChargeLevel = pMedigun ? pMedigun->GetMinChargeAmount() : 1.f;

		bool bCharged = flChargeLevel >= flMinChargeLevel;

		if ( bCharged )
		{
			// Had an ubercharge ready at death?
			CEconEntity *pVictimEconWeapon = dynamic_cast<CEconEntity *>( GetActiveTFWeapon() );
			EconEntity_OnOwnerKillEaterEventNoPartner( pVictimEconWeapon, this, kKillEaterEvent_NEGATIVE_UbersDropped );

			bElectrocuted = true;
			if ( pPlayerAttacker )
			{
				if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SCOUT ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SCOUT_KILL_CHARGED_MEDICS );
				}
				else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SNIPER ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_KILL_CHARGED_MEDIC );
				}
				else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SPY ) )
				{
					if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_BACKSTAB_MEDIC_CHARGED );
					}
				}

				CTF_GameStats.Event_PlayerAwardBonusPoints( pPlayerAttacker, this, 20 );
			}
		}

		// Disable radius healing
		m_Shared.Heal_Radius( false );

		IGameEvent * event = gameeventmanager->CreateEvent( "medic_death" );
		if ( event )
		{
			int iHealing = 0;

			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( this );
			if ( pPlayerStats )
			{
				iHealing = pPlayerStats->statsCurrentLife.m_iStat[TFSTAT_HEALING];

				// defensive fix for the moment for bug where healing value becomes bogus sometimes: if bogus, slam it to 0
				// ...copied from CTFGameRules::CalcPlayerScore()
				if ( iHealing < 0 || iHealing > 10000000 )
				{
					iHealing = 0;
				}
			}

			event->SetInt( "userid", GetUserID() );
			event->SetInt( "attacker", pPlayerAttacker ? pPlayerAttacker->GetUserID() : 0 );
			event->SetInt( "healing", iHealing );
			event->SetBool( "charged", bCharged );

			gameeventmanager->FireEvent( event );
		}
	}
	else if ( IsPlayerClass( TF_CLASS_SOLDIER ) || IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		if ( pPlayerAttacker && pPlayerAttacker->IsPlayerClass( TF_CLASS_SNIPER ) && RocketJumped() && !GetGroundEntity() ) 
		{
			if ( pKillerWeapon )
			{
				if ( WeaponID_IsSniperRifleOrBow( pKillerWeapon->GetWeaponID() ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_KILL_RJER );
				}

				if ( pKillerWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE_CLASSIC )
				{
					if ( ( info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT ) && ( info.GetDamageType() & DMG_CRITICAL ) )
					{
						if ( pPlayerAttacker->m_Shared.IsAiming() == false )
						{
							pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_HEADSHOT_JUMPER );
						}
					}
				}
			}
		}
	}
	else if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		if ( pPlayerAttacker && pPlayerAttacker->IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			// Has Engineer worked on his sentrygun recently?
			CBaseObject	*pSentry = GetObjectOfType( OBJ_SENTRYGUN );
			if ( pSentry && m_AchievementData.IsTargetInHistory( pSentry, 4.0 ) )
			{
				if ( pSentry->m_AchievementData.CountDamagersWithinTime( 3.0 ) > 0 )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_KILL_ENGY );
				}
			}
		}

		if ( m_Shared.IsCarryingObject() )
		{
			CTakeDamageInfo info( pPlayerAttacker, pPlayerAttacker, NULL, vec3_origin, GetAbsOrigin(), 0, DMG_GENERIC );
			info.SetDamageCustom( TF_DMG_CUSTOM_CARRIED_BUILDING );
			if ( m_Shared.GetCarriedObject() != NULL )
			{
				m_Shared.GetCarriedObject()->Killed( info );

				// Killeater event for being killed while carrying a building
				CEconEntity *pVictimEconWeapon = dynamic_cast<CEconEntity *>( Weapon_OwnsThisID( TF_WEAPON_WRENCH ) );
				EconEntity_OnOwnerKillEaterEventNoPartner( pVictimEconWeapon, this, kKillEaterEvent_NEGATIVE_DeathsWhileCarryingBuilding );
			}
		}
	}
	else if ( IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		if ( pPlayerAttacker )
		{
			if ( GetActiveTFWeapon() && ( GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_SNIPERRIFLE_CLASSIC ) )
			{
				if ( pKillerWeapon && ( pKillerWeapon->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_MELEE ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_MELEE_KILL_CLASSIC_RIFLE_SNIPER );
				}
			}
		}
	}

	if ( pPlayerAttacker )
	{
		if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SOLDIER ) )
		{
			if ( pPlayerAttacker->RocketJumped() || (gpGlobals->curtime - pPlayerAttacker->m_flBlastJumpLandTime) < 1 )
			{
				if ( pKillerWeapon && pKillerWeapon->GetWeaponID() == TF_WEAPON_SHOVEL )
				{
					CTFShovel *pShovel = static_cast< CTFShovel* >( pKillerWeapon );
					if ( pShovel && pShovel->GetShovelType() == SHOVEL_DAMAGE_BOOST )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SOLDIER_RJ_EQUALIZER_KILL );
					}
				}
			}
		}
		else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SNIPER ) )
		{
			if ( pKillerWeapon && WeaponID_IsSniperRifle( pKillerWeapon->GetWeaponID() ) && pPlayerAttacker->m_Shared.IsAiming() == false )
			{
				pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_KILL_UNSCOPED );
			}

			if ( pKillerWeapon && ( pKillerWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE_CLASSIC ) )
			{
				if ( ( info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT ) && ( info.GetDamageType() & DMG_CRITICAL ) )
				{
					if ( pPlayerAttacker->m_Shared.IsAiming() == false )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_CLASSIC_RIFLE_NOSCOPE_HEADSHOT );
					}
				}
			}
		}
		else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_SPY ) )
		{
			CTriggerAreaCapture *pAreaTrigger = GetControlPointStandingOn();
			if ( pAreaTrigger )
			{
				CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
				if ( pCP )
				{
					if ( pCP->GetOwner() == GetTeamNumber() )
					{
						// killed on a control point owned by my team
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_KILL_CP_DEFENDERS );
					}
					else
					{
						// killed on a control point NOT owned by my team, was it a backstab?
						if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
						{
							// was i able to capture the control point?
							if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
								 TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
							{
								pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_BACKSTAB_CAPPING_ENEMIES );
							}
						}
					}
				}
			}

			if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				//m_AchievementData.CountTargetsWithinTime
				int iHistory = 0;
				EntityHistory_t *pHistory = m_AchievementData.GetTargetHistory( iHistory );

				while ( pHistory )
				{
					if ( pHistory->hEntity && pHistory->hEntity->IsBaseObject() && m_AchievementData.IsTargetInHistory( pHistory->hEntity, 1.0f ) )
					{
						CBaseObject *pObject = dynamic_cast<CBaseObject *>( pHistory->hEntity.Get() );
					
						if ( pObject->ObjectType() == OBJ_SENTRYGUN )
						{
							pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SPY_KILL_WORKING_ENGY );
							break;
						}
					}

					iHistory++;
					pHistory = m_AchievementData.GetTargetHistory( iHistory );
				}
			}
		}
		else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			// Kill "x" players with a direct pipebomb hit
			if ( pPlayerAttacker->GetActiveTFWeapon() && ( pPlayerAttacker->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_GRENADELAUNCHER ) )
			{
				CBaseEntity *pInflictor = info.GetInflictor();
		
				if ( pInflictor && pInflictor->IsPlayer() == false )
				{
					CTFGrenadePipebombProjectile *pBaseGrenade = dynamic_cast< CTFGrenadePipebombProjectile* >( pInflictor );

					if ( pBaseGrenade && pBaseGrenade->m_bTouched != true )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_X_WITH_DIRECTPIPE );
					}
				}
			}
		}
		else if ( pPlayerAttacker->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// give achievement for killing someone who was recently damaged by our sentry
			// note that we don't check to see if the sentry is still alive
			if ( pKillerWeapon &&
				( pKillerWeapon->GetWeaponID() == TF_WEAPON_SENTRY_REVENGE ||
				  pKillerWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PRIMARY ||
				  pKillerWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_BUILDING_RESCUE ) )
			{
				if ( m_AchievementData.IsSentryDamagerInHistory( pPlayerAttacker, 5.0 ) )
				{
					pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_SHOTGUN_KILL_PREV_SENTRY_TARGET );
				}
			}
		}

		// Revenge Crits for Diamondback
		if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			pPlayerAttacker->m_Shared.IncrementRevengeCrits();
		}
	}

	// Check for CP_Foundry achievement
	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_TRIGGER_HURT )
	{
		if ( FStrEq( "cp_foundry", STRING( gpGlobals->mapname ) ) )
		{
			CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 5.0 );
			if ( pRecentDamager )
			{
				pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_MAPS_FOUNDRY_PUSH_INTO_CAULDRON );
			}
		}
	}

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED )
	{
// 		// Sketchek's Fire
// 		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
// 		{
// 			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
// 			if ( !pTFPlayer )
// 				continue;
// 
// 			if ( !pTFPlayer->IsAlive() )
// 				continue;
// 
// 			if ( !pTFPlayer->InSameTeam( this ) )
// 				continue;
// 
// 			if ( ( pTFPlayer->GetAbsOrigin() - GetAbsOrigin() ).Length2DSqr() > Square( 220.f ) )
// 				continue;
// 
// 			if ( !FVisible( pTFPlayer, MASK_OPAQUE ) )
// 				continue;
// 
// 			pTFPlayer->m_Shared.Burn( pPlayerAttacker, pKillerWeapon, 4.f );
// 			
			// Sketchek's Bequest
			if ( pPlayerAttacker )
			{
				pPlayerAttacker->m_Shared.AddCond( TF_COND_SPEED_BOOST, 3.f );
			}
// 		}
 	}

	// Record if we were stunned for achievement tracking.
	m_iOldStunFlags = m_Shared.GetStunFlags();

	// Determine the optional assist for the kill.
	DetermineAssistForKill( info );

	// put here to stop looping kritz sound from playing til respawn.
	if ( m_Shared.InCond( TF_COND_CRITBOOSTED ) )
	{
		StopSound( "TFPlayer.CritBoostOn" );
	}

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
	{
		SetPendingMerasmusPlayerBombExplode();
	}

	// check for MvM achievements
	if ( TFGameRules()->IsMannVsMachineMode() && IsBot() )
	{
		if ( pPlayerAttacker && ( pPlayerAttacker->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS ) )
		{
			if ( FStrEq( "mvm_mannhattan", STRING( gpGlobals->mapname ) ) )
			{
				CTFBot *pBot = dynamic_cast< CTFBot* >( this );
				if ( pBot )
				{
					// kill gate bots
					if ( pBot->HasTag( "bot_gatebot" ) )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_BOMB_BOT_GRIND );
					}
				}

				// kill stunned bots
				if ( m_Shared.InCond( TF_COND_MVM_BOT_STUN_RADIOWAVE ) )
				{
					if ( g_pPopulationManager->IsAdvancedPopFile() )
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "mvm_adv_wave_killed_stun_radio" );
						if ( event )
						{
							gameeventmanager->FireEvent( event );
						}
					}
				}
			}
		}
	}

	// Reset our model if we were disguised
	if ( bDisguised )
	{
		UpdateModel();
	}

	RemoveTeleportEffect();

	// Drop a pack with their leftover ammo
	// Arena: Only do this if the match hasn't started yet.
	if ( ShouldDropAmmoPack() )
	{
		DropAmmoPack( info, false, false );
	}

	if ( TFGameRules()->IsInMedievalMode() )
	{
		DropHealthPack( info, true );
	}

#ifdef TF_RAID_MODE
	// Bots sometimes drop health kits in Raid Mode
	if ( TFGameRules()->IsRaidMode() && GetTeamNumber() == TF_TEAM_RED )
	{
		if ( RandomInt( 1, 100 ) <= tf_raid_drop_healthkit_chance.GetInt() )
		{
			DropHealthPack( info, true );
		}
	}
#endif // TF_RAID_MODE

	// PvE mode credits/currency
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		MannVsMachineStats_PlayerEvent_Died( this );

		if ( IsBot() )
		{
			m_nCurrency = 0;
			if ( !IsMissionEnemy() && m_pWaveSpawnPopulator )
			{
				m_nCurrency = m_pWaveSpawnPopulator->GetCurrencyAmountPerDeath();
			}

			// only drop currency if the map designer has specified it
			if ( m_nCurrency > 0 )
			{
				// We only drop a pack when the game's accumulated enough to make it worth it
				int nDropAmount = TFGameRules()->CalculateCurrencyAmount_CustomPack( m_nCurrency );
				if ( nDropAmount )
				{
					bool bDropPack = true;

					// Give money directly to the team if a trigger killed us
					if ( info.GetDamageType() )
					{
						CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );
						if ( pTrigger )
						{
							bDropPack = false;	
							TFGameRules()->DistributeCurrencyAmount( nDropAmount, NULL, true, true );
						}
					}

					if ( bDropPack )
					{
						CTFPlayer* pMoneyMaker = NULL;
						if ( pPlayerAttacker && pPlayerAttacker->IsPlayerClass( TF_CLASS_SNIPER ) )
						{
							if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BLEEDING || ( pKillerWeapon && WeaponID_IsSniperRifleOrBow( pKillerWeapon->GetWeaponID() ) ) )
							{
								pMoneyMaker = pPlayerAttacker;

								if ( IsHeadshot( info.GetDamageCustom() ) || ( LastHitGroup() == HITGROUP_HEAD && pKillerWeapon && pKillerWeapon->GetJarateTime() ) )
								{
									IGameEvent *event = gameeventmanager->CreateEvent( "mvm_sniper_headshot_currency" );
									if ( event )
									{
										event->SetInt( "userid", pPlayerAttacker->GetUserID() );
										event->SetInt( "currency", nDropAmount );
										gameeventmanager->FireEvent( event );
									}
								}
							}
						}

						int iForceDistributeCurrency = 0;
						CALL_ATTRIB_HOOK_INT( iForceDistributeCurrency, force_distribute_currency_on_death );
						bool bForceDistribute = iForceDistributeCurrency != 0;

						// if I'm force to distribute currency, just give the credit to the attacker
						if ( !pMoneyMaker && bForceDistribute )
						{
							pMoneyMaker = pPlayerAttacker;
						}

						DropCurrencyPack( TF_CURRENCY_PACK_CUSTOM, nDropAmount, bForceDistribute, pMoneyMaker );
					}
				}
			}

			if ( !m_bIsSupportEnemy )
			{
				unsigned int iFlags = m_bIsMissionEnemy ? MVM_CLASS_FLAG_MISSION : MVM_CLASS_FLAG_NORMAL;
				if ( IsMiniBoss() )
				{
					iFlags |= MVM_CLASS_FLAG_MINIBOSS;
				}

				TFObjectiveResource()->DecrementMannVsMachineWaveClassCount( GetPlayerClass()->GetClassIconName(), iFlags );
			}

			if ( m_bIsLimitedSupportEnemy )
			{
				TFObjectiveResource()->DecrementMannVsMachineWaveClassCount( GetPlayerClass()->GetClassIconName(), MVM_CLASS_FLAG_SUPPORT_LIMITED );
			}

			// Electrical effect whenever a bot dies
			CPVSFilter filter( WorldSpaceCenter() );
			TE_TFParticleEffect( filter, 0.f, "bot_death", GetAbsOrigin(), vec3_angle );
		}
		else
		{
			// Players lose money for dying
			RemoveCurrency( tf_mvm_death_penalty.GetInt() );
		}

		// tell the population manager a player died
		// THIS MUST HAPPEN AFTER THE CURRENCY CALCULATION (ABOVE)
		// NOW THAT WE'RE CALCULATING CURRENCY ON-DEATH INSTEAD OF ON-SPAWN
		if ( g_pPopulationManager )
		{
			g_pPopulationManager->OnPlayerKilled( this );
		}

		if ( IsBot() && HasTheFlag() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			int nLevel = TFObjectiveResource()->GetFlagCarrierUpgradeLevel();
			IGameEvent *event = gameeventmanager->CreateEvent( "mvm_bomb_carrier_killed" );
			if ( event )
			{
				event->SetInt( "level", nLevel );
				gameeventmanager->FireEvent( event );
			}
		}

		if ( !IsBot() && !m_hReviveMarker )
		{
			m_hReviveMarker = CTFReviveMarker::Create( this );
		}
	}

	// This system is designed to coarsely measure a player's skill in public pvp games.
//	UpdateSkillRatingData();


	if ( pPlayerAttacker )
	{
		int iDropHealthOnKill = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayerAttacker, iDropHealthOnKill, drop_health_pack_on_kill );
		if ( iDropHealthOnKill == 1 )
		{
			DropHealthPack( info, true );
		}

		int iKillForcesAttackerToLaugh = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayerAttacker, iKillForcesAttackerToLaugh, kill_forces_attacker_to_laugh );
		if ( iKillForcesAttackerToLaugh == 1 )
		{
			// force yourself to laugh!
			pPlayerAttacker->Taunt( TAUNT_MISC_ITEM, MP_CONCEPT_TAUNT_LAUGH );
		}
	}

	// If the player has a capture flag and was killed by another player, award that player a defense
	if ( HasItem() && pPlayerAttacker && ( pPlayerAttacker != this ) )
	{
		CCaptureFlag *pCaptureFlag = dynamic_cast<CCaptureFlag *>( GetItem() );
		if ( pCaptureFlag )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( event )
			{
				event->SetInt( "player", pPlayerAttacker->entindex() );
				event->SetInt( "eventtype", TF_FLAGEVENT_DEFEND );
				event->SetInt( "carrier", entindex() );
				event->SetInt( "priority", 8 );
				event->SetInt( "team", pCaptureFlag->GetTeamNumber() );
				gameeventmanager->FireEvent( event );
			}
			CTF_GameStats.Event_PlayerDefendedPoint( pPlayerAttacker );

			if ( !CTFPlayerDestructionLogic::GetRobotDestructionLogic() || ( CTFPlayerDestructionLogic::GetRobotDestructionLogic()->GetType() != CTFPlayerDestructionLogic::TYPE_PLAYER_DESTRUCTION ) )
			{
				if ( pPlayerAttacker && pPlayerAttacker->IsPlayerClass( TF_CLASS_SNIPER ) )
				{
					CTFWeaponBase *pKillerWeapon = dynamic_cast < CTFWeaponBase * > ( info.GetWeapon() );

					if ( pKillerWeapon && pKillerWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
					{
						pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_SNIPER_BOW_KILL_FLAGCARRIER );
					}
				}

				// Handle the "you killed someone with the flag" event. We can't handle this with the usual block
				// in PlayerKilled() because by that point we've forgotten that we had the flag.
				EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pKillerWeapon ), pPlayerAttacker, this, kKillEaterEvent_DefenderKill );
			}
		}
	}

	CTFWeaponBase* pActiveWeapon = GetActiveTFWeapon();
	if( pActiveWeapon  )
	{
		CEconEntity *pVictimEconWeapon = dynamic_cast<CEconEntity *>( pActiveWeapon );

		EconEntity_OnOwnerKillEaterEventNoPartner( pVictimEconWeapon, this, kKillEaterEvent_NEGATIVE_Deaths );

		// Check if we died from environmental damage
		CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );
		if ( pTrigger )
		{
			EconEntity_OnOwnerKillEaterEventNoPartner( pVictimEconWeapon, this, kKillEaterEvent_NEGATIVE_DeathsFromEnvironment );
		}
		
		// Check if we died from fall damage
		if( info.GetDamageType() == DMG_FALL )
		{
			EconEntity_OnOwnerKillEaterEventNoPartner( pVictimEconWeapon, this, kKillEaterEvent_NEGATIVE_DeathsFromCratering );
		}
	}
	
	ClearZoomOwner();

	m_vecLastDeathPosition = GetAbsOrigin();

	CTakeDamageInfo info_modified = info;

	// Ragdoll, gib, or death animation.
	bool bRagdoll = true;
	bool bGib = false;

	// See if we should gib.
	if ( ShouldGib( info ) )
	{
		bGib = true;
		bRagdoll = false;
	}
	else
	// See if we should play a custom death animation.
	{
		// If this was a rocket/grenade kill that didn't gib, exaggerated the blast force
		if ( ( info.GetDamageType() & DMG_BLAST ) != 0 )
		{
			Vector vForceModifier = info.GetDamageForce();
			vForceModifier.x *= 2.5;
			vForceModifier.y *= 2.5;
			vForceModifier.z *= 2;
			info_modified.SetDamageForce( vForceModifier );
		}
	}

	if ( bElectrocuted && bGib )
	{
		const char *pEffectName = ( GetTeamNumber() == TF_TEAM_RED ) ? "electrocuted_gibbed_red" : "electrocuted_gibbed_blue";
		DispatchParticleEffect( pEffectName, GetAbsOrigin(), vec3_angle );
		EmitSound( "TFPlayer.MedicChargedDeath" );
	}
	
	SetGibbedOnLastDeath( bGib );

	bool bIsMvMRobot = TFGameRules()->IsMannVsMachineMode() && IsBot();
	if ( bGib && !bIsMvMRobot && IsPlayerClass( TF_CLASS_SCOUT ) && RandomInt( 1, 100 ) <= SCOUT_ADD_BIRD_ON_GIB_CHANCE )
	{
		Vector vecPos = WorldSpaceCenter();
		SpawnClientsideFlyingBird( vecPos );
	}

	// show killer in death cam mode
	// chopped down version of SetObserverTarget without the team check
	if( pPlayerAttacker )
	{
		// See if we were killed by a sentrygun. If so, look at that instead of the player
		if ( info.GetInflictor() && info.GetInflictor()->IsBaseObject() )
		{
			// Catches the case where we're killed directly by the sentrygun (i.e. bullets)
			// Look at the sentrygun
			m_hObserverTarget.Set( info.GetInflictor() ); 
		}
		// See if we were killed by a projectile emitted from a base object. The attacker
		// will still be the owner of that object, but we want the deathcam to point to the 
		// object itself.
		else if ( info.GetInflictor() && info.GetInflictor()->GetOwnerEntity() && 
					info.GetInflictor()->GetOwnerEntity()->IsBaseObject() )
		{
			m_hObserverTarget.Set( info.GetInflictor()->GetOwnerEntity() );
		}
		else
		{
			// Look at the player
			if ( m_Shared.InCond( TF_COND_HALLOWEEN_IN_HELL ) )
			{
				m_hObserverTarget.Set( pPlayerAttacker ); 
			}
			else
			{
				m_hObserverTarget.Set( info.GetAttacker() ); 
			}
		}

		// reset fov to default
		SetFOV( this, 0 );
	}
	else if ( info.GetAttacker() && info.GetAttacker()->IsBaseObject() )
	{
		// Catches the case where we're killed by entities spawned by the sentrygun (i.e. rockets)
		// Look at the sentrygun. 
		m_hObserverTarget.Set( info.GetAttacker() ); 
	}
	else if ( info.GetAttacker() && TFGameRules()->GetActiveBoss() && info.GetAttacker()->entindex() == TFGameRules()->GetActiveBoss()->entindex() )
	{
		// killed by the boss - look at him
		m_hObserverTarget.Set( info.GetAttacker() ); 

		if ( FStrEq( gpGlobals->mapname.ToCStr(), "koth_krampus" ) )
		{
			if ( FStrEq( info.GetInflictor()->GetClassname(), "base_boss" ) )
			{
				info_modified.SetDamageCustom( TF_DMG_CUSTOM_KRAMPUS_MELEE );
			}
			else if ( FStrEq( info.GetInflictor()->GetClassname(), "tf_projectile_rocket" ) )
			{
				info_modified.SetDamageCustom( TF_DMG_CUSTOM_KRAMPUS_RANGED );
			}
		}
	}
	else
	{
		m_hObserverTarget.Set( NULL );
	}

	bool bSuicide = false;
	if ( info_modified.GetDamageCustom() == TF_DMG_CUSTOM_SUICIDE )
	{
		bSuicide = true;
		// if this was suicide, recalculate attacker to see if we want to award the kill to a recent damager
		info_modified.SetAttacker( TFGameRules()->GetDeathScorer( info.GetAttacker(), info.GetInflictor(), this ) );
	}
	else if ( info.GetAttacker() == this )
	{
		bSuicide = true;
		// If we killed ourselves in non-suicide fashion, and we've been hurt lately, give that guy the kill.
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 0, 5.0 );
		if ( pRecentDamager )
		{
			info_modified.SetDamageCustom( TF_DMG_CUSTOM_SUICIDE );
			info_modified.SetDamageType( DMG_GENERIC );
			info_modified.SetAttacker( pRecentDamager );
			info_modified.SetWeapon( NULL );
			info_modified.SetInflictor( NULL );
		}
	}
	else if	( info.GetAttacker() == info.GetInflictor() && info.GetAttacker() && info.GetAttacker()->IsBSPModel() )
	{
		bSuicide = true;
		if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			// If we were killed by "the world", then give credit to the next damager in the list
			CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 10.0 );
			if ( pRecentDamager )
			{
				//info_modified.SetDamageCustom( TF_DMG_CUSTOM_SUICIDE );
				info_modified.SetDamageType( DMG_GENERIC );
				info_modified.SetAttacker( pRecentDamager );
				info_modified.SetWeapon( NULL );
				info_modified.SetInflictor( NULL );
			}
		}
		else
		{
			// If we were killed by "the world", then give credit to the next damager in the list
			CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 5.0 );
			if ( pRecentDamager )
			{
				if ( info.GetDamageCustom() != TF_DMG_CUSTOM_CROC )
				{
					info_modified.SetDamageCustom( TF_DMG_CUSTOM_SUICIDE );
				}
				info_modified.SetDamageType( DMG_GENERIC );
				info_modified.SetAttacker( pRecentDamager );
				info_modified.SetWeapon( NULL );
				info_modified.SetInflictor( NULL );
			}
			else if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) && ( info_modified.GetDamageType() & DMG_CLUB ) )
			{
				info_modified.SetDamageCustom( TF_DMG_CUSTOM_GIANT_HAMMER );
				info_modified.SetDamageType( info_modified.GetDamageType() | DMG_CRITICAL );
			}
		}
	}

	if ( pPlayerAttacker && pPlayerAttacker->m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) && !pPlayerAttacker->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		info_modified.SetDamageCustom( TF_DMG_CUSTOM_SPELL_TINY );
		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_TINY_SMASHER );
		}
	}

	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		// Attackers who fire 100% critical shots from the imbalance event don't get their (or their medic's) kills but we do count deaths as that's mostly a measure of participation
		CTFPlayer *pPowerupAttacker = ToTFPlayer( info_modified.GetAttacker() );
		// Report Kill
		CTF_GameStats.Event_PowerUpModeDeath( pPowerupAttacker, this );
		if ( pPowerupAttacker && ( pPowerupAttacker != this ) && !pPowerupAttacker->m_Shared.InCond( TF_COND_RUNE_IMBALANCE ) )
		{
			pPowerupAttacker->m_nMannpowerKills++;

			// Any medics who were healing the attacker also count this as a kill
			int nNumHealers = pPowerupAttacker->m_Shared.GetNumHealers();
		
			if ( nNumHealers > 0 )
			{
				for ( int i = 0; i < nNumHealers; i++ )
				{
					CTFPlayer *pMedic = ToTFPlayer( pPowerupAttacker->m_Shared.GetHealerByIndex( i ) );
					if ( pMedic )
					{
						pMedic->m_nMannpowerKills++;
					}
				}
			}
		}

		m_nMannpowerDeaths++;
	}

	// Drop your powerup rune when you die 
	if ( m_Shared.IsCarryingRune() )
	{
		int iTeam = GetEnemyTeam( GetTeamNumber() ); // Dead players drop opposing team colored powerups
		CTFRune::CreateRune( GetAbsOrigin(), m_Shared.GetCarryingRuneType(), iTeam, true, false );
	}

	// in PD, player death adds points to the flag drop
	if ( CTFPlayerDestructionLogic::GetRobotDestructionLogic()
		&& CTFPlayerDestructionLogic::GetRobotDestructionLogic()->GetType() == CTFPlayerDestructionLogic::TYPE_PLAYER_DESTRUCTION )
	{
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 0, 5.0 );
		int pointsOnDeath = ( !bSuicide || pRecentDamager ) ? CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->GetPointsOnPlayerDeath() : 0;

		CCaptureFlag *pFlag = NULL;
		if ( HasItem() )
		{
			pFlag = dynamic_cast<CCaptureFlag*>( GetItem() );
		}
		else
		{
			if ( pointsOnDeath && !PointInRespawnRoom( this, WorldSpaceCenter() ) )
			{
				pFlag = CCaptureFlag::Create( GetAbsOrigin(), CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->GetPropModelName(), TF_FLAGTYPE_PLAYER_DESTRUCTION );
			}
		}

		if ( pFlag )
		{
			// don't add more point to the dropping flag if the player suicided
			if ( pointsOnDeath )
			{
				pFlag->AddPointValue( pointsOnDeath );
			}
			pFlag->Drop( this, true, true, true );
		}
	}

	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>( g_pPlayerResource );
	if ( pResource )
	{
		pResource->SetPlayerClassWhenKilled( entindex(), GetPlayerClass()->GetClassIndex() );
	}

	BaseClass::Event_Killed( info_modified );

	if ( !m_bSwitchedClass )
	{
		SaveLastWeaponSlot();
	}

	if ( pPlayerAttacker && ( pPlayerAttacker != this ) && TFGameRules() && TFGameRules()->IsPasstimeMode() )
	{
		if ( m_Shared.HasPasstimeBall() )
		{
			CTFPlayer *pTFAssister = ToTFPlayer( TFGameRules()->GetAssister( this, pPlayerAttacker, info.GetInflictor() ) );
			IGameEvent *event = gameeventmanager->CreateEvent( "killed_ball_carrier" );
			if ( event )
			{
				event->SetInt( "attacker", pPlayerAttacker->entindex() );
				event->SetInt( "victim", entindex() );
				event->SetInt( "assister", pTFAssister ? pTFAssister->entindex() : -1 );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	// Remove all items...
	RemoveAllItems( true );

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon( iWeapon );

		if ( pWeapon )
		{
			pWeapon->WeaponReset();
		}
	}

	if ( GetActiveWeapon() )
	{
		m_iActiveWeaponTypePriorToDeath = GetActiveTFWeapon()->GetWeaponID();
		if ( m_iActiveWeaponTypePriorToDeath == TF_WEAPON_BUILDER )
			m_iActiveWeaponTypePriorToDeath = 0;
		GetActiveWeapon()->SendViewModelAnim( ACT_IDLE );
		GetActiveWeapon()->Holster();
		SetActiveWeapon( NULL );
	}
	else
	{
		m_iActiveWeaponTypePriorToDeath = 0;
	}

	int iIceRagdoll = 0;

	CTFPlayer *pInflictor = ToTFPlayer( info.GetInflictor() );
	if ( ( IsHeadshot( info.GetDamageCustom() ) ) && pPlayerAttacker )
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *) info.GetWeapon();
		bool bBowShot = false;
		if ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
		{
			bBowShot = true;
		}
		CTF_GameStats.Event_Headshot( pPlayerAttacker, bBowShot );
	}
	else if ( ( TF_DMG_CUSTOM_BACKSTAB == info.GetDamageCustom() ) && pInflictor )
	{
		CTF_GameStats.Event_Backstab( pInflictor );

		if ( pKillerWeapon )
		{
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pKillerWeapon, iIceRagdoll, freeze_backstab_victim );
		}
	}

	bool bCloakedCorpse = false;
	if ( pKillerWeapon && pKillerWeapon->GetWeaponID() == TF_WEAPON_KNIFE )
	{
		CTFKnife *pKnife = dynamic_cast<CTFKnife*>( pKillerWeapon );
		if ( pKnife && pKnife->ShouldDisguiseOnBackstab() )
		{
			bCloakedCorpse = true;
		}
	}

	int iGoldRagdoll = 0;
	if ( pKillerWeapon )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pKillerWeapon, iGoldRagdoll, set_turn_to_gold );
	}

	int iRagdollsBecomeAsh = 0;
	if ( info.GetWeapon() )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iRagdollsBecomeAsh, ragdolls_become_ash );
	}

	int iRagdollsPlasmaEffect = 0;
	if ( info.GetWeapon() )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iRagdollsPlasmaEffect, ragdolls_plasma_effect );
	}

	int iCustomDamage = info.GetDamageCustom();
	if ( iRagdollsPlasmaEffect )
	{
		iCustomDamage = TF_DMG_CUSTOM_PLASMA;
	}

	int iCritOnHardHit = 0;
	if ( info.GetWeapon() )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iCritOnHardHit, crit_on_hard_hit );
	}

	// Create the ragdoll entity.
	if ( bGib || bRagdoll )
	{
// 		if ( !bBurning && pKillerWeapon && ( pKillerWeapon->GetWeaponID() == TF_WEAPON_SLAP ) )
// 		{
// 			CTFSlap *pSlap = dynamic_cast< CTFSlap* >( pKillerWeapon );
// 			if ( pSlap )
// 			{
// 				bBurning = ( pSlap->GetNumKills() > 1 ); // first kill doesn't burn
// 			}
// 		}

		CreateRagdollEntity( bGib, bBurning, bElectrocuted, bOnGround, bCloakedCorpse, iGoldRagdoll != 0, iIceRagdoll != 0, iRagdollsBecomeAsh != 0, iCustomDamage, ( iCritOnHardHit != 0 ) );
	}


	// Remove all conditions...
	m_Shared.RemoveAllCond();

	// Don't overflow the value for this.
	m_iHealth = 0;

	// If we died in sudden death and we're an engineer, explode our buildings
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) && TFGameRules()->InStalemate() && TFGameRules()->IsInArenaMode() == false )
	{
		for (int i = GetObjectCount()-1; i >= 0; i--)
		{
			CBaseObject *obj = GetObject(i);
			Assert( obj );

			if ( obj )
			{
				obj->DetonateObject();
			}		
		}
	}

	// Achievement checks
	if ( pPlayerAttacker )
	{
		// ACHIEVEMENT_TF_MEDIC_KILL_HEALED_SPY - medic kills a spy he has been healing
		if ( IsPlayerClass( TF_CLASS_SPY ) && pPlayerAttacker->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			// if we were killed by a medic, see if he healed us most recently

			for ( int i=0;i<pPlayerAttacker->WeaponCount();i++ )
			{
				CTFWeaponBase *pWpn = ( CTFWeaponBase *)pPlayerAttacker->GetWeapon( i );

				if ( pWpn == NULL )
					continue;

				if ( pWpn->GetWeaponID() == TF_WEAPON_MEDIGUN )
				{
					CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun * >( pWpn );
					if ( pMedigun )
					{
						if ( pMedigun->GetMostRecentHealTarget() == this )
						{
							pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_MEDIC_KILL_HEALED_SPY );
						}
					}
				}
			}
		}

		if ( bBurning && pPlayerAttacker->IsPlayerClass( TF_CLASS_PYRO ) )
		{
			// ACHIEVEMENT_TF_PYRO_KILL_MULTIWEAPONS - Pyro kills previously ignited target with other weapon
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());

			if ( ( pOriginalBurner == pPlayerAttacker || pLastBurner == pPlayerAttacker ) && pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PYRO )
			{
				pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_MULTIWEAPONS );
			}

			// ACHIEVEMENT_TF_PYRO_KILL_TEAMWORK - Pyro kills an enemy previously ignited by another Pyro
			if ( pOriginalBurner != pPlayerAttacker )
			{
				pPlayerAttacker->AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_TEAMWORK );
			}
		}
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// Have teammates announce my death
		if ( GetTeamNumber() == TF_TEAM_PVE_DEFENDERS )
		{
			// have the last player on the defenders speak the last_man_standing line
			CUtlVector< CTFPlayer * > playerVector;
			CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS, true );
			if ( playerVector.Count() == 1 )
			{
				CTFPlayer *pAlivePlayer = playerVector[0];
				if ( pAlivePlayer )
				{
					pAlivePlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_LAST_MAN_STANDING );
				}
			}
			else
			{
				if ( pPlayerAttacker && pPlayerAttacker->IsMiniBoss() )
				{
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_GIANT_KILLED_TEAMMATE, TF_TEAM_PVE_DEFENDERS );
				}

				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_DEFENDER_DIED, TF_TEAM_PVE_DEFENDERS, CFmtStr( "victimclass:%s", g_aPlayerClassNames_NonLocalized[ GetPlayerClass()->GetClassIndex() ] ).Access() );
			}
		}
		else
		{
			if ( IsMiniBoss() )
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_GIANT_KILLED, TF_TEAM_PVE_DEFENDERS );
			}
		}
	}

	// Reset Streaks to zero
	m_Shared.ResetStreaks();
	for ( int i = 0; i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon(i);
		if ( !pWpn )
			continue;
		pWpn->SetKillStreak( 0 );
	}

	for ( int i = 0; i < GetNumWearables(); ++i )
	{
		CTFWearable* pWearable = dynamic_cast<CTFWearable*>( GetWearable(i) );
		if ( !pWearable )
			continue;
		pWearable->SetKillStreak( 0 );
	}

	// Is the player inside a respawn time override volume?
	// don't do this for MvM bots
	if ( !TFGameRules()->IsMannVsMachineMode() || !IsBot() )
	{
		FOR_EACH_VEC( ITriggerPlayerRespawnOverride::AutoList(), i )
		{
			CTriggerPlayerRespawnOverride *pTriggerRespawn = static_cast< CTriggerPlayerRespawnOverride* >( ITriggerPlayerRespawnOverride::AutoList()[i] );
			if ( !pTriggerRespawn->m_bDisabled && pTriggerRespawn->IsTouching( this ) )
			{
				SetRespawnOverride( pTriggerRespawn->GetRespawnTime(), pTriggerRespawn->GetRespawnName() );
				break;
			}
			else
			{
				SetRespawnOverride( -1.f, NULL_STRING );
			}
		}
	}

	// Is this an environmental death?
	if ( ( info.GetAttacker() == info.GetInflictor() && info.GetAttacker() && info.GetAttacker()->IsBSPModel() ) ||
		 ( info.GetDamageCustom() == TF_DMG_CUSTOM_TRIGGER_HURT ) ||
		 ( info.GetDamageType() & DMG_VEHICLE ) ||
		 ( info.GetDamageCustom() == TF_DMG_CUSTOM_CROC ) )
	{
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 1, 5.0 );
		if ( pRecentDamager )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "environmental_death" );
			if ( event )
			{
				event->SetInt( "killer", pRecentDamager->entindex() );
				event->SetInt( "victim", entindex() );
				event->SetInt( "priority", 9 );

				gameeventmanager->FireEvent( event );
			}
		}
	}

	// make sure to remove custom attributes
	RemoveAllCustomAttributes();
}

struct SkillRatingAttackRecord_t
{
	CHandle< CTFPlayer > hAttacker;
	float flDamagePercent;
};
	
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pWeapon - 
//			&vecOrigin - 
//			&vecAngles - 
//-----------------------------------------------------------------------------
bool CTFPlayer::CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles )
{
	// Look up the hand and weapon bones.
	int iHandBone = LookupBone( "weapon_bone" );
	if ( iHandBone == -1 )
		return false;

	GetBonePosition( iHandBone, vecOrigin, vecAngles );

	// need to fix up the z because the weapon bone position can be under the player
	if ( IsTaunting() )
	{
		// put the pack at the middle of the dying player
		vecOrigin = WorldSpaceCenter();
	}

	// Draw the position and angles.
	Vector vecDebugForward2, vecDebugRight2, vecDebugUp2;
	AngleVectors( vecAngles, &vecDebugForward2, &vecDebugRight2, &vecDebugUp2 );

	/*
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugForward2 * 25.0f ), 255, 0, 0, false, 30.0f );
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugRight2 * 25.0f ), 0, 255, 0, false, 30.0f );
	NDebugOverlay::Line( vecOrigin, ( vecOrigin + vecDebugUp2 * 25.0f ), 0, 0, 255, false, 30.0f ); 
	*/

	VectorAngles( vecDebugUp2, vecAngles );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// NOTE: If we don't let players drop ammo boxes, we don't need this code..
//-----------------------------------------------------------------------------
void CTFPlayer::AmmoPackCleanUp( void )
{
	// If we have more than 3 ammo packs out now, destroy the oldest one.
	int iNumPacks = 0;
	CTFAmmoPack *pOldestBox = NULL;

	// Cycle through all ammobox in the world and remove them
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_ammo_pack" );
	while ( pEnt )
	{
		CBaseEntity *pOwner = pEnt->GetOwnerEntity();
		if (pOwner == this)
		{
			CTFAmmoPack *pThisBox = dynamic_cast<CTFAmmoPack *>( pEnt );
			Assert( pThisBox );
			if ( pThisBox )
			{
				iNumPacks++;

				// Find the oldest one
				if ( pOldestBox == NULL || pOldestBox->GetCreationTime() > pThisBox->GetCreationTime() )
				{
					pOldestBox = pThisBox;
				}
			}
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, "tf_ammo_pack" );
	}

	// If they have more than 3 packs active, remove the oldest one
	if ( iNumPacks > 3 && pOldestBox )
	{
		UTIL_Remove( pOldestBox );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldDropAmmoPack()
{
	if ( TFGameRules()->IsMannVsMachineMode() && IsBot() )
		return false;

	if ( TFGameRules()->IsInArenaMode() && TFGameRules()->InStalemate() == false )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropAmmoPack( const CTakeDamageInfo &info, bool bEmpty, bool bDisguisedWeapon )
{
	// We want the ammo packs to look like the player's weapon model they were carrying.
	// except if they are melee or building weapons
	CTFWeaponBase *pWeapon = NULL;
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();

	if ( !pActiveWeapon || pActiveWeapon->GetTFWpnData().m_bDontDrop )
	{
		// Don't drop this one, find another one to drop

		int iWeight = -1;

		// find the highest weighted weapon
		for (int i = 0;i < WeaponCount(); i++) 
		{
			CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon(i);
			if ( !pWpn )
				continue;

			if ( pWpn->GetTFWpnData().m_bDontDrop )
				continue;

			int iThisWeight = pWpn->GetTFWpnData().iWeight;

			if ( iThisWeight > iWeight )
			{
				iWeight = iThisWeight;
				pWeapon = pWpn;
			}
		}
	}
	else
	{
		pWeapon = pActiveWeapon;
	}

	// If we didn't find one, bail
	if ( !pWeapon )
		return;

	// Figure out which model/skin to use for the drop. We may pull from our real weapon or
	// from the weapon we're disguised as.
	CTFWeaponBase *pDropWeaponProps = (bDisguisedWeapon && m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseWeapon())
									? m_Shared.GetDisguiseWeapon()
									: pWeapon;

	const char *pszWorldModel = pDropWeaponProps->GetWorldModel();
	int nSkin = pDropWeaponProps->GetDropSkinOverride();

	if ( nSkin < 0 )
	{
		nSkin = pDropWeaponProps->GetSkin();
	}

	if ( pszWorldModel == NULL )
		return;

	// Find the position and angle of the weapons so the "ammo box" matches.
	Vector vecPackOrigin;
	QAngle vecPackAngles;
	if( !CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles ) )
		return;

	CEconItemView *pItem = pDropWeaponProps->GetAttributeContainer()->GetItem();
	bool bIsSuicide = info.GetAttacker() ? info.GetAttacker()->GetTeamNumber() == GetTeamNumber() : false;

	CTFDroppedWeapon *pDroppedWeapon = CTFDroppedWeapon::Create( this, vecPackOrigin, vecPackAngles, pszWorldModel, pItem );
	if ( pDroppedWeapon )
	{
		pDroppedWeapon->InitDroppedWeapon( this, pDropWeaponProps, false, bIsSuicide );
	}

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( vecPackOrigin, vecPackAngles, this, "models/items/ammopack_medium.mdl" );
	Assert( pAmmoPack );
	if ( pAmmoPack )
	{
		pAmmoPack->InitAmmoPack( this, pWeapon, nSkin, bEmpty, bIsSuicide );
	
		// Clean up old ammo packs if they exist in the world
		AmmoPackCleanUp();	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropAmmoPackFromProjectile( CBaseEntity *pProjectile )
{
	QAngle qPackAngles = pProjectile->GetAbsAngles();
	Vector vecPackOrigin = pProjectile->GetAbsOrigin();
	UTIL_Remove( pProjectile );

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( vecPackOrigin, qPackAngles, this, "models/items/ammopack_small.mdl" );
	Assert( pAmmoPack );
	if ( pAmmoPack )
	{
		// half of ammopack_small
		float flAmmoRatio = 0.1f;
		pAmmoPack->InitAmmoPack( this, NULL, 0, false, false, flAmmoRatio );

		// Clean up old ammo packs if they exist in the world
		AmmoPackCleanUp();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DropHealthPack( const CTakeDamageInfo &info, bool bEmpty )
{
	Vector vecSrc = this->WorldSpaceCenter();
	CHealthKitSmall *pMedKit = assert_cast<CHealthKitSmall*>( CBaseEntity::Create( "item_healthkit_small", vecSrc, vec3_angle, this ) );
	if ( pMedKit )
	{
		Vector vecImpulse = RandomVector( -1,1 );
		vecImpulse.z = 1;
		VectorNormalize( vecImpulse );

		Vector vecVelocity = vecImpulse * 250.0;
		pMedKit->DropSingleInstance( vecVelocity, this, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::DropCurrencyPack( CurrencyRewards_t nSize /* = TF_CURRENCY_PACK_SMALL */, int nAmount /*= 0*/, bool bForceDistribute /*= false*/, CBasePlayer* pMoneyMaker /*= NULL*/ )
{
	// SMALL, MEDIUM, LARGE packs generate a default value on spawn
	// Only pass in an amount when dropping TF_CURRENCY_PACK_CUSTOM

	Vector vecSrc = this->WorldSpaceCenter();
	CCurrencyPack *pCurrencyPack = NULL;

	switch ( nSize )
	{
	case TF_CURRENCY_PACK_SMALL:
		pCurrencyPack = assert_cast<CCurrencyPackSmall*>( CBaseEntity::Create( "item_currencypack_small", vecSrc, vec3_angle, this ) );
		break;

	case TF_CURRENCY_PACK_MEDIUM:
		pCurrencyPack = assert_cast<CCurrencyPackMedium*>( CBaseEntity::Create( "item_currencypack_medium", vecSrc, vec3_angle, this ) );
		break;

	case TF_CURRENCY_PACK_LARGE:
		pCurrencyPack = assert_cast<CCurrencyPack*>( CBaseEntity::Create( "item_currencypack_large", vecSrc, vec3_angle, this ) );
		break;

	case TF_CURRENCY_PACK_CUSTOM:
		// Pop file may have said to not drop anything
		Assert( nAmount > 0 );
		if ( nAmount == 0 )
			return;

		// Create no spawn first so we can set the multiplier before it spawns & picks it model
		pCurrencyPack = assert_cast<CCurrencyPack*>( CBaseEntity::CreateNoSpawn( "item_currencypack_custom", vecSrc, vec3_angle, this ) );
		pCurrencyPack->SetAmount( nAmount );
		break;
	};

	if ( pCurrencyPack )
	{
		Vector vecImpulse = RandomVector( -1,1 );
		vecImpulse.z = 1;
		VectorNormalize( vecImpulse );
		Vector vecVelocity = vecImpulse * 250.0;

		if ( pMoneyMaker || bForceDistribute )
		{
			pCurrencyPack->DistributedBy( pMoneyMaker );
		}
		
		DispatchSpawn( pCurrencyPack );
		pCurrencyPack->DropSingleInstance( vecVelocity, this, 0, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayerDeathThink( void )
{
	// We're doing this here to avoid getting stuck
	// in a recursive loop if we do it in Event_Killed
	if ( m_bPendingMerasmusPlayerBombExplode )
	{
		m_bPendingMerasmusPlayerBombExplode = false;
		MerasmusPlayerBombExplode();
	}

	// don't need to think again...
}

//-----------------------------------------------------------------------------
// Purpose: Remove the tf items from the player then call into the base class
//          removal of items.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllItems( bool removeSuit )
{
	if ( TFGameRules() && TFGameRules()->IsPasstimeMode() && m_Shared.HasPasstimeBall() )
	{
		g_pPasstimeLogic->EjectBall( this, this );
	}

	// If the player has a capture flag, drop it.
	if ( HasItem() )
	{
		int nFlagTeamNumber = GetItem()->GetTeamNumber();
		GetItem()->Drop( this, true );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "player", entindex() );
			event->SetInt( "eventtype", TF_FLAGEVENT_DROPPED );
			event->SetInt( "priority", 8 );
			event->SetInt( "team", nFlagTeamNumber );
			gameeventmanager->FireEvent( event );
		}
	}

	m_Shared.Heal_Radius( false );

	if ( m_hOffHandWeapon.Get() )
	{ 
		HolsterOffHandWeapon();

		// hide the weapon model
		// don't normally have to do this, unless we have a holster animation
		CBaseViewModel *vm = GetViewModel( 1 );
		if ( vm )
		{
			vm->SetWeaponModel( NULL, NULL );
		}

		m_hOffHandWeapon = NULL;
	}

	Weapon_SetLast( NULL );
	UpdateClientData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClientHearVox( const char *pSentence )
{
	//TFTODO: implement this.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateModel( void )
{
	SetModel( GetPlayerClass()->GetModelName() );

	// Immediately reset our collision bounds - our collision bounds will be set to the model's bounds.
	SetCollisionBounds( GetPlayerMins(), GetPlayerMaxs() );

	m_PlayerAnimState->OnNewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iSkin - 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateSkin( int iTeam )
{
	// The player's skin is team - 2.
	int iSkin = iTeam - 2;

	// Check to see if the skin actually changed.
	if ( iSkin != m_iLastSkin )
	{
		m_nSkin = iSkin;
		m_iLastSkin = iSkin;
	}
}

//=========================================================================
// Displays the state of the items specified by the Goal passed in
void CTFPlayer::DisplayLocalItemStatus( CTFGoal *pGoal )
{
#if 0
	for (int i = 0; i < 4; i++)
	{
		if (pGoal->display_item_status[i] != 0)
		{
			CTFGoalItem *pItem = Finditem(pGoal->display_item_status[i]);
			if (pItem)
				DisplayItemStatus(pGoal, this, pItem);
			else
				ClientPrint( this, HUD_PRINTTALK, "#Item_missing" );
		}
	}
#endif
}

void CTFPlayer::SetIsCoaching( bool bIsCoaching )
{ 
	m_bIsCoaching = bIsCoaching;

	if ( !bIsCoaching )
	{
		// reset our last action time so we don't get kicked for being idle while we were coaching
		m_flLastAction = gpGlobals->curtime;
	}
}

//=========================================================================
// Called when the player disconnects from the server.
void CTFPlayer::TeamFortress_ClientDisconnected( void )
{
	RemoveAllOwnedEntitiesFromWorld( true );
	RemoveNemesisRelationships();

	StopTaunt();

	RemoveAllWeapons();

	RemoveAllItems( true );

	TFGameRules()->RemovePlayerFromQueue( this );
	TFGameRules()->PlayerHistory_AddPlayer( this );

	DuelMiniGame_NotifyPlayerDisconnect( this );

	// notify the vote controller
	if ( g_voteControllerGlobal )
		g_voteControllerGlobal->OnPlayerDisconnected( this );

	if ( GetTeamVoteController() )
		GetTeamVoteController()->OnPlayerDisconnected( this );

	// cleanup coaching
	if ( GetCoach() )
	{
		GetCoach()->SetIsCoaching( false );
		GetCoach()->SetStudent( NULL );
	}
	else if ( GetStudent() )
	{
		SetIsCoaching( false );
		GetStudent()->SetCoach( NULL );
	}

	if ( TFGameRules() && TFGameRules()->IsPowerupMode()  )
	{
		// Drop your powerup when you disconnect 
		if ( m_Shared.IsCarryingRune() )
		{
			CTFRune::CreateRune( GetAbsOrigin(), m_Shared.GetCarryingRuneType(), TEAM_ANY, true, false );
		}
		// Clean up Mannpower data
		m_nMannpowerKills = 0;
		m_nMannpowerDeaths = 0;
		m_bMannpowerHereForFullInterval = false;

		if ( m_bIsInMannpowerDominantCondition )
		{
			CSteamID steamIDForPlayer;
			if ( GetSteamID( &steamIDForPlayer ) )
			{
				TFGameRules()->PowerupModeDominantDisconnect( steamIDForPlayer, m_flRemoveDominantConditionTime );
			}
		}
	}
}

//=========================================================================
// Removes everything this player has (buildings, grenades, etc.) from the world
void CTFPlayer::RemoveAllOwnedEntitiesFromWorld( bool bExplodeBuildings /* = false */ )
{
	RemoveOwnedProjectiles();

	if ( TFGameRules()->IsMannVsMachineMode() && ( GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
	{
		// MvM engineer bots leave their sentries behind when they die
		return;
	}


#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() && ( GetTeamNumber() == TF_TEAM_RED ) )
	{
		// for now, leave Engineer's sentrygun alive after he dies
		return;
	}
#endif // TF_RAID_MODE

	if ( IsBotOfType( TF_BOT_TYPE ) && ToTFBot( this )->HasAttribute( CTFBot::RETAIN_BUILDINGS ) )
	{
		// keep this bot's buildings
		return;
	}

	// Destroy any buildables - this should replace TeamFortress_RemoveBuildings
	RemoveAllObjects( bExplodeBuildings );
}

//=========================================================================
// Removes all rockets the player has fired into the world
// (this prevents a team kill cheat where players would fire rockets 
// then change teams to kill their own team)
void CTFPlayer::RemoveOwnedProjectiles( void )
{
	FOR_EACH_VEC( IBaseProjectileAutoList::AutoList(), i )
	{
		CBaseProjectile *pProjectile = static_cast< CBaseProjectile* >( IBaseProjectileAutoList::AutoList()[i] );

		// if the player owns this entity, remove it
		bool bOwner = ( pProjectile->GetOwnerEntity() == this );

		if ( !bOwner )
		{
			if ( pProjectile->GetBaseProjectileType() == TF_BASE_PROJECTILE_GRENADE )
			{

				CTFWeaponBaseGrenadeProj *pGrenade = assert_cast<CTFWeaponBaseGrenadeProj*>( pProjectile );
				if ( pGrenade )
				{
					bOwner = ( pGrenade->GetThrower() == this );
				}
			}
			else if ( pProjectile->GetProjectileType() == TF_PROJECTILE_SENTRY_ROCKET )
			{
				CTFProjectile_SentryRocket *pRocket = assert_cast<CTFProjectile_SentryRocket*>( pProjectile );
				if ( pRocket )
				{
					bOwner = ( pRocket->GetScorer() == this );
				}
			}
		}

		if ( bOwner )
		{
			pProjectile->SetTouch( NULL );
			pProjectile->AddEffects( EF_NODRAW );
			UTIL_Remove( pProjectile );
		}
	}

	FOR_EACH_VEC( ITFFlameEntityAutoList::AutoList(), i )
	{
		CTFFlameEntity *pFlameEnt = static_cast< CTFFlameEntity* >( ITFFlameEntityAutoList::AutoList()[i] );

		if ( pFlameEnt->IsEntityAttacker( this ) )
		{
			pFlameEnt->SetTouch( NULL );
			pFlameEnt->AddEffects( EF_NODRAW );
			UTIL_Remove( pFlameEnt );
		}
	}

	FOR_EACH_VEC( ITFFlameManager::AutoList(), i )
	{
		CTFFlameManager *pFlameManager = static_cast< CTFFlameManager* >( ITFFlameManager::AutoList()[i] );
		if ( pFlameManager->GetAttacker() == this )
		{
			pFlameManager->ClearPoints();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::NoteWeaponFired()
{
	Assert( m_pCurrentCommand );
	if ( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}

	// Remember the tickcount when the weapon was fired and lock viewangles here!
	if ( m_iLockViewanglesTickNumber != gpGlobals->tickcount )
	{
		m_iLockViewanglesTickNumber = gpGlobals->tickcount;
		m_qangLockViewangles = pl.v_angle;
	}
}

//=============================================================================
//
// Player state functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPlayerStateInfo *CTFPlayer::StateLookupInfo( int nState )
{
	// This table MUST match the 
	static CPlayerStateInfo playerStateInfos[] =
	{
		{ TF_STATE_ACTIVE,				"TF_STATE_ACTIVE",				&CTFPlayer::StateEnterACTIVE,				NULL,	NULL },
		{ TF_STATE_WELCOME,				"TF_STATE_WELCOME",				&CTFPlayer::StateEnterWELCOME,				NULL,	&CTFPlayer::StateThinkWELCOME },
		{ TF_STATE_OBSERVER,			"TF_STATE_OBSERVER",			&CTFPlayer::StateEnterOBSERVER,				NULL,	&CTFPlayer::StateThinkOBSERVER },
		{ TF_STATE_DYING,				"TF_STATE_DYING",				&CTFPlayer::StateEnterDYING,				NULL,	&CTFPlayer::StateThinkDYING },
	};

	for ( int iState = 0; iState < ARRAYSIZE( playerStateInfos ); ++iState )
	{
		if ( playerStateInfos[iState].m_nPlayerState == nState )
			return &playerStateInfos[iState];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnter( int nState )
{
	m_Shared.m_nPlayerState = nState;
	m_pStateInfo = StateLookupInfo( nState );

	if ( tf_playerstatetransitions.GetInt() == -1 || tf_playerstatetransitions.GetInt() == entindex() )
	{
		if ( m_pStateInfo )
			Msg( "ShowStateTransitions: entering '%s'\n", m_pStateInfo->m_pStateName );
		else
			Msg( "ShowStateTransitions: entering #%d\n", nState );
	}

	// Initialize the new state.
	if ( m_pStateInfo && m_pStateInfo->pfnEnterState )
	{
		(this->*m_pStateInfo->pfnEnterState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateLeave( void )
{
	if ( m_pStateInfo && m_pStateInfo->pfnLeaveState )
	{
		(this->*m_pStateInfo->pfnLeaveState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateTransition( int nState )
{
	StateLeave();
	StateEnter( nState );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterWELCOME( void )
{
	PickWelcomeObserverPoint();  
	
	StartObserverMode( OBS_MODE_FIXED );

	// Important to set MOVETYPE_NONE or our physics object will fall while we're sitting at one of the intro cameras.
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );		

	PhysObjectSleep();

	if ( g_pServerBenchmark->IsLocalBenchmarkPlayer( this ) )
	{
		m_bSeenRoundInfo = true;

		ChangeTeam( TEAM_SPECTATOR );
	}
	else if ( gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bSeenRoundInfo = true;

		ChangeTeam( TEAM_SPECTATOR );
	}
	else if ( (TFGameRules() && TFGameRules()->IsLoadingBugBaitReport()) )
	{
		m_bSeenRoundInfo = true;
		
		ChangeTeam( TF_TEAM_BLUE );
		SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
		ForceRespawn();
	}
	else if ( IsInCommentaryMode()  )
	{
		m_bSeenRoundInfo = true;
	}
//=============================================================================
// HPE_BEGIN:
// [msmith]	When in training, we want the option to show an intro movie.
//=============================================================================
	else if ( TFGameRules()->IsInTraining() && IsFakeClient() == false )
	{
		ShowViewPortPanel( PANEL_INTRO, true );
		m_bSeenRoundInfo = true;
	}
//=============================================================================
// HPE_END
//=============================================================================
	else
	{
		if ( !IsX360() )
		{
			char pszWelcome[128];
			Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome" );
			if ( UTIL_GetActiveHolidayString() )
			{
				Q_snprintf( pszWelcome, sizeof(pszWelcome), "#TF_Welcome_%s", UTIL_GetActiveHolidayString() );
			}

			KeyValues *data = new KeyValues( "data" );
			data->SetString( "title", pszWelcome );		// info panel title
			data->SetString( "type", "1" );				// show userdata from stringtable entry
			data->SetString( "msg",	"motd" );			// use this stringtable entry
			data->SetString( "msg_fallback",	"motd_text" );			// use this stringtable entry if the base is HTML, and client has disabled HTML motds
			data->SetBool( "unload", sv_motd_unload_on_dismissal.GetBool() );

			ShowViewPortPanel( PANEL_INFO, true, data );

			data->deleteThis();
		}
		else
		{
			ShowViewPortPanel( PANEL_MAPINFO, true );
		}

		m_bSeenRoundInfo = false;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkWELCOME( void )
{
	if ( !IsFakeClient() )
	{
		if ( IsInCommentaryMode() )
		{
			ChangeTeam( TF_TEAM_BLUE );
			SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
			ForceRespawn();
		}
		else if ( TFGameRules()->IsInTraining() )
		{
			int iTeam = TFGameRules()->GetAssignedHumanTeam();
			int iClass = TFGameRules()->GetTrainingModeLogic() ? TFGameRules()->GetTrainingModeLogic()->GetDesiredClass() : TF_CLASS_SOLDIER;
			ChangeTeam( iTeam != TEAM_ANY ? iTeam : TF_TEAM_BLUE );
			SetDesiredPlayerClassIndex( iClass );
			ForceRespawn();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	RemoveEffects( EF_NODRAW | EF_NOSHADOW );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_Local.m_iHideHUD = 0;
	PhysObjectWake();

	m_flLastAction = gpGlobals->curtime;
	m_flLastHealthRegenAt = gpGlobals->curtime;
	SetContextThink( &CTFPlayer::RegenThink, gpGlobals->curtime + TF_REGEN_TIME, "RegenThink" );
	if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		SetContextThink( &CTFPlayer::RuneRegenThink, gpGlobals->curtime + TF_REGEN_TIME_RUNE, "RuneRegenThink" );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SetObserverMode(int mode)
{
	if ( !TFGameRules() )
		return false;

	if ( mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES )
		return false;

	if ( TFGameRules()->ShowMatchSummary() )
		return false;

	// Skip over OBS_MODE_POI if we're not in Passtime mode
	if ( mode == OBS_MODE_POI )
	{
		if ( !TFGameRules()->IsPasstimeMode() )
		{
			mode = OBS_MODE_ROAMING;
		}
	}

	// Skip over OBS_MODE_ROAMING for dead players
	if( GetTeamNumber() > TEAM_SPECTATOR )
	{
		if ( IsDead() && ( mode > OBS_MODE_FIXED ) && mp_fadetoblack.GetBool() )
		{
			mode = OBS_MODE_CHASE;
		}
		else if ( mode == OBS_MODE_ROAMING )
		{
			mode = OBS_MODE_IN_EYE;
		}
	}

	if ( m_iObserverMode > OBS_MODE_DEATHCAM )
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;

	if ( !m_bArenaIsAFK )
	{
		m_flLastAction = gpGlobals->curtime;
	}

	// this is the old behavior, still supported for community servers
	bool bAllowSpecModeChange = TFGameRules()->IsInTournamentMode() ? TFGameRules()->IsMannVsMachineMode() : true;

	// new behavior for Valve casual, competitive, and mvm matches
	const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		bAllowSpecModeChange = pMatchDesc->BAllowSpectatorModeChange();
	}

	if ( !bAllowSpecModeChange )
	{
		if ( ( mode != OBS_MODE_DEATHCAM ) && ( mode != OBS_MODE_FREEZECAM ) && ( GetTeamNumber() > TEAM_SPECTATOR ) )
		{
			if ( IsValidObserverTarget( GetObserverTarget() ) )
			{
				m_iObserverMode.Set( OBS_MODE_IN_EYE );
			}
			else
			{
				m_iObserverMode.Set( OBS_MODE_DEATHCAM );
			}
		}
	}

	switch ( m_iObserverMode )
	{
	case OBS_MODE_NONE:
	case OBS_MODE_FIXED :
	case OBS_MODE_DEATHCAM :
		SetFOV( this, 0 );	// Reset FOV
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_NONE );
		break;

	case OBS_MODE_CHASE :
	case OBS_MODE_IN_EYE :	
		// udpate FOV and viewmodels
		SetObserverTarget( m_hObserverTarget );	
		SetMoveType( MOVETYPE_OBSERVER );
		break;

	case OBS_MODE_POI : // PASSTIME
		SetObserverTarget( TFGameRules()->GetObjectiveObserverTarget() );	
		SetMoveType( MOVETYPE_OBSERVER );
		break;

	case OBS_MODE_ROAMING :
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
		
	case OBS_MODE_FREEZECAM:
		SetFOV( this, 0 );	// Reset FOV
		SetObserverTarget( m_hObserverTarget );
		SetViewOffset( vec3_origin );
		SetMoveType( MOVETYPE_OBSERVER );
		break;
	}

	CheckObserverSettings();

	return true;	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterOBSERVER( void )
{
	// Always start a spectator session in chase mode
	m_iObserverLastMode = OBS_MODE_CHASE;

	if( m_hObserverTarget == NULL )
	{
		// find a new observer target
		CheckObserverSettings();
	}

	if ( !m_bAbortFreezeCam )
	{
		FindInitialObserverTarget();
	}

	// If we haven't yet set a valid observer mode, such as when
	// the player aborts the freezecam and sets a mode "by hand"
	// force the initial mode to last mode
	if ( m_iObserverMode <= OBS_MODE_FREEZECAM )
	{
		if ( TFGameRules() && TFGameRules()->IsPasstimeMode() )
		{
			m_iObserverMode = OBS_MODE_POI;
		}
		else
		{
			m_iObserverMode = m_iObserverLastMode;
		}
	}

	// If we're in fixed mode, but we found an observer target, move to non fixed.
	if ( m_hObserverTarget.Get() != NULL && m_iObserverMode == OBS_MODE_FIXED )
	{
		m_iObserverMode.Set( OBS_MODE_IN_EYE );
	}

	StartObserverMode( m_iObserverMode );

	PhysObjectSleep();

	if ( GetTeamNumber() != TEAM_SPECTATOR )
	{
		HandleFadeToBlack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkOBSERVER()
{
	// Make sure nobody has changed any of our state.
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StateEnterDYING( void )
{
	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_bPlayedFreezeCamSound = false;
	m_bAbortFreezeCam = false;

	if ( TFGameRules() && TFGameRules()->IsInArenaMode() )
	{
		float flLastActionTime =  gpGlobals->curtime - m_flLastAction;
		float flAliveThisRoundTime = gpGlobals->curtime - TFGameRules()->GetRoundStart();

		if ( flAliveThisRoundTime - flLastActionTime < 0 )
		{
			m_bArenaIsAFK = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move the player to observer mode once the dying process is over
//-----------------------------------------------------------------------------
void CTFPlayer::StateThinkDYING( void )
{
	// If we have a ragdoll, it's time to go to deathcam
	if ( !m_bAbortFreezeCam && m_hRagdoll && 
		(m_lifeState == LIFE_DYING || m_lifeState == LIFE_DEAD) && 
		GetObserverMode() != OBS_MODE_FREEZECAM )
	{
		if ( GetObserverMode() != OBS_MODE_DEATHCAM )
		{
			StartObserverMode( OBS_MODE_DEATHCAM );	// go to observer mode
		}
		RemoveEffects( EF_NODRAW | EF_NOSHADOW );	// still draw player body
	}

	float flTimeInFreeze = spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();
	float flFreezeEnd = (m_flDeathTime + TF_DEATH_ANIMATION_TIME + flTimeInFreeze );
	if ( !m_bPlayedFreezeCamSound  && GetObserverTarget() && GetObserverTarget() != this )
	{
		// Start the sound so that it ends at the freezecam lock on time
		float flFreezeSoundLength = 0.3;
		float flFreezeSoundTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() - flFreezeSoundLength;
		if ( gpGlobals->curtime >= flFreezeSoundTime )
		{
			CSingleUserRecipientFilter filter( this );
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = "TFPlayer.FreezeCam";
			EmitSound( filter, entindex(), params );

			m_bPlayedFreezeCamSound = true;
		}
	}

	if ( gpGlobals->curtime >= (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) )	// allow x seconds death animation / death cam
	{
		if ( GetObserverTarget() && GetObserverTarget() != this )
		{
			if ( !m_bAbortFreezeCam && gpGlobals->curtime < flFreezeEnd )
			{
				if ( GetObserverMode() != OBS_MODE_FREEZECAM )
				{
					StartObserverMode( OBS_MODE_FREEZECAM );
					PhysObjectSleep();
				}
				return;
			}
		}

		if ( GetObserverMode() == OBS_MODE_FREEZECAM )
		{
			// If we're in freezecam, and we want out, abort.  (only if server is not using mp_fadetoblack)
			if ( m_bAbortFreezeCam && !mp_fadetoblack.GetBool() )
			{
				if ( m_hObserverTarget == NULL )
				{
					// find a new observer target
					CheckObserverSettings();
				}

				FindInitialObserverTarget();

				if ( TFGameRules() && TFGameRules()->IsPasstimeMode() )
				{
					SetObserverMode( OBS_MODE_POI );
				}
				else
				{
					SetObserverMode( OBS_MODE_CHASE );
				}
				ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(OBS_MODE_CHASE) );
			}
		}

		// Don't allow anyone to respawn until freeze time is over, even if they're not
		// in freezecam. This prevents players skipping freezecam to spawn faster.
		if ( gpGlobals->curtime < flFreezeEnd )
			return;

		m_lifeState = LIFE_RESPAWNABLE;

		StopAnimation();

		IncrementInterpolationFrame();

		if ( GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND) )
			SetMoveType( MOVETYPE_NONE );

		StateTransition( TF_STATE_OBSERVER );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AttemptToExitFreezeCam( void )
{
	float flFreezeTravelTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME ) + spec_freeze_traveltime.GetFloat() + 0.5;
	if ( gpGlobals->curtime < flFreezeTravelTime )
		return;

	m_bAbortFreezeCam = true;
}

class CIntroViewpoint : public CPointEntity
{
	DECLARE_CLASS( CIntroViewpoint, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	int			m_iIntroStep;
	float		m_flStepDelay;
	string_t	m_iszMessage;
	string_t	m_iszGameEvent;
	float		m_flEventDelay;
	int			m_iGameEventData;
	float		m_flFOV;
};

BEGIN_DATADESC( CIntroViewpoint )
	DEFINE_KEYFIELD( m_iIntroStep,	FIELD_INTEGER,	"step_number" ),
	DEFINE_KEYFIELD( m_flStepDelay,	FIELD_FLOAT,	"time_delay" ),
	DEFINE_KEYFIELD( m_iszMessage,	FIELD_STRING,	"hint_message" ),
	DEFINE_KEYFIELD( m_iszGameEvent,	FIELD_STRING,	"event_to_fire" ),
	DEFINE_KEYFIELD( m_flEventDelay,	FIELD_FLOAT,	"event_delay" ),
	DEFINE_KEYFIELD( m_iGameEventData,	FIELD_INTEGER,	"event_data_int" ),
	DEFINE_KEYFIELD( m_flFOV,	FIELD_FLOAT,	"fov" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( game_intro_viewpoint, CIntroViewpoint );

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CTFPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound )
{
 	return GiveAmmo( iCount, iAmmoIndex, bSuppressSound, kAmmoSource_Pickup );
}

//-----------------------------------------------------------------------------
// Purpose: Give the player some ammo.
// Input  : iCount - Amount of ammo to give.
//			iAmmoIndex - Index of the ammo into the AmmoInfoArray
//			iMax - Max carrying capability of the player
// Output : Amount of ammo actually given
//-----------------------------------------------------------------------------
int CTFPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource eAmmoSource )
{
	if ( iCount <= 0 )
	{
		return 0;
	}

	// Metal always ignores the eAmmoSource settings, which are really used only for determining
	// whether ammo should be converted into health or ignored or, in rare cases, treated as actual
	// ammo.
	if ( iAmmoIndex != TF_AMMO_METAL )
	{
		//int iAmmoBecomesHealth = 0;
		//CALL_ATTRIB_HOOK_INT( iAmmoBecomesHealth, ammo_becomes_health );
		//if ( iAmmoBecomesHealth == 1 )
		//{
		//	// Ammo from ground pickups is converted to health.
		//	if ( eAmmoSource == kAmmoSource_Pickup )
		//	{
		//		int iTakenHealth = TakeHealth( iCount, DMG_GENERIC );
		//		if ( iTakenHealth > 0 )
		//		{
		//			if ( !bSuppressSound )
		//			{
		//				EmitSound( "BaseCombatCharacter.AmmoPickup" );
		//			}
		//			m_Shared.HealthKitPickupEffects( iCount );
		//		}
		//		return iTakenHealth;
		//	}

		//	// Ammo from the cart or engineer dispensers is flatly ignored.
		//	if ( eAmmoSource == kAmmoSource_DispenserOrCart )
		//		return 0;

		//	Assert( eAmmoSource == kAmmoSource_Resupply );
		//}

		// Items that rely on timers to refill ammo use these attributes
		// Prevents "touch supply closet and spam the thing" scenario.
		if ( iAmmoIndex >= TF_AMMO_GRENADES1 && iAmmoIndex <= TF_AMMO_GRENADES3 )
		{
			if ( eAmmoSource != kAmmoSource_ResourceMeter )
			{
				int iDenyGRENADE1Resupply = 0;
				CALL_ATTRIB_HOOK_INT( iDenyGRENADE1Resupply, grenades1_resupply_denied );

				int iDenyGRENADE2Resupply = 0;
				CALL_ATTRIB_HOOK_INT( iDenyGRENADE2Resupply, grenades2_resupply_denied );

				int iDenyGRENADE3Resupply = 0;
				CALL_ATTRIB_HOOK_INT( iDenyGRENADE3Resupply, grenades3_resupply_denied );

				if ( iAmmoIndex == TF_AMMO_GRENADES1 && iDenyGRENADE1Resupply )
					return 0;
				else if ( iAmmoIndex == TF_AMMO_GRENADES2 && iDenyGRENADE2Resupply )
					return 0;
				else if ( iAmmoIndex == TF_AMMO_GRENADES3 && iDenyGRENADE3Resupply )
					return 0;
			}
		}
	}
	else if ( iAmmoIndex == TF_AMMO_METAL )
	{
		if ( eAmmoSource != kAmmoSource_Resupply )
		{
			float flMultMetal = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT( flMultMetal, mult_metal_pickup );
			iCount = (int)(flMultMetal * iCount );
		}
	}


	if ( !g_pGameRules->CanHaveAmmo( this, iAmmoIndex ) )
	{
		// game rules say I can't have any more of this ammo type.
		return 0;
	}

	if ( iAmmoIndex < 0 || iAmmoIndex >= MAX_AMMO_SLOTS )
	{
		return 0;
	}

	int iAdd = MIN( iCount, GetMaxAmmo(iAmmoIndex) - GetAmmoCount(iAmmoIndex) );
	if ( iAdd < 1 )
	{
		return 0;
	}

	// Ammo pickup sound
	if ( !bSuppressSound )
	{
		EmitSound( "BaseCombatCharacter.AmmoPickup" );
	}

	CBaseCombatCharacter::GiveAmmo( iAdd, iAmmoIndex, bSuppressSound );
	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAmmo( int iCount, int iAmmoIndex )
{

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetInt() > 1 && !IsBot() )
		return;
#endif // _DEBUG || STAGING_ONLY

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GIANT ) )
	{
		return;
	}

	// Infinite primary, secondary and metal in these game modes
	if ( TFGameRules() && iAmmoIndex < TF_AMMO_GRENADES1 )
	{
		if ( TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
			return;

	}

	CBaseCombatCharacter::RemoveAmmo( iCount, iAmmoIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAmmo( int iCount, const char *szName )
{
	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
			return;

		if ( TFGameRules()->GameModeUsesMiniBosses() && IsMiniBoss() )
			return;
	}

	CBaseCombatCharacter::RemoveAmmo( iCount, szName );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of a particular type owned
//			owned by the character
// Input  :	Ammo Index
// Output :	The amount of ammo
//-----------------------------------------------------------------------------
int CTFPlayer::GetAmmoCount( int iAmmoIndex ) const
{
	if ( iAmmoIndex == -1 )
		return 0;

	if ( IsFakeClient() && TFGameRules()->IsInItemTestingMode() )
		return 999;

	return BaseClass::GetAmmoCount( iAmmoIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Has to be const for override, but needs to access non-const member methods.
//-----------------------------------------------------------------------------
int	CTFPlayer::GetMaxHealth() const
{
	int iMax = const_cast<CTFPlayer*>( this )->GetMaxHealthForBuffing();

	// Also add the nonbuffed health bonuses
	CALL_ATTRIB_HOOK_INT( iMax, add_maxhealth_nonbuffed );

	return Max( iMax, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ResetMaxHealthDrain( void )
{
	m_nMaxHealthDrainBucket = 0;
	m_dMaxHealthDrainLastUpdate = -1.0;
	m_bMaxHealthRefilling = true;
	m_dMaxHealthDrainAccumulator = 0.0;
	m_dMaxHealthDrainHealthAccumulator = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetMaxHealthForBuffing()
{
	int iMax = m_PlayerClass.GetMaxHealth();
	CALL_ATTRIB_HOOK_INT( iMax, add_maxhealth );

	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		iMax += pWeapon->GetMaxHealthMod();
	}
	if ( const_cast<CTFPlayer*>(this)->GetPlayerClass()->GetClassIndex() == TF_CLASS_DEMOMAN )
	{
		CTFSword *pSword = dynamic_cast<CTFSword*>(const_cast<CTFPlayer*>(this)->Weapon_OwnsThisID( TF_WEAPON_SWORD ));
		if ( pSword )
		{
			iMax += pSword->GetSwordHealthMod();
		}
	}

	// Some Powerup Runes increase your Max Health
	iMax += GetRuneHealthBonus();

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GIANT ) )
	{
		return iMax * tf_halloween_giant_health_scale.GetFloat();
	}

	// We're draining or restoring health for a special-case attribute here.
	// This code is very fragile, and can break many things when changed.
	{
		int nOriginalMaxHealth = iMax;
		float flMaxHealthDrainRate = 0.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flMaxHealthDrainRate, mod_maxhealth_drain_rate );

		// Drain maxhealth while the right weapon is deployed
		if ( flMaxHealthDrainRate > 0.f )
		{
			int nActivationPenalty = 0;
			
			// Previously refilling?
			if ( m_bMaxHealthRefilling )
			{
				// Deploy penalty
				nActivationPenalty = tf_maxhealth_drain_deploy_cost.GetInt();
				m_bMaxHealthRefilling = false;

				if ( m_dMaxHealthDrainAccumulator < 0.f )
				{
					m_dMaxHealthDrainAccumulator = fabs( m_dMaxHealthDrainAccumulator );
				}
				if ( m_dMaxHealthDrainHealthAccumulator < 0.f )
				{
					m_dMaxHealthDrainHealthAccumulator = fabs( m_dMaxHealthDrainHealthAccumulator );
				}
			}
			
			if ( m_dMaxHealthDrainLastUpdate == -1.0 )
			{
				m_dMaxHealthDrainLastUpdate = gpGlobals->curtime;
			}

			const int nFloor = tf_maxhealth_drain_hp_min.GetInt();

			if ( nOriginalMaxHealth - nFloor > m_nMaxHealthDrainBucket )
			{
				int nMaxHealthDrainBucketPrev = m_nMaxHealthDrainBucket;

				// MaxHealth
				double dDrainRate = ( ( gpGlobals->curtime - m_dMaxHealthDrainLastUpdate ) * flMaxHealthDrainRate ) + nActivationPenalty;
				m_dMaxHealthDrainAccumulator += dDrainRate;
				int nMaxHealthDrain = floor( m_dMaxHealthDrainAccumulator );
				if ( nMaxHealthDrain > 0 )
				{
					m_dMaxHealthDrainAccumulator -= nMaxHealthDrain;
					m_nMaxHealthDrainBucket += nMaxHealthDrain;
				}

				// Health
				float flHealthPerc = (float)GetHealth() / (float)Max( ( iMax - nMaxHealthDrainBucketPrev ), nFloor );
				m_dMaxHealthDrainHealthAccumulator += ( dDrainRate * flHealthPerc );
				int nHealthDrain = floor( m_dMaxHealthDrainHealthAccumulator );
				if ( nHealthDrain > 0 )
				{
					m_dMaxHealthDrainHealthAccumulator -= nHealthDrain;
					int nNewHealth = Max( ( GetHealth() - nHealthDrain ), 1 );
					SetHealth( nNewHealth );
				}
			}
			
			iMax = Max( ( iMax - m_nMaxHealthDrainBucket ), nFloor );
			//DevMsg( "Health/MaxHealth: %i/%i\n", GetHealth(), iMax );

			m_dMaxHealthDrainLastUpdate = gpGlobals->curtime;
 		}
 		// Attribute no longer on the active weapon - regen maxhealth
 		else if ( m_nMaxHealthDrainBucket > 0 )
		{
			// Eat away any remaining value in the accumulators
			if ( !m_bMaxHealthRefilling )
			{
				if ( m_dMaxHealthDrainAccumulator > 0.f )
				{
					m_dMaxHealthDrainAccumulator *= -1.f;
				}
				if ( m_dMaxHealthDrainHealthAccumulator > 0.f )
				{
					m_dMaxHealthDrainHealthAccumulator *= -1.f;
				}
			}

			m_bMaxHealthRefilling = true;

			flMaxHealthDrainRate = 0.f;
			CALL_ATTRIB_HOOK_FLOAT( flMaxHealthDrainRate, mod_maxhealth_drain_rate );
		
 			// Something yanked the attribute off the player (load-out change, whatever)
			if ( flMaxHealthDrainRate == 0.f )
			{
				ResetMaxHealthDrain();
			}
 			else
 			{
				int nMaxHealthDrainBucketPrev = m_nMaxHealthDrainBucket;
				double dRestoreRate = ( gpGlobals->curtime - m_dMaxHealthDrainLastUpdate ) * flMaxHealthDrainRate;
				m_dMaxHealthDrainAccumulator += dRestoreRate;
				int nMaxHealthRefill = floor( m_dMaxHealthDrainAccumulator );
				if ( nMaxHealthRefill > 0 )
				{
					m_dMaxHealthDrainAccumulator -= nMaxHealthRefill;
					m_nMaxHealthDrainBucket -= nMaxHealthRefill;
				}

				// Use previous value to determine percentage
				float flHealthPerc = (float)GetHealth() / Max( ( iMax - nMaxHealthDrainBucketPrev ), 1 );
				
				m_dMaxHealthDrainHealthAccumulator += ( dRestoreRate * flHealthPerc );
				int nHealthRestore = floor( m_dMaxHealthDrainHealthAccumulator );
				if ( nHealthRestore > 0 )
				{
					m_dMaxHealthDrainHealthAccumulator -= nHealthRestore;
					int nHealthMaxOverheal = nOriginalMaxHealth * m_Shared.GetMaxOverhealMultiplier();
					int nHealthMaxAttribute = nOriginalMaxHealth;
					CALL_ATTRIB_HOOK_INT( nHealthMaxAttribute, add_maxhealth_nonbuffed );
					int nHealthMax = Max( nHealthMaxOverheal, nHealthMaxAttribute );
					int nNewHealth = Min( GetHealth() + nHealthRestore, nHealthMax );
					SetHealth( nNewHealth );
				}

				m_dMaxHealthDrainLastUpdate = gpGlobals->curtime;

				// Done
				iMax = Max( ( iMax - m_nMaxHealthDrainBucket ), 1 );
				// DevMsg( "Health/MaxHealth/Original: %i/%i/%i\n", GetHealth(), iMax, nOriginalMaxHealth );
				if ( iMax == nOriginalMaxHealth )
				{
					// Don't discard anything left over
					if ( m_dMaxHealthDrainHealthAccumulator )
					{
						SetHealth( GetHealth() + 1 );
					}
					
					ResetMaxHealthDrain();
				}
 			}
		}
		else if ( m_dMaxHealthDrainLastUpdate != -1.0 )
		{
			// This fixes a bug in public where players can trigger a sequence
			// that initializes m_dMaxHealthDrainLastUpdate, but we really shouldn't,
			// and when it's finally time to use the value, we end up trying to restore
			// a massive amount of health based on the time delta with curtime.
			// There's a better fix for this that probably requires touching more code,
			// but we should probably ship this first, and then figure it out.
			ResetMaxHealthDrain();
		}
	}
	//
	// This block should always be last
	//

	return iMax;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetRuneHealthBonus() const
{
	int nRuneType = m_Shared.GetCarryingRuneType();

	if ( nRuneType == RUNE_NONE )
	{
		return 0;
	}
	
	if ( nRuneType == RUNE_KNOCKOUT )
	{
		if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
		{
			// Swords have various extra melee benefits, so we reduce Max Health bonus
			if ( Weapon_GetSlot( TF_WPN_TYPE_MELEE ) )
			{
				int iDecapitateType = 0;
				CALL_ATTRIB_HOOK_INT( iDecapitateType, decapitate_type );

				if ( iDecapitateType )
				{
					return 20;
				}
			}
			// Shields have passive resistance so we reduce Max Health bonus
			if ( m_Shared.IsShieldEquipped() )
			{
				return 30;
			}
			return 150;
		}
		else if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) || IsPlayerClass( TF_CLASS_PYRO ) )
		{
			return 125;
		}
		else if ( IsPlayerClass( TF_CLASS_SOLDIER ) || IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			return 150;
		}
		else
		{
			return 175;
		}
	}
	else if ( nRuneType == RUNE_REFLECT )
	{
		if ( m_bIsInMannpowerDominantCondition )
		{
			return ( 320 - m_PlayerClass.GetMaxHealth() );
		}
		return ( 400 - m_PlayerClass.GetMaxHealth() );
	}
	else if ( nRuneType == RUNE_KING )
	{
		if ( m_bIsInMannpowerDominantCondition )
		{
			return 20;
		}
		return 100;
	}
	else if ( nRuneType == RUNE_VAMPIRE )
	{
		return 80;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StartPowerupModeDominant( bool bIsAlreadyDominant )
{
	if ( bIsAlreadyDominant )
	{
		ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_Dominant_Continue" );
		ClientPrint( this, HUD_PRINTTALK, "#TF_Powerup_Dominant_Continue" );
	}
	else
	{
		if ( m_Shared.GetCarryingRuneType() != RUNE_NONE )
		{
			m_Shared.AddCond( TF_COND_MARKEDFORDEATH, 5.0f );
		}
		m_Shared.AddCond( TF_COND_POWERUPMODE_DOMINANT, PERMANENT_CONDITION );
		ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_Dominant" );
		ClientPrint( this, HUD_PRINTTALK, "#TF_Powerup_Dominant" );
	}
	// condition is set to last two kill count compare cycles plus a few seconds so they don't potentially ping pong out then back in
	m_flRemoveDominantConditionTime = gpGlobals->curtime + ( ( tf_powerup_mode_killcount_timer_length.GetFloat() * 2 ) + 5.0f ); 
	m_bIsInMannpowerDominantCondition = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::EndPowerupModeDominant( void )
{
	m_Shared.RemoveCond( TF_COND_POWERUPMODE_DOMINANT );
	m_Shared.RemoveCond( TF_COND_MARKEDFORDEATH );
	ClientPrint( this, HUD_PRINTCENTER, "#TF_Powerup_OutOfDominant" );
	ClientPrint( this, HUD_PRINTTALK, "#TF_Powerup_OutOfDominant" );
	EmitSound( "Mannpower.PlayerIsNoLongerDominant" );
	if ( m_bIsInMannpowerDominantCondition )
	{
		m_bIsInMannpowerDominantCondition = false;
		m_flRemoveDominantConditionTime = 0;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ForceRegenerateAndRespawn( void )
{
	m_bRegenerating.Set( true );
	ForceRespawn();
	m_bRegenerating.Set( false );
}

//-----------------------------------------------------------------------------
// Purpose: Reset player's information and force him to spawn
//-----------------------------------------------------------------------------
void CTFPlayer::ForceRespawn( void )
{
	VPROF_BUDGET( "CTFPlayer::ForceRespawn", VPROF_BUDGETGROUP_PLAYER );

	CTF_GameStats.Event_PlayerForceRespawn( this );

	m_flSpawnTime = gpGlobals->curtime;
	m_Shared.m_flHolsterAnimTime = 0.f;	// BRETT SAID I COULD DO THIS

	bool bRandom = false;

	// force a random class if the server requires it
	if ( TFGameRules() && TFGameRules()->IsInArenaMode() )
	{
		if ( tf_arena_force_class.GetBool() == true )
		{
			bRandom = true;
			if ( GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( !IsAlive() || ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_STALEMATE ) )
				{
					HandleCommand_JoinClass( "random", false );
				}
			}
		}

		if ( ( tf_arena_use_queue.GetBool() == false && TFGameRules()->IsInWaitingForPlayers() ) || TFGameRules()->State_Get() == GR_STATE_PREGAME  )
		{
			return;
		}
	}

	int iDesiredClass = GetDesiredPlayerClassIndex();

	if ( iDesiredClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	if ( iDesiredClass == TF_CLASS_RANDOM )
	{
		bRandom = true;

		// Don't let them be the same class twice in a row
		do{
			iDesiredClass = random->RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS );
		} while( iDesiredClass == GetPlayerClass()->GetClassIndex() );
	}

	if ( HasTheFlag() )
	{
		DropFlag();
	}

	if ( GetPlayerClass()->GetClassIndex() != iDesiredClass )
	{
		// clean up any pipebombs/buildings in the world (no explosions)
		m_bSwitchedClass = true;

		RemoveAllOwnedEntitiesFromWorld();

		int iOldClass = GetPlayerClass()->GetClassIndex();

		GetPlayerClass()->Init( iDesiredClass );

		// Don't report class changes if we're random, because it's not a player choice
		if ( !bRandom )
		{
			m_iClassChanges++;
			CTF_GameStats.Event_PlayerChangedClass( this, iOldClass, iDesiredClass );
		}
	}
	else
	{
		m_bSwitchedClass = false;
	}

	m_Shared.RemoveAllCond();
	m_Shared.ResetRageMeter();

	if ( m_bSwitchedClass )
	{
		m_iLastWeaponSlot = 1;
		// Tell all the items we have that we've changed class. Some items need to change model.
		// Also reset KillStreaks
		for ( int i = 0; i < MAX_WEAPONS; i++ )
		{
			CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetWeapon(i);
			if ( pWeapon )
			{
				pWeapon->OnOwnerClassChange();
			}
		}
	}
	else
	{
		if ( IsAlive() )
		{
			if ( GetActiveTFWeapon() )
			{
				m_iActiveWeaponTypePriorToDeath = GetActiveTFWeapon()->GetWeaponID();
			}
			SaveLastWeaponSlot();
		}
	}

	// Any Respawns will reset killstreaks
	m_Shared.ResetStreaks();
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTFWeaponBase *pWpn = (CTFWeaponBase *)GetWeapon( i );
		if ( !pWpn )
			continue;
		pWpn->SetKillStreak( 0 );
	}

	for ( int i = 0; i < GetNumWearables(); ++i )
	{
		CTFWearable* pWearable = dynamic_cast<CTFWearable*>( GetWearable( i ) );
		if ( !pWearable )
			continue;
		pWearable->SetKillStreak( 0 );
	}

	RemoveAllItems( true );

	// Reset ground state for airwalk animations
	SetGroundEntity( NULL );

	// TODO: move this into conditions
	RemoveTeleportEffect();

	// remove invisibility very quickly	
	m_Shared.FadeInvis( 0.1f );

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;

	StateTransition( TF_STATE_ACTIVE );
	Spawn();

	if ( m_bSwitchedClass )
	{
		// reset meter charges to their default values
		m_Shared.SetDefaultItemChargeMeters();
	}

	m_bSwitchedClass = false;
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CTFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Handle cheat commands
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CTFPlayer::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
	case 101:
		{
			if( sv_cheats->GetBool() )
			{
				extern int gEvilImpulse101;
				gEvilImpulse101 = true;

				GiveAmmo( 1000, TF_AMMO_PRIMARY );
				GiveAmmo( 1000, TF_AMMO_SECONDARY );
				GiveAmmo( 1000, TF_AMMO_METAL );
				GiveAmmo( 1000, TF_AMMO_GRENADES1 );
				GiveAmmo( 1000, TF_AMMO_GRENADES2 );
				GiveAmmo( 1000, TF_AMMO_GRENADES3 );
				TakeHealth( 999, DMG_GENERIC );

				// Refills weapon clips, too
				for ( int i = 0; i < MAX_WEAPONS; i++ )
				{
					CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( GetWeapon( i ) );
					if ( !pWeapon )
						continue;

					pWeapon->GiveDefaultAmmo();

					pWeapon->WeaponRegenerate();
				}

				m_Shared.m_flRageMeter = 100.f;
				m_Shared.SetDemomanChargeMeter( 100.f );

				for( int i = FIRST_LOADOUT_SLOT_WITH_CHARGE_METER; i <= LAST_LOADOUT_SLOT_WITH_CHARGE_METER; ++i )
				{
					m_Shared.SetItemChargeMeter( (loadout_positions_t)i, 100.f );
				}

				gEvilImpulse101 = false;
			}
		}
		break;

	default:
		{
			BaseClass::CheatImpulseCommands( iImpulse );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetWeaponBuilder( CTFWeaponBuilder *pBuilder )
{
	m_hWeaponBuilder = pBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder *CTFPlayer::GetWeaponBuilder( void )
{
	Assert( 0 );
	return m_hWeaponBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this player is building something
//-----------------------------------------------------------------------------
bool CTFPlayer::IsBuilding( void )
{
	/*
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
		return pBuilder->IsBuilding();
		*/

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveBuildResources( int iAmount )
{
	RemoveAmmo( iAmount, TF_AMMO_METAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AddBuildResources( int iAmount )
{
	GiveAmmo( iAmount, TF_AMMO_METAL, false, kAmmoSource_Pickup );	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject	*CTFPlayer::GetObject( int index ) const
{
	return (CBaseObject *)( m_aObjects[index].Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject	*CTFPlayer::GetObjectOfType( int iObjectType, int iObjectMode ) const
{
	int iNumObjects = GetObjectCount();
	for ( int i=0; i<iNumObjects; i++ )
	{
		CBaseObject *pObj = GetObject(i);

		if ( !pObj )
			continue;

		if ( pObj->GetType() != iObjectType )
			continue;

		if ( pObj->GetObjectMode() != iObjectMode )
			continue;

		if ( pObj->IsDisposableBuilding() )
			continue;

		return pObj;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayer::GetObjectCount( void ) const
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Remove all the player's objects
//			If bExplodeBuildings is not set, remove all of them immediately.
//			Otherwise, make them all explode.
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllObjects( bool bExplodeBuildings /* = false */ )
{
	// Remove all the player's objects
	for (int i = GetObjectCount()-1; i >= 0; i--)
	{
		CBaseObject *obj = GetObject(i);
		Assert( obj );

		if ( obj )
		{
			// this is separate from the object_destroyed event, which does
			// not get sent when we remove the objects from the world
			IGameEvent *event = gameeventmanager->CreateEvent( "object_removed" );	
			if ( event )
			{
				event->SetInt( "userid", GetUserID() ); // user ID of the object owner
				event->SetInt( "objecttype", obj->GetType() ); // type of object removed
				event->SetInt( "index", obj->entindex() ); // index of the object removed
				gameeventmanager->FireEvent( event );
			}

			if ( bExplodeBuildings )
			{
				obj->DetonateObject();
			}
			else
			{
				// This fixes a bug in Raid mode where we could spawn where our sentry was but 
				// we didn't get the weapons because they couldn't trace to us in FVisible
				obj->SetSolid( SOLID_NONE );
				UTIL_Remove( obj );
			}
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StopPlacement( void )
{
	/*
	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->StopPlacement();
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Player has started building an object
//-----------------------------------------------------------------------------
int	CTFPlayer::StartedBuildingObject( int iObjectType )
{
	// Deduct the cost of the object
	int iCost = m_Shared.CalculateObjectCost( this, iObjectType );
	if ( iCost > GetBuildResources() )
	{
		// Player must have lost resources since he started placing
		return 0;
	}

	RemoveBuildResources( iCost );

	// If the object costs 0, we need to return non-0 to mean success
	if ( !iCost )
		return 1;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Player has aborted building something
//-----------------------------------------------------------------------------
void CTFPlayer::StoppedBuilding( int iObjectType )
{
	/*
	int iCost = CalculateObjectCost( iObjectType );

	AddBuildResources( iCost );

	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->StoppedBuilding( iObjectType );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Object has been built by this player
//-----------------------------------------------------------------------------
void CTFPlayer::FinishedObject( CBaseObject *pObject )
{
	AddObject( pObject );

	CTF_GameStats.Event_PlayerCreatedBuilding( this, pObject );

	if ( TFGameRules() && TFGameRules()->IsInTraining() && TFGameRules()->GetTrainingModeLogic() && IsFakeClient() == false )
	{
		TFGameRules()->GetTrainingModeLogic()->OnPlayerBuiltBuilding( this, pObject );
	}

	/*
	// Tell our builder weapon
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->FinishedObject();
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified object to this player's object list.
//-----------------------------------------------------------------------------
void CTFPlayer::AddObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::AddObject adding object %p:%s to player %s\n", gpGlobals->curtime, pObject, pObject->GetClassname(), GetPlayerName() ) );

	bool alreadyInList = PlayerOwnsObject( pObject );
	// Assert( !alreadyInList );
	if ( !alreadyInList )
	{
		m_aObjects.AddToTail( pObject );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Object built by this player has been destroyed
//-----------------------------------------------------------------------------
void CTFPlayer::OwnedObjectDestroyed( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::OwnedObjectDestroyed player %s object %p:%s\n", gpGlobals->curtime, 
		GetPlayerName(),
		pObject,
		pObject->GetClassname() ) );

	RemoveObject( pObject );

	// Tell our builder weapon so it recalculates the state of the build icons
	/*
	CTFWeaponBuilder *pBuilder = GetWeaponBuilder();
	if ( pBuilder )
	{
		pBuilder->RecalcState();
	}
	*/
}

//-----------------------------------------------------------------------------
// Removes an object from the player
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveObject( CBaseObject *pObject )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseTFPlayer::RemoveObject %p:%s from player %s\n", gpGlobals->curtime, 
		pObject,
		pObject->GetClassname(),
		GetPlayerName() ) );

	Assert( pObject );

	int i;
	for ( i = m_aObjects.Count(); --i >= 0; )
	{
		// Also, while we're at it, remove all other bogus ones too...
		if ( (!m_aObjects[i].Get()) || (m_aObjects[i] == pObject))
		{
			m_aObjects.FastRemove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// See if the player owns this object
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayerOwnsObject( CBaseObject *pObject )
{
	return ( m_aObjects.Find( pObject ) != -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayFlinch( const CTakeDamageInfo &info )
{
	// Don't play flinches if we just died. 
	if ( !IsAlive() )
		return;

	// No pain flinches while disguised, our man has supreme discipline
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

	PlayerAnimEvent_t flinchEvent;

	switch ( LastHitGroup() )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchEvent = PLAYERANIMEVENT_FLINCH_HEAD;
		break;
	case HITGROUP_LEFTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchEvent = PLAYERANIMEVENT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_STOMACH:
	case HITGROUP_CHEST:
	case HITGROUP_GEAR:
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchEvent = PLAYERANIMEVENT_FLINCH_CHEST;
		break;
	}

	DoAnimationEvent( flinchEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Plays the crit sound that players that get crit hear
//-----------------------------------------------------------------------------
float CTFPlayer::PlayCritReceivedSound( void )
{
	float flCritPainLength = 0;
	// Play a custom pain sound to the guy taking the damage
	CSingleUserRecipientFilter receiverfilter( this );
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = "TFPlayer.CritPain";
	params.m_pflSoundDuration = &flCritPainLength;
	EmitSound( receiverfilter, entindex(), params );

	return flCritPainLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PainSound( const CTakeDamageInfo &info )
{
	// Don't make sounds if we just died. DeathSound will handle that.
	if ( !IsAlive() )
		return;

	// no pain sounds while disguised, our man has supreme discipline
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

	if ( m_flNextPainSoundTime > gpGlobals->curtime )
		return;

	// play death sound as if we're taking huge damage when we landed on the ground
	if ( info.GetDamageType() & DMG_FALL )
	{
		CBaseEntity *pGround = GetGroundEntity();

		// don't play sound for fall stomp event
		if ( !( pGround && pGround->IsPlayer() && m_Shared.CanFallStomp() ) )
		{
			TFPlayerClassData_t *pData = GetPlayerClass()->GetData();
			if ( pData )
			{
				EmitSound( pData->GetDeathSound( DEATH_SOUND_GENERIC ) );
			}
		}
		return;
	}

	// No sound for DMG_GENERIC
	if ( info.GetDamageType() == 0 || info.GetDamageType() == DMG_PREVENT_PHYSICS_FORCE )
		return;

	if ( info.GetDamageType() & DMG_DROWN )
	{
		EmitSound( "TFPlayer.Drown" );
		return;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		// Looping fire pain sound is done in CTFPlayerShared::ConditionThink
		return;
	}

	float flPainLength = 0;

	bool bAttackerIsPlayer = ( info.GetAttacker() && info.GetAttacker()->IsPlayer() );

	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );

	pExpresser->AllowMultipleScenes();

	// speak a pain concept here, send to everyone but the attacker
	CPASFilter filter( GetAbsOrigin() );

	if ( bAttackerIsPlayer )
	{
		filter.RemoveRecipient( ToBasePlayer( info.GetAttacker() ) );
	}

	// play a crit sound to the victim ( us )
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		flPainLength = PlayCritReceivedSound();

		// remove us from hearing our own pain sound if we hear the crit sound
		filter.RemoveRecipient( this );
	}

	char szResponse[AI_Response::MAX_RESPONSE_NAME];

	if ( SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &filter ) )
	{
		flPainLength = MAX( GetSceneDuration( szResponse ), flPainLength );
	}

	// speak a louder pain concept to just the attacker
	if ( bAttackerIsPlayer )
	{
		CSingleUserRecipientFilter attackerFilter( ToBasePlayer( info.GetAttacker() ) );
		SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_ATTACKER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &attackerFilter );
	}

	pExpresser->DisallowMultipleScenes();

	m_flNextPainSoundTime = gpGlobals->curtime + flPainLength;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DeathSound( const CTakeDamageInfo &info )
{
	// Don't make death sounds when choosing a class
	if ( IsPlayerClass( TF_CLASS_UNDEFINED ) )
		return;

	TFPlayerClassData_t *pData = GetPlayerClass()->GetData();
	if ( !pData )
		return;

	if ( m_bGoingFeignDeath  )
	{
		bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED ) && (m_Shared.GetDisguiseTeam() == GetTeamNumber());
		if ( bDisguised )
		{
			// Use our disguise class, if we have one and will drop a disguise class corpse.
			pData = g_pTFPlayerClassDataMgr->Get( m_Shared.GetDisguiseClass() );
			if ( !pData )
				return;
		}
	}

	CTFPlayer *pAttacker = (CTFPlayer*)ToTFPlayer( info.GetAttacker() );
	if ( pAttacker )
	{
		CTFWeaponBase *pWpn = pAttacker->GetActiveTFWeapon();
		if ( pWpn && pWpn->IsSilentKiller() )
			return;
	}

	int nDeathSoundOffset = DEATH_SOUND_FIRST;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		nDeathSoundOffset = IsMiniBoss() ? DEATH_SOUND_GIANT_MVM_FIRST : DEATH_SOUND_MVM_FIRST;
	}
	
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && 
		 GetTeamNumber() != TF_TEAM_PVE_INVADERS && !m_bGoingFeignDeath )
	{
		EmitSound( "MVM.PlayerDied" );
		return;
	}

	if ( m_LastDamageType & DMG_FALL ) // Did we die from falling?
	{
		// They died in the fall. Play a splat sound.
		EmitSound( "Player.FallGib" );
	}
	else if ( m_LastDamageType & DMG_BLAST )
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_EXPLOSION + nDeathSoundOffset ) );
	}
	else if ( m_LastDamageType & DMG_CRITICAL )
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_CRIT + nDeathSoundOffset ) );

		PlayCritReceivedSound();
	}
	else if ( m_LastDamageType & DMG_CLUB )
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_MELEE + nDeathSoundOffset ) );
	}
	else
	{
		EmitSound( pData->GetDeathSound( DEATH_SOUND_GENERIC + nDeathSoundOffset ) );
	}

	// Play an additional sound when we're in MvM and have a boss death
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && IsMiniBoss() )
	{
		switch ( GetPlayerClass()->GetClassIndex() )
		{
			case TF_CLASS_HEAVYWEAPONS:
			{
				EmitSound( "MVM.GiantHeavyExplodes" );
				break;
			}
			default:
			{
				EmitSound( "MVM.GiantCommonExplodes" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFPlayer::GetSceneSoundToken( void )
{
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		if ( IsMiniBoss() )
		{
			return "M_MVM_";
		}
		else
		{
			return "MVM_";
		}
	}
	else
	{
		return "";
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StunSound( CTFPlayer* pAttacker, int iStunFlags, int iOldStunFlags )
{
	if ( !IsAlive() )
		return;

	if ( (iStunFlags & TF_STUN_BY_TRIGGER) && (iOldStunFlags != 0) )
		return; // Only play stun triggered sounds when not already stunned.

	// Play the stun sound for everyone but the attacker.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );

	pExpresser->AllowMultipleScenes();

	float flStunSoundLength = 0;
	EmitSound_t params;
	params.m_flSoundTime = 0;
	if ( iStunFlags & TF_STUN_SPECIAL_SOUND )
	{
		params.m_pSoundName = "TFPlayer.StunImpactRange";
	}
	else if ( (iStunFlags & TF_STUN_LOSER_STATE) && !pAttacker )
	{
		params.m_pSoundName = "Halloween.PlayerScream";
	}
	else
	{
		params.m_pSoundName = "TFPlayer.StunImpact";
	}
	params.m_pflSoundDuration = &flStunSoundLength;

	if ( pAttacker )
	{
		CPASFilter filter( GetAbsOrigin() );
		filter.RemoveRecipient( pAttacker );
		EmitSound( filter, entindex(), params );

		// Play a louder pain sound for the person who got the stun.
		CSingleUserRecipientFilter attackerFilter( pAttacker );
		EmitSound( attackerFilter, pAttacker->entindex(), params );
	}
	else
	{
		EmitSound( params.m_pSoundName );
	}

	pExpresser->DisallowMultipleScenes();

	// Suppress any pain sound that might come right after this stun sound.
	m_flNextPainSoundTime = gpGlobals->curtime + 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: called when this player burns another player
//-----------------------------------------------------------------------------
void CTFPlayer::OnBurnOther( CTFPlayer *pTFPlayerVictim, CTFWeaponBase *pWeapon )
{
#define ACHIEVEMENT_BURN_TIME_WINDOW	30.0f
#define ACHIEVEMENT_BURN_VICTIMS	5
	// add current time we burned another player to head of vector
	m_aBurnOtherTimes.AddToHead( gpGlobals->curtime );

	// remove any burn times that are older than the burn window from the list
	float flTimeDiscard = gpGlobals->curtime - ACHIEVEMENT_BURN_TIME_WINDOW;
	for ( int i = 1; i < m_aBurnOtherTimes.Count(); i++ )
	{
		if ( m_aBurnOtherTimes[i] < flTimeDiscard )
		{
			m_aBurnOtherTimes.RemoveMultiple( i, m_aBurnOtherTimes.Count() - i );
			break;
		}
	}

	// see if we've burned enough players in time window to satisfy achievement
	if ( m_aBurnOtherTimes.Count() >= ACHIEVEMENT_BURN_VICTIMS )
	{
		AwardAchievement( ACHIEVEMENT_TF_BURN_PLAYERSINMINIMIMTIME );
	}

	// ACHIEVEMENT_TF_PYRO_KILL_SPIES - Awarded for igniting enemy spies who have active sappers on friendly building
	if ( pTFPlayerVictim->IsPlayerClass(TF_CLASS_SPY))
	{
		CBaseObject	*pSapper = pTFPlayerVictim->GetObjectOfType( OBJ_ATTACHMENT_SAPPER, 0 );
		if ( pSapper )
		{
			AwardAchievement( ACHIEVEMENT_TF_PYRO_KILL_SPIES );
		}
	}

	// ACHIEVEMENT_TF_PYRO_BURN_RJ_SOLDIER - Pyro ignited a rocket jumping soldier in mid-air
	if ( pTFPlayerVictim->IsPlayerClass(TF_CLASS_SOLDIER) )
	{
		if ( pTFPlayerVictim->RocketJumped() && !pTFPlayerVictim->GetGroundEntity() )
		{
			AwardAchievement( ACHIEVEMENT_TF_PYRO_BURN_RJ_SOLDIER );
		}
	}

	// ACHIEVEMENT_TF_PYRO_DEFEND_POINTS - Pyro kills targets capping control points
	CTriggerAreaCapture *pAreaTrigger = pTFPlayerVictim->GetControlPointStandingOn();
	if ( pAreaTrigger )
	{
		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP && pCP->GetOwner() == GetTeamNumber() )
		{
			if ( TeamplayGameRules()->TeamMayCapturePoint( pTFPlayerVictim->GetTeamNumber(), pCP->GetPointIndex() ) && 
				TeamplayGameRules()->PlayerMayCapturePoint( pTFPlayerVictim, pCP->GetPointIndex() ) )
			{
				AwardAchievement( ACHIEVEMENT_TF_PYRO_DEFEND_POINTS );
			}
		}
	}

	// ACHIEVEMENT_TF_MEDIC_ASSIST_PYRO
	// if we're invuln, let the medic know that we burned someone
	if ( m_Shared.InCond( TF_COND_INVULNERABLE ) || m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
	{
		int i;
		int iNumHealers = m_Shared.GetNumHealers();

		for ( i=0;i<iNumHealers;i++ )
		{
			// Send a message to all medics invulning the Pyro at this time
			CTFPlayer *pMedic = ToTFPlayer( m_Shared.GetHealerByIndex( i ) );
			if ( pMedic && pMedic->GetChargeEffectBeingProvided() == MEDIGUN_CHARGE_INVULN )
			{
				// Tell the clients involved in the ignition
				CSingleUserRecipientFilter medic_filter( pMedic );
				UserMessageBegin( medic_filter, "PlayerIgnitedInv" );
					WRITE_BYTE( entindex() );
					WRITE_BYTE( pTFPlayerVictim->entindex() );
					WRITE_BYTE( pMedic->entindex() );
				MessageEnd();
			}
		}
	}

	// Tell the clients involved in the ignition
	CRecipientFilter involved_filter;
	involved_filter.AddRecipient( this );
	involved_filter.AddRecipient( pTFPlayerVictim );
	UserMessageBegin( involved_filter, "PlayerIgnited" );
		WRITE_BYTE( entindex() );
		WRITE_BYTE( pTFPlayerVictim->entindex() );
		WRITE_BYTE( pWeapon ? pWeapon->GetWeaponID() : 0 );
	MessageEnd();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_ignited" );
	if ( event )
	{
		event->SetInt( "pyro_entindex", entindex() );
		event->SetInt( "victim_entindex", pTFPlayerVictim->entindex() );
		event->SetInt( "weaponid", pWeapon ? pWeapon->GetWeaponID() : 0 );
		gameeventmanager->FireEvent( event, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the player is capturing a point.
//-----------------------------------------------------------------------------
bool CTFPlayer::IsCapturingPoint()
{
	CTriggerAreaCapture *pAreaTrigger = GetControlPointStandingOn();
	if ( pAreaTrigger )
	{
		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP )
		{
			if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
				TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
			{
				// if we own this point, we're no longer "capturing" it
				return pCP->GetOwner() != GetTeamNumber();
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeam *CTFPlayer::GetTFTeam( void )
{
	CTFTeam *pTeam = dynamic_cast<CTFTeam *>( GetTeam() );
	Assert( pTeam );
	return pTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeam *CTFPlayer::GetOpposingTFTeam( void )
{
	if ( TFTeamMgr() )
	{
		int iTeam = GetTeamNumber();
		if ( iTeam == TF_TEAM_RED )
		{
			return TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			return TFTeamMgr()->GetTeam( TF_TEAM_RED );
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Give this player the "i just teleported" effect for 12 seconds
//-----------------------------------------------------------------------------
void CTFPlayer::TeleportEffect( void )
{
	m_Shared.AddCond( TF_COND_TELEPORTED );

	float flDuration = 12.f;
	if ( TFGameRules()->IsMannVsMachineMode() && m_bIsABot && IsBotOfType( TF_BOT_TYPE ) )
	{
		flDuration = 30.f;
	}

	// Also removed on death
	SetContextThink( &CTFPlayer::RemoveTeleportEffect, gpGlobals->curtime + flDuration, "TFPlayer_TeleportEffect" );
}

//-----------------------------------------------------------------------------
// Purpose: Remove the teleporter effect
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveTeleportEffect( void )
{
	m_Shared.RemoveCond( TF_COND_TELEPORTED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::StopRagdollDeathAnim( void )
{
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );
	if ( pRagdoll )
	{
		pRagdoll->m_iDamageCustom = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::CreateRagdollEntity( void )
{
	CreateRagdollEntity( false, false, false, false, false, false, false, false );
}

//-----------------------------------------------------------------------------
// Purpose: Create a ragdoll entity to pass to the client.
//-----------------------------------------------------------------------------
void CTFPlayer::CreateRagdollEntity( bool bGib, bool bBurning, bool bElectrocuted, bool bOnGround, bool bCloakedCorpse, bool bGoldRagdoll, bool bIceRagdoll, bool bBecomeAsh, int iDamageCustom, bool bCritOnHardHit )
{
	// If we already have a ragdoll destroy it.
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	// Create a ragdoll.
	pRagdoll = dynamic_cast<CTFRagdoll*>( CreateEntityByName( "tf_ragdoll" ) );
	if ( pRagdoll )
	{
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_vecForce = m_vecForce;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_bGib = bGib;
		pRagdoll->m_bBurning = bBurning;
		pRagdoll->m_bElectrocuted = bElectrocuted;
		pRagdoll->m_bOnGround = bOnGround;
		pRagdoll->m_bCloaked = bCloakedCorpse;
		pRagdoll->m_iDamageCustom = iDamageCustom;
		pRagdoll->m_iTeam = GetTeamNumber();
		pRagdoll->m_iClass = GetPlayerClass()->GetClassIndex();
		pRagdoll->m_bGoldRagdoll = bGoldRagdoll;
		pRagdoll->m_bIceRagdoll = bIceRagdoll;
		pRagdoll->m_bBecomeAsh = bBecomeAsh;
		pRagdoll->m_bCritOnHardHit = bCritOnHardHit;
		pRagdoll->m_flHeadScale = m_flHeadScale;
		pRagdoll->m_flTorsoScale = m_flTorsoScale;
		pRagdoll->m_flHandScale = m_flHandScale;
	}

	// Turn off the player.
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW | EF_NOSHADOW );
	SetMoveType( MOVETYPE_NONE );

	// Add additional gib setup.
	if ( bGib )
	{
		m_nRenderFX = kRenderFxRagdoll;
	}

	// Save ragdoll handle.
	m_hRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy's a ragdoll, called with a player is disconnecting.
//-----------------------------------------------------------------------------
void CTFPlayer::DestroyRagdoll( void )
{
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}

	// Remove the feign death ragdoll at the same time.
	pRagdoll = dynamic_cast<CTFRagdoll*>( m_hFeignRagdoll.Get() );	
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The player appears to die, creating a corpse and silently stealthing.
//			Occurs when a player takes damage with the dead ringer active
//-----------------------------------------------------------------------------
void CTFPlayer::SpyDeadRingerDeath( const CTakeDamageInfo& info )
{
	// Can't feign death if we're actually dead or if we're not a spy.
	if ( !IsAlive() || !IsPlayerClass( TF_CLASS_SPY ) )
		return;

	// Can't feign death if we're already stealthed.
	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
		return;

	// Can't feign death if we aren't at full cloak energy.
	if ( !CanGoInvisible( true ) || ( m_Shared.GetSpyCloakMeter() < 100.0f ) )
		return;

	m_Shared.SetSpyCloakMeter( 50.0f );

	m_bGoingFeignDeath = true; 

	FeignDeath( info, true );

	// Go feign death.
	m_Shared.AddCond( TF_COND_FEIGN_DEATH, tf_feign_death_duration.GetFloat() );
	m_bGoingFeignDeath = false;
}

//-----------------------------------------------------------------------------
// Purpose: The player appears to die, creating a corpse
//-----------------------------------------------------------------------------
void CTFPlayer::FeignDeath( const CTakeDamageInfo& info, bool bDeathnotice )
{
	if ( HasTheFlag() )
	{
		DropFlag();
	}

	// Dead Ringer death removes Powerup Rune for authenticity
	DropRune();

	// Only drop disguised ragdoll & weapon if we're disguised as a teammate.
	bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED ) && (m_Shared.GetDisguiseTeam() == GetTeamNumber());

	// We want the ragdoll to burn if the player was burning and was not disguised as a pyro.
	bool bBurning = m_Shared.InCond( TF_COND_BURNING ) && (!bDisguised || (TF_CLASS_PYRO != m_Shared.GetDisguiseClass()));

	// Stop us from burning and other effects that would give the game away.
	m_Shared.RemoveCond( TF_COND_BURNING );
	m_Shared.RemoveCond( TF_COND_BLEEDING );
	RemoveTeleportEffect();

	// Fake death audio.
	EmitSound( "BaseCombatCharacter.StopWeaponSounds" );
	SpeakConceptIfAllowed( MP_CONCEPT_DIED );
	DeathSound( info );

	// Check if we should create gibs.
	bool bGib = ShouldGib( info );

	SetGibbedOnLastDeath( bGib );

	if ( bDeathnotice )
	{
		// Fake death notice.
		TFGameRules()->DeathNotice( this, info );
	}

	// Drop an empty ammo pack!
	if ( ShouldDropAmmoPack() )
	{
		DropAmmoPack( info, true /*Empty*/, bDisguised );
	}

	if ( TFGameRules()->IsInMedievalMode() )
	{
		DropHealthPack( info, true );
	}

	if ( GetActiveTFWeapon() )
	{
		int iDropHealthOnKill = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( GetActiveTFWeapon(), iDropHealthOnKill, drop_health_pack_on_kill );
		if ( iDropHealthOnKill == 1 )
		{
			DropHealthPack( info, true );
		}
	}

	CTFPlayer *pTFPlayer = ToTFPlayer( info.GetAttacker() );
	if ( pTFPlayer )
	{
		int iKillForcesAttackerToLaugh = 0;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFPlayer, iKillForcesAttackerToLaugh, kill_forces_attacker_to_laugh );
		if ( iKillForcesAttackerToLaugh == 1 )
		{
			// force the attacker to laugh!
			pTFPlayer->Taunt( TAUNT_MISC_ITEM, MP_CONCEPT_TAUNT_LAUGH );
		}

		CTFWeaponInvis *pWpn = (CTFWeaponInvis *)Weapon_OwnsThisID( TF_WEAPON_INVIS );
		if ( pWpn && pWpn->HasFeignDeath() )
		{
			DropDeathCallingCard( pTFPlayer, this );
		}

		// Check for Halloween Death Ghosts
		pTFPlayer->CheckSpellHalloweenDeathGhosts( info, this );
	}

	// Create a ragdoll.
	CreateFeignDeathRagdoll( info, bGib, bBurning, bDisguised );

	// Note that we succeeded for stats tracking.
	EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( GetEntityForLoadoutSlot( LOADOUT_POSITION_PDA2 ) ),
									  this,
									  pTFPlayer,			// in this case the "victim" is the person doing the damage
									  kKillEaterEvent_DeathsFeigned );
}

//-----------------------------------------------------------------------------
// Purpose: Create a ragdoll entity for feign death. Does not hide the player.
// Creates an entirely separate ragdoll that isn't used for client death cam or other real death stuff.
//-----------------------------------------------------------------------------
void CTFPlayer::CreateFeignDeathRagdoll( const CTakeDamageInfo& info, bool bGib, bool bBurning, bool bDisguised )
{
	// If we already have a feigning ragdoll destroy it.
	CTFRagdoll *pRagdoll = dynamic_cast<CTFRagdoll*>( m_hFeignRagdoll.Get() );
	if( pRagdoll )
	{
		UTIL_Remove( pRagdoll );
		pRagdoll = NULL;
	}
	Assert( pRagdoll == NULL );

	// Create a ragdoll.
	pRagdoll = dynamic_cast<CTFRagdoll*>( CreateEntityByName( "tf_ragdoll" ) );
	if ( pRagdoll )
	{
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = m_vecFeignDeathVelocity;
		pRagdoll->m_vecForce = CalcDamageForceVector( info );
		pRagdoll->m_nForceBone = m_nForceBone;
		Assert( entindex() >= 1 && entindex() <= MAX_PLAYERS );
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_bGib = bGib;
		pRagdoll->m_bBurning = bBurning;
		pRagdoll->m_bElectrocuted = false;
		pRagdoll->m_bFeignDeath = true;
		pRagdoll->m_bWasDisguised = bDisguised;
		pRagdoll->m_bBecomeAsh = false;
		pRagdoll->m_bOnGround = (bool) (GetFlags() & FL_ONGROUND);
		pRagdoll->m_iDamageCustom = info.GetDamageCustom();
		pRagdoll->m_bCritOnHardHit = false;
		pRagdoll->m_flHeadScale = m_flHeadScale;
		pRagdoll->m_flTorsoScale = m_flTorsoScale;
		pRagdoll->m_flHandScale = m_flHandScale;

		{
			int iGoldRagdoll = 0;
			if ( info.GetWeapon() )
			{
				 CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iGoldRagdoll, set_turn_to_gold );
			}
			pRagdoll->m_bGoldRagdoll = iGoldRagdoll != 0;

			int iIceRagdoll = 0;
			if ( info.GetWeapon() )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iIceRagdoll, set_turn_to_ice );
			}
			pRagdoll->m_bIceRagdoll = iIceRagdoll != 0;

			int iRagdollsBecomeAsh = 0;
			if ( info.GetWeapon() )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iRagdollsBecomeAsh, ragdolls_become_ash );
			}
			pRagdoll->m_bBecomeAsh = iRagdollsBecomeAsh != 0;

			int iRagdollsPlasmaEffect = 0;
			if ( info.GetWeapon() )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iRagdollsPlasmaEffect, ragdolls_plasma_effect );
			}
			if ( iRagdollsPlasmaEffect )
			{
				pRagdoll->m_iDamageCustom = TF_DMG_CUSTOM_PLASMA;
			}

			int iCritOnHardHit = 0;
			if ( info.GetWeapon() )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( info.GetWeapon(), iCritOnHardHit, crit_on_hard_hit );
			}
			pRagdoll->m_bCritOnHardHit = iCritOnHardHit != 0;
		}

		// If we are disguised, make the ragdoll look like our disguise.
		if ( bDisguised )
		{
			pRagdoll->m_iTeam = m_Shared.GetDisguiseTeam();
			pRagdoll->m_iClass = m_Shared.GetDisguiseClass();
		}
		else
		{
			pRagdoll->m_iTeam = GetTeamNumber();
			pRagdoll->m_iClass = GetPlayerClass()->GetClassIndex();
		}
	}

	// Exaggerate ragdoll velocity if recently hit by blast damage.
	if ( !bGib && ( info.GetDamageType() & DMG_BLAST ) )
	{
		Vector vForceModifier = info.GetDamageForce();
		vForceModifier.x *= 1.5;
		vForceModifier.y *= 1.5;
		vForceModifier.z *= 1;
		pRagdoll->m_vecForce = vForceModifier;
	}

	// Save ragdoll handle.
	m_hFeignRagdoll = pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_FrameUpdate( void )
{
	BaseClass::Weapon_FrameUpdate();

	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		m_hOffHandWeapon->Operator_FrameUpdate( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::Weapon_HandleAnimEvent( pEvent );

	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Operator_HandleAnimEvent( pEvent, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity ) 
{

}

//-----------------------------------------------------------------------------
// Purpose: Call this when this player fires a weapon to allow other systems to react
//-----------------------------------------------------------------------------
void CTFPlayer::OnMyWeaponFired( CBaseCombatWeapon *weapon )
{
	BaseClass::OnMyWeaponFired( weapon );

	// mark region as 'in combat'
	if ( m_inCombatThrottleTimer.IsElapsed() )
	{
		CTFWeaponBase *tfWeapon = static_cast< CTFWeaponBase * >( weapon );

		if ( !tfWeapon )
		{
			return;
		}

		switch ( tfWeapon->GetWeaponID() )
		{
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_INVIS:
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_BUFF_ITEM:
		case TF_WEAPON_PUMPKIN_BOMB:
		case TF_WEAPON_WRENCH:			// skip this so engineer building doesn't mark 'in combat'
		case TF_WEAPON_PDA_SPY_BUILD:
			// not a 'combat' weapon
			return;
		};

		// important to keep this at one second, so rate cvars make sense (units/sec)
		m_inCombatThrottleTimer.Start( 1.0f );

		// only search up/down StepHeight as a cheap substitute for line of sight
		CUtlVector< CNavArea * > nearbyAreaVector;
		CollectSurroundingAreas( &nearbyAreaVector, GetLastKnownArea(), tf_nav_in_combat_range.GetFloat(), StepHeight, StepHeight );

		for( int i=0; i<nearbyAreaVector.Count(); ++i )
		{
			static_cast< CTFNavArea * >( nearbyAreaVector[i] )->OnCombat();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Remove invisibility, called when player attacks
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveInvisibility( void )
{
	if ( !m_Shared.IsStealthed() )
		return;

	// remove quickly
	CTFPlayer *pProvider = ToTFPlayer( m_Shared.GetConditionProvider( TF_COND_STEALTHED_USER_BUFF ) );
	bool bAEStealth = ( m_Shared.InCond( TF_COND_STEALTHED_USER_BUFF ) && 
						pProvider && 
						( pProvider->IsPlayerClass( TF_CLASS_SPY ) ? true : false ) &&
						( pProvider != this ) );
	if ( m_Shared.InCond( TF_COND_STEALTHED_USER_BUFF ) )
	{
		m_Shared.AddCond( TF_COND_STEALTHED_USER_BUFF_FADING, ( bAEStealth ) ? 4.f : 0.5f );
	}

	m_Shared.FadeInvis( bAEStealth ? 2.f : 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SayAskForBall() 
{
	if ( !TFGameRules() || !TFGameRules()->IsPasstimeMode() 
		|| ( m_Shared.AskForBallTime() > gpGlobals->curtime ) )
	{
		return false;
	}

	CPasstimeBall *pBall = g_pPasstimeLogic->GetBall();
	if ( !pBall )
	{
		return false;
	}

	CTFPlayer *pBallCarrier = pBall->GetCarrier();
	if ( !pBallCarrier )
	{
		return false;
	}

	HudNotification_t cantCarryReason;
	if ( !CPasstimeGun::BValidPassTarget( pBallCarrier, this, &cantCarryReason ) )
	{
		if ( cantCarryReason ) 
		{
			CSingleUserReliableRecipientFilter filter( this );
			TFGameRules()->SendHudNotification( filter, cantCarryReason );
		}
		return false;
	}

	CRecipientFilter filter;
	filter.AddRecipient( this );
	filter.AddRecipient( pBallCarrier );
	filter.MakeReliable();
	EmitSound( filter, entindex(), "Passtime.AskForBall" );

	++CTF_GameStats.m_passtimeStats.summary.nTotalPassRequests;
	m_Shared.SetAskForBallTime( gpGlobals->curtime + 5.0f );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SaveMe( void )
{
	if ( !IsAlive() || IsPlayerClass( TF_CLASS_UNDEFINED ) || GetTeamNumber() < TF_TEAM_RED )
		return;

	m_bSaveMeParity = !m_bSaveMeParity;
}

//-----------------------------------------------------------------------------
// Purpose: drops the flag
//-----------------------------------------------------------------------------
void CC_DropItem( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.IsCarryingRune() )
	{
		pPlayer->DropRune();
		return;
	}
	
	if ( pPlayer->HasTheFlag() )
	{
		pPlayer->DropFlag();
	}
}
static ConCommand dropitem( "dropitem", CC_DropItem, "Drop the flag." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObserverPoint::CObserverPoint()
{
	m_bMatchSummary = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObserverPoint::Activate( void )
{
	BaseClass::Activate();

	if ( m_bMatchSummary )
	{
		// sanity check to make sure the competitive match summary target is disabled until we're ready for it
		SetDisabled( true );
	}

	if ( m_iszAssociateTeamEntityName != NULL_STRING )
	{
		m_hAssociatedTeamEntity = gEntList.FindEntityByName( NULL, m_iszAssociateTeamEntityName );
		if ( !m_hAssociatedTeamEntity )
		{
			Warning("info_observer_point (%s) couldn't find associated team entity named '%s'\n", GetDebugName(), STRING(m_iszAssociateTeamEntityName) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObserverPoint::CanUseObserverPoint( CTFPlayer *pPlayer )
{
	if ( m_bDisabled )
		return false;

	// Only spectate observer points on control points in the current miniround
	if ( g_pObjectiveResource->PlayingMiniRounds() && m_hAssociatedTeamEntity )
	{
		CTeamControlPoint *pPoint = dynamic_cast<CTeamControlPoint*>(m_hAssociatedTeamEntity.Get());
		if ( pPoint )
		{
			bool bInRound = g_pObjectiveResource->IsInMiniRound( pPoint->GetPointIndex() );
			if ( !bInRound )
				return false;
		}
	}

	if ( m_hAssociatedTeamEntity && mp_forcecamera.GetInt() == OBS_ALLOW_TEAM )
	{
		// don't care about this check during a team win
		if ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
		{
			// If we don't own the associated team entity, we can't use this point
			if ( m_hAssociatedTeamEntity->GetTeamNumber() != pPlayer->GetTeamNumber() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM )
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CObserverPoint::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObserverPoint::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObserverPoint::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

BEGIN_DATADESC( CObserverPoint )
DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
DEFINE_KEYFIELD( m_bDefaultWelcome, FIELD_BOOLEAN, "defaultwelcome" ),
DEFINE_KEYFIELD( m_iszAssociateTeamEntityName, FIELD_STRING, "associated_team_entity" ),
DEFINE_KEYFIELD( m_flFOV, FIELD_FLOAT, "fov" ),
DEFINE_KEYFIELD( m_bMatchSummary, FIELD_BOOLEAN, "match_summary" ),

DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_observer_point, CObserverPoint );

//-----------------------------------------------------------------------------
// Purpose: Builds a list of entities that this player can observe.
//			Returns the index into the list of the player's current observer target.
//-----------------------------------------------------------------------------
int CTFPlayer::BuildObservableEntityList( void )
{
	m_hObservableEntities.Purge();
	int iCurrentIndex = -1;

	// Add all the map-placed observer points
	CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
	while ( pObserverPoint )
	{
		m_hObservableEntities.AddToTail( pObserverPoint );

		if ( m_hObserverTarget.Get() == pObserverPoint )
		{
			iCurrentIndex = (m_hObservableEntities.Count()-1);
		}

		pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}

	// Add all the players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			m_hObservableEntities.AddToTail( pPlayer );

			if ( m_hObserverTarget.Get() == pPlayer )
			{
				iCurrentIndex = (m_hObservableEntities.Count()-1);
			}
		}
	}

	// Add all my objects
	int iNumObjects = GetObjectCount();
	for ( int i = 0; i < iNumObjects; i++ )
	{
		CBaseObject *pObj = GetObject( i );
		if ( pObj )
		{
			m_hObservableEntities.AddToTail( pObj );

			if ( m_hObserverTarget.Get() == pObj )
			{
				iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
			}
		}
	}

#ifdef TF_RAID_MODE
	// Add all of the objects for my team if we're in Raid mode
	if ( TFGameRules() && TFGameRules()->IsRaidMode() )
	{
		CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
		if ( pTeam )
		{
			int nTeamObjectCount = pTeam->GetNumObjects();

			for ( int iObject = 0; iObject < nTeamObjectCount; ++iObject )
			{
				CBaseObject *pObj = pTeam->GetObject( iObject );

				if ( !pObj )
					continue;

				// we've already added our own buildings in the previous loop
				if ( pObj->GetOwner() == this )
					continue;

				m_hObservableEntities.AddToTail( pObj );

				if ( m_hObserverTarget.Get() == pObj )
				{
					iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
				}
			}
		}
	}
#endif // TF_RAID_MODE

	// If there are any team_train_watchers, add the train they are linked to
	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->IsDisabled() )
		{
			CBaseEntity *pTrain = pWatcher->GetTrainEntity();
			if ( pTrain )
			{
				m_hObservableEntities.AddToTail( pTrain );

				if ( m_hObserverTarget.Get() == pTrain )
				{
					iCurrentIndex = (m_hObservableEntities.Count()-1);
				}
			}
		}		

		pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}

	// observe active bosses
	if ( TFGameRules()->GetActiveBoss() )
	{
		m_hObservableEntities.AddToTail( TFGameRules()->GetActiveBoss() );

		if ( m_hObserverTarget.Get() == TFGameRules()->GetActiveBoss() )
		{
			iCurrentIndex = ( m_hObservableEntities.Count() - 1 );
		}
	}

	return iCurrentIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1; 
	int startIndex = BuildObservableEntityList();
	int iMax = m_hObservableEntities.Count()-1;

	startIndex += iDir;
	if (startIndex > iMax)
		startIndex = 0;
	else if (startIndex < 0)
		startIndex = iMax;

	return startIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::FindNextObserverTarget(bool bReverse)
{
	int startIndex = GetNextObserverSearchStartPoint( bReverse );

	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1; 

	int iMax = m_hObservableEntities.Count()-1;

	// Make sure the current index is within the max. Can happen if we were previously
	// spectating an object which has been destroyed.
	if ( startIndex > iMax )
	{
		currentIndex = startIndex = 1;
	}

	do
	{
		CBaseEntity *nextTarget = m_hObservableEntities[currentIndex];

		if ( IsValidObserverTarget( nextTarget ) )
			return nextTarget;	
 
		currentIndex += iDir;

		// Loop through the entities
		if (currentIndex > iMax)
		{
			currentIndex = 0;
		}
		else if (currentIndex < 0)
		{
			currentIndex = iMax;
		}
	} while ( currentIndex != startIndex );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsValidObserverTarget( CBaseEntity * target )
{
	if ( !target || ( target == this ) )
		return false;

	// if we are coaching, the target is always valid
	if ( ( m_hStudent == target ) && target->IsPlayer() )
		return true;

	if ( TFGameRules()->IsPasstimeMode() && ( target == TFGameRules()->GetObjectiveObserverTarget() ) )
		return true;

	if ( !target->IsPlayer() )
	{
		bool bStrictRules = false;
		const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		if ( pMatchDesc )
		{
			bStrictRules = pMatchDesc->BUsesStrictSpectatorRules();
		}
		else
		{
			bStrictRules = ( TFGameRules()->IsInTournamentMode() && !TFGameRules()->IsMannVsMachineMode() );
		}

		if ( bStrictRules )
		{
			CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>( target );
			if ( pObsPoint )
			{
				// just connected, initial observer point is okay
				if ( GetTeamNumber() < TEAM_SPECTATOR )
					return pObsPoint->CanUseObserverPoint( this );
			}
			
			// players only
			return false;
		}

		CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>( target );
		if ( pObsPoint && !pObsPoint->CanUseObserverPoint( this ) )
			return false;

		CFuncTrackTrain *pTrain = dynamic_cast<CFuncTrackTrain *>( target );
		if ( pTrain )
		{
			// can only spec the trains while the round is running
			if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
				return false;
		}

		if ( GetTeamNumber() == TEAM_SPECTATOR )
			return true;

		// active bosses should be valid targets
		if ( target == TFGameRules()->GetActiveBoss() )
		{
			if ( TFGameRules()->GetActiveBoss()->GetBossType() == HALLOWEEN_BOSS_MERASMUS )
			{
				CMerasmus *pMerasmus = assert_cast< CMerasmus* >( TFGameRules()->GetActiveBoss() );
				if ( pMerasmus && pMerasmus->IsHiding() )
					return false;
			}

			return true;
		}
		
		switch ( mp_forcecamera.GetInt() )	
		{
		case OBS_ALLOW_ALL	:	break;
		case OBS_ALLOW_TEAM :	if ( target->GetTeamNumber() != TEAM_UNASSIGNED && GetTeamNumber() != target->GetTeamNumber() )
									return false;
								break;
		case OBS_ALLOW_NONE :	return false;
		}

		return true;
	}

	return BaseClass::IsValidObserverTarget( target );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PickWelcomeObserverPoint( void )
{
	//Don't just spawn at the world origin, find a nice spot to look from while we choose our team and class.
	CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );

	while ( pObserverPoint )
	{
		if ( IsValidObserverTarget( pObserverPoint ) )
		{
			SetObserverTarget( pObserverPoint );
		}

		if ( pObserverPoint->IsDefaultWelcome() )
			break;

		pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SetObserverTarget(CBaseEntity *target)
{
	ClearZoomOwner();
	SetFOV( this, 0 );
		
	if ( !BaseClass::SetObserverTarget(target) )
		return false;

	CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>(target);
	if ( pObsPoint )
	{
		SetViewOffset( vec3_origin );
		JumptoPosition( target->GetAbsOrigin(), target->EyeAngles() );
		SetFOV( pObsPoint, pObsPoint->m_flFOV );
	}

	if ( !m_bArenaIsAFK )
	{
		m_flLastAction = gpGlobals->curtime;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest team member within the distance of the origin.
//			Favor players who are the same class.
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::FindNearestObservableTarget( Vector vecOrigin, float flMaxDist )
{
	CTeam *pTeam = GetTeam();
	CBaseEntity *pReturnTarget = NULL;
	bool bFoundClass = false;
	float flCurDistSqr = (flMaxDist * flMaxDist);
	int iNumPlayers = pTeam->GetNumPlayers();

	if ( pTeam->GetTeamNumber() == TEAM_SPECTATOR )
	{
		iNumPlayers = gpGlobals->maxClients;
	}


	for ( int i = 0; i < iNumPlayers; i++ )
	{
		CTFPlayer *pPlayer = NULL;

		if ( pTeam->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		}
		else
		{
			pPlayer = ToTFPlayer( pTeam->GetPlayer(i) );
		}

		if ( !pPlayer )
			continue;

		if ( !IsValidObserverTarget(pPlayer) )
			continue;

		float flDistSqr = ( pPlayer->GetAbsOrigin() - vecOrigin ).LengthSqr();

		if ( flDistSqr < flCurDistSqr )
		{
			// If we've found a player matching our class already, this guy needs
			// to be a matching class and closer to boot.
			if ( !bFoundClass || pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;

				if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
				{
					bFoundClass = true;
				}
			}
		}
		else if ( !bFoundClass )
		{
			if ( pPlayer->IsPlayerClass( GetPlayerClass()->GetClassIndex() ) )
			{
				pReturnTarget = pPlayer;
				flCurDistSqr = flDistSqr;
				bFoundClass = true;
			}
		}
	}

	if ( !bFoundClass && IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		// let's spectate our sentry instead, we didn't find any other engineers to spec
		int iNumObjects = GetObjectCount();
		for ( int i=0;i<iNumObjects;i++ )
		{
			CBaseObject *pObj = GetObject(i);

			if ( pObj && pObj->GetType() == OBJ_SENTRYGUN )
			{
				pReturnTarget = pObj;
			}
		}
	}		

	return pReturnTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::FindInitialObserverTarget( void )
{
	// if there is a Boss active, watch him
	if ( TFGameRules()->GetActiveBoss() )
	{
		m_hObserverTarget.Set( TFGameRules()->GetActiveBoss() );
	}

	// If we're on a team (i.e. not a pure observer), try and find
	// a target that'll give the player the most useful information.
	if ( GetTeamNumber() >= FIRST_GAME_TEAM )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster )
		{
			// Has our forward cap point been contested recently?
			int iFarthestPoint = TFGameRules()->GetFarthestOwnedControlPoint( GetTeamNumber(), false );
			if ( iFarthestPoint != -1 )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > (gpGlobals->curtime - 30) )
				{
					// Does it have an associated viewpoint?
					CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname( NULL, "info_observer_point" );
					while ( pObserverPoint )
					{
						CObserverPoint *pObsPoint = assert_cast<CObserverPoint *>(pObserverPoint);
						if ( pObsPoint && pObsPoint->m_hAssociatedTeamEntity == pMaster->GetControlPoint(iFarthestPoint) )
						{
							if ( IsValidObserverTarget( pObsPoint ) )
							{
								m_hObserverTarget.Set( pObsPoint );
								return;
							}
						}

						pObserverPoint = gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
					}
				}
			}

			// Has the point beyond our farthest been contested lately?
			iFarthestPoint += (ObjectiveResource()->GetBaseControlPointForTeam( GetTeamNumber() ) == 0 ? 1 : -1);
			if ( iFarthestPoint >= 0 && iFarthestPoint < MAX_CONTROL_POINTS )
			{
				float flTime = pMaster->PointLastContestedAt( iFarthestPoint );
				if ( flTime != -1 && flTime > (gpGlobals->curtime - 30) )
				{
					// Try and find a player near that cap point
					CBaseEntity *pCapPoint = pMaster->GetControlPoint(iFarthestPoint);
					if ( pCapPoint )
					{
						CBaseEntity *pTarget = FindNearestObservableTarget( pCapPoint->GetAbsOrigin(), 1500 );
						if ( pTarget )
						{
							m_hObserverTarget.Set( pTarget );
							return;
						}
					}
				}
			}
		}
	}

	// Find the nearest guy near myself
	CBaseEntity *pTarget = FindNearestObservableTarget( GetAbsOrigin(), FLT_MAX );
	if ( pTarget )
	{
		m_hObserverTarget.Set( pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ValidateCurrentObserverTarget( void )
{
	// If our current target is a dead player who's gibbed / died, re-find as if 
	// we were finding our initial target, so we end up somewhere useful.
	if ( m_hObserverTarget && m_hObserverTarget->IsPlayer() )
	{
		CBasePlayer *player = ToBasePlayer( m_hObserverTarget );

		if ( player->m_lifeState == LIFE_DEAD || player->m_lifeState == LIFE_DYING )
		{
			// if we are coaching, don't switch
			if ( m_hStudent == m_hObserverTarget )
			{
				return;
			}

			// Once we're past the pause after death, find a new target
			if ( (player->GetDeathTime() + DEATH_ANIMATION_TIME ) < gpGlobals->curtime )
			{
				FindInitialObserverTarget();
			}

			return;
		}
	}

	if ( m_hObserverTarget && !m_hObserverTarget->IsPlayer() )
	{
		// can only spectate players in-eye
		if ( m_iObserverMode == OBS_MODE_IN_EYE )
		{
			ForceObserverMode( OBS_MODE_CHASE );
		}
	}

	BaseClass::ValidateCurrentObserverTarget();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CheckObserverSettings()
{
	// make sure we are always observing the student
	if ( m_hObserverTarget && m_hStudent && m_hStudent != m_hObserverTarget )
	{
		SetObserverTarget( m_hStudent );
	}
	else if ( TFGameRules() )
	{
		// is there a current entity that is the required spectator target?
		if ( TFGameRules()->GetRequiredObserverTarget() )
		{
			SetObserverTarget( TFGameRules()->GetRequiredObserverTarget() );
			return;
		}

		if ( TFGameRules()->IsPasstimeMode() && g_pPasstimeLogic && (GetObserverMode() == OBS_MODE_POI)  )
		{
			CPasstimeBall *pBall = g_pPasstimeLogic->GetBall();
			if ( !pBall || ((m_hObserverTarget.Get() == pBall) && pBall->BOutOfPlay()) )
			{
				FindInitialObserverTarget();
			}
			else if ( !pBall->BOutOfPlay() && (GetObserverTarget() != TFGameRules()->GetObjectiveObserverTarget()) )
			{
				SetObserverTarget( TFGameRules()->GetObjectiveObserverTarget() );
			}
			return;
		}
		
		// make sure we're not trying to spec the train during a team win
		// if we are, switch to spectating the last control point instead (where the train ended)
		if ( m_hObserverTarget && m_hObserverTarget->IsBaseTrain() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
		{
			// find the nearest spectator point to use instead of the train
			CObserverPoint *pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( NULL, "info_observer_point" );
			CObserverPoint *pClosestPoint = NULL;
			float flMinDistance = -1.0f;
			Vector vecTrainOrigin = m_hObserverTarget->GetAbsOrigin();

			while ( pObserverPoint )
			{
				if ( IsValidObserverTarget( pObserverPoint ) )
				{
					float flDist = pObserverPoint->GetAbsOrigin().DistTo( vecTrainOrigin );
					if ( flMinDistance < 0 || flDist < flMinDistance )
					{
						flMinDistance = flDist;
						pClosestPoint = pObserverPoint;
					}
				}

				pObserverPoint = (CObserverPoint *)gEntList.FindEntityByClassname( pObserverPoint, "info_observer_point" );
			}

			if ( pClosestPoint )
			{
				SetObserverTarget( pClosestPoint );
			}
		}
	}

	BaseClass::CheckObserverSettings();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Touch( CBaseEntity *pOther )
{
	CTFPlayer *pVictim = ToTFPlayer( pOther );

	if ( pVictim )
	{
		// ACHIEVEMENT_TF_SPY_BUMP_CLOAKED_SPY
		if ( !m_Shared.IsAlly( pVictim ) )
		{
			if ( IsPlayerClass( TF_CLASS_SPY ) && pVictim->IsPlayerClass( TF_CLASS_SPY ) )
			{
				if ( m_Shared.InCond( TF_COND_STEALTHED ) && pVictim->m_Shared.InCond( TF_COND_STEALTHED ) )
				{
					AwardAchievement( ACHIEVEMENT_TF_SPY_BUMP_CLOAKED_SPY );
				}
			}
		}

		CheckUncoveringSpies( pVictim );

		// ACHIEVEMENT_TF_HEAVY_BLOCK_INVULN_HEAVY
		if ( !m_Shared.IsAlly( pVictim ) )
		{
			if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && pVictim->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( GetTeamNumber() );
				if ( pTeam && pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
				{
					if ( m_Shared.InCond( TF_COND_INVULNERABLE ) || m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
					{
						if ( pVictim->m_Shared.InCond( TF_COND_INVULNERABLE ) || pVictim->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
						{
							float flMaxSpeed = 50.0f * 50.0f;
							if ( ( GetAbsVelocity().LengthSqr() < flMaxSpeed ) && ( pVictim->GetAbsVelocity().LengthSqr() < flMaxSpeed ) )
							{
								AwardAchievement( ACHIEVEMENT_TF_HEAVY_BLOCK_INVULN_HEAVY );
							}
						}
					}
				}
			}

			// ****************************************************************************************************************
			// Halloween Karts
			if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) && m_flHalloweenKartPushEventTime < gpGlobals->curtime )
			{
				// calculate a force and save it off, it is used on a later frame cause it is to late to apply the force here
				float flImpactForce = GetLocalVelocity().Length();
				if ( flImpactForce > 10.0f )
				{
					float flForceMult = 1.0f;

					Vector vAim = GetLocalVelocity();
					vAim.NormalizeInPlace();
					Vector vOrigin = GetAbsOrigin();

					// Force direction is velocity of the player in the case that this is a head on collison.
					// Trace
					trace_t pTrace;
					Ray_t ray;
					CTraceFilterOnlyNPCsAndPlayer pFilter( this, COLLISION_GROUP_NONE );
					//tf_halloween_kart_impact_lookahead
					//tf_halloween_kart_impact_bounds_scale
					ray.Init( vOrigin, Vector( 0, 0, 16 ) + vOrigin + vAim * tf_halloween_kart_impact_lookahead.GetFloat(), GetPlayerMins() * tf_halloween_kart_impact_bounds_scale.GetFloat(), GetPlayerMaxs() * tf_halloween_kart_impact_bounds_scale.GetFloat() );
					enginetrace->TraceRay( ray, MASK_SOLID, &pFilter, &pTrace );

					Vector vecForceDirection;
					vecForceDirection = vAim;
					vecForceDirection.z += 0.60f;
					vecForceDirection.NormalizeInPlace();
					if ( pTrace.m_pEnt == pVictim )
					{
						if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART_DASH ) )
						{
							flForceMult *= tf_halloween_kart_boost_impact_force.GetFloat();
							// Stop moving
							SetAbsVelocity( vec3_origin );
							SetCurrentTauntMoveSpeed( 0 );
							m_Shared.RemoveCond( TF_COND_HALLOWEEN_KART_DASH );
							EmitSound( "BumperCar.BumpHard" );
						}
						else
						{
							SetAbsVelocity( GetAbsVelocity() * tf_halloween_kart_impact_feedback.GetFloat() );
							SetCurrentTauntMoveSpeed( GetCurrentTauntMoveSpeed() * tf_halloween_kart_impact_feedback.GetFloat() );
							EmitSound( "BumperCar.Bump" );
						}

						// Invul Crash
						if ( m_Shared.InCond( TF_COND_INVULNERABLE_USER_BUFF ) )
						{
							flForceMult += 0.5f;
						}

						// Apply some kart damage
						//Speed maxes at 800, normally at 300?  we want about 10 damage a hit? 10-15?
						int iDamage = (int)( ( flImpactForce / 50.0f + RandomInt( 13, 19 ) ) * tf_halloween_kart_impact_damage.GetFloat() );

						// Apply force to enemy
						vecForceDirection *= flImpactForce * flForceMult * tf_halloween_kart_impact_force.GetFloat();
						pVictim->AddHalloweenKartPushEvent( this, NULL, NULL, vecForceDirection, iDamage );
					}
					else
					{
						DevMsg( "Collision with player not in Trace, %f Force \n", flImpactForce );
					}

					// can only give a kart push event every 0.2 seconds
					if ( vecForceDirection.LengthSqr() > 100.0f )
					{
						m_flHalloweenKartPushEventTime = gpGlobals->curtime + tf_halloween_kart_impact_rate.GetFloat();
					}
				}
			}
			if ( ( m_Shared.GetPercentInvisible() < 0.10f ) &&
				m_Shared.GetCarryingRuneType() == RUNE_PLAGUE && 
				!m_Shared.IsAlly( pVictim ) && 
				!pVictim->m_Shared.IsInvulnerable() && 
				!pVictim->m_Shared.InCond( TF_COND_PLAGUE ) && 
				pVictim->m_Shared.GetCarryingRuneType() != RUNE_RESIST )
			{
				pVictim->m_Shared.AddCond( TF_COND_PLAGUE, PERMANENT_CONDITION, this );

				//Plague transmission event infects nearby eligible players on the same team. Only works for powerup carrier to host, not host to host.
				const Vector& vecPos = pVictim->WorldSpaceCenter();
				for ( int i = 0; i < pVictim->GetTeam()->GetNumPlayers(); i++ )
				{
					CTFPlayer *pTeamMate = ToTFPlayer( pVictim->GetTeam()->GetPlayer( i ) );

					if ( pTeamMate && pTeamMate != pVictim && pTeamMate->IsAlive() && !pTeamMate->m_Shared.IsInvulnerable() && !pTeamMate->m_Shared.InCond( TF_COND_PLAGUE ) && pTeamMate->m_Shared.GetCarryingRuneType() != RUNE_RESIST )
					{
						// Only nearby teammates. Check for this before the more expensive visibility trace
						if ( ( vecPos - pTeamMate->WorldSpaceCenter() ).LengthSqr() < ( 350 * 350 ) )
						{
							// Doesn't go through walls
							if ( pVictim->FVisible( pTeamMate, MASK_SOLID ) )
							{
								pTeamMate->m_Shared.AddCond( TF_COND_PLAGUE, PERMANENT_CONDITION, this );
								CPVSFilter filter( WorldSpaceCenter() );
								Vector vStart = pVictim->EyePosition();
								Vector vEnd = pTeamMate->GetAbsOrigin() + Vector( 0, 0, 56 );
								te_tf_particle_effects_control_point_t controlPoint = { PATTACH_ABSORIGIN, vEnd };
								TE_TFParticleEffectComplex( filter, 0.f, "plague_transmission", vStart, QAngle( 0.f, 0.f, 0.f ), NULL, &controlPoint, pTeamMate, PATTACH_CUSTOMORIGIN );
							}
						}
					}
				}
			}
		}
	}

	BaseClass::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RefreshCollisionBounds( void )
{
	BaseClass::RefreshCollisionBounds();

	SetViewOffset( ( IsDucked() ) ? ( VEC_DUCK_VIEW_SCALED( this ) ) : ( GetClassEyeHeight() ) );
}

//-----------------------------------------------------------------------------
/**
 * Invoked (by UpdateLastKnownArea) when we enter a new nav area (or it is reset to NULL)
 */
void CTFPlayer::OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea )
{
	VPROF_BUDGET( "CTFPlayer::OnNavAreaChanged", "NextBot" );

	if ( !IsAlive() || GetTeamNumber() == TEAM_SPECTATOR )
	{
		return;
	}

	if ( leftArea )
	{
		// remove us from old visible set
		NavAreaCollector wasVisible;
		leftArea->ForAllPotentiallyVisibleAreas( wasVisible );

		for( int i=0; i<wasVisible.m_area.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)wasVisible.m_area[i];
			area->RemovePotentiallyVisibleActor( this );
		}
	}


	if ( enteredArea )
	{
		// add us to new visible set
		// @todo: is it faster to only do this for the areas that changed between sets?
		NavAreaCollector isVisible;
		enteredArea->ForAllPotentiallyVisibleAreas( isVisible );

		for( int i=0; i<isVisible.m_area.Count(); ++i )
		{
			CTFNavArea *area = (CTFNavArea *)isVisible.m_area[i];
			area->AddPotentiallyVisibleActor( this );
		}
	}
}

//-----------------------------------------------------------------------------------------------------
// Return true if the given threat is aiming in our direction
bool CTFPlayer::IsThreatAimingTowardMe( CBaseEntity *threat, float cosTolerance ) const
{
	CTFPlayer *player = ToTFPlayer( threat );
	Vector to = GetAbsOrigin() - threat->GetAbsOrigin();
	float threatRange = to.NormalizeInPlace();
	Vector forward;

	if ( player == NULL )
	{
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( threat );
		if ( sentry )
		{
			// are we in range?
			if ( threatRange < SENTRY_MAX_RANGE )
			{
				// is it pointing at us?
				AngleVectors( sentry->GetTurretAngles(), &forward );

				if ( DotProduct( to, forward ) > cosTolerance )
				{
					return true;
				}
			}
		}

		// not a player, not a sentry, not a threat?
		return false;
	}

	// is the player pointing at me?
	player->EyeVectors( &forward );

	if ( DotProduct( to, forward ) > cosTolerance )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------
// Return true if the given threat is aiming in our direction and firing its weapon
bool CTFPlayer::IsThreatFiringAtMe( CBaseEntity *threat ) const
{
	if ( IsThreatAimingTowardMe( threat ) )
	{
		CTFPlayer *player = ToTFPlayer( threat );

		if ( player )
		{
			return player->IsFiringWeapon();
		}

		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( threat );
		if ( sentry )
		{
			return sentry->GetTimeSinceLastFired() < 1.0f;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if this player has seen through an enemy spy's disguise
//-----------------------------------------------------------------------------
void CTFPlayer::CheckUncoveringSpies( CTFPlayer *pTouchedPlayer )
{
	// Only uncover enemies
	if ( m_Shared.IsAlly( pTouchedPlayer ) )
	{
		return;
	}

	// Only uncover if they're stealthed
	if ( !pTouchedPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		return;
	}

	// pulse their invisibility
	pTouchedPlayer->m_Shared.OnSpyTouchedByEnemy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DoNoiseMaker( void )
{
	if ( gpGlobals->curtime < m_Shared.GetNextNoiseMakerTime() )
		return;

	CSteamID steamIDForPlayer;
	GetSteamID( &steamIDForPlayer );

	// Check to see that we have a noise maker item equipped. We intentionally
	// want to check this to fix the infinite noise maker bugs.
	CEconItemView *pItem = GetEquippedItemForLoadoutSlot( LOADOUT_POSITION_ACTION );
	if ( !pItem )
		return;

	int iUnlimitedQuantity = 0;
	CALL_ATTRIB_HOOK_INT( iUnlimitedQuantity, unlimited_quantity );

	if ( pItem->GetItemQuantity() <= 0 && !iUnlimitedQuantity )
		return;

	perteamvisuals_t* vis = pItem->GetStaticData()->GetPerTeamVisual( 0 );
	if ( !vis )
		return;

	int iNumSounds = 0;
	for ( int i=0; i<MAX_VISUALS_CUSTOM_SOUNDS; ++i )
	{
		if ( vis->pszCustomSounds[i] )
			iNumSounds++;
	}

	if ( iNumSounds == 0 )
		return;

	int rand = RandomInt( 0, iNumSounds-1 );

	float flSoundLength = 0;
	EmitSound_t params;
	params.m_flSoundTime = 0;
	params.m_pSoundName = vis->pszCustomSounds[rand];
	params.m_pflSoundDuration = &flSoundLength;

	CPASFilter filter( GetAbsOrigin() );
	EmitSound( filter, entindex(), params );

	// Add a particle effect.
	const char *particleEffectName = pItem->GetStaticData()->GetParticleEffect( TEAM_UNASSIGNED );
	if ( particleEffectName )
	{
		TE_TFParticleEffect( filter, 0.0, particleEffectName, PATTACH_POINT_FOLLOW, this, "head" );
	}

	float flDelay = 1.0f;

	// Duck Badge Cooldown is based on badge level.  Noisemaker is more like an easter egg
	CSchemaAttributeDefHandle pAttr_DuckLevelBadge( "duck badge level" );
	uint32 iDuckBadgeLevel = 0;

	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttr_DuckLevelBadge, &iDuckBadgeLevel ) )
	{
		flDelay = 5.0f;
	}
	
	// Throttle the usage rate to sound duration plus some dead time.
	m_Shared.SetNextNoiseMakerTime( gpGlobals->curtime + flSoundLength + flDelay );
}

//-----------------------------------------------------------------------------
// Purpose: Finds an open space for a high five partner. flTolerance specifies the maximum amount that should be allowed underneath position.
//-----------------------------------------------------------------------------
bool CTFPlayer::FindOpenTauntPartnerPosition( const CEconItemView *pEconItemView, Vector &position, float *flTolerance )
{
	if ( !pEconItemView || !pEconItemView->IsValid() )
		return false;

	const GameItemDefinition_t *pItemDef = pEconItemView->GetItemDefinition();
	if ( !pItemDef || !pItemDef->GetTauntData() )
	{
		position = GetAbsOrigin();
		*flTolerance = tf_highfive_height_tolerance.GetFloat();
		return false;
	}

	const float flTauntSeparationForwardDistance = tf_highfive_separation_forward.GetFloat() != 0 ? tf_highfive_separation_forward.GetFloat() : pItemDef->GetTauntData()->GetTauntSeparationForwardDistance();
	const float flTauntSeparationRightDistance = tf_highfive_separation_right.GetFloat() != 0 ? tf_highfive_separation_right.GetFloat() : pItemDef->GetTauntData()->GetTauntSeparationRightDistance();

	bool ret = true;
	Vector forward, right;
	AngleVectors( GetAbsAngles(), &forward, &right, NULL );

	Vector vecStart = GetAbsOrigin();
	Vector vecEnd = vecStart + ( forward * flTauntSeparationForwardDistance ) + ( right * flTauntSeparationRightDistance );
	*flTolerance = tf_highfive_height_tolerance.GetFloat();
	trace_t result;
	CTraceFilterIgnoreTeammates filter( this, COLLISION_GROUP_NONE, GetAllowedTauntPartnerTeam() );
	UTIL_TraceHull( vecStart, vecEnd + ( forward * 2 ), VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, &filter, &result );

	if ( result.DidHit() )
	{
		// something's directly in front of us, but let's allow for a little bit of variation since we might be standing on an uneven displacement
		trace_t result2;
		vecStart = GetAbsOrigin() + Vector( 0, 0, *flTolerance );
		vecEnd = vecStart + ( forward * flTauntSeparationForwardDistance ) + ( right * flTauntSeparationRightDistance );
		UTIL_TraceHull( vecStart, vecEnd + ( forward * 2 ), VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, &filter, &result2 );

		// Now we can allow for twice the space underneath us.
		*flTolerance *= 2;

		if ( result2.DidHit() )
		{
			// Not enough space in front of us.
			ret = false;
		}
		else
		{
			position = vecEnd;
		}
	}
	else
	{
		position = vecEnd;
	}

	if( ret )
	{
		Vector vecStartCenter = WorldSpaceCenter();
		// Scale up how far we test.  Dont even let them get close.
		Vector vecEndSuperSafe = vecStartCenter + ( forward * flTauntSeparationForwardDistance * 2.f ) + ( right * flTauntSeparationRightDistance );

		// Dont allow crossing through the spawn room visualizers
		ret = !PointsCrossRespawnRoomVisualizer( vecStartCenter, vecEndSuperSafe );
	}


	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAllowedToInitiateTauntWithPartner( const CEconItemView *pEconItemView, char *pszErrorMessage, int cubErrorMessage )
{
	Vector vecEnd;
	float flTolerance;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return true;

	bool ret = FindOpenTauntPartnerPosition( pEconItemView, vecEnd, &flTolerance );

	// Check that there isn't too much space underneath the destination.
	if ( ret )
	{
		trace_t result3;
		CTraceFilterIgnoreTeammates filter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
		UTIL_TraceHull( vecEnd, vecEnd - Vector( 0, 0, flTolerance ), VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, &filter, &result3 );
		if ( !result3.DidHit() )
		{
			if ( pszErrorMessage && cubErrorMessage > 0 )
			{
				V_strncpy( pszErrorMessage, "#TF_PartnerTaunt_TooHigh", cubErrorMessage );
			}

			ret = false;
		}
	}
	else if ( pszErrorMessage && cubErrorMessage > 0 )
	{
		V_strncpy( pszErrorMessage, "#TF_PartnerTaunt_Blocked", cubErrorMessage );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsWormsGearEquipped( void ) const
{
	// If we have the Worms Gear equipped, play their custom sound
	static CSchemaItemDefHandle ppItemDefWearables[] = { CSchemaItemDefHandle( "Worms Gear" ) };
	return HasWearablesEquipped( ppItemDefWearables, ARRAYSIZE( ppItemDefWearables ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsRobotCostumeEquipped( void ) const
{
	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_SOLDIER )
		return false;

	static CSchemaItemDefHandle ppItemDefWearables[] = { CSchemaItemDefHandle( "Idiot Box" ), CSchemaItemDefHandle( "Steel Pipes" ), CSchemaItemDefHandle( "Shoestring Budget" ) };
	return HasWearablesEquipped( ppItemDefWearables, ARRAYSIZE( ppItemDefWearables ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsDemowolf( void ) const
{
	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN )
		return false;

	static CSchemaItemDefHandle ppItemDefWearables[] = { CSchemaItemDefHandle( "Hair of the Dog" ), CSchemaItemDefHandle( "Scottish Snarl" ), CSchemaItemDefHandle( "Pickled Paws" ) };
	return HasWearablesEquipped( ppItemDefWearables, ARRAYSIZE( ppItemDefWearables ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsFrankenHeavy( void ) const
{
	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_HEAVYWEAPONS )
		return false;

	static CSchemaItemDefHandle ppItemDefWearables[] = { CSchemaItemDefHandle( "Can Opener" ), CSchemaItemDefHandle( "Soviet Stitch-Up" ), CSchemaItemDefHandle( "Steel-Toed Stompers" ) };
	return HasWearablesEquipped( ppItemDefWearables, ARRAYSIZE( ppItemDefWearables ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsFairyHeavy( void ) const
{
	if ( GetPlayerClass()->GetClassIndex() != TF_CLASS_HEAVYWEAPONS )
		return false;

	static CSchemaItemDefHandle ppItemDefWearables[] = { CSchemaItemDefHandle( "The Grand Duchess Tutu" ), CSchemaItemDefHandle( "The Grand Duchess Fairy Wings" ), CSchemaItemDefHandle( "The Grand Duchess Tiara" ) };
	return HasWearablesEquipped( ppItemDefWearables, ARRAYSIZE( ppItemDefWearables ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsZombieCostumeEquipped( void ) const
{
	int iZombie = 0;
	CALL_ATTRIB_HOOK_INT( iZombie, zombiezombiezombiezombie );
	return iZombie != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::HasWearablesEquipped( const CSchemaItemDefHandle *ppItemDefs, int nWearables ) const
{
	for ( int i = 0; i < nWearables; i++ )
	{
		const CEconItemDefinition *pItemDef = ppItemDefs[i];
		
		// Backwards because our wearable items are probably sitting in our cosmetic slots near
		// the end of our list.
		bool bHasWearable = false;

		FOR_EACH_VEC_BACK( m_hMyWearables, wbl )
		{
			CEconWearable *pWearable = m_hMyWearables[wbl];
			if ( pWearable &&
				 pWearable->GetAttributeContainer()->GetItem() &&
				 pWearable->GetAttributeContainer()->GetItem()->GetItemDefinition() == pItemDef )
			{
				bHasWearable = true;
				break;
			}
		}

		if ( !bHasWearable )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the current concept for press-and-hold taunts or MP_CONCEPT_NONE if none is available.
//-----------------------------------------------------------------------------
int CTFPlayer::GetTauntConcept( CEconItemDefinition *pItemDef )
{
	for ( int i=0; i<pItemDef->GetNumAnimations( GetTeamNumber() ); ++i )
	{
		animation_on_wearable_t* pAnim = pItemDef->GetAnimationData( GetTeamNumber(), i );
		if ( pAnim && pAnim->pszActivity &&
			!Q_stricmp( pAnim->pszActivity, "taunt_concept" ) )
		{
			const char* pszConcept = pAnim->pszReplacement;
			if ( !pszConcept )
				return true;

			return GetMPConceptIndexFromString( pszConcept );
		}
	}

	return MP_CONCEPT_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PlayTauntSceneFromItem( const CEconItemView *pEconItemView )
{
	if ( !pEconItemView )
		return false;

	if ( !IsAllowedToTaunt() )
		return false;

	const GameItemDefinition_t *pItemDef = pEconItemView->GetItemDefinition();
	if ( !pItemDef )
		return false;

	CTFTauntInfo *pTauntData = pItemDef->GetTauntData();
	if ( !pTauntData )
		return false;

	int iClass = GetPlayerClass()->GetClassIndex();
	
	// If we didn't find any custom taunts, then we're done
	if ( pTauntData->GetIntroSceneCount( iClass ) == 0 )
	{
		return false;
	}

	int iScene = RandomInt( 0, pTauntData->GetIntroSceneCount( iClass ) - 1 );
	const char* pszScene = pTauntData->GetIntroScene( iClass, iScene );
	if ( pszScene )
	{
		int iTauntIndex = TAUNT_MISC_ITEM;
		int iTauntConcept = 0;

		// check if this is a long taunt
		static CSchemaAttributeDefHandle pAttrDef_TauntPressAndHold( "taunt is press and hold" );
		attrib_value_t iLongTaunt = 0;
		if ( pEconItemView->FindAttribute( pAttrDef_TauntPressAndHold, &iLongTaunt ) && iLongTaunt != 0 )
		{
			iTauntIndex = TAUNT_LONG;
			iTauntConcept = pTauntData->IsPartnerTaunt() ? MP_CONCEPT_PARTNER_TAUNT_READY : iTauntConcept;
			m_bIsTauntInitiator = true;

			ParseSharedTauntDataFromEconItemView( pEconItemView );

			/*cant we just network over the "taunting item id", since client and server both know all the item defs,
			then they can both look at attributes and we dont need to keep networking more and more stuff?*/
			// check if this taunt can be mimic by other players
			static CSchemaAttributeDefHandle pAttrDef_TauntMimic( "taunt mimic" );
			attrib_value_t iTauntMimic = 0;
			pEconItemView->FindAttribute( pAttrDef_TauntMimic, &iTauntMimic );
			m_bTauntMimic = iTauntMimic != 0;

			// check if we can initiate partner taunt (ignore mimic taunt to allow Conga initiation)
			char szClientError[64];
			if ( !m_bTauntMimic && pTauntData->IsPartnerTaunt() && !IsAllowedToInitiateTauntWithPartner( pEconItemView, szClientError, ARRAYSIZE( szClientError ) ) )
			{
				CSingleUserRecipientFilter filter( this );
				EmitSound_t params;
				params.m_pSoundName = "Player.DenyWeaponSelection";
				EmitSound( filter, entindex(), params );

				TFGameRules()->SendHudNotification( filter, szClientError, "ico_notify_partner_taunt" );

				return false;
			}
		}

		// Store this off so eventually we can let clients know which item ID is doing this taunt.
		m_iTauntItemDefIndex = pEconItemView->GetItemDefIndex();
		m_TauntEconItemView = *pEconItemView;

		// Should we play a sound?
		m_strTauntSoundName = "";
		m_flTauntSoundTime = 0.f;
		static CSchemaAttributeDefHandle pAttrDef_TauntSuccessSound( "taunt success sound" );
		CAttribute_String attrTauntSuccessSound;
		if ( pEconItemView->FindAttribute( pAttrDef_TauntSuccessSound, &attrTauntSuccessSound ) )
		{
			const char* pszTauntSoundName = attrTauntSuccessSound.value().c_str();
			Assert( pszTauntSoundName && *pszTauntSoundName );
			if ( pszTauntSoundName && *pszTauntSoundName )
			{
				m_strTauntSoundName = pszTauntSoundName;

				static CSchemaAttributeDefHandle pAttrDef_TauntSuccessSoundOffset( "taunt success sound offset" );
				attrib_value_t attrTauntSoundOffset = 0;
				pEconItemView->FindAttribute( pAttrDef_TauntSuccessSoundOffset, &attrTauntSoundOffset );
				float flTauntSoundOffset = (float&)attrTauntSoundOffset;
				m_flTauntSoundTime = gpGlobals->curtime + flTauntSoundOffset;
			}
		}

		// Should we play a looping sound?
		m_flTauntSoundLoopTime = 0.f;
		Assert( m_strTauntSoundLoopName.IsEmpty() );
		m_strTauntSoundLoopName = "";
		static CSchemaAttributeDefHandle pAttrDef_TauntSuccessSoundLoop( "taunt success sound loop" );
		CAttribute_String attrTauntSuccessSoundLoop;
		if ( pEconItemView->FindAttribute( pAttrDef_TauntSuccessSoundLoop, &attrTauntSuccessSoundLoop ) )
		{
			const char* pszTauntSoundLoopName = attrTauntSuccessSoundLoop.value().c_str();
			Assert( pszTauntSoundLoopName && *pszTauntSoundLoopName );
			if ( pszTauntSoundLoopName && *pszTauntSoundLoopName )
			{
				// play the looping sounds using the envelope controller
				m_strTauntSoundLoopName = pszTauntSoundLoopName;
			
				static CSchemaAttributeDefHandle pAttrDef_TauntSuccessSoundLoopOffset( "taunt success sound loop offset" );
				attrib_value_t attrTauntSoundLoopOffset = 0;
				pEconItemView->FindAttribute( pAttrDef_TauntSuccessSoundLoopOffset, &attrTauntSoundLoopOffset );
				float flTauntSoundLoopOffset = (float&)attrTauntSoundLoopOffset;
				m_flTauntSoundLoopTime = gpGlobals->curtime + flTauntSoundLoopOffset;
			}
		}

		m_iTauntAttack = TAUNTATK_NONE;
		m_flTauntAttackTime = 0.f;

		static CSchemaAttributeDefHandle pAttrDef_TauntAttackName( "taunt attack name" );
		const char* pszTauntAttackName = NULL;
		if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( pItemDef, pAttrDef_TauntAttackName, &pszTauntAttackName ) )
		{
			m_iTauntAttack = GetTauntAttackByName( pszTauntAttackName );
		}

		static CSchemaAttributeDefHandle pAttrDef_TauntAttackTime( "taunt attack time" );
		float flTauntAttackTime = 0.f;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItemDef, pAttrDef_TauntAttackTime, &flTauntAttackTime ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + flTauntAttackTime;
		}


		m_iPreTauntWeaponSlot = -1;
		if ( GetActiveWeapon() )
		{
			m_iPreTauntWeaponSlot = GetActiveWeapon()->GetSlot();
		}
		static CSchemaAttributeDefHandle pAttrDef_TauntForceWeaponSlot( "taunt force weapon slot" );
		const char* pszTauntForceWeaponSlotName = NULL;
		if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( pItemDef, pAttrDef_TauntForceWeaponSlot, &pszTauntForceWeaponSlotName ) )
		{
			int iForceWeaponSlot = StringFieldToInt( pszTauntForceWeaponSlotName, GetItemSchema()->GetWeaponTypeSubstrings() );
			Weapon_Switch( Weapon_GetSlot( iForceWeaponSlot ) );
		}

		m_bInitTaunt = true;

		// Allow voice commands, etc to be interrupted.
		CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
		Assert( pExpresser );
		pExpresser->AllowMultipleScenes();

		float flSceneDuration = PlayScene( pszScene );
		OnTauntSucceeded( pszScene, iTauntIndex, iTauntConcept );

		m_flNextAllowTauntRemapInputTime = iTauntIndex == TAUNT_LONG ? gpGlobals->curtime + flSceneDuration : -1.f;

		pExpresser->DisallowMultipleScenes();

		const char *pszTauntProp = pTauntData->GetProp( iClass );
		if ( pszTauntProp )
		{
			const char *pszTauntPropScene = pTauntData->GetPropIntroScene( iClass );
			if ( pszTauntPropScene )
			{
				CTFTauntProp *pProp = static_cast< CTFTauntProp * >( CreateEntityByName( "tf_taunt_prop" ) );
				if ( pProp )
				{
					pProp->SetModel( pszTauntProp );

					pProp->m_nSkin = GetTeamNumber() == TF_TEAM_RED ? 0 : 1;
					DispatchSpawn( pProp );
					pProp->SetAbsOrigin( GetAbsOrigin() );
					pProp->SetAbsAngles( GetAbsAngles() );
					pProp->SetEFlags( EFL_FORCE_CHECK_TRANSMIT );

					// prop should remove itself at the end of scene if it's not loopable
					pProp->SetAutoRemove( iTauntIndex != TAUNT_LONG );

					pProp->PlayScene( pszTauntPropScene );
				
					m_hTauntProp = pProp;
				}
			}
			else
			{
				CTFWeaponBase *pWeapon = GetActiveTFWeapon();
				if ( pWeapon && pWeapon->HideAttachmentsAndShowBodygroupsWhenPerformingWeaponIndependentTaunt() )
				{
					// If there's no prop scene, our weapon is being repurposed
					pWeapon->SetIsBeingRepurposedForTaunt( true );
				}
			}
		}

		// check for achievement
		static CSchemaItemDefHandle congaTaunt( "Conga Taunt" );
		if ( pEconItemView->GetItemDefinition() == congaTaunt )
		{
			CUtlVector< CTFPlayer * > vecPlayers;
			CollectPlayers( &vecPlayers, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
			CollectPlayers( &vecPlayers, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

			CUtlVector< CTFPlayer * > vecCongaLine;

			FOR_EACH_VEC( vecPlayers, i )
			{
				CTFPlayer *pPlayer = vecPlayers[i];
				if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
				{
					// is this player doing the Conga?
					if ( pPlayer->GetTauntEconItemView() && ( pPlayer->GetTauntEconItemView()->GetItemDefinition() == congaTaunt ) )
					{
						vecCongaLine.AddToTail( pPlayer );
					}
				}
			}

			if ( vecCongaLine.Count() >= 10 )
			{
				FOR_EACH_VEC( vecCongaLine, i )
				{
					CTFPlayer *pPlayer = vecCongaLine[i];
					if ( pPlayer )
					{
						pPlayer->AwardAchievement( ACHIEVEMENT_TF_TAUNT_CONGA_LINE );
					}
				}
			}
		}

		// override FOV
		m_iPreTauntFOV = GetFOV();
		if ( pTauntData->GetFOV() != 0 )
		{
			SetFOV( this, pTauntData->GetFOV() );
		}


		m_TauntStage = TAUNT_INTRO;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::PlayTauntRemapInputScene()
{
	CTFTauntInfo *pTaunt = m_TauntEconItemView.GetStaticData()->GetTauntData();
	if ( !pTaunt )
	{
		return -1.f;
	}

	if ( m_TauntStage != TAUNT_INTRO )
	{
		return -1.f;
	}

	int iClass = GetPlayerClass()->GetClassIndex();

	const char *pszCurrentSceneFileName = GetSceneFilename( m_hTauntScene );
	
	const char *pszSceneName = NULL;
	for ( int iButtonIndex=0; iButtonIndex<pTaunt->GetTauntInputRemapCount(); ++iButtonIndex )
	{
		const CTFTauntInfo::TauntInputRemap_t& tauntRemap = pTaunt->GetTauntInputRemapScene( iButtonIndex );
		if ( tauntRemap.m_vecButtonPressedScenes[iClass].IsEmpty() )
			continue;

		if ( m_afButtonPressed & tauntRemap.m_iButton )
		{
			int iRandomTaunt = RandomInt( 0, tauntRemap.m_vecButtonPressedScenes[iClass].Count() - 1 );
			pszSceneName = tauntRemap.m_vecButtonPressedScenes[iClass][iRandomTaunt];
			break;
		}

		const char *pszPressedScene = tauntRemap.m_vecButtonPressedScenes[iClass][0];
		if ( m_nButtons & tauntRemap.m_iButton )
		{
			// already in this scene, try again later for next state
			if ( FStrEq( pszCurrentSceneFileName, pszPressedScene ) )
			{
				return 0.f;
			}

			pszSceneName = pszPressedScene;
			break;
		}
		else if ( FStrEq( pszCurrentSceneFileName, pszPressedScene ) && !tauntRemap.m_vecButtonReleasedScenes[iClass].IsEmpty() )
		{
			int iRandomTaunt = RandomInt( 0, tauntRemap.m_vecButtonReleasedScenes[iClass].Count() - 1 );
			pszSceneName = tauntRemap.m_vecButtonReleasedScenes[iClass][iRandomTaunt];
			break;
		}
	}

	if ( pszSceneName )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_hTauntScene = NULL;

		CMultiplayer_Expresser *pInitiatorExpresser = GetMultiplayerExpresser();
		Assert( pInitiatorExpresser );

		pInitiatorExpresser->AllowMultipleScenes();

		// extend initiator's taunt duration to include actual high five
		m_bInitTaunt = true;

		float flSceneDuration = PlayScene( pszSceneName );

		m_bInitTaunt = false;
		pInitiatorExpresser->DisallowMultipleScenes();

		// check for a taunt prop override
		if ( m_hTauntProp != NULL )
		{
			const char *pszPropSceneName = NULL;
			for ( int iButtonIndex = 0; iButtonIndex < pTaunt->GetTauntPropInputRemapCount(); ++iButtonIndex )
			{
				const CTFTauntInfo::TauntInputRemap_t& tauntPropRemap = pTaunt->GetTauntPropInputRemapScene( iButtonIndex );
				if ( tauntPropRemap.m_vecButtonPressedScenes[ iClass ].IsEmpty() )
					continue;

				if ( m_afButtonPressed & tauntPropRemap.m_iButton )
				{
					int iRandomTaunt = RandomInt( 0, tauntPropRemap.m_vecButtonPressedScenes[ iClass ].Count() - 1 );
					pszPropSceneName = tauntPropRemap.m_vecButtonPressedScenes[ iClass ][ iRandomTaunt ];
					break;
				}

				const char *pszCurrentPropSceneFileName = GetSceneFilename( m_hTauntProp->GetSceneEntity() );
				const char *pszPropPressedScene = tauntPropRemap.m_vecButtonPressedScenes[ iClass ][ 0 ];
				if ( m_nButtons & tauntPropRemap.m_iButton )
				{
					pszPropSceneName = pszPropPressedScene;
					break;
				}
				else if ( FStrEq( pszCurrentPropSceneFileName, pszPropPressedScene ) && !tauntPropRemap.m_vecButtonReleasedScenes[ iClass ].IsEmpty() )
				{
					int iRandomTaunt = RandomInt( 0, tauntPropRemap.m_vecButtonReleasedScenes[ iClass ].Count() - 1 );
					pszPropSceneName = tauntPropRemap.m_vecButtonReleasedScenes[ iClass ][ iRandomTaunt ];
					break;
				}
			}

			if ( pszPropSceneName )
			{
				m_hTauntProp->PlayScene( pszPropSceneName );
			}
		}

		return flSceneDuration;
	}

	return 0.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::OnTauntSucceeded( const char* pszSceneName, int iTauntIndex /*= 0*/, int iTauntConcept /*= 0*/ )
{
	float flDuration = GetSceneDuration( pszSceneName ) + 0.2f;

	float flDurationMod = 1;
	CALL_ATTRIB_HOOK_FLOAT( flDurationMod, mult_gesture_time ); // Modify by attributes.
	flDuration /= flDurationMod;

	// Set player state as taunting.
	m_Shared.m_iTauntIndex = iTauntIndex;
	m_Shared.m_iTauntConcept.Set( iTauntConcept );
	m_flTauntStartTime = gpGlobals->curtime;
	m_flTauntNextStartTime = m_flTauntStartTime + flDuration;

	const itemid_t unTauntSourceItemID = m_TauntEconItemView.IsValid() ? m_TauntEconItemView.GetItemID() : INVALID_ITEM_ID;
	m_Shared.m_unTauntSourceItemID_Low = unTauntSourceItemID & 0xffffffff;
	m_Shared.m_unTauntSourceItemID_High = (unTauntSourceItemID >> 32) & 0xffffffff;
	m_Shared.AddCond( TF_COND_TAUNTING );

	if ( iTauntIndex == TAUNT_LONG )
	{
		m_flTauntRemoveTime = gpGlobals->curtime;
		m_bAllowedToRemoveTaunt = false;
		if ( iTauntConcept == MP_CONCEPT_PARTNER_TAUNT_READY )
		{
			GetReadyToTauntWithPartner();
		}

		m_flTauntYaw = BodyAngles().y;

		// min time for looping taunts
		m_flTauntNextStartTime = m_flTauntStartTime + 2.f;
	}
	else
	{
		m_flTauntRemoveTime = gpGlobals->curtime + flDuration;
		m_bAllowedToRemoveTaunt = true;
	}

	m_angTauntCamera = EyeAngles();

	// Slam velocity to zero.
	SetAbsVelocity( vec3_origin );

	// play custom set taunt particle if we have a full set equipped
	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		// FIX ME:	We should be using string attribute type instead of float when we add code support to it
		// Hand Coded for this effect which may change later
		int iCustomTauntParticle = 0;
		CALL_ATTRIB_HOOK_INT( iCustomTauntParticle, custom_taunt_particle_attr );
		if ( iCustomTauntParticle )
		{
			DispatchParticleEffect( "set_taunt_saharan_spy", PATTACH_ABSORIGIN_FOLLOW, this );
		}
	}

	// set initial taunt yaw to make sure that the client anim not off because of lag
	SetTauntYaw( GetAbsAngles()[YAW] );

	m_vecTauntStartPosition = GetAbsOrigin();

	// Strange Taunts
	EconItemInterface_OnOwnerKillEaterEventNoPartner( &m_TauntEconItemView, this, kKillEaterEvent_TauntsPerformed );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Taunt( taunts_t iTauntIndex, int iTauntConcept )
{
	if ( !IsAllowedToTaunt() )
		return;

	if ( iTauntIndex == TAUNT_LONG )
	{
		AssertMsg( false, "Long Taunt should be using the new system which reads scene names from item definitions" );
		return;
	}

	// Heavies can purchase a rage-based knockback+stun effect in MvM,
	// so ignore taunt and activate rage if we're at full rage
	if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
	{
		if ( GetActiveTFWeapon() && GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MINIGUN )
		{
			int iRage = 0;
			CALL_ATTRIB_HOOK_INT( iRage, generate_rage_on_dmg );
			if ( iRage )
			{
				if ( m_Shared.GetRageMeter() >= 100.f )
				{
					m_Shared.m_bRageDraining = true;
					EmitSound( "Heavy.Battlecry03" );
					return;
				}

				if ( m_Shared.IsRageDraining() )
					return;
			}
		}
	}

	// Allow voice commands, etc to be interrupted.
	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert( pExpresser );
	pExpresser->AllowMultipleScenes();

	m_hTauntItem = NULL;

	m_bInitTaunt = true;
	char szResponse[AI_Response::MAX_RESPONSE_NAME];
	bool bTauntSucceeded = false;
	switch ( iTauntIndex )
	{
	case TAUNT_SHOW_ITEM:
		iTauntConcept = MP_CONCEPT_PLAYER_SHOW_ITEM_TAUNT;
		break;

	// use the concept specified for these two
	case TAUNT_MISC_ITEM:
	case TAUNT_SPECIAL:
		break;

	default:
	case TAUNT_BASE_WEAPON:
		iTauntConcept = MP_CONCEPT_PLAYER_TAUNT;
		break;
	};

	bTauntSucceeded = SpeakConceptIfAllowed( iTauntConcept, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME );
	if ( bTauntSucceeded )
	{
		OnTauntSucceeded( szResponse, iTauntIndex, iTauntConcept );
	}
	else
	{
		m_bInitTaunt = false;
	}

	pExpresser->DisallowMultipleScenes();

	m_flTauntAttackTime = 0;
	m_iTauntAttack = TAUNTATK_NONE;

	if ( !bTauntSucceeded )
		return;

	// should we play a sound?
	CAttribute_String attrCosmeticTauntSound;
	CALL_ATTRIB_HOOK_STRING( attrCosmeticTauntSound, cosmetic_taunt_sound );
	const char* pszTauntSoundName = attrCosmeticTauntSound.value().c_str();
	if ( pszTauntSoundName && *pszTauntSoundName )
	{
		EmitSound( pszTauntSoundName );
	}

	if ( iTauntIndex == TAUNT_SHOW_ITEM )
	{
		m_flTauntAttackTime = gpGlobals->curtime + 1.5;
		m_iTauntAttack = TAUNTATK_SHOW_ITEM;
		return;
	}
	
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( iTauntIndex == TAUNT_BASE_WEAPON )
	{
		// phlogistinator
		if ( IsPlayerClass( TF_CLASS_PYRO ) && m_Shared.GetRageMeter() >= 100.0f && 
			 StringHasPrefix( szResponse, "scenes/player/pyro/low/taunt01" ) )
		{
			// Pyro Rage!
			CBaseCombatWeapon *pWeapon = GetActiveWeapon();
			if ( pWeapon )
			{
				int iBuffType = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iBuffType, set_buff_type );

				if ( iBuffType > 0 )
				{
					// Time for crits!
					m_Shared.ActivateRageBuff( this, iBuffType );

					// Pyro needs high defense while he's taunting
					//m_Shared.AddCond( TF_COND_DEFENSEBUFF_HIGH, 3.0f );
					m_Shared.AddCond( TF_COND_INVULNERABLE_USER_BUFF, 2.60f );
					m_Shared.AddCond( TF_COND_MEGAHEAL, 2.60f );
				}
			}
		}
		else if ( IsPlayerClass( TF_CLASS_SCOUT ) )
		{
			if ( m_Shared.InCond( TF_COND_PHASE ) == false )
			{
				if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_LUNCHBOX )
				{
					m_flTauntAttackTime = gpGlobals->curtime + 0.9;
					m_iTauntAttack = TAUNTATK_SCOUT_DRINK;
				}
			}
		}
		else if ( IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_LUNCHBOX )
			{
				m_flTauntAttackTime = gpGlobals->curtime + 1.0;
				m_iTauntAttack = TAUNTATK_HEAVY_EAT;

				// Only count sandviches for "eat 100 sandviches" achievement
				CTFLunchBox *pLunchbox = (CTFLunchBox*)pActiveWeapon;
				if ( ( pLunchbox->GetLunchboxType() == LUNCHBOX_STANDARD ) || ( pLunchbox->GetLunchboxType() == LUNCHBOX_STANDARD_ROBO ) || ( pLunchbox->GetLunchboxType() == LUNCHBOX_STANDARD_FESTIVE ) )
				{
					AwardAchievement( ACHIEVEMENT_TF_HEAVY_EAT_SANDWICHES );
				}
			}
		}
	}
	else if ( iTauntIndex == TAUNT_SPECIAL )
	{
		if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// Wrenchmotron taunt teleport home effect
			if ( !Q_stricmp( szResponse, "scenes/player/engineer/low/taunt_drg_melee.vcd" ) )
			{
				m_bIsTeleportingUsingEurekaEffect = true;

				m_teleportHomeFlashTimer.Start( 1.9f );

				// play teleport sound at location we are leaving
				Vector soundOrigin = WorldSpaceCenter();
				CPASAttenuationFilter filter( soundOrigin );

				EmitSound_t ep;
				ep.m_nChannel = CHAN_STATIC;
				ep.m_pSoundName = "Weapon_DRG_Wrench.Teleport";
				ep.m_flVolume = 1.0f;
				ep.m_SoundLevel = SNDLVL_150dB;
				ep.m_nFlags = 0;
				ep.m_nPitch = PITCH_NORM;
				ep.m_pOrigin = &soundOrigin;

				int worldEntIndex = 0;
				EmitSound( filter, worldEntIndex, ep );
			}
		}
	}

	// Setup taunt attacks. Hacky, but a lot easier to do than getting server side anim events working.
	if ( IsPlayerClass(TF_CLASS_PYRO) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/pyro/low/taunt02.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 2.1f;
			m_iTauntAttack = TAUNTATK_PYRO_HADOUKEN;
		}
		else if ( !V_stricmp( szResponse, "scenes/player/pyro/low/taunt_bubbles.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 3.0f;
			m_iTauntAttack = TAUNTATK_PYRO_ARMAGEDDON;

			// We need to parent this to a target instead of the player because the player changing their camera view can twist the rainbow
			CBaseEntity *pTarget = CreateEntityByName( "info_target" );
			if ( pTarget )
			{
				DispatchSpawn( pTarget );
				pTarget->SetAbsOrigin( GetAbsOrigin() );
				pTarget->SetAbsAngles( GetAbsAngles() );
				pTarget->SetEFlags( EFL_FORCE_CHECK_TRANSMIT );
				pTarget->SetThink( &BaseClass::SUB_Remove );
				pTarget->SetNextThink( gpGlobals->curtime + 8.0f );

				CBaseEntity *pGround = GetGroundEntity();
				if ( pGround && pGround->GetMoveType() == MOVETYPE_PUSH )
				{
					pTarget->SetParent( pGround );
				}
			}

			DispatchParticleEffect( "pyrotaunt_rainbow_norainbow", PATTACH_ABSORIGIN_FOLLOW, pTarget );
		}
		else if ( !V_stricmp( szResponse, "scenes/player/pyro/low/taunt_scorch_shot.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 1.9f;
			m_iTauntAttack = TAUNTATK_PYRO_SCORCHSHOT;
		}
	}
	else if ( IsPlayerClass(TF_CLASS_HEAVYWEAPONS) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/heavy/low/taunt03_v1.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 1.8;
			m_iTauntAttack = TAUNTATK_HEAVY_HIGH_NOON;
		}
		else if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_FISTS )
		{
			CTFFists *pFists = dynamic_cast<CTFFists*>(pActiveWeapon);
			if ( pFists && pFists->GetFistType() == FISTTYPE_RADIAL_BUFF )
			{
				m_flTauntAttackTime = gpGlobals->curtime + 1.0;
				m_iTauntAttack = TAUNTATK_HEAVY_RADIAL_BUFF;
			}
		}
	}
	else if ( IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/scout/low/taunt05_v1.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 4.03f;
			m_iTauntAttack = TAUNTATK_SCOUT_GRAND_SLAM;
		}
	}
	else if ( IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/medic/low/taunt06.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 0.8f;
			m_flTauntInhaleTime = gpGlobals->curtime + 1.8f;
			
			const char *pszParticleEffect;
			pszParticleEffect = ( GetTeamNumber() == TF_TEAM_RED ? "healhuff_red" : "healhuff_blu" );
			DispatchParticleEffect( pszParticleEffect, PATTACH_POINT_FOLLOW, this, "eyes" );

			m_iTauntAttack = TAUNTATK_MEDIC_INHALE;
		}
		else if ( !V_stricmp( szResponse, "scenes/player/medic/low/taunt08.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 2.2f;
			m_iTauntAttack = TAUNTATK_MEDIC_UBERSLICE_IMPALE;
		}
	}
	else if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( !V_strnicmp( szResponse, "scenes/player/spy/low/taunt03", 29 ) )		// There's taunt03_v1 & taunt03_v2
		{
			m_flTauntAttackTime = gpGlobals->curtime + 1.8f;
			m_iTauntAttack = TAUNTATK_SPY_FENCING_SLASH_A;
		}
	}
	else if ( IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/sniper/low/taunt04.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 0.85f;
			m_iTauntAttack = TAUNTATK_SNIPER_ARROW_STAB_IMPALE;
		}
	}
	else if ( IsPlayerClass( TF_CLASS_SOLDIER ) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/soldier/low/taunt05.vcd" ) )
		{
			if ( IsWormsGearEquipped() )
			{
				m_flTauntAttackTime = gpGlobals->curtime + 1.4f;
				m_iTauntAttack = TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN;
				return;
			}

			m_flTauntAttackTime = gpGlobals->curtime + 3.5f;
			m_iTauntAttack = TAUNTATK_SOLDIER_GRENADE_KILL;
		}
	}
	else if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/demoman/low/taunt09.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 2.55f;
			m_iTauntAttack = TAUNTATK_DEMOMAN_BARBARIAN_SWING;
		}
	}
	else if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		if ( !V_stricmp( szResponse, "scenes/player/engineer/low/taunt07.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 3.695f;
			m_iTauntAttack = TAUNTATK_ENGINEER_GUITAR_SMASH;
		}
		else if ( !V_stricmp( szResponse, "scenes/player/engineer/low/taunt09.vcd" ) )
		{
			m_flTauntAttackTime = gpGlobals->curtime + 3.2f;
			m_iTauntAttack = TAUNTATK_ENGINEER_ARM_IMPALE;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Aborts a taunt in progress.
//-----------------------------------------------------------------------------
void CTFPlayer::CancelTaunt( void )
{
	m_bIsTeleportingUsingEurekaEffect = false;
	m_teleportHomeFlashTimer.Reset();

	StopTaunt();
}

//-----------------------------------------------------------------------------
// Purpose: Stops taunting
//-----------------------------------------------------------------------------
void CTFPlayer::StopTaunt( bool bForceRemoveProp /* = true */ )
{
	if ( m_hTauntScene.Get() )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_flTauntRemoveTime = 0.0f;
		m_bAllowedToRemoveTaunt = true;
		m_hTauntScene = NULL;
	}

	if ( m_hTauntProp.Get() && ( !m_hTauntProp->ShouldSelfRemove() || bForceRemoveProp ) )
	{
		UTIL_Remove( m_hTauntProp );
		m_hTauntProp = NULL;
	}

	if ( IsReadyToTauntWithPartner() )
	{
		CancelTauntWithPartner();
	}

	StopTauntSoundLoop();

	// reset the FOV
	if ( m_TauntEconItemView.IsValid() )
	{
		SetFOV( this, m_iPreTauntFOV );
	}

	m_hHighFivePartner = NULL;
	m_bAllowMoveDuringTaunt = false;
	m_flTauntOutroTime = 0.f;
	m_bTauntForceMoveForward = false;
	m_flTauntForceMoveForwardSpeed = 0.f;
	m_flTauntMoveAccelerationTime = 0.f;
	m_flTauntTurnSpeed = 0.f;
	m_flTauntTurnAccelerationTime = 0.f;
	m_bTauntMimic = false;
	m_bIsTauntInitiator = false;
	m_TauntEconItemView.Invalidate();
	m_flNextAllowTauntRemapInputTime = -1.f;
	m_flCurrentTauntMoveSpeed = 0.f;
	m_nActiveTauntSlot = LOADOUT_POSITION_INVALID;
	m_iTauntItemDefIndex = INVALID_ITEM_DEF_INDEX;
	m_TauntStage = TAUNT_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::EndLongTaunt()
{
	Assert( m_Shared.GetTauntIndex() == TAUNT_LONG );

	m_bAllowedToRemoveTaunt = true;
	m_flTauntRemoveTime = gpGlobals->curtime;

	int iClass = GetPlayerClass()->GetClassIndex();
	CTFTauntInfo *pTauntData = m_TauntEconItemView.GetStaticData()->GetTauntData();
	if ( pTauntData )
	{
		// Make sure press-and-hold taunts last a minimum amount of time
		float flMinTime = pTauntData->GetMinTauntTime();
		if ( m_flTauntStartTime + flMinTime > gpGlobals->curtime )
		{
			m_flTauntRemoveTime = m_flTauntStartTime + flMinTime;
		}

		// should we play outro?
		if ( pTauntData->GetOutroSceneCount( iClass ) > 0 )
		{
			m_bAllowedToRemoveTaunt = false;
			m_flTauntOutroTime = m_flTauntRemoveTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::PlayTauntOutroScene()
{
	m_TauntStage = TAUNT_OUTRO;

	float flDuration = 0.f;
	int iClass = GetPlayerClass()->GetClassIndex();
	CTFTauntInfo *pTauntData = m_TauntEconItemView.GetStaticData()->GetTauntData();
	if ( pTauntData )
	{
		if ( pTauntData->GetOutroSceneCount( iClass ) > 0 )
		{
			// play outro
			const char *pszOutroScene = pTauntData->GetOutroScene( iClass, RandomInt( 0, pTauntData->GetOutroSceneCount( iClass ) - 1 ) );
			if ( m_hTauntScene.Get() )
			{
				StopScriptedScene( this, m_hTauntScene );
				m_hTauntScene = NULL;

				StopTauntSoundLoop();
			}

			// Allow voice commands, etc to be interrupted.
			CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
			Assert( pExpresser );
			pExpresser->AllowMultipleScenes();

			m_bInitTaunt = true;

			flDuration = PlayScene( pszOutroScene );
			OnTauntSucceeded( pszOutroScene, TAUNT_MISC_ITEM, MP_CONCEPT_HIGHFIVE_SUCCESS );

			m_bInitTaunt = false;

			pExpresser->DisallowMultipleScenes();

			if ( m_hTauntProp != NULL )
			{
				const char *pszPropScene = pTauntData->GetPropOutroScene( iClass );
				if ( pszPropScene )
				{
					m_hTauntProp->SetAutoRemove( true );
					m_hTauntProp->PlayScene( pszPropScene );
				}
			}
		}
	}

	return flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleTauntCommand( int iTauntSlot )
{
	if ( !IsAllowedToTaunt() )
		return;

	m_nActiveTauntSlot = LOADOUT_POSITION_INVALID;
	if ( iTauntSlot > 0 && iTauntSlot <= 8 )
	{
		m_nActiveTauntSlot = LOADOUT_POSITION_TAUNT + iTauntSlot - 1;
		CEconItemView* pItem = GetEquippedItemForLoadoutSlot( m_nActiveTauntSlot );
		PlayTauntSceneFromItem( pItem );
		return;
	}
	else
	{
		// Check if I should accept taunt with partner
		CTFPlayer *initiator = FindPartnerTauntInitiator();
		if ( initiator )
		{
			if ( initiator->m_bTauntMimic )
			{
				MimicTauntFromPartner( initiator );
			}
			else
			{
				AcceptTauntWithPartner( initiator );
			}
			return;
		}

		// does this weapon prevent player from doing manual taunt?
		CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
		if ( !pActiveWeapon || !pActiveWeapon->AllowTaunts() )
			return;

		const CEconItemView *pTauntItem = pActiveWeapon->GetTauntItem();
		CUtlVector< CTFWeaponBase* > vecPassiveWeapons;
		if ( GetPassiveWeapons( vecPassiveWeapons ) )
		{
			// find the first passive weapon taunt that might stomp this
			FOR_EACH_VEC( vecPassiveWeapons, i )
			{
				const CEconItemView *pPassiveTauntItem = vecPassiveWeapons[i]->GetTauntItem();
				if ( pPassiveTauntItem )
				{
					pTauntItem = pPassiveTauntItem;
					break;
				}
			}
		}

		if ( pTauntItem && pTauntItem->IsValid() && PlayTauntSceneFromItem( pTauntItem ) )
		{
			// taunts played from item
			return;
		}
		else
		{
			Taunt( TAUNT_BASE_WEAPON );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearTauntAttack()
{
	m_flTauntAttackTime = 0.f;
	m_flTauntInhaleTime = 0.f;
	m_iTauntAttack = TAUNTATK_NONE;
	m_iTauntAttackCount = 0;
	m_iTauntRPSResult = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::HandleWeaponSlotAfterTaunt()
{
	if ( m_iPreTauntWeaponSlot != -1 )
	{
		// HonorBound check
		if ( GetActiveTFWeapon() && GetActiveTFWeapon()->IsHonorBound() && GetActiveTFWeapon()->GetSlot() != m_iPreTauntWeaponSlot )
		{
			// Hack to prevent losing health for deploying/holstering an HonorBound weapon that was force-deployed for the taunt (Taunt: Roar O'War)
			m_Shared.m_iKillCountSinceLastDeploy++;
		}

		// switch back to the active weapon before taunting
		Weapon_Switch( Weapon_GetSlot( m_iPreTauntWeaponSlot ) );
		m_iPreTauntWeaponSlot = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void DispatchRPSEffect( const CTFPlayer *pPlayer, const char* pszParticleName )
{
	CEffectData	data;
	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );
	data.m_vOrigin = pPlayer->GetAbsOrigin() + Vector( 0, 0, 87.0f );
	data.m_vAngles = vec3_angle;

	CPASFilter intiatorFilter( data.m_vOrigin );
	intiatorFilter.SetIgnorePredictionCull( true );

	te->DispatchEffect( intiatorFilter, 0.0, data.m_vOrigin, "ParticleEffect", data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::DoTauntAttack( void )
{
	if ( !IsTaunting() || !IsAlive() || m_iTauntAttack == TAUNTATK_NONE )
	{
		return;
	}

	int iTauntAttack = m_iTauntAttack;
	m_iTauntAttack = TAUNTATK_NONE;

	if ( iTauntAttack == TAUNTATK_PYRO_HADOUKEN || iTauntAttack == TAUNTATK_SPY_FENCING_SLASH_A || 
		 iTauntAttack == TAUNTATK_SPY_FENCING_SLASH_B || iTauntAttack == TAUNTATK_SPY_FENCING_STAB || iTauntAttack == TAUNTATK_PYRO_GASBLAST )
	{
		// Pyro Hadouken fireball attack
		// Kill all enemies within a small volume in front of the player.
		Vector vecForward;
		AngleVectors( QAngle(0, m_angEyeAngles[YAW], 0), &vecForward );
		Vector vecCenter = WorldSpaceCenter() + vecForward * 64;
		Vector vecSize = Vector(24,24,24);
		CBaseEntity *pList[256];
		int count = UTIL_EntitiesInBox( pList, 256, vecCenter - vecSize, vecCenter + vecSize, FL_CLIENT|FL_OBJECT );
		if ( count )
		{
			// Launch them up a little
			AngleVectors( QAngle(-45, m_angEyeAngles[YAW], 0), &vecForward );

			for ( int i = 0; i < count; i++ )
			{
				// Team damage doesn't prevent us hurting ourself, so we do it manually here
				if ( pList[i] == this )
					continue;

				if ( FVisible( pList[i], MASK_SOLID ) == false )
					continue;

				Vector vecPos = WorldSpaceCenter();
				vecPos += (pList[i]->WorldSpaceCenter() - vecPos) * 0.75;

				// Spy taunt does two quick slashes, followed by a killing blow
				if ( iTauntAttack == TAUNTATK_SPY_FENCING_SLASH_A || iTauntAttack == TAUNTATK_SPY_FENCING_SLASH_B )
				{
					// No physics push so it doesn't push the player out of the range of the stab
					pList[i]->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 100, vecPos, 25, DMG_SLASH | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_TAUNTATK_FENCING ) );
				}
				else if ( iTauntAttack == TAUNTATK_SPY_FENCING_STAB )
				{
					pList[i]->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 20000, vecPos, 500.0f, DMG_SLASH, TF_DMG_CUSTOM_TAUNTATK_FENCING ) );
				}
				else if ( iTauntAttack == TAUNTATK_PYRO_HADOUKEN )
				{
					pList[i]->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 25000, vecPos, 500.0f, DMG_BURN | DMG_IGNITE, TF_DMG_CUSTOM_TAUNTATK_HADOUKEN ) );
				}
				else if ( iTauntAttack == TAUNTATK_PYRO_GASBLAST )
				{
					pList[i]->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 25000, vecPos, 500.0f, DMG_BURN | DMG_IGNITE, TF_DMG_CUSTOM_TAUNTATK_GASBLAST ) );
				}
			}
		}

		if ( iTauntAttack == TAUNTATK_SPY_FENCING_SLASH_A )
		{
			m_iTauntAttack = TAUNTATK_SPY_FENCING_SLASH_B;
			m_flTauntAttackTime = gpGlobals->curtime + 0.47;
		}
		else if ( iTauntAttack == TAUNTATK_SPY_FENCING_SLASH_B )
		{
			m_iTauntAttack = TAUNTATK_SPY_FENCING_STAB;
			m_flTauntAttackTime = gpGlobals->curtime + 1.73;
		}

		if ( tf_debug_damage.GetBool() )
		{
			NDebugOverlay::Box( vecCenter, -vecSize, vecSize, 0, 255, 0, 40, 10 );
		}
	}
	else if ( iTauntAttack == TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN )
	{
		EmitSound( "Taunt.WormsHHG" );
		m_iTauntAttack = TAUNTATK_SOLDIER_GRENADE_KILL;
		m_flTauntAttackTime = gpGlobals->curtime + 2.1;
	}
	else if ( iTauntAttack == TAUNTATK_SOLDIER_GRENADE_KILL )
	{
		matrix3x4_t worldSpace;
		MatrixCopy( EntityToWorldTransform(), worldSpace );

		Vector bonePos;
		QAngle boneAngles;
		int iRightHand = LookupBone( "bip_hand_r" );
		if ( iRightHand != -1 )
		{
			GetBonePosition( iRightHand, bonePos, boneAngles );

			CPVSFilter filter( bonePos );
			TE_TFExplosion( filter, 0.0f, bonePos, Vector(0,0,1), TF_WEAPON_GRENADELAUNCHER, entindex() );

			CTakeDamageInfo info( this, this, GetActiveTFWeapon(), vec3_origin, bonePos, 200.f, DMG_BLAST | DMG_USEDISTANCEMOD, TF_DMG_CUSTOM_TAUNTATK_GRENADE, &bonePos );
			CTFRadiusDamageInfo radiusinfo( &info, bonePos, 100.f );
			TFGameRules()->RadiusDamage( radiusinfo );
		}
	}
	else if ( iTauntAttack == TAUNTATK_SNIPER_ARROW_STAB_IMPALE || iTauntAttack == TAUNTATK_SNIPER_ARROW_STAB_KILL ||
			  iTauntAttack == TAUNTATK_ENGINEER_ARM_IMPALE || iTauntAttack == TAUNTATK_ENGINEER_ARM_KILL || iTauntAttack == TAUNTATK_ENGINEER_ARM_BLEND )
	{
		Vector vecForward;
		AngleVectors( EyeAngles(), &vecForward );
		Vector vecEnd = EyePosition() + vecForward * 128;

		trace_t tr;
		UTIL_TraceLine( EyePosition(), vecEnd, MASK_SOLID & ~CONTENTS_HITBOX, this, COLLISION_GROUP_PLAYER, &tr );

		if ( tr.fraction < 1.0 )
		{
			CBaseEntity *pEnt = tr.m_pEnt;

			if ( pEnt && pEnt->IsPlayer() && pEnt->GetTeamNumber() > LAST_SHARED_TEAM && pEnt->GetTeamNumber() != GetTeamNumber() )
			{
				CTFPlayer *pVictim = ToTFPlayer( pEnt );

				switch ( iTauntAttack )
				{
				case TAUNTATK_SNIPER_ARROW_STAB_IMPALE:
				case TAUNTATK_ENGINEER_ARM_IMPALE:
					if ( pVictim )
					{
						// don't stun giants
						if ( !pVictim->IsMiniBoss() )
						{
							pVictim->m_Shared.StunPlayer( 3.0f, 1.0, TF_STUN_BOTH | TF_STUN_NO_EFFECTS, this );
						}

						if ( iTauntAttack == TAUNTATK_ENGINEER_ARM_IMPALE )
						{
							pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward, pEnt->WorldSpaceCenter(), 1, DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL ) );
						}
					}
					break;

				case TAUNTATK_ENGINEER_ARM_BLEND:
					pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward, pEnt->WorldSpaceCenter(), 1, DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL ) );
					break;

				case TAUNTATK_SNIPER_ARROW_STAB_KILL:
					// Launch them up a little
					vecForward = (WorldSpaceCenter() - pEnt->WorldSpaceCenter());
					VectorNormalize( vecForward );
					pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 12000, pEnt->WorldSpaceCenter(), 500.0f, DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB ) );
					break;

				case TAUNTATK_ENGINEER_ARM_KILL:
					pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 12000, pEnt->WorldSpaceCenter(), 500.0f, DMG_BLAST, TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL ) );
					break;
				}
			}
		}

		if ( iTauntAttack == TAUNTATK_SNIPER_ARROW_STAB_IMPALE )
		{
			m_iTauntAttack = TAUNTATK_SNIPER_ARROW_STAB_KILL;
			m_flTauntAttackTime = gpGlobals->curtime + 1.30;
		}
		else if ( iTauntAttack == TAUNTATK_ENGINEER_ARM_IMPALE )
		{
			m_iTauntAttack = TAUNTATK_ENGINEER_ARM_BLEND;
			m_flTauntAttackTime = gpGlobals->curtime + 0.05;
			m_iTauntAttackCount = 0;
		}
		else if ( iTauntAttack == TAUNTATK_ENGINEER_ARM_BLEND )
		{
			m_iTauntAttack = TAUNTATK_ENGINEER_ARM_BLEND;
			m_flTauntAttackTime = gpGlobals->curtime + 0.05;
			m_iTauntAttackCount++;
			if ( m_iTauntAttackCount == 13 )
			{
				m_iTauntAttack = TAUNTATK_ENGINEER_ARM_KILL;
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_HEAVY_EAT )
	{
		CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
		if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_LUNCHBOX )
		{
			CTFLunchBox *pLunchbox = (CTFLunchBox*)pActiveWeapon;
			pLunchbox->ApplyBiteEffects( this );
		}

		// Keep eating until the taunt is over
		m_iTauntAttack = TAUNTATK_HEAVY_EAT;
		m_flTauntAttackTime = gpGlobals->curtime + 1.0;

		// If we're going to finish eating after this bite, say our line
		if ( m_flTauntRemoveTime < m_flTauntAttackTime )
		{
			if ( IsSpeaking() )
			{
				// The player may technically still be speaking even though the actual VO is over and just 
				// hasn't been cleared yet. We need to force it to end so our next concept can be played.
				CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
				if ( pExpresser )
				{
					pExpresser->ForceNotSpeaking();
				}
			}
				
			SpeakConceptIfAllowed( MP_CONCEPT_ATE_FOOD );
		}
	}
	else if ( iTauntAttack == TAUNTATK_HEAVY_RADIAL_BUFF )
	{
		Vector vecOrg = GetAbsOrigin();

		// Find nearby team mates and give them bonus health & crit chance
		for ( int i = 0; i < GetTeam()->GetNumPlayers(); i++ )
		{
			CTFPlayer *pTeamPlayer = ToTFPlayer( GetTeam()->GetPlayer(i) );
			if ( pTeamPlayer && pTeamPlayer->IsAlive() )
			{
				// If they're within the radius, give 'em the buff
				if ( (vecOrg - pTeamPlayer->GetAbsOrigin()).LengthSqr() < (1024*1024) )
				{
					pTeamPlayer->TakeHealth( 50, DMG_GENERIC );
					pTeamPlayer->m_Shared.AddTempCritBonus( 0.5 );

					IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
					if ( event )
					{
						event->SetInt( "amount", 50 );
						event->SetInt( "entindex", pTeamPlayer->entindex() );
						event->SetInt( "weapon_def_index", INVALID_ITEM_DEF_INDEX );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_HEAVY_HIGH_NOON )
	{
		// Heavy "High Noon" attack
		Vector vecForward;
 		AngleVectors( EyeAngles(), &vecForward );
		Vector vecEnd = EyePosition() + vecForward * 500;

		trace_t tr;
		UTIL_TraceLine( EyePosition(), vecEnd, ( MASK_SOLID | CONTENTS_HITBOX ), this, COLLISION_GROUP_PLAYER, &tr );
//		DebugDrawLine( EyePosition(), vecEnd, 0, 0, 255, true, 3.0f );

		if ( tr.fraction < 1.0 )
		{
			CBaseEntity *pEnt = tr.m_pEnt;

			if ( pEnt && pEnt->IsPlayer() && pEnt->GetTeamNumber() > LAST_SHARED_TEAM && pEnt->GetTeamNumber() != GetTeamNumber() )
			{
				// Launch them up a little
				AngleVectors( QAngle(-45, m_angEyeAngles[YAW], 0), &vecForward );
				pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 25000, WorldSpaceCenter(), 500.0f, DMG_BULLET, TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON ) );
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_SCOUT_DRINK )
	{
		if ( !m_Shared.IsControlStunned() )
		{
			// Check for CritBerry flavor
			CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
			if ( pActiveWeapon && pActiveWeapon->GetWeaponID() == TF_WEAPON_LUNCHBOX )
			{
				float flDropDeadTime = ( 100.f / tf_scout_energydrink_consume_rate.GetFloat() ) + 1.f;	// Just in case.  Normally over in 8 seconds.

				CTFLunchBox *pLunchbox = static_cast< CTFLunchBox* >( pActiveWeapon );
				if ( pLunchbox && pLunchbox->GetLunchboxType() == LUNCHBOX_ADDS_MINICRITS )
				{
					m_Shared.AddCond( TF_COND_ENERGY_BUFF, flDropDeadTime );
				}
				else
				{
					m_Shared.AddCond( TF_COND_PHASE, flDropDeadTime );

					if ( HasTheFlag() )
					{
						bool bShouldDrop = true;

						// Always allow teams to hear each other in TD mode
						if ( TFGameRules()->IsMannVsMachineMode() && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
						{
							bShouldDrop = false;
						}

						if ( bShouldDrop )
						{
							DropFlag();
						}
					}
				}

				SelectLastItem();
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_SCOUT_GRAND_SLAM )
	{
		// Find a player in front of us and knock 'em across the map.
		// Same box logic as hadouken & pyro knockback.
		Vector vecForward;
		AngleVectors( QAngle(0, m_angEyeAngles[YAW], 0), &vecForward );
		Vector vecCenter = WorldSpaceCenter() + vecForward * 64;
		Vector vecSize = Vector(24,24,24);
		CBaseEntity *pObjects[256];
		int count = UTIL_EntitiesInBox( pObjects, 256, vecCenter - vecSize, vecCenter + vecSize, FL_CLIENT|FL_OBJECT );
		if ( count )
		{
			for ( int i=0; i<count; i++ )
			{
				// Must be facing whoever we knock back.
				Vector vecToTarget;
				vecToTarget = pObjects[i]->WorldSpaceCenter() - WorldSpaceCenter();
				VectorNormalize( vecToTarget );
				float flDot = DotProduct( vecForward, vecToTarget );
				if ( flDot < 0.80 )
					continue;

				CTFPlayer *pTarget = ToTFPlayer( pObjects[i] );
				if ( !pTarget )
					continue;

				if ( pTarget->GetTeamNumber() == GetTeamNumber() )
					continue;

				// Do a quick trace and make sure we have LOS.
				trace_t tr;
				UTIL_TraceLine( WorldSpaceCenter(), pObjects[i]->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_PLAYER, &tr );

				if ( tr.fraction < 1.0 )
					continue;

				pTarget->SetAbsVelocity( vec3_origin );
				//pTarget->m_Shared.StunPlayer( 8.f, 1.f, TF_STUN_BOTH | TF_STUN_SPECIAL_SOUND );
				pTarget->StunSound( this, TF_STUN_BOTH | TF_STUN_SPECIAL_SOUND );
				pTarget->ApplyPunchImpulseX( RandomInt( 10, 15 ) );

				AngleVectors( QAngle(-45, m_angEyeAngles[YAW], 0), &vecForward );
				pTarget->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 130000, WorldSpaceCenter(), 500.0f, DMG_BULLET, TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM ) );

				// Tell the achievement system we swatted someone.
				IGameEvent *event = gameeventmanager->CreateEvent( "scout_grand_slam" );
				if ( event )
				{
					event->SetInt( "scout_id", GetUserID() );
					event->SetInt( "target_id", pTarget->GetUserID() );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_MEDIC_HEROIC_TAUNT )
	{
		// do these later
		m_flTauntAttackTime = gpGlobals->curtime + 3.0f;
		m_iTauntAttack = TAUNTATK_MEDIC_RELEASE_DOVES;

		// send a reliable message to make sure the effect happens
		CPVSFilter filter( GetAbsOrigin() );
		UserMessageBegin( filter, "PlayerGodRayEffect" );
			WRITE_BYTE( entindex() );
		MessageEnd();

		EmitSound( "Taunt.MedicHeroic" );
	}
	else if ( iTauntAttack == TAUNTATK_MEDIC_RELEASE_DOVES )
	{
		// not really a taunt "attack", just a hook to release some doves at the appropriate time
		Vector launchSpot = ( WorldSpaceCenter() + GetAbsOrigin() ) / 2.0f;
		for( int i=0; i<MEDIC_RELEASE_DOVE_COUNT; ++i )
		{
			Vector vecPos = launchSpot + Vector( 0, 0, RandomFloat( -10.0f, 20.0f ) );
			SpawnClientsideFlyingBird( vecPos );
		}
	}
	else if ( iTauntAttack == TAUNTATK_PYRO_ARMAGEDDON )
	{
		Vector origin( GetAbsOrigin() );

		CPVSFilter filter( origin );
		TE_TFExplosion( filter, 0.0f, origin, Vector( 0.0f, 0.0f, 1.0f ), TF_WEAPON_GRENADELAUNCHER, entindex() );

		int nRandomPick[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		CUtlVector< CTFPlayer* > vecDamagedPlayers;
		const float flRadius = 100.0f;
		const float flRadiusSqr = flRadius * flRadius;	

		CBaseEntity *pEntity = NULL;
		for ( CEntitySphereQuery sphere( origin, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL && vecDamagedPlayers.Count() < ARRAYSIZE( nRandomPick ); sphere.NextEntity() )
		{
			// Skip players on the same team or who are invuln
			CTFPlayer *pPlayer = ToTFPlayer( pEntity );
			if ( !pPlayer || InSameTeam( pPlayer ) || pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
				continue;

			// CEntitySphereQuery actually does a box test. So we need to make sure the distance is less than the radius first.
			Vector vecPos;
			pEntity->CollisionProp()->CalcNearestPoint( origin, &vecPos );
			if ( ( origin - vecPos ).LengthSqr() > flRadiusSqr )
				continue;

			// Finally LOS test
			trace_t	tr;
			Vector vecSrc = WorldSpaceCenter();
			Vector vecSpot = pEntity->WorldSpaceCenter();
			CTraceFilterSimple filter( this, COLLISION_GROUP_PROJECTILE );
			UTIL_TraceLine( vecSrc, vecSpot, MASK_SOLID_BRUSHONLY, &filter, &tr );

			// If we don't trace the whole way to the target, and we didn't hit the target entity, we're blocked
			if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
				continue;

			vecDamagedPlayers.AddToTail( pPlayer );
		}

		if ( vecDamagedPlayers.Count() )
		{
			int nBurnCount = 0;
			float fDamage = 400.0f; 

			for ( int i = vecDamagedPlayers.Count() - 1; i >= 0; --i )
			{
				// Pick a random player
				int nRand = RandomInt( 0, i );
				CTFPlayer *pPlayer = vecDamagedPlayers[ nRandomPick[ nRand ] ];
				if ( pPlayer )
				{
					bool bBurning = pPlayer->m_Shared.InCond( TF_COND_BURNING );

					pPlayer->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vec3_origin, origin, fDamage, DMG_PLASMA, ( iTauntAttack == TAUNTATK_PYRO_ARMAGEDDON ) ? TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON : TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF, &origin ) );

					// If they weren't burning before but now they are, count it
					if ( !bBurning && pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
					{
						nBurnCount++;
					}

					// Next choice gets half that amount
					fDamage /= 2;

					// The end of the list moves overwrites the one we just picked
					nRandomPick[ nRand ] = nRandomPick[ i ];
				}
			}

			if ( iTauntAttack == TAUNTATK_PYRO_ARMAGEDDON )
			{
				if ( nBurnCount >= 3 )
				{
					AwardAchievement( ACHIEVEMENT_TF_PYRO_IGNITE_WITH_RAINBOW );
				}
			}
		}

		UTIL_ScreenShake( origin, 15.0, 150.0, 0.75f, 500.0f, SHAKE_START );

	}
	else if ( iTauntAttack == TAUNTATK_PYRO_SCORCHSHOT )
	{
		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_FLAREGUN )
		{
			CTFWeaponBaseGun *pGun = dynamic_cast< CTFWeaponBaseGun* >( pWeapon );
			if ( pGun )
			{
				pGun->FireProjectile( this );
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_ALLCLASS_GUITAR_RIFF )
	{
		// We need to parent this to a target instead of the player because the player changing their camera view can twist the rainbow
		CBaseEntity *pTarget = CreateEntityByName( "info_target" );
		if ( pTarget )
		{
			DispatchSpawn( pTarget );
			pTarget->SetAbsOrigin( GetAbsOrigin() );
			pTarget->SetAbsAngles( GetAbsAngles() );
			pTarget->SetEFlags( EFL_FORCE_CHECK_TRANSMIT );
			pTarget->SetThink( &BaseClass::SUB_Remove );
			pTarget->SetNextThink( gpGlobals->curtime + 6.0f );

			CBaseEntity *pGround = GetGroundEntity();
			if ( pGround && pGround->GetMoveType() == MOVETYPE_PUSH )
			{
				pTarget->SetParent( pGround );
			}
		}

		CBroadcastRecipientFilter filter;
		TE_TFParticleEffect( filter, 0.0, "bl_killtaunt", GetAbsOrigin(), GetAbsAngles(), pTarget, PATTACH_ABSORIGIN_FOLLOW );
		EmitSound( "Taunt.GuitarRiff" );
	}
	else if ( iTauntAttack == TAUNTATK_MEDIC_INHALE )
	{
		int iHealed = TakeHealth( 1, DMG_GENERIC );

		if ( iHealed > 0 )
		{
			CTF_GameStats.Event_PlayerHealedOther( this, iHealed );
		}

		// Keep eating until the taunt is over
		if ( m_flTauntInhaleTime > gpGlobals->curtime )
		{
			m_iTauntAttack = TAUNTATK_MEDIC_INHALE;
			m_flTauntAttackTime = gpGlobals->curtime + 0.1;
		}
	}
	else if ( iTauntAttack == TAUNTATK_MEDIC_UBERSLICE_IMPALE || iTauntAttack == TAUNTATK_MEDIC_UBERSLICE_KILL )
	{
		Vector vecForward;
		AngleVectors( EyeAngles(), &vecForward );
		Vector vecEnd = EyePosition() + vecForward * 128;

		trace_t tr;
		UTIL_TraceLine( EyePosition(), vecEnd, MASK_SOLID & ~CONTENTS_HITBOX, this, COLLISION_GROUP_PLAYER, &tr );

		if ( tr.fraction < 1.0 )
		{
			CBaseEntity *pEnt = tr.m_pEnt;

			if ( pEnt && pEnt->IsPlayer() && pEnt->GetTeamNumber() > LAST_SHARED_TEAM && pEnt->GetTeamNumber() != GetTeamNumber() )
			{
				CTFPlayer *pVictim = ToTFPlayer( pEnt );

				if ( iTauntAttack == TAUNTATK_MEDIC_UBERSLICE_IMPALE )
				{
					if ( pVictim )
					{
						// don't stun giants
						if ( !pVictim->IsMiniBoss() )
						{
							pVictim->m_Shared.StunPlayer( 1.5f, 1.0, TF_STUN_BOTH | TF_STUN_NO_EFFECTS, this );
						}
						pVictim->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward, WorldSpaceCenter(), 1, DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_TAUNTATK_UBERSLICE ) );
					}
				}
				else
				{
					// Launch them up a little
					vecForward = (WorldSpaceCenter() - pVictim->WorldSpaceCenter());
					VectorNormalize( vecForward );

					pVictim->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 12000, WorldSpaceCenter(), 500.0f, DMG_BULLET | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_TAUNTATK_UBERSLICE ) );

					CWeaponMedigun *pMedigun = (CWeaponMedigun *) Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
					if ( pMedigun )
					{
						pMedigun->AddCharge( 0.5f );
					}
				}
			}
		}

		if ( iTauntAttack == TAUNTATK_MEDIC_UBERSLICE_IMPALE )
		{
			m_iTauntAttack = TAUNTATK_MEDIC_UBERSLICE_KILL;
			m_flTauntAttackTime = gpGlobals->curtime + 0.75;
		}
	}
	else if ( iTauntAttack == TAUNTATK_DEMOMAN_BARBARIAN_SWING )
	{
		Vector vecForward;
		AngleVectors( EyeAngles(), &vecForward );
		Vector vecEnd = EyePosition() + vecForward * 128;

		trace_t tr;
		UTIL_TraceLine( EyePosition(), vecEnd, MASK_SOLID & ~CONTENTS_HITBOX, this, COLLISION_GROUP_PLAYER, &tr );

		if ( tr.fraction < 1.0 )
		{
			CBaseEntity *pEnt = tr.m_pEnt;

			if ( pEnt && pEnt->IsPlayer() && pEnt->GetTeamNumber() > LAST_SHARED_TEAM && pEnt->GetTeamNumber() != GetTeamNumber() )
			{
				vecForward = (WorldSpaceCenter() - pEnt->WorldSpaceCenter());
				VectorNormalize( vecForward );
				pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 12000, WorldSpaceCenter(), 500.0f, DMG_CLUB, TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING ) );
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_ENGINEER_GUITAR_SMASH )
	{
		Vector vecForward;
		AngleVectors( EyeAngles(), &vecForward );
		Vector vecEnd = EyePosition() + vecForward * 128;

		trace_t tr;
		UTIL_TraceLine( EyePosition(), vecEnd, MASK_SOLID & ~CONTENTS_HITBOX, this, COLLISION_GROUP_PLAYER, &tr );

		if ( tr.fraction < 1.0 )
		{
			CBaseEntity *pEnt = tr.m_pEnt;

			if ( pEnt && pEnt->IsPlayer() && pEnt->GetTeamNumber() > LAST_SHARED_TEAM && pEnt->GetTeamNumber() != GetTeamNumber() )
			{
				vecForward = (WorldSpaceCenter() - pEnt->WorldSpaceCenter());
				VectorNormalize( vecForward );
				pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 12, WorldSpaceCenter(), 500.0f, DMG_CLUB, TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH ) );
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_SHOW_ITEM )
	{
		if ( m_hTauntItem == NULL )
		{
			int itemCount = Inventory()->GetItemCount();

			CUtlVector< CEconItemView * > hatVector;

			for( int i=0; i<itemCount; ++i )
			{
				CEconItemView *econItemView = Inventory()->GetItem( i );

				int iSlot = econItemView->GetStaticData()->GetLoadoutSlot( GetPlayerClass()->GetClassIndex() );

				if ( iSlot == LOADOUT_POSITION_HEAD )
				{
					hatVector.AddToTail( econItemView );
				}
			}

			if ( hatVector.Count() > 0 )
			{
				int which = RandomInt( 0, hatVector.Count()-1 );

				CEconItemView *hatView = hatVector[ which ];

				int iHandBone = LookupBone( "weapon_bone" );
				if ( iHandBone != -1 )
				{
					Vector pos;
					QAngle angles;
					GetBonePosition( iHandBone, pos, angles );

					pos = Vector( 0, 0, 50.0f );

					m_hTauntItem = ItemGeneration()->GenerateItemFromScriptData( hatView, pos, angles, NULL );

					if ( m_hTauntItem != NULL )
					{
						m_hTauntItem->AddSolidFlags( FSOLID_NOT_SOLID );
						m_hTauntItem->SetOwnerEntity( this );
					}
				}
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_HIGHFIVE_PARTICLE )
	{
		if ( m_hHighFivePartner.Get() )
		{
			QAngle bodyAngles = BodyAngles();
			bodyAngles.x = 0;
			Vector vecForward, vecRight, vecUp;
			AngleVectors( bodyAngles, &vecForward, &vecRight, &vecUp );

			//Msg( "forward: %f %f %f   right: %f %f %f   up: %f %f %f\n", vecForward.x, vecForward.y, vecForward.z,
			//	vecRight.x, vecRight.y, vecRight.z,
			//	vecUp.x, vecUp.y, vecUp.z );

			Vector vecParticle = GetAbsOrigin() + (vecForward * 30.0f) + (vecRight * -3.0f) + (vecUp * 87.0f);
			//Msg( "particle: %f %f %f\n", vecParticle.x, vecParticle.y, vecParticle.z );

			CEffectData	data;
			data.m_nHitBox = GetParticleSystemIndex( GetTeamNumber() == TF_TEAM_RED ? "highfive_red" : "highfive_blue" );
			data.m_vOrigin = vecParticle;
			data.m_vAngles = vec3_angle;

			CPASFilter filter( data.m_vOrigin );
			filter.SetIgnorePredictionCull( true );

			te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );
		}
	}
	else if ( iTauntAttack == TAUNTATK_RPS_PARTICLE )
	{
		if ( m_hHighFivePartner.Get() )
		{
			bool bInitiatorWin = ( m_iTauntRPSResult / 3 ) == 0;

			// figure out for RPS
			// 0:rock 1:paper 2:scissors
			int iInitiator = m_iTauntRPSResult % 3;
			int iReceiver = ( iInitiator + ( bInitiatorWin ? 2 : 1 ) ) % 3;

			// offset to get the correct particle name
			if ( bInitiatorWin )
			{
				iInitiator += 3;
			}
			else
			{
				iReceiver += 3;
			}

			if ( GetTeamNumber() == TF_TEAM_BLUE )
			{
				iInitiator += 6;
			}

			if ( m_hHighFivePartner->GetTeamNumber() == TF_TEAM_BLUE )
			{
				iReceiver += 6;
			}

			DispatchRPSEffect( this, s_pszTauntRPSParticleNames[iInitiator] );
			DispatchRPSEffect( m_hHighFivePartner.Get(), s_pszTauntRPSParticleNames[iReceiver] );

			// setup time to kill the opposing team loser
			if ( GetTeamNumber() != m_hHighFivePartner->GetTeamNumber() )
			{
				m_iTauntAttack = TAUNTATK_RPS_KILL;
				m_flTauntAttackTime = m_flTauntRemoveTime - 1.2f;
			}

			IGameEvent *event = gameeventmanager->CreateEvent( "rps_taunt_event" );
			if ( event )
			{
				int iInitiatorRPS = m_iTauntRPSResult % 3;
				int iReceiverRPS = ( iInitiatorRPS + ( bInitiatorWin ? 2 : 1 ) ) % 3;

				event->SetInt( "winner", bInitiatorWin ? entindex() : m_hHighFivePartner.Get()->entindex() );
				event->SetInt( "winner_rps", bInitiatorWin ? iInitiatorRPS : iReceiverRPS );
				event->SetInt( "loser", bInitiatorWin ? m_hHighFivePartner.Get()->entindex() : entindex() );
				event->SetInt( "loser_rps",  bInitiatorWin ? iReceiverRPS : iInitiatorRPS );
				gameeventmanager->FireEvent( event );
			}
		}
	}
	else if ( iTauntAttack == TAUNTATK_RPS_KILL )
	{
		if ( m_hHighFivePartner.Get() )
		{
			bool bInitiatorWin = ( m_iTauntRPSResult / 3 ) == 0;

			CTFPlayer *pWinner = NULL;
			CTFPlayer *pLoser = NULL;
			if ( bInitiatorWin )
			{
				pWinner = this;
				pLoser = m_hHighFivePartner.Get();
			}
			else
			{
				pWinner = m_hHighFivePartner.Get();
				pLoser = this;
			}
			
			// gib the loser
			pLoser->m_bSuicideExplode = true;
			pLoser->TakeDamage( CTakeDamageInfo( pWinner, pWinner, NULL, 999, DMG_GENERIC, 0 ) );
		}
	}
	else if ( iTauntAttack == TAUNTATK_ENGINEER_TRICKSHOT )
	{
		// Engineer "Texan Trickshot" attack
		Vector vecForward;
		AngleVectors( EyeAngles(), &vecForward );
		Vector vecEnd = EyePosition() + vecForward * 500;

		trace_t tr;
		UTIL_TraceLine( EyePosition(), vecEnd, ( MASK_SOLID | CONTENTS_HITBOX ), this, COLLISION_GROUP_PLAYER, &tr );
		//		DebugDrawLine( EyePosition(), vecEnd, 0, 0, 255, true, 3.0f );

		if ( tr.fraction < 1.0 )
		{
			CBaseEntity *pEnt = tr.m_pEnt;

			if ( pEnt && pEnt->IsPlayer() && pEnt->GetTeamNumber() > LAST_SHARED_TEAM && pEnt->GetTeamNumber() != GetTeamNumber() )
			{
				// Launch them up a little
				AngleVectors( QAngle( -45, m_angEyeAngles[ YAW ], 0 ), &vecForward );
				pEnt->TakeDamage( CTakeDamageInfo( this, this, GetActiveTFWeapon(), vecForward * 25000, WorldSpaceCenter(), 500.0f, DMG_BULLET, TF_DMG_CUSTOM_TAUNTATK_TRICKSHOT ) );
			}
		}
	}

	// Particle Being played in VCD instead
	//else if ( iTauntAttack == TAUNTATK_FLIP_LAND_PARTICLE )
	//{
	//	if ( m_hHighFivePartner.Get() )
	//	{
	//		CEffectData	data;
	//		data.m_nHitBox = GetParticleSystemIndex( GetTeamNumber() == TF_TEAM_RED ? "taunt_flip_land_red" : "taunt_flip_land_blue" );
	//		data.m_vOrigin = m_hHighFivePartner.Get()->GetAbsOrigin();
	//		data.m_vAngles = m_hHighFivePartner.Get()->GetAbsAngles();

	//		CPASFilter filter( data.m_vOrigin );
	//		filter.SetIgnorePredictionCull( true );

	//		te->DispatchEffect( filter, 0.0, data.m_vOrigin, "ParticleEffect", data );
	//	}
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::GetSpecialDSP( void )
{
	int iSpecialDSP = 0;
	CALL_ATTRIB_HOOK_INT( iSpecialDSP, special_dsp );

	return iSpecialDSP;
}

//-----------------------------------------------------------------------------
// Purpose: Play a one-shot scene
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CTFPlayer::PlayScene( const char *pszScene, float flDelay, AI_Response *response, IRecipientFilter *filter )
{
	MDLCACHE_CRITICAL_SECTION();

	// This is a lame way to detect a taunt!
	if ( m_bInitTaunt )
	{
		m_bInitTaunt = false;
		return InstancedScriptedScene( this, pszScene, &m_hTauntScene, flDelay, false, response, true, filter );
	}
	else
	{
		return InstancedScriptedScene( this, pszScene, NULL, flDelay, false, response, true, filter );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	// If we have 'disguiseclass' criteria, pretend that we are actually our
	// disguise class. That way we just look up the scene we would play as if 
	// we were that class.
	int disguiseIndex = criteriaSet.FindCriterionIndex( "disguiseclass" );

	if ( disguiseIndex != -1 )
	{
		criteriaSet.AppendCriteria( "playerclass", criteriaSet.GetValue(disguiseIndex) );
	}
	else
	{
		if ( GetPlayerClass() )
		{
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[ GetPlayerClass()->GetClassIndex() ] );
		}
	}

	bool bRedTeam = ( GetTeamNumber() == TF_TEAM_RED );
	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		bRedTeam = ( m_Shared.GetDisguiseTeam() == TF_TEAM_RED );
	}
	criteriaSet.AppendCriteria( "OnRedTeam", bRedTeam ? "1" : "0" );

//=============================================================================
// HPE_BEGIN:
// [msmith]	When in training, we kill a lot of guys... a WHOLE LOT.  This was
//			triggering some response sounds that got very annoying after a while.
//=============================================================================
	if ( TFGameRules()->IsInTraining() )
	{
		criteriaSet.AppendCriteria( "recentkills", UTIL_VarArgs("%d", 0) );
	}
	else
	{
		criteriaSet.AppendCriteria( "recentkills", UTIL_VarArgs("%d", m_Shared.GetNumKillsInTime(30.0)) );
	}
//=============================================================================
// HPE_END
//=============================================================================

	int iTotalKills = 0;
	PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( this );
	if ( pStats )
	{
		iTotalKills = pStats->statsCurrentLife.m_iStat[TFSTAT_KILLS] + pStats->statsCurrentLife.m_iStat[TFSTAT_KILLASSISTS]+ 
			pStats->statsCurrentLife.m_iStat[TFSTAT_BUILDINGSDESTROYED];
	}
	criteriaSet.AppendCriteria( "killsthislife", UTIL_VarArgs( "%d", iTotalKills ) );
	criteriaSet.AppendCriteria( "disguised", m_Shared.InCond( TF_COND_DISGUISED ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "cloaked", ( m_Shared.IsStealthed() || m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "invulnerable", m_Shared.InCond( TF_COND_INVULNERABLE ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "beinghealed", m_Shared.InCond( TF_COND_HEALTH_BUFF ) ? "1" : "0" );
	criteriaSet.AppendCriteria( "waitingforplayers", (TFGameRules()->IsInWaitingForPlayers() || TFGameRules()->IsInPreMatch()) ? "1" : "0" );

	criteriaSet.AppendCriteria( "stunned", m_Shared.IsControlStunned() ? "1" : "0" );
	criteriaSet.AppendCriteria( "snared",  m_Shared.IsSnared() ? "1" : "0" );
	criteriaSet.AppendCriteria( "dodging",  (m_Shared.InCond( TF_COND_PHASE ) || m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION )) ? "1" : "0" );
	criteriaSet.AppendCriteria( "doublejumping", (m_Shared.GetAirDash()>0) ? "1" : "0" );

	switch ( GetTFTeam()->GetRole() )
	{
	case TEAM_ROLE_DEFENDERS:
		criteriaSet.AppendCriteria( "teamrole", "defense" );
		break;
	case TEAM_ROLE_ATTACKERS:
		criteriaSet.AppendCriteria( "teamrole", "offense" );
		break;
	}

	// Current weapon role
	CTFWeaponBase *pActiveWeapon = m_Shared.GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		int iWeaponRole = pActiveWeapon->GetTFWpnData().m_iWeaponType;
		switch( iWeaponRole )
		{
		case TF_WPN_TYPE_PRIMARY:
		default:
			criteriaSet.AppendCriteria( "weaponmode", "primary" );
			break;
		case TF_WPN_TYPE_SECONDARY:
			criteriaSet.AppendCriteria( "weaponmode", "secondary" );
			break;
		case TF_WPN_TYPE_MELEE:
			criteriaSet.AppendCriteria( "weaponmode", "melee" );
			break;
		case TF_WPN_TYPE_BUILDING:
			criteriaSet.AppendCriteria( "weaponmode", "building" );
			break;
		case TF_WPN_TYPE_PDA:
			criteriaSet.AppendCriteria( "weaponmode", "pda" );
			break;
		case TF_WPN_TYPE_ITEM1:
			criteriaSet.AppendCriteria( "weaponmode", "item1" );
			break;
		case TF_WPN_TYPE_ITEM2:
			criteriaSet.AppendCriteria( "weaponmode", "item2" );
			break;
		}

		if ( WeaponID_IsSniperRifle( pActiveWeapon->GetWeaponID() ) )
		{
			CTFSniperRifle *pRifle = dynamic_cast<CTFSniperRifle*>(pActiveWeapon);
			if ( pRifle && pRifle->IsZoomed() )
			{
				criteriaSet.AppendCriteria( "sniperzoomed", "1" );
			}
		}
		else if ( pActiveWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
		{
			CTFMinigun *pMinigun = dynamic_cast<CTFMinigun*>(pActiveWeapon);
			if ( pMinigun )
			{
				criteriaSet.AppendCriteria( "minigunfiretime", UTIL_VarArgs( "%.1f", pMinigun->GetFiringDuration() ) );
			}
		}

		CEconItemView *pItem = pActiveWeapon->GetAttributeContainer()->GetItem();
		if ( pItem && pItem->GetItemQuality() != AE_NORMAL )
		{
			criteriaSet.AppendCriteria( "item_name", pItem->GetStaticData()->GetDefinitionName() );
			criteriaSet.AppendCriteria( "item_type_name", pItem->GetStaticData()->GetItemTypeName() );
		}
	}

	// equipped loadout items
	{
		static const char* kSlotCriteriaName[CLASS_LOADOUT_POSITION_COUNT] = 
		{
			"loadout_slot_primary",		// LOADOUT_POSITION_PRIMARY = 0,
			"loadout_slot_secondary",	// LOADOUT_POSITION_SECONDARY,
			"loadout_slot_melee",		// LOADOUT_POSITION_MELEE,
			"loadout_slot_utility",		// LOADOUT_POSITION_UTILITY,
			"loadout_slot_building",	// LOADOUT_POSITION_BUILDING,
			"loadout_slot_pda",			// LOADOUT_POSITION_PDA,
			"loadout_slot_pda2",		// LOADOUT_POSITION_PDA2,
			"loadout_slot_head",		// LOADOUT_POSITION_HEAD,
			"loadout_slot_misc",		// LOADOUT_POSITION_MISC,
			"loadout_slot_action",		// LOADOUT_POSITION_ACTION,
			"loadout_slot_misc2",		// LOADOUT_POSITION_MISC2
			"loadout_slot_taunt",		// LOADOUT_POSITION_TAUNT
			"loadout_slot_taunt2",		// LOADOUT_POSITION_TAUNT2
			"loadout_slot_taunt3",		// LOADOUT_POSITION_TAUNT3
			"loadout_slot_taunt4",		// LOADOUT_POSITION_TAUNT4
			"loadout_slot_taunt5",		// LOADOUT_POSITION_TAUNT5
			"loadout_slot_taunt6",		// LOADOUT_POSITION_TAUNT6
			"loadout_slot_taunt7",		// LOADOUT_POSITION_TAUNT7
			"loadout_slot_taunt8",		// LOADOUT_POSITION_TAUNT8
		};
		COMPILE_TIME_ASSERT( ARRAYSIZE(kSlotCriteriaName) == CLASS_LOADOUT_POSITION_COUNT );
		CEconItemView *pItem = NULL;
		for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; ++i )
		{
			if ( m_EquippedLoadoutItemIndices[i] != LOADOUT_SLOT_USE_BASE_ITEM )
			{
				pItem = m_Inventory.GetInventoryItemByItemID( m_EquippedLoadoutItemIndices[i] );
				if ( pItem )
				{
					criteriaSet.AppendCriteria( kSlotCriteriaName[i], pItem->GetStaticData()->GetDefinitionName() );
				}
			}
		}
	}

	// Player under crosshair
	trace_t tr;
	Vector forward;
	EyeVectors( &forward );
	UTIL_TraceLine( EyePosition(), EyePosition() + (forward * MAX_TRACE_LENGTH), MASK_BLOCKLOS_AND_NPCS, this, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && pEntity->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer(pEntity);
			if ( pTFPlayer )
			{
				int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
				if ( !InSameTeam(pTFPlayer) )
				{
					// Prevent spotting stealthed enemies who haven't been exposed recently
					if ( pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
					{
						if ( pTFPlayer->m_Shared.GetLastStealthExposedTime() < (gpGlobals->curtime - 3.0) )
						{
							iClass = TF_CLASS_UNDEFINED;
						}
						else
						{
							iClass = TF_CLASS_SPY;
						}
					}
					else if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					{
						iClass = pTFPlayer->m_Shared.GetDisguiseClass();
					}
				}

				if ( iClass > TF_CLASS_UNDEFINED && iClass <= TF_LAST_NORMAL_CLASS )
				{
					criteriaSet.AppendCriteria( "crosshair_on", g_aPlayerClassNames_NonLocalized[iClass] );

					int iVisibleTeam = pTFPlayer->GetTeamNumber();
					if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
					{
						iVisibleTeam = pTFPlayer->m_Shared.GetDisguiseTeam();
					}

					if ( iVisibleTeam != GetTeamNumber() )
					{
						criteriaSet.AppendCriteria( "crosshair_enemy", "yes" );
					}
				}
			}
		}
	}

	// Previous round win
	bool bLoser = ( TFGameRules()->GetPreviousRoundWinners() != TEAM_UNASSIGNED && TFGameRules()->GetPreviousRoundWinners() != GetPrevRoundTeamNum() );
	criteriaSet.AppendCriteria( "LostRound", bLoser ? "1" : "0" );

	bool bPrevRoundTie = ( ( TFGameRules()->GetRoundsPlayed() > 0 ) && ( TFGameRules()->GetPreviousRoundWinners() == TEAM_UNASSIGNED ) );
	criteriaSet.AppendCriteria( "PrevRoundWasTie", bPrevRoundTie ? "1" : "0" );

	// Control points
	CTriggerAreaCapture *pAreaTrigger = GetControlPointStandingOn();
	if ( pAreaTrigger )
	{
		CTeamControlPoint *pCP = pAreaTrigger->GetControlPoint();
		if ( pCP )
		{
			if ( pCP->GetOwner() == GetTeamNumber() )
			{
				criteriaSet.AppendCriteria( "OnFriendlyControlPoint", "1" );
			}
			else 
			{
				if ( TeamplayGameRules()->TeamMayCapturePoint( GetTeamNumber(), pCP->GetPointIndex() ) && 
					 TeamplayGameRules()->PlayerMayCapturePoint( this, pCP->GetPointIndex() ) )
				{
					criteriaSet.AppendCriteria( "OnCappableControlPoint", "1" );
				}
			}
		}
	}

	bool bIsBonusTime = false;
	bool bGameOver = false;

	// Current game state
	criteriaSet.AppendCriteria( "GameRound", UTIL_VarArgs( "%d", TFGameRules()->State_Get() ) ); 
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		criteriaSet.AppendCriteria( "OnWinningTeam", ( TFGameRules()->GetWinningTeam() == GetTeamNumber() ) ? "1" : "0" ); 

		bIsBonusTime = ( TFGameRules()->GetStateTransitionTime() > gpGlobals->curtime );
		bGameOver = TFGameRules()->IsGameOver();
	}

	// Number of rounds played
	criteriaSet.AppendCriteria( "RoundsPlayed", UTIL_VarArgs( "%d", TFGameRules()->GetRoundsPlayed() ) );

	// Is this a 6v6 match?
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	bool bIsComp6v6 = ( pMatch && pMatch->m_eMatchGroup == k_eTFMatchGroup_Ladder_6v6 );
	criteriaSet.AppendCriteria( "IsComp6v6", bIsComp6v6 ? "1" : "0" );

	bool bIsCompWinner = m_Shared.InCond( TF_COND_COMPETITIVE_WINNER );
	criteriaSet.AppendCriteria( "IsCompWinner",  bIsCompWinner ? "1" : "0" );
	

	// Holiday Taunt
	int iSpecialTaunt = 0;
	if ( pActiveWeapon )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pActiveWeapon, iSpecialTaunt, special_taunt );
	}

	// only roll random halloween taunt if the active weapon doesn't have special taunt attribute
	if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) && iSpecialTaunt == 0 )
	{
		if ( !TFGameRules()->IsMannVsMachineMode() || ( GetTeamNumber() != TF_TEAM_PVE_INVADERS ) )
		{
			if ( pActiveWeapon )
			{
				int iRageTaunt = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pActiveWeapon, iRageTaunt, burn_damage_earns_rage );		
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pActiveWeapon, iRageTaunt, generate_rage_on_dmg );

				int iWeaponID = pActiveWeapon->GetWeaponID();
				if ( iWeaponID != TF_WEAPON_LUNCHBOX && !( iRageTaunt && m_Shared.GetRageMeter() >= 100.f ) )
				{
					float frand = (float) rand() / VALVE_RAND_MAX;
					if ( frand < 0.4f )
					{
						criteriaSet.AppendCriteria( "IsHalloweenTaunt", "1" );
					}
				}
			}
		}
	}

	if ( TFGameRules()->IsHolidayActive( kHoliday_AprilFools ) && iSpecialTaunt == 0 )
	{
		if ( pActiveWeapon )
		{
			int iRageTaunt = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pActiveWeapon, iRageTaunt, burn_damage_earns_rage );
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pActiveWeapon, iRageTaunt, generate_rage_on_dmg );

			int iWeaponID = pActiveWeapon->GetWeaponID();
			if ( iWeaponID != TF_WEAPON_LUNCHBOX && !( iRageTaunt && m_Shared.GetRageMeter() >= 100.f ) )
			{
				float frand = (float)rand() / VALVE_RAND_MAX;
				if ( frand < 0.8f )
				{		
					criteriaSet.AppendCriteria( "IsAprilFoolsTaunt", "1" );
				}
			}
		}
	}

	// Force the thriller taunt if we have the thriller condition
	if( m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
	{
		criteriaSet.AppendCriteria( "IsHalloweenTaunt", "1" );
	}

	// Only allow these rules if in the holiday
	if ( TFGameRules()->IsHolidayActive( kHoliday_HalloweenOrFullMoon ) && iSpecialTaunt == 0 )
	{
		// Halloween costume sets
		if ( IsRobotCostumeEquipped() )
		{
			criteriaSet.AppendCriteria( "IsRobotCostume", "1" );
		}
		else if ( IsDemowolf() )
		{
			criteriaSet.AppendCriteria( "IsDemowolf", "1" );
		}
		else if ( IsFrankenHeavy() )
		{
			criteriaSet.AppendCriteria( "IsFrankenHeavy", "1" );
		}
		// Single items with response rules
		else
		{
			static CSchemaAttributeDefHandle pAttrDef_AdditionalHalloweenResponseRule( "additional halloween response criteria name" );
			FOR_EACH_VEC_BACK( m_hMyWearables, wbl )
			{
				CEconWearable *pWearable = m_hMyWearables[wbl];
				if ( pWearable && pWearable->GetAttributeContainer()->GetItem() )
				{
					const char *pszAdditionalResponseRule = NULL;
					if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( pWearable->GetAttributeContainer()->GetItem(), pAttrDef_AdditionalHalloweenResponseRule, &pszAdditionalResponseRule ) )
					{
						criteriaSet.AppendCriteria( pszAdditionalResponseRule, "1" );
					}
				}
			}
		}

		// Zombie could work in addition to any of these
		if ( IsZombieCostumeEquipped() )
		{
			criteriaSet.AppendCriteria( "IsZombieCostume", "1" );
		}
	}

	if ( TFGameRules() && TFGameRules()->GetActiveBoss() && ( TFGameRules()->GetActiveBoss()->GetBossType() == HALLOWEEN_BOSS_MERASMUS ) )
	{
		CMerasmus* pMerasmus = assert_cast< CMerasmus* >( TFGameRules()->GetActiveBoss() );
		if ( pMerasmus )
		{
			if ( pMerasmus->IsHiding() )
			{
				criteriaSet.AppendCriteria( "IsMerasmusHiding", "1" );
			}
		}
	}

	bool bInHell = false;
	if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) && ( TFGameRules()->ArePlayersInHell() == true ) )
	{
		bInHell = true;
	}
	criteriaSet.AppendCriteria( "IsInHell", bInHell ? "1" : "0" );

	if ( TFGameRules()->IsHolidayActive( kHoliday_HalloweenOrFullMoonOrValentines ) )
	{
		if ( IsFairyHeavy() )
		{
			criteriaSet.AppendCriteria( "IsFairyHeavy", "1" );
		}
	}
	
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( GetTeamNumber() == TF_TEAM_PVE_DEFENDERS )
		{
			criteriaSet.AppendCriteria( "IsMvMDefender", "1" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTriggerAreaCapture *CTFPlayer::GetControlPointStandingOn( void )
{
	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch && pTouch->IsSolidFlagSet( FSOLID_TRIGGER ) && pTouch->IsBSPModel() )
			{
				CTriggerAreaCapture *pAreaTrigger = dynamic_cast<CTriggerAreaCapture*>(pTouch);
				if ( pAreaTrigger )
					return pAreaTrigger;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Usable by CTFPlayers, not just CTFBots
class CTFPlayertPathCost : public IPathCost
{
public:
	CTFPlayertPathCost( const CTFPlayer *me )
	{
		m_me = me;
		m_stepHeight = StepHeight;
		m_maxJumpHeight = 72.0f;
		m_maxDropHeight = 200.0f;
	}

	virtual float operator()( CNavArea *baseArea, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
	{
		VPROF_BUDGET( "CTFPlayertPathCost::operator()", "NextBot" );

		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			if ( !m_me->IsAreaTraversable( area ) )
			{
				return -1.0f;
			}

			// don't path through enemy spawn rooms
			if ( ( m_me->GetTeamNumber() == TF_TEAM_RED && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) ) ||
				 ( m_me->GetTeamNumber() == TF_TEAM_BLUE && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) ) )
			{
				if ( !TFGameRules()->RoundHasBeenWon() )
				{
					return -1.0f;
				}
			}

			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange( area );

			if ( deltaZ >= m_stepHeight )
			{
				if ( deltaZ >= m_maxJumpHeight )
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = 2.0f;
				dist *= jumpPenalty;
			}
			else if ( deltaZ < -m_maxDropHeight )
			{
				// too far to drop
				return -1.0f;
			}

			float cost = dist + fromArea->GetCostSoFar();

			return cost;
		}
	}

	const CTFPlayer *m_me;
	float m_stepHeight;
	float m_maxJumpHeight;
	float m_maxDropHeight;
};

//-----------------------------------------------------------------------------
// Given a vector of points, return the point we can actually travel to the quickest (requires a nav mesh)
CTeamControlPoint *CTFPlayer::SelectClosestControlPointByTravelDistance( CUtlVector< CTeamControlPoint * > *pointVector ) const
{
	if ( !pointVector || pointVector->Count() == 0 )
	{
		return NULL;
	}

	if ( GetLastKnownArea() == NULL )
	{
		return NULL;
	}

	CTeamControlPoint *closestPoint = NULL;
	float closestPointTravelRange = FLT_MAX;
	CTFPlayertPathCost cost( this );

	for( int i=0; i<pointVector->Count(); ++i )
	{
		CTeamControlPoint *point = pointVector->Element(i);
		
		if ( IsBot() && point->ShouldBotsIgnore() )
			continue;

		float travelRange = NavAreaTravelDistance( GetLastKnownArea(), TheTFNavMesh()->GetControlPointCenterArea( point->GetPointIndex() ), cost );

		if ( travelRange >= 0.0 && travelRange < closestPointTravelRange )
		{
			closestPoint = point;
			closestPointTravelRange = travelRange;
		}
	}

	return closestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// always can hear coach
	if ( m_hCoach && m_hCoach == pPlayer )
		return true;

	// always can hear student
	if ( m_hStudent && m_hStudent == pPlayer )
		return true;

	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

	// check if we're ignoring all but teammates
	if ( ( m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM ) && ( g_pGameRules->PlayerRelationship( this, pPlayer ) != GR_TEAMMATE ) )
		return false;

	// Always allow teams to hear each other in TD mode
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( IsHLTV() || IsReplay() )
			return true;
		
		return ( GetTeamNumber() == pPlayer->GetTeamNumber() );
	}

	if ( pPlayer->m_lifeState != LIFE_ALIVE && m_lifeState == LIFE_ALIVE )
	{
		// Everyone can chat like normal when the round/game ends
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN || TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
			return true;

		if ( !tf_gravetalk.GetBool() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanBeAutobalanced()
{
	if ( DuelMiniGame_IsInDuel( this ) )
		return false;

	if ( IsBot() )
		return false;

	if ( IsCoaching() )
		return false;

	if ( GetCoach() )
		return false;

	if ( m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		return false;
	
	if ( m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IResponseSystem *CTFPlayer::GetResponseSystem()
{
	int iClass = GetPlayerClass()->GetClassIndex();

	if ( m_bSpeakingConceptAsDisguisedSpy && m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iClass = m_Shared.GetDisguiseClass();
	}

	bool bValidClass = ( iClass >= TF_CLASS_SCOUT && iClass <= TF_LAST_NORMAL_CLASS );
	bool bValidConcept = ( m_iCurrentConcept >= 0 && m_iCurrentConcept < MP_TF_CONCEPT_COUNT );
	Assert( bValidClass );
	Assert( bValidConcept );

	if ( !bValidClass || !bValidConcept )
	{
		return BaseClass::GetResponseSystem();
	}
	else
	{
		return TFGameRules()->m_ResponseRules[iClass].m_ResponseSystems[m_iCurrentConcept];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::SpeakConceptIfAllowed( int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter )
{
	if ( !IsAlive() )
		return false;

	bool bReturn = false;

	if ( IsSpeaking() )
	{
		if ( iConcept != MP_CONCEPT_DIED )
			return false;
	}

	if ( iConcept == MP_CONCEPT_PLAYER_ASK_FOR_BALL )
	{
		if ( !SayAskForBall() ) 
			return false;
	}

	// Save the current concept.
	m_iCurrentConcept = iConcept;

	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !filter && ( iConcept != MP_CONCEPT_KILLED_PLAYER ) )
	{
		CSingleUserRecipientFilter filter(this);

		int iEnemyTeam = ( GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;

		// test, enemies and myself
		CTeamRecipientFilter disguisedFilter( iEnemyTeam );
		disguisedFilter.AddRecipient( this );

		CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
		Assert( pExpresser );

		pExpresser->AllowMultipleScenes();

		// play disguised concept to enemies and myself
		char buf[128];
		Q_snprintf( buf, sizeof(buf), "disguiseclass:%s", g_aPlayerClassNames_NonLocalized[ m_Shared.GetDisguiseClass() ] );

		if ( modifiers )
		{
			Q_strncat( buf, ",", sizeof(buf), 1 );
			Q_strncat( buf, modifiers, sizeof(buf), COPY_ALL_CHARACTERS );
		}

		m_bSpeakingConceptAsDisguisedSpy = true;

		bool bPlayedDisguised = SpeakIfAllowed( g_pszMPConcepts[iConcept], buf, pszOutResponseChosen, bufsize, &disguisedFilter );

		m_bSpeakingConceptAsDisguisedSpy = false;

		// test, everyone except enemies and myself
		CBroadcastRecipientFilter undisguisedFilter;
		undisguisedFilter.RemoveRecipientsByTeam( GetGlobalTFTeam(iEnemyTeam) );
		undisguisedFilter.RemoveRecipient( this );

		// play normal concept to teammates
		bool bPlayedNormally = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, &undisguisedFilter );

		pExpresser->DisallowMultipleScenes();

		bReturn = ( bPlayedDisguised || bPlayedNormally );
	}
	else
	{	
		if ( IsPlayerClass( TF_CLASS_SOLDIER ) && !filter && iConcept == MP_CONCEPT_PLAYER_MEDIC )
		{
			// Prevent the medic call+effect when we have the weapon_blocks_healing attribute
			CTFWeaponBase *pTFWeapon = GetActiveTFWeapon();
			if ( pTFWeapon )
			{
				int iBlockHealing = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFWeapon, iBlockHealing, weapon_blocks_healing );
				if ( iBlockHealing )
					return false;
			}
		}

		// play normally
		bReturn = SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter );
	}

	//Add bubble on top of a player calling for medic.
	if ( bReturn )
	{
		if ( iConcept == MP_CONCEPT_PLAYER_MEDIC )
		{
			SaveMe();
		}
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateExpression( void )
{
	char szScene[ MAX_PATH ];
	if ( !GetResponseSceneFromConcept( MP_CONCEPT_PLAYER_EXPRESSION, szScene, sizeof( szScene ) ) )
	{
		ClearExpression();
		m_flNextRandomExpressionTime = gpGlobals->curtime + RandomFloat(30,40);
		return;
	}
	
	// Ignore updates that choose the same scene
	if ( m_iszExpressionScene != NULL_STRING && stricmp( STRING(m_iszExpressionScene), szScene ) == 0 )
		return;

	if ( m_hExpressionSceneEnt )
	{
		ClearExpression();
	}

	m_iszExpressionScene = AllocPooledString( szScene );
	float flDuration = InstancedScriptedScene( this, szScene, &m_hExpressionSceneEnt, 0.0, true, NULL, true );
	m_flNextRandomExpressionTime = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearExpression( void )
{
	if ( m_hExpressionSceneEnt != NULL )
	{
		StopScriptedScene( this, m_hExpressionSceneEnt );
	}
	for ( int i = 0; i < MAX_FIRE_WEAPON_SCENES; i++ )
	{
		if ( m_hFireWeaponScenes[ i ] != NULL )
		{
			StopScriptedScene( this, m_hFireWeaponScenes[ i ] );
		}
	}
	m_flNextRandomExpressionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Only show subtitle to enemy if we're disguised as the enemy
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldShowVoiceSubtitleToEnemy( void )
{
	return ( m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseTeam() != GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: Don't allow rapid-fire voice commands
//-----------------------------------------------------------------------------
bool CTFPlayer::CanSpeakVoiceCommand( void )
{
	if ( tf_voice_command_suspension_mode.GetInt() == 1 )
		return false;

	if ( gpGlobals->curtime <= m_flNextVoiceCommandTime )
		return false;

	// misyl: New F2P voice command rate-limiting path.
	if ( BHaveChatSuspensionInCurrentMatch() && tf_voice_command_suspension_mode.GetInt() == 2 )
	{
		if ( !m_RateLimitedVoiceCommandTokenBucket.BTakeToken( gpGlobals->curtime ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Note the time we're allowed to next speak a voice command
//-----------------------------------------------------------------------------
void CTFPlayer::NoteSpokeVoiceCommand( const char *pszScenePlayed )
{
	Assert( pszScenePlayed );

	float flTimeSinceAllowedVoice = gpGlobals->curtime - m_flNextVoiceCommandTime;

	// if its longer than 5 seconds, reset the counter
	if ( flTimeSinceAllowedVoice > 5.0f )
	{
		m_iVoiceSpamCounter = 0;
	}
	// if its less than a second past the allowed time, player is spamming
	else if ( flTimeSinceAllowedVoice < 1.0f )
	{
		m_iVoiceSpamCounter++;
	}

	m_flNextVoiceCommandTime = gpGlobals->curtime + MIN( GetSceneDuration( pszScenePlayed ), tf_max_voice_speak_delay.GetFloat() );

	if ( m_iVoiceSpamCounter > 0 )
	{
		m_flNextVoiceCommandTime += m_iVoiceSpamCounter * 0.5f;
	}
}

extern ConVar friendlyfire;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	bool bIsMedic = false;
	bool bIsMeleeingTeamMate = false;

	if ( !friendlyfire.GetBool() )
	{
		//Do Lag comp on medics trying to heal team mates.
		if ( IsPlayerClass( TF_CLASS_MEDIC ) == true )
		{
			bIsMedic = true;

			if ( pPlayer->GetTeamNumber() == GetTeamNumber()  )
			{
				CWeaponMedigun *pWeapon = dynamic_cast <CWeaponMedigun*>( GetActiveWeapon() );

				if ( pWeapon && pWeapon->GetHealTarget() )
				{
					if ( pWeapon->GetHealTarget() == pPlayer )
						return true;
					else
						return false;
				}
			}
		}

		if ( pPlayer->GetTeamNumber() == GetTeamNumber() )
		{
			// Josh: Lag compensate melee attacks on teammates. Helps with weapons like the Solider's whip, etc.
			CTFWeaponBaseMelee *pWeapon = dynamic_cast< CTFWeaponBaseMelee * >( GetActiveWeapon() );
			if ( pWeapon )
			{
				bIsMeleeingTeamMate = true;
			}
			else
			{
				// Josh: Don't do any lag compensation on team-mates if we aren't a medic and not using melee.
				if ( bIsMedic == false )
					return false;
			}
		}
	}

	const Vector& vMyOrigin = GetAbsOrigin();
	const Vector& vHisOrigin = pPlayer->GetAbsOrigin();
	
	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pPlayer->entindex() ) )
		return false;

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5 * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// Josh: Don't do cone check when melee-ing team mates, as we could be inside them.
	if ( !bIsMeleeingTeamMate )
	{
		// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
		Vector vForward;
		AngleVectors( pCmd->viewangles, &vForward );

		Vector vDiff = vHisOrigin - vMyOrigin;
		VectorNormalize( vDiff );

		float flCosAngle = 0.707107f;	// 45 degree angle
		if ( vForward.Dot( vDiff ) < flCosAngle )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SpeakWeaponFire( int iCustomConcept )
{
	if ( iCustomConcept == MP_CONCEPT_NONE )
	{
		if ( m_flNextSpeakWeaponFire > gpGlobals->curtime )
			return;

		iCustomConcept = MP_CONCEPT_FIREWEAPON;
	}

	m_flNextSpeakWeaponFire = gpGlobals->curtime + 5;

	char szScene[ MAX_PATH ];
	if ( !GetResponseSceneFromConcept( iCustomConcept, szScene, sizeof( szScene ) ) )
		return;

	if ( m_hFireWeaponScenes[ m_nNextFireWeaponScene ] != NULL )
	{
		StopScriptedScene( this, m_hFireWeaponScenes[ m_nNextFireWeaponScene ] );
	}
	
	float flDuration = InstancedScriptedScene( this, szScene, &m_hFireWeaponScenes[ m_nNextFireWeaponScene ], 0.0, true, NULL, true);
	m_flNextSpeakWeaponFire = gpGlobals->curtime + flDuration;
	m_nNextFireWeaponScene = ( m_nNextFireWeaponScene + 1 ) % MAX_FIRE_WEAPON_SCENES;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearWeaponFireScene( void )
{
	m_flNextSpeakWeaponFire = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayer::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf( tempstr, sizeof( tempstr ),"Health: %d / %d ( %.1f )", GetHealth(), GetMaxHealth(), (float)GetHealth() / (float)GetMaxHealth() );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Get response scene corresponding to concept
//-----------------------------------------------------------------------------
bool CTFPlayer::GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes )
{
	AI_Response response;
	bool result = SpeakConcept( response, iConcept );

	if ( result )
	{
		// Apply contexts
		if ( response.IsApplyContextToWorld() )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) );
			if ( pEntity )
			{
				pEntity->AddContext( response.GetContext() );
			}
		}
		else
		{
			AddContext( response.GetContext() );
		}

		const char *szResponse = response.GetResponsePtr();
		Q_strncpy( chSceneBuffer, szResponse, numSceneBufferBytes );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:calculate a score for this player. higher is more likely to be switched
//-----------------------------------------------------------------------------
int	CTFPlayer::CalculateTeamBalanceScore( void )
{
	int iScore = BaseClass::CalculateTeamBalanceScore();

	// switch engineers less often
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		iScore -= 120;
	}

	return iScore;
}

//-----------------------------------------------------------------------------
// Purpose: Exclude during win state
//-----------------------------------------------------------------------------
void CTFPlayer::AwardAchievement( int iAchievement, int iCount )
{
	if ( TFGameRules()->State_Get() >= GR_STATE_TEAM_WIN )
	{
		// allow the Helltower loot island achievement during the bonus time
		if ( iAchievement != ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKULL_ISLAND_REWARD )
		{
			// reject in endround
			return;
		}
	}

	BaseClass::AwardAchievement( iAchievement, iCount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
// Debugging Stuff
void DebugParticles( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );

		// print out their conditions
		pPlayer->m_Shared.DebugPrintConditions();	
	}
}

static ConCommand sv_debug_stuck_particles( "sv_debug_stuck_particles", DebugParticles, "Debugs particles attached to the player under your crosshair.", FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: Debug concommand to set the player on fire
//-----------------------------------------------------------------------------
void IgnitePlayer()
{
	CTFPlayer *pPlayer = ToTFPlayer( ToTFPlayer( UTIL_PlayerByIndex( 1 ) ) );
	pPlayer->m_Shared.Burn( pPlayer, pPlayer->GetActiveTFWeapon() );
}
static ConCommand cc_IgnitePlayer( "tf_ignite_player", IgnitePlayer, "Sets you on fire", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestVCD( const CCommand &args )
{
	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			if ( args.ArgC() >= 2 )
			{
				InstancedScriptedScene( pPlayer, args[1], NULL, 0.0f, false, NULL, true );
			}
			else
			{
				InstancedScriptedScene( pPlayer, "scenes/heavy_test.vcd", NULL, 0.0f, false, NULL, true );
			}
		}
	}
}
static ConCommand tf_testvcd( "tf_testvcd", TestVCD, "Run a vcd on the player currently under your crosshair. Optional parameter is the .vcd name (default is 'scenes/heavy_test.vcd')", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TestRR( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg("No concept specified. Format is tf_testrr <concept>\n");
		return;
	}

	CBaseEntity *pEntity = NULL;
	const char *pszConcept = args[1];

	if ( args.ArgC() == 3 )
	{
		pszConcept = args[2];
		pEntity = UTIL_PlayerByName( args[1] );
	}

	if ( !pEntity || !pEntity->IsPlayer() )
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		if ( !pEntity || !pEntity->IsPlayer() )
		{
			pEntity = ToTFPlayer( UTIL_GetCommandClient() ); 
		}
	}

	if ( pEntity && pEntity->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( pPlayer )
		{
			int iConcept = GetMPConceptIndexFromString( pszConcept );
			if ( iConcept != MP_CONCEPT_NONE )
			{
				pPlayer->SpeakConceptIfAllowed( iConcept );
			}
			else
			{
				Msg( "Attempted to speak unknown multiplayer concept: %s\n", pszConcept );
			}
		}
	}
}
static ConCommand tf_testrr( "tf_testrr", TestRR, "Force the player under your crosshair to speak a response rule concept. Format is tf_testrr <concept>, or tf_testrr <player name> <concept>", FCVAR_CHEAT );

#ifdef _DEBUG
CON_COMMAND_F( tf_crashclients, "testing only, crashes about 50 percent of the connected clients.", FCVAR_DEVELOPMENTONLY )
{
	for ( int i = 1; i < gpGlobals->maxClients; ++i )
	{
		if ( RandomFloat( 0.0f, 1.0f ) < 0.5f )
		{
			CBasePlayer *pl = UTIL_PlayerByIndex( i + 1 );
			if ( pl )
			{
				engine->ClientCommand( pl->edict(), "crash\n" );
			}
		}
	}
}
#endif // _DEBUG
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldAnnounceAchievement( void )
{ 
	if ( BHaveChatSuspensionInCurrentMatch() )
		return false;

	if ( IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( m_Shared.IsStealthed() ||
			 m_Shared.InCond( TF_COND_DISGUISED ) ||
			 m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}
	}

	return BaseClass::ShouldAnnounceAchievement(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
medigun_charge_types CTFPlayer::GetChargeEffectBeingProvided( void )
{
	if ( !IsPlayerClass(TF_CLASS_MEDIC) )
		return MEDIGUN_CHARGE_INVALID;

	if ( !IsBot() )
	{
		INetChannelInfo *pNetChanInfo = engine->GetPlayerNetInfo( entindex() );
		if ( !pNetChanInfo || pNetChanInfo->IsTimingOut() )
			return MEDIGUN_CHARGE_INVALID;

		float flUberDuration = weapon_medigun_chargerelease_rate.GetFloat();

		// Return invalid when the medic hasn't sent a usercommand in awhile
		if ( GetTimeSinceLastUserCommand() > flUberDuration + 1.f )
			return MEDIGUN_CHARGE_INVALID;

		// Prevent an exploit where clients invalidate tickcount -
		// which causes their think functions to shut down
		if ( GetTimeSinceLastThink() > flUberDuration )
			return MEDIGUN_CHARGE_INVALID;
	}

	CTFWeaponBase *pWpn = GetActiveTFWeapon();
	if ( !pWpn )
		return MEDIGUN_CHARGE_INVALID;

	CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>(pWpn);
	if ( pMedigun && pMedigun->IsReleasingCharge() )
		return pMedigun->GetChargeType();

	return MEDIGUN_CHARGE_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY handler
//-----------------------------------------------------------------------------
void CTFPlayer::HandleAchievement_Medic_AssistHeavy( CTFPlayer *pPunchVictim )
{
	if ( !pPunchVictim )
	{
		// reset
		m_aPunchVictims.RemoveAll();
		return;
	}

	// we assisted punching this guy, while invuln

	// if this is a new unique punch victim
	if ( m_aPunchVictims.Find( pPunchVictim ) == m_aPunchVictims.InvalidIndex() )
	{
		m_aPunchVictims.AddToTail( pPunchVictim );

		if ( m_aPunchVictims.Count() >= 2 )
		{
			AwardAchievement( ACHIEVEMENT_TF_MEDIC_ASSIST_HEAVY );
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: ACHIEVEMENT_TF_PYRO_KILL_FROM_BEHIND handler
//-----------------------------------------------------------------------------
void CTFPlayer::HandleAchievement_Pyro_BurnFromBehind( CTFPlayer *pBurner )
{
	if ( !pBurner )
	{
		// reset
		m_aBurnFromBackAttackers.RemoveAll();
		return;
	}

	if ( m_aBurnFromBackAttackers.Find( pBurner ) == m_aBurnFromBackAttackers.InvalidIndex() )
	{
		m_aBurnFromBackAttackers.AddToTail( pBurner );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ResetPerRoundStats( void )
{
	m_Shared.ResetArenaNumChanges();
	BaseClass::ResetPerRoundStats();
}

//-----------------------------------------------------------------------------
// Purpose: Steam has just notified us that the player changed his inventory
//-----------------------------------------------------------------------------
void CTFPlayer::InventoryUpdated( CPlayerInventory *pInventory )
{
	m_Shared.SetLoadoutUnavailable( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SaveLastWeaponSlot( void )
{
	if( !m_bRememberLastWeapon && !m_bRememberActiveWeapon )
	  return;
	
	if ( GetLastWeapon() )
	{	
		if ( !m_bSwitchedClass )
		{
			if ( !m_bRememberLastWeapon )
			{
				m_iLastWeaponSlot = 0;

				CTFWeaponBase *pWpn = m_Shared.GetActiveTFWeapon();
				if ( pWpn && m_iLastWeaponSlot == pWpn->GetSlot() )
				{
					m_iLastWeaponSlot = (m_iLastWeaponSlot == 0) ? 1 : 0;
				}
			}
			else 
			{
				m_iLastWeaponSlot = GetLastWeapon()->GetSlot();

				if ( !m_bRememberActiveWeapon )
				{
	 				if ( m_iLastWeaponSlot == 0 && m_Shared.GetActiveTFWeapon() )
	 				{
	 					m_iLastWeaponSlot = m_Shared.GetActiveTFWeapon()->GetSlot();
	 				}
				}
			}
		}
		else	
		{
			m_iLastWeaponSlot = 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllWeapons()
{
	// Base class RemoveAllWeapons() doesn't remove them properly.
	// (doesn't call unequip, or remove immediately. Results in incorrect provision
	//  state for players over round restarts, because players have 2x weapon entities)
	ClearActiveWeapon();
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseCombatWeapon *pWpn = m_hMyWeapons[i];
		if ( pWpn )
		{
			Weapon_Detach( pWpn );
			UTIL_Remove( pWpn );
		}
	}

	m_Shared.RemoveDisguiseWeapon();

	// Remove all our wearables
	for ( int wbl = m_hMyWearables.Count()-1; wbl >= 0; wbl-- )
	{
		CEconWearable *pWearable = m_hMyWearables[wbl];
		if ( pWearable )
		{
			RemoveWearable( pWearable );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	BaseClass::Weapon_Equip( pWeapon );

	// Drop the flag if we're no longer supposed to be able to carry it
	// This can happen if we're carrying a flag and then pick up a weapon
	// that disallows flag carrying (ex. Rocket Jumper, Sticky Jumper)
	if ( !IsAllowedToPickUpFlag() && HasTheFlag() )
	{
		DropFlag();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::OnAchievementEarned( int iAchievement )
{
	BaseClass::OnAchievementEarned( iAchievement );

	SpeakConceptIfAllowed( MP_CONCEPT_ACHIEVEMENT_AWARD );
}

//-----------------------------------------------------------------------------
// Purpose: Handles USE keypress
//-----------------------------------------------------------------------------
void CTFPlayer::PlayerUse ( void )
{
	if ( tf_allow_player_use.GetBool() == false )
	{
		if ( !IsObserver() && !IsInCommentaryMode() )
		{
			return;
		}
	}

	BaseClass::PlayerUse();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearSpells()
{
	CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
	if ( pSpellBook )
	{
		// Take away players' spells on round restart
		pSpellBook->ClearSpell();
	}
}

void CTFPlayer::InputRoundSpawn( inputdata_t &inputdata )
{
	ClearSpells();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::Internal_HandleMapEvent( inputdata_t &inputdata )
{
	if ( FStrEq( "mvm_mannhattan", STRING( gpGlobals->mapname ) ) )
	{
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
			{
				if ( FStrEq( inputdata.value.String(), "banana" ) )
				{
					CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 0, 5.0 );
					if ( pRecentDamager && ( pRecentDamager->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS ) )
					{
						pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_MVM_MAPS_MANNHATTAN_MYSTERY );
					}
				}
				else if ( FStrEq( inputdata.value.String(), "pit" ) )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "mvm_mannhattan_pit" );
					if ( event )
					{
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}
	}
	else if ( FStrEq( "mvm_rottenburg", STRING( gpGlobals->mapname ) ) )
	{
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
			{
				if ( FStrEq( inputdata.value.String(), "pit" ) )
				{
					CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 0, 5.0 );
					if ( pRecentDamager && ( pRecentDamager->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS ) )
					{
						pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_MVM_MAPS_ROTTENBURG_PIT_GRIND );
					}
				}
			}
		}
	}

	BaseClass::Internal_HandleMapEvent( inputdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::IgnitePlayer()
{
	if ( FStrEq( "sd_doomsday", STRING( gpGlobals->mapname ) ) )
	{
		CTFPlayer *pRecentDamager = TFGameRules()->GetRecentDamager( this, 0, 5.0 );
		if ( pRecentDamager && ( pRecentDamager->GetTeamNumber() != GetTeamNumber() ) )
		{
			pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_MAPS_DOOMSDAY_PUSH_INTO_EXHAUST );
		}
	}

	m_Shared.Burn( this, NULL );
}

void CTFPlayer::InputIgnitePlayer( inputdata_t &inputdata )
{
	IgnitePlayer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModel( const char *pszModel )
{
	m_PlayerClass.SetCustomModel( pszModel );
	UpdateModel();
}

void CTFPlayer::InputSetCustomModel( inputdata_t &inputdata )
{
	SetCustomModel( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelWithClassAnimations( const char *pszModel )
{
	m_PlayerClass.SetCustomModel( pszModel, USE_CLASS_ANIMATIONS );
	UpdateModel();
}

void CTFPlayer::InputSetCustomModelWithClassAnimations( inputdata_t &inputdata )
{
	SetCustomModelWithClassAnimations( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelOffset( const Vector &offset )
{
	m_PlayerClass.SetCustomModelOffset( offset );
	InvalidatePhysicsRecursive( POSITION_CHANGED );
}

void CTFPlayer::InputSetCustomModelOffset( inputdata_t &inputdata )
{
	Vector vecTmp;
	inputdata.value.Vector3D( vecTmp );
	SetCustomModelOffset( vecTmp );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelRotation( const QAngle &angle )
{
	m_PlayerClass.SetCustomModelRotation( angle );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::InputSetCustomModelRotation( inputdata_t &inputdata )
{
	Vector vecTmp;
	inputdata.value.Vector3D( vecTmp );
	QAngle angTmp(vecTmp.x, vecTmp.y, vecTmp.z);
	m_PlayerClass.SetCustomModelRotation( angTmp );
	SetCustomModelRotation( angTmp );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearCustomModelRotation()
{
	m_PlayerClass.ClearCustomModelRotation();
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::InputClearCustomModelRotation( inputdata_t &inputdata )
{
	ClearCustomModelRotation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelRotates( bool bRotates )
{
	m_PlayerClass.SetCustomModelRotates( bRotates );
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
}

void CTFPlayer::InputSetCustomModelRotates( inputdata_t &inputdata )
{
	SetCustomModelRotates( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetCustomModelVisibleToSelf( bool bVisibleToSelf )
{
	m_PlayerClass.SetCustomModelVisibleToSelf( bVisibleToSelf );
}

void CTFPlayer::InputSetCustomModelVisibleToSelf( inputdata_t &inputdata )
{
	SetCustomModelVisibleToSelf( inputdata.value.Bool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetForcedTauntCam( int nForceTauntCam )
{
	m_nForceTauntCam = nForceTauntCam;
}

void CTFPlayer::InputSetForcedTauntCam( inputdata_t &inputdata )
{
	SetForcedTauntCam( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ExtinguishPlayerBurning()
{
	if ( m_Shared.InCond( TF_COND_BURNING ) )
	{
		EmitSound( "TFPlayer.FlameOut" );
		m_Shared.RemoveCond( TF_COND_BURNING );
	}
}

void CTFPlayer::InputExtinguishPlayer( inputdata_t &inputdata )
{
	ExtinguishPlayerBurning();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::InputTriggerLootIslandAchievement( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
	{
		if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_VIADUCT ) )
		{
			AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_LOOT_ISLAND );
		}
		else if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_LAKESIDE ) )
		{
			AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_MERASMUS_COLLECT_LOOT );
		}
		else if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
		{
			// the other maps require a min number of players before the boss appears but this one doesn't
			// so we need to have at least 1 player on the enemy team before granting the achievement
			CUtlVector< CTFPlayer* > playerVector;
			CollectHumanPlayers( &playerVector, ( GetTeamNumber() == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED );
			if ( playerVector.Count() >= 1 )
			{
				AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKULL_ISLAND_REWARD );
			}
		}

		IGameEvent *pEvent = gameeventmanager->CreateEvent( "escape_hell" );
		if ( pEvent )
		{
			pEvent->SetInt( "player", GetUserID() );
			gameeventmanager->FireEvent( pEvent, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::InputTriggerLootIslandAchievement2( inputdata_t &inputdata )
{
	// nothing here yet

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RollRareSpell()
{
	CTFSpellBook *pSpellBook = dynamic_cast< CTFSpellBook* >( GetEntityForLoadoutSlot( LOADOUT_POSITION_ACTION ) );
	if ( pSpellBook )
	{
		pSpellBook->RollNewSpell( 1 );

		CSingleUserRecipientFilter user( this );
		EmitSound( user, entindex(), "Halloween.Merasmus_TP_In" );
	}

	IGameEvent *pEvent = gameeventmanager->CreateEvent( "cross_spectral_bridge" );
	if ( pEvent )
	{
		pEvent->SetInt( "player", GetUserID() );
		gameeventmanager->FireEvent( pEvent, true );
	}
}

void CTFPlayer::InputRollRareSpell( inputdata_t &inputdata )
{
	RollRareSpell();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::BleedPlayer( float flBleedingTime )
{
	m_Shared.MakeBleed( this, GetActiveTFWeapon(), flBleedingTime );
}

void CTFPlayer::BleedPlayerEx( float flBleedingTime, int nBleedDmg, bool bPermenantBleeding, int nDmgType )
{
	m_Shared.MakeBleed( this, GetActiveTFWeapon(), flBleedingTime, nBleedDmg, bPermenantBleeding, nDmgType );
}

void CTFPlayer::InputBleedPlayer( inputdata_t &inputdata )
{
	BleedPlayer( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: Adds this damager to the history list of people who damaged player
//-----------------------------------------------------------------------------
void CAchievementData::AddDamagerToHistory( EHANDLE hDamager )
{
	if ( !hDamager )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hDamager;
	newHist.flTimeDamage = gpGlobals->curtime;
	aDamagers.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pDamager has damaged the player in the time specified
//-----------------------------------------------------------------------------
bool CAchievementData::IsDamagerInHistory( CBaseEntity *pDamager, float flTimeWindow )
{
	for ( int i = 0; i < aDamagers.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aDamagers[i].flTimeDamage ) > flTimeWindow )
			return false;

		if ( aDamagers[i].hEntity == pDamager )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of players who've damaged us in the time specified
//-----------------------------------------------------------------------------
int	CAchievementData::CountDamagersWithinTime( float flTime )
{
	int iCount = 0;
	for ( int i = 0; i < aDamagers.Count(); i++ )
	{
		if ( gpGlobals->curtime - aDamagers[i].flTimeDamage < flTime )
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementData::AddTargetToHistory( EHANDLE hTarget )
{
	if ( !hTarget )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hTarget;
	newHist.flTimeDamage = gpGlobals->curtime;
	aTargets.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAchievementData::IsTargetInHistory( CBaseEntity *pTarget, float flTimeWindow )
{
	for ( int i = 0; i < aTargets.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aTargets[i].flTimeDamage ) > flTimeWindow )
			return false;

		if ( aTargets[i].hEntity == pTarget )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAchievementData::CountTargetsWithinTime( float flTime )
{
	int iCount = 0;
	for ( int i = 0; i < aTargets.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aTargets[i].flTimeDamage ) < flTime )
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementData::DumpDamagers( void )
{
	Msg("Damagers:\n");
	for ( int i = 0; i < aDamagers.Count(); i++ )
	{
		if ( aDamagers[i].hEntity )
		{
			if ( aDamagers[i].hEntity->IsPlayer() )
			{
				Msg("   %s : at %.2f (%.2f ago)\n", ToTFPlayer(aDamagers[i].hEntity)->GetPlayerName(), aDamagers[i].flTimeDamage, gpGlobals->curtime - aDamagers[i].flTimeDamage );
			}
			else
			{
				Msg("   %s : at %.2f (%.2f ago)\n", aDamagers[i].hEntity->GetDebugName(), aDamagers[i].flTimeDamage, gpGlobals->curtime - aDamagers[i].flTimeDamage );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds this attacker to the history of people who damaged this player
//-----------------------------------------------------------------------------
void CAchievementData::AddDamageEventToHistory( EHANDLE hAttacker, float flDmgAmount /*= 0.f*/ )
{
	if ( !hAttacker )
		return;

	EntityDamageHistory_t newHist;
	newHist.hEntity = hAttacker;
	newHist.flTimeDamage = gpGlobals->curtime;
	newHist.nDamageAmount = flDmgAmount;
	aDamageEvents.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pEntity has damaged the player in the time specified
//-----------------------------------------------------------------------------
bool CAchievementData::IsEntityInDamageEventHistory( CBaseEntity *pEntity, float flTimeWindow )
{
	for ( int i = 0; i < aDamageEvents.Count(); i++ )
	{
		if ( aDamageEvents[i].hEntity != pEntity )
			continue;

		// Sorted
		if ( ( gpGlobals->curtime - aDamageEvents[i].flTimeDamage ) > flTimeWindow )
			break;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: The sum of damage events from pEntity
//-----------------------------------------------------------------------------
int CAchievementData::GetAmountForDamagerInEventHistory( CBaseEntity *pEntity, float flTimeWindow )
{
	int nAmount = 0;

	for ( int i = 0; i < aDamageEvents.Count(); i++ )
	{
		if ( aDamageEvents[i].hEntity != pEntity )
			continue;
		
		// Msg( "   %s : at %.2f (%.2f ago)\n", ToTFPlayer( aDamageEvents[i].hEntity )->GetPlayerName(), aDamageEvents[i].flTimeDamage, gpGlobals->curtime - aDamageEvents[i].flTimeDamage );

		// Sorted
		if ( ( gpGlobals->curtime - aDamageEvents[i].flTimeDamage ) > flTimeWindow )
			break;

		nAmount += aDamageEvents[i].nDamageAmount;
	}

	return nAmount;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CAchievementData::GetFirstEntryTimeForDamagerInHistory( CBaseEntity *pEntity )
{
	float flEarliestTime = gpGlobals->curtime;

	for ( int i = 0; i < aDamageEvents.Count(); i++ )
	{
		if ( aDamageEvents[i].hEntity != pEntity )
			continue;

		// Sorted
		if ( aDamageEvents[i].flTimeDamage < flEarliestTime )
		{
			flEarliestTime = aDamageEvents[i].flTimeDamage;
		}
	}

	return flEarliestTime;
}

//-----------------------------------------------------------------------------
// Purpose: Adds hPlayer to the history of people who pushed this player
//-----------------------------------------------------------------------------
void CAchievementData::AddPusherToHistory( EHANDLE hPlayer )
{
	if ( !hPlayer )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hPlayer;
	newHist.flTimeDamage = gpGlobals->curtime;
	aPushers.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pPlayer pushed the player in the time specified
//-----------------------------------------------------------------------------
bool CAchievementData::IsPusherInHistory( CBaseEntity *pPlayer, float flTimeWindow )
{
	for ( int i = 0; i < aPushers.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aPushers[i].flTimeDamage ) > flTimeWindow )
			return false;

		if ( aPushers[i].hEntity == pPlayer )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Adds this damager to the history list of people whose sentry damaged player
//-----------------------------------------------------------------------------
void CAchievementData::AddSentryDamager( EHANDLE hDamager, EHANDLE hObject )
{
	if ( !hDamager )
		return;

	EntityHistory_t newHist;
	newHist.hEntity = hDamager;
	newHist.hObject = hObject;
	newHist.flTimeDamage = gpGlobals->curtime;
	aSentryDamagers.InsertHistory( newHist );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not pDamager has damaged the player in the time specified (by way of sentry gun)
//-----------------------------------------------------------------------------
EntityHistory_t* CAchievementData::IsSentryDamagerInHistory( CBaseEntity *pDamager, float flTimeWindow )
{
	for ( int i = 0; i < aSentryDamagers.Count(); i++ )
	{
		if ( ( gpGlobals->curtime - aSentryDamagers[i].flTimeDamage ) > flTimeWindow )
			return NULL;

		if ( aSentryDamagers[i].hEntity == pDamager )
		{
			return &aSentryDamagers[i];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Client has sent us some KVs describing an item they want to test.
//-----------------------------------------------------------------------------
void CTFPlayer::ItemTesting_Start( KeyValues *pKV )
{
	static itemid_t s_iTestIndex = 1;

	// We have to be a listen server, with 1 player on it, and the request must come from the listen client.
	if ( this != UTIL_GetListenServerHost() )
		return;
	int iPlayers = 0;
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer && !pPlayer->IsFakeClient() )
		{
			iPlayers++;
		}
	}
	if ( iPlayers > 1 )
		return;

	// We also need to be on the item testing map.
	if ( !Q_stricmp(STRING(gpGlobals->mapname), "item_test.bsp" ) )
		return;

	FOR_EACH_VEC( m_ItemsToTest, i )
	{
		m_ItemsToTest[i].pKV->deleteThis();
	}
	m_ItemsToTest.Purge();

	TFGameRules()->SetInItemTestingMode( true );

	int iClassUsage = pKV->GetInt( "class_usage", 0 );

	ItemTesting_DeleteItems(); // Remove items before creating new defs. Some def clean-up depends on existing static values.

	for ( int iItemType = 0; iItemType < TI_TYPE_COUNT; iItemType++ )
	{
		KeyValues *pItemKV = pKV->FindKey( UTIL_VarArgs("Item%d",iItemType) );
		if ( !pItemKV )
			continue;

		// We need to copy these, because the econ item def will want to point at pieces of it
		int iNewItem = m_ItemsToTest.AddToTail();
		m_ItemsToTest[iNewItem].pKV = pItemKV->MakeCopy();
		m_ItemsToTest[iNewItem].pKV->SetInt( "class_usage", iClassUsage );

		bool bTestingExistingItem = pItemKV->GetBool( "test_existing_item", false );
		item_definition_index_t iReplacedItemDef = pItemKV->GetInt( "item_replace", INVALID_ITEM_DEF_INDEX );

		item_definition_index_t iNewDef = pItemKV->GetInt( "item_def", INVALID_ITEM_DEF_INDEX );
		if ( iNewDef == INVALID_ITEM_DEF_INDEX )
			return;

		// Create the econ item data from it
		ItemSystem()->GetItemSchema()->ItemTesting_CreateTestDefinition( iReplacedItemDef, iNewDef, m_ItemsToTest[iNewItem].pKV );

		// Build our test script item 
		m_ItemsToTest[iNewItem].scriptItem.Init( iNewDef, AE_USE_SCRIPT_VALUE, AE_USE_SCRIPT_VALUE, false );
		if ( !m_ItemsToTest[iNewItem].scriptItem.GetStaticData() )
			return;

		m_ItemsToTest[iNewItem].scriptItem.SetItemID( s_iTestIndex );
		s_iTestIndex++;

		bool bPrecache = !bTestingExistingItem;
		if ( bPrecache )
		{
			// Only dynamically load definitions tagged as streamable
			GameItemDefinition_t *pEconItemDef = m_ItemsToTest[iNewItem].scriptItem.GetStaticData();
			bPrecache = !pEconItemDef->IsContentStreamable();
		}
		if ( bPrecache )
		{
			bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
			CBaseEntity::SetAllowPrecache( true );
			for ( int i = 0; i < LOADOUT_COUNT; i++ )
			{
				const char *pszModel = m_ItemsToTest[iNewItem].scriptItem.GetStaticData()->GetPlayerDisplayModel(i);
				if ( pszModel && pszModel[0] )
				{
					int iModelIndex = CBaseEntity::PrecacheModel( pszModel );
					PrecacheGibsForModel( iModelIndex );
				}
			}
			CBaseEntity::SetAllowPrecache( bAllowPrecache );
		}
	}

	// Spawn the right bots, and give them the item
	ItemTesting_UpdateBots( pKV );

	// Make the player respawn (he might have been holding test weapons)
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( 1 ) );
	if ( pPlayer && !pPlayer->IsFakeClient() )
	{
		if ( pPlayer->IsAlive() )
		{
			pPlayer->m_bItemTestingRespawn = true;
		}
		pPlayer->ForceRespawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ItemTesting_DeleteItems()
{
	// Take away every test item.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->RemoveAllItems();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ItemTesting_UpdateBots( KeyValues *pKV )
{
	bool bNeedsBot[TF_LAST_NORMAL_CLASS];
	memset( bNeedsBot, 0, sizeof(bNeedsBot) );

	// Figure out what classes we'll need for all the items we're testing
	FOR_EACH_VEC( m_ItemsToTest, i )
	{
		CEconItemView *pItem = &m_ItemsToTest[i].scriptItem;
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
		{
			if ( pItem->GetStaticData()->CanBeUsedByClass(iClass) )
			{
				bNeedsBot[iClass] = true;
			}
		}
	}

	bool bAutoAdd = pKV->GetInt( "auto_add_bots", 1 ) != 0;
	bool bBlueTeam = pKV->GetInt( "bots_on_blue_team", 0 ) != 0;

	// Kick every bot that's not one of the valid classes for the item
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsFakeClient() )
		{
			int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
			bool bWrongTeam = pPlayer->GetTeamNumber() != (bBlueTeam ? TF_TEAM_BLUE : TF_TEAM_RED);
			if ( bAutoAdd && (!bNeedsBot[iClass] || bWrongTeam) )
			{
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );
			}
			else
			{
				bNeedsBot[iClass] = false;
				pPlayer->m_bItemTestingRespawn = true;
				pPlayer->ForceRespawn();
			}
		}
	}

	// Spawn bots of each class that uses the item (if we're doing auto addition)
	if ( bAutoAdd )
	{
		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			if ( bNeedsBot[i] )
			{
				engine->ServerCommand( UTIL_VarArgs( "bot -team %s -class %s\n", bBlueTeam ? "blue" : "red", g_aPlayerClassNames_NonLocalized[i] ) );
			}
		}
	}

	TFGameRules()->ItemTesting_SetupFromKV( pKV );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView	*CTFPlayer::ItemTesting_GetTestItem( int iClass, int iSlot )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( 1 ) );
	if ( pPlayer && !pPlayer->IsFakeClient() )
	{
		// Loop through all the items we're testing
		FOR_EACH_VEC( pPlayer->m_ItemsToTest, i )
		{
			CEconItemView *pItem = &pPlayer->m_ItemsToTest[i].scriptItem;
			if ( !pItem->GetStaticData()->CanBeUsedByClass( iClass ) )
				continue;

			if ( pItem->GetStaticData()->GetLoadoutSlot( iClass ) == iSlot )
				return pItem;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::GetReadyToTauntWithPartner( void )
{
	m_bIsReadyToHighFive = true;

	/*IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_highfive_start" );
	if ( pEvent )
	{
		pEvent->SetInt( "entindex", entindex() );

		gameeventmanager->FireEvent( pEvent );
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CancelTauntWithPartner( void )
{
	m_bIsReadyToHighFive = false;

	/*IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_highfive_cancel" );
	if ( pEvent )
	{
		pEvent->SetInt( "entindex", entindex() );

		gameeventmanager->FireEvent( pEvent );
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::StopTauntSoundLoop()
{
	if ( !m_strTauntSoundLoopName.IsEmpty() )
	{
		CReliableBroadcastRecipientFilter filter;
		UserMessageBegin( filter, "PlayerTauntSoundLoopEnd" );
			WRITE_BYTE( entindex() );
		MessageEnd();

		m_strTauntSoundLoopName = "";
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look for a nearby players who has started a 
// high five and is waiting for a partner
//-----------------------------------------------------------------------------
CTFPlayer *CTFPlayer::FindPartnerTauntInitiator( void )
{
	if ( tf_highfive_debug.GetBool() )
		Msg( "%s looking for a partner taunt initiator.\n", GetPlayerName() );

	CTFPlayer *pTargetInitiator = NULL;
	float flDistSqrToTargetInitiator = FLT_MAX;

	CUtlVector< CTFPlayer* > playerList;
	CollectPlayers( &playerList, GetAllowedTauntPartnerTeam(), true );
	for( int t=0; t<playerList.Count(); ++t )
	{
		CTFPlayer *pPlayer = playerList[t];

		if ( pPlayer == this )
			continue;

		// don't allow bot to taunt with each other
		if ( pPlayer->IsBot() && IsBot() )
			continue;

		if ( !pPlayer->IsReadyToTauntWithPartner() )
			continue;

		if ( tf_highfive_debug.GetBool() )
			Msg( "%s is ready to %s.\n", pPlayer->GetPlayerName(), pPlayer->m_TauntEconItemView.GetStaticData()->GetDefinitionName() );

		Vector toPartner = pPlayer->GetAbsOrigin() - GetAbsOrigin();
		float flDistSqrToPlayer = toPartner.LengthSqr();
		if ( flDistSqrToPlayer > Square( tf_highfive_max_range.GetFloat() ) )
		{
			if ( tf_highfive_debug.GetBool() )
				Msg( " - but that player was too far away.\n" );
				
			// too far away
			continue;
		}

		// skip if this player is too far to be our initiator
		if ( flDistSqrToPlayer >= flDistSqrToTargetInitiator )
		{
			if ( tf_highfive_debug.GetBool() )
			{
				Msg( " - is further than the current potential initiator.\n" );
			}
			continue;
		}

		toPartner.NormalizeInPlace();
			
		Vector forward;
		EyeVectors( &forward );

		// check if I'm facing this player
		if ( DotProduct( toPartner, forward ) < 0.6f )
		{
			if ( tf_highfive_debug.GetBool() )
				Msg( " - but we are not looking at that player.\n" );

			// we are not looking at this partner
			continue;
		}

		bool bShouldCheckFacing = !pPlayer->m_bTauntMimic;
		// check if the player is facing us
		if ( bShouldCheckFacing )
		{
			Vector partnerForward = pPlayer->BodyDirection2D();
			float toPartnerDotProduct = DotProduct( toPartner, partnerForward );
			if ( tf_highfive_debug.GetBool() )
				Msg( " - dot product to partner is %f\n", toPartnerDotProduct );

			if ( toPartnerDotProduct > -0.6f )
			{
				if ( tf_highfive_debug.GetBool() )
					Msg( " - but that player is not facing us.\n" );

				// they are not facing us
				continue;
			}
		}

		// check if there's something between us
		trace_t result;
		CTraceFilterIgnoreTeammates filter( this, COLLISION_GROUP_NONE, GetAllowedTauntPartnerTeam() );
		UTIL_TraceHull( GetAbsOrigin(), pPlayer->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, &filter, &result );
		if ( result.DidHit() )
		{
			if ( tf_highfive_debug.GetBool() )
				Msg( " - entity [%i %s %s] in between. tracing again with tolerance.\n",
					result.GetEntityIndex(),
					result.m_pEnt ? result.m_pEnt->GetClassname() : "NULL",
					result.surface.name );

			Vector offset( 0, 0, tf_highfive_height_tolerance.GetFloat() );
			trace_t result2;
			UTIL_TraceHull( GetAbsOrigin() + offset, pPlayer->GetAbsOrigin() + offset, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, &filter, &result2 );
			if ( result2.DidHit() )
			{
				if ( tf_highfive_debug.GetBool() )
					Msg( " - entity [%i %s %s] in between.\n",
					result2.GetEntityIndex(),
					result2.m_pEnt ? result2.m_pEnt->GetClassname() : "NULL",
					result2.surface.name );

				// something is in between us
				continue;
			}
		}

		// Check to see if there's a spawn room visualizer between us and our partner
		if( PointsCrossRespawnRoomVisualizer( WorldSpaceCenter(), pPlayer->WorldSpaceCenter() ) )
		{
			if ( tf_highfive_debug.GetBool() )
				Msg( " - spawn room visualizer in between.\n" );

			continue;
		}

		// update to closer target player
		if ( flDistSqrToPlayer < flDistSqrToTargetInitiator )
		{
			// success!
			if ( tf_highfive_debug.GetBool() )
				Msg( " - is potentially the closest target player.\n" );
			flDistSqrToTargetInitiator = flDistSqrToPlayer;	
			pTargetInitiator = pPlayer;
		}
		else if ( tf_highfive_debug.GetBool() )
		{
			Msg( " - is further than the current target player.\n" );
		}
	}

	// pick the closest target player over the closest player
	return pTargetInitiator;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool SelectPartnerTaunt( const GameItemDefinition_t *pItemDef, CTFPlayer *initiator, CTFPlayer *receiver, const char **pszInitiatorScene, const char **pszReceiverScene )
{
	static CSchemaItemDefHandle pItemDef_rpsTaunt( "RPS Taunt" );
	static CSchemaItemDefHandle pItemDef_TauntNeckSnap( "Taunt: Neck Snap" );
	static CSchemaItemDefHandle pItemDef_TauntBearHug( "Taunt: Bear Hug" );

	CTFTauntInfo *pTauntData = pItemDef->GetTauntData();
	if ( !pTauntData )
		return false;

	int iInitiatorClass = initiator->GetPlayerClass()->GetClassIndex();
	int iReceiverClass = receiver->GetPlayerClass()->GetClassIndex();

	// check if we have any scene
	const int iInitiatorSceneCount = pTauntData->GetPartnerTauntInitiatorSceneCount( iInitiatorClass );
	const int iReceiverSceneCount = pTauntData->GetPartnerTauntReceiverSceneCount( iReceiverClass );
	if ( iInitiatorSceneCount == 0 ||
		 iReceiverSceneCount == 0 )
	{
		return false;
	}

	int iInitiator = 0;
	int iReceiver = 0;
	if ( pItemDef == pItemDef_rpsTaunt )
	{
		Assert( iInitiatorSceneCount == 6 && iReceiverSceneCount == 6 );

		int iWinner = RandomInt( 0, 2 );
		int iLoser =  ( ( iWinner + 2 ) % 3 ) + 3;

		/*static const char* s_pszRPS[3] = { "rock", "paper", "scissor" };
		DevMsg( "%s beats %s\n", s_pszRPS[iWinner], s_pszRPS[iLoser%3] );*/

		if ( RandomInt( 0, 1 ) )
		{
			iInitiator = iWinner;
			iReceiver = iLoser;
		}
		else
		{
			iInitiator = iLoser;
			iReceiver = iWinner;
		}

		initiator->SetRPSResult( iInitiator );
	}
	else if ( pItemDef == pItemDef_TauntNeckSnap )
	{
		Assert( iInitiatorSceneCount == 2 && iReceiverSceneCount > 0 );

		iInitiator = 0;
		iReceiver = ( iReceiverClass != TF_CLASS_SOLDIER ) ? 0 : 1;
	}
	else if ( pItemDef == pItemDef_TauntBearHug )
	{
		Assert( iInitiatorSceneCount == 2 && iReceiverSceneCount > 0 );

		iInitiator = 0;
		iReceiver = ( iReceiverClass != TF_CLASS_HEAVYWEAPONS ) ? 0 : 1;
	}
	else
	{
		// randomly select a player to pick 0 (could be silent taunt)
		// and other player select a different one if there's any
		if ( RandomInt( 0, 1 ) == 0 )
		{
			iReceiver = iReceiverSceneCount > 1 ? RandomInt( 1, iReceiverSceneCount - 1 ) : 0;
		}
		else
		{
			iInitiator = iInitiatorSceneCount > 1 ? RandomInt( 1, iInitiatorSceneCount - 1 ) : 0;
		}
	}

	*pszInitiatorScene = pTauntData->GetPartnerTauntInitiatorScene( iInitiatorClass, iInitiator );
	*pszReceiverScene = pTauntData->GetPartnerTauntReceiverScene( iReceiverClass, iReceiver );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AcceptTauntWithPartner( CTFPlayer *initiator )
{
	if ( !initiator )
	{
		return;
	}

	if ( tf_highfive_debug.GetBool() )
		Msg( "%s doing %s with initiator %s.\n", GetPlayerName(), initiator->m_TauntEconItemView.GetStaticData()->GetDefinitionName(), initiator->GetPlayerName() );

	// make sure this won't get us stuck
	Vector newOrigin;
	float flTolerance;
	if ( !initiator->FindOpenTauntPartnerPosition( &initiator->m_TauntEconItemView, newOrigin, &flTolerance ))
	{
		if ( tf_highfive_debug.GetBool() )
			Msg( " - but there is no open space for us.\n" );

		return;
	}

	trace_t result;
	CTraceFilterIgnoreTeammates filter( this, COLLISION_GROUP_NONE, GetTeamNumber() );
	UTIL_TraceHull( newOrigin, newOrigin - Vector( 0, 0, flTolerance ), VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, &filter, &result );
	if ( !result.DidHit() )
	{
		if ( tf_highfive_debug.GetBool() )
			Msg( " - there's too much space underneath where we need to be.\n" );

		return;
	}
	else
	{
		newOrigin = result.endpos;
	}

	trace_t	stucktrace;
	UTIL_TraceEntity( this, newOrigin, newOrigin, MASK_PLAYERSOLID, &filter, &stucktrace );
	if ( stucktrace.startsolid != 0 )
	{
		if ( tf_highfive_debug.GetBool() )
			Msg( " - but we'd get stuck on entity [%i %s %s] going in front of %s.\n",
			stucktrace.GetEntityIndex(),
			stucktrace.m_pEnt ? stucktrace.m_pEnt->GetClassname() : "NULL",
			stucktrace.surface.name,
			initiator->GetPlayerName() );

		return;
	}

	// move us into facing position with initiator
	SetAbsOrigin( newOrigin );
	QAngle newAngles = initiator->GetAbsAngles();
	// turn 180 degree to face the initiator
	newAngles[YAW] = AngleNormalize( newAngles[YAW] - 180 );
	SetAbsAngles( newAngles );

	m_bIsReadyToHighFive = false;
	initiator->m_bIsReadyToHighFive = false;

	// note who our partner is so we can lock our facing toward them on the client
	m_hHighFivePartner = initiator;
	initiator->m_hHighFivePartner = this;

	if ( initiator->m_hTauntScene.Get() )
	{
		StopScriptedScene( initiator, initiator->m_hTauntScene );
		initiator->m_hTauntScene = NULL;

		initiator->StopTauntSoundLoop();
	}

	const char *pszInitiatorScene = NULL;
	const char *pszOurScene = NULL;
	const GameItemDefinition_t *pItemDef = initiator->m_TauntEconItemView.GetItemDefinition();
	if ( !SelectPartnerTaunt( pItemDef, initiator, this, &pszInitiatorScene, &pszOurScene ) )
	{
		if ( tf_highfive_debug.GetBool() )
		{
			Msg( "SpeakConceptIfAllowed failed on partner taunt initiator. Aborting taunt.\n" );
		}
		AssertMsg( false, "SpeakConceptIfAllowed failed on partner taunt initiator. Aborting taunt." );

		initiator->m_flTauntRemoveTime = gpGlobals->curtime;
		initiator->m_bAllowedToRemoveTaunt = true;

		return;
	}
	m_TauntEconItemView = initiator->m_TauntEconItemView;

	int initiatorConcept = MP_CONCEPT_HIGHFIVE_SUCCESS_FULL;
	int ourConcept = MP_CONCEPT_HIGHFIVE_SUCCESS;

	CMultiplayer_Expresser *pInitiatorExpresser = initiator->GetMultiplayerExpresser();
	Assert( pInitiatorExpresser );

	pInitiatorExpresser->AllowMultipleScenes();

	// extend initiator's taunt duration to include actual high five
	initiator->m_bInitTaunt = true;

	initiator->PlayScene( pszInitiatorScene );
	
	if ( tf_highfive_debug.GetBool() )
		Msg( " concept %i started fine for initiator %s.\n", initiatorConcept, initiator->GetPlayerName() );

	initiator->m_Shared.m_iTauntIndex = TAUNT_MISC_ITEM;
	initiator->m_Shared.m_iTauntConcept.Set( initiatorConcept );
	initiator->m_flTauntRemoveTime = gpGlobals->curtime + GetSceneDuration( pszInitiatorScene ) + 0.2f;
	initiator->m_bAllowedToRemoveTaunt = true;

	initiator->m_iTauntAttack = TAUNTATK_NONE;
	initiator->m_flTauntAttackTime = 0.f;

	static CSchemaAttributeDefHandle pAttrDef_TauntAttackName( "taunt attack name" );
	const char* pszTauntAttackName = NULL;
	if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( pItemDef, pAttrDef_TauntAttackName, &pszTauntAttackName ) )
	{
		initiator->m_iTauntAttack = GetTauntAttackByName( pszTauntAttackName );
	}

	static CSchemaAttributeDefHandle pAttrDef_TauntAttackTime( "taunt attack time" );
	float flTauntAttackTime = 0.f;
	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItemDef, pAttrDef_TauntAttackTime, &flTauntAttackTime ) )
	{
		initiator->m_flTauntAttackTime = gpGlobals->curtime + flTauntAttackTime;
	}

	if ( GetActiveWeapon() )
	{
		m_iPreTauntWeaponSlot = GetActiveWeapon()->GetSlot();
	}

	PlayScene( pszOurScene );
	OnTauntSucceeded( pszOurScene, TAUNT_MISC_ITEM, ourConcept );

	const char *pszTauntSound = pItemDef->GetCustomSound( initiator->GetTeamNumber(), 0 );
	if ( pszTauntSound )
	{
		// each participant hears the sound without PAS attenuation, but everyone else gets the PAS attenuation
		EmitSound_t params;
		params.m_pSoundName = pszTauntSound;

		CSingleUserRecipientFilter soundFilterInitiator( initiator );
		initiator->EmitSound( soundFilterInitiator, initiator->entindex(), params );

		CSingleUserRecipientFilter soundFilter( this );
		EmitSound( soundFilter, this->entindex(), params );

		CPASAttenuationFilter attenuationFilter( this, params.m_pSoundName );
		attenuationFilter.RemoveRecipient( this );
		attenuationFilter.RemoveRecipient( initiator );
		initiator->EmitSound( attenuationFilter, initiator->entindex(), params );
	}

	/*static CSchemaItemDefHandle highfiveTaunt( "High Five Taunt" );
	if ( pItemDef == highfiveTaunt )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "player_highfive_success" );
		if ( pEvent )
		{
			pEvent->SetInt( "initiator_entindex", initiator->entindex() );
			pEvent->SetInt( "partner_entindex", entindex() );

			gameeventmanager->FireEvent( pEvent );
		}
	}*/

	initiator->m_bInitTaunt = false;
	pInitiatorExpresser->DisallowMultipleScenes();

	// check for taunt achievements
	if ( TFGameRules() && ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP ) )
	{
		if ( !IsBot() && !initiator->IsBot() && ( GetTeamNumber() == initiator->GetTeamNumber() ) )
		{
			if ( IsCapturingPoint() && initiator->IsCapturingPoint() )
			{
				AwardAchievement( ACHIEVEMENT_TF_TAUNT_WHILE_CAPPING );
				initiator->AwardAchievement( ACHIEVEMENT_TF_TAUNT_WHILE_CAPPING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::MimicTauntFromPartner( CTFPlayer *initiator )
{
	Assert( initiator->m_bAllowMoveDuringTaunt );
	if ( initiator->m_TauntEconItemView.IsValid() && initiator->m_TauntEconItemView.GetItemDefinition() != NULL )
	{
		PlayTauntSceneFromItem( &initiator->m_TauntEconItemView );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern ConVar tf_allow_all_team_partner_taunt;
int CTFPlayer::GetAllowedTauntPartnerTeam() const
{
	return tf_allow_all_team_partner_taunt.GetBool() ? TEAM_ANY : GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::IncrementKillCountSinceLastDeploy( const CTakeDamageInfo &info )
{
	// track kills since last deploy, but only if our deployed weapon is the one we
	// just killed someone with (this fixes the problem where you fire a rocket, switch
	// weapons, and then get the kill tracked on the newly-deployed weapon)
	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
	if ( pTFWeapon && ( pTFWeapon == GetActiveTFWeapon() ) )
	{
		m_Shared.m_iKillCountSinceLastDeploy++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if any enemy sentry has LOS and is facing me and is in range to attack
//-----------------------------------------------------------------------------
bool CTFPlayer::IsAnyEnemySentryAbleToAttackMe( void ) const
{
	if ( m_Shared.InCond( TF_COND_DISGUISED ) ||
		 m_Shared.InCond( TF_COND_DISGUISING ) ||
		 m_Shared.IsStealthed() )
	{
		// I'm a disguised or cloaked Spy
		return false;
	}

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject* pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() != OBJ_SENTRYGUN )
			continue;
		
		if ( pObj->HasSapper() )
			continue;

		if ( pObj->IsPlasmaDisabled() )
			continue;

		if ( pObj->IsDisabled() )
			continue;

		if ( pObj->IsBuilding() )
			continue;

		if ( pObj->IsCarried() )
			continue;

		// are we in range?
		if ( ( GetAbsOrigin() - pObj->GetAbsOrigin() ).IsLengthGreaterThan( SENTRY_MAX_RANGE ) )
			continue;

		// is the sentry aiming towards me?
		if ( !IsThreatAimingTowardMe( pObj, 0.95f ) )
			continue;

		// does the sentry have clear line of fire?
		if ( !IsLineOfSightClear( pObj, IGNORE_ACTORS ) )
			continue;

		// this sentry can attack me
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// MVM Con Commands
//-----------------------------------------------------------------------------
CON_COMMAND_F( currency_give, "Have some in-game money.", FCVAR_CHEAT )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	int nAmount = atoi( args[1] );


	pPlayer->AddCurrency( nAmount );
}

//-----------------------------------------------------------------------------
// Purpose: Currency awarded directly will not be tracked by stats - see TFGameRules
//-----------------------------------------------------------------------------
void CTFPlayer::AddCurrency( int nAmount )
{
	if ( nAmount + m_nCurrency > 30000 )
	{
		m_nCurrency = 30000;
	}
	else if ( nAmount + m_nCurrency < 0 )
	{
		m_nCurrency = 0;
	}
	else
	{
		m_nCurrency += nAmount;
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Remove Currency from Display and track it as currency spent
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveCurrency( int nAmount )
{ 
	m_nCurrency = Max( m_nCurrency - nAmount, 0 ); 

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		g_pPopulationManager->AddPlayerCurrencySpent( this, nAmount );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ultra crude experience and level system
//-----------------------------------------------------------------------------
void CTFPlayer::AddExperiencePoints( int nAmount, bool bGiveCurrency /*= false*/, CTFPlayer *pSource /*= NULL*/ )
{
	int nMyLevel = GetExperienceLevel();

	// Adjust experience based on level difference of source player
	if ( pSource )
	{
		int nLevelDiff = pSource->GetExperienceLevel() - nMyLevel;
		if ( nLevelDiff <= -5 )
			return;

		if ( nLevelDiff > 0 )
		{
			nAmount *= ( nLevelDiff + 1 );
		}
		else if ( nLevelDiff < 0 )
		{
			nAmount /= ( abs( nLevelDiff ) + 1 );
		}
	}

	m_nExperiencePoints += nAmount;
	CalculateExperienceLevel();
	
	// Money?
	if ( bGiveCurrency && TFGameRules() )
	{
		TFGameRules()->DistributeCurrencyAmount( nAmount, this, false );
		CTF_GameStats.Event_PlayerCollectedCurrency( this, nAmount );
		EmitSound( "MVM.MoneyPickup" );
	}

	// DevMsg( "Exp: %d, Level: %d Perc: %d\n", GetExperiencePoints(), GetExperienceLevel(), m_nExperienceLevelProgress );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RefundExperiencePoints( void )
{
	SetExperienceLevel( 1 );

	int nAmount = 0;
	PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( this );
	if ( pPlayerStats ) 
	{
		nAmount = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_CURRENCY_COLLECTED];
	}
	
	if ( nAmount > 0 )
	{
		SetExperiencePoints(nAmount);
		SetCurrency(nAmount);
	}

	CalculateExperienceLevel(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CalculateExperienceLevel( bool bAnnounce /*= true*/ )
{
	int nMyLevel = GetExperienceLevel();

	int nPrevLevel = nMyLevel;
	float flLevel = ( (float)m_nExperiencePoints / 400.f ) + 1.f;
	flLevel = Min( flLevel, 20.f );

	// Ding?
	if ( bAnnounce )
	{
		if ( flLevel > 1 && nPrevLevel != (int)flLevel )
		{
			const char *pszTeamName = GetTeamNumber() == TF_TEAM_RED ? "RED" : "BLU";
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#TF_PlayerLeveled", pszTeamName, GetPlayerName(), CFmtStr( "%d", (int)flLevel ) );
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#TF_PlayerLeveled", pszTeamName, GetPlayerName(), CFmtStr( "%d", (int)flLevel ) );
			DispatchParticleEffect( "mvm_levelup1", PATTACH_POINT_FOLLOW, this, "head" );
			EmitSound( "Achievement.Earned" );
		}
	}

	flLevel = floor( flLevel );
	SetExperienceLevel( Max( flLevel, 1.f ) );

	// Update level progress percentage - networked
	float flLevelPerc = ( flLevel - floor( flLevel ) ) * 100.f;
	if ( m_nExperienceLevelProgress != flLevelPerc )
	{
		m_nExperienceLevelProgress.Set( (int)flLevelPerc );
	}
}

CUtlVector< CUpgradeInfo > * CTFPlayer::GetPlayerUpgradeHistory( void )
{	
	if ( g_pPopulationManager != NULL )
		return g_pPopulationManager->GetPlayerUpgradeHistory( this );

	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			Warning( "Remember Upgrade Error: Population Manager does not exist!\n" );
			return NULL;
		}
		else if ( TFGameRules()->GameModeUsesUpgrades() )
		{
			return &m_LocalUpgradeHistory;
		}
	}

	return NULL;
}

void CTFPlayer::GrantOrRemoveAllUpgrades( bool bRemove, bool bRefund )
{
	// Remove upgrade attributes from the player and their items
	if ( g_hUpgradeEntity )
	{
		g_hUpgradeEntity->GrantOrRemoveAllUpgrades( this, bRemove, bRefund );
	}

	// Remove the appropriate upgrade info from upgrade histories
	if ( g_pPopulationManager && bRemove )
	{
		g_pPopulationManager->RemovePlayerAndItemUpgradesFromHistory( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Store this upgrade for restoring at a checkpoint
//-----------------------------------------------------------------------------
void CTFPlayer::RememberUpgrade( int iPlayerClass, CEconItemView *pItem, int iUpgrade, int nCost, bool bDowngrade )
{
	if ( IsBot() )
		return;

	if ( TFGameRules() == NULL || !TFGameRules()->GameModeUsesUpgrades() )
		return;

	item_definition_index_t iItemIndex = pItem ? pItem->GetItemDefIndex() : INVALID_ITEM_DEF_INDEX;
	
	CUtlVector< CUpgradeInfo > *upgrades = GetPlayerUpgradeHistory();

	if ( !bDowngrade )
	{
		CUpgradeInfo info;
		info.m_iPlayerClass = iPlayerClass;
		info.m_itemDefIndex = iItemIndex;
		info.m_upgrade = iUpgrade;
		info.m_nCost = nCost;

		if ( upgrades )
		{
			upgrades->AddToTail( info );
		}

		m_RefundableUpgrades.AddToTail( info );
	}
	else
	{
		if ( upgrades )
		{
			for ( int i = 0; i < upgrades->Count(); ++i )
			{
				CUpgradeInfo pInfo = upgrades->Element(i);
				if ( ( pInfo.m_itemDefIndex == iItemIndex ) && ( pInfo.m_upgrade == iUpgrade ) && ( pInfo.m_nCost == -nCost ) )
				{
					upgrades->Remove( i );
					break;
				}
			}
		}

		// Subset of upgrades that can be sold back
		for ( int i = 0; i < m_RefundableUpgrades.Count(); ++i )
		{
			CUpgradeInfo pInfo = m_RefundableUpgrades.Element( i );
			if ( ( pInfo.m_itemDefIndex == iItemIndex ) && ( pInfo.m_upgrade == iUpgrade ) && ( pInfo.m_nCost == -nCost ) )
			{
				m_RefundableUpgrades.Remove( i );
				break;
			}
		}
	}

	const char *upgradeName = g_hUpgradeEntity->GetUpgradeAttributeName( iUpgrade );

	DevMsg( "%3.2f: %s: Player '%s', item '%s', upgrade '%s', cost '%d'\n",
		gpGlobals->curtime, 
		bDowngrade ? "FORGET_UPGRADE" : "REMEMBER_UPGRADE",
		GetPlayerName(),
		pItem ? pItem->GetStaticData()->GetItemBaseName() : "<self>",
		upgradeName ? upgradeName : "<NULL>",
		nCost );
}

//-----------------------------------------------------------------------------
// Purpose: Erase the first upgrade stored for this item (for powerup bottles)
//-----------------------------------------------------------------------------
void CTFPlayer::ForgetFirstUpgradeForItem( CEconItemView *pItem )
{
	if ( IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->GameModeUsesUpgrades() )
		return;

	DevMsg( "%3.2f: FORGET_FIRST_UPGRADE_FOR_ITEM: Player '%s', item '%s'\n",
			gpGlobals->curtime, 
			GetPlayerName(),
			pItem ? pItem->GetStaticData()->GetItemBaseName() : "<self>" );
	
	CUtlVector< CUpgradeInfo > *upgrades = GetPlayerUpgradeHistory();
	if ( upgrades == NULL )
		return;

	for( int i = 0; i < upgrades->Count(); ++i )
	{
		if ( ( pItem == NULL && upgrades->Element( i ).m_itemDefIndex == INVALID_ITEM_DEF_INDEX ) ||		// self upgrade
			upgrades->Element(i).m_itemDefIndex == pItem->GetItemDefIndex() )		// item upgrade
		{
			upgrades->Remove( i );
			if ( g_pPopulationManager )
				g_pPopulationManager->SendUpgradesToPlayer( this );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearUpgradeHistory( void )
{
	CUtlVector< CUpgradeInfo > *upgrades = GetPlayerUpgradeHistory();
	if ( upgrades != NULL )
		upgrades->RemoveAll();

	ResetAccumulatedSentryGunDamageDealt();
	ResetAccumulatedSentryGunKillCount();

	if ( g_pPopulationManager )
		g_pPopulationManager->SendUpgradesToPlayer( this );

	DevMsg( "%3.2f: CLEAR_UPGRADE_HISTORY: Player '%s'\n", gpGlobals->curtime, GetPlayerName() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ReapplyItemUpgrades( CEconItemView *pItem )
{
	if ( IsBot() )
		return;

	int iClassIndex = GetPlayerClass()->GetClassIndex();

	// Restore player Upgrades
	CUtlVector< CUpgradeInfo > *upgrades = GetPlayerUpgradeHistory();
	if ( upgrades == NULL )
		return;

	BeginPurchasableUpgrades();

	for( int u = 0; u < upgrades->Count(); ++u )
	{
		// Player Upgrades for this class and item
		const CUpgradeInfo& upgrade = upgrades->Element(u);
		if ( iClassIndex == upgrade.m_iPlayerClass && pItem->GetItemDefIndex() == upgrade.m_itemDefIndex )
		{
			g_hUpgradeEntity->ApplyUpgradeToItem( this, pItem, upgrade.m_upgrade, upgrade.m_nCost );
		}
	}

	EndPurchasableUpgrades();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ReapplyPlayerUpgrades( void )
{
	if ( IsBot() )
		return;

	int iClassIndex = GetPlayerClass()->GetClassIndex();
	RemovePlayerAttributes( false );

	CUtlVector< CUpgradeInfo > *upgrades = GetPlayerUpgradeHistory();
	if ( upgrades == NULL )
		return;

	BeginPurchasableUpgrades();

	// Restore player Upgrades
	for( int u = 0; u < upgrades->Count(); ++u )
	{
		// Player Upgrades for this class
		if ( iClassIndex == upgrades->Element(u).m_iPlayerClass)
		{
			// Upgrades applied to player
			if ( upgrades->Element(u).m_itemDefIndex == INVALID_ITEM_DEF_INDEX )
			{
				g_hUpgradeEntity->ApplyUpgradeToItem( this, NULL, upgrades->Element(u).m_upgrade, upgrades->Element(u).m_nCost );
			}
		}
	}

	EndPurchasableUpgrades();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::BeginPurchasableUpgrades( void )
{
	m_nCanPurchaseUpgradesCount++;

	if ( TFObjectiveResource()->GetMannVsMachineWaveCount() > 1 )
	{
		m_RefundableUpgrades.RemoveAll();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::EndPurchasableUpgrades( void )
{
	AssertMsg( m_nCanPurchaseUpgradesCount > 0, "EndPurchasableUpgrades called when m_nCanPurchaseUpgradesCount <= 0" );
	if ( m_nCanPurchaseUpgradesCount <= 0 )
		return;

	m_nCanPurchaseUpgradesCount--;

	if ( TFObjectiveResource()->GetMannVsMachineWaveCount() > 1 )
	{
		m_RefundableUpgrades.RemoveAll();
	}

	// report all upgrades
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->SendUpgradesToPlayer( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::ScriptGetCustomAttribute( const char *pName, float flFallbackValue )
{
	CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pName );
	if ( pDef )
	{
		CEconGetAttributeIterator it( pDef->GetDefinitionIndex(), flFallbackValue );
		GetAttributeList()->IterateAttributes( &it );
		return it.m_flValue;
	}

	return flFallbackValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::PlayReadySound( void )
{
	if ( m_flLastReadySoundTime < gpGlobals->curtime )
	{
		if ( TFGameRules() )
		{
			int iTeam = GetTeamNumber();
			const char *pszFormat = "%s.Ready";

			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				pszFormat = "%s.ReadyMvM";
			}
			else if ( TFGameRules()->IsCompetitiveMode() )
			{
				pszFormat = "%s.ReadyComp";
			}

			CFmtStr goYell( pszFormat, g_aPlayerClassNames_NonLocalized[ m_Shared.GetDesiredPlayerClassIndex() ] );
			TFGameRules()->BroadcastSound( iTeam, goYell, 0, this );
			TFGameRules()->BroadcastSound( TEAM_SPECTATOR, goYell, 0, this ); // spectators hear the ready sounds, too

			m_flLastReadySoundTime = gpGlobals->curtime + 4.f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetDesiredHeadScale() const
{
	float flDesiredHeadScale = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flDesiredHeadScale, head_scale );
	return flDesiredHeadScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetHeadScaleSpeed() const
{
	// change size now
	if (
		m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) ||
		m_Shared.InCond( TF_COND_MELEE_ONLY ) ||
		m_Shared.InCond( TF_COND_HALLOWEEN_KART ) ||
		m_Shared.InCond( TF_COND_BALLOON_HEAD )
		)
	{	
		return GetDesiredHeadScale();
	}

	return gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetDesiredTorsoScale() const
{
	float flDesiredTorsoScale = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flDesiredTorsoScale, torso_scale );
	return flDesiredTorsoScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetTorsoScaleSpeed() const
{
	return gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetDesiredHandScale() const
{
	float flDesiredHandScale = 1.f;
	CALL_ATTRIB_HOOK_FLOAT( flDesiredHandScale, hand_scale );
	return flDesiredHandScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetHandScaleSpeed() const
{
	if ( m_Shared.InCond( TF_COND_MELEE_ONLY ) )
	{
		return GetDesiredHandScale();
	}

	return gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetBombHeadTimestamp()
{
	m_fLastBombHeadTimestamp = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayer::GetTimeSinceWasBombHead() const
{
	return gpGlobals->curtime - m_fLastBombHeadTimestamp;
}

//-----------------------------------------------------------------------------
// Purpose: Can the player breathe under water?
//-----------------------------------------------------------------------------
bool CTFPlayer::CanBreatheUnderwater() const
{
	if ( m_Shared.InCond( TF_COND_SWIMMING_CURSE ) )
		return true;

	int iCanBreatheUnderWater = 0;
	CALL_ATTRIB_HOOK_INT( iCanBreatheUnderWater, can_breathe_under_water );
	if ( iCanBreatheUnderWater != 0 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::InputSpeakResponseConcept( inputdata_t &inputdata )
{
	const char *pInputString = STRING(inputdata.value.StringID());
	// if no params, early out
	if (!pInputString || *pInputString == 0)
	{
		Warning( "empty SpeakResponse input from %s to %s\n", inputdata.pCaller->GetDebugName(), GetDebugName() );
		return;
	}

	char buf[512] = {0}; // temporary for tokenizing
	char outputmodifiers[512] = {0}; // eventual output to speak
	int outWritten = 0;
	V_strncpy(buf, pInputString, 510);
	buf[511] = 0; // just in case the last character is a comma -- enforce that the 
	// last character in the buffer is always a terminator.
	// special syntax allowing designers to submit inputs with contexts like
	// "concept,context1:value1,context2:value2,context3:value3"
	// except that entity i/o seems to eat commas these days (didn't used to be the case)
	// so instead of commas we have to use spaces in the entity IO, 
	// and turn them into commas here. AWESOME.
	char *pModifiers = const_cast<char *>(V_strnchr(buf, ' ', 510));
	if ( pModifiers )
	{
		*pModifiers = 0;
		++pModifiers;

		// tokenize on spaces
		char *token = strtok(pModifiers, " ");
		while (token)
		{
			// find the start characters for the key and value
			// (seperated by a : which we replace with null)
			char * RESTRICT key = token;
			char * RESTRICT colon = const_cast<char *>(V_strnchr(key, ':', 510)); 
			char * RESTRICT value;
			if (!colon)
			{
				Warning( "faulty context k:v pair in entity io %s\n", pInputString );
				break;
			}

			// write the key and colon to the output string
			int toWrite = colon - key + 1;
			if ( outWritten + toWrite >= 512 )
			{
				Warning( "Speak input to %s had overlong parameter %s", GetDebugName(), pInputString );
				return;
			}
			memcpy(outputmodifiers + outWritten, key, toWrite);
			outWritten += toWrite;

			*colon = 0;
			value = colon + 1;

			// determine if the value is actually a procedural name
			CBaseEntity *pProcedural = gEntList.FindEntityProcedural( value, this, inputdata.pActivator, inputdata.pCaller );

			// write the value to the output -- if it's a procedural name, replace appropriately; 
			// if not, just copy over.
			const char *valString; 
			if (pProcedural)
			{
				valString = STRING(pProcedural->GetEntityName());
			}
			else
			{
				valString = value;
			}
			toWrite = strlen(valString);
			toWrite = MIN( 511-outWritten, toWrite );
			V_strncpy( outputmodifiers + outWritten, valString, toWrite+1 );
			outWritten += toWrite;

			// get the next token
			token = strtok(NULL, " ");
			if (token)
			{
				// if there is a next token, write in a comma
				if (outWritten < 511)
				{
					outputmodifiers[outWritten++]=',';
				}
			}
		}
	}

	// null terminate just in case
	outputmodifiers[outWritten <= 511 ? outWritten : 511] = 0;

	SpeakConceptIfAllowed( GetMPConceptIndexFromString( buf ), outputmodifiers[0] ? outputmodifiers : NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::CreateDisguiseWeaponList( CTFPlayer *pDisguiseTarget )
{
	ClearDisguiseWeaponList();

	// copy disguise target weapons
	if ( pDisguiseTarget )
	{
		for ( int i=0; i<TF_PLAYER_WEAPON_COUNT; ++i )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( pDisguiseTarget->GetWeapon( i ) );

			if ( !pWeapon )
				continue;

			CEconItemView *pItem = NULL;
			// We are copying a generated, non-base item.
			CAttributeContainer *pContainer = pWeapon->GetAttributeContainer();
			if ( pContainer )
			{
				pItem = pContainer->GetItem();
			}

			int iSubType = 0;
			CTFWeaponBase *pCopyWeapon = dynamic_cast<CTFWeaponBase*>( GiveNamedItem( pWeapon->GetClassname(), iSubType, pItem, true ) );
			if ( pCopyWeapon )
			{
				pCopyWeapon->SetSolid( SOLID_NONE );
				pCopyWeapon->SetSolidFlags( FSOLID_NOT_SOLID );
				pCopyWeapon->AddEffects( EF_NODRAW | EF_NOSHADOW );
				m_hDisguiseWeaponList.AddToTail( pCopyWeapon );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ClearDisguiseWeaponList()
{
	FOR_EACH_VEC( m_hDisguiseWeaponList, i )
	{
		if ( m_hDisguiseWeaponList[i] )
		{
			m_hDisguiseWeaponList[i]->Drop( vec3_origin );
		}
	}

	m_hDisguiseWeaponList.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanScorePointForPD( void ) const
{ 
	// These conditions block being able to score in PD
	ETFCond blockingConds[] = { TF_COND_STEALTHED			// Invis spies
							  , TF_COND_STEALTHED_BLINK
							  , TF_COND_DISGUISING			// Disguised spies
							  , TF_COND_DISGUISED
							  , TF_COND_INVULNERABLE		// Uber
							  , TF_COND_PHASE };			// Bonked Scouts
	
	// Check for blocking conditions
	for( int i=0; i<ARRAYSIZE(blockingConds); ++i )
	{
		if ( m_Shared.InCond( blockingConds[i] ) )
		{
			return false;
		}
	}

	// More aggressively deny invis than the code above
	if ( m_Shared.GetPercentInvisible() > 0.f )
	{
		return false;
	}

	// Rate limit
	return ( ( m_flNextScorePointForPD < 0 ) || ( m_flNextScorePointForPD < gpGlobals->curtime ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::PickupWeaponFromOther( CTFDroppedWeapon *pDroppedWeapon )
{
	const CEconItemView *pItem = pDroppedWeapon->GetItem();
	if ( !pItem )
		return false;

	if ( pItem->IsValid() )
	{
		int iClass = GetPlayerClass()->GetClassIndex();
		int iItemSlot = pItem->GetStaticData()->GetLoadoutSlot( iClass );
		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( GetEntityForLoadoutSlot( iItemSlot ) );

		if ( !pWeapon )
		{
			AssertMsg( false, "No weapon to put down when picking up a dropped weapon!" );
			return false;
		}
		
		// we need to force translating the name here.
		// GiveNamedItem will not translate if we force creating the item
		const char *pTranslatedWeaponName = TranslateWeaponEntForClass( pItem->GetStaticData()->GetItemClass(), iClass );
		CTFWeaponBase *pNewItem = dynamic_cast<CTFWeaponBase*>( GiveNamedItem( pTranslatedWeaponName, 0, pItem, true ));
		Assert( pNewItem );
		if ( pNewItem )
		{
			CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder*>( (CBaseEntity*)pNewItem );
			if ( pBuilder )
			{
				pBuilder->SetSubType( GetPlayerClass()->GetData()->m_aBuildable[0] );
			}

			// make sure we removed our current weapon				
			if ( pWeapon )
			{
				// drop current weapon
				Vector vecPackOrigin;
				QAngle vecPackAngles;
				CalculateAmmoPackPositionAndAngles( pWeapon, vecPackOrigin, vecPackAngles );

				bool bShouldThrowHeldWeapon = true;

				// When in the spawn room, you won't throw down your held weapon if you own that weapon.
				// This is to prevent folks from standing near a supply closet and spawning their items
				// over and over and over.
				if ( PointInRespawnRoom( this, WorldSpaceCenter() ) )
				{
					CSteamID playerSteamID;
					GetSteamID( &playerSteamID );
					uint32 nItemAccountID = pWeapon->GetAttributeContainer()->GetItem()->GetAccountID();
					// Stock weapons have accountID 0
					if ( playerSteamID.GetAccountID() == nItemAccountID || nItemAccountID == 0 )
					{
						bShouldThrowHeldWeapon = false;
					}
				}

				if ( bShouldThrowHeldWeapon )
				{
					CTFDroppedWeapon *pNewDroppedWeapon = CTFDroppedWeapon::Create( this, vecPackOrigin, vecPackAngles, pWeapon->GetWorldModel(), pWeapon->GetAttributeContainer()->GetItem() );
					if ( pNewDroppedWeapon )
					{
						pNewDroppedWeapon->InitDroppedWeapon( this, pWeapon, true );
					}
				}

				Weapon_Detach( pWeapon );
				UTIL_Remove( pWeapon );
			}
				
			CBaseCombatWeapon *pLastWeapon = GetLastWeapon();
			pNewItem->MarkAttachedEntityAsValidated();
			pNewItem->GiveTo( this );
			Weapon_SetLast( pLastWeapon );
			
			pDroppedWeapon->InitPickedUpWeapon( this, pNewItem );

			// can't use the weapon we just picked up?
			if ( !Weapon_CanSwitchTo( pNewItem ) )
			{
				// try next best thing we can use
				SwitchToNextBestWeapon( pNewItem );
			}

			// delay pickup weapon message
			m_flSendPickupWeaponMessageTime = gpGlobals->curtime + 0.1f;

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::TryToPickupDroppedWeapon()
{
	if ( !CanAttack() )
		return false;

	if ( GetActiveWeapon() && ( GetActiveWeapon()->m_flNextPrimaryAttack > gpGlobals->curtime ) )
		return false;

	CTFDroppedWeapon *pDroppedWeapon = GetDroppedWeaponInRange();
	if ( pDroppedWeapon && !pDroppedWeapon->IsMarkedForDeletion() )
	{
		if ( PickupWeaponFromOther( pDroppedWeapon ) )
		{
			UTIL_Remove( pDroppedWeapon );
			return true;
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::AddCustomAttribute( const char *pszAttributeName, float flVal, float flDuration /*= -1.f*/ )
{
	float flExpireTime = flDuration > 0 ? gpGlobals->curtime + flDuration : flDuration;
	int iIndex = m_mapCustomAttributes.Find( pszAttributeName );
	if ( iIndex == m_mapCustomAttributes.InvalidIndex() )
	{
		m_mapCustomAttributes.Insert( pszAttributeName, flExpireTime );
	}
	else
	{
		// stomp the previous expire time for now
		m_mapCustomAttributes[iIndex] = flExpireTime;
	}

	// just stomp the value
	m_Shared.ApplyAttributeToPlayer( pszAttributeName, flVal );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveCustomAttribute( const char *pszAttributeName )
{
	int iIndex = m_mapCustomAttributes.Find( pszAttributeName );
	if ( iIndex != m_mapCustomAttributes.InvalidIndex() )
	{
		m_Shared.RemoveAttributeFromPlayer( pszAttributeName );
		m_mapCustomAttributes.RemoveAt( iIndex );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::UpdateCustomAttributes()
{
	// check if we should remove custom attributes from player
	bool bShouldCheckCustomAttributes = m_mapCustomAttributes.Count() > 0;
	while ( bShouldCheckCustomAttributes )
	{
		bShouldCheckCustomAttributes = false;
		FOR_EACH_MAP_FAST( m_mapCustomAttributes, i )
		{
			float flExpireTime = m_mapCustomAttributes[i];
			if ( flExpireTime > 0 && gpGlobals->curtime > flExpireTime )
			{
				const char *pszAttributeName = m_mapCustomAttributes.Key( i );
				m_Shared.RemoveAttributeFromPlayer( pszAttributeName );
				m_mapCustomAttributes.RemoveAt( i );

				bShouldCheckCustomAttributes = true;
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::RemoveAllCustomAttributes()
{
	FOR_EACH_MAP_FAST( m_mapCustomAttributes, i )
	{
		const char *pszAttributeName = m_mapCustomAttributes.Key( i );
		m_Shared.RemoveAttributeFromPlayer( pszAttributeName );
	}
	m_mapCustomAttributes.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldForceTransmitsForTeam( int iTeam )
{ 
	return ( ( GetTeamNumber() == TEAM_SPECTATOR ) || 
			 ( ( GetTeamNumber() == iTeam ) && ( m_Shared.InCond( TF_COND_TEAM_GLOWS ) || !IsAlive() ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::ShouldGetBonusPointsForExtinguishEvent( int userID )
{
	int iIndex = m_PlayersExtinguished.Find( userID );
	if ( iIndex != m_PlayersExtinguished.InvalidIndex() )
	{
		if ( ( gpGlobals->curtime - m_PlayersExtinguished[iIndex] ) < 20.f )
			return false;

		m_PlayersExtinguished[iIndex] = gpGlobals->curtime;
	}
	else
	{
		m_PlayersExtinguished.Insert( userID, gpGlobals->curtime );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::IsTruceValidForEnt( void ) const
{
	if ( PointInRespawnRoom( this, WorldSpaceCenter(), true ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::BHaveChatSuspensionInCurrentMatch()
{
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( pMatch )
	{
		CSteamID steamID;
		GetSteamID( &steamID );

		CMatchInfo::PlayerMatchData_t *pMatchPlayer = pMatch->GetMatchDataForPlayer( steamID );
		if ( pMatchPlayer && pMatchPlayer->bChatSuspension )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::BCanCallVote()
{
	if ( GetTeamNumber() == TEAM_UNASSIGNED )
		return false;

	if ( GetTeamNumber() == TEAM_SPECTATOR )
		return sv_vote_allow_spectators.GetBool();

	if ( !m_bFirstSpawnAndCanCallVote )
		return false;

	return BaseClass::BCanCallVote();
}

CVoteController *CTFPlayer::GetTeamVoteController()
{
	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			return g_voteControllerRed;
		
		case TF_TEAM_BLUE:
			return g_voteControllerBlu;

		default:
			return g_voteControllerGlobal;
	}
}

void	CTFPlayer::ScriptAddCond( int nCond )
{
	ScriptAddCondEx( nCond, -1.0f, nullptr );
}

void	CTFPlayer::ScriptAddCondEx( int nCond, float flDuration, HSCRIPT hProvider )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return;

	m_Shared.AddCond( cond, flDuration, ToEnt( hProvider ) );
}

void	CTFPlayer::ScriptRemoveCond( int nCond )
{
	ScriptRemoveCondEx( nCond, false );
}

void	CTFPlayer::ScriptRemoveCondEx( int nCond, bool bIgnoreDuration )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return;

	return m_Shared.RemoveCond( cond, bIgnoreDuration );
}

bool	CTFPlayer::ScriptInCond( int nCond )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return false;

	return m_Shared.InCond( cond );
}

bool	CTFPlayer::ScriptWasInCond( int nCond )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return false;

	return m_Shared.WasInCond( cond );
}

void	CTFPlayer::ScriptRemoveAllCond()
{
	m_Shared.RemoveAllCond();
}

float	CTFPlayer::ScriptGetCondDuration( int nCond )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return -1.0f;

	return m_Shared.GetConditionDuration( cond );
}

void	CTFPlayer::ScriptSetCondDuration( int nCond, float flNewDuration )
{
	ETFCond cond = TFCondIndexToEnum( nCond );
	if ( cond == TF_COND_INVALID )
		return;

	return m_Shared.SetConditionDuration( cond, flNewDuration );
}

HSCRIPT	CTFPlayer::ScriptGetDisguiseTarget()
{
	return ToHScript( m_Shared.GetDisguiseTarget() );
}

Vector CTFPlayer::ScriptWeapon_ShootPosition()
{
	return this->Weapon_ShootPosition();
}

bool CTFPlayer::ScriptWeapon_CanUse( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return false;

	return this->Weapon_CanUse( pCombatWeapon );
}

void CTFPlayer::ScriptWeapon_Equip( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Equip( pCombatWeapon );
}

void CTFPlayer::ScriptWeapon_Drop( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Drop( pCombatWeapon, NULL, NULL );
}

void CTFPlayer::ScriptWeapon_DropEx( HSCRIPT hWeapon, Vector vecTarget, Vector vecVelocity )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Drop( pCombatWeapon, &vecTarget, &vecVelocity );
}

void CTFPlayer::ScriptWeapon_Switch( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_Switch( pCombatWeapon );
}

void CTFPlayer::ScriptWeapon_SetLast( HSCRIPT hWeapon )
{
	CBaseCombatWeapon *pCombatWeapon = ScriptToEntClass< CBaseCombatWeapon >( hWeapon );
	if ( !pCombatWeapon )
		return;

	this->Weapon_SetLast( pCombatWeapon );
}

HSCRIPT	CTFPlayer::ScriptGetLastWeapon()
{
	return ToHScript(this->GetLastWeapon() );
}

void CTFPlayer::ScriptEquipWearableViewModel( HSCRIPT hWearableViewModel )
{
	CTFWearableVM *pVM = ScriptToEntClass< CTFWearableVM >( hWearableViewModel );
	if ( !pVM )
		return;

	if ( !pVM->IsViewModelWearable() )
		return;

	this->EquipWearable( pVM );
}

void CTFPlayer::ScriptStunPlayer( float flTime, float flReductionAmount, int iStunFlags /* = TF_STUN_MOVEMENT */, HSCRIPT hAttacker /* = NULL */ )
{
	m_Shared.StunPlayer( flTime, flReductionAmount, iStunFlags, ScriptToEntClass< CTFPlayer >( hAttacker ) );
}
