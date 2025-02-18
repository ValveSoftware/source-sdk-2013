// NextBotChasePath.h
// Maintain and follow a "chase path" to a selected Actor
// Author: Michael Booth, September 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_CHASE_PATH_
#define _NEXT_BOT_CHASE_PATH_

#include "nav.h"
#include "NextBotInterface.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotChasePath.h"
#include "NextBotUtil.h"
#include "NextBotPathFollow.h"
#include "tier0/vprof.h"


//----------------------------------------------------------------------------------------------
/**
 * A ChasePath extends a PathFollower to periodically recompute a path to a chase
 * subject, and to move along the path towards that subject.
 */
class ChasePath : public PathFollower
{
public:
	enum SubjectChaseType
	{
		LEAD_SUBJECT,
		DONT_LEAD_SUBJECT
	};
	ChasePath( SubjectChaseType chaseHow = DONT_LEAD_SUBJECT );

	virtual ~ChasePath() { }

	virtual void Update( INextBot *bot, CBaseEntity *subject, const IPathCost &cost, Vector *pPredictedSubjectPos = NULL );	// update path to chase target and move bot along path

	virtual float GetLeadRadius( void ) const;			// range where movement leading begins - beyond this just head right for the subject
	virtual float GetMaxPathLength( void ) const;		// return maximum path length
	virtual Vector PredictSubjectPosition( INextBot *bot, CBaseEntity *subject ) const;	// try to cutoff our chase subject, knowing our relative positions and velocities
	virtual bool IsRepathNeeded( INextBot *bot, CBaseEntity *subject ) const;			// return true if situation has changed enough to warrant recomputing the current path

	virtual float GetLifetime( void ) const;			// Return duration this path is valid. Path will become invalid at its earliest opportunity once this duration elapses. Zero = infinite lifetime

	virtual void Invalidate( void );					// (EXTEND) cause the path to become invalid

private:
	void RefreshPath( INextBot *bot, CBaseEntity *subject, const IPathCost &cost, Vector *pPredictedSubjectPos );

	CountdownTimer m_failTimer;							// throttle re-pathing if last path attempt failed
	CountdownTimer m_throttleTimer;						// require a minimum time between re-paths
	CountdownTimer m_lifetimeTimer;
	EHANDLE m_lastPathSubject;							// the subject used to compute the current/last path
	SubjectChaseType m_chaseHow;
};

inline ChasePath::ChasePath( SubjectChaseType chaseHow )
{
	m_failTimer.Invalidate();
	m_throttleTimer.Invalidate();
	m_lifetimeTimer.Invalidate();
	m_lastPathSubject = NULL;
	m_chaseHow = chaseHow;
}

inline float ChasePath::GetLeadRadius( void ) const 
{ 
	return 500.0f; // 1000.0f; 
}

inline float ChasePath::GetMaxPathLength( void ) const
{
	// no limit
	return 0.0f;
}

inline float ChasePath::GetLifetime( void ) const
{
	// infinite duration
	return 0.0f;
}

inline void ChasePath::Invalidate( void )
{
	// path is gone, repath at earliest opportunity
	m_throttleTimer.Invalidate();
	m_lifetimeTimer.Invalidate();

	// extend
	PathFollower::Invalidate();	
}




//----------------------------------------------------------------------------------------------
/**
 * Maintain a path to our chase subject and move along that path
 */
inline void ChasePath::Update( INextBot *bot, CBaseEntity *subject, const IPathCost &cost, Vector *pPredictedSubjectPos )
{
	VPROF_BUDGET( "ChasePath::Update", "NextBot" );

	// maintain the path to the subject
	RefreshPath( bot, subject, cost, pPredictedSubjectPos );

	// move along the path towards the subject
	PathFollower::Update( bot );
}


//----------------------------------------------------------------------------------------------
/**
 * Return true if situation has changed enough to warrant recomputing the current path
 */
inline bool ChasePath::IsRepathNeeded( INextBot *bot, CBaseEntity *subject ) const
{
	// the closer we get, the more accurate our path needs to be
	Vector to = subject->GetAbsOrigin() - bot->GetPosition();

	const float minTolerance = 0.0f; // 25.0f;
	const float toleranceRate = 0.33f; // 1.0f; // 0.15f;

	float tolerance = minTolerance + toleranceRate * to.Length();

	return ( subject->GetAbsOrigin() - GetEndPosition() ).IsLengthGreaterThan( tolerance );
}


//----------------------------------------------------------------------------------------------
/**
 * Periodically rebuild the path to our victim
 */
inline void ChasePath::RefreshPath( INextBot *bot, CBaseEntity *subject, const IPathCost &cost, Vector *pPredictedSubjectPos )
{
	VPROF_BUDGET( "ChasePath::RefreshPath", "NextBot" );

	ILocomotion *mover = bot->GetLocomotionInterface();

	// don't change our path if we're on a ladder
	if ( IsValid() && mover->IsUsingLadder() )
	{
		if ( bot->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "%3.2f: bot(#%d) ChasePath::RefreshPath failed. Bot is on a ladder.\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
		}

		// don't allow repath until a moment AFTER we have left the ladder
		m_throttleTimer.Start( 1.0f );

		return;
	}

	if ( subject == NULL )
	{
		if ( bot->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "%3.2f: bot(#%d) CasePath::RefreshPath failed. No subject.\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
		}
		return;
	}

	if ( !m_failTimer.IsElapsed() )
	{
// 		if ( bot->IsDebugging( NEXTBOT_PATH ) )
// 		{
// 			DevMsg( "%3.2f: bot(#%d) ChasePath::RefreshPath failed. Fail timer not elapsed.\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
// 		}
		return;
	}

	// if our path subject changed, repath immediately
	if ( subject != m_lastPathSubject )
	{
		if ( bot->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "%3.2f: bot(#%d) Chase path subject changed (from %p to %p).\n", gpGlobals->curtime, bot->GetEntity()->entindex(), m_lastPathSubject.Get(), subject );
		}

		Invalidate();

		// new subject, fresh attempt
		m_failTimer.Invalidate();
	}

	if ( IsValid() && !m_throttleTimer.IsElapsed() )
	{
		// require a minimum time between repaths, as long as we have a path to follow
// 		if ( bot->IsDebugging( NEXTBOT_PATH ) )
// 		{
// 			DevMsg( "%3.2f: bot(#%d) ChasePath::RefreshPath failed. Rate throttled.\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
// 		}
		return;
	}

	if ( IsValid() && m_lifetimeTimer.HasStarted() && m_lifetimeTimer.IsElapsed() )
	{
		// this path's lifetime has elapsed
		Invalidate();
	}
	
	if ( !IsValid() || IsRepathNeeded( bot, subject ) )
	{
		// the situation has changed - try a new path
		bool isPath;
		Vector pathTarget = subject->GetAbsOrigin();

		if ( m_chaseHow == LEAD_SUBJECT )
		{
			pathTarget = pPredictedSubjectPos ? *pPredictedSubjectPos : PredictSubjectPosition( bot, subject );
			isPath = Compute( bot, pathTarget, cost, GetMaxPathLength() );
		}
		else if ( subject->MyCombatCharacterPointer() && subject->MyCombatCharacterPointer()->GetLastKnownArea() )
		{
			isPath = Compute( bot, subject->MyCombatCharacterPointer(), cost, GetMaxPathLength() );
		}
		else
		{
			isPath = Compute( bot, pathTarget, cost, GetMaxPathLength() );
		}

		if ( isPath )
		{
			if ( bot->IsDebugging( NEXTBOT_PATH ) )
			{
				//const float size = 20.0f;			
				//NDebugOverlay::VertArrow( bot->GetPosition() + Vector( 0, 0, size ), bot->GetPosition(), size, 255, RandomInt( 0, 200 ), 255, 255, true, 30.0f );

				DevMsg( "%3.2f: bot(#%d) REPATH\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
			}

			m_lastPathSubject = subject;

			const float minRepathInterval = 0.5f;
			m_throttleTimer.Start( minRepathInterval );

			// track the lifetime of this new path
			float lifetime = GetLifetime();
			if ( lifetime > 0.0f )
			{
				m_lifetimeTimer.Start( lifetime );
			}
			else
			{
				m_lifetimeTimer.Invalidate();
			}
		}
		else
		{
			// can't reach subject - throttle retry based on range to subject
			m_failTimer.Start( 0.005f * ( bot->GetRangeTo( subject ) ) );
			
			// allow bot to react to path failure
			bot->OnMoveToFailure( this, FAIL_NO_PATH_EXISTS );

			if ( bot->IsDebugging( NEXTBOT_PATH ) )
			{
				//const float size = 20.0f;	
				const float dT = 90.0f;		
				int c = RandomInt( 0, 100 );
				//NDebugOverlay::VertArrow( bot->GetPosition() + Vector( 0, 0, size ), bot->GetPosition(), size, 255, c, c, 255, true, dT );
				NDebugOverlay::HorzArrow( bot->GetPosition(), pathTarget, 5.0f, 255, c, c, 255, true, dT );

				DevMsg( "%3.2f: bot(#%d) REPATH FAILED\n", gpGlobals->curtime, bot->GetEntity()->entindex() );
			}

			Invalidate();
		}
	}
}


//----------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * Directly beeline toward victim if we have a clear shot, otherwise pathfind.
 */
class DirectChasePath : public ChasePath
{
public:

	DirectChasePath( ChasePath::SubjectChaseType chaseHow = ChasePath::DONT_LEAD_SUBJECT ) : ChasePath( chaseHow )
	{

	}

	//-------------------------------------------------------------------------------------------------------
	virtual void Update( INextBot *me, CBaseEntity *victim, const IPathCost &pathCost, Vector *pPredictedSubjectPos = NULL )	// update path to chase target and move bot along path
	{
		Assert( !pPredictedSubjectPos );
		bool bComputedPredictedPosition;
		Vector vecPredictedPosition;
		if ( !DirectChase( &bComputedPredictedPosition, &vecPredictedPosition, me, victim ) )
		{
			// path around obstacles to reach our victim
			ChasePath::Update( me, victim, pathCost, bComputedPredictedPosition ? &vecPredictedPosition : NULL );
		}
		NotifyVictim( me, victim );
	}

	//-------------------------------------------------------------------------------------------------------
	bool DirectChase( bool *pPredictedPositionComputed, Vector *pPredictedPos, INextBot *me, CBaseEntity *victim )		// if there is nothing between us and our victim, run directly at them
	{
		*pPredictedPositionComputed = false;

		ILocomotion *mover = me->GetLocomotionInterface();

		if ( me->IsImmobile() || mover->IsScrambling() )
		{
			return false;
		}

		if ( IsDiscontinuityAhead( me, CLIMB_UP ) )
		{
			return false;
		}

		if ( IsDiscontinuityAhead( me, JUMP_OVER_GAP ) )
		{
			return false;
		}

		Vector leadVictimPos = PredictSubjectPosition( me, victim );

		// Don't want to have to compute the predicted position twice.
		*pPredictedPositionComputed = true;
		*pPredictedPos = leadVictimPos;

		if ( !mover->IsPotentiallyTraversable( mover->GetFeet(), leadVictimPos  ) )
		{
			return false;
		}

		// the way is clear - move directly towards our victim
		mover->FaceTowards( leadVictimPos );
		mover->Approach( leadVictimPos );

		me->GetBodyInterface()->AimHeadTowards( victim );

		// old path is no longer useful since we've moved off of it
		Invalidate();

		return true;
	}

	//-------------------------------------------------------------------------------------------------------
	virtual bool IsRepathNeeded( INextBot *bot, CBaseEntity *subject ) const			// return true if situation has changed enough to warrant recomputing the current path
	{
		if ( ChasePath::IsRepathNeeded( bot, subject ) )
		{
			return true;
		}

		return bot->GetLocomotionInterface()->IsStuck() && bot->GetLocomotionInterface()->GetStuckDuration() > 2.0f;
	}

	//-------------------------------------------------------------------------------------------------------
	/**
	 * Determine exactly where the path goes between the given two areas
	 * on the path. Return this point in 'crossPos'.
	 */
	virtual void ComputeAreaCrossing( INextBot *bot, const CNavArea *from, const Vector &fromPos, const CNavArea *to, NavDirType dir, Vector *crossPos ) const
	{
		Vector center;
		float halfWidth;
		from->ComputePortal( to, dir, &center, &halfWidth );

		*crossPos = center;
	}

	void NotifyVictim( INextBot *me, CBaseEntity *victim );
};




#endif // _NEXT_BOT_CHASE_PATH_
