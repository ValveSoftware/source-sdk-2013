//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IMATERIALSYSTEMSTUB_H
#define IMATERIALSYSTEMSTUB_H
#ifdef _WIN32
#pragma once
#endif


#include "materialsystem/imaterialsystem.h"


// If you get this interface out of the material system, it'll return an IMaterialSystem
// with everything stubbed. This is used for running the client in text mode.
#define MATERIAL_SYSTEM_STUB_INTERFACE_VERSION "VMaterialSystemStub001"


class IMaterialSystemStub : public IMaterialSystem
{
public:
	// If this is called, then the stub will call through to the real material 
	// system in some functions.
	virtual void	SetRealMaterialSystem( IMaterialSystem *pSys ) = 0;
};


#endif // IMATERIALSYSTEMSTUB_H
