//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// netadr.h
#ifndef NETADR_H
#define NETADR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#undef SetPort

typedef enum
{ 
	NA_NULL = 0,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
} netadrtype_t;

typedef struct netadr_s
{
public:
	netadr_s() { SetIP( 0 ); SetPort( 0 ); SetType( NA_IP ); }
	netadr_s( uint unIP, uint16 usPort ) { SetIP( unIP ); SetPort( usPort ); SetType( NA_IP ); }
	netadr_s( const char *pch ) { SetFromString( pch ); }
	void	Clear();	// invalids Address

	void	SetType( netadrtype_t type );
	void	SetPort( unsigned short port );
	bool	SetFromSockadr(const struct sockaddr *s);
	void	SetIP(uint8 b1, uint8 b2, uint8 b3, uint8 b4);
	void	SetIP(uint unIP);									// Sets IP.  unIP is in host order (little-endian)
	void    SetIPAndPort( uint unIP, unsigned short usPort ) { SetIP( unIP ); SetPort( usPort ); }
	void	SetFromString(const char *pch, bool bUseDNS = false ); // if bUseDNS is true then do a DNS lookup if needed
	
	bool	CompareAdr (const netadr_s &a, bool onlyBase = false) const;
	bool	CompareClassBAdr (const netadr_s &a) const;
	bool	CompareClassCAdr (const netadr_s &a) const;

	netadrtype_t	GetType() const;
	unsigned short	GetPort() const;
	const char*		ToString( bool onlyBase = false ) const; // returns xxx.xxx.xxx.xxx:ppppp
	void			ToSockadr(struct sockaddr *s) const;
	unsigned int	GetIPHostByteOrder() const;
	unsigned int	GetIPNetworkByteOrder() const;

	bool	IsLocalhost() const; // true, if this is the localhost IP 
	bool	IsLoopback() const;	// true if engine loopback buffers are used
	bool	IsReservedAdr() const; // true, if this is a private LAN IP
	bool	IsValid() const;	// ip & port != 0
	void    SetFromSocket( int hSocket );
	bool operator==(const netadr_s &netadr) const {return ( CompareAdr( netadr ) );}
	bool operator<(const netadr_s &netadr) const;

public:	// members are public to avoid to much changes

	netadrtype_t	type;
	unsigned char	ip[4];
	unsigned short	port;
} netadr_t;

#endif // NETADR_H
