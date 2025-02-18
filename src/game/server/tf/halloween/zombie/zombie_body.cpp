//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "NextBot.h"
#include "zombie.h"
#include "zombie_body.h"


//-------------------------------------------------------------------------------------------
CZombieBody::CZombieBody( INextBot *bot ) : IBody( bot )
{
	m_moveXPoseParameter = -1;
	m_moveYPoseParameter = -1;
	m_currentActivity = -1;
}


//-------------------------------------------------------------------------------------------
bool CZombieBody::StartActivity( Activity act, unsigned int flags )
{
	CZombie *me = (CZombie *)GetBot()->GetEntity();

	int animSequence = ::SelectWeightedSequence( me->GetModelPtr(), act, me->GetSequence() );

	if ( animSequence )
	{
		m_currentActivity = act;
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();

		return true;
	}

	return false;
}


//-------------------------------------------------------------------------------------------
void CZombieBody::Update( void )
{
	CZombie *me = (CZombie *)GetBot()->GetEntity();

	if ( m_moveXPoseParameter < 0 )
	{
		m_moveXPoseParameter = me->LookupPoseParameter( "move_x" );
	}

	if ( m_moveYPoseParameter < 0 )
	{
		m_moveYPoseParameter = me->LookupPoseParameter( "move_y" );
	}


	// Update the pose parameters
	float speed = me->GetLocomotionInterface()->GetGroundSpeed(); // me->GetAbsVelocity().Length();

	if ( speed < 0.01f )
	{
		// stopped
		if ( m_moveXPoseParameter >= 0 )
		{
			me->SetPoseParameter( m_moveXPoseParameter, 0.0f );
		}

		if ( m_moveYPoseParameter >= 0 )
		{
			me->SetPoseParameter( m_moveYPoseParameter, 0.0f );
		}
	}
	else
	{
		Vector forward, right, up;
		me->GetVectors( &forward, &right, &up );

		const Vector &motionVector = me->GetLocomotionInterface()->GetGroundMotionVector();

		// move_x == 1.0 at full forward motion and -1.0 in full reverse
		if ( m_moveXPoseParameter >= 0 )
		{
			float forwardVel = DotProduct( motionVector, forward );

			me->SetPoseParameter( m_moveXPoseParameter, forwardVel );
		}

		if ( m_moveYPoseParameter >= 0 )
		{
			float sideVel = DotProduct( motionVector, right );

			me->SetPoseParameter( m_moveYPoseParameter, sideVel );
		}
	}

	// adjust animation speed to actual movement speed
	if ( me->m_flGroundSpeed > 0.0f )
	{
		// Clamp playback rate to avoid datatable warnings.  Anything faster would look silly, anyway.
		float playbackRate = clamp( speed / me->m_flGroundSpeed, -4.f, 12.f );
		me->SetPlaybackRate( playbackRate );
	}

	// move the animation ahead in time	
	me->StudioFrameAdvance();
	me->DispatchAnimEvents( me );
}


//---------------------------------------------------------------------------------------------
// return the bot's collision mask (hack until we get a general hull trace abstraction here or in the locomotion interface)
unsigned int CZombieBody::GetSolidMask( void ) const
{
	return MASK_NPCSOLID | CONTENTS_PLAYERCLIP;
}
