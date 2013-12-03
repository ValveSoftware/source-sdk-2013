//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYRENDERQUEUE_H
#define IREPLAYRENDERQUEUE_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

abstract_class IReplayRenderQueue : IBaseInterface
{
public:
	virtual void	Add( ReplayHandle_t hReplay, int iPerformance ) = 0;
	virtual void	Remove( ReplayHandle_t hReplay, int iPerformance ) = 0;
	virtual void	Clear() = 0;

	virtual int		GetCount() const = 0;
	virtual bool	GetEntryData( int iIndex, ReplayHandle_t *pHandleOut, int *pPerformanceOut ) const = 0;
	virtual bool	IsInQueue( ReplayHandle_t hReplay, int iPerformance ) const = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYRENDERQUEUE_H