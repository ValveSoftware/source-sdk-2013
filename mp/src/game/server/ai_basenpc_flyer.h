//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_BASENPC_FLYER_H
#define AI_BASENPC_FLYER_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_navigator.h"

//-----------------------------------------------------------------------------
// The combot.
//-----------------------------------------------------------------------------
abstract_class CAI_BaseFlyingBot : public CAI_BaseNPC
{
	DECLARE_CLASS( CAI_BaseFlyingBot, CAI_BaseNPC );
public:
	DECLARE_DATADESC();

	void			StartTask( const Task_t *pTask );
	void			GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity);
	virtual QAngle	BodyAngles();

protected:
	
	CAI_BaseFlyingBot();

	Vector	VelocityToAvoidObstacles(float flInterval);
	virtual float	MinGroundDist(void);

	void TurnHeadToTarget( float flInterval, const Vector &moveTarget );

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

	AI_NavPathProgress_t ProgressFlyPath(   float flInterval, 
						const CBaseEntity *pNewTarget, 
						unsigned collisionMask, 
						bool bNewTrySimplify = true, 
						float strictPointTolerance = 32.0 );

	virtual float GetHeadTurnRate( void ) { return 15.0f; }	// Degrees per second

	const Vector &GetCurrentVelocity() const		{ return m_vCurrentVelocity; }
	void SetCurrentVelocity(const Vector &vNewVel)	{ m_vCurrentVelocity = vNewVel; }

	const Vector &GetNoiseMod() const				{ return m_vNoiseMod; }
	void SetNoiseMod( float x, float y, float z )	{ m_vNoiseMod.Init( x, y, z ); }
	void SetNoiseMod( const Vector &noise )			{ m_vNoiseMod = noise; }

	virtual void MoveToTarget(float flInterval, const Vector &MoveTarget) = 0;

	void TranslateNavGoal( CBaseEntity *pTarget, Vector &chasePosition );

	// -------------------------------
	//  Movement vars
	// -------------------------------
	Vector			m_vCurrentVelocity;
	Vector			m_vCurrentAngularVelocity;
	Vector			m_vCurrentBanking;
	Vector			m_vNoiseMod;
	float			m_fHeadYaw;
	Vector			m_vLastPatrolDir;
};

#endif // AI_BASENPC_FLYER_H
