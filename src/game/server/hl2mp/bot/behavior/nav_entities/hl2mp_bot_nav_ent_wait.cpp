//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "nav_mesh.h"
#include "hl2mp_player.h"
#include "bot/hl2mp_bot.h"
#include "bot/behavior/nav_entities/hl2mp_bot_nav_ent_wait.h"

extern ConVar hl2mp_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
CHL2MPBotNavEntWait::CHL2MPBotNavEntWait( const CFuncNavPrerequisite *prereq )
{
	m_prereq = prereq;
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot > CHL2MPBotNavEntWait::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed before we started" );
	}

	m_timer.Start( m_prereq->GetTaskValue() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot > CHL2MPBotNavEntWait::Update( CHL2MPBot *me, float interval )
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


