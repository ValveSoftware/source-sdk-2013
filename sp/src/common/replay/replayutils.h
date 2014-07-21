//========= Copyright Valve Corporation, All rights reserved. ============//
//
//----------------------------------------------------------------------------------------

#ifndef REPLAYUTILS_H
#define REPLAYUTILS_H
#ifdef _WIN32
#pragma once
#endif

#include "utlstring.h"

void Replay_GetFirstAvailableFilename( char *pDst, int nDstLen, const char *pIdealFilename, const char *pExt,
									   const char *pFilePath, int nStartIndex );

void Replay_ConstructReplayFilenameString( CUtlString &strOut, const char *pReplaySubDir, const char *pFilename, const char *pGameDir );

//----------------------------------------------------------------------------------------
// Util function, copied from src/engine/common.cpp
//----------------------------------------------------------------------------------------
char *Replay_va( PRINTF_FORMAT_STRING const char *format, ... );

//----------------------------------------------------------------------------------------
// Return the base dir, e.g. "c:\...\game\tf\replays\"
//----------------------------------------------------------------------------------------
const char *Replay_GetBaseDir();

//----------------------------------------------------------------------------------------
// Set the game directory (only to be called from ReplayLib_Init())
//----------------------------------------------------------------------------------------
void Replay_SetGameDir( const char *pGameDir );

//----------------------------------------------------------------------------------------
// Return the base dir, e.g. "c:\...\game\tf\replays\"
//----------------------------------------------------------------------------------------
const char *Replay_GetGameDir();

//----------------------------------------------------------------------------------------
// Get a name of the format "<map>: <current date & time>" - used for replays and takes.
//----------------------------------------------------------------------------------------
void Replay_GetAutoName( OUT_Z_BYTECAP(nDestSizeInBytes) wchar_t *pDest, int nDestSizeInBytes, const char *pMapName );

#endif // REPLAY_H
