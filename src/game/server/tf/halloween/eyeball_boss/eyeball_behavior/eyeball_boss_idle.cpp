//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_idle.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "particle_parse.h"
#include "tf_gamerules.h"

#include "../eyeball_boss.h"
#include "eyeball_boss_idle.h"
#include "eyeball_boss_teleport.h"
#include "eyeball_boss_notice.h"
#include "eyeball_boss_emote.h"


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossIdle::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	m_attacker = NULL;

	m_talkTimer.Start( RandomFloat( 3.0f, 5.0f ) );

	m_lifeTimer.Start( me->IsSpell() ? tf_eyeball_boss_lifetime_spell.GetFloat() : tf_eyeball_boss_lifetime.GetFloat() );

	m_moveTimer.Start( RandomFloat( 10.0f, 15.0f ) );

	m_lastWarnTime = 0.0f;
	m_isLaughReady = false;
	m_lastHealth = me->GetHealth();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossIdle::Update( CEyeballBoss *me, float interval )
{
	if ( tf_eyeball_boss_debug_orientation.GetBool() )
	{
		CBaseCombatCharacter *target = me->FindClosestVisibleVictim();

		if ( target )
		{
			me->GetBodyInterface()->AimHeadTowards( target );
		}

		if ( me->GetInjuryRate() > 0.0001f )
		{
			DevMsg( "%3.2f: DPS = %3.2f, Max DPS = %3.2f\n", gpGlobals->curtime, me->GetInjuryRate(), me->GetMaxInjuryRate() );
		}

		return Continue();
	}

	if ( !me->IsSpell() )
	{
		if ( m_lifeTimer.GetRemainingTime() < 10.0f && m_lastWarnTime > 10.0f )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escape_imminent" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );
				event->SetInt( "time_remaining", 10 );
				gameeventmanager->FireEvent( event );
			}
		}
		else if ( m_lifeTimer.GetRemainingTime() < 30.0f && m_lastWarnTime > 30.0f )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escape_imminent" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );
				event->SetInt( "time_remaining", 30 );
				gameeventmanager->FireEvent( event );
			}
		}
		else if ( m_lifeTimer.GetRemainingTime() < 60.0f && m_lastWarnTime > 60.0f )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escape_imminent" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );
				event->SetInt( "time_remaining", 60 );
				gameeventmanager->FireEvent( event );
			}
		}
		m_lastWarnTime = m_lifeTimer.GetRemainingTime();
	}

	// just leave
	if ( me->IsSpell() && m_lifeTimer.IsElapsed() )
	{
		return ChangeTo( new CEyeballBossEscape, "Escaping..." );
	}

	// teleport to a new place
	if ( m_moveTimer.IsElapsed() && !me->IsSpell() )
	{
		// get low
		me->GetLocomotionInterface()->SetDesiredAltitude( 0.0f );

		// only teleport if we're low enough for players to reach the vortex we leave behind
		Vector ground = me->WorldSpaceCenter();
		if ( TheNavMesh->GetSimpleGroundHeight( me->WorldSpaceCenter(), &ground.z ) )
		{
			const float maxTeleportHeight = 300.0f;

			// make sure the ground is actually reachable by the players
			CNavArea *area = TheNavMesh->GetNearestNavArea( ground, true, maxTeleportHeight * 1.5f );

			if ( area )
			{
				if ( me->WorldSpaceCenter().z - ground.z < maxTeleportHeight )
				{
					if ( m_lifeTimer.IsElapsed() )
					{
						return ChangeTo( new CEyeballBossEscape, "Escaping..." );
					}

					m_moveTimer.Start( RandomFloat( 10.0f, 15.0f ) );

					return SuspendFor( new CEyeballBossTeleport, "Moving..." );
				}
			}
		}
	}
	else
	{
		// resume hovering
		me->GetLocomotionInterface()->SetDesiredAltitude( tf_eyeball_boss_hover_height.GetFloat() );
	}

	CBaseCombatCharacter *victim = me->FindClosestVisibleVictim();
	if ( victim )
	{
		me->SetVictim( victim );

		return SuspendFor( new CEyeballBossNotice, "Target found..." );
	}

	if ( m_attacker != NULL )
	{
		// look at attacker that just injured us
		me->GetBodyInterface()->AimHeadTowards( m_attacker );

		m_attacker = NULL;
		m_lookAroundTimer.Start( RandomFloat( 0.5f, 2.0f ) );
	}
	else
	{
		if ( m_lookAroundTimer.IsElapsed() )
		{
			// look around
			m_lookAroundTimer.Start( RandomFloat( 2.0f, 4.0f ) );

			Vector target;

			SinCos( RandomFloat( -3.141592f, 3.141592f ), &target.y, &target.x );
			target.z = 0.0f;

			me->GetBodyInterface()->AimHeadTowards( me->GetAbsOrigin() + 100.0f * target );
		}

		// we have no target - if someone died recently, laugh
		if ( m_isLaughReady )
		{
			m_isLaughReady = false;
			return SuspendFor( new CEyeballBossEmote( me->LookupSequence( "laugh" ), "Halloween.EyeballBossLaugh" ), "Taunt our victim" );
		}
	}

	int animSequence = 0;

	if ( me->IsEnraged() )
	{
		animSequence = me->LookupSequence( "lookaround3" );
	}
	else if ( me->IsGrumpy() )
	{
		animSequence = me->LookupSequence( "lookaround2" );
	}
	else
	{
		animSequence = me->LookupSequence( "lookaround1" );
	}

	if ( animSequence )
	{
		if ( me->GetSequence() != animSequence || me->IsSequenceFinished() )
		{
			me->SetSequence( animSequence );
			me->SetPlaybackRate( 1.0f );
			me->SetCycle( 0 );
			me->ResetSequenceInfo();
		}
	}

	if ( m_talkTimer.IsElapsed() )
	{
		if ( me->IsEnraged() )
		{
			me->EmitSound( "Halloween.EyeballBossRage" );
			m_talkTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}
		else
		{
			me->EmitSound( "Halloween.EyeballBossIdle" );
			m_talkTimer.Start( RandomFloat( 3.0f, 5.0f ) );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossIdle::OnResume( CEyeballBoss *me, Action< CEyeballBoss > *interruptingAction )
{
	m_talkTimer.Start( RandomFloat( 3.0f, 5.0f ) );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CEyeballBoss > CEyeballBossIdle::OnInjured( CEyeballBoss *me, const CTakeDamageInfo &info )
{
	m_attacker = info.GetAttacker();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CEyeballBoss > CEyeballBossIdle::OnOtherKilled( CEyeballBoss *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	m_isLaughReady = true;
	return TryContinue();
}

