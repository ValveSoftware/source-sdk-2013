//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// c_fish.cpp
// Simple fish client-side logic
// Author: Michael S. Booth, April 2005

#include "cbase.h"
#include <bitbuf.h>
#include "engine/ivdebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern float UTIL_WaterLevel( const Vector &position, float minz, float maxz );


ConVar FishDebug( "fish_debug", "0", FCVAR_CHEAT, "Show debug info for fish" );


//-----------------------------------------------------------------------------
/**
 * Client-side fish entity
 */
class C_Fish : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_Fish, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	virtual void Spawn( void );
	virtual void ClientThink();

	virtual void OnDataChanged( DataUpdateType_t type );

private:
	friend void RecvProxy_FishOriginX( const CRecvProxyData *pData, void *pStruct, void *pOut );
	friend void RecvProxy_FishOriginY( const CRecvProxyData *pData, void *pStruct, void *pOut );

	Vector m_pos;						///< local position
	Vector m_vel;						///< local velocity
	QAngle m_angles;					///< local angles

	int m_localLifeState;				///< our version of m_lifeState

	float m_deathDepth;					///< water depth when we died
	float m_deathAngle;					///< angle to float at when dead
	float m_buoyancy;					///< so each fish floats at a different rate when dead

	CountdownTimer m_wiggleTimer;		///< for simulating swimming motions
	float m_wigglePhase;				///< where in the wiggle sinusoid we are
	float m_wiggleRate;					///< the speed of our wiggling

	Vector m_actualPos;					///< position from server
	QAngle m_actualAngles;				///< angles from server

	Vector m_poolOrigin;
	float m_waterLevel;					///< Z coordinate of water surface

	bool m_gotUpdate;					///< true after we have received a network update

	enum { MAX_ERROR_HISTORY = 20 };
	float m_errorHistory[ MAX_ERROR_HISTORY ];	///< error history samples
	int m_errorHistoryIndex;
	int m_errorHistoryCount;
	float m_averageError;
};


//-----------------------------------------------------------------------------
void RecvProxy_FishOriginX( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Fish *fish = (C_Fish *)pStruct;
	float *out = (float *)pOut;

	*out = pData->m_Value.m_Float + fish->m_poolOrigin.x;
}

void RecvProxy_FishOriginY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_Fish *fish = (C_Fish *)pStruct;
	float *out = (float *)pOut;

	*out = pData->m_Value.m_Float + fish->m_poolOrigin.y;
}


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_Fish, DT_CFish, CFish )

	RecvPropVector( RECVINFO(m_poolOrigin) ),

	RecvPropFloat( RECVINFO_NAME( m_actualPos.x, m_x ), 0, RecvProxy_FishOriginX ),
	RecvPropFloat( RECVINFO_NAME( m_actualPos.y, m_y ), 0, RecvProxy_FishOriginY ),
	RecvPropFloat( RECVINFO_NAME( m_actualPos.z, m_z ) ),

	RecvPropFloat( RECVINFO_NAME( m_actualAngles.y, m_angle ) ),

	RecvPropInt( RECVINFO(m_nModelIndex) ),
	RecvPropInt( RECVINFO(m_lifeState) ),

	RecvPropFloat( RECVINFO(m_waterLevel) ),		///< get this from the server in case we die when slightly out of the water due to error correction

END_RECV_TABLE()



//-----------------------------------------------------------------------------
void C_Fish::Spawn( void )
{
	BaseClass::Spawn();

	m_angles = QAngle( 0, 0, 0 );
	m_actualAngles = m_angles;

	m_vel = Vector( 0, 0, 0 );
	m_gotUpdate = false;
	m_localLifeState = LIFE_ALIVE;
	m_buoyancy = RandomFloat( 0.4f, 1.0f );

	m_errorHistoryIndex = 0;
	m_errorHistoryCount = 0;
	m_averageError = 0.0f;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


//-----------------------------------------------------------------------------
void C_Fish::ClientThink()
{
	if (FishDebug.GetBool())
	{
		debugoverlay->AddLineOverlay( m_pos, m_actualPos, 255, 0, 0, true, 0.1f );
		switch( m_localLifeState )
		{
			case LIFE_DYING:
				debugoverlay->AddTextOverlay( m_pos, 0.1f, "DYING" );
				break;

			case LIFE_DEAD:
				debugoverlay->AddTextOverlay( m_pos, 0.1f, "DEAD" );
				break;
		}
	}

	float deltaT = gpGlobals->frametime;


	// check if we just died
	if (m_localLifeState == LIFE_ALIVE && m_lifeState != LIFE_ALIVE)
	{
		// we have died
		m_localLifeState = LIFE_DYING;

		m_deathDepth = m_pos.z;

		// determine surface float angle
		m_deathAngle = RandomFloat( 87.0f, 93.0f ) * ((RandomInt( 0, 100 ) < 50) ? 1.0f : -1.0f);
	}


	switch( m_localLifeState )
	{
		case LIFE_DYING:
		{
			// depth parameter
			float t = (m_pos.z - m_deathDepth) / (m_waterLevel - m_deathDepth);
			t *= t;

			// roll onto side
			m_angles.z = m_deathAngle * t;

			// float to surface
			const float fudge = 2.0f;
			if (m_pos.z < m_waterLevel - fudge)
			{
				m_vel.z += (1.0f - t) * m_buoyancy * deltaT;
			}
			else
			{
				m_localLifeState = LIFE_DEAD;
			}

			break;
		}

		case LIFE_DEAD:
		{
			// depth parameter
			float t = (m_pos.z - m_deathDepth) / (m_waterLevel - m_deathDepth);
			t *= t;

			// roll onto side
			m_angles.z = m_deathAngle * t;

			// keep near water surface
			const float sub = 0.5f;
			m_vel.z += 10.0f * (m_waterLevel - m_pos.z - sub) * deltaT;

			// bob on surface
			const float rollAmp = 5.0f;
			const float rollFreq = 2.33f;
			m_angles.z += rollAmp * sin( rollFreq * (gpGlobals->curtime + 10.0f * entindex()) ) * deltaT;

			const float rollAmp2 = 7.0f;
			const float rollFreq2 = 4.0f;
			m_angles.x += rollAmp2 * sin( rollFreq2 * (gpGlobals->curtime + 10.0f * entindex()) ) * deltaT;

			const float bobAmp = 0.75f;
			const float bobFreq = 4.0f;
			m_vel.z += bobAmp * sin( bobFreq * (gpGlobals->curtime + 10.0f * entindex()) ) * deltaT;

			const float bobAmp2 = 0.75f;
			const float bobFreq2 = 3.333f;
			m_vel.z += bobAmp2 * sin( bobFreq2 * (gpGlobals->curtime + 10.0f * entindex()) ) * deltaT;

			// decay movement speed to zero
			const float drag = 1.0f;
			m_vel.z -= drag * m_vel.z * deltaT;

			break;
		}

		case LIFE_ALIVE:
		{
			// use server-side Z coordinate directly
			m_pos.z = m_actualPos.z;

			// use server-side angles
			m_angles = m_actualAngles;

			// fishy wiggle based on movement
			if (!m_wiggleTimer.IsElapsed())
			{
				float swimPower = 1.0f - (m_wiggleTimer.GetElapsedTime() / m_wiggleTimer.GetCountdownDuration());
				const float amp = 6.0f * swimPower;
				float wiggle = amp * sin( m_wigglePhase );

				m_wigglePhase += m_wiggleRate * deltaT;

				// wiggle decay
				const float wiggleDecay = 5.0f;
				m_wiggleRate -= wiggleDecay * deltaT;

				m_angles.y += wiggle;
			}

			break;
		}
	}

	// compute error between our local position and actual server position
	Vector error = m_actualPos - m_pos;
	error.z = 0.0f;
	float errorLen = error.Length();

	if (m_localLifeState == LIFE_ALIVE)
	{
		// if error is far above average, start swimming
		const float wiggleThreshold = 2.0f;
		if (errorLen - m_averageError > wiggleThreshold)
		{
			// if error is large, we must have started swimming
			const float swimTime = 5.0f;
			m_wiggleTimer.Start( swimTime );

			m_wiggleRate = 2.0f * errorLen;

			const float maxWiggleRate = 30.0f;
			if (m_wiggleRate > maxWiggleRate)
			{
				m_wiggleRate = maxWiggleRate;
			}
		}

		// update average error
		m_errorHistory[ m_errorHistoryIndex++ ] = errorLen;
		if (m_errorHistoryIndex >= MAX_ERROR_HISTORY)
		{
			m_errorHistoryIndex = 0;
			m_errorHistoryCount = MAX_ERROR_HISTORY;
		}
		else if (m_errorHistoryCount < MAX_ERROR_HISTORY)
		{
			++m_errorHistoryCount;
		}

		m_averageError = 0.0f;
		if (m_errorHistoryCount)
		{
			for( int r=0; r<m_errorHistoryCount; ++r )
			{
				m_averageError += m_errorHistory[r];
			}
			m_averageError /= (float)m_errorHistoryCount;
		}
	}

	// keep fish motion smooth by correcting towards actual server position
	// NOTE: This only tracks XY motion
	const float maxError = 20.0f;
	float errorT = errorLen / maxError;
	if (errorT > 1.0f)
	{
		errorT = 1.0f;
	}

	// we want a nonlinear spring force for tracking
	errorT *= errorT;

	// as fish move faster, their error increases - use a stiffer spring when fast, and a weak one when slow
	const float trackRate = 0.0f + errorT * 115.0f;
	m_vel.x += trackRate * error.x * deltaT;
	m_vel.y += trackRate * error.y * deltaT;

	const float trackDrag = 2.0f + errorT * 6.0f;
	m_vel.x -= trackDrag * m_vel.x * deltaT;
	m_vel.y -= trackDrag * m_vel.y * deltaT;


	// euler integration
	m_pos += m_vel * deltaT;

	SetNetworkOrigin( m_pos );
	SetAbsOrigin( m_pos );

	SetNetworkAngles( m_angles );
	SetAbsAngles( m_angles );
}


//-----------------------------------------------------------------------------
void C_Fish::OnDataChanged( DataUpdateType_t type )
{
	//if (!m_gotUpdate)

	if (type == DATA_UPDATE_CREATED)
	{
		// initial update
		m_gotUpdate = true;

		m_pos = m_actualPos;
		m_vel = Vector( 0, 0, 0 );

		return;
	}
}

