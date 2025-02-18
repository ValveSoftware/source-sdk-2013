//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <windows.h>
#include "messagemgr.h"
#include "tcpsocket.h"
#include "iphelpers.h"
#include "tier0/platform.h"
#include "threadhelpers.h"


#define MSGMGR_LISTEN_PORT_FIRST			22512
#define MSGMGR_LISTEN_PORT_LAST				22520



#define BROADCAST_INTERVAL	2	// Broadcast our presence every N seconds.

#define NUM_QUEUED_MESSAGES	200



class CMessageMgr : public IMessageMgr
{
public:
					CMessageMgr();
					~CMessageMgr();

	bool			Init();
	void			Term();


// IMessageMgr overrides.
public:
	
	virtual void	Print( const char *pMsg );



private:

	DWORD				ThreadFn();
	static DWORD WINAPI StaticThreadFn( LPVOID pParameter );


private:

	// Only our thread touches this, NOT the main thread.
	CUtlLinkedList<ITCPSocket*,int>	m_Sockets;

	HANDLE							m_hThread;
	DWORD							m_dwThreadID;

	HANDLE							m_hExitObj;			// This is signalled when we want the thread to exit.
	HANDLE							m_hExitResponseObj;	// The thread sets this when it exits.
	
	HANDLE							m_hMessageObj;		// This signals to the thread that there's a message to send.
	HANDLE							m_hMessageSentObj;	// This signals back to the main thread that the message was sent.
	const char						*m_pMessageText;	// The text to send.

	// This is only touched by the thread.
	CUtlLinkedList<char*,int>		m_MessageQ;			// FIFO of NUM_QUEUED_MESSAGES.

	ITCPListenSocket				*m_pListenSocket;
	int								m_iListenPort;
	
	ISocket							*m_pBroadcastSocket;
	double							m_flLastBroadcast;

};



CMessageMgr::CMessageMgr()
{
	m_pBroadcastSocket = NULL;
	m_pListenSocket = NULL;
	m_hThread = NULL;
	m_hExitObj = m_hExitResponseObj = m_hMessageObj = m_hMessageSentObj = NULL;
}


CMessageMgr::~CMessageMgr()
{
	Term();
}


bool CMessageMgr::Init()
{
	m_hExitObj = CreateEvent( NULL, false, false, NULL );
	m_hExitResponseObj = CreateEvent( NULL, false, false, NULL );
	m_hMessageObj = CreateEvent( NULL, false, false, NULL );
	m_hMessageSentObj = CreateEvent( NULL, false, false, NULL );
	if ( !m_hExitObj || !m_hExitResponseObj || !m_hMessageObj || !m_hMessageSentObj )
		return false;

	// Create the broadcast socket.
	m_pBroadcastSocket = CreateIPSocket();
	if ( !m_pBroadcastSocket )
		return false;

	if ( !m_pBroadcastSocket->BindToAny( 0 ) )
		return false;

	
	// Create the listen socket.
	m_pListenSocket = NULL;
	for ( m_iListenPort=MSGMGR_LISTEN_PORT_FIRST; m_iListenPort <= MSGMGR_LISTEN_PORT_LAST; m_iListenPort++ )
	{
		m_pListenSocket = CreateTCPListenSocket( m_iListenPort );
		if ( m_pListenSocket )
			break;
	}
	if ( !m_pListenSocket )
		return false;


	// Create our broadcast/connection thread.
	m_flLastBroadcast = 0;
	m_hThread = CreateThread( 
		NULL,
		0,
		&CMessageMgr::StaticThreadFn,
		this,
		0,
		&m_dwThreadID );

	if ( !m_hThread )
		return false;

	ThreadSetDebugName( m_dwThreadID, "MessageMgr" );
	return true;
}


void CMessageMgr::Term()
{
	// Wait for the thread to exit?
	if ( m_hThread )
	{
		DWORD dwExitCode = 0;
		if ( GetExitCodeThread( m_hThread, &dwExitCode ) && dwExitCode == STILL_ACTIVE )
		{
			SetEvent( m_hExitObj );
			WaitForSingleObject( m_hExitResponseObj, INFINITE );
		}
		
		CloseHandle( m_hThread );
		m_hThread = NULL;
	}

	CloseHandle( m_hExitObj );
	m_hExitObj = NULL;

	CloseHandle( m_hExitResponseObj );
	m_hExitResponseObj = NULL;

	CloseHandle( m_hMessageObj );
	m_hMessageObj = NULL;

	CloseHandle( m_hMessageSentObj );
	m_hMessageSentObj = NULL;

	if ( m_pListenSocket )
	{
		m_pListenSocket->Release();
		m_pListenSocket = NULL;
	}

	if ( m_pBroadcastSocket )
	{
		m_pBroadcastSocket->Release();
		m_pBroadcastSocket = NULL;
	}
}


void CMessageMgr::Print( const char *pMsg )
{
	m_pMessageText = pMsg;
	SetEvent( m_hMessageObj );
	WaitForSingleObject( m_hMessageSentObj, INFINITE );
}


DWORD CMessageMgr::ThreadFn()
{
	while ( 1 )
	{
		// Broadcast our presence?
		double flCurTime = Plat_FloatTime();
		if ( flCurTime - m_flLastBroadcast >= BROADCAST_INTERVAL )
		{
			// Broadcast our presence.
			char msg[9];
			msg[0] = MSGMGR_PACKETID_ANNOUNCE_PRESENCE;
			*((int*)&msg[1]) = MSGMGR_VERSION;
			*((int*)&msg[5]) = m_iListenPort;
			m_pBroadcastSocket->Broadcast( msg, sizeof( msg ), MSGMGR_BROADCAST_PORT );

			m_flLastBroadcast = flCurTime;
		}

		
		// Accept new connections.
		CIPAddr addr;
		ITCPSocket *pConn = m_pListenSocket->UpdateListen( &addr );
		if ( pConn )
		{
			// Send what's in our queue.
			FOR_EACH_LL( m_MessageQ, iQ )
			{
				char *pMsg = m_MessageQ[iQ];
				int bufLen = strlen( pMsg ) + 1;
				
				char packetID = MSGMGR_PACKETID_MSG;
				const void *data[2] = { &packetID, pMsg };
				int len[2] = { 1, bufLen };

				// Send it out to our sockets.
				pConn->SendChunks( data, len, 2 );
			}				

			m_Sockets.AddToTail( pConn );
		}

		
		// Should we exit?
		HANDLE handles[2] = {m_hExitObj, m_hMessageObj};
		DWORD ret = WaitForMultipleObjects( 2, handles, FALSE, 200 );
		if ( ret == WAIT_OBJECT_0 )
		{
			break;
		}
		else if ( ret == (WAIT_OBJECT_0+1) )
		{
			// Add it to the queue.
			int index;
			if ( m_MessageQ.Count() >= NUM_QUEUED_MESSAGES )
			{
				index = m_MessageQ.Tail();
				delete m_MessageQ[index];
			}
			else
			{
				index = m_MessageQ.AddToTail();
			}
			int bufLen = strlen( m_pMessageText ) + 1;
			m_MessageQ[index] = new char[ bufLen ];
			strcpy( m_MessageQ[index], m_pMessageText );


			
			// Ok, send out the message.
			char packetID = MSGMGR_PACKETID_MSG;
			const void *data[2] = { &packetID, m_pMessageText };
			int len[2] = { 1, bufLen };

			// Send it out to our sockets.
			FOR_EACH_LL( m_Sockets, i )
			{
				m_Sockets[i]->SendChunks( data, len, 2 );
			}

			// Notify the main thread that we've sent it.
			SetEvent( m_hMessageSentObj );
		}
	}

	// Cleanup all our sockets (the main thread should never touch them).
	FOR_EACH_LL( m_Sockets, i )
		m_Sockets[i]->Release();

	m_Sockets.Purge();
	
	m_MessageQ.PurgeAndDeleteElements();

	SetEvent( m_hExitResponseObj );
	return 0;
}


DWORD CMessageMgr::StaticThreadFn( LPVOID pParameter )
{
	return ((CMessageMgr*)pParameter)->ThreadFn();
}


static CMessageMgr g_MessageMgr;

IMessageMgr* GetMessageMgr()
{
	return &g_MessageMgr;
}

