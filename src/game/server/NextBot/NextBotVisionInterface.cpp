// NextBotVisionInterface.cpp
// Implementation of common vision system
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "nav.h"
#include "functorutils.h"

#include "NextBot.h"
#include "NextBotVisionInterface.h"
#include "NextBotBodyInterface.h"
#include "NextBotUtil.h"

#ifdef TERROR
#include "querycache.h"
#endif

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar nb_blind( "nb_blind", "0", FCVAR_CHEAT, "Disable vision" );
ConVar nb_debug_known_entities( "nb_debug_known_entities", "0", FCVAR_CHEAT, "Show the 'known entities' for the bot that is the current spectator target" );


//------------------------------------------------------------------------------------------
IVision::IVision( INextBot *bot ) : INextBotComponent( bot )
{ 
	Reset();
}


//------------------------------------------------------------------------------------------
/**
 * Reset to initial state
 */
void IVision::Reset( void )
{
	INextBotComponent::Reset();

	m_knownEntityVector.RemoveAll();
	m_lastVisionUpdateTimestamp = 0.0f;
	m_primaryThreat = NULL;

	m_FOV = GetDefaultFieldOfView();
	m_cosHalfFOV = cos( 0.5f * m_FOV * M_PI / 180.0f );
	
	for( int i=0; i<MAX_TEAMS; ++i )
	{
		m_notVisibleTimer[i].Invalidate();
	}
}


//------------------------------------------------------------------------------------------
/**
 * Ask the current behavior to select the most dangerous threat from
 * our set of currently known entities
 * TODO: Find a semantically better place for this to live.
 */
const CKnownEntity *IVision::GetPrimaryKnownThreat( bool onlyVisibleThreats ) const
{
	if ( m_knownEntityVector.Count() == 0 )
		return NULL;

	const CKnownEntity *threat = NULL;
	int i;

	// find the first valid entity
	for( i=0; i<m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &firstThreat = m_knownEntityVector[i];

		// check in case status changes between updates
		if ( IsAwareOf( firstThreat ) && !firstThreat.IsObsolete() && !IsIgnored( firstThreat.GetEntity() ) && GetBot()->IsEnemy( firstThreat.GetEntity() ) )
		{
			if ( !onlyVisibleThreats || firstThreat.IsVisibleRecently() )
			{
				threat = &firstThreat;
				break;
			}
		}
	}

	if ( threat == NULL )
	{
		m_primaryThreat = NULL;
		return NULL;
	}

	for( ++i; i<m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &newThreat = m_knownEntityVector[i];

		// check in case status changes between updates
		if ( IsAwareOf( newThreat ) && !newThreat.IsObsolete() && !IsIgnored( newThreat.GetEntity() ) && GetBot()->IsEnemy( newThreat.GetEntity() ) )
		{
			if ( !onlyVisibleThreats || newThreat.IsVisibleRecently() )
			{
				threat = GetBot()->GetIntentionInterface()->SelectMoreDangerousThreat( GetBot(), GetBot()->GetEntity(), threat, &newThreat );
			}
		}
	}

	// cache off threat
	m_primaryThreat = threat ? threat->GetEntity() : NULL;

	return threat;
}


//------------------------------------------------------------------------------------------
/**
 * Return the closest recognized entity
 */
const CKnownEntity *IVision::GetClosestKnown( int team ) const
{
	const Vector &myPos = GetBot()->GetPosition();
	
	const CKnownEntity *close = NULL;
	float closeRange = 999999999.9f;
	
	for( int i=0; i < m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &known = m_knownEntityVector[i];

		if ( !known.IsObsolete() && IsAwareOf( known ) )
		{
			if ( team == TEAM_ANY || known.GetEntity()->GetTeamNumber() == team )
			{
				Vector to = known.GetLastKnownPosition() - myPos;
				float rangeSq = to.LengthSqr();
				
				if ( rangeSq < closeRange )
				{
					close = &known;
					closeRange = rangeSq;
				}
			}
		}
	}
	
	return close;
}


//------------------------------------------------------------------------------------------
/**
 * Return the closest recognized entity that passes the given filter
 */
const CKnownEntity *IVision::GetClosestKnown( const INextBotEntityFilter &filter ) const
{
	const Vector &myPos = GetBot()->GetPosition();

	const CKnownEntity *close = NULL;
	float closeRange = 999999999.9f;

	for( int i=0; i < m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &known = m_knownEntityVector[i];

		if ( !known.IsObsolete() && IsAwareOf( known ) )
		{
			if ( filter.IsAllowed( known.GetEntity() ) )
			{
				Vector to = known.GetLastKnownPosition() - myPos;
				float rangeSq = to.LengthSqr();

				if ( rangeSq < closeRange )
				{
					close = &known;
					closeRange = rangeSq;
				}
			}
		}
	}

	return close;	
}


//------------------------------------------------------------------------------------------
/**
 * Given an entity, return our known version of it (or NULL if we don't know of it)
 */
const CKnownEntity *IVision::GetKnown( const CBaseEntity *entity ) const
{
	if ( entity == NULL )
		return NULL;

	for( int i=0; i < m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &known = m_knownEntityVector[i];

		if ( known.GetEntity() && known.GetEntity()->entindex() == entity->entindex() && !known.IsObsolete() )
		{
			return &known;
		}
	}

	return NULL;
}


//------------------------------------------------------------------------------------------
/**
 * Introduce a known entity into the system. Its position is assumed to be known
 * and will be updated, and it is assumed to not yet have been seen by us, allowing for learning
 * of known entities by being told about them, hearing them, etc.
 */
void IVision::AddKnownEntity( CBaseEntity *entity )
{
	if ( entity == NULL || entity->IsWorld() )
	{
		// the world is not an entity we can deal with
		return;
	}

	CKnownEntity known( entity );

	// only add it if we don't already know of it
	if ( m_knownEntityVector.Find( known ) == m_knownEntityVector.InvalidIndex() )
	{
		m_knownEntityVector.AddToTail( known );
	}
}


//------------------------------------------------------------------------------------------
// Remove the given entity from our awareness (whether we know if it or not)
// Useful if we've moved to where we last saw the entity, but it's not there any longer.
void IVision::ForgetEntity( CBaseEntity *forgetMe )
{
	if ( !forgetMe )
		return;

	FOR_EACH_VEC( m_knownEntityVector, it )
	{
		const CKnownEntity &known = m_knownEntityVector[ it ];

		if ( known.GetEntity() && known.GetEntity()->entindex() == forgetMe->entindex() )
		{
			m_knownEntityVector.FastRemove( it );
			return;
		}
	}
}


//------------------------------------------------------------------------------------------
void IVision::ForgetAllKnownEntities( void )
{
	m_knownEntityVector.RemoveAll();
}


//------------------------------------------------------------------------------------------
/**
 * Return the number of entity on the given team known to us closer than rangeLimit
 */
int IVision::GetKnownCount( int team, bool onlyVisible, float rangeLimit ) const
{
	int count = 0;

	FOR_EACH_VEC( m_knownEntityVector, it )
	{
		const CKnownEntity &known = m_knownEntityVector[ it ];

		if ( !known.IsObsolete() && IsAwareOf( known ) )
		{
			if ( team == TEAM_ANY || known.GetEntity()->GetTeamNumber() == team )
			{
				if ( !onlyVisible || known.IsVisibleRecently() )
				{
					if ( rangeLimit < 0.0f || GetBot()->IsRangeLessThan( known.GetLastKnownPosition(), rangeLimit ) )
					{
						++count;
					}
				}
			}
		}
	}

	return count;
}


//------------------------------------------------------------------------------------------
class PopulateVisibleVector
{
public:
	PopulateVisibleVector( CUtlVector< CBaseEntity * > *potentiallyVisible )
	{
		m_potentiallyVisible = potentiallyVisible;
	}

	bool operator() ( CBaseEntity *actor )
	{
		m_potentiallyVisible->AddToTail( actor );
		return true;
	}

	CUtlVector< CBaseEntity * > *m_potentiallyVisible;
};


//------------------------------------------------------------------------------------------
/**
 * Populate "potentiallyVisible" with the set of all entities we could potentially see. 
 * Entities in this set will be tested for visibility/recognition in IVision::Update()
 */
void IVision::CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity * > *potentiallyVisible )
{
	potentiallyVisible->RemoveAll();

	// by default, only consider players and other bots as potentially visible
	PopulateVisibleVector populate( potentiallyVisible );
	ForEachActor( populate );
}


//------------------------------------------------------------------------------------------
class CollectVisible
{
public:
	CollectVisible( IVision *vision )
	{
		m_vision = vision;
	}
	
	bool operator() ( CBaseEntity *entity )
	{
		if ( entity &&
			 !m_vision->IsIgnored( entity ) &&
			 entity->IsAlive() &&
			 entity != m_vision->GetBot()->GetEntity() &&
			 m_vision->IsAbleToSee( entity, IVision::USE_FOV ) )
		{
			m_recognized.AddToTail( entity );	
		}
			
		return true;
	}
	
	bool Contains( CBaseEntity *entity ) const
	{
		for( int i=0; i < m_recognized.Count(); ++i )
		{
			if ( entity->entindex() == m_recognized[ i ]->entindex() )
			{
				return true;
			}
		}
		return false;
	}
	
	IVision *m_vision;
	CUtlVector< CBaseEntity * > m_recognized;
};


//------------------------------------------------------------------------------------------
void IVision::UpdateKnownEntities( void )
{
	VPROF_BUDGET( "IVision::UpdateKnownEntities", "NextBot" );

	// construct set of potentially visible objects
	CUtlVector< CBaseEntity * > potentiallyVisible;
	CollectPotentiallyVisibleEntities( &potentiallyVisible );

	// collect set of visible and recognized entities at this moment
	CollectVisible visibleNow( this );
	FOR_EACH_VEC( potentiallyVisible, pit )
	{
		VPROF_BUDGET( "IVision::UpdateKnownEntities( collect visible )", "NextBot" );

		if ( visibleNow( potentiallyVisible[ pit ] ) == false )
			break;
	}
	
	// update known set with new data
	{	VPROF_BUDGET( "IVision::UpdateKnownEntities( update status )", "NextBot" );

		int i;
		for( i=0; i < m_knownEntityVector.Count(); ++i )
		{
			CKnownEntity &known = m_knownEntityVector[i];

			// clear out obsolete knowledge
			if ( known.GetEntity() == NULL || known.IsObsolete() )
			{
				m_knownEntityVector.Remove( i );
				--i;
				continue;
			}
			
			if ( visibleNow.Contains( known.GetEntity() ) )
			{
				// this visible entity was already known (but perhaps not visible until now)
				known.UpdatePosition();
				known.UpdateVisibilityStatus( true );

				// has our reaction time just elapsed?
				if ( gpGlobals->curtime - known.GetTimeWhenBecameVisible() >= GetMinRecognizeTime() &&
					 m_lastVisionUpdateTimestamp - known.GetTimeWhenBecameVisible() < GetMinRecognizeTime() )
				{
					if ( GetBot()->IsDebugging( NEXTBOT_VISION ) )
					{
						ConColorMsg( Color( 0, 255, 0, 255 ), "%3.2f: %s caught sight of %s(#%d)\n", 
										gpGlobals->curtime,
										GetBot()->GetDebugIdentifier(),
										known.GetEntity()->GetClassname(),
										known.GetEntity()->entindex() );

						NDebugOverlay::Line( GetBot()->GetBodyInterface()->GetEyePosition(), known.GetLastKnownPosition(), 255, 255, 0, false, 0.2f );
					}

					GetBot()->OnSight( known.GetEntity() );
				}
		
				// restart 'not seen' timer
				m_notVisibleTimer[ known.GetEntity()->GetTeamNumber() ].Start();
			}
			else // known entity is not currently visible
			{
				if ( known.IsVisibleInFOVNow() )
				{
					// previously known and visible entity is now no longer visible
					known.UpdateVisibilityStatus( false );

					// lost sight of this entity
					if ( GetBot()->IsDebugging( NEXTBOT_VISION ) )
					{
						ConColorMsg( Color( 255, 0, 0, 255 ), "%3.2f: %s Lost sight of %s(#%d)\n", 
										gpGlobals->curtime,
										GetBot()->GetDebugIdentifier(),
										known.GetEntity()->GetClassname(),
										known.GetEntity()->entindex() );
					}

					GetBot()->OnLostSight( known.GetEntity() );
				}

				if ( !known.HasLastKnownPositionBeenSeen() )
				{
					// can we see the entity's last know position?
					if ( IsAbleToSee( known.GetLastKnownPosition(), IVision::USE_FOV ) )
					{
						known.MarkLastKnownPositionAsSeen();
					}
				}
			}
		}
	}
		
	// check for new recognizes that were not in the known set
	{	VPROF_BUDGET( "IVision::UpdateKnownEntities( new recognizes )", "NextBot" );

		int i, j;
		for( i=0; i < visibleNow.m_recognized.Count(); ++i )
		{	
			for( j=0; j < m_knownEntityVector.Count(); ++j )
			{
				if ( visibleNow.m_recognized[i] == m_knownEntityVector[j].GetEntity() )
				{
					break;
				}
			}
			
			if ( j == m_knownEntityVector.Count() )
			{
				// recognized a previously unknown entity (emit OnSight() event after reaction time has passed)
				CKnownEntity known( visibleNow.m_recognized[i] );
				known.UpdatePosition();
				known.UpdateVisibilityStatus( true );
				m_knownEntityVector.AddToTail( known );
			}
		}
	}

	// debugging
	if ( nb_debug_known_entities.GetBool() )
	{
		CBasePlayer *watcher = UTIL_GetListenServerHost();
		if ( watcher )
		{
			CBaseEntity *subject = watcher->GetObserverTarget();

			if ( subject && GetBot()->IsSelf( subject ) )
			{
				CUtlVector< CKnownEntity > knownVector;
				CollectKnownEntities( &knownVector );

				for( int i=0; i < knownVector.Count(); ++i )
				{
					CKnownEntity &known = knownVector[i];

					if ( GetBot()->IsFriend( known.GetEntity() ) )
					{
						if ( IsAwareOf( known ) )
						{
							if ( known.IsVisibleInFOVNow() )
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 5.0f, 0, 255, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							else
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 2.0f, 0, 100, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
						else
						{
							NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 1.0f, 0, 100, 0, 128, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
					}
					else
					{
						if ( IsAwareOf( known ) )
						{
							if ( known.IsVisibleInFOVNow() )
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 5.0f, 255, 0, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							else
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 2.0f, 100, 0, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
						else
						{
							NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 1.0f, 100, 0, 0, 128, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
					}
				}
			}
		}
	}
}


//------------------------------------------------------------------------------------------
/**
 * Update internal state
 */
void IVision::Update( void )
{
	VPROF_BUDGET( "IVision::Update", "NextBotExpensive" );

/* This adds significantly to bot's reaction times
	// throttle update rate
	if ( !m_scanTimer.IsElapsed() )
	{
		return;
	}

	m_scanTimer.Start( 0.5f * GetMinRecognizeTime() );
*/

	if ( nb_blind.GetBool() )
	{
		m_knownEntityVector.RemoveAll();
		return;
	}

	UpdateKnownEntities();

	m_lastVisionUpdateTimestamp = gpGlobals->curtime;
}


//------------------------------------------------------------------------------------------
bool IVision::IsAbleToSee( CBaseEntity *subject, FieldOfViewCheckType checkFOV, Vector *visibleSpot ) const
{
	VPROF_BUDGET( "IVision::IsAbleToSee", "NextBotExpensive" );

	if ( GetBot()->IsRangeGreaterThan( subject, GetMaxVisionRange() ) )
	{
		return false;
	}

	
	if ( GetBot()->GetEntity()->IsHiddenByFog( subject ) )
	{
		// lost in the fog
		return false;
	}

	if ( checkFOV == USE_FOV && !IsInFieldOfView( subject ) )
	{
		return false;
	}

	CBaseCombatCharacter *combat = subject->MyCombatCharacterPointer();
	if ( combat )
	{
		CNavArea *subjectArea = combat->GetLastKnownArea();
		CNavArea *myArea = GetBot()->GetEntity()->GetLastKnownArea();
		if ( myArea && subjectArea )
		{
			if ( !myArea->IsPotentiallyVisible( subjectArea ) )
			{
				// subject is not potentially visible, skip the expensive raycast
				return false;
			}
		}
	}

	// do actual line-of-sight trace
	if ( !IsLineOfSightClearToEntity( subject ) )
	{
		return false;
	}

	return IsVisibleEntityNoticed( subject );
}


//------------------------------------------------------------------------------------------
bool IVision::IsAbleToSee( const Vector &pos, FieldOfViewCheckType checkFOV ) const
{
	VPROF_BUDGET( "IVision::IsAbleToSee", "NextBotExpensive" );

	
	if ( GetBot()->IsRangeGreaterThan( pos, GetMaxVisionRange() ) )
	{
		return false;
	}

	if ( GetBot()->GetEntity()->IsHiddenByFog( pos ) )
	{
		// lost in the fog
		return false;
	}

	if ( checkFOV == USE_FOV && !IsInFieldOfView( pos ) )
	{
		return false;
	}

	// do actual line-of-sight trace	
	return IsLineOfSightClear( pos );
}


//------------------------------------------------------------------------------------------
/**
 * Angle given in degrees
 */
void IVision::SetFieldOfView( float horizAngle )
{
	m_FOV = horizAngle;
	m_cosHalfFOV = cos( 0.5f * m_FOV * M_PI / 180.0f );
}


//------------------------------------------------------------------------------------------
bool IVision::IsInFieldOfView( const Vector &pos ) const
{
#ifdef CHECK_OLD_CODE_AGAINST_NEW
	bool bCheck = PointWithinViewAngle( GetBot()->GetBodyInterface()->GetEyePosition(), pos, GetBot()->GetBodyInterface()->GetViewVector(), m_cosHalfFOV );
	Vector to = pos - GetBot()->GetBodyInterface()->GetEyePosition();
	to.NormalizeInPlace();
	
	float cosDiff = DotProduct( GetBot()->GetBodyInterface()->GetViewVector(), to );
	
	if ( ( cosDiff > m_cosHalfFOV ) != bCheck )
	{
		Assert(0);
		bool bCheck2 =
			PointWithinViewAngle( GetBot()->GetBodyInterface()->GetEyePosition(), pos, GetBot()->GetBodyInterface()->GetViewVector(), m_cosHalfFOV );

	}

	return ( cosDiff > m_cosHalfFOV );
#else
	return PointWithinViewAngle( GetBot()->GetBodyInterface()->GetEyePosition(), pos, GetBot()->GetBodyInterface()->GetViewVector(), m_cosHalfFOV );
#endif
	
	return true;
}


//------------------------------------------------------------------------------------------
bool IVision::IsInFieldOfView( CBaseEntity *subject ) const
{
	/// @todo check more points
	if ( IsInFieldOfView( subject->WorldSpaceCenter() ) )
	{
		return true;
	}

	return IsInFieldOfView( subject->EyePosition() );
}


//------------------------------------------------------------------------------------------
/**
 * Return true if the ray to the given point is unobstructed
 */
bool IVision::IsLineOfSightClear( const Vector &pos ) const
{
	VPROF_BUDGET( "IVision::IsLineOfSightClear", "NextBot" );
	VPROF_INCREMENT_COUNTER( "IVision::IsLineOfSightClear", 1 );

	trace_t result;
	NextBotVisionTraceFilter filter( GetBot()->GetEntity(), COLLISION_GROUP_NONE );
	
	UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), pos, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
	
	return ( result.fraction >= 1.0f && !result.startsolid );
}


//------------------------------------------------------------------------------------------
bool IVision::IsLineOfSightClearToEntity( const CBaseEntity *subject, Vector *visibleSpot ) const
{
#ifdef TERROR
	// TODO: Integration querycache & its dependencies

	VPROF_INCREMENT_COUNTER( "IVision::IsLineOfSightClearToEntity", 1 );
	VPROF_BUDGET( "IVision::IsLineOfSightClearToEntity", "NextBotSpiky" );

	bool bClear = IsLineOfSightBetweenTwoEntitiesClear( GetBot()->GetBodyInterface()->GetEntity(), EOFFSET_MODE_EYEPOSITION,
														subject, EOFFSET_MODE_WORLDSPACE_CENTER,
														subject, COLLISION_GROUP_NONE,
														MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, VisionTraceFilterFunction, 1.0 );

#ifdef USE_NON_CACHE_QUERY
	trace_t result;
	NextBotTraceFilterIgnoreActors filter( subject, COLLISION_GROUP_NONE );
	
	UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->WorldSpaceCenter(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
	Assert( result.DidHit() != bClear );
	if ( subject->IsPlayer() && ! bClear )
	{
		UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->EyePosition(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
		bClear = IsLineOfSightBetweenTwoEntitiesClear( GetBot()->GetEntity(),
													   EOFFSET_MODE_EYEPOSITION,
													   subject, EOFFSET_MODE_EYEPOSITION,
													   subject, COLLISION_GROUP_NONE,
													   MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, 
													   IgnoreActorsTraceFilterFunction, 1.0 );

		// this WILL assert - the query interface happens at a different time, and has hysteresis.
		Assert( result.DidHit() != bClear );
	}
#endif

	return bClear;

#else

	// TODO: Use plain-old traces until querycache/etc gets integrated
	VPROF_BUDGET( "IVision::IsLineOfSightClearToEntity", "NextBot" );

	trace_t result;
	NextBotTraceFilterIgnoreActors filter( subject, COLLISION_GROUP_NONE );

	UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->WorldSpaceCenter(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
	if ( result.DidHit() )
	{
		UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->EyePosition(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );

		if ( result.DidHit() )
		{
			UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->GetAbsOrigin(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
		}
	}

	if ( visibleSpot )
	{
		*visibleSpot = result.endpos;
	}

	return ( result.fraction >= 1.0f && !result.startsolid );

#endif
}


//------------------------------------------------------------------------------------------
/**
 * Are we looking directly at the given position
 */
bool IVision::IsLookingAt( const Vector &pos, float cosTolerance ) const
{
	Vector to = pos - GetBot()->GetBodyInterface()->GetEyePosition();
	to.NormalizeInPlace();

	Vector forward;
	AngleVectors( GetBot()->GetEntity()->EyeAngles(), &forward );

	return DotProduct( to, forward ) > cosTolerance;
}


//------------------------------------------------------------------------------------------
/**
 * Are we looking directly at the given actor
 */
bool IVision::IsLookingAt( const CBaseCombatCharacter *actor, float cosTolerance ) const
{
	return IsLookingAt( actor->EyePosition(), cosTolerance );
}


