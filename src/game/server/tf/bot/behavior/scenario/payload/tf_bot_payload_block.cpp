//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_payload_block.cpp
// Prevent the other team from moving the cart
// Michael Booth, April 2010

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "team_control_point_master.h"
#include "team_train_watcher.h"
#include "bot/tf_bot.h"
#include "bot/behavior/scenario/payload/tf_bot_payload_block.h"
#include "bot/behavior/medic/tf_bot_medic_heal.h"
#include "bot/behavior/engineer/tf_bot_engineer_build.h"


extern ConVar tf_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotPayloadBlock::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_path.Invalidate();

	m_giveUpTimer.Start( RandomFloat( 3.0f, 5.0f ) );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotPayloadBlock::Update( CTFBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_giveUpTimer.IsElapsed() )
	{
		return Done( "Been blocking long enough" );
	}

	// move toward the point, periodically repathing to account for changing situation
	if ( m_repathTimer.IsElapsed() )
	{
		VPROF_BUDGET( "CTFBotPayloadBlock::Update( repath )", "NextBot" );
		
		CTeamTrainWatcher *trainWatcher = TFGameRules()->GetPayloadToBlock( me->GetTeamNumber() );
		if ( !trainWatcher )
		{
			return Done( "Train Watcher is missing" );
		}

		CBaseEntity *cart = trainWatcher->GetTrainEntity();
		if ( !cart )
		{
			return Done( "Cart is missing" );
		}

		CTFBotPathCost cost( me, DEFAULT_ROUTE );
		m_path.Compute( me, cart->WorldSpaceCenter(), cost );
		m_repathTimer.Start( RandomFloat( 0.2f, 0.4f ) );
	}

	// move towards next capture point
	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotPayloadBlock::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	VPROF_BUDGET( "CTFBotPayloadBlock::OnResume", "NextBot" );

	m_repathTimer.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotPayloadBlock::OnStuck( CTFBot *me )
{
	VPROF_BUDGET( "CTFBotPayloadBlock::OnStuck", "NextBot" );

	m_repathTimer.Invalidate();
	me->GetLocomotionInterface()->ClearStuckStatus();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotPayloadBlock::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotPayloadBlock::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	VPROF_BUDGET( "CTFBotPayloadBlock::OnMoveToFailure", "NextBot" );

	m_repathTimer.Invalidate();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotPayloadBlock::OnTerritoryContested( CTFBot *me, int territoryID )
{
	return TryToSustain( RESULT_IMPORTANT );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotPayloadBlock::OnTerritoryCaptured( CTFBot *me, int territoryID )
{
	return TryToSustain( RESULT_IMPORTANT );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotPayloadBlock::OnTerritoryLost( CTFBot *me, int territoryID )
{
	return TryToSustain( RESULT_IMPORTANT );
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotPayloadBlock::ShouldRetreat( const INextBot *bot ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotPayloadBlock::ShouldHurry( const INextBot *bot ) const
{
	// hurry and block the cart - don't retreat, etc
	return ANSWER_YES;
}

