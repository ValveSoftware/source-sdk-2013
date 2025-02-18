//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// ThreadedTCPSocketTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "IThreadedTCPSocket.h"
#include "threadhelpers.h"
#include "vstdlib/random.h"


CCriticalSection g_MsgCS;


IThreadedTCPSocket *g_pClientSocket = NULL;
IThreadedTCPSocket *g_pServerSocket = NULL;

CEvent g_ClientPacketEvent;
CUtlVector<char> g_ClientPacket;



SpewRetval_t MySpewFunc( SpewType_t type, char const *pMsg )
{
	CCriticalSectionLock csLock( &g_MsgCS );
	csLock.Lock();

		printf( "%s", pMsg );
		OutputDebugString( pMsg );

	csLock.Unlock();	

	if( type == SPEW_ASSERT )
		return SPEW_DEBUGGER;
	else if( type == SPEW_ERROR )
		return SPEW_ABORT;
	else
		return SPEW_CONTINUE;
}


class CHandler_Server : public ITCPSocketHandler
{
public:
	virtual void Init( IThreadedTCPSocket *pSocket )
	{
	}
	
	virtual void OnPacketReceived( CTCPPacket *pPacket )
	{
		// Echo the data back.
		g_pServerSocket->Send( pPacket->GetData(), pPacket->GetLen() );
		pPacket->Release();
	}

	virtual void OnError( int errorCode, const char *pErrorString )
	{
		Msg( "Server error: %s\n", pErrorString );
	}
};



class CHandler_Client : public ITCPSocketHandler
{
public:
	virtual void Init( IThreadedTCPSocket *pSocket )
	{
	}
	
	virtual void OnPacketReceived( CTCPPacket *pPacket )
	{
		if ( g_ClientPacket.Count() < pPacket->GetLen() )
			g_ClientPacket.SetSize( pPacket->GetLen() );
		
		memcpy( g_ClientPacket.Base(), pPacket->GetData(), pPacket->GetLen() );
		g_ClientPacketEvent.SetEvent();
		pPacket->Release();
	}

	virtual void OnError( int errorCode, const char *pErrorString )
	{
		Msg( "Client error: %s\n", pErrorString );
	}
};



class CHandlerCreator_Server : public IHandlerCreator
{
public:
	virtual ITCPSocketHandler* CreateNewHandler()
	{
		return new CHandler_Server;
	}
};

class CHandlerCreator_Client : public IHandlerCreator
{
public:
	virtual ITCPSocketHandler* CreateNewHandler()
	{
		return new CHandler_Client;
	}
};



int main(int argc, char* argv[])
{
	SpewOutputFunc( MySpewFunc );

	// Figure out a random port to use.
	CCycleCount cnt;
	cnt.Sample();
	CUniformRandomStream randomStream;
	randomStream.SetSeed( cnt.GetMicroseconds() );
	int iPort = randomStream.RandomInt( 20000, 30000 );


	g_ClientPacketEvent.Init( false, false );

	
	// Setup the "server".
	CHandlerCreator_Server serverHandler;
	CIPAddr addr( 127, 0, 0, 1, iPort );

	ITCPConnectSocket *pListener = ThreadedTCP_CreateListener( 
		&serverHandler,
		(unsigned short)iPort );

	
	// Setup the "client".
	CHandlerCreator_Client clientCreator;
	ITCPConnectSocket *pConnector = ThreadedTCP_CreateConnector( 
		CIPAddr( 127, 0, 0, 1, iPort ),
		CIPAddr(),
		&clientCreator );


	// Wait for them to connect.
	while ( !g_pClientSocket )
	{
		if ( !pConnector->Update( &g_pClientSocket ) )
		{
			Error( "Error in client connector!\n" );
		}
	}
	pConnector->Release();


	while ( !g_pServerSocket )
	{
		if ( !pListener->Update( &g_pServerSocket ) )
			Error( "Error in server connector!\n" );
	}
	pListener->Release();


	// Send some data.
	__int64 totalBytes = 0;
	CCycleCount startTime;
	int iPacket = 1;

	startTime.Sample();
	CUtlVector<char> buf;

	while ( (GetAsyncKeyState( VK_SHIFT ) & 0x8000) == 0 )
	{
		int size = randomStream.RandomInt( 1024*0, 1024*320 );
		if ( buf.Count() < size )
			buf.SetSize( size );

		if ( g_pClientSocket->Send( buf.Base(), size ) )
		{
			// Server receives the data and echoes it back. Verify that the data is good.
			WaitForSingleObject( g_ClientPacketEvent.GetEventHandle(), INFINITE );
			Assert( memcmp( g_ClientPacket.Base(), buf.Base(), size ) == 0 );

			totalBytes += size;
			CCycleCount curTime, elapsed;
			curTime.Sample();
			CCycleCount::Sub( curTime, startTime, elapsed );
			double flSeconds = elapsed.GetSeconds();
			Msg( "Packet %d, %d bytes, %dk/sec\n", iPacket++, size, (int)(((totalBytes+511)/1024) / flSeconds) );
		}
	}
	
	g_pClientSocket->Release();
	g_pServerSocket->Release();
	return 0;
}

