//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// netadr.h
#ifndef NS_ADDRESS_H
#define NS_ADDRESS_H
#pragma once

#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier1/netadr.h"
#include "steam/steamclientpublic.h" // for CSteamID
#include "tier1/strtools.h" // V_memset

#if defined( NO_STEAM )
typedef CSteamID uint64;
#endif

enum PeerToPeerAddressType_t
{
	P2P_STEAMID,
};

const int STEAM_P2P_CHANNEL_DEFAULT = 0;
const int STEAM_P2P_CHANNEL_HLTV = 3;
	
class CPeerToPeerAddress
{
public:
	CPeerToPeerAddress ( void )
	: m_AddrType( P2P_STEAMID )
	, m_steamChannel( STEAM_P2P_CHANNEL_DEFAULT )
	{}

	void Clear ( void )
	{
		m_AddrType = P2P_STEAMID;
		m_steamID.Clear();
		m_steamChannel = STEAM_P2P_CHANNEL_DEFAULT;
	}

	void SetSteamChannel( int nChannel )
	{
		m_steamChannel = nChannel;
	}

	int GetSteamChannel() const
	{
		return m_steamChannel;
	}
	
	const CSteamID &GetSteamID() const
	{
		return m_steamID;
	}

	CPeerToPeerAddress ( const CSteamID &steamID, int nSteamChannel )
		: m_AddrType ( P2P_STEAMID )
		, m_steamID ( steamID )
		, m_steamChannel( nSteamChannel )
	{
	}

private:
	// disallow since we can't deal with steamchannel
	CPeerToPeerAddress &operator=(const CSteamID &steamID);
public:

	// Like operator =
	CPeerToPeerAddress &SetFromSteamID( const CSteamID &steamID, int nSteamChannel )
	{
		m_AddrType = P2P_STEAMID;
		m_steamID = steamID;
		m_steamChannel = nSteamChannel;
		return *this;
	}

	bool IsValid ( void ) const
	{
		switch ( m_AddrType )
		{
		case P2P_STEAMID:
			return m_steamID.IsValid ( );
		};

		return false;
	}

	const char *ToString ( void ) const
	{
		switch ( m_AddrType )
		{
		case P2P_STEAMID:
			return m_steamID.Render ( );
		};

		return "";
	}

	bool CompareAdr ( const CPeerToPeerAddress &other, bool onlyBase = false ) const
	{
		if ( m_AddrType != -other.m_AddrType )
			return false;

		switch ( m_AddrType )
		{
		case P2P_STEAMID:
			return ((m_steamID == other.m_steamID) && (m_steamChannel == other.m_steamChannel));
		};

		return false;
	}

	bool operator==(const CPeerToPeerAddress &rhs) const
	{
		return CompareAdr( rhs, false );
	}

	PeerToPeerAddressType_t GetAddressType ( void ) const
	{
		return m_AddrType;
	}


	template <typename T> bool IsType ( void ) const
	{
		return false;
	}

	template <typename T> T *Get ( void ) { return NULL; }
	template <typename T> const T *Get ( void ) const { return NULL; }

	template <typename T> T &AsType ( void ) { static T dummy; return dummy; }
	template <typename T> const T &AsType ( void ) const { static T dummy; return dummy; }

private:
	CSteamID m_steamID;
	int m_steamChannel; // SteamID channel (like a port number to disambiguate multiple connections)

	PeerToPeerAddressType_t m_AddrType;
};

template <>
inline bool CPeerToPeerAddress::IsType<CSteamID> ( void ) const
{
	return (m_AddrType == P2P_STEAMID);
}

template <>
inline CSteamID * CPeerToPeerAddress::Get<CSteamID> ( void )
{
	return IsType<CSteamID> ( ) ? &m_steamID : NULL;
}

template <>
inline const CSteamID * CPeerToPeerAddress::Get<CSteamID> ( void ) const
{
	return IsType<CSteamID> ( ) ? &m_steamID : NULL;
}

template <>
inline CSteamID &CPeerToPeerAddress::AsType<CSteamID> ( void )
{
	Assert ( IsType<CSteamID> ( ) );
	return m_steamID;
}

template <>
inline const CSteamID &CPeerToPeerAddress::AsType<CSteamID> ( void ) const
{
	Assert ( IsType<CSteamID> ( ) );
	return m_steamID;
}

enum NetworkSystemAddressType_t
{
	NSAT_NETADR,
	NSAT_P2P,
	NSAT_PROXIED_GAMESERVER,	// Client proxied through Steam Datagram Transport
	NSAT_PROXIED_CLIENT,		// Client proxied through Steam Datagram Transport
};

// Used to represent a remote address which can be a network address or SteamID
// Basically if m_steamID.IsValid() then we send to that otherwise we use the m_adr
struct ns_address
{
	netadr_t m_adr; // ip:port and network type (NULL/IP/BROADCAST/etc).
	CPeerToPeerAddress m_steamID; // SteamID destination
	NetworkSystemAddressType_t m_AddrType;

	ns_address() : m_AddrType( NSAT_NETADR ){}

	~ns_address()
	{
		Clear();
	}

	void Clear ( )
	{
		m_AddrType = NSAT_NETADR;
		m_adr.Clear ( );
		m_steamID.Clear ( );
	}

	ns_address ( const netadr_t &other )
		: m_AddrType ( NSAT_NETADR )
		, m_adr ( other )
	{
		m_steamID.Clear();
	}

	ns_address& operator=(const netadr_t &other)
	{
		Clear ( );
		m_AddrType = NSAT_NETADR;
		m_adr = other;
		return *this;
	}


	ns_address ( const CPeerToPeerAddress &steamID )
		: m_AddrType ( NSAT_P2P )
		, m_steamID ( steamID )
	{
		m_adr.Clear ( );
	}

	ns_address& operator=(const CPeerToPeerAddress &steamID)
	{
		Clear ( );
		m_AddrType = NSAT_P2P;
		m_steamID = steamID;
		return *this;
	}

	ns_address &SetFromSteamID ( const CSteamID &steamID, int nSteamChannel )
	{
		Clear ( );
		m_AddrType = NSAT_P2P;
		m_steamID.SetFromSteamID( steamID, nSteamChannel );
		return *this;
	}


	bool IsLoopback ( void ) const
	{
		return (m_AddrType == NSAT_NETADR) && m_adr.IsLoopback ( );
	}

	bool IsLocalhost ( void ) const
	{
		return (m_AddrType == NSAT_NETADR) && m_adr.IsLocalhost ( );
	}

	bool IsBroadcast( void ) const
	{
		return (m_AddrType == NSAT_NETADR) && m_adr.GetType() == NA_BROADCAST;
	}

	bool IsReservedAdr( void ) const
	{
		return (m_AddrType == NSAT_NETADR) && m_adr.IsReservedAdr();
	}

	bool IsNull ( void ) const
	{
		switch ( m_AddrType )
		{
		case NSAT_NETADR:
			return m_adr.GetType ( ) == NA_NULL;
		case NSAT_P2P:
		case NSAT_PROXIED_GAMESERVER:
		case NSAT_PROXIED_CLIENT:
			return !m_steamID.IsValid ( );
		};

		return true;
	}

	bool IsValid ( ) const
	{
		switch ( m_AddrType )
		{
		case NSAT_NETADR:
			return m_adr.IsValid ( );
		case NSAT_P2P:
		case NSAT_PROXIED_GAMESERVER:
		case NSAT_PROXIED_CLIENT:
			return m_steamID.IsValid ( );
		};

		return false;
	}

	bool CompareAdr ( const ns_address &other, bool onlyBase = false ) const
	{
		if ( m_AddrType != other.m_AddrType )
			return false;

		switch ( m_AddrType )
		{
		case NSAT_NETADR:
			return m_adr.CompareAdr ( other.m_adr, onlyBase );

		case NSAT_P2P:
		case NSAT_PROXIED_GAMESERVER:
		case NSAT_PROXIED_CLIENT:
			return m_steamID.CompareAdr( other.m_steamID, onlyBase );
		};

		return false;
	}

	bool operator==( const ns_address &rhs ) const
	{
		return CompareAdr( rhs, false );
	}

	bool operator!=( const ns_address &rhs ) const
	{
		return !CompareAdr( rhs, false );
	}

	NetworkSystemAddressType_t GetAddressType ( void ) const
	{
		return m_AddrType;
	}

	void SetAddrType( NetworkSystemAddressType_t type )
	{
		Clear();
		m_AddrType = type;
	}

	bool SetFromSockadr ( const struct sockaddr * s )
	{
		m_AddrType = NSAT_NETADR;
		m_steamID.Clear ( );
		return m_adr.SetFromSockadr ( s );
	}

	unsigned int GetIP ( void ) const
	{
		return (m_AddrType == NSAT_NETADR) ? m_adr.GetIPHostByteOrder( ) : 0;
	}

	unsigned short GetPort ( void ) const
	{
		return (m_AddrType == NSAT_NETADR) ? m_adr.GetPort ( ) : 0;
	}

	template <typename T> bool IsType ( void ) const
	{
		//defer to subtype
		switch ( m_AddrType )
		{
		case NSAT_P2P:
			return m_steamID.IsType<T> ( );
		};
		return false;
	}

	template <typename T> T *Get ( void )
	{
		//defer to subtype
		switch ( m_AddrType )
		{
		case NSAT_P2P:
			return m_steamID.Get<T> ( );
		};
		return NULL;
	}

	template <typename T> const T *Get ( void ) const
	{
		//defer to subtype
		switch ( m_AddrType )
		{
		case NSAT_P2P:
			return m_steamID.Get<T> ( );
		};
		return NULL;
	}

	template <typename T> T &AsType ( void )
	{
		//defer to subtype
		switch ( m_AddrType )
		{
		case NSAT_P2P:
			return m_steamID.AsType<T> ( );
		};
		Assert ( false );
		static T dummy;
		return dummy;
	}

	template <typename T> const T &AsType ( void ) const
	{
		//defer to subtype
		switch ( m_AddrType )
		{
		case NSAT_P2P:
			return m_steamID.AsType<T> ( );
		};
		Assert ( false );
		static T dummy;
		return dummy;
	}

	bool SetFromString( const char *s )
	{
		Clear();
		if ( !s )
			return false;
		bool bProxied = false;
		if ( *s == '=' )
		{
			++s;
			bProxied = true;
		}
		if ( *s == '[' )
		{
			CSteamID tempSteamID( s );
			if ( !tempSteamID.IsValid() )
				return false;

			if ( !bProxied )
				m_AddrType = NSAT_P2P;
			else if ( tempSteamID.BGameServerAccount() )
				m_AddrType = NSAT_PROXIED_GAMESERVER;
			else
				m_AddrType = NSAT_PROXIED_CLIENT;
			m_steamID.SetFromSteamID( tempSteamID, STEAM_P2P_CHANNEL_DEFAULT );
			s = strchr( s, ']' );
			int nChannel = -1;
			if ( s && s[1] == ':' && sscanf( s+2, "%d", &nChannel ) == 1 && nChannel >= 0 )
			{
				m_steamID.SetSteamChannel( nChannel );
			}
			return true;
		}

		if ( !bProxied && m_adr.SetFromString( s, true ) )
		{
			m_AddrType = NSAT_NETADR;
			return true;
		}

		return false;
	}


};

template <>
inline bool ns_address::IsType<netadr_t> ( void ) const
{
	return (m_AddrType == NSAT_NETADR);
}

template <>
inline bool ns_address::IsType<CPeerToPeerAddress> ( void ) const
{
	return (m_AddrType == NSAT_P2P);
}

template <>
inline netadr_t *ns_address::Get<netadr_t> ( void )
{
	return IsType<netadr_t> ( ) ? &m_adr : NULL;
}

template <>
inline const netadr_t *ns_address::Get<netadr_t> ( void ) const
{
	return IsType<netadr_t> ( ) ? &m_adr : NULL;
}

template <>
inline CPeerToPeerAddress *ns_address::Get<CPeerToPeerAddress> ( void )
{
	return IsType<CPeerToPeerAddress> ( ) ? &m_steamID : NULL;
}

template <>
inline const CPeerToPeerAddress *ns_address::Get<CPeerToPeerAddress> ( void ) const
{
	return IsType<CPeerToPeerAddress> ( ) ? &m_steamID : NULL;
}

template <>
inline netadr_t &ns_address::AsType<netadr_t> ( void )
{
	Assert ( IsType<netadr_t> ( ) );
	return m_adr;
}

template <>
inline const netadr_t &ns_address::AsType<netadr_t> ( void ) const
{
	Assert ( IsType<netadr_t> ( ) );
	return m_adr;
}

template <>
inline CPeerToPeerAddress &ns_address::AsType<CPeerToPeerAddress> ( void )
{
	Assert ( IsType<CPeerToPeerAddress> ( ) );
	return m_steamID;
}

template <>
inline const CPeerToPeerAddress &ns_address::AsType<CPeerToPeerAddress> ( void ) const
{
	Assert ( IsType<CPeerToPeerAddress> ( ) );
	return m_steamID;
}

/// Utility class used to render an address
class ns_address_render
{
	char m_buf[64];
public:
	ns_address_render( const ns_address &a )
	{
		switch ( a.m_AddrType )
		{
		case NSAT_NETADR:
			a.m_adr.ToString( m_buf, sizeof( m_buf ) );
			break;

		case NSAT_P2P:
			V_sprintf_safe( m_buf, "%s:%d", a.m_steamID.ToString(), a.m_steamID.GetSteamChannel() );
			break;

		case NSAT_PROXIED_CLIENT:
		case NSAT_PROXIED_GAMESERVER:
			V_sprintf_safe( m_buf, "=%s:%d", a.m_steamID.ToString(), a.m_steamID.GetSteamChannel() );
			break;

		default:
			m_buf[0] = '\0';
		};
	}
	const char *String() const { return m_buf; }
};

#endif // NS_ADDRESS_H
