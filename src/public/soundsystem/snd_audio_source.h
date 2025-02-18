//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#ifndef SND_AUDIO_SOURCE_H
#define SND_AUDIO_SOURCE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CAudioSource;
class IAudioDevice;
struct channel_t;


//-----------------------------------------------------------------------------
// Purpose: This is an instance of an audio source.
//			Mixers are attached to channels and reference an audio source.
//			Mixers are specific to the sample format and source format.
//			Mixers are never re-used, so they can track instance data like
//			sample position, fractional sample, stream cache, faders, etc.
//-----------------------------------------------------------------------------
abstract_class CAudioMixer
{
public:
	virtual ~CAudioMixer( void ) {}

	// UNDONE: time compress
	virtual bool MixDataToDevice( IAudioDevice *pDevice, channel_t *pChannel, int startSample, int sampleCount, int outputRate, bool forward = true ) = 0;
	virtual void IncrementSamples( channel_t *pChannel, int startSample, int sampleCount,int outputRate, bool forward = true ) = 0;
	virtual bool SkipSamples( IAudioDevice *pDevice, channel_t *pChannel, int startSample, int sampleCount, int outputRate, bool forward = true ) = 0;

	virtual CAudioSource *GetSource( void ) = 0;

	virtual int GetSamplePosition( void ) = 0;
	virtual int GetScubPosition( void ) = 0;

	virtual bool SetSamplePosition( int position, bool scrubbing = false ) = 0;
	virtual void SetLoopPosition( int position ) = 0;
	virtual int	GetStartPosition( void ) = 0;

	virtual bool	GetActive( void ) = 0;
	virtual void	SetActive( bool active ) = 0;

	virtual void	SetModelIndex( int index ) = 0;
	virtual int		GetModelIndex( void ) const = 0;

	virtual void	SetDirection( bool forward ) = 0;
	virtual bool	GetDirection( void ) const = 0;

	virtual void	SetAutoDelete( bool autodelete ) = 0;
	virtual bool	GetAutoDelete( void ) const = 0;

	virtual void	SetVolume( float volume ) = 0;
	virtual channel_t *GetChannel() = 0;
};

//-----------------------------------------------------------------------------
// Purpose: A source is an abstraction for a stream, cached file, or procedural
//			source of audio.
//-----------------------------------------------------------------------------
class CSentence;

abstract_class CAudioSource
{
public:
	CAudioSource( void );
	virtual ~CAudioSource( void );

	// Create an instance (mixer) of this audio source
	virtual CAudioMixer			*CreateMixer( void ) = 0;
	virtual int					GetOutputData( void **pData, int samplePosition, int sampleCount, bool forward = true ) = 0;
	virtual int					SampleRate( void ) = 0;
	virtual int					SampleSize( void ) = 0;
	virtual int					SampleCount( void ) = 0;
	virtual float				TrueSampleSize( void ) = 0;
	virtual bool				IsLooped( void ) = 0;
	virtual bool				IsStreaming( void ) = 0;
	virtual float				GetRunningLength( void ) = 0;
	virtual int					GetNumChannels() = 0;

	virtual CSentence			*GetSentence( void ) { return NULL; };

};


extern CAudioSource *AudioSource_Create( const char *pName );

#endif // SND_AUDIO_SOURCE_H
