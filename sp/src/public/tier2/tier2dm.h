//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef TIER2DM_H
#define TIER2DM_H

#if defined( _WIN32 )
#pragma once
#endif

#include "tier2/tier2.h"

//-----------------------------------------------------------------------------
// Set up methods related to datamodel interfaces
//-----------------------------------------------------------------------------
bool ConnectDataModel( CreateInterfaceFn factory );
InitReturnVal_t InitDataModel();
void ShutdownDataModel();
void DisconnectDataModel();

//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------
template< class IInterface, int ConVarFlag = 0 > 
class CTier2DmAppSystem : public CTier2AppSystem< IInterface, ConVarFlag >
{
	typedef CTier2AppSystem< IInterface, ConVarFlag > BaseClass;

public:
	CTier2DmAppSystem( bool bIsPrimaryAppSystem = true ) : BaseClass( bIsPrimaryAppSystem )
	{
	}

	virtual bool Connect( CreateInterfaceFn factory ) 
	{
		if ( !BaseClass::Connect( factory ) )
			return false;

		ConnectDataModel( factory );

		return true;
	}

	virtual InitReturnVal_t Init()
	{
		InitReturnVal_t nRetVal = BaseClass::Init();
		if ( nRetVal != INIT_OK )
			return nRetVal;

		nRetVal = InitDataModel();
		if ( nRetVal != INIT_OK )
			return nRetVal;

		return INIT_OK;
	}

	virtual void Shutdown()
	{
		ShutdownDataModel();
		BaseClass::Shutdown();
	}

	virtual void Disconnect() 
	{
		DisconnectDataModel();
		BaseClass::Disconnect();
	}
};


#endif // TIER2DM_H

