//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: setjmp/longjmp based cooperative multitasking system
//
//=============================================================================

#ifndef COROUTINE_H
#define COROUTINE_H
#pragma once

#include "vstdlib/vstdlib.h"

// enable this to do coroutine tracing
// this will tell coroutine API users to set coroutine names
// #define COROUTINE_TRACE

//-----------------------------------------------------------------------------
// Purpose: handles running coroutines
//			setjmp/longjmp based cooperative multitasking system
//-----------------------------------------------------------------------------

// coroutine callback
typedef void (__cdecl *CoroutineFunc_t )(void *);

// handle to a coroutine
typedef int32 HCoroutine;

// creates a new coroutine
// no coroutine code is executed until Coroutine_Continue() is called
VSTDLIB_INTERFACE HCoroutine Coroutine_Create( CoroutineFunc_t pFunc, void *pvParam );

// continues the specified coroutine
// returns true if the coroutine is still running, false otherwise
VSTDLIB_INTERFACE bool Coroutine_Continue( HCoroutine hCoroutine, const char *pchName = NULL );

// cancels a currently running coroutine
VSTDLIB_INTERFACE void Coroutine_Cancel( HCoroutine hCoroutine );

// 'load' a coroutine only to debug it - immediately breaks into debugger
// when continued, pops back to the prior coroutine
VSTDLIB_INTERFACE void Coroutine_DebugBreak( HCoroutine hCoroutine );

// Load a coroutine and generate an assert.  Used to get a minidump of a job
VSTDLIB_INTERFACE void Coroutine_DebugAssert( HCoroutine hCoroutine, const char *pchMsg );

// called from the coroutine to return control to the main thread
VSTDLIB_INTERFACE void Coroutine_YieldToMain();

// returns true if the code is currently running inside of a coroutine
VSTDLIB_INTERFACE bool Coroutine_IsActive();

// returns a handle the currently active coroutine
VSTDLIB_INTERFACE HCoroutine Coroutine_GetCurrentlyActive();

// call when a thread is quiting to release any per-thread memory
VSTDLIB_INTERFACE void Coroutine_ReleaseThreadMemory();

// runs a self-test of the coroutine system
VSTDLIB_INTERFACE bool Coroutine_Test();

// memory validation 
VSTDLIB_INTERFACE void Coroutine_ValidateGlobals( class CValidator &validator );

// for debugging purposes - returns stack depth of current coroutine
VSTDLIB_INTERFACE size_t Coroutine_GetStackDepth();



#endif // COROUTINE_H
