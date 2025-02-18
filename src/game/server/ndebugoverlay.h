//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Namespace for functions dealing with Debug Overlays
//
// $NoKeywords: $
//=============================================================================//
#ifndef NDEBUGOVERLAY_H
#define NDEBUGOVERLAY_H

#ifdef _WIN32
#pragma once
#endif

#include "debugoverlay_shared.h"

// An overlay line
struct OverlayLine_t 
{
	Vector			origin;
	Vector			dest;
	int				r;
	int				g;
	int				b;
	bool			noDepthTest;
	bool			draw;
};

extern void	UTIL_AddDebugLine( const Vector &startPos, const Vector &endPos, bool noDepthTest, bool testLOS );
extern void	UTIL_DrawPositioningOverlay( float flCrossDistance );
extern void UTIL_DrawOverlayLines( void );

extern void DebugDrawLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration );

#endif // NDEBUGOVERLAY_H
