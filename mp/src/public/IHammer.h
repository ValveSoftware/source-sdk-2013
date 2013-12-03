//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The application object.
//
//=============================================================================//

#ifndef IHAMMER_H
#define IHAMMER_H

#include "appframework/IAppSystem.h"

typedef struct tagMSG MSG;


class IStudioDataCache;


//-----------------------------------------------------------------------------
// Return values for RequestNewConfig
//-----------------------------------------------------------------------------
enum RequestRetval_t
{
	REQUEST_OK = 0,
	REQUEST_QUIT
};


//-----------------------------------------------------------------------------
// Interface used to drive hammer
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_HAMMER	"Hammer001"
class IHammer : public IAppSystem
{
public:
	virtual bool HammerPreTranslateMessage( MSG * pMsg ) = 0;
	virtual bool HammerIsIdleMessage( MSG * pMsg ) = 0;
	virtual bool HammerOnIdle( long count ) = 0;

	virtual void RunFrame() = 0;

	// Returns the mod and the game to initially start up
	virtual const char *GetDefaultMod() = 0;
	virtual const char *GetDefaultGame() = 0;

	virtual bool InitSessionGameConfig( const char *szGameDir ) = 0;

	// Request a new config from hammer's config system
	virtual RequestRetval_t RequestNewConfig() = 0;

	// Returns the full path to the mod and the game to initially start up
	virtual const char *GetDefaultModFullPath() = 0;

	virtual int MainLoop() = 0;
};	

#endif // IHAMMER_H
