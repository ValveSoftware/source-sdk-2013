//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: client sound i/o functions
//
//===========================================================================//
#ifndef SOUND_H
#define SOUND_H
#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "datamap.h"
#include "mathlib/vector.h"
#include "mathlib/mathlib.h"
#include "tier1/strtools.h"
#include "soundflags.h"
#include "utlvector.h"
#include "engine/SndInfo.h"
#include "soundstartparams.h"

#define MAX_SFX  2048

#define AUDIOSOURCE_CACHE_ROOTDIR	"maps/soundcache"

class CSfxTable;
enum soundlevel_t;
struct SoundInfo_t;
struct AudioState_t;
class IFileList;

void S_Init (void);
void S_Shutdown (void);
bool S_IsInitted();

void S_StopAllSounds(bool clear);
void S_Update( const AudioState_t *pAudioState );
void S_ExtraUpdate (void);
void S_ClearBuffer (void);
void S_BlockSound (void);
void S_UnblockSound (void);
void S_UpdateWindowFocus( bool bWindowHasFocus );
float S_GetMasterVolume( void );
void S_SoundFade( float percent, float holdtime, float intime, float outtime );
void S_OnLoadScreen(bool value);
void S_EnableThreadedMixing( bool bEnable );
void S_EnableMusic( bool bEnable );

struct audio_device_description_t;
void S_GetAudioDeviceList( CUtlVector<audio_device_description_t> &audioList );

int S_StartSound( StartSoundParams_t& params );
void S_StopSound ( int entnum, int entchannel );
enum clocksync_index_t
{
	CLOCK_SYNC_CLIENT = 0,
	CLOCK_SYNC_SERVER,
	NUM_CLOCK_SYNCS
};

extern float S_ComputeDelayForSoundtime( float soundtime, clocksync_index_t syncIndex );

void S_StopSoundByGuid( int guid );
float S_SoundDurationByGuid( int guid );
int S_GetGuidForLastSoundEmitted();
bool S_IsSoundStillPlaying( int guid );
void S_GetActiveSounds( CUtlVector< SndInfo_t >& sndlist );
void S_SetVolumeByGuid( int guid, float fvol );
float S_GetElapsedTimeByGuid( int guid );
bool S_IsLoopingSoundByGuid( int guid );
void S_ReloadSound( const char *pSample );
float S_GetMono16Samples( const char *pszName, CUtlVector< short >& sampleList );

CSfxTable *S_DummySfx( const char *name );
CSfxTable *S_PrecacheSound (const char *sample );
void S_PrefetchSound( char const *name, bool bPlayOnce );
void S_MarkUISound( CSfxTable *pSfx );
void S_ReloadFilesInList( IFileList *pFilesToReload );

vec_t S_GetNominalClipDist();

extern bool TestSoundChar(const char *pch, char c);
extern char *PSkipSoundChars(const char *pch);

#include "soundchars.h"

// for recording movies
void SND_MovieStart( void );
void SND_MovieEnd( void );
void SND_MovieUpdateChannelCount( void );

//-------------------------------------

int S_GetCurrentStaticSounds( SoundInfo_t *pResult, int nSizeResult, int entchannel );

//-----------------------------------------------------------------------------

float S_GetGainFromSoundLevel( soundlevel_t soundlevel, vec_t dist );

struct musicsave_t
{
	DECLARE_SIMPLE_DATADESC();

	char	songname[ 128 ];
	int		sampleposition;
	short	master_volume;
};

void S_GetCurrentlyPlayingMusic( CUtlVector< musicsave_t >& list );
void S_RestartSong( const musicsave_t *song );

#endif // SOUND_H
