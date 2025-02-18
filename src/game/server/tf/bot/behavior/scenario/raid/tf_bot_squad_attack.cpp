//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_squad_attack.cpp
// Move and attack as a small, cohesive, group
// Michael Booth, October 2009

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "team.h"
#include "raid/tf_raid_logic.h"
#include "bot/tf_bot.h"
#include "bot/behavior/scenario/raid/tf_bot_wander.h"
#include "bot/behavior/scenario/raid/tf_bot_squad_attack.h"
#include "bot/behavior/medic/tf_bot_medic_heal.h"
#include "bot/behavior/tf_bot_move_to_vantage_point.h"


ConVar tf_squad_radius( "tf_squad_radius", "200", FCVAR_CHEAT );
ConVar tf_squad_debug( "tf_squad_debug", "0", FCVAR_CHEAT );
ConVar tf_raid_squad_vocalize_min_interval( "tf_raid_squad_vocalize_min_interval", "5", FCVAR_CHEAT );
ConVar tf_raid_squad_vocalize_max_interval( "tf_raid_squad_vocalize_max_interval", "8", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSquadAttack::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_vocalizeTimer.Start( RandomFloat( tf_raid_squad_vocalize_min_interval.GetFloat(), tf_raid_squad_vocalize_max_interval.GetFloat() ) );
	m_victim = NULL;

	return Continue();
}


//---------------------------------------------------------------------------------------------
// the leader is the slowest member of the squad
CTFBot *CTFBotSquadAttack::GetSquadLeader( CTFBot *me ) const
{
	CTFBot *leader = NULL;
	float leaderSpeed = FLT_MAX;

	CTFBotSquad *squad = me->GetSquad();
	CTFBotSquad::Iterator it;
	for( it = squad->GetFirstMember(); it != squad->InvalidIterator(); it = squad->GetNextMember( it ) )
	{
		CTFBot *bot = it();

		float speed = bot->GetPlayerClass()->GetMaxSpeed();

		if ( speed < leaderSpeed )
		{
			leader = bot;
			leaderSpeed = speed;
		}
	}

	return leader;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotSquadAttack::Update( CTFBot *me, float interval )
{
	if ( !me->IsInASquad() )
		return Done( "Not in a squad" );

	if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
	{
		return SuspendFor( new CTFBotMedicHeal );
	}

	CTFBot *leader = GetSquadLeader( me );
	CTFBotPathCost cost( me, FASTEST_ROUTE );

	if ( m_victim == NULL || m_victimConsiderTimer.IsElapsed() )
	{
		m_victimConsiderTimer.Start( 3.0f );

		m_victim = TFGameRules()->GetRaidLogic()->SelectRaiderToAttack();
	}

	if ( m_victim )
	{
		const float engageRange = 500.0f;
		if ( me->IsPlayerClass( TF_CLASS_PYRO ) || 
			 me->IsRangeGreaterThan( m_victim->GetAbsOrigin(), engageRange ) || 
			 !me->GetVisionInterface()->IsAbleToSee( m_victim, IVision::DISREGARD_FOV ) )
		{
			if ( me->IsSelf( leader ) || me->IsRangeLessThan( leader, tf_squad_radius.GetFloat() ) )
			{
				// chase down the enemy
				m_chasePath.Update( me, m_victim, cost );
			}
		}

		if ( !me->IsSelf( leader ) && me->IsRangeGreaterThan( leader, 1.25f * tf_squad_radius.GetFloat() ) )
		{
			// too far from leader - return to him
			m_chasePath.Update( me, leader, cost );
		}

		if ( tf_squad_debug.GetBool() && me->IsSelf( leader ) )
		{
			NDebugOverlay::Circle( me->GetAbsOrigin(), 20.0f, 255, 255, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );

			CTFBotSquad *squad = me->GetSquad();
			CTFBotSquad::Iterator it;
			for( it = squad->GetFirstMember(); it != squad->InvalidIterator(); it = squad->GetNextMember( it ) )
			{
				CTFBot *bot = it();

				if ( me->IsSelf( bot ) )
					continue;

				NDebugOverlay::Line( me->WorldSpaceCenter(), bot->WorldSpaceCenter(), 0, 255, 0, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}
	}

	if ( m_vocalizeTimer.IsElapsed() )
	{
		m_vocalizeTimer.Start( RandomFloat( tf_raid_squad_vocalize_min_interval.GetFloat(), tf_raid_squad_vocalize_max_interval.GetFloat() ) );

		if ( me->IsPlayerClass( TF_CLASS_SCOUT ) )
			me->EmitSound( "Scout.MobJabber" );
		else if ( me->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			me->EmitSound( "Heavy.MobJabber" );
		else
			me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotSquadAttack::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_path.Invalidate();
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSquadAttack::OnStuck( CTFBot *me )
{
	m_path.Invalidate();
	return TryContinue();
}

#endif // TF_RAID_MODE
