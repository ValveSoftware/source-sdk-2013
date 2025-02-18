#pragma once

#include "soundflags.h"
#include "mathlib/vector.h"

class CSfxTable;

struct StartSoundParams_t
{
	StartSoundParams_t() :
		staticsound( false ),
		userdata( 0 ),
		soundsource( 0 ), 
		entchannel( CHAN_AUTO ), 
		pSfx( 0 ), 
		bUpdatePositions( true ),
		fvol( 1.0f ),  
		soundlevel( SNDLVL_NORM ), 
		flags( SND_NOFLAGS ), 
		pitch( PITCH_NORM ), 
		specialdsp( 0 ),
		fromserver( false ),
		delay( 0.0f ),
		speakerentity( -1 ),
		suppressrecording( false ),
		initialStreamPosition( 0 )
	{
		origin.Init();
		direction.Init();
	}

	bool			staticsound;
	int				userdata;
    int				soundsource;
	int				entchannel;
	CSfxTable		*pSfx;
	Vector			origin; 
	Vector			direction; 
	bool			bUpdatePositions;
	float			fvol;
	soundlevel_t	soundlevel;
	int				flags;
	int				pitch;
	int				specialdsp;
	bool			fromserver;
	float			delay;
	int				speakerentity;
	bool			suppressrecording;
	int				initialStreamPosition;
};
