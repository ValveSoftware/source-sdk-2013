//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_BASENPC_PHYSICSFLYER_H
#define AI_BASENPC_PHYSICSFLYER_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_navigator.h"

//-----------------------------------------------------------------------------
// The combot.
//-----------------------------------------------------------------------------
abstract_class CAI_BasePhysicsFlyingBot : public CAI_BaseNPC, public IMotionEvent
{
	DECLARE_CLASS( CAI_BasePhysicsFlyingBot, CAI_BaseNPC );
public:
	DECLARE_DATADESC();

	void			StartTask( const Task_t *pTask );
	void			GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity);
	virtual QAngle	BodyAngles();

	virtual bool	ShouldSavePhysics() { return true; }

protected:
	
	CAI_BasePhysicsFlyingBot();
	~CAI_BasePhysicsFlyingBot();

	Vector	VelocityToAvoidObstacles(float flInterval);
	virtual float	MinGroundDist(void);

	virtual void TurnHeadToTarget( float flInterval, const Vector &moveTarget );

	void MoveInDirection( float flInterval, const Vector &targetDir, 
						 float accelXY, float accelZ, float decay)
	{
		decay = ExponentialDecay( decay, 1.0, flInterval );
		accelXY *= flInterval;
		accelZ  *= flInterval;

		m_vCurrentVelocity.x = ( decay * m_vCurrentVelocity.x + accelXY * targetDir.x );
		m_vCurrentVelocity.y = ( decay * m_vCurrentVelocity.y + accelXY * targetDir.y );
		m_vCurrentVelocity.z = ( decay * m_vCurrentVelocity.z + accelZ  * targetDir.z );
	}

	void MoveToLocation( float flInterval, const Vector &target, 
						 float accelXY, float accelZ, float decay)
	{
		Vector targetDir = target - GetLocalOrigin();
		VectorNormalize(targetDir);

		MoveInDirection(flInterval, targetDir, accelXY, accelZ, decay);
	}

	void Decelerate( float flInterval, float decay )
	{
		decay *= flInterval;
		m_vCurrentVelocity.x = (decay * m_vCurrentVelocity.x);
		m_vCurrentVelocity.y = (decay * m_vCurrentVelocity.y);
		m_vCurrentVelocity.z = (decay * m_vCurrentVelocity.z);
	}

	void AddNoiseToVelocity( float noiseScale = 1.0 )
	{
		if( m_vNoiseMod.x )
		{
			m_vCurrentVelocity.x += noiseScale*sin(m_vNoiseMod.x * gpGlobals->curtime + m_vNoiseMod.x);
		}

		if( m_vNoiseMod.y )
		{
			m_vCurrentVelocity.y += noiseScale*cos(m_vNoiseMod.y * gpGlobals->curtime + m_vNoiseMod.y);
		}

		if( m_vNoiseMod.z )
		{
			m_vCurrentVelocity.z -= noiseScale*cos(m_vNoiseMod.z * gpGlobals->curtime + m_vNoiseMod.z);
		}
	}

	void LimitSpeed( float zLimit, float maxSpeed = -1 )
	{
		if ( maxSpeed == -1 )
			maxSpeed = m_flSpeed;
		if (m_vCurrentVelocity.Length() > maxSpeed)
		{
			VectorNormalize(m_vCurrentVelocity);
			m_vCurrentVelocity *= maxSpeed;
		}
		// Limit fall speed
		if (zLimit > 0 && m_vCurrentVelocity.z < -zLimit)
		{
			m_vCurrentVelocity.z = -zLimit;
		}
	}

	AI_NavPathProgress_t ProgressFlyPath( float flInterval,
										  const CBaseEntity *pNewTarget, 
										  unsigned collisionMask, 
										  bool bNewTrySimplify = true, 
										  float strictPointTolerance = 32.0 );

	const Vector &GetCurrentVelocity() const		{ return m_vCurrentVelocity; }
	void SetCurrentVelocity(const Vector &vNewVel)	{ m_vCurrentVelocity = vNewVel; }

	const Vector &GetNoiseMod() const				{ return m_vNoiseMod; }
	void SetNoiseMod( float x, float y, float z )	{ m_vNoiseMod.Init( x, y, z ); }
	void SetNoiseMod( const Vector &noise )			{ m_vNoiseMod = noise; }

	void TranslateNavGoal( CBaseEntity *pTarget, Vector &chasePosition );

	virtual void MoveToTarget(float flInterval, const Vector &MoveTarget) = 0;

	virtual float GetHeadTurnRate( void ) { return 15.0f; }	// Degrees per second

	bool			CreateVPhysics( void );
	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

	virtual void ClampMotorForces( Vector &linear, AngularImpulse &angular )
	{ 
		// limit reaction forces
		linear.x = clamp( linear.x, -3000.f, 3000.f );
		linear.y = clamp( linear.y, -3000.f, 3000.f );
		linear.z = clamp( linear.z, -3000.f, 3000.f );

		// add in weightlessness
		linear.z += 800.f;
	}

	// -------------------------------
	//  Movement vars
	// -------------------------------
	Vector			m_vCurrentVelocity;
	Vector			m_vCurrentBanking;
	Vector			m_vNoiseMod;
	float			m_fHeadYaw;
	Vector			m_vLastPatrolDir;
	IPhysicsMotionController	*m_pMotionController;
};

#endif // AI_BASENPC_PHYSICSFLYER_H
