//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERBROWSER_H
#define ISERVERBROWSER_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: Interface to server browser module
//-----------------------------------------------------------------------------
abstract_class IServerBrowser
{
public:
	// activates the server browser window, brings it to the foreground
	virtual bool Activate() = 0;

	// joins a game directly
	virtual bool JoinGame( uint32 unGameIP, uint16 usGamePort, const char *pszConnectCode ) = 0;

	// joins a specified game - game info dialog will only be opened if the server is fully or passworded
	virtual bool JoinGame( uint64 ulSteamIDFriend, const char *pszConnectCode ) = 0;

	// opens a game info dialog to watch the specified server; associated with the friend 'userName'
	virtual bool OpenGameInfoDialog( uint64 ulSteamIDFriend, const char *pszConnectCode ) = 0;

	// forces the game info dialog closed
	virtual void CloseGameInfoDialog( uint64 ulSteamIDFriend ) = 0;

	// closes all the game info dialogs
	virtual void CloseAllGameInfoDialogs() = 0;

	/// Given a map name, strips off some stuff and returns the "friendly" name of the map.
	/// Returns the cleaned out map name into the caller's buffer, and returns the friendly
	/// game type name.
	virtual const char *GetMapFriendlyNameAndGameType( const char *pszMapName, char *szFriendlyMapName, int cchFriendlyName ) = 0;

	// Enable filtering of workshop maps, requires the game/tool loading us to feed subscription data. This is a
	// slightly ugly workaround to TF2 not yet having native workshop UI in quickplay, once that is in place this should
	// either be stripped back out or expanded to be directly aware of the steam workshop without being managed.
	virtual void SetWorkshopEnabled( bool bManaged ) = 0;
	virtual void AddWorkshopSubscribedMap( const char *pszMapName ) = 0;
	virtual void RemoveWorkshopSubscribedMap( const char *pszMapName ) = 0;
};

#define SERVERBROWSER_INTERFACE_VERSION "ServerBrowser005"



#endif // ISERVERBROWSER_H
