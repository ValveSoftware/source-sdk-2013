//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef SOUNDFLAGS_H
#define SOUNDFLAGS_H

#if defined( _WIN32 )
#pragma once
#endif


//-----------------------------------------------------------------------------
// channels
//-----------------------------------------------------------------------------
enum
{
	CHAN_REPLACE	= -1,
	CHAN_AUTO		= 0,
	CHAN_WEAPON		= 1,
	CHAN_VOICE		= 2,
	CHAN_ITEM		= 3,
	CHAN_BODY		= 4,
	CHAN_STREAM		= 5,		// allocate stream channel from the static or dynamic area
	CHAN_STATIC		= 6,		// allocate channel from the static area 
	CHAN_VOICE2		= 7,
	CHAN_VOICE_BASE	= 8,		// allocate channel for network voice data
	CHAN_USER_BASE	= (CHAN_VOICE_BASE+128)		// Anything >= this number is allocated to game code.
};

//-----------------------------------------------------------------------------
// common volume values
//-----------------------------------------------------------------------------
#define VOL_NORM		1.0f


//-----------------------------------------------------------------------------
// common attenuation values
//-----------------------------------------------------------------------------
#define ATTN_NONE		0.0f
#define ATTN_NORM		0.8f
#define ATTN_IDLE		2.0f
#define ATTN_STATIC		1.25f 
#define ATTN_RICOCHET	1.5f

// HL2 world is 8x bigger now! We want to hear gunfire from farther.
// Don't change this without consulting Kelly or Wedge (sjb).
#define ATTN_GUNFIRE	0.27f

enum soundlevel_t
{
	SNDLVL_NONE			= 0,

	SNDLVL_20dB			= 20,			// rustling leaves
	SNDLVL_25dB			= 25,			// whispering
	SNDLVL_30dB			= 30,			// library
	SNDLVL_35dB			= 35,
	SNDLVL_40dB			= 40,
	SNDLVL_45dB			= 45,			// refrigerator

	SNDLVL_50dB			= 50,	// 3.9	// average home
	SNDLVL_55dB			= 55,	// 3.0

	SNDLVL_IDLE			= 60,	// 2.0	
	SNDLVL_60dB			= 60,	// 2.0	// normal conversation, clothes dryer

	SNDLVL_65dB			= 65,	// 1.5	// washing machine, dishwasher
	SNDLVL_STATIC		= 66,	// 1.25

	SNDLVL_70dB			= 70,	// 1.0	// car, vacuum cleaner, mixer, electric sewing machine

	SNDLVL_NORM			= 75,
	SNDLVL_75dB			= 75,	// 0.8	// busy traffic

	SNDLVL_80dB			= 80,	// 0.7	// mini-bike, alarm clock, noisy restaurant, office tabulator, outboard motor, passing snowmobile
	SNDLVL_TALKING		= 80,	// 0.7
	SNDLVL_85dB			= 85,	// 0.6	// average factory, electric shaver
	SNDLVL_90dB			= 90,	// 0.5	// screaming child, passing motorcycle, convertible ride on frw
	SNDLVL_95dB			= 95,
	SNDLVL_100dB		= 100,	// 0.4	// subway train, diesel truck, woodworking shop, pneumatic drill, boiler shop, jackhammer
	SNDLVL_105dB		= 105,			// helicopter, power mower
	SNDLVL_110dB		= 110,			// snowmobile drvrs seat, inboard motorboat, sandblasting
	SNDLVL_120dB		= 120,			// auto horn, propeller aircraft
	SNDLVL_130dB		= 130,			// air raid siren

	SNDLVL_GUNFIRE		= 140,	// 0.27	// THRESHOLD OF PAIN, gunshot, jet engine
	SNDLVL_140dB		= 140,	// 0.2

	SNDLVL_150dB		= 150,	// 0.2

	SNDLVL_180dB		= 180,			// rocket launching

	// NOTE: Valid soundlevel_t values are 0-255.
	//       256-511 are reserved for sounds using goldsrc compatibility attenuation.
};

#define MAX_SNDLVL_BITS		9	// Used to encode 0-255 for regular soundlevel_t's and 256-511 for goldsrc-compatible ones.
#define MIN_SNDLVL_VALUE	0
#define MAX_SNDLVL_VALUE	((1<<MAX_SNDLVL_BITS)-1)


#define ATTN_TO_SNDLVL( a ) (soundlevel_t)(int)((a) ? (50 + 20 / ((float)a)) : 0 )
#define SNDLVL_TO_ATTN( a ) ((a > 50) ? (20.0f / (float)(a - 50)) : 4.0 )

// This is a limit due to network encoding.
// It encodes attenuation * 64 in 8 bits, so the maximum is (255 / 64)
#define MAX_ATTENUATION		3.98f

//-----------------------------------------------------------------------------
// Flags to be or-ed together for the iFlags field
//-----------------------------------------------------------------------------
enum SoundFlags_t
{
	SND_NOFLAGS			= 0,			// to keep the compiler happy
	SND_CHANGE_VOL		= (1<<0),		// change sound vol
	SND_CHANGE_PITCH	= (1<<1),		// change sound pitch
	SND_STOP			= (1<<2),		// stop the sound
	SND_SPAWNING		= (1<<3),		// we're spawning, used in some cases for ambients
										// not sent over net, only a param between dll and server.
	SND_DELAY			= (1<<4),		// sound has an initial delay
	SND_STOP_LOOPING	= (1<<5),		// stop all looping sounds on the entity.
	SND_SPEAKER			= (1<<6),		// being played again by a microphone through a speaker
 
	SND_SHOULDPAUSE		= (1<<7),		// this sound should be paused if the game is paused
	SND_IGNORE_PHONEMES	= (1<<8),
	SND_IGNORE_NAME		= (1<<9),		// used to change all sounds emitted by an entity, regardless of scriptname

	SND_DO_NOT_OVERWRITE_EXISTING_ON_CHANNEL = (1<<10),
};

#define SND_FLAG_BITS_ENCODE 11

#define MAX_SOUND_INDEX_BITS	14
#define	MAX_SOUNDS				(1<<MAX_SOUND_INDEX_BITS)

#if !defined( IN_XBOX_CODELINE )
// +/-4096 msec
#define MAX_SOUND_DELAY_MSEC_ENCODE_BITS	(13)
#else
// +/-65536 msec, 64 seconds
#define MAX_SOUND_DELAY_MSEC_ENCODE_BITS	(17)
#endif

// Subtract one to leave room for the sign bit
#define MAX_SOUND_DELAY_MSEC				(1<<(MAX_SOUND_DELAY_MSEC_ENCODE_BITS-1))    // 4096 msec or about 4 seconds

//-----------------------------------------------------------------------------
// common pitch values
//-----------------------------------------------------------------------------
#define	PITCH_NORM		100			// non-pitch shifted
#define PITCH_LOW		95			// other values are possible - 0-255, where 255 is very high
#define PITCH_HIGH		120

#define DEFAULT_SOUND_PACKET_VOLUME 1.0f
#define DEFAULT_SOUND_PACKET_PITCH	100
#define DEFAULT_SOUND_PACKET_DELAY	0.0f

#endif // SOUNDFLAGS_H
