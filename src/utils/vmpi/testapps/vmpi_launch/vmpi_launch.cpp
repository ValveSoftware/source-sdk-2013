//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// vmpi_launch.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "iphelpers.h"
#include "bitbuf.h"
#include "vmpi.h"

bool g_bBroadcast = false;

int PrintUsage()
{
	printf( "vmpi_launch -machine <remote machine> -priority <priority> [-mpi_pw <password>] -command \"command line...\"\n" );
	printf( "-command must be the last switch..\n" );
	return 1;
}


int GetCurMicrosecondsAndSleep( int sleepLen )
{
	Sleep( sleepLen );

	int retVal;
	__asm
	{
		rdtsc
		mov		retVal,   eax
	}
	return retVal;
}


const char* FindArg( int argc, char **argv, const char *pName, const char *pDefault )
{
	for ( int i=0; i < argc; i++ )
	{
		if ( stricmp( argv[i], pName ) == 0 )
		{
			if ( (i+1) < argc )
				return argv[i+1];
			else
				return pDefault;
		}
	}
	return NULL;
}


int ParseArgs( int argc, char **argv, CIPAddr &remoteIP, int &iPriority, int &iFirstArg )
{
	if ( FindArg( argc, argv, "-broadcast", "1" ) )
		 g_bBroadcast = true;

	if ( g_bBroadcast == false )
	{
		const char *pRemoteIPStr = FindArg( argc, argv, "-machine", NULL );
		if ( !pRemoteIPStr || !ConvertStringToIPAddr( pRemoteIPStr, &remoteIP ) )
		{
			printf( "%s is not a valid machine name or IP address.\n", pRemoteIPStr );
			return PrintUsage();
		}
	}
	
	iPriority = 0;
	const char *pPriorityStr = FindArg( argc, argv, "-priority", NULL );
	if ( pPriorityStr )
		iPriority = atoi( pPriorityStr );
	
	if ( iPriority < 0 || iPriority > 1000 )
	{
		printf( "%s is not a valid priority.\n", pPriorityStr );
		return PrintUsage();
	}

	const char *pCommand = FindArg( argc, argv, "-command", NULL );
	if ( !pCommand )
	{
		return PrintUsage();
	}
	for ( iFirstArg=1; iFirstArg < argc; iFirstArg++ )
	{
		if ( argv[iFirstArg] == pCommand )
			break;
	}

	return 0;
}


void SendJobRequest( 
	ISocket *pSocket,
	int argc, 
	char **argv, 
	CIPAddr &remoteIP, 
	int &iPriority, 
	int &iFirstArg,
	int jobID[4] )
{
	// Build the packet to send out the job.
	char packetData[4096];
	bf_write packetBuf;
	
	// Come up with a unique job ID.
	jobID[0] = GetCurMicrosecondsAndSleep( 1 );
	jobID[1] = GetCurMicrosecondsAndSleep( 1 );
	jobID[2] = GetCurMicrosecondsAndSleep( 1 );
	jobID[3] = GetCurMicrosecondsAndSleep( 1 );

	
	// Broadcast out to tell all the machines we want workers.
	packetBuf.StartWriting( packetData, sizeof( packetData ) );
	packetBuf.WriteByte( VMPI_PROTOCOL_VERSION );

	const char *pPassword = FindArg( argc, argv, "-mpi_pw", "" );
	packetBuf.WriteString( pPassword );
	
	packetBuf.WriteByte( VMPI_LOOKING_FOR_WORKERS );
	
	packetBuf.WriteShort( 0 );	// Tell the port that we're listening on.
								// In this case, there is no VMPI master waiting for the app to connect, so
								// this parameter doesn't matter.
	packetBuf.WriteShort( iPriority );

	packetBuf.WriteLong( jobID[0] );
	packetBuf.WriteLong( jobID[1] );
	packetBuf.WriteLong( jobID[2] );
	packetBuf.WriteLong( jobID[3] );
	packetBuf.WriteWord( argc-iFirstArg ); // 1 command line argument..

	// Write the alternate exe name.
	for ( int iArg=iFirstArg; iArg < argc; iArg++ )
		packetBuf.WriteString( argv[iArg] );

	for ( int iBroadcastPort=VMPI_SERVICE_PORT; iBroadcastPort <= VMPI_LAST_SERVICE_PORT; iBroadcastPort++ )
	{
		remoteIP.port = iBroadcastPort;

		if ( g_bBroadcast == false )
			 pSocket->SendTo( &remoteIP, packetBuf.GetBasePointer(), packetBuf.GetNumBytesWritten() );
		else
			 pSocket->Broadcast( packetBuf.GetBasePointer(), packetBuf.GetNumBytesWritten(), iBroadcastPort );
	}

	if ( g_bBroadcast == false )
		 printf( "Sent command, waiting for reply...\n" );
	else
		 printf( "Sent command\n" );
}


bool WaitForJobStart( ISocket *pSocket, const CIPAddr &remoteIP, const int jobID[4] )
{
	while ( 1 )
	{
		CIPAddr senderAddr;
		char data[4096];
		int len = -1;
		
		if ( g_bBroadcast == false )
			 pSocket->RecvFrom( data, sizeof( data ), &senderAddr );
		else 
			 pSocket->RecvFrom( data, sizeof( data ), NULL );

		if ( len == 19 && 
			memcmp( senderAddr.ip, remoteIP.ip, sizeof( senderAddr.ip ) ) == 0 &&
			data[1] == VMPI_NOTIFY_START_STATUS &&
			memcmp( &data[2], jobID, 16 ) == 0 )
		{
			if ( data[18] == 0 )
			{
				// Wasn't able to run.
				printf( "Wasn't able to run on target machine.\n" );
				return false;
			}
			else
			{
				// Ok, the process is running now.
				printf( "Process running, waiting for completion...\n" );
				return true;
			}
		}

		Sleep( 100 );
	}		
}


void WaitForJobEnd( ISocket *pSocket, const CIPAddr &remoteIP, const int jobID[4] )
{
	while ( 1 )
	{
		CIPAddr senderAddr;
		char data[4096];
		int len = pSocket->RecvFrom( data, sizeof( data ), &senderAddr );
		if ( len == 18 && 
			memcmp( senderAddr.ip, remoteIP.ip, sizeof( senderAddr.ip ) ) == 0 &&
			data[1] == VMPI_NOTIFY_END_STATUS &&
			memcmp( &data[2], jobID, 16 ) == 0 )
		{
			int ret = *((int*)&data[2]);
			printf( "Finished [%d].\n", ret );
			break;
		}

		Sleep( 100 );
	}		
}


int main(int argc, char* argv[])
{
	if ( argc < 4 )
	{
		return PrintUsage();
	}

	
	// Parse the command line.
	CIPAddr remoteIP;
	int iFirstArg, iPriority;
	int jobID[4];

	int ret = ParseArgs( argc, argv, remoteIP, iPriority, iFirstArg );
	if ( ret != 0 )
		return ret;

	// Now send the command to the vmpi service on that machine.
	ISocket *pSocket = CreateIPSocket();
	if ( !pSocket->BindToAny( 0 ) )
	{
		printf( "Error binding a socket.\n" );
		return 1;
	}
	
	SendJobRequest( pSocket, argc, argv, remoteIP, iPriority, iFirstArg, jobID );

	// Wait for a reply, positive or negative.
	if ( g_bBroadcast == false )
	{
		if ( !WaitForJobStart( pSocket, remoteIP, jobID ) )
			return 2;

		WaitForJobEnd( pSocket, remoteIP, jobID );
	}
	
	pSocket->Release();
	return 0;
}

