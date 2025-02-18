//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_escort_flag_carrier.cpp
// Escort the flag carrier to their destination
// Michael Booth, May 2011

#include "cbase.h"

#include "bot/tf_bot.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_escort_flag_carrier.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_attack_flag_defenders.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_deliver_flag.h"

extern ConVar tf_bot_flag_escort_range;

ConVar tf_bot_flag_escort_give_up_range( "tf_bot_flag_escort_give_up_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_flag_escort_max_count( "tf_bot_flag_escort_max_count", "4", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
// 
// Count the number of TFBots currently engaged in the "EscortFlagCarrier" behavior
//
int GetBotEscortCount( int team )
{
	int count = 0;

	CUtlVector< CTFPlayer * > livePlayerVector;
	CollectPlayers( &livePlayerVector, team, COLLECT_ONLY_LIVING_PLAYERS );

	int i;
	for( i=0; i<livePlayerVector.Count(); ++i )
	{
		CTFBot *bot = dynamic_cast< CTFBot * >( livePlayerVector[i] );
		if ( bot )
		{
			Behavior< CTFBot > *behavior = (Behavior< CTFBot > *)bot->GetIntentionInterface()->FirstContainedResponder();
			if ( behavior )
			{
				Action< CTFBot > *action = (Action< CTFBot > *)behavior->FirstContainedResponder();

				while( action && action->GetActiveChildAction() )
				{
					action = action->GetActiveChildAction();
				}

				if ( action && action->IsNamed( "EscortFlagCarrier" ) )
				{
					++count;
				}
			}
		}
	}

	return count;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEscortFlagCarrier::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotEscortFlagCarrier::Update( CTFBot *me, float interval )
{
	CCaptureFlag *flag = me->GetFlagToFetch();

	if ( !flag )
	{
		return Done( "No flag" );
	}

	CTFPlayer *carrier = ToTFPlayer( flag->GetOwnerEntity() );
	if ( !carrier )
	{
		return Done( "Flag was dropped" );
	}
	else if ( me->IsSelf( carrier ) )
	{
		return Done( "I picked up the flag!" );
	}

	// stay near the carrier
	if ( me->IsRangeGreaterThan( carrier, tf_bot_flag_escort_give_up_range.GetFloat() ) )
	{
		if ( me->SelectRandomReachableEnemy() )
		{
			// too far away - give up
			return ChangeTo( new CTFBotAttackFlagDefenders, "Too far from flag carrier - attack defenders!" );
		}
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
	if ( myWeapon && myWeapon->IsMeleeWeapon() )
	{
		if ( me->IsRangeLessThan( carrier, tf_bot_flag_escort_range.GetFloat() ) && me->IsLineOfSightClear( carrier ) )
		{
			ActionResult< CTFBot > result = m_meleeAttackAction.Update( me, interval );

			if ( result.IsContinue() )
			{
				// we have a melee target, and we're still reasonably close to the flag carrier
				return Continue();
			}
		}
	}

	if ( me->IsRangeGreaterThan( carrier, 0.5f * tf_bot_flag_escort_range.GetFloat() ) )
	{
		// move near carrier
		if ( m_repathTimer.IsElapsed() )
		{
			if ( GetBotEscortCount( me->GetTeamNumber() ) > tf_bot_flag_escort_max_count.GetInt() )
			{
				if ( me->SelectRandomReachableEnemy() )
				{
					return Done( "Too many flag escorts - giving up" );
				}
			}

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, carrier, cost );

			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}

		m_path.Update( me );
	}

	return Continue();
}
