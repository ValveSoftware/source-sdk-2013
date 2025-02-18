//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_terrify.cpp
// The Halloween Boss leans over and yells "Boo!", terrifying nearby victims
// Michael Booth, October 2010

#include "cbase.h"

#include "tf_player.h"
#include "tf_team.h"

#include "../headless_hatman.h"
#include "headless_hatman_terrify.h"


//---------------------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CHeadlessHatmanTerrify::OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction )
{
	me->AddGesture( ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1 );

	m_booTimer.Start( 0.25f );
	m_scareTimer.Start( 0.75f );
	m_timer.Start( 1.25f );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CHeadlessHatmanTerrify::Update( CHeadlessHatman *me, float interval )
{
	if ( m_timer.IsElapsed() )
	{
		return Done();
	}

	if ( m_booTimer.HasStarted() && m_booTimer.IsElapsed() )
	{
		m_booTimer.Invalidate();
		me->EmitSound( "Halloween.HeadlessBossBoo" );
	}

	if ( m_scareTimer.IsElapsed() )
	{
		CUtlVector< CTFPlayer * > playerVector;
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

		for( int i=0; i<playerVector.Count(); ++i )
		{
			CTFPlayer *victim = playerVector[i];

			if ( me->IsRangeLessThan( victim, tf_halloween_bot_terrify_radius.GetFloat() ) )
			{
				if ( !IsWearingPumpkinHeadOrSaxtonMask( victim ) && me->IsLineOfSightClear( victim ) )
				{
					// scare them!
					const float scareTime = 2.0f;
					const float speedReduction = 0.0f;

					// "stun by trigger" results in the Halloween "yikes" effects
					int stunFlags = TF_STUN_LOSER_STATE | TF_STUN_BY_TRIGGER;
					victim->m_Shared.StunPlayer( scareTime, speedReduction, stunFlags, NULL );
				}
			}
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
bool CHeadlessHatmanTerrify::IsWearingPumpkinHeadOrSaxtonMask( CTFPlayer *player )
{
	const int pumpkinHeadHat = 278;
	const int saxtonMask = 277;

	for( int i=0; i<player->GetNumWearables(); ++i )
	{
		CEconWearable *wearable = player->GetWearable( i );
		if ( wearable && wearable->GetAttributeContainer() )
		{
			CEconItemView *item = wearable->GetAttributeContainer()->GetItem();
			if ( item && item->IsValid() )
			{
				if ( item->GetItemDefIndex() == pumpkinHeadHat || item->GetItemDefIndex() == saxtonMask )
				{
					return true;
				}
			}
		}
	}

	return false;
}
