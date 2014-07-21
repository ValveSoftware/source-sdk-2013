//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef IPHELPERS_H
#define IPHELPERS_H


#include "ichannel.h"


// Loops that poll sockets should Sleep for this amount of time between iterations
// so they don't hog all the CPU.
#define LOOP_POLL_INTERVAL	5


// Useful for putting the arguments into a printf statement.
#define EXPAND_ADDR( x ) (x).ip[0], (x).ip[1], (x).ip[2], (x).ip[3], (x).port


// This is a simple wrapper layer for UDP sockets.
class CIPAddr
{
public:
					CIPAddr();
					CIPAddr( const int inputIP[4], const int inputPort );
					CIPAddr( int ip0, int ip1, int ip2, int ip3, int ipPort );
	
	void			Init( int ip0, int ip1, int ip2, int ip3, int ipPort );
	bool			operator==( const CIPAddr &o ) const;
	bool			operator!=( const CIPAddr &o ) const;

	// Setup to send to the local machine on the specified port.
	void			SetupLocal( int inPort );

public:

	unsigned char	ip[4];
	unsigned short	port;
};



// The chunk walker provides an easy way to copy data out of the chunks as though it were a
// single contiguous chunk of memory.s
class CChunkWalker
{
public:
					CChunkWalker( void const * const *pChunks, const int *pChunkLengths, int nChunks );

	int				GetTotalLength() const;
	void			CopyTo( void *pOut, int nBytes );	

private:
	
	void const * const		*m_pChunks;
	const int				*m_pChunkLengths;
	int						m_nChunks;
	
	int						m_iCurChunk;
	int						m_iCurChunkPos;

	int						m_TotalLength;
};


// This class makes loop that wait on something look nicer. ALL loops using this class
// should follow this pattern, or they can wind up with unforeseen delays that add a whole
// lot of lag.
//
// CWaitTimer waitTimer( 5.0 );
// while ( 1 )
// {
//		do your thing here like Recv() from a socket.
//
//		if ( waitTimer.ShouldKeepWaiting() )
//			Sleep() for some time interval like 5ms so you don't hog the CPU
//		else
//			BREAK HERE
// }
class CWaitTimer
{
public:
			CWaitTimer( double flSeconds );

	bool	ShouldKeepWaiting();	

private:
	unsigned long	m_StartTime;
	unsigned long	m_WaitMS;
};


// Helper function to get time in milliseconds.
unsigned long SampleMilliseconds();


class ISocket
{
public:

	// Call this when you're done.	
	virtual void	Release() = 0;

	
	// Bind the socket so you can send and receive with it.
	// If you bind to port 0, then the system will select the port for you.
	virtual bool	Bind( const CIPAddr *pAddr ) = 0;
	virtual bool	BindToAny( const unsigned short port ) = 0;

	
	// Broadcast some data.
	virtual bool	Broadcast( const void *pData, const int len, const unsigned short port ) = 0;
	
	// Send a packet.
	virtual bool	SendTo( const CIPAddr *pAddr, const void *pData, const int len ) = 0;
	virtual bool	SendChunksTo( const CIPAddr *pAddr, void const * const *pChunks, const int *pChunkLengths, int nChunks ) = 0;

	// Receive a packet. Returns the length received or -1 if no data came in.
	// If pFrom is set, then it is filled in with the sender's IP address.
	virtual int		RecvFrom( void *pData, int maxDataLen, CIPAddr *pFrom ) = 0;

	// How long has it been since we successfully received a packet?
	virtual double	GetRecvTimeout() = 0;
};

// Create a connectionless socket that you can send packets out of.
ISocket* CreateIPSocket();

// This sets up the socket to receive multicast data on the specified group.
// By default, localInterface is INADDR_ANY, but if you want to specify a specific interface
// the data should come in through, you can.
ISocket* CreateMulticastListenSocket( 
	const CIPAddr &addr, 
	const CIPAddr &localInterface = CIPAddr() 
	);


// Setup a CIPAddr from the string. The string can be a dotted IP address or
// a hostname, and it can be followed by a colon and a port number like "1.2.3.4:3443"
// or "myhostname.valvesoftware.com:2342".
//
// Note: if the string does not contain a port, then pOut->port will be left alone.
bool ConvertStringToIPAddr( const char *pStr, CIPAddr *pOut );

// Do a DNS lookup on the IP.
// You can optionally get a service name back too.
bool ConvertIPAddrToString( const CIPAddr *pIn, char *pOut, int outLen );


void IP_GetLastErrorString( char *pStr, int maxLen );

void SockAddrToIPAddr( const struct sockaddr_in *pIn, CIPAddr *pOut );
void IPAddrToSockAddr( const CIPAddr *pIn, struct sockaddr_in *pOut );


#endif

