//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: MOVEMENT ENTITIES TEST
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entitylist.h"
#include "entityoutput.h"
#include "keyframe/keyframe.h" // BUG: this needs to move if keyframe is a standard thing

#include "mathlib/mathlib.h"	// FIXME: why do we still need this?

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Hack, sort of. These interpolators don't get to hold state, but the ones
// that need state (like the rope simulator) should NOT be used as paths here.
IPositionInterpolator *g_pPositionInterpolators[8] = {0,0,0,0,0,0,0,0};

IPositionInterpolator* GetPositionInterpolator( int iInterp )
{
	if( !g_pPositionInterpolators[iInterp] )
		g_pPositionInterpolators[iInterp] = Motion_GetPositionInterpolator( iInterp );

	return g_pPositionInterpolators[iInterp];
}


static float Fix( float angle )
{
	while ( angle < 0 )
		angle += 360;
	while ( angle > 360 )
		angle -= 360;

	return angle;
}

void FixupAngles( QAngle &v )
{
	v.x = Fix( v.x );
	v.y = Fix( v.y );
	v.z = Fix( v.z );
}


//-----------------------------------------------------------------------------
//
// Purpose: Contains a description of a keyframe
//			has no networked representation, so has to store origin, etc. itself
//
//-----------------------------------------------------------------------------
class CPathKeyFrame : public CLogicalEntity
{
public:
	DECLARE_CLASS( CPathKeyFrame, CLogicalEntity );

	void Spawn( void );
	void Activate( void );
	void Link( void );

	Vector m_Origin;
	QAngle m_Angles;	// euler angles PITCH YAW ROLL (Y Z X)
	Quaternion m_qAngle;	// quaternion angle (generated from m_Angles)

	string_t m_iNextKey;
	float m_flNextTime;

	CPathKeyFrame *NextKey( int direction );
	CPathKeyFrame *PrevKey( int direction );

	float Speed( void ) { return m_flSpeed; }
	void SetKeyAngles( QAngle angles );

	CPathKeyFrame *InsertNewKey( Vector newPos, QAngle newAngles );
	void CalculateFrameDuration( void );

protected:
	CPathKeyFrame *m_pNextKey;
	CPathKeyFrame *m_pPrevKey;

	float m_flSpeed;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( keyframe_track, CPathKeyFrame );

BEGIN_DATADESC( CPathKeyFrame )

	DEFINE_FIELD( m_Origin, FIELD_VECTOR ),
	DEFINE_FIELD( m_Angles, FIELD_VECTOR ),
	DEFINE_FIELD( m_qAngle, FIELD_QUATERNION ),

	DEFINE_KEYFIELD( m_iNextKey, FIELD_STRING, "NextKey" ),
	DEFINE_FIELD( m_flNextTime, FIELD_FLOAT ),	// derived from speed
	DEFINE_KEYFIELD( m_flSpeed, FIELD_FLOAT, "MoveSpeed" ),
	DEFINE_FIELD( m_pNextKey, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pPrevKey, FIELD_CLASSPTR ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Converts inputed euler angles to internal angle format (quaternions)
//-----------------------------------------------------------------------------
void CPathKeyFrame::Spawn( void )
{
	m_Origin = GetLocalOrigin();
	m_Angles = GetLocalAngles();

	SetKeyAngles( m_Angles );
}

//-----------------------------------------------------------------------------
// Purpose: Adds the keyframe into the path after all the other keys have spawned
//-----------------------------------------------------------------------------
void CPathKeyFrame::Activate( void )
{
	BaseClass::Activate();
	
	Link();

	CalculateFrameDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathKeyFrame::CalculateFrameDuration( void )
{
	// calculate time from speed
	if ( m_pNextKey && m_flSpeed > 0 )
	{
		m_flNextTime = (m_Origin - m_pNextKey->m_Origin).Length() / m_flSpeed;

		// couldn't get time from distance, get it from rotation instead
		if ( !m_flNextTime )
		{
			// speed is in degrees per second
			// find the largest rotation component and use that
			QAngle ang = m_Angles - m_pNextKey->m_Angles;
			FixupAngles( ang );
			float x = 0;
			for ( int i = 0; i < 3; i++ )
			{
				if ( abs(ang[i]) > x )
				{
					x = abs(ang[i]);
				}
			}

			m_flNextTime = x / m_flSpeed;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Links the key frame into the key frame list
//-----------------------------------------------------------------------------
void CPathKeyFrame::Link( void )
{
	m_pNextKey = dynamic_cast<CPathKeyFrame*>( gEntList.FindEntityByName(NULL, m_iNextKey ) );

	if ( m_pNextKey )
	{
		m_pNextKey->m_pPrevKey = this;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : angles - 
//-----------------------------------------------------------------------------
void CPathKeyFrame::SetKeyAngles( QAngle angles )
{
	m_Angles = angles;
	AngleQuaternion( m_Angles, m_qAngle );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : direction - 
// Output : CPathKeyFrame
//-----------------------------------------------------------------------------
CPathKeyFrame* CPathKeyFrame::NextKey( int direction )
{
	if ( direction == 1 )
	{
		return m_pNextKey;
	}
	else if ( direction == -1 )
	{
		return m_pPrevKey;
	}
	
	return this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : direction - 
// Output : CPathKeyFrame
//-----------------------------------------------------------------------------
CPathKeyFrame *CPathKeyFrame::PrevKey( int direction )
{
	if ( direction == 1 )
	{
		return m_pPrevKey;
	}
	else if ( direction == -1 )
	{
		return m_pNextKey;
	}
	
	return this;
}

//-----------------------------------------------------------------------------
// Purpose: Creates and insterts a new keyframe into the sequence
// Input  : newPos - 
//			newAngles - 
// Output : CPathKeyFrame
//-----------------------------------------------------------------------------
CPathKeyFrame *CPathKeyFrame::InsertNewKey( Vector newPos, QAngle newAngles )
{
	CPathKeyFrame *newKey = CREATE_ENTITY( CPathKeyFrame, "keyframe_track" ); 

	// copy data across
	newKey->SetKeyAngles( newAngles );
	newKey->m_Origin = newPos;
	newKey->m_flSpeed = m_flSpeed;
	newKey->SetEFlags( GetEFlags() );
	if ( m_iParent != NULL_STRING )
	{
		newKey->SetParent( m_iParent, NULL );
	}

	// link forward
	newKey->m_pNextKey = m_pNextKey;
	m_pNextKey->m_pPrevKey = newKey;

	// link back
	m_pNextKey = newKey;
	newKey->m_pPrevKey = this;

	// calculate new times
	CalculateFrameDuration();
	newKey->CalculateFrameDuration();

	return newKey;
}


//-----------------------------------------------------------------------------
//
// Purpose: Basic keyframed movement behavior
//
//-----------------------------------------------------------------------------
class CBaseMoveBehavior : public CPathKeyFrame
{
public:
	DECLARE_CLASS( CBaseMoveBehavior, CPathKeyFrame );

	void Spawn( void );
	void Activate( void );
	void MoveDone( void );
	float SetObjectPhysicsVelocity( float moveTime );

	// methods
	virtual bool StartMoving( int direction );
	virtual void StopMoving( void );
	virtual bool IsMoving( void );

	// derived classes should override this to get notification of arriving at new keyframes
//	virtual void ArrivedAtKeyFrame( CPathKeyFrame * ) {}

	bool IsAtSequenceStart( void );
	bool IsAtSequenceEnd( void );

	// interpolation functions
//	int m_iTimeModifier;
	int m_iPositionInterpolator;
	int m_iRotationInterpolator;

	// animation vars
	float m_flAnimStartTime;
	float m_flAnimEndTime;
	float m_flAverageSpeedAcrossFrame; // for advancing time with speed (not the normal visa-versa)
	CPathKeyFrame *m_pCurrentKeyFrame;	// keyframe currently moving from
	CPathKeyFrame *m_pTargetKeyFrame;	// keyframe being moved to
	CPathKeyFrame *m_pPreKeyFrame, *m_pPostKeyFrame;	// pre- and post-keyframe's for spline interpolation
	float m_flTimeIntoFrame;

	int m_iDirection;		// 1 for forward, -1 for backward, and 0 for at rest

	float CalculateTimeAdvancementForSpeed( float moveTime, float speed );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( move_keyframed, CBaseMoveBehavior );

BEGIN_DATADESC( CBaseMoveBehavior )

//	DEFINE_KEYFIELD( m_iTimeModifier, FIELD_INTEGER, "TimeModifier" ),
	DEFINE_KEYFIELD( m_iPositionInterpolator, FIELD_INTEGER, "PositionInterpolator" ),
	DEFINE_KEYFIELD( m_iRotationInterpolator, FIELD_INTEGER, "RotationInterpolator" ),

	DEFINE_FIELD( m_pCurrentKeyFrame, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pTargetKeyFrame, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pPreKeyFrame, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pPostKeyFrame, FIELD_CLASSPTR ),
	
	DEFINE_FIELD( m_flAnimStartTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAnimEndTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAverageSpeedAcrossFrame, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTimeIntoFrame, FIELD_FLOAT ),
	DEFINE_FIELD( m_iDirection, FIELD_INTEGER ),

END_DATADESC()


void CBaseMoveBehavior::Spawn( void )
{
	m_pCurrentKeyFrame = this;
	m_flTimeIntoFrame = 0;
	SetMoveType( MOVETYPE_PUSH );

	// a move behavior is also it's first keyframe
	m_Origin = GetLocalOrigin();
	m_Angles = GetLocalAngles();

	BaseClass::Spawn();
}

void CBaseMoveBehavior::Activate( void )
{
	BaseClass::Activate();

	SetMoveDoneTime( 0.5 );	// start moving in 0.2 seconds time

	// if we are just the basic keyframed entity, cycle our animation
	if ( !stricmp(GetClassname(), "move_keyframed") )
	{
		StartMoving( 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the we're at the start of the keyframe sequence
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMoveBehavior::IsAtSequenceStart( void )
{
	if ( !m_pCurrentKeyFrame )
		return true;

	if ( m_flAnimStartTime && m_flAnimStartTime >= GetLocalTime() )
	{
		if ( !m_pCurrentKeyFrame->PrevKey(1) && !m_pTargetKeyFrame )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if we're at the end of the keyframe sequence
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMoveBehavior::IsAtSequenceEnd( void )
{
	if ( !m_pCurrentKeyFrame )
		return false;

	if ( !m_pCurrentKeyFrame->NextKey(1) && !m_pTargetKeyFrame )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMoveBehavior::IsMoving( void )
{
	if ( m_iDirection != 0 )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Starts the object moving from it's current position, in the direction indicated
// Input  : direction - 1 is forward through the sequence, -1 is backwards, and 0 is stop
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseMoveBehavior::StartMoving( int direction )
{
	// 0 direction is to stop moving
	if ( direction == 0 )
	{
		StopMoving();
		return false;
	}
	
	// check to see if we should keep moving in the current direction
	if ( m_iDirection == direction )
	{
		// if we're at the end of the current anim key, move to the next one
		if ( GetLocalTime() >= m_flAnimEndTime )
		{
			m_pCurrentKeyFrame = m_pTargetKeyFrame;
			m_flTimeIntoFrame = 0;
			
			if ( !m_pTargetKeyFrame->NextKey(direction) )
			{
				// we've hit the end of the sequence
				m_flAnimEndTime = 0;
				m_flAnimStartTime = 0;
				StopMoving();
				return false;
			}

			// advance the target keyframe
			m_pTargetKeyFrame = m_pTargetKeyFrame->NextKey(direction);
		}
	}
	else
	{
		// we're changing direction

		// need to calculate current position in the frame
		// stop first, then start again
		if ( m_iDirection != 0 )
		{
			StopMoving();
		}

		m_iDirection = direction;

		// if we're going in reverse, swap the currentkey and targetkey (since we're going opposite dir)
		if ( direction == 1 )
		{
			m_pTargetKeyFrame = m_pCurrentKeyFrame->NextKey( direction );
		}
		else if ( direction == -1 )
		{
			if ( m_flTimeIntoFrame > 0 )
			{
				m_pTargetKeyFrame = m_pCurrentKeyFrame;
				m_pCurrentKeyFrame = m_pCurrentKeyFrame->NextKey( 1 );
			}
			else
			{
				m_pTargetKeyFrame = m_pCurrentKeyFrame->PrevKey( 1 );
			}
		}

		// recalculate our movement from the stored data
		if ( !m_pTargetKeyFrame )
		{
			StopMoving();
			return false;
		}

		// calculate the keyframes before and after the keyframes we're interpolating between
		m_pPostKeyFrame = m_pTargetKeyFrame->NextKey( direction );
		if ( !m_pPostKeyFrame )
		{
			m_pPostKeyFrame = m_pTargetKeyFrame;
		}
		m_pPreKeyFrame = m_pCurrentKeyFrame->PrevKey( direction );
		if ( !m_pPreKeyFrame )
		{
			m_pPreKeyFrame = m_pCurrentKeyFrame;
		}
	}

	// no target, can't move
	if ( !m_pTargetKeyFrame )
		return false;
	
	// calculate start/end time
	// ->m_flNextTime is the time to traverse to the NEXT key, so we need the opposite if travelling backwards
	if ( m_iDirection == 1 )
	{
		m_flAnimStartTime = GetLocalTime() - m_flTimeIntoFrame;
		m_flAnimEndTime = GetLocalTime() + m_pCurrentKeyFrame->m_flNextTime - m_flTimeIntoFrame;
	}
	else
	{
		// flip the timing, since we're in reverse
		if ( m_flTimeIntoFrame )
			m_flTimeIntoFrame = m_pTargetKeyFrame->m_flNextTime - m_flTimeIntoFrame;

		m_flAnimStartTime = GetLocalTime() - m_flTimeIntoFrame;
		m_flAnimEndTime = GetLocalTime() + m_pTargetKeyFrame->m_flNextTime - m_flTimeIntoFrame;
	}

	// calculate the average speed at which we cross 
	float animDuration = (m_flAnimEndTime - m_flAnimStartTime);
	float dist = (m_pCurrentKeyFrame->m_Origin - m_pTargetKeyFrame->m_Origin).Length();
	m_flAverageSpeedAcrossFrame = animDuration / dist;

	SetMoveDoneTime( m_flAnimEndTime - GetLocalTime() );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: stops the object from moving
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CBaseMoveBehavior::StopMoving( void )
{
	// remember exactly where we are in the frame
	m_flTimeIntoFrame = 0;

	if ( m_iDirection == 1 )
	{
		// record the time if we're not at the end of the frame
		if ( GetLocalTime() < m_flAnimEndTime )
		{
			m_flTimeIntoFrame = GetLocalTime() - m_flAnimStartTime;
		}
		else
		{
			// we're actually at the end
			if ( m_pTargetKeyFrame )
			{
				m_pCurrentKeyFrame = m_pTargetKeyFrame;
			}
		}
	}
	else if ( m_iDirection == -1 )
	{
		// store it only as a forward movement
		m_pCurrentKeyFrame = m_pTargetKeyFrame;

		if ( GetLocalTime() < m_flAnimEndTime )
		{
			m_flTimeIntoFrame = m_flAnimEndTime - GetLocalTime();
		}
	}

	// stop moving totally
	SetMoveDoneTime( -1 );
	m_iDirection = 0;
	m_flAnimStartTime = 0;
	m_flAnimEndTime = 0;
	m_pTargetKeyFrame = NULL;
	SetAbsVelocity(vec3_origin);
	SetLocalAngularVelocity( vec3_angle );
}


//-----------------------------------------------------------------------------
// Purpose: We have just arrived at a key, move onto the next keyframe
//-----------------------------------------------------------------------------
void CBaseMoveBehavior::MoveDone( void )
{
	// if we're just a base then keep playing the anim
	if ( !stricmp(STRING(m_iClassname), "move_keyframed") )
	{
		int direction = m_iDirection;
		// start moving from the keyframe we've just reached
		if ( !StartMoving(direction) )
		{
			// try moving in the other direction
			StartMoving( -direction );
		}
	}

	BaseClass::MoveDone();
}

//-----------------------------------------------------------------------------
// Purpose: Calculates a new moveTime based on the speed and the current point
//			in the animation.
//			used to advance keyframed objects that have dynamic speeds.
// Input  : moveTime - 
// Output : float - the new time in the keyframing sequence
//-----------------------------------------------------------------------------
float CBaseMoveBehavior::CalculateTimeAdvancementForSpeed( float moveTime, float speed )
{
	return (moveTime * speed * m_flAverageSpeedAcrossFrame);
}


//-----------------------------------------------------------------------------
// Purpose: 
//			GetLocalTime() is the objects local current time
// Input  : destTime - new time that is being moved to
//			moveTime - amount of time to be advanced this frame
// Output : float - the actual amount of time to move (usually moveTime)
//-----------------------------------------------------------------------------
float CBaseMoveBehavior::SetObjectPhysicsVelocity( float moveTime )
{
	// make sure we have a valid set up
	if ( !m_pCurrentKeyFrame || !m_pTargetKeyFrame )
		return moveTime;

	// if we're not moving, we're not moving
	if ( !IsMoving() )
		return moveTime;
	
	float destTime = moveTime + GetLocalTime();

	// work out where we want to be, using destTime
	m_flTimeIntoFrame = destTime - m_flAnimStartTime;
	float newTime = (destTime - m_flAnimStartTime) / (m_flAnimEndTime - m_flAnimStartTime);
	Vector newPos;
	QAngle newAngles;

	IPositionInterpolator *pInterp = GetPositionInterpolator( m_iPositionInterpolator );
	if( pInterp )
	{
		// setup key frames
		pInterp->SetKeyPosition( -1, m_pPreKeyFrame->m_Origin );
		Motion_SetKeyAngles( -1, m_pPreKeyFrame->m_qAngle );

		pInterp->SetKeyPosition( 0, m_pCurrentKeyFrame->m_Origin );
		Motion_SetKeyAngles( 0, m_pCurrentKeyFrame->m_qAngle );

		pInterp->SetKeyPosition( 1, m_pTargetKeyFrame->m_Origin );
		Motion_SetKeyAngles( 1, m_pTargetKeyFrame->m_qAngle );

		pInterp->SetKeyPosition( 2, m_pPostKeyFrame->m_Origin );
		Motion_SetKeyAngles( 2, m_pPostKeyFrame->m_qAngle );

		// find new interpolated position & rotation
		pInterp->InterpolatePosition( newTime, newPos );
	}
	else
	{
		newPos.Init();
	}

	Quaternion qRot;
	Motion_InterpolateRotation( newTime, m_iRotationInterpolator, qRot );
	QuaternionAngles( qRot, newAngles );

	// find our velocity vector (newPos - currentPos) and scale velocity vector according to the movetime
	float oneOnMoveTime = 1 / moveTime;
	SetAbsVelocity( (newPos - GetLocalOrigin()) * oneOnMoveTime );
	SetLocalAngularVelocity( (newAngles - GetLocalAngles()) * oneOnMoveTime );

	return moveTime;
}

