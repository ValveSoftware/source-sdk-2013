//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "vprof.h"

#include "hl2mp_bot.h"
#include "hl2mp_bot_vision.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"

ConVar hl2mp_bot_choose_target_interval( "hl2mp_bot_choose_target_interval", "0.3f", FCVAR_CHEAT, "How often, in seconds, a HL2MPBot can reselect his target" );
ConVar hl2mp_bot_sniper_choose_target_interval( "hl2mp_bot_sniper_choose_target_interval", "3.0f", FCVAR_CHEAT, "How often, in seconds, a zoomed-in Sniper can reselect his target" );

extern ConVar hl2mp_bot_ignore_real_players;

//------------------------------------------------------------------------------------------
void CHL2MPBotVision::CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity* >* potentiallyVisible )
{
	VPROF_BUDGET( "CHL2MPBotVision::CollectPotentiallyVisibleEntities", "NextBot" );

	potentiallyVisible->RemoveAll();

	// include all players
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer* player = UTIL_PlayerByIndex( i );

		if ( player == NULL )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( !player->IsPlayer() )
			continue;

		if ( !player->IsConnected() )
			continue;

		if ( !player->IsAlive() )
			continue;

		if ( hl2mp_bot_ignore_real_players.GetBool() )
		{
			if ( !player->IsBot() )
				continue;
		}

		potentiallyVisible->AddToTail( player );
	}

	// include sentry guns
	UpdatePotentiallyVisibleNPCVector();

	FOR_EACH_VEC( m_potentiallyVisibleNPCVector, it )
	{
		potentiallyVisible->AddToTail( m_potentiallyVisibleNPCVector[it] );
	}
}


//------------------------------------------------------------------------------------------
void CHL2MPBotVision::UpdatePotentiallyVisibleNPCVector( void )
{
	if ( m_potentiallyVisibleUpdateTimer.IsElapsed() )
	{
		m_potentiallyVisibleUpdateTimer.Start( RandomFloat( 3.0f, 4.0f ) );

		// collect list of active buildings
		m_potentiallyVisibleNPCVector.RemoveAll();

		CUtlVector< INextBot* > botVector;
		TheNextBots().CollectAllBots( &botVector );
		for ( int i = 0; i < botVector.Count(); ++i )
		{
			CBaseCombatCharacter* botEntity = botVector[i]->GetEntity();
			if ( botEntity && !botEntity->IsPlayer() )
			{
				// NPC
				m_potentiallyVisibleNPCVector.AddToTail( botEntity );
			}
		}
	}
}


//------------------------------------------------------------------------------------------
/**
 * Return true to completely ignore this entity.
 * This is mostly for enemy spies.  If we don't ignore them, we will look at them.
 */
bool CHL2MPBotVision::IsIgnored( CBaseEntity* subject ) const
{
	CHL2MPBot* me = ( CHL2MPBot* )GetBot()->GetEntity();

	if ( me->IsAttentionFocused() )
	{
		// our attention is restricted to certain subjects
		if ( !me->IsAttentionFocusedOn( subject ) )
		{
			return false;
		}
	}

	if ( !me->IsEnemy( subject ) )
	{
		// don't ignore friends
		return false;
	}

	if ( subject->IsEffectActive( EF_NODRAW ) )
	{
		return true;
	}

	return false;
}


//------------------------------------------------------------------------------------------
// Return VISUAL reaction time
float CHL2MPBotVision::GetMinRecognizeTime( void ) const
{
	CHL2MPBot* me = ( CHL2MPBot* )GetBot();

	switch ( me->GetDifficulty() )
	{
	case CHL2MPBot::EASY:	return 1.0f;
	case CHL2MPBot::NORMAL:	return 0.5f;
	case CHL2MPBot::HARD:	return 0.3f;
	case CHL2MPBot::EXPERT:	return 0.2f;
	}

	return 1.0f;
}



//------------------------------------------------------------------------------------------
float CHL2MPBotVision::GetMaxVisionRange( void ) const
{
	CHL2MPBot *me = (CHL2MPBot *)GetBot();

	if ( me->GetMaxVisionRangeOverride() > 0.0f )
	{
		// designer specified vision range
		return me->GetMaxVisionRangeOverride();
	}

	// long range, particularly for snipers
	return 6000.0f;
}
