//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_taunt.cpp
// Stand still and play a taunt animation
// Michael Booth, November 2009

#include "cbase.h"
#include "team.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_taunt.h"


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotTaunt::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	// wait a short random time so entire mob doesn't taunt in unison
	m_tauntTimer.Start( RandomFloat( 0, 1.0f ) );
	m_didTaunt = false;

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotTaunt::Update( CTFBot *me, float interval )
{
	if ( m_tauntTimer.IsElapsed() )
	{
		if ( m_didTaunt )
		{
			// Stop taunting after a while
			if ( m_tauntEndTimer.IsElapsed() && me->m_Shared.GetTauntIndex() == TAUNT_LONG )
			{
				me->EndLongTaunt();
			}

			if ( me->m_Shared.InCond( TF_COND_TAUNTING ) == false )
			{
				return Done( "Taunt finished" );
			}			
		}
		else
		{
			me->HandleTauntCommand();
			// Start a timer to end our taunt in case we're still going after awhile
			m_tauntEndTimer.Start( RandomFloat( 3.f, 5.f ) );	
			
			m_didTaunt = true;
		}
	}

	return Continue();
}

