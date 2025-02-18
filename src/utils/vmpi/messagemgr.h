//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MESSAGEMGR_H
#define MESSAGEMGR_H
#ifdef _WIN32
#pragma once
#endif


#define MSGMGR_VERSION						52314
#define MSGMGR_BROADCAST_PORT				22511

#define MSGMGR_PACKETID_MSG					0
#define MSGMGR_PACKETID_ANNOUNCE_PRESENCE	1	// followed by version # and port


// IMessageMgr provides a simple interface apps can use to generate output. Apps
// on the network can connect to the messagemgr to get its output and display it.
class IMessageMgr
{
public:
	virtual bool	Init() = 0;
	virtual void	Term() = 0;

	virtual void	Print( const char *pMsg ) = 0;
};


// Get the message manager. It's a global singleton so this will always
// return the same value (null if the manager can't initialize).
IMessageMgr* GetMessageMgr();


#endif // MESSAGEMGR_H
