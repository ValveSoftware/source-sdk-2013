//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef TIER2_H
#define TIER2_H

#if defined( _WIN32 )
#pragma once
#endif

#include "tier1/tier1.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IFileSystem;
class IMaterialSystem;
class IColorCorrectionSystem;
class IMaterialSystemHardwareConfig;
class IDebugTextureInfo;
class IVBAllocTracker;
class IInputSystem;
class INetworkSystem;
class IP4;
class IMdlLib;
class IQueuedLoader;


//-----------------------------------------------------------------------------
// These tier2 libraries must be set by any users of this library.
// They can be set by calling ConnectTier2Libraries or InitDefaultFileSystem.
// It is hoped that setting this, and using this library will be the common mechanism for
// allowing link libraries to access tier2 library interfaces
//-----------------------------------------------------------------------------
extern IFileSystem *g_pFullFileSystem;
extern IMaterialSystem *materials;
extern IMaterialSystem *g_pMaterialSystem;
extern IInputSystem *g_pInputSystem;
extern INetworkSystem *g_pNetworkSystem;
extern IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig;
extern IDebugTextureInfo *g_pMaterialSystemDebugTextureInfo;
extern IVBAllocTracker *g_VBAllocTracker;
extern IColorCorrectionSystem *colorcorrection;
extern IP4 *p4;
extern IMdlLib *mdllib;
extern IQueuedLoader *g_pQueuedLoader;


//-----------------------------------------------------------------------------
// Call this to connect to/disconnect from all tier 2 libraries.
// It's up to the caller to check the globals it cares about to see if ones are missing
//-----------------------------------------------------------------------------
void ConnectTier2Libraries( CreateInterfaceFn *pFactoryList, int nFactoryCount );
void DisconnectTier2Libraries();


//-----------------------------------------------------------------------------
// Call this to get the file system set up to stdio for utilities, etc:
//-----------------------------------------------------------------------------
void InitDefaultFileSystem(void);
void ShutdownDefaultFileSystem(void);


//-----------------------------------------------------------------------------
// for simple utilities using valve libraries, call the entry point below in main(). It will
// init a filesystem for you, init mathlib, and create the command line.
//-----------------------------------------------------------------------------
void InitCommandLineProgram( int argc, char **argv );


//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------
template< class IInterface, int ConVarFlag = 0 > 
class CTier2AppSystem : public CTier1AppSystem< IInterface, ConVarFlag >
{
	typedef CTier1AppSystem< IInterface, ConVarFlag > BaseClass;

public:
	CTier2AppSystem( bool bIsPrimaryAppSystem = true ) : BaseClass( bIsPrimaryAppSystem )
	{
	}

	virtual bool Connect( CreateInterfaceFn factory ) 
	{
		if ( !BaseClass::Connect( factory ) )
			return false;

		if ( BaseClass::IsPrimaryAppSystem() )
		{
			ConnectTier2Libraries( &factory, 1 );
		}

		return true;
	}

	virtual InitReturnVal_t Init()
	{
		InitReturnVal_t nRetVal = BaseClass::Init();
		if ( nRetVal != INIT_OK )
			return nRetVal;

		return INIT_OK;
	}

	virtual void Shutdown()
	{
		BaseClass::Shutdown();
	}

	virtual void Disconnect() 
	{
		if ( BaseClass::IsPrimaryAppSystem() )
		{
			DisconnectTier2Libraries();
		}
		BaseClass::Disconnect();
	}
};


#endif // TIER2_H

