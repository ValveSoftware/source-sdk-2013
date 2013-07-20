//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_SOUNDENVELOPE_H
#define SRCPY_SOUNDENVELOPE_H

#ifdef _WIN32
#pragma once
#endif

#include "soundenvelope.h"

class CSoundPatch;

struct CSoundPatchHandle
{
	CSoundPatchHandle( CSoundPatch *pSoundPatch ) { m_pSoundPatch = pSoundPatch; }
	CSoundPatch *m_pSoundPatch;
};

#define CHECK_SOUNDPATCH(patch_handle)			\
	if( patch_handle == NULL )					\
		return;									\
	if( patch_handle->m_pSoundPatch == NULL )	\
		return;									\
	
#define CHECK_SOUNDPATCH_RV(patch_handle, rv)	\
	if( patch_handle == NULL )					\
		return rv;								\
	if( patch_handle->m_pSoundPatch == NULL )	\
		return rv;								\

class CSoundEnvelopeControllerHandle
{
public:
	CSoundEnvelopeControllerHandle( CSoundEnvelopeController *pController )
	{
		m_pController = pController;
	}

public:
	inline void			SystemReset( void ) { m_pController->SystemReset(); }
	inline void			SystemUpdate( void ) { m_pController->SystemUpdate(); }
	inline void			Play( CSoundPatchHandle *pSound, float volume, float pitch, float flStartTime = 0 ) { CHECK_SOUNDPATCH(pSound); m_pController->Play(pSound->m_pSoundPatch, volume, pitch, flStartTime); }
	inline void			CommandAdd( CSoundPatchHandle *pSound, float executeDeltaTime, soundcommands_t command, float commandTime, float value ) { CHECK_SOUNDPATCH(pSound); m_pController->CommandAdd(pSound->m_pSoundPatch, executeDeltaTime, command, commandTime, value); }
	inline void			CommandClear( CSoundPatchHandle *pSound ) { CHECK_SOUNDPATCH(pSound); m_pController->CommandClear(pSound->m_pSoundPatch); }
	inline void			Shutdown( CSoundPatchHandle *pSound ) { CHECK_SOUNDPATCH(pSound); m_pController->Shutdown(pSound->m_pSoundPatch); }

	inline CSoundPatchHandle	SoundCreate( IRecipientFilter& filter, int nEntIndex, const char *pSoundName ) { return CSoundPatchHandle(m_pController->SoundCreate(filter, nEntIndex, pSoundName)); }
	inline CSoundPatchHandle	SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
		float attenuation ) { return CSoundPatchHandle(m_pController->SoundCreate(filter, nEntIndex, channel, pSoundName, attenuation)); }
	inline CSoundPatchHandle	SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
		soundlevel_t soundlevel ) { return CSoundPatchHandle(m_pController->SoundCreate(filter, nEntIndex, channel, pSoundName, soundlevel)); }
	inline CSoundPatchHandle	SoundCreate( IRecipientFilter& filter, int nEntIndex, const EmitSound_t &es ) { return CSoundPatchHandle(m_pController->SoundCreate(filter, nEntIndex, es)); }
	inline void			SoundDestroy( CSoundPatchHandle	*pSound ) { CHECK_SOUNDPATCH(pSound); m_pController->SoundDestroy(pSound->m_pSoundPatch); pSound->m_pSoundPatch = NULL; }
	inline void			SoundChangePitch( CSoundPatchHandle *pSound, float pitchTarget, float deltaTime ) { CHECK_SOUNDPATCH(pSound); m_pController->SoundChangePitch(pSound->m_pSoundPatch, pitchTarget, deltaTime); }
	inline void			SoundChangeVolume( CSoundPatchHandle *pSound, float volumeTarget, float deltaTime ) { CHECK_SOUNDPATCH(pSound); m_pController->SoundChangeVolume(pSound->m_pSoundPatch, volumeTarget, deltaTime); }
	inline void			SoundFadeOut( CSoundPatchHandle *pSound, float deltaTime, bool destroyOnFadeout = false ) { CHECK_SOUNDPATCH(pSound); m_pController->SoundFadeOut(pSound->m_pSoundPatch, deltaTime, destroyOnFadeout); }
	inline float		SoundGetPitch( CSoundPatchHandle *pSound ) { CHECK_SOUNDPATCH_RV(pSound, 0); return m_pController->SoundGetPitch(pSound->m_pSoundPatch); }
	inline float		SoundGetVolume( CSoundPatchHandle *pSound ) { CHECK_SOUNDPATCH_RV(pSound, 0); return m_pController->SoundGetVolume(pSound->m_pSoundPatch); }

	//inline float		SoundPlayEnvelope( CSoundPatchHandle *pSound, soundcommands_t soundCommand, envelopePoint_t *points, int numPoints ) = 0;
	//inline float		SoundPlayEnvelope( CSoundPatchHandle *pSound, soundcommands_t soundCommand, envelopeDescription_t *envelope ) = 0;

	inline void			CheckLoopingSoundsForPlayer( CBasePlayer *pPlayer ) { m_pController->CheckLoopingSoundsForPlayer(pPlayer); }

	inline string_t		SoundGetName( CSoundPatchHandle *pSound ) { CHECK_SOUNDPATCH_RV(pSound, NULL_STRING); return m_pController->SoundGetName(pSound->m_pSoundPatch); }
	static	CSoundEnvelopeControllerHandle GetController( void ) { return CSoundEnvelopeControllerHandle(&(CSoundEnvelopeController::GetController()));}

	inline void			SoundSetCloseCaptionDuration( CSoundPatchHandle *pSound, float flDuration ) { CHECK_SOUNDPATCH(pSound); m_pController->SoundSetCloseCaptionDuration(pSound->m_pSoundPatch, flDuration); }

private:
	CSoundEnvelopeController *m_pController;
};

// Enginesound
#define VALIDATE_GUID(guid)										\
	if( !enginesound->IsSoundStillPlaying(guid) )				\
	{															\
		PyErr_SetString(PyExc_Exception, "Invalid guid" );		\
		throw boost::python::error_already_set();				\
		return;													\
	}																

class PyEngineSound
{
public:
	// Precache a particular sample
	inline bool PrecacheSound( const char *pSample, bool bPreload = false, bool bIsUISound = false ) { return enginesound->PrecacheSound(pSample, bPreload, bIsUISound); }
	inline bool IsSoundPrecached( const char *pSample ) { return enginesound->IsSoundPrecached(pSample); }
	inline void PrefetchSound( const char *pSample ) { enginesound->PrefetchSound(pSample); }

	// Just loads the file header and checks for duration (not hooked up for .mp3's yet)
	// Is accessible to server and client though
	inline float GetSoundDuration( const char *pSample ) { return enginesound->GetSoundDuration(pSample); }

	// Pitch of 100 is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
	// down to 1 is a lower pitch.   150 to 70 is the realistic range.
	// EmitSound with pitch != 100 should be used sparingly, as it's not quite as
	// fast (the pitchshift mixer is not native coded).

	// NOTE: setting iEntIndex to -1 will cause the sound to be emitted from the local
	// player (client-side only)

	// .... TODO (EMITSOUND)

	inline void StopSound( int iEntIndex, int iChannel, const char *pSample ) { enginesound->StopSound(iEntIndex, iChannel, pSample); }

#ifdef CLIENT_DLL
	// stop all active sounds (client only)
	inline void StopAllSounds(bool bClearBuffers) { enginesound->StopAllSounds(bClearBuffers); }

	// Set the room type for a player (client only)
	inline void SetRoomType( IRecipientFilter& filter, int roomType ) { enginesound->SetRoomType(filter, roomType); } 

	// Set the dsp preset for a player (client only)
	inline void SetPlayerDSP( IRecipientFilter& filter, int dspType, bool fastReset ) { enginesound->SetPlayerDSP(filter, dspType, fastReset); }

	// emit an "ambient" sound that isn't spatialized
	// only available on the client, assert on server
	inline void EmitAmbientSound( const char *pSample, float flVolume, int iPitch = PITCH_NORM, int flags = 0, float soundtime = 0.0f )
	{
		enginesound->EmitAmbientSound(pSample, flVolume, iPitch, flags, soundtime);
	}
#endif // CLIENT_DLL

	//	virtual EntChannel_t	CreateEntChannel() = 0;

	inline float GetDistGainFromSoundLevel( soundlevel_t soundlevel, float dist ) { return enginesound->GetDistGainFromSoundLevel(soundlevel, dist); }

#ifdef CLIENT_DLL
	// Client .dll only functions
	inline int		GetGuidForLastSoundEmitted() { return enginesound->GetGuidForLastSoundEmitted(); }
	inline bool	IsSoundStillPlaying( int guid ) { return enginesound->IsSoundStillPlaying(guid); }
	inline void	StopSoundByGuid( int guid ) { enginesound->StopSoundByGuid(guid); }
	// Set's master volume (0.0->1.0)
	inline void	SetVolumeByGuid( int guid, float fvol ) 
	{ 
		VALIDATE_GUID(guid);
		if( fvol < 0.0f || fvol > 1.0f )
		{
			PyErr_SetString(PyExc_Exception, "Volume must be in range 0.0->1.0" );
			throw boost::python::error_already_set(); 
			return;
		}
		enginesound->SetVolumeByGuid(guid, fvol); 
	}
#endif // CLIENT_DLL

	// Retrieves list of all active sounds
	//void	GetActiveSounds( CUtlVector< SndInfo_t >& sndlist ) { enginesound->GetActiveSounds(); }

	inline void	PrecacheSentenceGroup( const char *pGroupName ) { enginesound->PrecacheSentenceGroup(pGroupName); } 
	inline void	NotifyBeginMoviePlayback() { enginesound->NotifyBeginMoviePlayback(); }
	inline void	NotifyEndMoviePlayback() { enginesound->NotifyEndMoviePlayback(); }
};

extern PyEngineSound *pysoundengine;

#endif // SRCPY_SOUNDENVELOPE_H