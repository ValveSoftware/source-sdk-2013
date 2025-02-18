//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_lurk.cpp
// Wait for victims
// Michael Booth, September 2011

#include "cbase.h"
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "bot/tf_bot.h"
#include "bot/behavior/spy/tf_bot_spy_lurk.h"
#include "bot/behavior/spy/tf_bot_spy_sap.h"
#include "bot/behavior/spy/tf_bot_spy_attack.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"
#include "bot/behavior/spy/tf_bot_spy_sap.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;
extern ConVar tf_bot_debug_spy;


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyLurk::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	// cloak 
	if ( !me->m_Shared.IsStealthed() )
	{
		me->PressAltFireButton();
	}

	// disguise as the enemy team
	me->DisguiseAsMemberOfEnemyTeam();

	m_lurkTimer.Start( RandomFloat( 3.0f, 5.0f ) );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyLurk::Update( CTFBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->GetEntity() )
	{
		CBaseObject *enemyObject = dynamic_cast< CBaseObject * >( threat->GetEntity() );
		if ( enemyObject && !enemyObject->HasSapper() && me->IsEnemy( enemyObject ) )
		{
			return SuspendFor( new CTFBotSpySap( enemyObject ), "Sapping an enemy object" );
		}
	}

	if ( me->GetEnemySentry() != NULL && !me->GetEnemySentry()->HasSapper() )
	{
		return SuspendFor( new CTFBotSpySap( me->GetEnemySentry() ), "Sapping a Sentry" );
	}

	if ( m_lurkTimer.IsElapsed() )
	{
		return Done( "Lost patience with my hiding spot" );
	}

	CTFNavArea *myArea = me->GetLastKnownArea();

	if ( !myArea )
	{
		return Continue();
	}

	// go after victims we've gotten behind
	if ( threat && threat->GetTimeSinceLastKnown() < 3.0f )
	{
		CTFPlayer *victim = ToTFPlayer( threat->GetEntity() );
		if ( victim )
		{
			if ( !victim->IsLookingTowards( me ) )
			{
				return ChangeTo( new CTFBotSpyAttack( victim ), "Going after a backstab victim" );
			}
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSpyLurk::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	return ANSWER_NO;
}
