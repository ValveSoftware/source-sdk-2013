// NextBotPathFollow.h
// Path following
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_PATH_FOLLOWER_
#define _NEXT_BOT_PATH_FOLLOWER_

#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "NextBotPath.h"

class INextBot;
class ILocomotion;


//--------------------------------------------------------------------------------------------------------
/**
 * A PathFollower extends a Path to include mechanisms to move along (follow) it
 */
class PathFollower : public Path
{
public:
	PathFollower( void );
	virtual ~PathFollower();

	virtual void Invalidate( void );				// (EXTEND) cause the path to become invalid
	virtual void Draw( const Path::Segment *start = NULL ) const;	// (EXTEND) draw the path for debugging
	virtual void OnPathChanged( INextBot *bot, Path::ResultType result );	// invoked when the path is (re)computed (path is valid at the time of this call)

	virtual void Update( INextBot *bot );			// move bot along path

	virtual const Path::Segment *GetCurrentGoal( void ) const;	// return current goal along the path we are trying to reach

	virtual void SetMinLookAheadDistance( float value );		// minimum range movement goal must be along path
	
	virtual CBaseEntity *GetHindrance( void ) const;			// returns entity that is hindering our progress along the path

	virtual bool IsDiscontinuityAhead( INextBot *bot, Path::SegmentType type, float range = -1.0f ) const;	// return true if there is a the given discontinuity ahead in the path within the given range (-1 = entire remaining path)

	void SetGoalTolerance( float range );			// set tolerance within at which we're considered to be at our goal

	Path::ResultType GetResult() const { return m_result;  }

private:
	const Path::Segment *m_goal;					// our current goal along the path
	float m_minLookAheadRange;

	bool CheckProgress( INextBot *bot );
	bool IsAtGoal( INextBot *bot ) const;			// return true if reached current path goal

	//bool IsOnStairs( INextBot *bot ) const;		// return true if bot is standing on a stairway
	bool m_isOnStairs;

	Path::ResultType m_result = NO_PATH;

	CountdownTimer m_avoidTimer;					// do avoid check more often if we recently avoided

	CountdownTimer m_waitTimer;						// for waiting for a blocker to move off our path
	CHandle< CBaseEntity > m_hindrance;
	
	// debug display data for avoid volumes
	bool m_didAvoidCheck;
	Vector m_leftFrom;
	Vector m_leftTo;
	bool m_isLeftClear;
	Vector m_rightFrom;
	Vector m_rightTo;
	bool m_isRightClear;
	Vector m_hullMin, m_hullMax;

	void AdjustSpeed( INextBot *bot );				// adjust speed based on path curvature

	Vector Avoid( INextBot *bot, const Vector &goalPos, const Vector &forward, const Vector &left );		// avoidance movements for very nearby obstacles. returns modified goal position
	bool Climbing( INextBot *bot, const Path::Segment *goal, const Vector &forward, const Vector &left, float goalRange );		// climb up ledges 
	bool JumpOverGaps( INextBot *bot, const Path::Segment *goal, const Vector &forward, const Vector &left, float goalRange );	// jump over gaps

	bool LadderUpdate( INextBot *bot );				// move bot along ladder
	CBaseEntity *FindBlocker( INextBot *bot );		// if entity is returned, it is blocking us from continuing along our path

	float m_goalTolerance;
};


inline void PathFollower::SetGoalTolerance( float range )
{
	m_goalTolerance = range;
}


inline const Path::Segment *PathFollower::GetCurrentGoal( void ) const
{
	return m_goal;
}


inline void PathFollower::SetMinLookAheadDistance( float value )
{
	m_minLookAheadRange = value;
}

inline CBaseEntity *PathFollower::GetHindrance( void ) const
{
	return m_hindrance;
}


#endif // _NEXT_BOT_PATH_FOLLOWER_


