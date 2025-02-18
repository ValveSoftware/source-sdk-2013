//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "gcsdk/gcsdk_auto.h"
#include "tf_lobby_server.h"

using namespace GCSDK;

//-----------------------------------------------------------------------------
void CTFGSLobby::Dump() const
{
	CProtoBufSharedObjectBase::Dump( GSObj() );
}
