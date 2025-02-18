// NextBotIntentionInterface.h
// Interface for intentional thinking
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_INTENTION_INTERFACE_H_
#define _NEXT_BOT_INTENTION_INTERFACE_H_

#include "NextBotComponentInterface.h"
#include "NextBotContextualQueryInterface.h"

class INextBot;

//
// Insert this macro in your INextBot-derived class declaration to
// create a IIntention-derived class that handles the bookkeeping
// of instantiating a Behavior with an initial Action and updating it.
//
#define DECLARE_INTENTION_INTERFACE( Actor )	\
												\
	class Actor##Intention : public IIntention	\
	{											\
	public:										\
		Actor##Intention( Actor *me );			\
		virtual ~Actor##Intention();			\
		virtual void Reset( void );				\
		virtual void Update( void );			\
		virtual INextBotEventResponder *FirstContainedResponder( void ) const  { return m_behavior; }	\
		virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const { return NULL; }	\
	private:									\
		Behavior< Actor > *m_behavior;			\
	};											\
												\
	public: virtual IIntention *GetIntentionInterface( void ) const 	{ return m_intention; }	\
	private: Actor##Intention *m_intention;		\
	public:										


//
// Use this macro to create the implementation code for the IIntention-derived class
// declared above.  Since this requires InitialAction, it must occur after
// that Action has been declared, so it can be new'd here.
//
#define IMPLEMENT_INTENTION_INTERFACE( Actor, InitialAction ) \
	Actor::Actor##Intention::Actor##Intention( Actor *me ) : IIntention( me )	{ m_behavior = new Behavior< Actor >( new InitialAction ); }	\
	Actor::Actor##Intention::~Actor##Intention() {	delete m_behavior; }	\
	void Actor::Actor##Intention::Reset( void ) { delete m_behavior; m_behavior = new Behavior< Actor >( new InitialAction ); }	\
	void Actor::Actor##Intention::Update( void ) { m_behavior->Update( static_cast< Actor * >( GetBot() ), GetUpdateInterval() ); }


//
// Use this macro in the constructor of your bot to allocate the IIntention-derived class
//
#define ALLOCATE_INTENTION_INTERFACE( Actor )	{ m_intention = new Actor##Intention( this ); }

//
// Use this macro in the destructor of your bot to deallocate the IIntention-derived class
//
#define DEALLOCATE_INTENTION_INTERFACE	{ if ( m_intention ) delete m_intention; }


//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for intentional thinking.
 * The assumption is that this is a container for one or more concurrent Behaviors.
 * The "primary" Behavior is the FirstContainedResponder, and so on.  
 * IContextualQuery requests are prioritized in contained responder order, such that the first responder
 * that returns a definitive answer is accepted.  WITHIN a given responder (ie: a Behavior), the deepest child
 * Behavior in the active stack is asked first, then its parent, and so on, allowing the most specific active
 * Behavior to override the query responses of its more general parent Behaviors.
 */
class IIntention : public INextBotComponent, public IContextualQuery
{
public:
	IIntention( INextBot *bot ) : INextBotComponent( bot ) { }
	virtual ~IIntention() { }

	virtual void Reset( void )  { INextBotComponent::Reset(); }	// reset to initial state
	virtual void Update( void ) { }								// update internal state

	// IContextualQuery propagation --------------------------------
	virtual QueryResultType			ShouldPickUp( const INextBot *me, CBaseEntity *item ) const;		// if the desired item was available right now, should we pick it up?
	virtual QueryResultType			ShouldHurry( const INextBot *me ) const;							// are we in a hurry?
	virtual QueryResultType			ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType			ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?
	virtual QueryResultType			IsHindrance( const INextBot *me, CBaseEntity *blocker ) const;		// return true if we should wait for 'blocker' that is across our path somewhere up ahead.
	virtual Vector					SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *subject ) const;		// given a subject, return the world space position we should aim at
	virtual QueryResultType			IsPositionAllowed( const INextBot *me, const Vector &pos ) const;	// is the a place we can be?
	virtual const CKnownEntity *	SelectMoreDangerousThreat( const INextBot *me, 
															   const CBaseCombatCharacter *subject,		// the subject of the danger
															   const CKnownEntity *threat1, 
															   const CKnownEntity *threat2 ) const;	// return the more dangerous of the two threats, or NULL if we have no opinion
	// NOTE: As further queries are added, update the Behavior class to propagate them
};


inline QueryResultType IIntention::ShouldPickUp( const INextBot *me, CBaseEntity *item ) const
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		const IContextualQuery *query = dynamic_cast< const IContextualQuery * >( sub );
		if ( query )
		{
			// return the response of the first responder that gives a definitive answer
			QueryResultType result = query->ShouldPickUp( me, item );
			if ( result != ANSWER_UNDEFINED )
			{
				return result;
			}
		}
	}	
	return ANSWER_UNDEFINED;
}


inline QueryResultType IIntention::ShouldHurry( const INextBot *me ) const
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		const IContextualQuery *query = dynamic_cast< const IContextualQuery * >( sub );
		if ( query )
		{
			// return the response of the first responder that gives a definitive answer
			QueryResultType result = query->ShouldHurry( me );
			if ( result != ANSWER_UNDEFINED )
			{
				return result;
			}
		}
	}	
	return ANSWER_UNDEFINED;
}


inline QueryResultType IIntention::ShouldRetreat( const INextBot *me ) const
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		const IContextualQuery *query = dynamic_cast< const IContextualQuery * >( sub );
		if ( query )
		{
			// return the response of the first responder that gives a definitive answer
			QueryResultType result = query->ShouldRetreat( me );
			if ( result != ANSWER_UNDEFINED )
			{
				return result;
			}
		}
	}	
	return ANSWER_UNDEFINED;
}


inline QueryResultType IIntention::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		const IContextualQuery *query = dynamic_cast< const IContextualQuery * >( sub );
		if ( query )
		{
			// return the response of the first responder that gives a definitive answer
			QueryResultType result = query->ShouldAttack( me, them );
			if ( result != ANSWER_UNDEFINED )
			{
				return result;
			}
		}
	}	
	return ANSWER_UNDEFINED;
}


inline QueryResultType IIntention::IsHindrance( const INextBot *me, CBaseEntity *blocker ) const
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		const IContextualQuery *query = dynamic_cast< const IContextualQuery * >( sub );
		if ( query )
		{
			// return the response of the first responder that gives a definitive answer
			QueryResultType result = query->IsHindrance( me, blocker );
			if ( result != ANSWER_UNDEFINED )
			{
				return result;
			}
		}
	}	
	return ANSWER_UNDEFINED;
}


inline QueryResultType IIntention::IsPositionAllowed( const INextBot *me, const Vector &pos ) const
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		const IContextualQuery *query = dynamic_cast< const IContextualQuery * >( sub );
		if ( query )
		{
			// return the response of the first responder that gives a definitive answer
			QueryResultType result = query->IsPositionAllowed( me, pos );
			if ( result != ANSWER_UNDEFINED )
			{
				return result;
			}
		}
	}	
	return ANSWER_UNDEFINED;
}


#endif // _NEXT_BOT_INTENTION_INTERFACE_H_
