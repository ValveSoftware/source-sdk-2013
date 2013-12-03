//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef COMPRESSED_LIGHT_CUBE_H
#define COMPRESSED_LIGHT_CUBE_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib/mathlib.h"


struct CompressedLightCube
{
	DECLARE_BYTESWAP_DATADESC();
	ColorRGBExp32 m_Color[6];
};


#endif // COMPRESSED_LIGHT_CUBE_H
