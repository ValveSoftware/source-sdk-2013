//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Mapbase's implementation of VScript in VBSP, allowing users to modify map compilation behavior with scripts.
//
// $NoKeywords: $
//=============================================================================//

#ifndef VSCRIPT_VBSP_H
#define VSCRIPT_VBSP_H

#include "vscript/ivscript.h"

#if defined( _WIN32 )
#pragma once
#endif

extern IScriptVM *g_pScriptVM;
extern IScriptManager *scriptmanager;

HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing = false );
bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing = false );
inline bool VScriptRunScript( const char *pszScriptName, bool bWarnMissing = false ) { return VScriptRunScript( pszScriptName, NULL, bWarnMissing ); }

bool VScriptVBSPInit();
void VScriptVBSPTerm();

#endif // VSCRIPT_SERVER_H
