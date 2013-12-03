//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef SURFINFO_H
#define SURFINFO_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vplane.h"

//#include "mathlib/vector.h"
#define MAX_SURFINFO_VERTS	16
class SurfInfo
{
public:
	// Shape of the surface.
	Vector			m_Verts[ MAX_SURFINFO_VERTS ];
	unsigned long	m_nVerts;

	// Plane of the surface.
	VPlane			m_Plane;

	// For engine use only..
	void			*m_pEngineData;
};

#endif // SURFINFO_H
