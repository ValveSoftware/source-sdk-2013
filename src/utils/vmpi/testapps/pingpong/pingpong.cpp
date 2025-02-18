//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// pingpong.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>
#include <stdlib.h>
#include "tcpsocket.h"
#include "tier0/fasttimer.h"
#include "vmpi.h"
#include "tcpsocket_helpers.h"

//#define USE_MPI


#if defined( USE_MPI )
	#include "mpi/mpi.h"
	#include "vmpi.h"
	#include "tier1/bitbuf.h"

	int myProcId = -1;
#else
	IChannel *g_pSocket = NULL;
	int g_iPortNum = 27141;
#endif


int PrintUsage()
{
	printf( "pingpong <-server or -client ip>\n" );
	return 1;
}


void DoClientConnect( const char *pIP )
{
#if defined( USE_MPI )
	int argc = 1;
	char *testargv[1] = { "-nounc" };
	char **argv = testargv;
	if ( MPI_Init( &argc, &argv ) )
	{
		assert( false );
	}
	MPI_Comm_rank( MPI_COMM_WORLD, &myProcId );

	int nProcs;
	MPI_Comm_size( MPI_COMM_WORLD, &nProcs );
	if ( nProcs != 2 )
	{
		assert( false );
	}
#else
	// Try to connect, or listen.
	ITCPSocket *pTCPSocket = CreateTCPSocket();
	if ( !pTCPSocket->BindToAny( 0 ) )
	{
		assert( false );
	}

	CIPAddr addr;
	if ( !ConvertStringToIPAddr( pIP, &addr ) )
	{
		assert( false );
	}

	addr.port = g_iPortNum;
	printf( "Client connecting to %d.%d.%d.%d:%d\n", addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3], addr.port );
	if ( !TCPSocket_Connect( pTCPSocket, &addr, 50000 ) )
	{
		assert( false );
	}

	printf( "Client connected...\n ");
	g_pSocket = pTCPSocket;
#endif
}


void DoServerConnect()
{
#if defined( USE_MPI )
	ISocket *pSocket = CreateIPSocket();
	if ( !pSocket )
	{
		printf( "Error creating a socket.\n" );
		assert( false );
		return;
	}
	else if ( !pSocket->BindToAny( VMPI_SERVICE_PORT ) )
	{	
		printf( "Error binding a socket to port %d.\n", VMPI_SERVICE_PORT );
		assert( false );
		return;
	}

	printf( "Waiting for jobs...\n" );
	while ( 1 )
	{
		// Any incoming packets?
		char data[2048];
		CIPAddr ipFrom;
		int len = pSocket->RecvFrom( data, sizeof( data ), &ipFrom );
		if ( len > 3 )
		{
			bf_read buf( data, len );
			if ( buf.ReadByte() == VMPI_PROTOCOL_VERSION )
			{
				if ( buf.ReadByte() == VMPI_LOOKING_FOR_WORKERS )
				{
					// Read the listen port.
					int iListenPort = buf.ReadLong();

					static char ipString[128];
					_snprintf( ipString, sizeof( ipString ), "%d.%d.%d.%d:%d", ipFrom.ip[0], ipFrom.ip[1], ipFrom.ip[2], ipFrom.ip[3], iListenPort );
					
					int argc = 3;
					char *testargv[3];
					testargv[0] = "<supposedly the executable name!>";
					testargv[1] = "-mpi_worker";
					testargv[2] = ipString;
					
					char **argv = testargv;
					if ( MPI_Init( &argc, &argv ) )
					{
						assert( false );
					}
					MPI_Comm_rank( MPI_COMM_WORLD, &myProcId );

					int nProcs;
					MPI_Comm_size( MPI_COMM_WORLD, &nProcs );
					if ( nProcs != 2 )
					{
						assert( false );
					}
					break;
				}
			}
		}

		Sleep( 100 );		
	}

	pSocket->Release();
#else
	// Try to connect, or listen.
	ITCPListenSocket *pListen = CreateTCPListenSocket( g_iPortNum );
	if ( !pListen )
	{
		assert( false );
	}

	printf( "Server listening...\n" );

	CIPAddr addr;
	ITCPSocket *pTCPSocket = TCPSocket_ListenForOneConnection( pListen, &addr, 50000 );
	if ( !pTCPSocket )
	{
		assert( false );
	}
	pListen->Release();

	printf( "Server connected...\n ");
	g_pSocket = pTCPSocket;
#endif
}


void SendData( const void *pBuf, int size )
{
#if defined( USE_MPI )
    MPI_Send( (void*)pBuf, size, MPI_BYTE, !myProcId, 0, MPI_COMM_WORLD );
#else
	g_pSocket->Send( pBuf, size );
#endif
}


void RecvData( CUtlVector<unsigned char> &recvBuf )
{
#if defined( USE_MPI )
	MPI_Status stat;
	MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
	
	recvBuf.SetCount( stat.count );
	MPI_Recv( recvBuf.Base(), stat.count, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
#else
	if ( !g_pSocket->Recv( recvBuf, 50000 ) )
	{
		g_pSocket->Release();
		g_pSocket = NULL;
	}
#endif
}


int main( int argc, char* argv[] )
{
	if ( argc < 2 )
	{
		return PrintUsage();
	}

	const char *pClientOrServer = argv[1];
	const char *pIP = NULL;

	bool bClient = false;
	if ( stricmp( pClientOrServer, "-client" ) == 0 )
	{
		if ( argc < 3 )
		{
			return PrintUsage();
		}

		bClient = true;
		pIP = argv[2];
	}

	CUtlVector<unsigned char> recvBuf;
	if ( bClient )
	{
		DoClientConnect( pIP );

		// Ok, now start blasting packets of different sizes and measure how long it takes to get an ack back.
		int nIterations = 30;
		
		for ( int size=350; size <= 350000; size += 512 )
		{
			CUtlVector<unsigned char> buf;
			buf.SetCount( size );

			double flTotalRoundTripTime = 0;

			CFastTimer throughputTimer;
			throughputTimer.Start();

			for ( int i=0; i < nIterations; i++ )
			{
				for ( int z=0; z < size; z++ )
					buf[z] = (char)rand();
				
				SendData( buf.Base(), buf.Count() );
				
				CFastTimer timer;
				timer.Start();
					RecvData( recvBuf );
				timer.End();

				
				// Make sure we got the same data back.
				assert( recvBuf.Count() == buf.Count() );
				for ( z=0; z < size; z++ )
				{
					assert( recvBuf[z] == buf[z] );
				}
					

				//if ( i % 100 == 0 )
				//	printf( "%05d\n", i );
printf( "%d\n", i );
				flTotalRoundTripTime += timer.GetDuration().GetMillisecondsF();
			}
			throughputTimer.End();
			double flTotalSeconds = throughputTimer.GetDuration().GetSeconds();

			double flAvgRoundTripTime = flTotalRoundTripTime / nIterations;
			printf( "%d: %.2f ms per roundtrip (%d bytes/sec) sec: %.2f megs: %.2f\n", 
				size, 
				flAvgRoundTripTime, 
				(int)((size*nIterations)/flTotalSeconds),
				flTotalSeconds,
				(double)(size*nIterations) / (1024*1024)  );
		}

		// Send an 'end' message to the server.
		int val = -1;
		SendData( &val, sizeof( val ) );
	}
	else
	{
		// Wait for a connection.
		DoServerConnect();

		// Wait for packets and ack them.
		while ( 1 )
		{
			RecvData( recvBuf );
			if ( !g_pSocket )
				break;

			if ( recvBuf.Count() < 4 )
			{
				assert( false );
			}

			SendData( recvBuf.Base(), recvBuf.Count() );
		}
	}

	return 0;
}

