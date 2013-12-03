//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef RENDERMOVIEPARAMS_H
#define RENDERMOVIEPARAMS_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "tier1/utlstring.h"
#include "tier1/strtools.h"
#include "replay/replayhandle.h"
#include "replay/shared_defs.h"
#include "video/ivideoservices.h"

//----------------------------------------------------------------------------------------

typedef unsigned int MovieHandle_t;

struct RenderMovieParams_t
{
	inline RenderMovieParams_t() : m_iPerformance( -1 ) { V_memset( this, 0, sizeof( RenderMovieParams_t ) ); m_Settings.m_FPS.SetFPS( 0, false ); }

	ReplayHandle_t		m_hReplay;
	int					m_iPerformance;		// -1 for default view, otherwise this is an index into the replay's m_vecPerformances vector.
	wchar_t				m_wszTitle[MAX_REPLAY_TITLE_LENGTH];
	char				m_szVideoPreset[64];
	char				m_szExtension[16];	// File extension
	bool				m_bQuitWhenFinished;
	bool				m_bExportRaw;		// Export movie as raw TGA frames and a .WAV
	float				m_flEngineFps;

	struct ReplayRenderSettings_t
	{
		uint16				m_nWidth;
		uint16				m_nHeight;
		int8				m_nMotionBlurQuality;	// [0,MAX_MOTION_BLUR_QUALITY]
		VideoFrameRate_t	m_FPS;					// Actual framerate can be calculated with m_FPS.GetFps()
		VideoEncodeCodec_t	m_Codec;
		bool				m_bMotionBlurEnabled;	// Motion blur enabled?
		bool				m_bAAEnabled;			// Antialiasing enabled?
		int8				m_nEncodingQuality;		// [0,100]
		bool				m_bRaw;					// This movie was exported as raw TGA frames and a .WAV file?
	}
	m_Settings;
};

typedef RenderMovieParams_t::ReplayRenderSettings_t ReplayRenderSettings_t;

//----------------------------------------------------------------------------------------

#define MAX_DOF_QUALITY			2
#define MAX_MOTION_BLUR_QUALITY 3
#define SUBPIXEL_JITTER_SAMPLES	16
#define CHEAP_DOF_SAMPLES		4

//----------------------------------------------------------------------------------------

#endif // RENDERMOVIEPARAMS_H
