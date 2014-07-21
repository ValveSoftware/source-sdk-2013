//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef INETWORK_H
#define INETWORK_H
#ifdef _WIN32
#pragma once
#endif

class IConnectionlessPacketHandler;

abstract_class INetwork
{
public:
	virtual	~INetwork( void ) {};

	virtual void Init( void ) = 0;
	virtual void Config (bool multiplayer);	
	virtual void IsMultiplayer( void ) = 0; // true = full MP mode, false = loopback SP mode
	virtual void IsEnabled( void ) = 0;	

	// shuts down Network, closes all UPD/TCP channels
	virtual void Shutdown( void ) = 0;

	// must be called each system frame to do any asynchronouse TCP stuff 
	virtual void RunFrame( double time ) = 0;

	virtual void ProcessSocket( netsrc_t sock, IConnectionlessPacketHandler * handler ) = 0;

	virtual void OutOfBandPrintf(netsrc_t sock, netadr_t &adr, PRINTF_FORMAT_STRING const char *format, ...) = 0;
	virtual void SendConnectionless(netsrc_t sock, netadr_t &adr, unsigned char * data, int length ) = 0;

	virtual void LogBadPacket(netpacket_t * packet) = 0;
	
	// Address conversion
	virtual bool StringToAdr ( const char *s, netadr_t *a) = 0;

	// Convert from host to network byte ordering
	virtual unsigned short HostToNetShort( unsigned short us_in );

	// and vice versa
	virtual unsigned short NetToHostShort( unsigned short us_in );
	

	
};


#endif // INETWORK_H
