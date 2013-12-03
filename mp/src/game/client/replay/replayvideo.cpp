//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayvideo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

static ReplayVideoMode_t s_VideoModes[] =
{
	{ 720, 480,   24, true,  "#Replay_Res_480p" },
	{ 1280, 720,  24, true,  "#Replay_Res_720p" },
	{ 1920, 1080, 24, true,  "#Replay_Res_1080p" },

	{ 320, 240,   15, false, "#Replay_Res_Web" },
	{ 960, 640,   24, true,  "#Replay_Res_iPhone_Horizontal" },
	{ 640, 960,   24, true,  "#Replay_Res_iPhone_Vertical" },
};

#define  VIDEO_MODE_COUNT  ( sizeof( s_VideoModes ) / sizeof( s_VideoModes[0] ) )

//-----------------------------------------------------------------------------
#ifdef USE_WEBM_FOR_REPLAY
static ReplayCodec_t s_Codecs[] = 
{
	{ VideoEncodeCodec::WEBM_CODEC, "#Replay_Codec_WEBM" },
};
static int s_nNumCodecs = ARRAYSIZE( s_Codecs );

//-----------------------------------------------------------------------------

static ReplayQualityPreset_t s_QualityPresets[] = 
{
	{ "#Replay_RenderSetting_Low", VideoEncodeCodec::WEBM_CODEC, 0, false, 0 },
	{ "#Replay_RenderSetting_Medium", VideoEncodeCodec::WEBM_CODEC, 50, false, 1 },
	{ "#Replay_RenderSetting_High", VideoEncodeCodec::WEBM_CODEC, 100, true, 2 },
	{ "#Replay_RenderSetting_Max", VideoEncodeCodec::WEBM_CODEC, 100, true, 3 },
};
static int s_NumQualityPresets = ARRAYSIZE( s_QualityPresets );
static int s_DefaultQualityPreset = 1;

#else
static ReplayCodec_t s_Codecs[] = 
{
	{ VideoEncodeCodec::MJPEG_A_CODEC, "#Replay_Codec_MJPEGA" },
	{ VideoEncodeCodec::H264_CODEC, "#Replay_Codec_H264" },
};
static int s_nNumCodecs = ARRAYSIZE( s_Codecs );

//-----------------------------------------------------------------------------

static ReplayQualityPreset_t s_QualityPresets[] = 
{
	{ "#Replay_RenderSetting_Low", VideoEncodeCodec::MJPEG_A_CODEC, 0, false, 0 },
	{ "#Replay_RenderSetting_Medium", VideoEncodeCodec::MJPEG_A_CODEC, 50, false, 1 },
	{ "#Replay_RenderSetting_High", VideoEncodeCodec::MJPEG_A_CODEC, 100, true, 2 },
	{ "#Replay_RenderSetting_Max", VideoEncodeCodec::H264_CODEC, 100, true, 3 },
};
static int s_NumQualityPresets = ARRAYSIZE( s_QualityPresets );
static int s_DefaultQualityPreset = 1;

#endif


//-----------------------------------------------------------------------------

static const int s_QualityRange = 4;
static const int s_QualityInterval = 100 / s_QualityRange;

//-----------------------------------------------------------------------------

int ReplayVideo_GetVideoModeCount()
{
	return VIDEO_MODE_COUNT;
}

const ReplayVideoMode_t &ReplayVideo_GetVideoMode( int i )
{
	AssertMsg( i >= 0 && i < VIDEO_MODE_COUNT, "Replay video mode out of range!" );
	return s_VideoModes[ i ];
}

int ReplayVideo_GetDefaultQualityPreset()
{
	return s_DefaultQualityPreset;
}

int ReplayVideo_GetQualityInterval()
{
	return s_QualityInterval;
}

int ReplayVideo_GetQualityRange()
{
	return s_QualityRange;
}

int ReplayVideo_GetQualityPresetCount()
{
	return s_NumQualityPresets;
}

const ReplayQualityPreset_t &ReplayVideo_GetQualityPreset( int i )
{
	return s_QualityPresets[ i ];
}

int ReplayVideo_GetCodecCount()
{
	return s_nNumCodecs;
}

const ReplayCodec_t &ReplayVideo_GetCodec( int i )
{
	AssertMsg( i >= 0 && i < s_nNumCodecs, "Replay codec out of range!" );
	return s_Codecs[ i ];
}

int ReplayVideo_FindCodecPresetFromCodec( VideoEncodeCodec_t nCodecId )
{
	AssertMsg( nCodecId < VideoEncodeCodec::CODEC_COUNT, "Codec ID out of range!" );
	for ( int i = 0; i < VideoEncodeCodec::CODEC_COUNT; ++i )
	{
		if ( s_Codecs[ i ].m_nCodecId == nCodecId )
			return i;
	}
	
	AssertMsg( 0, "Codec not found!  This should never happen!" );

	return 0;
}

//-----------------------------------------------------------------------------

#endif
