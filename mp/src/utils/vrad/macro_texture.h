//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MACRO_TEXTURE_H
#define MACRO_TEXTURE_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib/vector.h"


// The macro texture looks for a TGA file with the same name as the BSP file and in
// the same directory. If it finds one, it maps this texture onto the world dimensions
// (in the worldspawn entity) and masks all lightmaps with it.
void InitMacroTexture( const char *pBSPFilename );
void ApplyMacroTextures( int iFace, const Vector &vWorldPos, Vector &outLuxel );


#endif // MACRO_TEXTURE_H
