//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_fetch_flag.cpp
// Go get the flag!
// Michael Booth, May 2011

#include "cbase.h"

#include "bot/tf_bot.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_fetch_flag.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_escort_flag_carrier.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_attack_flag_defenders.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_deliver_flag.h"


//---------------------------------------------------------------------------------------------
CTFBotFetchFlag::CTFBotFetchFlag( bool isTemporary )
{
	m_isTemporary = isTemporary;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotFetchFlag::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotFetchFlag::Update( CTFBot *me, float interval )
{
	CCaptureFlag *flag = me->GetFlagToFetch();

	if ( !flag )
	{
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			return SuspendFor( new CTFBotAttackFlagDefenders, "Flag flag exists - Attacking the enemy flag defenders" );
		}

		return Done( "No flag" );
	}

	// uncloak so we can attack
	if ( me->m_Shared.IsStealthed() )
	{
		me->PressAltFireButton();
	}

	if ( TFGameRules()->IsMannVsMachineMode() && flag->IsHome() )
	{
		if ( gpGlobals->curtime - me->GetSpawnTime() < 1.0f && me->GetTeamNumber() != TEAM_SPECTATOR )
		{
			// we just spawned - give us the flag
			flag->PickUp( me, true );
		}
		else
		{
			if ( m_isTemporary )
			{
				return Done( "Flag unreachable" );
			}

			// flag is at home and we're out in the world - can't reach it
			return SuspendFor( new CTFBotAttackFlagDefenders, "Flag unreachable at home - Attacking the enemy flag defenders" );
		}
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	CTFPlayer *carrier = ToTFPlayer( flag->GetOwnerEntity() );
	if ( carrier )
	{
		if ( m_isTemporary )
		{
			return Done( "Someone else picked up the flag" );
		}

		// NOTE: if I've picked up the flag, the ScenarioMonitor will handle it
		return SuspendFor( new CTFBotAttackFlagDefenders, "Someone has the flag - attacking the enemy defenders" );
	}

	// go pick up the flag
	if ( m_repathTimer.IsElapsed() )
	{
		CTFBotPathCost cost( me, DEFAULT_ROUTE );
		float maxPathLength = TFGameRules()->IsMannVsMachineMode() ? TFBOT_MVM_MAX_PATH_LENGTH : 0.0f;
		if ( m_path.Compute( me, flag->WorldSpaceCenter(), cost, maxPathLength ) == false )
		{
			if ( flag->IsDropped() )
			{
				// flag is unreachable - attack for awhile and hope someone else can dislodge it
				return SuspendFor( new CTFBotAttackFlagDefenders( RandomFloat( 5.0f, 10.0f ) ), "Flag unreachable - Attacking" );

				// just give it to me
				// flag->PickUp( me, true );
			}
		}

		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
// are we in a hurry?
QueryResultType CTFBotFetchFlag::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
// is it time to retreat?
QueryResultType	CTFBotFetchFlag::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}
