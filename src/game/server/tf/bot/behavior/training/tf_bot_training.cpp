//========= Copyright Valve Corporation, All rights reserved. ============//
////////////////////////////////////////////////////////////////////////////////////////////////////
// tf_bot_training.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "team.h"
#include "bot/tf_bot.h"
#include "bot/map_entities/tf_bot_generator.h"
#include "bot/behavior/training/tf_bot_training.h"
#include "tf_obj_sentrygun.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

ActionResult< CTFBot > CTFDespawn::Update( CTFBot *me, float interval )
{
	// players need to be kicked, not deleted
	if ( me->GetEntity()->IsPlayer() )
	{
		CBasePlayer *player = dynamic_cast< CBasePlayer * >( me->GetEntity() );
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", player->GetUserID() ) );
	}
	else
	{
		UTIL_Remove( me->GetEntity() );
	}
	return Continue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ActionResult< CTFBot > CTFTrainingAttackSentryActionPoint::Update( CTFBot *me, float interval )
{
	CTFBotActionPoint* pActionPoint = me->GetActionPoint();
	if ( pActionPoint == NULL )
	{
		return Done();
	}

	if ( pActionPoint->IsWithinRange( me ) )
	{
		CObjectSentrygun *pSentrygun = me->GetEnemySentry();
		if ( pSentrygun )
		{
			me->GetBodyInterface()->AimHeadTowards( pSentrygun, IBody::MANDATORY, 1.0f, NULL, "Aiming at enemy sentry" );

			// because sentries are stationary, check if XY is on target to allow SelectTargetPoint() to adjust Z for grenades
			Vector toSentry = pSentrygun->WorldSpaceCenter() - me->EyePosition();
			toSentry.NormalizeInPlace();
			Vector forward;
			me->EyeVectors( &forward );
			
			if ( ( forward.x * toSentry.x + forward.y * toSentry.y ) > 0.95f )
			{			
				me->PressFireButton();
			}
		}
	}
	else
	{
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
			
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, pActionPoint->GetAbsOrigin(), cost );
		}
		
		m_path.Update( me );
	}

	return Continue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ActionResult< CTFBot > CTFGotoActionPoint::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_stayTimer.Invalidate();
	m_wasTeleported = false;

	return Continue();
}

ActionResult< CTFBot > CTFGotoActionPoint::Update( CTFBot *me, float interval )
{
	CTFBotActionPoint* pActionPoint = me->GetActionPoint();
	if ( pActionPoint == NULL )
	{
		return Done();
	}

	if ( pActionPoint->IsWithinRange( me ) )
	{
		// track if we ever get teleported during this process
		m_wasTeleported |= me->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT );

		// we're at the action point
		if ( m_stayTimer.HasStarted() == false )
		{
			// this method may cause us to become suspended for other actions
			pActionPoint->ReachedActionPoint( me );

			m_stayTimer.Start( pActionPoint->m_stayTime );
		}
		else if ( m_stayTimer.IsElapsed() )
		{
			me->SetActionPoint( dynamic_cast< CTFBotActionPoint * >( pActionPoint->m_moveGoal.Get() ) );
			return ChangeTo( new CTFGotoActionPoint, "Reached point, going to next" );
		}		
	}
	else if ( m_wasTeleported )
	{
		// we reached our action point, but were teleported far away.
		// presumably we've resumed, so just go to the next action point.
		me->SetActionPoint( dynamic_cast< CTFBotActionPoint * >( pActionPoint->m_moveGoal.Get() ) );
		return ChangeTo( new CTFGotoActionPoint, "Reached point, going to next" );
	}
	else
	{
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
			
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, pActionPoint->GetAbsOrigin(), cost );
		}
		
		m_path.Update( me );
	}
	return Continue();
}
