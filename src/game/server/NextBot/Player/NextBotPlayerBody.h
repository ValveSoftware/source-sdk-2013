// NextBotPlayerBody.h
// Control and information about the bot's body state (posture, animation state, etc)
// Author: Michael Booth, October 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_PLAYER_BODY_H_
#define _NEXT_BOT_PLAYER_BODY_H_

#include "NextBotBodyInterface.h"


//----------------------------------------------------------------------------------------------------------------
/**
 * A useful reply for IBody::AimHeadTowards.  When the
 * head is aiming on target, press the fire button.
 */
class PressFireButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess( INextBot *bot );	// invoked when process completed successfully
};


//----------------------------------------------------------------------------------------------------------------
/**
 * A useful reply for IBody::AimHeadTowards.  When the
 * head is aiming on target, press the alt-fire button.
 */
class PressAltFireButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess( INextBot *bot );	// invoked when process completed successfully
};


//----------------------------------------------------------------------------------------------------------------
/**
 * A useful reply for IBody::AimHeadTowards.  When the
 * head is aiming on target, press the jump button.
 */
class PressJumpButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess( INextBot *bot );	// invoked when process completed successfully
};


//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for control and information about the bot's body state (posture, animation state, etc)
 */
class PlayerBody : public IBody
{
public:
	PlayerBody( INextBot *bot );
	virtual ~PlayerBody();

	virtual void Reset( void );										// reset to initial state
	virtual void Upkeep( void );									// lightweight update guaranteed to occur every server tick

	virtual bool SetPosition( const Vector &pos );

	virtual const Vector &GetEyePosition( void ) const;				// return the eye position of the bot in world coordinates
	virtual const Vector &GetViewVector( void ) const;				// return the view unit direction vector in world coordinates

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

	virtual bool IsHeadAimingOnTarget( void ) const;				// return true if the bot's head has achieved its most recent lookat target
	virtual bool IsHeadSteady( void ) const;						// return true if head is not rapidly turning to look somewhere else
	virtual float GetHeadSteadyDuration( void ) const;				// return the duration that the bot's head has been on-target
	virtual void ClearPendingAimReply( void );						// clear out currently pending replyWhenAimed callback

	virtual float GetMaxHeadAngularVelocity( void ) const;			// return max turn rate of head in degrees/second

	virtual bool StartActivity( Activity act, unsigned int flags );
	virtual Activity GetActivity( void ) const;						// return currently animating activity
	virtual bool IsActivity( Activity act ) const;					// return true if currently animating activity matches the given one
	virtual bool HasActivityType( unsigned int flags ) const;		// return true if currently animating activity has any of the given flags

	virtual void SetDesiredPosture( PostureType posture );			// request a posture change
	virtual PostureType GetDesiredPosture( void ) const;			// get posture body is trying to assume
	virtual bool IsDesiredPosture( PostureType posture ) const;		// return true if body is trying to assume this posture
	virtual bool IsInDesiredPosture( void ) const;					// return true if body's actual posture matches its desired posture

	virtual PostureType GetActualPosture( void ) const;				// return body's current actual posture
	virtual bool IsActualPosture( PostureType posture ) const;		// return true if body is actually in the given posture

	virtual bool IsPostureMobile( void ) const;						// return true if body's current posture allows it to move around the world
	virtual bool IsPostureChanging( void ) const;					// return true if body's posture is in the process of changing to new posture

	virtual void SetArousal( ArousalType arousal );					// arousal level change
	virtual ArousalType GetArousal( void ) const;					// get arousal level
	virtual bool IsArousal( ArousalType arousal ) const;			// return true if body is at this arousal level

	virtual float GetHullWidth( void ) const;						// width of bot's collision hull in XY plane
	virtual float GetHullHeight( void ) const;						// height of bot's current collision hull based on posture
	virtual float GetStandHullHeight( void ) const;					// height of bot's collision hull when standing
	virtual float GetCrouchHullHeight( void ) const;				// height of bot's collision hull when crouched
	virtual const Vector &GetHullMins( void ) const;				// return current collision hull minimums based on actual body posture
	virtual const Vector &GetHullMaxs( void ) const;				// return current collision hull maximums based on actual body posture

	virtual unsigned int GetSolidMask( void ) const;				// return the bot's collision mask (hack until we get a general hull trace abstraction here or in the locomotion interface)

	virtual CBaseEntity *GetEntity( void );					// get the entity

	CBaseEntity *GetLookAtSubject( void ) const;
private:
	CBasePlayer *m_player;
	
	PostureType m_posture;
	ArousalType m_arousal;

	mutable Vector m_eyePos;			// for use with GetEyePosition() ONLY
	mutable Vector m_viewVector;		// for use with GetViewVector() ONLY
	mutable Vector m_hullMins;			// for use with GetHullMins() ONLY
	mutable Vector m_hullMaxs;			// for use with GetHullMaxs() ONLY

	Vector m_lookAtPos;					// if m_lookAtSubject is non-NULL, it continually overwrites this position with its own
	EHANDLE m_lookAtSubject;
	Vector m_lookAtVelocity;			// world velocity of lookat point, for tracking moving subjects
	CountdownTimer m_lookAtTrackingTimer;	

	LookAtPriorityType m_lookAtPriority;
	CountdownTimer m_lookAtExpireTimer;		// how long until this lookat expired
	IntervalTimer m_lookAtDurationTimer;	// how long have we been looking at this target
	INextBotReply *m_lookAtReplyWhenAimed;
	bool m_isSightedIn;					// true if we are looking at our last lookat target
	bool m_hasBeenSightedIn;			// true if we have hit the current lookat target

	IntervalTimer m_headSteadyTimer;
	QAngle m_priorAngles;				// last update's head angles
	QAngle m_desiredAngles;

	CountdownTimer m_anchorRepositionTimer;	// the time is takes us to recenter our virtual mouse
	Vector m_anchorForward;
};

inline bool PlayerBody::IsHeadAimingOnTarget( void ) const
{
	// TODO: Calling this immediately after AimHeadTowards will always return false until next Upkeep() (MSB)
	return m_isSightedIn;
}


#endif // _NEXT_BOT_PLAYER_BODY_H_
