//====== Copyright © 1996-2009, Valve Corporation, All rights reserved. =======
//
// Purpose: common definitions for post-processing effects
//
//=============================================================================

#ifndef POSTPROCESS_SHARED_H
#define POSTPROCESS_SHARED_H

#if defined( COMPILER_MSVC )
#pragma once
#endif

enum PostProcessParameterNames_t
{
	PPPN_FADE_TIME = 0,
	PPPN_LOCAL_CONTRAST_STRENGTH,
	PPPN_LOCAL_CONTRAST_EDGE_STRENGTH,
	PPPN_VIGNETTE_START,
	PPPN_VIGNETTE_END,
	PPPN_VIGNETTE_BLUR_STRENGTH,
	PPPN_FADE_TO_BLACK_STRENGTH,
	PPPN_DEPTH_BLUR_FOCAL_DISTANCE,
	PPPN_DEPTH_BLUR_STRENGTH,
	PPPN_SCREEN_BLUR_STRENGTH,
	PPPN_FILM_GRAIN_STRENGTH,

	POST_PROCESS_PARAMETER_COUNT
};

struct PostProcessParameters_t
{
	PostProcessParameters_t()
	{
		memset( m_flParameters, 0, sizeof( m_flParameters ) );
		m_flParameters[ PPPN_VIGNETTE_START ] = 0.8f;
		m_flParameters[ PPPN_VIGNETTE_END ] = 1.1f;
	}

	float m_flParameters[ POST_PROCESS_PARAMETER_COUNT ];

    bool operator !=(PostProcessParameters_t other)
    {
        for (int i = 0; i < POST_PROCESS_PARAMETER_COUNT; ++i)
        {
            if (m_flParameters[i] != other.m_flParameters[i])
                return true;
        }

        return false;
    }
};

#endif // POSTPROCESS_SHARED_H