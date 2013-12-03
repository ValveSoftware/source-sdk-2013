//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Helper methods + classes for sound
//
//===========================================================================//

#ifndef SOUNDUTILS_H
#define SOUNDUTILS_H

#if defined( _WIN32 )
#pragma once
#endif

#include "tier2/riff.h"


//-----------------------------------------------------------------------------
// RIFF reader/writers that use the file system
//-----------------------------------------------------------------------------
extern IFileReadBinary *g_pFSIOReadBinary;
extern IFileWriteBinary *g_pFSIOWriteBinary;


//-----------------------------------------------------------------------------
// Returns the duration of a wav file
//-----------------------------------------------------------------------------
float GetWavSoundDuration( const char *pWavFile );


#endif // SOUNDUTILS_H

