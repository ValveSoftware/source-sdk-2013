//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef LOOPBACK_CHANNEL_H
#define LOOPBACK_CHANNEL_H
#ifdef _WIN32
#pragma once
#endif


#include "ichannel.h"


// Loopback sockets receive the same data they send.
IChannel* CreateLoopbackChannel();


#endif // LOOPBACK_CHANNEL_H
