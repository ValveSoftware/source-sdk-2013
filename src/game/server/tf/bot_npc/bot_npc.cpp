//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc.cpp
// A NextBot non-player derived actor
// Michael Booth, November 2010

#include "cbase.h"

#ifdef OBSOLETE_USE_BOSS_ALPHA

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
#include "bot_npc.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "econ_wearable.h"
#include "team_control_point_master.h"
#include "particle_parse.h"
#include "CRagdollMagnet.h"
#include "nav_mesh/tf_path_follower.h"
#include "bot_npc_minion.h"
#include "player_vs_environment/monster_resource.h"
#include "bot/map_entities/tf_bot_generator.h"
#include "player_vs_environment/tf_population_manager.h"

//#define USE_BOSS_SENTRY


ConVar tf_bot_npc_health( "tf_bot_npc_health", "100000"/*, FCVAR_CHEAT*/ );		// 50000

ConVar tf_bot_npc_speed( "tf_bot_npc_speed", "300"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_attack_range( "tf_bot_npc_attack_range", "300"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_melee_damage( "tf_bot_npc_melee_damage", "150"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_threat_tolerance( "tf_bot_npc_threat_tolerance", "100"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_shoot_interval( "tf_bot_npc_shoot_interval", "15"/*, FCVAR_CHEAT*/ ); // 2
ConVar tf_bot_npc_aim_time( "tf_bot_npc_aim_time", "1"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_chase_range( "tf_bot_npc_chase_range", "300"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_grenade_launch_range( "tf_bot_npc_grenade_launch_range", "300"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grenade_damage( "tf_bot_npc_grenade_damage", "25"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_minion_launch_count_initial( "tf_bot_npc_minion_launch_count_initial", "5"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_launch_count_increase_interval( "tf_bot_npc_minion_launch_count_increase_interval", "999999999"/*, FCVAR_CHEAT*/ );	// 30
ConVar tf_bot_npc_minion_launch_initial_interval( "tf_bot_npc_minion_launch_initial_interval", "20"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_launch_interval( "tf_bot_npc_minion_launch_interval", "30"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_chase_duration( "tf_bot_npc_chase_duration", "30"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_quit_range( "tf_bot_npc_quit_range", "2500"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_reaction_time( "tf_bot_npc_reaction_time", "0.5"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_charge_interval( "tf_bot_npc_charge_interval", "10"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_charge_pushaway_force( "tf_bot_npc_charge_pushaway_force", "500"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_charge_damage( "tf_bot_npc_charge_damage", "150"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_nuke_charge_time( "tf_bot_npc_nuke_charge_time", "5" );
ConVar tf_bot_npc_nuke_interval( "tf_bot_npc_nuke_interval", "20" );
ConVar tf_bot_npc_nuke_lethal_time( "tf_bot_npc_nuke_lethal_time", "999999999" );		// 300

ConVar tf_bot_npc_block_dps_react( "tf_bot_npc_block_dps_react", "150" );

ConVar tf_bot_npc_become_stunned_damage( "tf_bot_npc_become_stunned_damage", "500" );
ConVar tf_bot_npc_stunned_injury_multiplier( "tf_bot_npc_stunned_injury_multiplier", "10" );
ConVar tf_bot_npc_stunned_duration( "tf_bot_npc_stunned_duration", "5" );
ConVar tf_bot_npc_head_radius( "tf_bot_npc_head_radius", "75" );	// 50 

ConVar tf_bot_npc_stun_rocket_reflect_count( "tf_bot_npc_stun_rocket_reflect_count", "2"/*, FCVAR_CHEAT */ );
ConVar tf_bot_npc_stun_rocket_reflect_duration( "tf_bot_npc_stun_rocket_reflect_duration", "1"/*, FCVAR_CHEAT */ );

ConVar tf_bot_npc_grenade_interval( "tf_bot_npc_grenade_interval", "10" );

ConVar tf_bot_npc_hate_taunt_cooldown( "tf_bot_npc_hate_taunt_cooldown", "10"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_debug_damage( "tf_bot_npc_debug_damage", "0"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_always_stun( "tf_bot_npc_always_stun", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_min_nuke_after_stun_time( "tf_bot_npc_min_nuke_after_stun_time", "5" /*, FCVAR_CHEAT */ );



//-----------------------------------------------------------------------------------------------------
// The Bot NPC
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( bot_boss, CBotNPC );

PRECACHE_REGISTER( bot_boss );

IMPLEMENT_SERVERCLASS_ST( CBotNPC, DT_BotNPC )

	SendPropEHandle( SENDINFO( m_laserTarget ) ),
	SendPropBool( SENDINFO( m_isNuking ) ),

END_SEND_TABLE()


//------------------------------------------------------------------------------
void CBotNPC::InputSpawn( inputdata_t &inputdata )
{
	DispatchSpawn( this );
}


//-----------------------------------------------------------------------------------------------------
CBotNPC::CBotNPC()
{
	m_intention = new CBotNPCIntention( this );
	m_locomotor = new CBotNPCLocomotion( this );
	m_body = new CBotNPCBody( this );
	m_vision = new CBotNPCVision( this );

	m_conditionFlags = 0;
	m_laserTarget = NULL;
	m_isNuking = false;
	m_ageTimer.Invalidate();
	m_spawner = NULL;
	ClearStunDamage();
}


//-----------------------------------------------------------------------------------------------------
CBotNPC::~CBotNPC()
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
void CBotNPC::Precache()
{
	BaseClass::Precache();

#ifdef USE_BOSS_SENTRY
	int model = PrecacheModel( "models/bots/boss_sentry/boss_sentry.mdl" );
#else
	int model = PrecacheModel( "models/bots/knight/knight.mdl" );
#endif

	PrecacheGibsForModel( model );

	PrecacheModel( "models/weapons/c_models/c_bigsword/c_bigsword.mdl" );
	PrecacheModel( "models/weapons/c_models/c_bigshield/c_bigshield.mdl" );
	PrecacheModel( "models/weapons/c_models/c_big_mean_mother_hubbard/c_big_mean.mdl" );
	
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
	PrecacheScriptSound( "Cart.Explode" );

	PrecacheParticleSystem( "asplode_hoodoo_embers" );
	PrecacheParticleSystem( "charge_up" );

	PrecacheArmorParts();
}


//-----------------------------------------------------------------------------------------------------
void CBotNPC::PrecacheArmorParts( void )
{
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::TEXT_BUFFER );

	// filename is local to game dir for Steam, so we need to prepend game dir
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	char filename[256];
	Q_snprintf( filename, sizeof( filename ), "%s\\models\\bots\\knight\\armor_parts.txt", gamePath );

	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to read %s\n", filename );
	}
	else
	{
		while( true )
		{
			char partName[256];

			if ( fileBuffer.Scanf( "%s", partName ) <= 0 )
			{
				break;
			}

			// Make sure we have a valid string before trying to precache it.
			if ( Q_strlen( partName ) > 0 )
			{
				PrecacheModel( partName );
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------------
void CBotNPC::InstallArmorParts( void )
{
	if ( IsMiniBoss() )
		return;

	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::TEXT_BUFFER );

	// filename is local to game dir for Steam, so we need to prepend game dir
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );

	char filename[256];
	Q_snprintf( filename, sizeof( filename ), "%s\\models\\bots\\knight\\armor_parts.txt", gamePath );

	if ( !filesystem->ReadFile( filename, "MOD", fileBuffer ) )
	{
		Warning( "Unable to read %s\n", filename );
	}
	else
	{
		while( true )
		{
			char partName[256];

			if ( fileBuffer.Scanf( "%s", partName ) <= 0 )
			{
				break;
			}

			CBaseAnimating *part = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
			if ( part )
			{
				part->SetModel( partName );

				// bonemerge into our model
				part->FollowEntity( this, true );

				m_armorPartVector.AddToTail( part );
			}		
		}
	}
}


//-----------------------------------------------------------------------------------------------------
void CBotNPC::Spawn( void )
{
	BaseClass::Spawn();

#ifdef USE_BOSS_SENTRY
	SetModel( "models/bots/boss_sentry/boss_sentry.mdl" );
#else
	SetModel( "models/bots/knight/knight.mdl" );
#endif

	InstallArmorParts();

	ModifyMaxHealth( tf_bot_npc_health.GetInt() );

	// show Boss' health meter on HUD
	if ( g_pMonsterResource )
	{
		g_pMonsterResource->SetBossHealthPercentage( 1.0f );
	}

	m_damagePoseParameter = -1;
	m_conditionFlags = 0;

	// randomize initial check
	m_nearestVisibleEnemy = NULL;
	m_nearestVisibleEnemyTimer.Start( RandomFloat( 0.0f, tf_bot_npc_reaction_time.GetFloat() ) );

	m_homePos = GetAbsOrigin();

	m_currentDamagePerSecond = 0.0f;
	m_lastDamagePerSecond = 0.0f;

	m_attackTarget = NULL;
	m_attackTargetTimer.Invalidate();
	m_isAttackTargetLocked = false;

	m_nukeTimer.Start( tf_bot_npc_nuke_interval.GetFloat() );
	m_isNuking = false;

	m_grenadeTimer.Start( GetGrenadeInterval() );
	m_ageTimer.Start();

	ChangeTeam( TF_TEAM_RED );

	TFGameRules()->SetActiveBoss( this );
}


//-----------------------------------------------------------------------------------------------------
ConVar tf_bot_npc_dmg_mult_sniper( "tf_bot_npc_dmg_mult_sniper", "1.5"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_dmg_mult_minigun( "tf_bot_npc_dmg_mult_minigun", "0.5"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_dmg_mult_flamethrower( "tf_bot_npc_dmg_mult_flamethrower", "1"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_dmg_mult_sentrygun( "tf_bot_npc_dmg_mult_sentrygun", "0.5"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_dmg_mult_grenade( "tf_bot_npc_dmg_mult_grenade", "2"/*, FCVAR_CHEAT*/ );


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
			return info.GetDamage() * tf_bot_npc_dmg_mult_sniper.GetFloat();

		case TF_WEAPON_MINIGUN:
			return info.GetDamage() * tf_bot_npc_dmg_mult_minigun.GetFloat();

		case TF_WEAPON_FLAMETHROWER:
			return info.GetDamage() * tf_bot_npc_dmg_mult_flamethrower.GetFloat();

		case TF_WEAPON_SENTRY_BULLET:
			return info.GetDamage() * tf_bot_npc_dmg_mult_sentrygun.GetFloat();

		case TF_WEAPON_GRENADE_DEMOMAN:
			return info.GetDamage() * tf_bot_npc_dmg_mult_grenade.GetFloat();
		}
	}

	// unmodified
	return info.GetDamage();
}


//-----------------------------------------------------------------------------------------------------
int CBotNPC::OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo )
{
	CTakeDamageInfo info = rawInfo;

	// don't take damage from myself
	if ( info.GetAttacker() == this )
	{
		return 0;
	}

	if ( IsInCondition( INVULNERABLE ) )
	{
		return 0;
	}

	if ( IsInCondition( SHIELDED ) )
	{
		// no damage from the front
		CBaseEntity *inflictor = info.GetInflictor();
		if ( inflictor )
		{
			Vector myForward;
			GetVectors( &myForward, NULL, NULL );

			Vector themForward;
			inflictor->GetVectors( &themForward, NULL, NULL );

			if ( DotProduct( themForward, myForward ) < -0.7071f )
			{
				// blocked by my shield
				EmitSound( "FX_RicochetSound.Ricochet" );
				DispatchParticleEffect( "asplode_hoodoo_embers", info.GetDamagePosition(), GetAbsAngles() );

				return 0;
			}
		}
	}

	
	// weapon-specific damage modification
	info.SetDamage( ModifyBossDamage( info ) );


	if ( IsInCondition( VULNERABLE_TO_STUN ) )
	{
		// Heavies can't deal stun damage (too high DPS)
		//CTFPlayer *playerAttacker = ToTFPlayer( info.GetAttacker() );

		if ( true ) // !playerAttacker ) || !playerAttacker->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			// track head damage when vulnerable
			Vector headPos;
			QAngle headAngles;
			if ( GetAttachment( "head", headPos, headAngles ) )
			{
				Vector damagePos = info.GetDamagePosition();

/*
				const trace_t &pTrace = CBaseEntity::GetTouchTrace();
				damagePos = pTrace.endpos;
*/

/*
				CBaseEntity *inflictor = info.GetInflictor();
				if ( inflictor )
				{
					damagePos = inflictor->GetAbsOrigin() + 3.0f * gpGlobals->frametime * inflictor->GetAbsVelocity();
				}
*/

				if ( tf_bot_npc_debug_damage.GetBool() )
				{
					NDebugOverlay::Cross3D( headPos, 5.0f, 255, 0, 0, true, 5.0f );
					NDebugOverlay::Cross3D( damagePos, 5.0f, 0, 255, 0, true, 5.0f );
					NDebugOverlay::Line( damagePos, headPos, 255, 255, 0, true, 5.0f );
				}

				bool isHeadHit = ( damagePos - headPos ).IsLengthLessThan( tf_bot_npc_head_radius.GetFloat() );

				if ( isHeadHit )
				{
					// hit the head
					AccumulateStunDamage( info.GetDamage() );
					DispatchParticleEffect( "asplode_hoodoo_embers", info.GetDamagePosition(), GetAbsAngles() );

					if ( tf_bot_npc_debug_damage.GetBool() )
					{
						DevMsg( "Stun dmg = %f\n", GetStunDamage() );
						NDebugOverlay::Circle( headPos, tf_bot_npc_head_radius.GetFloat(), 255, 0, 0, 255, true, 5.0f );
					}
				}
				else if ( tf_bot_npc_debug_damage.GetBool() )
				{
					NDebugOverlay::Circle( headPos, tf_bot_npc_head_radius.GetFloat(), 255, 255, 0, 255, true, 5.0f );
				}
			}
		}
	}

	// take extra damage when stunned
	if ( IsInCondition( STUNNED ) )
	{
		info.SetDamage( info.GetDamage() * tf_bot_npc_stunned_injury_multiplier.GetFloat() );

		if ( m_ouchTimer.IsElapsed() )
		{
			m_ouchTimer.Start( 1.0f );
			EmitSound( "RobotBoss.Hurt" );
		}
	}
	else if ( info.GetDamageType() & DMG_CRITICAL )
	{
		// do the critical damage increase
		info.SetDamage( info.GetDamage() * TF_DAMAGE_CRIT_MULTIPLIER );
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

	EmitSound( "TFPlayer.Pain" );

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

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->SetBossHealthPercentage( (float)GetHealth() / (float)GetMaxHealth() );
	}

	return result;
}


//---------------------------------------------------------------------------------------------
// Returns true if we're in a condition that means we can't start another action
bool CBotNPC::IsBusy( void ) const
{
	return IsInCondition( (Condition)( CHARGING | STUNNED | VULNERABLE_TO_STUN | BUSY ) );
}


//---------------------------------------------------------------------------------------------
void CBotNPC::RememberAttacker( CBaseCombatCharacter *attacker, float damage, bool wasCritical )
{
	AttackerInfo attackerInfo;

	attackerInfo.m_attacker = attacker;
	attackerInfo.m_timestamp = gpGlobals->curtime;
	attackerInfo.m_damage = damage;
	attackerInfo.m_wasCritical = wasCritical;

	m_attackerVector.AddToHead( attackerInfo );
}


//----------------------------------------------------------------------------------
CTFPlayer *CBotNPC::GetClosestMinionPrisoner( void )
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
bool CBotNPC::IsPrisonerOfMinion( CBaseCombatCharacter *victim )
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
void CBotNPC::UpdateDamagePerSecond( void )
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
const CBotNPC::ThreatInfo *CBotNPC::GetMaxThreat( void ) const
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
const CBotNPC::ThreatInfo *CBotNPC::GetThreat( CBaseCombatCharacter *who ) const
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
void CBotNPC::UpdateAttackTarget( void )
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

	if ( !attackTargetThreat || maxThreat->m_threat > attackTargetThreat->m_threat + tf_bot_npc_threat_tolerance.GetFloat() )
	{
		// change threats
		SetAttackTarget( maxThreat->m_who );
	}
}


//----------------------------------------------------------------------------------
void CBotNPC::RemoveCondition( Condition c )
{
	if ( c == STUNNED )
	{
		// reset the accumulator
		ClearStunDamage();
	}

	m_conditionFlags &= ~c;
}


//----------------------------------------------------------------------------------
void CBotNPC::SwingAxe( void )
{
	if ( !IsSwingingAxe() )
	{
		AddGesture( ACT_MP_ATTACK_STAND_ITEM1 );
		m_axeSwingTimer.Start( 0.58f );
		EmitSound( "Weapon_Sword.Swing" );
	}
}


//----------------------------------------------------------------------------------
void CBotNPC::UpdateAxeSwing( void )
{
	if ( !m_axeSwingTimer.HasStarted() )
	{
		return;
	}

	// continue axe swing
	if ( !m_axeSwingTimer.IsElapsed() )
	{
		return;
	}

	// moment of impact - did axe swing hit?
	m_axeSwingTimer.Invalidate();

	CBaseCombatCharacter *victim = GetAttackTarget();

	if ( victim )
	{
		Vector forward;
		GetVectors( &forward, NULL, NULL );

		Vector toVictim = victim->WorldSpaceCenter() - WorldSpaceCenter();
		toVictim.NormalizeInPlace();

		if ( DotProduct( forward, toVictim ) > 0.7071f )
		{
			if ( IsRangeLessThan( victim, 0.9f * tf_bot_npc_attack_range.GetFloat() ) )
			{
				if ( IsLineOfSightClear( victim ) )
				{
					// CHOP!
					CTakeDamageInfo info( this, this, tf_bot_npc_melee_damage.GetFloat(), DMG_SLASH, TF_DMG_CUSTOM_NONE );
					CalculateMeleeDamageForce( &info, toVictim, WorldSpaceCenter(), 1.0f );
					victim->TakeDamage( info );
					EmitSound( "Weapon_Sword.HitFlesh" );
					return;
				}
			}
		}
	}

	EmitSound( "Weapon_Sword.HitWorld" );
}


//----------------------------------------------------------------------------------
bool CBotNPC::IsSwingingAxe( void ) const
{
	return const_cast< CBotNPC * >( this )->IsPlayingGesture( ACT_MP_ATTACK_STAND_ITEM1 );
}


//---------------------------------------------------------------------------------------------
void CBotNPC::Update( void )
{
	BaseClass::Update();

	UpdateNearestVisibleEnemy();
	UpdateAxeSwing();
	UpdateDamagePerSecond();
	UpdateAttackTarget();

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
				m_hateTauntTimer.Start( tf_bot_npc_hate_taunt_cooldown.GetFloat() );

				if ( IsLineOfSightClear( playerVector[i], IGNORE_ACTORS ) )
				{
					// the taunter becomes our new attack target
					SetAttackTarget( playerVector[i], tf_bot_npc_hate_taunt_cooldown.GetFloat() );
				}	
			}
		}
	}	
}


//---------------------------------------------------------------------------------------------
bool CBotNPC::IsPotentiallyChaseable( CTFPlayer *victim )
{
	if ( !victim )
	{
		return false;
	}

	if ( !victim->IsAlive() )
	{
		// victim is dead - pick a new one
		return false;
	}

	CTFNavArea *victimArea = (CTFNavArea *)victim->GetLastKnownArea();
	if ( !victimArea || victimArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
	{
		// unreachable - pick a new victim
		return false;
	}

	if ( victim->GetGroundEntity() != NULL )
	{
		Vector victimAreaPos;
		victimArea->GetClosestPointOnArea( victim->GetAbsOrigin(), &victimAreaPos );
		if ( ( victim->GetAbsOrigin() - victimAreaPos ).AsVector2D().IsLengthGreaterThan( 50.0f ) )
		{
			// off the mesh and unreachable - pick a new victim
			return false;
		}
	}

	if ( victim->m_Shared.IsInvulnerable() )
	{
		// invulnerable - pick a new victim
		return false;
	}

	Vector toHome = m_homePos - victim->GetAbsOrigin();
	if ( toHome.IsLengthGreaterThan( tf_bot_npc_quit_range.GetFloat() ) )
	{
		// too far from home - pick a new victim
		return false;
	}

	return true;
}


//---------------------------------------------------------------------------------------------
bool CBotNPC::IsIgnored( CTFPlayer *player ) const
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
void CBotNPC::UpdateNearestVisibleEnemy( void )
{
	if ( !m_nearestVisibleEnemyTimer.IsElapsed() )
	{
		return;
	}

	m_nearestVisibleEnemyTimer.Start( tf_bot_npc_reaction_time.GetFloat() );

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
void CBotNPC::SetAttackTarget( CBaseCombatCharacter *target, float duration )
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
CBaseCombatCharacter *CBotNPC::GetAttackTarget( void ) const
{
	if ( m_attackTarget != NULL && m_attackTarget->IsAlive() )
	{
		return m_attackTarget;
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
void CBotNPC::Break( void )
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
void CBotNPC::CollectPlayersStandingOnMe( CUtlVector< CTFPlayer * > *playerVector )
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
//---------------------------------------------------------------------------------------------
class CBotNPCStunned : public Action< CBotNPC >
{
public:
	CBotNPCStunned( float duration, Action< CBotNPC > *nextAction = NULL );

	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual EventDesiredResult< CBotNPC > OnInjured( CBotNPC *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "Stunned"; }		// return name of this action

private:
	CountdownTimer m_timer;
	enum StunStateType
	{
		BECOMING_STUNNED,
		STUNNED,
		RECOVERING
	}
	m_state;
	int m_layerUsed;

	Action< CBotNPC > *m_nextAction;
};


//---------------------------------------------------------------------------------------------
CBotNPCStunned::CBotNPCStunned( float duration, Action< CBotNPC > *nextAction )
{
	m_timer.Start( duration );
	m_nextAction = nextAction;
}


//---------------------------------------------------------------------------------------------
ConVar tf_bot_npc_stun_ammo_count( "tf_bot_npc_stun_ammo_count", "3"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_stun_ammo_amount( "tf_bot_npc_stun_ammo_amount", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_stun_ammo_velocity( "tf_bot_npc_stun_ammo_velocity", "100"/*, FCVAR_CHEAT*/ );

void TossAmmoPack( CBotNPC *me )
{
	int iPrimary = tf_bot_npc_stun_ammo_amount.GetInt();
	int iSecondary = tf_bot_npc_stun_ammo_amount.GetInt();
	int iMetal = tf_bot_npc_stun_ammo_amount.GetInt();

	// Create the ammo pack.
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( me->GetAbsOrigin(), me->GetAbsAngles(), NULL, "models/items/ammopack_medium.mdl" );
	if ( pAmmoPack )
	{
/*
		Vector vel;
		
		vel.x = RandomFloat( -1.0f, 1.0f ) * tf_bot_npc_stun_ammo_velocity.GetFloat();
		vel.y = RandomFloat( -1.0f, 1.0f ) * tf_bot_npc_stun_ammo_velocity.GetFloat();
		vel.z = tf_bot_npc_stun_ammo_velocity.GetFloat();

		pAmmoPack->SetInitialVelocity( vel );
*/
		pAmmoPack->m_nSkin = 0;

		// Give the ammo pack some health, so that trains can destroy it.
		pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		pAmmoPack->m_takedamage = DAMAGE_YES;		
		pAmmoPack->SetHealth( 900 );

		pAmmoPack->SetBodygroup( 1, 1 );

		pAmmoPack->ApplyLocalAngularVelocityImpulse( AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ) );

		DispatchSpawn( pAmmoPack );

		// Fill up the ammo pack.
		pAmmoPack->GiveAmmo( iPrimary, TF_AMMO_PRIMARY );
		pAmmoPack->GiveAmmo( iSecondary, TF_AMMO_SECONDARY );
		pAmmoPack->GiveAmmo( iMetal, TF_AMMO_METAL );
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCStunned::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_MELEE );
	m_layerUsed = me->AddLayeredSequence( me->LookupSequence( "PRIMARY_Stun_begin" ), 0 );
	m_state = BECOMING_STUNNED;

	m_timer.Reset();

	me->AddCondition( CBotNPC::STUNNED );
	me->EmitSound( "RobotBoss.StunStart" );

	// throw out some ammo
	for( int i=0; i<tf_bot_npc_stun_ammo_count.GetInt(); ++i )
	{
		TossAmmoPack( me );
	}

	me->m_outputOnStunned.FireOutput( me, me );

	// relay the event to the map logic
	CTFSpawnerBoss *spawner = me->GetSpawner();
	if ( spawner )
	{
		spawner->OnBotStunned( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCStunned::Update( CBotNPC *me, float interval )
{
	switch( m_state )
	{
	case BECOMING_STUNNED:
		if ( me->IsSequenceFinished() )
		{
			me->FastRemoveLayer( m_layerUsed );

			m_state = STUNNED;
			m_layerUsed = me->AddLayeredSequence( me->LookupSequence( "PRIMARY_stun_middle" ), 0 );
			me->SetLayerLooping( m_layerUsed, true );
			me->EmitSound( "RobotBoss.Stunned" );
		}
		break;

	case STUNNED:
		if ( m_timer.IsElapsed() )
		{
			me->FastRemoveLayer( m_layerUsed );

			m_state = RECOVERING;
			m_layerUsed = me->AddLayeredSequence( me->LookupSequence( "PRIMARY_stun_end" ), 0 );
			me->StopSound( "RobotBoss.Stunned" );
			me->EmitSound( "RobotBoss.StunRecover" );
		}
		break;

	case RECOVERING:
		if ( me->IsSequenceFinished() )
		{
			me->FastRemoveLayer( m_layerUsed );

			if ( m_nextAction )
			{
				return ChangeTo( m_nextAction, "Stun finished" );
			}

			return Done( "Stun finished" );
		}
		break;
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCStunned::OnInjured( CBotNPC *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL );
}


//---------------------------------------------------------------------------------------------
void CBotNPCStunned::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->RemoveCondition( CBotNPC::STUNNED );

	if ( me->HasAbility( CBotNPC::CAN_ENRAGE ) )
	{
		// being stunned makes the boss ANGRY!
		me->AddCondition( CBotNPC::ENRAGED );
	}

	// make sure the boss attacks at least once before he starts a nuke
	if ( me->GetNukeTimer()->GetRemainingTime() < tf_bot_npc_min_nuke_after_stun_time.GetFloat() )
	{
		me->GetNukeTimer()->Start( tf_bot_npc_min_nuke_after_stun_time.GetFloat() );
	}
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCBigJump : public Action< CBotNPC >
{
public:
	CBotNPCBigJump( const Vector &destination, Action< CBotNPC > *nextAction = NULL );

	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual EventDesiredResult< CBotNPC > OnInjured( CBotNPC *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "Jump"; }		// return name of this action

private:
	enum StunStateType
	{
		JUMPING_UP,
		FLOATING_UP,
		FALLING_DOWN
	}
	m_state;

	CountdownTimer m_timer;
	Vector m_destination;

	Action< CBotNPC > *m_nextAction;
};


//---------------------------------------------------------------------------------------------
CBotNPCBigJump::CBotNPCBigJump( const Vector &destination, Action< CBotNPC > *nextAction )
{
	m_destination = destination;
	m_nextAction = nextAction;
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCBigJump::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_JUMP_START_MELEE );
	m_state = JUMPING_UP;
	m_timer.Start( 3.0f );

	// disconnect us from the ground
	me->GetLocomotionInterface()->Jump();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCBigJump::Update( CBotNPC *me, float interval )
{
	// animation state
	switch( m_state )
	{
	case JUMPING_UP:
		if ( me->IsSequenceFinished() )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_JUMP_FLOAT_MELEE );
			m_state = FLOATING_UP;
		}
		break;
	}

	// movement
	switch( m_state )
	{
	case JUMPING_UP:
	case FLOATING_UP:
		me->GetLocomotionInterface()->SetVelocity( Vector( 0, 0, 1200.0f ) );

		if ( m_timer.IsElapsed() )
		{
			m_state = FALLING_DOWN;

			// move so we fall on our destination point
			me->SetAbsOrigin( m_destination + Vector( 0, 0, 1300.0f ) );
			me->GetLocomotionInterface()->SetVelocity( vec3_origin );
		}
		break;

	case FALLING_DOWN:
		if ( me->GetLocomotionInterface()->IsOnGround() )
		{
			me->AddGesture( ACT_MP_JUMP_LAND_MELEE );

			if ( m_nextAction )
			{
				return ChangeTo( m_nextAction, "Finished jump" );
			}

			return Done( "Finished jump" );
		}
		break;
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCBigJump::OnInjured( CBotNPC *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL );
}


//---------------------------------------------------------------------------------------------
void CBotNPCBigJump::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCLaunchMinions : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBotNPC >	OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction )	{ return Done(); }

	virtual const char *GetName( void ) const	{ return "LaunchMinions"; }		// return name of this action

private:
	CountdownTimer m_timer;
	int m_minionsLeft;

	bool SpawnMinion( CBotNPC *me );
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaunchMinions::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_SECONDARY );

	me->AddGestureSequence( me->LookupSequence( "taunt01" ) );

	m_timer.Start( 4.0f );

	int bonus = (int)( me->GetAge() / tf_bot_npc_minion_launch_count_increase_interval.GetFloat() );
	m_minionsLeft = tf_bot_npc_minion_launch_count_initial.GetInt() + bonus;

	return Continue();
}


//---------------------------------------------------------------------------------------------
bool CBotNPCLaunchMinions::SpawnMinion( CBotNPC *me )
{
	Vector spawnSpot = me->WorldSpaceCenter();

	Vector headPos;
	QAngle headAngles;
	if ( me->GetAttachment( "head", headPos, headAngles ) )
	{
		spawnSpot = headPos + RandomVector( -10.0f, 10.0f );
	}

	CBaseCombatCharacter *minion = static_cast< CBaseCombatCharacter * >( CreateEntityByName( "bot_npc_minion" ) );
	if ( minion )
	{
		minion->SetAbsAngles( me->GetAbsAngles() );
		minion->SetAbsOrigin( spawnSpot );
		minion->SetOwnerEntity( me );

		DispatchSpawn( minion );

		return true;
	}

	return false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaunchMinions::Update( CBotNPC *me, float interval )
{
	CBaseCombatCharacter *target = me->GetAttackTarget();

	if ( target )
	{
		me->GetLocomotionInterface()->FaceTowards( target->WorldSpaceCenter() );
	}

	if ( m_timer.IsElapsed() )
	{
		while( m_minionsLeft-- )
		{
			SpawnMinion( me );
		}

		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCNukeAttack : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual EventDesiredResult< CBotNPC > OnInjured( CBotNPC *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "NukeAttack"; }		// return name of this action

private:
	CountdownTimer m_shakeTimer;
	CountdownTimer m_chargeUpTimer;
};

ConVar tf_bot_npc_nuke_damage( "tf_bot_npc_nuke_damage", "75"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_nuke_max_remaining_health( "tf_bot_npc_nuke_max_remaining_health", "60"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_nuke_afterburn_time( "tf_bot_npc_nuke_afterburn_time", "5"/*, FCVAR_CHEAT*/ );

//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCNukeAttack::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_MP_JUMP_FLOAT_LOSERSTATE );
	me->StartNukeEffect();

	me->EmitSound( "RobotBoss.ChargeUpNukeAttack" );
	me->AddCondition( CBotNPC::VULNERABLE_TO_STUN );

	m_chargeUpTimer.Start( tf_bot_npc_nuke_charge_time.GetFloat() );
	m_shakeTimer.Start( 0.25f );


	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCNukeAttack::Update( CBotNPC *me, float interval )
{
	float stunRatio = me->GetStunDamage() / me->GetBecomeStunnedDamage();

	if ( me->HasAbility( CBotNPC::CAN_BE_STUNNED ) && stunRatio >= 1.0f )
	{
		return ChangeTo( new CBotNPCStunned( tf_bot_npc_stunned_duration.GetFloat() ), "They got me" );
	}

	// update the client's HUD
	if ( g_pMonsterResource )
	{
		g_pMonsterResource->SetBossStunPercentage( 1.0f - stunRatio );
	}

	if ( m_shakeTimer.IsElapsed() )
	{
		m_shakeTimer.Reset();
		UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 3000.0f, SHAKE_START );
	}

	if ( m_chargeUpTimer.IsElapsed() )
	{
		// BLAST!
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

		me->EmitSound( "RobotBoss.NukeAttack" );

		CUtlVector< CBaseCombatCharacter * > victimVector;

		int i;

		// players
		for ( i=0; i<playerVector.Count(); ++i )
		{
			CBasePlayer *player = playerVector[i];

			if ( player && player->IsAlive() && player->GetTeamNumber() == TF_TEAM_BLUE )
			{
				victimVector.AddToTail( player );
			}
		}

		// objects
		CTFTeam *team = GetGlobalTFTeam( TF_TEAM_BLUE );
		if ( team )
		{
			for ( i=0; i<team->GetNumObjects(); ++i )
			{
				CBaseObject *object = team->GetObject( i );
				if ( object )
				{
					victimVector.AddToTail( object );
				}
			}
		}

#ifdef SKIPME
		team = GetGlobalTFTeam( TF_TEAM_RED );
		if ( team )
		{
			for ( i=0; i<team->GetNumObjects(); ++i )
			{
				CBaseObject *object = team->GetObject( i );
				if ( object )
				{
					victimVector.AddToTail( object );
				}
			}
		}

		// non-player bots
		CUtlVector< INextBot * > botVector;
		TheNextBots().CollectAllBots( &botVector );
		for( i=0; i<botVector.Count(); ++i )
		{
			CBaseCombatCharacter *bot = botVector[i]->GetEntity();

			if ( !bot->IsPlayer() && bot->IsAlive() )
			{
				victimVector.AddToTail( bot );
			}
		}
#endif // SKIPME

		for( int i=0; i<victimVector.Count(); ++i )
		{
			CBaseCombatCharacter *victim = victimVector[i];

			if ( me->IsSelf( victim ) )
				continue;

			if ( me->IsLineOfSightClear( victim ) )
			{
				Vector toVictim = victim->WorldSpaceCenter() - me->WorldSpaceCenter();
				toVictim.NormalizeInPlace();

				float damage = tf_bot_npc_nuke_damage.GetFloat();

				if ( me->GetAge() > tf_bot_npc_nuke_lethal_time.GetFloat() )
				{
					// nuke is now lethal
					damage = 999.9f;
				}
				else if ( tf_bot_npc_nuke_max_remaining_health.GetFloat() >= 0.0f )
				{
					// nuke slams everyone's health to this
					if ( victim->GetHealth() > tf_bot_npc_nuke_max_remaining_health.GetFloat() )
					{
						damage = victim->GetHealth() - tf_bot_npc_nuke_max_remaining_health.GetFloat();
					}
				}

				CTakeDamageInfo info( me, me, damage, DMG_ENERGYBEAM, TF_DMG_CUSTOM_NONE );
				CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 1.0f );
				victim->TakeDamage( info );

				if ( victim->IsPlayer() )
				{
					CTFPlayer *playerVictim = ToTFPlayer( victim );

					// catch them on fire (unless they are a Pyro)
					if ( !playerVictim->IsPlayerClass( TF_CLASS_PYRO ) )
					{
						playerVictim->m_Shared.Burn( me, tf_bot_npc_nuke_afterburn_time.GetFloat() );
					}

					color32 colorHit = { 255, 255, 255, 255 };
					UTIL_ScreenFade( victim, colorHit, 1.0f, 0.1f, FFADE_IN );
				}
			}
		}

		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCNukeAttack::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->RemoveCondition( CBotNPC::VULNERABLE_TO_STUN );
	me->StopNukeEffect();
	me->ClearStunDamage();
	me->GetNukeTimer()->Start( tf_bot_npc_nuke_interval.GetFloat() );

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->HideBossStunMeter();
	}
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCNukeAttack::OnInjured( CBotNPC *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCLaunchRockets : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBotNPC >	OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction )	{ return Done(); }

	virtual const char *GetName( void ) const	{ return "LaunchRockets"; }		// return name of this action

private:
	CountdownTimer m_timer;

	CountdownTimer m_launchTimer;
	int m_rocketsLeft;

	int m_animLayer;

	CHandle< CBaseCombatCharacter > m_target;
	Vector m_lastTargetPosition;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaunchRockets::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_SECONDARY );

	m_animLayer = me->AddLayeredSequence( me->LookupSequence( "taunt02" ), 0 );

	m_timer.Start( 1.0f );

	m_rocketsLeft = me->GetRocketLaunchCount();

	me->AddCondition( CBotNPC::BUSY );
	me->LockAttackTarget();

	me->EmitSound( "RobotBoss.LaunchRockets" );

	if ( me->GetAttackTarget() == NULL )
	{
		return Done( "No target" );
	}

	m_target = me->GetAttackTarget();
	m_lastTargetPosition = m_target->WorldSpaceCenter();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaunchRockets::Update( CBotNPC *me, float interval )
{
	if ( m_target != NULL )
	{
		m_lastTargetPosition = m_target->WorldSpaceCenter();
	}

	me->GetLocomotionInterface()->FaceTowards( m_lastTargetPosition );

	if ( m_timer.IsElapsed() && m_launchTimer.IsElapsed() )
	{
		if ( !m_rocketsLeft )
		{
			return Done();
		}

		--m_rocketsLeft;
		m_launchTimer.Start( me->GetRocketInterval() );

		QAngle launchAngles = me->GetAbsAngles();

		if ( m_target == NULL )
		{
			Vector to = m_lastTargetPosition - me->WorldSpaceCenter();
			VectorAngles( to, launchAngles );
		}
		else
		{
			float range = me->GetRangeTo( m_target->EyePosition() );

			const float rocketSpeed = me->GetRocketAimError() * 1100.0f; // 2000.0f; // 1100.0f;  nerfing accuracy
			float flightTime = range / rocketSpeed;

			Vector aimSpot = m_target->EyePosition() + m_target->GetAbsVelocity() * flightTime;

			Vector to = aimSpot - me->WorldSpaceCenter();
			VectorAngles( to, launchAngles );
		}

		CTFProjectile_Rocket *pRocket = CTFProjectile_Rocket::Create( me, me->WorldSpaceCenter(), launchAngles, me, me );
		if ( pRocket )
		{
			if ( me->IsInCondition( CBotNPC::ENRAGED ) )
			{
				pRocket->SetCritical( true );
				pRocket->EmitSound( "Weapon_RPG.SingleCrit" );
			}
			else
			{
				me->EmitSound( me->GetRocketSoundEffect() );
			}

			pRocket->SetDamage( me->GetRocketDamage() );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCLaunchRockets::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->RemoveCondition( CBotNPC::ENRAGED );
	me->RemoveCondition( CBotNPC::BUSY );
	me->FastRemoveLayer( m_animLayer );
	me->UnlockAttackTarget();
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCRush : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBotNPC >	OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction )	{ return Done(); }

	virtual EventDesiredResult< CBotNPC > OnContact( CBotNPC *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "Rush"; }		// return name of this action

private:
	CountdownTimer m_timer;
	Vector m_chargeOrigin;
	float m_maxAttainedSpeed;
	float m_lastSpeed;
	bool m_didHitVictim;
};


//---------------------------------------------------------------------------------------------
void PushawayPlayer( CTFPlayer *victim, const Vector &pushOrigin, float pushForce )
{
	if ( !victim )
		return;

	if ( victim->GetFlags() & FL_ONGROUND )
	{
		// launching into the air
		victim->SetAbsVelocity( vec3_origin );

		const float stunTime = 0.5f;
		victim->m_Shared.StunPlayer( stunTime, 1.0, TF_STUN_MOVEMENT );

		victim->ApplyPunchImpulseX( RandomInt( 10, 15 ) );
		victim->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:0,victim:1" );
	}

	victim->RemoveFlag( FL_ONGROUND );

	Vector toVictim = victim->WorldSpaceCenter() - pushOrigin;
	toVictim.z = 0.0f;
	toVictim.NormalizeInPlace();
	toVictim.z = 1.0f;

	victim->ApplyAbsVelocityImpulse( pushForce * toVictim );
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCRush::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	m_timer.Start( 1.5f );
	m_chargeOrigin = me->GetAbsOrigin();
	m_maxAttainedSpeed = 0.0f;
	m_lastSpeed = 0.0f;
	m_didHitVictim = false;

	me->AddCondition( CBotNPC::CHARGING );
	me->AddCondition( CBotNPC::SHIELDED );

	me->EmitSound( "Halloween.HeadlessBossAttack" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCRush::Update( CBotNPC *me, float interval )
{
	// pushaway/hit nearby players
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	Vector chargeVector = me->GetAbsOrigin() - m_chargeOrigin;
	chargeVector.NormalizeInPlace();

	const float chargeRadius = 150.0f;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *victim = playerVector[i];

		if ( me->IsRangeGreaterThan( victim, chargeRadius ) )
			continue;

		Vector closestPointOnChargePath;
		CalcClosestPointOnLine( victim->GetAbsOrigin(), m_chargeOrigin, me->GetAbsOrigin(), closestPointOnChargePath );

		Vector fromChargePath = victim->GetAbsOrigin() - closestPointOnChargePath;
		float range = fromChargePath.NormalizeInPlace();

		if ( range >= chargeRadius )
			continue;

		if ( !me->IsLineOfSightClear( victim ) )
			continue;

		float nearness = 1.0f - ( range / chargeRadius );

		// push 'em
		float pushForce = tf_bot_npc_charge_pushaway_force.GetFloat() * nearness;
		PushawayPlayer( victim, closestPointOnChargePath, pushForce );

		// crunch 'em
		CTakeDamageInfo info( me, me, tf_bot_npc_charge_damage.GetFloat() * nearness, DMG_CRUSH, TF_DMG_CUSTOM_NONE );

		CalculateMeleeDamageForce( &info, fromChargePath, closestPointOnChargePath, 1.0f );

		victim->TakeDamage( info );

		color32 color = { 255, 0, 0, 255 };
		UTIL_ScreenFade( victim, color, 0.5f, 0.1f, FFADE_IN );

		if ( nearness > 0.5f )
		{
			m_didHitVictim = true;
		}
	}

	float speed = me->GetLocomotionInterface()->GetVelocity().Length();
	m_maxAttainedSpeed = MAX( m_maxAttainedSpeed, speed );

	if ( m_timer.IsElapsed() )
	{
		return ChangeTo( new CBotNPCLaunchRockets, "Finished charge" );
	}
	else
	{
		// chaaarge!
		me->GetLocomotionInterface()->Run();

		Vector forward;
		me->GetVectors( &forward, NULL, NULL );
		me->GetLocomotionInterface()->Approach( 100.0f * forward + me->GetLocomotionInterface()->GetFeet() );

		if ( !m_didHitVictim && m_maxAttainedSpeed > 350.0f && speed - m_lastSpeed < -200.0f )
		{
			// abrupt slowdown = bonk!
			return ChangeTo( new CBotNPCStunned( 3.0f, new CBotNPCLaunchRockets ), "Smacked into the world" );
		}
	}

	// animation
	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_CROUCHWALK_PRIMARY ) )
	{
		me->GetBodyInterface()->StartActivity( ACT_MP_CROUCHWALK_PRIMARY );
	}

	m_lastSpeed = speed;

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCRush::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->RemoveCondition( CBotNPC::SHIELDED );
	me->RemoveCondition( CBotNPC::CHARGING );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCRush::OnContact( CBotNPC *me, CBaseEntity *other, CGameTrace *result )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCBlock : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual ActionResult< CBotNPC >	OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction );

	virtual const char *GetName( void ) const	{ return "Block"; }		// return name of this action

private:
	CountdownTimer m_timer;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCBlock::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	// start animation
	me->SetSequence( me->LookupSequence( "marketing_pose_001" ) );
	me->SetPlaybackRate( 1.0f );
	me->SetCycle( 0 );
	me->ResetSequenceInfo();

	m_timer.Start( 3.0f );

	me->AddCondition( CBotNPC::SHIELDED );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCBlock::Update( CBotNPC *me, float interval )
{
	if ( m_timer.IsElapsed() )
	{
		return Done();
	}

	if ( me->GetAttackTarget() )
	{
		me->GetLocomotionInterface()->FaceTowards( me->GetAttackTarget()->WorldSpaceCenter() );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCBlock::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->RemoveCondition( CBotNPC::SHIELDED );
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCBlock::OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction )
{ 
	return Done(); 
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCLaunchGrenades : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBotNPC >	OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction )	{ return Done(); }

	virtual const char *GetName( void ) const	{ return "LaunchGrenades"; }		// return name of this action

private:
	CountdownTimer m_timer;
	CountdownTimer m_detonateTimer;
	CUtlVector< CHandle< CTFGrenadePipebombProjectile > > m_grenadeVector;
	void LaunchGrenade( CBotNPC *me, const Vector &launchVel, CTFWeaponInfo *weaponInfo );
	void LaunchGrenadeRings( CBotNPC *me );
	void LaunchGrenadeSpokes( CBotNPC *me );
	int m_animLayer;
};

ConVar tf_bot_npc_grenade_ring_min_horiz_vel( "tf_bot_npc_grenade_ring_min_horiz_vel", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grenade_ring_max_horiz_vel( "tf_bot_npc_grenade_ring_max_horiz_vel", "350"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grenade_vert_vel( "tf_bot_npc_grenade_vert_vel", "750"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grenade_det_time( "tf_bot_npc_grenade_det_time", "3"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaunchGrenades::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_SECONDARY );
	m_animLayer = me->AddLayeredSequence( me->LookupSequence( "gesture_melee_cheer" ), 0 );

	m_timer.Start( 1.0f );
	m_detonateTimer.Invalidate();
	me->AddCondition( CBotNPC::BUSY );
	me->GetGrenadeTimer()->Start( me->GetGrenadeInterval() );

	me->EmitSound( "RobotBoss.LaunchGrenades" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCLaunchGrenades::LaunchGrenade( CBotNPC *me, const Vector &launchVel, CTFWeaponInfo *weaponInfo )
{
	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( me->WorldSpaceCenter(), vec3_angle, launchVel, 
																					  AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), 
																					  me, *weaponInfo, TF_PROJECTILE_PIPEBOMB_REMOTE, 1 );
	if ( pProjectile )
	{
		pProjectile->SetLauncher( me );
		pProjectile->SetDamage( tf_bot_npc_grenade_damage.GetFloat() );

		if ( me->IsInCondition( CBotNPC::ENRAGED ) )
		{
			pProjectile->SetCritical( true );
		}

		m_grenadeVector.AddToTail( pProjectile );
	}
}


//---------------------------------------------------------------------------------------------
void CBotNPCLaunchGrenades::LaunchGrenadeRings( CBotNPC *me )
{
	const char *weaponAlias = WeaponIdToAlias( TF_WEAPON_GRENADELAUNCHER );
	if ( !weaponAlias )
		return;

	WEAPON_FILE_INFO_HANDLE	weaponInfoHandle = LookupWeaponInfoSlot( weaponAlias );
	if ( weaponInfoHandle == GetInvalidWeaponInfoHandle() )
		return;

	CTFWeaponInfo *weaponInfo = static_cast< CTFWeaponInfo * >( GetFileWeaponInfoFromHandle( weaponInfoHandle ) );

	QAngle myAngles = me->EyeAngles();

	// create rings of stickies
	float deltaVel = tf_bot_npc_grenade_ring_max_horiz_vel.GetFloat() - tf_bot_npc_grenade_ring_min_horiz_vel.GetFloat();
	const int ringCount = 2;
	for( int r=0; r<ringCount; ++r )
	{
		float u = (float)r/(float)(ringCount-1);

		float horizVel = tf_bot_npc_grenade_ring_min_horiz_vel.GetFloat() + u * deltaVel;

		float angleDelta = 10.0f + 20.0f * ( 1.0f - u );

		for( float angle=0.0f; angle<360.0f; angle += angleDelta )
		{
			Vector forward;
			AngleVectors( myAngles, &forward );

			Vector vecVelocity( horizVel * forward.x, horizVel * forward.y, tf_bot_npc_grenade_vert_vel.GetFloat() );

			LaunchGrenade( me, vecVelocity, weaponInfo );

			myAngles.y += angleDelta;
		}
	}
}


ConVar tf_bot_npc_grenade_spoke_angle( "tf_bot_npc_grenade_spoke_angle", "45"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grenade_spoke_count( "tf_bot_npc_grenade_spoke_count", "15"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grenade_spoke_min_horiz_vel( "tf_bot_npc_grenade_spoke_min_horiz_vel", "100"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grenade_spoke_max_horiz_vel( "tf_bot_npc_grenade_spoke_max_horiz_vel", "750"/*, FCVAR_CHEAT*/ );


//---------------------------------------------------------------------------------------------
void CBotNPCLaunchGrenades::LaunchGrenadeSpokes( CBotNPC *me )
{
	const char *weaponAlias = WeaponIdToAlias( TF_WEAPON_GRENADELAUNCHER );
	if ( !weaponAlias )
		return;

	WEAPON_FILE_INFO_HANDLE	weaponInfoHandle = LookupWeaponInfoSlot( weaponAlias );
	if ( weaponInfoHandle == GetInvalidWeaponInfoHandle() )
		return;

	CTFWeaponInfo *weaponInfo = static_cast< CTFWeaponInfo * >( GetFileWeaponInfoFromHandle( weaponInfoHandle ) );

	// create spokes of stickies
	float deltaVel = tf_bot_npc_grenade_spoke_max_horiz_vel.GetFloat() - tf_bot_npc_grenade_spoke_min_horiz_vel.GetFloat();
	float angleDelta = tf_bot_npc_grenade_spoke_angle.GetFloat();
	QAngle myAngles = me->EyeAngles();

	for( float angle=0.0f; angle<360.0f; angle += angleDelta )
	{
		Vector forward;
		AngleVectors( myAngles, &forward );

		int spokeCount = tf_bot_npc_grenade_spoke_count.GetInt();

		for( int i=0; i<spokeCount; ++i )
		{
			float u = (float)i/(float)(spokeCount-1);

			float horizVel = tf_bot_npc_grenade_spoke_min_horiz_vel.GetFloat() + u * deltaVel;

			Vector vecVelocity( horizVel * forward.x, horizVel * forward.y, tf_bot_npc_grenade_vert_vel.GetFloat() );

			LaunchGrenade( me, vecVelocity, weaponInfo );
		}

		myAngles.y += angleDelta;
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaunchGrenades::Update( CBotNPC *me, float interval )
{
	QAngle myAngles = me->EyeAngles();

	if ( m_timer.HasStarted() && m_timer.IsElapsed() )
	{
		m_timer.Invalidate();

		if ( RandomInt( 0, 100 ) < 50 )
		{
			LaunchGrenadeRings( me );
		}
		else
		{
			LaunchGrenadeSpokes( me );
		}

		me->EmitSound( "Weapon_Grenade_Normal.Single" );

		m_detonateTimer.Start( tf_bot_npc_grenade_det_time.GetFloat() );
	}

	if ( m_detonateTimer.HasStarted() && m_detonateTimer.IsElapsed() )
	{
		// detonate the stickies
		for( int i=0; i<m_grenadeVector.Count(); ++i )
		{
			if ( m_grenadeVector[i] )
			{
				m_grenadeVector[i]->Detonate();
			}
		}

		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCLaunchGrenades::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	// fizzle any outstanding stickies
	for( int i=0; i<m_grenadeVector.Count(); ++i )
	{
		if ( m_grenadeVector[i] )
		{
			m_grenadeVector[i]->Fizzle();
			m_grenadeVector[i]->Detonate();
		}
	}

	me->RemoveCondition( CBotNPC::ENRAGED );
	me->RemoveCondition( CBotNPC::BUSY );
	me->FastRemoveLayer( m_animLayer );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCShootCrossbow : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );

	// if anything interrupts this action, abort it
	virtual ActionResult< CBotNPC >	OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction )	{ return Done(); }

	virtual const char *GetName( void ) const	{ return "ShootCrossbow"; }		// return name of this action

private:
	CountdownTimer m_timer;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCShootCrossbow::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_SECONDARY );
	m_timer.Start( tf_bot_npc_aim_time.GetFloat() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCShootCrossbow::Update( CBotNPC *me, float interval )
{
	CBaseCombatCharacter *target = me->GetAttackTarget();

	if ( !target )
	{
		return Done( "No target" );
	}

	me->GetLocomotionInterface()->FaceTowards( target->WorldSpaceCenter() );

	if ( m_timer.IsElapsed() )
	{
		// fire bolt
		const float arrowSpeed = 4000.0f;
		const float arrowGravity = 0.0f;		// railgun

		Vector muzzleOrigin;
		QAngle muzzleAngles;
		if ( me->GetWeapon()->GetAttachment( "muzzle", muzzleOrigin, muzzleAngles ) == false )
		{
			return Done( "No muzzle attachment!" );
		}

		// lead target
		float range = me->GetRangeTo( target->EyePosition() );
		float flightTime = range / arrowSpeed;

		Vector aimSpot = target->EyePosition() + target->GetAbsVelocity() * flightTime;

		Vector to = aimSpot - muzzleOrigin;
		VectorAngles( to, muzzleAngles );

		CTFProjectile_Arrow *arrow = CTFProjectile_Arrow::Create( muzzleOrigin, muzzleAngles, arrowSpeed, arrowGravity, TF_PROJECTILE_ARROW, me, me );
		if ( arrow )
		{
			arrow->SetLauncher( me );
			arrow->SetCritical( true );

			// set damage to 5 points more than our target's max health so a Medic can save us
			// arrow->SetDamage( ( target->GetMaxHealth() + 5.0f ) / TF_DAMAGE_CRIT_MULTIPLIER );
			arrow->SetDamage( 200.0f );

			me->EmitSound( "Weapon_CompoundBow.Single" );
		}

		return Done();
	}

	return Continue();
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCLostVictim : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual const char *GetName( void ) const	{ return "LostVictim"; }		// return name of this action

private:
	CountdownTimer m_timer;
	float m_headTurn;
	int m_headYawPoseParameter;
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLostVictim::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	m_headTurn = 0.0f;
	m_headYawPoseParameter = me->LookupPoseParameter( "body_yaw" );

	m_timer.Start( RandomFloat( 3.0f, 5.0f ) );

	me->EmitSound( "RobotBoss.Scanning" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLostVictim::Update( CBotNPC *me, float interval )
{
	if ( m_timer.IsElapsed() )
	{
		return Done( "Giving up" );
	}

	CBaseCombatCharacter *target = me->GetAttackTarget();
	if ( target )
	{
		if ( me->IsLineOfSightClear( target ) || me->IsPrisonerOfMinion( target ) )
		{
			me->EmitSound( "RobotBoss.Acquire" );
			me->AddGesture( ACT_MP_GESTURE_FLINCH_CHEST );
			return Done( "Ah hah!" );
		}
	}

	const float rate = M_PI / 3.0f;
	m_headTurn += rate * interval;

	float s, c;
	SinCos( m_headTurn, &s, &c );

	me->SetPoseParameter( m_headYawPoseParameter, 40.0f * s );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCLostVictim::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->SetPoseParameter( m_headYawPoseParameter, 0 );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCChaseVictim : public Action< CBotNPC >
{
public:
	CBotNPCChaseVictim( CBaseCombatCharacter *chaseTarget );

	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual EventDesiredResult< CBotNPC > OnStuck( CBotNPC *me );
	virtual EventDesiredResult< CBotNPC > OnMoveToSuccess( CBotNPC *me, const Path *path );
	virtual EventDesiredResult< CBotNPC > OnMoveToFailure( CBotNPC *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "ChaseVictim"; }		// return name of this action

private:
	CTFPathFollower m_path;
	IntervalTimer m_visibleTimer;
	CHandle< CBaseCombatCharacter > m_lastTarget;

	CHandle< CBaseCombatCharacter > m_chaseTarget;
	Vector m_lastKnownTargetSpot;
};


//---------------------------------------------------------------------------------------------
CBotNPCChaseVictim::CBotNPCChaseVictim( CBaseCombatCharacter *chaseTarget )
{
	m_chaseTarget = chaseTarget;
	m_lastKnownTargetSpot = chaseTarget->GetAbsOrigin();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCChaseVictim::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	if ( m_chaseTarget == NULL )
	{
		return Done( "Target is NULL" );
	}

	m_lastKnownTargetSpot = m_chaseTarget->GetAbsOrigin();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCChaseVictim::Update( CBotNPC *me, float interval )
{
	if ( m_chaseTarget == NULL || !m_chaseTarget->IsAlive() )
	{
		return ChangeTo( new CBotNPCLostVictim, "No victim" );
	}

	if ( m_chaseTarget != me->GetAttackTarget() )
	{
		return Done( "Changing targets" );
	}

	Vector moveGoal = m_chaseTarget->GetAbsOrigin();

	if ( me->IsLineOfSightClear( m_chaseTarget ) )
	{
		if ( !m_visibleTimer.HasStarted() )
		{
			m_visibleTimer.Start();
		}

		if ( me->HasAbility( CBotNPC::CAN_NUKE ) && me->GetNukeTimer()->IsElapsed() )
		{
			return SuspendFor( new CBotNPCNukeAttack, "Nuking!" );
		}

		m_lastKnownTargetSpot = m_chaseTarget->GetAbsOrigin();

		if ( me->HasAbility( CBotNPC::CAN_LAUNCH_STICKIES ) )
		{
			if ( ( me->GetGrenadeTimer()->IsElapsed() && me->IsRangeLessThan( m_chaseTarget, tf_bot_npc_grenade_launch_range.GetFloat() ) ) ||
				   me->IsInCondition( CBotNPC::ENRAGED ) )
			{
				return SuspendFor( new CBotNPCLaunchGrenades, "Target is close (or I am enraged) - grenades!" );
			}
		}

		// chase into line of sight a bit so they can't immediately get behind cover again
		if ( me->HasAbility( CBotNPC::CAN_FIRE_ROCKETS ) )
		{
			if ( m_visibleTimer.IsGreaterThen( 1.0f ) ||
				 me->IsRangeLessThan( m_chaseTarget, tf_bot_npc_chase_range.GetFloat() ) )
			{
				return SuspendFor( new CBotNPCLaunchRockets, "Fire!" );
			}
		}

		if ( me->IsRangeLessThan( m_chaseTarget, 150.0f ) )
		{
			// too close - stand still
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_STAND_MELEE ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_STAND_MELEE );
			}

			return Continue();
		}
	}
	else
	{
		m_visibleTimer.Invalidate();

		// move to where we last saw our target
		moveGoal = m_lastKnownTargetSpot;

		if ( me->IsRangeLessThan( m_lastKnownTargetSpot, 20.0f ) )
		{
			// reached spot where we last saw our victim - give up
			me->SetAttackTarget( NULL );

			return ChangeTo( new CBotNPCLostVictim, "I lost my chase victim" );
		}
	}


	// move into sight of target
	if ( m_path.GetAge() > 1.0f )
	{
		CBotNPCPathCost cost( me );
		m_path.Compute( me, moveGoal, cost );
	}

	me->GetLocomotionInterface()->Run();
	m_path.Update( me );

	// play running animation
	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
	{
		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCChaseVictim::OnMoveToSuccess( CBotNPC *me, const Path *path )
{
	return TryDone( RESULT_CRITICAL, "Reached move goal" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCChaseVictim::OnMoveToFailure( CBotNPC *me, const Path *path, MoveToFailureType reason )
{
	return TryDone( RESULT_CRITICAL, "Path follow failed" );
}


//---------------------------------------------------------------------------------------------
void CBotNPCChaseVictim::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCChaseVictim::OnStuck( CBotNPC *me )
{
	// we're stuck - just warp to the our next path goal
	if ( m_path.GetCurrentGoal() )
	{
		me->SetAbsOrigin( m_path.GetCurrentGoal()->pos + Vector( 0, 0, 10.0f ) );
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCLaserBlast : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual ActionResult< CBotNPC >	OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction );

	virtual EventDesiredResult< CBotNPC > OnStuck( CBotNPC *me );

	virtual const char *GetName( void ) const	{ return "LaserBlast"; }		// return name of this action

private:
	CTFPathFollower m_path;
	CountdownTimer m_laserTimer;
	IntervalTimer m_visibleTimer;
	CHandle< CBaseCombatCharacter > m_lastTarget;
};

ConVar tf_bot_npc_laser_damage_rate( "tf_bot_npc_laser_damage_rate", "40"/*, FCVAR_CHEAT*/ );				// 20
ConVar tf_bot_npc_laser_damage_gain_rate( "tf_bot_npc_laser_damage_gain_rate", "0"/*, FCVAR_CHEAT*/ );		// 0
ConVar tf_bot_npc_laser_damage_ignite_threshold( "tf_bot_npc_laser_damage_ignite_threshold", "999"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_laser_damage_ignite_time( "tf_bot_npc_laser_damage_ignite_time", "3"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_laser_afterburn_time( "tf_bot_npc_laser_afterburn_time", "10"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_laser_damage_building_multiplier( "tf_bot_npc_laser_damage_building_multiplier", "4"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_laser_duration( "tf_bot_npc_laser_duration", "8"/*, FCVAR_CHEAT*/ );


//----------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaserBlast::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	m_laserTimer.Start( tf_bot_npc_laser_duration.GetFloat() );
	m_visibleTimer.Invalidate();
	m_lastTarget = NULL;

	return Continue();
}


//----------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaserBlast::Update( CBotNPC *me, float interval )
{
	CBaseCombatCharacter *target = me->GetAttackTarget();

	if ( !target )
	{
		return Done( "No victim" );
	}

	if ( me->HasAbility( CBotNPC::CAN_NUKE ) && me->GetNukeTimer()->IsElapsed() )
	{
		return ChangeTo( new CBotNPCNukeAttack, "Nuking!" );
	}

	if ( target != m_lastTarget )
	{
		// new target, reset laser
		m_laserTimer.Reset();
		m_lastTarget = target;
	}

	if ( me->HasAbility( CBotNPC::CAN_FIRE_ROCKETS ) && m_laserTimer.IsElapsed() )
	{
		// laser not effective - try rockets!
		return ChangeTo( new CBotNPCLaunchRockets, "Launching Rockets!" );
	}

	if ( me->IsLineOfSightClear( target ) )
	{
		if ( !m_visibleTimer.HasStarted() )
		{
			m_visibleTimer.Start();
		}

		me->GetLocomotionInterface()->FaceTowards( target->WorldSpaceCenter() );

		// blast 'em
		me->SetLaserTarget( target );

		float damage = tf_bot_npc_laser_damage_rate.GetFloat() + m_laserTimer.GetElapsedTime() * tf_bot_npc_laser_damage_gain_rate.GetFloat();

		// lasers do extra damage to buildings
		if ( target->IsBaseObject() )
		{
			damage *= tf_bot_npc_laser_damage_building_multiplier.GetFloat();
		}

		CTakeDamageInfo info( me, me, damage * interval, DMG_ENERGYBEAM, TF_DMG_CUSTOM_NONE );

		Vector toVictim = target->WorldSpaceCenter() - me->EyePosition();
		toVictim.NormalizeInPlace();

		CalculateMeleeDamageForce( &info, toVictim, me->EyePosition(), 1.0f );
		target->TakeDamage( info );

		if ( target->IsPlayer() && damage > tf_bot_npc_laser_damage_ignite_threshold.GetFloat() )
		{
			ToTFPlayer( target )->m_Shared.Burn( me, tf_bot_npc_laser_afterburn_time.GetFloat() );
		}

		if ( target->IsPlayer() && m_laserTimer.GetElapsedTime() > tf_bot_npc_laser_damage_ignite_time.GetFloat() )
		{
			ToTFPlayer( target )->m_Shared.Burn( me, tf_bot_npc_laser_afterburn_time.GetFloat() );
		}

		// me->EmitSound( "Weapon_Sword.HitFlesh" );

		if ( !me->IsPlayingGesture( ACT_MP_GESTURE_FLINCH_CHEST ) )
		{
			me->AddGesture( ACT_MP_GESTURE_FLINCH_CHEST );
		}
	}
	else
	{
		me->SetLaserTarget( NULL );
		m_laserTimer.Reset();
		m_visibleTimer.Invalidate();
	}

	// chase into line of sight a bit so they can't immediately get behind cover again
	if ( !m_visibleTimer.HasStarted() || m_visibleTimer.IsLessThen( 1.0f ) )
	{
		// don't get too close to avoid penetration/stuck issues
		if ( me->IsRangeGreaterThan( target, 100.0f ) )
		{
			// move into sight of target
			if ( m_path.GetAge() > 1.0f )
			{
				CBotNPCPathCost cost( me );
				m_path.Compute( me, target, cost );
			}

			me->GetLocomotionInterface()->Run();
			m_path.Update( me );
		}
	}

	if ( me->GetLocomotionInterface()->IsAttemptingToMove() )
	{
		// play running animation
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
		}
	}
	else
	{
		// standing still
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_STAND_ITEM1 ) )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM1 );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCLaserBlast::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->SetLaserTarget( NULL );
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCLaserBlast::OnSuspend( CBotNPC *me, Action< CBotNPC > *interruptingAction )
{
	return Done();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCLaserBlast::OnStuck( CBotNPC *me )
{
	// we're stuck - just warp to the our next path goal
	if ( m_path.GetCurrentGoal() )
	{
		me->SetAbsOrigin( m_path.GetCurrentGoal()->pos + Vector( 0, 0, 10.0f ) );
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCAttack : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );

	virtual ActionResult< CBotNPC >	OnResume( CBotNPC *me, Action< CBotNPC > *interruptingAction );

	virtual EventDesiredResult< CBotNPC > OnStuck( CBotNPC *me );
	virtual EventDesiredResult< CBotNPC > OnContact( CBotNPC *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "Attack"; }		// return name of this action

private:
	CTFPathFollower m_path;

	CountdownTimer m_chargeTimer;

	CHandle< CTFPlayer > m_closestVisible;
	CountdownTimer m_attackThrottleTimer;

	void ValidateChaseVictim( CBotNPC *me );

	CountdownTimer m_attackTargetFocusTimer;
};


//----------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCAttack::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	m_attackThrottleTimer.Invalidate();

	m_closestVisible = NULL;

	m_attackTargetFocusTimer.Invalidate();

	m_chargeTimer.Invalidate();

	return Continue();
}


//----------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCAttack::Update( CBotNPC *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done();
	}

	CBaseCombatCharacter *target = me->GetAttackTarget();

	if ( !target )
	{
		return Done( "No victim" );
	}

	me->GetLocomotionInterface()->FaceTowards( target->WorldSpaceCenter() );

	// swing our axe at our attack target if they are in range
	if ( !me->IsSwingingAxe() )
	{
		if ( me->IsRangeLessThan( target, tf_bot_npc_attack_range.GetFloat() ) )
		{
			me->SwingAxe();
		}
	}

	if ( !me->IsSwingingAxe() )
	{
		if ( m_chargeTimer.IsElapsed() && me->IsLookingTowards( target->WorldSpaceCenter(), 0.9f ) )
		{
			m_chargeTimer.Start( tf_bot_npc_charge_interval.GetFloat() );
			return SuspendFor( new CBotNPCRush, "Chaaarge!" );
		}

		if ( me->GetReceivedDamagePerSecond() > tf_bot_npc_block_dps_react.GetFloat() &&
			 target->IsPlayer() && 
			 ToTFPlayer( target )->GetTimeSinceWeaponFired() < 1.0f )
		{
			return SuspendFor( new CBotNPCBlock, "Blocking" );
		}
	}

	// chase after our victim
	const float standAndSwingRange = 0.5f * tf_bot_npc_attack_range.GetFloat();

	if ( me->IsRangeGreaterThan( target, standAndSwingRange ) || !me->IsLineOfSightClear( target ) )
	{
		if ( m_path.GetAge() > 1.0f )
		{
			CBotNPCPathCost cost( me );
			m_path.Compute( me, target, cost );
		}

		m_path.Update( me );
	}

	if ( me->GetLocomotionInterface()->IsAttemptingToMove() )
	{
		// play running animation
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
		}
	}
	else
	{
		// standing still
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_STAND_ITEM1 ) )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM1 );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCAttack::OnResume( CBotNPC *me, Action< CBotNPC > *interruptingAction )
{
	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCAttack::OnStuck( CBotNPC *me )
{
	// we're stuck - just warp to the our next path goal
	if ( m_path.GetCurrentGoal() )
	{
		me->SetAbsOrigin( m_path.GetCurrentGoal()->pos + Vector( 0, 0, 10.0f ) );
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCAttack::OnContact( CBotNPC *me, CBaseEntity *other, CGameTrace *result )
{
	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCGuardSpot : public Action< CBotNPC >
{
public:
	//-----------------------------------------------------------------------------------------------------
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
	{
		m_path.SetMinLookAheadDistance( 300.0f );

		me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM1 );
		me->SetHomePosition( me->GetAbsOrigin() );

		m_lookAtSpot = vec3_origin;

		return Continue();
	}

	//-----------------------------------------------------------------------------------------------------
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval )
	{
		CBaseCombatCharacter *target = me->GetAttackTarget();
		if ( target )
		{
			if ( me->IsLineOfSightClear( target ) || me->IsPrisonerOfMinion( target ) )
			{
				return SuspendFor( new CBotNPCChaseVictim( me->GetAttackTarget() ), "Get 'em!" );
			}
		}

		CBaseCombatCharacter *visible = me->GetNearestVisibleEnemy();
		if ( visible )
		{
			// look at visible victim out of range
			me->GetLocomotionInterface()->FaceTowards( visible->WorldSpaceCenter() );
		}

		const float atHomeRange = 50.0f;
		if ( me->IsRangeGreaterThan( me->GetHomePosition(), atHomeRange ) )
		{
			if ( m_path.GetAge() > 3.0f )
			{
				CBotNPCPathCost cost( me );
				if ( m_path.Compute( me, me->GetHomePosition(), cost ) == false )
				{
					// can't reach guard post - just jump there for now
					me->Teleport( &me->GetHomePosition(), NULL, NULL );
				}
			}

			m_path.Update( me );
		}
		else
		{
			// on guard spot - look around
			if ( m_lookTimer.IsElapsed() )
			{
				m_lookTimer.Start( RandomFloat( 1.0f, 2.0f ) );

				CTFNavArea *myArea = (CTFNavArea *)me->GetLastKnownArea();
				if ( myArea )
				{
					const CUtlVector< CTFNavArea * > &invasionAreaVector = myArea->GetEnemyInvasionAreaVector( TF_TEAM_RED );

					if ( invasionAreaVector.Count() > 0 )
					{
						// try to not look directly at walls
						const float minGazeRange = 300.0f;
						const int retryCount = 20.0f;
						for( int r=0; r<retryCount; ++r )
						{
							int which = RandomInt( 0, invasionAreaVector.Count()-1 );
							Vector gazeSpot = invasionAreaVector[ which ]->GetRandomPoint() + Vector( 0, 0, 0.75f * HumanHeight );

							if ( me->IsRangeGreaterThan( gazeSpot, minGazeRange ) && me->GetVisionInterface()->IsLineOfSightClear( gazeSpot ) )
							{
								// use maxLookInterval so these looks override body aiming from path following
								m_lookAtSpot = gazeSpot;
								break;
							}
						}
					}
				}
			}

			me->GetLocomotionInterface()->FaceTowards( m_lookAtSpot );
		}

		if ( me->GetLocomotionInterface()->IsAttemptingToMove() )
		{
			// play running animation
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
			}
		}
		else
		{
			// standing still
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_STAND_ITEM1 ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM1 );
			}
		}

		return Continue();
	}

	//-----------------------------------------------------------------------------------------------------
	virtual EventDesiredResult< CBotNPC > OnInjured( CBotNPC *me, const CTakeDamageInfo &info )
	{
		CTFPlayer *attacker = ToTFPlayer( info.GetAttacker() );

		if ( me->HasAbility( CBotNPC::CAN_BE_STUNNED ) && attacker )
		{
			if ( tf_bot_npc_always_stun.GetBool() )
			{
				return TrySuspendFor( new CBotNPCStunned( tf_bot_npc_stunned_duration.GetFloat() ), RESULT_CRITICAL, "CVar force stunned" );
			}


			bool isDeflectedRocket = false;
			CTFBaseRocket *pBaseRocket = dynamic_cast< CTFBaseRocket * >( info.GetInflictor() );
			if ( pBaseRocket && pBaseRocket->GetDeflected() )
			{
				isDeflectedRocket = true;
			}

			const float hardHit = 50.0f;
			bool isPotentialStunHit = info.GetDamage() > hardHit || isDeflectedRocket;
			
			if ( m_headStunTimer.IsElapsed() && isPotentialStunHit )
			{
				Vector headPos;
				QAngle headAngles;
				if ( me->GetAttachment( "head", headPos, headAngles ) )
				{
					if ( ( info.GetDamagePosition() - headPos ).IsLengthLessThan( tf_bot_npc_head_radius.GetFloat() ) )
					{
						// hit head

						// deflecting consecutive Boss' rockets into his head == stun
						if ( isDeflectedRocket )
						{
							if ( !m_consecutiveRocketTimer.HasStarted() ||	// first rocket hit
								 m_consecutiveRocketTimer.IsElapsed() )		// too much time between hits - treat as first hit
							{
								m_consecutiveRocketTimer.Start( tf_bot_npc_stun_rocket_reflect_duration.GetFloat() );
								m_consecutiveRockets = 1;
							}
							else
							{
								// successive rocket hit
								if ( ++m_consecutiveRockets >= tf_bot_npc_stun_rocket_reflect_count.GetInt() )
								{
									return TrySuspendFor( new CBotNPCStunned( tf_bot_npc_stunned_duration.GetFloat() ), RESULT_CRITICAL, "My own rockets reflected into my head!" );								
								}
							}

							me->EmitSound( "RobotBoss.Vulnerable" );
						}

						// look for hard hits from above
						Vector toAttacker = attacker->EyePosition() - headPos;
						toAttacker.NormalizeInPlace();

						if ( toAttacker.z > 0.9f )
						{
							// just got hit in the head from an attacker above me - stun
							m_headStunTimer.Start( 20.0f );

							return TrySuspendFor( new CBotNPCStunned( tf_bot_npc_stunned_duration.GetFloat() ), RESULT_CRITICAL, "Hard head hit from above!" );
						}
					}
				}
			}
		}

		return TryContinue();
	}

	//-----------------------------------------------------------------------------------------------------
	virtual const char *GetName( void ) const	{ return "GuardSpot"; }		// return name of this action

private:
	CTFPathFollower m_path;
	CountdownTimer m_lookTimer;
	Vector m_lookAtSpot;
	CountdownTimer m_headStunTimer;

	CountdownTimer m_consecutiveRocketTimer;
	int m_consecutiveRockets;
};


//---------------------------------------------------------------------------------------------
ConVar tf_bot_npc_get_off_me_duration( "tf_bot_npc_get_off_me_duration", "3"/*, FCVAR_CHEAT */ );

ActionResult< CBotNPC >	CBotNPCGetOffMe::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	me->AddGestureSequence( me->LookupSequence( "gesture_melee_help" ) );
	m_timer.Start( 0.5f );

	me->AddCondition( CBotNPC::BUSY );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCGetOffMe::Update( CBotNPC *me, float interval )
{
	if ( m_timer.IsElapsed() )
	{
		// blast players off of my head
		CUtlVector< CTFPlayer * > onMeVector;
		me->CollectPlayersStandingOnMe( &onMeVector );

		Vector headPos;
		QAngle headAngles;
		if ( me->GetAttachment( "head", headPos, headAngles ) )
		{
			for( int i=0; i<onMeVector.Count(); ++i )
			{
				// push 'em off
				PushawayPlayer( onMeVector[i], headPos, tf_bot_npc_charge_pushaway_force.GetFloat() );
			}
		}

		me->EmitSound( "Weapon_FlameThrower.AirBurstAttack" );

		return Done();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCGetOffMe::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->RemoveCondition( CBotNPC::BUSY );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCWaitForPlayers : public Action< CBotNPC >
{
public:
	virtual ActionResult< CBotNPC >	OnStart( CBotNPC *me, Action< CBotNPC > *priorAction );
	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval );
	virtual void					OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction );

	virtual EventDesiredResult< CBotNPC > OnInjured( CBotNPC *me, const CTakeDamageInfo &info );
	virtual EventDesiredResult< CBotNPC > OnContact( CBotNPC *me, CBaseEntity *other, CGameTrace *result = NULL );

	virtual const char *GetName( void ) const	{ return "WaitForPlayers"; }		// return name of this action
};


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCWaitForPlayers::OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
{
	me->AddCondition( CBotNPC::BUSY );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBotNPC >	CBotNPCWaitForPlayers::Update( CBotNPC *me, float interval )
{
	CBaseCombatCharacter *target = me->GetAttackTarget();
	if ( target )
	{
		return ChangeTo( new CBotNPCGuardSpot, "I see you..." );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBotNPCWaitForPlayers::OnEnd( CBotNPC *me, Action< CBotNPC > *nextAction )
{
	me->RemoveCondition( CBotNPC::BUSY );

	me->GetNukeTimer()->Start( tf_bot_npc_nuke_interval.GetFloat() );
	me->GetGrenadeTimer()->Reset();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCWaitForPlayers::OnInjured( CBotNPC *me, const CTakeDamageInfo &info )
{
	return TryChangeTo( new CBotNPCGuardSpot, RESULT_CRITICAL, "Ouch!" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBotNPC > CBotNPCWaitForPlayers::OnContact( CBotNPC *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other && other->IsPlayer() )
	{
		return TryChangeTo( new CBotNPCGuardSpot, RESULT_CRITICAL, "Don't touch me" );
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCTacticalMonitor : public Action< CBotNPC >
{
public:
	virtual Action< CBotNPC > *InitialContainedAction( CBotNPC *me )	
	{
		if ( TFGameRules()->IsBossBattleMode() )
		{
			return new CBotNPCWaitForPlayers;
		}

		return NULL;
	}

	virtual ActionResult< CBotNPC > OnStart( CBotNPC *me, Action< CBotNPC > *priorAction )
	{ 
		m_getOffMeTimer.Invalidate();

		return Continue(); 
	}

	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval )
	{
		// HACK: If we fell off the ledge, jump back
/*
		if ( me->GetLocomotionInterface()->IsOnGround() && 
			 me->GetAbsOrigin().z < me->GetHomePosition().z - 200.0f )
		{
			return SuspendFor( new CBotNPCBigJump( me->GetHomePosition(), new CBotNPCLaunchRockets ), "Jumping home" );
		}
*/

		if ( !m_getOffMeTimer.HasStarted() )
		{
			CUtlVector< CTFPlayer * > onMeVector;
			me->CollectPlayersStandingOnMe( &onMeVector );

			if ( onMeVector.Count() )
			{
				// someone is standing on me - push them off soon
				m_getOffMeTimer.Start( tf_bot_npc_get_off_me_duration.GetFloat() );
			}
		}
		else if ( m_getOffMeTimer.IsElapsed() )
		{
			if ( !me->IsBusy() )
			{
				m_getOffMeTimer.Invalidate();

				// if someone is still on me, push them off
				CUtlVector< CTFPlayer * > onMeVector;
				me->CollectPlayersStandingOnMe( &onMeVector );
				if ( onMeVector.Count() )
				{
					return SuspendFor( new CBotNPCGetOffMe, "Get offa me!" );
				}
			}
		}

		return Continue();
	}

	virtual const char *GetName( void ) const	{ return "TacticalMonitor"; }		// return name of this action

private:
	CountdownTimer m_backOffCooldownTimer;

	CountdownTimer m_getOffMeTimer;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CBotNPCBehavior : public Action< CBotNPC >
{
public:
	virtual Action< CBotNPC > *InitialContainedAction( CBotNPC *me )	
	{
		return new CBotNPCTacticalMonitor;
	}

	virtual ActionResult< CBotNPC >	Update( CBotNPC *me, float interval )
	{
		if ( m_vocalTimer.IsElapsed() )
		{
			m_vocalTimer.Start( RandomFloat( 3.0f, 5.0f ) );

			if ( !me->IsBusy() )
			{
				me->EmitSound( "RobotBoss.Vocalize" );
			}
		}

		return Continue();
	}

	virtual EventDesiredResult< CBotNPC > OnKilled( CBotNPC *me, const CTakeDamageInfo &info )
	{
		// relay the event to the map logic
		CTFSpawnerBoss *spawner = me->GetSpawner();
		if ( spawner )
		{
			spawner->OnBotKilled( me );
		}

		// Calculate death force
		Vector forceVector = me->CalcDamageForceVector( info );

		// See if there's a ragdoll magnet that should influence our force.
		CRagdollMagnet *magnet = CRagdollMagnet::FindBestMagnet( me );
		if ( magnet )
		{
			forceVector += magnet->GetForceVector( me );
		}

		if ( me->IsMiniBoss() )
		{
			me->EmitSound( "Cart.Explode" );
			me->BecomeRagdoll( info, forceVector );

			if ( g_pMonsterResource )
			{
				g_pMonsterResource->HideBossHealthMeter();
			}
		}
		else
		{
			// full end-of-game boss
			UTIL_Remove( me );

			if ( TFGameRules()->IsBossBattleMode() )
			{
				// check that ALL bosses are dead
				bool isBossBattleWon = true;

				CBotNPC *boss = NULL;
				while( ( boss = (CBotNPC *)gEntList.FindEntityByClassname( boss, "bot_boss" ) ) != NULL )
				{
					if ( !me->IsSelf( boss ) && boss->IsAlive() && !boss->IsMiniBoss() )
					{
						isBossBattleWon = false;
					}
				}

				if ( isBossBattleWon )
				{
					TFGameRules()->SetWinningTeam( TF_TEAM_BLUE, WINREASON_OPPONENTS_DEAD );

					if ( g_pMonsterResource )
					{
						g_pMonsterResource->HideBossHealthMeter();
					}
				}
			}
		}

		return TryDone();
	}

	virtual EventDesiredResult< CBotNPC > OnContact( CBotNPC *me, CBaseEntity *other, CGameTrace *result = NULL )
	{
		return TryContinue();
	}

	virtual const char *GetName( void ) const	{ return "Behavior"; }		// return name of this action

private:
	CountdownTimer m_vocalTimer;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CBotNPCIntention::CBotNPCIntention( CBotNPC *me ) : IIntention( me )
{ 
	m_behavior = new Behavior< CBotNPC >( new CBotNPCBehavior ); 
}

CBotNPCIntention::~CBotNPCIntention()
{
	delete m_behavior;
}

void CBotNPCIntention::Reset( void )
{ 
	delete m_behavior; 
	m_behavior = new Behavior< CBotNPC >( new CBotNPCBehavior );
}

void CBotNPCIntention::Update( void )
{
	m_behavior->Update( static_cast< CBotNPC * >( GetBot() ), GetUpdateInterval() ); 
}

// is the a place we can be?
QueryResultType CBotNPCIntention::IsPositionAllowed( const INextBot *meBot, const Vector &pos ) const
{
	return ANSWER_YES;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CBotNPCLocomotion::CBotNPCLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) 
{ 
	CBotNPC *me = (CBotNPC *)GetBot()->GetEntity();

	m_runSpeed = me->GetMoveSpeed();
}


//---------------------------------------------------------------------------------------------
float CBotNPCLocomotion::GetRunSpeed( void ) const
{
	CBotNPC *me = (CBotNPC *)GetBot()->GetEntity();

	return me->IsInCondition( CBotNPC::CHARGING ) ? 1000.0f : m_runSpeed;
}


//---------------------------------------------------------------------------------------------
// if delta Z is greater than this, we have to jump to get up
float CBotNPCLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum height of a jump
float CBotNPCLocomotion::GetMaxJumpHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// Return true to completely ignore this entity (may not be in sight when this is called)
bool CBotNPCVision::IsIgnored( CBaseEntity *subject ) const
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

#endif // OBSOLETE_USE_BOSS_ALPHA
