//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FRIENDSNET_INTERFACE_H
#define FRIENDSNET_INTERFACE_H
#pragma once

class CUtlMsgBuffer;
class CServerSession;

#include "interface.h"
#include "Friends/AddOns/AddOnTypes.h"

class IFriendsNET : public IBaseInterface
{
public:
	// check if we have network information for this user
    virtual bool CheckUserRegistered(unsigned int userID) = 0;

	// update a user's network information
    virtual void UpdateUserNetInfo(unsigned int userID, unsigned int netSessionID, int serverID, int IP, int port) = 0;

	// set whether or not we send directly to user or through the server
    virtual void SetUserSendViaServer(unsigned int userID, bool bSendViaServer) = 0;

	// Gets a blob of data that represents this user's information
    virtual bool GetUserNetInfoBlob(unsigned int userID, unsigned int dataBlob[8]) = 0;
	// Sets a user's information using the same blob of data type
    virtual bool SetUserNetInfoBlob(unsigned int userID, const unsigned int dataBlob[8]) = 0;

    // send binary data to user, marked with game/sessionID
    virtual void SendAddOnPacket(const char *pszGameID, SessionInt64 addOnSessionID, unsigned int userID, const CUtlMsgBuffer& buffer) = 0;
};

#define FRIENDSNET_INTERFACE_VERSION "FriendsNET003"

#endif // FRIENDSNET_INTERFACE_H

