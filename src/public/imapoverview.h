//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( IMAPOVERVIEW_H )
#define IMAPOVERVIEW_H
#ifdef _WIN32
#pragma once
#endif

// #include "interface.h"

// #define INTERFACEVERSION_HLTVPANEL	"HLTVPANEL001"

//-----------------------------------------------------------------------------
// Purpose: interface for map overview panel
//-----------------------------------------------------------------------------

#include "mathlib/vector.h"
#include "mathlib/vector2d.h"

class IMapOverview // : public IBaseInterface
{
public:
	virtual	~IMapOverview( void ) {};

	virtual	void SetVisible(bool state) = 0;	// set map panel visible
	virtual void SetBounds(int x, int y, int wide, int tall) = 0; // set pos & size
	virtual void SetZoom( float zoom ) = 0; // set zoom
	virtual void SetTime( float time ) = 0; // set game time
	virtual void SetAngle( float angle) = 0; // set map orientation
	virtual void SetFollowAngle(bool state) = 0; // if true, map rotates with spectators view
	virtual void SetCenter( Vector2D &mappos) = 0; // set map pos in center of panel
	virtual void SetPlayerPositions(int index, const Vector &position, const QAngle &angle) = 0; // update player position
	virtual Vector2D WorldToMap( Vector &worldpos ) = 0; // convert 3d world to 2d map pos

	virtual bool  IsVisible( void )= 0;	// true if MapOverview is visible
	virtual void  GetBounds(int& x, int& y, int& wide, int& tall) = 0; // get current pos & size
	virtual float GetZoom( void )= 0;

	// deatils properties
	virtual	void ShowPlayerNames(bool state) = 0;	// show player names under icons
	virtual	void ShowTracers(bool state) = 0;	// show shooting traces as lines
	virtual	void ShowExplosions(bool state) = 0;	// show, smoke, flash & HE grenades
	virtual	void ShowHealth(bool state) = 0;		// show player health under icon
	virtual	void ShowHurts(bool state) = 0;	// show player icon flashing if player is hurt
	virtual	void ShowTracks(float seconds) = 0; // show player trails for n seconds
};

#endif // IMAPOVERVIEW_H