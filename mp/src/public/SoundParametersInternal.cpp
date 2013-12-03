//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"

#if !defined(_STATIC_LINKED) || defined(SOUNDEMITTERSYSTEM_DLL)

#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "interval.h"
#include "soundchars.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct SoundChannels
{
	int			channel;
	const char *name;
};

// NOTE:  This will need to be updated if channel names are added/removed
static SoundChannels g_pChannelNames[] =
{
	{ CHAN_AUTO, "CHAN_AUTO" },
	{ CHAN_WEAPON, "CHAN_WEAPON" },
	{ CHAN_VOICE, "CHAN_VOICE" },
	{ CHAN_ITEM, "CHAN_ITEM" },
	{ CHAN_BODY, "CHAN_BODY" },
	{ CHAN_STREAM, "CHAN_STREAM" },
	{ CHAN_STATIC, "CHAN_STATIC" },
	{ CHAN_VOICE2, "CHAN_VOICE2" },
};

struct VolumeLevel
{
	float		volume;
	const char *name;
};

static VolumeLevel g_pVolumeLevels[] = 
{
	{ VOL_NORM, "VOL_NORM" },
};

struct PitchLookup
{
	float		pitch;
	const char *name;
};

static PitchLookup g_pPitchLookup[] =
{
	{ PITCH_NORM,	"PITCH_NORM" },
	{ PITCH_LOW,	"PITCH_LOW" },
	{ PITCH_HIGH,	"PITCH_HIGH" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct SoundLevelLookup
{
	soundlevel_t	level;
	char const		*name;
};

// NOTE:  Needs to reflect the soundlevel_t enum defined in soundflags.h
static SoundLevelLookup g_pSoundLevels[] =
{
	{ SNDLVL_NONE, "SNDLVL_NONE" },
	{ SNDLVL_20dB, "SNDLVL_20dB" },
	{ SNDLVL_25dB, "SNDLVL_25dB" },
	{ SNDLVL_30dB, "SNDLVL_30dB" },
	{ SNDLVL_35dB, "SNDLVL_35dB" },
	{ SNDLVL_40dB, "SNDLVL_40dB" },
	{ SNDLVL_45dB, "SNDLVL_45dB" },
	{ SNDLVL_50dB, "SNDLVL_50dB" },
	{ SNDLVL_55dB, "SNDLVL_55dB" },
	{ SNDLVL_IDLE, "SNDLVL_IDLE" },
	{ SNDLVL_TALKING, "SNDLVL_TALKING" },
	{ SNDLVL_60dB, "SNDLVL_60dB" },
	{ SNDLVL_65dB, "SNDLVL_65dB" },
	{ SNDLVL_STATIC, "SNDLVL_STATIC" },
	{ SNDLVL_70dB, "SNDLVL_70dB" },
	{ SNDLVL_NORM, "SNDLVL_NORM" },
	{ SNDLVL_75dB, "SNDLVL_75dB" },
	{ SNDLVL_80dB, "SNDLVL_80dB" },
	{ SNDLVL_85dB, "SNDLVL_85dB" },
	{ SNDLVL_90dB, "SNDLVL_90dB" },
	{ SNDLVL_95dB, "SNDLVL_95dB" },
	{ SNDLVL_100dB, "SNDLVL_100dB" },
	{ SNDLVL_105dB, "SNDLVL_105dB" },
	{ SNDLVL_110dB, "SNDLVL_110dB" },
	{ SNDLVL_120dB, "SNDLVL_120dB" },
	{ SNDLVL_130dB, "SNDLVL_130dB" },
	{ SNDLVL_GUNFIRE, "SNDLVL_GUNFIRE" },
	{ SNDLVL_140dB, "SNDLVL_140dB" },
	{ SNDLVL_150dB, "SNDLVL_150dB" },
	{ SNDLVL_180dB, "SNDLVL_180dB" },
};

static const char *_SoundLevelToString( soundlevel_t level )
{
	int c = ARRAYSIZE( g_pSoundLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundLevelLookup *entry = &g_pSoundLevels[ i ];
		if ( entry->level == level )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)level );
	return sz;
}

static const char *_ChannelToString( int channel )
{
	int c = ARRAYSIZE( g_pChannelNames );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundChannels *entry = &g_pChannelNames[ i ];
		if ( entry->channel == channel )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)channel );
	return sz;
}

static const char *_VolumeToString( float volume )
{
	int c = ARRAYSIZE( g_pVolumeLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		VolumeLevel *entry = &g_pVolumeLevels[ i ];
		if ( entry->volume == volume )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", volume );
	return sz;
}

static const char *_PitchToString( float pitch )
{
	int c = ARRAYSIZE( g_pPitchLookup );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		PitchLookup *entry = &g_pPitchLookup[ i ];
		if ( entry->pitch == pitch )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", pitch );
	return sz;
}

#define SNDLVL_PREFIX "SNDLVL_"

soundlevel_t TextToSoundLevel( const char *key )
{
	if ( !key )
	{
		Assert( 0 );
		return SNDLVL_NORM;
	}

	int c = ARRAYSIZE( g_pSoundLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundLevelLookup *entry = &g_pSoundLevels[ i ];
		if ( !Q_strcasecmp( key, entry->name ) )
			return entry->level;
	}

	if ( !Q_strnicmp( key, SNDLVL_PREFIX, Q_strlen( SNDLVL_PREFIX ) ) )
	{
		char const *val = key + Q_strlen( SNDLVL_PREFIX );
		int sndlvl = atoi( val );
		if ( sndlvl > 0 && sndlvl <= 180 )
		{
			return ( soundlevel_t )sndlvl;
		}
	}

	DevMsg( "CSoundEmitterSystem:  Unknown sound level %s\n", key );

	return SNDLVL_NORM;
}

//-----------------------------------------------------------------------------
// Purpose: Convert "chan_xxx" into integer value for channel
// Input  : *name - 
// Output : static int
//-----------------------------------------------------------------------------
int TextToChannel( const char *name )
{
	if ( !name )
	{
		Assert( 0 );
		// CHAN_AUTO
		return CHAN_AUTO;
	}

	if ( Q_strncasecmp( name, "chan_", strlen( "chan_" ) ) )
	{
		return atoi( name );
	}

	int c = ARRAYSIZE( g_pChannelNames );
	int i;

	for ( i = 0; i < c; i++ )
	{
		if ( !Q_strcasecmp( name, g_pChannelNames[ i ].name ) )
		{
			return g_pChannelNames[ i ].channel;
		}
	}

	// At this point, it starts with chan_ but is not recognized
	// atoi would return 0, so just do chan auto
	DevMsg( "CSoundEmitterSystem:  Warning, unknown channel type in sounds.txt (%s)\n", name );

	return CHAN_AUTO;
}

const char *SoundLevelToString( soundlevel_t level )
{
	int c = ARRAYSIZE( g_pSoundLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundLevelLookup *entry = &g_pSoundLevels[ i ];
		if ( entry->level == level )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)level );
	return sz;
}

const char *ChannelToString( int channel )
{
	int c = ARRAYSIZE( g_pChannelNames );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundChannels *entry = &g_pChannelNames[ i ];
		if ( entry->channel == channel )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)channel );
	return sz;
}

const char *VolumeToString( float volume )
{
	int c = ARRAYSIZE( g_pVolumeLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		VolumeLevel *entry = &g_pVolumeLevels[ i ];
		if ( entry->volume == volume )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", volume );
	return sz;
}

const char *PitchToString( float pitch )
{
	int c = ARRAYSIZE( g_pPitchLookup );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		PitchLookup *entry = &g_pPitchLookup[ i ];
		if ( entry->pitch == pitch )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", pitch );
	return sz;
}

CSoundParametersInternal::CSoundParametersInternal()
{
	m_pConvertedNames = m_pSoundNames = NULL;
	m_nConvertedNames = m_nSoundNames = 0;
	channel			= CHAN_AUTO; // 0

	volume.start	= VOL_NORM;  // 1.0f
	volume.range	= 0.0f;

	pitch.start		= PITCH_NORM; // 100
	pitch.range		= 0;

	soundlevel.start = SNDLVL_NORM; // 75dB
	soundlevel.range = 0;
	delay_msec = 0;
	play_to_owner_only = false;
	had_missing_wave_files = false;
	uses_gender_token = false;

}

CSoundParametersInternal::CSoundParametersInternal( const CSoundParametersInternal& src )
{
	m_pSoundNames = NULL;
	m_pConvertedNames = NULL;
	m_nSoundNames = 0;
	m_nConvertedNames = 0;
	CopyFrom( src );
}

CSoundParametersInternal::~CSoundParametersInternal()
{
	if ( m_nSoundNames > 1 )
		free(m_pSoundNames );
	if ( m_nConvertedNames > 1 )
		free( m_pConvertedNames);

	m_pConvertedNames = NULL;
	m_pSoundNames = NULL;
	m_nSoundNames = 0;
	m_nConvertedNames = 0;
}

void CSoundParametersInternal::CopyFrom( const CSoundParametersInternal& src )
{
	if ( m_nSoundNames > 1 )
		free(m_pSoundNames);
	if ( m_nConvertedNames > 1 )
		free(m_pConvertedNames);

	channel = src.channel;
	volume = src.volume;
	pitch = src.pitch;
	soundlevel = src.soundlevel;
	delay_msec = src.delay_msec;
	play_to_owner_only = src.play_to_owner_only;

	m_nSoundNames = src.m_nSoundNames;
	if ( m_nSoundNames )
	{
		if ( m_nSoundNames > 1 )
		{
			m_pSoundNames = (SoundFile*)malloc( sizeof(SoundFile)*m_nSoundNames);
			memcpy( m_pSoundNames, src.m_pSoundNames, m_nSoundNames * sizeof(SoundFile) );
		}
		else
		{
			m_pSoundNames = src.m_pSoundNames;
		}
	}
	else
	{
		m_pSoundNames = NULL;
	}

	m_nConvertedNames = src.m_nConvertedNames;
	if ( m_nConvertedNames )
	{
		if ( m_nConvertedNames > 1 )
		{
			m_pConvertedNames = (SoundFile*)malloc( sizeof(SoundFile)*m_nConvertedNames);
			memcpy( m_pConvertedNames, src.m_pConvertedNames, m_nConvertedNames * sizeof(SoundFile) );
		}
		else
		{
			m_pConvertedNames = src.m_pConvertedNames;
		}
	}
	else
	{
		m_pConvertedNames = NULL;
	}

	had_missing_wave_files = src.had_missing_wave_files;
	uses_gender_token = src.uses_gender_token;
}

#define CompareInterval( i1, i2 ) ( memcmp( &(i1), &(i2), sizeof(i1) ) == 0 )

bool CSoundParametersInternal::operator == ( const CSoundParametersInternal& other ) const
{
	if ( this == &other )
		return true;

	if ( channel != other.channel )
		return false;
	if ( !CompareInterval( volume, other.volume ) )
		return false;
	if ( !CompareInterval( pitch, other.pitch ) )
		return false;
	if ( !CompareInterval( soundlevel, other.soundlevel ) )
		return false;
	if ( delay_msec != other.delay_msec )
		return false;
	if ( play_to_owner_only != other.play_to_owner_only )
		return false;

	if ( m_nSoundNames != other.m_nSoundNames )
		return false;

	// Compare items
	int c = m_nSoundNames;
	for ( int i = 0; i < c; i++ )
	{
		if ( GetSoundNames()[ i ].symbol != other.GetSoundNames()[ i ].symbol )
			return false;
	}

	return true;
}

float16 ZERO_FLOAT16;

const char *CSoundParametersInternal::VolumeToString( void ) const
{
	if ( volume.range == ZERO_FLOAT16 )
	{
		return _VolumeToString( volume.start );
	}

	static char sz[ 64 ];
	Q_snprintf( sz, sizeof( sz ),  "%.3f, %.3f", (float)volume.start, (float)volume.start + (float)volume.range );
	return sz;
}

const char *CSoundParametersInternal::ChannelToString( void ) const 
{
	return _ChannelToString( channel );
}

const char *CSoundParametersInternal::SoundLevelToString( void ) const
{
	if ( soundlevel.range == 0 )
	{
		return _SoundLevelToString( (soundlevel_t)(int)soundlevel.start );
	}

	static char sz[ 64 ];
	Q_snprintf( sz, sizeof( sz ),  "%i, %i", (soundlevel_t)(int)soundlevel.start, (soundlevel_t)(int)(soundlevel.start + soundlevel.range ) );
	return sz;
}

const char *CSoundParametersInternal::PitchToString( void ) const
{
	if ( pitch.range == 0 )
	{
		return _PitchToString( (int)pitch.start );
	}

	static char sz[ 64 ];
	Q_snprintf( sz, sizeof( sz ),  "%i, %i", (int)pitch.start, (int)(pitch.start + pitch.range ) );
	return sz;
}

void CSoundParametersInternal::VolumeFromString( const char *sz )
{
	if ( !Q_strcasecmp( sz, "VOL_NORM" ) )
	{
		volume.start = VOL_NORM;
		volume.range = 0.0f;
	}
	else
	{
		volume.FromInterval( ReadInterval( sz ) );
	}
}

void CSoundParametersInternal::ChannelFromString( const char *sz )
{
	channel = TextToChannel( sz );
}

void CSoundParametersInternal::PitchFromString( const char *sz )
{
	if ( !Q_strcasecmp( sz, "PITCH_NORM" ) )
	{
		pitch.start	= PITCH_NORM;
		pitch.range = 0;
	}
	else if ( !Q_strcasecmp( sz, "PITCH_LOW" ) )
	{
		pitch.start	= PITCH_LOW;
		pitch.range = 0;
	}
	else if ( !Q_strcasecmp( sz, "PITCH_HIGH" ) )
	{
		pitch.start	= PITCH_HIGH;
		pitch.range = 0;
	}
	else
	{
		pitch.FromInterval( ReadInterval( sz ) );
	}
}

void CSoundParametersInternal::SoundLevelFromString( const char *sz )
{
	if ( !Q_strncasecmp( sz, "SNDLVL_", strlen( "SNDLVL_" ) ) )
	{
		soundlevel.start = TextToSoundLevel( sz );
		soundlevel.range = 0;
	}
	else
	{
		soundlevel.FromInterval( ReadInterval( sz ) );
	}
}

void CSoundParametersInternal::AddToTail( SoundFile **pDest, uint16 *pDestCount, const SoundFile &source )
{
	(*pDestCount)++;
	if ( *pDestCount == 1 )
	{
		// NOTE: when there's only one soundfile in the list, we store it
		// packed into the pointer itself, the four bytes for the pointer is just used to store the sound file!
		COMPILE_TIME_ASSERT( sizeof(SoundFile) <= sizeof(SoundFile *) );
		*((SoundFile *)(pDest)) = source;
	}
	else
	{
		SoundFile temp;
		if ( *pDestCount == 2 )
		{
			// Copying from a list of one soundfile. Save off the struct
			// packed into the pointer field.
			temp = *((SoundFile *)(pDest));
			*pDest = NULL;
		}

		*pDest = (SoundFile *)realloc( *pDest, (*pDestCount) * sizeof(SoundFile) );
		(*pDest)[ *pDestCount - 1 ] = source;

		if ( *pDestCount == 2 )
		{
			(*pDest)[0] = temp;
		}
	}
}

#endif // !_STATIC_LINKED || SOUNDEMITTERSYSTEM_DLL
