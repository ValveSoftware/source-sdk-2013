//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SOUNDENVELOPE_H
#define SOUNDENVELOPE_H

#ifdef _WIN32
#pragma once
#endif

#include "engine/IEngineSound.h"

class CSoundPatch;

enum soundcommands_t
{
	SOUNDCTRL_CHANGE_VOLUME,
	SOUNDCTRL_CHANGE_PITCH,
	SOUNDCTRL_STOP,
	SOUNDCTRL_DESTROY,
};

//Envelope point
struct envelopePoint_t
{
	float	amplitudeMin, amplitudeMax;
	float	durationMin, durationMax;
};

//Envelope description
struct envelopeDescription_t
{
	envelopePoint_t	*pPoints;
	int				nNumPoints;
};

class IRecipientFilter;

abstract_class CSoundEnvelopeController
{
public:
	virtual void		SystemReset( void ) = 0;
	virtual void		SystemUpdate( void ) = 0;
	virtual void		Play( CSoundPatch *pSound, float volume, float pitch, float flStartTime = 0 ) = 0;
	virtual void		CommandAdd( CSoundPatch *pSound, float executeDeltaTime, soundcommands_t command, float commandTime, float value ) = 0;
	virtual void		CommandClear( CSoundPatch *pSound ) = 0;
	virtual void		Shutdown( CSoundPatch *pSound ) = 0;

	virtual CSoundPatch	*SoundCreate( IRecipientFilter& filter, int nEntIndex, const char *pSoundName ) = 0;
	virtual CSoundPatch	*SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
							float attenuation ) = 0;
	virtual CSoundPatch	*SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
							soundlevel_t soundlevel ) = 0;
	virtual CSoundPatch	*SoundCreate( IRecipientFilter& filter, int nEntIndex, const EmitSound_t &es ) = 0;
	virtual void		SoundDestroy( CSoundPatch	* ) = 0;
	virtual void		SoundChangePitch( CSoundPatch *pSound, float pitchTarget, float deltaTime ) = 0;
	virtual void		SoundChangeVolume( CSoundPatch *pSound, float volumeTarget, float deltaTime ) = 0;
	virtual void		SoundFadeOut( CSoundPatch *pSound, float deltaTime, bool destroyOnFadeout = false ) = 0;
	virtual float		SoundGetPitch( CSoundPatch *pSound ) = 0;
	virtual float		SoundGetVolume( CSoundPatch *pSound ) = 0;
	
	virtual float		SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopePoint_t *points, int numPoints ) = 0;
	virtual float		SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopeDescription_t *envelope ) = 0;

	virtual void		CheckLoopingSoundsForPlayer( CBasePlayer *pPlayer ) = 0;

	virtual string_t	SoundGetName( CSoundPatch *pSound ) = 0;
	static	CSoundEnvelopeController &GetController( void );

	virtual void		SoundSetCloseCaptionDuration( CSoundPatch *pSound, float flDuration ) = 0;
};


//-----------------------------------------------------------------------------
// Save/restore
//-----------------------------------------------------------------------------
class ISaveRestoreOps;

ISaveRestoreOps *GetSoundSaveRestoreOps( );

#define DEFINE_SOUNDPATCH(name) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, FTYPEDESC_SAVE, NULL, GetSoundSaveRestoreOps( ), NULL }


#endif // SOUNDENVELOPE_H
