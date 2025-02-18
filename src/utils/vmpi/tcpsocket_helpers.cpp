//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <windows.h>
#include "tcpsocket_helpers.h"


// This connects to an ISocket listening with Listen(). 
bool TCPSocket_Connect( ITCPSocket *pSocket, const CIPAddr *pAddr, double flTimeout )
{
	pSocket->BeginConnect( *pAddr );
	
	CWaitTimer waitTimer( flTimeout );
	while ( 1 )
	{
		if ( pSocket->UpdateConnect() )
			return true;
	
		if ( waitTimer.ShouldKeepWaiting() )
			Sleep( 10 );
		else
			break;
	}

	return false;
}


ITCPSocket* TCPSocket_ListenForOneConnection( ITCPListenSocket *pSocket, CIPAddr *pAddr, double flTimeout )
{
	CWaitTimer waitTimer( flTimeout );
	while ( 1 )
	{
		ITCPSocket *pRet = pSocket->UpdateListen( pAddr );
		if ( pRet )
			return pRet;

		if ( waitTimer.ShouldKeepWaiting() )
			Sleep( 10 );
		else
			break;
	}

	return NULL;
}
