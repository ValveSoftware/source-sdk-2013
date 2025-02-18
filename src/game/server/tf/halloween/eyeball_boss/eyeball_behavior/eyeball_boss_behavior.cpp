//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_behavior.cpp
// The 2011 Halloween Boss' top level behavior, containing all other actions as children
// Michael Booth, October 2011

#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "tf/halloween/eyeball_boss/teleport_vortex.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_behavior.h"
#include "eyeball_boss_emerge.h"
#include "eyeball_boss_stunned.h"


//---------------------------------------------------------------------------------------------
Action< CEyeballBoss > *CEyeballBossBehavior::InitialContainedAction( CEyeballBoss *me )	
{
	return new CEyeballBossEmerge;
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossBehavior::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	return Continue(); 
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossBehavior::Update( CEyeballBoss *me, float interval )
{
	if ( tf_eyeball_boss_debug.GetBool() )
	{
		DevMsg( "%3.2f: DPS = %3.2f\n", gpGlobals->curtime, me->GetInjuryRate() );
	}
	
	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CEyeballBoss > CEyeballBossBehavior::OnInjured( CEyeballBoss *me, const CTakeDamageInfo &info )
{ 
	CTFPlayer *attacker = ToTFPlayer( info.GetAttacker() );
	if ( attacker )
	{
		if ( attacker->HasPurgatoryBuff() && m_stunCooldownTimer.IsElapsed() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_stunned" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );
				event->SetInt( "player_entindex", attacker->entindex() );
				gameeventmanager->FireEvent( event );
			}

			me->LogPlayerInteraction( "eyeball_stunned", attacker );

			m_stunCooldownTimer.Start( 10.0f );

			return TrySuspendFor( new CEyeballBossStunned, RESULT_IMPORTANT, "Hurt by Purgatory Buff!" );
		}

		// critz piss me off
		if ( info.GetDamageType() & DMG_CRITICAL )
		{
			me->BecomeEnraged( 5.0f );
		}

		// heavy DPS pisses me off
		if ( me->GetInjuryRate() > 300.0f )
		{
			me->BecomeEnraged( 5.0f );
		}
	}

	return TryContinue(); 
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CEyeballBoss > CEyeballBossBehavior::OnKilled( CEyeballBoss *me, const CTakeDamageInfo &info )
{ 
	// award achievement to everyone who injured me within the last few seconds
	const float deathTime = 5.0f;
	const CUtlVector< CEyeballBoss::AttackerInfo > &attackerVector = me->GetAttackerVector();
	for( int i=0; i<attackerVector.Count(); ++i )
	{
		if ( attackerVector[i].m_attacker != NULL && 
			gpGlobals->curtime - attackerVector[i].m_timestamp < deathTime )
		{
			if ( !me->IsSpell() )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_killer" );
				if ( event )
				{
					event->SetInt( "level", me->GetLevel() );
					event->SetInt( "player_entindex", attackerVector[i].m_attacker->entindex() );
					gameeventmanager->FireEvent( event );
				}
			}

			if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_VIADUCT ) )
			{
				if ( !me->WasSpawnedByCheats() )
				{
					attackerVector[i].m_attacker->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_EYEBOSS_KILL );
				}
			}

			me->LogPlayerInteraction( "eyeball_killer", attackerVector[i].m_attacker );
		}
	}

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, false, APPEND_PLAYERS );

	UTIL_LogPrintf( "HALLOWEEN: eyeball_death (max_dps %3.2f) (max_health %d) (player_count %d) (level %d)\n", me->GetMaxInjuryRate(), me->GetMaxHealth(), playerVector.Count(), me->GetLevel() );

	return TryChangeTo( new CEyeballBossDead, RESULT_CRITICAL, "I died!" );
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossDead::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	int animSequence = me->LookupSequence( "death" );
	if ( animSequence )
	{
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();
	}

	me->EmitSound( "Halloween.EyeballBossStunned" );

	m_giveUpTimer.Start( 10.0f );

	if ( tf_eyeball_boss_debug.GetBool() )
	{
		DevMsg( "Max Eyeball DPS taken = %3.2f\n", me->GetMaxInjuryRate() );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossDead::Update( CEyeballBoss *me, float interval )
{
	float ground;
	TheNavMesh->GetSimpleGroundHeight( me->WorldSpaceCenter(), &ground );

	if ( m_giveUpTimer.IsElapsed() || ( me->WorldSpaceCenter().z - ground ) < 100.0f )
	{
		// we're on the ground - pop
		DispatchParticleEffect( "eyeboss_death", me->GetAbsOrigin(), me->GetAbsAngles() );

		me->EmitSound( "Cart.Explode" );
		me->EmitSound( "Halloween.EyeballBossDie" );

		UTIL_ScreenShake( me->GetAbsOrigin(), 25.0f, 5.0f, 5.0f, 1000.0f, SHAKE_START );

		UTIL_Remove( me );

		me->SetVictim( NULL );

		if ( !me->IsSpell() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_killed" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );
				gameeventmanager->FireEvent( event );
			}

			// next time, I'll be tougher!
			me->GainLevel();
		}

		// coat nearby players with goo
		const float gooRange = 750.0f;
		me->JarateNearbyPlayers( gooRange );

		// create vortex to loot
		CTeleportVortex *vortex = (CTeleportVortex *)CBaseEntity::Create( "teleport_vortex", me->GetAbsOrigin(), vec3_angle );
		if ( vortex )
		{
			vortex->SetupVortex( true );
		}
	}

	return Continue();
}
