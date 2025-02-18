// BehaviorMoveTo.h
// Move to a potentially far away position
// Author: Michael Booth, June 2007
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _BEHAVIOR_MOVE_TO_H_
#define _BEHAVIOR_MOVE_TO_H_


//----------------------------------------------------------------------------------------------
/**
 * Move to a potentially far away position, using path planning.
 */
template < typename Actor, typename PathCost >
class BehaviorMoveTo : public Action< Actor >
{
public:
	BehaviorMoveTo( const Vector &goal, Action< Actor > *successAction = NULL, Action< Actor > *failAction = NULL );

	virtual ActionResult< Actor > OnStart( Actor *me, Action< Actor > *priorAction );
	virtual ActionResult< Actor > Update( Actor *me, float interval );

	virtual EventDesiredResult< Actor > OnMoveToSuccess( Actor *me, const Path *path );
	virtual EventDesiredResult< Actor > OnMoveToFailure( Actor *me, const Path *path, MoveToFailureType reason );

	virtual bool ComputePath( Actor *me, const Vector &goal, PathFollower *path );

	virtual const char *GetName( void ) const	{ return "BehaviorMoveTo"; }

private:
	Vector m_goal;
	PathFollower m_path;
	Action< Actor > *m_successAction;
	Action< Actor > *m_failAction;
};


//----------------------------------------------------------------------------------------------
template < typename Actor, typename PathCost >
inline BehaviorMoveTo< Actor, PathCost >::BehaviorMoveTo( const Vector &goal, Action< Actor > *successAction, Action< Actor > *failAction )
{
	m_goal = goal;
	m_path.Invalidate();
	m_successAction = successAction;
	m_failAction = failAction;
}


//----------------------------------------------------------------------------------------------
template < typename Actor, typename PathCost >
inline bool BehaviorMoveTo< Actor, PathCost >::ComputePath( Actor *me, const Vector &goal, PathFollower *path )
{
	PathCost cost( me );
	return path->Compute( me, goal, cost );
}


//----------------------------------------------------------------------------------------------
template < typename Actor, typename PathCost >
inline ActionResult< Actor > BehaviorMoveTo< Actor, PathCost >::OnStart( Actor *me, Action< Actor > *priorAction )
{
	if ( !this->ComputePath( me, m_goal, &m_path ) )
	{
		if ( m_failAction )
		{
			return this->ChangeTo( m_failAction, "No path to goal" );
		}

		return this->Done( "No path to goal" );
	}

	return this->Continue();
}


//----------------------------------------------------------------------------------------------
template < typename Actor, typename PathCost >
inline ActionResult< Actor > BehaviorMoveTo< Actor, PathCost >::Update( Actor *me, float interval )
{
	// if path became invalid during last tick for any reason, we're done
	if ( !m_path.IsValid() )
	{
		if ( m_failAction )
		{
			return this->ChangeTo( m_failAction, "Path is invalid" );
		}

		return this->Done( "Path is invalid" );
	}

	// move along path - success/fail event handlers will exit behavior when goal is reached
	m_path.Update( me );

	return this->Continue();
}


//----------------------------------------------------------------------------------------------
template < typename Actor, typename PathCost >
inline EventDesiredResult< Actor > BehaviorMoveTo< Actor, PathCost >::OnMoveToSuccess( Actor *me, const Path *path )
{
	if ( m_successAction )
	{
		return this->TryChangeTo( m_successAction, RESULT_CRITICAL, "OnMoveToSuccess" );
	}

	return this->TryDone( RESULT_CRITICAL, "OnMoveToSuccess" );
}


//----------------------------------------------------------------------------------------------
template < typename Actor, typename PathCost >
inline EventDesiredResult< Actor > BehaviorMoveTo< Actor, PathCost >::OnMoveToFailure( Actor *me, const Path *path, MoveToFailureType reason )
{
	if ( m_failAction )
	{
		return this->TryChangeTo( m_failAction, RESULT_CRITICAL, "OnMoveToFailure" );
	}

	return this->TryDone( RESULT_CRITICAL, "OnMoveToFailure" );
}



#endif // _BEHAVIOR_MOVE_TO_H_

