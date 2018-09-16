/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright  1997  Microsoft Corporation.  All Rights Reserved.

Module Name:

     tux.h  

Abstract:
Functions:
Notes:
--*/
//******************************************************************************
//
// TUX.H
//
// Definitions of Tux types, structures, and messages.
//
// Date     Name     Description
// -------- -------- -----------------------------------------------------------
// 02/14/95 SteveMil Created
//
//******************************************************************************

#ifndef __TUX_H__
#define __TUX_H__

//******************************************************************************
//***** Function Types
//******************************************************************************

// Forward declaration of LPFUNCTION_TABLE_ENTRY
typedef struct _FUNCTION_TABLE_ENTRY *LPFUNCTION_TABLE_ENTRY;

// Define our ShellProc Param and TestProc Param types
typedef LPARAM  SPPARAM;
typedef LPDWORD TPPARAM;

// Shell and Test message handling procs
typedef INT (WINAPI *SHELLPROC)(UINT uMsg, SPPARAM spParam);
typedef INT (WINAPI *TESTPROC )(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE);

// SHELLPROCAPI and TESTPROCAPI
#ifdef __cplusplus
#define SHELLPROCAPI extern "C" INT __declspec(dllexport) WINAPI
#else
#define SHELLPROCAPI INT __declspec(dllexport) WINAPI
#endif
#define TESTPROCAPI  INT WINAPI

//******************************************************************************
//***** Function Table Entry Structure
//******************************************************************************

typedef struct _FUNCTION_TABLE_ENTRY {
   LPCTSTR  lpDescription; // description of test
   UINT     uDepth;        // depth of item in tree hierarchy
   LPVOID   dwUserData;    // user defined data that will be passed to TestProc at runtime
   DWORD    dwUniqueID;    // uniquely identifies the test - used in loading/saving scripts
   TESTPROC lpTestProc;    // pointer to TestProc function to be called for this test
} FUNCTION_TABLE_ENTRY, *LPFUNCTION_TABLE_ENTRY;

extern FUNCTION_TABLE_ENTRY g_lpFTE[];

//******************************************************************************
//***** ShellProc() Message values
//******************************************************************************

#define SPM_LOAD_DLL               1
#define SPM_UNLOAD_DLL             2
#define SPM_START_SCRIPT           3
#define SPM_STOP_SCRIPT            4
#define SPM_BEGIN_GROUP            5
#define SPM_END_GROUP              6
#define SPM_SHELL_INFO             7
#define SPM_REGISTER               8
#define SPM_EXCEPTION              9
#define SPM_BEGIN_TEST            10
#define SPM_END_TEST              11

//******************************************************************************
//***** ShellProc() Return values
//******************************************************************************

#define SPR_NOT_HANDLED            0  
#define SPR_HANDLED                1
#define SPR_SKIP                   2
#define SPR_FAIL                   3

//******************************************************************************
//***** TestProc() Message values
//******************************************************************************

#define TPM_EXECUTE              101
#define TPM_QUERY_THREAD_COUNT   102

//******************************************************************************
//***** TestProc() Return values
//******************************************************************************

#define TPR_SKIP                   2
#define TPR_PASS                   3
#define TPR_FAIL                   4
#define TPR_ABORT                  5
#define TPR_SUPPORTED              6
#define TPR_UNSUPPORTED            7

//******************************************************************************
//***** ShellProc() Structures
//******************************************************************************

// ShellProc() Structure for SPM_LOAD_DLL message
typedef struct _SPS_LOAD_DLL {
   BOOL fUnicode;  // Set to true if your Dll is UNICODE
} SPS_LOAD_DLL, *LPSPS_LOAD_DLL;

// ShellProc() Structure for SPM_SHELL_INFO message
typedef struct _SPS_SHELL_INFO {
   HINSTANCE hInstance;     // Instance handle of shell.
   HWND      hWnd;          // Main window handle of shell (currently set to NULL).
   HINSTANCE hLib;          // Test Dll instance handle.
   HANDLE    hevmTerminate; // Manual event that is set by Tux to inform all
                            // tests to shutdown (currently not used).
   BOOL      fUsingServer;  // Set if Tux is connected to Tux Server.
   LPCTSTR   szDllCmdLine;  // Command line arguments for test DLL.
} SPS_SHELL_INFO, *LPSPS_SHELL_INFO;

// ShellProc() Structure for SPM_REGISTER message
typedef struct _SPS_REGISTER {
   LPFUNCTION_TABLE_ENTRY lpFunctionTable;
} SPS_REGISTER, *LPSPS_REGISTER;

// ShellProc() Structure for SPM_BEGIN_TEST message
typedef struct _SPS_BEGIN_TEST {
   LPFUNCTION_TABLE_ENTRY lpFTE;
   DWORD                  dwRandomSeed;
   DWORD                  dwThreadCount;
} SPS_BEGIN_TEST, *LPSPS_BEGIN_TEST;

// ShellProc() Structure for SPM_END_TEST message
typedef struct _SPS_END_TEST {
   LPFUNCTION_TABLE_ENTRY lpFTE;
   DWORD                  dwResult;
   DWORD                  dwRandomSeed;
   DWORD                  dwThreadCount;
   DWORD                  dwExecutionTime;
} SPS_END_TEST, *LPSPS_END_TEST;

// ShellProc() Structure for SPM_EXCEPTION message
typedef struct _SPS_EXCEPTION {
   LPFUNCTION_TABLE_ENTRY lpFTE;
   DWORD                  dwExceptionCode;
   EXCEPTION_POINTERS    *lpExceptionPointers;
   DWORD                  dwExceptionFilter;
   UINT                   uMsg;
} SPS_EXCEPTION, *LPSPS_EXCEPTION;


//******************************************************************************
//***** TestProc() Structures
//******************************************************************************

// TestProc() Structure for TPM_EXECUTE message
typedef struct _TPS_EXECUTE {
   DWORD dwRandomSeed;
   DWORD dwThreadCount;
   DWORD dwThreadNumber;
} TPS_EXECUTE, *LPTPS_EXECUTE;

// TestProc() Structure for TPM_QUERY_THREAD_COUNT message
typedef struct _TPS_QUERY_THREAD_COUNT {
   DWORD dwThreadCount;
} TPS_QUERY_THREAD_COUNT, *LPTPS_QUERY_THREAD_COUNT;

//******************************************************************************
//***** Old constants defined for compatibility - DO NOT USE THESE CONSTANTS!!!
//******************************************************************************

#define TPR_NOT_HANDLED   0
#define TPR_HANDLED       1
#define SPM_START_TESTS   SPM_BEGIN_GROUP
#define SPM_STOP_TESTS    SPM_END_GROUP
#define SHELLINFO         SPS_SHELL_INFO
#define LPSHELLINFO       LPSPS_SHELL_INFO
#define SPF_UNICODE       0x00010000

#endif //__TUX_H__
