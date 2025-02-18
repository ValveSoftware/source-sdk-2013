//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IREPLAYMANAGER_H
#define IREPLAYMANAGER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/replayhandle.h"
#include "utllinkedlist.h"

//----------------------------------------------------------------------------------------

class CReplay;
class IQueryableReplayItem;

//----------------------------------------------------------------------------------------

class IReplayManager : public IBaseInterface
{
public:
	virtual CReplay	*GetReplay( ReplayHandle_t hReplay ) = 0;
	virtual CReplay	*GetPlayingReplay() = 0;
	virtual CReplay	*GetReplayForCurrentLife() = 0;
	virtual void	FlagReplayForFlush( CReplay *pReplay, bool bForceImmediate ) = 0;
	virtual void	DeleteReplay( ReplayHandle_t hReplay, bool bNotifyUI ) = 0;
	virtual int		GetReplayCount() const = 0;
	virtual int		GetUnrenderedReplayCount() = 0;	// Get the number of unrendered replays
	virtual void	GetReplays( CUtlLinkedList< CReplay *, int > &lstReplays ) = 0;
	virtual void	GetReplaysAsQueryableItems( CUtlLinkedList< IQueryableReplayItem *, int > &lstReplays ) = 0;
	virtual float	GetDownloadProgress( const CReplay *pReplay ) = 0;
	virtual const char	*GetReplaysDir() const = 0;
};

//----------------------------------------------------------------------------------------

#endif // IREPLAYMANAGER_H
