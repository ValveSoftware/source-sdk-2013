//========= Copyright Valve Corporation, All rights reserved. ============//
// NextBotKnownEntity.h
// Encapsulation of being aware of an entity
// Author: Michael Booth, June 2009

#ifndef NEXT_BOT_KNOWN_ENTITY_H
#define NEXT_BOT_KNOWN_ENTITY_H

//----------------------------------------------------------------------------
/**
 * A "known entity" is an entity that we have seen or heard at some point
 * and which may or may not be immediately visible to us right now but which
 * we remember the last place we encountered it, and when.
 *
 * TODO: Enhance interface to allow for sets of areas where an unseen entity 
 * could potentially be, knowing his last position and his rate of movement.
 */
class CKnownEntity
{
public:
	// constructing assumes we currently know about this entity
	CKnownEntity( CBaseEntity *who )
	{
		m_who = who;
		m_whenLastSeen = -1.0f;
		m_whenLastBecameVisible = -1.0f;
		m_isVisible = false;
		m_whenBecameKnown = gpGlobals->curtime;
		m_hasLastKnownPositionBeenSeen = false;
		UpdatePosition();
	}

	virtual ~CKnownEntity() { }

	virtual void Destroy( void )
	{
		m_who = NULL;
		m_isVisible = false;
	}

	virtual void UpdatePosition( void )		// could be seen or heard, but now the entity's position is known
	{
		if ( m_who.Get() )
		{
			m_lastKnownPostion = m_who->GetAbsOrigin();
			m_lastKnownArea = m_who->MyCombatCharacterPointer() ? m_who->MyCombatCharacterPointer()->GetLastKnownArea() : NULL;
			m_whenLastKnown = gpGlobals->curtime;
		}
	}

	virtual CBaseEntity *GetEntity( void ) const
	{
		return m_who;
	}

	virtual const Vector &GetLastKnownPosition( void ) const
	{
		return m_lastKnownPostion;
	}

	// Have we had a clear view of the last known position of this entity?
	// This encapsulates the idea of "I just saw a guy right over *there* a few seconds ago, but I don't know where he is now"
	virtual bool HasLastKnownPositionBeenSeen( void ) const
	{
		return m_hasLastKnownPositionBeenSeen;
	}

	virtual void MarkLastKnownPositionAsSeen( void )
	{
		m_hasLastKnownPositionBeenSeen = true;
	}

	virtual const CNavArea *GetLastKnownArea( void ) const
	{
		return m_lastKnownArea;
	}

	virtual float GetTimeSinceLastKnown( void ) const
	{
		return gpGlobals->curtime - m_whenLastKnown;
	}

	virtual float GetTimeSinceBecameKnown( void ) const
	{
		return gpGlobals->curtime - m_whenBecameKnown;
	}

	virtual void UpdateVisibilityStatus( bool visible )
	{
		if ( visible )
		{
			if ( !m_isVisible )
			{
				// just became visible
				m_whenLastBecameVisible = gpGlobals->curtime;
			}

			m_whenLastSeen = gpGlobals->curtime;
		}

		m_isVisible = visible;
	}

	virtual bool IsVisibleInFOVNow( void ) const	// return true if this entity is currently visible and in my field of view
	{
		return m_isVisible;
	}

	virtual bool IsVisibleRecently( void ) const	// return true if this entity is visible or was very recently visible
	{
		if ( m_isVisible )
			return true;

		if ( WasEverVisible() && GetTimeSinceLastSeen() < 3.0f )
			return true;

		return false;
	}

	virtual float GetTimeSinceBecameVisible( void ) const
	{
		return gpGlobals->curtime - m_whenLastBecameVisible;
	}

	virtual float GetTimeWhenBecameVisible( void ) const
	{
		return m_whenLastBecameVisible;
	}

	virtual float GetTimeSinceLastSeen( void ) const
	{
		return gpGlobals->curtime - m_whenLastSeen;
	}

	virtual bool WasEverVisible( void ) const
	{
		return m_whenLastSeen > 0.0f;
	}

	// has our knowledge of this entity become obsolete?
	virtual bool IsObsolete( void ) const
	{
		return GetEntity() == NULL || !m_who->IsAlive() || GetTimeSinceLastKnown() > 10.0f;
	}

	virtual bool operator==( const CKnownEntity &other ) const
	{
		if ( GetEntity() == NULL || other.GetEntity() == NULL )
			return false;

		return ( GetEntity() == other.GetEntity() );
	}

	virtual bool Is( CBaseEntity *who ) const
	{
		if ( GetEntity() == NULL || who == NULL )
			return false;

		return ( GetEntity() == who );
	}

private:
	CHandle< CBaseEntity > m_who;
	Vector m_lastKnownPostion;
	bool m_hasLastKnownPositionBeenSeen;
	CNavArea *m_lastKnownArea;
	float m_whenLastSeen;
	float m_whenLastBecameVisible;
	float m_whenLastKnown;			// last seen or heard, confirming its existance
	float m_whenBecameKnown;
	bool m_isVisible;				// flagged by IVision update as visible or not
};


#endif // NEXT_BOT_KNOWN_ENTITY_H
