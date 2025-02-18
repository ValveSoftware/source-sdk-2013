//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_wait_for_players.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_wait_for_players.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_guard_spot.h"


extern ConVar tf_boss_alpha_nuke_interval;

ConVar tf_boss_alpha_sleep( "tf_boss_alpha_sleep", "0"/*, FCVAR_CHEAT */ );


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaWaitForPlayers::OnStart( CBossAlpha *me, Action< CBossAlpha > *priorAction )
{
	me->AddCondition( CBossAlpha::BUSY );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaWaitForPlayers::Update( CBossAlpha *me, float interval )
{
	if ( tf_boss_alpha_sleep.GetBool() )
	{
		return Continue();
	}

	CBaseCombatCharacter *target = me->GetAttackTarget();
	if ( target )
	{
		return ChangeTo( new CBossAlphaGuardSpot, "I see you..." );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CBossAlphaWaitForPlayers::OnEnd( CBossAlpha *me, Action< CBossAlpha > *nextAction )
{
	me->RemoveCondition( CBossAlpha::BUSY );

	me->GetNukeTimer()->Start( tf_boss_alpha_nuke_interval.GetFloat() );
	me->GetGrenadeTimer()->Reset();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaWaitForPlayers::OnInjured( CBossAlpha *me, const CTakeDamageInfo &info )
{
	if ( tf_boss_alpha_sleep.GetBool() )
	{
		return TryContinue();
	}

	return TryChangeTo( new CBossAlphaGuardSpot, RESULT_CRITICAL, "Ouch!" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaWaitForPlayers::OnContact( CBossAlpha *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other && other->IsPlayer() && !tf_boss_alpha_sleep.GetBool() )
	{
		return TryChangeTo( new CBossAlphaGuardSpot, RESULT_CRITICAL, "Don't touch me" );
	}

	return TryContinue();
}

#endif // TF_RAID_MODE
