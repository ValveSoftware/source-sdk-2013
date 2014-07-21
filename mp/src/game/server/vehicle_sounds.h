//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef VEHICLE_SOUNDS_H
#define VEHICLE_SOUNDS_H
#ifdef _WIN32
#pragma once
#endif

#include "vcollide_parse.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
enum vehiclesound
{
	VS_SKID_FRICTION_LOW,
	VS_SKID_FRICTION_NORMAL,
	VS_SKID_FRICTION_HIGH,
	VS_ENGINE2_START,
	VS_ENGINE2_STOP,
	VS_MISC1,
	VS_MISC2,
	VS_MISC3,
	VS_MISC4,

	VS_NUM_SOUNDS,
};

extern const char *vehiclesound_parsenames[VS_NUM_SOUNDS];

// This is a list of vehiclesounds to automatically stop when the vehicle's driver exits the vehicle
#define NUM_SOUNDS_TO_STOP_ON_EXIT	4
extern vehiclesound g_iSoundsToStopOnExit[NUM_SOUNDS_TO_STOP_ON_EXIT];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct vehicle_gear_t
{
	DECLARE_DATADESC();

	float		flMinSpeed;
	float		flMaxSpeed;
	float		flSpeedApproachFactor;
};

struct vehicle_crashsound_t
{
	DECLARE_DATADESC();

	float		flMinSpeed;
	float		flMinDeltaSpeed;
	int			gearLimit;
	string_t	iszCrashSound;
};

enum sound_states
{
	SS_NONE = 0,
	SS_SHUTDOWN,
	SS_SHUTDOWN_WATER,
	SS_START_WATER,
	SS_START_IDLE,
	SS_IDLE,
	SS_GEAR_0,
	SS_GEAR_1,
	SS_GEAR_2,
	SS_GEAR_3,
	SS_GEAR_4,
	SS_SLOWDOWN,
	SS_SLOWDOWN_HIGHSPEED,	// not a real state, just a slot for state sounds
	SS_GEAR_0_RESUME,
	SS_GEAR_1_RESUME,
	SS_GEAR_2_RESUME,
	SS_GEAR_3_RESUME,
	SS_GEAR_4_RESUME,
	SS_TURBO,
	SS_REVERSE,

	SS_NUM_STATES,
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct vehiclesounds_t
{
	void Init( void )
	{
		pGears.Purge();
		crashSounds.Purge();
		
		for ( int i = 0; i < VS_NUM_SOUNDS; i++ )
		{
			iszSound[i] = NULL_STRING;
		}

		for ( int i = 0; i < SS_NUM_STATES; i++ )
		{
			iszStateSounds[i] = NULL_STRING;
			minStateTime[i] = 0.0f;
		}
	}

	DECLARE_DATADESC();

	CUtlVector<vehicle_gear_t>	pGears;
	CUtlVector<vehicle_crashsound_t> crashSounds;
	string_t					iszSound[ VS_NUM_SOUNDS ];
	string_t					iszStateSounds[SS_NUM_STATES];
	float						minStateTime[SS_NUM_STATES];
};

//-----------------------------------------------------------------------------
// Purpose: A KeyValues parse for vehicle sound blocks
//-----------------------------------------------------------------------------
class CVehicleSoundsParser : public IVPhysicsKeyHandler
{
public:
	CVehicleSoundsParser( void );

	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue );
	virtual void SetDefaults( void *pData );

private:
	// Index of the gear we're currently reading data into
	int	m_iCurrentGear;
	int	m_iCurrentState;
	int m_iCurrentCrashSound;
};

#endif // VEHICLE_SOUNDS_H
