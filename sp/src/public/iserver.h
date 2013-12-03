//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ISERVER_H
#define ISERVER_H
#ifdef _WIN32
#pragma once
#endif

#include <inetmsghandler.h>
#include <bitvec.h>
#include <const.h>

class INetMessage;
class IRecipientFilter;
class IClient;

typedef struct player_info_s player_info_t;

abstract_class IServer : public IConnectionlessPacketHandler
{
public:
	virtual	~IServer() {}

	virtual int		GetNumClients( void ) const = 0; // returns current number of clients
	virtual int		GetNumProxies( void ) const = 0; // returns number of attached HLTV proxies
	virtual int		GetNumFakeClients() const = 0; // returns number of fake clients/bots
	virtual int		GetMaxClients( void ) const = 0; // returns current client limit
	virtual IClient	*GetClient( int index ) = 0; // returns interface to client 
	virtual int		GetClientCount() const = 0; // returns number of clients slots (used & unused)
	virtual int		GetUDPPort( void ) const = 0; // returns current used UDP port
	virtual float	GetTime( void ) const = 0;	// returns game world time
	virtual int		GetTick( void ) const = 0;	// returns game world tick
	virtual float	GetTickInterval( void ) const = 0; // tick interval in seconds
	virtual const char *GetName( void ) const = 0;	// public server name
	virtual const char *GetMapName( void ) const = 0; // current map name (BSP)
	virtual int		GetSpawnCount( void ) const = 0;	
	virtual int		GetNumClasses( void ) const = 0;
	virtual int		GetClassBits( void ) const = 0;
	virtual void	GetNetStats( float &avgIn, float &avgOut ) = 0; // total net in/out in bytes/sec
	virtual int		GetNumPlayers() = 0;
	virtual	bool	GetPlayerInfo( int nClientIndex, player_info_t *pinfo ) = 0;

	virtual bool	IsActive( void ) const = 0;	
	virtual bool	IsLoading( void ) const = 0;
	virtual bool	IsDedicated( void ) const = 0;
	virtual bool	IsPaused( void ) const = 0;
	virtual bool	IsMultiplayer( void ) const = 0;
	virtual bool	IsPausable() const = 0;
	virtual bool	IsHLTV() const = 0;
	virtual bool	IsReplay() const = 0;

	virtual const char * GetPassword() const = 0;	// returns the password or NULL if none set	

	virtual void	SetPaused(bool paused) = 0;
	virtual void	SetPassword(const char *password) = 0; // set password (NULL to disable)

	virtual void	BroadcastMessage( INetMessage &msg, bool onlyActive = false, bool reliable = false) = 0;
	virtual void	BroadcastMessage( INetMessage &msg, IRecipientFilter &filter ) = 0;

	virtual void	DisconnectClient( IClient *client, const char *reason ) = 0;
};


#endif // ISERVER_H
