//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Colors used for nav editing

#ifndef NAV_COLORS_H
#define NAV_COLORS_H

//--------------------------------------------------------------------------------------------------------------
enum NavEditColor
{
	// Degenerate area colors
	NavDegenerateFirstColor = 0,
	NavDegenerateSecondColor,

	// Place painting color
	NavSamePlaceColor,
	NavDifferentPlaceColor,
	NavNoPlaceColor,

	// Normal colors
	NavSelectedColor,
	NavMarkedColor,
	NavNormalColor,
	NavCornerColor,
	NavBlockedByDoorColor,
	NavBlockedByFuncNavBlockerColor,

	// Hiding spot colors
	NavIdealSniperColor,
	NavGoodSniperColor,
	NavGoodCoverColor,
	NavExposedColor,
	NavApproachPointColor,

	// Connector colors
	NavConnectedTwoWaysColor,
	NavConnectedOneWayColor,
	NavConnectedContiguous,
	NavConnectedNonContiguous,

	// Editing colors
	NavCursorColor,
	NavSplitLineColor,
	NavCreationColor,
	NavInvalidCreationColor,
	NavGridColor,
	NavDragSelectionColor,

	// Nav attribute colors
	NavAttributeCrouchColor,
	NavAttributeJumpColor,
	NavAttributePreciseColor,
	NavAttributeNoJumpColor,
	NavAttributeStopColor,
	NavAttributeRunColor,
	NavAttributeWalkColor,
	NavAttributeAvoidColor,
	NavAttributeStairColor,
};

//--------------------------------------------------------------------------------------------------------------

void NavDrawLine( const Vector& from, const Vector& to, NavEditColor navColor );
void NavDrawTriangle( const Vector& point1, const Vector& point2, const Vector& point3, NavEditColor navColor );
void NavDrawFilledTriangle( const Vector& point1, const Vector& point2, const Vector& point3, NavEditColor navColor, bool dark );
void NavDrawHorizontalArrow( const Vector& from, const Vector& to, float width, NavEditColor navColor );
void NavDrawDashedLine( const Vector& from, const Vector& to, NavEditColor navColor );
void NavDrawVolume( const Vector &vMin, const Vector &vMax, int zMidline, NavEditColor navColor );

//--------------------------------------------------------------------------------------------------------------

#endif // NAV_COLORS_H
