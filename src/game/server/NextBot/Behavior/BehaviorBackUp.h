// BehaviorBackUp.h
// Back up for a short duration
// Author: Michael Booth, March 2007
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _BEHAVIOR_BACK_UP_H_
#define _BEHAVIOR_BACK_UP_H_


//----------------------------------------------------------------------------------------------
/**
 * Move backwards for a short duration away from a given position.  
 * Useful to dislodge ourselves if we get stuck while following our path.
 */
template < typename Actor >
class BehaviorBackUp : public Action< Actor >
{
public:
	BehaviorBackUp( const Vector &avoidPos );

	virtual ActionResult< Actor > OnStart( Actor *me, Action< Actor > *priorAction );
	virtual ActionResult< Actor > Update( Actor *me, float interval );

	virtual EventDesiredResult< Actor > OnStuck( Actor *me );

	virtual const char *GetName( void ) const	{ return "BehaviorBackUp"; }

private:
	CountdownTimer m_giveUpTimer;
	CountdownTimer m_backupTimer;
	CountdownTimer m_jumpTimer;
	Vector m_way;
	Vector m_avoidPos;
};


//----------------------------------------------------------------------------------------------
template < typename Actor >
inline BehaviorBackUp< Actor >::BehaviorBackUp( const Vector &avoidPos )
{
	m_avoidPos = avoidPos;
}


//----------------------------------------------------------------------------------------------
template < typename Actor >
inline ActionResult< Actor > BehaviorBackUp< Actor >::OnStart( Actor *me, Action< Actor > *priorAction )
{
	ILocomotion *mover = me->GetLocomotionInterface();

	// don't back off if we're on a ladder
	if ( mover && mover->IsUsingLadder() )
	{
		return Done();
	}
	
	float backupTime = RandomFloat( 0.3f, 0.5f );
	
	m_backupTimer.Start( backupTime );
	m_jumpTimer.Start( 1.5f * backupTime );
	m_giveUpTimer.Start( 2.5f * backupTime );
	
	m_way = me->GetPosition() - m_avoidPos;
	m_way.NormalizeInPlace();
	
	return Continue();
}


//----------------------------------------------------------------------------------------------
template < typename Actor >
inline ActionResult< Actor > BehaviorBackUp< Actor >::Update( Actor *me, float interval )
{
	if ( m_giveUpTimer.IsElapsed() )
	{
		return Done();
	}

// 	if ( m_jumpTimer.HasStarted() && m_jumpTimer.IsElapsed() )
// 	{
// 		me->GetLocomotionInterface()->Jump();
// 		m_jumpTimer.Invalidate();
// 	}

	ILocomotion *mover = me->GetLocomotionInterface();
	if ( mover )
	{
		Vector goal;

		if ( m_backupTimer.IsElapsed() )
		{
			// move towards bad spot
			goal = m_avoidPos; // me->GetPosition() - 100.0f * m_way;
		}
		else
		{
			// move away from bad spot
			goal = me->GetPosition() + 100.0f * m_way;
		}

		mover->Approach( goal );
	}
	
	return Continue();
}


//----------------------------------------------------------------------------------------------
template < typename Actor >
inline EventDesiredResult< Actor > BehaviorBackUp< Actor >::OnStuck( Actor *me )
{
	return TryToSustain( RESULT_IMPORTANT, "Stuck while trying to back up" );
}



#endif // _BEHAVIOR_BACK_UP_H_

