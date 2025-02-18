//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#pragma warning (disable:4127)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning (default:4127)

#include "iphelpers.h"
#include "basetypes.h"
#include <assert.h>
#include "utllinkedlist.h"
#include "utlvector.h"
#include "tier1/strtools.h"
#include "tier0/fasttimer.h"

// This automatically calls WSAStartup for the app at startup.
class CIPStarter
{
public:
	CIPStarter()
	{
		WSADATA wsaData;
		WSAStartup( WINSOCK_VERSION, &wsaData );
	}
};
static CIPStarter g_Starter;


unsigned long SampleMilliseconds()
{
	CCycleCount cnt;
	cnt.Sample();
	return cnt.GetMilliseconds();
}


// ------------------------------------------------------------------------------------------ //
// CChunkWalker.
// ------------------------------------------------------------------------------------------ //

CChunkWalker::CChunkWalker( void const * const *pChunks, const int *pChunkLengths, int nChunks )
{
	m_TotalLength = 0;
	for ( int i=0; i < nChunks; i++ )
		m_TotalLength += pChunkLengths[i];
	
	m_iCurChunk = 0;
	m_iCurChunkPos = 0;
	m_pChunks = pChunks;
	m_pChunkLengths = pChunkLengths;
	m_nChunks = nChunks;
}

int CChunkWalker::GetTotalLength() const
{
	return m_TotalLength;
}

void CChunkWalker::CopyTo( void *pOut, int nBytes )
{
	unsigned char *pOutPos = (unsigned char*)pOut;

	int nBytesLeft = nBytes;
	while ( nBytesLeft > 0 )
	{
		int toCopy = nBytesLeft;
		int curChunkLen = m_pChunkLengths[m_iCurChunk];
		
		int amtLeft = curChunkLen - m_iCurChunkPos;
		if ( nBytesLeft > amtLeft )
		{
			toCopy = amtLeft;
		}

		unsigned char *pCurChunkData = (unsigned char*)m_pChunks[m_iCurChunk];
		memcpy( pOutPos, &pCurChunkData[m_iCurChunkPos], toCopy );
		nBytesLeft -= toCopy;
		pOutPos += toCopy;

		// Slide up to the next chunk if we're done with the one we're on.
		m_iCurChunkPos += toCopy;
		assert( m_iCurChunkPos <= curChunkLen );
		if ( m_iCurChunkPos == curChunkLen )
		{
			++m_iCurChunk;
			m_iCurChunkPos = 0;
			if ( m_iCurChunk == m_nChunks )
			{
				assert( nBytesLeft == 0 );
			}
		}
	}
}


// ------------------------------------------------------------------------------------------ //
// CWaitTimer
// ------------------------------------------------------------------------------------------ //

bool g_bForceWaitTimers = false;

CWaitTimer::CWaitTimer( double flSeconds )
{
	m_StartTime = SampleMilliseconds();
	m_WaitMS = (unsigned long)( flSeconds * 1000.0 );
}


bool CWaitTimer::ShouldKeepWaiting()
{
	if ( m_WaitMS == 0 )
	{
		return false;
	}
	else
	{
		return ( SampleMilliseconds() - m_StartTime ) <= m_WaitMS || g_bForceWaitTimers;
	}
}



// ------------------------------------------------------------------------------------------ //
// CIPAddr.
// ------------------------------------------------------------------------------------------ //

CIPAddr::CIPAddr()
{
	Init( 0, 0, 0, 0, 0 ); 
}


CIPAddr::CIPAddr( const int inputIP[4], const int inputPort )
{
	Init( inputIP[0], inputIP[1], inputIP[2], inputIP[3], inputPort ); 
}


CIPAddr::CIPAddr( int ip0, int ip1, int ip2, int ip3, int ipPort )
{
	Init( ip0, ip1, ip2, ip3, ipPort );
}


void CIPAddr::Init( int ip0, int ip1, int ip2, int ip3, int ipPort )
{
	ip[0] = (unsigned char)ip0;
	ip[1] = (unsigned char)ip1;
	ip[2] = (unsigned char)ip2;
	ip[3] = (unsigned char)ip3;
	port  = (unsigned short)ipPort;
}

bool CIPAddr::operator==( const CIPAddr &o ) const
{
	return ip[0] == o.ip[0] && ip[1] == o.ip[1] && ip[2] == o.ip[2] && ip[3] == o.ip[3] && port == o.port;
}


bool CIPAddr::operator!=( const CIPAddr &o ) const
{
	return !( *this == o );
}


void CIPAddr::SetupLocal( int inPort )
{
	ip[0] = 0x7f;
	ip[1] = 0;
	ip[2] = 0;
	ip[3] = 1;
	port = inPort;
}


// ------------------------------------------------------------------------------------------ //
// Static helpers.
// ------------------------------------------------------------------------------------------ //

static double IP_FloatTime()
{
	CCycleCount cnt;
	cnt.Sample();
	return cnt.GetSeconds();
}

TIMEVAL SetupTimeVal( double flTimeout )
{
	TIMEVAL timeVal;
	timeVal.tv_sec = (long)flTimeout;
	timeVal.tv_usec = (long)( (flTimeout - (long)flTimeout) * 1000.0 );
	return timeVal;
}

// Convert a CIPAddr to a sockaddr_in.
void IPAddrToInAddr( const CIPAddr *pIn, in_addr *pOut )
{
	u_char *p = (u_char*)pOut;
	p[0] = pIn->ip[0];
	p[1] = pIn->ip[1];
	p[2] = pIn->ip[2];
	p[3] = pIn->ip[3];
}

// Convert a CIPAddr to a sockaddr_in.
void IPAddrToSockAddr( const CIPAddr *pIn, struct sockaddr_in *pOut )
{
	memset( pOut, 0, sizeof(*pOut) );
	pOut->sin_family = AF_INET;
	pOut->sin_port = htons( pIn->port );
	
	IPAddrToInAddr( pIn, &pOut->sin_addr );
}

// Convert a CIPAddr to a sockaddr_in.
void SockAddrToIPAddr( const struct sockaddr_in *pIn, CIPAddr *pOut )
{
	const u_char *p = (const u_char*)&pIn->sin_addr;
	pOut->ip[0] = p[0];
	pOut->ip[1] = p[1];
	pOut->ip[2] = p[2];
	pOut->ip[3] = p[3];
	pOut->port = ntohs( pIn->sin_port );
}


class CIPSocket : public ISocket
{
public:
					CIPSocket()
					{
						m_Socket = INVALID_SOCKET;
						m_bSetupToBroadcast = false;
					}

	virtual			~CIPSocket()
	{
		Term();
	}


// ISocket implementation.
public:

	virtual void	Release()
	{
		delete this;
	}

	
	virtual bool CreateSocket()
	{
		// Clear any old socket we had around.
		Term();

		// Create a socket to send and receive through.
		SOCKET sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
		if ( sock == INVALID_SOCKET )
		{
			Assert( false );
			return false;
		}

		// Nonblocking please..
		int status;
		DWORD val = 1;
		status = ioctlsocket( sock, FIONBIO, &val );
		if ( status != 0 )
		{
			assert( false );
			closesocket( sock );
			return false;
		}

		m_Socket = sock;
		return true;
	}


	// Called after we have a socket.
	virtual bool BindPart2( const CIPAddr *pAddr )
	{
		Assert( m_Socket != INVALID_SOCKET );

		// bind to it!
		sockaddr_in addr;
		IPAddrToSockAddr( pAddr, &addr );

		int status = bind( m_Socket, (sockaddr*)&addr, sizeof(addr) );
		if ( status == 0 )
		{
			return true;
		}
		else
		{
			Term();
			return false;
		}
	}


	virtual bool	Bind( const CIPAddr *pAddr )
	{
		if ( !CreateSocket() )
			return false;
    
		return BindPart2( pAddr );
	}

	virtual bool	BindToAny( const unsigned short port )
	{
		// (INADDR_ANY)
		CIPAddr addr;
		addr.ip[0] = addr.ip[1] = addr.ip[2] = addr.ip[3] = 0;
		addr.port = port;
		return Bind( &addr );
	}

	virtual bool	ListenToMulticastStream( const CIPAddr &addr, const CIPAddr &localInterface )
	{
		ip_mreq mr;
		IPAddrToInAddr( &addr, &mr.imr_multiaddr );
		IPAddrToInAddr( &localInterface, &mr.imr_interface );

		// This helps a lot if the stream is sending really fast.
		int rcvBuf = 1024*1024*2;
		setsockopt( m_Socket, SOL_SOCKET, SO_RCVBUF, (char*)&rcvBuf, sizeof( rcvBuf ) );

		if ( setsockopt( m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mr, sizeof( mr ) ) == 0 )
		{
			// Remember this so we do IP_DEL_MEMBERSHIP on shutdown.
			m_bMulticastGroupMembership = true;
			m_MulticastGroupMREQ = mr;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	virtual bool	Broadcast( const void *pData, const int len, const unsigned short port )
	{
		assert( m_Socket != INVALID_SOCKET );

		// Make sure we're setup to broadcast.
		if ( !m_bSetupToBroadcast )
		{
			BOOL bBroadcast = true;
			if ( setsockopt( m_Socket, SOL_SOCKET, SO_BROADCAST, (char*)&bBroadcast, sizeof( bBroadcast ) ) != 0 )
			{
				assert( false );
				return false;
			}

			m_bSetupToBroadcast = true;
		}

		CIPAddr addr;
		addr.ip[0] = addr.ip[1] = addr.ip[2] = addr.ip[3] = 0xFF;
		addr.port = port;
		return SendTo( &addr, pData, len );
	}
	
	virtual bool	SendTo( const CIPAddr *pAddr, const void *pData, const int len )
	{
		return SendChunksTo( pAddr, &pData, &len, 1 );
	}

	virtual bool SendChunksTo( const CIPAddr *pAddr, void const * const *pChunks, const int *pChunkLengths, int nChunks )
	{
		WSABUF bufs[32];
		if ( nChunks > 32 )
		{
			Error( "CIPSocket::SendChunksTo: too many chunks (%d).", nChunks );
		}

		int nTotalBytes = 0;
		for ( int i=0; i < nChunks; i++ )
		{
			bufs[i].len = pChunkLengths[i];
			bufs[i].buf = (char*)pChunks[i];
			nTotalBytes += pChunkLengths[i];
		}

		assert( m_Socket != INVALID_SOCKET );

		// Translate the address.
		sockaddr_in addr;
		IPAddrToSockAddr( pAddr, &addr );

		DWORD dwNumBytesSent = 0;
		DWORD ret = WSASendTo( 
			m_Socket, 
			bufs,
			nChunks,
			&dwNumBytesSent,
			0,
			(sockaddr*)&addr,
			sizeof( addr ),
			NULL,
			NULL
			);

		return ret == 0 && (int)dwNumBytesSent == nTotalBytes;
	}

	virtual int		RecvFrom( void *pData, int maxDataLen, CIPAddr *pFrom )
	{
		assert( m_Socket != INVALID_SOCKET );

		fd_set readSet;
		readSet.fd_count = 1;
		readSet.fd_array[0] = m_Socket;

		TIMEVAL timeVal = SetupTimeVal( 0 );

		// See if it has a packet waiting.
		int status = select( 0, &readSet, NULL, NULL, &timeVal );
		if ( status == 0 || status == SOCKET_ERROR )
			return -1;

		// Get the data.
		sockaddr_in sender;
		int fromSize = sizeof( sockaddr_in );
		status = recvfrom( m_Socket, (char*)pData, maxDataLen, 0, (struct sockaddr*)&sender, &fromSize );
		if ( status == 0 || status == SOCKET_ERROR )
		{
			return -1;
		}
		else
		{
			if ( pFrom )
			{
				SockAddrToIPAddr( &sender, pFrom );
			}
	
			m_flLastRecvTime = IP_FloatTime();
			return status;
		}
	}

	virtual double	GetRecvTimeout()
	{
		return IP_FloatTime() - m_flLastRecvTime;
	}
	

private:

	void			Term()
	{
		if ( m_Socket != INVALID_SOCKET )
		{
			if ( m_bMulticastGroupMembership )
			{
				// Undo our multicast group membership.
				setsockopt( m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&m_MulticastGroupMREQ, sizeof( m_MulticastGroupMREQ ) );
			}

			closesocket( m_Socket );
			m_Socket = INVALID_SOCKET;
		}

		m_bSetupToBroadcast = false;
		m_bMulticastGroupMembership = false;
	}


private:

	SOCKET			m_Socket;
	
	bool			m_bMulticastGroupMembership; // Did we join a multicast group?
	ip_mreq			m_MulticastGroupMREQ;

	bool			m_bSetupToBroadcast;
	double			m_flLastRecvTime;
	bool			m_bListenSocket;
};



ISocket* CreateIPSocket()
{
	return new CIPSocket;
}


ISocket* CreateMulticastListenSocket( 
	const CIPAddr &addr, 
	const CIPAddr &localInterface )
{
	CIPSocket *pSocket = new CIPSocket;

	CIPAddr bindAddr = localInterface;
	bindAddr.port = addr.port;

	if ( pSocket->Bind( &bindAddr ) &&
		pSocket->ListenToMulticastStream( addr, localInterface )
		)
	{
		return pSocket;
	}
	else
	{
		pSocket->Release();
		return NULL;
	}
}


bool ConvertStringToIPAddr( const char *pStr, CIPAddr *pOut )
{
	char ipStr[512];

	const char *pColon = strchr( pStr, ':' );
	if ( pColon )
	{
		int toCopy = pColon - pStr;
		if ( toCopy < 2 || toCopy > sizeof(ipStr)-1 )
		{
			assert( false );
			return false;
		}

		memcpy( ipStr, pStr, toCopy );
		ipStr[toCopy] = 0;

		pOut->port = (unsigned short)atoi( pColon+1 );
	}
	else
	{
		strncpy( ipStr, pStr, sizeof( ipStr ) );
		ipStr[ sizeof(ipStr)-1 ] = 0;
	}

	if ( ipStr[0] >= '0' && ipStr[0] <= '9' )
	{
		// It's numbers.
		int ip[4];
		sscanf( ipStr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3] );
		pOut->ip[0] = (unsigned char)ip[0];
		pOut->ip[1] = (unsigned char)ip[1];
		pOut->ip[2] = (unsigned char)ip[2];
		pOut->ip[3] = (unsigned char)ip[3];
	}
	else
	{
		// It's a text string.
		struct hostent *pHost = gethostbyname( ipStr );
		if( !pHost )
			return false;

		pOut->ip[0] = pHost->h_addr_list[0][0];
		pOut->ip[1] = pHost->h_addr_list[0][1];
		pOut->ip[2] = pHost->h_addr_list[0][2];
		pOut->ip[3] = pHost->h_addr_list[0][3];
	}

	return true;
}


bool ConvertIPAddrToString( const CIPAddr *pIn, char *pOut, int outLen )
{
	in_addr addr;
	addr.S_un.S_un_b.s_b1 = pIn->ip[0];
	addr.S_un.S_un_b.s_b2 = pIn->ip[1];
	addr.S_un.S_un_b.s_b3 = pIn->ip[2];
	addr.S_un.S_un_b.s_b4 = pIn->ip[3];

	HOSTENT *pEnt = gethostbyaddr( (char*)&addr, sizeof( addr ), AF_INET );
	if ( pEnt )
	{
		Q_strncpy( pOut, pEnt->h_name, outLen );
		return true;
	}
	else
	{
		return false;
	}
}


void IP_GetLastErrorString( char *pStr, int maxLen )
{
	char *lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	Q_strncpy( pStr, lpMsgBuf, maxLen );
	LocalFree( lpMsgBuf );	
}

