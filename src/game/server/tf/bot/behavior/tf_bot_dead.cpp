//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_dead.cpp
// Push up daisies
// Michael Booth, May 2009

#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_dead.h"
#include "bot/behavior/tf_bot_behavior.h"

#include "nav_mesh.h"


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDead::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_deadTimer.Start();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDead::Update( CTFBot *me, float interval )
{
	if ( me->IsAlive() )
	{
		// how did this happen?
		return ChangeTo( new CTFBotMainAction, "This should not happen!" );
	}

	if ( m_deadTimer.IsGreaterThen( 5.0f ) )
	{
		if ( me->HasAttribute( CTFBot::REMOVE_ON_DEATH ) )
		{
			// remove dead bots
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", me->GetUserID() ) );
		}
		else if ( me->HasAttribute( CTFBot::BECOME_SPECTATOR_ON_DEATH ) )
		{
			me->ChangeTeam( TEAM_SPECTATOR, false, true );
			return Done();
		}
	}

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() && me->GetTeamNumber() == TF_TEAM_RED )
	{
		// dead defenders go to spectator for recycling
		me->ChangeTeam( TEAM_SPECTATOR, false, true );
	}
#endif // TF_RAID_MODE

	return Continue();
}

