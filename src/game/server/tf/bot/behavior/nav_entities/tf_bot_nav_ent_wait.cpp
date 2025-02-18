//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_nav_ent_wait.cpp
// Wait for awhile, as directed by nav entity
// Michael Booth, September 2009

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_wait.h"

extern ConVar tf_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
CTFBotNavEntWait::CTFBotNavEntWait( const CFuncNavPrerequisite *prereq )
{
	m_prereq = prereq;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotNavEntWait::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed before we started" );
	}

	m_timer.Start( m_prereq->GetTaskValue() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotNavEntWait::Update( CTFBot *me, float interval )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed" );
	}

	if ( !m_prereq->IsEnabled() )
	{
		return Done( "Prerequisite has been disabled" );
	}

	if ( m_timer.IsElapsed() )
	{
		return Done( "Wait time elapsed" );
	}

	return Continue();
}


