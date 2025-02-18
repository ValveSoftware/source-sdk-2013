//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_tactical_monitor.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_gamerules.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_tactical_monitor.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_get_off_me.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_wait_for_players.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_stunned.h"


ConVar tf_boss_alpha_get_off_me_duration( "tf_boss_alpha_get_off_me_duration", "3"/*, FCVAR_CHEAT */ );
ConVar tf_boss_alpha_stunned_duration( "tf_boss_alpha_stunned_duration", "10" );


//---------------------------------------------------------------------------------------------
Action< CBossAlpha > *CBossAlphaTacticalMonitor::InitialContainedAction( CBossAlpha *me )	
{
	if ( TFGameRules()->IsBossBattleMode() )
	{
		return new CBossAlphaWaitForPlayers;
	}

	return NULL;
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha > CBossAlphaTacticalMonitor::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{ 
	m_getOffMeTimer.Invalidate();

	return Continue(); 
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaTacticalMonitor::Update( CBossAlpha *me, float interval )
{
	if ( me->IsInCondition( CBossAlpha::STUNNED ) )
	{
		return SuspendFor( new CBossAlphaStunned( tf_boss_alpha_stunned_duration.GetFloat() ), "Ouch!" );
	}

	if ( !m_getOffMeTimer.HasStarted() )
	{
		CUtlVector< CTFPlayer * > onMeVector;
		me->CollectPlayersStandingOnMe( &onMeVector );

		if ( onMeVector.Count() )
		{
			// someone is standing on me - push them off soon
			m_getOffMeTimer.Start( tf_boss_alpha_get_off_me_duration.GetFloat() );
		}
	}
	else if ( m_getOffMeTimer.IsElapsed() )
	{
		if ( !me->IsBusy() )
		{
			m_getOffMeTimer.Invalidate();

			// if someone is still on me, push them off
			CUtlVector< CTFPlayer * > onMeVector;
			me->CollectPlayersStandingOnMe( &onMeVector );
			if ( onMeVector.Count() )
			{
				return SuspendFor( new CBossAlphaGetOffMe, "Get offa me!" );
			}
		}
	}

	return Continue();
}


#endif // TF_RAID_MODE
