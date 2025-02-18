//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_attack.cpp
// Halloween Boss chase and attack behavior
// Michael Booth, October 2010

#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "particle_parse.h"
#include "team_control_point_master.h"

#include "../headless_hatman.h"
#include "headless_hatman_attack.h"
#include "headless_hatman_terrify.h"


ConVar tf_halloween_hhh_attack_kart_radius( "tf_halloween_hhh_attack_kart_radius", "300", FCVAR_CHEAT );

//----------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CHeadlessHatmanAttack::OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction )
{
	// smooth out the bot's path following by moving toward a point farther down the path
	m_path.SetMinLookAheadDistance( 100.0f );

	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_ITEM1 );

	m_axeSwingTimer.Invalidate();
	m_attackTimer.Invalidate();

	m_closestVisible = NULL;
	TFGameRules()->SetIT( NULL );
	m_lastIT = NULL;
	m_chaseVictimTimer.Invalidate();

	m_attackTarget = NULL;
	m_attackTargetFocusTimer.Invalidate();

	m_scareTimer.Start( 20.0f );

	m_homePos = me->GetAbsOrigin();
	m_homePosRecalcTimer.Start( 3.0f );

	return Continue();
}


//----------------------------------------------------------------------------------
bool CHeadlessHatmanAttack::IsSwingingAxe( void ) const
{
	return !m_axeSwingTimer.IsElapsed();
}


//----------------------------------------------------------------------------------
bool CHeadlessHatmanAttack::IsPotentiallyChaseable( CHeadlessHatman *me, CTFPlayer *victim )
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

	if ( victim->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		// don't attack ghost
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

	if ( victim->m_Shared.IsInvulnerable() )
	{
		// invulnerable - pick a new victim
		return false;
	}

	Vector toHome = m_homePos - victim->GetAbsOrigin();
	if ( !victim->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		if ( toHome.IsLengthGreaterThan( tf_halloween_bot_quit_range.GetFloat() ) )
		{
			// too far from home - pick a new victim
			return false;
		}
	}

	return true;
}


//----------------------------------------------------------------------------------
void CHeadlessHatmanAttack::ValidateChaseVictim( CHeadlessHatman *me )
{
	CTFPlayer *victim = ToTFPlayer( TFGameRules()->GetIT() );

	if ( victim && m_lastIT != victim )
	{
		// something has changed our victim
		m_chaseVictimTimer.Start( tf_halloween_bot_chase_duration.GetFloat() );
		m_lastIT = victim;
	}

	if ( !IsPotentiallyChaseable( me, victim ) )
	{
		// we can no longer reach this victim
		TFGameRules()->SetIT( NULL );
	}
}


//----------------------------------------------------------------------------------
void CHeadlessHatmanAttack::SelectVictim( CHeadlessHatman *me )
{
	ValidateChaseVictim( me );

	m_closestVisible = ToTFPlayer( TFGameRules()->GetIT() );

	if ( TFGameRules()->GetIT() == NULL )
	{
		// pick a new victim to chase
		CTFPlayer *newVictim = NULL;
		CUtlVector< CTFPlayer * > playerVector;

		// collect everyone
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

		float victimRangeSq = FLT_MAX;
		float visibleVictimRangeSq = FLT_MAX;

		for( int i=0; i<playerVector.Count(); ++i )
		{
			if ( !IsPotentiallyChaseable( me, playerVector[i] ) )
			{
				continue;
			}

			float rangeSq = me->GetRangeSquaredTo( playerVector[i] );
			if ( rangeSq < visibleVictimRangeSq )
			{
				if ( me->IsLineOfSightClear( playerVector[i] ) )
				{
					// track closest visible victim so we can look at players out of attack range
					m_closestVisible = playerVector[i];
					visibleVictimRangeSq = rangeSq;
				}
			}

			// check distance to home - don't pick victims too far away from home
			if ( !playerVector[i]->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
			{
				Vector toHome = m_homePos - playerVector[i]->GetAbsOrigin();
				if ( toHome.IsLengthGreaterThan( tf_halloween_bot_chase_range.GetFloat() ) )
				{
					continue;
				}
			}

			if ( rangeSq < victimRangeSq )
			{
				newVictim = playerVector[i];
				victimRangeSq = rangeSq;
			}
		}

		if ( newVictim )
		{
			// we have a new victim
			m_chaseVictimTimer.Start( tf_halloween_bot_chase_duration.GetFloat() );

			TFGameRules()->SetIT( newVictim );
		}
	}

	// attack the "IT" player unless we're focusing on something else
	if ( m_attackTarget == NULL || m_attackTargetFocusTimer.IsElapsed() || !m_attackTarget->IsAlive() )
	{
		m_attackTarget = ToTFPlayer( TFGameRules()->GetIT() );
	}
}

//----------------------------------------------------------------------------------
void CHeadlessHatmanAttack::AttackTarget( CHeadlessHatman *me, CBaseCombatCharacter *pTarget, float flAttackRange )
{
	// out of range? don't do anything
	if ( !me->IsRangeLessThan( pTarget, flAttackRange ) )
		return;

	Vector forward;
	me->GetVectors( &forward, NULL, NULL );

	Vector toVictim = pTarget->WorldSpaceCenter() - me->WorldSpaceCenter();
	toVictim.NormalizeInPlace();

	CTFPlayer *pPlayer = pTarget->IsPlayer() ? ToTFPlayer( pTarget ) : NULL;
	// push kart player or ghost
	if ( pPlayer && ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) || pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) ) )
	{
		// Apply Huge Force
		Vector vecDir = toVictim;
		vecDir.NormalizeInPlace();
		vecDir.z += 0.7f;

		vecDir *= 1300.0f;

		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			pPlayer->AddHalloweenKartPushEvent( NULL, NULL, NULL, vecDir, 100.0f, TF_DMG_CUSTOM_DECAPITATION_BOSS );
		}
		else
		{
			pPlayer->ApplyAbsVelocityImpulse( vecDir );
		}
	}
	else
	{
		// looser tolerance as victim gets closer
		const float closeRange = 0.5f * flAttackRange;
		float range = me->GetRangeTo( pTarget );
		float closeness = ( range < closeRange ) ? 0.0f : ( range - closeRange ) / ( flAttackRange - closeRange );
		float hitAngle = 0.0f + closeness * 0.27f;

		// is target in front?
		if ( DotProduct( forward, toVictim ) > hitAngle )
		{
			if ( me->IsLineOfSightClear( pTarget ) )
			{
				// CHOP!
				CTakeDamageInfo info( me, me, m_attackTarget->GetMaxHealth() * 0.8f, DMG_CLUB, TF_DMG_CUSTOM_DECAPITATION_BOSS );
				CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 5.0f );
				m_attackTarget->TakeDamage( info );
				me->EmitSound( "Halloween.HeadlessBossAxeHitFlesh" );
			}
		}
	}
}


//----------------------------------------------------------------------------------
void CHeadlessHatmanAttack::UpdateAxeSwing( CHeadlessHatman *me )
{
	if ( m_axeSwingTimer.HasStarted() )
	{
		// continue axe swing
		if ( m_axeSwingTimer.IsElapsed() )
		{
			// moment of impact - did axe swing hit?
			m_axeSwingTimer.Invalidate();

			if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
			{
				CUtlVector< CTFPlayer* > playerVector;
				CollectPlayers( &playerVector, TEAM_ANY );
				for( CTFPlayer* pPlayer : playerVector )
				{
					AttackTarget( me, pPlayer, tf_halloween_hhh_attack_kart_radius.GetFloat() );
				}

				DispatchParticleEffect( "hammer_impact_button", me->GetAbsOrigin(), me->GetAbsAngles() );

				me->EmitSound( "Halloween.HammerImpact" );
				
				// after HHH punt off a kart target, pick a new target right away
				TFGameRules()->SetIT( NULL );
			}
			else if ( m_attackTarget != NULL )
			{
				AttackTarget( me, m_attackTarget, tf_halloween_bot_attack_range.GetFloat() );
			}

			// always playe the axe-hit-world impact sound, since it carries through the world better
			me->EmitSound( "Halloween.HeadlessBossAxeHitWorld" );
			if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
			{
				UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
			}

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
ActionResult< CHeadlessHatman >	CHeadlessHatmanAttack::Update( CHeadlessHatman *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done();
	}

	SelectVictim( me );
	RecomputeHomePosition();

	if ( !IsSwingingAxe() && m_laughTimer.IsElapsed() )
	{
		me->EmitSound( "Halloween.HeadlessBossLaugh" );
		m_laughTimer.Start( RandomFloat( 3.0f, 5.0f ) );

		if ( TFGameRules()->GetIT() == NULL )
		{
			// if we have no victim, show our frustration
			int r = RandomInt( 0, 100 );
			if ( r < 25 )
			{
				me->AddGesture( ACT_MP_GESTURE_VC_FISTPUMP_MELEE );
			}
			else if ( r < 50 )
			{
				me->AddGesture( ACT_MP_GESTURE_VC_FINGERPOINT_MELEE );
			}
		}
	}

	if ( TFGameRules()->GetIT() == NULL )
	{
		// go home
		const float atHomeRange = 50.0f;
		if ( me->IsRangeGreaterThan( m_homePos, atHomeRange ) )
		{
			if ( m_path.GetAge() > 3.0f )
			{
				CHeadlessHatmanPathCost cost( me );
				m_path.Compute( me, m_homePos, cost );
			}

			m_path.Update( me );
		}
		else // at home
		{
			if ( m_closestVisible != NULL )
			{
				// look at visible victim out of range
				me->GetLocomotionInterface()->FaceTowards( m_closestVisible->WorldSpaceCenter() );
			}
		}
	}
	else
	{
		// chase after our chase victim
		const float standAndSwingRange = 100.0f;
		CTFPlayer *chaseVictim = ToTFPlayer( TFGameRules()->GetIT() );

		if ( chaseVictim && m_warnITTimer.IsElapsed() && !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			m_warnITTimer.Start( 7.0f );
			ClientPrint( chaseVictim, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", chaseVictim->GetPlayerName() );		
		}

		if ( me->IsRangeGreaterThan( chaseVictim, standAndSwingRange ) || !me->IsLineOfSightClear( chaseVictim ) )
		{
			if ( m_path.GetAge() > 1.0f )
			{
				CHeadlessHatmanPathCost cost( me );
				m_path.Compute( me, chaseVictim, cost );
			}

			m_path.Update( me );
		}
	}

	// swing our axe at our attack target if they are in range
	if ( m_attackTarget != NULL && m_attackTarget->IsAlive() )
	{
		if ( me->IsRangeLessThan( m_attackTarget, tf_halloween_bot_attack_range.GetFloat() ) )
		{
			if ( m_scareTimer.IsElapsed() && m_attackTarget->IsPlayer() )
			{
				m_scareTimer.Reset();
				return SuspendFor( new CHeadlessHatmanTerrify, "Boo!" );
			}

			if ( m_attackTimer.IsElapsed() )
			{
				if ( !IsSwingingAxe() )
				{
					if ( TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
					{
						me->AddGesture( ACT_MP_ATTACK_STAND_ITEM2 );
					}
					else
					{
						me->AddGesture( ACT_MP_ATTACK_STAND_ITEM1 );
					}
					
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

		if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			if ( m_footfallTimer.IsElapsed() )
			{
				m_footfallTimer.Start( 0.25f );

				// periodically shake nearby players' screens when we're moving
				UTIL_ScreenShake( me->GetAbsOrigin(), 15.0f, 5.0f, 1.0f, 1000.0f, SHAKE_START );
			}
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
EventDesiredResult< CHeadlessHatman > CHeadlessHatmanAttack::OnStuck( CHeadlessHatman *me )
{
	// we're stuck - just warp to the our next path goal
	if ( m_path.GetCurrentGoal() )
	{
		me->SetAbsOrigin( m_path.GetCurrentGoal()->pos + Vector( 0, 0, 10.0f ) );
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHeadlessHatman > CHeadlessHatmanAttack::OnContact( CHeadlessHatman *me, CBaseEntity *other, CGameTrace *result )
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


//---------------------------------------------------------------------------------------------
void CHeadlessHatmanAttack::RecomputeHomePosition( void )
{
	if ( !m_homePosRecalcTimer.IsElapsed() )
	{
		return;
	}

	m_homePosRecalcTimer.Reset();

	CTeamControlPoint *contestedPoint = NULL;
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		for( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			contestedPoint = pMaster->GetControlPoint( i );
			if ( contestedPoint && pMaster->IsInRound( contestedPoint ) )
			{
				if ( ObjectiveResource()->GetOwningTeam( contestedPoint->GetPointIndex() ) == TF_TEAM_BLUE )
					continue;

				// blue are the invaders
				if ( !TeamplayGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, contestedPoint->GetPointIndex() ) )
					continue;

				break;
			}
		}
	}

	if ( contestedPoint )
	{
		m_homePos = contestedPoint->GetAbsOrigin();
	}
}


