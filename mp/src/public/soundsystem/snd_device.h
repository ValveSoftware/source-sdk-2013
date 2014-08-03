//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#ifndef SND_DEVICE_H
#define SND_DEVICE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

//-----------------------------------------------------------------------------
// 4.28 fixed point stuff for real-time resampling
//-----------------------------------------------------------------------------
#define FIX_BITS			28
#define FIX_SCALE			(1 << FIX_BITS)
#define FIX_MASK			((1 << FIX_BITS)-1)
#define FIX_FLOAT(a)		((int)((a) * FIX_SCALE))
#define FIX(a)				(((int)(a)) << FIX_BITS)
#define FIX_INTPART(a)		(((int)(a)) >> FIX_BITS)
#define FIX_FRACTION(a,b)	(FIX(a)/(b))
#define FIX_FRACPART(a)		((a) & FIX_MASK)

typedef unsigned int fixedint;


//-----------------------------------------------------------------------------
// sound rate defines
//-----------------------------------------------------------------------------
#define SOUND_DMA_SPEED		44100		// hardware playback rate
#define SOUND_11k			11025		// 11khz sample rate
#define SOUND_22k			22050		// 22khz sample rate
#define SOUND_44k			44100		// 44khz sample rate
#define SOUND_ALL_RATES		1			// mix all sample rates


//-----------------------------------------------------------------------------
// Information about the channel
//-----------------------------------------------------------------------------
struct channel_t
{
	int		leftvol;
	int		rightvol;
	float	pitch;
};


//-----------------------------------------------------------------------------
// The audio device is responsible for mixing
//-----------------------------------------------------------------------------
abstract_class IAudioDevice
{
public:
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
	virtual ~IAudioDevice() {}

	// This initializes the sound hardware.  true on success, false on failure
	virtual bool		Init( void ) = 0;

	// This releases all sound hardware
	virtual void		Shutdown( void ) = 0;

	// device parameters
	virtual const char *DeviceName( void ) const = 0;
	virtual int			DeviceChannels( void ) const = 0;		// 1 = mono, 2 = stereo
	virtual int			DeviceSampleBits( void ) const = 0;	// bits per sample (8 or 16)
	virtual int			DeviceSampleBytes( void ) const = 0;	// above / 8
	virtual int			DeviceSampleRate( void ) const = 0;		// Actual DMA speed
	virtual int			DeviceSampleCount( void ) const = 0;	// Total samples in buffer

	// Called each time a new paint buffer is mixed (may be multiple times per frame)
	virtual void MixBegin( void ) = 0;

	// Main mixing routines
	virtual void Mix8Mono( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress, bool forward = true ) = 0;
	virtual void Mix8Stereo( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress, bool forward = true ) = 0;
	virtual void Mix16Mono( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress, bool forward = true ) = 0;
	virtual void Mix16Stereo( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress, bool forward = true ) = 0;

	// Size of the paint buffer in samples
	virtual int PaintBufferSampleCount( void ) const = 0;

	// Adds a mixer to be mixed
	virtual void			AddSource( CAudioMixer *pSource ) = 0;

	// Stops all sounds
	virtual void			StopSounds( void ) = 0;

	// Updates sound mixing
	virtual void			Update( float time ) = 0;

	// Resets the device
	virtual void			Flush( void ) = 0;

	virtual int				FindSourceIndex( CAudioMixer *pSource ) = 0;
	virtual CAudioMixer		*GetMixerForSource( CAudioSource *source ) = 0;
	virtual void			FreeChannel( int channelIndex ) = 0;
};


#endif // SND_DEVICE_H
