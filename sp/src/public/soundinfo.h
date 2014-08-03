//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SOUNDINFO_H
#define SOUNDINFO_H

#ifdef _WIN32
#pragma once
#endif

#include "bitbuf.h"
#include "const.h"
#include "soundflags.h"
#include "coordsize.h"
#include "mathlib/vector.h"


#define WRITE_DELTA_UINT( name, length )	\
	if ( name == delta->name )		\
		buffer.WriteOneBit(0);		\
	else					\
	{					\
		buffer.WriteOneBit(1);		\
		buffer.WriteUBitLong( name, length ); \
	}

#define READ_DELTA_UINT( name, length )			\
	if ( buffer.ReadOneBit() != 0 )			\
	{ name = buffer.ReadUBitLong( length ); }\
	else { name = delta->name; }

#define WRITE_DELTA_SINT( name, length )	\
	if ( name == delta->name )		\
		buffer.WriteOneBit(0);		\
	else					\
	{					\
		buffer.WriteOneBit(1);		\
		buffer.WriteSBitLong( name, length ); \
	}

#define WRITE_DELTA_SINT_SCALE( name, scale, length )	\
	if ( name == delta->name )		\
	buffer.WriteOneBit(0);		\
	else					\
	{					\
	buffer.WriteOneBit(1);		\
	buffer.WriteSBitLong( name / scale, length ); \
	}

#define READ_DELTA_SINT( name, length )			\
	if ( buffer.ReadOneBit() != 0 )			\
	{ name = buffer.ReadSBitLong( length ); }	\
	else { name = delta->name; }

#define READ_DELTA_SINT_SCALE( name, scale, length )			\
	if ( buffer.ReadOneBit() != 0 )			\
	{ name = scale * buffer.ReadSBitLong( length ); }	\
	else { name = delta->name; }

#define SOUND_SEQNUMBER_BITS	10
#define SOUND_SEQNUMBER_MASK	( (1<<SOUND_SEQNUMBER_BITS) - 1 )

// offset sound delay encoding by 60ms since we encode negative sound delays with less precision
// This means any negative sound delay greater than -100ms will get encoded at full precision
#define SOUND_DELAY_OFFSET					(0.100f)

#pragma pack(4)
//-----------------------------------------------------------------------------
struct SoundInfo_t
{
	int				nSequenceNumber;
	int				nEntityIndex;
	int				nChannel;
	const char		*pszName;		// UNDONE: Make this a FilenameHandle_t to avoid bugs with arrays of these
	Vector			vOrigin;
	Vector			vDirection;
	float			fVolume;
	soundlevel_t	Soundlevel;
	bool			bLooping;
	int				nPitch;
	int				nSpecialDSP;
	Vector			vListenerOrigin;
	int				nFlags;
	int 			nSoundNum;
	float			fDelay;
	bool			bIsSentence;
	bool			bIsAmbient;
	int				nSpeakerEntity;
	
	//---------------------------------
	
	SoundInfo_t()
	{
		SetDefault();
	}

	void Set(int newEntity, int newChannel, const char *pszNewName, const Vector &newOrigin, const Vector& newDirection, 
			float newVolume, soundlevel_t newSoundLevel, bool newLooping, int newPitch, const Vector &vecListenerOrigin, int speakerentity )
	{
		nEntityIndex = newEntity;
		nChannel = newChannel;
		pszName = pszNewName;
		vOrigin = newOrigin;
		vDirection = newDirection;
		fVolume = newVolume;
		Soundlevel = newSoundLevel;
		bLooping = newLooping;
		nPitch = newPitch;
		vListenerOrigin = vecListenerOrigin;
		nSpeakerEntity = speakerentity;
	}

	void SetDefault()
	{
		fDelay = DEFAULT_SOUND_PACKET_DELAY;
		fVolume = DEFAULT_SOUND_PACKET_VOLUME;
		Soundlevel = SNDLVL_NORM;
		nPitch = DEFAULT_SOUND_PACKET_PITCH;
		nSpecialDSP = 0;

		nEntityIndex = 0;
		nSpeakerEntity = -1;
		nChannel = CHAN_STATIC;
		nSoundNum = 0;
		nFlags = 0;
		nSequenceNumber = 0;

		pszName = NULL;
	
		bLooping = false;
		bIsSentence = false;
		bIsAmbient = false;
		
		vOrigin.Init();
		vDirection.Init();
		vListenerOrigin.Init();
	}

	void ClearStopFields()
	{
		fVolume = 0;
		Soundlevel = SNDLVL_NONE;
		nPitch = PITCH_NORM;
		nSpecialDSP = 0;
		pszName = NULL;
		fDelay = 0.0f;
		nSequenceNumber = 0;

		vOrigin.Init();
		nSpeakerEntity = -1;
	}

	// this cries for Send/RecvTables:
	void WriteDelta( SoundInfo_t *delta, bf_write &buffer)
	{
		if ( nEntityIndex == delta->nEntityIndex )
		{
			buffer.WriteOneBit( 0 );
		}
		else
		{
			buffer.WriteOneBit( 1 );
		
			if ( nEntityIndex <= 31)
			{
				buffer.WriteOneBit( 1 );
				buffer.WriteUBitLong( nEntityIndex, 5 );
			}
			else
			{
				buffer.WriteOneBit( 0 );
				buffer.WriteUBitLong( nEntityIndex, MAX_EDICT_BITS );
			}
		}

		WRITE_DELTA_UINT( nSoundNum, MAX_SOUND_INDEX_BITS );

		WRITE_DELTA_UINT( nFlags, SND_FLAG_BITS_ENCODE );

		WRITE_DELTA_UINT( nChannel, 3 );

		buffer.WriteOneBit( bIsAmbient?1:0 );
		buffer.WriteOneBit( bIsSentence?1:0 ); // NOTE: SND_STOP behavior is different depending on this flag

		if ( nFlags != SND_STOP )
		{
			if ( nSequenceNumber == delta->nSequenceNumber )
			{
				// didn't change, most often case
				buffer.WriteOneBit( 1 );
			}
			else if ( nSequenceNumber == (delta->nSequenceNumber+1) )
			{
				// increased by one
				buffer.WriteOneBit( 0 );
				buffer.WriteOneBit( 1 );
			}
			else
			{
				// send full seqnr
				buffer.WriteUBitLong( 0, 2 ); // 2 zero bits
				buffer.WriteUBitLong( nSequenceNumber, SOUND_SEQNUMBER_BITS ); 
			}
						
			if ( fVolume == delta->fVolume )
			{
				buffer.WriteOneBit( 0 );
			}
			else
			{
				buffer.WriteOneBit( 1 );
				buffer.WriteUBitLong( (unsigned int)(fVolume*127.0f), 7 );
			}

			WRITE_DELTA_UINT( Soundlevel, MAX_SNDLVL_BITS );

			WRITE_DELTA_UINT( nPitch, 8 );

			WRITE_DELTA_UINT( nSpecialDSP, 8 );

			if ( fDelay == delta->fDelay )
			{
				buffer.WriteOneBit( 0 );
			}
			else
			{
				buffer.WriteOneBit( 1 );

				// skipahead works in 10 ms increments
				// bias results so that we only incur the precision loss on relatively large skipaheads
				fDelay += SOUND_DELAY_OFFSET;

				// Convert to msecs
				int iDelay = fDelay * 1000.0f;

				iDelay = clamp( iDelay, (int)(-10 * MAX_SOUND_DELAY_MSEC), (int)(MAX_SOUND_DELAY_MSEC) );

				if ( iDelay < 0 )
				{
					iDelay /=10;	
				}
				
				buffer.WriteSBitLong( iDelay , MAX_SOUND_DELAY_MSEC_ENCODE_BITS );
			}

			// don't transmit sounds with high precision
			WRITE_DELTA_SINT_SCALE( vOrigin.x, 8.0f, COORD_INTEGER_BITS - 2 );
			WRITE_DELTA_SINT_SCALE( vOrigin.y, 8.0f, COORD_INTEGER_BITS - 2  );
			WRITE_DELTA_SINT_SCALE( vOrigin.z, 8.0f, COORD_INTEGER_BITS - 2 );

			WRITE_DELTA_SINT( nSpeakerEntity, MAX_EDICT_BITS + 1 );
		}
		else
		{
			ClearStopFields();
		}
	};

	void ReadDelta( SoundInfo_t *delta, bf_read &buffer, int nProtoVersion )
	{
		if ( !buffer.ReadOneBit() )
		{
			nEntityIndex = delta->nEntityIndex;
		}
		else
		{
			if ( buffer.ReadOneBit() )
			{
				nEntityIndex = buffer.ReadUBitLong( 5 );
			}
			else
			{
				nEntityIndex = buffer.ReadUBitLong( MAX_EDICT_BITS );
			}
		}

		if ( nProtoVersion > 22 )
		{	
			READ_DELTA_UINT( nSoundNum, MAX_SOUND_INDEX_BITS );
		}
		else
		{
			READ_DELTA_UINT( nSoundNum, 13 );
		}

		if ( nProtoVersion > 18 )
		{
			READ_DELTA_UINT( nFlags, SND_FLAG_BITS_ENCODE );
		}
		else
		{
			// There was 9 flag bits for version 18 and below (prior to Halloween 2011)
			READ_DELTA_UINT( nFlags, 9 );
		}

		READ_DELTA_UINT( nChannel, 3 );

		bIsAmbient = buffer.ReadOneBit() != 0;
		bIsSentence = buffer.ReadOneBit() != 0; // NOTE: SND_STOP behavior is different depending on this flag

		if ( nFlags != SND_STOP )
		{
			if ( buffer.ReadOneBit() != 0 )
			{
				nSequenceNumber = delta->nSequenceNumber;
			}
			else if ( buffer.ReadOneBit() != 0 )
			{
				nSequenceNumber = delta->nSequenceNumber + 1;
			}
			else
			{
				nSequenceNumber = buffer.ReadUBitLong( SOUND_SEQNUMBER_BITS );
			}
				
			if ( buffer.ReadOneBit() != 0 )
			{
				fVolume = (float)buffer.ReadUBitLong( 7 )/127.0f;
			}
			else
			{
				fVolume = delta->fVolume;
			}

			if ( buffer.ReadOneBit() != 0 )
			{
				Soundlevel = (soundlevel_t)buffer.ReadUBitLong( MAX_SNDLVL_BITS );
			}
			else
			{
				Soundlevel = delta->Soundlevel;
			}

			READ_DELTA_UINT( nPitch, 8 );

			if ( nProtoVersion > 21 )
			{
				// These bit weren't written in version 19 and below
				READ_DELTA_UINT( nSpecialDSP, 8 );
			}

			if ( buffer.ReadOneBit() != 0 )
			{
				// Up to 4096 msec delay
				fDelay = (float)buffer.ReadSBitLong( MAX_SOUND_DELAY_MSEC_ENCODE_BITS ) / 1000.0f; ;
				
				if ( fDelay < 0 )
				{
					fDelay *= 10.0f;
				}
				// bias results so that we only incur the precision loss on relatively large skipaheads
				fDelay -= SOUND_DELAY_OFFSET;
			}
			else
			{
				fDelay = delta->fDelay;
			}

			READ_DELTA_SINT_SCALE( vOrigin.x, 8.0f, COORD_INTEGER_BITS - 2 );
			READ_DELTA_SINT_SCALE( vOrigin.y, 8.0f, COORD_INTEGER_BITS - 2 );
			READ_DELTA_SINT_SCALE( vOrigin.z, 8.0f, COORD_INTEGER_BITS - 2 );

			READ_DELTA_SINT( nSpeakerEntity, MAX_EDICT_BITS + 1 );
		}
		else
		{
			ClearStopFields();
		}
	}
};

struct SpatializationInfo_t
{
	typedef enum
	{
		SI_INCREATION = 0,
		SI_INSPATIALIZATION
	} SPATIALIZATIONTYPE;

	// Inputs
	SPATIALIZATIONTYPE	type;
	// Info about the sound, channel, origin, direction, etc.
	SoundInfo_t			info;

	// Requested Outputs ( NULL == not requested )
	Vector				*pOrigin;
	QAngle				*pAngles;
	float				*pflRadius;
};
#pragma pack()

#endif // SOUNDINFO_H
