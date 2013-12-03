//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL2_PLAYER_SHARED_H
#define HL2_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Shared header file for players
#if defined( CLIENT_DLL )
#define CHL2_Player C_BaseHLPlayer	//FIXME: Lovely naming job between server and client here...
#include "c_basehlplayer.h"
#else
#include "hl2_player.h"
#endif

#endif // HL2_PLAYER_SHARED_H
