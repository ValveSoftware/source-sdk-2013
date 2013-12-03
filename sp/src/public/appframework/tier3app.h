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
// The application objects for apps that use tier3
//=============================================================================

#ifndef TIER3APP_H
#define TIER3APP_H

#ifdef _WIN32
#pragma once
#endif


#include "appframework/tier2app.h"
#include "tier3/tier3.h"
#include "vgui_controls/Controls.h"


//-----------------------------------------------------------------------------
// The application object for apps that use tier3
//-----------------------------------------------------------------------------
class CTier3SteamApp : public CTier2SteamApp
{
	typedef CTier2SteamApp BaseClass;

public:
	// Methods of IApplication
	virtual bool PreInit()
	{
		if ( !BaseClass::PreInit() )
			return false;

		CreateInterfaceFn factory = GetFactory();
		ConnectTier3Libraries( &factory, 1 );
		return true;			
	}

	virtual void PostShutdown()
	{
		DisconnectTier3Libraries();
		BaseClass::PostShutdown();
	}
};


//-----------------------------------------------------------------------------
// The application object for apps that use tier3
//-----------------------------------------------------------------------------
class CTier3DmSteamApp : public CTier2DmSteamApp
{
	typedef CTier2DmSteamApp BaseClass;

public:
	// Methods of IApplication
	virtual bool PreInit()
	{
		if ( !BaseClass::PreInit() )
			return false;

		CreateInterfaceFn factory = GetFactory();
		ConnectTier3Libraries( &factory, 1 );
		return true;			
	}

	virtual void PostShutdown()
	{
		DisconnectTier3Libraries();
		BaseClass::PostShutdown();
	}
};


//-----------------------------------------------------------------------------
// The application object for apps that use vgui
//-----------------------------------------------------------------------------
class CVguiSteamApp : public CTier3SteamApp
{
	typedef CTier3SteamApp BaseClass;

public:
	// Methods of IApplication
	virtual bool PreInit()
	{
		if ( !BaseClass::PreInit() )
			return false;

		CreateInterfaceFn factory = GetFactory();
		return vgui::VGui_InitInterfacesList( "CVguiSteamApp", &factory, 1 );
	}
};


//-----------------------------------------------------------------------------
// The application object for apps that use vgui
//-----------------------------------------------------------------------------
class CVguiDmSteamApp : public CTier3DmSteamApp
{
	typedef CTier3DmSteamApp BaseClass;

public:
	// Methods of IApplication
	virtual bool PreInit()
	{
		if ( !BaseClass::PreInit() )
			return false;

		CreateInterfaceFn factory = GetFactory();
		return vgui::VGui_InitInterfacesList( "CVguiSteamApp", &factory, 1 );
	}
};


#endif // TIER3APP_H
