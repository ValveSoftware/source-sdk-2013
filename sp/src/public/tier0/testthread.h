//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: exposes testing thread functions
//
//=============================================================================

#ifndef TESTTHREAD_H
#define TESTTHREAD_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"

// test callback
typedef void (STDCALL *TestFunc)(void *pv);

// runs the test function
DBG_INTERFACE void Test_RunTest(TestFunc func, void *pvArg);

// call to give the test thread a chance to run
// calling thread will block until the test thread yields
// doesn't do anything if no tests are running
DBG_INTERFACE void Test_RunFrame();

// true if any tests are running, or have ran
DBG_INTERFACE bool Test_IsActive();

// sets that the test has failed
DBG_INTERFACE void Test_SetFailed();

// true if any tests have failed, due to an assert, warning, or explicit fail
DBG_INTERFACE bool Test_HasFailed();

// true if any tests have completed
DBG_INTERFACE bool Test_HasFinished();

// terminates the test thread
DBG_INTERFACE void Test_TerminateThread();

// the following functions should only be called from the test thread

// yields to the main thread for a single frame
// passing in is a count of the number of frames that have been yielded by this yield macro
// can be used to assert if a test thread is blocked foor
DBG_INTERFACE void TestThread_Yield();

// utility functions to pause the test frame until the selected condition is true
#define YIELD_UNTIL(x) { int iYieldCount = 0; while (!(x)) { TestThread_Yield(); iYieldCount++; if ( iYieldCount >= 100 ) { AssertMsg( false, #x ); break; } } }

// use this like a while(1) loop, with break; to stop yielding
#define YIELD_UNTIL_BREAK() for (; true; TestThread_Yield())

// yields for a single frame
#define YIELD_FRAME() { TestThread_Yield(); }
#define YIELD_TWO_FRAMES() { TestThread_Yield(); TestThread_Yield(); }



#endif // TESTTHREAD_H
