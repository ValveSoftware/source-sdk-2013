//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef THREADEDTCPSOCKETEMU_H
#define THREADEDTCPSOCKETEMU_H
#ifdef _WIN32
#pragma once
#endif


#include "tcpsocket.h"


// This creates a class that's based on IThreadedTCPSocket, but emulates the old ITCPSocket interface.
// This is used for stress-testing IThreadedTCPSocket.
ITCPSocket* CreateTCPSocketEmu();


ITCPListenSocket* CreateTCPListenSocketEmu( const unsigned short port, int nQueueLength = -1 );


#endif // THREADEDTCPSOCKETEMU_H
