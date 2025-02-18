//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTFGSLobby object for server
//
//=============================================================================

#ifndef TF_LOBBY_SERVER_H
#define TF_LOBBY_SERVER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.h"
#include "gcsdk/protobufsharedobject.h"
#include "tf_lobby_shared.h"

// CTFLobby supercedes this, this just provides CTFLobbyShared + SharedObject (whereas the GS Lobby is nested within the
// greater proto object on the GC -- see CTFGSLobby_Dummy)

class CTFGSLobby : public CTFLobbyShared, public GCSDK::CProtoBufSharedObject<CSOTFGameServerLobby, k_EProtoObjectTFGameServerLobby>
{
	typedef GCSDK::CProtoBufSharedObject<CSOTFGameServerLobby, k_EProtoObjectTFGameServerLobby> BaseClass;
public:
	virtual ~CTFGSLobby() {}

	virtual void Dump() const OVERRIDE;

	virtual CSharedObject* GetSharedObjectForMember( const CSteamID & ) OVERRIDE { return this; }

private:
	virtual const CSOTFGameServerLobby &GSObj() const OVERRIDE final { return Obj(); }
	virtual CSOTFGameServerLobby &GSObj() OVERRIDE final { return Obj(); }
};

#endif // TF_LOBBY_SERVER_H
