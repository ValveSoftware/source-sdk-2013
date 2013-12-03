//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYS_CONTROLLER_H
#define PHYS_CONTROLLER_H
#ifdef _WIN32
#pragma once
#endif


CBaseEntity *CreateKeepUpright( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, float flAngularLimit, bool bActive );

AngularImpulse ComputeRotSpeedToAlignAxes( const Vector &testAxis, const Vector &alignAxis, const AngularImpulse &currentSpeed, 
										  float damping, float scale, float maxSpeed );

#endif // PHYS_CONTROLLER_H
