//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_guard_spot.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_player.h"
#include "nav_mesh/tf_nav_area.h"
#include "tf_projectile_rocket.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_guard_spot.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_chase_victim.h"


//-----------------------------------------------------------------------------------------------------
ActionResult< CBossAlpha > CBossAlphaGuardSpot::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	m_path.SetMinLookAheadDistance( 300.0f );

	me->GetBodyInterface()->StartActivity( ACT_MP_STAND_ITEM1 );
	me->SetHomePosition( me->GetAbsOrigin() );

	m_lookAtSpot = vec3_origin;

	return Continue();
}


//-----------------------------------------------------------------------------------------------------
ActionResult< CBossAlpha > CBossAlphaGuardSpot::Update( CBossAlpha *me, float interval )
{
	CBaseCombatCharacter *target = me->GetAttackTarget();
	if ( target )
	{
		if ( me->IsLineOfSightClear( target ) || me->IsPrisonerOfMinion( target ) )
		{
			return SuspendFor( new CBossAlphaChaseVictim( me->GetAttackTarget() ), "Get 'em!" );
		}
	}

	CBaseCombatCharacter *visible = me->GetNearestVisibleEnemy();
	if ( visible )
	{
		// look at visible victim out of range
		me->GetLocomotionInterface()->FaceTowards( visible->WorldSpaceCenter() );
	}

	const float atHomeRange = 50.0f;
	if ( me->IsRangeGreaterThan( me->GetHomePosition(), atHomeRange ) )
	{
		if ( m_path.GetAge() > 3.0f )
		{
			CBossAlphaPathCost cost( me );
			if ( m_path.Compute( me, me->GetHomePosition(), cost ) == false )
			{
				// can't reach guard post - just jump there for now
				me->Teleport( &me->GetHomePosition(), NULL, NULL );
			}
		}

		m_path.Update( me );
	}
	else
	{
		// on guard spot - look around
		if ( m_lookTimer.IsElapsed() )
		{
			m_lookTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFNavArea *myArea = (CTFNavArea *)me->GetLastKnownArea();
			if ( myArea )
			{
				const CUtlVector< CTFNavArea * > &invasionAreaVector = myArea->GetEnemyInvasionAreaVector( TF_TEAM_RED );

				if ( invasionAreaVector.Count() > 0 )
				{
					// try to not look directly at walls
					const float minGazeRange = 300.0f;
					const int retryCount = 20.0f;
					for( int r=0; r<retryCount; ++r )
					{
						int which = RandomInt( 0, invasionAreaVector.Count()-1 );
						Vector gazeSpot = invasionAreaVector[ which ]->GetRandomPoint() + Vector( 0, 0, 0.75f * HumanHeight );

						if ( me->IsRangeGreaterThan( gazeSpot, minGazeRange ) && me->GetVisionInterface()->IsLineOfSightClear( gazeSpot ) )
						{
							// use maxLookInterval so these looks override body aiming from path following
							m_lookAtSpot = gazeSpot;
							break;
						}
					}
				}
			}
		}

		me->GetLocomotionInterface()->FaceTowards( m_lookAtSpot );
	}

	if ( me->GetLocomotionInterface()->IsAttemptingToMove() )
	{
		// play running animation
		if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
		{
			me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
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


//-----------------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaGuardSpot::OnInjured( CBossAlpha *me, const CTakeDamageInfo &info )
{
	return TryContinue();
}


#endif // TF_RAID_MODE
