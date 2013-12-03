//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYSICS_FX_H
#define PHYSICS_FX_H
#ifdef _WIN32
#pragma once
#endif


class CBaseEntity;
class IPhysicsFluidController;

void PhysicsSplash( IPhysicsFluidController *pFluid, IPhysicsObject *pObject, CBaseEntity *pEntity );

#endif // PHYSICS_FX_H
