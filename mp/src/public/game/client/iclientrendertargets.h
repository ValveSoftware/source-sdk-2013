//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Exposes interfaces to the engine which allow the client to setup their own render targets
//			during the proper period of material system's init. 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICLIENTRENDERTARGETS_H
#define ICLIENTRENDERTARGETS_H
#ifdef _WIN32
	#pragma once
#endif

#include "interface.h"		// For base interface

class IMaterialSystem;
class IMaterialSystemHardwareConfig;

//---------------------------------------------------------------------------------------------------
// Purpose: Exposes interfaces to the engine which allow the client to setup their own render targets
//			during the proper period of material system's init.
//---------------------------------------------------------------------------------------------------
abstract_class IClientRenderTargets
{
public:
	// Pass the material system interface to the client-- Their Material System singleton has not been created
	// at the time they receive this call.
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig ) = 0;

	// Call shutdown on every created refrence-- Clients keep track of this themselves
	// and should add shutdown code to this function whenever they add a new render target.
	virtual void ShutdownClientRenderTargets( void ) = 0;

};

#define CLIENTRENDERTARGETS_INTERFACE_VERSION "ClientRenderTargets001"

extern IClientRenderTargets * g_pClientRenderTargets;

#endif // ICLIENTRENDERTARGETS_H
