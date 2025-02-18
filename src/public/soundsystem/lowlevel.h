//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: This abstracts the various hardware dependent implementations of sound
//			At the time of this writing there are Windows WAVEOUT, Direct Sound,
//			and Null implementations.
//
//=====================================================================================//

#ifndef SOUNDSYSTEM_LOWLEVEL_H
#define SOUNDSYSTEM_LOWLEVEL_H
#pragma once

#include "utlvector.h"

#define SOUND_DEVICE_MAX_CHANNELS 8			// we support 2, 4, 6, & 8 channels currently.
											// Long term we may build 4 & 8 as matrix mix-downs of 6 channels
#define MIX_BUFFER_SIZE 512

class IAudioDevice2;
struct audio_device_init_params_t;

class ALIGN16 CAudioMixBuffer
{
public:
	float m_flData[MIX_BUFFER_SIZE];
} ALIGN16_POST;

const float MIX_DEFAULT_SAMPLING_RATE = 44100.0f;
const float SECONDS_PER_SAMPLE	= (1.0f / MIX_DEFAULT_SAMPLING_RATE);
const float MIX_SECONDS_PER_BUFFER = float( MIX_BUFFER_SIZE ) / float( MIX_DEFAULT_SAMPLING_RATE );
const float MIX_BUFFERS_PER_SECOND = float( MIX_DEFAULT_SAMPLING_RATE ) / float( MIX_BUFFER_SIZE );


enum eSubSystems_t
{
	AUDIO_SUBSYSTEM_XAUDIO = 0,
	AUDIO_SUBSYSTEM_DSOUND = 1,
	AUDIO_SUBSYSTEM_SDL = 2,
	AUDIO_SUBSYSTEM_NULL = 3,		// fake, emulated device for failure cases
};

#define AUDIO_DEVICE_NAME_MAX 256
struct audio_device_description_t
{
	wchar_t m_deviceName[AUDIO_DEVICE_NAME_MAX];
	char m_friendlyName[AUDIO_DEVICE_NAME_MAX];
	uint8 m_nSubsystemId;
	uint8 m_nChannelCount;
	bool m_bIsDefault : 1;
	bool m_bIsAvailable : 1;

	audio_device_description_t() {}
	explicit audio_device_description_t( eSubSystems_t nSubsystem ) : m_nSubsystemId( (uint8)nSubsystem ) { Assert( nSubsystem >= 0 && nSubsystem <= UINT8_MAX ); }

	inline void InitAsNullDevice()
	{
		V_memset( m_deviceName, 0, sizeof(m_deviceName) );
		V_memset( m_friendlyName, 0, sizeof(m_friendlyName) );
		m_nChannelCount = 2;
		m_nSubsystemId = AUDIO_SUBSYSTEM_NULL;
		m_bIsDefault = true;
		m_bIsAvailable = true;
	}
};

class CAudioDeviceList
{
public:
	eSubSystems_t							m_nSubsystem;
	CUtlVector<audio_device_description_t>	m_list;
	int										m_nDefaultDevice;

	CAudioDeviceList() {}
	void BuildDeviceList( eSubSystems_t nPreferredSubsystem );
	bool UpdateDeviceList();	// returns true if new devices or defaults show up
	audio_device_description_t *FindDeviceById( const char *pId );  // returns NULL if not found
	audio_device_description_t *GetDefaultDevice(); // returns NULL if not set
	bool IsValid() { return m_list.Count() > 0; }
	IAudioDevice2 *CreateDevice( audio_device_init_params_t &params );
	const wchar_t *GetDeviceToCreate( audio_device_init_params_t &params );

private:
	uint								m_nDeviceStamp;
	void UpdateDefaultDevice();
	enum finddevice_t
	{
		FIND_ANY_DEVICE = 0,
		FIND_AVAILABLE_DEVICE_ONLY = 1,
	};
	int FindDeviceById( const wchar_t *pId, finddevice_t nFind );
};

#define DEFAULT_MIX_BUFFER_COUNT 4
#define DEFAULT_MIX_BUFFER_SAMPLE_COUNT MIX_BUFFER_SIZE
struct audio_device_init_params_t
{
	const audio_device_description_t	*m_pDesc;
	void								*m_pWindowHandle;
	int									m_nOutputBufferCount;
	int									m_nSampleCountPerOutputBuffer;
	int									m_nOverrideSpeakerConfig;		// only used if m_bOverrideSpeakerConfig is true
	bool								m_bOverrideDevice;				// If this is set use m_overrideDevice
	bool								m_bOverrideSpeakerConfig;
	bool								m_bPlayEvenWhenNotInFocus;
	// When we set the override device it is important to copy the memory since
	// the original device description may get realloced and thus become a stale
	// pointer.
	wchar_t								m_overrideDeviceName[AUDIO_DEVICE_NAME_MAX];
	int									m_nOverrideSubsystem;

	inline void OverrideDevice( audio_device_description_t *pDevice )
	{
		m_bOverrideDevice = true;
		V_wcscpy_safe( m_overrideDeviceName, pDevice->m_deviceName );
		m_nOverrideSubsystem = pDevice->m_nSubsystemId;
	}

	inline void OverrideSpeakerConfig( int nSpeakerConfig )
	{
		Assert(nSpeakerConfig >= 0 && nSpeakerConfig < 8);
		m_nOverrideSpeakerConfig = nSpeakerConfig;
		m_bOverrideSpeakerConfig = true;
	}

	audio_device_init_params_t() : m_bOverrideSpeakerConfig(false), m_bOverrideDevice(false) {}
	inline void Defaults()
	{
		m_nOutputBufferCount = DEFAULT_MIX_BUFFER_COUNT;
		m_nSampleCountPerOutputBuffer = MIX_BUFFER_SIZE;
		m_bOverrideDevice = false;
		m_bOverrideSpeakerConfig = false;
		m_nOverrideSpeakerConfig = 0;
		m_bPlayEvenWhenNotInFocus = true;
		m_pWindowHandle = NULL;
	}
};

extern int Audio_EnumerateDevices( eSubSystems_t nSubsystem, audio_device_description_t *pDeviceListOut, int nListCount );
extern int Audio_EnumerateXAudio2Devices( audio_device_description_t *pDeviceListOut, int nListCount );
extern int Audio_EnumerateDSoundDevices( audio_device_description_t *pDeviceListOut, int nListCount );
#ifdef POSIX
extern int Audio_EnumerateSDLDevices( audio_device_description_t *pDeviceListOut, int nListCount );
#endif

// return true if there was an error event and the device needs to be restarted
extern bool Audio_PollErrorEvents();

class IAudioDevice2
{
public:
	virtual ~IAudioDevice2() {}
	virtual void		OutputBuffer( int nChannels, CAudioMixBuffer *pChannelArray ) = 0;
	virtual void		Shutdown( void ) = 0;
	virtual int			QueuedBufferCount() = 0;
	virtual int			EmptyBufferCount() = 0;
	virtual void		CancelOutput( void ) = 0;
	virtual void		WaitForComplete() = 0;
	virtual void		UpdateFocus( bool bWindowHasFocus ) = 0;
	virtual void		ClearBuffer() = 0;
	virtual const wchar_t *GetDeviceID() const = 0;
	virtual void		OutputDebugInfo() const = 0;
	virtual bool		SetShouldPlayWhenNotInFocus( bool bPlayEvenWhenNotInFocus ) = 0;

	inline const char *Name() const { return m_pName; }
	inline int ChannelCount() const { return m_nChannels; }
	inline int MixChannelCount() const { return m_nChannels > 6 ? 6 : m_nChannels; } // 7.1 mixes as 5.1
	inline int BitsPerSample() const { return m_nSampleBits; }
	inline int SampleRate() const { return m_nSampleRate; }

	inline bool IsSurround() const { return m_nChannels > 2 ? true : false; }
	inline bool IsSurroundCenter() const { return m_nChannels > 4 ? true : false; }
	inline bool IsActive() const { return m_bIsActive; }
	inline bool IsHeadphone() const { return m_bIsHeadphone; } // mixing makes some choices differently for stereo vs headphones, expose that here.
	inline bool CanDetectBufferStarvation() { return m_bSupportsBufferStarvationDetection; }
	inline bool IsCaptureDevice() { return m_bIsCaptureDevice; }


	inline int	DeviceSampleBytes( void ) const { return BitsPerSample() / 8; }
	
	// UNDONE: Need to implement these
	void Pause() {}
	void UnPause() {}
	void TransferSamples( uint32 nEndTimeIgnored );

protected:
	// NOTE: Derived classes MUST initialize these before returning a device from a factory
	const char *m_pName;
	int			m_nChannels;
	int			m_nSampleBits;
	int			m_nSampleRate;
	bool		m_bIsActive;
	bool		m_bIsHeadphone;
	bool		m_bSupportsBufferStarvationDetection;
	bool		m_bIsCaptureDevice;
};


// device handling
extern IAudioDevice2 *Audio_CreateXAudio2Device( const audio_device_init_params_t &params );
extern IAudioDevice2 *Audio_CreateDSoundDevice( const audio_device_init_params_t &params );

#ifdef POSIX
extern IAudioDevice2 *Audio_CreateSDLDevice( const audio_device_init_params_t &params );
#endif

extern IAudioDevice2 *Audio_CreateNullDevice();
#if IS_WINDOWS_PC
extern bool GetWindowsDefaultAudioDevice( wchar_t *pName, size_t nNameBufSize );
#endif

// speaker config
extern int SpeakerConfigValueToChannelCount( int nSpeakerConfig );
extern int ChannelCountToSpeakerConfigValue( int nChannelCount, bool bIsHeadphone );

// buffer library
extern void ScaleBuffer( float flOutput[MIX_BUFFER_SIZE], const float flInput[MIX_BUFFER_SIZE], float flScale );
extern void ScaleBufferRamp( float flOutput[MIX_BUFFER_SIZE], const float flInput[MIX_BUFFER_SIZE], float flScaleStart, float flScaleEnd );
extern void MixBuffer( float flOutput[MIX_BUFFER_SIZE], const float flInput[MIX_BUFFER_SIZE], float flScale );
extern void MixBufferRamp( float flOutput[MIX_BUFFER_SIZE], const float flInput[MIX_BUFFER_SIZE], float flScaleStart, float flScaleEnd );

inline void ScaleBufferAuto( float flOutput[MIX_BUFFER_SIZE], const float flInput[MIX_BUFFER_SIZE], float flScaleStart, float flScaleEnd )
{
	if ( flScaleStart == flScaleEnd )
	{
		ScaleBuffer( flOutput, flInput, flScaleEnd );
	}
	else
	{
		ScaleBufferRamp( flOutput, flInput, flScaleStart, flScaleEnd );
	}
}

inline void MixBufferAuto( float flOutput[MIX_BUFFER_SIZE], const float flInput[MIX_BUFFER_SIZE], float flScaleStart, float flScaleEnd )
{
	if ( flScaleStart == flScaleEnd )
	{
		MixBuffer( flOutput, flInput, flScaleEnd );
	}
	else
	{
		MixBufferRamp( flOutput, flInput, flScaleStart, flScaleEnd );
	}
}

extern void SilenceBuffer( float flBuffer[MIX_BUFFER_SIZE] );
extern void SilenceBuffers( CAudioMixBuffer *pBuffers, int nBufferCount );
extern void SumBuffer2x1( float flOutput[MIX_BUFFER_SIZE], float flInput0[MIX_BUFFER_SIZE], float flScale0, float flInput1[MIX_BUFFER_SIZE], float flScale1 );
extern void SwapBuffersInPlace( float flInput0[MIX_BUFFER_SIZE], float flInput1[MIX_BUFFER_SIZE] );
extern float BufferLevel( float flInput[MIX_BUFFER_SIZE] );
extern float AvergeBufferAmplitude( float flInput[MIX_BUFFER_SIZE] );

extern void ConvertFloat32Int16_Clamp_Interleave2( short *pOut, float *pflLeft, float *pflRight, int nSampleCount );
extern void ConvertFloat32Int16_Clamp_InterleaveStride( short *pOut, int nOutputChannelCount, int nChannelStrideFloats, float *pflChannel0, int nInputChannelCount, int nSampleCount );


// some conversion routines used by mixing code
// convert mono short to mono float, multiply by scale
extern void ConvertShortToFloat( float *pFloatOut, const short *pMonoInput, uint nCount, float flScale );
// convert a stereo source from signed 16-bit PCM to mono signed unit scale float, add L/R stereo channels to produce mono, multiply by scale
extern void SumStereoShortToFloat( float *pFloatOut, const short *pStereoInput, uint nCount, float flScale );
extern void ConvertFloatToShort( short *pOutput, const float *pFloatInput, uint nCount, float flScale );
extern void AddStereoFloatToStereoShort( short *pOutput, float *pFloatInput, float flFloatScale, short *pShortInput, float flShortScale, uint nCount );
extern void AddStereoFloatToMonoShort( short *pOutput, float *pFloatInput, float flFloatScale, short *pShortInput, float flShortScale, uint nCount );

#if IS_WINDOWS_PC
void InitCOM();
void ShutdownCOM();
#else
inline void InitCOM() {}
inline void ShutdownCOM() {}
#endif

#endif // SOUNDSYSTEM_LOWLEVEL_H
