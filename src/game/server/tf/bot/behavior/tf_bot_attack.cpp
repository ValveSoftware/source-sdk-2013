//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_attack.cpp
// Attack our threat
// Michael Booth, February 2009

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_attack.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;
extern ConVar tf_bot_offense_must_push_time;


//---------------------------------------------------------------------------------------------
CTFBotAttack::CTFBotAttack( void ) : m_chasePath( ChasePath::LEAD_SUBJECT )
{
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotAttack::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
// head aiming and weapon firing is handled elsewhere - we just need to get into position to fight
ActionResult< CTFBot >	CTFBotAttack::Update( CTFBot *me, float interval )
{
	CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
	bool isUsingCloseRangeWeapon = ( myWeapon && ( myWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) || myWeapon->IsMeleeWeapon() ) );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat == NULL || threat->IsObsolete() || !me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return Done( "No threat" );
	}

	me->EquipBestWeaponForThreat( threat );

	if ( isUsingCloseRangeWeapon && threat->IsVisibleRecently() && me->IsRangeLessThan( threat->GetLastKnownPosition(), 1.1f * me->GetDesiredAttackRange() ) )
	{
		// circle around our victim
		if ( me->TransientlyConsistentRandomValue( 3.0f ) < 0.5f )
		{
			me->PressLeftButton();
		}
		else
		{
			me->PressRightButton();
		}
	}


	// pursue the threat. if not visible, go to the last known position
	if ( !threat->IsVisibleRecently() || 
		 me->IsRangeGreaterThan( threat->GetEntity()->GetAbsOrigin(), me->GetDesiredAttackRange() ) || 
		 !me->IsLineOfFireClear( threat->GetEntity()->EyePosition() ) )
	{
		if ( threat->IsVisibleRecently() )
		{
			if ( isUsingCloseRangeWeapon && !TFGameRules()->IsMannVsMachineMode() )	// all bots in MvM use the default route
			{
				CTFBotPathCost cost( me, SAFEST_ROUTE );
				m_chasePath.Update( me, threat->GetEntity(), cost );
			}
			else
			{
				CTFBotPathCost cost( me, DEFAULT_ROUTE );
				m_chasePath.Update( me, threat->GetEntity(), cost );
			}
		}
		else
		{
			// if we're at the threat's last known position and he's still not visible, we lost him
			m_chasePath.Invalidate();

			if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), 20.0f ) )
			{
				me->GetVisionInterface()->ForgetEntity( threat->GetEntity() );
				return Done( "I lost my target!" );
			}

			// look where we last saw him as we approach
			if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), me->GetMaxAttackRange() ) )
			{
				me->GetBodyInterface()->AimHeadTowards( threat->GetLastKnownPosition() + Vector( 0, 0, HumanEyeHeight ), IBody::IMPORTANT, 0.2f, NULL, "Looking towards where we lost sight of our victim" );
			}

			m_path.Update( me );

			if ( m_repathTimer.IsElapsed() )
			{
				//m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
				m_repathTimer.Start( RandomFloat( 3.0f, 5.0f ) );

				if ( isUsingCloseRangeWeapon && !TFGameRules()->IsMannVsMachineMode() )	// all bots in MvM use the default route
				{
					CTFBotPathCost cost( me, SAFEST_ROUTE );
					m_path.Compute( me, threat->GetLastKnownPosition(), cost );
				}
				else
				{
					CTFBotPathCost cost( me, DEFAULT_ROUTE );
					float maxPathLength = TFGameRules()->IsMannVsMachineMode() ? TFBOT_MVM_MAX_PATH_LENGTH : 0.0f;
					m_path.Compute( me, threat->GetLastKnownPosition(), cost, maxPathLength );
				}
			}
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotAttack::OnStuck( CTFBot *me )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotAttack::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotAttack::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotAttack::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotAttack::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

