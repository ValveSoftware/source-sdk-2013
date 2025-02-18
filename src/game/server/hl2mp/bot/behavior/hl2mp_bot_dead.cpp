//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "bot/hl2mp_bot.h"
#include "bot/behavior/hl2mp_bot_dead.h"
#include "bot/behavior/hl2mp_bot_behavior.h"

#include "nav_mesh.h"

extern void respawn( CBaseEntity* pEdict, bool fCopyCorpse );

//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotDead::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	m_deadTimer.Start();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotDead::Update( CHL2MPBot *me, float interval )
{
	if ( me->IsAlive() )
	{
		// how did this happen?
		return ChangeTo( new CHL2MPBotMainAction, "This should not happen!" );
	}

	if ( m_deadTimer.IsGreaterThen( 5.0f ) )
	{
		if ( me->HasAttribute( CHL2MPBot::REMOVE_ON_DEATH ) )
		{
			// remove dead bots
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", me->GetUserID() ) );
		}
		else if ( me->HasAttribute( CHL2MPBot::BECOME_SPECTATOR_ON_DEATH ) )
		{
			me->ChangeTeam( TEAM_SPECTATOR );
			return Done();
		}
	}

	// I want to respawn!
	// Some gentle massaging for bots to get unstuck if they are holding keys, etc.
	if ( me->m_lifeState == LIFE_DEAD || me->m_lifeState == LIFE_RESPAWNABLE )
	{
		if ( g_pGameRules->FPlayerCanRespawn( me ) )
		{
			respawn( me, !me->IsObserver() );
		}
	}

	return Continue();
}

