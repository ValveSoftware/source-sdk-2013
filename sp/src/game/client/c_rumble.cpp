//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Rumble effects mixer for XBox
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "c_rumble.h"
#include "rumble_shared.h"
#include "inputsystem/iinputsystem.h"

ConVar cl_rumblescale( "cl_rumblescale", "1.0", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Scale sensitivity of rumble effects (0 to 1.0)" ); 
ConVar cl_debugrumble( "cl_debugrumble", "0", FCVAR_ARCHIVE, "Turn on rumble debugging spew" );

#define MAX_RUMBLE_CHANNELS 3	// Max concurrent rumble effects

#define NUM_WAVE_SAMPLES 30		// Effects play at 10hz

typedef struct
{
	float			amplitude_left[NUM_WAVE_SAMPLES];
	float			amplitude_right[NUM_WAVE_SAMPLES];
	int				numSamples;
} RumbleWaveform_t;

//=========================================================
// Structure for a rumble effect channel. This is akin to
// a sound channel that is playing a sound.
//=========================================================
typedef struct
{
	float			starttime;			// When did this effect start playing? (gpGlobals->curtime)
	int				waveformIndex;		// Type of effect waveform used (an enum from rumble_shared.h)
	int				priority;			// How important this effect is (for making replacement decisions)
	bool			in_use;				// Is this channel in use?? (true if effect is currently playing, false if done or otherwise available)
	unsigned char	rumbleFlags;		// Flags pertaining to the effect currently playing on this channel.
	float			scale;				// Some effects are updated while they are running.
} RumbleChannel_t;

//=========================================================
// This structure contains parameters necessary to generate
// a sine or sawtooth waveform. 
//=========================================================
typedef struct tagWaveGenParams
{
	float	cycles;				// AKA frequency
	float	amplitudescale;	
	bool	leftChannel;		// If false, generating for the right channel

	float	maxAmplitude;		// Clamping
	float	minAmplitude;

	void Set( float c_cycles, float c_amplitudescale, bool c_leftChannel, float c_minAmplitude, float c_maxAmplitude )
	{
		cycles = c_cycles;
		amplitudescale = c_amplitudescale;
		leftChannel = c_leftChannel;
		minAmplitude = c_minAmplitude;
		maxAmplitude = c_maxAmplitude;
	}

	// CTOR
	tagWaveGenParams( float c_cycles, float c_amplitudescale, bool c_leftChannel, float c_minAmplitude, float c_maxAmplitude ) 
	{
		Set( c_cycles, c_amplitudescale, c_leftChannel, c_minAmplitude, c_maxAmplitude );
	}

} WaveGenParams_t;

//---------------------------------------------------------
//---------------------------------------------------------
void TerminateWaveform( RumbleWaveform_t *pWaveform, int samples )
{
	if( samples <= NUM_WAVE_SAMPLES )
	{
		pWaveform->numSamples = samples;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void EaseInWaveform( RumbleWaveform_t *pWaveform, int samples, bool left )
{
	float step = 1.0f / ((float)samples);
	float factor = 0.0f;

	for( int i = 0 ; i < samples ; i++ )
	{
		if( left )
		{
			pWaveform->amplitude_left[i] *= factor;
		}
		else
		{
			pWaveform->amplitude_right[i] *= factor;
		}

		factor += step;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void EaseOutWaveform( RumbleWaveform_t *pWaveform, int samples, bool left )
{
	float step = 1.0f / ((float)samples);
	float factor = 0.0f;

	int i = NUM_WAVE_SAMPLES - 1;

	for( int j = 0 ; j < samples ; j++ )
	{
		if( left )
		{
			pWaveform->amplitude_left[i] *= factor;
		}
		else
		{
			pWaveform->amplitude_right[i] *= factor;
		}

		factor += step;
		i--;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void GenerateSawtoothEffect( RumbleWaveform_t *pWaveform, const WaveGenParams_t &params )
{
	float delta = params.maxAmplitude - params.minAmplitude;
	int waveLength = NUM_WAVE_SAMPLES / params.cycles;
	float vstep = (delta / waveLength);

	float amplitude = params.minAmplitude;

	for( int i = 0 ; i < NUM_WAVE_SAMPLES ; i++ )
	{
		if( params.leftChannel )
		{
			pWaveform->amplitude_left[i] = amplitude;
		}
		else
		{
			pWaveform->amplitude_right[i] = amplitude;
		}

		amplitude += vstep;

		if( amplitude > params.maxAmplitude )
		{
			amplitude = params.minAmplitude;
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void GenerateSquareWaveEffect( RumbleWaveform_t *pWaveform, const WaveGenParams_t &params )
{
	int i = 0;
	int j;

	int steps = ((float)NUM_WAVE_SAMPLES) / (params.cycles*2.0f);

	while( i < NUM_WAVE_SAMPLES )
	{
		for( j = 0 ; j < steps ; j++ )
		{
			if( params.leftChannel )
			{
				pWaveform->amplitude_left[i++] = params.minAmplitude;
			}
			else
			{
				pWaveform->amplitude_right[i++] = params.minAmplitude;
			}
		}
		for( j = 0 ; j < steps ; j++ )
		{
			if( params.leftChannel )
			{
				pWaveform->amplitude_left[i++] = params.maxAmplitude;
			}
			else
			{
				pWaveform->amplitude_right[i++] = params.maxAmplitude;
			}
		}
	}
}

//---------------------------------------------------------
// If you pass a numSamples, this wave will only be that many
// samples long.
//---------------------------------------------------------
void GenerateFlatEffect( RumbleWaveform_t *pWaveform, const WaveGenParams_t &params )
{
	for( int i = 0 ; i < NUM_WAVE_SAMPLES ; i++ )
	{
		if( params.leftChannel )
		{
			pWaveform->amplitude_left[i] = params.maxAmplitude;
		}
		else
		{
			pWaveform->amplitude_right[i] = params.maxAmplitude;
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void GenerateSineWaveEffect( RumbleWaveform_t *pWaveform, const WaveGenParams_t &params )
{
	float step = (360.0f * (params.cycles * 0.5f) ) / ((float)NUM_WAVE_SAMPLES);
	float degrees = 180.0f + step; // 180 to start at 0

	for( int i = 0 ; i < NUM_WAVE_SAMPLES ; i++ )
	{
		float radians = DEG2RAD(degrees);
		float value = fabs( sin(radians) );

		value *= params.amplitudescale;

		if( value < params.minAmplitude )
			value = params.minAmplitude;

		if( value > params.maxAmplitude )
			value = params.maxAmplitude;

		if( params.leftChannel )
		{
			pWaveform->amplitude_left[i] = value;
		}
		else
		{
			pWaveform->amplitude_right[i] = value;
		}

		degrees += step;
	}
}

//=========================================================
//=========================================================
class CRumbleEffects
{
public:
	CRumbleEffects()
	{
		Init();
	}

	void Init();
	void SetOutputEnabled( bool bEnable );
	void StartEffect( unsigned char effectIndex, unsigned char rumbleData, unsigned char rumbleFlags );
	void StopEffect( int effectIndex );
	void StopAllEffects();
	void ComputeAmplitudes( RumbleChannel_t *pChannel, float curtime, float *pLeft, float *pRight );
	void UpdateEffects( float curtime );
	void UpdateScreenShakeRumble( float shake, float balance );

	RumbleChannel_t *FindExistingChannel( int index );
	RumbleChannel_t	*FindAvailableChannel( int priority );

public:
	RumbleChannel_t	m_Channels[ MAX_RUMBLE_CHANNELS ];

	RumbleWaveform_t m_Waveforms[ NUM_RUMBLE_EFFECTS ];

	float	m_flScreenShake;
	bool	m_bOutputEnabled;
};


CRumbleEffects g_RumbleEffects;

//---------------------------------------------------------
//---------------------------------------------------------
void CRumbleEffects::Init()
{
	SetOutputEnabled( true );
	
	int i;

	for( i = 0 ; i < MAX_RUMBLE_CHANNELS ; i++ )
	{
		m_Channels[i].in_use = false;
		m_Channels[i].priority = 0;
	}

	// Every effect defaults to this many samples. Call TerminateWaveform() to trim these.
	for ( i = 0 ; i < NUM_RUMBLE_EFFECTS ; i++ )
	{
		m_Waveforms[i].numSamples = NUM_WAVE_SAMPLES;
	}

	// Jeep Idle
	WaveGenParams_t params( 1, 1.0f, false, 0.0f, 0.15f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_JEEP_ENGINE_LOOP], params );

	// Pistol
	params.Set( 1, 1.0f, false, 0.0f, 0.5f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_PISTOL], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_PISTOL], 3 );

	// SMG1
	params.Set( 1, 1.0f, true, 0.0f, 0.2f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_SMG1], params );
	params.Set( 1, 1.0f, false, 0.0f, 0.4f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_SMG1], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_SMG1], 3 );

	// AR2
	params.Set( 1, 1.0f, true, 0.0f, 0.5f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_AR2], params );
	params.Set( 1, 1.0f, false, 0.0f, 0.3f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_AR2], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_AR2], 3 );

	// AR2 Alt
	params.Set( 1, 1.0f, true, 0.0, 0.5f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_AR2_ALT_FIRE], params );
	EaseInWaveform( &m_Waveforms[RUMBLE_AR2_ALT_FIRE], 5, true );
	params.Set( 1, 1.0f, false, 0.0, 0.7f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_AR2_ALT_FIRE], params );
	EaseInWaveform( &m_Waveforms[RUMBLE_AR2_ALT_FIRE], 5, false );
	TerminateWaveform( &m_Waveforms[RUMBLE_AR2_ALT_FIRE], 7 );

	// 357
	params.Set( 1, 1.0f, true, 0.0f, 0.75f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_357], params );
	params.Set( 1, 1.0f, false, 0.0f, 0.75f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_357], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_357], 2 );

	// Shotgun
	params.Set( 1, 1.0f, true, 0.0f, 0.8f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_SHOTGUN_SINGLE], params );
	params.Set( 1, 1.0f, false, 0.0f, 0.8f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_SHOTGUN_SINGLE], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_SHOTGUN_SINGLE], 3 );

	params.Set( 1, 1.0f, true, 0.0f, 1.0f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_SHOTGUN_DOUBLE], params );
	params.Set( 1, 1.0f, false, 0.0f, 1.0f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_SHOTGUN_DOUBLE], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_SHOTGUN_DOUBLE], 3 );

	// RPG Missile
	params.Set( 1, 1.0f, false, 0.0f, 0.3f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_RPG_MISSILE], params );
	EaseOutWaveform( &m_Waveforms[RUMBLE_RPG_MISSILE], 30, false );
	TerminateWaveform( &m_Waveforms[RUMBLE_RPG_MISSILE], 6 );

	// Physcannon open forks
	params.Set( 1, 1.0f, false, 0.0f, 0.25f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_PHYSCANNON_OPEN], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_PHYSCANNON_OPEN], 4 );

	// Physcannon holding something
	params.Set( 1, 1.0f, true, 0.0f, 0.2f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_PHYSCANNON_LOW], params );
	params.Set( 6, 1.0f, false, 0.0f, 0.25f );
	GenerateSquareWaveEffect( &m_Waveforms[RUMBLE_PHYSCANNON_LOW], params );

	// Crowbar
	params.Set( 1, 1.0f, false, 0.0f, 0.35f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_CROWBAR_SWING], params );
	EaseOutWaveform( &m_Waveforms[RUMBLE_CROWBAR_SWING], 30, false );
	TerminateWaveform( &m_Waveforms[RUMBLE_CROWBAR_SWING], 4 );

	// Airboat gun
	params.Set( 1, 1.0f, false, 0.0f, 0.4f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_AIRBOAT_GUN], params );
	params.Set( 12, 1.0f, true, 0.0f, 0.5f );
	GenerateSawtoothEffect( &m_Waveforms[RUMBLE_AIRBOAT_GUN], params );

	// Generic flat effects.
	params.Set( 1, 1.0f, true, 0.0f, 1.0f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FLAT_LEFT], params );

	params.Set( 1, 1.0f, false, 0.0f, 1.0f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FLAT_RIGHT], params );
	
	params.Set( 1, 1.0f, true, 0.0f, 1.0f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FLAT_BOTH], params );
	params.Set( 1, 1.0f, false, 0.0f, 1.0f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FLAT_BOTH], params );

	// Impact from a long fall
	params.Set( 1, 1.0f, false, 0.0f, 0.5f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FALL_LONG], params );
	params.Set( 1, 1.0f, true, 0.0f, 0.5f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FALL_LONG], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_FALL_LONG], 3 );

	// Impact from a short fall
	params.Set( 1, 1.0f, false, 0.0f, 0.3f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FALL_SHORT], params );
	params.Set( 1, 1.0f, true, 0.0f, 0.3f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_FALL_SHORT], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_FALL_SHORT], 2 );

	// Portalgun left (blue) shot
	params.Set( 1, 1.0f, true, 0.0f, 0.3f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_PORTALGUN_LEFT], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_PORTALGUN_LEFT], 2 );

	// Portalgun right (red) shot
	params.Set( 1, 1.0f, false, 0.0f, 0.3f );
	GenerateFlatEffect( &m_Waveforms[RUMBLE_PORTALGUN_RIGHT], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_PORTALGUN_RIGHT], 2 );

	// Portal failed to place feedback
	params.Set( 12, 1.0f, true, 0.0f, 1.0f );
	GenerateSquareWaveEffect( &m_Waveforms[RUMBLE_PORTAL_PLACEMENT_FAILURE], params );
	params.Set( 12, 1.0f, false, 0.0f, 1.0f );
	GenerateSquareWaveEffect( &m_Waveforms[RUMBLE_PORTAL_PLACEMENT_FAILURE], params );
	TerminateWaveform( &m_Waveforms[RUMBLE_PORTAL_PLACEMENT_FAILURE], 6 );
}

//---------------------------------------------------------
//---------------------------------------------------------
RumbleChannel_t *CRumbleEffects::FindExistingChannel( int index )
{
	RumbleChannel_t *pChannel;

	for( int i = 0 ; i < MAX_RUMBLE_CHANNELS ; i++ )
	{
		pChannel = &m_Channels[i];

		if( pChannel->in_use && pChannel->waveformIndex == index )
		{
			// This effect is already playing. Provide this channel for the 
			// effect to be re-started on.
			return pChannel;
		}
	}

	return NULL;
}

//---------------------------------------------------------
// priority - the priority of the effect we want to play.
//---------------------------------------------------------
RumbleChannel_t	*CRumbleEffects::FindAvailableChannel( int priority )
{
	RumbleChannel_t *pChannel;
	int i;

	for( i = 0 ; i < MAX_RUMBLE_CHANNELS ; i++ )
	{
		pChannel = &m_Channels[i];

		if( !pChannel->in_use )
		{
			return pChannel;
		}
	}

	int lowestPriority = priority;
	RumbleChannel_t	*pBestChannel = NULL;
	float oldestChannel = FLT_MAX;

	// All channels already in use. Find a channel to slam.
	for( i = 0 ; i < MAX_RUMBLE_CHANNELS ; i++ )
	{
		pChannel = &m_Channels[i];

		if( (pChannel->rumbleFlags & RUMBLE_FLAG_LOOP) )
			continue;

		if( pChannel->priority < lowestPriority )
		{
			// Always happily slam a lower priority sound.
			pBestChannel = pChannel;
			lowestPriority = pChannel->priority;
		}
		else if( pChannel->priority == lowestPriority )
		{
			// Priority is the same, so replace the oldest.
			if( pBestChannel )
			{
				// If we already have a channel of the same priority to discard, make sure we discard the oldest.
				float age = gpGlobals->curtime - pChannel->starttime;

				if( age > oldestChannel )
				{
					pBestChannel = pChannel;
					oldestChannel = age;
				}
			}
			else
			{
				// Take it.
				pBestChannel = pChannel;
				oldestChannel = gpGlobals->curtime - pChannel->starttime;
			}
		}
	}

	return pBestChannel; // Can still be NULL if we couldn't find a channel to slam.
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRumbleEffects::SetOutputEnabled( bool bEnable )
{
	m_bOutputEnabled = bEnable;

	if( !bEnable )
	{
		// Tell the hardware to shut down motors right now, in case this gets called
		// and some other process blocks us before the next rumble system update.
		m_flScreenShake = 0.0f;

		inputsystem->StopRumble();
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRumbleEffects::StartEffect( unsigned char effectIndex, unsigned char rumbleData, unsigned char rumbleFlags )
{
	if( effectIndex == RUMBLE_STOP_ALL )
	{
		StopAllEffects();
		return;
	}

	if( rumbleFlags & RUMBLE_FLAG_STOP )
	{
		StopEffect( effectIndex );
		return;
	}

	int priority = 1;
	RumbleChannel_t *pChannel = NULL;

	if( (rumbleFlags & RUMBLE_FLAG_RESTART) )
	{
		// Try to find any active instance of this effect and replace it.
		pChannel = FindExistingChannel( effectIndex );
	}

	if( (rumbleFlags & RUMBLE_FLAG_ONLYONE) )
	{
		pChannel = FindExistingChannel( effectIndex );

		if( pChannel )
		{
			// Bail out. An instance of this effect is already playing.
			return;
		}
	}

	if( (rumbleFlags & RUMBLE_FLAG_UPDATE_SCALE) )
	{
		pChannel = FindExistingChannel( effectIndex );
		if( pChannel )
		{
			pChannel->scale = ((float)rumbleData) / 100.0f;
		}

		// It's possible to return without finding a rumble to update.
		// This means you tried to update a rumble you never started.
		return;
	}

	if( !pChannel )
	{
		pChannel = FindAvailableChannel( priority );
	}

	if( pChannel )
	{
		pChannel->waveformIndex = effectIndex;
		pChannel->priority = 1;
		pChannel->starttime = gpGlobals->curtime;
		pChannel->in_use = true;
		pChannel->rumbleFlags = rumbleFlags;

		if( rumbleFlags & RUMBLE_FLAG_INITIAL_SCALE )
		{
			pChannel->scale = ((float)rumbleData) / 100.0f;
		}
		else
		{
			pChannel->scale = 1.0f;
		}
	}

	if( (rumbleFlags & RUMBLE_FLAG_RANDOM_AMPLITUDE) )
	{
		pChannel->scale = random->RandomFloat( 0.1f, 1.0f );
	}
}

//---------------------------------------------------------
// Find all playing effects of this type and stop them.
//---------------------------------------------------------
void CRumbleEffects::StopEffect( int effectIndex )
{
	for( int i = 0 ; i < MAX_RUMBLE_CHANNELS ; i++ )
	{
		if( m_Channels[i].in_use && m_Channels[i].waveformIndex == effectIndex )
		{
			m_Channels[i].in_use = false;
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRumbleEffects::StopAllEffects()
{
	for( int i = 0 ; i < MAX_RUMBLE_CHANNELS ; i++ )
	{
		m_Channels[i].in_use = false;
	}

	m_flScreenShake = 0.0f;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRumbleEffects::ComputeAmplitudes( RumbleChannel_t *pChannel, float curtime, float *pLeft, float *pRight )
{
	// How long has this waveform been playing?
	float elapsed = curtime - pChannel->starttime;
	
	if( elapsed >= (NUM_WAVE_SAMPLES/10) )
	{
		if( (pChannel->rumbleFlags & RUMBLE_FLAG_LOOP) )
		{
			// This effect loops. Just fixup the start time and recompute elapsed.
			pChannel->starttime = curtime;
			elapsed = curtime - pChannel->starttime;
		}
		else
		{
			// This effect is done! Should it loop?
			*pLeft = 0;
			*pRight = 0;
			pChannel->in_use = false;
			return;
		}
	}

	// Figure out which sample we're playing FROM. 
	int seconds = ((int) elapsed);
	int sample = (int)(elapsed*10.0f);

	// Get the fraction bit.
	float fraction = elapsed - seconds;

	float left, right;

	if( sample == m_Waveforms[pChannel->waveformIndex].numSamples )
	{
		// This effect is done. Send zeroes to the mixer for this
		// final frame and then turn the channel off. (Unless it loops!)

		if( (pChannel->rumbleFlags & RUMBLE_FLAG_LOOP) )
		{
			// Loop this effect
			pChannel->starttime = gpGlobals->curtime;

			// Send the first sample.
			left = m_Waveforms[pChannel->waveformIndex].amplitude_left[0];
			right = m_Waveforms[pChannel->waveformIndex].amplitude_right[0];
		}
		else
		{
			left = 0.0f;
			right = 0.0f;
			pChannel->in_use = false;
		}
	}
	else
	{
		// Use values for the last sample that we have passed
		left = m_Waveforms[pChannel->waveformIndex].amplitude_left[sample];
		right = m_Waveforms[pChannel->waveformIndex].amplitude_right[sample];
	}

	left *= pChannel->scale;
	right *= pChannel->scale;

	if( cl_debugrumble.GetBool() )
	{
		Msg("Seconds:%d Fraction:%f Sample:%d  L:%f R:%f\n", seconds, fraction, sample, left, right );
	}

	if( !m_bOutputEnabled )
	{
		// Send zeroes to stop any current rumbling, and to keep it silenced.
		left = 0;
		right = 0;
	}

	*pLeft = left;
	*pRight = right;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRumbleEffects::UpdateScreenShakeRumble( float shake, float balance )
{
	if( m_bOutputEnabled )
	{
		m_flScreenShake = shake;
	}
	else
	{
		// Silence
		m_flScreenShake = 0.0f;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRumbleEffects::UpdateEffects( float curtime )
{
	float fLeftMotor = 0.0f;
	float fRightMotor = 0.0f;

	for( int i = 0 ; i < MAX_RUMBLE_CHANNELS ; i++ )
	{
		// Expire old channels
		RumbleChannel_t *pChannel = & m_Channels[i];

		if( pChannel->in_use )
		{
			float left, right;

			ComputeAmplitudes( pChannel, curtime, &left, &right );
			
			fLeftMotor += left;
			fRightMotor += right;
		}
	}

	// Add in any screenshake
	float shakeLeft = 0.0f;
	float shakeRight = 0.0f;
	if( m_flScreenShake != 0.0f )
	{
		if( m_flScreenShake < 0.0f )
		{
			shakeLeft = fabs( m_flScreenShake );
		}
		else
		{
			shakeRight = m_flScreenShake;
		}
	}

	fLeftMotor += shakeLeft;
	fRightMotor += shakeRight;

	fLeftMotor *= cl_rumblescale.GetFloat();
	fRightMotor *= cl_rumblescale.GetFloat();

	if( engine->IsPaused() )
	{
		// Send nothing when paused.
		fLeftMotor = 0.0f;
		fRightMotor = 0.0f;
	}

	inputsystem->SetRumble( fLeftMotor, fRightMotor );
}

//---------------------------------------------------------
//---------------------------------------------------------
void StopAllRumbleEffects( void )
{
	g_RumbleEffects.StopAllEffects();

	inputsystem->StopRumble();
}

//---------------------------------------------------------
//---------------------------------------------------------
void RumbleEffect( unsigned char effectIndex, unsigned char rumbleData, unsigned char rumbleFlags )
{
	g_RumbleEffects.StartEffect( effectIndex, rumbleData, rumbleFlags );	
}

//---------------------------------------------------------
//---------------------------------------------------------
void UpdateRumbleEffects()
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if( !localPlayer || !localPlayer->IsAlive() )
	{
		StopAllRumbleEffects();
		return;
	}

	g_RumbleEffects.UpdateEffects( gpGlobals->curtime );
}

//---------------------------------------------------------
//---------------------------------------------------------
void UpdateScreenShakeRumble( float shake, float balance )
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
	if( !localPlayer || !localPlayer->IsAlive() )
	{
		return;
	}

	g_RumbleEffects.UpdateScreenShakeRumble( shake, balance );
}

//---------------------------------------------------------
//---------------------------------------------------------
void EnableRumbleOutput( bool bEnable )
{
	g_RumbleEffects.SetOutputEnabled( bEnable );
}
