//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha.cpp
// Our first "real" TF Boss
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "tf_projectile_arrow.h"
#include "tf_projectile_rocket.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_ammo_pack.h"
#include "tf_obj_sentrygun.h"
#include "nav_mesh/tf_nav_area.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "CRagdollMagnet.h"
#include "nav_mesh/tf_path_follower.h"
#include "bot_npc/bot_npc_minion.h"
#include "player_vs_environment/monster_resource.h"
#include "bot/map_entities/tf_bot_generator.h"

#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_behavior.h"


//#define USE_BOSS_SENTRY


ConVar tf_boss_alpha_health( "tf_boss_alpha_health", "30000"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_attack_range( "tf_boss_alpha_attack_range", "300"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_threat_tolerance( "tf_boss_alpha_threat_tolerance", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_chase_range( "tf_boss_alpha_chase_range", "300"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_grenade_launch_range( "tf_boss_alpha_grenade_launch_range", "300"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_quit_range( "tf_boss_alpha_quit_range", "2500"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_reaction_time( "tf_boss_alpha_reaction_time", "0.5"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_stunned_injury_multiplier( "tf_boss_alpha_stunned_injury_multiplier", "10" );
ConVar tf_boss_alpha_head_radius( "tf_boss_alpha_head_radius", "75" );	// 50 
ConVar tf_boss_alpha_hate_taunt_cooldown( "tf_boss_alpha_hate_taunt_cooldown", "10"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_debug_damage( "tf_boss_alpha_debug_damage", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_min_nuke_after_stun_time( "tf_boss_alpha_min_nuke_after_stun_time", "5" /*, FCVAR_CHEAT */ );

ConVar tf_boss_alpha_always_stun( "tf_boss_alpha_always_stun", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_stun_rocket_reflect_count( "tf_boss_alpha_stun_rocket_reflect_count", "2"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_stun_rocket_reflect_duration( "tf_boss_alpha_stun_rocket_reflect_duration", "1"/*, FCVAR_CHEAT */ );

ConVar tf_boss_alpha_debug_skill_shots( "tf_boss_alpha_debug_skill_shots", "0"/*, FCVAR_CHEAT */ );

extern ConVar tf_boss_alpha_nuke_interval;


//-----------------------------------------------------------------------------------------------------
// The Alpha Boss: A rocket and stickybomb firing giant robot that periodically charges up a big
// "nuke" attack, and is invulnerable unless stunned.
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( boss_alpha, CBossAlpha );

PRECACHE_REGISTER( boss_alpha );

IMPLEMENT_SERVERCLASS_ST( CBossAlpha, DT_BossAlpha )

	SendPropBool( SENDINFO( m_isNuking ) ),

END_SEND_TABLE()


BEGIN_DATADESC( CBossAlpha )
	DEFINE_OUTPUT( m_outputOnStunned,				"OnStunned" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow90Percent,	"OnHealthBelow90Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow80Percent,	"OnHealthBelow80Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow70Percent,	"OnHealthBelow70Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow60Percent,	"OnHealthBelow60Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow50Percent,	"OnHealthBelow50Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow40Percent,	"OnHealthBelow40Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow30Percent,	"OnHealthBelow30Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow20Percent,	"OnHealthBelow20Percent" ),
	DEFINE_OUTPUT( m_outputOnHealthBelow10Percent,	"OnHealthBelow10Percent" ),
	DEFINE_OUTPUT( m_outputOnKilled,				"OnKilled" ),
END_DATADESC()




//-----------------------------------------------------------------------------------------------------
CBossAlpha::CBossAlpha()
{
	m_intention = new CBossAlphaIntention( this );
	m_locomotor = new CBossAlphaLocomotion( this );
	m_body = new CBotNPCBody( this );
	m_vision = new CBossAlphaVision( this );

	m_conditionFlags = 0;
	m_isNuking = false;
	m_ageTimer.Invalidate();

	m_lastHealthPercentage = 1.0f;

	ClearStunDamage();
}


//-----------------------------------------------------------------------------------------------------
CBossAlpha::~CBossAlpha()
{
	if ( m_intention )
		delete m_intention;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_body )
		delete m_body;

	if ( m_vision )
		delete m_vision;
}


//-----------------------------------------------------------------------------------------------------
void CBossAlpha::Precache()
{
	BaseClass::Precache();

#ifdef USE_BOSS_SENTRY
	int model = PrecacheModel( "models/bots/boss_sentry/boss_sentry.mdl" );
#else
	int model = PrecacheModel( "models/bots/knight/knight.mdl" );
#endif

	PrecacheGibsForModel( model );

	PrecacheScriptSound( "Weapon_Sword.Swing" );
	PrecacheScriptSound( "Weapon_Sword.HitFlesh" );
	PrecacheScriptSound( "Weapon_Sword.HitWorld" );
	PrecacheScriptSound( "DemoCharge.HitWorld" );
	PrecacheScriptSound( "TFPlayer.Pain" );
	PrecacheScriptSound( "Halloween.HeadlessBossAttack" );
	PrecacheScriptSound( "RobotBoss.StunStart" );
	PrecacheScriptSound( "RobotBoss.Stunned" );
	PrecacheScriptSound( "RobotBoss.StunRecover" );
	PrecacheScriptSound( "RobotBoss.Acquire" );
	PrecacheScriptSound( "RobotBoss.Vocalize" );
	PrecacheScriptSound( "RobotBoss.Footstep" );
	PrecacheScriptSound( "RobotBoss.LaunchGrenades" );
	PrecacheScriptSound( "RobotBoss.LaunchRockets" );
	PrecacheScriptSound( "RobotBoss.Hurt" );
	PrecacheScriptSound( "RobotBoss.Vulnerable" );
	PrecacheScriptSound( "RobotBoss.ChargeUpNukeAttack" );
	PrecacheScriptSound( "RobotBoss.NukeAttack" );
	PrecacheScriptSound( "RobotBoss.Scanning" );
	PrecacheScriptSound( "RobotBoss.ReinforcementsArrived" );
	PrecacheScriptSound( "RobotBoss.HardHitSkillShot" );
	PrecacheScriptSound( "RobotBoss.DamageSpongeSkillShot" );
	PrecacheScriptSound( "RobotBoss.PreciseHit1SkillShot" );
	PrecacheScriptSound( "RobotBoss.PreciseHit2SkillShot" );
	PrecacheScriptSound( "RobotBoss.PreciseHit3SkillShot" );
	PrecacheScriptSound( "Cart.Explode" );
	PrecacheScriptSound( "Weapon_Crowbar.Melee_HitWorld" );

	PrecacheParticleSystem( "asplode_hoodoo_embers" );
	PrecacheParticleSystem( "charge_up" );
}


//-----------------------------------------------------------------------------------------------------
void CBossAlpha::Spawn( void )
{
	BaseClass::Spawn();

#ifdef USE_BOSS_SENTRY
	SetModel( "models/bots/boss_sentry/boss_sentry.mdl" );
#else
	SetModel( "models/bots/knight/knight.mdl" );
#endif

	m_conditionFlags = 0;

	ClearStunDamage();
	ResetSkillShots();

	int health = tf_boss_alpha_health.GetInt();
	SetHealth( health );
	SetMaxHealth( health );

	// show Boss' health meter on HUD
	if ( g_pMonsterResource )
	{
		g_pMonsterResource->SetBossHealthPercentage( 1.0f );
	}

	m_damagePoseParameter = -1;

	// randomize initial check
	m_nearestVisibleEnemy = NULL;
	m_nearestVisibleEnemyTimer.Start( RandomFloat( 0.0f, tf_boss_alpha_reaction_time.GetFloat() ) );

	m_homePos = GetAbsOrigin();

	m_currentDamagePerSecond = 0.0f;
	m_lastDamagePerSecond = 0.0f;

	m_attackTarget = NULL;
	m_attackTargetTimer.Invalidate();
	m_isAttackTargetLocked = false;

	m_nukeTimer.Start( tf_boss_alpha_nuke_interval.GetFloat() );
	m_isNuking = false;

	m_grenadeTimer.Start( GetGrenadeInterval() );
	m_ageTimer.Start();

	m_lastHealthPercentage = 1.0f;

	ChangeTeam( TF_TEAM_RED );

	TFGameRules()->SetActiveBoss( this );

	// CollisionProp()->SetSurroundingBoundsType( USE_HITBOXES );

	Vector mins( -50, -50, 0 );
	Vector maxs( 100, 100, 275 );
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );

	Vector collideMins( -50, -50, 125 );
	Vector collideMaxs( 50, 50, 260 );
	CollisionProp()->SetCollisionBounds( collideMins, collideMaxs );
}


//-----------------------------------------------------------------------------------------------------
ConVar tf_boss_alpha_dmg_mult_sniper( "tf_boss_alpha_dmg_mult_sniper", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_dmg_mult_minigun( "tf_boss_alpha_dmg_mult_minigun", "0.3"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_dmg_mult_flamethrower( "tf_boss_alpha_dmg_mult_flamethrower", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_dmg_mult_sentrygun( "tf_boss_alpha_dmg_mult_sentrygun", "0.3"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_dmg_mult_grenade( "tf_boss_alpha_dmg_mult_grenade", "0.3"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_dmg_mult_rocket( "tf_boss_alpha_dmg_mult_rocket", "0.5"/*, FCVAR_CHEAT*/ );


float ModifyBossDamage( const CTakeDamageInfo &info )
{
	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( info.GetWeapon() );

	if ( pWeapon )
	{
		switch( pWeapon->GetWeaponID() )
		{
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		case TF_WEAPON_COMPOUND_BOW:
			return info.GetDamage() * tf_boss_alpha_dmg_mult_sniper.GetFloat();

		case TF_WEAPON_MINIGUN:
			return info.GetDamage() * tf_boss_alpha_dmg_mult_minigun.GetFloat();

		case TF_WEAPON_FLAMETHROWER:
			return info.GetDamage() * tf_boss_alpha_dmg_mult_flamethrower.GetFloat();

		case TF_WEAPON_SENTRY_BULLET:
			return info.GetDamage() * tf_boss_alpha_dmg_mult_sentrygun.GetFloat();

		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_GRENADE_DEMOMAN:
			return info.GetDamage() * tf_boss_alpha_dmg_mult_grenade.GetFloat();

		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
			return info.GetDamage() * tf_boss_alpha_dmg_mult_rocket.GetFloat();
		}
	}

	// unmodified
	return info.GetDamage();
}

#define HITBOX_SKILL_STICKYBOMB_1 23
#define HITBOX_SKILL_STICKYBOMB_2 24

#define HITBOX_SKILL_PRECISION_1 20
#define HITBOX_SKILL_PRECISION_2 21
#define HITBOX_SKILL_PRECISION_3 22
#define PRECISION_SHOT_COUNT 3

#define HITBOX_SKILL_DAMAGE_SPONGE 19

#define HITBOX_SKILL_HARD_HIT 18

ConVar tf_boss_alpha_skill_shot_combo_time( "tf_boss_alpha_skill_shot_combo_time", "10"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_skill_shot_count( "tf_boss_alpha_skill_shot_count", "3"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_skill_shot_precision_time( "tf_boss_alpha_skill_shot_precision_time", "6"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_skill_shot_hard_hit_damage( "tf_boss_alpha_skill_shot_hard_hit_damage", "40"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_skill_shot_hard_hit_z( "tf_boss_alpha_skill_shot_hard_hit_z", "0"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_skill_shot_damage_sponge_total( "tf_boss_alpha_skill_shot_damage_sponge_total", "500"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_skill_shot_damage_sponge_decay( "tf_boss_alpha_skill_shot_damage_sponge_decay", "100"/*, FCVAR_CHEAT */ );


//-----------------------------------------------------------------------------------------------------
void CBossAlpha::ResetSkillShots( void )
{
	m_skillShotComboTimer.Invalidate();
	m_skillShotCount = 0;

	m_isPrecisionShotDone = false;
	m_precisionSkillShotTimer.Invalidate();

	for( int i=0; i<PRECISION_SHOT_COUNT; ++i )
	{
		m_isPrecisionShotHit[i] = false;
	}

	m_isDamageSpongeSkillShotDone = false;
	m_damageSpongeSkillShotAmount = 0.0f;

	m_isHardHitSkillShotDone = false;

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->HideSkillShotComboMeter();
	}
}


//-----------------------------------------------------------------------------------------------------
void CBossAlpha::OnSkillShotComboStarted( void )
{
	m_skillShotComboTimer.Start( tf_boss_alpha_skill_shot_combo_time.GetFloat() );

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->StartSkillShotComboMeter( tf_boss_alpha_skill_shot_combo_time.GetFloat() );
	}
}


//-----------------------------------------------------------------------------------------------------
void CBossAlpha::OnSkillShot( void )
{
	if ( !m_skillShotComboTimer.HasStarted() || m_skillShotComboTimer.IsElapsed() )
	{
		// start a new combo
		OnSkillShotComboStarted();
		m_skillShotCount = 1;
	}
	else
	{
		// combo in progress
		++m_skillShotCount;

		if ( g_pMonsterResource )
		{
			g_pMonsterResource->IncrementSkillShotComboMeter();
		}
	}

	if ( m_skillShotCount >= tf_boss_alpha_skill_shot_count.GetInt() )
	{
		AddCondition( STUNNED );
		EmitSound( "RobotBoss.Vulnerable" );
	}
}


//-----------------------------------------------------------------------------------------------------
//
// Invoked when we are struck. Check if a vulnerability was hit, and update the skill shot combo
//
bool CBossAlpha::CheckSkillShots( const CTakeDamageInfo &info )
{
	if ( !HasAbility( CBossAlpha::CAN_BE_STUNNED ) || !info.GetAttacker() )
	{
		return false;
	}

	// skill shots are not available until the boss recovers
	if ( IsInCondition( STUNNED ) )
	{
		return false;
	}

	if ( tf_boss_alpha_always_stun.GetBool() )
	{
		m_skillShotComboTimer.Start( 1.0f );
		m_skillShotCount = 999;
		OnSkillShot();
		return true;
	}

//	const Vector &hitSpot = info.GetDamagePosition();

	CBaseEntity *inflictor = info.GetInflictor();
	if ( !inflictor )
	{
		return false;
	}

	Vector hitDir = m_lastTraceAttackDir;

/*
	Vector hitDir = inflictor->GetAbsVelocity();
	
	if ( inflictor->IsPlayer() )
	{
		hitDir = hitSpot - inflictor->EyePosition();
	}
	else
	{
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( inflictor );
		if ( sentry )
		{
			hitDir = hitSpot - sentry->EyePosition();
		}
	}

	hitDir.NormalizeInPlace();
*/

	Vector traceFrom = m_lastTraceAttackTrace.startpos - m_lastTraceAttackDir * 10.0f;
	Vector traceTo = m_lastTraceAttackTrace.endpos + m_lastTraceAttackDir * 100.0f;

	trace_t result;
	//UTIL_TraceLine( hitSpot - 50.0f * hitDir, hitSpot + 50.0f * hitDir, MASK_SOLID | CONTENTS_HITBOX, inflictor, COLLISION_GROUP_NONE, &result );
	UTIL_TraceLine( traceFrom, traceTo, MASK_SOLID | CONTENTS_HITBOX, inflictor, COLLISION_GROUP_NONE, &result );

	if ( tf_boss_alpha_debug_skill_shots.GetBool() )
	{
		if ( result.hitbox != 0 )
		{
			NDebugOverlay::HorzArrow( traceFrom, traceTo, 3.0f, 0, 255, 0, 255, true, 9999.9f );
		}
		else
		{
			NDebugOverlay::HorzArrow( traceFrom, traceTo, 3.0f, 255, 0, 0, 255, true, 9999.9f );
		}
	}

	if ( !result.DidHit() )
	{
		return false;
	}

	switch( result.hitbox )
	{
	case HITBOX_SKILL_PRECISION_1:
	case HITBOX_SKILL_PRECISION_2:
	case HITBOX_SKILL_PRECISION_3:
		{
			int which = result.hitbox - HITBOX_SKILL_PRECISION_1;

			if ( !m_isPrecisionShotDone && !m_isPrecisionShotHit[ which ] )
			{
				if ( !m_precisionSkillShotTimer.HasStarted() )
				{
					m_precisionSkillShotTimer.Start( tf_boss_alpha_skill_shot_precision_time.GetFloat() );
				}

				m_isPrecisionShotHit[ which ] = true;
				UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "PRECISION SHOT %d...", which+1 ) );

				int i;
				for( i=0; i<PRECISION_SHOT_COUNT; ++i )
				{
					if ( !m_isPrecisionShotHit[i] )
						break;
				}

				if ( i == PRECISION_SHOT_COUNT )
				{
					// successfully completed the precision shot
					m_isPrecisionShotDone = true;
					UTIL_ClientPrintAll( HUD_PRINTTALK, "PRECISION SKILL SHOT!" );
					EmitSound( CFmtStr( "RobotBoss.PreciseHit%dSkillShot", which+1 ) );
					OnSkillShot();
				}
				return true;
			}
			break;
		}

	case HITBOX_SKILL_DAMAGE_SPONGE:
		if ( !m_isDamageSpongeSkillShotDone )
		{
			m_damageSpongeSkillShotAmount += info.GetDamage();

			if ( m_damageSpongeSkillShotAmount > tf_boss_alpha_skill_shot_damage_sponge_total.GetFloat() )
			{
				// successfully completed the damage sponge shot
				m_isDamageSpongeSkillShotDone = true;
				m_damageSpongeSkillShotAmount = 0.0f;
				UTIL_ClientPrintAll( HUD_PRINTTALK, "DAMAGE SPONGE SKILL SHOT!" );
				EmitSound( "RobotBoss.DamageSpongeSkillShot" );
				OnSkillShot();
				return true;
			}

			UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "DAMAGE SPONGE = %3.2f", m_damageSpongeSkillShotAmount ) );
			return true;
		}
		break;

	case HITBOX_SKILL_HARD_HIT:
		if ( !m_isHardHitSkillShotDone )
		{
			if ( info.GetDamage() > tf_boss_alpha_skill_shot_hard_hit_damage.GetFloat() )
			{
				// make sure player hit from above
				if ( info.GetAttacker() )
				{
					Vector toAttacker = info.GetAttacker()->EyePosition() - m_lastTraceAttackTrace.endpos;
					toAttacker.NormalizeInPlace();

					if ( toAttacker.z > tf_boss_alpha_skill_shot_hard_hit_z.GetFloat() )
					{
						// successfully completed the hard hit shot
						m_isHardHitSkillShotDone = true;
						UTIL_ClientPrintAll( HUD_PRINTTALK, "HARD HIT SKILL SHOT!" );
						EmitSound( "RobotBoss.HardHitSkillShot" );
						OnSkillShot();
					}
				}
			}
			return true;
		}
		break;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
void CBossAlpha::UpdateSkillShots( void )
{
	m_damageSpongeSkillShotAmount -= tf_boss_alpha_skill_shot_damage_sponge_decay.GetFloat() * gpGlobals->frametime;
	if ( m_damageSpongeSkillShotAmount < 0.0f )
	{
		m_damageSpongeSkillShotAmount = 0.0f;
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "DAMAGE SPONGE = %3.2f/%3.2f", m_damageSpongeSkillShotAmount, tf_boss_alpha_skill_shot_damage_sponge_total.GetFloat() ) );
	}

	if ( m_skillShotComboTimer.HasStarted() && m_skillShotComboTimer.IsElapsed() )
	{
		// took too long to perform skill shots - reset combo
		ResetSkillShots();
		UTIL_ClientPrintAll( HUD_PRINTTALK, "SKILL SHOT CHAIN FAILED - TOO SLOW!" );
	}

	if ( !m_isPrecisionShotDone && m_precisionSkillShotTimer.HasStarted() && m_precisionSkillShotTimer.IsElapsed() )
	{
		// took too long to hit all the precision targets - reset
		m_precisionSkillShotTimer.Invalidate();
		for( int i=0; i<PRECISION_SHOT_COUNT; ++i )
		{
			m_isPrecisionShotHit[i] = false;
		}
		UTIL_ClientPrintAll( HUD_PRINTTALK, "PRECISION SHOTS RESET - TOO SLOW!" );
	}
}


//-----------------------------------------------------------------------------------------------------
int CBossAlpha::OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo )
{
	CTakeDamageInfo info = rawInfo;

	// don't take damage from myself
	if ( info.GetAttacker() == this )
	{
		return 0;
	}

	// weapon-specific damage modification
	info.SetDamage( ModifyBossDamage( info ) );

	// do the critical damage increase
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		info.SetDamage( info.GetDamage() * TF_DAMAGE_CRIT_MULTIPLIER );
	}

	bool isSkillShot = false;
	if ( CheckSkillShots( info ) )
	{
		isSkillShot = true;

		// skill shots don't deal damage
		info.SetDamage( 0 );
	}

	bool isHeadHit = false;
	if ( IsInCondition( VULNERABLE_TO_STUN ) )
	{
		// track head damage when vulnerable
		Vector headPos;
		QAngle headAngles;
		if ( GetAttachment( "head", headPos, headAngles ) )
		{
			Vector damagePos = info.GetDamagePosition();

			if ( tf_boss_alpha_debug_damage.GetBool() )
			{
				NDebugOverlay::Cross3D( headPos, 5.0f, 255, 0, 0, true, 5.0f );
				NDebugOverlay::Cross3D( damagePos, 5.0f, 0, 255, 0, true, 5.0f );
				NDebugOverlay::Line( damagePos, headPos, 255, 255, 0, true, 5.0f );
			}

			isHeadHit = ( damagePos - headPos ).IsLengthLessThan( tf_boss_alpha_head_radius.GetFloat() );

			if ( isHeadHit )
			{
				// hit the head
				AccumulateStunDamage( info.GetDamage() );
				DispatchParticleEffect( "asplode_hoodoo_embers", info.GetDamagePosition(), GetAbsAngles() );

				if ( tf_boss_alpha_debug_damage.GetBool() )
				{
					DevMsg( "Stun dmg = %f\n", GetStunDamage() );
					NDebugOverlay::Circle( headPos, tf_boss_alpha_head_radius.GetFloat(), 255, 0, 0, 255, true, 5.0f );
				}
			}
			else if ( tf_boss_alpha_debug_damage.GetBool() )
			{
				NDebugOverlay::Circle( headPos, tf_boss_alpha_head_radius.GetFloat(), 255, 255, 0, 255, true, 5.0f );
			}
		}
	}

	// take extra damage when stunned
	if ( IsInCondition( STUNNED ) )
	{
		info.SetDamage( info.GetDamage() * tf_boss_alpha_stunned_injury_multiplier.GetFloat() );

		if ( m_ouchTimer.IsElapsed() )
		{
			m_ouchTimer.Start( 1.0f );
			EmitSound( "RobotBoss.Hurt" );
		}
	}
	else if ( !isHeadHit && !isSkillShot )
	{
		// invulnerable until stunned
		if ( m_ricochetSoundTimer.IsElapsed() )
		{
			TFGameRules()->BroadcastSound( 255, "Weapon_Crowbar.Melee_HitWorld" );
			m_ricochetSoundTimer.Start( 0.15f );
		}

		return 0;
	}

	
	// keep a list of everyone who hurt me, and when
	if ( info.GetAttacker() && info.GetAttacker()->MyCombatCharacterPointer() && !InSameTeam( info.GetAttacker() ) )
	{
		CBaseCombatCharacter *attacker = info.GetAttacker()->MyCombatCharacterPointer();

		// sentry guns are first class attackers
		if ( info.GetInflictor() )
		{
			CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );
			if ( sentry )
			{
				attacker = sentry;
			}
		}

		RememberAttacker( attacker, info.GetDamage(), ( info.GetDamageType() & DMG_CRITICAL ) ? true : false );

		CTFPlayer *playerAttacker = ToTFPlayer( attacker );
		if ( playerAttacker )
		{
			for( int i=0; i<playerAttacker->m_Shared.GetNumHealers(); ++i )
			{
				CTFPlayer *medic = ToTFPlayer( playerAttacker->m_Shared.GetHealerByIndex( i ) );
				if ( medic )
				{
					// medics healing my attacker are also considered attackers
					RememberAttacker( medic, 0, 0 );
				}
			}
		}

		// if we don't have an attack target yet, we do now
		if ( !HasAttackTarget() )
		{
			SetAttackTarget( attacker );
		}
	}


	// fire event for client combat text, beep, etc.
	IGameEvent *event = gameeventmanager->CreateEvent( "npc_hurt" );
	if ( event )
	{
		event->SetInt( "entindex", entindex() );
		event->SetInt( "health", MAX( 0, GetHealth() ) );
		event->SetInt( "damageamount", info.GetDamage() );
		event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) ? true : false );

		CTFPlayer *attackerPlayer = ToTFPlayer( info.GetAttacker() );
		if ( attackerPlayer )
		{
			event->SetInt( "attacker_player", attackerPlayer->GetUserID() );

			if ( attackerPlayer->GetActiveTFWeapon() )
			{
				event->SetInt( "weaponid", attackerPlayer->GetActiveTFWeapon()->GetWeaponID() );
			}
			else
			{
				event->SetInt( "weaponid", 0 );
			}
		}
		else
		{
			// hurt by world
			event->SetInt( "attacker_player", 0 );
			event->SetInt( "weaponid", 0 );
		}

		gameeventmanager->FireEvent( event );
	}

	int result = BaseClass::OnTakeDamage_Alive( info );

	// emit injury outputs
	float healthPercentage = (float)GetHealth() / (float)GetMaxHealth();

	if ( m_lastHealthPercentage > 0.9f && healthPercentage < 0.9f )
	{
		m_outputOnHealthBelow90Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.8f && healthPercentage < 0.8f )
	{
		m_outputOnHealthBelow80Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.7f && healthPercentage < 0.7f )
	{
		m_outputOnHealthBelow70Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.6f && healthPercentage < 0.6f )
	{
		m_outputOnHealthBelow60Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.5f && healthPercentage < 0.5f )
	{
		m_outputOnHealthBelow50Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.4f && healthPercentage < 0.4f )
	{
		m_outputOnHealthBelow40Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.3f && healthPercentage < 0.3f )
	{
		m_outputOnHealthBelow30Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.2f && healthPercentage < 0.2f )
	{
		m_outputOnHealthBelow20Percent.FireOutput( this, this );
	}
	else if ( m_lastHealthPercentage > 0.1f && healthPercentage < 0.1f )
	{
		m_outputOnHealthBelow10Percent.FireOutput( this, this );
	}

	m_lastHealthPercentage = healthPercentage;

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->SetBossHealthPercentage( healthPercentage );
	}

	return result;
}


//---------------------------------------------------------------------------------------------
// Returns true if we're in a condition that means we can't start another action
bool CBossAlpha::IsBusy( void ) const
{
	return IsInCondition( (Condition)( CHARGING | STUNNED | VULNERABLE_TO_STUN | BUSY ) );
}


//---------------------------------------------------------------------------------------------
void CBossAlpha::RememberAttacker( CBaseCombatCharacter *attacker, float damage, bool wasCritical )
{
	AttackerInfo attackerInfo;

	attackerInfo.m_attacker = attacker;
	attackerInfo.m_timestamp = gpGlobals->curtime;
	attackerInfo.m_damage = damage;
	attackerInfo.m_wasCritical = wasCritical;

	m_attackerVector.AddToHead( attackerInfo );
}


//----------------------------------------------------------------------------------
CTFPlayer *CBossAlpha::GetClosestMinionPrisoner( void )
{
	CUtlVector< CBotNPCMinion * > minionVector;
	CBotNPCMinion *minion = NULL;
	while( ( minion = (CBotNPCMinion *)gEntList.FindEntityByClassname( minion, "bot_npc_minion" ) ) != NULL )
	{
		minionVector.AddToTail( minion );
	}

	CTFPlayer *closeCapture = NULL;
	float captureRangeSq = FLT_MAX;

	for( int m=0; m<minionVector.Count(); ++m )
	{
		minion = minionVector[m];

		if ( minion->HasTarget() )
		{
			CTFPlayer *victim = minion->GetTarget();
			if ( victim->m_Shared.InCond( TF_COND_STUNNED ) )
			{
				// they've got one!
				float rangeSq = GetRangeSquaredTo( victim );
				if ( rangeSq < captureRangeSq )
				{
					closeCapture = victim;
					captureRangeSq = rangeSq;
				}
			}
		}
	}

	return closeCapture;
}


//----------------------------------------------------------------------------------
bool CBossAlpha::IsPrisonerOfMinion( CBaseCombatCharacter *victim )
{
	if ( !victim->IsPlayer() )
	{
		return false;
	}

	CUtlVector< CBotNPCMinion * > minionVector;
	CBotNPCMinion *minion = NULL;
	while( ( minion = (CBotNPCMinion *)gEntList.FindEntityByClassname( minion, "bot_npc_minion" ) ) != NULL )
	{
		minionVector.AddToTail( minion );
	}

	for( int m=0; m<minionVector.Count(); ++m )
	{
		minion = minionVector[m];

		if ( minion->HasTarget() && minion->GetTarget() == victim )
		{
			if ( minion->GetTarget()->m_Shared.InCond( TF_COND_STUNNED ) )
			{
				return true;
			}
		}
	}

	return false;
}


//----------------------------------------------------------------------------------
void CBossAlpha::UpdateDamagePerSecond( void )
{
	m_lastDamagePerSecond = m_currentDamagePerSecond;

	m_currentDamagePerSecond = 0.0f;

	const float windowDuration = 10.0f; // 5.0f;
	int i;

	m_threatVector.RemoveAll();

	for( i=0; i<m_attackerVector.Count(); ++i )
	{
		float age = gpGlobals->curtime - m_attackerVector[i].m_timestamp;

		if ( age > windowDuration )
		{
			// too old
			break;
		}

		float decayedDamage = ( ( windowDuration - age ) / windowDuration ) * m_attackerVector[i].m_damage;

		m_currentDamagePerSecond += decayedDamage;

		CBaseCombatCharacter *attacker = m_attackerVector[i].m_attacker;

		if ( attacker && attacker->IsAlive() )
		{
			int j;
			for( j=0; j<m_threatVector.Count(); ++j )
			{
				if ( m_threatVector[j].m_who == attacker )
				{
					m_threatVector[j].m_threat += decayedDamage;
					break;
				}
			}

			if ( j >= m_threatVector.Count() )
			{
				// new threat
				ThreatInfo threat;
				threat.m_who = attacker;
				threat.m_threat = decayedDamage;
				m_threatVector.AddToTail( threat );
			}
		}
	}

// 	if ( m_currentDamagePerSecond > 0.0001f )
// 	{
// 		DevMsg( "%3.2f: dps = %3.2f\n", gpGlobals->curtime, m_currentDamagePerSecond );
// 	}
}


//----------------------------------------------------------------------------------
const CBossAlpha::ThreatInfo *CBossAlpha::GetMaxThreat( void ) const
{
	int maxThreatIndex = -1;

	for( int i=0; i<m_threatVector.Count(); ++i )
	{
		if ( maxThreatIndex < 0 || m_threatVector[i].m_threat > m_threatVector[ maxThreatIndex ].m_threat )
		{
			maxThreatIndex = i;
		}
	}

	if ( maxThreatIndex < 0 )
	{
		// no threat yet
		return NULL;
	}

	return &m_threatVector[ maxThreatIndex ];
}


//----------------------------------------------------------------------------------
const CBossAlpha::ThreatInfo *CBossAlpha::GetThreat( CBaseCombatCharacter *who ) const
{
	for( int i=0; i<m_threatVector.Count(); ++i )
	{
		if ( m_threatVector[i].m_who == who )
		{
			return &m_threatVector[i];
		}
	}

	return NULL;
}


//----------------------------------------------------------------------------------
void CBossAlpha::UpdateAttackTarget( void )
{
	if ( m_isAttackTargetLocked && HasAttackTarget() )
	{
		return;
	}

	// who is most dangerous to me at the moment
	const ThreatInfo *maxThreat = GetMaxThreat();

	if ( !maxThreat )
	{
		// nobody is hurting me at the moment

		if ( HasAttackTarget() )
		{
			// stay focused on current target
			return;
		}

		// we have no current target, either

		// if my minions have captured someone, go get them
		CTFPlayer *closeCapture = GetClosestMinionPrisoner();
		if ( closeCapture )
		{
			SetAttackTarget( closeCapture );
			return;
		}

		// if we see an enemy, attack them
		CBaseCombatCharacter *visible = GetNearestVisibleEnemy();
		if ( visible )
		{
			SetAttackTarget( visible );
		}

		return;
	}

	// we are under attack, if we don't have a target, attack the highest threat
	if ( !HasAttackTarget() )
	{
		SetAttackTarget( maxThreat->m_who );
		return;
	}

	if ( IsAttackTarget( maxThreat->m_who ) )
	{
		// our current target is still dealing the most damage to us
		return;
	}

	// switch to new threat if is is more dangerous
	const ThreatInfo *attackTargetThreat = GetThreat( GetAttackTarget() );

	if ( !attackTargetThreat || maxThreat->m_threat > attackTargetThreat->m_threat + tf_boss_alpha_threat_tolerance.GetFloat() )
	{
		// change threats
		SetAttackTarget( maxThreat->m_who );
	}
}


//----------------------------------------------------------------------------------
void CBossAlpha::RemoveCondition( Condition c )
{
	if ( c == STUNNED )
	{
		// reset the accumulator
		ClearStunDamage();

		ResetSkillShots();
	}

	m_conditionFlags &= ~c;
}


//---------------------------------------------------------------------------------------------
void CBossAlpha::Update( void )
{
	BaseClass::Update();

	UpdateNearestVisibleEnemy();
	UpdateDamagePerSecond();
	UpdateAttackTarget();
	UpdateSkillShots();

	if ( m_damagePoseParameter < 0 )
	{
		m_damagePoseParameter = LookupPoseParameter( "damage" );
	}

	if ( m_damagePoseParameter >= 0 )
	{
		SetPoseParameter( m_damagePoseParameter, 1.0f - ( (float)GetHealth() / (float)GetMaxHealth() ) );
	}

	// chase down players who taunt me
	if ( m_hateTauntTimer.IsElapsed() )
	{
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );

		for( int i=0; i<playerVector.Count(); ++i )
		{
			if ( playerVector[i]->IsTaunting() )
			{
				m_hateTauntTimer.Start( tf_boss_alpha_hate_taunt_cooldown.GetFloat() );

				if ( IsLineOfSightClear( playerVector[i], IGNORE_ACTORS ) )
				{
					// the taunter becomes our new attack target
					SetAttackTarget( playerVector[i], tf_boss_alpha_hate_taunt_cooldown.GetFloat() );
				}	
			}
		}
	}
}


//---------------------------------------------------------------------------------------------
bool CBossAlpha::IsIgnored( CTFPlayer *player ) const
{
	if ( player->m_Shared.IsStealthed() )
	{
		if ( player->m_Shared.GetPercentInvisible() < 0.75f )
		{
			// spy is partially cloaked, and therefore attracts our attention
			return false;
		}

		if ( player->m_Shared.InCond( TF_COND_BURNING ) ||
			 player->m_Shared.InCond( TF_COND_URINE ) ||
			 player->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
			 player->m_Shared.InCond( TF_COND_BLEEDING ) )
		{
			// always notice players with these conditions
			return false;
		}

		// invisible!
		return true;
	}

	return false;
}


//---------------------------------------------------------------------------------------------
void CBossAlpha::UpdateNearestVisibleEnemy( void )
{
	if ( !m_nearestVisibleEnemyTimer.IsElapsed() )
	{
		return;
	}

	m_nearestVisibleEnemyTimer.Start( tf_boss_alpha_reaction_time.GetFloat() );

	// collect everyone
	CUtlVector< CTFPlayer * > playerVector;
	//CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	Vector myForward;
	GetVectors( &myForward, NULL, NULL );

	m_nearestVisibleEnemy = NULL;
	float victimRangeSq = FLT_MAX;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *victim = playerVector[i];

		if ( IsIgnored( victim ) )
		{
			continue;
		}

		float rangeSq = GetRangeSquaredTo( playerVector[i] );
		if ( rangeSq < victimRangeSq )
		{
			// FOV check
			Vector to = playerVector[i]->WorldSpaceCenter() - WorldSpaceCenter();
			to.NormalizeInPlace();

			if ( DotProduct( to, myForward ) > -0.7071f )
			{
				if ( IsLineOfSightClear( playerVector[i] ) )
				{
					m_nearestVisibleEnemy = playerVector[i];
					victimRangeSq = rangeSq;
				}
			}
		}
	}
}


//---------------------------------------------------------------------------------------------
void CBossAlpha::SetAttackTarget( CBaseCombatCharacter *target, float duration )
{
	if ( target && m_attackTarget != NULL && m_attackTarget->IsAlive() && m_attackTargetTimer.HasStarted() && !m_attackTargetTimer.IsElapsed() )
	{
		// can't switch away from our still valid target yet
		return;
	}

	if ( m_attackTarget != target )
	{
		if ( target )
		{
			EmitSound( "RobotBoss.Acquire" );
			AddGesture( ACT_MP_GESTURE_FLINCH_CHEST );
		}

		TFGameRules()->SetIT( m_attackTarget );

		m_attackTarget = target;
	}

	if ( duration > 0.0f )
	{
		m_attackTargetTimer.Start( duration );
	}
	else
	{
		m_attackTargetTimer.Invalidate();
	}
}


//---------------------------------------------------------------------------------------------
CBaseCombatCharacter *CBossAlpha::GetAttackTarget( void ) const
{
	if ( m_attackTarget != NULL && m_attackTarget->IsAlive() )
	{
		return m_attackTarget;
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
void CBossAlpha::Break( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( GetSkin() );
	MessageEnd();
}


//---------------------------------------------------------------------------------------------
void CBossAlpha::CollectPlayersStandingOnMe( CUtlVector< CTFPlayer * > *playerVector )
{
	CUtlVector< CTFPlayer * > allPlayerVector;
	CollectPlayers( &allPlayerVector, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );

	for( int i=0; i<allPlayerVector.Count(); ++i )
	{
		CTFPlayer *player = allPlayerVector[i];

		if ( player->GetGroundEntity() == this )
		{
			playerVector->AddToTail( player );
		}
	}
}


//---------------------------------------------------------------------------------------------
void CBossAlpha::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// cache the trace info so we can precisely re-trace to find hitbox hits in OnTakeDamage_Alive() later
	if ( ptr )
	{
		m_lastTraceAttackTrace = *ptr;
	}

	m_lastTraceAttackDir = vecDir;

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}


//---------------------------------------------------------------------------------------------
// Intention interface
//---------------------------------------------------------------------------------------------
CBossAlphaIntention::CBossAlphaIntention( CBossAlpha *me ) : IIntention( me )
{ 
	m_behavior = new Behavior< CBossAlpha >( new CBossAlphaBehavior ); 
}

CBossAlphaIntention::~CBossAlphaIntention()
{
	delete m_behavior;
}

void CBossAlphaIntention::Reset( void )
{ 
	delete m_behavior; 
	m_behavior = new Behavior< CBossAlpha >( new CBossAlphaBehavior );
}

void CBossAlphaIntention::Update( void )
{
	m_behavior->Update( static_cast< CBossAlpha * >( GetBot() ), GetUpdateInterval() ); 
}

QueryResultType CBossAlphaIntention::IsPositionAllowed( const INextBot *meBot, const Vector &pos ) const
{
	// is this a place we can be?
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
// Locomotion interface
//---------------------------------------------------------------------------------------------
CBossAlphaLocomotion::CBossAlphaLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) 
{ 
	CBossAlpha *me = (CBossAlpha *)GetBot()->GetEntity();

	m_runSpeed = me->GetMoveSpeed();
}


//---------------------------------------------------------------------------------------------
float CBossAlphaLocomotion::GetRunSpeed( void ) const
{
	CBossAlpha *me = (CBossAlpha *)GetBot()->GetEntity();

	return me->IsInCondition( CBossAlpha::CHARGING ) ? 1000.0f : m_runSpeed;
}


//---------------------------------------------------------------------------------------------
// if delta Z is greater than this, we have to jump to get up
float CBossAlphaLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum height of a jump
float CBossAlphaLocomotion::GetMaxJumpHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// Vision interface
//---------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------
// Return true to completely ignore this entity (may not be in sight when this is called)
bool CBossAlphaVision::IsIgnored( CBaseEntity *subject ) const
{
	if ( subject->IsPlayer() )
	{
		CTFPlayer *enemy = static_cast< CTFPlayer * >( subject );

		if ( enemy->m_Shared.InCond( TF_COND_BURNING ) ||
			 enemy->m_Shared.InCond( TF_COND_URINE ) ||
			 enemy->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
			 enemy->m_Shared.InCond( TF_COND_BLEEDING ) )
		{
			// always notice players with these conditions
			return false;
		}

		if ( enemy->m_Shared.IsStealthed() )
		{
			if ( enemy->m_Shared.GetPercentInvisible() < 0.75f )
			{
				// spy is partially cloaked, and therefore attracts our attention
				return false;
			}

			// invisible!
			return true;
		}

		if ( enemy->IsPlacingSapper() )
		{
			return false;
		}

		if ( enemy->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}
		
		if ( enemy->m_Shared.InCond( TF_COND_DISGUISED ) && enemy->m_Shared.GetDisguiseTeam() == GetBot()->GetEntity()->GetTeamNumber() )
		{
			// spy is disguised as a member of my team
			return true;
		}
	}

	return false;
}

#endif // TF_RAID_MODE
