// cbase.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifdef SCRIPTO_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#define SOURCE_ENGINE


// standard libs
#include <cstdarg>

#ifdef SOURCE_ENGINE

// tier0
#include <tier0/dbg.h>
//#include <tier0/platform.h>

// tier1
#include <tier1/utlvector.h>

// tier2

// tier3

// shared
#include <Color.h>

#else

// Non-source implementations

template< class T >
class CUtlMemory;

template< class T, class A = CUtlMemory<T> >
class CUtlVector;

#endif


#define COLOR_CYAN Color(0, 255, 255, 255)

void ScriptLog(const char* msg, ...);
void ScriptError(const char* msg, ...);