//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYSCREENSHOTSYSTEM_H
#define IREPLAYSCREENSHOTSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

struct WriteReplayScreenshotParams_t;

//----------------------------------------------------------------------------------------

//
// Implementation lives in the client - allows replay to tell the client to grab a
// screenshot or update the cache.
//
class IReplayScreenshotSystem : public IBaseInterface
{
public:
	virtual void	WriteReplayScreenshot( WriteReplayScreenshotParams_t &params ) = 0;
	virtual void	UpdateReplayScreenshotCache() = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYSCREENSHOTSYSTEM_H
