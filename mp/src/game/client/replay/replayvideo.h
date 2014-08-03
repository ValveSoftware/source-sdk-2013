//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYVIDEO_H
#define REPLAYVIDEO_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

#include "video/ivideoservices.h"

//-----------------------------------------------------------------------------

struct ReplayVideoMode_t
{
	int	m_nWidth;
	int m_nHeight;
	int m_nBaseFPS;
	bool m_bNTSCRate;
	const char *m_pName;	// Can be a localization token, e.g. "#Replay_Blah"
};

struct ReplayQualityPreset_t
{
	const char *m_pName;
	VideoEncodeCodec::EVideoEncodeCodec_t m_nCodecId;
	int m_iQuality;
	bool m_bMotionBlurEnabled;
	int m_iMotionBlurQuality;
};

struct ReplayCodec_t
{
	VideoEncodeCodec::EVideoEncodeCodec_t m_nCodecId;
	const char *m_pName;
};

//-----------------------------------------------------------------------------

int						ReplayVideo_GetVideoModeCount();
const ReplayVideoMode_t	&ReplayVideo_GetVideoMode( int i );

int						ReplayVideo_GetDefaultQualityPreset();
int						ReplayVideo_GetQualityInterval();	// TODO: Wtf is this?
int						ReplayVideo_GetQualityRange();
int						ReplayVideo_GetQualityPresetCount();
const ReplayQualityPreset_t &ReplayVideo_GetQualityPreset( int i );

int						ReplayVideo_GetCodecCount();
const ReplayCodec_t		&ReplayVideo_GetCodec( int i );

int ReplayVideo_FindCodecPresetFromCodec( VideoEncodeCodec_t nCodec );

//-----------------------------------------------------------------------------

#endif // REPLAYVIDEO_H
