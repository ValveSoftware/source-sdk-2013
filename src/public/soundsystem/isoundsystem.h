//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef ISOUNDSYSTEM_H
#define ISOUNDSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/iappsystem.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IAudioDevice;
class CAudioSource;
class CAudioMixer;


//-----------------------------------------------------------------------------
// Sound handle
//-----------------------------------------------------------------------------
typedef unsigned short AudioSourceHandle_t;
enum
{
	AUDIOSOURCEHANDLE_INVALID = (AudioSourceHandle_t)~0
};


//-----------------------------------------------------------------------------
// Flags for FindAudioSource
//-----------------------------------------------------------------------------
enum FindAudioSourceFlags_t
{
	FINDAUDIOSOURCE_NODELAY = 0x1,
	FINDAUDIOSOURCE_PREFETCH = 0x2,
	FINDAUDIOSOURCE_PLAYONCE = 0x4,
};


//-----------------------------------------------------------------------------
// Purpose: DLL interface for low-level sound utilities
//-----------------------------------------------------------------------------
#define SOUNDSYSTEM_INTERFACE_VERSION "SoundSystem001"

abstract_class ISoundSystem : public IAppSystem
{
public:
	virtual void		Update( float time ) = 0;
	virtual void		Flush( void ) = 0;

	virtual CAudioSource *FindOrAddSound( const char *filename ) = 0;
	virtual CAudioSource *LoadSound( const char *wavfile ) = 0;

	virtual void		PlaySound( CAudioSource *source, float volume, CAudioMixer **ppMixer ) = 0;

	virtual bool		IsSoundPlaying( CAudioMixer *pMixer ) = 0;
	virtual CAudioMixer *FindMixer( CAudioSource *source ) = 0;

	virtual void		StopAll( void ) = 0;
	virtual void		StopSound( CAudioMixer *mixer ) = 0;
};



#endif // ISOUNDSYSTEM_H
