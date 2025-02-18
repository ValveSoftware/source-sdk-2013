//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"

#include "../zombie.h"
#include "zombie_attack.h"
#include "zombie_special_attack.h"

#define ZOMBIE_CHASE_MIN_DURATION 3.0f

ConVar tf_halloween_zombie_damage( "tf_halloween_zombie_damage", "10", FCVAR_CHEAT, "How much damage a zombie melee hit does." );


//----------------------------------------------------------------------------------
ActionResult< CZombie >	CZombieAttack::OnStart( CZombie *me, Action< CZombie > *priorAction )
{
	// smooth out the bot's path following by moving toward a point farther down the path
	m_path.SetMinLookAheadDistance( 100.0f );

	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );

	m_specialAttackTimer.Start( RandomFloat( 5.f, 10.f ) );

	return Continue();
}


//----------------------------------------------------------------------------------
bool CZombieAttack::IsPotentiallyChaseable( CZombie *me, CBaseCombatCharacter *victim )
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

	return true;
}


//----------------------------------------------------------------------------------
void CZombieAttack::SelectVictim( CZombie *me )
{
	if ( IsPotentiallyChaseable( me, m_attackTarget ) && !m_attackTargetFocusTimer.IsElapsed() )
	{
		// Continue chasing current target
		return;
	}

	// pick a new victim to chase
	CBaseCombatCharacter *newVictim = NULL;
	CUtlVector< CTFPlayer * > playerVector;

	// collect everyone
	if ( me->GetTeamNumber() == TF_TEAM_RED )
	{
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );
	}
	else if ( me->GetTeamNumber() == TF_TEAM_BLUE )
	{
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	}
	else
	{
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );
	}

	float victimRangeSq = FLT_MAX;
	// find closest player
	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *pPlayer = playerVector[i];
		if ( !IsPotentiallyChaseable( me, pPlayer ) )
		{
			continue;
		}

		// ignore stealth player
		if ( pPlayer->m_Shared.IsStealthed() )
		{
			if ( !pPlayer->m_Shared.InCond( TF_COND_BURNING ) &&
				!pPlayer->m_Shared.InCond( TF_COND_URINE ) &&
				!pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) &&
				!pPlayer->m_Shared.InCond( TF_COND_BLEEDING ) )
			{
				// cloaked spies are invisible to us
				continue;
			}
		}

		// ignore player who disguises as my team
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() )
		{
			continue;
		}

		// ignore ghost players
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		{
			continue;
		}

		float rangeSq = me->GetRangeSquaredTo( pPlayer );
		if ( rangeSq < victimRangeSq )
		{
			newVictim = pPlayer;
			victimRangeSq = rangeSq;
		}
	}

	// find closest zombie
	for ( int i=0; i<IZombieAutoList::AutoList().Count(); ++i )
	{
		CZombie* pZombie = static_cast< CZombie* >( IZombieAutoList::AutoList()[i] );
		if ( pZombie->GetTeamNumber() == me->GetTeamNumber() )
		{
			continue;
		}

		if ( !IsPotentiallyChaseable( me, pZombie ) )
		{
			continue;
		}

		float rangeSq = me->GetRangeSquaredTo( pZombie );
		if ( rangeSq < victimRangeSq )
		{
			newVictim = pZombie;
			victimRangeSq = rangeSq;
		}
	}

	if ( newVictim )
	{
		// we have a new victim
		m_attackTargetFocusTimer.Start( ZOMBIE_CHASE_MIN_DURATION );
	}

	m_attackTarget = newVictim;
}


//----------------------------------------------------------------------------------
ActionResult< CZombie >	CZombieAttack::Update( CZombie *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done();
	}

	if ( !m_tauntTimer.IsElapsed() )
	{
		// wait for taunt to finish
		return Continue();
	}

	SelectVictim( me );

	if ( m_attackTarget == NULL || !m_attackTarget->IsAlive() )
	{
		return Continue();
	}

	// chase after our chase victim
	const float standAndSwingRange = 50.0f;

	bool isLineOfSightClear = me->IsLineOfSightClear( m_attackTarget );

	if ( me->IsRangeGreaterThan( m_attackTarget, standAndSwingRange ) || !isLineOfSightClear )
	{
		if ( m_path.GetAge() > 0.5f )
		{
			CZombiePathCost cost( me );
			m_path.Compute( me, m_attackTarget, cost );
		}

		m_path.Update( me );
	}

	// claw at attack target if they are in range
	const float zombieSwingRange = 150.0f;
	if ( me->IsRangeLessThan( m_attackTarget, zombieSwingRange ) )
	{
		me->GetLocomotionInterface()->FaceTowards( m_attackTarget->WorldSpaceCenter() );

		// swing!
		if ( !me->IsPlayingGesture( ACT_MP_ATTACK_STAND_MELEE ) )
		{
			me->AddGesture( ACT_MP_ATTACK_STAND_MELEE );
		}

		const float zombieAttackRange = me->GetAttackRange();
		if ( me->IsRangeLessThan( m_attackTarget, zombieAttackRange ) )
		{
			if ( me->GetSkeletonType() == 1 && m_specialAttackTimer.IsElapsed() )
			{
				m_specialAttackTimer.Start( RandomFloat( 5.f, 10.f ) );
				return SuspendFor( new CZombieSpecialAttack, "Do Special Attack!" );
			}

			if ( m_attackTimer.IsElapsed() )
			{
				m_attackTimer.Start( RandomFloat( 0.8f, 1.2f ) );

				Vector toVictim = m_attackTarget->WorldSpaceCenter() - me->WorldSpaceCenter();
				toVictim.NormalizeInPlace();

				// hit!
				CBaseEntity *pAttacker = me->GetOwnerEntity() ? me->GetOwnerEntity() : me;
				CTakeDamageInfo info( pAttacker, pAttacker, me->GetAttackDamage(), DMG_SLASH );
				info.SetDamageCustom( TF_DMG_CUSTOM_SPELL_SKELETON );
				CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 5.0f );
				m_attackTarget->TakeDamage( info );
			}
		}
	}

	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
	{
		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CZombie > CZombieAttack::OnStuck( CZombie *me )
{
	// if we're stuck just die
	CTakeDamageInfo info( me, me, 99999.9f, DMG_SLASH );
	me->TakeDamage( info );

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CZombie > CZombieAttack::OnContact( CZombie *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( other );
		if ( pTFPlayer )
		{
			if ( pTFPlayer->IsAlive() && me->GetTeamNumber() != TF_TEAM_HALLOWEEN && me->GetTeamNumber() != pTFPlayer->GetTeamNumber() )
			{
				// force attack the thing we bumped into
				// this prevents us from being stuck on dispensers, for example
				m_attackTarget = pTFPlayer;
				m_attackTargetFocusTimer.Start( ZOMBIE_CHASE_MIN_DURATION );
			}
		}
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CZombie > CZombieAttack::OnOtherKilled( CZombie *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	/*if ( victim && victim->IsPlayer() && me->GetLocomotionInterface()->IsOnGround() )
	{
		me->AddGestureSequence( me->LookupSequence( "taunt06" ) );
		m_tauntTimer.Start( 3.0f );
	}*/

	return TryContinue( RESULT_TRY );
}
