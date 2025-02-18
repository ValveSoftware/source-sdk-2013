//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_behavior.cpp
// Team Fortress NextBot
// Michael Booth, February 2009

#include "cbase.h"
#include "fmtstr.h"

#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_projectile_rocket.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_compound_bow.h"
#include "bot/tf_bot.h"
#include "bot/tf_bot_manager.h"
#include "bot/behavior/tf_bot_behavior.h"
#include "bot/behavior/tf_bot_dead.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_wait.h"
#include "bot/behavior/tf_bot_tactical_monitor.h"
#include "bot/behavior/tf_bot_taunt.h"
#include "bot/behavior/scenario/creep_wave/tf_bot_creep_wave.h"
#include "player_vs_environment/tf_population_manager.h"


extern ConVar tf_bot_health_ok_ratio;

ConVar tf_bot_path_lookahead_range( "tf_bot_path_lookahead_range", "300" );
ConVar tf_bot_sniper_aim_error( "tf_bot_sniper_aim_error", "0.01", FCVAR_CHEAT );
ConVar tf_bot_sniper_aim_steady_rate( "tf_bot_sniper_aim_steady_rate", "10", FCVAR_CHEAT );
ConVar tf_bot_debug_sniper( "tf_bot_debug_sniper", "0", FCVAR_CHEAT );
ConVar tf_bot_fire_weapon_min_time( "tf_bot_fire_weapon_min_time", "1", FCVAR_CHEAT );
ConVar tf_bot_taunt_victim_chance( "tf_bot_taunt_victim_chance", "20" );		// community requested this not be a cheat cvar

ConVar tf_bot_notice_backstab_chance( "tf_bot_notice_backstab_chance", "25", FCVAR_CHEAT );
ConVar tf_bot_notice_backstab_min_range( "tf_bot_notice_backstab_min_range", "100", FCVAR_CHEAT );
ConVar tf_bot_notice_backstab_max_range( "tf_bot_notice_backstab_max_range", "750", FCVAR_CHEAT );

ConVar tf_bot_arrow_elevation_rate( "tf_bot_arrow_elevation_rate", "0.0001", FCVAR_CHEAT, "When firing arrows at far away targets, this is the degree/range slope to raise our aim" );
ConVar tf_bot_ballistic_elevation_rate( "tf_bot_ballistic_elevation_rate", "0.01", FCVAR_CHEAT, "When lobbing grenades at far away targets, this is the degree/range slope to raise our aim" );

ConVar tf_bot_hitscan_range_limit( "tf_bot_hitscan_range_limit", "1800", FCVAR_CHEAT );

ConVar tf_bot_always_full_reload( "tf_bot_always_full_reload", "0", FCVAR_CHEAT );

ConVar tf_bot_fire_weapon_allowed( "tf_bot_fire_weapon_allowed", "1", FCVAR_CHEAT, "If zero, TFBots will not pull the trigger of their weapons (but will act like they did)" );
ConVar tf_bot_reevaluate_class_in_spawnroom( "tf_bot_reevaluate_class_in_spawnroom", "1", FCVAR_CHEAT, "If set, bots will opportunisticly switch class while in spawnrooms if their current class is no longer their first choice." );


//---------------------------------------------------------------------------------------------
Action< CTFBot > *CTFBotMainAction::InitialContainedAction( CTFBot *me )
{
	return new CTFBotTacticalMonitor;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMainAction::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_lastTouch = NULL;
	m_lastTouchTime = 0.0f;
	m_aimErrorRadius = 0.0f;
	m_aimErrorAngle = 0.0f;
	m_nextDisguise = TF_CLASS_UNDEFINED;

	m_yawRate = 0.0f;
	m_priorYaw = 0.0f;

	m_isWaitingForFullReload = false;

	// if bot is already dead at this point, make sure it's dead
	// check for !IsAlive because bot could be DYING
	if ( !me->IsAlive() )
	{
		return ChangeTo( new CTFBotDead, "I'm actually dead" );
	}

#ifdef TF_CREEP_MODE
	if ( TFGameRules()->IsCreepWaveMode() )
	{
		return ChangeTo( new CTFBotCreepWave, "I'm a creep" );
	}
#endif // TF_CREEP_MODE



	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMainAction::Update( CTFBot *me, float interval )
{
	VPROF_BUDGET( "CTFBotMainAction::Update", "NextBot" );

	if ( me->GetTeamNumber() != TF_TEAM_BLUE && me->GetTeamNumber() != TF_TEAM_RED )
	{
		// not on a team - do nothing
		return Done( "Not on a playing team" );
	}

	// Should I accept taunt from my partner?
	if ( me->FindPartnerTauntInitiator() )
	{
		return SuspendFor( new CTFBotTaunt, "Responding to teammate partner taunt" );
	}

	// make sure our vision FOV matches the player's
	me->GetVisionInterface()->SetFieldOfView( me->GetFOV() );

	// teammates in training have infinite ammo
	if ( TFGameRules()->IsInTraining() && me->GetTeamNumber() == TF_TEAM_BLUE )
	{
		me->GiveAmmo( 1000, TF_AMMO_METAL, true );
	}

	// track aim velocity ourselves, since body aim "steady" is too loose
	float deltaYaw = me->EyeAngles().y - m_priorYaw;
	m_yawRate = fabs( deltaYaw / ( interval + 0.0001f ) );
	m_priorYaw = me->EyeAngles().y;

	if ( m_yawRate < tf_bot_sniper_aim_steady_rate.GetFloat() )
	{
		if ( !m_steadyTimer.HasStarted() )
			m_steadyTimer.Start();

// 		if ( tf_bot_debug_sniper.GetBool() )
// 		{
// 			DevMsg( "%3.2f: STEADY\n", gpGlobals->curtime );
// 		}
	}
	else
	{
		m_steadyTimer.Invalidate();

// 		if ( tf_bot_debug_sniper.GetBool() )
// 		{
// 			DevMsg( "%3.2f: Yaw rate = %3.2f\n", gpGlobals->curtime, m_yawRate );
// 		}
	}

	CTFNavArea *myArea = me->GetLastKnownArea();
	int spawnRoomFlag = me->GetTeamNumber() == TF_TEAM_RED ? TF_NAV_SPAWN_ROOM_RED : TF_NAV_SPAWN_ROOM_BLUE;

	// should I try to change class?
	if ( tf_bot_reevaluate_class_in_spawnroom.GetBool() &&
	     !TFGameRules()->IsMannVsMachineMode() && 
		 !TFGameRules()->IsInTraining() && 
		 myArea && myArea->HasAttributeTF( spawnRoomFlag ) )
	{
		if ( !m_reevaluateClassTimer.HasStarted() )
		{
			// try to reevaluate class in a bit
			m_reevaluateClassTimer.Start( RandomFloat( 1.f, 2.f ) );
		}
		else if ( m_reevaluateClassTimer.IsElapsed() )
		{
			// try changing class
			m_reevaluateClassTimer.Invalidate();

			// reevaluate if we need to
			if ( me->ShouldReEvaluateCurrentClass() )
			{
				me->ReEvaluateCurrentClass();
				return Continue();
			}
		}
	}

	if ( TFGameRules()->IsMannVsMachineMode() && me->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		// infinite ammo
		// me->GiveAmmo( 100, TF_AMMO_PRIMARY, true );
		// me->GiveAmmo( 100, TF_AMMO_SECONDARY, true );
		// This resets the Sandman
		//me->GiveAmmo( 100, TF_AMMO_GRENADES1, true );
		// This resets the Bonk drink meter...
		//me->GiveAmmo( 100, TF_AMMO_GRENADES2, true );
		me->GiveAmmo( 100, TF_AMMO_METAL, true );

		me->m_Shared.AddToSpyCloakMeter( 100.0f, true );

		if ( myArea && myArea->HasAttributeTF( spawnRoomFlag ) )
		{
			// invading bots get uber while they leave their spawn so they don't drop their cash where players can't pick it up
			me->m_Shared.AddCond( TF_COND_INVULNERABLE, 0.5f );
			me->m_Shared.AddCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED, 0.5f );
			me->m_Shared.AddCond( TF_COND_INVULNERABLE_WEARINGOFF, 0.5f );
			me->m_Shared.AddCond( TF_COND_IMMUNE_TO_PUSHBACK, 1.0f );
		}

		// watch for bots that have fallen through the ground
		if ( myArea && myArea->GetZ( me->GetAbsOrigin() ) - me->GetAbsOrigin().z > 100.0f )
		{
			if ( !m_undergroundTimer.HasStarted() )
			{
				m_undergroundTimer.Start();
			}
			else if ( m_undergroundTimer.IsGreaterThen( 3.0f ) )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" underground (position \"%3.2f %3.2f %3.2f\")\n",
								me->GetPlayerName(),
								me->GetUserID(),
								me->GetNetworkIDString(),
								me->GetTeam()->GetName(),
								me->GetAbsOrigin().x, me->GetAbsOrigin().y, me->GetAbsOrigin().z );

				// teleport bot to a reasonable place
				me->SetAbsOrigin( myArea->GetCenter() );
			}
		}
		else
		{
			m_undergroundTimer.Invalidate();
		}

		if ( me->ShouldAutoJump() )
		{
			me->GetLocomotionInterface()->Jump();
		}
	}

	// spies always want to be disguised
	if ( !me->IsFiringWeapon() && !me->m_Shared.InCond( TF_COND_DISGUISED ) && !me->m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		if ( me->CanDisguise() )
		{
			if ( m_nextDisguise == TF_CLASS_UNDEFINED )
			{
				if ( me->IsDifficulty( CTFBot::EASY ) || me->IsDifficulty( CTFBot::NORMAL ) )
				{
					// disguise as a random class
					me->m_Shared.Disguise( GetEnemyTeam( me->GetTeamNumber() ), RandomInt( TF_FIRST_NORMAL_CLASS, TF_LAST_NORMAL_CLASS-1 ) );
				}
				else
				{
					me->DisguiseAsMemberOfEnemyTeam();
				}
			}
			else
			{
				// disguise as the class we just killed
				me->m_Shared.Disguise( GetEnemyTeam( me->GetTeamNumber() ), m_nextDisguise );
				m_nextDisguise = TF_CLASS_UNDEFINED;
			}
		}
	}

	me->EquipRequiredWeapon();

	me->UpdateLookingAroundForEnemies();
	FireWeaponAtEnemy( me );
	Dodge( me );

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		// dont auto reload, so we fire stickies fast
		me->SetAutoReload( false );
	}
	else
	{
		// reload weapons
		me->SetAutoReload( true );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult<CTFBot> CTFBotMainAction::OnKilled( CTFBot *me, const CTakeDamageInfo& info )
{
	return TryChangeTo( new CTFBotDead, RESULT_CRITICAL, "I died!" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMainAction::OnInjured( CTFBot *me, const CTakeDamageInfo &info )
{
	CBaseObject *obj = dynamic_cast< CBaseObject * >( info.GetInflictor() );

	// if an object hurt me, it must be a sentry
	CBaseEntity *subject = obj ? obj : info.GetAttacker();

	// notice the gunfire - needed for sentry guns, which don't go through the player OnWeaponFired() system
	me->GetVisionInterface()->AddKnownEntity( subject );

	if ( info.GetInflictor() && info.GetInflictor()->GetTeamNumber() != me->GetTeamNumber() )
	{
		CObjectSentrygun *sentrygun = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );

		if ( sentrygun )
		{
			// we were injured by an enemy sentry - remember it
			me->RememberEnemySentry( sentrygun, me->GetAbsOrigin() );
		}

		if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB )
		{
			// backstabs that don't kill me make me mad
			me->DelayedThreatNotice( info.GetInflictor(), 0.5f );

			// chance of nearby friends noticing the backstab
			CUtlVector< CTFPlayer * > playerVector;
			CollectPlayers( &playerVector, me->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );

			float minRange = tf_bot_notice_backstab_min_range.GetFloat();
			float maxRange = tf_bot_notice_backstab_max_range.GetFloat();
			float deltaRange = maxRange - minRange;

			for( int i=0; i<playerVector.Count(); ++i )
			{
				CTFBot *bot = ToTFBot( playerVector[i] );
				if ( bot )
				{
					if ( !me->IsSelf( bot ) )
					{
						float range = me->GetRangeTo( bot );

						if ( range > maxRange )
						{
							// too far away to notice
							continue;
						}

						int noticeChance = tf_bot_notice_backstab_chance.GetInt();

						if ( range > minRange )
						{
							// scale notice chance down to zero at max range
							noticeChance *= ( range - minRange ) / deltaRange;
						}

						if ( RandomInt( 0, 100 ) < noticeChance )
						{
							bot->DelayedThreatNotice( info.GetInflictor(), 0.5f );
						}
					}
				}
			}
		}
		else if ( info.GetAttacker() && ( info.GetDamageType() & DMG_CRITICAL ) && ( info.GetDamageType() & DMG_BURN ) )
		{
			// Notice anyone nearby hitting us with crit fire (i.e. Backburner)
			if ( me->GetRangeTo( info.GetAttacker() ) < tf_bot_notice_backstab_max_range.GetFloat() )
			{
				me->DelayedThreatNotice( info.GetAttacker(), 0.5f );
			}
		}
	}


#ifdef UNNEEDED	// known entity/listening to gunfire handles this without insta-turn

	if ( false && !me->IsSelf( info.GetAttacker() ) )
	{
		// hack to stop engineers from looking away from healing their sentry
		if ( !me->IsPlayerClass( TF_CLASS_ENGINEER ) && !me->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			CBaseObject *obj = dynamic_cast< CBaseObject * >( info.GetInflictor() );

			// if an object hurt me, it must be a sentry
			CBaseEntity *subject = obj ? obj : info.GetAttacker();

			if ( !me->GetVisionInterface()->IsInFieldOfView( subject ) )
			{
				// something out of my field of view hurt me - look around for it
				// turn right or left, since player's damage indicators tell them which way
				Vector forward, right;
				me->EyeVectors( &forward, &right );

				Vector toAttacker = subject->EyePosition() - me->EyePosition();
				Vector newForward;
				float error = 1.0f; RandomFloat( -1.0f, 1.0f );

				if ( DotProduct( right, toAttacker ) > 0.0f )
				{
					newForward = error * forward + right;
				}
				else
				{
					newForward = error * forward - right;
				}

				me->GetBodyInterface()->AimHeadTowards( me->EyePosition() + 100.0f * newForward, IBody::IMPORTANT, RandomFloat( 0.5f, 1.0f ), NULL, "Something hurt me!" );
			}
		}
	}
#endif // _DEBUG

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMainAction::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other && !other->IsSolidFlagSet( FSOLID_NOT_SOLID ) && !other->IsWorld() && !other->IsPlayer() )
	{
		m_lastTouch = other;
		m_lastTouchTime = gpGlobals->curtime;

		// Mini-bosses destroy non-Sentrygun objects they bump into (ie: Dispensers)
		if ( TFGameRules()->IsMannVsMachineMode() && me->IsMiniBoss() )
		{
			if ( other->IsBaseObject() )
			{
				CBaseObject *pObject = assert_cast< CBaseObject* >( other );
				if ( pObject->GetType() != OBJ_SENTRYGUN || pObject->IsMiniBuilding() )
				{
					int damage = MAX( other->GetMaxHealth(), other->GetHealth() );

					Vector toVictim = other->WorldSpaceCenter() - me->WorldSpaceCenter();

					CTakeDamageInfo info( me, me, 4 * damage, DMG_BLAST, TF_DMG_CUSTOM_NONE );
					CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 1.0f );
					other->TakeDamage( info );
				}
			}
		}
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
class BlockOverlappingAreaScan
{
public:
	BlockOverlappingAreaScan( int teamID, CBaseEntity *blocker )
	{
		m_teamID = teamID;
		m_blocker = blocker;
	}

	bool operator() ( CNavArea *baseArea )
	{
		CTFNavArea *area = static_cast< CTFNavArea * >( baseArea );

		area->SetAttributeTF( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE );

		return true;
	}

	int m_teamID;
	CBaseEntity *m_blocker;
};


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMainAction::OnStuck( CTFBot *me )
{
/*
	// if we are touching a func_door while stuck, assume the door is locked and block
	// the nav areas underneath it until the next stage of the scenario
	if ( m_lastTouch != NULL && gpGlobals->curtime - m_lastTouchTime < 2.0f )
	{
		if ( FClassnameIs( m_lastTouch, "func_door*" ) || FClassnameIs( m_lastTouch, "prop_door*" ) || FClassnameIs( m_lastTouch, "func_brush" ) )
		{
			Extent extent;
			extent.Init( m_lastTouch );

			BlockOverlappingAreaScan block( me->GetTeamNumber(), m_lastTouch );
			TheNavMesh->ForAllAreasOverlappingExtent( block, extent );
		}
	}
*/

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( me->m_Shared.InCond( TF_COND_MVM_BOT_STUN_RADIOWAVE ) )
		{
			// bot is stunned, not stuck
			return TryContinue();
		}

		if ( m_lastTouch != NULL && gpGlobals->curtime - m_lastTouchTime < 2.0f )
		{
			if ( m_lastTouch->IsBaseObject() && dynamic_cast< CObjectSentrygun * >( m_lastTouch.Get() ) == NULL )
			{
				// we are stuck on a teleporter or dispenser - destroy it!
				int damage = MAX( m_lastTouch->GetMaxHealth(), m_lastTouch->GetHealth() );

				Vector toVictim = m_lastTouch->WorldSpaceCenter() - me->WorldSpaceCenter();

				CTakeDamageInfo info( me, me, 4 * damage, DMG_BLAST, TF_DMG_CUSTOM_NONE );
				CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 1.0f );
				m_lastTouch->TakeDamage( info );
			}
		}
	}

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" stuck (position \"%3.2f %3.2f %3.2f\") (duration \"%3.2f\") ",
					me->GetPlayerName(),
					me->GetUserID(),
					me->GetNetworkIDString(),
					me->GetTeam()->GetName(),
					me->GetAbsOrigin().x, me->GetAbsOrigin().y, me->GetAbsOrigin().z,
					me->GetLocomotionInterface()->GetStuckDuration() );

	const PathFollower *path = me->GetCurrentPath();
	if ( path && path->GetCurrentGoal() )
	{
		UTIL_LogPrintf( "   path_goal ( \"%3.2f %3.2f %3.2f\" )\n",
						path->GetCurrentGoal()->pos.x, 
						path->GetCurrentGoal()->pos.y, 
						path->GetCurrentGoal()->pos.z );
	}
	else
	{
		UTIL_LogPrintf( "   path_goal ( \"NULL\" )\n" );
	}

	me->GetLocomotionInterface()->Jump();

	if ( RandomInt( 0, 100 ) < 50 )
	{
		me->PressLeftButton();
	}
	else
	{
		me->PressRightButton();
	}

/*
	if ( me->GetLocomotionInterface()->GetStuckDuration() > 3.0f )
	{
		// stuck for too long, do something drastic
		// warp to the our next path goal
		if ( me->GetCurrentPath() && me->GetCurrentPath()->GetCurrentGoal() )
		{
			me->SetAbsOrigin( me->GetCurrentPath()->GetCurrentGoal()->pos + Vector( 0, 0, StepHeight ) );

			UTIL_LogPrintf( "%3.2f: TFBot '%s' stuck for too long - slammed to goal position. Entindex = %d.\n", gpGlobals->curtime, me->GetPlayerName(), me->entindex() );
		}
	}
*/

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMainAction::OnOtherKilled( CTFBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	// make sure we forget about this guy
	me->GetVisionInterface()->ForgetEntity( victim );

	bool do_taunt = victim && victim->IsPlayer();


	if ( do_taunt )
	{
		CTFPlayer *playerVictim = ToTFPlayer( victim );

		me->ForgetSpy( playerVictim );

		if ( me->IsSelf( info.GetAttacker() ) && me->IsPlayerClass( TF_CLASS_SPY ) )
		{
			// disguise as our victim
			m_nextDisguise = playerVictim->GetPlayerClass()->GetClassIndex();
		}

		if ( !ToTFPlayer( victim )->IsBot() && me->IsEnemy( victim ) && me->IsSelf( info.GetAttacker() ) )
		{
			bool isTaunting = !me->HasTheFlag() && RandomFloat( 0.0f, 100.0f ) <= tf_bot_taunt_victim_chance.GetFloat();

			if ( TFGameRules()->IsMannVsMachineMode() && me->IsMiniBoss() )
			{
				// Bosses don't taunt puny humans
				isTaunting = false;
			}

			if ( isTaunting )
			{
				// we just killed a human - taunt!
				return TrySuspendFor( new CTFBotTaunt, RESULT_IMPORTANT, "Taunting our victim" );
			}
		}
	}

	// if we saw a friend killed by a sentry, kill the sentry
	if ( victim && victim->IsPlayer() && me->IsFriend( victim ) && info.GetInflictor() && me->IsEnemy( info.GetInflictor() ) && me->IsLineOfSightClear( victim->WorldSpaceCenter() ) )
	{
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( info.GetInflictor() );

		if ( sentry && !me->GetEnemySentry() )
		{
			me->RememberEnemySentry( sentry, victim->GetAbsOrigin() );
		}
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
/**
 * Given a subject, return the world space position we should aim at
 */
Vector CTFBotMainAction::SelectTargetPoint( const INextBot *meBot, const CBaseCombatCharacter *subject ) const
{
	CTFBot *me = (CTFBot *)meBot->GetEntity();

	if ( subject )
	{
		// if our subject is a sentry gun, aim at it's "eye position", which is updated based on the sentry's level
		if ( subject->IsBaseObject() )
		{
			CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( const_cast< CBaseCombatCharacter * >( subject ) );
			if ( sentry )
			{
				// Aim a bit lower than eye height to ensure we hit the body of the sentry
				return sentry->GetAbsOrigin() + 0.5f * sentry->GetViewOffset(); 
			}
		}

		CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
		if ( myWeapon )
		{
			// lead our target and aim for the feet with the rocket launcher
			if ( !me->IsDifficulty( CTFBot::EASY ) )
			{
				if ( myWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER )
				{
					// if they are above us, don't aim for the feet
					const float aboveTolerance = 30.0f;
					if ( subject->GetAbsOrigin().z - aboveTolerance > me->GetAbsOrigin().z )
					{
						if ( me->GetVisionInterface()->IsAbleToSee( subject->GetAbsOrigin(), IVision::DISREGARD_FOV ) )
							return subject->GetAbsOrigin();

						if ( me->GetVisionInterface()->IsAbleToSee( subject->WorldSpaceCenter(), IVision::DISREGARD_FOV ) )
							return subject->WorldSpaceCenter();

						return subject->EyePosition();
					}

					// aim at the ground under the subject
					if ( subject->GetGroundEntity() == NULL )
					{
						// they are airborne, find the ground underneath them, if they aren't too high
						trace_t result;
						UTIL_TraceLine( subject->GetAbsOrigin(), subject->GetAbsOrigin() + Vector( 0, 0, -200 ), MASK_SOLID, subject, COLLISION_GROUP_NONE, &result );
						if ( result.DidHit() )
						{
							return result.endpos;
						}
					}

					// aim at their feet

					// lead our target
					const float missileSpeed = 1100.0f;
					float rangeBetween = me->GetRangeTo( subject->GetAbsOrigin() );

					const float veryCloseRange = 150.0f;
					if ( rangeBetween > veryCloseRange )
					{
						float timeToTravel = rangeBetween / missileSpeed;

						Vector targetPos = subject->GetAbsOrigin() + timeToTravel * subject->GetAbsVelocity();

						if ( me->GetVisionInterface()->IsAbleToSee( targetPos, IVision::DISREGARD_FOV ) )
							return targetPos;

						// try their head and hope
						return subject->EyePosition() + timeToTravel * subject->GetAbsVelocity();
					}

					return subject->EyePosition();
				}
				else if ( myWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
				{
					// lead our target
					const float missileSpeed = ( (CTFCompoundBow *)myWeapon )->GetProjectileSpeed();
					float rangeBetween = me->GetRangeTo( subject->GetAbsOrigin() );

					const float veryCloseRange = 150.0f;
					if ( rangeBetween > veryCloseRange )
					{
						float timeToTravel = rangeBetween / missileSpeed;

						Vector targetSpot = me->IsDifficulty( CTFBot::NORMAL ) ? subject->WorldSpaceCenter() : subject->EyePosition();

						Vector leadTargetSpot = targetSpot + timeToTravel * subject->GetAbsVelocity();

						// elevate our aim based on range
						float elevationAngle = rangeBetween * tf_bot_arrow_elevation_rate.GetFloat();

						if ( elevationAngle > 45.0f )
						{
							// ballistic range maximum at 45 degrees - aiming higher would decrease the range
							elevationAngle = 45.0f;
						}

						float s, c;
						FastSinCos( elevationAngle * M_PI / 180.0f, &s, &c );

						if ( c > 0.0f ) 
						{
							float elevation = rangeBetween * s / c;
							return leadTargetSpot + Vector( 0, 0, elevation );
						}

						return leadTargetSpot;
					}

					return subject->EyePosition();
				}
			}

			if ( WeaponID_IsSniperRifle( myWeapon->GetWeaponID() ) )
			{
				if ( m_aimAdjustTimer.IsElapsed() )
				{
					m_aimAdjustTimer.Start( RandomFloat( 0.5f, 1.5f ) );

					m_aimErrorAngle = RandomFloat( -M_PI, M_PI );
					m_aimErrorRadius = RandomFloat( 0.0f, tf_bot_sniper_aim_error.GetFloat() );
				}

				Vector toThreat = subject->GetAbsOrigin() - me->GetAbsOrigin();
				float threatRange = toThreat.NormalizeInPlace();

				float s1, c1;
				FastSinCos( m_aimErrorRadius, &s1, &c1 );

				float error = threatRange * s1;

				Vector up( 0, 0, 1 );
				Vector side;
				CrossProduct( toThreat, up, side );

				float s, c;
				FastSinCos( m_aimErrorAngle, &s, &c );

				// aim a bit lower than the head - the imperfections may yet give us a headshot
				Vector desiredAimSpot;
				
				switch( me->GetDifficulty() )
				{
				case CTFBot::EXPERT:
				case CTFBot::HARD:
					// aim for the head - reaction times will differentiate the skill levels
					desiredAimSpot = subject->EyePosition();
					break;

				default:
					Assert(0);
				case CTFBot::NORMAL:
					desiredAimSpot = ( subject->EyePosition() + subject->EyePosition() + subject->WorldSpaceCenter() ) / 3.0f;
					break;

				case CTFBot::EASY:
					desiredAimSpot = subject->WorldSpaceCenter();
					break;
				}

				Vector imperfectAimSpot = desiredAimSpot + error * s * up + error * c * side;

				return imperfectAimSpot;
			}

			if ( myWeapon->IsWeapon( TF_WEAPON_GRENADELAUNCHER ) ||
				 myWeapon->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) )
			{
				Vector toThreat = subject->GetAbsOrigin() - me->GetAbsOrigin();
				float threatRange = toThreat.NormalizeInPlace();
				float elevationAngle = threatRange * tf_bot_ballistic_elevation_rate.GetFloat();

				if ( elevationAngle > 45.0f )
				{
					// ballistic range maximum at 45 degrees - aiming higher would decrease the range
					elevationAngle = 45.0f;
				}

				float s, c;
				FastSinCos( elevationAngle * M_PI / 180.0f, &s, &c );

				if ( c > 0.0f ) 
				{
					float elevation = threatRange * s / c;
					return subject->WorldSpaceCenter() + Vector( 0, 0, elevation );
				}
			}
		}
	}

	// aim for the center of the object (ie: sentry gun)
	return subject->WorldSpaceCenter();
}


//---------------------------------------------------------------------------------------------
/**
 * Allow bot to approve of positions game movement tries to put him into.
 * This is most useful for bots derived from CBasePlayer that go through
 * the player movement system.
 */
QueryResultType CTFBotMainAction::IsPositionAllowed( const INextBot *me, const Vector &pos ) const
{
	return ANSWER_YES;

	// This is causing bots to get hung up on drop-downs, particularly in MvM. MSB 6/11/2012
	/*
	if ( me->GetLocomotionInterface()->IsScrambling() )
	{
		// anything goes when we're in the air/etc
		return ANSWER_YES;
	}

	// if we are at a DROP_DOWN segment of our path, allow us to drop
	const PathFollower *path = me->GetCurrentPath();
	if ( path && path->IsValid() )
	{
		const Path::Segment *goal = path->GetCurrentGoal();
		if ( goal )
		{ 
			if ( goal->type == Path::DROP_DOWN || me->GetLocomotionInterface()->GetFeet().z - goal->pos.z >= me->GetLocomotionInterface()->GetMaxJumpHeight() )
			{
				// our goal requires us to drop down
				return ANSWER_YES;
			}
		}
	}

	// do not fall off someplace we can't get back up from!
	trace_t result;
	NextBotTraceFilterIgnoreActors filter( me->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT );
	ILocomotion *mover = me->GetLocomotionInterface();
	IBody *body = me->GetBodyInterface();

	// slightly smaller to allow skirting the edge
	float halfWidth = 0.4f * body->GetHullWidth();

	mover->TraceHull( pos + Vector( 0, 0, mover->GetStepHeight() ),	// start up a bit to handle rough terrain
					  pos + Vector( 0, 0, -mover->GetMaxJumpHeight() ), 
					  Vector( -halfWidth, -halfWidth, 0 ), 
					  Vector( halfWidth, halfWidth, body->GetHullHeight() ), 
					  body->GetSolidMask(), 
					  &filter, 
					  &result );

	if ( result.DidHit() )
	{
		// there is ground safe beneath us
		return ANSWER_YES;
	}

	return ANSWER_NO;
	*/
}


//---------------------------------------------------------------------------------------------
bool CTFBotMainAction::IsImmediateThreat( const CBaseCombatCharacter *subject, const CKnownEntity *threat ) const
{
	CTFBot *me = GetActor();

	// the TFBot code assumes the subject is always "me"
	if ( !me || !me->IsSelf( subject ) )
		return false;

	if ( me->InSameTeam( threat->GetEntity() ) )
		return false;

	if ( !threat->GetEntity()->IsAlive() )
		return false;

	if ( !threat->IsVisibleRecently() )
		return false;

	// if they can't hurt me, they aren't an immediate threat
	if ( !me->IsLineOfFireClear( threat->GetEntity() ) )
		return false;

	CTFPlayer *threatPlayer = ToTFPlayer( threat->GetEntity() );

	Vector to = me->GetAbsOrigin() - threat->GetLastKnownPosition();
	float threatRange = to.NormalizeInPlace();

	const float nearbyRange = 500.0f;
	if ( threatRange < nearbyRange )
	{
		// very near threats are always immediately dangerous
		return true;
	}
	
	// mid-to-far away threats

	if ( me->IsThreatFiringAtMe( threat->GetEntity() ) )
	{
		// distant threat firing on me - an immediate threat whether in my FOV or not
		return true;
	}

	if ( threatPlayer == NULL )
	{
		// non-player threat - sentry guns

		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( threat->GetEntity() );
		if ( sentry && !sentry->HasSapper() && !sentry->IsPlasmaDisabled() && !sentry->IsPlacing() )
		{
			// are we in range? (or will be very soon)
			if ( threatRange < 1.5f * SENTRY_MAX_RANGE )
			{
				// is it pointing at us?
				Vector sentryForward;
				AngleVectors( sentry->GetTurretAngles(), &sentryForward );

				if ( DotProduct( to, sentryForward ) > 0.8f )
				{
					return true;
				}
			}
		}

		return false;
	}

	// does a sniper have a shot on me?
	if ( threatPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		Vector sniperForward;
		threatPlayer->EyeVectors( &sniperForward );

		if ( DotProduct( to, sniperForward ) > 0.0f )
		{
			return true;
		}

		return false;
	}

	if ( me->GetDifficulty() > CTFBot::NORMAL && threatPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		// always try to kill these guys first
		return true;
	}

	if ( me->GetDifficulty() > CTFBot::NORMAL && threatPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		// take out engineers to let the team kill their sentry nests
		return true;
	}

	return false;
}


//---------------------------------------------------------------------------------------------
const CKnownEntity *CTFBotMainAction::SelectCloserThreat( CTFBot *me, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	float rangeSq1 = me->GetRangeSquaredTo( threat1->GetEntity() );
	float rangeSq2 = me->GetRangeSquaredTo( threat2->GetEntity() );

	if ( rangeSq1 < rangeSq2 )
		return threat1;

	return threat2;
}


//---------------------------------------------------------------------------------------------
// If the given threat is being healed by a Medic, return the Medic, otherwise just
// return the threat.
const CKnownEntity *CTFBotMainAction::GetHealerOfThreat( const CKnownEntity *threat ) const
{
	if ( !threat || !threat->GetEntity() )
		return NULL;

	CTFPlayer *playerThreat = ToTFPlayer( threat->GetEntity() );
	if ( playerThreat )
	{
		for( int i=0; i<playerThreat->m_Shared.GetNumHealers(); ++i )
		{
			CBaseEntity *healer = playerThreat->m_Shared.GetHealerByIndex( i );
			CTFPlayer *playerHealer = ToTFPlayer( healer );

			if ( playerHealer )
			{
				const CKnownEntity *knownHealer = GetActor()->GetVisionInterface()->GetKnown( playerHealer );

				if ( knownHealer && knownHealer->IsVisibleInFOVNow() )
				{
					return knownHealer;
				}
			}
		}
	}

	return threat;
}


//---------------------------------------------------------------------------------------------
// return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreat( const INextBot *meBot, 
																 const CBaseCombatCharacter *subject,
																 const CKnownEntity *threat1, 
																 const CKnownEntity *threat2 ) const
{
	CTFBot *me = ToTFBot( meBot->GetEntity() );

	// determine the actual threat
	const CKnownEntity *threat = SelectMoreDangerousThreatInternal( me, subject, threat1, threat2 );

	if ( me->IsDifficulty( CTFBot::EASY ) )
	{
		return threat;
	}

	if ( me->IsDifficulty( CTFBot::NORMAL ) && me->TransientlyConsistentRandomValue() < 0.5f )
	{
		return threat;
	}

 	// smarter bots first aim at the Medic healing our dangerous target
	return GetHealerOfThreat( threat );
}


//---------------------------------------------------------------------------------------------
// Given a pair of enemy players, return the closest Spy of those two, or NULL if neither is a Spy
const CKnownEntity *SelectClosestSpyToMe( CTFBot *me, const CKnownEntity *threat1, const CKnownEntity *threat2 )
{
	CTFPlayer *playerThreat1 = ToTFPlayer( threat1->GetEntity() );
	CTFPlayer *playerThreat2 = ToTFPlayer( threat2->GetEntity() );

	if ( playerThreat1 && playerThreat1->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( playerThreat2 && playerThreat2->IsPlayerClass( TF_CLASS_SPY ) )
		{
			if ( me->GetRangeSquaredTo( playerThreat1 ) < me->GetRangeSquaredTo( playerThreat2 ) )
				return threat1;

			return threat2;
		}

		return threat1;
	}
	else if ( playerThreat2 && playerThreat2->IsPlayerClass( TF_CLASS_SPY ) )
	{
		return threat2;
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
// Return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreatInternal( const INextBot *meBot, 
																		 const CBaseCombatCharacter *subject,
																		 const CKnownEntity *threat1, 
																		 const CKnownEntity *threat2 ) const
{
	CTFBot *me = ToTFBot( meBot->GetEntity() );
	const CKnownEntity *closerThreat = SelectCloserThreat( me, threat1, threat2 );

	if ( me->HasWeaponRestriction( CTFBot::MELEE_ONLY ) )
	{
		// melee only bots just use closest threat
		return closerThreat;
	}
	
	// close range sentries are the most dangerous of all
	bool shouldFearSentryGuns = true;

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// MvM bots are not afraid of sentry guns and treat them like other enemy players
		shouldFearSentryGuns = false;
	}

	if ( shouldFearSentryGuns )
	{
		CObjectSentrygun *sentry1 = NULL;
		if ( threat1->IsVisibleRecently() && !threat1->GetEntity()->IsPlayer() )
		{
			sentry1 = dynamic_cast< CObjectSentrygun * >( threat1->GetEntity() );
		}

		CObjectSentrygun *sentry2 = NULL;
		if ( threat2->IsVisibleRecently() && !threat2->GetEntity()->IsPlayer() )
		{
			sentry2 = dynamic_cast< CObjectSentrygun * >( threat2->GetEntity() );
		}

		if ( sentry1 && me->IsRangeLessThan( sentry1, SENTRY_MAX_RANGE ) && !sentry1->HasSapper() && !sentry1->IsPlasmaDisabled() && !sentry1->IsPlacing() )
		{
			// in range of a visible sentry!
			if ( sentry2 && me->IsRangeLessThan( sentry2, SENTRY_MAX_RANGE ) && !sentry2->HasSapper() && !sentry2->IsPlasmaDisabled() && !sentry2->IsPlacing() )
			{
				// in range of two visible sentries!  we're probably dead meat at this point.
				// default is choose closest
				return closerThreat;
			}

			return threat1;
		}

		if ( sentry2 && me->IsRangeLessThan( sentry2, SENTRY_MAX_RANGE ) && !sentry2->HasSapper() && !sentry2->IsPlasmaDisabled() && !sentry2->IsPlacing() )
		{
			// in range of a visible sentry!
			return threat2;
		}
	}

	// enforce Spy hatred in MvM mode
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		const float spyHateRadius = 1000.0f;

		const CKnownEntity *spyThreat = SelectClosestSpyToMe( me, threat1, threat2 );
		if ( spyThreat && me->IsRangeLessThan( spyThreat->GetEntity(), spyHateRadius ) )
		{
			return spyThreat;
		}
	}


	bool isImmediateThreat1 = IsImmediateThreat( subject, threat1 );
	bool isImmediateThreat2 = IsImmediateThreat( subject, threat2 );

	if ( isImmediateThreat1 && !isImmediateThreat2 )
	{
		return threat1;
	}
	else if ( !isImmediateThreat1 && isImmediateThreat2 )
	{
		return threat2;
	}
	else if ( !isImmediateThreat1 && !isImmediateThreat2 )
	{
		// neither threat is immediately dangerous - use closest
		return closerThreat;
	}

	// both threats are immediately dangerous!
	// check if any are extremely dangerous

	const CKnownEntity *spyThreat = SelectClosestSpyToMe( me, threat1, threat2 );
	if ( spyThreat )
	{
		return spyThreat;
	}

	// choose most recent attacker (assume an enemy firing their weapon at us has attacked us)
	if ( me->IsThreatFiringAtMe( threat1->GetEntity() ) )
	{
		if ( me->IsThreatFiringAtMe( threat2->GetEntity() ) )
		{
			// choose closest
			return closerThreat;
		}

		return threat1;
	}
	else if ( me->IsThreatFiringAtMe( threat2->GetEntity() ) )
	{
		return threat2;
	}

	// choose closest
	return closerThreat;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotMainAction::ShouldAttack( const INextBot *meBot, const CKnownEntity *them ) const
{
	if ( g_pPopulationManager )
	{
		// if I'm in my spawn room, obey the population manager's attack restrictions
		CTFBot *me = ToTFBot( meBot->GetEntity() );
		CTFNavArea *myArea = me->GetLastKnownArea();
		int spawnRoomFlag = me->GetTeamNumber() == TF_TEAM_RED ? TF_NAV_SPAWN_ROOM_RED : TF_NAV_SPAWN_ROOM_BLUE;

		if ( myArea && myArea->HasAttributeTF( spawnRoomFlag ) )
		{
			return g_pPopulationManager->CanBotsAttackWhileInSpawnRoom() ? ANSWER_YES : ANSWER_NO;
		}
	}

	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotMainAction::ShouldHurry( const INextBot *meBot ) const
{
	if ( g_pPopulationManager )
	{
		// if I'm in my spawn room, obey the population manager's attack restrictions
		CTFBot *me = ToTFBot( meBot->GetEntity() );
		CTFNavArea *myArea = me->GetLastKnownArea();
		int spawnRoomFlag = me->GetTeamNumber() == TF_TEAM_RED ? TF_NAV_SPAWN_ROOM_RED : TF_NAV_SPAWN_ROOM_BLUE;

		if ( myArea && myArea->HasAttributeTF( spawnRoomFlag ) )
		{
			if ( !g_pPopulationManager->CanBotsAttackWhileInSpawnRoom() )
			{
				// hurry to leave the spawn
				return ANSWER_YES;
			}
		}
	}

	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
void CTFBotMainAction::FireWeaponAtEnemy( CTFBot *me )
{
	if ( !me->IsAlive() )
		return;

	if ( me->HasAttribute( CTFBot::SUPPRESS_FIRE ) )
		return;

	if ( me->HasAttribute( CTFBot::IGNORE_ENEMIES ) )
		return;

	if ( me->m_Shared.InCond( TF_COND_TAUNTING ) )
		return;

	if ( !tf_bot_fire_weapon_allowed.GetBool() )
	{
		return;
	}

	CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
	if ( !myWeapon )
		return;

	if ( me->IsBarrageAndReloadWeapon( myWeapon ) )
	{
		if ( me->HasAttribute( CTFBot::HOLD_FIRE_UNTIL_FULL_RELOAD ) || tf_bot_always_full_reload.GetBool() )
		{
			if ( myWeapon->Clip1() <= 0 )
			{
				m_isWaitingForFullReload = true;
			}

			if ( m_isWaitingForFullReload )
			{
				if ( myWeapon->Clip1() < myWeapon->GetMaxClip1() )
				{
					return;
				}

				// we are fully reloaded
				m_isWaitingForFullReload = false;
			}
		}
	}

	if ( me->HasAttribute( CTFBot::ALWAYS_FIRE_WEAPON ) )
	{
		me->PressFireButton();
		return;
	}

	if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		if ( myWeapon && myWeapon->IsWeapon( TF_WEAPON_MEDIGUN ) )
		{
			// don't interfere with medic healing behaviors
			return;
		}
	}

	// if we're a heavy and just saw a bad guy, keep the barrel spinning (unless we're in a hurry)
	if ( me->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) && !me->IsAmmoLow() && me->GetIntentionInterface()->ShouldHurry( me ) != ANSWER_YES )
	{
		const float spinTime = 3.0f;
		if ( me->GetVisionInterface()->GetTimeSinceVisible( GetEnemyTeam( me->GetTeamNumber() ) ) < spinTime )
		{
			me->PressAltFireButton();
		}
	}

	// shoot at bad guys
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	// ignore non-visible threats here so we don't force a premature weapon switch if we're doing something else
	if ( threat == NULL || !threat->GetEntity() || !threat->IsVisibleRecently() )
		return;

	// don't shoot through windows/etc
	if ( !me->IsLineOfFireClear( threat->GetEntity()->EyePosition() ) )
	{
		if ( !me->IsLineOfFireClear( threat->GetEntity()->WorldSpaceCenter() ) )
		{
			if ( !me->IsLineOfFireClear( threat->GetEntity()->GetAbsOrigin() ) )
				return;
		}
	}

	// if our target is uber'd, most weapons are useless - unless we're in MvM, where invuln tanking is valuable
	if ( TFGameRules() && !TFGameRules()->IsMannVsMachineMode() )
	{
		CTFPlayer *playerThreat = ToTFPlayer( threat->GetEntity() );
		if ( playerThreat && playerThreat->m_Shared.IsInvulnerable() )
		{
			if ( !myWeapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER ) &&
				!myWeapon->IsWeapon( TF_WEAPON_GRENADELAUNCHER ) &&
				!myWeapon->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) &
				!myWeapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT ) )
			{
				// firing would just waste ammo, so don't
				return;
			}
		}
	}

	if ( me->GetIntentionInterface()->ShouldAttack( me, threat ) == ANSWER_NO )
		return;

	if ( TFGameRules()->InSetup() )
	{
		// wait until the gates open
		return;
	}

	if ( myWeapon->IsMeleeWeapon() )
	{
		if ( me->IsRangeLessThan( threat->GetEntity(), 250.0f ) )
		{
			me->PressFireButton();
		}
		return;
	}

	// limit range of hitscan weapon fire in MvM
	if ( TFGameRules()->IsMannVsMachineMode() && !me->IsPlayerClass( TF_CLASS_SNIPER ) && me->IsHitScanWeapon( myWeapon ) )
	{
		if ( me->IsRangeGreaterThan( threat->GetEntity(), tf_bot_hitscan_range_limit.GetFloat() ) )
		{
			return;
		}
	}

	if ( myWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )
	{
		CTFFlameThrower *pFlamethrower = assert_cast< CTFFlameThrower* >( myWeapon );
		// watch for enemy projectiles heading our way
		if ( pFlamethrower->CanAirBlast() && me->ShouldFireCompressionBlast() )
		{
			// bounce missiles with compression blast
			me->PressAltFireButton();
		}
		else if ( threat->GetTimeSinceLastSeen() < 1.0f && 
				  me->IsDistanceBetweenLessThan( threat->GetEntity(), me->GetMaxAttackRange() ) )
		{
			me->PressFireButton( tf_bot_fire_weapon_min_time.GetFloat() );
		}

		return;
	}

	float threatRange = ( threat->GetEntity()->GetAbsOrigin() - me->GetAbsOrigin() ).Length();

	// actual head aiming is handled elsewhere, just check if we're on target
	if ( me->GetBodyInterface()->IsHeadAimingOnTarget() && threatRange < me->GetMaxAttackRange() )
	{
		if ( myWeapon->IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
		{
			CTFCompoundBow *myBow = (CTFCompoundBow *)myWeapon;

			if ( myBow->GetCurrentCharge() < 0.95f || !me->IsLineOfFireClear( threat->GetEntity() ) )
			{
				me->PressFireButton();
			}
		}
		else if ( WeaponID_IsSniperRifle( myWeapon->GetWeaponID() ) )
		{
			// only fire if zoomed in
			if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
			{
				const float reactionTime = TFGameRules()->IsMannVsMachineMode() ? 0.5f : 0.1f;	// just a moment to stop headshots when obviously panning too fast to see
				if ( m_steadyTimer.HasStarted() && m_steadyTimer.IsGreaterThen( reactionTime ) )
				{
					trace_t trace;
					
					Vector forward;
					me->EyeVectors( &forward );

					// allow bot to see through projectile shield
					CTraceFilterIgnoreFriendlyCombatItems filter( me, COLLISION_GROUP_NONE, me->GetTeamNumber() );
					UTIL_TraceLine( me->EyePosition(), me->EyePosition() + 9000.0f * forward, MASK_SHOT, &filter, &trace );

					if ( trace.m_pEnt == threat->GetEntity() )
					{
						// we're on target - fire!
						me->PressFireButton();
					}
				}
			}
		}
		else if ( me->IsCombatWeapon( MY_CURRENT_GUN ) )
		{
			if ( me->IsContinuousFireWeapon( MY_CURRENT_GUN ) )
			{
				// spray for a bit
				me->PressFireButton( tf_bot_fire_weapon_min_time.GetFloat() );
			}
			else 
			{
				if ( me->IsExplosiveProjectileWeapon( MY_CURRENT_GUN ) )
				{
					// don't fire if we're going to hit a nearby wall
					trace_t trace;

					Vector forward;
					me->EyeVectors( &forward );

					// allow bot to see through projectile shield
					CTraceFilterIgnoreFriendlyCombatItems filter( me, COLLISION_GROUP_NONE, me->GetTeamNumber() );
					UTIL_TraceLine( me->EyePosition(), me->EyePosition() + 1.1f * threatRange * forward, MASK_SHOT, &filter, &trace );

					float hitRange = trace.fraction * 1.1f * threatRange;

					if ( hitRange < TF_ROCKET_RADIUS )
					{
						// shot will impact very near us
						if ( !trace.m_pEnt || ( trace.m_pEnt && !trace.m_pEnt->MyCombatCharacterPointer() ) )
						{
							// don't fire, we'd only hit the world or a non-player or non-sentry
							return;
						}
					}
				}

				me->PressFireButton();
			}
		}
	
	}
}


//---------------------------------------------------------------------------------------------
/**
 * Compute nearby friends influence and visible enemy influence
 */
class CCompareFriendFoeInfluence : public IVision::IForEachKnownEntity
{
public:
	CCompareFriendFoeInfluence( CTFBot *me )
	{
		m_me = me;
		m_friendScore = 0;
		m_foeScore = 0;
	}

	virtual bool Inspect( const CKnownEntity &known )
	{
		if ( known.GetEntity()->IsAlive() )
		{
			const float nearRange = 750.0f;
			if ( m_me->IsRangeLessThan( known.GetEntity(), nearRange ) )
			{
				if ( m_me->IsFriend( known.GetEntity() ) )
				{
					m_friendScore += m_me->GetThreatDanger( known.GetEntity()->MyCombatCharacterPointer() );
				}
				else if ( known.WasEverVisible() && known.GetTimeSinceLastSeen() < 3.0f && m_me->IsEnemy( known.GetEntity() ) )
				{
					// ignore disguised spies, etc
					if ( m_me->GetVisionInterface()->IsIgnored( known.GetEntity() ) )
						return true;

					// only count them if they are facing me
					if ( UTIL_IsFacingWithinTolerance( known.GetEntity(), m_me->EyePosition(), 0.5f ) )
					{
						m_foeScore += m_me->GetThreatDanger( known.GetEntity()->MyCombatCharacterPointer() );
					}
				}
			}
		}

		return true;
	}

	CTFBot *m_me;
	float m_friendScore;
	float m_foeScore;
};


//---------------------------------------------------------------------------------------------
/**
 * If we're outnumbered, retreat and wait for backup - unless we're ubered!
 */
QueryResultType	CTFBotMainAction::ShouldRetreat( const INextBot *bot ) const
{
	CTFBot *me = (CTFBot *)bot->GetEntity();

	// don't retreat if we're in "melee only" mode
	if ( TheTFBots().IsMeleeOnly() )
		return ANSWER_NO;

	// don't retreat if ubered
	if ( me->m_Shared.IsInvulnerable() )
		return ANSWER_NO;

	// don't retreat if we're ignoring enemies
	if ( me->HasAttribute( CTFBot::IGNORE_ENEMIES ) )
		return ANSWER_NO;

	// retreat if stunned
	if ( me->m_Shared.IsControlStunned() || me->m_Shared.IsLoserStateStunned() )
		return ANSWER_YES;

	// don't retreat during setup time, since we're always safe
	if ( TFGameRules()->InSetup() )
		return ANSWER_NO;

	// if we're an undercover spy, don't blow our cover
	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( me->m_Shared.InCond( TF_COND_DISGUISED ) ||
			 me->m_Shared.InCond( TF_COND_DISGUISING ) ||
			 me->m_Shared.IsStealthed() )
		{
			return ANSWER_NO;
		}
	}

	CCompareFriendFoeInfluence compare( me );
	me->GetVisionInterface()->ForEachKnownEntity( compare );

	if ( compare.m_friendScore < compare.m_foeScore )
	{
		return ANSWER_YES;
	}

	return ANSWER_NO;
}


//-----------------------------------------------------------------------------------------
void CTFBotMainAction::Dodge( CTFBot *me )
{
	// low-skill bots don't dodge
	if ( me->IsDifficulty( CTFBot::EASY ) )
		return;

	// no need to dodge if we're invulnerable
	if ( me->m_Shared.IsInvulnerable() )
		return;

	// don't dodge if we're trying to snipe
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
		return;

	// don't dodge if we are taunting
	if ( me->m_Shared.InCond( TF_COND_TAUNTING ) )
		return;

	// don't dodge if that ability is "turned off"
	if ( me->HasAttribute( CTFBot::DISABLE_DODGE ) )
		return;

	// don't dodge if we're not trying to fight back
	if ( !me->IsCombatWeapon( MY_CURRENT_GUN ) )
		return;

	// don't waste time doding if we're in a hurry
	if ( me->GetIntentionInterface()->ShouldHurry( me ) == ANSWER_YES )
		return;

	// for now, engies don't dodge
	if ( me->IsPlayerClass( TF_CLASS_ENGINEER ) )
		return;

	// disguised/cloaked spies don't dodge
	if ( me->m_Shared.InCond( TF_COND_DISGUISED ) ||
		 me->m_Shared.InCond( TF_COND_DISGUISING ) ||
		 me->m_Shared.IsStealthed() )
	{
		return;
	}


#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
		return;
#endif // TF_RAID_MODE

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		bool isShotClear = true;

		CTFWeaponBase *myGun = (CTFWeaponBase *)me->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
		if ( myGun && myGun->IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
		{
			CTFCompoundBow *myBow = (CTFCompoundBow *)myGun;
			if ( myBow->GetCurrentCharge() > 0.0f )
			{
				// we're drawing back our bow - hold still
				return;
			}

			// if we don't have a clear shot, dodge around until we do
			isShotClear = true;
		}
		else
		{
			isShotClear = me->IsLineOfFireClear( threat->GetLastKnownPosition() );
		}

		// don't dodge if they can't hit us
		if ( !isShotClear )
			return;

		Vector forward;
		me->EyeVectors( &forward );
		Vector left( -forward.y, forward.x, 0.0f );
		left.NormalizeInPlace();

		const float sideStepSize = 25.0f;

		int rnd = RandomInt( 0, 100 );
		if ( rnd < 33 )
		{
			if ( !me->GetLocomotionInterface()->HasPotentialGap( me->GetAbsOrigin(), me->GetAbsOrigin() + sideStepSize * left ) )
			{
				me->PressLeftButton();
			}
		}
		else if ( rnd > 66 )
		{
			if ( !me->GetLocomotionInterface()->HasPotentialGap( me->GetAbsOrigin(), me->GetAbsOrigin() - sideStepSize * left ) )
			{
				me->PressRightButton();
			}
		}
	}
}

