//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BOT_NPC_BODY_H
#define BOT_NPC_BODY_H

#include "animation.h"
#include "NextBotBodyInterface.h"

class INextBot;


//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for control and information about the bot's body state (posture, animation state, etc)
 */
class CBotNPCBody : public IBody
{
public:
	CBotNPCBody( INextBot *bot );
	virtual ~CBotNPCBody() { }

	virtual void Update( void );

	virtual bool StartActivity( Activity act, unsigned int flags = 0 );
	virtual Activity GetActivity( void ) const;							// return currently animating activity
	virtual bool IsActivity( Activity act ) const;						// return true if currently animating activity matches the given one

	virtual void AimHeadTowards( const Vector &lookAtPos, 
								 LookAtPriorityType priority = BORING, 
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = NULL,
								 const char *reason = NULL );		// aim the bot's head towards the given goal
	virtual void AimHeadTowards( CBaseEntity *subject,
								 LookAtPriorityType priority = BORING, 
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = NULL,
								 const char *reason = NULL );		// continually aim the bot's head towards the given subject

	virtual unsigned int GetSolidMask( void ) const;					// return the bot's collision mask (hack until we get a general hull trace abstraction here or in the locomotion interface)
	virtual unsigned int GetCollisionGroup( void ) const;

private:
	int m_currentActivity;
	int m_moveXPoseParameter;	
	int m_moveYPoseParameter;
	QAngle m_desiredAimAngles;
};


inline Activity CBotNPCBody::GetActivity( void ) const
{
	return (Activity)m_currentActivity;
}

inline bool CBotNPCBody::IsActivity( Activity act ) const
{
	return act == m_currentActivity ? true : false;
}

inline unsigned int CBotNPCBody::GetCollisionGroup( void ) const
{
	return COLLISION_GROUP_PLAYER_MOVEMENT;
}

#endif // BOT_NPC_BODY_H
