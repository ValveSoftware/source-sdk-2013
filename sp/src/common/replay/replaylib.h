//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYLIB_H
#define REPLAYLIB_H
#ifdef _WIN32
#pragma once
#endif

//----------------------------------------------------------------------------------------

class IClientReplayContext;

//----------------------------------------------------------------------------------------

bool ReplayLib_Init( const char *pGameDir, IClientReplayContext *pClientReplayContext );

//----------------------------------------------------------------------------------------

#endif // REPLAYLIB_H