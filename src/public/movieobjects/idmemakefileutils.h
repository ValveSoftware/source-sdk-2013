//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Interface for makefiles to build differently depending on where they are run from
//
//===========================================================================//

#ifndef IDMEMAKEFILEUTILS_H
#define IDMEMAKEFILEUTILS_H

#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "vstdlib/iprocessutils.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmElement;


//-----------------------------------------------------------------------------
// Interface version
//-----------------------------------------------------------------------------
#define DMEMAKEFILE_UTILS_INTERFACE_VERSION "VDmeMakeFileUtils001"


//-----------------------------------------------------------------------------
// Interface for makefiles to build differently depending on where they are run from
//-----------------------------------------------------------------------------
enum CompilationState_t
{
	COMPILATION_SUCCESSFUL = 0,
	COMPILATION_NOT_COMPLETE,
	COMPILATION_FAILED,
};

abstract_class IDmeMakefileUtils : public IAppSystem
{
public:
	// Methods related to compilation
	virtual void PerformCompile( CDmElement *pElement, bool bBuildAllDependencies ) = 0;

	// Are we in the middle of compiling something?
	virtual bool IsCurrentlyCompiling( ) = 0;

	// Returns the size of the buffer to pass into UpdateCompilation()
	virtual int GetCompileOutputSize() = 0;

	// Updates the compilation
	virtual CompilationState_t UpdateCompilation( char *pOutputBuf, int nBufLen ) = 0;

	// Aborts the compilation
	virtual void AbortCurrentCompilation() = 0;

	// Opens an external editor for this element
	virtual void PerformOpenEditor( CDmElement *pElement ) = 0;

	// Returns the exit code of the failed compilation (if COMPILATION_FAILED occurred)
	virtual int GetExitCode() = 0;

	// Somewhere in here, we need a method of populating choice lists
	// for things like choosing vstInfoNodes to export for DCC makefiles
};


//-----------------------------------------------------------------------------
// Default implementation
//-----------------------------------------------------------------------------
IDmeMakefileUtils* GetDefaultDmeMakefileUtils();


#endif // IDMEMAKEFILEUTILS_H
