//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "matsys_controls/matsyscontrols.h"
#include <materialsystem/imaterialsystem.h>
#include <materialsystem/imaterialsystemhardwareconfig.h>
#include <datacache/imdlcache.h>
#include <VGuiMatSurface/IMatSystemSurface.h>
#include <istudiorender.h>
#include "vgui_controls/Controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace vgui
{

IMaterialSystem *g_pMaterialSystem = NULL;
IMaterialSystem *MaterialSystem()
{
	return g_pMaterialSystem;
}

IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig = NULL;
IMaterialSystemHardwareConfig *MaterialSystemHardwareConfig()
{
	return g_pMaterialSystemHardwareConfig;
}

IMDLCache *g_pMDLCache = NULL;
IMDLCache *MDLCache()
{
	return g_pMDLCache;
}

IMatSystemSurface *g_pMatSystemSurface = NULL;
IMatSystemSurface *MatSystemSurface()
{
	return g_pMatSystemSurface;
}

IStudioRender *g_pStudioRender = NULL;
IStudioRender *StudioRender()
{
	return g_pStudioRender;
}


//-----------------------------------------------------------------------------
// Purpose: finds a particular interface in the factory set
//-----------------------------------------------------------------------------
static void *InitializeInterface( char const *interfaceName, CreateInterfaceFn *factoryList, int numFactories )
{
	void *retval;

	for ( int i = 0; i < numFactories; i++ )
	{
		CreateInterfaceFn factory = factoryList[ i ];
		if ( !factory )
			continue;

		retval = factory( interfaceName, NULL );
		if ( retval )
			return retval;
	}

	// No provider for requested interface!!!
	// Assert( !"No provider for requested interface!!!" );

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Initializes the controls
//-----------------------------------------------------------------------------
bool VGui_InitMatSysInterfacesList( const char *moduleName, CreateInterfaceFn *factoryList, int numFactories )
{
	if ( !vgui::VGui_InitInterfacesList( moduleName, factoryList, numFactories ) )
		return false;

	g_pMaterialSystem = (IMaterialSystem *)InitializeInterface( MATERIAL_SYSTEM_INTERFACE_VERSION, factoryList, numFactories );
	g_pMatSystemSurface = (IMatSystemSurface *)InitializeInterface( MAT_SYSTEM_SURFACE_INTERFACE_VERSION, factoryList, numFactories );
	g_pMDLCache = (IMDLCache *)InitializeInterface( MDLCACHE_INTERFACE_VERSION, factoryList, numFactories );
	g_pStudioRender = (IStudioRender *)InitializeInterface( STUDIO_RENDER_INTERFACE_VERSION, factoryList, numFactories );
	g_pMaterialSystemHardwareConfig = (IMaterialSystemHardwareConfig *)InitializeInterface( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, factoryList, numFactories );

	// MDL cache + studiorender are optional
	return ( g_pMaterialSystem && g_pMatSystemSurface && g_pMaterialSystemHardwareConfig );
}


} // namespace vgui



