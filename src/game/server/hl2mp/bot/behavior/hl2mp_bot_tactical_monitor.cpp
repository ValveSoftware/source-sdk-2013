//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "fmtstr.h"

#include "hl2mp_gamerules.h"
#include "hl2mp/weapon_slam.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

#include "bot/hl2mp_bot.h"
#include "bot/hl2mp_bot_manager.h"

#include "bot/behavior/hl2mp_bot_tactical_monitor.h"
#include "bot/behavior/hl2mp_bot_scenario_monitor.h"

#include "bot/behavior/hl2mp_bot_seek_and_destroy.h"
#include "bot/behavior/hl2mp_bot_retreat_to_cover.h"
#include "bot/behavior/hl2mp_bot_get_health.h"
#include "bot/behavior/hl2mp_bot_get_ammo.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_wait.h"

extern ConVar hl2mp_bot_health_ok_ratio;
extern ConVar hl2mp_bot_health_critical_ratio;

ConVar hl2mp_bot_force_jump( "hl2mp_bot_force_jump", "0", FCVAR_CHEAT, "Force bots to continuously jump" );

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Attempts to kick/despawn the bot in the Update()

class CHL2MPDespawn : public Action< CHL2MPBot >
{
public:
	virtual ActionResult< CHL2MPBot >	Update( CHL2MPBot* me, float interval );
	virtual const char* GetName( void ) const { return "Despawn"; };
};


ActionResult< CHL2MPBot > CHL2MPDespawn::Update( CHL2MPBot* me, float interval )
{
	// players need to be kicked, not deleted
	if ( me->GetEntity()->IsPlayer() )
	{
		CBasePlayer* player = dynamic_cast< CBasePlayer* >( me->GetEntity() );
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", player->GetUserID() ) );
	}
	else
	{
		UTIL_Remove( me->GetEntity() );
	}
	return Continue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Action< CHL2MPBot > *CHL2MPBotTacticalMonitor::InitialContainedAction( CHL2MPBot *me )
{
	return new CHL2MPBotScenarioMonitor;
}


//-----------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotTacticalMonitor::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	return Continue();
}


//-----------------------------------------------------------------------------------------
void CHL2MPBotTacticalMonitor::MonitorArmedStickyBombs( CHL2MPBot *me )
{
	if ( m_stickyBombCheckTimer.IsElapsed() )
	{
		m_stickyBombCheckTimer.Start( RandomFloat( 0.3f, 1.0f ) );

		// are there any enemies on/near my sticky bombs?
		CWeapon_SLAM *slam = dynamic_cast< CWeapon_SLAM* >( me->Weapon_OwnsThisType( "weapon_slam" ) );
		if ( slam )
		{
			const CUtlVector< CBaseEntity* > &satchelVector = slam->GetSatchelVector();

			if ( satchelVector.Count() > 0 )
			{
				CUtlVector< CKnownEntity > knownVector;
				me->GetVisionInterface()->CollectKnownEntities( &knownVector );

				for( int p=0; p< satchelVector.Count(); ++p )
				{
					CBaseEntity *satchel = satchelVector[p];
					if ( !satchel )
					{
						continue;
					}

					for( int k=0; k<knownVector.Count(); ++k )
					{
						if ( knownVector[k].IsObsolete() )
						{
							continue;
						}

						if ( knownVector[k].GetEntity()->IsBaseObject() )
						{
							// we want to put several stickies on a sentry and det at once
							continue;
						}

						if ( satchel->GetTeamNumber() != GetEnemyTeam( knownVector[k].GetEntity()->GetTeamNumber() ) )
						{
							// "known" is either a spectator, or on our team
							continue;
						}

						const float closeRange = 150.0f;
						if ( ( knownVector[k].GetLastKnownPosition() - satchel->GetAbsOrigin() ).IsLengthLessThan( closeRange ) )
						{
							// they are close - blow it!
							me->PressFireButton();
							return;
						}
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------------------
void CHL2MPBotTacticalMonitor::AvoidBumpingEnemies( CHL2MPBot *me )
{
	if ( me->GetDifficulty() < CHL2MPBot::HARD )
		return;

	const float avoidRange = 200.0f;

	CUtlVector< CHL2MP_Player * > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( me->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	CHL2MP_Player *closestEnemy = NULL;
	float closestRangeSq = avoidRange * avoidRange;

	for( int i=0; i<enemyVector.Count(); ++i )
	{
		CHL2MP_Player *enemy = enemyVector[i];

		float rangeSq = ( enemy->GetAbsOrigin() - me->GetAbsOrigin() ).LengthSqr();
		if ( rangeSq < closestRangeSq )
		{
			closestEnemy = enemy;
			closestRangeSq = rangeSq;
		}
	}

	if ( !closestEnemy )
		return;

	// avoid unless hindrance returns a definitive "no"
	if ( me->GetIntentionInterface()->IsHindrance( me, closestEnemy ) == ANSWER_UNDEFINED )
	{
		me->ReleaseForwardButton();
		me->ReleaseLeftButton();
		me->ReleaseRightButton();
		me->ReleaseBackwardButton();

		Vector away = me->GetAbsOrigin() - closestEnemy->GetAbsOrigin();

		me->GetLocomotionInterface()->Approach( me->GetLocomotionInterface()->GetFeet() + away );
	}
}


//-----------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotTacticalMonitor::Update( CHL2MPBot *me, float interval )
{
	if ( hl2mp_bot_force_jump.GetBool() )
	{
		if ( !me->GetLocomotionInterface()->IsClimbingOrJumping() )
		{
			me->GetLocomotionInterface()->Jump();
		}
	}

	const CKnownEntity* threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	Action< CHL2MPBot > *result = me->OpportunisticallyUseWeaponAbilities();
	if ( result )
	{
		return SuspendFor( result, "Opportunistically using buff item" );
	}

	// check if we need to get to cover
	QueryResultType shouldRetreat = me->GetIntentionInterface()->ShouldRetreat( me );

	if ( shouldRetreat == ANSWER_YES )
	{
		return SuspendFor( new CHL2MPBotRetreatToCover, "Backing off" );
	}
	else if ( shouldRetreat != ANSWER_NO )
	{
		if ( !me->IsPropFreak() )
		{
			// retreat if we need to do a full reload (ie: soldiers shot all their rockets)
			if ( me->IsDifficulty( CHL2MPBot::HARD ) || me->IsDifficulty( CHL2MPBot::EXPERT ) )
			{
				CBaseHL2MPCombatWeapon *weapon = ( CBaseHL2MPCombatWeapon *) me->GetActiveWeapon();
				if ( weapon && me->GetAmmoCount( weapon->GetPrimaryAmmoType() ) > 0 && me->IsBarrageAndReloadWeapon( weapon ) )
				{
					if ( weapon->Clip1() <= 1 )
					{
						return SuspendFor( new CHL2MPBotRetreatToCover, "Moving to cover to reload" );
					}
				}
			}
		}
	}

	bool isAvailable = ( me->GetIntentionInterface()->ShouldHurry( me ) != ANSWER_YES );

	// collect ammo and health kits, unless we're in a big hurry
	if ( isAvailable )
	{
		if ( m_maintainTimer.IsElapsed() )
		{
			m_maintainTimer.Start( RandomFloat( 0.3f, 0.5f ) );

			bool isHurt = ( me->GetFlags() & FL_ONFIRE ) || ( ( float )me->GetHealth() / ( float )me->GetMaxHealth() ) < hl2mp_bot_health_ok_ratio.GetFloat();

			if ( isHurt && CHL2MPBotGetHealth::IsPossible( me ) )
			{
				return SuspendFor( new CHL2MPBotGetHealth, "Grabbing nearby health" );
			}

			// Prop freaks go for props instead of ammo.
			if ( !me->IsPropFreak() )
			{
				if ( me->IsAmmoLow() && CHL2MPBotGetAmmo::IsPossible( me ) )
				{
					return SuspendFor( new CHL2MPBotGetAmmo, "Grabbing nearby ammo" );
				}
			}
		}
	}

	// detonate sticky bomb traps when victims are near
	MonitorArmedStickyBombs( me );

	me->UpdateDelayedThreatNotices();

	return Continue();
}


//-----------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotTacticalMonitor::OnOtherKilled( CHL2MPBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	return TryContinue();
}


//-----------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotTacticalMonitor::OnNavAreaChanged( CHL2MPBot *me, CNavArea *newArea, CNavArea *oldArea )
{
	return TryContinue();
}

//-----------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotTacticalMonitor::OnCommandString( CHL2MPBot *me, const char *command )
{
	if ( FStrEq( command, "despawn" ) )
	{
		return TrySuspendFor( new CHL2MPDespawn(), RESULT_CRITICAL, "Received command to go to de-spawn" );
	}

	return TryContinue();
}
