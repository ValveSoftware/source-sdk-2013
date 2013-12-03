//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IFRIENDSUSER_H
#define IFRIENDSUSER_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: Interface to accessing information about Friends Users
//-----------------------------------------------------------------------------
class IFriendsUser : public IBaseInterface
{
public:
	// returns true if the interface is ready for use
	virtual bool IsValid() = 0;

	// returns the Friends ID of the current user
	virtual unsigned int GetFriendsID() = 0;

	// returns information about a user
	// information may not be known about some users, "" will be returned
	virtual const char *GetUserName(unsigned int friendsID) = 0;
	virtual const char *GetFirstName(unsigned int friendsID) = 0;
	virtual const char *GetLastName(unsigned int friendsID) = 0;
	virtual const char *GetEmail(unsigned int friendsID) = 0;

	// returns true if buddyID is a buddy of the current user 
	// ie. the current is authorized to see when the buddy is online
	virtual bool IsBuddy(unsigned int buddyID) = 0;

	// requests authorization from a user
	virtual void RequestAuthorizationFromUser(unsigned int potentialBuddyID) = 0;

	// returns the status of the buddy, > 0 is online, 4 is ingame
	virtual int GetBuddyStatus(unsigned int friendsID) = 0;

	// gets the IP address of the server the buddy is on, returns false if couldn't get
	virtual bool GetBuddyGameAddress(unsigned int friendsID, int *ip, int *port) = 0;

	// returns the number of buddies
	virtual int GetNumberOfBuddies() = 0;

	// returns the FriendsID of a buddy - buddyIndex is valid in the range [0, GetNumberOfBuddies)
	virtual unsigned int GetBuddyFriendsID(int buddyIndex) = 0;

	// sets whether or not the user can receive messages at this time
	// messages will be queued until this is set to true
	virtual void SetCanReceiveMessages(bool state) = 0;
};

#define FRIENDSUSER_INTERFACE_VERSION "FriendsUser001"


#endif // IFRIENDSUSER_H
