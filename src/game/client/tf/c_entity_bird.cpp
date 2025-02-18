//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dove entity for the Meet the Medic tease.
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "c_baseanimating.h"
#include "usermessages.h"

#define ENTITY_FLYING_BIRD_MODEL	"models/props_forest/dove.mdl"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_EntityFlyingBird : public CBaseAnimating
{
	DECLARE_CLASS( C_EntityFlyingBird, CBaseAnimating );
public:
	void			InitFromServerData( float flyAngle, float flyAngleRate, float flAccelZ, float flSpeed, float flGlideTime );
	virtual void	Touch( CBaseEntity *pOther ); 

private:
	virtual void	ClientThink( void );
	void			UpdateFlyDirection( void );

private:
	Vector m_flyForward;
	float m_flyAngle;
	float m_flyAngleRate;
	float m_flyZ;
	float m_accelZ;
	float m_speed;
	float m_timestamp;

	CountdownTimer m_lifetimeTimer;
	CountdownTimer m_glideTimer;
};

//-----------------------------------------------------------------------------
// Purpose: Server message that tells us to create a dove
//-----------------------------------------------------------------------------
USER_MESSAGE( SpawnFlyingBird )
{
	Vector vecPos;
	msg.ReadBitVec3Coord( vecPos );
	float flyAngle = msg.ReadFloat();
	float flyAngleRate = msg.ReadFloat();
	float flAccelZ = msg.ReadFloat();
	float flSpeed = msg.ReadFloat();
	float flGlideTime = msg.ReadFloat();

	C_EntityFlyingBird *pBird = new C_EntityFlyingBird();
	if ( !pBird )
		return;

	pBird->SetAbsOrigin( vecPos );
	pBird->InitFromServerData( flyAngle, flyAngleRate, flAccelZ, flSpeed, flGlideTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlyingBird::UpdateFlyDirection( void )
{
	Vector forward;

	forward.x = cos( m_flyAngle );
	forward.y = sin( m_flyAngle );
	forward.z = m_flyZ;
	forward.NormalizeInPlace();

	SetAbsVelocity( forward * m_speed );

	QAngle angles;
	VectorAngles( forward, angles );

	SetAbsAngles( angles );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlyingBird::InitFromServerData( float flyAngle, float flyAngleRate, float flAccelZ, float flSpeed, float flGlideTime )
{
	if ( InitializeAsClientEntity( ENTITY_FLYING_BIRD_MODEL, RENDER_GROUP_OPAQUE_ENTITY ) == false )
	{
		Release();
		return;
	}

	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	SetSize( -Vector(8,8,0), Vector(8,8,16) );

	m_flyAngle = flyAngle;
	m_flyAngleRate = flyAngleRate;
	m_accelZ = flAccelZ;
	m_flyZ = 0.0;
	m_speed = flSpeed;

	UpdateFlyDirection();

	SetSequence( 0 );
	SetPlaybackRate( 1.0f );
	SetCycle( 0 );
	ResetSequenceInfo();

	// make sure the bird is removed
	m_lifetimeTimer.Start( 10.0f );

	m_glideTimer.Start( flGlideTime );

	SetNextClientThink( CLIENT_THINK_ALWAYS );
	m_timestamp = gpGlobals->curtime;

	SetModelScale( 0.1f );
	SetModelScale( 1.0f, 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: Fly away!
//-----------------------------------------------------------------------------
void C_EntityFlyingBird::ClientThink( void )
{
	if ( m_lifetimeTimer.IsElapsed() )
	{
		Release();
		return;
	}

	if ( m_glideTimer.HasStarted() && m_glideTimer.IsElapsed() )
	{
		SetSequence( 1 );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
		ResetSequenceInfo();
		m_glideTimer.Invalidate();
	}

	StudioFrameAdvance();

	PhysicsSimulate();

	const float deltaT = gpGlobals->curtime - m_timestamp;
	m_flyAngle += m_flyAngleRate * deltaT;
	m_flyZ += m_accelZ * deltaT;

	UpdateFlyDirection();

	m_timestamp = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityFlyingBird::Touch( CBaseEntity *pOther )
{
	if ( !pOther || !pOther->IsWorld() )
		return;
	
	BaseClass::Touch( pOther );

	// Die at next think. Not safe to remove ourselves during physics touch.
	m_lifetimeTimer.Invalidate();
}

