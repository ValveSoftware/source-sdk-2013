//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef SCREENSHOT_H
#define SCREENSHOT_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "replay/basereplayserializeable.h"
#include "mathlib/vector.h"
#include "qlimits.h"
#include "strtools.h"

//----------------------------------------------------------------------------------------

#define SUBDIR_SCREENSHOTS	"screenshots"

//----------------------------------------------------------------------------------------

class CReplayScreenshot : public CBaseReplaySerializeable
{
public:
	inline CReplayScreenshot( int nWidth = 0, int nHeight = 0, const char *pBaseFilename = NULL )
	:	m_nWidth( nWidth ), m_nHeight( nHeight )
	{
		if ( pBaseFilename )
		{
			V_strncpy( m_szBaseFilename, pBaseFilename, sizeof( m_szBaseFilename ) );
		}
	}

	virtual bool		Read( KeyValues *pIn );
	virtual void		Write( KeyValues *pOut );
	virtual const char	*GetSubKeyTitle() const;
	virtual const char	*GetPath() const;

	int		m_nWidth;				// Screenshot width (does not include power-of-2 padding)
	int		m_nHeight;				// Screenshot height (does not include power-of-2 padding)
	char	m_szBaseFilename[ MAX_OSPATH ];
};

//----------------------------------------------------------------------------------------

struct CaptureScreenshotParams_t	// To be passed from the client into IReplayHistoryManager::CaptureScreenshot()
{
	float		m_flDelay;			// Delay from now (in seconds) when we will take the screenshot
	int			m_nEntity;			// Should be 0 if no camera adjustment is needed, otherwise should be the index of the entity index from which m_posCamera will be based
	Vector		m_posCamera;		// Local position, relative to entity's index (if m_nEntity > 0) for camera position
	QAngle		m_angCamera;		// World angles of camera - used if m_bUseCameraAngles is true
	bool		m_bUseCameraAngles;	// Should we use m_angCamera - m_nEntity can't be 0
	bool		m_bIgnoreMinTimeBetweenScreenshots;	// Force screenshot, regardless of replay_mintimebetweenscreenshots?
	bool		m_bPrimary;			// Only set to true for the primary screenshot, which is taken when the user saves their replay
};

//----------------------------------------------------------------------------------------

struct WriteReplayScreenshotParams_t	// Passed from the engine into the client to take a screenshot
{
	const char	*m_pFilename;
	int			m_nWidth;
	int			m_nHeight;
	Vector		*m_pOrigin;		// Perspective origin from which to render.  Can be NULL
	QAngle		*m_pAngles;		// Perspective angles from which to render.  Can be NULL
};

//----------------------------------------------------------------------------------------

#endif // SCREENSHOT_H
