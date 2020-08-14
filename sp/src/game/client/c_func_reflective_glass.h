//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef C_FUNC_REFLECTIVE_GLASS
#define C_FUNC_REFLECTIVE_GLASS

#ifdef _WIN32
#pragma once
#endif

struct cplane_t;
class CViewSetup;


//-----------------------------------------------------------------------------
// Do we have reflective glass in view? If so, what's the reflection plane?
//-----------------------------------------------------------------------------
bool IsReflectiveGlassInView( const CViewSetup& view, cplane_t &plane );

#ifdef MAPBASE
C_BaseEntity *NextReflectiveGlass( C_BaseEntity *pStart, const CViewSetup& view, cplane_t &plane,
	const Frustum_t &frustum, ITexture **pRenderTargets = NULL );
#endif


#endif // C_FUNC_REFLECTIVE_GLASS


