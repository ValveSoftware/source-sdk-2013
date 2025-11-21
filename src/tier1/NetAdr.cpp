//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// NetAdr.cpp: implementation of the CNetAdr class.
//
//===========================================================================//

#if defined( _WIN32 ) && !defined( _X360 )
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include "tier0/dbg.h"
#include "netadr.h"
#include "tier1/strtools.h"

#if defined( _WIN32 ) && !defined( _X360 )
typedef int socklen_t;
#elif !defined( _X360 )
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool netadr_t::CompareAdr (const netadr_t &a, bool onlyBase) const
{
	if ( a.type != type )
		return false;

	if ( type == NA_LOOPBACK )
		return true;

	if ( type == NA_BROADCAST )
		return true;

	if ( type == NA_IP )
	{
		if ( !onlyBase && (port != a.port) )
			return false;

		if ( a.ip[0] == ip[0] && a.ip[1] == ip[1] && a.ip[2] == ip[2] && a.ip[3] == ip[3] )
			return true;
	}

	return false;
}

bool netadr_t::CompareClassBAdr (const netadr_t &a) const
{
	if ( a.type != type )
		return false;

	if ( type == NA_LOOPBACK )
		return true;

	if ( type == NA_IP )
	{
		if (a.ip[0] == ip[0] && a.ip[1] == ip[1] )
			return true;
	}

	return false;
}

bool netadr_t::CompareClassCAdr (const netadr_t &a) const
{
	if ( a.type != type )
		return false;

	if ( type == NA_LOOPBACK )
		return true;

	if ( type == NA_IP )
	{
		if (a.ip[0] == ip[0] && a.ip[1] == ip[1] && a.ip[2] == ip[2] )
			return true;
	}

	return false;
}
// reserved addresses are not routeable, so they can all be used in a LAN game
bool netadr_t::IsReservedAdr () const
{
	if ( type == NA_LOOPBACK )
		return true;

	if ( type == NA_IP )
	{
		if ( (ip[0] == 10) ||									// 10.x.x.x is reserved
			 (ip[0] == 127) ||									// 127.x.x.x 
			 (ip[0] == 172 && ip[1] >= 16 && ip[1] <= 31) ||	// 172.16.x.x  - 172.31.x.x 
			 (ip[0] == 192 && ip[1] >= 168) ) 					// 192.168.x.x
			return true;
	}
	return false;
}

const char * netadr_t::ToString( bool onlyBase ) const
{
	// Select a static buffer
	static	char	s[4][64];
	static int slot = 0;
	int useSlot = ( slot++ ) % 4;

	// Render into it
	ToString( s[useSlot], sizeof(s[0]), onlyBase );

	// Pray the caller uses it before it gets clobbered
	return s[useSlot];
}

void netadr_t::ToString( char *pchBuffer, uint32 unBufferSize, bool onlyBase ) const
{

	if (type == NA_LOOPBACK)
	{
		V_strncpy( pchBuffer, "loopback", unBufferSize );
	}
	else if (type == NA_BROADCAST)
	{
		V_strncpy( pchBuffer, "broadcast", unBufferSize );
	}
	else if (type == NA_IP)
	{
		if ( onlyBase )
		{
			V_snprintf( pchBuffer, unBufferSize, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3]);
		}
		else
		{
			V_snprintf( pchBuffer, unBufferSize, "%i.%i.%i.%i:%i", ip[0], ip[1], ip[2], ip[3], ntohs(port));
		}
	}
	else
	{
		V_strncpy( pchBuffer, "unknown", unBufferSize );
	}
}

bool netadr_t::IsLocalhost() const
{
	// are we 127.0.0.1 ?
	return (ip[0] == 127) && (ip[1] == 0) && (ip[2] == 0) && (ip[3] == 1);
}

bool netadr_t::IsLoopback() const
{
	// are we useding engine loopback buffers
	return type == NA_LOOPBACK;
}

void netadr_t::Clear()
{
	ip[0] = ip[1] = ip[2] = ip[3] = 0;
	port = 0;
	type = NA_NULL;
}

void netadr_t::SetIP(uint8 b1, uint8 b2, uint8 b3, uint8 b4)
{
	ip[0] = b1;
	ip[1] = b2;
	ip[2] = b3;
	ip[3] = b4;
}

void netadr_t::SetIP(uint unIP)
{
	*((uint*)ip) = BigLong( unIP );
}

void netadr_t::SetType(netadrtype_t newtype)
{
	type = newtype;
}

netadrtype_t netadr_t::GetType() const
{
	return type;
}

unsigned short netadr_t::GetPort() const
{
	return BigShort( port );
}

unsigned int netadr_t::GetIPNetworkByteOrder() const
{
	return *(unsigned int *)&ip;
}

unsigned int netadr_t::GetIPHostByteOrder() const
{
	return BigDWord( GetIPNetworkByteOrder() );
}

void netadr_t::ToSockadr (struct sockaddr * s) const
{
	Q_memset ( s, 0, sizeof(struct sockaddr));

	if (type == NA_BROADCAST)
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_port = port;
		((struct sockaddr_in*)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if (type == NA_IP)
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_addr.s_addr = *(int *)&ip;
		((struct sockaddr_in*)s)->sin_port = port;
	}
	else if (type == NA_LOOPBACK )
	{
		((struct sockaddr_in*)s)->sin_family = AF_INET;
		((struct sockaddr_in*)s)->sin_port = port;
		((struct sockaddr_in*)s)->sin_addr.s_addr = INADDR_LOOPBACK ;
	}
}

bool netadr_t::SetFromSockadr(const struct sockaddr * s)
{
	if (s->sa_family == AF_INET)
	{
		type = NA_IP;
		*(int *)&ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		port = ((struct sockaddr_in *)s)->sin_port;
		return true;
	}
	else
	{
		Clear();
		return false;
	}
}

bool netadr_t::IsValid() const
{
	return ( (port !=0 ) && (type != NA_NULL) &&
			 ( ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0 ) );
}

bool netadr_t::IsBaseAdrValid() const
{
	return ( (type != NA_NULL) &&
		( ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0 ) );
}

#ifdef _WIN32
#undef SetPort	// get around stupid WINSPOOL.H macro
#endif

void netadr_t::SetPort(unsigned short newport)
{
	port = BigShort( newport );
}

bool netadr_t::SetFromString( const char *pch, bool bUseDNS )
{
	Clear();
	if ( !pch || !pch[0] )
		return false;

	char szInput[256];
	V_strncpy( szInput, pch, sizeof(szInput) );
	V_StripTrailingWhitespaceASCII( szInput );

	if ( !V_strnicmp( szInput, "loopback", 8 ) )
	{
		type = NA_LOOPBACK;
		char szTemp[256];
		V_snprintf( szTemp, sizeof(szTemp), "127.0.0.1%s", szInput + 8 );
		V_strncpy( szInput, szTemp, sizeof(szInput) );
	}
	else if ( !V_strnicmp( szInput, "localhost", 9 ) )
	{
		type = NA_IP;
		V_memcpy( szInput, "127.0.0.1", 9 );
	}
	else
	{
		type = NA_IP;
	}

	char szHost[256];
	V_strncpy( szHost, szInput, sizeof(szHost) );
	unsigned short usPort = 0;

	char *pColon = strrchr( szHost, ':' );
	if ( pColon )
	{
		*pColon = '\0';
		int nPort = V_atoi( pColon + 1 );
		if ( nPort > 0 && nPort <= 65535 )
			usPort = (unsigned short)nPort;
		else
			return false;
	}

	bool bIsIPv4Literal = ( szHost[0] >= '0' && szHost[0] <= '9' && strchr( szHost, '.' ) );

	if ( bIsIPv4Literal )
	{
		int o1, o2, o3, o4;
		if ( sscanf( szHost, "%d.%d.%d.%d", &o1, &o2, &o3, &o4 ) == 4 )
		{
			if ( o1 >= 0 && o1 <= 255 &&
				 o2 >= 0 && o2 <= 255 &&
				 o3 >= 0 && o3 <= 255 &&
				 o4 >= 0 && o4 <= 255 )
			{
				SetIP( o1, o2, o3, o4 );
				SetPort( usPort );
				return true;
			}
		}
		return false;
	}

	if ( !bUseDNS )
		return false;

#if !defined( _X360 ) && !defined( _PS3 )
	struct addrinfo hints, *res = NULL, *p = NULL;
	Q_memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ( getaddrinfo( szHost, NULL, &hints, &res ) != 0 || !res )
		return false;

	bool bSuccess = false;
	for ( p = res; p != NULL; p = p->ai_next )
	{
		if ( p->ai_family == AF_INET )
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *)p->ai_addr;
			SetIP( ntohl( pAddr->sin_addr.s_addr ) );
			SetPort( usPort );
			bSuccess = true;
			break;
		}
		else if ( p->ai_family == AF_INET6 )
		{
			struct sockaddr_in6 *pAddr6 = (struct sockaddr_in6 *)p->ai_addr;
			unsigned char *b = pAddr6->sin6_addr.s6_addr;

			if ( b[0] == 0 && b[1] == 0 &&
				 b[2] == 0 && b[3] == 0 &&
				 b[4] == 0 && b[5] == 0 &&
				 b[6] == 0 && b[7] == 0 &&
				 b[8] == 0 && b[9] == 0 &&
				 b[10] == 0xFF && b[11] == 0xFF )
			{
				SetIP( b[12], b[13], b[14], b[15] );
				SetPort( usPort );
				bSuccess = true;
				break;
			}
		}
	}

	if ( res )
		freeaddrinfo( res );

	if ( bSuccess )
		return true;

	for ( p = res; p != NULL; p = p->ai_next )
	{
		if ( p->ai_family == AF_INET6 )
		{
			Warning( "DNS for '%s' returned IPv6, but Source Engine requires IPv4\n", szHost );
			break;
		}
	}

	return false;
#else
	return false;
#endif
}

bool netadr_t::operator<(const netadr_t &netadr) const
{
	if ( *((uint *)netadr.ip) < *((uint *)ip) )
		return true;
	else if ( *((uint *)netadr.ip) > *((uint *)ip) )
		return false;
	return ( netadr.port < port );
}


void netadr_t::SetFromSocket( int hSocket )
{	
	// dgoodenough - since this is skipped on X360, seems reasonable to skip as well on PS3
	// PS3_BUILDFIX
	// FIXME - Leap of faith, this works without asserting on X360, so I assume it will on PS3
#if !defined( _X360 ) && !defined( _PS3 )
	Clear();
	type = NA_IP;

	struct sockaddr address;
	socklen_t namelen = sizeof(address);
	if ( getsockname( hSocket, (struct sockaddr *)&address, &namelen) == 0 )
	{
		SetFromSockadr( &address );
	}
#else
	Assert(0);
#endif
}
