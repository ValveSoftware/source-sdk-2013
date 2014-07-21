// FunctorUtils.h
// Useful functors
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _FUNCTOR_UTILS_H_
#define _FUNCTOR_UTILS_H_

#ifdef NEXT_BOT
#include "NextBotInterface.h"
#include "NextBotManager.h"
#endif // NEXT_BOT

//--------------------------------------------------------------------------------------------------------
/**
 * NOTE: The functors in this file should ideally be game-independent, 
 * and work for any Source based game
 */
//--------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------
/**
 * Count the number of living players on a given team (or TEAM_ANY)
 */
class LivePlayerCounter
{
public:
	static const bool EXCLUDE_BOTS = false;
	LivePlayerCounter( int team, bool includeBots = true )
	{
		m_team = team;
		m_includeBots = includeBots;
		m_count = 0;
	}

	bool operator() ( CBasePlayer *player )
	{
		if (player->IsAlive() && (m_team == TEAM_ANY || player->GetTeamNumber() == m_team))
		{
			if (m_includeBots || !player->IsBot())
			{
				++m_count;
			}
		}
		return true;
	}

	int GetCount( void ) const
	{
		return m_count;
	}

	int m_team;
	bool m_includeBots;
	int m_count;
};


//--------------------------------------------------------------------------------------------------------
/**
* Count the number of dead players on a given team (or TEAM_ANY)
*/
class DeadPlayerCounter
{
public:
	static const bool EXCLUDE_BOTS = false;
	DeadPlayerCounter( int team, bool includeBots = true )
	{
		m_team = team;
		m_includeBots = includeBots;
		m_count = 0;
	}

	bool operator() ( CBasePlayer *player )
	{
		if (!player->IsAlive() && (m_team == TEAM_ANY || player->GetTeamNumber() == m_team))
		{
			if (m_includeBots || !player->IsBot())
			{
				++m_count;
			}
		}
		return true;
	}

	int GetCount( void ) const
	{
		return m_count;
	}

	int m_team;
	bool m_includeBots;
	int m_count;
};


//--------------------------------------------------------------------------------------------------------
/**
* Count the number of players on a given team (or TEAM_ANY)
*/
class PlayerCounter
{
public:
	static const bool EXCLUDE_BOTS = false;
	PlayerCounter( int team, int lifeState = -1, bool includeBots = true )
	{
		m_team = team;
		m_includeBots = includeBots;
		m_count = 0;
		m_lifeState = lifeState;
	}

	bool operator() ( CBasePlayer *player )
	{
		if ((player->m_lifeState == m_lifeState || m_lifeState == -1) && (m_team == TEAM_ANY || player->GetTeamNumber() == m_team))
		{
			if (m_includeBots || !player->IsBot())
			{
				++m_count;
			}
		}
		return true;
	}

	int GetCount( void ) const
	{
		return m_count;
	}

	int m_lifeState;
	int m_team;
	bool m_includeBots;
	int m_count;
};


//--------------------------------------------------------------------------------------------------------
/**
 * Return the closest living player on the given team (or TEAM_ANY)
 */
class ClosestPlayerScan
{
public:
	static const bool EXCLUDE_BOTS = false;
	ClosestPlayerScan( const Vector &spot, int team, float maxRange = 0.0f, CBasePlayer *ignore = NULL, bool includeBots = true )
	{
		m_spot = spot;
		m_team = team;
		m_includeBots = includeBots;
		m_close = NULL;
		
		if ( maxRange > 0.0f )
		{
			m_closeRangeSq = maxRange * maxRange;
		}
		else
		{
			m_closeRangeSq = 999999999.9f;
		}
		
		m_ignore = ignore;
	}
	
	bool operator() ( CBasePlayer *player )
	{
		if (player == m_ignore)
			return true;
			
		if (player->IsAlive() && (m_team == TEAM_ANY || player->GetTeamNumber() == m_team))
		{
			if ( !m_includeBots && player->IsBot() )
				return true;

			Vector to = player->WorldSpaceCenter() - m_spot;
			float rangeSq = to.LengthSqr();
			if (rangeSq < m_closeRangeSq)
			{
				m_closeRangeSq = rangeSq;
				m_close = player;
			}
		}
		return true;
	}
	
	CBasePlayer *GetPlayer( void ) const
	{
		return m_close;
	}
	
	bool IsCloserThan( float range )
	{
		return (m_closeRangeSq < (range * range));
	}

	bool IsFartherThan( float range )
	{
		return (m_closeRangeSq > (range * range));
	}
	
	Vector m_spot;
	int m_team;
	bool m_includeBots;
	CBasePlayer *m_close;
	float m_closeRangeSq;
	CBasePlayer *m_ignore;
};


//--------------------------------------------------------------------------------------------------------
/**
* Return the closest living BaseCombatCharacter on the given team (or TEAM_ANY)
*/
class ClosestActorScan
{
public:
	ClosestActorScan( const Vector &spot, int team, float maxRange = 0.0f, CBaseCombatCharacter *ignore = NULL )
	{
		m_spot = spot;
		m_team = team;
		m_close = NULL;

		if ( maxRange > 0.0f )
		{
			m_closeRangeSq = maxRange * maxRange;
		}
		else
		{
			m_closeRangeSq = 999999999.9f;
		}

		m_ignore = ignore;
	}

	bool operator() ( CBaseCombatCharacter *actor )
	{
		if (actor == m_ignore)
			return true;

		if (actor->IsAlive() && (m_team == TEAM_ANY || actor->GetTeamNumber() == m_team))
		{
			Vector to = actor->WorldSpaceCenter() - m_spot;
			float rangeSq = to.LengthSqr();
			if (rangeSq < m_closeRangeSq)
			{
				m_closeRangeSq = rangeSq;
				m_close = actor;
			}
		}
		return true;
	}

	CBaseCombatCharacter *GetClosestActor( void ) const
	{
		return m_close;
	}

	bool IsClosestActorCloserThan( float range )
	{
		return (m_closeRangeSq < (range * range));
	}

	bool IsClosestActorFartherThan( float range )
	{
		return (m_closeRangeSq > (range * range));
	}

	Vector m_spot;
	int m_team;
	CBaseCombatCharacter *m_close;
	float m_closeRangeSq;
	CBaseCombatCharacter *m_ignore;
};


//--------------------------------------------------------------------------------------------------------
class CShowViewportPanel
{
	int m_team;
	const char *m_panelName;
	bool m_show;
	KeyValues *m_data;

public:
	CShowViewportPanel( int team, const char *panelName, bool show, KeyValues *data = NULL )
	{
		m_team = team;
		m_panelName = panelName;
		m_show = show;
		m_data = data;
	}

	bool operator() ( CBasePlayer *player )
	{
		if ( m_team != TEAM_ANY && m_team != player->GetTeamNumber() )
			return true;

		player->ShowViewPortPanel( m_panelName, m_show, m_data );
		return true;
	}
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Iterate each "actor" in the game, where an actor is a Player or NextBot
 */
template < typename Functor >
inline bool ForEachActor( Functor &func )
{
	// iterate all non-bot players
	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );

		if ( player == NULL )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( !player->IsPlayer() )
			continue;

		if ( !player->IsConnected() )
			continue;

#ifdef NEXT_BOT
		// skip bots - ForEachCombatCharacter will catch them
		INextBot *bot = player->MyNextBotPointer();
		if ( bot )
		{
			continue;
		}
#endif // NEXT_BOT

		if ( func( player ) == false )
		{
			return false;
		}
	}

#ifdef NEXT_BOT
	// iterate all NextBots
	return TheNextBots().ForEachCombatCharacter( func );
#else
	return true;
#endif // NEXT_BOT
}


//--------------------------------------------------------------------------------------------------------------
/**
 * The interface for functors for use with ForEachActor() that
 * want notification before iteration starts and after interation
 * is complete (successful or not).
 */
class IActorFunctor
{
public:
	virtual void OnBeginIteration( void )						{ }		// invoked once before iteration begins

	virtual bool operator() ( CBaseCombatCharacter *them ) = 0;

	virtual void OnEndIteration( bool allElementsIterated )		{ }		// invoked once after iteration is complete whether successful or not
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Iterate each "actor" in the game, where an actor is a Player or NextBot
 * Template specialization for IActorFunctors.
 */
template <>
inline bool ForEachActor( IActorFunctor &func )
{
	func.OnBeginIteration();

	bool isComplete = true;

	// iterate all non-bot players
	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );

		if ( player == NULL )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( !player->IsPlayer() )
			continue;

		if ( !player->IsConnected() )
			continue;

#ifdef NEXT_BOT
		// skip bots - ForEachCombatCharacter will catch them
		INextBot *bot = dynamic_cast< INextBot * >( player );
		if ( bot )
		{
			continue;
		}
#endif // NEXT_BOT

		if ( func( player ) == false )
		{
			isComplete = false;
			break;
		}
	}

#ifdef NEXT_BOT
	if ( !isComplete )
	{
		// iterate all NextBots
		isComplete = TheNextBots().ForEachCombatCharacter( func );
	}
#endif // NEXT_BOT

	func.OnEndIteration( isComplete );

	return isComplete;
}


//--------------------------------------------------------------------------------------------------------
class CTraceFilterOnlyClassname : public CTraceFilterSimple
{
public:
	CTraceFilterOnlyClassname( const IHandleEntity *passentity, const char *pchClassname, int collisionGroup ) :
		CTraceFilterSimple( passentity, collisionGroup ), m_pchClassname( pchClassname )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return false;

		return FClassnameIs( pEntity, m_pchClassname ) && CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:

	const char *m_pchClassname;
};


#endif // _FUNCTOR_UTILS_H_
