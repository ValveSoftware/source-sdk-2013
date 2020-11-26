//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PRECIPITATION_SHARED_H
#define PRECIPITATION_SHARED_H
#ifdef _WIN32
#pragma once
#endif


// Types of precipitation
enum PrecipitationType_t
{
	PRECIPITATION_TYPE_RAIN = 0,
	PRECIPITATION_TYPE_SNOW,
	PRECIPITATION_TYPE_ASH,
	PRECIPITATION_TYPE_SNOWFALL,
	PRECIPITATION_TYPE_PARTICLERAIN,
	PRECIPITATION_TYPE_PARTICLEASH,
	PRECIPITATION_TYPE_PARTICLERAINSTORM,
	PRECIPITATION_TYPE_PARTICLESNOW,
	NUM_PRECIPITATION_TYPES
};

// Returns true if the precipitation type involves the new particle system code
// 
// NOTE: We can get away with >= PARTICLERAIN, but if you're adding any new precipitation types
// which DO NOT use the new particle system, please change this code to prevent it from being recognized
// as a particle type.
inline bool IsParticleRainType( PrecipitationType_t type )
{
	// m_nPrecipType == PRECIPITATION_TYPE_PARTICLERAIN || m_nPrecipType == PRECIPITATION_TYPE_PARTICLEASH 
	//		|| m_nPrecipType == PRECIPITATION_TYPE_PARTICLERAINSTORM || m_nPrecipType == PRECIPITATION_TYPE_PARTICLESNOW
	return type >= PRECIPITATION_TYPE_PARTICLERAIN;
}

#ifdef MAPBASE
#define SF_PRECIP_PARTICLE_CLAMP		(1 << 0) // Clamps particle types to the precipitation bounds; Mapbase uses this to compensate for the lack of blocker support.
#define SF_PRECIP_PARTICLE_NO_OUTER		(1 << 1) // Suppresses the outer particle system.
#endif


#endif // PRECIPITATION_SHARED_H
