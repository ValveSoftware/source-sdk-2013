//========= Copyright Valve Corporation, All rights reserved. ============//
// crybaby_boss_attack.cpp
// Halloween Boss 2011 chase and attack behavior
// Michael Booth, October 2011

#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "particle_parse.h"
#include "team_control_point_master.h"

#include "../headless_hatman.h"
#include "crybaby_boss_attack.h"


//----------------------------------------------------------------------------------
CCryBabyBossAttack::CCryBabyBossAttack( CTFPlayer *victim )
{
	m_victim = victim;
}


//----------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CCryBabyBossAttack::OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction )
{
	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM1 );

	m_axeSwingTimer.Invalidate();
	m_attackTimer.Invalidate();

	m_attackTarget = NULL;
	m_attackTargetFocusTimer.Invalidate();

	return Continue();
}


//----------------------------------------------------------------------------------
bool CCryBabyBossAttack::IsSwingingAxe( void ) const
{
	return !m_axeSwingTimer.IsElapsed();
}


//----------------------------------------------------------------------------------
void CCryBabyBossAttack::UpdateAxeSwing( CHeadlessHatman *me )
{
	if ( m_axeSwingTimer.HasStarted() )
	{
		// continue axe swing
		if ( m_axeSwingTimer.IsElapsed() )
		{
			// moment of impact - did axe swing hit?
			m_axeSwingTimer.Invalidate();

			if ( m_attackTarget != NULL )
			{
				Vector forward;
				me->GetVectors( &forward, NULL, NULL );

				Vector toVictim = m_attackTarget->WorldSpaceCenter() - me->WorldSpaceCenter();
				toVictim.NormalizeInPlace();

				// looser tolerance as victim gets closer
				const float closeRange = 100.0f;
				float range = me->GetRangeTo( m_attackTarget );
				float closeness = ( range < closeRange ) ? 0.0f : ( range - closeRange ) / ( tf_halloween_bot_attack_range.GetFloat() - closeRange );
				float hitAngle = 0.0f + closeness * 0.27f;

				if ( DotProduct( forward, toVictim ) > hitAngle )
				{
					if ( me->IsRangeLessThan( m_attackTarget, 0.9f * tf_halloween_bot_attack_range.GetFloat() ) )
					{
						if ( me->IsLineOfSightClear( m_attackTarget ) )
						{
							// CHOP!
							CTakeDamageInfo info( me, me, 2.0f * m_attackTarget->GetHealth(), DMG_CLUB, TF_DMG_CUSTOM_DECAPITATION_BOSS );
							CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 5.0f );
							m_attackTarget->TakeDamage( info );
							me->EmitSound( "Halloween.HeadlessBossAxeHitFlesh" );
						}
					}
				}
			}

			// always playe the axe-hit-world impact sound, since it carries through the world better
			me->EmitSound( "Halloween.HeadlessBossAxeHitWorld" );
			UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );

			Vector bladePos;
			QAngle bladeAngle;
			if ( me->GetAxe() && me->GetAxe()->GetAttachment( "axe_blade", bladePos, bladeAngle ) )
			{
				DispatchParticleEffect( "halloween_boss_axe_hit_world", bladePos, bladeAngle );
			}
		}
	}
}


//----------------------------------------------------------------------------------
// Validate that our victim is still alive and someplace we can actually get to
bool CCryBabyBossAttack::IsVictimChaseable( CHeadlessHatman *me )
{
	if ( m_victim == NULL )
	{
		return false;
	}

	if ( !m_victim->IsAlive() )
	{
		return false;
	}

	CTFNavArea *victimArea = (CTFNavArea *)m_victim->GetLastKnownArea();
	if ( !victimArea || victimArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
	{
		// unreachable
		return false;
	}

	if ( m_victim->GetGroundEntity() != NULL )
	{
		Vector victimAreaPos;
		victimArea->GetClosestPointOnArea( m_victim->GetAbsOrigin(), &victimAreaPos );
		if ( ( m_victim->GetAbsOrigin() - victimAreaPos ).AsVector2D().IsLengthGreaterThan( 50.0f ) )
		{
			// off the mesh and unreachable
			return false;
		}
	}

	return true;
}


//----------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CCryBabyBossAttack::Update( CHeadlessHatman *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done();
	}

	if ( !IsVictimChaseable( me ) )
	{
		return Done( "Victim is dead or unreachable" );
	}

	if ( !IsSwingingAxe() && m_laughTimer.IsElapsed() )
	{
		me->EmitSound( "Halloween.HeadlessBossLaugh" );
		m_laughTimer.Start( RandomFloat( 3.0f, 5.0f ) );
	}

	// chase after our chase victim
	const float standAndSwingRange = 100.0f;

	if ( me->IsRangeGreaterThan( m_victim, standAndSwingRange ) || !me->IsLineOfSightClear( m_victim ) )
	{
		CHeadlessHatmanPathCost cost( me );
		m_chasePath.Update( me, m_victim, cost );
	}

	if ( m_attackTargetFocusTimer.IsElapsed() || m_attackTarget == NULL )
	{
		m_attackTarget = m_victim.Get();
	}

	// swing our axe at our attack target if they are in range
	if ( m_attackTarget != NULL && m_attackTarget->IsAlive() )
	{
		if ( me->IsRangeLessThan( m_attackTarget, tf_halloween_bot_attack_range.GetFloat() ) )
		{
			if ( m_attackTimer.IsElapsed() )
			{
				if ( !IsSwingingAxe() )
				{
					me->AddGesture( ACT_MP_ATTACK_STAND_ITEM1 );
					m_axeSwingTimer.Start( 0.58f );
					me->EmitSound( "Halloween.HeadlessBossAttack" );

					m_attackTimer.Start( 1.0f );
				}
			}

			me->GetLocomotionInterface()->FaceTowards( m_attackTarget->WorldSpaceCenter() );
		}
	}

	// finish axe swing once it has begun
	UpdateAxeSwing( me );

	if ( me->GetLocomotionInterface()->IsAttemptingToMove() )
	{
		// play running animation
		int healthRatio = 100 * me->GetHealth() / me->GetMaxHealth();

		if ( healthRatio > 50 )
		{
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_ITEM1 ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM1 );
			}
		}
		else
		{
			if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
			{
				me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
			}
		}

		if ( m_footfallTimer.IsElapsed() )
		{
			m_footfallTimer.Start( 0.25f );

			// periodically shake nearby players' screens when we're moving
			UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
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
EventDesiredResult< CHeadlessHatman > CCryBabyBossAttack::OnStuck( CHeadlessHatman *me )
{
	// we're stuck - just warp to the our next path goal
	if ( m_chasePath.GetCurrentGoal() )
	{
		me->SetAbsOrigin( m_chasePath.GetCurrentGoal()->pos + Vector( 0, 0, 10.0f ) );
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHeadlessHatman > CCryBabyBossAttack::OnContact( CHeadlessHatman *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other )
	{
		CBaseCombatCharacter *combatChar = dynamic_cast< CBaseCombatCharacter * >( other );
		if ( combatChar && combatChar->IsAlive() )
		{
			// force attack the thing we bumped into
			// this prevents us from being stuck on dispensers, for example
			m_attackTarget = combatChar;
			m_attackTargetFocusTimer.Start( 3.0f );
		}
	}

	return TryContinue( RESULT_TRY );
}

