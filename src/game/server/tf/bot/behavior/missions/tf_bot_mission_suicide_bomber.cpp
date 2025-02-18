//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_mission_suicide_bomber.cpp
// Move to target and explode
// Michael Booth, October 2011

#include "cbase.h"
#include "tf_team.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/missions/tf_bot_mission_suicide_bomber.h"
#include "particle_parse.h"
#include "tf_obj_sentrygun.h"
#include "player_vs_environment/tf_populators.h"

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_suicide_bomb_range( "tf_bot_suicide_bomb_range", "300", FCVAR_CHEAT );
ConVar tf_bot_suicide_bomb_friendly_fire( "tf_bot_suicide_bomb_friendly_fire", "1", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
CTFBotMissionSuicideBomber::CTFBotMissionSuicideBomber( void )
{
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMissionSuicideBomber::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_detonateTimer.Invalidate();
	m_bHasDetonated = false;
	m_consecutivePathFailures = 0;
	m_bWasSuccessful = false;
	m_bWasKilled = false;

	m_victim = me->GetMissionTarget();

	if ( m_victim != NULL )
	{
		m_lastKnownVictimPosition = m_victim->GetAbsOrigin();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMissionSuicideBomber::Update( CTFBot *me, float interval )
{
	// one we start detonating, there's no turning back
	if ( m_detonateTimer.HasStarted() )
	{
		if ( m_detonateTimer.IsElapsed() )
		{
			m_vecDetLocation = me->GetAbsOrigin();
			Detonate( me );

			// Send out an event
			if ( m_bWasSuccessful && m_victim && m_victim->IsBaseObject() )
			{
				CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( m_victim.Get() );
				if ( sentry && sentry->GetOwner() )
				{
					CTFPlayer *pOwner = ToTFPlayer( sentry->GetOwner() );
					if ( pOwner )
					{
						IGameEvent *event = gameeventmanager->CreateEvent( "mvm_sentrybuster_detonate" );
						if ( event )
						{
							event->SetInt( "player", pOwner->entindex() );
							event->SetFloat( "det_x", m_vecDetLocation.x );
							event->SetFloat( "det_y", m_vecDetLocation.y );
							event->SetFloat( "det_z", m_vecDetLocation.z );
							gameeventmanager->FireEvent( event );
						}
					}
				}
			}

			return Done( "KABOOM!" );
		}

		return Continue();
	}


	if ( me->GetHealth() == 1 )
	{
		// low on health - detonate where we are!
		StartDetonate( me, false, true );

		return Continue();
	}

	if ( m_victim != NULL )
	{
		// update chase destination
		if ( m_victim->IsAlive() && !m_victim->IsEffectActive( EF_NODRAW ) )
		{
			m_lastKnownVictimPosition = m_victim->GetAbsOrigin();
		}

		// if the engineer is carrying his sentry, he becomes the victim
		if ( m_victim->IsBaseObject() )
		{
			CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( m_victim.Get() );
			if ( sentry && sentry->IsCarried() && sentry->GetOwner() )
			{
				// path to the engineer carrying the sentry
				m_lastKnownVictimPosition = sentry->GetOwner()->GetAbsOrigin();
			}
		}
	}

	// Get to a third of the damage range before detonating
	const float detonateRange = tf_bot_suicide_bomb_range.GetFloat() / 3.0f;
	if ( me->IsDistanceBetweenLessThan( m_lastKnownVictimPosition, detonateRange ) )
	{
		if ( me->IsLineOfFireClear( m_lastKnownVictimPosition + Vector( 0, 0, StepHeight ) ) )
		{
			StartDetonate( me, true );
		}
	}

	if ( m_talkTimer.IsElapsed() )
	{
		m_talkTimer.Start( 4.0f );
		me->EmitSound( "MVM.SentryBusterIntro" );
	}

	if ( m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( RandomFloat( 0.5f, 1.0f ) );

		CTFBotPathCost cost( me, FASTEST_ROUTE );

		if ( m_path.Compute( me, m_lastKnownVictimPosition, cost ) == false )
		{
			++m_consecutivePathFailures;

			if ( m_consecutivePathFailures >= 3 )
			{
				// really can't reach my victim - detonate!
				StartDetonate( me );
			}
		}
		else
		{
			m_consecutivePathFailures = 0;
		}
	}

	// move to the victim
	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotMissionSuicideBomber::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMissionSuicideBomber::OnKilled( CTFBot *me, const CTakeDamageInfo &info )
{
	if ( !m_bHasDetonated )
	{
		if ( !m_detonateTimer.HasStarted() )
		{
			StartDetonate( me );
		}
		else if ( m_detonateTimer.IsElapsed() )
		{
			Detonate( me );
		}
		else
		{
			// We're in detonate mode, and something's trying to kill us.  Prevent it.
			if ( me->GetTeamNumber() != TEAM_SPECTATOR )
			{
				me->m_lifeState = LIFE_ALIVE;
				me->SetHealth( 1 );
			}
		}
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMissionSuicideBomber::OnStuck( CTFBot *me )
{
	// we're stuck, decide to detonate now!
	if ( !m_bHasDetonated && !m_detonateTimer.HasStarted() )
	{
		StartDetonate( me );
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
void CTFBotMissionSuicideBomber::StartDetonate( CTFBot *me, bool bWasSuccessful /* = false */, bool bWasKilled /*= false*/ )
{
	if ( m_detonateTimer.HasStarted() )
		return;

	if ( !me->IsAlive() || me->GetHealth() < 1 )
	{
		if ( me->GetTeamNumber() != TEAM_SPECTATOR)
		{
			me->m_lifeState = LIFE_ALIVE;
			me->SetHealth( 1 );
		}
	}

	m_bWasSuccessful = bWasSuccessful;
	m_bWasKilled = bWasKilled;

	me->m_takedamage = DAMAGE_NO;

	me->Taunt( TAUNT_BASE_WEAPON );
	m_detonateTimer.Start( 2.0f );
	me->EmitSound( "MvM.SentryBusterSpin" );
}


//---------------------------------------------------------------------------------------------
void CTFBotMissionSuicideBomber::Detonate( CTFBot *me )
{
	// BLAST!
	m_bHasDetonated = true;
 
	DispatchParticleEffect( "explosionTrail_seeds_mvm", me->GetAbsOrigin(), me->GetAbsAngles() );
	DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", me->GetAbsOrigin(), me->GetAbsAngles() );

	me->EmitSound( "MVM.SentryBusterExplode" );

	UTIL_ScreenShake( me->GetAbsOrigin(), 25.0f, 5.0f, 5.0f, 1000.0f, SHAKE_START );

	if ( !m_bWasSuccessful )
	{
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_SENTRY_BUSTER_DOWN, TF_TEAM_PVE_DEFENDERS );

			// ACHIEVEMENT_TF_MVM_KILL_SENTRY_BUSTER
			for ( int iDamager = 0 ; iDamager < MAX_ACHIEVEMENT_HISTORY_SLOTS ; iDamager ++ )
			{	
				EntityHistory_t *damagerHistory = me->m_AchievementData.GetDamagerHistory( iDamager );
				if ( damagerHistory )
				{
					if ( damagerHistory->hEntity && ( gpGlobals->curtime - damagerHistory->flTimeDamage <= 5.0f ) )
					{
						CTFPlayer *pRecentDamager = ToTFPlayer( damagerHistory->hEntity );
						if ( pRecentDamager )
						{
							pRecentDamager->AwardAchievement( ACHIEVEMENT_TF_MVM_KILL_SENTRY_BUSTER );
						}
					}
				}
			}
		}
	}

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	CUtlVector< CBaseCombatCharacter * > victimVector;

	int i;

	// players
	for ( i=0; i<playerVector.Count(); ++i )
	{
		victimVector.AddToTail( playerVector[i] );
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

	// Send out an event whenever players damaged us to the point where we had to detonate
	if ( m_bWasKilled )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "mvm_sentrybuster_killed" );
		if ( event )
		{
			event->SetInt( "sentry_buster", me->entindex() );
			gameeventmanager->FireEvent( event );
		}
	}

	// Clear my mission before we have everyone take damage so I will die with the rest
	me->SetMission( CTFBot::NO_MISSION, MISSION_DOESNT_RESET_BEHAVIOR_SYSTEM );
	me->m_takedamage = DAMAGE_YES;

	// kill victims (including me)
	for( int i=0; i<victimVector.Count(); ++i )
	{
		CBaseCombatCharacter *victim = victimVector[i];

		Vector toVictim = victim->WorldSpaceCenter() - me->WorldSpaceCenter();

		if ( toVictim.IsLengthGreaterThan( tf_bot_suicide_bomb_range.GetFloat() ) )
			continue;

		if ( victim->IsPlayer() )
		{
			color32 colorHit = { 255, 255, 255, 255 };
			UTIL_ScreenFade( victim, colorHit, 1.0f, 0.1f, FFADE_IN );
		}

		if ( me->IsLineOfFireClear( victim ) )
		{
			toVictim.NormalizeInPlace();

			int damage = MAX( victim->GetMaxHealth(), victim->GetHealth() );

			CTakeDamageInfo info( me, me, 4 * damage, DMG_BLAST, TF_DMG_CUSTOM_NONE );
			if ( tf_bot_suicide_bomb_friendly_fire.GetBool() )
			{
				info.SetForceFriendlyFire( true );
			}

			CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 1.0f );
			victim->TakeDamage( info );
		}
	}

	// make sure we're removed (in case we detonated in our spawn area where we are invulnerable)
	me->CommitSuicide( false, true );
	if ( me->IsAlive() )
	{
		me->ForceChangeTeam( TEAM_SPECTATOR );
	}

	if ( m_bWasKilled )
	{
		// increment num sentry killed this wave
		CWave *pWave = g_pPopulationManager ? g_pPopulationManager->GetCurrentWave() : NULL;
		if ( pWave )
		{
			pWave->IncrementSentryBustersKilled();
		}
	}
}


// Should we attack "them"?
QueryResultType CTFBotMissionSuicideBomber::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	// buster never "attacks", just approaches and self-detonates
	return ANSWER_NO;
}


