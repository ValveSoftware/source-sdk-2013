//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-server neutral sound interface
//
// $NoKeywords: $
//=============================================================================//

#ifndef IENGINESOUND_H
#define IENGINESOUND_H

#ifdef _WIN32
#pragma once
#endif


#include "basetypes.h"
#include "interface.h"
#include "soundflags.h"
#include "irecipientfilter.h"
#include "utlvector.h"
#include "engine/SndInfo.h"

//-----------------------------------------------------------------------------
// forward declaration
//-----------------------------------------------------------------------------
class Vector;

// Handy defines for EmitSound
#define SOUND_FROM_UI_PANEL			-2		// Sound being played inside a UI panel on the client
#define SOUND_FROM_LOCAL_PLAYER		-1
#define SOUND_FROM_WORLD			0



// These are used to feed a soundlevel to the sound system and have it use
// goldsrc-type attenuation. We should use this as little as possible and 
// phase it out as soon as possible.

// Take a regular sndlevel and convert it to compatibility mode.
#define SNDLEVEL_TO_COMPATIBILITY_MODE( x )		((soundlevel_t)(int)( (x) + 256 ))

// Take a compatibility-mode sndlevel and get the REAL sndlevel out of it.
#define SNDLEVEL_FROM_COMPATIBILITY_MODE( x )	((soundlevel_t)(int)( (x) - 256 ))

// Tells if the given sndlevel is marked as compatibility mode.
#define SNDLEVEL_IS_COMPATIBILITY_MODE( x )		( (x) >= soundlevel_t(256) )


// IAudioOutputStream from source2
// a separate stream that connects to the main output
class IAudioOutputStream
{
public:
	virtual ~IAudioOutputStream() {}
	// queue a set of samples for output.  
	// NOTE: nSampleCount is the number of samples being written to each channel, not the total # of all channels' samples
	virtual void WriteAudioData( const int16 *pData, uint nSampleCount, uint nChannels ) = 0;

	// change output volume (will be ramped linearly per sample over one mix quantum)
	virtual void SetVolume( float flVolume ) = 0;

	// how many samples are queued to play?
	virtual uint32 QueuedSampleCount() = 0;

	// how many samples can we write in a WriteAudioData call without truncation in the internal buffer?
	virtual uint32 MaxWriteSampleCount() = 0;

	// how many samples of latency
	virtual uint32 LatencySamplesCount() = 0;

	virtual void Pause() = 0;
	virtual void Resume() = 0;
};



//-----------------------------------------------------------------------------
// Client-server neutral effects interface
//-----------------------------------------------------------------------------
#define IENGINESOUND_CLIENT_INTERFACE_VERSION	"IEngineSoundClient003"
#define IENGINESOUND_SERVER_INTERFACE_VERSION	"IEngineSoundServer003"

struct AudioState_t;

abstract_class IEngineSound
{
public:
	// Precache a particular sample
	virtual bool PrecacheSound( const char *pSample, bool bPreload = false, bool bIsUISound = false ) = 0;
	virtual bool IsSoundPrecached( const char *pSample ) = 0;
	virtual void PrefetchSound( const char *pSample ) = 0;

	// Just loads the file header and checks for duration (not hooked up for .mp3's yet)
	// Is accessible to server and client though
	virtual float GetSoundDuration( const char *pSample ) = 0;  

	// Pitch of 100 is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
	// down to 1 is a lower pitch.   150 to 70 is the realistic range.
	// EmitSound with pitch != 100 should be used sparingly, as it's not quite as
	// fast (the pitchshift mixer is not native coded).

	// NOTE: setting iEntIndex to -1 will cause the sound to be emitted from the local
	// player (client-side only)
	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, int iSpecialDSP = 0, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;

	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM, int iSpecialDSP = 0, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;

	virtual void EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM,int iSpecialDSP = 0, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 ) = 0;

	virtual void StopSound( int iEntIndex, int iChannel, const char *pSample ) = 0;

	// stop all active sounds (client only)
	virtual void StopAllSounds(bool bClearBuffers) = 0;

	// Set the room type for a player (client only)
	virtual void SetRoomType( IRecipientFilter& filter, int roomType ) = 0;

	// Set the dsp preset for a player (client only)
	virtual void SetPlayerDSP( IRecipientFilter& filter, int dspType, bool fastReset ) = 0;
	
	// emit an "ambient" sound that isn't spatialized
	// only available on the client, assert on server
	virtual void EmitAmbientSound( const char *pSample, float flVolume, int iPitch = PITCH_NORM, int flags = 0, float soundtime = 0.0f ) = 0;


//	virtual EntChannel_t	CreateEntChannel() = 0;

	virtual float GetDistGainFromSoundLevel( soundlevel_t soundlevel, float dist ) = 0;

	// Client .dll only functions
	virtual int		GetGuidForLastSoundEmitted() = 0;
	virtual bool	IsSoundStillPlaying( int guid ) = 0;
	virtual void	StopSoundByGuid( int guid ) = 0;
	// Set's master volume (0.0->1.0)
	virtual void	SetVolumeByGuid( int guid, float fvol ) = 0;

	// Retrieves list of all active sounds
	virtual void	GetActiveSounds( CUtlVector< SndInfo_t >& sndlist ) = 0;

	virtual void	PrecacheSentenceGroup( const char *pGroupName ) = 0;
	virtual void	NotifyBeginMoviePlayback() = 0;
	virtual void	NotifyEndMoviePlayback() = 0;

	// create/destroy an audio stream
	virtual IAudioOutputStream *CreateOutputStream( uint nSampleRate, uint nChannels, uint nBits ) = 0;
	virtual void DestroyOutputStream( IAudioOutputStream *pStream ) = 0;

	// Force an update, for instances where we are otherwise deadlocked from the main loop.
	virtual void	ManualUpdate( const AudioState_t *pListenerState ) = 0;
	// Force an extra update.
	virtual void	ExtraUpdate() = 0;
};


#endif // IENGINESOUND_H
