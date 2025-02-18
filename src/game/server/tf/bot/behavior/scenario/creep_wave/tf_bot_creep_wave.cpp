//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_creep_wave.cpp
// Move in a "creep wave" to the next available control point to capture it
// Michael Booth, August 2010

#include "cbase.h"

#ifdef TF_CREEP_MODE

#include "team.h"
#include "team_control_point_master.h"
#include "bot/tf_bot_manager.h"
#include "bot/tf_bot.h"
#include "bot/behavior/scenario/creep_wave/tf_bot_creep_wave.h"

ConVar tf_creep_aggro_range( "tf_creep_aggro_range", "250" );
ConVar tf_creep_give_up_range( "tf_creep_give_up_range", "300" );


CTFPlayer *FindNearestEnemy( CTFBot *me, float maxRange )
{
	CBasePlayer *closest = NULL;
	float closeRangeSq = maxRange * maxRange;

	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast< CBasePlayer * >( UTIL_PlayerByIndex( i ) );

		if ( !player )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( me->IsFriend( player ) )
			continue;

		if ( !player->IsAlive() )
			continue;

		float rangeSq = me->GetRangeSquaredTo( player );
		if ( rangeSq < closeRangeSq )
		{
			if ( me->IsLineOfFireClear( player ) )
			{
				closeRangeSq = rangeSq;
				closest = player;
			}
		}
	}

	return (CTFPlayer *)closest;
}



//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotCreepWave::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	me->StopLookingAroundForEnemies();
	m_stuckTimer.Invalidate();
	
	me->GetPlayerClass()->SetCustomModel( "models/bots/bot_heavy.mdl", USE_CLASS_ANIMATIONS );
	me->UpdateModel();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotCreepWave::Update( CTFBot *me, float interval )
{
	if ( !me->IsAlive() && me->StateGet() != TF_STATE_DYING )
	{
		// remove dead creeps for now
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", me->GetUserID() ) );
	}

	CBaseCombatWeapon *melee = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	if ( melee )
	{
		me->Weapon_Switch( melee );
	}

	CUtlVector< CTeamControlPoint * > captureVector;
	TFGameRules()->CollectCapturePoints( me, &captureVector );

	if ( captureVector.Count() == 0 )
	{
		return Continue();
	}

	if ( m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_path.Compute( me, captureVector[0]->WorldSpaceCenter(), cost );
	}

	m_path.Update( me );

	if ( m_stuckTimer.HasStarted() )
	{
		// juke and dodge to escape stuck situation
		switch( RandomInt( 0, 3 ) )
		{
		case 0:	me->PressBackwardButton(); break;
		case 1:	me->PressForwardButton(); break;
		case 2:	me->PressLeftButton(); break;
		case 3:	me->PressRightButton(); break;
		}
	}

	CTFPlayer *enemy = FindNearestEnemy( me, tf_creep_aggro_range.GetFloat() );
	if ( enemy )
	{
		me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_INCOMING );
		return SuspendFor( new CTFBotCreepAttack( enemy ), "Attacking nearby enemy" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCreepWave::OnKilled( CTFBot *me, const CTakeDamageInfo &info )
{
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && me->IsEnemy( info.GetAttacker() ) )
	{
		TheTFBots().OnCreepKilled( ToTFPlayer( info.GetAttacker() ) );
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCreepWave::OnStuck( CTFBot *me )
{
	m_stuckTimer.Start();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotCreepWave::OnUnStuck( CTFBot *me )
{
	m_stuckTimer.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CTFBotCreepAttack::CTFBotCreepAttack( CTFPlayer *victim )
{
	m_victim = victim;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotCreepAttack::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotCreepAttack::Update( CTFBot *me, float interval )
{
	if ( !me->IsAlive() && me->StateGet() != TF_STATE_DYING )
	{
		// remove dead creeps for now
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", me->GetUserID() ) );
	}

	if ( m_victim.Get() == NULL || !m_victim->IsAlive() )
	{
		me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_JEERS );
		return Done( "Killed victim" );
	}

	if ( me->IsRangeGreaterThan( m_victim, tf_creep_give_up_range.GetFloat() ) ||
		 !me->IsLineOfFireClear( m_victim ) )
	{
		me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_NEGATIVE );
		return Done( "Lost victim" );
	}

	CTFPlayer *newVictim = FindNearestEnemy( me, tf_creep_aggro_range.GetFloat() );
	if ( newVictim )
	{
		float newRangeSq = me->GetRangeSquaredTo( newVictim );
		float victimRangeSq = me->GetRangeSquaredTo( m_victim );

		if ( newRangeSq < victimRangeSq )
		{
			// switch to closer target
			m_victim = newVictim;
			me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_BATTLECRY );
		}
	}

	CBaseCombatWeapon *melee = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	if ( melee )
	{
		me->Weapon_Switch( melee );
	}

	me->GetBodyInterface()->AimHeadTowards( m_victim, IBody::CRITICAL, 0.2f, NULL, "Looking at enemy" );

	// swing weapon
	me->PressFireButton();

	// beeline towards our victim
	const float combatRange = 40.0f;
	if ( me->IsRangeGreaterThan( m_victim, combatRange ) )
	{
		me->GetLocomotionInterface()->Approach( m_victim->GetAbsOrigin() );
	}
	else
	{
		// juke and dodge to avoid interpenetration
		switch( RandomInt( 0, 3 ) )
		{
		case 0:	me->PressBackwardButton(); break;
		case 1:	me->PressForwardButton(); break;
		case 2:	me->PressLeftButton(); break;
		case 3:	me->PressRightButton(); break;
		}
	}

	return Continue();
}




#endif // TF_CREEP_MODE
