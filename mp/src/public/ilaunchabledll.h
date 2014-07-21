//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ILAUNCHABLEDLL_H
#define ILAUNCHABLEDLL_H
#ifdef _WIN32
#pragma once
#endif


#define LAUNCHABLE_DLL_INTERFACE_VERSION "launchable_dll_1"


// vmpi_service can use this to debug worker apps in-process,
// and some of the launchers (like texturecompile) use this.
class ILaunchableDLL
{
public:
	// All vrad.exe does is load the VRAD DLL and run this.
	virtual int			main( int argc, char **argv ) = 0;
};



#endif // ILAUNCHABLEDLL_H
