// NextBotInterface.h
// Interface for NextBot
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_INTERFACE_H_
#define _NEXT_BOT_INTERFACE_H_

#include "NextBot/NextBotKnownEntity.h"
#include "NextBotComponentInterface.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"
#include "NextBotIntentionInterface.h"
#include "NextBotVisionInterface.h"
#include "NextBotDebug.h"

class CBaseCombatCharacter;
class PathFollower;

//----------------------------------------------------------------------------------------------------------------
/**
 * A general purpose filter interface for various bot systems
 */
class INextBotFilter
{
public:
	virtual bool IsSelected( const CBaseEntity *candidate ) const = 0;			// return true if this entity passes the filter
};


//----------------------------------------------------------------------------------------------------------------
class INextBot : public INextBotEventResponder
{
public:
	INextBot( void );
	virtual ~INextBot();

	int GetBotId() const;

	bool BeginUpdate();
	void EndUpdate();

	virtual void Reset( void );										// (EXTEND) reset to initial state
	virtual void Update( void );									// (EXTEND) update internal state
	virtual void Upkeep( void );									// (EXTEND) lightweight update guaranteed to occur every server tick

	void FlagForUpdate( bool b = true );
	bool IsFlaggedForUpdate();
	int GetTickLastUpdate() const;
	void SetTickLastUpdate( int );

	virtual bool IsRemovedOnReset( void ) const { return true; }	// remove this bot when the NextBot manager calls Reset

	virtual CBaseCombatCharacter *GetEntity( void ) const	= 0;
	virtual class NextBotCombatCharacter *GetNextBotCombatCharacter( void ) const	{ return NULL; }

#ifdef TERROR
	virtual class SurvivorBot *MySurvivorBotPointer() const { return NULL; }
#endif

	// interfaces are never NULL - return base no-op interfaces at a minimum
	virtual ILocomotion *	GetLocomotionInterface( void ) const;
	virtual IBody *			GetBodyInterface( void ) const;
	virtual IIntention *	GetIntentionInterface( void ) const;
	virtual IVision *		GetVisionInterface( void ) const;
	HSCRIPT ScriptGetLocomotionInterface( void ) const { return ToHScript( this->GetLocomotionInterface() ); }
	HSCRIPT ScriptGetIntentionInterface( void ) const { return ToHScript( this->GetIntentionInterface() ); }
	HSCRIPT ScriptGetBodyInterface( void ) const { return ToHScript( this->GetBodyInterface() ); }
	HSCRIPT ScriptGetVisionInterface( void ) const { return ToHScript( this->GetVisionInterface() ); }

	/**
	 * Attempt to change the bot's position. Return true if successful.
	 */
	virtual bool SetPosition( const Vector &pos );
	virtual const Vector &GetPosition( void ) const;				// get the global position of the bot

	/**
	 * Friend/enemy/neutral queries
	 */
	virtual bool IsEnemy( const CBaseEntity *them ) const;			// return true if given entity is our enemy
	virtual bool IsFriend( const CBaseEntity *them ) const;			// return true if given entity is our friend
	virtual bool IsSelf( const CBaseEntity *them ) const;			// return true if 'them' is actually me
	bool ScriptIsEnemy( HSCRIPT hThem ) const { return this->IsEnemy( ToEnt( hThem ) ); }
	bool ScriptIsFriend( HSCRIPT hThem ) const { return this->IsFriend( ToEnt( hThem ) ); }
	bool ScriptIsSelf( HSCRIPT hThem ) const { return this->IsSelf( ToEnt( hThem ) ); }

	/**
	 * Can we climb onto this entity?
	 */	
	virtual bool IsAbleToClimbOnto( const CBaseEntity *object ) const;

	/**
	 * Can we break this entity?
	 */	
	virtual bool IsAbleToBreak( const CBaseEntity *object ) const;

	/**
	 * Sometimes we want to pass through other NextBots. OnContact() will always
	 * be invoked, but collision resolution can be skipped if this
	 * method returns false.
	 */
	virtual bool IsAbleToBlockMovementOf( const INextBot *botInMotion ) const	{ return true; }

	/**
	 * Should we ever care about noticing physical contact with this entity?
	 */
	virtual bool ShouldTouch( const CBaseEntity *object ) const		{ return true; }

	/**
	 * This immobile system is used to track the global state of "am I actually moving or not".
	 * The OnStuck() event is only emitted when following a path, and paths can be recomputed, etc.
	 */
	virtual bool IsImmobile( void ) const;					// return true if we haven't moved in awhile
	virtual float GetImmobileDuration( void ) const;		// how long have we been immobile
	virtual void ClearImmobileStatus( void );		
	virtual float GetImmobileSpeedThreshold( void ) const;	// return units/second below which this actor is considered "immobile"

	/**
	 * Get the last PathFollower we followed. This method gives other interfaces a
	 * single accessor to the most recent Path being followed by the myriad of 
	 * different PathFollowers used in the various behaviors the bot may be doing.
	 */
	virtual const PathFollower *GetCurrentPath( void ) const;
	virtual void SetCurrentPath( const PathFollower *path );
	virtual void NotifyPathDestruction( const PathFollower *path );		// this PathFollower is going away, which may or may not be ours

	// between distance utility methods
	virtual bool IsRangeLessThan( CBaseEntity *subject, float range ) const;
	virtual bool IsRangeLessThan( const Vector &pos, float range ) const;
	virtual bool IsRangeGreaterThan( CBaseEntity *subject, float range ) const;
	virtual bool IsRangeGreaterThan( const Vector &pos, float range ) const;
	virtual float GetRangeTo( CBaseEntity *subject ) const;
	virtual float GetRangeTo( const Vector &pos ) const;
	virtual float GetRangeSquaredTo( CBaseEntity *subject ) const;
	virtual float GetRangeSquaredTo( const Vector &pos ) const;

	// event propagation
	virtual INextBotEventResponder *FirstContainedResponder( void ) const;
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const;

	virtual bool IsDebugging( unsigned int type ) const;		// return true if this bot is debugging any of the given types
	virtual const char *GetDebugIdentifier( void ) const;		// return the name of this bot for debugging purposes
	virtual bool IsDebugFilterMatch( const char *name ) const;	// return true if we match the given debug symbol
	virtual void DisplayDebugText( const char *text ) const;	// show a line of text on the bot in the world
	void DebugConColorMsg( NextBotDebugType debugType, const Color &color, PRINTF_FORMAT_STRING const char *fmt, ... );

	enum {
		MAX_NEXTBOT_DEBUG_HISTORY = 100,
		MAX_NEXTBOT_DEBUG_LINE_LENGTH = 256,
	};
	struct NextBotDebugLineType
	{
		NextBotDebugType debugType;
		char data[ MAX_NEXTBOT_DEBUG_LINE_LENGTH ];
	};
	void GetDebugHistory( unsigned int type, CUtlVector< const NextBotDebugLineType * > *lines ) const;	// build a vector of debug history of the given types
	//------------------------------------------------------------------------------


private:
	friend class INextBotComponent;
	void RegisterComponent( INextBotComponent *comp );		// components call this to register themselves with the bot that contains them
	INextBotComponent *m_componentList;						// the first component

	const PathFollower *m_currentPath;						// the path we most recently followed

	int m_id;
	bool m_bFlaggedForUpdate;
	int m_tickLastUpdate;

	unsigned int m_debugType;
	mutable int m_debugDisplayLine;

	Vector m_immobileAnchor;
	CountdownTimer m_immobileCheckTimer;
	IntervalTimer m_immobileTimer;
	void UpdateImmobileStatus( void );

	mutable ILocomotion *m_baseLocomotion;
	mutable IBody		*m_baseBody;
	mutable IIntention	*m_baseIntention;
	mutable IVision		*m_baseVision;
	//mutable IAttention	*m_baseAttention;

	// Debugging info
	void ResetDebugHistory( void );
	CUtlVector< NextBotDebugLineType * > m_debugHistory;
};


inline const PathFollower *INextBot::GetCurrentPath( void ) const
{
	return m_currentPath;
}

inline void INextBot::SetCurrentPath( const PathFollower *path )
{
	m_currentPath = path;
}

inline void INextBot::NotifyPathDestruction( const PathFollower *path )
{
	if ( m_currentPath == path )
		m_currentPath = NULL;
}


inline ILocomotion *INextBot::GetLocomotionInterface( void ) const
{
	// these base interfaces are lazy-allocated (instead of being fully instanced classes) for two reasons:
	// 1) so the memory is only used if needed
	// 2) so the component is registered properly
	if ( m_baseLocomotion == NULL )
	{
		m_baseLocomotion = new ILocomotion( const_cast< INextBot * >( this ) );
	}

	return m_baseLocomotion;
}

inline IBody *INextBot::GetBodyInterface( void ) const
{
	if ( m_baseBody == NULL )
	{
		m_baseBody = new IBody( const_cast< INextBot * >( this ) );
	}

	return m_baseBody;
}

inline IIntention *INextBot::GetIntentionInterface( void ) const
{
	if ( m_baseIntention == NULL )
	{
		m_baseIntention = new IIntention( const_cast< INextBot * >( this ) );
	}

	return m_baseIntention;
}

inline IVision *INextBot::GetVisionInterface( void ) const
{
	if ( m_baseVision == NULL )
	{
		m_baseVision = new IVision( const_cast< INextBot * >( this ) );
	}

	return m_baseVision;
}

inline int INextBot::GetBotId() const
{
	return m_id;
}

inline void INextBot::FlagForUpdate( bool b )
{
	m_bFlaggedForUpdate = b;
}

inline bool INextBot::IsFlaggedForUpdate()
{
	return m_bFlaggedForUpdate;
}

inline int INextBot::GetTickLastUpdate() const
{
	return m_tickLastUpdate;
}

inline void INextBot::SetTickLastUpdate( int tick )
{
	m_tickLastUpdate = tick;
}

inline bool INextBot::IsImmobile( void ) const
{
	return m_immobileTimer.HasStarted();
}

inline float INextBot::GetImmobileDuration( void ) const
{
	return m_immobileTimer.GetElapsedTime();
}

inline void INextBot::ClearImmobileStatus( void )
{
	m_immobileTimer.Invalidate();
	m_immobileAnchor = GetEntity()->GetAbsOrigin();
}

inline float INextBot::GetImmobileSpeedThreshold( void ) const
{
	return 30.0f;
}

inline INextBotEventResponder *INextBot::FirstContainedResponder( void ) const
{
	return m_componentList;
}


inline INextBotEventResponder *INextBot::NextContainedResponder( INextBotEventResponder *current ) const
{
	return static_cast< INextBotComponent * >( current )->m_nextComponent;
}


#endif // _NEXT_BOT_INTERFACE_H_
