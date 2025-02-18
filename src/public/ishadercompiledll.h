//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ISHADERCOMPILEDLL_H
#define ISHADERCOMPILEDLL_H
#ifdef _WIN32
#pragma once
#endif


#include "interface.h"

#define SHADER_COMPILE_INTERFACE_VERSION "shadercompiledll_0"

// This is the DLL interface to ShaderCompile
abstract_class IShaderCompileDLL
{
public:
	// All vrad.exe does is load the VRAD DLL and run this.
	virtual int			main( int argc, char **argv ) = 0;
};

#endif // ISHADERCOMPILEDLL_H
