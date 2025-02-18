// NextBotContextualQueryInterface.h
// Queries within the context of the bot's current behavior state
// Author: Michael Booth, June 2007
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_CONTEXTUAL_QUERY_H_
#define _NEXT_BOT_CONTEXTUAL_QUERY_H_

class INextBot;
class CBaseEntity;
class CBaseCombatCharacter;
class Path;
class CKnownEntity;

/** 
 * Since behaviors can have several concurrent actions active, we ask
 * the topmost child action first, and if it defers, its parent, and so
 * on, until we get a definitive answer.
 */
enum QueryResultType
{
	ANSWER_NO,
	ANSWER_YES,
	ANSWER_UNDEFINED
};

// Can pass this into IContextualQuery::IsHindrance to see if any hindrance is ever possible
#define IS_ANY_HINDRANCE_POSSIBLE	( (CBaseEntity*)0xFFFFFFFF )


//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for queries that are dependent on the bot's current behavior state
 */
class IContextualQuery
{
public:
	virtual ~IContextualQuery() { }

	virtual QueryResultType			ShouldPickUp( const INextBot *me, CBaseEntity *item ) const;		// if the desired item was available right now, should we pick it up?
	virtual QueryResultType			ShouldHurry( const INextBot *me ) const;							// are we in a hurry?
	virtual QueryResultType			ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType			ShouldAttack( const INextBot *me, const CKnownEntity *them ) const;	// should we attack "them"?
	virtual QueryResultType			IsHindrance( const INextBot *me, CBaseEntity *blocker ) const;		// return true if we should wait for 'blocker' that is across our path somewhere up ahead.

	virtual Vector					SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *subject ) const;		// given a subject, return the world space position we should aim at

	/**
	 * Allow bot to approve of positions game movement tries to put him into.
	 * This is most useful for bots derived from CBasePlayer that go through
	 * the player movement system.
	 */
	virtual QueryResultType IsPositionAllowed( const INextBot *me, const Vector &pos ) const;

	virtual const CKnownEntity *	SelectMoreDangerousThreat( const INextBot *me, 
															   const CBaseCombatCharacter *subject,
															   const CKnownEntity *threat1, 
															   const CKnownEntity *threat2 ) const;	// return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
};

inline QueryResultType IContextualQuery::ShouldPickUp( const INextBot *me, CBaseEntity *item ) const
{
	return ANSWER_UNDEFINED;
}

inline QueryResultType IContextualQuery::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

inline QueryResultType IContextualQuery::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

inline QueryResultType IContextualQuery::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	return ANSWER_UNDEFINED;
}

inline QueryResultType IContextualQuery::IsHindrance( const INextBot *me, CBaseEntity *blocker ) const
{
	return ANSWER_UNDEFINED;
}

inline Vector IContextualQuery::SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *subject ) const
{
	return vec3_origin;
}

inline QueryResultType IContextualQuery::IsPositionAllowed( const INextBot *me, const Vector &pos ) const
{
	return ANSWER_UNDEFINED;
}

inline const CKnownEntity *IContextualQuery::SelectMoreDangerousThreat( const INextBot *me, const CBaseCombatCharacter *subject, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	return NULL;
}


#endif // _NEXT_BOT_CONTEXTUAL_QUERY_H_
