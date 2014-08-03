//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: insulates client DLL from dependencies on vphysics
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHYSICS_H
#define PHYSICS_H
#ifdef _WIN32
#pragma once
#endif


#include "interface.h"
#include "physics_shared.h"

struct objectparams_t;
struct solid_t;

// HACKHACK: Make this part of IClientSystem somehow???
extern bool PhysicsDLLInit( CreateInterfaceFn physicsFactory );
extern void PhysicsReset();
extern void PhysicsSimulate();
extern float PhysGetSyncCreateTime();

#endif // PHYSICS_H
