//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  GC based lobby.  Matchmaking assigns players to a lobby
//
//=============================================================================

#ifndef LOBBY_H
#define LOBBY_H
#ifdef _WIN32
#pragma once
#endif

#include "playergroup.h"

namespace GCSDK
{
class CSharedObject;

class ILobby : public IPlayerGroup
{
public:
	virtual ~ILobby() { }

};

}

#endif
