//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_CLIENT_H
#define VSCRIPT_CLIENT_H

#include "vscript/ivscript.h"
#include "vscript_shared.h"

#if defined( _WIN32 )
#pragma once
#endif

extern IScriptVM * g_pScriptVM;

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );

#ifdef MAPBASE_VSCRIPT
extern IScriptManager * scriptmanager;
#endif

#endif // VSCRIPT_CLIENT_H
