//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds the CEconGameAccountClient object
//
// $NoKeywords: $
//=============================================================================//

#ifndef ECON_GAME_ACCOUNT_CLIENT_H
#define ECON_GAME_ACCOUNT_CLIENT_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "base_gcmessages.pb.h"

//---------------------------------------------------------------------------------
// Purpose: All the account-level information that the GC tracks
//---------------------------------------------------------------------------------
class CEconGameAccountClient : public GCSDK::CProtoBufSharedObject< CSOEconGameAccountClient, k_EEconTypeGameAccountClient >
{
};

#endif //ECON_GAME_ACCOUNT_CLIENT_H
