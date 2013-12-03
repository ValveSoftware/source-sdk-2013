//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ROTORWASH_H
#define ROTORWASH_H
#ifdef _WIN32
#pragma once
#endif

CBaseEntity *CreateRotorWashEmitter( const Vector &localOrigin, const QAngle &localAngles, CBaseEntity *pOwner, float flAltitude );

#endif // ROTORWASH_H
