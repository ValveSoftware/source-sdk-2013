//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "NextBot.h"
//#include "bot_npc.h"
#include "bot_npc_body.h"

//-------------------------------------------------------------------------------------------
CBotNPCBody::CBotNPCBody( INextBot *bot ) : IBody( bot )
{
	m_moveXPoseParameter = -1;
	m_moveYPoseParameter = -1;
	m_currentActivity = -1;
	m_desiredAimAngles = vec3_angle;
}


//-------------------------------------------------------------------------------------------
bool CBotNPCBody::StartActivity( Activity act, unsigned int flags )
{
	NextBotCombatCharacter *me = (NextBotCombatCharacter *)GetBot()->GetEntity();

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
void CBotNPCBody::Update( void )
{
	NextBotCombatCharacter *me = (NextBotCombatCharacter *)GetBot()->GetEntity();

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

	// update aim angles
	QAngle currentAngles = me->GetAbsAngles();

	QAngle angles;
	const float approachRate = GetMaxHeadAngularVelocity(); // 3000.0f;
	angles.y = ApproachAngle( m_desiredAimAngles.y, currentAngles.y, approachRate * TICK_INTERVAL );
	angles.x = ApproachAngle( m_desiredAimAngles.x, currentAngles.x, 0.5f * approachRate * TICK_INTERVAL );
	angles.z = 0.0f;

	angles.x = AngleNormalize( angles.x );
	angles.y = AngleNormalize( angles.y );

	me->SetAbsAngles( angles );
}


//---------------------------------------------------------------------------------------------
// return the bot's collision mask (hack until we get a general hull trace abstraction here or in the locomotion interface)
unsigned int CBotNPCBody::GetSolidMask( void ) const
{
	return MASK_NPCSOLID | CONTENTS_PLAYERCLIP;
}


//---------------------------------------------------------------------------------------------
void CBotNPCBody::AimHeadTowards( const Vector &lookAtPos, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	Vector toTarget = lookAtPos - me->WorldSpaceCenter();
	VectorAngles( toTarget, m_desiredAimAngles );
}


//---------------------------------------------------------------------------------------------
void CBotNPCBody::AimHeadTowards( CBaseEntity *subject, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	if ( !subject )
		return;

	CBaseCombatCharacter *me = GetBot()->GetEntity();

	Vector toTarget = subject->WorldSpaceCenter() - me->WorldSpaceCenter();

	QAngle angles;
	VectorAngles( toTarget, m_desiredAimAngles );
}
