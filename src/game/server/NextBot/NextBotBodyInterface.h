// NextBotBodyInterface.h
// Control and information about the bot's body state (posture, animation state, etc)
// Author: Michael Booth, April 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_BODY_INTERFACE_H_
#define _NEXT_BOT_BODY_INTERFACE_H_

#include "animation.h"
#include "NextBotComponentInterface.h"

class INextBot;
struct animevent_t;


//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for control and information about the bot's body state (posture, animation state, etc)
 */
class IBody : public INextBotComponent
{
public:
	IBody( INextBot *bot ) : INextBotComponent( bot ) { }
	virtual ~IBody() { }

	virtual void Reset( void ) { INextBotComponent::Reset(); }			// reset to initial state
	virtual void Update( void ) { }										// update internal state

	/**
	 * Move the bot to a new position.
	 * If the body is not currently movable or if it
	 * is in a motion-controlled animation activity 
	 * the position will not be changed and false will be returned.
	 */
	virtual bool SetPosition( const Vector &pos );

	virtual const Vector &GetEyePosition( void ) const;					// return the eye position of the bot in world coordinates
	virtual const Vector &GetViewVector( void ) const;					// return the view unit direction vector in world coordinates

	enum LookAtPriorityType
	{
		BORING,
		INTERESTING,				// last known enemy location, dangerous sound location
		IMPORTANT,					// a danger
		CRITICAL,					// an active threat to our safety
		MANDATORY					// nothing can interrupt this look at - two simultaneous look ats with this priority is an error
	};
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
	virtual float GetHeadSteadyDuration( void ) const;				// return the duration that the bot's head has not been rotating
	virtual float GetHeadAimSubjectLeadTime( void ) const;			// return how far into the future we should predict our moving subject's position to aim at when tracking subject look-ats
	virtual float GetHeadAimTrackingInterval( void ) const;			// return how often we should sample our target's position and velocity to update our aim tracking, to allow realistic slop in tracking
	virtual void ClearPendingAimReply( void ) { }					// clear out currently pending replyWhenAimed callback

	virtual float GetMaxHeadAngularVelocity( void ) const;			// return max turn rate of head in degrees/second

	enum ActivityType 
	{ 
		MOTION_CONTROLLED_XY	= 0x0001,	// XY position and orientation of the bot is driven by the animation.
		MOTION_CONTROLLED_Z		= 0x0002,	// Z position of the bot is driven by the animation.
		ACTIVITY_UNINTERRUPTIBLE= 0x0004,	// activity can't be changed until animation finishes
		ACTIVITY_TRANSITORY		= 0x0008,	// a short animation that takes over from the underlying animation momentarily, resuming it upon completion
		ENTINDEX_PLAYBACK_RATE	= 0x0010,	// played back at different rates based on entindex
	};

	/**
	 * Begin an animation activity, return false if we cant do that right now.
	 */
	virtual bool StartActivity( Activity act, unsigned int flags = 0 );
	virtual int SelectAnimationSequence( Activity act ) const;			// given an Activity, select and return a specific animation sequence within it

	virtual Activity GetActivity( void ) const;							// return currently animating activity
	virtual bool IsActivity( Activity act ) const;						// return true if currently animating activity matches the given one
	virtual bool HasActivityType( unsigned int flags ) const;			// return true if currently animating activity has any of the given flags

	enum PostureType
	{
		STAND,
		CROUCH,
		SIT,
		CRAWL,
		LIE
	};
	virtual void SetDesiredPosture( PostureType posture ) { }			// request a posture change
	virtual PostureType GetDesiredPosture( void ) const;				// get posture body is trying to assume
	virtual bool IsDesiredPosture( PostureType posture ) const;			// return true if body is trying to assume this posture
	virtual bool IsInDesiredPosture( void ) const;						// return true if body's actual posture matches its desired posture

	virtual PostureType GetActualPosture( void ) const;					// return body's current actual posture
	virtual bool IsActualPosture( PostureType posture ) const;			// return true if body is actually in the given posture

	virtual bool IsPostureMobile( void ) const;							// return true if body's current posture allows it to move around the world
	virtual bool IsPostureChanging( void ) const;						// return true if body's posture is in the process of changing to new posture
	
	
	/**
	 * "Arousal" is the level of excitedness/arousal/anxiety of the body.
	 * Is changes instantaneously to avoid complex interactions with posture transitions.
	 */
	enum ArousalType
	{
		NEUTRAL,
		ALERT,
		INTENSE
	};
	virtual void SetArousal( ArousalType arousal ) { }					// arousal level change
	virtual ArousalType GetArousal( void ) const;						// get arousal level
	virtual bool IsArousal( ArousalType arousal ) const;				// return true if body is at this arousal level


	virtual float GetHullWidth( void ) const;							// width of bot's collision hull in XY plane
	virtual float GetHullHeight( void ) const;							// height of bot's current collision hull based on posture
	virtual float GetStandHullHeight( void ) const;						// height of bot's collision hull when standing
	virtual float GetCrouchHullHeight( void ) const;					// height of bot's collision hull when crouched
	virtual const Vector &GetHullMins( void ) const;					// return current collision hull minimums based on actual body posture
	virtual const Vector &GetHullMaxs( void ) const;					// return current collision hull maximums based on actual body posture

	virtual unsigned int GetSolidMask( void ) const;					// return the bot's collision mask (hack until we get a general hull trace abstraction here or in the locomotion interface)
	virtual unsigned int GetCollisionGroup( void ) const;
};


inline bool IBody::IsHeadSteady( void ) const
{
	return true;
}

inline float IBody::GetHeadSteadyDuration( void ) const
{
	return 0.0f;
}

inline float IBody::GetHeadAimSubjectLeadTime( void ) const
{
	return 0.0f;
}

inline float IBody::GetHeadAimTrackingInterval( void ) const
{
	return 0.0f;
}

inline float IBody::GetMaxHeadAngularVelocity( void ) const
{
	return 1000.0f;
}

inline bool IBody::StartActivity( Activity act, unsigned int flags )
{
	return false;
}

inline int IBody::SelectAnimationSequence( Activity act ) const
{
	return 0;
}

inline Activity IBody::GetActivity( void ) const
{
	return ACT_INVALID;
}

inline bool IBody::IsActivity( Activity act ) const
{
	return false;
}

inline bool IBody::HasActivityType( unsigned int flags ) const
{
	return false;
}

inline IBody::PostureType IBody::GetDesiredPosture( void ) const
{
	return IBody::STAND;
}

inline bool IBody::IsDesiredPosture( PostureType posture ) const
{
	return true;
}

inline bool IBody::IsInDesiredPosture( void ) const
{
	return true;
}

inline IBody::PostureType IBody::GetActualPosture( void ) const
{
	return IBody::STAND;
}

inline bool IBody::IsActualPosture( PostureType posture ) const
{
	return true;
}

inline bool IBody::IsPostureMobile( void ) const
{
	return true;
}

inline bool IBody::IsPostureChanging( void ) const
{
	return false;
}

inline IBody::ArousalType IBody::GetArousal( void ) const
{
	return IBody::NEUTRAL;
}

inline bool IBody::IsArousal( ArousalType arousal ) const
{
	return true;
}

//---------------------------------------------------------------------------------------------------------------------------
/**
 * Width of bot's collision hull in XY plane
 */
inline float IBody::GetHullWidth( void ) const
{
	return 26.0f;
}


//---------------------------------------------------------------------------------------------------------------------------
/**
 * Height of bot's current collision hull based on posture
 */
inline float IBody::GetHullHeight( void ) const
{
	switch( GetActualPosture() )
	{
	case LIE:
		return 16.0f;

	case SIT:
	case CROUCH:
		return GetCrouchHullHeight();

	case STAND:
	default:
		return GetStandHullHeight();
	}
}


//---------------------------------------------------------------------------------------------------------------------------
/**
 * Height of bot's collision hull when standing
 */
inline float IBody::GetStandHullHeight( void ) const
{
	return 68.0f;
}


//---------------------------------------------------------------------------------------------------------------------------
/**
 * Height of bot's collision hull when crouched
 */
inline float IBody::GetCrouchHullHeight( void ) const
{
	return 32.0f;
}


//---------------------------------------------------------------------------------------------------------------------------
/**
 * Return current collision hull minimums based on actual body posture
 */
inline const Vector &IBody::GetHullMins( void ) const
{
	static Vector hullMins;

	hullMins.x = -GetHullWidth()/2.0f;
	hullMins.y = hullMins.x;
	hullMins.z = 0.0f;

	return hullMins;
}


//---------------------------------------------------------------------------------------------------------------------------
/**
 * Return current collision hull maximums based on actual body posture
 */
inline const Vector &IBody::GetHullMaxs( void ) const
{
	static Vector hullMaxs;

	hullMaxs.x = GetHullWidth()/2.0f;
	hullMaxs.y = hullMaxs.x;
	hullMaxs.z = GetHullHeight();

	return hullMaxs;
}


inline unsigned int IBody::GetSolidMask( void ) const
{
	return MASK_NPCSOLID;
}

inline unsigned int IBody::GetCollisionGroup( void ) const
{
	return COLLISION_GROUP_NONE;
}



#endif // _NEXT_BOT_BODY_INTERFACE_H_
