//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replay/iserverreplay.h"
#include "replay_gamestats_shared.h"

//----------------------------------------------------------------------------------------

class CServerReplayImp : public IServerReplay
{
public:
	virtual void UploadOgsData( KeyValues *pData, bool bIncludeTimeField )
	{
		GetReplayGameStatsHelper().UploadError( pData, bIncludeTimeField );
	}
};

static CServerReplayImp s_ServerReplayImp;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerReplayImp, IServerReplay, SERVER_REPLAY_INTERFACE_VERSION, s_ServerReplayImp );

#endif	// #if defined( REPLAY_ENABLED )