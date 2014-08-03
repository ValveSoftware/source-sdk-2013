//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "../EventLog.h"
#include "KeyValues.h"

class CSDKEventLog : public CEventLog
{
private:
	typedef CEventLog BaseClass;

public:
	virtual ~CSDKEventLog() {};

public:
	bool PrintEvent( IGameEvent * event )	// override virtual function
	{
		if ( BaseClass::PrintEvent( event ) )
		{
			return true;
		}
	
		if ( Q_strcmp(event->GetName(), "sdk_") == 0 )
		{
			return PrintSDKEvent( event );
		}

		return false;
	}

protected:

	bool PrintSDKEvent( IGameEvent * event )	// print Mod specific logs
	{
		//const char * name = event->GetName() + Q_strlen("sdk_"); // remove prefix
		return false;
	}

};

CSDKEventLog g_SDKEventLog;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* GameLogSystem()
{
	return &g_SDKEventLog;
}

