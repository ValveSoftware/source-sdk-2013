//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Interface to a Steam Add On
//
//=============================================================================//

#ifndef ISTEAMADDON_H
#define ISTEAMADDON_H
#pragma once

#include "interface.h"
#include "AddOnTypes.h"
#include <vgui_controls/Panel.h>

class CUtlMsgBuffer;

class ISteamAddOn : public IBaseInterface
{
public:
	// allows SteamAddOn to link to the core vgui factories
	virtual bool Initialize(CreateInterfaceFn *vguiFactories, int factoryCount) = 0;

	// allows SteamAddOns to link to all other modules
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount) = 0;

	// when Friends closes down - all SteamAddOns are notified
	virtual void Deactivate() = 0;

	// notifies the addon who its VGUI parent panel is
	virtual void SetParent( vgui::Panel *parent ) = 0;

	// notifies the SteamAddOn of the user's ID and username. 
	// Note: username can be set mulitple times due to changing of name
	virtual void SetUserID(unsigned int userID) = 0;
	virtual void SetUserName(const char *userName) = 0;

	// Query if there are any 'open' sessions - open meaning allowing new users to join the sessions
	virtual int QueryOpenSessionCount() = 0;

	// will be valid right after a call to QueryOpenInviteCount will set the addOnSessionID and hostname for 
	// any open sessions for this addOn. Return true if it's a valid index
	virtual bool QueryOpenSessionInfo(int nOpenGameIndex, SessionInt64 &addOnSessionID, char *pszHostName) = 0;

	// returns true if this userID is involved in an addOnSession with this ID
	virtual bool QueryUserInvolved(SessionInt64 addOnSessionID, unsigned int userID) = 0;

	// if session doesn't exist, then the SteamAddOn body should deal with it
	virtual bool OnReceiveMsg(SessionInt64 addOnSessionID, CUtlMsgBuffer *msgBuffer) = 0;

	// Let's the SteamAddOn know when when any friend's status has changed
	virtual void OnFriendStatusChanged() = 0;

	// A request to start/join this AddOn with this user ID/name. addOnSessionID will be zero if it's a new session request
	virtual void OnInviteUser(unsigned int targetUserID, const char *username, SessionInt64 addOnSessionID) = 0;

	// user accepted this host's invite request
	virtual void OnAcceptInviteRequest(unsigned int hostID, const char *hostUserName, SessionInt64 addOnSessionID, const char *pAppData, int dataLen) = 0;

	// user accepted this host's rejoin request
	virtual void OnAcceptRejoinRequest(unsigned int hostID, const char *hostUserName, SessionInt64 addOnSessionID, const char *pAppData, int dataLen) = 0;
	
	// user starts this addOn from a menu
	virtual void StartAddOn() = 0;
};

#define STEAMADDON_INTERFACE_VERSION "SteamAddOn007"

#endif // ISTEAMADDON_H

