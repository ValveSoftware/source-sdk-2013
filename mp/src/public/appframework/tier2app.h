//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// The application object for apps that use tier2
//=============================================================================

#ifndef TIER2APP_H
#define TIER2APP_H

#ifdef _WIN32
#pragma once
#endif


#include "appframework/AppFramework.h"
#include "tier2/tier2dm.h"
#include "tier1/convar.h"


//-----------------------------------------------------------------------------
// The application object for apps that use tier2
//-----------------------------------------------------------------------------
class CTier2SteamApp : public CSteamAppSystemGroup
{
	typedef CSteamAppSystemGroup BaseClass;

public:
	// Methods of IApplication
	virtual bool PreInit()
	{
		CreateInterfaceFn factory = GetFactory();
		ConnectTier1Libraries( &factory, 1 );
		ConVar_Register( 0 );
		ConnectTier2Libraries( &factory, 1 );
		return true;			
	}

	virtual void PostShutdown()
	{
		DisconnectTier2Libraries();
		ConVar_Unregister();
		DisconnectTier1Libraries();
	}
};


//-----------------------------------------------------------------------------
// The application object for apps that use tier2 and datamodel
//-----------------------------------------------------------------------------
class CTier2DmSteamApp : public CTier2SteamApp
{
	typedef CTier2SteamApp BaseClass;

public:
	// Methods of IApplication
	virtual bool PreInit()
	{
		if ( !BaseClass::PreInit() )
			return false;

		CreateInterfaceFn factory = GetFactory();
		if ( !ConnectDataModel( factory ) )
			return false;

		InitReturnVal_t nRetVal = InitDataModel();
		return ( nRetVal == INIT_OK );
	}

	virtual void PostShutdown()
	{
		ShutdownDataModel();
		DisconnectDataModel();
		BaseClass::PostShutdown();
	}
};


#endif // TIER2APP_H
