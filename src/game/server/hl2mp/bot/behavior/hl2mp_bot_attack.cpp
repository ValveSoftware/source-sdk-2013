//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "team_control_point_master.h"
#include "bot/hl2mp_bot.h"
#include "bot/behavior/hl2mp_bot_attack.h"

#include "nav_mesh.h"

extern ConVar hl2mp_bot_path_lookahead_range;
extern ConVar hl2mp_bot_offense_must_push_time;

ConVar hl2mp_bot_aggressive( "hl2mp_bot_aggressive", "0", FCVAR_NONE );

//---------------------------------------------------------------------------------------------
CHL2MPBotAttack::CHL2MPBotAttack( void ) : m_chasePath( ChasePath::LEAD_SUBJECT )
{
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotAttack::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
// head aiming and weapon firing is handled elsewhere - we just need to get into position to fight
ActionResult< CHL2MPBot >	CHL2MPBotAttack::Update( CHL2MPBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	if ( threat == NULL || threat->IsObsolete() || !me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return Done( "No threat" );
	}

	if ( me->IsPropFreak() && me->Physcannon_GetHeldProp() == NULL )
	{
		// No prop? Oh no
		return Done( "Prop freak with no prop to throw!" );
	}

	CBaseHL2MPCombatWeapon* myWeapon = dynamic_cast< CBaseHL2MPCombatWeapon* >( me->GetActiveWeapon() );
	bool isUsingCloseRangeWeapon = me->IsCloseRange( myWeapon );
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

	bool bHasRangedWeapon = me->IsRanged( myWeapon );

	// Go after them!
	bool bAggressive = hl2mp_bot_aggressive.GetBool() &&
					   !bHasRangedWeapon &&
					   me->GetDifficulty() > CHL2MPBot::EASY;

	// pursue the threat. if not visible, go to the last known position
	if ( bAggressive ||
	     !threat->IsVisibleRecently() || 
		 me->IsRangeGreaterThan( threat->GetEntity()->GetAbsOrigin(), me->GetDesiredAttackRange() ) || 
		 !me->IsLineOfFireClear( threat->GetEntity()->EyePosition() ) )
	{
		if ( threat->IsVisibleRecently() )
		{
			if ( isUsingCloseRangeWeapon )
			{
				CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
				m_chasePath.Update( me, threat->GetEntity(), cost );
			}
			else
			{
				CHL2MPBotPathCost cost( me, DEFAULT_ROUTE );
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

				if ( isUsingCloseRangeWeapon )
				{
					CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
					m_path.Compute( me, threat->GetLastKnownPosition(), cost );
				}
				else
				{
					CHL2MPBotPathCost cost( me, DEFAULT_ROUTE );
					m_path.Compute( me, threat->GetLastKnownPosition(), cost );
				}
			}
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotAttack::OnStuck( CHL2MPBot *me )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotAttack::OnMoveToSuccess( CHL2MPBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CHL2MPBot > CHL2MPBotAttack::OnMoveToFailure( CHL2MPBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CHL2MPBotAttack::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CHL2MPBotAttack::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

