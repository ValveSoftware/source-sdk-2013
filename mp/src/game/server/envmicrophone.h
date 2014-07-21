//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENVMICROPHONE_H
#define ENVMICROPHONE_H
#ifdef _WIN32
#pragma once
#endif

class CBaseFilter;


const int SF_MICROPHONE_SOUND_COMBAT			= 0x01;
const int SF_MICROPHONE_SOUND_WORLD				= 0x02;
const int SF_MICROPHONE_SOUND_PLAYER			= 0x04;
const int SF_MICROPHONE_SOUND_BULLET_IMPACT		= 0x08;
const int SF_MICROPHONE_SWALLOW_ROUTED_SOUNDS	= 0x10;
const int SF_MICROPHONE_SOUND_EXPLOSION			= 0x20;
const int SF_MICROPHONE_IGNORE_NONATTENUATED	= 0x40;


// Return codes from SoundPlayed
enum MicrophoneResult_t
{
    MicrophoneResult_Ok = 0,
    MicrophoneResult_Swallow,       // The microphone swallowed the sound. Don't play it.
    MicrophoneResult_Remove,        // The microphone should be removed from the list of microphones.
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvMicrophone : public CPointEntity
{
	DECLARE_CLASS( CEnvMicrophone, CPointEntity );

public:
	~CEnvMicrophone();

	void Spawn(void);
	void Activate(void);
	void OnRestore( void );
	void ActivateSpeaker( void );
	void Think(void);
	bool CanHearSound(CSound *pSound, float &flVolume);
	bool CanHearSound( int entindex, soundlevel_t soundlevel, float &flVolume, const Vector *pOrigin );

	void SetSensitivity( float flSensitivity );
	void SetSpeakerName( string_t iszSpeakerName );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetSpeakerName( inputdata_t &inputdata );

	DECLARE_DATADESC();

	// Hook for the sound system to tell us when a sound's been played. Returns true if it's to swallow the passed in sound.
	static bool OnSoundPlayed( int entindex, const char *soundname, soundlevel_t soundlevel, 
		float flVolume, int iFlags, int iPitch, const Vector *pOrigin, float soundtime, CUtlVector< Vector >& soundorigins );

private:

	// Per-microphone notification that a sound has played.
	MicrophoneResult_t SoundPlayed( int entindex, const char *soundname, soundlevel_t soundlevel, 
		float flVolume, int iFlags, int iPitch, const Vector *pOrigin, float soundtime, CUtlVector< Vector >& soundorigins );

	bool		m_bDisabled;			// If true, the microphone will not measure sound.
	EHANDLE		m_hMeasureTarget;		// Point at which to measure sound level.
	int			m_nSoundMask;			// Which sound types we are interested in.
	float		m_flSensitivity;		// 0 = deaf, 1 = default, 10 = maximum sensitivity
	float		m_flSmoothFactor;		// 0 = no smoothing of samples, 0.9 = maximum smoothing
	float		m_flMaxRange;			// Maximum sound hearing range, irrelevant of attenuation
	string_t	m_iszSpeakerName;		// Name of a speaker to output any heard sounds through
	EHANDLE		m_hSpeaker;				// Speaker to output any heard sounds through
	bool		m_bAvoidFeedback;
	int			m_iSpeakerDSPPreset;	// Speaker DSP preset to use when this microphone is enabled
	string_t	m_iszListenFilter;
	CHandle<CBaseFilter>	m_hListenFilter;

	COutputFloat m_SoundLevel;			// Fired when the sampled volume level changes.
	COutputEvent m_OnRoutedSound;		// Fired when a sound has been played through our speaker
	COutputEvent m_OnHeardSound;		// Heard sound.

	char		m_szLastSound[256];
};

#endif // ENVMICROPHONE_H
