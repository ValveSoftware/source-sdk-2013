//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_emerge.cpp
// The Halloween Boss emerging from the ground
// Michael Booth, October 2011

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "particle_parse.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_emerge.h"
#include "eyeball_boss_idle.h"


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossEmerge::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	if ( me->IsSpell() )
	{
		// just do teleport in code
		me->RemoveEffects( EF_NOINTERP | EF_NODRAW );

		DispatchParticleEffect( "eyeboss_tp_normal", me->GetAbsOrigin(), me->GetAbsAngles() );

		int animSequence = me->LookupSequence( "teleport_in" );
		if ( animSequence )
		{
			me->SetSequence( animSequence );
			me->SetPlaybackRate( 1.0f );
			me->SetCycle( 0 );
			me->ResetSequenceInfo();
		}
	}
	else
	{
		int animSequence = me->LookupSequence( "arrives" );
		if ( animSequence )
		{
			me->SetSequence( animSequence );
			me->SetPlaybackRate( 1.0f );
			me->SetCycle( 0 );
			me->ResetSequenceInfo();
		}

		DispatchParticleEffect( "halloween_boss_summon", me->GetAbsOrigin(), me->GetAbsAngles() );

		m_groundPos = me->GetAbsOrigin();
		m_riseTimer.Start( 3.0f );
		m_emergePos = me->GetAbsOrigin() + Vector( 0, 0, 100.0f );

		m_height = 150.0f;
		me->SetAbsOrigin( m_emergePos + Vector( 0, 0, -m_height ) );
		me->EmitSound( "Halloween.HeadlessBossSpawnRumble" );

		IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_summoned" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );
			gameeventmanager->FireEvent( event );
		}

		m_killTimer.Start( 2.0f );

		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_RED );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, false, APPEND_PLAYERS );

		UTIL_LogPrintf( "HALLOWEEN: eyeball_spawn (max_health %d) (player_count %d) (level %d)\n", me->GetMaxHealth(), playerVector.Count(), me->GetLevel() );
	}

	return Continue();
}


//----------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossEmerge::Update( CEyeballBoss *me, float interval )
{
	if ( !m_riseTimer.IsElapsed() )
	{
		me->SetAbsOrigin( m_emergePos + Vector( 0, 0, -m_height * m_riseTimer.GetRemainingTime() / m_riseTimer.GetCountdownDuration() ) );

		if ( m_rumbleTimer.IsElapsed() )
		{
			m_rumbleTimer.Start( 0.25f );

			// shake nearby players' screens.
			UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
		}
	}

	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CEyeballBossIdle, "Here I am!" );
	}

	// don't do any kill players or remove pipe bomb code below if I'm a spell
	if ( me->IsSpell() )
	{
		return Continue();
	}

	// push players away to avoid penetration issues
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	const float pushRange = 250.0f;
	const float pushForce = 200.0f;

	const float deathRange = me->GetLevel() > 1 ? 200.0f : 100.0f;

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		Vector toPlayer = player->EyePosition() - m_groundPos;
		float range = toPlayer.NormalizeInPlace();

		if ( range < pushRange )
		{
			// make sure we push players up and away
			toPlayer.z = 0.0f;
			toPlayer.NormalizeInPlace();
			toPlayer.z = 1.0f;

			Vector push = pushForce * toPlayer;

			player->RemoveFlag( FL_ONGROUND );
			player->ApplyAbsVelocityImpulse( push );
		}

		// kill anyone touching the summon portal
		if ( !m_killTimer.IsElapsed() )
		{
			if ( range < deathRange )
			{
				CTakeDamageInfo info( me, me, 2.0f * player->GetMaxHealth(), DMG_BLAST, TF_DMG_CUSTOM_PLASMA );
				player->TakeDamage( info );
			}
		}
	}

	// leveled-up boss fizzles any grenades/etc near his summon portal
	if ( !m_killTimer.IsElapsed() && me->GetLevel() > 1 )
	{
		Vector vecSize = Vector( 256, 256, 256 );

		const int maxCollectedEntities = 1024;
		CBaseEntity	*pObjects[ maxCollectedEntities ];
		int count = UTIL_EntitiesInBox( pObjects, maxCollectedEntities, m_groundPos - vecSize, m_groundPos + vecSize, FL_GRENADE );

		for( int i = 0; i < count; ++i )
		{
			if ( pObjects[i] == NULL )
				continue;

			if ( pObjects[i]->IsPlayer() )
				continue;

			// Remove the enemy pipe 
			pObjects[i]->SetThink( &CBaseEntity::SUB_Remove );
			pObjects[i]->SetNextThink( gpGlobals->curtime );
			pObjects[i]->SetTouch( NULL );
			pObjects[i]->AddEffects( EF_NODRAW );
		}
	}

	return Continue();
}
