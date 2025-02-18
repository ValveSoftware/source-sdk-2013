//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYSCREENSHOTMANAGER_H
#define IREPLAYSCREENSHOTMANAGER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

class CReplay;
struct CaptureScreenshotParams_t;

//----------------------------------------------------------------------------------------

class IReplayScreenshotManager : public IBaseInterface
{
public:
	virtual void		CaptureScreenshot( CaptureScreenshotParams_t& params ) = 0;
	virtual void		GetUnpaddedScreenshotSize( int &nOutWidth, int &nOutHeight ) = 0;
	virtual void		DeleteScreenshotsForReplay( CReplay *pReplay ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYSCREENSHOTMANAGER_H
