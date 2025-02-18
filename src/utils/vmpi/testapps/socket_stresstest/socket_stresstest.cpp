//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// socket_stresstest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "utllinkedlist.h"


class CSocketInfo
{
public:

	bool IsValid()
	{
		return m_pSocket != 0;
	}
	
	void Term();

	void ThreadFn();



public:
	ITCPSocket			*m_pSocket;
	int					m_iListenPort;

	DWORD				m_CreateTime;		// When this socket was created.
	DWORD				m_ExpireTime;
};



CSocketInfo g_Infos[132];
CRITICAL_SECTION g_CS, g_PrintCS;
HANDLE g_hThreads[ ARRAYSIZE( g_Infos ) ];
bool g_bShouldExit = false;
CUtlLinkedList<int,int> g_ListenPorts;


SpewRetval_t StressTestSpew( SpewType_t type, char const *pMsg )
{
	EnterCriticalSection( &g_PrintCS );
		printf( "%s", pMsg );
	LeaveCriticalSection( &g_PrintCS );

	if( type == SPEW_ASSERT )
		return SPEW_DEBUGGER;
	else if( type == SPEW_ERROR )
		return SPEW_ABORT;
	else
		return SPEW_CONTINUE;
}


void CSocketInfo::Term()
{
	if ( m_pSocket )
	{
		m_pSocket->Release();
		m_pSocket = 0;
	}
}


CSocketInfo* FindOldestSocketInfo( CSocketInfo *pInfos, int nInfos )
{
	int iOldest = 0;
	DWORD oldestTime = 0xFFFFFFFF;
	for ( int i=0; i < nInfos; i++ )
	{
		if ( !pInfos[i].IsValid() )
			return &pInfos[i];

		if ( pInfos[i].m_CreateTime < oldestTime )
		{
			oldestTime = pInfos[i].m_CreateTime;
			iOldest = i;
		}
	}
	return &pInfos[iOldest];
}


int g_iNextExpire = -1;

void CSocketInfo::ThreadFn()
{
	int iInfo = this - g_Infos;

	while ( !g_bShouldExit )
	{
		DWORD curTime = GetTickCount();

		// Break the connection after a certain amount of time.
		if ( m_pSocket && curTime >= m_ExpireTime )
		{
			Term();
			Msg( "%02d: expire.\n", iInfo, m_iListenPort );
		}

		if ( m_pSocket )
		{
			EnterCriticalSection( &g_CS );
				if ( g_iNextExpire == -1 )
				{
					g_iNextExpire = iInfo;
					LeaveCriticalSection( &g_CS );

					Msg( "%02d: forcing an expire.\n", iInfo, m_iListenPort );
					Sleep( 16000 );
					
					EnterCriticalSection( &g_CS );
						g_iNextExpire = -1;
				}
			LeaveCriticalSection( &g_CS );

			if ( m_pSocket->IsConnected() )
			{
				// Receive whatever data it has waiting for it.
				CUtlVector<unsigned char> data;
				while ( m_pSocket->Recv( data ) )
				{
					Msg( "%02d: recv %d.\n", iInfo, data.Count() );
				}

				// Send some data.
				int size = rand() % 8192;
				data.SetSize( size );
				m_pSocket->Send( data.Base(), data.Count() );
				//Msg( "%02d: send %d.\n", iInfo, data.Count() );
			}
			else
			{
				Term();
			}
		}
		else
		{
			// Not initialized.. either listen or connect.
			int iConnectPort = -1;
			if ( rand() > VALVE_RAND_MAX/2 )
			{
				if ( rand() % 100 < 50 )
					Sleep( 500 );

				EnterCriticalSection( &g_CS );
					int iHead = g_ListenPorts.Head();
					if ( iHead != g_ListenPorts.InvalidIndex() )
						iConnectPort = g_ListenPorts[iHead];
				LeaveCriticalSection( &g_CS );				
			}

			if ( iConnectPort != -1 )
			{
				CIPAddr addr( 127, 0, 0, 1, iConnectPort );
				
				m_pSocket = CreateTCPSocket();
				m_pSocket->BindToAny( 0 );
				m_CreateTime = curTime;
				m_ExpireTime = curTime + rand() % 5000;
				if ( !TCPSocket_Connect( m_pSocket, &addr, 3.0f ) )
				{
					Term();
				}
			}
			else
			{
				for ( int iTry=0; iTry < 32; iTry++ )
				{
					m_iListenPort = 100 + rand() % (VALVE_RAND_MAX/2);
					ITCPListenSocket *pListenSocket = CreateTCPListenSocket( m_iListenPort );
					if ( pListenSocket )
					{
						Msg( "%02d: listen on %d.\n", iInfo, m_iListenPort );

						// Add us to the list of ports to connect to.
						EnterCriticalSection( &g_CS );
							g_ListenPorts.AddToTail( m_iListenPort );
						LeaveCriticalSection( &g_CS );

						// Listen for a connection.
						CIPAddr connectedAddr;
						m_pSocket = TCPSocket_ListenForOneConnection( pListenSocket, &connectedAddr, 4.0 );

						// Remove us from the list of ports to connect to.
						EnterCriticalSection( &g_CS );
							g_ListenPorts.Remove( g_ListenPorts.Find( m_iListenPort ) );
						LeaveCriticalSection( &g_CS );

						pListenSocket->Release();

						if ( m_pSocket )
						{
							Msg( "%02d: listen found connection.\n", iInfo );
							m_CreateTime = curTime;
							m_ExpireTime = curTime + rand() % 5000;
						}
						break;
					}
				}
			}
		}

		Sleep( 1 );
	}

	g_hThreads[iInfo] = 0;
}


DWORD WINAPI ThreadFn( LPVOID lpParameter )
{
	CSocketInfo *pInfo = (CSocketInfo*)lpParameter;
	pInfo->ThreadFn();
	return 0;
}


void AllocError( unsigned long size )
{
	Assert( false );
}


int main(int argc, char* argv[])
{
	memset( g_Infos, 0, sizeof( g_Infos ) );
	memset( g_hThreads, 0, sizeof( g_hThreads ) );

	InitializeCriticalSection( &g_CS );
	InitializeCriticalSection( &g_PrintCS );

	SpewOutputFunc( StressTestSpew );
	Plat_SetAllocErrorFn( AllocError );

	SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );

	for ( int i=0; i < ARRAYSIZE( g_Infos ); i++ )
	{
		DWORD dwThreadID = 0;
		g_hThreads[i] = CreateThread( 
			NULL, 
			0,
			ThreadFn,
			&g_Infos[i],
			0,
			&dwThreadID );
	}


	while ( !kbhit() )
	{
	}

	g_bShouldExit = true;
	
	HANDLE hZeroArray[ ARRAYSIZE( g_Infos ) ];
	memset( hZeroArray, 0, sizeof( hZeroArray ) );

	while ( memcmp( hZeroArray, g_hThreads, sizeof( hZeroArray ) ) != 0 )
	{
		Sleep( 10 );
	}
	
	return 0;
}

