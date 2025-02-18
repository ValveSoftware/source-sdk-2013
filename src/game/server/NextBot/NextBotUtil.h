// NextBotUtil.h
// Utilities for the NextBot system
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_UTIL_H_
#define _NEXT_BOT_UTIL_H_

#include "NextBotLocomotionInterface.h"
#include "nav_area.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"

//--------------------------------------------------------------------------------------------
/**
 * A simple filter interface for various NextBot queries
 */
class INextBotEntityFilter
{
public:
	// return true if the given entity passes this filter
	virtual bool IsAllowed( CBaseEntity *entity ) const = 0;
};


// trace filter callback functions. needed for use with the querycache/optimization functionality
bool VisionTraceFilterFunction( IHandleEntity *pServerEntity, int contentsMask );
bool IgnoreActorsTraceFilterFunction( IHandleEntity *pServerEntity, int contentsMask );


//--------------------------------------------------------------------------------------------
/**
 * Trace filter that skips all players and NextBots
 */
class NextBotTraceFilterIgnoreActors : public CTraceFilterSimple
{
public:
	NextBotTraceFilterIgnoreActors( const IHandleEntity *passentity, int collisionGroup ) : CTraceFilterSimple( passentity, collisionGroup, IgnoreActorsTraceFilterFunction )
	{
	}
};


//--------------------------------------------------------------------------------------------
/**
 * Trace filter that skips all players, NextBots, and non-LOS blockers
 */
class NextBotVisionTraceFilter : public CTraceFilterSimple
{
public:
	NextBotVisionTraceFilter( const IHandleEntity *passentity, int collisionGroup )	: CTraceFilterSimple( passentity, collisionGroup, VisionTraceFilterFunction )
	{
	}
};


//--------------------------------------------------------------------------------------------
/**
 * Trace filter that skips all NextBots, but includes Players
 */
class NextBotTraceFilterIgnoreNextBots : public CTraceFilterSimple
{
public:
	NextBotTraceFilterIgnoreNextBots( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask ) )
		{
			CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );
#ifdef TERROR
			CBasePlayer *player = ToBasePlayer( entity );
			if ( player && player->IsGhost() )
				return false;
#endif // TERROR

			return ( entity->MyNextBotPointer() == NULL );
		}
		return false;
	}
};


//--------------------------------------------------------------------------------------------
/**
 * Trace filter that obeys INextBot::IsAbleToBlockMovementOf()
 */
class NextBotTraceFilter : public CTraceFilterSimple
{
public:
	NextBotTraceFilter( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
		CBaseEntity *entity = const_cast<CBaseEntity *>(EntityFromEntityHandle( passentity ));
		m_passBot = entity->MyNextBotPointer();
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask ) )
		{
			CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );
#ifdef TERROR
			CBasePlayer *player = ToBasePlayer( entity );
			if ( player && player->IsGhost() )
				return false;
#endif // TERROR

			// Skip players on the same team - they're not solid to us, and we'll avoid them
			if ( entity->IsPlayer() && m_passBot && m_passBot->GetEntity() &&
				m_passBot->GetEntity()->GetTeamNumber() == entity->GetTeamNumber() )
				return false;

			INextBot *bot = entity->MyNextBotPointer();
			
			return ( !bot || bot->IsAbleToBlockMovementOf( m_passBot ) );
		}
		return false;
	}
	
	const INextBot *m_passBot;
};


//--------------------------------------------------------------------------------------------
/**
 * Trace filter that only hits players and NextBots
 */
class NextBotTraceFilterOnlyActors : public CTraceFilterSimple
{
public:
	NextBotTraceFilterOnlyActors( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask ) )
		{
			CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );

#ifdef TERROR
			CBasePlayer *player = ToBasePlayer( entity );
			if ( player && player->IsGhost() )
				return false;
#endif // TERROR

			return ( entity->MyNextBotPointer() || entity->IsPlayer() );
		}
		return false;
	}
};


//--------------------------------------------------------------------------------------------
/**
 * Trace filter that skips "traversable" entities.  The "when" argument creates
 * a temporal context for asking if an entity is IMMEDIATELY traversable (like thin
 * glass that just breaks as we walk through it) or EVENTUALLY traversable (like a
 * breakable object that will take some time to break through)
 */
class NextBotTraversableTraceFilter : public CTraceFilterSimple
{
public:
	NextBotTraversableTraceFilter( INextBot *bot, ILocomotion::TraverseWhenType when = ILocomotion::EVENTUALLY ) : CTraceFilterSimple( bot->GetEntity(), COLLISION_GROUP_NONE )
	{
		m_bot = bot;
		m_when = when;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );

		if ( m_bot->IsSelf( entity ) )
		{
			return false;
		}

		if ( CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask ) )
		{
			return !m_bot->GetLocomotionInterface()->IsEntityTraversable( entity, m_when );
		}

		return false;
	}

private:
	INextBot *m_bot;
	ILocomotion::TraverseWhenType m_when;
};


//---------------------------------------------------------------------------------------------
/**
 * Given a vector of entities, a nav area, and a max travel distance, return 
 * the entity that has the shortest travel distance.
 */
inline CBaseEntity *SelectClosestEntityByTravelDistance( INextBot *me, const CUtlVector< CBaseEntity * > &candidateEntities, CNavArea *startArea, float travelRange )
{
	// collect nearby walkable areas within travelRange
	CUtlVector< CNavArea * > nearbyAreaVector;
	CollectSurroundingAreas( &nearbyAreaVector, startArea, travelRange, me->GetLocomotionInterface()->GetStepHeight(), me->GetLocomotionInterface()->GetDeathDropHeight() );

	// find closest entity in the collected area set
	CBaseEntity *closeEntity = NULL;
	float closeTravelRange = FLT_MAX;

	for( int i=0; i<candidateEntities.Count(); ++i )
	{
		CBaseEntity *candidate = candidateEntities[i];

		CNavArea *area = TheNavMesh->GetNearestNavArea( candidate, GETNAVAREA_CHECK_LOS, 500.0f );

		if ( area && area->IsMarked() && area->GetCostSoFar() < closeTravelRange )
		{
			closeEntity = candidate;
			closeTravelRange = area->GetCostSoFar();
		}
	}

	return closeEntity;
}


#ifdef OBSOLETE
//--------------------------------------------------------------------------------------------
/**
 * Trace filter that skips "traversable" entities, but hits other Actors.
 * Used for obstacle avoidance.
 */
class NextBotMovementAvoidanceTraceFilter : public CTraceFilterSimple
{
public:
	NextBotMovementAvoidanceTraceFilter( INextBot *bot ) : CTraceFilterSimple( bot->GetEntity(), COLLISION_GROUP_NONE )
	{
		m_bot = bot;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *entity = EntityFromEntityHandle( pServerEntity );

#ifdef TERROR
		CBasePlayer *player = ToBasePlayer( entity );
		if ( player && player->IsGhost() )
			return false;
#endif // TERROR

		if ( m_bot->IsSelf( entity ) )
		{
			return false;
		}

		if ( CTraceFilterSimple::ShouldHitEntity( pServerEntity, contentsMask ) )
		{
			return !m_bot->GetLocomotionInterface()->IsEntityTraversable( entity, ILocomotion::IMMEDIATELY );
		}

		return false;
	}

private:
	INextBot *m_bot;
};
#endif


#endif // _NEXT_BOT_UTIL_H_
