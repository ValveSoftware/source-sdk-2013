//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// This makes it easy to dynamically load a shared library and lookup a
//  function in that library.
//
//  Usage:
//     CDynamicFunction<void (*)(const char *)> MyPuts(libname, "puts");
//     if (MyPuts)
//         MyPuts("Hello world!");
//
//  Please note that this interface does not distinguish between functions and
//   data. If you look up a global variable in your shared library, or simply
//   mess up the function signature, you'll get a valid pointer and a crash
//   if you call it as a function.

#ifndef DYNFUNCTION_H
#define DYNFUNCTION_H
#pragma once

#include "tier0/platform.h"

// The heavy lifting isn't template-specific, so we move it out of the header.
DLL_EXPORT void *VoidFnPtrLookup_Tier0(const char *libname, const char *fn, void *fallback);

template < class FunctionType >
class CDynamicFunction
{
public:
	// Construct with a NULL function pointer. You must manually call
	//  Lookup() before you can call a dynamic function through this interface.
	CDynamicFunction() : m_pFn(NULL) {}

	// Construct and do a lookup right away. You will need to make sure that
	//  the lookup actually succeeded, as (libname) might have failed to load
	//  or (fn) might not exist in it.
	CDynamicFunction(const char *libname, const char *fn, FunctionType fallback=NULL) : m_pFn(NULL)
	{
		Lookup(libname, fn, fallback);
	}

	// Construct and do a lookup right away. See comments in Lookup() about what (okay) does.
	CDynamicFunction(const char *libname, const char *fn, bool &okay, FunctionType fallback=NULL) : m_pFn(NULL)
	{
		Lookup(libname, fn, okay, fallback);
	}

	// Load library if necessary, look up symbol. Returns true and sets
	//  m_pFn on successful lookup, returns false otherwise. If the
	//  function pointer is already looked up, this return true immediately.
	// Use Reset() first if you want to look up the symbol again.
	//  This function will return false immediately unless (okay) is true.
	//  This allows you to chain lookups like this:
	//     bool okay = true;
	//     x.Lookup(lib, "x", okay);
	//     y.Lookup(lib, "y", okay);
	//     z.Lookup(lib, "z", okay);
	//     if (okay) { printf("All functions were loaded successfully!\n"); }
	// If you supply a fallback, it'll be used if the lookup fails (and if
	//  non-NULL, means this will always return (okay)).
	bool Lookup(const char *libname, const char *fn, bool &okay, FunctionType fallback=NULL)
	{
		if (!okay)
			return false;
		else if (m_pFn == NULL)
			m_pFn = (FunctionType) VoidFnPtrLookup_Tier0(libname, fn, (void *) fallback);
		okay = m_pFn != NULL;
		return okay;
	}

	// Load library if necessary, look up symbol. Returns true and sets
	//  m_pFn on successful lookup, returns false otherwise. If the
	//  function pointer is already looked up, this return true immediately.
	// Use Reset() first if you want to look up the symbol again.
	//  This function will return false immediately unless (okay) is true.
	// If you supply a fallback, it'll be used if the lookup fails (and if
	//  non-NULL, means this will always return true).
	bool Lookup(const char *libname, const char *fn, FunctionType fallback=NULL)
	{
		bool okay = true;
		return Lookup(libname, fn, okay, fallback);
	}

	// Invalidates the current lookup. Makes the function pointer NULL. You
	//  will need to call Lookup() before you can call a dynamic function
	//  through this interface again.
	void Reset() { m_pFn = NULL; }

	// Force this to be a specific function pointer.
	void Force(FunctionType ptr) { m_pFn = ptr; }

	// Retrieve the actual function pointer.
	FunctionType Pointer() const { return m_pFn; }
	operator FunctionType() const { return m_pFn; }

	// Can be used to verify that we have an actual function looked up and
	//  ready to call: if (!MyDynFunc) { printf("Function not found!\n"); }
	operator bool () const { return m_pFn != NULL; }
	bool operator !() const { return m_pFn == NULL; }

protected:
	FunctionType m_pFn;
};


// This is the same as CDynamicFunction, but we made the default constructor
//  private, forcing you to do loading/lookup during construction.
// The usage pattern is to have a list of dynamic functions that are
//  constructed en masse as part of another class's constructor, with the
//  possibility of human error removed (the compiler will complain if you
//  forget to initialize one).
template < class FunctionType >
class CDynamicFunctionMustInit : public CDynamicFunction < FunctionType >
{
private:  // forbid default constructor.
	CDynamicFunctionMustInit() {}

public:
	CDynamicFunctionMustInit(const char *libname, const char *fn, FunctionType fallback=NULL)
	    : CDynamicFunction< FunctionType >(libname, fn, fallback)
	{
	}

	CDynamicFunctionMustInit(const char *libname, const char *fn, bool &okay, FunctionType fallback=NULL)
	    : CDynamicFunction< FunctionType >(libname, fn, okay, fallback)
	{
	}
};

#endif  // DYNFUNCTION_H

