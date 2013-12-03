//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IRECORDINGSESSIONMANAGER_H
#define IRECORDINGSESSIONMANAGER_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

#include "interface.h"
#include "replay/replayhandle.h"

//----------------------------------------------------------------------------------------

class CBaseRecordingSession;

//----------------------------------------------------------------------------------------

class IRecordingSessionManager : public IBaseInterface
{
public:
	virtual CBaseRecordingSession	*FindSession( ReplayHandle_t hSession ) = 0;
	virtual const CBaseRecordingSession	*FindSession( ReplayHandle_t hSession ) const = 0;
	virtual void					FlagSessionForFlush( CBaseRecordingSession *pSession, bool bForceImmediate ) = 0;
	virtual int						GetServerStartTickForSession( ReplayHandle_t hSession ) = 0;
};

//----------------------------------------------------------------------------------------

#endif // IRECORDINGSESSIONMANAGER_H