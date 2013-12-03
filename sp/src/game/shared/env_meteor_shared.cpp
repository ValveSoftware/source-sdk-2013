//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "env_meteor_shared.h"
#include "mapdata_shared.h"
#include "sharedInterface.h"

//=============================================================================
//
// Meteor Functions.
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CEnvMeteorShared::CEnvMeteorShared()
{
	m_nID = 0;
	m_vecStartPosition.Init();
	m_vecDirection.Init();
	m_flSpeed = 0.0f;
	m_flDamageRadius = 0.0f;
	m_flStartTime = METEOR_INVALID_TIME;
	m_flPassiveTime = METEOR_INVALID_TIME;
	m_flWorldEnterTime = METEOR_INVALID_TIME;
	m_flWorldExitTime = METEOR_INVALID_TIME;
	m_nLocation = METEOR_LOCATION_INVALID;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvMeteorShared::Init( int nID, float flStartTime, float flPassiveTime, 
							 const Vector &vecStartPosition, 
							 const Vector &vecDirection, float flSpeed, float flDamageRadius,
							 const Vector &vecTriggerMins, const Vector &vecTriggerMaxs )
{
	// Setup initial parametric state.
	m_nID = nID;
	VectorCopy( vecStartPosition, m_vecStartPosition );
	VectorCopy( vecStartPosition, m_vecPos );
	VectorCopy( vecDirection, m_vecDirection );
	m_flSpeed = flSpeed;
	m_flDamageRadius = flDamageRadius;
	m_flStartTime = flPassiveTime + flStartTime;
	m_flPassiveTime = flPassiveTime;
	m_flPosTime = m_flStartTime;

	// Calculate the enter/exit times.
	CalcEnterAndExitTimes( vecTriggerMins, vecTriggerMaxs );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvMeteorShared::GetPositionAtTime( float flTime, Vector &vecPosition )
{
	float flDeltaTime = flTime - m_flPosTime;
	Vector vecVelocity( m_vecDirection.x * m_flSpeed, m_vecDirection.y * m_flSpeed, m_vecDirection.z * m_flSpeed );
	VectorMA( m_vecPos, flDeltaTime, vecVelocity, vecPosition );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvMeteorShared::ConvertFromSkyboxToWorld( void )
{
	// The new start position is the position at which the meteor enters
	// the skybox.
	Vector vecSkyboxOrigin;
	g_pMapData->Get3DSkyboxOrigin( vecSkyboxOrigin );
	float flSkyboxScale = g_pMapData->Get3DSkyboxScale();

	m_vecPos += ( m_flSpeed * m_vecDirection ) * ( m_flWorldEnterTime - m_flStartTime );
	m_vecPos -= vecSkyboxOrigin;
	m_vecPos *= flSkyboxScale;

	// Scale the speed.
	m_flSpeed *= flSkyboxScale;

	// Reset the start time.
	m_flPosTime = m_flWorldEnterTime;

	// Set the location to world.
	m_nLocation = METEOR_LOCATION_WORLD;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvMeteorShared::ConvertFromWorldToSkybox( void )
{
	// Scale the speed.
	float flSkyboxScale = g_pMapData->Get3DSkyboxScale();
	m_flSpeed /= flSkyboxScale;

	float flDeltaTime = m_flWorldExitTime - m_flStartTime;
	Vector vecVelocity( m_vecDirection.x * m_flSpeed, m_vecDirection.y * m_flSpeed, m_vecDirection.z * m_flSpeed );
	VectorMA( m_vecStartPosition, flDeltaTime, vecVelocity, m_vecPos );
	
	// Reset the start time.
	m_flPosTime = m_flWorldExitTime;

	// Set the location to skybox.
	m_nLocation = METEOR_LOCATION_SKYBOX;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CEnvMeteorShared::IsInSkybox( float flTime )
{
	// Check to see if we are always in the skybox!
	if ( m_flWorldEnterTime == METEOR_INVALID_TIME )
		return true;

	return ( ( flTime < m_flWorldEnterTime ) || ( flTime > m_flWorldExitTime ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CEnvMeteorShared::IsPassive( float flTime )
{
	return ( flTime < m_flPassiveTime );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CEnvMeteorShared::WillTransition( void )
{
	return ( m_flWorldEnterTime == METEOR_INVALID_TIME );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CEnvMeteorShared::GetDamageRadius( void )
{
	return m_flDamageRadius;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvMeteorShared::CalcEnterAndExitTimes( const Vector &vecTriggerMins, 
											  const Vector &vecTriggerMaxs )
{
#define METEOR_TRIGGER_EPSILON	0.001f

	// Initialize the enter/exit fractions.
	float flEnterFrac = 0.0f;
	float flExitFrac = 1.0f;

	// Create an arbitrarily large end position.
	Vector vecEndPosition;
	VectorMA( m_vecStartPosition, 32000.0f, m_vecDirection, vecEndPosition );

	float flFrac, flDistStart, flDistEnd;
	for( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		// Negative Axis
		flDistStart = -m_vecStartPosition[iAxis] + vecTriggerMins[iAxis];
		flDistEnd = -vecEndPosition[iAxis] + vecTriggerMins[iAxis];

		if ( ( flDistStart > 0.0f ) && ( flDistEnd < 0.0f ) ) 
		{ 
			flFrac = ( flDistStart - METEOR_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if ( flFrac > flEnterFrac ) { flEnterFrac = flFrac; }
		}

		if ( ( flDistStart < 0.0f ) && ( flDistEnd > 0.0f ) ) 
		{ 
			flFrac = ( flDistStart + METEOR_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if( flFrac < flExitFrac ) { flExitFrac = flFrac; }
		}

		if ( ( flDistStart > 0.0f ) && ( flDistEnd > 0.0f ) )
			return;

		// Positive Axis
		flDistStart = m_vecStartPosition[iAxis] - vecTriggerMaxs[iAxis];
		flDistEnd = vecEndPosition[iAxis] - vecTriggerMaxs[iAxis];

		if ( ( flDistStart > 0.0f ) && ( flDistEnd < 0.0f ) ) 
		{ 
			flFrac = ( flDistStart - METEOR_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if ( flFrac > flEnterFrac ) { flEnterFrac = flFrac; }
		}

		if ( ( flDistStart < 0.0f ) && ( flDistEnd > 0.0f ) ) 
		{ 
			flFrac = ( flDistStart + METEOR_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if( flFrac < flExitFrac ) { flExitFrac = flFrac; }
		}

		if ( ( flDistStart > 0.0f ) && ( flDistEnd > 0.0f ) )
			return;
	}

	// Check for intersection.
	if ( flExitFrac >= flEnterFrac )
	{
		// Check to see if we start in the world or the skybox!
		if ( flEnterFrac == 0.0f )
		{
			m_nLocation = METEOR_LOCATION_WORLD;
		}
		else
		{
			m_nLocation = METEOR_LOCATION_SKYBOX;
		}

		// Calculate the enter/exit times.
		Vector vecEnterPoint, vecExitPoint, vecDeltaPosition;
		VectorSubtract( vecEndPosition, m_vecStartPosition, vecDeltaPosition );
		VectorScale( vecDeltaPosition, flEnterFrac, vecEnterPoint );
		VectorScale( vecDeltaPosition, flExitFrac, vecExitPoint );

		m_flWorldEnterTime = vecEnterPoint.Length() / m_flSpeed;
		m_flWorldExitTime = vecExitPoint.Length() / m_flSpeed;
		m_flWorldEnterTime += m_flStartTime;
		m_flWorldExitTime += m_flStartTime;
	}

#undef METEOR_TRIGGER_EPSILON
}

//=============================================================================
//
// Meteor Spawner Functions.
//

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CEnvMeteorSpawnerShared::CEnvMeteorSpawnerShared()
{
	m_pFactory = NULL;
	m_nMeteorCount = 0;
	
	m_flStartTime = 0.0f;
	m_nRandomSeed = 0;

	m_iMeteorType = -1;
	m_flMeteorDamageRadius = 0.0f;
	m_bSkybox = true;

	m_flMinSpawnTime = 0.0f;
	m_flMaxSpawnTime = 0.0f;
	m_nMinSpawnCount = 0;
	m_nMaxSpawnCount = 0;
	m_vecMinBounds.Init();
	m_vecMaxBounds.Init();
	m_flMinSpeed = 0.0f;
	m_flMaxSpeed = 0.0f;

	m_flNextSpawnTime = 0.0f;

	m_vecTriggerMins.Init();
	m_vecTriggerMaxs.Init();
	m_vecTriggerCenter.Init();

	// Debug!
	m_nRandomCallCount = 0;

	m_aTargets.Purge();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvMeteorSpawnerShared::Init( IMeteorFactory *pFactory, int nRandomSeed, float flTime,
									const Vector &vecMinBounds, const Vector &vecMaxBounds,
									const Vector &vecTriggerMins, const Vector &vecTriggerMaxs )
{
	// Factory.
	m_pFactory = pFactory;

	// Setup the random number stream.
	m_nRandomSeed = nRandomSeed;
	m_NumberStream.SetSeed( nRandomSeed );

	// Start time.
	m_flStartTime = flTime;

	// Copy the spawner bounds.
	m_vecMinBounds = vecMinBounds;
	m_vecMaxBounds = vecMaxBounds;

	// Copy the trigger bounds.
	m_vecTriggerMins = vecTriggerMins;
	m_vecTriggerMaxs = vecTriggerMaxs;

	// Get the center of the trigger bounds.
	m_vecTriggerCenter = ( m_vecTriggerMins + m_vecTriggerMaxs ) * 0.5f;

	// Setup spawn time.
	m_flNextSpawnTime = m_flStartTime + m_flMaxSpawnTime;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CEnvMeteorSpawnerShared::GetRandomInt( int nMin, int nMax )
{
	m_nRandomCallCount++;
	return m_NumberStream.RandomInt( nMin, nMax );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CEnvMeteorSpawnerShared::GetRandomFloat( float flMin, float flMax )
{
	m_nRandomCallCount++;
	return m_NumberStream.RandomFloat( flMin, flMax );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CEnvMeteorSpawnerShared::MeteorThink( float flTime )
{
	// Check for spawn.
	if ( flTime < m_flNextSpawnTime )
		return m_flNextSpawnTime;

	while ( m_flNextSpawnTime < flTime )
	{
		// Get a random number of meteors to spawn and spawn them.
		int nMeteorCount = GetRandomInt( m_nMinSpawnCount, m_nMaxSpawnCount );
		for ( int iMeteor = 0; iMeteor < nMeteorCount; iMeteor++ )
		{
			// Increment the number of meteors created (starting with 1).
			m_nMeteorCount++;

			// Get a random meteor position.
			Vector meteorOrigin( GetRandomFloat( m_vecMinBounds.GetX(), m_vecMaxBounds.GetX() ) /* x */,
				                 GetRandomFloat( m_vecMinBounds.GetY(), m_vecMaxBounds.GetY() ) /* y */,
				                 GetRandomFloat( m_vecMinBounds.GetZ(), m_vecMaxBounds.GetZ() ) /* z */ );
			
			// Calculate the direction of the meteor based on "targets."
			Vector vecDirection( 0.0f, 0.0f, -1.0f );
			if ( m_aTargets.Count() > 0 )
			{
				float flFreq = 1.0f / m_aTargets.Count();
				float flFreqAccum = flFreq;

				int iTarget;
				for( iTarget = 0; iTarget < m_aTargets.Count(); ++iTarget )
				{
					float flRandom = GetRandomFloat( 0.0f, 1.0f );
					if ( flRandom < flFreqAccum )
						break;

					flFreqAccum += flFreq;
				}

				// Should ever be here!
				if ( iTarget == m_aTargets.Count() )
				{
					iTarget--;
				}

				// Just set it to the first target for now!!!
				// NOTE: Will randomly generate from list of targets when more than 1 in
				//       the future.

				// Move the meteor into the "world."
				Vector vecPositionInWorld;
				Vector vecSkyboxOrigin;
				g_pMapData->Get3DSkyboxOrigin( vecSkyboxOrigin );
				vecPositionInWorld = ( meteorOrigin - vecSkyboxOrigin );
				vecPositionInWorld *= g_pMapData->Get3DSkyboxScale();

				Vector vecTargetPos = m_aTargets[iTarget].m_vecPosition;
				vecTargetPos.x += GetRandomFloat( -m_aTargets[iTarget].m_flRadius, m_aTargets[iTarget].m_flRadius );
				vecTargetPos.y += GetRandomFloat( -m_aTargets[iTarget].m_flRadius, m_aTargets[iTarget].m_flRadius );
				vecTargetPos.z += GetRandomFloat( -m_aTargets[iTarget].m_flRadius, m_aTargets[iTarget].m_flRadius );

				vecDirection = vecTargetPos - vecPositionInWorld;
				VectorNormalize( vecDirection );
			}
			
			// Pass in the randomized position, randomized speed, and start time.
			m_pFactory->CreateMeteor( m_nMeteorCount, m_iMeteorType, meteorOrigin,
				                      vecDirection /* direction */,
				                      GetRandomFloat( m_flMinSpeed, m_flMaxSpeed ) /* speed */,
									  m_flNextSpawnTime, m_flMeteorDamageRadius,
				                      m_vecTriggerMins, m_vecTriggerMaxs );
		}
		
		// Set next spawn time.
		m_flNextSpawnTime += GetRandomFloat( m_flMinSpawnTime, m_flMaxSpawnTime );
	}

	// Return the next spawn time.
	return ( m_flNextSpawnTime - gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvMeteorSpawnerShared::AddToTargetList( const Vector &vecPosition, float flRadius )
{
	int iTarget = m_aTargets.AddToTail();
	m_aTargets[iTarget].m_vecPosition = vecPosition;
	m_aTargets[iTarget].m_flRadius = flRadius;
}
