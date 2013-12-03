//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MOVEVARS_SHARED_H
#define MOVEVARS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

float GetCurrentGravity( void );

extern ConVar sv_gravity;
extern ConVar sv_stopspeed;
extern ConVar sv_noclipaccelerate;
extern ConVar sv_noclipspeed;
extern ConVar sv_maxspeed;
extern ConVar sv_accelerate;
extern ConVar sv_airaccelerate;
extern ConVar sv_wateraccelerate;
extern ConVar sv_waterfriction;
extern ConVar sv_footsteps;
extern ConVar sv_rollspeed;
extern ConVar sv_rollangle;
extern ConVar sv_friction;
extern ConVar sv_bounce;
extern ConVar sv_maxvelocity;
extern ConVar sv_stepsize;
extern ConVar sv_skyname;
extern ConVar sv_backspeed;
extern ConVar sv_waterdist;
extern ConVar sv_specaccelerate;
extern ConVar sv_specspeed;
extern ConVar sv_specnoclip;

// Vehicle convars
extern ConVar r_VehicleViewDampen;

// Jeep convars
extern ConVar r_JeepViewDampenFreq;
extern ConVar r_JeepViewDampenDamp;
extern ConVar r_JeepViewZHeight;

// Airboat convars
extern ConVar r_AirboatViewDampenFreq;
extern ConVar r_AirboatViewDampenDamp;
extern ConVar r_AirboatViewZHeight;

#endif // MOVEVARS_SHARED_H
