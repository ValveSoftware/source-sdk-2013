//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
// Information about algorithmic stuff that can occur on both client + server
//
// In order to reduce network traffic, it's possible to create a algorithms 
// that will work on both the client and the server and be totally repeatable. 
// All we need do is to send down initial conditions and let the algorithm 
// compute the values at various times. Note that this algorithm will be called 
// at different times with different frequencies on the client and server.
//
// The trick here is that in order for it to be repeatable, the algorithm either
// cannot depend on random numbers, or, if it does, we need to make sure that 
// the random numbers generated are effectively done at the beginning of time, 
// so that differences in frame rate on client and server won't matter. It also 
// is important that the initial state sent across the network is identical 
// bitwise so that we produce the exact same results. Therefore no compression 
// should be used in the datatables. 
//
// Note also that each algorithm must have its own random number stream so that
// it cannot possibly interact with other code using random numbers that will
// be called at various different intervals on the client + server. Use the
// CUniformRandomStream class for this.
//
// There are two types of client-server neutral code: Code that doesn't interact
// with player prediction, and code that does. The code that doesn't interact 
// with player prediction simply has to be able to produce the result f(time) 
// where time is monotonically increasing. For prediction, we have to produce
// the result f(time) where time does *not* monotonically increase (time can be 
// anywhere between the "current" time and the prior 10 seconds).
//
// Code that is not used by player prediction can maintain state because later 
// calls will always compute the value at some future time. This computation can
// use random number generation, but with the following restriction: Your code 
// must generate exactly the same number of random numbers regardless of how 
// frequently the code is called.
//
// In specific, this means that all random numbers used must either be computed
// at init time, or must be used in an 'event-based form'. Namely, use random 
// numbers to compute the time at which events occur and the random inputs for
// those events.  When simulating forward, you must simulate all intervening
// time and generate the same number of random numbers.
//
// For functions planned to be used by player prediction, one method is to use
// some sort of stateless computation (where the only states are the initial
// state and time). Note that random number generators have state implicit in
// the number of calls made to that random number generator, and therefore you
// cannot call a random number generator unless you are able to 
//
// 1) Use a random number generator that can return the ith random number, namely:
//
//	float r = random( i );	// i == the ith number in the random sequence
//
// 2) Be able to accurately know at any given time t how many random numbers 
//		have already been generated (namely, compute the i in part 1 above).
//
// There is another alternative for code meant to be used by player prediction: 
// you could just store a history of 'events' from which you could completely 
// determine the value of f(time). That history would need to be at least 10
// seconds long, which is guaranteed to be longer than the amount of time that
// prediction would need. I've written a class which I haven't tested yet (but
// will be using soon) called CTimedEventQueue (currently located in 
// env_wind_shared.h) which I plan to use to solve my problem (getting wind to
// blow players).
//
//=============================================================================//
#include "cbase.h"
#include "env_wind_shared.h"
#include "soundenvelope.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "sharedInterface.h"
#include "renderparm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------
static Vector s_vecWindVelocity( 0, 0, 0 );

static CUtlLinkedList< CEnvWindShared * > s_windControllers;

CEnvWindShared::CEnvWindShared() : m_WindAveQueue(10), m_WindVariationQueue(10)
{
	m_pWindSound = NULL;
	s_windControllers.AddToTail( this );
}

CEnvWindShared::~CEnvWindShared()
{
	if (m_pWindSound)
	{
		CSoundEnvelopeController::GetController().Shutdown( m_pWindSound );
	}
	s_windControllers.FindAndRemove( this );
}

void CEnvWindShared::Init( int nEntIndex, int iRandomSeed, float flTime, 
						  int iInitialWindYaw, float flInitialWindSpeed )
{
	m_iEntIndex = nEntIndex;
	m_flWindAngleVariation = m_flWindSpeedVariation = 1.0f;
	m_flStartTime = m_flSimTime = m_flSwitchTime = m_flVariationTime = flTime;
	m_iWindSeed = iRandomSeed;
	m_Stream.SetSeed( iRandomSeed );
	m_WindVariationStream.SetSeed( iRandomSeed );
	m_iWindDir = m_iInitialWindDir = iInitialWindYaw;
	// Bound it for networking as a postive integer
	m_iInitialWindDir = (int)( anglemod( m_iInitialWindDir ) );

	m_flAveWindSpeed = m_flWindSpeed = m_flInitialWindSpeed = flInitialWindSpeed;

	/*
	// Cache in the wind sound...
	if (!g_pEffects->IsServer())
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pWindSound = controller.SoundCreate( -1, CHAN_STATIC, 
			"EnvWind.Loop", ATTN_NONE );
		controller.Play( m_pWindSound, 0.0f, 100 );
	}
	*/

	// Next time a change happens (which will happen immediately), it'll stop gusting
	m_bGusting = true;
}


//-----------------------------------------------------------------------------
// Computes wind variation
//-----------------------------------------------------------------------------

#define WIND_VARIATION_UPDATE_TIME 0.1f

void CEnvWindShared::ComputeWindVariation( float flTime )
{
	// The wind variation is updated every 10th of a second..
	while( flTime >= m_flVariationTime )
	{
		m_flWindAngleVariation = m_WindVariationStream.RandomFloat( -10, 10 );
		m_flWindSpeedVariation = 1.0 + m_WindVariationStream.RandomFloat( -0.2, 0.2 );
		m_flVariationTime += WIND_VARIATION_UPDATE_TIME;
	}
}



//-----------------------------------------------------------------------------
// Updates the wind sound
//-----------------------------------------------------------------------------
void CEnvWindShared::UpdateWindSound( float flTotalWindSpeed )
{
	if (!g_pEffects->IsServer())
	{
		float flDuration = random->RandomFloat( 1.0f, 2.0f );
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		// FIXME: Tweak with these numbers
		float flNormalizedWindSpeed = flTotalWindSpeed / 150.0f;
		if (flNormalizedWindSpeed > 1.0f)
			flNormalizedWindSpeed = 1.0f;
		float flPitch = 120 * Bias( flNormalizedWindSpeed, 0.3f ) + 100;
		float flVolume = 0.3f * Bias( flNormalizedWindSpeed, 0.3f ) + 0.7f;
		controller.SoundChangePitch( m_pWindSound, flPitch, flDuration );
		controller.SoundChangeVolume( m_pWindSound, flVolume, flDuration );
	}
}

//-----------------------------------------------------------------------------
// Updates the swaying of trees
//-----------------------------------------------------------------------------
#define TREE_SWAY_UPDATE_TIME 2.0f

void CEnvWindShared::UpdateTreeSway( float flTime )
{
#ifdef CLIENT_DLL
	while( flTime >= m_flSwayTime )
	{
		// Since the wind is constantly changing, but we need smooth values, we cache them off here.
		m_PrevSwayVector = m_CurrentSwayVector;
		m_CurrentSwayVector = m_currentWindVector;
		m_flSwayTime += TREE_SWAY_UPDATE_TIME;
	}

	// Update vertex shader
	float flPercentage = ( 1 - ( ( m_flSwayTime - flTime ) / TREE_SWAY_UPDATE_TIME ) );
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	// Dividing by 2 helps the numbers the shader is expecting stay in line with other expected game values.
	Vector vecWind = Lerp( flPercentage, m_PrevSwayVector, m_CurrentSwayVector ) / 2;
	pRenderContext->SetVectorRenderingParameter( VECTOR_RENDERPARM_WIND_DIRECTION, vecWind );
#endif
}


//-----------------------------------------------------------------------------
// Updates the wind speed
//-----------------------------------------------------------------------------

#define WIND_ACCELERATION	150.0f	// wind speed can accelerate this many units per second
#define WIND_DECELERATION	15.0f	// wind speed can decelerate this many units per second

float CEnvWindShared::WindThink( float flTime )
{
	// NOTE: This algorithm can be client-server neutal because we're using 
	// the random number generator to generate *time* at which the wind changes.
	// We therefore need to structure the algorithm so that no matter the
	// frequency of calls to this function we produce the same wind speeds...

	ComputeWindVariation( flTime );

	// Update Tree Sway
	UpdateTreeSway( flTime );

	while (true)
	{
		// First, simulate up to the next switch time...
		float flTimeToSwitch = m_flSwitchTime - m_flSimTime;
		float flMaxDeltaTime = flTime - m_flSimTime;

		bool bGotToSwitchTime = (flMaxDeltaTime > flTimeToSwitch);

		float flSimDeltaTime = bGotToSwitchTime ? flTimeToSwitch : flMaxDeltaTime;

		// Now that we've chosen 
		// either ramp up, or sleep till change
		bool bReachedSteadyState = true;
		if ( m_flAveWindSpeed > m_flWindSpeed )
		{
			m_flWindSpeed += WIND_ACCELERATION * flSimDeltaTime;
			if (m_flWindSpeed > m_flAveWindSpeed)
				m_flWindSpeed = m_flAveWindSpeed;
			else
				bReachedSteadyState = false;
		}
		else if ( m_flAveWindSpeed < m_flWindSpeed )
		{
			m_flWindSpeed -= WIND_DECELERATION * flSimDeltaTime;
			if (m_flWindSpeed < m_flAveWindSpeed)
				m_flWindSpeed = m_flAveWindSpeed;
			else
				bReachedSteadyState = false;
		}

		// Update the sim time

		// If we didn't get to a switch point, then we're done simulating for now 
		if (!bGotToSwitchTime)
		{
			m_flSimTime = flTime;

			// We're about to exit, let's set the wind velocity...
			QAngle vecWindAngle( 0, m_iWindDir + m_flWindAngleVariation, 0 );
			AngleVectors( vecWindAngle, &m_currentWindVector );
			float flTotalWindSpeed = m_flWindSpeed * m_flWindSpeedVariation;
			m_currentWindVector *= flTotalWindSpeed;

			// If we reached a steady state, we don't need to be called until the switch time
			// Otherwise, we should be called immediately

			// FIXME: If we ever call this from prediction, we'll need
			// to only update the sound if it's a new time
			// Or, we'll need to update the sound elsewhere.
			// Update the sound....
//			UpdateWindSound( flTotalWindSpeed );

			// Always immediately call, the wind is forever varying
			return ( flTime + 0.01f );
		}

		m_flSimTime = m_flSwitchTime;

		// Switch gusting state..
		if( m_bGusting )
		{
			// wind is gusting, so return to normal wind
			m_flAveWindSpeed = m_Stream.RandomInt( m_iMinWind, m_iMaxWind );

			// set up for another gust later
			m_bGusting = false;
			m_flSwitchTime += m_flMinGustDelay + m_Stream.RandomFloat( 0, m_flMaxGustDelay );

#ifndef CLIENT_DLL
			m_OnGustEnd.FireOutput( NULL, NULL );
#endif
		}
		else
		{
			// time for a gust.
			m_flAveWindSpeed = m_Stream.RandomInt( m_iMinGust, m_iMaxGust );

			// change wind direction, maybe a lot
			m_iWindDir = anglemod( m_iWindDir + m_Stream.RandomInt(-m_iGustDirChange, m_iGustDirChange) );

			// set up to stop the gust in a short while
			m_bGusting = true;

#ifndef CLIENT_DLL
			m_OnGustStart.FireOutput( NULL, NULL );
#endif

			// !!!HACKHACK - gust duration tied to the length of a particular wave file
			m_flSwitchTime += m_flGustDuration;
		}
	}
}

void CEnvWindShared::Reset()
{
	m_currentWindVector.Init( 0, 0, 0 );
}

//-----------------------------------------------------------------------------
// Method to reset windspeed..
//-----------------------------------------------------------------------------
void ResetWindspeed()
{
	FOR_EACH_LL( s_windControllers, it )
	{
		s_windControllers[it]->Reset();
	}
}

//-----------------------------------------------------------------------------
// GetWindspeedAtTime was never finished to actually take time in to consideration.  We don't need 
// features that aren't written, but we do need to have multiple wind controllers on a map, so
// we need to find the one that is affecting the given location and return its speed.
//-----------------------------------------------------------------------------
Vector GetWindspeedAtLocation( const Vector &location )
{
	FOR_EACH_LL( s_windControllers, it )
	{
		CEnvWindShared *thisWindController = s_windControllers[it];
		float distance = (thisWindController->m_location - location).Length();

		if( distance < thisWindController->m_windRadius )
		{
			// This location is within our area of influence, so return our computer wind vector
			return thisWindController->m_currentWindVector;
		}
	}

	FOR_EACH_LL( s_windControllers, it )
	{
		CEnvWindShared *thisWindController = s_windControllers[it];

		if( thisWindController->m_windRadius == -1.0f )
		{
			// We do a second search for a global controller so you don't have to worry about order in the list.  
			return thisWindController->m_currentWindVector;
		}
	}

	return Vector(0,0,0);// No wind
}


//-----------------------------------------------------------------------------
// Method to sample the windspeed at a particular time
//-----------------------------------------------------------------------------
void GetWindspeedAtTime( float flTime, Vector &vecVelocity )
{
	// For now, ignore history and time.. fix later when we use wind to affect
	// client-side prediction
	if ( s_windControllers.Count() == 0 )
	{
		vecVelocity.Init( 0, 0, 0 );
	}
	else
	{
		VectorCopy( s_windControllers[ s_windControllers.Head() ]->m_currentWindVector, vecVelocity );
	}
}
