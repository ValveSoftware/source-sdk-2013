//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DEMOFORMAT_H
#define DEMOFORMAT_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "utlvector.h"
#include "tier0/platform.h"

#define DEMO_HEADER_ID		"HL2DEMO"
#define DEMO_PROTOCOL		3

#if !defined( MAX_OSPATH )
#define	MAX_OSPATH		260			// max length of a filesystem pathname
#endif

// Demo messages
enum
{
	// it's a startup message, process as fast as possible
	dem_signon	= 1,
	// it's a normal network packet that we stored off
	dem_packet,
	// sync client clock to demo tick
	dem_synctick,
	// console command 
	dem_consolecmd,
	// user input command
	dem_usercmd,
	// network data tables
	dem_datatables,
	// end of time.
	dem_stop,

	dem_stringtables,

	// Last command
	dem_lastcmd		= dem_stringtables
};

struct demoheader_t
{
	char	demofilestamp[8];				// Should be HL2DEMO
	int		demoprotocol;					// Should be DEMO_PROTOCOL
	int		networkprotocol;				// Should be PROTOCOL_VERSION
	char	servername[ MAX_OSPATH ];		// Name of server
	char	clientname[ MAX_OSPATH ];		// Name of client who recorded the game
	char	mapname[ MAX_OSPATH ];			// Name of map
	char	gamedirectory[ MAX_OSPATH ];	// Name of game directory (com_gamedir)
	float	playback_time;					// Time of track
	int     playback_ticks;					// # of ticks in track
	int     playback_frames;				// # of frames in track
	int		signonlength;					// length of sigondata in bytes
};

inline void ByteSwap_demoheader_t( demoheader_t &swap )
{
	swap.demoprotocol = LittleDWord( swap.demoprotocol );
	swap.networkprotocol = LittleDWord( swap.networkprotocol );
	LittleFloat( &swap.playback_time, &swap.playback_time );
	swap.playback_ticks = LittleDWord( swap.playback_ticks );
	swap.playback_frames = LittleDWord( swap.playback_frames );
	swap.signonlength = LittleDWord( swap.signonlength );
}

#define FDEMO_NORMAL		0
#define FDEMO_USE_ORIGIN2	(1<<0)
#define FDEMO_USE_ANGLES2	(1<<1)
#define FDEMO_NOINTERP		(1<<2)	// don't interpolate between this an last view

struct democmdinfo_t
{
	// Default constructor
	democmdinfo_t()
	{
		flags = FDEMO_NORMAL;
		viewOrigin.Init();
		viewAngles.Init();
		localViewAngles.Init();

		// Resampled origin/angles
		viewOrigin2.Init();
		viewAngles2.Init();
		localViewAngles2.Init();
	}

	// Copy constructor
	// Assignment
	democmdinfo_t&	operator=(const democmdinfo_t& src )
	{
		if ( this == &src )
			return *this;

		flags = src.flags;
		viewOrigin = src.viewOrigin;
		viewAngles = src.viewAngles;
		localViewAngles = src.localViewAngles;
		viewOrigin2 = src.viewOrigin2;
		viewAngles2 = src.viewAngles2;
		localViewAngles2 = src.localViewAngles2;

		return *this;
	}

	const Vector& GetViewOrigin()
	{
		if ( flags & FDEMO_USE_ORIGIN2 )
		{
			return viewOrigin2;
		}
		return viewOrigin;
	}

	const QAngle& GetViewAngles()
	{
		if ( flags & FDEMO_USE_ANGLES2 )
		{
			return viewAngles2;
		}
		return viewAngles;
	}
	const QAngle& GetLocalViewAngles()
	{
		if ( flags & FDEMO_USE_ANGLES2 )
		{
			return localViewAngles2;
		}
		return localViewAngles;
	}

	void Reset( void )
	{
		flags = 0;
		viewOrigin2 = viewOrigin;
		viewAngles2 = viewAngles;
		localViewAngles2 = localViewAngles;
	}

	int			flags;

	// original origin/viewangles
	Vector		viewOrigin;
	QAngle		viewAngles;
	QAngle		localViewAngles;

	// Resampled origin/viewangles
	Vector		viewOrigin2;
	QAngle		viewAngles2;
	QAngle		localViewAngles2;
};

struct demosmoothing_t
{
	demosmoothing_t()
	{
		file_offset = 0;
		frametick = 0;
		selected = false;
		samplepoint = false;

		vecmoved.Init();
		angmoved.Init();

		targetpoint = false;
		vectarget.Init();
	}

	demosmoothing_t&	operator=(const demosmoothing_t& src )
	{
		if ( this == &src )
			return *this;

		file_offset = src.file_offset;
		frametick = src.frametick;
		selected = src.selected;
		samplepoint = src.samplepoint;
		vecmoved = src.vecmoved;
		angmoved = src.angmoved;

		targetpoint = src.targetpoint;
		vectarget = src.vectarget;

		info = src.info;

		return *this;
	}

	int					file_offset;

	int					frametick;

	bool				selected;

	// For moved sample points
	bool				samplepoint;
	Vector				vecmoved;
	QAngle				angmoved;

	bool				targetpoint;
	Vector				vectarget;

	democmdinfo_t		info;
};

struct CSmoothingContext
{
	CSmoothingContext()
	{
		active = false;
		filename[ 0 ] = 0;
		m_nFirstSelectableSample = 0;
	}

	CSmoothingContext&	operator=(const CSmoothingContext& src )
	{
		if ( this == &src )
			return *this;

		active = src.active;
		Q_strncpy( filename, src.filename, sizeof( filename ) );

		smooth.RemoveAll();
		int c = src.smooth.Count();
		int i;
		for ( i = 0; i < c; i++ )
		{
			demosmoothing_t newitem;
			newitem = src.smooth[ i ];
			smooth.AddToTail( newitem );
		}

		m_nFirstSelectableSample = src.m_nFirstSelectableSample;

		return *this;
	}

	bool							active;
	char							filename[ 512 ];
	CUtlVector< demosmoothing_t >	smooth;
	int								m_nFirstSelectableSample;
};

#endif // DEMOFORMAT_H
