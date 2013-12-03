//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A set of utilities to render standard shapes
//
//===========================================================================//

#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier2/tier2.h"
#include "Color.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class Vector;
class QAngle;
class IMaterial;
struct matrix3x4_t;


// Renders a wireframe sphere
void RenderWireframeSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer );

// Renders a sphere
void RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer );
void RenderSphere( const Vector &vCenter, float flRadius, int nTheta, int nPhi, Color c, IMaterial *pMaterial );

// Renders a wireframe box relative to an origin 
void RenderWireframeBox( const Vector &vOrigin, const QAngle& angles, const Vector &vMins, const Vector &vMaxs, Color c, bool bZBuffer );

// Renders a swept wireframe box
void RenderWireframeSweptBox( const Vector &vStart, const Vector &vEnd, const QAngle &angles, const Vector &vMins, const Vector &vMaxs, Color c, bool bZBuffer );

// Renders a solid box 
void RenderBox( const Vector& origin, const QAngle& angles, const Vector& mins, const Vector& maxs, Color c, bool bZBuffer, bool bInsideOut = false );
void RenderBox( const Vector& origin, const QAngle& angles, const Vector& mins, const Vector& maxs, Color c, IMaterial *pMaterial, bool bInsideOut = false );

// Renders axes, red->x, green->y, blue->z (axis aligned)
void RenderAxes( const Vector &vOrigin, float flScale, bool bZBuffer );
void RenderAxes( const matrix3x4_t &transform, float flScale, bool bZBuffer );

// Render a line
void RenderLine( const Vector& v1, const Vector& v2, Color c, bool bZBuffer );

// Draws a triangle
void RenderTriangle( const Vector& p1, const Vector& p2, const Vector& p3, Color c, bool bZBuffer );
void RenderTriangle( const Vector& p1, const Vector& p2, const Vector& p3, Color c, IMaterial *pMaterial );

// Draws a axis-aligned quad
void RenderQuad( IMaterial *pMaterial, float x, float y, float w, float h, float z, float s0, float t0, float s1, float t1, const Color& clr );

// Renders a screen space quad
void DrawScreenSpaceRectangle( IMaterial *pMaterial, 
	int nDestX, int nDestY, int nWidth, int nHeight,	// Rect to draw into in screen space
	float flSrcTextureX0, float flSrcTextureY0,			// which texel you want to appear at destx/y
	float flSrcTextureX1, float flSrcTextureY1,			// which texel you want to appear at destx+width-1, desty+height-1
	int nSrcTextureWidth, int nSrcTextureHeight,		// needed for fixup
	void *pClientRenderable = NULL,						// Used to pass to the bind proxies
	int nXDice = 1,
	int nYDice = 1,
	float fDepth = 0.0 );								// what Z value to put in the verts

#endif // RENDERUTILS_H

