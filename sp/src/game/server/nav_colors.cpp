//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Central point for defining colors and drawing routines for Navigation Mesh edit mode
// Author: Matthew Campbell, 2004

#include "cbase.h"
#include "nav_colors.h"
#include "Color.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


//--------------------------------------------------------------------------------------------------------------
/**
 * This MUST be kept in sync with the NavEditColor definition
 */
Color NavColors[] =
{
	// Degenerate area colors
	Color( 255, 255, 255 ),		// NavDegenerateFirstColor
	Color( 255, 0, 255 ),		// NavDegenerateSecondColor

	// Place painting color
	Color( 0, 255, 0 ),			// NavSamePlaceColor
	Color( 0, 0, 255 ),			// NavDifferentPlaceColor
	Color( 255, 0, 0 ),			// NavNoPlaceColor

	// Normal colors
	Color( 255, 255, 0 ),		// NavSelectedColor
	Color( 0, 255, 255 ),		// NavMarkedColor
	Color( 255, 0, 0 ),			// NavNormalColor
	Color( 0, 0, 255 ),			// NavCornerColor
	Color( 0, 0, 255 ),			// NavBlockedByDoorColor
	Color( 0, 255, 255 ),		// NavBlockedByFuncNavBlockerColor

	// Hiding spot colors
	Color( 255, 0, 0 ),			// NavIdealSniperColor
	Color( 255, 0, 255 ),		// NavGoodSniperColor
	Color( 0, 255, 0 ),			// NavGoodCoverColor
	Color( 255, 0, 255 ),		// NavExposedColor
	Color( 255, 100, 0 ),		// NavApproachPointColor

	// Connector colors
	Color( 0, 255, 255 ),		// NavConnectedTwoWaysColor
	Color( 0, 0, 255 ),			// NavConnectedOneWayColor
	Color( 0, 255, 0 ),			// NavConnectedContiguous
	Color( 255, 0, 0 ),			// NavConnectedNonContiguous

	// Editing colors
	Color( 255, 255, 255 ),		// NavCursorColor
	Color( 255, 255, 255 ),		// NavSplitLineColor
	Color( 0, 255, 255 ),		// NavCreationColor
	Color( 255, 0, 0 ),			// NavInvalidCreationColor
	Color( 0, 64, 64 ),			// NavGridColor
	Color( 255, 255, 255 ),		// NavDragSelectionColor

	// Nav attribute colors
	Color( 0, 0, 255 ),			// NavAttributeCrouchColor
	Color( 0, 255, 0 ),			// NavAttributeJumpColor
	Color( 0, 255, 0 ),			// NavAttributePreciseColor
	Color( 255, 0, 0 ),			// NavAttributeNoJumpColor
	Color( 255, 0, 0 ),			// NavAttributeStopColor
	Color( 0, 0, 255 ),			// NavAttributeRunColor
	Color( 0, 255, 0 ),			// NavAttributeWalkColor
	Color( 255, 0, 0 ),			// NavAttributeAvoidColor
	Color( 0, 200, 0 ),			// NavAttributeStairColor
};


//--------------------------------------------------------------------------------------------------------------
void NavDrawLine( const Vector& from, const Vector& to, NavEditColor navColor )
{
	const Vector offset( 0, 0, 1 );

	Color color = NavColors[navColor];
	NDebugOverlay::Line( from + offset, to + offset, color[0], color[1], color[2], false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	NDebugOverlay::Line( from + offset, to + offset, color[0]/2, color[1]/2, color[2]/2, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
}


//--------------------------------------------------------------------------------------------------------------
void NavDrawTriangle( const Vector& point1, const Vector& point2, const Vector& point3, NavEditColor navColor )
{
	NavDrawLine( point1, point2, navColor );
	NavDrawLine( point2, point3, navColor );
	NavDrawLine( point1, point3, navColor );
}


//--------------------------------------------------------------------------------------------------------------
void NavDrawFilledTriangle( const Vector& point1, const Vector& point2, const Vector& point3, NavEditColor navColor, bool dark )
{
	Color color = NavColors[navColor];
	if ( dark )
	{
		color[0] = color[0] / 2;
		color[1] = color[1] / 2;
		color[2] = color[2] / 2;
	}
	NDebugOverlay::Triangle( point1, point2, point3, color[0], color[1], color[2], 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
}


//--------------------------------------------------------------------------------------------------------------
void NavDrawHorizontalArrow( const Vector& from, const Vector& to, float width, NavEditColor navColor )
{
	const Vector offset( 0, 0, 1 );

	Color color = NavColors[navColor];
	NDebugOverlay::HorzArrow( from + offset, to + offset, width, color[0], color[1], color[2], 255, false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	NDebugOverlay::HorzArrow( from + offset, to + offset, width, color[0]/2, color[1]/2, color[2]/2, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
}


//--------------------------------------------------------------------------------------------------------------
void NavDrawDashedLine( const Vector& from, const Vector& to, NavEditColor navColor )
{
	const Vector offset( 0, 0, 1 );

	Color color = NavColors[navColor];

	const float solidLen = 7.0f;
	const float   gapLen = 3.0f;

	Vector unit = (to - from);
	const float totalDistance = unit.NormalizeInPlace();

	float distance = 0.0f;

	while ( distance < totalDistance )
	{
		Vector start = from + unit * distance;
		float endDistance = distance + solidLen;
		endDistance = MIN( endDistance, totalDistance );
		Vector end = from + unit * endDistance;

		distance += solidLen + gapLen;

		NDebugOverlay::Line( start + offset, end + offset, color[0], color[1], color[2], false, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		NDebugOverlay::Line( start + offset, end + offset, color[0]/2, color[1]/2, color[2]/2, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}
}


//--------------------------------------------------------------------------------------------------------------
void NavDrawVolume( const Vector &vMin, const Vector &vMax, int zMidline, NavEditColor navColor )
{
	// Center rectangle
	NavDrawLine( Vector( vMax.x, vMax.y, zMidline ), Vector( vMin.x, vMax.y, zMidline ),	navColor );
	NavDrawLine( Vector( vMin.x, vMin.y, zMidline ), Vector( vMin.x, vMax.y, zMidline ),	navColor );
	NavDrawLine( Vector( vMin.x, vMin.y, zMidline ), Vector( vMax.x, vMin.y, zMidline ),	navColor );
	NavDrawLine( Vector( vMax.x, vMax.y, zMidline ), Vector( vMax.x, vMin.y, zMidline ),	navColor );

	// Bottom rectangle
	NavDrawLine( Vector( vMax.x, vMax.y, vMin.z ), Vector( vMin.x, vMax.y, vMin.z ),	navColor );
	NavDrawLine( Vector( vMin.x, vMin.y, vMin.z ), Vector( vMin.x, vMax.y, vMin.z ),	navColor );
	NavDrawLine( Vector( vMin.x, vMin.y, vMin.z ), Vector( vMax.x, vMin.y, vMin.z ),	navColor );
	NavDrawLine( Vector( vMax.x, vMax.y, vMin.z ), Vector( vMax.x, vMin.y, vMin.z ),	navColor );

	// Top rectangle
	NavDrawLine( Vector( vMax.x, vMax.y, vMax.z ), Vector( vMin.x, vMax.y, vMax.z ),	navColor );
	NavDrawLine( Vector( vMin.x, vMin.y, vMax.z ), Vector( vMin.x, vMax.y, vMax.z ),	navColor );
	NavDrawLine( Vector( vMin.x, vMin.y, vMax.z ), Vector( vMax.x, vMin.y, vMax.z ),	navColor );
	NavDrawLine( Vector( vMax.x, vMax.y, vMax.z ), Vector( vMax.x, vMin.y, vMax.z ),	navColor );

	// Edges
	NavDrawLine( Vector( vMax.x, vMax.y, vMin.z ), Vector( vMax.x, vMax.y, vMax.z ),	navColor );
	NavDrawLine( Vector( vMin.x, vMin.y, vMin.z ), Vector( vMin.x, vMin.y, vMax.z ),	navColor );
	NavDrawLine( Vector( vMax.x, vMin.y, vMin.z ), Vector( vMax.x, vMin.y, vMax.z ),	navColor );
	NavDrawLine( Vector( vMin.x, vMax.y, vMin.z ), Vector( vMin.x, vMax.y, vMax.z ),	navColor );
}




//--------------------------------------------------------------------------------------------------------------
