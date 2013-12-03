//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#ifndef TGALOADER_H
#define TGALOADER_H

#ifdef _WIN32
#pragma once
#endif

#include "bitmap/imageformat.h"
#include "tier1/utlmemory.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;


namespace TGALoader
{

int TGAHeaderSize();

bool GetInfo( const char *fileName, int *width, int *height, ImageFormat *imageFormat, float *sourceGamma );
bool GetInfo( CUtlBuffer &buf, int *width, int *height, ImageFormat *imageFormat, float *sourceGamma );

bool Load( unsigned char *imageData, const char *fileName, int width, int height, 
		   ImageFormat imageFormat, float targetGamma, bool mipmap );
bool Load( unsigned char *imageData, CUtlBuffer &buf, int width, int height, 
			ImageFormat imageFormat, float targetGamma, bool mipmap );

bool LoadRGBA8888( const char *pFileName, CUtlMemory<unsigned char> &outputData, int &outWidth, int &outHeight );
bool LoadRGBA8888( CUtlBuffer &buf, CUtlMemory<unsigned char> &outputData, int &outWidth, int &outHeight );

} // end namespace TGALoader

#endif // TGALOADER_H
