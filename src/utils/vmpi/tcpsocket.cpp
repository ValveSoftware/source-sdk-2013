//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


//#define PARANOID

#if defined( PARANOID )
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#include <winsock2.h>
#include <mswsock.h>
#include "tcpsocket.h"
#include "tier1/utllinkedlist.h"
#include <stdio.h>
#include "threadhelpers.h"
#include "tier0/dbg.h"



#error "I am TCPSocket and I suck. Use IThreadedTCPSocket or ThreadedTCPSocketEmu instead."


extern TIMEVAL SetupTimeVal( double flTimeout );
extern void IPAddrToSockAddr( const CIPAddr *pIn, sockaddr_in *pOut );
extern void SockAddrToIPAddr( const sockaddr_in *pIn, CIPAddr *pOut );


#define SENTINEL_DISCONNECT	-1
#define SENTINEL_KEEPALIVE	-2


#define KEEPALIVE_INTERVAL_MS		3000	// keepalives are sent every N MS
#define KEEPALIVE_TIMEOUT_SECONDS	15.0	// connections timeout after this long


static bool g_bEnableTCPTimeout = true;


class CRecvData
{
public:
	int				m_Count;
	unsigned char	m_Data[1];
};
	


SOCKET TCPBind( const CIPAddr *pAddr )
{
	// Create a socket to send and receive through.
	SOCKET sock = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
	if ( sock == INVALID_SOCKET )
	{
		Assert( false );
		return INVALID_SOCKET;
	}

	// bind to it!
	sockaddr_in addr;
	IPAddrToSockAddr( pAddr, &addr );

	int status = bind( sock, (sockaddr*)&addr, sizeof(addr) );
	if ( status == 0 )
	{
		return sock;
	}
	else
	{
		closesocket( sock );
		return INVALID_SOCKET;
	}
}



// ---------------------------------------------------------------------------------------- //
// TCP sockets.
// ---------------------------------------------------------------------------------------- //

enum
{
	OP_RECV=111,
	OP_SEND
};

// We use this for all OVERLAPPED structures.
class COverlappedPlus : public WSAOVERLAPPED
{
public:
				COverlappedPlus()
				{
					memset( this, 0, sizeof( WSAOVERLAPPED ) );
				}

	int					m_OPType;		// One of the OP_ defines.
};

typedef struct SendBuf_t
{
	COverlappedPlus		m_Overlapped;
	int					m_Index;		// Index into m_SendBufs.
	int					m_DataLength;
	char				m_Data[1];
} SendBuf_s;


// These manage a thread that calls SendKeepalive() on all TCPSockets.
// AddGlobalTCPSocket shouldn't be called until you're ready for SendKeepalive() to be called.
class CTCPSocket;
void AddGlobalTCPSocket( CTCPSocket *pSocket );
void RemoveGlobalTCPSocket( CTCPSocket *pSocket );



// ------------------------------------------------------------------------------------------ //
// CTCPSocket implementation.
// ------------------------------------------------------------------------------------------ //

class CTCPSocket : public ITCPSocket
{
friend class CTCPListenSocket;

public:

					CTCPSocket()
					{
						m_Socket = INVALID_SOCKET;
						m_bConnected = false;
						
						m_hIOCP = NULL;
						
						m_bShouldExitThreads = false;
						m_bConnectionLost = false;
						m_nSizeBytesReceived = 0;

						m_pIncomingData = NULL;

						memset( &m_RecvOverlapped, 0, sizeof( m_RecvOverlapped ) );
						m_RecvOverlapped.m_OPType = OP_RECV;

						m_hRecvSignal = CreateEvent( NULL, FALSE, FALSE, NULL );
						m_RecvStage = -1;

						m_MainThreadID = GetCurrentThreadId();
					}
	
	virtual			~CTCPSocket()
	{
		Term();
		CloseHandle( m_hRecvSignal );
	}

	void Term()
	{
		Assert( GetCurrentThreadId() == m_MainThreadID );

		RemoveGlobalTCPSocket( this );

		if ( m_Socket != SOCKET_ERROR && !m_bConnectionLost )
		{
			SendDisconnectSentinel();
	
			// Give the sends a second to complete. SO_LINGER is having trouble for some reason.
			WaitForSendsToComplete( 1 );
		}


		StopThreads();

		if ( m_Socket != INVALID_SOCKET )
		{
			closesocket( m_Socket );
			m_Socket = INVALID_SOCKET;
		}

		if ( m_hIOCP )
		{
			CloseHandle( m_hIOCP );
			m_hIOCP = NULL;
		}

		m_bConnected = false;
		m_bConnectionLost = true;
		m_RecvStage = -1;
		
		FOR_EACH_LL( m_SendBufs, i )
		{
			SendBuf_t *pSendBuf = m_SendBufs[i];
			ParanoidMemoryCheck( pSendBuf );
			free( pSendBuf );
		}
		m_SendBufs.Purge();

		FOR_EACH_LL( m_RecvDatas, j )
		{
			CRecvData *pRecvData = m_RecvDatas[j];
			ParanoidMemoryCheck( pRecvData );
			free( pRecvData );
		}
		m_RecvDatas.Purge();

		if ( m_pIncomingData )
		{
			ParanoidMemoryCheck( m_pIncomingData );
			free( m_pIncomingData );
			m_pIncomingData = 0;
		}
	}

	virtual void	Release()
	{
		delete this;
	}


	void ParanoidMemoryCheck( void *ptr = NULL )
	{
#if defined( PARANOID )
		Assert( _CrtIsValidHeapPointer( this ) );

		if ( ptr )
		{
			Assert( _CrtIsValidHeapPointer( ptr ) );
		}

		Assert( _CrtCheckMemory() == TRUE );
#endif
	}

	
	virtual bool	BindToAny( const unsigned short port )
	{
		Term();

		CIPAddr addr( 0, 0, 0, 0, port ); // INADDR_ANY
		m_Socket = TCPBind( &addr );
		if ( m_Socket == INVALID_SOCKET )
		{
			return false;
		}
		else
		{
			SetInitialSocketOptions();
			return true;
		}
	}

	
	// Set the initial socket options that we want.
	void SetInitialSocketOptions()
	{
		// Set nodelay to improve latency.
		BOOL val = TRUE;
		setsockopt( m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char FAR *)&val, sizeof(BOOL) );

		// Make it linger for 3 seconds when it exits.
		LINGER linger;
		linger.l_onoff = 1;
		linger.l_linger = 3;
		setsockopt( m_Socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof( linger ) );
	}


	// Called only by main thread interface functions.
	// Returns true if the connection is lost.
	bool CheckConnectionLost()
	{
		Assert( GetCurrentThreadId() == m_MainThreadID );

		if ( m_Socket == SOCKET_ERROR )
			return true;

		// Have we timed out?
		if ( g_bEnableTCPTimeout && (Plat_FloatTime() - m_LastRecvTime > KEEPALIVE_TIMEOUT_SECONDS) )
		{
			SetConnectionLost( "Connection timed out." );
		}

		// Has any thread posted that the connection has been lost?
		CCriticalSectionLock postLock( &m_ConnectionLostCS );
		postLock.Lock();
		if ( m_bConnectionLost )
		{
			Term();
			return true;
		}
		else
		{
			return false;
		}
	}

	// Called by any thread. All interface functions call CheckConnectionLost() and return errors if it's lost.
	void SetConnectionLost( const char *pErrorString, int err = -1 )
	{
		CCriticalSectionLock postLock( &m_ConnectionLostCS );
		postLock.Lock();
			m_bConnectionLost = true;
		postLock.Unlock();

		// Handle it right away if we're in the main thread. If we're in an IO thread,
		// it has to wait until the next interface function calls CheckConnectionLost().
		if ( GetCurrentThreadId() == m_MainThreadID )
		{
			Term();
		}
		
		if ( pErrorString )
		{
			m_ErrorString.CopyArray( pErrorString, strlen( pErrorString ) + 1 );
		}
		else
		{
			char *lpMsgBuf;
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
			);

			m_ErrorString.CopyArray( lpMsgBuf, strlen( lpMsgBuf ) + 1 );
			LocalFree( lpMsgBuf );
		}
	}
		

	// -------------------------------------------------------------------------------------------------- //
	// The receive code.
	// -------------------------------------------------------------------------------------------------- //

	virtual bool StartWaitingForSize( bool bFresh )
	{
		Assert( m_Socket != INVALID_SOCKET );
		Assert( m_bConnected );

		m_RecvStage = 0;
		m_RecvDataSize = -1;
		if ( bFresh )
			m_nSizeBytesReceived = 0;

		DWORD dwNumBytesReceived = 0;
		WSABUF buf = { sizeof( &m_RecvDataSize ) - m_nSizeBytesReceived, ((char*)&m_RecvDataSize) + m_nSizeBytesReceived };
		DWORD dwFlags = 0;

		int status = WSARecv( 
			m_Socket,
			&buf,
			1,
			&dwNumBytesReceived,
			&dwFlags,
			&m_RecvOverlapped,
			NULL );

		int err = -1;
		if ( status == SOCKET_ERROR && (err = WSAGetLastError()) != ERROR_IO_PENDING )
		{
			SetConnectionLost( NULL, err );
			return false;
		}
		else
		{
			return true;
		}
	}


	bool PostNextDataPart()
	{
		DWORD dwNumBytesReceived = 0;
		WSABUF buf = { m_RecvDataSize - m_AmountReceived, (char*)m_pIncomingData->m_Data + m_AmountReceived }; 
		DWORD dwFlags = 0;

		int status = WSARecv( 
			m_Socket,
			&buf,
			1,
			&dwNumBytesReceived,
			&dwFlags,
			&m_RecvOverlapped,
			NULL );

		int err = -1;
		if ( status == SOCKET_ERROR && (err = WSAGetLastError()) != ERROR_IO_PENDING )
		{
			SetConnectionLost( NULL, err );
			return false;
		}
		else
		{
			return true;
		}
	}


	bool StartWaitingForData()
	{
		Assert( m_Socket != INVALID_SOCKET );
		Assert( m_RecvStage == 0 );
		Assert( m_bConnected );
		Assert( m_RecvDataSize > 0 );

		m_RecvStage = 1;

		// Add a CRecvData element.
		ParanoidMemoryCheck();
		m_pIncomingData = (CRecvData*)malloc( sizeof( CRecvData ) - 1 + m_RecvDataSize );
		if ( !m_pIncomingData )
		{
			char str[512];
			_snprintf( str, sizeof( str ), "malloc() failed. m_RecvDataSize = %d\n", m_RecvDataSize );
			SetConnectionLost( str );
			return false;
		}

		m_pIncomingData->m_Count = m_RecvDataSize;

		m_AmountReceived = 0;

		return PostNextDataPart();
	}

	virtual bool	Recv( CUtlVector<unsigned char> &data, double flTimeout )
	{
		if ( CheckConnectionLost() )
			return false;

		// Wait in 50ms chunks, checking for disconnections along the way.
		bool bGotData = false;
		DWORD msToWait = (DWORD)( flTimeout * 1000.0 );
		do
		{
			DWORD curWaitTime = min( msToWait, 50 );
			DWORD ret = WaitForSingleObject( m_hRecvSignal, curWaitTime );
			if ( ret == WAIT_OBJECT_0 )
			{
				bGotData = true;
				break;
			}

			// Did the connection timeout?
			if ( CheckConnectionLost() )
				return false;

			msToWait -= curWaitTime;
		} while ( msToWait );
		
		// If we never got a WAIT_OBJECT_0, then we never received anything.
		if ( !bGotData )
			return false;
		
		
		CCriticalSectionLock csLock( &m_RecvDataCS );
		csLock.Lock();

		// Pickup the head m_RecvDatas element.
		CRecvData *pRecvData = m_RecvDatas[ m_RecvDatas.Head() ];
		data.CopyArray( pRecvData->m_Data, pRecvData->m_Count );

		// Now free it.
		m_RecvDatas.Remove( m_RecvDatas.Head() );
		ParanoidMemoryCheck( pRecvData );
		free( pRecvData );

		// Set the event again for the next time around, if there is more data waiting.
		if ( m_RecvDatas.Count() > 0 )
			SetEvent( m_hRecvSignal );

		return true;
	}

	// INSIDE IO THREAD.
	void HandleRecvCompletion( COverlappedPlus *pInfo, DWORD dwNumBytes )
	{
		if ( dwNumBytes == 0 )
		{
			SetConnectionLost( "Got 0 bytes in HandleRecvCompletion" );
			return;
		}

		m_LastRecvTime = Plat_FloatTime();
		if ( m_RecvStage == 0 )
		{
			m_nSizeBytesReceived += dwNumBytes;
			if ( m_nSizeBytesReceived == sizeof( m_RecvDataSize ) )
			{
				// Size of -1 means the other size is breaking the connection.
				if ( m_RecvDataSize == SENTINEL_DISCONNECT )
				{
					SetConnectionLost( "Got a graceful disconnect message." );
					return;
				}
				else if ( m_RecvDataSize == SENTINEL_KEEPALIVE )
				{
					// No data follows this. Just let m_LastRecvTime get updated.
					StartWaitingForSize( true );
					return;
				}

				StartWaitingForData();
			}
			else if ( m_nSizeBytesReceived < sizeof( m_RecvDataSize ) )
			{
				// Handle the case where we only got some of the data (maybe one of the clients got disconnected).
				StartWaitingForSize( false );
			}
			else
			{
				// This case should never ever happen!
#if defined( _DEBUG )
					__asm int 3;
#endif

				SetConnectionLost( "Received too much data in a packet!" );
				return;
			}
		}
		else if ( m_RecvStage == 1 )
		{
			// Got the data, make sure we got it all.
			m_AmountReceived += dwNumBytes;

			// Sanity check.
#if defined( _DEBUG )
			Assert( m_RecvDataSize == m_pIncomingData->m_Count );
			Assert( m_AmountReceived <= m_RecvDataSize );	// TODO: make this threadsafe for multiple IO threads.
#endif

			if ( m_AmountReceived == m_RecvDataSize )
			{
				m_RecvStage = 2;
				
				// Add the data to the list of packets waiting to be picked up.
				CCriticalSectionLock csLock( &m_RecvDataCS );
				csLock.Lock();
				
				m_RecvDatas.AddToTail( m_pIncomingData );
				m_pIncomingData = NULL;

				if ( m_RecvDatas.Count() == 1 )
					SetEvent( m_hRecvSignal );	// Notify the Recv() function.

				StartWaitingForSize( true );
			}
			else
			{
				PostNextDataPart();
			}
		}
		else
		{
			Assert( false );
		}
	}
	

	// -------------------------------------------------------------------------------------------------- //
	// The send code.
	// -------------------------------------------------------------------------------------------------- //

	virtual void	WaitForSendsToComplete( double flTimeout )
	{
		CWaitTimer waitTimer( flTimeout );
		while ( 1 )
		{
			CCriticalSectionLock sendBufLock( &m_SendCS );
			sendBufLock.Lock();
				if( m_SendBufs.Count() == 0 )
					return;
			sendBufLock.Unlock();

			if ( waitTimer.ShouldKeepWaiting() )
				Sleep( 10 );
			else
				break;
		}
	}


	// This is called in the keepalive thread.
	void SendKeepalive()
	{
		// Send a message saying we're exiting.
		ParanoidMemoryCheck();
		SendBuf_t *pBuf = (SendBuf_t*)malloc( sizeof( SendBuf_t ) - 1 + sizeof( int ) );
		if ( !pBuf )
		{
			SetConnectionLost( "malloc() in SendKeepalive() failed." );
			return;
		}

		pBuf->m_DataLength = sizeof( int );
		*((int*)pBuf->m_Data) = SENTINEL_KEEPALIVE;
		InternalSendDataBuf( pBuf );
	}


	void SendDisconnectSentinel()
	{
		// Send a message saying we're exiting.
		ParanoidMemoryCheck();
		SendBuf_t *pBuf = (SendBuf_t*)malloc( sizeof( SendBuf_t ) - 1 + sizeof( int ) );
		if ( pBuf )
		{
			pBuf->m_DataLength = sizeof( int );
			*((int*)pBuf->m_Data) = SENTINEL_DISCONNECT;	// This signifies that we're exiting.
			InternalSendDataBuf( pBuf );
		}
	}


	virtual bool	Send( const void *pData, int len )
	{
		const void *pChunks[1] = { pData };
		int chunkLengths[1] = { len };
		return SendChunks( pChunks, chunkLengths, 1 );
	}

	
	virtual bool	SendChunks( void const * const *pChunks, const int *pChunkLengths, int nChunks )
	{
		if ( CheckConnectionLost() )
			return false;
		
		CChunkWalker walker( pChunks, pChunkLengths, nChunks );
		int totalLength = walker.GetTotalLength();

		if ( !totalLength )
			return true;

		// Create a buffer to hold the data and copy the data in.
		ParanoidMemoryCheck();
		SendBuf_t *pBuf = (SendBuf_t*)malloc( sizeof( SendBuf_t ) - 1 + totalLength + sizeof( int ) );
		if ( !pBuf )
		{
			char str[512];
			_snprintf( str, sizeof( str ), "malloc() in SendChunks() failed. totalLength = %d.", totalLength );
			SetConnectionLost( str );
			return false;
		}

		pBuf->m_DataLength = totalLength + sizeof( int );

		int *pByteCountPos = (int*)pBuf->m_Data;
		*pByteCountPos = totalLength;

		char *pDataPos = &pBuf->m_Data[ sizeof( int ) ];
		walker.CopyTo( pDataPos, totalLength );

		int status = InternalSendDataBuf( pBuf );
		int err = -1;
		if ( status == SOCKET_ERROR && (err = WSAGetLastError()) != ERROR_IO_PENDING )
		{
			SetConnectionLost( NULL, err );
			return false;
		}
		else
		{
			return true;
		}
	}


	int InternalSendDataBuf( SendBuf_t *pBuf )
	{
		// Protect against interference from the keepalive thread.
		CCriticalSectionLock csLock( &m_SendCS );
		csLock.Lock();


		pBuf->m_Overlapped.m_OPType = OP_SEND;
		pBuf->m_Overlapped.hEvent = NULL;

		// Add it to our list of buffers.
		pBuf->m_Index = m_SendBufs.AddToTail( pBuf );

		// Tell Winsock to send it.
		WSABUF buf = { pBuf->m_DataLength, pBuf->m_Data };
		
		DWORD dwNumBytesSent = 0;
		return WSASend(
			m_Socket,
			&buf,
			1,
			&dwNumBytesSent,
			0,
			&pBuf->m_Overlapped,
			NULL );
	}


	// INSIDE IO THREAD.
	void HandleSendCompletion( COverlappedPlus *pInfo, DWORD dwNumBytes )
	{
		if ( dwNumBytes == 0 )
		{
			SetConnectionLost( "0 bytes in HandleSendCompletion." );
			return;
		}

		// Just free the buffer.
		SendBuf_t *pBuf = (SendBuf_t*)pInfo;
		Assert( dwNumBytes == (DWORD)pBuf->m_DataLength );

		CCriticalSectionLock sendBufLock( &m_SendCS );
		sendBufLock.Lock();
			m_SendBufs.Remove( pBuf->m_Index );
		sendBufLock.Unlock();

		ParanoidMemoryCheck( pBuf );
		free( pBuf );
	}


	// -------------------------------------------------------------------------------------------------- //
	// The connect code.
	// -------------------------------------------------------------------------------------------------- //

	virtual bool BeginConnect( const CIPAddr &inputAddr )
	{
		sockaddr_in addr;
		IPAddrToSockAddr( &inputAddr, &addr );

		m_bConnected = false;
		int ret = connect( m_Socket, (struct sockaddr*)&addr, sizeof( addr ) );
		ret=ret;

		return true;
	}


	virtual bool UpdateConnect()
	{
		// We're still ok.. just wait until the socket becomes writable (is connected) or we timeout.
		fd_set writeSet;
		writeSet.fd_count = 1;
		writeSet.fd_array[0] = m_Socket;
		TIMEVAL timeVal = SetupTimeVal( 0 );

		// See if it has a packet waiting.
		int status = select( 0, NULL, &writeSet, NULL, &timeVal );
		if ( status > 0 )
		{
			SetupConnected();
			return true;
		}

		return false;
	}


	void SetupConnected()
	{
		m_bConnected = true;
		m_bConnectionLost = false;
		m_LastRecvTime = Plat_FloatTime(); 

		CreateThreads();
		StartWaitingForSize( true );
		AddGlobalTCPSocket( this );
	}


	virtual bool	IsConnected()
	{
		CheckConnectionLost();
		return m_bConnected;
	}


	virtual void GetDisconnectReason( CUtlVector<char> &reason )
	{
		reason = m_ErrorString;
	}


	// -------------------------------------------------------------------------------------------------- //
	// Threads code.
	// -------------------------------------------------------------------------------------------------- //

	// Create our IO Completion Port threads.
	bool CreateThreads()
	{
		int nThreads = 1;
		SetShouldExitThreads( false );

        // Create our IO completion port and hook it to our socket.
		m_hIOCP = CreateIoCompletionPort(        
            INVALID_HANDLE_VALUE, NULL, 0, 0); 

		m_hIOCP = CreateIoCompletionPort( (HANDLE)m_Socket, m_hIOCP, (unsigned long)this, nThreads ); 

		for ( int i=0; i < nThreads; i++ )
		{
			DWORD dwThreadID = 0;
			HANDLE hThread = CreateThread( 
				NULL,
				0,
				&CTCPSocket::StaticThreadFn,
				this,
				0,
				&dwThreadID );

			if ( hThread )
			{
				SetThreadPriority( hThread, THREAD_PRIORITY_ABOVE_NORMAL );
				m_Threads.AddToTail( hThread );
			}
			else
			{
				StopThreads();
				return false;
			}
		}
	
		return true;
	}

	
	void StopThreads()
	{
		// Tell the threads to exit, then wait for them to do so.
		SetShouldExitThreads( true );
		WaitForMultipleObjects( m_Threads.Count(), m_Threads.Base(), TRUE, INFINITE );

		for ( int i=0; i < m_Threads.Count(); i++ )
		{
			CloseHandle( m_Threads[i] );
		}
		m_Threads.Purge();
	}


	void SetShouldExitThreads( bool bShouldExit )
	{
		CCriticalSectionLock lock( &m_ThreadsCS );
		lock.Lock();
		m_bShouldExitThreads = bShouldExit;
	}


	bool ShouldExitThreads()
	{
		CCriticalSectionLock lock( &m_ThreadsCS );
		lock.Lock();

		bool bRet = m_bShouldExitThreads;
		return bRet;
	}


	DWORD ThreadFn()
	{
		while ( 1 )
		{
			DWORD dwNumBytes = 0;
			unsigned long pInputTCPSocket;
			LPOVERLAPPED pOverlapped;

			if ( GetQueuedCompletionStatus( 
				m_hIOCP,		// the port we're listening on
				&dwNumBytes,	// # bytes received on the port
				&pInputTCPSocket,// "completion key" = CTCPSocket*
				&pOverlapped,	// the overlapped info that was passed into AcceptEx, WSARecv, or WSASend.
				100				// listen for 100ms at a time so we can exit gracefully when the socket is deleted.
				) )
			{
				COverlappedPlus *pInfo = (COverlappedPlus*)pOverlapped;
				ParanoidMemoryCheck( pInfo );
				
				if ( pInfo->m_OPType == OP_RECV )
				{
					Assert( pInfo == &m_RecvOverlapped );
					HandleRecvCompletion( pInfo, dwNumBytes );
				}
				else
				{
					Assert( pInfo->m_OPType == OP_SEND );
					HandleSendCompletion( pInfo, dwNumBytes );
				}
			}
			
			if ( ShouldExitThreads() )
				break;
		}

		return 0;
	}
	

	static DWORD WINAPI StaticThreadFn( LPVOID pParameter )
	{
		return ((CTCPSocket*)pParameter)->ThreadFn();
	}

	

private:

	SOCKET		m_Socket;
	bool		m_bConnected;


	// m_RecvOverlapped is setup to first wait for the size, then the data.
	// Then it is not posted until the app grabs the data.
	HANDLE						m_hRecvSignal;	// Tells Recv() when we have data.
	COverlappedPlus				m_RecvOverlapped;
	int							m_RecvStage;	// -1 = not initialized
												//  0 = waiting for size
												//  1 = waiting for data
												//  2 = waiting for app to pickup the data

	CUtlLinkedList<CRecvData*,int>	m_RecvDatas;	// The head element is the next one to be picked up.
	CRecvData					*m_pIncomingData;	// The packet we're currently receiving.
	CCriticalSection			m_RecvDataCS;		// This protects adds and removes in the list.

	// These reference the element at the tail of m_RecvData. It is the current one getting 
	volatile int				m_nSizeBytesReceived;	// How much of m_RecvDataSize have we received yet?
	int							m_RecvDataSize;			// this is received over the network
	int							m_AmountReceived;		// How much we've received so far.

	// Last time we received anything from this connection. Used to determine if the connection is 
	// still active.
	double						m_LastRecvTime;


	// Outgoing send buffers.
	CUtlLinkedList<SendBuf_t*,int>	m_SendBufs;
	CCriticalSection				m_SendCS;
	

	// All the threads waiting for IO.
	CUtlVector<HANDLE>		m_Threads;
	HANDLE					m_hIOCP;

	// Used during shutdown.
	volatile bool			m_bShouldExitThreads;
	CCriticalSection		m_ThreadsCS;

	// For debugging.
	DWORD					m_MainThreadID;

	// Set by the main thread or IO threads to signal connection lost.
	bool					m_bConnectionLost;
	CCriticalSection		m_ConnectionLostCS;

	// This is set when we get disconnected.
	CUtlVector<char>		m_ErrorString;
};


// ------------------------------------------------------------------------------------------ //
// ITCPListenSocket implementation.
// ------------------------------------------------------------------------------------------ //

class CTCPListenSocket : public ITCPListenSocket
{
public:

						CTCPListenSocket()
						{
							m_Socket = INVALID_SOCKET;
						}


	virtual				~CTCPListenSocket()
	{
		if ( m_Socket != INVALID_SOCKET )
		{
			closesocket( m_Socket );
		}
	}	


	// The main function to create one of these suckers.
	static ITCPListenSocket*	Create( const unsigned short port, int nQueueLength )
	{
		CTCPListenSocket *pRet = new CTCPListenSocket;
		if ( !pRet )
			return NULL;

		// Bind it to a socket and start listening.
		CIPAddr addr( 0, 0, 0, 0, port ); // INADDR_ANY
		pRet->m_Socket = TCPBind( &addr );
		if ( pRet->m_Socket == INVALID_SOCKET || 
			listen( pRet->m_Socket, nQueueLength == -1 ? SOMAXCONN : nQueueLength ) != 0 )
		{
			pRet->Release();
			return false;
		}

		return pRet;
	}


	virtual void		Release()
	{
		delete this;
	}


	virtual ITCPSocket*	UpdateListen( CIPAddr *pAddr )
	{
		// We're still ok.. just wait until the socket becomes writable (is connected) or we timeout.
		fd_set readSet;
		readSet.fd_count = 1;
		readSet.fd_array[0] = m_Socket;
		TIMEVAL timeVal = SetupTimeVal( 0 );

		// Wait until it connects.
		int status = select( 0, &readSet, NULL, NULL, &timeVal );
		if ( status > 0 )
		{
			sockaddr_in addr;
			int addrSize = sizeof( addr );

			// Now accept the final connection.
			SOCKET newSock = accept( m_Socket, (struct sockaddr*)&addr, &addrSize );
			if ( newSock == INVALID_SOCKET )
			{
				Assert( false );
			}
			else
			{
				CTCPSocket *pRet = new CTCPSocket;
				if ( !pRet )
				{
					closesocket( newSock );
					return NULL;
				}

				pRet->m_Socket = newSock;
				pRet->SetInitialSocketOptions();
				pRet->SetupConnected();

				// Report the address..
				SockAddrToIPAddr( &addr, pAddr );

				return pRet;
			}
		}

		return NULL;
	}


private:
	SOCKET		m_Socket;	
};



ITCPListenSocket* CreateTCPListenSocket( const unsigned short port, int nQueueLength )
{
	return CTCPListenSocket::Create( port, nQueueLength );
}


ITCPSocket* CreateTCPSocket()
{
	return new CTCPSocket;
}


void TCPSocket_EnableTimeout( bool bEnable )
{
	g_bEnableTCPTimeout = bEnable;
}


// --------------------------------------------------------------------------------- //	
// This thread sends keepalives on all active TCP sockets.
// --------------------------------------------------------------------------------- //	

HANDLE							g_hKeepaliveThread;
HANDLE							g_hKeepaliveThreadSignal;
HANDLE							g_hKeepaliveThreadReply;
CUtlLinkedList<CTCPSocket*,int>	g_TCPSockets;
CCriticalSection				g_TCPSocketsCS;


DWORD WINAPI TCPKeepaliveThread( LPVOID pParameter )
{
	while ( 1 )
	{
		if ( WaitForSingleObject( g_hKeepaliveThreadSignal, KEEPALIVE_INTERVAL_MS ) == WAIT_OBJECT_0 )
			break;

		// Tell all TCP sockets to send a keepalive.
		CCriticalSectionLock csLock( &g_TCPSocketsCS );
		csLock.Lock();

		FOR_EACH_LL( g_TCPSockets, i )
		{
			g_TCPSockets[i]->SendKeepalive();
		}
	}

	SetEvent( g_hKeepaliveThreadReply );
	return 0;
}


void AddGlobalTCPSocket( CTCPSocket *pSocket )
{
	CCriticalSectionLock csLock( &g_TCPSocketsCS );
	csLock.Lock();
	
	Assert( g_TCPSockets.Find( pSocket ) == g_TCPSockets.InvalidIndex() );
	g_TCPSockets.AddToTail( pSocket );

	// If this is the first one, create the keepalive thread.
	if ( g_TCPSockets.Count() == 1 )
	{
		g_hKeepaliveThreadSignal = CreateEvent( NULL, false, false, NULL );
		g_hKeepaliveThreadReply = CreateEvent( NULL, false, false, NULL );

		DWORD dwThreadID = 0;
		g_hKeepaliveThread = CreateThread(
			NULL,
			0,
			TCPKeepaliveThread,
			NULL,
			0,
			&dwThreadID
			);
	}
}


void RemoveGlobalTCPSocket( CTCPSocket *pSocket )
{
	bool bThreadRunning = false;
	DWORD dwExitCode = 0;
	if ( GetExitCodeThread( g_hKeepaliveThread, &dwExitCode ) && dwExitCode == STILL_ACTIVE )
	{
		bThreadRunning = true;
	}

	CCriticalSectionLock csLock( &g_TCPSocketsCS );
	csLock.Lock();
	
	int index = g_TCPSockets.Find( pSocket );
	if ( index != g_TCPSockets.InvalidIndex() )
	{
		g_TCPSockets.Remove( index );

		// If this was the last one, delete the thread.
		if ( g_TCPSockets.Count() == 0 )
		{
			csLock.Unlock();

			if ( bThreadRunning )
			{
				SetEvent( g_hKeepaliveThreadSignal );
				WaitForSingleObject( g_hKeepaliveThreadReply, INFINITE );
			}

			CloseHandle( g_hKeepaliveThreadSignal );
			CloseHandle( g_hKeepaliveThreadReply );
			CloseHandle( g_hKeepaliveThread );
			return;
		}
	}

	csLock.Unlock();
}
