//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_nuke_attack.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_player.h"
#include "tf_team.h"
#include "player_vs_environment/monster_resource.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_nuke_attack.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_stunned.h"


ConVar tf_boss_alpha_nuke_charge_time( "tf_boss_alpha_nuke_charge_time", "5" );
ConVar tf_boss_alpha_nuke_interval( "tf_boss_alpha_nuke_interval", "20" );
ConVar tf_boss_alpha_nuke_lethal_time( "tf_boss_alpha_nuke_lethal_time", "999999999" );		// 300
ConVar tf_boss_alpha_nuke_damage( "tf_boss_alpha_nuke_damage", "75"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_nuke_max_remaining_health( "tf_boss_alpha_nuke_max_remaining_health", "60"/*, FCVAR_CHEAT*/ );
ConVar tf_boss_alpha_nuke_afterburn_time( "tf_boss_alpha_nuke_afterburn_time", "5"/*, FCVAR_CHEAT*/ );

extern ConVar tf_boss_alpha_stunned_duration;


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaNukeAttack::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_MP_JUMP_FLOAT_LOSERSTATE );
	me->StartNukeEffect();

	me->EmitSound( "RobotBoss.ChargeUpNukeAttack" );
	// me->AddCondition( CBossAlpha::VULNERABLE_TO_STUN );

	m_chargeUpTimer.Start( tf_boss_alpha_nuke_charge_time.GetFloat() );
	m_shakeTimer.Start( 0.25f );


	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaNukeAttack::Update( CBossAlpha *me, float interval )
{
	float stunRatio = me->GetStunDamage() / me->GetBecomeStunnedDamage();

	if ( me->HasAbility( CBossAlpha::CAN_BE_STUNNED ) && stunRatio >= 1.0f )
	{
		return ChangeTo( new CBossAlphaStunned( tf_boss_alpha_stunned_duration.GetFloat() ), "They got me" );
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

				float damage = tf_boss_alpha_nuke_damage.GetFloat();

				if ( me->GetAge() > tf_boss_alpha_nuke_lethal_time.GetFloat() )
				{
					// nuke is now lethal
					damage = 999.9f;
				}
				else if ( tf_boss_alpha_nuke_max_remaining_health.GetFloat() >= 0.0f )
				{
					// nuke slams everyone's health to this
					if ( victim->GetHealth() > tf_boss_alpha_nuke_max_remaining_health.GetFloat() )
					{
						damage = victim->GetHealth() - tf_boss_alpha_nuke_max_remaining_health.GetFloat();
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
						playerVictim->m_Shared.Burn( me, tf_boss_alpha_nuke_afterburn_time.GetFloat() );
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
void CBossAlphaNukeAttack::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
	me->RemoveCondition( CBossAlpha::VULNERABLE_TO_STUN );
	me->StopNukeEffect();
	me->ClearStunDamage();
	me->GetNukeTimer()->Start( tf_boss_alpha_nuke_interval.GetFloat() );

	if ( g_pMonsterResource )
	{
		g_pMonsterResource->HideBossStunMeter();
	}
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaNukeAttack::OnInjured( CBossAlpha *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL );
}

#endif // TF_RAID_MODE
