//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_sap.cpp
// Sap nearby enemy buildings
// Michael Booth, June 2010

#include "cbase.h"
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "bot/tf_bot.h"
#include "bot/behavior/spy/tf_bot_spy_sap.h"
#include "bot/behavior/tf_bot_approach_object.h"
#include "bot/behavior/spy/tf_bot_spy_attack.h"

extern ConVar tf_bot_path_lookahead_range;
extern ConVar tf_bot_debug_spy;


//---------------------------------------------------------------------------------------------
CTFBotSpySap::CTFBotSpySap( CBaseObject *sapTarget )
{
	m_sapTarget = sapTarget;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpySap::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	me->StopLookingAroundForEnemies();

	// uncloak so we can sap
	if ( me->m_Shared.IsStealthed() )
	{
		me->PressAltFireButton();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpySap::Update( CTFBot *me, float interval )
{
	CBaseObject *newSapTarget = me->GetNearestKnownSappableTarget();

	if ( newSapTarget )
	{
		m_sapTarget = newSapTarget;
	}

	if ( m_sapTarget == NULL )
	{
		return Done( "Sap target gone" );
	}

	CTFPlayer *victim = NULL;

	CUtlVector< CKnownEntity > knownVector;
	me->GetVisionInterface()->CollectKnownEntities( &knownVector );

	for( int i=0; i<knownVector.Count(); ++i )
	{
		CTFPlayer *playerThreat = ToTFPlayer( knownVector[i].GetEntity() );
		if ( playerThreat && me->IsEnemy( playerThreat ) )
		{
			victim = playerThreat;
			break;
		}
	}

	// opportunistic backstab if engineer is between me and my sap target
	if ( victim && victim->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		const float nearbyRange = 150.0f;
		if ( m_sapTarget->GetOwner() == victim && me->IsRangeLessThan( victim, nearbyRange ) )
		{
			if ( me->IsEntityBetweenTargetAndSelf( victim, m_sapTarget ) )
			{
				return SuspendFor( new CTFBotSpyAttack( victim ), "Backstabbing the engineer before I sap his buildings" );
			}
		}
	}

	const float sapRange = 40.0f;

	if ( me->IsRangeLessThan( m_sapTarget, 2.0f * sapRange ) )
	{
		// switch to our sapper and spam it
		CBaseCombatWeapon *mySapper = me->Weapon_GetWeaponByType( TF_WPN_TYPE_BUILDING );
		if ( !mySapper )
		{
			return Done( "I have no sapper" );
		}

		me->Weapon_Switch( mySapper );

		// uncloak
		if ( me->m_Shared.IsStealthed() )
		{
			me->PressAltFireButton();
		}

		// sap our target
		me->GetBodyInterface()->AimHeadTowards( m_sapTarget, IBody::MANDATORY, 0.1f, NULL, "Aiming my sapper" );

		me->PressFireButton();
	}

	if ( me->IsRangeGreaterThan( m_sapTarget, sapRange ) )
	{
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			if ( m_path.Compute( me, m_sapTarget, cost ) == false )
			{
				return Done( "No path to sap target!" );
			}
		}

		m_path.Update( me );

		return Continue();
	}

	// if our target is sapped, look for other nearby buildings to sap
	if ( m_sapTarget->HasSapper() )
	{
		CBaseObject *nextTarget = me->GetNearestKnownSappableTarget();
		if ( nextTarget )
		{
			m_sapTarget = nextTarget;
		}
		else
		{
			// everything is sapped - explicitly attack nearby enemy Engineers
			if ( victim && victim->IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				return SuspendFor( new CTFBotSpyAttack( victim ), "Attacking an engineer" );
			}

			return Done( "All targets sapped" );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotSpySap::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	me->StartLookingAroundForEnemies();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotSpySap::OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	me->StartLookingAroundForEnemies();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotSpySap::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	me->StopLookingAroundForEnemies();

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSpySap::OnStuck( CTFBot *me )
{
	return TryDone( RESULT_CRITICAL, "I'm stuck, probably on a sapped building that hasn't exploded yet" );
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSpySap::ShouldAttack( const INextBot *meBot, const CKnownEntity *them ) const
{
	CTFBot *me = ToTFBot( meBot->GetEntity() );

	if ( m_sapTarget && !m_sapTarget->HasSapper() )
	{
		// mission not accomplished
		return ANSWER_NO;
	}

	if ( !me->m_Shared.InCond( TF_COND_DISGUISED ) &&
		 !me->m_Shared.InCond( TF_COND_DISGUISING ) &&
		 !me->m_Shared.IsStealthed() )
	{
		// our cover is blown!
		return ANSWER_YES;
	}

	// if we've sapped, attack
	return AreAllDangerousSentriesSapped( me ) ? ANSWER_YES : ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
// Don't avoid enemies when we're going in for the sap
QueryResultType CTFBotSpySap::IsHindrance( const INextBot *me, CBaseEntity *blocker ) const
{
	if ( m_sapTarget.Get() && me->IsRangeLessThan( m_sapTarget, 300.0f ) )
	{
		// we're almost to our sap target - don't avoid anyone
		return ANSWER_NO;
	}

	// avoid everyone while we move to our sap target
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotSpySap::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
bool CTFBotSpySap::AreAllDangerousSentriesSapped( CTFBot *me ) const
{
	CUtlVector< CKnownEntity > knownVector;
	me->GetVisionInterface()->CollectKnownEntities( &knownVector );

	for( int i=0; i<knownVector.Count(); ++i )
	{
		CBaseObject *enemyObject = dynamic_cast< CBaseObject * >( knownVector[i].GetEntity() );
		if ( enemyObject && enemyObject->ObjectType() == OBJ_SENTRYGUN && !enemyObject->HasSapper() && me->IsEnemy( enemyObject ) )
		{
			// this is an active enemy sentry, are we in range and line of fire?
			if ( me->IsRangeLessThan( enemyObject, SENTRY_MAX_RANGE ) && me->IsLineOfFireClear( enemyObject ) )
			{
				return false;
			}
		}
	}

	return true;
}


