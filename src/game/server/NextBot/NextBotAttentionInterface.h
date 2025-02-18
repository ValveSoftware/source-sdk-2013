// NextBotAttentionInterface.h
// Manage what this bot pays attention to
// Author: Michael Booth, April 2007
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_ATTENTION_INTERFACE_H_
#define _NEXT_BOT_ATTENTION_INTERFACE_H_

#include "NextBotComponentInterface.h"

class INextBot;
class IBody;


//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for managing what a bot pays attention to.
 * Vision determines what see see and notice -> Attention determines which of those things we look at -> Low level head/aiming simulation actually moves our head/eyes
 */
class IAttention : public INextBotComponent
{
public:
	IAttention( INextBot *bot ) : INextBotComponent( bot ) { }
	virtual ~IAttention() { }

	virtual void Reset( void )	{ }										// reset to initial state
	virtual void Update( void ) { }										// update internal state

	enum SignificanceLevel
	{
		BORING,					// background noise
		INTERESTING,			// notably interesting
		COMPELLING,				// very hard to pay attention to anything else
		IRRESISTIBLE,			// can't look away
	};

	// override these to control the significance of entities in a context-specific way
	virtual int CompareSignificance( const CBaseEntity *a, const CBaseEntity *b ) const;	// returns <0 if a < b, 0 if a==b, or >0 if a>b

	// bring things to our attention
	virtual void AttendTo( CBaseEntity *what, const char *reason = NULL );
	virtual void AttendTo( const Vector &where, SignificanceLevel significance, const char *reason = NULL );

	// remove things from our attention
	virtual void Disregard( CBaseEntity *what, const char *reason = NULL );

	virtual bool IsAwareOf( CBaseEntity *what ) const;								// return true if given object is in our attending set
	virtual float GetAwareDuration( CBaseEntity *what ) const;						// return how long we've been aware of this entity

	// INextBotEventResponder ------------------------------------------------------------------
	virtual void OnInjured( const CTakeDamageInfo &info );							// when bot is damaged by something
	virtual void OnContact( CBaseEntity *other, CGameTrace *result = NULL );		// invoked when bot touches 'other'
	virtual void OnSight( CBaseEntity *subject );									// when subject initially enters bot's visual awareness
	virtual void OnLostSight( CBaseEntity *subject );								// when subject leaves enters bot's visual awareness
	virtual void OnSound( CBaseEntity *source, const CSoundParameters &params );	// when an entity emits a sound


private:
	IBody *m_body;							// to access head aiming

	struct PointOfInterest
	{
		enum { ENTITY, POSITION } m_type;
		CHandle< CBaseEntity > m_entity;
		Vector m_position;

		IntervalTimer m_duration;			// how long has this PoI been in our attention set
	};

	CUtlVector< PointOfInterest > m_attentionSet;	// the set of things we are attending to

	
};

inline int IAttention::CompareSignificance( const CBaseEntity *a, const CBaseEntity *b ) const
{
	return 0;
}

#endif // _NEXT_BOT_ATTENTION_INTERFACE_H_

