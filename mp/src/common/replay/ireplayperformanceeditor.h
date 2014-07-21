//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYPERFORMANCEEDITOR_H
#define IREPLAYPERFORMANCEEDITOR_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"

//----------------------------------------------------------------------------------------

class CReplay;

//----------------------------------------------------------------------------------------

//
// Interface to allow the replay DLL to talk to the actual UI.
//
class IReplayPerformanceEditor : public IBaseInterface
{
public:
	virtual CReplay *GetReplay() = 0;
	virtual void	OnRewindComplete() = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYPERFORMANCEEDITOR_H
