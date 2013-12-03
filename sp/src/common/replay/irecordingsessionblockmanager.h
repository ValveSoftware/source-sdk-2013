//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IRECORDINGSESSIONBLOCKMANAGER_H
#define IRECORDINGSESSIONBLOCKMANAGER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

class IRecordingSessionBlockManager : public IBaseInterface
{
public:
	virtual CBaseRecordingSessionBlock	*GetBlock( ReplayHandle_t hBlock ) = 0;
	virtual void						DeleteBlock( CBaseRecordingSessionBlock *pBlock ) = 0;
	virtual void						UnloadBlock( CBaseRecordingSessionBlock *pBlock ) = 0;
	virtual const char					*GetBlockPath() const = 0;
	virtual void						LoadBlockFromFileName( const char *pFilename, IRecordingSession *pSession ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IRECORDINGSESSIONBLOCKMANAGER_H
