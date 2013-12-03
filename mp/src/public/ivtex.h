//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IVTEX_H
#define IVTEX_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "appframework/IAppSystem.h"


class IVTex : public IAppSystem
{
public:
	// For use by command-line tools
	virtual int VTex( int argc, char **argv ) = 0;

	// For use by engine
	virtual int VTex( CreateInterfaceFn filesystemFactory, const char *pGameDir, int argc, char **argv ) = 0;
};

#define IVTEX_VERSION_STRING "VTEX_003"


#endif // IVTEX_H
