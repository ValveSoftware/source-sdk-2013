//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TOOLS_MINIDUMP_H
#define TOOLS_MINIDUMP_H
#ifdef _WIN32
#pragma once
#endif



// Defaults to false. If true, it'll write larger minidump files with the contents
// of global variables and following pointers from where the crash occurred.
void EnableFullMinidumps( bool bFull );


// This handler catches any crash, writes a minidump, and runs the default system
// crash handler (which usually shows a dialog).
void SetupDefaultToolsMinidumpHandler();


// (Used by VMPI) - you specify your own crash handler.
// Arguments passed to ToolsExceptionHandler
//		exceptionCode		- exception code
//		pvExceptionInfo		- on Win32 platform points to "struct _EXCEPTION_POINTERS"
//							  otherwise NULL
//
typedef void (*ToolsExceptionHandler)( unsigned long exceptionCode, void *pvExceptionInfo );
void SetupToolsMinidumpHandler( ToolsExceptionHandler fn );


#endif // MINIDUMP_H
