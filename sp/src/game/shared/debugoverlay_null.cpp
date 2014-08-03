//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "engine/ivdebugoverlay.h"
#include "mathlib/vector.h"

#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// NDebugOverlay
//=============================================================================
namespace NDebugOverlay
{
	void	Box(const Vector &origin, const Vector &mins, const Vector &maxs, int r, int g, int b, int a, float flDuration) {}
	void	BoxDirection(const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &forward, int r, int g, int b, int a, float flDuration) {}
	void	BoxAngles(const Vector &origin, const Vector &mins, const Vector &maxs, const QAngle &angles, int r, int g, int b, int a, float flDuration) {}
	void	SweptBox(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle & angles, int r, int g, int b, int a, float flDuration) {}
	void	EntityBounds( const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration ) {}
	void	Line( const Vector &origin, const Vector &target, int r, int g, int b, bool noDepthTest, float flDuration ) {}
	void	Triangle( const Vector &p1, const Vector &p2, const Vector &p3, int r, int g, int b, int a, bool noDepthTest, float duration ) {}
	void	EntityText( int entityID, int text_offset, const char *text, float flDuration, int r = 255, int g = 255, int b = 255, int a = 255) {}
	void	EntityTextAtPosition( const Vector &origin, int text_offset, const char *text, float flDuration, int r = 255, int g = 255, int b = 255, int a = 255) {}
	void	Grid( const Vector &vPosition ) {}
	void	Text( const Vector &origin, const char *text, bool bViewCheck, float flDuration ) {}
	void	ScreenText( float fXpos, float fYpos, const char *text, int r, int g, int b, int a, float flDuration) {}
	void	Cross3D(const Vector &position, const Vector &mins, const Vector &maxs, int r, int g, int b, bool noDepthTest, float flDuration ) {}
	void	Cross3D(const Vector &position, float size, int r, int g, int b, bool noDepthTest, float flDuration ) {}
	void	Cross3DOriented( const Vector &position, const QAngle &angles, float size, int r, int g, int b, bool noDepthTest, float flDuration ) {}
	void	Cross3DOriented( const matrix3x4_t &m, float size, int c, bool noDepthTest, float flDuration ) {}
	void	DrawOverlayLines(void){}
	void	DrawTickMarkedLine(const Vector &startPos, const Vector &endPos, float tickDist, int tickTextDist, int r, int g, int b, bool noDepthTest, float flDuration ) {}
	void	DrawGroundCrossHairOverlay() {}
	void	HorzArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration) {}
	void	YawArrow( const Vector &startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration) {}
	void	VertArrow( const Vector &startPos, const Vector &endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration) {}
};

