//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ICLIENTVEHICLE_H
#define ICLIENTVEHICLE_H

#ifdef _WIN32
#pragma once
#endif

#include "IVehicle.h"

class C_BasePlayer;
class Vector;
class QAngle;
class C_BaseEntity;


//-----------------------------------------------------------------------------
// Purpose: All client vehicles must implement this interface.
//-----------------------------------------------------------------------------
abstract_class IClientVehicle : public IVehicle
{
public:
	// When a player is in a vehicle, here's where the camera will be
	virtual void GetVehicleFOV( float &flFOV ) = 0;

	// Allows the vehicle to restrict view angles, blend, etc.
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) = 0;

	// Hud redraw...
	virtual void DrawHudElements() = 0;

	// Is this predicted?
	virtual bool IsPredicted() const = 0;

	// Get the entity associated with the vehicle.
	virtual C_BaseEntity *GetVehicleEnt() = 0;

	// Allows the vehicle to change the near clip plane
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const = 0;
	
	// Allows vehicles to choose their own curves for players using joysticks
	virtual int GetJoystickResponseCurve() const = 0;

#ifdef HL2_CLIENT_DLL
	// Ammo in the vehicles
	virtual int GetPrimaryAmmoType() const = 0;
	virtual int GetPrimaryAmmoClip() const = 0;
	virtual bool PrimaryAmmoUsesClips() const = 0;
	virtual int GetPrimaryAmmoCount() const = 0;
#endif
};


#endif // ICLIENTVEHICLE_H
