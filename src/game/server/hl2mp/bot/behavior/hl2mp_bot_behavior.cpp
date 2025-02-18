//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "fmtstr.h"

#include "nav_mesh.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "hl2mp/weapon_rpg.h"
#include "bot/hl2mp_bot.h"
#include "bot/hl2mp_bot_manager.h"
#include "bot/behavior/hl2mp_bot_behavior.h"
#include "bot/behavior/hl2mp_bot_dead.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_wait.h"
#include "bot/behavior/hl2mp_bot_tactical_monitor.h"


extern ConVar hl2mp_bot_health_ok_ratio;

ConVar hl2mp_bot_path_lookahead_range( "hl2mp_bot_path_lookahead_range", "300" );
ConVar hl2mp_bot_sniper_aim_error( "hl2mp_bot_sniper_aim_error", "0.01", FCVAR_CHEAT );
ConVar hl2mp_bot_sniper_aim_steady_rate( "hl2mp_bot_sniper_aim_steady_rate", "10", FCVAR_CHEAT );
ConVar hl2mp_bot_debug_sniper( "hl2mp_bot_debug_sniper", "0", FCVAR_CHEAT );
ConVar hl2mp_bot_fire_weapon_min_time( "hl2mp_bot_fire_weapon_min_time", "1", FCVAR_CHEAT );
ConVar hl2mp_bot_taunt_victim_chance( "hl2mp_bot_taunt_victim_chance", "20" );		// community requested this not be a cheat cvar

ConVar hl2mp_bot_notice_backstab_chance( "hl2mp_bot_notice_backstab_chance", "25", FCVAR_CHEAT );
ConVar hl2mp_bot_notice_backstab_min_range( "hl2mp_bot_notice_backstab_min_range", "100", FCVAR_CHEAT );
ConVar hl2mp_bot_notice_backstab_max_range( "hl2mp_bot_notice_backstab_max_range", "750", FCVAR_CHEAT );

ConVar hl2mp_bot_ballistic_elevation_rate( "hl2mp_bot_ballistic_elevation_rate", "0.01", FCVAR_CHEAT, "When lobbing grenades at far away targets, this is the degree/range slope to raise our aim" );

ConVar hl2mp_bot_hitscan_range_limit( "hl2mp_bot_hitscan_range_limit", "1800", FCVAR_CHEAT );

ConVar hl2mp_bot_always_full_reload( "hl2mp_bot_always_full_reload", "0", FCVAR_CHEAT );

ConVar hl2mp_bot_fire_weapon_allowed( "hl2mp_bot_fire_weapon_allowed", "1", FCVAR_CHEAT, "If zero, HL2MPBots will not pull the trigger of their weapons (but will act like they did)" );

ConVar hl2mp_bot_allow_retreat( "hl2mp_bot_allow_retreat", "1", FCVAR_CHEAT, "If zero, bots will not attempt to retreat if they are are in a bad situation." );
ConVar hl2mp_bot_physcannon_wait_fire_time( "hl2mp_bot_physcannon_wait_fire_time", "1", FCVAR_CHEAT, "Time to wait after picking up a prop to firing." );

//---------------------------------------------------------------------------------------------
Action< CHL2MPBot > *CHL2MPBotMainAction::InitialContainedAction( CHL2MPBot *me )
{
	return new CHL2MPBotTacticalMonitor;
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotMainAction::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	m_lastTouch = NULL;
	m_lastTouchTime = 0.0f;
	m_aimErrorRadius = 0.0f;
	m_aimErrorAngle = 0.0f;

	m_yawRate = 0.0f;
	m_priorYaw = 0.0f;

	m_isWaitingForFullReload = false;

	// if bot is already dead at this point, make sure it's dead
	// check for !IsAlive because bot could be DYING
	if ( !me->IsAlive() )
	{
		return ChangeTo( new CHL2MPBotDead, "I'm actually dead" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotMainAction::Update( CHL2MPBot *me, float interval )
{
	VPROF_BUDGET( "CHL2MPBotMainAction::Update", "NextBot" );

	// TEAM_UNASSIGNED -> deathmatch
	if ( me->GetTeamNumber() != TEAM_COMBINE && me->GetTeamNumber() != TEAM_REBELS && me->GetTeamNumber() != TEAM_UNASSIGNED )
	{
		// not on a team - do nothing
		return Done( "Not on a playing team" );
	}

	// make sure our vision FOV matches the player's
	me->GetVisionInterface()->SetFieldOfView( me->GetFOV() );

	// track aim velocity ourselves, since body aim "steady" is too loose
	float deltaYaw = me->EyeAngles().y - m_priorYaw;
	m_yawRate = fabs( deltaYaw / ( interval + 0.0001f ) );
	m_priorYaw = me->EyeAngles().y;

	if ( m_yawRate < hl2mp_bot_sniper_aim_steady_rate.GetFloat() )
	{
		if ( !m_steadyTimer.HasStarted() )
			m_steadyTimer.Start();

// 		if ( hl2mp_bot_debug_sniper.GetBool() )
// 		{
// 			DevMsg( "%3.2f: STEADY\n", gpGlobals->curtime );
// 		}
	}
	else
	{
		m_steadyTimer.Invalidate();

// 		if ( hl2mp_bot_debug_sniper.GetBool() )
// 		{
// 			DevMsg( "%3.2f: Yaw rate = %3.2f\n", gpGlobals->curtime, m_yawRate );
// 		}
	}

	me->EquipRequiredWeapon();

	me->UpdateLookingAroundForEnemies();
	FireWeaponAtEnemy( me );
	Dodge( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult<CHL2MPBot> CHL2MPBotMainAction::OnKilled( CHL2MPBot *me, const CTakeDamageInfo& info )
{
	return TryChangeTo( new CHL2MPBotDead, RESULT_CRITICAL, "I died!" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotMainAction::OnInjured( CHL2MPBot *me, const CTakeDamageInfo &info )
{
	// if an object hurt me, it must be a sentry
	CBaseEntity *subject = info.GetAttacker();

	// notice the gunfire - needed for sentry guns, which don't go through the player OnWeaponFired() system
	me->GetVisionInterface()->AddKnownEntity( subject );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotMainAction::OnContact( CHL2MPBot *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other && !other->IsSolidFlagSet( FSOLID_NOT_SOLID ) && !other->IsWorld() && !other->IsPlayer() )
	{
		m_lastTouch = other;
		m_lastTouchTime = gpGlobals->curtime;
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotMainAction::OnStuck( CHL2MPBot *me )
{
	UTIL_LogPrintf( "\"%s<%i><%s>\" stuck (position \"%3.2f %3.2f %3.2f\") (duration \"%3.2f\") ",
					me->GetPlayerName(),
					me->GetUserID(),
					me->GetNetworkIDString(),
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

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotMainAction::OnOtherKilled( CHL2MPBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	// make sure we forget about this guy
	me->GetVisionInterface()->ForgetEntity( victim );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
/**
 * Given a subject, return the world space position we should aim at
 */
Vector CHL2MPBotMainAction::SelectTargetPoint( const INextBot *meBot, const CBaseCombatCharacter *subject ) const
{
	CHL2MPBot *me = (CHL2MPBot *)meBot->GetEntity();

	if ( subject )
	{
		CBaseHL2MPCombatWeapon *myWeapon = (CBaseHL2MPCombatWeapon *)me->GetActiveWeapon();
		if ( myWeapon )
		{
			// lead our target and aim for the feet with the rocket launcher
			if ( !me->IsDifficulty( CHL2MPBot::EASY ) )
			{
				if ( myWeapon->ClassMatches( "weapon_rpg" ) )
				{
					// TODO: Write some code that makes them look at the threat directly when the missile is out, rather than this logic.

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
					const float missileSpeed = 1500.0f;
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
				else if ( myWeapon->ClassMatches( "weapon_physcannon" ) )
				{
					CBaseEntity *pHeldEntity = me->Physcannon_GetHeldProp();
					if ( pHeldEntity )
					{
						float rangeBetween = me->GetRangeTo( subject->WorldSpaceCenter() );

						float flForceMax = physcannon_maxforce.GetFloat();
						float flForce = flForceMax;

						float mass = pHeldEntity->VPhysicsGetObject()->GetMass();
						if ( mass > 100 )
						{
							mass = MIN( mass, 1000 );
							float flForceMin = physcannon_minforce.GetFloat();
							flForce = SimpleSplineRemapVal( mass, 100, 600, flForceMax, flForceMin );
						}

						const float veryCloseRange = 150.0f;
						if ( rangeBetween > veryCloseRange )
						{
							float timeToTravel = rangeBetween / flForce;

							Vector targetPos = subject->WorldSpaceCenter() + timeToTravel * subject->GetAbsVelocity();

							if ( me->GetVisionInterface()->IsAbleToSee( targetPos, IVision::DISREGARD_FOV ) )
								return targetPos;

							// try their head and hope
							return subject->WorldSpaceCenter() + timeToTravel * subject->GetAbsVelocity();
						}
					}
				}
			}
			if ( myWeapon->ClassMatches( "weapon_frag" ) )
			{
				Vector toThreat = subject->GetAbsOrigin() - me->GetAbsOrigin();
				float threatRange = toThreat.NormalizeInPlace();
				float elevationAngle = threatRange * hl2mp_bot_ballistic_elevation_rate.GetFloat();

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

		// aim for the center of the object (ie: sentry gun)
		return subject->WorldSpaceCenter();
	}

	return vec3_origin;
}


//---------------------------------------------------------------------------------------------
/**
 * Allow bot to approve of positions game movement tries to put him into.
 * This is most useful for bots derived from CBasePlayer that go through
 * the player movement system.
 */
QueryResultType CHL2MPBotMainAction::IsPositionAllowed( const INextBot *me, const Vector &pos ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
bool CHL2MPBotMainAction::IsImmediateThreat( const CBaseCombatCharacter *subject, const CKnownEntity *threat ) const
{
	CHL2MPBot *me = GetActor();

	// the HL2MPBot code assumes the subject is always "me"
	if ( !me || !me->IsSelf( subject ) )
		return false;

	if ( !me->IsEnemy( threat->GetEntity() ) )
		return false;

	if ( !threat->GetEntity()->IsAlive() )
		return false;

	if ( !threat->IsVisibleRecently() )
		return false;

	// if they can't hurt me, they aren't an immediate threat
	if ( !me->IsLineOfFireClear( threat->GetEntity() ) )
		return false;

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

	return false;
}


//---------------------------------------------------------------------------------------------
const CKnownEntity *CHL2MPBotMainAction::SelectCloserThreat( CHL2MPBot *me, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
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
const CKnownEntity *CHL2MPBotMainAction::GetHealerOfThreat( const CKnownEntity *threat ) const
{
	if ( !threat || !threat->GetEntity() )
		return NULL;

	return threat;
}


//---------------------------------------------------------------------------------------------
// return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
const CKnownEntity *CHL2MPBotMainAction::SelectMoreDangerousThreat( const INextBot *meBot, 
																 const CBaseCombatCharacter *subject,
																 const CKnownEntity *threat1, 
																 const CKnownEntity *threat2 ) const
{
	CHL2MPBot *me = ToHL2MPBot( meBot->GetEntity() );

	// determine the actual threat
	const CKnownEntity *threat = SelectMoreDangerousThreatInternal( me, subject, threat1, threat2 );

	if ( me->IsDifficulty( CHL2MPBot::EASY ) )
	{
		return threat;
	}

	if ( me->IsDifficulty( CHL2MPBot::NORMAL ) && me->TransientlyConsistentRandomValue() < 0.5f )
	{
		return threat;
	}

 	// smarter bots first aim at the Medic healing our dangerous target
	return GetHealerOfThreat( threat );
}


//---------------------------------------------------------------------------------------------
// Return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
const CKnownEntity *CHL2MPBotMainAction::SelectMoreDangerousThreatInternal( const INextBot *meBot, 
																		 const CBaseCombatCharacter *subject,
																		 const CKnownEntity *threat1, 
																		 const CKnownEntity *threat2 ) const
{
	CHL2MPBot *me = ToHL2MPBot( meBot->GetEntity() );
	const CKnownEntity *closerThreat = SelectCloserThreat( me, threat1, threat2 );

	if ( me->HasWeaponRestriction( CHL2MPBot::MELEE_ONLY ) )
	{
		// melee only bots just use closest threat
		return closerThreat;
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
QueryResultType CHL2MPBotMainAction::ShouldAttack( const INextBot *meBot, const CKnownEntity *them ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType	CHL2MPBotMainAction::ShouldHurry( const INextBot *meBot ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
void CHL2MPBotMainAction::FireWeaponAtEnemy( CHL2MPBot *me )
{
	if ( !me->IsAlive() )
		return;

	if ( me->HasAttribute( CHL2MPBot::SUPPRESS_FIRE ) )
		return;

	if ( me->HasAttribute( CHL2MPBot::IGNORE_ENEMIES ) )
		return;

	if ( !hl2mp_bot_fire_weapon_allowed.GetBool() )
	{
		return;
	}

	CBaseHL2MPCombatWeapon* myWeapon = dynamic_cast< CBaseHL2MPCombatWeapon *>( me->GetActiveWeapon() );
	if ( !myWeapon )
		return;

	if ( me->IsBarrageAndReloadWeapon( myWeapon ) )
	{
		if ( me->HasAttribute( CHL2MPBot::HOLD_FIRE_UNTIL_FULL_RELOAD ) || hl2mp_bot_always_full_reload.GetBool() )
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

	if ( me->HasAttribute( CHL2MPBot::ALWAYS_FIRE_WEAPON ) )
	{
		me->PressFireButton();
		return;
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

	if ( me->GetIntentionInterface()->ShouldAttack( me, threat ) == ANSWER_NO )
		return;

	if ( me->IsBludgeon( myWeapon ) )
	{
		if ( me->IsRangeLessThan( threat->GetEntity(), 250.0f ) )
		{
			me->PressFireButton();
		}
		return;
	}

	float threatRange = ( threat->GetEntity()->GetAbsOrigin() - me->GetAbsOrigin() ).Length();

	// actual head aiming is handled elsewhere, just check if we're on target
	//
	// misyl: make sure we are actually looking at the target...
	// tf2 doesn't do this check... i think its right to do this here...
	if ( me->GetBodyInterface()->GetLookAtSubject() == threat->GetEntity() &&
		 me->GetBodyInterface()->IsHeadAimingOnTarget() &&
		 threatRange < me->GetMaxAttackRange() )
	{
		if ( me->IsCombatWeapon( MY_CURRENT_GUN ) )
		{
			if ( me->Physcannon_GetHeldProp() != NULL )
			{
				// misyl: Wait a second so the prop can move and we can look at the right entity...
				if ( gpGlobals->curtime > me->GetPhyscannonPickupTime() + hl2mp_bot_physcannon_wait_fire_time.GetFloat() )
				{
					me->PressFireButton();
				}
			}
			else if ( me->IsContinuousFireWeapon( MY_CURRENT_GUN ) )
			{
				// spray for a bit
				me->PressFireButton( hl2mp_bot_fire_weapon_min_time.GetFloat() );
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

					const float k_flRPGRange = 200.0f;
					if ( hitRange < k_flRPGRange )
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
 * If we're outnumbered, retreat and wait for backup - unless we're ubered!
 */
QueryResultType	CHL2MPBotMainAction::ShouldRetreat( const INextBot *bot ) const
{
	CHL2MPBot *me = (CHL2MPBot *)bot->GetEntity();

	if ( !hl2mp_bot_allow_retreat.GetBool() )
		return ANSWER_NO;

	// don't retreat if we're in "melee only" mode
	if ( TheHL2MPBots().IsMeleeOnly() )
		return ANSWER_NO;

	// don't retreat if we're in "grav gun only" mode
	if ( TheHL2MPBots().IsGravGunOnly() )
		return ANSWER_NO;

	// If we are currently trying to melee, don't retreat. Chaaaarge!
	if ( me->IsBludgeon( me->GetActiveWeapon() ) )
		return ANSWER_NO;

	// Don't retreat if we have a prop.
	if ( me->Physcannon_GetHeldProp() != NULL )
		return ANSWER_NO;

	// don't retreat if we're ignoring enemies
	if ( me->HasAttribute( CHL2MPBot::IGNORE_ENEMIES ) )
		return ANSWER_NO;

	return ANSWER_NO;
}


//-----------------------------------------------------------------------------------------
void CHL2MPBotMainAction::Dodge( CHL2MPBot *me )
{
	// low-skill bots don't dodge
	if ( me->IsDifficulty( CHL2MPBot::EASY ) )
		return;

	// don't dodge if that ability is "turned off"
	if ( me->HasAttribute( CHL2MPBot::DISABLE_DODGE ) )
		return;

	// don't dodge if we're not trying to fight back
	if ( !me->IsCombatWeapon( MY_CURRENT_GUN ) )
		return;

	// don't waste time doding if we're in a hurry
	if ( me->GetIntentionInterface()->ShouldHurry( me ) == ANSWER_YES )
		return;

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		bool isShotClear = me->IsLineOfFireClear( threat->GetLastKnownPosition() );

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

