//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISP_VRAD_H
#define DISP_VRAD_H
#ifdef _WIN32
#pragma once
#endif


#include "builddisp.h"


// Blend the normals of neighboring displacement surfaces so they match at edges and corners.
void SmoothNeighboringDispSurfNormals( CCoreDispInfo **ppListBase, int listSize );


#endif // DISP_VRAD_H
