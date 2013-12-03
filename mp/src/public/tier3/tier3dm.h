//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef TIER3DM_H
#define TIER3DM_H

#if defined( _WIN32 )
#pragma once
#endif

#include "tier3/tier3.h"
#include "tier2/tier2dm.h"

//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------
template< class IInterface, int ConVarFlag = 0 > 
class CTier3DmAppSystem : public CTier2DmAppSystem< IInterface, ConVarFlag >
{
	typedef CTier2DmAppSystem< IInterface, ConVarFlag > BaseClass;

public:
	CTier3DmAppSystem( bool bIsPrimaryAppSystem = true ) : BaseClass(	bIsPrimaryAppSystem )
	{
	}

	virtual bool Connect( CreateInterfaceFn factory ) 
	{
		if ( !BaseClass::Connect( factory ) )
			return false;

		if ( IsPrimaryAppSystem() )
		{
			ConnectTier3Libraries( &factory, 1 );
		}
		return true;
	}

	virtual void Disconnect() 
	{
		if ( IsPrimaryAppSystem() )
		{
			DisconnectTier3Libraries();
		}
		BaseClass::Disconnect();
	}
};


#endif // TIER3DM_H

