//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_escort.cpp
// Move near an entity and protect it
// Michael Booth, April 2011

#include "cbase.h"

#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_escort.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/demoman/tf_bot_prepare_stickybomb_trap.h"
#include "bot/behavior/tf_bot_destroy_enemy_sentry.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_escort_range( "tf_bot_escort_range", "300", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CTFBotEscort::CTFBotEscort( CBaseEntity *who )
{
	SetWho( who );
}


//---------------------------------------------------------------------------------------------
void CTFBotEscort::SetWho( CBaseEntity *who )
{
	m_who = who;
}


//---------------------------------------------------------------------------------------------
CBaseEntity *CTFBotEscort::GetWho( void ) const
{
	return m_who;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEscort::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_pathToWho.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEscort::Update( CTFBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleInFOVNow() )
	{
		return SuspendFor( new CTFBotAttack, "Attacking nearby threat" );
	}
	else
	{
		// no enemy is visible - move near who we are escorting
		if ( m_who != NULL )
		{
			if ( me->IsRangeGreaterThan( m_who, tf_bot_escort_range.GetFloat() ) )
			{
				if ( m_repathTimer.IsElapsed() )
				{
					CTFBotPathCost cost( me, FASTEST_ROUTE );
					m_pathToWho.Compute( me, m_who->GetAbsOrigin(), cost );
					m_repathTimer.Start( RandomFloat( 2.0f, 3.0f ) );
				}

				m_pathToWho.Update( me );
			}
			else
			{
				if ( CTFBotPrepareStickybombTrap::IsPossible( me ) )
				{
					return SuspendFor( new CTFBotPrepareStickybombTrap, "Laying sticky bombs!" );
				}
			}
		}

		// destroy enemy sentry guns we've encountered
		if ( me->GetEnemySentry() && CTFBotDestroyEnemySentry::IsPossible( me ) )
		{
			return SuspendFor( new CTFBotDestroyEnemySentry, "Going after an enemy sentry to destroy it" );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEscort::OnStuck( CTFBot *me )
{
	m_repathTimer.Invalidate();

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEscort::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEscort::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotEscort::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotEscort::OnCommandApproach( CTFBot *me, const Vector &pos, float range )
{
	return TryContinue();
}
