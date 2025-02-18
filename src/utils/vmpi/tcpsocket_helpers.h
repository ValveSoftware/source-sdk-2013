//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TCPSOCKET_HELPERS_H
#define TCPSOCKET_HELPERS_H
#ifdef _WIN32
#pragma once
#endif


#include "tcpsocket.h"


bool TCPSocket_Connect( ITCPSocket *pSocket, const CIPAddr *pAddr, double flTimeout );
ITCPSocket* TCPSocket_ListenForOneConnection( ITCPListenSocket *pSocket, CIPAddr *pAddr, double flTimeout );


#endif // TCPSOCKET_HELPERS_H
